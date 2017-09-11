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


/******************************************************************************/
//                  GLOBAL CONSTANTS IN THE GEN-X86 MODULE
/******************************************************************************/


/// These are the x86 register sizes
#define BYTE  ((size_t) 1)
#define WORD  ((size_t) 2)
#define DWORD ((size_t) 4)
#define QWORD ((size_t) 8)

#define PTR (mode == xm_long? QWORD:DWORD)


/******************************************************************************/
//                              MODULE VARIABLES
/******************************************************************************/


static PPD        * program_data;
static FILE       * fd          ; ///< the output file descriptor
static x86_mode_t   mode        ; ///< the processor mode we are building for


static size_t param_sz;
static size_t frame_sz;


/******************************************************************************/
//                          STRING WRITING FUNCTIONS
/******************************************************************************/


/** Return a string representation of a number. */
static inline const char * str_num(umax num){
	static char array[20];
	sprintf(array, "0x%llu", num);
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


static inline void put_lbl(lbl_pt op){ put_str(FORM_LBL, op->get_name()); }


void x86_declarations(void);
void set_struct_size(Structure * structure);



const char * str_reg(size_t width, reg_t reg);



typedef struct _root * DS;

class Reg_man{
	DS index;
	
	
	/* what this really becomes is a label translation from labels in the symbol index to register labels or [spilled temps].
	*/
	
public:
	/****************************** CONSTRUCTOR *******************************/
	
	Reg_man(void);
	~Reg_man(void);
	
	/******************************* ACCESSOR *********************************/
	
	void  find(std::string &lbl_s, lbl_pt label);
	lbl_pt get (reg_t registr);
	void   get_open(std::string &);
	
	/******************************* MUTATORS *********************************/
	
	void set  (reg_t registr, lbl_pt label);
	void clear(lbl_pt label);
	void clear(void);
};




#endif // X86_HPP


