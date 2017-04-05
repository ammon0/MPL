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

/**	@file gen.hpp
 *	
 *	Code generators for the Minimum Portable Language
 */


#ifndef _GEN_HPP
#define _GEN_HPP


/// x86 processor modes
typedef enum{
	xm_real,      ///< Real address (legacy) Mode not supported
	xm_smm,       ///< System Management Mode not supported
	xm_protected, ///< Protected 32-bit mode
	xm_long       ///< 64-bit mode
} x86_mode_t;


/**	Generate an x86 assembler file for the program data provided
 *
 *	@param out_fd The file descriptor where the assembler commands will be written.
 *	@param prog The program data for which assembler code will be generated.
 *	@param mode The x86 proccessor mode the code will be generated for.
 */
void gen_x86(FILE * out_fd, PPD prog, x86_mode_t mode);
//void arm(FILE * out_fd, PPD prog, mode_t mode);


#endif // _GEN_HPP


