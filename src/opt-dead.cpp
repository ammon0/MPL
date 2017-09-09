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


lbl_pt arg1, arg2;
Sym_index * syms;


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
		
		// l-r unary
		case i_neg:
		case i_not:
			// if the result is dead remove it
			if(inst->dest->get_mode() == am_temp && !inst->dest->live){
				// remove the temp symbol
				syms->remove(inst->dest->get_name());
				// and the instruction
				blk->remove();
				break;
			}
			
			// now that we know the destination is live
			if(inst->left){
				inst->left->live = true;
				inst->dest->live = false;
				arg1 = inst->left;
			}else{
				arg1 = inst->dest;
			}
			
			if(inst->dest == arg1 || inst->dest == arg2)
				inst->used_next = true;
			
			arg2 = NULL;
			break;
		
		// l-r binary
		case i_add:
		case i_sub:
		case i_and:
		case i_or :
		case i_xor:
		case i_shl:
		case i_shr:
		case i_rol:
		case i_ror:
			// if the result is dead remove it
			if(inst->dest->get_mode() == am_temp && !inst->dest->live){
				// remove the temp symbol
				syms->remove(inst->dest->get_name());
				// and the instruction
				blk->remove();
				break;
			}
			
			// now that we know the destination is live
			if(inst->right){
				inst->left->live = true;
				inst->right->live = true;
				inst->dest->live = false;
				arg1 = inst->left;
				arg2 = inst->right;
			}else{
				inst->left->live = true;
				arg1 = inst->dest;
				arg2 = inst->left;
			}
			
			if(inst->dest == arg1 || inst->dest == arg2)
				inst->used_next = true;
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
	
	syms = &prog_data->symbols;
	
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


