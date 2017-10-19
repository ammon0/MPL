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
interface_h:=$(srcdir)/interface.h
interface_c:=$(srcdir)/interface.c 


allfiles:= $(headers) $(cpp_sources) $(prv_headers)

# Object files
c_objects  :=interface.o
cpp_objects:=main.o

# Prefix the object files
c_objects  :=$(addprefix $(WORKDIR)/, $(c_objects)  )
cpp_objects:=$(addprefix $(WORKDIR)/, $(cpp_objects))


mpl_objects:= $(c_objects) $(cpp_objects)

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
libs:=-lppd -lmsg


################################### TARGETS ####################################


.PHONEY:  debug

debug: mpl

################################# PRODUCTIONS ##################################


mpl: $(mpl_objects)
	$(CXX) $(CXXFLAGS) -o $@ $(mpl_objects) $(libs)

$(interface_c) $(interface_h): $(yuck_source)
	yuck gen -H$(interface_h) -o $(interface_c) $<

$(cpp_objects): $(WORKDIR)/%.o: $(srcdir)/%.cpp $(headers) $(prv_headers) | $(WORKDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(c_objects): $(WORKDIR)/%.o: $(srcdir)/%.c $(headers) $(prv_headers) | $(WORKDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

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


