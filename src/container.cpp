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

#include <mpl/sym.hpp>
#include <util/data.h>
#include <util/msg.h>

#include <string.h>

static inline const void * sym_name(const void * obj){
	return (*(sym_pt*)obj)->get_name();
}
static inline imax lbl_cmp(const void * left, const void * right){
	return strcmp((char*) left, (char*) right);
}


Sym_index::Sym_index(void){
	index = DS_new_bst(
		sizeof(sym_pt),
		false,
		&sym_name,
		&lbl_cmp
	);
}
Sym_index::~Sym_index(void){ DS_delete(index); }

/******************************* ACCESSOR *********************************/

sym_pt Sym_index::find(const char * name) const{
	return *(sym_pt*)DS_find(index, name);
}
sym_pt Sym_index::first(void)const{
	return *(sym_pt*)DS_first(index);
}
sym_pt Sym_index::next (void)const{
	return *(sym_pt*)DS_next(index);
}

/******************************* MUTATORS *********************************/

sym_pt Sym_index::remove(const char * name){
	if(DS_find(index, name)) return *(sym_pt*)DS_remove(index);
	else{
		msg_print(NULL, V_ERROR, "Internal PPD::remove(): no such object");
		throw;
	}
}

sym_pt Sym_index::add(sym_pt object){
//	if(!object->named()){
//		msg_print(NULL, V_ERROR, "Internal PPD::add(): object has no name");
//		throw;
//	}
	
	return *(sym_pt*)DS_insert(index, object);
}


