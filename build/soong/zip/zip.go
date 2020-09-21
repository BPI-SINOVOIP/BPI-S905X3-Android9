// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package zip

import (
	"bytes"
	"compress/flate"
	"errors"
	"fmt"
	"hash/crc32"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"runtime/pprof"
	"runtime/trace"
	"sort"
	"strings"
	"sync"
	"time"
	"unicode"

	"github.com/google/blueprint/pathtools"

	"android/soong/jar"
	"android/soong/third_party/zip"
)

// Block size used during parallel compression of a single file.
const parallelBlockSize = 1 * 1024 * 1024 // 1MB

// Minimum file size to use parallel compression. It requires more
// flate.Writer allocations, since we can't change the dictionary
// during Reset
const minParallelFileSize = parallelBlockSize * 6

// Size of the ZIP compression window (32KB)
const windowSize = 32 * 1024

type nopCloser struct {
	io.Writer
}

func (nopCloser) Close() error {
	return nil
}

type byteReaderCloser struct {
	*bytes.Reader
	io.Closer
}

type pathMapping struct {
	dest, src string
	zipMethod uint16
}

type uniqueSet map[string]bool

func (u *uniqueSet) String() string {
	return `""`
}

func (u *uniqueSet) Set(s string) error {
	if _, found := (*u)[s]; found {
		return fmt.Errorf("File %q was specified twice as a file to not deflate", s)
	} else {
		(*u)[s] = true
	}

	return nil
}

type FileArg struct {
	PathPrefixInZip, SourcePrefixToStrip string
	SourceFiles                          []string
	GlobDir                              string
}

type FileArgs []FileArg

type ZipWriter struct {
	time         time.Time
	createdFiles map[string]string
	createdDirs  map[string]string
	directories  bool

	errors   chan error
	writeOps chan chan *zipEntry

	cpuRateLimiter    *CPURateLimiter
	memoryRateLimiter *MemoryRateLimiter

	compressorPool sync.Pool
	compLevel      int
}

type zipEntry struct {
	fh *zip.FileHeader

	// List of delayed io.Reader
	futureReaders chan chan io.Reader

	// Only used for passing into the MemoryRateLimiter to ensure we
	// release as much memory as much as we request
	allocatedSize int64
}

type ZipArgs struct {
	FileArgs                 FileArgs
	OutputFilePath           string
	CpuProfileFilePath       string
	TraceFilePath            string
	EmulateJar               bool
	AddDirectoryEntriesToZip bool
	CompressionLevel         int
	ManifestSourcePath       string
	NumParallelJobs          int
	NonDeflatedFiles         map[string]bool
	WriteIfChanged           bool
}

const NOQUOTE = '\x00'

func ReadRespFile(bytes []byte) []string {
	var args []string
	var arg []rune

	isEscaping := false
	quotingStart := NOQUOTE
	for _, c := range string(bytes) {
		switch {
		case isEscaping:
			if quotingStart == '"' {
				if !(c == '"' || c == '\\') {
					// '\"' or '\\' will be escaped under double quoting.
					arg = append(arg, '\\')
				}
			}
			arg = append(arg, c)
			isEscaping = false
		case c == '\\' && quotingStart != '\'':
			isEscaping = true
		case quotingStart == NOQUOTE && (c == '\'' || c == '"'):
			quotingStart = c
		case quotingStart != NOQUOTE && c == quotingStart:
			quotingStart = NOQUOTE
		case quotingStart == NOQUOTE && unicode.IsSpace(c):
			// Current character is a space outside quotes
			if len(arg) != 0 {
				args = append(args, string(arg))
			}
			arg = arg[:0]
		default:
			arg = append(arg, c)
		}
	}

	if len(arg) != 0 {
		args = append(args, string(arg))
	}

	return args
}

