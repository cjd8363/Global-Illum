ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

SUBDIRS = jbig pnmtopalm jpeg2000
ifneq ($(BUILD_FIASCO), N)
  SUBDIRS += fiasco
endif

INCLUDES = -I$(SRCDIR)/util 
ifneq ($(TIFFLIB),NONE)
  ifneq ($(TIFFHDR_DIR)x,x)
    INCLUDES += -I$(TIFFHDR_DIR)
  endif
endif
ifneq ($(PNGLIB),NONE)
  ifneq ($(PNGHDR_DIR)x,x)
    INCLUDES += -I$(PNGHDR_DIR)
  endif
  CFLAGS += -DPNG_USE_DLL 
  ifneq ($(ZHDR_DIR)x,x)
    INCLUDES += -I$(ZHDR_DIR)
  endif
endif
ifneq ($(JPEGLIB),NONE)
  ifneq ($(JPEGHDR_DIR)x,x)
    INCLUDES += -I$(JPEGHDR_DIR)
  endif
endif
ifneq ($(URTLIB),NONE)
  ifneq ($(URTHDR_DIR)x,x)
    INCLUDES += -I$(URTHDR_DIR)
  endif
endif

ifeq ($(TIFFLIB),NONE)
  TIFF_PREREQ_MISSING = Y
endif

TIFFLIB_EXTRALIBS =
ifeq ($(TIFFLIB_NEEDS_JPEG),Y)
  ifeq ($(JPEGLIB),NONE)
    TIFF_PREREQ_MISSING = Y
  else
    TIFFLIB_EXTRALIBS += $(JPEGLIB)
  endif
endif
ifeq ($(TIFFLIB_NEEDS_Z),Y)
  ifeq ($(ZLIB),NONE)
    TIFF_PREREQ_MISSING = Y
  else
    TIFFLIB_EXTRALIBS += $(ZLIB)
  endif
endif

PORTBINARIES =  bmptopnm fitstopnm \
		gemtopnm giftopnm hdifftopam infotopam \
		pamtodjvurle \
		pamtohdiff pamtohtmltbl pamtopfm pamtopnm pamtouil \
		pbmtopgm pfmtopam \
	        pgmtopbm pgmtoppm ppmtopgm pnmtoddif pnmtofits \
		pnmtopclxl \
		pnmtosgi pnmtosir pamtotga pnmtoxwd pstopnm \
		sgitopnm sirtopnm xwdtopnm zeisstopnm

BINARIES = $(PORTBINARIES) pnmtorast rasttopnm

ifneq ($(PNGLIB),NONE)
  ifneq ($(ZLIB),NONE)
    BINARIES += pnmtopng pngtopnm
  endif
endif
ifneq ($(JPEGLIB),NONE)
  BINARIES += jpegtopnm pnmtojpeg
endif
ifneq ($(TIFF_PREREQ_MISSING),Y)
  BINARIES += tifftopnm pnmtotiff pnmtotiffcmyk
endif
ifneq ($(URTLIB),NONE)
  BINARIES +=  rletopnm pnmtorle
endif
ifneq ($(ZLIB),NONE)
  BINARIES +=  pnmtops
endif

MERGEBINARIES = $(BINARIES)

EXTRA_OBJECTS = exif.o rast.o pngtxt.o bmepsoe.o
ifneq ($(JPEGLIB),NONE)
  EXTRA_OBJECTS += jpegdatasource.o
endif

OBJECTS = $(BINARIES:%=%.o) $(EXTRA_OBJECTS)
MERGE_OBJECTS = $(MERGEBINARIES:%=%.o) $(EXTRA_OBJECTS)


SCRIPTS = anytopnm pnmtoplainpnm

.PHONY: all
all:	$(BINARIES) $(SUBDIRS:%=%/all)

HEADERLINKS = pm_config.h pm.h pbm.h pgm.h ppm.h ppmcmap.h pnm.h pam.h \
	shhopt.h nstring.h bmp.h tga.h bitio.h ppmcmap.h pammap.h colorname.h \
	mallocvar.h pm_gamma.h

