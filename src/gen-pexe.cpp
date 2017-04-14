/*******************************************************************************
 *
 *	occ : The Omega Code Compiler
 *
 *	Copyright (c) 2016 Ammon Dodson
 *
 ******************************************************************************/


#include "proto.h"
#include "errors.h"
#include <stdio.h>


/******************************************************************************/
//                            TYPE DEFINITIONS
/******************************************************************************/

/** This is the header of a pexe file
 */
typedef struct pexe_header{
	/** @defgroup magic Header magic numbers
	 *  This is the file signature in the header. There are 8 bytes total in
	 *  file signature
	 * @{
	 */
	uint8_t  magic0;
	uint8_t  magic1;
	uint8_t  magic2;
	uint8_t  magic3;
	uint8_t  magic4;
	uint8_t  magic5;
	uint16_t version; // File version
	/// @}
	
	// locations and sizes of components
	uint32_t dx_entries;    ///< just to keep the fields alligned
	uint32_t name_array_sz; ///< Name Array length in bytes
	uint32_t off_sym;       ///< File position for start of symbol table
	uint32_t off_instq;     ///< File position for start of op queue
	
	
	// file integity verification
	uint64_t checksum; ///< checksum of everything after the header
} pexe_h;

/// a sorted index of declarations
typedef struct {
	uint32_t hash_key;
	uint32_t offset;
} pexe_dx_entry;

//typedef struct pexe_symbol{
//	
//} pexe_sym;

//typedef struct pexe_operation{
//	
//} pexe_op;


/******************************************************************************/
//                                CONSTANTS
/******************************************************************************/


// File signature
const uint8_t  magic0  = 0xB4;
const uint8_t  magic1  = 'P';

const uint8_t  magicE1 = 'E';
const uint8_t  magicX  = 'X';
const uint8_t  magicE2 = 'E';

const uint8_t  magicL = 'L';
const uint8_t  magicI = 'I';
const uint8_t  magicB = 'B';

const uint8_t  magic5  = 0xAF;
const uint16_t version = 0x0000;




/**	Create a Portable Executable file from the intermediate represetation
	Takes the output file name as a parameter. stack or register based VM?
 */
void pexe (char * filename){
	FILE* fd;
	pexe_h header;
	size_t result;
	
	info_msg("pexe(): start");
	
	//Initialize the header
	header.magic0  = magic0;
	header.magic1  = magic1;
	header.magic2  = magicE1;
	header.magic3  = magicX;
	header.magic4  = magicE2;
	header.magic5  = magic5;
	header.version = version;
	
	header.name_array_sz = 0;
	header.off_sym       = 0;
	header.off_instq     = 0;
	header.dx_entries    = 0;
	
	header.checksum = 0;
	
	
	sprintf(err_array, "pexe(): Size of pexe_h is: %lu", sizeof(pexe_h));
	info_msg(err_array);
	
	// open the file
	sprintf(err_array, "pexe(): Creating file: '%s'", filename);
	info_msg(err_array);
	fd = fopen(filename, "w");
	if(!fd){
		err_msg("pexe(): Could not open file");
		return;
	}
	
	// write header
	result = fwrite(&header, 1, sizeof(pexe_h), fd);
	
	sprintf(err_array, "pexe(): fwrite() returned %lu", result);
	info_msg(err_array);
	
	fclose(fd);
	fd=NULL;
	
	info_msg("pexe(): stop");
}


