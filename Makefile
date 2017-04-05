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


#################################### FILES #####################################


srcdir    :=./src
headerdir :=./mpl

# Change these variables to point to the appropriate installation directories
WORKDIR   :=./work
INSTALLDIR:=$(HOME)/prg
LIBDIR    :=$(INSTALLDIR)/lib
INCDIR    :=$(INSTALLDIR)/include

headers:=$(wildcard $(headerdir)/*.hpp)
cpp_sources:=$(wildcard $(srcdir)/*.cpp)

allfiles:= $(headers) $(cpp_sources)

# Object files
ppd_objects :=operands.o instructions.o
gen_objects :=gen-arm.o gen-x86.o
pexe_objects:=gen-pexe.o read-pexe.o
opt_objects :=opt-liveblock.o

# Prefix the object files
ppd_objects :=$(addprefix $(WORKDIR)/, $(ppd_objects) )
gen_objects :=$(addprefix $(WORKDIR)/, $(gen_objects) )
pexe_objects:=$(addprefix $(WORKDIR)/, $(pexe_objects))
opt_objects :=$(addprefix $(WORKDIR)/, $(opt_objects))

CPP_OBJECTS:=$(ppd_objects) $(gen_objects) $(pexe_objects) $(opt_objects)


#################################### FLAGS #####################################


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

CFLAGS:=  --std=c11   -g $(CWARNINGS)   -I./ -I$(INCDIR) -L$(LIBDIR)
CXXFLAGS:=--std=c++14 -g $(CXXWARNINGS) -I./ -I$(INCDIR) -L$(LIBDIR)
LFLAGS:=#-d
LEX:= flex


################################### TARGETS ####################################


.PHONEY: docs


################################# PRODUCTIONS ##################################


libgen.a: $(gen_objects) $(ppd_objects)
	ar rcs $@ $(gen_objects) $(ppd_objects)
libopt.a: $(opt_objects) $(ppd_objects)
	ar rcs $@ $(opt_objects) $(ppd_objects)
libpexe.a: $(pexe_objects)


docs: Doxyfile README.md $(allfiles)
	doxygen Doxyfile

$(CPP_OBJECTS): $(WORKDIR)/%.o: $(srcdir)/%.cpp $(headers) | $(WORKDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# working directory
$(WORKDIR):
	mkdir -p $@


################################## UTILITIES ###################################


cleanfiles:=*.a *.o

.PHONEY: clean todolist

clean:
	rm -f $(cleanfiles)
	rm -fr $(WORKDIR)

todolist:
	-@for file in $(allfiles:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true


