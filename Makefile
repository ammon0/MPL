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
headerdir :=./ppd

# Change these variables to point to the appropriate installation directories
WORKDIR   :=./work
INSTALLDIR:=$(HOME)/prg
LIBDIR    :=$(INSTALLDIR)/lib
INCDIR    :=$(INSTALLDIR)/include
BINDIR    :=$(INSTALLDIR)/bin

headers:=$(wildcard $(headerdir)/*.hpp)
cpp_sources:=$(wildcard $(srcdir)/*.cpp)
prv_headers:=$(wildcard $(srcdir)/*.hpp)

yuck_source:=$(srcdir)/interface.yuck
yuck_c     :=$(srcdir)/yuck.c
yuck_h     :=$(srcdir)/yuck.h

allfiles:= $(headers) $(cpp_sources) $(prv_headers)

# Object files
ppd_objects :=instructions.o container.o routine.o
gen_objects :=gen-arm.o x86_declarations.o x86_reg.o gen-x86.o
pexe_objects:=gen-pexe.o read-pexe.o
opt_objects :=opt-dead.o

# Prefix the object files
ppd_objects :=$(addprefix $(WORKDIR)/, $(ppd_objects) )
gen_objects :=$(addprefix $(WORKDIR)/, $(gen_objects) )
pexe_objects:=$(addprefix $(WORKDIR)/, $(pexe_objects))
opt_objects :=$(addprefix $(WORKDIR)/, $(opt_objects) )
yuck_o      :=$(addprefix $(WORKDIR)/, $(yuck_o)      )

CPP_OBJECTS:= $(ppd_objects) $(opt_objects) $(gen_objects) $(pexe_objects)


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
	-Wsuggest-attribute=pure -Wsuggest-attribute=const \
	-Wsuggest-attribute=noreturn -Wsuggest-attribute=format \
	-Wtrampolines -Wstack-protector \
	-Wwrite-strings \
	-Wconversion -Wdisabled-optimization -Wno-switch
#	-Wswitch -Wswitch-default -Wswitch-enum

DEBUG_OPT:=

CFLAGS:=  --std=c11   -g $(CWARNINGS)   -I./ -I$(INCDIR) -L$(LIBDIR)
CXXFLAGS:=--std=c++14 -g $(CXXWARNINGS) -I./ -I$(INCDIR) -L$(LIBDIR)
LFLAGS:=#-d
LEX:= flex


################################### TARGETS ####################################


.PHONEY:  debug



################################# PRODUCTIONS ##################################


mpl: $(srcdir)/mpl.cpp libmpl.a $(yuck_h)
	$(CXX) $(CXXFLAGS) -o $@ $(srcdir)/mpl.cpp $(yuck_o) -lmpl

$(yuck_c) $(yuck_h): $(yuck_source)
	yuck gen -H$(yuck_h) -o $(yuck_c) $<

$(CPP_OBJECTS): $(WORKDIR)/%.o: $(srcdir)/%.cpp $(headers) $(prv_headers) | $(WORKDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# working directory
$(WORKDIR):
	mkdir -p $@

install: mpl
	install -C mpl $(BINDIR)


################################## UTILITIES ###################################


cleanfiles:=*.a *.o mpl

.PHONEY: clean todolist count

clean:
	rm -f $(cleanfiles)
	rm -fr $(WORKDIR)

todolist:
	-@for file in $(allfiles:Makefile=); do fgrep -nH -e TODO -e FIXME $$file; done; true

count:
	cat $(allfiles) |line_count


