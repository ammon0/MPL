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

/**	A function to extract the label from the operand for indexing purposes
 */
static inline const void * Operands::label(void * op){
	return labels->get( ((op_pt)op)->label );
}

/**	A function to compare labels
 */
static inline imax     Operands::cmp(const void * left, const void * right){
	return strcmp((char*) left, (char*) right);
}

Operands::Operands(String_Array * array){
	index = DS_new_bst(
		sizeof(Operand),
		false,
		&Operands::label,
		&Operands::cmp
	);
	labels = array;
}
Operands::~Operands(void){ DS_delete(index); }

op_pt Operands::find(char * label){
	return (op_pt)DS_find(index, label);
}
void  Operands::remove(char * label){
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