func Run(args ZipArgs) (err error) {
	if args.CpuProfileFilePath != "" {
		f, err := os.Create(args.CpuProfileFilePath)
		if err != nil {
			fmt.Fprintln(os.Stderr, err.Error())
			os.Exit(1)
		}
		defer f.Close()
		pprof.StartCPUProfile(f)
		defer pprof.StopCPUProfile()
	}

	if args.TraceFilePath != "" {
		f, err := os.Create(args.TraceFilePath)
		if err != nil {
			fmt.Fprintln(os.Stderr, err.Error())
			os.Exit(1)
		}
		defer f.Close()
		err = trace.Start(f)
		if err != nil {
			fmt.Fprintln(os.Stderr, err.Error())
			os.Exit(1)
		}
		defer trace.Stop()
	}

	if args.OutputFilePath == "" {
		return fmt.Errorf("output file path must be nonempty")
	}

	if args.EmulateJar {
		args.AddDirectoryEntriesToZip = true
	}

	w := &ZipWriter{
		time:         jar.DefaultTime,
		createdDirs:  make(map[string]string),
		createdFiles: make(map[string]string),
		directories:  args.AddDirectoryEntriesToZip,
		compLevel:    args.CompressionLevel,
	}
	pathMappings := []pathMapping{}

	for _, fa := range args.FileArgs {
		srcs := fa.SourceFiles
		if fa.GlobDir != "" {
			srcs = append(srcs, recursiveGlobFiles(fa.GlobDir)...)
		}
		for _, src := range srcs {
			if err := fillPathPairs(fa.PathPrefixInZip,
				fa.SourcePrefixToStrip, src, &pathMappings, args.NonDeflatedFiles); err != nil {
				log.Fatal(err)
			}
		}
	}

	buf := &bytes.Buffer{}
	var out io.Writer = buf

	if !args.WriteIfChanged {
		f, err := os.Create(args.OutputFilePath)
		if err != nil {
			return err
		}

		defer f.Close()
		defer func() {
			if err != nil {
				os.Remove(args.OutputFilePath)
			}
		}()

		out = f
	}

	err = w.write(out, pathMappings, args.ManifestSourcePath, args.EmulateJar, args.NumParallelJobs)
	if err != nil {
		return err
	}

	if args.WriteIfChanged {
		err := pathtools.WriteFileIfChanged(args.OutputFilePath, buf.Bytes(), 0666)
		if err != nil {
			return err
		}
	}

	return nil
}

func fillPathPairs(prefix, rel, src string, pathMappings *[]pathMapping, nonDeflatedFiles map[string]bool) error {
	src = strings.TrimSpace(src)
	if src == "" {
		return nil
	}
	src = filepath.Clean(src)
	dest, err := filepath.Rel(rel, src)
	if err != nil {
		return err
	}
	dest = filepath.Join(prefix, dest)

	zipMethod := zip.Deflate
	if _, found := nonDeflatedFiles[dest]; found {
		zipMethod = zip.Store
	}
	*pathMappings = append(*pathMappings,
		pathMapping{dest: dest, src: src, zipMethod: zipMethod})

	return nil
}

func jarSort(mappings []pathMapping) {
	less := func(i int, j int) (smaller bool) {
		return jar.EntryNamesLess(mappings[i].dest, mappings[j].dest)
	}
	sort.SliceStable(mappings, less)
}

type readerSeekerCloser interface {
	io.Reader
	io.ReaderAt
	io.Closer
	io.Seeker
}

