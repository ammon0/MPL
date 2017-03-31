# MPL : Minimum Portable Language

This is a retargetable back-end for a machine language translator. The goal here is to create the portable program data format as a common intermediate language for compilers. Then we will create one or more machine language generators to translate it for specific hardware.

## Project Pages
*	[Latest Release](https://github.com/ammon0/MPL/releases/latest)
*	[Documentation](https://ammon0.github.io/MPL/)
*	[github](https://github.com/ammon0/MPL)

## Minimum Portable Language
Minimum Portable Language is a type of machine independent assembler language.

mpl.l: Lexer for the Minimum Portable Language

## Portable Program Data
This is the internal data structure that is directly representative of the Minimum Portable Language.

Portable Program Data must contain all the necessary information of a machine independent assembly language. It will have to record labels and sequences of instructions acting on them. It will have to keep track of the width of each data field. It will have to keep track of static memory labels as well as stack offsets for automatic allocations. Just as an assembly language PPD is typeless.

ppd.hpp: brings together classes defining the various parts of the Portable Program Data. will pull in string_array.hpp to contain the various labels. It will need some sort of instruction queue. It will need an indexed container for labels. It will need a data structure to contain the properties of each label.

ppd.cpp: Instantiates the portable program data

## Portable Executable
This is a serialization of the Portable Program Data that should allow software to be distributed in a partially compiled format that could be accepted by virtual machines or installation programs that finish the compilation.

## I need:
*	An internal data structure that holds Portable Program Data
*	A front end that lexes/parses Minimum Portable Language and converts it to Portable Program Data
*	A library interface that accepts Portable Program Data
*	A front end that accepts pexe and converts it into Portable Program Data
*	The code generators

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
