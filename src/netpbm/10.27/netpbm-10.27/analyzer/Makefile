ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = analyzer
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

# We tend to separate out the build targets so that we don't have
# any more dependencies for a given target than it really needs.
# That way, if there is a problem with a dependency, we can still
# successfully build all the stuff that doesn't depend upon it.
# This package is so big, it's useful even when some parts won't 
# build.

PORTBINARIES = pamfile pamslice pamsumm \
               pgmhist pnmhistmap ppmhist pgmminkowski
MATHBINARIES = pamsharpmap pamsharpness pgmtexture pnmpsnr 

BINARIES = $(PORTBINARIES) $(MATHBINARIES)
SCRIPT =

OBJECTS = $(BINARIES:%=%.o)

# We don't include programs that have special library dependencies in the
# merge scheme, because we don't want those dependencies to prevent us
# from building all the other programs.

MERGEBINARIES = $(BINARIES)
MERGE_OBJECTS = $(MERGEBINARIES:%=%.o2)

.PHONY: all
all: $(BINARIES)

HEADERLINKS = pm_config.h pm.h pbm.h pgm.h ppm.h pnm.h pam.h shhopt.h \
	nstring.h mallocvar.h ppmcmap.h
include $(SRCDIR)/Makefile.common

install.bin: install.bin.local
.PHONY: install.bin.local
install.bin.local: $(PKGDIR)/bin
# Remember that $(SYMLINK) might just be a copy command.
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) pamslice$(EXE) pgmslice$(EXE)
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) pamfile$(EXE) pnmfile$(EXE)

FORCE:

