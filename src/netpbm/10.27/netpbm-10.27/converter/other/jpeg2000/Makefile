ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/jpeg2000
VPATH=.:$(SRCDIR)/$(SUBDIR)

SUBDIRS = libjasper

include $(BUILDDIR)/Makefile.config

INCLUDES =
ifneq ($(JASPERHDR_DIR),NONE)
  INCLUDES += -I$(JASPERHDR_DIR)
endif

ifneq ($(JASPERHDR_DIR),NONE)
  ifneq ($(JASPERLIB),NONE)
    BINARIES = pamtojpeg2k jpeg2ktopam
  endif
endif

INTERNAL_JASPERLIB = $(BUILDDIR)/$(SUBDIR)/libjasper/libjasper.a
INTERNAL_JASPERHDR_DIR = $(BUILDDIR)/$(SUBDIR)/libjasper/include

ifeq ($(JASPERLIB),$(INTERNAL_JASPERLIB))
  JASPERLIB_DEP = $(JASPERLIB)
else
  # It's not our internal version; user's on his own to make sure it's built
endif


OBJECTS = $(BINARIES:%=%.o)
MERGE_OBJECTS = $(BINARIES:%=%.o2) 
ifeq ($(JASPERLIB),$(INTERNAL_JASPERLIB))
  MERGE_OBJECTS += $(JASPERLIB)
endif
MERGEBINARIES = $(BINARIES)

.PHONY: all
all: $(BINARIES)

HEADERLINKS = pm_config.h pm.h pbm.h pgm.h ppm.h ppmcmap.h \
  pnm.h pam.h shhopt.h nstring.h mallocvar.h

include $(SRCDIR)/Makefile.common

$(BINARIES): %: %.o %-res.o $(JASPERLIB_DEP) $(NETPBMLIB) $(LIBOPT)
	echo deps: $^
	$(LD) $(LDFLAGS) $(MATHLIB) $(LDLIBS) -o $@ $@-res.o $< \
	 `$(LIBOPT) $(NETPBMLIB) $(JASPERLIB)` $(JASPERDEPLIBS) -lm \
	  $(RPATH) $(LADD)

$(INTERNAL_JASPERLIB): FORCE
	$(MAKE) -f $(SRCDIR)/$(SUBDIR)/libjasper/Makefile \
	   -C $(dir $@) $(notdir $@)

distclean: clean

clean: localclean

.PHONY: localclean
localclean:
	$(MAKE) -f $(SRCDIR)/$(SUBDIR)/libjasper/Makefile -C libjasper clean

.PHONY: FORCE
FORCE:
