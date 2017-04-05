/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *	Copyright (c) 2017 Ammon Dodson
 *	You should have received a copy of the licence terms with this software. If
 *	not, please visit the project homepage at:
 *	https://github.com/ammon0/MPL
 *
 ******************************************************************************/

/**	@file ppd.cpp
 *	
 *	Bring all the Portable Program Data together.
 */


#include "instructions.hpp"

/** This is a container for all the components of the portable program data.
*/
class PPD{
public:
	String_Array labels;       ///< the string space
	Operands     operands;     ///< The symbol table
	Instructions instructions; ///< The program code
};


