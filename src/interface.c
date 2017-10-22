/* -*- c -*- */
#if defined HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */
#if defined HAVE_VERSION_H
# include "version.h"
#endif	/* HAVE_VERSION_H */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "interface.h"

#if defined __INTEL_COMPILER
# pragma warning (push)
# pragma warning (disable:177)
# pragma warning (disable:111)
# pragma warning (disable:3280)
#elif defined __GNUC__
# if __GNUC__ > 4 || __GNUC__ == 4 &&  __GNUC_MINOR__ >= 6
#  pragma GCC diagnostic push
# endif	 /* GCC version */
# pragma GCC diagnostic ignored "-Wunused-label"
# pragma GCC diagnostic ignored "-Wunused-variable"
# pragma GCC diagnostic ignored "-Wunused-function"
# pragma GCC diagnostic ignored "-Wshadow"
#endif	/* __INTEL_COMPILER */


static inline bool
yuck_streqp(const char *s1, const char *s2)
{
	return !strcmp(s1, s2);
}

/* for multi-args */
static inline char**
yuck_append(char **array, size_t n, char *val)
{
	if (!(n % 16U)) {
		/* resize */
		void *tmp = realloc(array, (n + 16U) * sizeof(*array));
		if (tmp == NULL) {
			free(array);
			return NULL;
		}
		/* otherwise make it persistent */
		array = tmp;
	}
	array[n] = val;
	return array;
}

static enum yuck_cmds_e yuck_parse_cmd(const char *cmd)
{
	if (0) {
		;
	} else {
		/* error here? */
		fprintf(stderr, "mpl: invalid command `%s'\n\
Try `--help' for a list of commands.\n", cmd);
	}
	return (enum yuck_cmds_e)-1;
}


 int yuck_parse(yuck_t tgt[static 1U], int argc, char *argv[])
{
	char *op;
	int i;

	/* we'll have at most this many args */
	memset(tgt, 0, sizeof(*tgt));
	if ((tgt->args = calloc(argc, sizeof(*tgt->args))) == NULL) {
		return -1;
	}
	for (i = 1; i < argc && tgt->nargs < (size_t)-1; i++) {
		op = argv[i];

		switch (*op) {
		case '-':
			/* could be an option */
			switch (*++op) {
			default:
				/* could be glued into one */
				for (; *op; op++) {
					goto shortopt; back_from_shortopt:;
				}
				break;
			case '-':
				if (*++op == '\0') {
					i++;
					goto dashdash; back_from_dashdash:;
					break;
				}
				goto longopt; back_from_longopt:;
				break;
			case '\0':
				goto plain_dash;
			}
			break;
		default:
		plain_dash:
			goto arg; back_from_arg:;
			break;
		}
	}
	if (i < argc) {
		op = argv[i];

		if (*op++ == '-' && *op++ == '-' && !*op) {
			/* another dashdash, filter out */
			i++;
		}
	}
	/* has to be here as the max_pargs condition might drive us here */
	dashdash:
	{
		/* dashdash loop, pile everything on tgt->args
		 * don't check for subcommands either, this is in accordance to
		 * the git tool which won't accept commands after -- */
		for (; i < argc; i++) {
			tgt->args[tgt->nargs++] = argv[i];
		}
	}
	return 0;

	longopt:
	{
		/* split into option and arg part */
		char *arg;

		if ((arg = strchr(op, '=')) != NULL) {
			/* \nul this one out */
			*arg++ = '\0';
		}

		switch (tgt->cmd) {
		default:
			goto MPL_CMD_NONE_longopt; back_from_MPL_CMD_NONE_longopt:;
			break;
		}
		goto back_from_longopt;


		MPL_CMD_NONE_longopt:
		{
			if (0) {
				;
			} else if (yuck_streqp(op, "help")) {
				/* invoke auto action and exit */
				yuck_auto_help(tgt);
				goto success;
			} else if (yuck_streqp(op, "version")) {
				/* invoke auto action and exit */
				yuck_auto_version(tgt);
				goto success;
			}     else if (yuck_streqp(op, "x86-long")) {
				tgt->x86_long_flag++; goto xtra_chk;
			} else if (yuck_streqp(op, "x86-protected")) {
				tgt->x86_protected_flag++; goto xtra_chk;
			} else if (yuck_streqp(op, "arm-v7")) {
				tgt->arm_v7_flag++; goto xtra_chk;
			} else if (yuck_streqp(op, "arm-v8")) {
				tgt->arm_v8_flag++; goto xtra_chk;
			} else if (yuck_streqp(op, "output")) {
				tgt->output_arg = arg ?: argv[++i];
			} else {
				/* grml */
				fprintf(stderr, "mpl: unrecognized option `--%s'\n", op);
				goto failure;
			xtra_chk:
				if (arg != NULL) {
					fprintf(stderr, "mpl: option `--%s' doesn't allow an argument\n", op);
					goto failure;
				}
			}
			if (i >= argc) {
				fprintf(stderr, "mpl: option `--%s' requires an argument\n", op);
				goto failure;
			}
			goto back_from_MPL_CMD_NONE_longopt;
		}
		
	}

	shortopt:
	{
		char *arg = op + 1U;

		switch (tgt->cmd) {
		default:
			goto MPL_CMD_NONE_shortopt; back_from_MPL_CMD_NONE_shortopt:;
			break;
		}
		goto back_from_shortopt;


		MPL_CMD_NONE_shortopt:
		{
			switch (*op) {
			default:
				/* again for clarity */
				switch (*op) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (op[-1] == '-') {
						/* literal treatment of numeral */
						goto arg;
					}
					/* fallthrough */
				default:
					break;
				}
				;
				;
				fprintf(stderr, "mpl: unrecognized option -%c\n", *op);
				goto failure;



				
			case 'h':
				/* invoke auto action and exit */
				yuck_auto_help(tgt);
				goto success;
				break;
			case 'V':
				/* invoke auto action and exit */
				yuck_auto_version(tgt);
				goto success;
				break;
			case 'v':
				tgt->dashv_flag++;
				break;
			case 'q':
				tgt->dashq_flag++;
				break;
			case 'd':
				tgt->dashd_flag++;
				break;
			case 'p':
				tgt->dashp_flag++;
				break;
			case 'o':
				tgt->output_arg = *arg
					? (op += strlen(arg), arg)
					: argv[++i];
				break;
			}
			if (i >= argc) {
				fprintf(stderr, "mpl: option `--%s' requires an argument\n", op);
				goto failure;
			}
			goto back_from_MPL_CMD_NONE_shortopt;
		}
		
	}

	arg:
	{
		if (tgt->cmd || YUCK_NCMDS == 0U) {
			tgt->args[tgt->nargs++] = argv[i];
		} else {
			/* ah, might be an arg then */
			if ((tgt->cmd = yuck_parse_cmd(op)) > YUCK_NCMDS) {
				return -1;
			}
		}
		goto back_from_arg;
	}

	failure:
	{
		exit(EXIT_FAILURE);
	}

	success:
	{
		exit(EXIT_SUCCESS);
	}
}

 void yuck_free(yuck_t tgt[static 1U])
{
	if (tgt->args != NULL) {
		/* free despite const qualifier */
		free(tgt->args);
	}
	/* free mulargs */
	switch (tgt->cmd) {
		void *ptr;
	default:
		break;
	case MPL_CMD_NONE:
;
;
;
;
;
;
;
;
;
;
;
		break;
	}
	return;
}

 void yuck_auto_usage(const yuck_t src[static 1U])
{
	switch (src->cmd) {
	default:
	YUCK_NOCMD:
		puts("Usage: mpl [OPTION]... FILE\n\
\n\
An MPL (Minimum Portable Language) compiler.\n\
");
		break;

	}

#if defined yuck_post_usage
	yuck_post_usage(src);
#endif	/* yuck_post_usage */
	return;
}

 void yuck_auto_help(const yuck_t src[static 1U])
{
	yuck_auto_usage(src);


	/* leave a not about common options */
	if (src->cmd == YUCK_NOCMD) {
		;
	}

	switch (src->cmd) {
	default:
	case MPL_CMD_NONE:
		puts("\
  -h, --help            display this help and exit\n\
  -V, --version         output version information and exit\n\
  -v                    be verbose. -vv produces tons of debuging output.\n\
  -q                    be less verbose supress all warnings. -qq silently fail on errors\n\
  -d                    produce debug file.\n\
  -p                    produce portable executable\n\
      --x86-long        produce assembler for x86 Long Mode\n\
      --x86-protected   produce assembler for x86 Protected Mode\n\
      --arm-v7\n\
      --arm-v8\n\
  -o, --output=FILE     set the name of the output file\n\
");
		break;

	}

#if defined yuck_post_help
	yuck_post_help(src);
#endif	/* yuck_post_help */

#if defined PACKAGE_BUGREPORT
	puts("\n\
Report bugs to " PACKAGE_BUGREPORT);
#endif	/* PACKAGE_BUGREPORT */
	return;
}

 void yuck_auto_version(const yuck_t src[static 1U])
{
	switch (src->cmd) {
	default:
#if 0

#elif defined package_string
		puts(package_string);
#elif defined package_version
		printf("mpl %s\n", package_version);
#elif defined PACKAGE_STRING
		puts(PACKAGE_STRING);
#elif defined PACKAGE_VERSION
		puts("mpl " PACKAGE_VERSION);
#elif defined VERSION
		puts("mpl " VERSION);
#else  /* !PACKAGE_VERSION, !VERSION */
		puts("mpl unknown version");
#endif	/* PACKAGE_VERSION */
		break;
	}

#if defined yuck_post_version
	yuck_post_version(src);
#endif	/* yuck_post_version */
	return;
}

#if defined __INTEL_COMPILER
# pragma warning (pop)
#elif defined __GNUC__
# if __GNUC__ > 4 || __GNUC__ == 4 &&  __GNUC_MINOR__ >= 6
#  pragma GCC diagnostic pop
# endif	 /* GCC version */
#endif	/* __INTEL_COMPILER */
