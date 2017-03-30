# MPL
Minimum Portable Language

This is a retargetable back-end of a machine language translator.

##Minimum Portable Language
Minimum Portable Language is a type of machine independent assembler language.

mpl.l: Lexer for the Minimum Portable Language

##Portable Program Data
This is the internal data structure that is directly representative of the Minimum Portable Language.

Portable Program Data must contain all the necessary information of a machine independent assembly language. It will have to record labels and sequences of instructions acting on them. It will have to keep track of the width of each data field. It will have to keep track of static memory labels as well as stack offsets for automatic allocations. Just as an assembly language PPD is typeless.

ppd.hpp: brings together classes defining the various parts of the Portable Program Data. will pull in string_array.hpp to contain the various labels. It will need some sort of instruction queue. It will need an indexed container for labels. It will need a data structure to contain the properties of each label.

ppd.cpp: Instantiates the portable program data

##Portable Executable
This is a serialization of the Portable Program Data that should allow software to be distributed in a partially compiled format that could be accepted by virtual machines or installation programs that finish the compilation.

##I need:
*	An internal data structure that holds Portable Program Data
*	A front end that lexes/parses Minimum Portable Language and converts it to Portable Program Data
*	A library interface that accepts Portable Program Data
*	A front end that accepts pexe and converts it into Portable Program Data
*	The code generators
