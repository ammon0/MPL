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


Obj_Index * objects;
obj_pt arg1, arg2;


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
		// no dest
		case i_lbl :
		case i_jmp :
		case i_jt  :
		case i_jf  :
		case i_ret :
			inst->left->live = true;
			inst->used_next = false;
			
			arg1 = inst->left;
			arg2 = NULL;
			break;
		
		// dest & left
		case i_ass :
		case i_call:
			break;
		
		// unary
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
			inst->dest->live = false;
			inst->left->live = true;
			
			if(inst->dest == arg1 || inst->dest == arg2)
				inst->used_next = true;
			else inst->used_next = false;
			
			arg1 = inst->left;
			arg2 = NULL;
			break;
		
		// binary ops
		case i_mul:
		case i_div:
		case i_mod:
		case i_shl:
		case i_shr:
		case i_rol:
		case i_ror:
		case i_add:
		case i_sub:
		case i_xor:
		case i_and:
		case i_or :
		case i_cpy:
			// if the result is dead remove it
			if(inst->result->type == st_temp && !inst->result->live){
				// remove the temp symbol
				operands->remove(strings->get(inst->result->label));
				// and the instruction
				blk->remove();
				break;
			}
			
			// now we know the result is live
			inst->dest->live = false;
			inst->left->live = true;
			inst->right->live = true;
			
			if(inst->dest == arg1 || inst->dest == arg2)
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
	blk_pt blk;
	sym_pt sym;
	Routine * routine;
	
	msg_print(NULL, V_TRACE, "opt_dead(): start");
	
	sym = prog_data->symbols.first();
	do{
		if(sym->get_type() == st_routine){
			routine = (Routine*) sym;
			
			blk = routine->first();
			do{
				Liveness(blk);
				blk->flush(); // save some memory
			} while(( blk = routine->next() ));
		}
		
	} while(( sym = prog_data->symbols.next() ));
	
	prog_data->dead = true;
	
	msg_print(NULL, V_TRACE, "opt_dead(): stop");
}


