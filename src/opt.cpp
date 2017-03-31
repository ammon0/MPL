/*******************************************************************************
 *
 *	occ : The Omega Code Compiler
 *
 *	Copyright (c) 2016 Ammon Dodson
 *
 ******************************************************************************/

#include "prog_data.h"
#include "proto.h"
#include "errors.h"
#include <string.h>

/**	@file opt.c
 *	Optomize the intermediate representation.
 *	This function converts the two intermediate instruction queues into a single,
 *	optomized, basic-block queue.
 */


/******************************************************************************/
//                               GLOBALS
/******************************************************************************/



/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


static Instruction_Queue * Mk_blk(Instruction_Queue * q){
	Instruction_Queue * blk = NULL;
	iop_pt iop;
	
	debug_msg("\tMk_blk(): start");
	sprintf(err_array, "\tMk_blk(): queue has %u", q->count());
	debug_msg(err_array);
	
	if(!q->isempty()){
		
		debug_msg("\tMk_blk(): the queue is not empty");
		
		// Each block must contain at least one instruction
		iop = q->dq();
		if(!iop)
			crit_error("\tInternal: Mk_blk(): counld not dq from non empty q");
		
		info_msg("\tMk_blk(): Making Block");
		
		blk = new Instruction_Queue();
		
		blk->nq(iop);
		
		if(
			iop->op == I_JMP  ||
			iop->op == I_JZ   ||
			iop->op == I_RTRN ||
			iop->op == I_CALL
		);
		else while(( iop = q->first() )){
			if(iop->label != NO_NAME) break; // entry points are leaders
			blk->nq(q->dq());
			if(
				iop->op == I_JMP  ||
				iop->op == I_JZ   ||
				iop->op == I_RTRN ||
				iop->op == I_CALL
			) break;
			// statements after exits are leaders
		}
	}
	else info_msg("\tMk_blk(): The queue is empty, no block made");
	
	// recover memory
	q->flush();
	
	debug_msg("\tMk_blk(): stop");
	
	return blk;
}


// Colapse labels

static void Dead_blks(DS block_q){
	/* If a block ends with an unconditional jump, and the next block has no label then the second block must be dead
	*/
	
	/* If a block ends with a jump and the next block starts with its target, then the two lines can be removed
	*/
	
	/* After merging two blocks Liveness() should be run again
	*/
}


// optomize inner loops
static void Inner_loop(Instruction_Queue * blk){
	const char * first_lbl = Program_data::get_string(blk->first()->label);
	const char * last_targ = Program_data::get_string(blk->last()->target);
	
	/*
	If the end of a basic block is a jmp to its head then surely it is an inner loop
	*/
	if( first_lbl && last_targ && !strcmp(first_lbl, last_targ) ){
		info_msg("\tInner_loop(): start");
		// aggressively optomize the loop
		info_msg("\tInner_loop(): stop");
	}
}


// Determine whether each symbol is live in each instruction
static void Liveness(Instruction_Queue * blk){
	iop_pt iop;
	
	debug_msg("\tLiveness(): start");
	
	iop = blk->last();
	if(!iop) crit_error("Internal: Liveness() received an empty block");
	
	do {
		switch(iop->op){
		// No Args
		case I_NOP :
		case I_CALL:
			break;
		
		// NO RESULTS
		case I_PUSH:
		case I_PARM:
		case I_JMP :
		case I_JZ  :
		case I_RTRN: // treat them like optional unaries
			if(!iop->arg1_lit && iop->arg1.symbol)
				iop->arg1.symbol->live = true;
			break;
		
		// Unary Ops
		case I_ASS :
		case I_REF :
		case I_DREF:
		case I_NEG :
		case I_NOT :
		case I_INV :
		case I_INC :
		case I_DEC :
			// if the result is dead remove the op
			if(iop->result->storage == sc_temp && !iop->result->live){
				// remove the temp symbol
				Program_data::remove_sym(iop->result->full_name);
				
				// and the iop
				blk->remove();
				break;
			}
			
			// now we know the result is live
			iop->result_live = true;
			iop->result->live = false;
			
			if(!iop->arg1_lit && iop->arg1.symbol->live) iop->arg1_live = true;
			if(!iop->arg1_lit) iop->arg1.symbol->live = true;
			
			break;
		
		// Binary Ops
		case I_MUL:
		case I_DIV:
		case I_MOD:
		case I_EXP:
		case I_LSH:
		case I_RSH:
		case I_ADD :
		case I_SUB :
		case I_BAND:
		case I_BOR :
		case I_XOR :
		case I_EQ :
		case I_NEQ:
		case I_LT :
		case I_GT :
		case I_LTE:
		case I_GTE:
		case I_AND:
		case I_OR :
			// if the result is dead remove the op
			if(iop->result->storage == sc_temp && !iop->result->live){
				// remove the temp symbol
				Program_data::remove_sym(iop->result->full_name);
				
				// and the iop
				blk->remove();
				break;
			}
			
			// now we know the result is live
			iop->result_live = true;
			iop->result->live = false;
			
			if(!iop->arg1_lit && iop->arg1.symbol->live) iop->arg1_live = true;
			if(!iop->arg2_lit && iop->arg2.symbol->live) iop->arg2_live = true;
			
			if(!iop->arg1_lit) iop->arg1.symbol->live = true;
			if(!iop->arg2_lit) iop->arg2.symbol->live = true;
			
			break;
		
		
		case NUM_I_CODES:
		default: crit_error("Liveness(): got a bad op");
		}
	} while(( iop = blk->previous() ));
	
	debug_msg("\tLiveness(): stop");
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


void Optomize(Instruction_Queue * inst_q){
	Instruction_Queue * blk_pt;
	
	info_msg("Optomize(): start");
	
	if(! inst_q->isempty() ){
		
		// collapse labels
		
		if(make_debug) inst_q->Dump(debug_fd);
		
		while (( blk_pt = Mk_blk(inst_q) )){
		
			#ifdef BLK_ADDR
			sprintf(
				err_array,
				"Optomize(): Mk_blk() returned address: %p",
				(void*) blk_pt
			);
			debug_msg(err_array);
			#endif
			
/*			sprintf(*/
/*				err_array,*/
/*				"Optomize(): Printing block of size %u",*/
/*				DS_count(blk)*/
/*			);*/
/*			debug_msg(err_array);*/
/*			*/
/*			if(verbosity >= V_DEBUG) Dump_iq(stderr, blk);*/
		
			Liveness(blk_pt);
			Inner_loop(blk_pt);
			
/*			sprintf(*/
/*				err_array,*/
/*				"Block queue has %u, adding one",*/
/*				DS_count(prog->block_q)*/
/*			);*/
/*			debug_msg(err_array);*/
			
			Program_data::block_q.nq(blk_pt);
			
/*			sprintf(*/
/*				err_array,*/
/*				"Block queue has %u now",*/
/*				DS_count(prog->block_q)*/
/*			);*/
/*			debug_msg(err_array);*/
			
			if( blk_pt != Program_data::block_q.last() )
				err_msg("Internal: Queued block does not match first block");
		}
	}
	else info_msg("Optomize(): The queue is empty");
	
	delete inst_q;
	
	sprintf(
				err_array,
				"Block queue has %u finally",
				Program_data::block_q.count()
			);
			debug_msg(err_array);
	
	info_msg("Optomize(): stop");
}


