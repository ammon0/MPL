################################################################################
#
#	MPL : Minimum Portable Language
#
#	Copyright (c) 2017 Ammon Dodson
#	You should have received a copy of the licence terms with this software. If
#	not, please visit the project homepage at:
#	https://github.com/ammon0/MPL
#
################################################################################


################################## FILES #######################################


workdir   :=./work
srcdir    :=./src

# Change these variables to point to the appropriate installation directories
INSTALLDIR:=$(HOME)/prg
LIBDIR    :=$(INSTALLDIR)/lib
INCDIR    :=$(INSTALLDIR)/include

allfiles:=

################################## FLAGS #######################################


# My code builds without warnings--ALWAYS
CWARNINGS:=-Wall -Wextra -pedantic \
	-Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations \
	-Wredundant-decls -Werror=implicit-function-declaration -Wnested-externs \
	-Wshadow -Wbad-function-cast \
	-Wcast-align \
	-Wdeclaration-after-statement -Werror=uninitialized \
	-Winline \
	-Wswitch-default -Wswitch-enum \
	-Wsuggest-attribute=pure -Wsuggest-attribute=const \
	-Wsuggest-attribute=noreturn -Wsuggest-attribute=format \
	-Wtrampolines -Wstack-protector \
	-Wwrite-strings \
	-Wconversion -Wdisabled-optimization \
	 -Wc++-compat -Wpadded

CXXWARNINGS:=-Wall -Wextra -pedantic \
	-Wmissing-declarations -Werror=implicit-function-declaration \
	-Wredundant-decls -Wshadow \
	-Wpointer-arith -Wcast-align \
	-Wuninitialized -Wmaybe-uninitialized -Werror=uninitialized \
	-Winline -Wno-long-long \
	-Wswitch \
	-Wsuggest-attribute=pure -Wsuggest-attribute=const \
	-Wsuggest-attribute=noreturn -Wsuggest-attribute=format \
	-Wtrampolines -Wstack-protector \
	-Wwrite-strings \
	-Wconversion -Wdisabled-optimization

DEBUG_OPT:=

CFLAGS:=  --std=c11   -g $(CWARNINGS) -I$(INCDIR) -L$(LIBDIR)
CXXFLAGS:=--std=c++14 -g $(CXXWARNINGS) -I$(INCDIR) -L$(LIBDIR)
LFLAGS:=#-d
LEX:= flex


################################# TARGETS ######################################


.PHONEY: docs


############################### PRODUCTIONS ####################################


docs: Doxyfile README.md
	doxygen Doxyfile


################################## UTILITIES ###################################

cleanfiles:=

.PHONEY: clean todolist

clean:
	rm -f $(cleanfiles)
	rm -fr $(workdir)

todolist:
	-@for file in $(allfiles:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true


