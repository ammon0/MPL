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


#ifndef X86_HPP
#define X86_HPP

#include <mpl/ppd.hpp>
#include <mpl/gen.hpp>

#include <util/msg.h>

#include <stdio.h>
#include <string.h>
#include <string>
#include <stdarg.h>

/******************************************************************************/
//                               DEFINITIONS
/******************************************************************************/


/// These are the x86 register sizes
#define BYTE  ((size_t) 1)
#define WORD  ((size_t) 2)
#define DWORD ((size_t) 4)
#define QWORD ((size_t) 8)

#define PTR (mode == xm_long? QWORD:DWORD)


//typedef umax index_t;

//typedef struct loc{ // can contain the location of an operand
//	bool ref;
//	reg_t reg;
//	std::string mem;
//} loc;

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
	sym_pt reg[NUM_reg];
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
	
	void set_ref(reg_t r, sym_pt o){ reg[r] = o; ref[r] = true ; }
	void set_val(reg_t r, sym_pt o){ reg[r] = o; ref[r] = false; }
	void clear(void){
		memset(reg, 0, sizeof(obj_pt)*NUM_reg);
		memset(ref, 0, sizeof(bool)*NUM_reg);
	}
	void clear(reg_t r){ reg[r] = NULL; }
	void xchg(reg_t a, reg_t b);
	
};


/******************************************************************************/
//                  GLOBAL CONSTANTS IN THE GEN-X86 MODULE
/******************************************************************************/




/******************************************************************************/
//                              MODULE VARIABLES
/******************************************************************************/


static PPD        * program_data;
static FILE       * fd          ; ///< the output file descriptor
static x86_mode_t   mode        ; ///< the processor mode we are building for

static size_t param_sz;
static size_t frame_sz;

/**	the register descriptor.
 *	keeps track of what value is in each register at any time
 *
 */
//static obj_pt reg_d[NUM_reg];

static Reg_man reg_d;


/******************************************************************************/
//                          STRING WRITING FUNCTIONS
/******************************************************************************/


/** Return a string representation of a number. */
static inline const char * str_num(umax num){
	static char array[20];
	sprintf(array, "0x%llu", num);
	return array;
}

/** Return the appropriate string to use the given x86 register. */
static const char * str_reg(size_t width, reg_t reg){
	static char array[4] = "   ";

	if(mode != xm_long && width == QWORD){
		msg_print(NULL, V_ERROR,
			"Internal str_reg(): qword only available in long mode");
		return "!!Bad width!!";
	}
	
	if(mode != xm_long && reg > SP){
		msg_print(NULL, V_ERROR,
			"Internal str_reg(): R8-R15 only available in long mode");
		return "!!Bad register!!";
	}
	
	switch (width){
	case BYTE : array[0] = ' '; array[2] = ' '; break;
	case WORD : array[0] = ' '; array[2] = 'x'; break;
	case DWORD: array[0] = 'e'; array[2] = 'x'; break;
	case QWORD: array[0] = 'r'; array[2] = 'x'; break;
	
	default:
		msg_print(NULL, V_ERROR, "str_reg(): got a bad width");
		return NULL;
	}

	switch (reg){
	case A  : array[1] = 'a'; break;
	case B  : array[1] = 'b'; break;
	case C  : array[1] = 'c'; break;
	case D  : array[1] = 'd'; break;
	case SI : array[1] = 's'; array[2] = 'i'; break;
	case DI : array[1] = 'd'; array[2] = 'i'; break;
	case BP : array[1] = 'b'; array[2] = 'p'; break;
	case SP : array[1] = 's'; array[2] = 'p'; break;
	case R8 : array[1] = '8'; array[2] = ' '; break;
	case R9 : array[1] = '9'; array[2] = ' '; break;
	case R10: array[1] = '1'; array[2] = '0'; break;
	case R11: array[1] = '1'; array[2] = '1'; break;
	case R12: array[1] = '1'; array[2] = '2'; break;
	case R13: array[1] = '1'; array[2] = '3'; break;
	case R14: array[1] = '1'; array[2] = '4'; break;
	case R15: array[1] = '1'; array[2] = '5'; break;

	case NUM_reg:
	default:
		msg_print(NULL, V_ERROR, "str_reg(): got a bad reg_t");
		return "!!bad!!";
	}

	return array;
}

/** Add a command string to the output file. */
static void __attribute__((format(printf, 1, 2)))
put_str(const char * format, ...){
	va_list ap;

	va_start(ap, format);
	vfprintf(fd, format, ap);
	va_end(ap);
}

#define FORM_2   "\t%6s                     ;                     ;%s\n"
#define FORM_3   "\t%6s %20s;                     ;%s\n"
#define FORM_4   "\t%6s %20s, %20s;%s\n"
#define FORM_LBL "%s:\n"


static inline void lbl(obj_pt op){ put_str(FORM_LBL, op->get_label()); }


void x86_declarations(void);
void set_struct_size(Structure * structure);

#endif // X86_HPP


