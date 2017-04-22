/*******************************************************************************
 *
 *	lib-util : A Utility Library
 *
 *	Copyright (c) 2016-2017 Ammon Dodson
 *	You should have received a copy of the license terms with this software. If
 *	not, please visit the project homepage at:
 *	https://github.com/ammon0/lib-util
 *
 ******************************************************************************/


/******************************************************************************/
//                                 HEADERS
/******************************************************************************/


#include <util/msg.h>
#include <util/string_array.hpp>

#include <stdlib.h>
#include <string.h>


/******************************************************************************/
//                          PRIVATE TYPE DEFINITIONS
/******************************************************************************/


#define NAME_ARR_SZ 1024 ///< Starting size for the dynamic name array

/********************************* MESSAGES ***********************************/

// ERRORS
const char* _e_mem    ="ERROR: Could not allocate more memory";
const char* _e_nimp   ="ERROR: that feature is not implemented";
const char* _e_over   ="ERROR: Overflow";


/******************************************************************************/
//                       PRIVATE FUNCTION DEFINITIONS
/******************************************************************************/



/******************************************************************************/
//                       PUBLIC FUNCTION DEFINITIONS
/******************************************************************************/


String_Array::String_Array(void){
	array = (char*)malloc(sizeof(char) * NAME_ARR_SZ);
	if(!array) {
		msg_print(NULL, V_ERROR, _e_mem);
		throw STRING_EXCEPTION_MEM;
	}
	
	size = NAME_ARR_SZ;
}

String_Array::~String_Array(void){
	free(array);
}


str_dx String_Array::operator+(const char * name){ return add(name); }
str_dx String_Array::add(const char * name){
	str_dx name_sz, temporary;
	
	name_sz = (str_dx)strlen(name)+1; // +1 for the null
	
	// resize the array if necessary
	if(size - next < name_sz){
		if ((unsigned long int)size * 2 > UINT_MAX ){
			msg_print(NULL, V_ERROR, _e_over);
			throw STRING_EXCEPTION_MAX;
		}
		
		array = (char*) realloc(array, (size *= 2));
		if(!array){
			msg_print(NULL, V_ERROR, _e_mem);
			throw STRING_EXCEPTION_MEM;
		}
	}
	
	// Add name
	strncpy(array+next, name, name_sz);
	temporary = next;
	next += name_sz;
	return temporary;
}

const char * __attribute__((pure))
String_Array::get(const str_dx dx) const {
	if(dx == NO_NAME) return NULL;
	else return array+dx;
}


