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

#include <mpl/prime.hpp>
#include <util/msg.h>
#include <stdio.h>


Prime::Prime(void){
	width = w_none;
	sign = false;
	// const_value
}

/******************************* ACCESSOR *********************************/

const char * Prime::print_obj (void) const{
	static char   array[100];
	static const char * str_width[w_NUM]={
		"none   ",
		"byte   ",
		"2byte  ",
		"4byte  ",
		"8byte  ",
		"word   ",
		"max    ",
		"pointer"
	};
	
	sprintf(array, "%s %s %s %s",
		str_sclass[sclass],
		sign? "  signed":"unsigned",
		str_width[width],
		get_label()
	);
	
	return array;
}

/******************************* MUTATORS *********************************/

void Prime::set_width(width_t size){
	if(width == w_none) width = size;
	else{
		msg_print(NULL, V_ERROR, "Prime::set_width(): width already set");
		throw;
	}
}

void Prime::set_signed(void){
	if(!sign) sign = true;
	else{
		msg_print(NULL, V_ERROR, "Prime::set_signed(): sign already set");
		throw;
	}
}

void Prime::set_init(umax val){
	if(sclass == sc_stack || sclass == sc_data) value = val;
	else{
		msg_print(NULL, V_ERROR,
			"Prime::set_constant(): object cannot be initialized",
			get_label()
		);
		throw;
	}
}

void Prime::set_constant(umax val){
	if(sclass == sc_const) value = val;
	else{
		msg_print(NULL, V_ERROR,
			"Prime::set_constant(): object is not a constant",
			get_label()
		);
		throw;
	}
}


