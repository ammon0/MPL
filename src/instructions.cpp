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
 *	Definitions for machine instructions
 */


#include <mpl/instructions.hpp>


Instructions::Instructions(void){ q = DS_new_list(sizeof(Instruction)); }
Instructions::~Instructions(void){ DS_delete(q); }

uint Instructions::count  (void){ return DS_count  (q); }
bool Instructions::isempty(void){ return DS_isempty(q); }

inst_pt Instructions::current(void){ return (inst_pt)DS_current (q); }
inst_pt Instructions::next   (void){ return (inst_pt)DS_next    (q); }
inst_pt Instructions::prev   (void){ return (inst_pt)DS_previous(q); }
inst_pt Instructions::last   (void){ return (inst_pt)DS_last    (q); }

inst_pt
Instructions::insert(inst_code op, op_pt result, op_pt left, op_pt right){
	Instruction i;
	
	i.op     = op;
	i.result = result;
	i.left   = left;
	i.right  = right;
	
	return (inst_pt)DS_insert(q, &i);
}

inst_pt Instructions::nq(inst_pt inst){ return (inst_pt)DS_nq(q,inst); }
inst_pt Instructions::dq(void)        { return (inst_pt)DS_dq(q); }

void Instructions::flush(void){ DS_flush(q); }


