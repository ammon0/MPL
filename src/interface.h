/* -*- c -*- */
#if !defined INCLUDED_yuck_h_
#define INCLUDED_yuck_h_

#include <stddef.h>

#define YUCK_OPTARG_NONE	((void*)0x1U)

enum yuck_cmds_e {
	/* value used when no command was specified */
	MPL_CMD_NONE = 0U,

	/* actual commands */
	
	/* convenience identifiers */
	YUCK_NOCMD = MPL_CMD_NONE,
	YUCK_NCMDS = MPL_CMD_NONE
};



typedef struct yuck_s yuck_t;



/* generic struct */
struct yuck_s {
	enum yuck_cmds_e cmd;

	/* left-over arguments,
	 * the command string is never a part of this */
	size_t nargs;
	char **args;

	/* slots common to all commands */

	/* help is handled automatically */
	/* version is handled automatically */
	unsigned int dashv_flag;
	unsigned int dashq_flag;
	unsigned int dashd_flag;
	unsigned int dashp_flag;
	unsigned int x86_long_flag;
	unsigned int x86_protected_flag;
	unsigned int arm_v7_flag;
	unsigned int arm_v8_flag;
	char *output_arg;
};


extern __attribute__((nonnull(1))) int
yuck_parse(yuck_t*, int argc, char *argv[]);
extern __attribute__((nonnull(1))) void yuck_free(yuck_t*);

extern __attribute__((nonnull(1))) void yuck_auto_help(const yuck_t*);
extern __attribute__((nonnull(1))) void yuck_auto_usage(const yuck_t*);
extern __attribute__((nonnull(1))) void yuck_auto_version(const yuck_t*);

/* some hooks */
#if defined yuck_post_help
extern __attribute__((nonnull(1))) void yuck_post_help(const yuck_t*);
#endif	/* yuck_post_help */

#if defined yuck_post_usage
extern __attribute__((nonnull(1))) void yuck_post_usage(const yuck_t*);
#endif	/* yuck_post_usage */

#if defined yuck_post_version
extern __attribute__((nonnull(1))) void yuck_post_version(const yuck_t*);
#endif	/* yuck_post_version */

#endif	/* INCLUDED_yuck_h_ */
