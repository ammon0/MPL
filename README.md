# MPL : Minimum Portable Language

## Project Goal
This is a retargetable back-end for a machine language translator. The goal here is to create the portable program data format as an intermediate language. Then create one or more machine language generators to translate it for specific hardware. I'm specifically interested in x86 and arm as my back end targets.

## Project Components
*	PPD: Portable Program Data would be a machine independent format for programs. It is currently made up of an index of generic objects that can represent register data, arrays, records, or program routines. Instructions are stored in the "quad" format. This should facilitate optimization in the future.
*	MPL: Minimum Portable Language would be a text format that closely resembles the PPD's internal format. It should be fairly easy to "dump" a PPD object into an MPL file and then parse it back into PPD.
*	PEXE: Portable Executable would be a binary serialization of PPD allowing partially compiled machine independent programs to be shared and distributed. A PEXE file forms PPD when it is read into memory. It should then be easy to either execute the PPD directly in a virtual machine, or complete its compilation to native machine code. PEXE files should have a built-in file signing method to ensure validity.
*	OID: An Object Interface Description would be a binary file that should be produced when any PPD that doesn't contain a `start` routine is compiled to native code. the OID file contains the necessary information to link against the compiled library.
*	gen-XXX: code generators that convert PPD into various serialized formats. I am currently developing gen-x86 that will convert PPD into NASM assembly code. there are place holders for gen-arm, and gen-pexe.
*	opt-XXX: PPD optimizers read through the quad instructions and apply transformations that should improve size and speed. opt-dead is intended to remove dead code and temp variables although it will have to be reworked since there has been extensive modifications to PPD.

## Project Pages
*	[Latest Release](https://github.com/ammon0/MPL/releases/latest)
*	[Documentation](https://ammon0.github.io/MPL/)
*	[github](https://github.com/ammon0/MPL)

## MIT License
Copyright (c) 2016-2017 Ammon Dodson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
