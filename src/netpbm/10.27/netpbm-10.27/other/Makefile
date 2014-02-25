ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = other
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

ifneq ($(LINUXSVGALIB),NONE)
  INCLUDES += -I$(LINUXSVGAHDR_DIR)
endif

# We tend to separate out the build targets so that we don't have
# any more dependencies for a given target than it really needs.
# That way, if there is a problem with a dependency, we can still
# successfully build all the stuff that doesn't depend upon it.
# This package is so big, it's useful even when some parts won't 
# build.

PORTBINARIES = pamarith pamchannel pamendian pamlookup \
	pamstack pamsummcol pnmdepth pnmsplit

MATHBINARIES = pnmcolormap 

BINARIES = $(PORTBINARIES) $(MATHBINARIES)

ifneq ($(LINUXSVGALIB),NONE)
  BINARIES += ppmsvgalib
endif

SCRIPTS = ppmtomap

OBJECTS = $(BINARIES:%=%.o)

# We don't include programs that have special library dependencies in the
# merge scheme, because we don't want those dependencies to prevent us
# from building all the other programs.

MERGEBINARIES = $(BINARIES)
MERGE_OBJECTS = $(MERGEBINARIES:%=%.o2)

.PHONY: all
all: $(BINARIES)

HEADERLINKS = pm_config.h pm.h pbm.h pgm.h ppm.h ppmcmap.h \
        pnm.h pam.h pammap.h \
	colorname.h shhopt.h nstring.h mallocvar.h pm_system.h

include $(SRCDIR)/Makefile.common

ppmsvgalib: %: %.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $< \
	  `$(LIBOPT) $(NETPBMLIB)` $(LINUXSVGALIB) $(MATHLIB) $(LDLIBS) \
	  $(LADD) 

install.bin: install.bin.local
.PHONY: install.bin.local
install.bin.local: $(PKGDIR)/bin
# Remember that $(SYMLINK) might just be a copy command.
# backward compatibility: program replaces pnmarith
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) pamarith$(EXE) pnmarith$(EXE)
FORCE:
