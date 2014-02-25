ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/pbm/pbmtoppa
VPATH=.:$(SRCDIR)/$(SUBDIR):$(CSRCDIR)/res

include $(BUILDDIR)/Makefile.config

all: pbmtoppa

BINARIES = pbmtoppa

MERGEBINARIES = $(BINARIES)

OBJECTS = pbmtoppa.o ppa.o pbm.o cutswath.o
MERGE_OBJECTS = pbmtoppa.o2 ppa.o pbm.o cutswath.o

HEADERLINKS = pm_config.h pm.h pbm.h

include $(SRCDIR)/Makefile.common

pbmtoppa: %: $(OBJECTS) %-res.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o pbmtoppa $(OBJECTS) $@-res.o \
	  -lm `$(LIBOPT) $(NETPBMLIB)` $(LDLIBS) \
	  $(RPATH) $(LADD)

