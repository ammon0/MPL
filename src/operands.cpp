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


#include <mpl/operands.hpp>

#include <string.h>
#include <util/data.h>



Operand::Operand(void){
	formal_params = DS_new_list(sizeof(op_pt));
	autos         = DS_new_list(sizeof(op_pt));
}
Operand::~Operand(void){
	DS_delete(autos);
	DS_delete(formal_params);
}


static const void * op_label(const void * op){
	return Operands::labels->get( ((op_pt)op)->label );
}

static imax lbl_cmp(const void * left, const void * right){
	return strcmp((char*) left, (char*) right);
}

Operands::Operands(String_Array * array){
	index = DS_new_bst(
		sizeof(Operand),
		false,
		&op_label,
		&lbl_cmp
	);
	labels = array;
}
Operands::~Operands(void){ DS_delete(index); }

op_pt Operands::find(char * label){
	return (op_pt)DS_find(index, label);
}
void  Operands::remove(const char * label){
	if (DS_find(index, label)) DS_remove(index);
}
op_pt Operands::add(str_dx label, width_t size, segment_t where, bool sign){
	Operand op;
	
	op.label = label;
	op.width = size;
	op.type  = where;
	op.sign  = sign;
	
	return (op_pt)DS_insert(index, &op);
}

op_pt Operands::first(void)const{ return (op_pt)DS_first(index); }
op_pt Operands::next (void)const{ return (op_pt)DS_next (index); }

uint Operand::param_cnt(void) const{ return DS_count(formal_params); }


