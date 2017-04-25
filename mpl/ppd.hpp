/*******************************************************************************
 *
 *	MPL : Minimum Portable Language
 *
 *	Copyright (c) 2017 Ammon Dodson
 *	You should have received a copy of the licence terms with this software. If
 *	not, please visit the project homepage at:
 *	https://github.com/ammon0/MPL
 *
 ******************************************************************************/

/**	@file ppd.hpp
 *	
 *	Bring all the Portable Program Data together.
 */


#ifndef _PPD_HPP
#define _PPD_HPP


#include <mpl/obj_container.hpp>


/** This is a container for all the components of the portable program data.
*/
class PPD{
	// Optimizations applied
	bool dead;      ///< indicates whether this PPD has passed opt_dead
	bool constants; ///< indicates whether this PPD has passed opt_const
	
public:
	Obj_Index objects;
	
	/****************************** CONSTRUCTOR *******************************/
	
	PPD(void){ dead = constants = false; }
	
	/******************************* ACCESSOR *********************************/
	
	/******************************* MUTATORS *********************************/
	
};


#endif // _PPD_HPP


