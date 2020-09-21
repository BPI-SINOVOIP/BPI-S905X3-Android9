// Copyright 2018 Google Inc. All rights reserved.
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

package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"math"
	"os"
)

var (
	input  = flag.String("i", "", "input file")
	output = flag.String("o", "", "output file")
	symbol = flag.String("s", "", "symbol to inject into")
	from   = flag.String("from", "", "optional existing value of the symbol for verification")
	value  = flag.String("v", "", "value to inject into symbol")

	dump = flag.Bool("dump", false, "dump the symbol table for copying into a test")
)

var maxUint64 uint64 = math.MaxUint64

type cantParseError struct {
	error
}

func main() {
	flag.Parse()

	usageError := func(s string) {
		fmt.Fprintln(os.Stderr, s)
		flag.Usage()
		os.Exit(1)
	}

	if *input == "" {
		usageError("-i is required")
	}

	if !*dump {
		if *output == "" {
			usageError("-o is required")
		}

		if *symbol == "" {
			usageError("-s is required")
		}

		if *value == "" {
			usageError("-v is required")
		}
	}

	r, err := os.Open(*input)
	if err != nil {
		fmt.Fprintln(os.Stderr, err.Error())
		os.Exit(2)
	}
	defer r.Close()

	if *dump {
		err := dumpSymbols(r)
		if err != nil {
			fmt.Fprintln(os.Stderr, err.Error())
			os.Exit(6)
		}
		return
	}

	w, err := os.OpenFile(*output, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0777)
	if err != nil {
		fmt.Fprintln(os.Stderr, err.Error())
		os.Exit(3)
	}
	defer w.Close()

	file, err := openFile(r)
	if err != nil {
		fmt.Fprintln(os.Stderr, err.Error())
		os.Exit(4)
	}

	err = injectSymbol(file, w, *symbol, *value, *from)
	if err != nil {
		fmt.Fprintln(os.Stderr, err.Error())
		os.Remove(*output)
		os.Exit(5)
	}
}

func openFile(r io.ReaderAt) (*File, error) {
	file, err := elfSymbolsFromFile(r)
	if elfError, ok := err.(cantParseError); ok {
		// Try as a mach-o file
		file, err = machoSymbolsFromFile(r)
		if _, ok := err.(cantParseError); ok {
			// Try as a windows PE file
			file, err = peSymbolsFromFile(r)
			if _, ok := err.(cantParseError); ok {
				// Can't parse as elf, macho, or PE, return the elf error
				return nil, elfError
			}
		}
	}
	if err != nil {
		return nil, err
	}

	file.r = r

	return file, err
}

func injectSymbol(file *File, w io.Writer, symbol, value, from string) error {
	offset, size, err := findSymbol(file, symbol)
	if err != nil {
		return err
	}

	if uint64(len(value))+1 > size {
		return fmt.Errorf("value length %d overflows symbol size %d", len(value), size)
	}

	if from != "" {
		// Read the exsting symbol contents and verify they match the expected value
		expected := make([]byte, size)
		existing := make([]byte, size)
		copy(expected, from)
		_, err := file.r.ReadAt(existing, int64(offset))
		if err != nil {
			return err
		}
		if bytes.Compare(existing, expected) != 0 {
			return fmt.Errorf("existing symbol contents %q did not match expected value %q",
				string(existing), string(expected))
		}
	}

	return copyAndInject(file.r, w, offset, size, value)
}

func copyAndInject(r io.ReaderAt, w io.Writer, offset, size uint64, value string) (err error) {
	buf := make([]byte, size)
	copy(buf, value)

	// Copy the first bytes up to the symbol offset
	_, err = io.Copy(w, io.NewSectionReader(r, 0, int64(offset)))

	// Write the injected value in the output file
	if err == nil {
		_, err = w.Write(buf)
	}

	// Write the remainder of the file
	pos := int64(offset + size)
	if err == nil {
		_, err = io.Copy(w, io.NewSectionReader(r, pos, 1<<63-1-pos))
	}

	if err == io.EOF {
		err = io.ErrUnexpectedEOF
	}

	return err
}

func findSymbol(file *File, symbolName string) (uint64, uint64, error) {
	for i, symbol := range file.Symbols {
		if symbol.Name == symbolName {
			// Find the next symbol (n the same section with a higher address
			var n int
			for n = i; n < len(file.Symbols); n++ {
				if file.Symbols[n].Section != symbol.Section {
					n = len(file.Symbols)
					break
				}
				if file.Symbols[n].Addr > symbol.Addr {
					break
				}
			}

			size := symbol.Size
			if size == 0 {
				var end uint64
				if n < len(file.Symbols) {
					end = file.Symbols[n].Addr
				} else {
					end = symbol.Section.Size
				}

				if end <= symbol.Addr || end > symbol.Addr+4096 {
					return maxUint64, maxUint64, fmt.Errorf("symbol end address does not seem valid, %x:%x", symbol.Addr, end)
				}

				size = end - symbol.Addr
			}

			offset := symbol.Section.Offset + symbol.Addr

			return uint64(offset), uint64(size), nil
		}
	}

	return maxUint64, maxUint64, fmt.Errorf("symbol not found")
}

type File struct {
	r        io.ReaderAt
	Symbols  []*Symbol
	Sections []*Section
}

type Symbol struct {
	Name    string
	Addr    uint64 // Address of the symbol inside the section.
	Size    uint64 // Size of the symbol, if known.
	Section *Section
}

type Section struct {
	Name   string
	Addr   uint64 // Virtual address of the start of the section.
	Offset uint64 // Offset into the file of the start of the section.
	Size   uint64
}

func dumpSymbols(r io.ReaderAt) error {
	err := dumpElfSymbols(r)
	if elfError, ok := err.(cantParseError); ok {
		// Try as a mach-o file
		err = dumpMachoSymbols(r)
		if _, ok := err.(cantParseError); ok {
			// Try as a windows PE file
			err = dumpPESymbols(r)
			if _, ok := err.(cantParseError); ok {
				// Can't parse as elf, macho, or PE, return the elf error
				return elfError
			}
		}
	}
	return err
}
