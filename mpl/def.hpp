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


#ifndef _DEF_HPP
#define _DEF_HPP

#include <util/types.h>

#include <vector>

/*
the code generator doesn't actually need to know anything about the structure of data. the only reason these exist is because the sizes are machine dependent. we use these to calculate the sizes of various offset symbols and that's it.
*/


class Definition{
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	/******************************* ACCESSOR *********************************/
	
	/******************************* MUTATORS *********************************/
	
};

typedef Definition * def_pt;


class Data: public Definition{
	size_t sz;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	/******************************* ACCESSOR *********************************/
	
	size_t sz(void)const{ return sz; };
	
	/******************************* MUTATORS *********************************/
	
};


/******************************************************************************/
//                            PRIMATIVE DATA TYPES
/******************************************************************************/


/// data sizes for numerical operands
typedef enum width_t{
	w_none,
	w_byte,
	w_byte2,
	w_byte4,
	w_byte8,
	w_max,
	w_word,
	w_ptr,
	w_NUM
} width_t;

typedef enum{
	pt_unsign,
	pt_signed
} prime_t;

class Primative: public Data{
	width_t w;
	prime_t sign;
	umax    value;
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Primative();
	
	/******************************* ACCESSOR *********************************/
	
	
	/******************************* MUTATORS *********************************/
	
	
};


/******************************************************************************/
//                                  ARRAY TYPES
/******************************************************************************/


class Array: public Data{
	sym_pt member; // COLLISION_CHAR step
	umax   count;
	
public:
	std::vector<uint8_t> value;
	
	/****************************** CONSTRUCTOR *******************************/
	
	Array();
	
	/******************************* ACCESSOR *********************************/
	
	
	/******************************* MUTATORS *********************************/
	
	
};


/******************************************************************************/
//                               STRUCTURE TYPES
/******************************************************************************/


class Structure: public Data{
	
	
public:
	Obj_List members; // list of constant symbols
	
	/****************************** CONSTRUCTOR *******************************/
	
	Structure();
	
	/******************************* ACCESSOR *********************************/
	
	
	/******************************* MUTATORS *********************************/
	
	
};


/******************************************************************************/
//                               ROUTINE TYPES
/******************************************************************************/


class Routine: public Definition{
	DS blocks;
	uint concurrent_temps;
	
public:
	Structure params;
	Structure autos;
	
	/****************************** CONSTRUCTOR *******************************/
	
	Routine();
	
	/******************************* ACCESSOR *********************************/
	
	
	/******************************* MUTATORS *********************************/
	
	
};


#endif // _DEF_HPP


