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
//                              BLOCK METHODS
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
//                            BLOCK_QUEUE METHODS
/******************************************************************************/


Block_Queue::Block_Queue(void){ q = DS_new_list(sizeof(Block)); }
Block_Queue::~Block_Queue(void){
	blk_pt blk;
	
	while((blk = first())) delete blk;
	DS_delete(q);
}

bool Block_Queue::isempty(void) const{ return DS_isempty(q); }

blk_pt Block_Queue::first(void) const{ return (blk_pt)DS_first(q); }
blk_pt Block_Queue::next (void) const{ return (blk_pt)DS_next (q); }

/** This function adds one instruction to the instruction queue and forms basic
 *  blocks as it goes.
 */
inst_pt Block_Queue::add (inst_pt instruction){
	blk_pt last_blk;
	inst_pt inst;
	
	msg_print(NULL, V_TRACE, "Block_Queue::add(): start");
	
	// if the instruction is a label it is a leader
	if(instruction->op == i_lbl && !isempty()) DS_nq(q, new Block);
	
	// add the instruction
	last_blk = (blk_pt)DS_last(q);
	inst = last_blk->enqueue(instruction);
	
	// if the instruction is a branch the next one is a leader
	if(
		instruction->op == i_jmp  ||
		instruction->op == i_jz   ||
		instruction->op == i_loop ||
		instruction->op == i_rtrn ||
		instruction->op == i_call
	) DS_nq(q, new Block);
	
	
	msg_print(NULL, V_TRACE, "Block_Queue::add(): stop");
	return inst;
}


