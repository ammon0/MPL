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
	
	msg_print(NULL, V_TRACE, "Mk_blk(): start");
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
	
	msg_print(NULL, V_TRACE, "Mk_blk(): stop");
	return blk;
}


// Determine whether each symbol is live in each instruction
static void Liveness(Instructions * blk){
	inst_pt inst;
	
	msg_print(NULL, V_TRACE, "Liveness(): start");
	
	inst = blk->last();
	if(!inst) {
		msg_print(NULL, V_TRACE, "Internal: Liveness() received an empty block");
		return;
	}
	
	do {
		switch (inst->op){
		// no args
		case i_nop :
		case i_call:
			break;
	
		// no results
		case i_push:
		case i_parm:
		case i_jmp :
		case i_jz  :
		case i_rtrn: inst->left->live = true; break;
	
		// unary ops
		case i_ass :
		case i_ref :
		case i_dref:
		case i_neg :
		case i_not :
		case i_inv :
		case i_inc :
		case i_dec :
			// if the result is dead remove the op
			if(inst->result->type == st_temp && !inst->result->live){
				// remove the temp symbol
				program_data::remove_sym(iop->result->full_name);
			
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
	
		// binary ops
		case i_mul:
		case i_div:
		case i_mod:
		case i_exp:
		case i_lsh:
		case i_rsh:
		case i_add :
		case i_sub :
		case i_band:
		case i_bor :
		case i_xor :
		case i_eq :
		case i_neq:
		case i_lt :
		case i_gt :
		case i_lte:
		case i_gte:
		case i_and:
		case i_or :
			// if the result is dead remove the op
			if(iop->result->storage == sc_temp && !iop->result->live){
				// remove the temp symbol
				program_data::remove_sym(iop->result->full_name);
			
				// and the iop
				blk->remove();
				break;
			
			// now we know the result is live
			iop->result_live = true;
			iop->result->live = false;
			
			if(!iop->arg1_lit && iop->arg1.symbol->live) iop->arg1_live = true;
			if(!iop->arg2_lit && iop->arg2.symbol->live) iop->arg2_live = true;
			
			if(!iop->arg1_lit) iop->arg1.symbol->live = true;
			if(!iop->arg2_lit) iop->arg2.symbol->live = true;
			
			break;
		
		case i_NUM:
		default:
			msg_print(NULL, V_ERROR,
				"Liveness(): got a bad op"
			);
		}
	} while(( inst = blk->previous() ));
	
	msg_print(NULL, V_TRACE, "Liveness(): stop");
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


void opt_liveblock(PPD * prog_data){
	Instructions * blk_pt;
	
	msg_print(NULL, V_TRACE, "Optomize(): start");
	
	if(! prog_data->instructions.isempty() ){
		while (( blk_pt = Mk_blk(prog_data->instructions) )){
		
			#ifdef BLK_ADDR
			msg_print(
				NULL,
				V_TRACE,
				"Optomize(): Mk_blk() returned address: %p",
				(void*) blk_pt);
			#endif
			
/*			sprintf(*/
/*				err_array,*/
/*				"Optomize(): Printing block of size %u",*/
/*				DS_count(blk)*/
/*			);*/
/*			debug_msg(err_array);*/
/*			*/
/*			if(verbosity >= V_DEBUG) Dump_iq(stderr, blk);*/
		
			Liveness(blk_pt, prog_data->operands);
			
/*			sprintf(*/
/*				err_array,*/
/*				"Block queue has %u, adding one",*/
/*				DS_count(prog->block_q)*/
/*			);*/
/*			debug_msg(err_array);*/
			
			prog_data->bq.nq(blk_pt);
			
/*			sprintf(*/
/*				err_array,*/
/*				"Block queue has %u now",*/
/*				DS_count(prog->block_q)*/
/*			);*/
/*			debug_msg(err_array);*/
			
			if( blk_pt != blk_q->last() )
				msg_print(NULL, V_ERROR,
					"Internal: Queued block does not match first block"
				);
		}
	}
	else msg_print(NULL, V_DEBUG, "Optomize(): The queue is empty");
	
	msg_print(NULL, V_TRACE, "Block queue has %u finally", blk_q->count() );
	msg_print(NULL, V_TRACE, "Optomize(): stop");
}


