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


#include "x86.hpp"
#include <util/data.h>

typedef struct{
	lbl_pt l;
	reg_t r;
	int   offset;
}location_t;

static inline const void * lbl(const void * data){
	return ((location_t*)data)->l;
}
static inline imax lbl_cmp(const void * left, const void * right){
	return (umax)left-(umax)right;
}

/** Return the appropriate string to use the given x86 register. */
const char * str_reg(size_t width, reg_t reg){
	static char array[4] = "   ";

	if(mode != xm_long && width == QWORD){
		msg_print(NULL, V_ERROR,
			"Internal str_reg(): qword only available in long mode");
		return "!!Bad width!!";
	}
	
	if(mode != xm_long && reg > SP){
		msg_print(NULL, V_ERROR,
			"Internal str_reg(): R8-R15 only available in long mode");
		return "!!Bad register!!";
	}
	
	switch (width){
	case BYTE : array[0] = ' '; array[2] = ' '; break;
	case WORD : array[0] = ' '; array[2] = 'x'; break;
	case DWORD: array[0] = 'e'; array[2] = 'x'; break;
	case QWORD: array[0] = 'r'; array[2] = 'x'; break;
	
	default:
		msg_print(NULL, V_ERROR, "str_reg(): got a bad width");
		return NULL;
	}

	switch (reg){
	case A  : array[1] = 'a'; break;
	case B  : array[1] = 'b'; break;
	case C  : array[1] = 'c'; break;
	case D  : array[1] = 'd'; break;
	case SI : array[1] = 's'; array[2] = 'i'; break;
	case DI : array[1] = 'd'; array[2] = 'i'; break;
	case BP : array[1] = 'b'; array[2] = 'p'; break;
	case SP : array[1] = 's'; array[2] = 'p'; break;
	case R8 : array[1] = '8'; array[2] = ' '; break;
	case R9 : array[1] = '9'; array[2] = ' '; break;
	case R10: array[1] = '1'; array[2] = '0'; break;
	case R11: array[1] = '1'; array[2] = '1'; break;
	case R12: array[1] = '1'; array[2] = '2'; break;
	case R13: array[1] = '1'; array[2] = '3'; break;
	case R14: array[1] = '1'; array[2] = '4'; break;
	case R15: array[1] = '1'; array[2] = '5'; break;

	case NUM_reg:
	default:
		msg_print(NULL, V_ERROR, "str_reg(): got a bad reg_t");
		return "!!bad!!";
	}

	return array;
}


/****************************** CONSTRUCTOR *******************************/

Reg_man::Reg_man(){
	index = DS_new_bst(
		sizeof(location_t),
		false,
		&lbl,
		&lbl_cmp
	);
}

Reg_man::~Reg_man(){
	DS_delete(index);
}

/******************************* ACCESSOR *********************************/

void Reg_man::find(std::string &lbl_s, lbl_pt lbl){
	location_t * info;
	
	if(!( info=(location_t*)DS_find(index,lbl->get_name()) )){
		msg_print(NULL, V_ERROR,
			"Reg_man::find(): could not find label %s.", lbl->get_name());
		lbl_s = "ERROR";
	}
	
	if(info->r=NUM_reg){
		// it's a spilled register on the stack
		
	}
	else lbl_s = str_reg(lbl->get_size(), info->r);
}

lbl_pt Reg_man::get (reg_t r){
	
}
void Reg_man::get_open(std::string &lbl_s){
	
}

/******************************* MUTATORS *********************************/

void Reg_man::set(reg_t r, lbl_pt l){
	location_t s;
	
	s.l=l;
	s.r=r;
	
	DS_insert(index, &s);
}

void Reg_man::clear(lbl_pt lbl){
	if(DS_find(index, lbl->get_name())) DS_remove(index);
	else{
		msg_print(NULL, V_WARN,
			"Reg_man::clear(): label was not present in index.");
	}
}

void Reg_man::clear(void){
	DS_empty(index);
}



