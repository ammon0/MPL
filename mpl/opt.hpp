/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *	You should have received a copy of the license terms with this software. If
 *	not, please visit the project homepage at:
 *	https://github.com/ammon0/MPL
 *
 ******************************************************************************/

/**	@file opt.hpp
 *	
 *	Code optimization routines
 */


#ifndef _OPT_HPP
#define _OPT_HPP


#include <mpl/ppd.hpp>

/** Removes dead instructions and operands.
 */
void opt_dead(PPD * prog_data);

/** Propagates constants.
 */
void opt_const(PPD * prog_data);


#endif // _OPT_HPP


