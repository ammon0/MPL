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

#include <ppd/ppd.hpp>
#include <util/msg.h>
#include <string.h>

extern "C"{
	#include <src/interface.h>
}

// Default output files
const char * default_dbg  = ".dbg" ;
const char * default_out  = "out"  ;
const char * default_asm  = ".asm" ;
const char * default_pexe = ".pexe";
const char * default_plib = ".plib";

bool make_debug;
FILE * debug_fd;



static inline void Set_files(char ** infilename, yuck_t * arg_pt){
	uint   sum;
	char * debug_file;
	
	// default verbosity
	msg_set_verbosity((msg_log_lvl)(V_NOTE + arg_pt->dashv_flag - arg_pt->dashq_flag));
	
//	verbosity = (verb_t) (DEFAULT_VERBOSITY + arg_pt->dashv_flag - arg_pt->dashq_flag);
	
	if (arg_pt->nargs > 1)
		msg_print(NULL, V_WARN, "Too many arguments...Ignoring.");
	
	msg_print(NULL, V_DEBUG, "\
ARGUMENTS PASSED\n\
		nargs             : %lu\n\
		args              : %s\n\
		dashv_flag        : %u\n\
		dashq_flag        : %u\n\
		dashd_flag        : %u\n\
		dashp_flag        : %u\n\
		x86_long_flag     : %u\n\
		x86_protected_flag: %u\n\
		arm_v7_flag       : %u\n\
		arm_v8_flag       : %u\n\
		output_arg        : %s\n" ,
		arg_pt->nargs      ,
		*arg_pt->args      ,
		arg_pt->dashv_flag ,
		arg_pt->dashq_flag ,
		arg_pt->dashd_flag ,
		arg_pt->dashp_flag ,
		arg_pt->x86_long_flag     ,
		arg_pt->x86_protected_flag,
		arg_pt->arm_v7_flag       ,
		arg_pt->arm_v8_flag,
		arg_pt->output_arg
	);
	
	// test for a target architecture
	sum = arg_pt->x86_long_flag + arg_pt->x86_protected_flag;
	sum += arg_pt->arm_v7_flag + arg_pt->arm_v8_flag;
	
	// if too many targets
	if(sum > 1){
		msg_print(NULL, V_ERROR,
			"Must specify exactly one target architecture.\n");
		exit(EXIT_FAILURE);
	}
		
	
	// if no target has been selected
	if(sum < 1 && !arg_pt->dashp_flag) {
		msg_print(NULL, V_INFO, "Using default target x86-long\n");
		arg_pt->x86_long_flag = true;
	}
	
	// Set the infile
	if(arg_pt->nargs) *infilename = *arg_pt->args;
	else *infilename = NULL;
	
	// Set the debug file
	if (arg_pt->dashd_flag){
		make_debug = true;
		
		debug_file = (char*) malloc(strlen(*arg_pt->args)+strlen(default_dbg)+1);
		if(!debug_file){
			msg_print(NULL, V_ERROR, "Out of Memory\n");
			exit(EXIT_FAILURE);
		}
		
		debug_file = strcpy(debug_file, *arg_pt->args);
		debug_file = strncat(debug_file, default_dbg, strlen(default_dbg));
		
		// clear the file
		fclose(fopen(debug_file, "w"));
		debug_fd = NULL;
		
		debug_fd = fopen(debug_file, "a");
		
		free(debug_file);
	}
	else make_debug = false;
}


int main(int argc, char ** argv){
	yuck_t arg_pt[1];
	char * infile;
	
	
	// Setup
	yuck_parse(arg_pt, argc, argv);
	Set_files(&infile, arg_pt);
	
	
	
	// Cleanup
	yuck_free(arg_pt);
	
	
	return EXIT_SUCCESS;
}