func (z *ZipWriter) write(f io.Writer, pathMappings []pathMapping, manifest string, emulateJar bool, parallelJobs int) error {
	z.errors = make(chan error)
	defer close(z.errors)

	// This channel size can be essentially unlimited -- it's used as a fifo
	// queue decouple the CPU and IO loads. Directories don't require any
	// compression time, but still cost some IO. Similar with small files that
	// can be very fast to compress. Some files that are more difficult to
	// compress won't take a corresponding longer time writing out.
	//
	// The optimum size here depends on your CPU and IO characteristics, and
	// the the layout of your zip file. 1000 was chosen mostly at random as
	// something that worked reasonably well for a test file.
	//
	// The RateLimit object will put the upper bounds on the number of
	// parallel compressions and outstanding buffers.
	z.writeOps = make(chan chan *zipEntry, 1000)
	z.cpuRateLimiter = NewCPURateLimiter(int64(parallelJobs))
	z.memoryRateLimiter = NewMemoryRateLimiter(0)
	defer func() {
		z.cpuRateLimiter.Stop()
		z.memoryRateLimiter.Stop()
	}()

	if manifest != "" && !emulateJar {
		return errors.New("must specify --jar when specifying a manifest via -m")
	}

	if emulateJar {
		// manifest may be empty, in which case addManifest will fill in a default
		pathMappings = append(pathMappings, pathMapping{jar.ManifestFile, manifest, zip.Deflate})

		jarSort(pathMappings)
	}

	go func() {
		var err error
		defer close(z.writeOps)

		for _, ele := range pathMappings {
			if emulateJar && ele.dest == jar.ManifestFile {
				err = z.addManifest(ele.dest, ele.src, ele.zipMethod)
			} else {
				err = z.addFile(ele.dest, ele.src, ele.zipMethod, emulateJar)
			}
			if err != nil {
				z.errors <- err
				return
			}
		}
	}()

	zipw := zip.NewWriter(f)

	var currentWriteOpChan chan *zipEntry
	var currentWriter io.WriteCloser
	var currentReaders chan chan io.Reader
	var currentReader chan io.Reader
	var done bool

	for !done {
		var writeOpsChan chan chan *zipEntry
		var writeOpChan chan *zipEntry
		var readersChan chan chan io.Reader

		if currentReader != nil {
			// Only read and process errors
		} else if currentReaders != nil {
			readersChan = currentReaders
		} else if currentWriteOpChan != nil {
			writeOpChan = currentWriteOpChan
		} else {
			writeOpsChan = z.writeOps
		}

		select {
		case writeOp, ok := <-writeOpsChan:
			if !ok {
				done = true
			}

			currentWriteOpChan = writeOp

		case op := <-writeOpChan:
			currentWriteOpChan = nil

			var err error
			if op.fh.Method == zip.Deflate {
				currentWriter, err = zipw.CreateCompressedHeader(op.fh)
			} else {
				var zw io.Writer

				op.fh.CompressedSize64 = op.fh.UncompressedSize64

				zw, err = zipw.CreateHeaderAndroid(op.fh)
				currentWriter = nopCloser{zw}
			}
			if err != nil {
				return err
			}

			currentReaders = op.futureReaders
			if op.futureReaders == nil {
				currentWriter.Close()
				currentWriter = nil
			}
			z.memoryRateLimiter.Finish(op.allocatedSize)

		case futureReader, ok := <-readersChan:
			if !ok {
				// Done with reading
				currentWriter.Close()
				currentWriter = nil
				currentReaders = nil
			}

			currentReader = futureReader

		case reader := <-currentReader:
			_, err := io.Copy(currentWriter, reader)
			if err != nil {
				return err
			}

			currentReader = nil

		case err := <-z.errors:
			return err
		}
	}

	// One last chance to catch an error
	select {
	case err := <-z.errors:
		return err
	default:
		zipw.Close()
		return nil
	}
}

// imports (possibly with compression) <src> into the zip at sub-path <dest>
func (z *ZipWriter) addFile(dest, src string, method uint16, emulateJar bool) error {
	var fileSize int64
	var executable bool

	if s, err := os.Lstat(src); err != nil {
		return err
	} else if s.IsDir() {
		if z.directories {
			return z.writeDirectory(dest, src, emulateJar)
		}
		return nil
	} else {
		if err := z.writeDirectory(filepath.Dir(dest), src, emulateJar); err != nil {
			return err
		}

		if prev, exists := z.createdDirs[dest]; exists {
			return fmt.Errorf("destination %q is both a directory %q and a file %q", dest, prev, src)
		}
		if prev, exists := z.createdFiles[dest]; exists {
			return fmt.Errorf("destination %q has two files %q and %q", dest, prev, src)
		}

		z.createdFiles[dest] = src

		if s.Mode()&os.ModeSymlink != 0 {
			return z.writeSymlink(dest, src)
		} else if !s.Mode().IsRegular() {
			return fmt.Errorf("%s is not a file, directory, or symlink", src)
		}

		fileSize = s.Size()
		executable = s.Mode()&0100 != 0
	}

	r, err := os.Open(src)
	if err != nil {
		return err
	}

	header := &zip.FileHeader{
		Name:               dest,
		Method:             method,
		UncompressedSize64: uint64(fileSize),
	}

	if executable {
		header.SetMode(0700)
	}

	return z.writeFileContents(header, r)
}

func (z *ZipWriter) addManifest(dest string, src string, method uint16) error {
	if prev, exists := z.createdDirs[dest]; exists {
		return fmt.Errorf("destination %q is both a directory %q and a file %q", dest, prev, src)
	}
	if prev, exists := z.createdFiles[dest]; exists {
		return fmt.Errorf("destination %q has two files %q and %q", dest, prev, src)
	}

	if err := z.writeDirectory(filepath.Dir(dest), src, true); err != nil {
		return err
	}

	fh, buf, err := jar.ManifestFileContents(src)
	if err != nil {
		return err
	}

	reader := &byteReaderCloser{bytes.NewReader(buf), ioutil.NopCloser(nil)}

	return z.writeFileContents(fh, reader)
}

