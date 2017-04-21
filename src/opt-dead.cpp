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

/**	@file opt-dead.cpp
 *
 *	Removes dead operands and instructions. After running, all dead temp
 *	variables, and the instructions that produced them will be deleted.
 *	Additionally, all instructions operands will be marked if they continue to
 *	be live in the block after being used by the instruction. Since temps are
 *	only ever used once, this can only apply to programmer variables used again
 *	in the same block.
 */


#include <mpl/opt.hpp>

#include <util/msg.h>


/******************************************************************************/
//                               PRIVATE DATA
/******************************************************************************/


String_Array * strings;  ///< A pointer to the PPD string space
Operands     * operands; ///< A pointer to the PPD operands

op_pt arg1, arg2;


/******************************************************************************/
//                             PRIVATE FUNCTIONS
/******************************************************************************/


/// Determine whether each symbol is live in each instruction
static void Liveness(blk_pt blk){
	inst_pt inst;
	
	msg_print(NULL, V_TRACE, "Liveness(): start");
	
	arg1 = arg2 = NULL;
	
	inst = blk->last();
	if(!inst) {
		msg_print(NULL, V_DEBUG, "Internal: Liveness() received an empty block");
		return;
	}
	
	do {
		switch (inst->op){
		// no args
		case i_nop :
			break;
		
		// no results
		case i_lbl :
		case i_call:
		case i_parm:
		case i_proc:
		case i_jmp :
		case i_jz  :
		case i_loop:
		case i_rtrn:
			inst->left->live = true;
			inst->used_next = false;
			
			arg1 = inst->left;
			arg2 = NULL;
			break;
		
		// unary ops
		case i_ass :
		case i_ref :
		case i_dref:
		case i_neg :
		case i_not :
		case i_inv :
		case i_inc :
		case i_dec :
		case i_sz  :
			// if the result is dead remove it
			if(inst->result->type == st_temp && !inst->result->live){
				// remove the temp symbol
				operands->remove(strings->get(inst->result->label));
				// and the instruction
				blk->remove();
				break;
			}
			
			// now we know the result is live
			inst->result->live = false;
			inst->left->live = true;
			
			if(inst->result == arg1 || inst->result == arg2)
				inst->used_next = true;
			else inst->used_next = false;
			
			arg1 = inst->left;
			arg2 = NULL;
			break;
		
		// binary ops
		case i_mul:
		case i_div:
		case i_mod:
		case i_exp:
		case i_lsh:
		case i_rsh:
		case i_rol:
		case i_ror:
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
			// if the result is dead remove it
			if(inst->result->type == st_temp && !inst->result->live){
				// remove the temp symbol
				operands->remove(strings->get(inst->result->label));
				// and the instruction
				blk->remove();
				break;
			}
			
			// now we know the result is live
			inst->result->live = false;
			inst->left->live = true;
			inst->right->live = true;
			
			if(inst->result == arg1 || inst->result == arg2)
				inst->used_next = true;
			else inst->used_next = false;
			
			arg1 = inst->left;
			arg2 = inst->right;
			break;
		
		case i_NUM:
		default:
			msg_print(NULL, V_ERROR,
				"Internal Liveness(): unknown instruction");
		}
	} while(( inst = blk->prev() ));
	
	msg_print(NULL, V_TRACE, "Liveness(): stop");
}


/******************************************************************************/
//                             PUBLIC FUNCTION
/******************************************************************************/


void opt_dead(PPD * prog_data){
	blk_pt  blk;
	proc_pt proc;
	
	msg_print(NULL, V_TRACE, "opt_dead(): start");
	
	strings  = &prog_data->strings;
	operands = &prog_data->operands;
	
	proc = prog_data->instructions.first();
	do{
		blk = proc->first();
		do{
			Liveness(blk);
			blk->flush();
		} while(( blk = proc->next() ));
		
	} while(( proc = prog_data->instructions.next() ));
	
	prog_data->dead = true;
	
	msg_print(NULL, V_TRACE, "opt_dead(): stop");
}