include $(SRCDIR)/Makefile.common

ifeq ($(NEED_RUNTIME_PATH),Y)
  LIBOPTR = -runtime
else
  LIBOPTR =
endif

tifftopnm pnmtotiff pnmtotiffcmyk: %: %.o %-res.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $@.o $@-res.o \
	  `$(LIBOPT) $(NETPBMLIB) $(LIBOPTR) $(TIFFLIB) $(TIFFLIB_EXTRALIBS)` \
	   $(MATHLIB) $(LDLIBS) $(RPATH) $(LADD)

pngtopnm: %: %.o %-res.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $@.o $@-res.o \
	  `$(LIBOPT) $(NETPBMLIB) $(LIBOPTR) $(PNGLIB) $(ZLIB)` \
	  $(MATHLIB) $(LDLIBS) $(RPATH) $(LADD)

pnmtopng: %: %.o %-res.o pngtxt.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $@.o $@-res.o pngtxt.o \
	  `$(LIBOPT) $(NETPBMLIB) $(LIBOPTR) $(PNGLIB) $(ZLIB)` \
	  $(MATHLIB) $(LDLIBS) $(RPATH) $(LADD)

jpegtopnm: %: %.o %-res.o jpegdatasource.o exif.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $< jpegdatasource.o exif.o $@-res.o \
	  `$(LIBOPT) $(NETPBMLIB) $(LIBOPTR) $(JPEGLIB)` \
	  $(MATHLIB) $(LDLIBS) $(RPATH) $(LADD) 

pnmtojpeg: %: %.o %-res.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $@.o $@-res.o \
	  `$(LIBOPT) $(NETPBMLIB) $(LIBOPTR) $(JPEGLIB)` \
	  $(MATHLIB) $(LDLIBS) $(RPATH) $(LADD)

# If URTLIB is BUNDLED_URTLIB, then we're responsible for building it, which
# means it needs to be a dependency:
ifeq ($(URTLIB), $(BUNDLED_URTLIB))
  URTLIBDEP = $(URTLIB)
endif

rletopnm pnmtorle: %: %.o %-res.o $(NETPBMLIB) $(URTLIBDEP) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $@.o $@-res.o \
	  `$(LIBOPT) $(NETPBMLIB) $(URTLIB)` $(MATHLIB) $(LDLIBS) \
	   $(RPATH) $(LADD)

pnmtops: %: %.o %-res.o bmepsoe.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $@.o $@-res.o bmepsoe.o \
	  `$(LIBOPT) $(NETPBMLIB) $(ZLIB)` $(MATHLIB) $(LDLIBS) \
	   $(RPATH) $(LADD)

pnmtorast rasttopnm: %: %.o %-res.o rast.o $(NETPBMLIB) $(LIBOPT)
	$(LD) $(LDFLAGS) -o $@ $@.o $@-res.o rast.o \
	  `$(LIBOPT) $(NETPBMLIB)` $(MATHLIB) $(LDLIBS) \
	   $(RPATH) $(LADD)

install.bin: install.bin.local
.PHONY: install.bin.local
install.bin.local: $(PKGDIR)/bin
# Remember that $(SYMLINK) might just be a copy command.
# backward compatibility: program used to be named pnmnoraw
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) pnmtoplainpnm pnmnoraw
# backward compatibility: program used to be gemtopbm
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) gemtopnm$(EXE) gemtopbm$(EXE)
# In October 2001, pnmtojpeg replaced ppmtojpeg
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) pnmtojpeg$(EXE) ppmtojpeg$(EXE)
# In March 2002, bmptopnm replaced bmptoppm
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) bmptopnm$(EXE) bmptoppm$(EXE)
# In May 2002, pamtouil replaced ppmtouil
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) pamtouil$(EXE) ppmtouil$(EXE)
# In March 2005, we realized that pamtopnm obviates pnmtopnm
	cd $(PKGDIR)/bin ; \
	$(SYMLINK) pamtopnm$(EXE) pnmtopnm$(EXE)