func (z *ZipWriter) writeFileContents(header *zip.FileHeader, r readerSeekerCloser) (err error) {

	header.SetModTime(z.time)

	compressChan := make(chan *zipEntry, 1)
	z.writeOps <- compressChan

	// Pre-fill a zipEntry, it will be sent in the compressChan once
	// we're sure about the Method and CRC.
	ze := &zipEntry{
		fh: header,
	}

	ze.allocatedSize = int64(header.UncompressedSize64)
	z.cpuRateLimiter.Request()
	z.memoryRateLimiter.Request(ze.allocatedSize)

	fileSize := int64(header.UncompressedSize64)
	if fileSize == 0 {
		fileSize = int64(header.UncompressedSize)
	}

	if header.Method == zip.Deflate && fileSize >= minParallelFileSize {
		wg := new(sync.WaitGroup)

		// Allocate enough buffer to hold all readers. We'll limit
		// this based on actual buffer sizes in RateLimit.
		ze.futureReaders = make(chan chan io.Reader, (fileSize/parallelBlockSize)+1)

		// Calculate the CRC in the background, since reading the entire
		// file could take a while.
		//
		// We could split this up into chunks as well, but it's faster
		// than the compression. Due to the Go Zip API, we also need to
		// know the result before we can begin writing the compressed
		// data out to the zipfile.
		wg.Add(1)
		go z.crcFile(r, ze, compressChan, wg)

		for start := int64(0); start < fileSize; start += parallelBlockSize {
			sr := io.NewSectionReader(r, start, parallelBlockSize)
			resultChan := make(chan io.Reader, 1)
			ze.futureReaders <- resultChan

			z.cpuRateLimiter.Request()

			last := !(start+parallelBlockSize < fileSize)
			var dict []byte
			if start >= windowSize {
				dict, err = ioutil.ReadAll(io.NewSectionReader(r, start-windowSize, windowSize))
				if err != nil {
					return err
				}
			}

			wg.Add(1)
			go z.compressPartialFile(sr, dict, last, resultChan, wg)
		}

		close(ze.futureReaders)

		// Close the file handle after all readers are done
		go func(wg *sync.WaitGroup, closer io.Closer) {
			wg.Wait()
			closer.Close()
		}(wg, r)
	} else {
		go func() {
			z.compressWholeFile(ze, r, compressChan)
			r.Close()
		}()
	}

	return nil
}

func (z *ZipWriter) crcFile(r io.Reader, ze *zipEntry, resultChan chan *zipEntry, wg *sync.WaitGroup) {
	defer wg.Done()
	defer z.cpuRateLimiter.Finish()

	crc := crc32.NewIEEE()
	_, err := io.Copy(crc, r)
	if err != nil {
		z.errors <- err
		return
	}

	ze.fh.CRC32 = crc.Sum32()
	resultChan <- ze
	close(resultChan)
}

func (z *ZipWriter) compressPartialFile(r io.Reader, dict []byte, last bool, resultChan chan io.Reader, wg *sync.WaitGroup) {
	defer wg.Done()

	result, err := z.compressBlock(r, dict, last)
	if err != nil {
		z.errors <- err
		return
	}

	z.cpuRateLimiter.Finish()

	resultChan <- result
}

func (z *ZipWriter) compressBlock(r io.Reader, dict []byte, last bool) (*bytes.Buffer, error) {
	buf := new(bytes.Buffer)
	var fw *flate.Writer
	var err error
	if len(dict) > 0 {
		// There's no way to Reset a Writer with a new dictionary, so
		// don't use the Pool
		fw, err = flate.NewWriterDict(buf, z.compLevel, dict)
	} else {
		var ok bool
		if fw, ok = z.compressorPool.Get().(*flate.Writer); ok {
			fw.Reset(buf)
		} else {
			fw, err = flate.NewWriter(buf, z.compLevel)
		}
		defer z.compressorPool.Put(fw)
	}
	if err != nil {
		return nil, err
	}

	_, err = io.Copy(fw, r)
	if err != nil {
		return nil, err
	}
	if last {
		fw.Close()
	} else {
		fw.Flush()
	}

	return buf, nil
}

