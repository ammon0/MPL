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

/**	@file instructions.cpp
 *	
 *	Definitions for portable instructions
 */


#include <mpl/instructions.hpp>
#include <util/msg.h>


/******************************************************************************/
//                           BASIC BLOCK FUNCTIONS
/******************************************************************************/


Block:: Block(void){ q = DS_new_list(sizeof(Instruction)); }
Block::~Block(void){ DS_delete(q); }

uint Block::count(void)const{ return DS_count  (q); }
void Block::flush(void){ DS_flush(q); }

inst_pt Block::next   (void)const{ return (inst_pt)DS_next    (q); }
inst_pt Block::prev   (void)const{ return (inst_pt)DS_previous(q); }
inst_pt Block::first  (void)const{ return (inst_pt)DS_first   (q); }
inst_pt Block::last   (void)const{ return (inst_pt)DS_last    (q); }

inst_pt Block::enqueue(inst_pt inst){ return (inst_pt)DS_nq    (q, inst); }
inst_pt Block::remove (void        ){ return (inst_pt)DS_remove(q      ); }


/******************************************************************************/
//                            PROCEDURE FUNCTIONS
/******************************************************************************/


Procedure::Procedure(void){ blocks = DS_new_list(sizeof(Block)); }
Procedure::~Procedure(void){
	blk_pt blk;
	
	while((blk = first())) delete blk;
	DS_delete(blocks);
}

bool Procedure::isempty(void) const{ return DS_isempty(blocks); }

blk_pt Procedure::first(void) const{ return (blk_pt)DS_first(blocks); }
blk_pt Procedure::next (void) const{ return (blk_pt)DS_next (blocks); }

/** This function adds one instruction to the instruction queue and forms basic
 *  blocks as it goes.
 */
inst_pt Procedure::add (inst_pt instruction){
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


/******************************************************************************/
//                        INSTRUCTION_QUEUE FUNCTIONS
/******************************************************************************/


Instruction_Queue::Instruction_Queue(void){
	q = DS_new_list(sizeof(Procedure));
	DS_nq(q, new Procedure);
}
Instruction_Queue::~Instruction_Queue(void){
	proc_pt proc;
	
	while((proc = (proc_pt)DS_first(q))) delete proc;
	DS_delete(q);
}

bool Instruction_Queue::isempty(void) const{ return DS_isempty(q); }

proc_pt Instruction_Queue::proc(void) const{ return (proc_pt)DS_current(q); }

blk_pt Instruction_Queue::first(void) const{
	return (blk_pt)( (proc_pt)DS_first(q) )->first();
}
blk_pt Instruction_Queue::next (void) const{
	blk_pt temp;
	
	if(( temp=proc()->next() )) temp = ((proc_pt)DS_next(q))->first();
	
	return temp;
}

inst_pt Instruction_Queue::add(inst_pt instruction){
	proc_pt last;
	inst_pt temp;

	msg_print(NULL, V_TRACE, "Instruction_Queue::add(): start");
	
	if(instruction->op == i_proc) DS_nq(q, new Procedure);
	
	last = (proc_pt)DS_last(q);
	temp = last->add(instruction);
	
	msg_print(NULL, V_TRACE, "Instruction_Queue::add(): start");
	return temp;
}

