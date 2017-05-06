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


#include <mpl/routine.hpp>

#include <util/data.h>
#include <util/msg.h>


Routine::Routine(void): Object(){
	blocks        = DS_new_list(sizeof(Block));
}
Routine::~Routine(void){
	blk_pt blk;
	
	while((blk = get_first_blk())) delete blk;
	DS_delete(blocks);
}

/******************************* ACCESSOR *********************************/

bool Routine::isempty(void) const{ return DS_isempty(blocks); }

blk_pt Routine::get_first_blk(void) const{ return (blk_pt)DS_first(blocks); }
blk_pt Routine::get_next_blk (void) const{ return (blk_pt)DS_next (blocks); }

const char * Routine::print_obj (void) const{
	std::string str;
	obj_pt obj;
	
	str = "Routine: ";
	str += get_label();
	
	if(( obj = formal_params.first() )){
		str += "\n\tParameters:";
		do{
			str += "\n";
			str += obj->print_obj();
		}while(( obj = formal_params.next() ));
	}
	
	if(( obj = auto_storage.first() )){
		str += "\n\tStack Variables:";
		do{
			str += "\n";
			str += obj->print_obj();
		}while(( obj = auto_storage.next() ));
	}
	
	return str.c_str();
}

/******************************* MUTATORS *********************************/

void Routine::set_sclass(storage_class_t storage_class){
	
	if(storage_class != sc_private && storage_class != sc_private){
		msg_print(NULL, V_ERROR,
			"Routine::set_sclass(): cannot change the storage class to %d",
			storage_class
		);
		throw;
	}
	
	sclass = storage_class;
}

inst_pt Routine::add_inst (inst_pt instruction){
	blk_pt last_blk;
	inst_pt inst;
	
	msg_print(NULL, V_TRACE, "Procedure::add(): start");
	
	// if the instruction is a label it is a leader
	if(instruction->op == i_lbl && !isempty()) DS_nq(blocks, new Block);
	
	// add the instruction
	last_blk = (blk_pt)DS_last(blocks);
	inst = last_blk->enqueue(instruction);
	
	// if the instruction is a branch the next one is a leader
	if(
		instruction->op == i_jmp  ||
		instruction->op == i_jz   ||
		instruction->op == i_loop ||
		instruction->op == i_rtrn ||
		instruction->op == i_call
	) DS_nq(blocks, new Block);
	
	
	msg_print(NULL, V_TRACE, "Procedure::add(): stop");
	return inst;
}


