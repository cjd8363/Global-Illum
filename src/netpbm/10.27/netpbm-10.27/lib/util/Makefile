ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = lib/util
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

CCOPT = $(CFLAGS) $(CFLAGS_SHLIB) $(CADD)

# nstring is required for asprintf(), etc.  Also some systems don't have
# snprintf(), e.g. Solaris 2.5.1.  2002.03.29.
UTILOBJECTS = shhopt.o nstring.o

# We have nothing to contribute to the merge, but must supply at least one
# merge object file.
MERGE_OBJECTS = empty.o

all: $(UTILOBJECTS)

include $(SRCDIR)/Makefile.common

$(UTILOBJECTS):%.o:%.c
	$(CC) -c $(CCOPT) -o $@ $<

testnstring: test.c nstring.h nstring.o
	$(CC) $(CFLAGS) $(CADD) -o $@ nstring.o $<

include Makefile.depend
