// Copyright 2016 Google Inc. All rights reserved.
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
	"errors"
	"io"
)

const DataDescriptorFlag = 0x8
const ExtendedTimeStampTag = 0x5455

func (w *Writer) CopyFrom(orig *File, newName string) error {
	if w.last != nil && !w.last.closed {
		if err := w.last.close(); err != nil {
			return err
		}
		w.last = nil
	}

	fileHeader := orig.FileHeader
	fileHeader.Name = newName
	fh := &fileHeader

	// In some cases, we need strip the extras if it change between Central Directory
	// and Local File Header.
	fh.Extra = stripExtras(fh.Extra)

	h := &header{
		FileHeader: fh,
		offset:     uint64(w.cw.count),
	}
	w.dir = append(w.dir, h)

	if err := writeHeader(w.cw, fh); err != nil {
		return err
	}
	dataOffset, err := orig.DataOffset()
	if err != nil {
		return err
	}
	io.Copy(w.cw, io.NewSectionReader(orig.zipr, dataOffset, int64(orig.CompressedSize64)))

	if orig.hasDataDescriptor() {
		// Write data descriptor.
		var buf []byte
		if fh.isZip64() {
			buf = make([]byte, dataDescriptor64Len)
		} else {
			buf = make([]byte, dataDescriptorLen)
		}
		b := writeBuf(buf)
		b.uint32(dataDescriptorSignature)
		b.uint32(fh.CRC32)
		if fh.isZip64() {
			b.uint64(fh.CompressedSize64)
			b.uint64(fh.UncompressedSize64)
		} else {
			b.uint32(fh.CompressedSize)
			b.uint32(fh.UncompressedSize)
		}
		_, err = w.cw.Write(buf)
	}
	return err
}

// The zip64 extras change between the Central Directory and Local File Header, while we use
// the same structure for both. The Local File Haeder is taken care of by us writing a data
// descriptor with the zip64 values. The Central Directory Entry is written by Close(), where
// the zip64 extra is automatically created and appended when necessary.
//
// The extended-timestamp extra block changes between the Central Directory Header and Local
// File Header.
// Extended-Timestamp extra(LFH): <tag-size-flag-modtime-actime-changetime>
// Extended-Timestamp extra(CDH): <tag-size-flag-modtime>
func stripExtras(input []byte) []byte {
	ret := []byte{}

	for len(input) >= 4 {
		r := readBuf(input)
		tag := r.uint16()
		size := r.uint16()
		if int(size) > len(r) {
			break
		}
		if tag != zip64ExtraId && tag != ExtendedTimeStampTag {
			ret = append(ret, input[:4+size]...)
		}
		input = input[4+size:]
	}

	// Keep any trailing data
	ret = append(ret, input...)

	return ret
}

// CreateCompressedHeader adds a file to the zip file using the provied
// FileHeader for the file metadata.
// It returns a Writer to which the already compressed file contents
// should be written.
//
// The UncompressedSize64 and CRC32 entries in the FileHeader must be filled
// out already.
//
// The file's contents must be written to the io.Writer before the next
// call to Create, CreateHeader, CreateCompressedHeader, or Close. The
// provided FileHeader fh must not be modified after a call to
// CreateCompressedHeader
func (w *Writer) CreateCompressedHeader(fh *FileHeader) (io.WriteCloser, error) {
	if w.last != nil && !w.last.closed {
		if err := w.last.close(); err != nil {
			return nil, err
		}
	}
	if len(w.dir) > 0 && w.dir[len(w.dir)-1].FileHeader == fh {
		// See https://golang.org/issue/11144 confusion.
		return nil, errors.New("archive/zip: invalid duplicate FileHeader")
	}

	fh.Flags |= DataDescriptorFlag // we will write a data descriptor

	fh.CreatorVersion = fh.CreatorVersion&0xff00 | zipVersion20 // preserve compatibility byte
	fh.ReaderVersion = zipVersion20

	fw := &compressedFileWriter{
		fileWriter{
			zipw:      w.cw,
			compCount: &countWriter{w: w.cw},
		},
	}

	h := &header{
		FileHeader: fh,
		offset:     uint64(w.cw.count),
	}
	w.dir = append(w.dir, h)
	fw.header = h

	if err := writeHeader(w.cw, fh); err != nil {
		return nil, err
	}

	w.last = &fw.fileWriter
	return fw, nil
}

// Updated version of CreateHeader that doesn't enforce writing a data descriptor
func (w *Writer) CreateHeaderAndroid(fh *FileHeader) (io.Writer, error) {
	writeDataDescriptor := fh.Method != Store
	if writeDataDescriptor {
		fh.Flags &= DataDescriptorFlag
	} else {
		fh.Flags &= ^uint16(DataDescriptorFlag)
	}
	return w.createHeaderImpl(fh)
}

type compressedFileWriter struct {
	fileWriter
}

func (w *compressedFileWriter) Write(p []byte) (int, error) {
	if w.closed {
		return 0, errors.New("zip: write to closed file")
	}
	return w.compCount.Write(p)
}

func (w *compressedFileWriter) Close() error {
	if w.closed {
		return errors.New("zip: file closed twice")
	}
	w.closed = true

	// update FileHeader
	fh := w.header.FileHeader
	fh.CompressedSize64 = uint64(w.compCount.count)

	if fh.isZip64() {
		fh.CompressedSize = uint32max
		fh.UncompressedSize = uint32max
		fh.ReaderVersion = zipVersion45 // requires 4.5 - File uses ZIP64 format extensions
	} else {
		fh.CompressedSize = uint32(fh.CompressedSize64)
		fh.UncompressedSize = uint32(fh.UncompressedSize64)
	}

	// Write data descriptor. This is more complicated than one would
	// think, see e.g. comments in zipfile.c:putextended() and
	// http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=7073588.
	// The approach here is to write 8 byte sizes if needed without
	// adding a zip64 extra in the local header (too late anyway).
	var buf []byte
	if fh.isZip64() {
		buf = make([]byte, dataDescriptor64Len)
	} else {
		buf = make([]byte, dataDescriptorLen)
	}
	b := writeBuf(buf)
	b.uint32(dataDescriptorSignature) // de-facto standard, required by OS X
	b.uint32(fh.CRC32)
	if fh.isZip64() {
		b.uint64(fh.CompressedSize64)
		b.uint64(fh.UncompressedSize64)
	} else {
		b.uint32(fh.CompressedSize)
		b.uint32(fh.UncompressedSize)
	}
	_, err := w.zipw.Write(buf)
	return err
}
