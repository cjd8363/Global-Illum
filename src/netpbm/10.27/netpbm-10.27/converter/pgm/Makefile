ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/pgm
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

PORTBINARIES =	asciitopgm bioradtopgm fstopgm hipstopgm \
		lispmtopgm pgmtofs pgmtolispm pgmtopgm \
	        psidtopgm spottopgm sbigtopgm
MATHBINARIES =	rawtopgm
BINARIES =	$(PORTBINARIES) $(MATHBINARIES)

OBJECTS = $(BINARIES:%=%.o)
MERGEBINARIES = $(BINARIES)
MERGE_OBJECTS = $(MERGEBINARIES:%=%.o2)

.PHONY: all
all: $(BINARIES)

HEADERLINKS = pm_config.h pm.h pbm.h pgm.h shhopt.h nstring.h mallocvar.h

include $(SRCDIR)/Makefile.common
