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

#include <mpl/obj_container.hpp>
#include <util/data.h>
#include <util/msg.h>

#include <string.h>
static inline const void * obj_label(const void * obj){
	return (*(obj_pt*)obj)->get_label();
}
static inline imax lbl_cmp(const void * left, const void * right){
	return strcmp((char*) left, (char*) right);
}


Obj_Index::Obj_Index(void){
	index = DS_new_bst(
		sizeof(obj_pt),
		false,
		&obj_label,
		&lbl_cmp
	);
}
Obj_Index::~Obj_Index(void){ DS_delete(index); }

/******************************* ACCESSOR *********************************/

obj_pt Obj_Index::find(const char * name) const{
	return *(obj_pt*)DS_find(index, name);
}
obj_pt Obj_Index::first(void)const{
	return *(obj_pt*)DS_first(index);
}
obj_pt Obj_Index::next (void)const{
	return *(obj_pt*)DS_next(index);
}

/******************************* MUTATORS *********************************/

obj_pt Obj_Index::remove(const char * name){
	if(DS_find(index, name)) return *(obj_pt*)DS_remove(index);
	else{
		msg_print(NULL, V_ERROR, "Internal PPD::remove(): no such object");
		throw;
	}
}

obj_pt Obj_Index::add(obj_pt object){
	if(!object->named()){
		msg_print(NULL, V_ERROR, "Internal PPD::add(): object has no name");
		throw;
	}
	
	return *(obj_pt*)DS_insert(index, object);
}


