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

/**	@file opt-liveblock.cpp
 *
 *	Removes dead operands and splits the code into basic blocks
 */


#include <util/msg.h>
#include <mpl/ppd.hpp>


/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


static Instructions * Mk_blk(Instructions * q){
	Instructions * blk = NULL;
	inst_pt inst;
	
	msg_print(NULL, V_DEBUG, "Mk_blk(): start");
	
	msg_print(NULL, V_TRACE, "Mk_blk(): queue has %u", q->count());
	
	if(!q->isempty()){
		msg_print(NULL, V_TRACE, "Mk_blk(): the queue is not empty");
		
		// Each block must contain at least one instruction
		inst = q->dq();
		if(!inst){
			msg_print(
				NULL,
				V_ERROR,
				"Internal: Mk_blk(): counld not dq from non empty q"
			);
			return NULL;
		}
		
		msg_print(NULL, V_INFO, "Mk_blk(): Making Block");
		
		blk = new Instructions();
		
		blk->nq(inst);
		
		if(
			inst->op == i_jmp  ||
			inst->op == i_jz   ||
			inst->op == i_rtrn ||
			inst->op == i_call
		);
		else while(( inst = q->first() )){
			if(inst->op == i_lbl) break; // entry points are leaders
			blk->nq(q->dq());
			if(
				inst->op == i_jmp  ||
				inst->op == i_jz   ||
				inst->op == i_rtrn ||
				inst->op == i_call
			) break;
			// statements after exits are leaders
		}
	}
	else msg_print(NULL, V_INFO,"Mk_blk(): The queue is empty, no block made");
	
	// recover memory
	q->flush();
	
	msg_print(NULL, V_DEBUG, "Mk_blk(): stop");
	return blk;
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


