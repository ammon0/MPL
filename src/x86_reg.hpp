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


#ifndef _X86_REG_HPP
#define _X86_REG_HPP


/// These are all the x86 "general purpose" registers
typedef enum{
	A,   ///< Accumulator
	B,   ///< General Purpose
	C,   ///< Counter
	D,   ///< Data
	SI,  ///< Source Index
	DI,  ///< Destination Index
	BP,  ///< Base Pointer
	SP,  ///< Stack Pointer
	R8,  ///< General Purpose
	R9,  ///< General Purpose
	R10, ///< General Purpose
	R11, ///< General Purpose
	R12, ///< General Purpose
	R13, ///< General Purpose
	R14, ///< General Purpose
	R15, ///< General Purpose
	NUM_reg
} reg_t;


class Reg_man{
	obj_pt reg[NUM_reg];
	bool   ref[NUM_reg];
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Reg_man();
	
	/******************************* ACCESSOR *********************************/
	
	reg_t find_ref(obj_pt);
	reg_t find_val(obj_pt obj){
		uint i;
		
		for(i=A; i!=NUM_reg; i++){
			reg_t j = static_cast<reg_t>(i);
			if(reg[j] == obj) break;
		}
		return (reg_t)i;
	}
	bool   is_ref(reg_t);
	bool   is_clear(reg_t);
	reg_t  check(void);
	Data * get_obj(reg_t);
	
	/******************************* MUTATORS *********************************/
	
	void set_ref(reg_t r, obj_pt o){ reg[r] = o; ref[r] = true ; }
	void set_val(reg_t r, obj_pt o){ reg[r] = o; ref[r] = false; }
	void clear(void){
		memset(reg, 0, sizeof(obj_pt)*NUM_reg);
		memset(ref, 0, sizeof(bool)*NUM_reg);
	}
	void clear(reg_t r){ reg[r] = NULL; }
	void xchg(reg_t a, reg_t b);
	
};


#endif // _X86_REG_HPP


