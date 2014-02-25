ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/pnmtopalm
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

BINARIES = palmtopnm pnmtopalm
SCRIPTS =
OBJECTS = $(BINARIES:%=%.o) palmcolormap.o
MERGE_OBJECTS = $(BINARIES:%=%.o2) palmcolormap.o
MERGEBINARIES = $(BINARIES)
DATAFILES = palmcolor8.map palmgray1.map palmgray2.map palmgray4.map

all: $(BINARIES)

HEADERLINKS = pm_config.h pm.h pbm.h pgm.h ppm.h ppmcmap.h pnm.h \
	shhopt.h mallocvar.h

include $(SRCDIR)/Makefile.common

$(BINARIES): %: %.o %-res.o palmcolormap.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $< $@-res.o palmcolormap.o `$(LIBOPT) $(NETPBMLIB)` \
	  $(MATHLIB) $(LDLIBS) $(RPATH) $(LADD)

gen_palm_colormap : $(SUBDIR)/gen_palm_colormap.c palmcolormap.o %-res.o
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) -o $@ $@-res.o $< palmcolormap.o \
	  `$(LIBOPT) $(NETPBMLIB)` $(MATHLIB) $(LDLIBS) $(LADD)


clean: cleanspecial
.PHONY: cleanspecial
cleanspecial:
	rm -f gen_palm_colormap
