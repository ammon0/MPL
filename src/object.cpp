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


#include <mpl/object.hpp>
#include <util/msg.h>


Object::Object(void){
	sclass = sc_none;
}

/******************************* MUTATORS *********************************/

void Object::set_name  (const char * name){
	if(label.empty()) label = name;
	else{
		msg_print(NULL, V_ERROR, "Object::set_name(): name already set");
		throw;
	}
}
void Object::set_sclass(storage_class_t storage_class){
	if(sclass == sc_none) sclass = storage_class;
	else{
		msg_print(NULL, V_ERROR,
			"Object::set_sclass(): storage class already set");
		throw;
	}
}