func (z *ZipWriter) compressWholeFile(ze *zipEntry, r io.ReadSeeker, compressChan chan *zipEntry) {

	crc := crc32.NewIEEE()
	_, err := io.Copy(crc, r)
	if err != nil {
		z.errors <- err
		return
	}

	ze.fh.CRC32 = crc.Sum32()

	_, err = r.Seek(0, 0)
	if err != nil {
		z.errors <- err
		return
	}

	readFile := func(reader io.ReadSeeker) ([]byte, error) {
		_, err := reader.Seek(0, 0)
		if err != nil {
			return nil, err
		}

		buf, err := ioutil.ReadAll(reader)
		if err != nil {
			return nil, err
		}

		return buf, nil
	}

	ze.futureReaders = make(chan chan io.Reader, 1)
	futureReader := make(chan io.Reader, 1)
	ze.futureReaders <- futureReader
	close(ze.futureReaders)

	if ze.fh.Method == zip.Deflate {
		compressed, err := z.compressBlock(r, nil, true)
		if err != nil {
			z.errors <- err
			return
		}
		if uint64(compressed.Len()) < ze.fh.UncompressedSize64 {
			futureReader <- compressed
		} else {
			buf, err := readFile(r)
			if err != nil {
				z.errors <- err
				return
			}
			ze.fh.Method = zip.Store
			futureReader <- bytes.NewReader(buf)
		}
	} else {
		buf, err := readFile(r)
		if err != nil {
			z.errors <- err
			return
		}
		ze.fh.Method = zip.Store
		futureReader <- bytes.NewReader(buf)
	}

	z.cpuRateLimiter.Finish()

	close(futureReader)

	compressChan <- ze
	close(compressChan)
}

// writeDirectory annotates that dir is a directory created for the src file or directory, and adds
// the directory entry to the zip file if directories are enabled.
func (z *ZipWriter) writeDirectory(dir string, src string, emulateJar bool) error {
	// clean the input
	dir = filepath.Clean(dir)

	// discover any uncreated directories in the path
	zipDirs := []string{}
	for dir != "" && dir != "." {
		if _, exists := z.createdDirs[dir]; exists {
			break
		}

		if prev, exists := z.createdFiles[dir]; exists {
			return fmt.Errorf("destination %q is both a directory %q and a file %q", dir, src, prev)
		}

		z.createdDirs[dir] = src
		// parent directories precede their children
		zipDirs = append([]string{dir}, zipDirs...)

		dir = filepath.Dir(dir)
	}

	if z.directories {
		// make a directory entry for each uncreated directory
		for _, cleanDir := range zipDirs {
			var dirHeader *zip.FileHeader

			if emulateJar && cleanDir+"/" == jar.MetaDir {
				dirHeader = jar.MetaDirFileHeader()
			} else {
				dirHeader = &zip.FileHeader{
					Name: cleanDir + "/",
				}
				dirHeader.SetMode(0700 | os.ModeDir)
			}

			dirHeader.SetModTime(z.time)

			ze := make(chan *zipEntry, 1)
			ze <- &zipEntry{
				fh: dirHeader,
			}
			close(ze)
			z.writeOps <- ze
		}
	}

	return nil
}

func (z *ZipWriter) writeSymlink(rel, file string) error {
	fileHeader := &zip.FileHeader{
		Name: rel,
	}
	fileHeader.SetModTime(z.time)
	fileHeader.SetMode(0700 | os.ModeSymlink)

	dest, err := os.Readlink(file)
	if err != nil {
		return err
	}

	ze := make(chan *zipEntry, 1)
	futureReaders := make(chan chan io.Reader, 1)
	futureReader := make(chan io.Reader, 1)
	futureReaders <- futureReader
	close(futureReaders)
	futureReader <- bytes.NewBufferString(dest)
	close(futureReader)

	ze <- &zipEntry{
		fh:            fileHeader,
		futureReaders: futureReaders,
	}
	close(ze)
	z.writeOps <- ze

	return nil
}

func recursiveGlobFiles(path string) []string {
	var files []string
	filepath.Walk(path, func(path string, info os.FileInfo, err error) error {
		if !info.IsDir() {
			files = append(files, path)
		}
		return nil
	})

	return files
}
