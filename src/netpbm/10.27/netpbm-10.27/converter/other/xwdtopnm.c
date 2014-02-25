/* xwdtopnm.c - read an X11 or X10 window dump file and write a portable anymap
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/* IMPLEMENTATION NOTES:

   The file X11/XWDFile.h from the X Window System is an authority for the
   format of an XWD file.  Netpbm uses its own declaration, though.
*/


#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <stdio.h>
#include <string.h>
#include "pnm.h"
#include "x10wd.h"
#include "x11wd.h"
#include "shhopt.h"
#include "mallocvar.h"

static struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    char *input_filename;
    unsigned int verbose;
    unsigned int debug;
    unsigned int headerdump;
} cmdline;


static bool debug;
static bool verbose;

#ifdef DEBUG_PIXEL
static unsigned int pixel_count = 0;
#endif

struct row_control {
    /* This structure contains the state of the getpix() reader as it
       reads across a row in the input image.
       */
    union {
        /* The current item (the item most recently read from the
           image file.  Meaningless if 'bits_left' == 0.
           
           Which of the members applies depends on the bits per item of
           the input image.  
        */
        long l;    /* 32 bits per item */
        short s;   /* 16 bits per item */
        char c;    /*  8 bits per item */
    } item;
    int bits_used;
      /* This is the number of bits in the current item that have already
         been returned as part of a pixel for a previous call or saved
         as carryover_bits for use in a future call.
         */
    int bits_left;
      /* This is the number of bits in the current item that have not yet
         been returned as part of a pixel or saved as carryover bits.
         */
    unsigned long carryover_bits;
      /* This is the odd bits left over from one item that need to be
         combined with more bits from the next item to form a pixel
         value (the pixel is split between two items).  An item is a
         unit of data read from the image file.  

         It is a 32 bit bitstring with the carryover bits right-justified
         in it.  
      */
    int bits_carried_over;
      /* This is the number of carryover bits in 'carryover_bits' */
};

static short bs_short ARGS(( short s ));
static long bs_long ARGS(( long l ));


static int
zero_bits(const unsigned long mask) {
/*----------------------------------------------------------------------------
   Return the number of consective zero bits at the least significant end
   of the binary representation of 'mask'.  E.g. if mask == 0x00fff800,
   we would return 11.
-----------------------------------------------------------------------------*/
    int i;
    unsigned long shifted_mask;

    for (i=0, shifted_mask = mask; 
         i < sizeof(mask)*8 && (shifted_mask & 0x00000001) == 0;
         i++, shifted_mask >>= 1 );
    return(i);
}



static int
one_bits(const unsigned long input) {
/*----------------------------------------------------------------------------
   Return the number of one bits in the binary representation of 'input'.
-----------------------------------------------------------------------------*/
    int one_bits;
    unsigned long mask;

    one_bits = 0;   /* initial value */
    for (mask = 0x00000001; mask != 0x00000000; mask <<= 1)
        if (input & mask) one_bits++;

    return(one_bits);
}



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdline_info *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdline_p structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */

    OPTENT3(0,   "verbose",    OPT_FLAG,   NULL, &cmdlineP->verbose,       0);
    OPTENT3(0,   "debug",      OPT_FLAG,   NULL, &cmdlineP->debug,         0);
    OPTENT3(0,   "headerdump", OPT_FLAG,   NULL, &cmdlineP->headerdump,    0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc - 1 == 0)
        cmdlineP->input_filename = NULL;  /* he wants stdin */
    else if (argc - 1 == 1) {
        if (strcmp(argv[1], "-") == 0)
            cmdlineP->input_filename = NULL;  /* he wants stdin */
        else 
            cmdlineP->input_filename = strdup(argv[1]);
    } else 
        pm_error("Too many arguments.  The only argument accepted\n"
                 "is the input file specificaton");
}



static void
processX10Header(X10WDFileHeader *  const h10P, 
                 FILE *             const file,
                 int *              const colsP, 
                 int *              const rowsP, 
                 int *              const padrightP, 
                 xelval *           const maxvalP, 
                 enum visualclass * const visualclassP, 
                 int *              const formatP, 
                 xel **             const colorsP, 
                 int *              const bits_per_pixelP, 
                 int *              const bits_per_itemP, 
                 unsigned long *    const red_maskP, 
                 unsigned long *    const green_maskP, 
                 unsigned long *    const blue_maskP,
                 enum byteorder *   const byte_orderP,
                 enum byteorder *   const bit_orderP) {

    int i;
    X10Color* x10colors;
    bool grayscale;
    bool byte_swap;

    *maxvalP = 65535;   /* Initial assumption */

    if ( h10P->file_version != X10WD_FILE_VERSION ) {
        byte_swap = TRUE;
        h10P->header_size = bs_long( h10P->header_size );
        h10P->file_version = bs_long( h10P->file_version );
        h10P->display_type = bs_long( h10P->display_type );
        h10P->display_planes = bs_long( h10P->display_planes );
        h10P->pixmap_format = bs_long( h10P->pixmap_format );
        h10P->pixmap_width = bs_long( h10P->pixmap_width );
        h10P->pixmap_height = bs_long( h10P->pixmap_height );
        h10P->window_width = bs_short( h10P->window_width );
        h10P->window_height = bs_short( h10P->window_height );
        h10P->window_x = bs_short( h10P->window_x );
        h10P->window_y = bs_short( h10P->window_y );
        h10P->window_bdrwidth = bs_short( h10P->window_bdrwidth );
        h10P->window_ncolors = bs_short( h10P->window_ncolors );
    } else
        byte_swap = FALSE;

    for ( i = 0; i < h10P->header_size - sizeof(*h10P); ++i )
        if ( getc( file ) == EOF )
            pm_error( "couldn't read rest of X10 XWD file header" );

    /* Check whether we can handle this dump. */
    if ( h10P->window_ncolors > 256 )
        pm_error( "can't handle X10 window_ncolors > %d", 256 );
    if ( h10P->pixmap_format != ZFormat && h10P->display_planes != 1 )
        pm_error(
            "can't handle X10 pixmap_format %d with planes != 1",
            h10P->pixmap_format );

    grayscale = TRUE;  /* initial assumption */
    if ( h10P->window_ncolors != 0 ) {
        /* Read X10 colormap. */
        MALLOCARRAY( x10colors, h10P->window_ncolors );
        if ( x10colors == NULL )
            pm_error( "out of memory" );
        for ( i = 0; i < h10P->window_ncolors; ++i ) {
            if ( fread( &x10colors[i], sizeof(X10Color), 1, file ) != 1 )
                pm_error( "couldn't read X10 XWD colormap" );
            if ( byte_swap ) {
                x10colors[i].red = bs_short( x10colors[i].red );
                x10colors[i].green = bs_short( x10colors[i].green );
                x10colors[i].blue = bs_short( x10colors[i].blue );
            }
            if ( x10colors[i].red != x10colors[i].green ||
                 x10colors[i].green != x10colors[i].blue )
                grayscale = FALSE;
        }
    }

    if ( h10P->display_planes == 1 ) {
        *formatP = PBM_TYPE;
        *visualclassP = StaticGray;
        *maxvalP = 1;
        *colorsP = pnm_allocrow( 2 );
        PNM_ASSIGN1( (*colorsP)[0], 0 );
        PNM_ASSIGN1( (*colorsP)[1], *maxvalP );
        overflow_add(h10P->pixmap_width, 15);
        if(h10P->pixmap_width < 0)
            pm_error("assert: negative width");
        *padrightP =
            ( ( h10P->pixmap_width + 15 ) / 16 ) * 16 - h10P->pixmap_width;
        *bits_per_itemP = 16;
        *bits_per_pixelP = 1;
    } else if ( h10P->window_ncolors == 0 ) { 
        /* Must be grayscale. */
        *formatP = PGM_TYPE;
        *visualclassP = StaticGray;
        *maxvalP = ( 1 << h10P->display_planes ) - 1;
        overflow_add(*maxvalP, 1);
        *colorsP = pnm_allocrow( *maxvalP + 1 );
        for ( i = 0; i <= *maxvalP; ++i )
            PNM_ASSIGN1( (*colorsP)[i], i );
        overflow_add(h10P->pixmap_width, 15);
        if(h10P->pixmap_width < 0)
            pm_error("assert: negative width");
        *padrightP =
            ( ( h10P->pixmap_width + 15 ) / 16 ) * 16 - h10P->pixmap_width;
        *bits_per_itemP = 16;
        *bits_per_pixelP = 1;
    } else {
        *colorsP = pnm_allocrow( h10P->window_ncolors );
        *visualclassP = PseudoColor;
        if ( grayscale ) {
            *formatP = PGM_TYPE;
            for ( i = 0; i < h10P->window_ncolors; ++i )
                PNM_ASSIGN1( (*colorsP)[i], x10colors[i].red );
        } else {
            *formatP = PPM_TYPE;
            for ( i = 0; i < h10P->window_ncolors; ++i )
                PPM_ASSIGN(
                    (*colorsP)[i], x10colors[i].red, x10colors[i].green,
                    x10colors[i].blue);
        }

        *padrightP = h10P->pixmap_width & 1;
        *bits_per_itemP = 8;
        *bits_per_pixelP = 8;
    }
    *colsP = h10P->pixmap_width;
    *rowsP = h10P->pixmap_height;
    *byte_orderP = MSBFirst;
    *bit_orderP = LSBFirst;
}



static void
fixH11ByteOrder(X11WDFileHeader *  const h11P,
                X11WDFileHeader ** const h11FixedPP) {

    X11WDFileHeader * h11FixedP;

    MALLOCVAR_NOFAIL(h11FixedP);

    if (h11P->file_version == X11WD_FILE_VERSION) {
        memcpy(h11FixedP, h11P, sizeof(*h11FixedP));
    } else {
        h11FixedP->header_size = bs_long( h11P->header_size );
        h11FixedP->file_version = bs_long( h11P->file_version );
        h11FixedP->pixmap_format = bs_long( h11P->pixmap_format );
        h11FixedP->pixmap_depth = bs_long( h11P->pixmap_depth );
        h11FixedP->pixmap_width = bs_long( h11P->pixmap_width );
        h11FixedP->pixmap_height = bs_long( h11P->pixmap_height );
        h11FixedP->xoffset = bs_long( h11P->xoffset );
        h11FixedP->byte_order = bs_long( h11P->byte_order );
        h11FixedP->bitmap_unit = bs_long( h11P->bitmap_unit );
        h11FixedP->bitmap_bit_order = bs_long( h11P->bitmap_bit_order );
        h11FixedP->bitmap_pad = bs_long( h11P->bitmap_pad );
        h11FixedP->bits_per_pixel = bs_long( h11P->bits_per_pixel );
        h11FixedP->bytes_per_line = bs_long( h11P->bytes_per_line );
        h11FixedP->visual_class = bs_long( h11P->visual_class );
        h11FixedP->red_mask = bs_long( h11P->red_mask );
        h11FixedP->green_mask = bs_long( h11P->green_mask );
        h11FixedP->blue_mask = bs_long( h11P->blue_mask );
        h11FixedP->bits_per_rgb = bs_long( h11P->bits_per_rgb );
        h11FixedP->colormap_entries = bs_long( h11P->colormap_entries );
        h11FixedP->ncolors = bs_long( h11P->ncolors );
        h11FixedP->window_width = bs_long( h11P->window_width );
        h11FixedP->window_height = bs_long( h11P->window_height );
        h11FixedP->window_x = bs_long( h11P->window_x );
        h11FixedP->window_y = bs_long( h11P->window_y );
        h11FixedP->window_bdrwidth = bs_long( h11P->window_bdrwidth );
    }
    *h11FixedPP = h11FixedP;
}



static void
readX11Colormap(FILE *      const file,
                int         const ncolors, 
                bool        const byteSwap,
                X11XColor** const x11colorsP) {
                
    X11XColor* x11colors;
    int rc;

    /* Read X11 colormap. */
    MALLOCARRAY(x11colors, ncolors);
    if (x11colors == NULL)
        pm_error("out of memory");
    rc = fread(x11colors, sizeof(X11XColor), ncolors, file);
    if (rc != ncolors)
        pm_error("couldn't read X11 XWD colormap");
    if (byteSwap) {
        unsigned int i;
        for (i = 0; i < ncolors; ++i) {
            x11colors[i].red   = bs_short(x11colors[i].red);
            x11colors[i].green = bs_short(x11colors[i].green);
            x11colors[i].blue  = bs_short(x11colors[i].blue);
        }
    }
    if (debug) {
        unsigned int i;
        for (i = 0; i < ncolors && i < 8; ++i)
            pm_message("Color %d r/g/b = %d/%d/%d", i, 
                       x11colors[i].red, x11colors[i].green, 
                       x11colors[i].blue);
    }
    *x11colorsP = x11colors;
}



static bool
colormapAllGray(const X11XColor* const x11colors,
                unsigned int     const ncolors) {

    unsigned int i;
    bool grayscale;

    grayscale = TRUE;  /* initial assumption */
    for (i = 0; i < ncolors; ++i) 
        if (x11colors[i].red != x11colors[i].green ||
            x11colors[i].green != x11colors[i].blue )
            grayscale = FALSE;

    return grayscale;
}



static void
dumpX11Header(X11WDFileHeader * const h11P) {

    const char * formatDesc;
    const char * byteOrderDesc;
    const char * bitOrderDesc;
    const char * visualClassDesc;

    X11WDFileHeader * h11FixedP;

    fixH11ByteOrder(h11P, &h11FixedP);
 
    switch(h11FixedP->pixmap_format) {
    case XYBitmap: formatDesc = "XY bit map"; break;
    case XYPixmap: formatDesc = "XY pix map"; break;
    case ZPixmap:  formatDesc = "Z pix map";  break;
    default:       formatDesc = "???";
    }

    switch(h11FixedP->byte_order) {
    case LSBFirst: byteOrderDesc = "LSB first"; break;
    case MSBFirst: byteOrderDesc = "MSB first"; break;
    default:       byteOrderDesc = "???";
    }

    switch (h11FixedP->bitmap_bit_order) {
    case LSBFirst: bitOrderDesc = "LSB first"; break;
    case MSBFirst: bitOrderDesc = "MSB first"; break;
    default:       bitOrderDesc = "???";
    }

    switch (h11FixedP->visual_class) {
    case StaticGray:  visualClassDesc = "StaticGray";  break;
    case GrayScale:   visualClassDesc = "GrayScale";   break;
    case StaticColor: visualClassDesc = "StaticColor"; break;
    case PseudoColor: visualClassDesc = "PseudoColor"; break;
    case TrueColor:   visualClassDesc = "TrueColor"; break;
    case DirectColor: visualClassDesc = "DirectColor"; break;
    default:          visualClassDesc = "???";
    }

    pm_message("File version: %u", h11FixedP->file_version);
    pm_message("Format: %s (%u)", formatDesc, h11FixedP->pixmap_format);
    pm_message("Width:  %u", h11FixedP->pixmap_width);
    pm_message("Height: %u", h11FixedP->pixmap_height);
    pm_message("Depth: %u bits", h11FixedP->pixmap_depth);
    pm_message("X offset: %u", h11FixedP->xoffset);
    pm_message("byte order: %s (%u)", byteOrderDesc, h11FixedP->byte_order);
    pm_message("bitmap unit: %u", h11FixedP->bitmap_unit);
    pm_message("bit order: %s (%u)", 
               bitOrderDesc, h11FixedP->bitmap_bit_order);
    pm_message("bitmap pad: %u", h11FixedP->bitmap_pad);
    pm_message("bits per pixel: %u", h11FixedP->bits_per_pixel);
    pm_message("bytes per line: %u", h11FixedP->bytes_per_line);
    pm_message("visual class: %s (%u)", 
               visualClassDesc, h11FixedP->visual_class);
    pm_message("red mask:   %08x", h11FixedP->red_mask);
    pm_message("green mask: %08x", h11FixedP->green_mask);
    pm_message("blue mask:  %08x", h11FixedP->blue_mask);
    pm_message("bits per rgb: %u", h11FixedP->bits_per_rgb);
    pm_message("number of colormap entries: %u", h11FixedP->colormap_entries);
    pm_message("number of colors in colormap: %u", h11FixedP->ncolors);
    pm_message("window width:  %u", h11FixedP->window_width);
    pm_message("window height: %u", h11FixedP->window_height);
    pm_message("window upper left X coordinate: %u", h11FixedP->window_x);
    pm_message("window upper left Y coordinate: %u", h11FixedP->window_y);
    pm_message("window border width: %u", h11FixedP->window_bdrwidth);

    free(h11FixedP);
}



static void
processX11Header(X11WDFileHeader *  const h11P, 
                 FILE *             const file,
                 int *              const colsP, 
                 int *              const rowsP, 
                 int *              const padrightP, 
                 xelval *           const maxvalP, 
                 enum visualclass * const visualclassP, 
                 int *              const formatP, 
                 xel **             const colorsP, 
                 int *              const bits_per_pixelP, 
                 int *              const bits_per_itemP, 
                 unsigned long *    const red_maskP, 
                 unsigned long *    const green_maskP, 
                 unsigned long *    const blue_maskP,
                 enum byteorder *   const byte_orderP,
                 enum byteorder *   const bit_orderP) {

    int i;
    X11XColor* x11colors;
    bool grayscale;
    bool const byte_swap = (h11P->file_version != X11WD_FILE_VERSION);
    X11WDFileHeader * h11FixedP;
 
    fixH11ByteOrder(h11P, &h11FixedP);

    if (byte_swap && verbose)
        pm_message("Input is different endianness from this machine.");
    
    for (i = 0; i < h11FixedP->header_size - sizeof(*h11FixedP); ++i)
        if (getc(file) == EOF)
            pm_error("couldn't read rest of X11 XWD file header");

    /* Check whether we can handle this dump. */
    if ( h11FixedP->pixmap_depth > 24 )
        pm_error( "can't handle X11 pixmap_depth > 24" );
    if ( h11FixedP->bits_per_rgb > 24 )
        pm_error( "can't handle X11 bits_per_rgb > 24" );
    if ( h11FixedP->pixmap_format != ZPixmap && h11FixedP->pixmap_depth != 1 )
        pm_error(
            "can't handle X11 pixmap_format %d with depth != 1",
            h11FixedP->pixmap_format );
    if ( h11FixedP->bitmap_unit != 8 && h11FixedP->bitmap_unit != 16 &&
         h11FixedP->bitmap_unit != 32 )
        pm_error(
            "X11 bitmap_unit (%d) is non-standard - can't handle",
            h11FixedP->bitmap_unit );
    /* The following check was added in 10.19 (November 2003) */
    if ( h11FixedP->bitmap_pad != 8 && h11FixedP->bitmap_pad != 16 &&
         h11FixedP->bitmap_pad != 32 )
        pm_error(
            "X11 bitmap_pad (%d) is non-standard - can't handle",
            h11FixedP->bitmap_unit );

    if ( h11FixedP->ncolors > 0 ) {
        readX11Colormap( file, h11FixedP->ncolors, byte_swap, &x11colors );
        grayscale = colormapAllGray( x11colors, h11FixedP->ncolors );
    } else
        grayscale = TRUE;

    *visualclassP = (enum visualclass) h11FixedP->visual_class;
    if ( *visualclassP == DirectColor ) {
        unsigned int i;
        *formatP = PPM_TYPE;
        *maxvalP = 65535;
        /*
          DirectColor is like PseudoColor except that there are essentially
          3 colormaps (shade maps) -- one for each color component.  Each pixel
          is composed of 3 separate indices.
        */

        *colorsP = pnm_allocrow( h11FixedP->ncolors );
        for ( i = 0; i < h11FixedP->ncolors; ++i )
            PPM_ASSIGN(
                (*colorsP)[i], x11colors[i].red, x11colors[i].green,
                x11colors[i].blue);
    } else if ( *visualclassP == TrueColor ) {
        *formatP = PPM_TYPE;

        *maxvalP = pm_lcm(pm_bitstomaxval(one_bits(h11FixedP->red_mask)),
                          pm_bitstomaxval(one_bits(h11FixedP->green_mask)),
                          pm_bitstomaxval(one_bits(h11FixedP->blue_mask)),
                          PPM_OVERALLMAXVAL
            );
    }
    else if ( *visualclassP == StaticGray && h11FixedP->bits_per_pixel == 1 ) {
        *formatP = PBM_TYPE;
        *maxvalP = 1;
        *colorsP = pnm_allocrow( 2 );
        PNM_ASSIGN1( (*colorsP)[0], *maxvalP );
        PNM_ASSIGN1( (*colorsP)[1], 0 );
    } else if ( *visualclassP == StaticGray ) {
        unsigned int i;
        *formatP = PGM_TYPE;
        *maxvalP = ( 1 << h11FixedP->bits_per_pixel ) - 1;
        *colorsP = pnm_allocrow( *maxvalP + 1 );
        for ( i = 0; i <= *maxvalP; ++i )
            PNM_ASSIGN1( (*colorsP)[i], i );
    } else {
        *colorsP = pnm_allocrow( h11FixedP->ncolors );
        if ( grayscale ) {
            unsigned int i;
            *formatP = PGM_TYPE;
            for ( i = 0; i < h11FixedP->ncolors; ++i )
                PNM_ASSIGN1( (*colorsP)[i], x11colors[i].red );
        } else {
            unsigned int i;
            *formatP = PPM_TYPE;
            for ( i = 0; i < h11FixedP->ncolors; ++i )
                PPM_ASSIGN(
                    (*colorsP)[i], x11colors[i].red, x11colors[i].green,
                    x11colors[i].blue);
        }
        *maxvalP = 65535;
    }

    *colsP = h11FixedP->pixmap_width;
    *rowsP = h11FixedP->pixmap_height;
    overflow2(h11FixedP->bytes_per_line, 8);
    *padrightP =
        h11FixedP->bytes_per_line * 8 / h11FixedP->bits_per_pixel -
        h11FixedP->pixmap_width;
    /* According to X11/XWDFile.h, the item size is 'bitmap_pad' for some
       images and 'bitmap_unit' for others.  This is strange, so there may
       be some subtlety of their definitions that we're missing.

       See comments in getpix() about what an item is.

       Ben Kelley in January 2002 had a 32 bits-per-pixel xwd file
       from a truecolor 32 bit window on a Hummingbird Exceed X server
       on Win32, and it had bitmap_unit = 8 and bitmap_pad = 32.

       But Björn Eriksson in October 2003 had an xwd file from a 24
       bit-per-pixel direct color window that had bitmap_unit = 32 and
       bitmap_pad = 8.  This was made by Xwd in Red Hat Xfree86 4.3.0-2.

       Before Netpbm 9.23 (January 2002), we used bitmap_unit as the
       item size always.  Then, until 10.19 (November 2003), we used
       bitmap_pad when pixmap_depth > 1 and pixmap_format == ZPixmap.
       We still don't see any logic in these fields at all, but we
       figure whichever one is greater (assuming both are meaningful)
       has to be the item size.  
    */
    *bits_per_itemP = MAX(h11FixedP->bitmap_pad, h11FixedP->bitmap_unit);
    if (h11FixedP->bits_per_pixel > *bits_per_itemP)
        pm_error("Invalid xwd.  bits per pixel (%u) is greater than the "
                 "bits per item (%u)", 
                 h11FixedP->bits_per_pixel, *bits_per_itemP);

    *bits_per_pixelP = h11FixedP->bits_per_pixel;

    *byte_orderP = (enum byteorder) h11FixedP->byte_order;
    *bit_orderP = (enum byteorder) h11FixedP->bitmap_bit_order;
    *red_maskP = h11FixedP->red_mask;
    *green_maskP = h11FixedP->green_mask;
    *blue_maskP = h11FixedP->blue_mask;

    free(h11FixedP);
} 



static void
getinit(FILE *             const file, 
        int *              const colsP, 
        int *              const rowsP, 
        int *              const padrightP, 
        xelval *           const maxvalP, 
        enum visualclass * const visualclassP, 
        int *              const formatP, 
        xel **             const colorsP,
        int *              const bits_per_pixelP, 
        int *              const bits_per_itemP, 
        unsigned long *    const red_maskP, 
        unsigned long *    const green_maskP,
        unsigned long *    const blue_maskP,
        enum byteorder *   const byte_orderP,
        enum byteorder *   const bit_orderP,
        bool               const headerDump) {

    /* Assume X11 headers are larger than X10 ones. */
    unsigned char header[sizeof(X11WDFileHeader)];
    X10WDFileHeader* h10P;
    X11WDFileHeader* h11P;
    int rc;

    h10P = (X10WDFileHeader*) header;
    h11P = (X11WDFileHeader*) header;
    if ( sizeof(*h10P) > sizeof(*h11P) )
        pm_error("ARGH!  On this machine, X10 headers are larger than "
                 "X11 headers!\n    You will have to re-write xwdtopnm." );
    
    /* We read an X10 header's worth of data from the file, then look
       at it to see if it looks like an X10 header.  If so we process
       the X10 header.  If not, but it looks like the beginning of an
       X11 header, we read more bytes so we have an X11 header's worth
       of data, then process the X11 header.  Otherwise, we raise an
       error.  
    */

    rc = fread(&header[0], sizeof(*h10P), 1, file);
    if (rc != 1)
        pm_error( "couldn't read XWD file header" );

    if (h10P->file_version == X10WD_FILE_VERSION ||
        bs_long(h10P->file_version) == X10WD_FILE_VERSION) {
        
        if (verbose)
            pm_message("Input is X10");
        processX10Header(h10P, file, colsP, rowsP, padrightP, maxvalP, 
                         visualclassP, formatP, 
                         colorsP, bits_per_pixelP, bits_per_itemP, 
                         red_maskP, green_maskP, blue_maskP, 
                         byte_orderP, bit_orderP);
    } else if (h11P->file_version == X11WD_FILE_VERSION ||
               bs_long(h11P->file_version) == X11WD_FILE_VERSION) {
        
        int rc;

        if (verbose)
            pm_message("Input is X11");

        /* Read the balance of the X11 header */
        rc = fread(&header[sizeof(*h10P)], 
                   sizeof(*h11P) - sizeof(*h10P), 1, file);
        if (rc != 1)
            pm_error("couldn't read end of X11 XWD file header");

        if (headerDump)
            dumpX11Header(h11P);

        processX11Header(h11P, file, colsP, rowsP, padrightP, maxvalP, 
                         visualclassP, formatP, 
                         colorsP, bits_per_pixelP, bits_per_itemP, 
                         red_maskP, green_maskP, blue_maskP, 
                         byte_orderP, bit_orderP);
    } else
        pm_error("unknown XWD file version: %u", h11P->file_version);
}



#ifdef DEBUG_PIXEL
#define DEBUG_PIXEL_1 \
   if (pixel_count < 4) \
       pm_message("getting pixel %d", pixel_count);

#define DEBUG_PIXEL_2 \
    if (pixel_count < 4) \
        pm_message("item: %.8lx", row_controlP->item.l); 


#define DEBUG_PIXEL_3 \
    if (pixel_count < 4) \
        pm_message("  bits_taken: %lx(%d), carryover_bits: %lx(%d), " \
                     "pixel: %lx", \
                     bits_taken, bits_to_take, row_controlP->carryover_bits, \
                     row_controlP->bits_carried_over, pixel);

#define DEBUG_PIXEL_4 \
    if (pixel_count < 4) { \
        pm_message("  row_control.bits_carried_over = %d" \
                   "  carryover_bits= %.8lx", \
                   row_controlP->bits_carried_over, \
                   row_controlP->carryover_bits); \
        pm_message("  row_control.bits_used = %d", \
                   row_controlP->bits_used); \
        pm_message("  row_control.bits_left = %d", \
                   row_controlP->bits_left); \
                   } \
    \
    pixel_count++;
#else
#define DEBUG_PIXEL_1 do {} while(0)
#define DEBUG_PIXEL_2 do {} while(0)
#define DEBUG_PIXEL_3 do {} while(0)
#define DEBUG_PIXEL_4 do {} while(0)
#endif


static unsigned long
getpix(FILE *file, struct row_control * const row_controlP,
       const int bits_per_pixel, const int bits_per_item, 
       const enum byteorder byte_order, const enum byteorder bit_order) {
/*----------------------------------------------------------------------------
   Get a pixel from the input image.

   A pixel is a bit string.  It may be either an rgb triplet or an index
   into the colormap (or even an rgb triplet of indices into the colormaps!).
   We don't care -- it's just a bit string.
   
   The basic unit of storage in the input file is an "item."  An item
   can be 1, 2, or 4 bytes, and 'bits_per_item' tells us which.  Each
   item can have its bytes stored in forward or reverse order, and
   'byte_order' tells us which.

   Each item can contain one or more pixels, and may contain
   fractional pixels.  'bits_per_pixel' tells us how many bits each
   pixel has, and 'bits_per_pixel' is always less than
   'bits_per_item', but not necessarily a factor of it.  Within an item,
   after taking care of the endianness of its storage format, the pixels
   may be arranged from left to right or right to left.  'bit_order' tells
   us which.

   But it's not that simple.  Each individual row contains an integral
   number of items.  This means the last item in the row may contain
   padding bits.  The padding is on the right or left, according to
   'bit_order'.
   
   The most difficult part of getting the pixel is the ones that span
   items.  We detect when an item has part, but not all, of the next
   pixel in it and store the fragment as "carryover" bits for use in the
   next call to this function.

   All this state information (carryover bits, etc.) is kept in 
   *row_controlP.

-----------------------------------------------------------------------------*/
    unsigned long pixel;
      /* This is the pixel value we ultimately return.  It is a 32 bit
         bitstring, with the pixel value right-justified in it.
         */

    int bits_to_take;
      /* This is the number of bits we need to take from the current
         item to combine with carryover bits to form a pixel.
         */
    DEBUG_PIXEL_1;

    if ( row_controlP->bits_left <= 0) {
        /* We've used up the most recently read item.  Get the next one
           from the input file.
           */
        switch ( bits_per_item )
        {
        case 8:
            row_controlP->item.c = getc( file );
            break;

        case 16:
            switch (byte_order) {
            case MSBFirst:
                if (pm_readbigshort(file, &row_controlP->item.s) == -1)
                    pm_error( "error reading image" );
                break;
            case LSBFirst:
                if (pm_readlittleshort(file, &row_controlP->item.s) == -1)
                    pm_error( "error reading image" );
                break;
            }
            break;

        case 32:
            switch (byte_order) {
            case MSBFirst:
                if (pm_readbiglong(file, &row_controlP->item.l) == -1)
                    pm_error( "error reading image" );
                break;
            case LSBFirst:
                if (pm_readlittlelong(file, &row_controlP->item.l) == -1) 
                    pm_error( "error reading image");
                break;
            }
            break;

        default:
            pm_error( "can't happen" );
        }
        DEBUG_PIXEL_2;

        row_controlP->bits_used = 0;  
          /* Fresh item from file; none of it used yet. */
        row_controlP->bits_left = bits_per_item;
    }
    
    bits_to_take = bits_per_pixel - row_controlP->bits_carried_over;

    { 
        unsigned long bits_to_take_mask;
        static unsigned long bits_taken;
          /* The bits we took from the current item:  'bits_to_take' bits
             right-justified in a 32 bit bitstring.
          */
        int bit_shift;
          /* How far to shift item to get the bits we're taking
             right-justified 
          */

        if (bit_order == MSBFirst)
            bit_shift = row_controlP->bits_left - bits_to_take;
        else
            bit_shift = row_controlP->bits_used;

        if ( bits_to_take == sizeof(bits_to_take_mask) * 8 )
            bits_to_take_mask = -1;
        else
            bits_to_take_mask = ( 1 << bits_to_take ) - 1;
        
        switch ( bits_per_item ) {
        case 8:
            bits_taken = (row_controlP->item.c >> bit_shift) 
                & bits_to_take_mask;
            break;
        case 16:
            bits_taken = (row_controlP->item.s >> bit_shift) 
                & bits_to_take_mask;
            break;
        case 32:
            bits_taken = (row_controlP->item.l >> bit_shift) 
                & bits_to_take_mask;
            break;
        }

        /* Now combine any carried over bits with the bits just taken */
        if (bit_order == MSBFirst)
            pixel = bits_taken | row_controlP->carryover_bits << bits_to_take;
        else
            pixel = bits_taken << row_controlP->bits_carried_over |
                row_controlP->carryover_bits;

        DEBUG_PIXEL_3;
    }

    row_controlP->bits_used += bits_to_take;
    row_controlP->bits_left -= bits_to_take;

    if (row_controlP->bits_left > 0 && 
        row_controlP->bits_left < bits_per_pixel) {
        /* Part, but not all, of the next pixel is in this item. Get it
           into carryover_bits and then mark this item all used up.
           */
        unsigned long bits_left_mask;
        int bit_shift;
           /* How far to shift item to get the bits that are left
              right-justified 
           */
        if ( row_controlP->bits_left == sizeof(bits_left_mask) * 8 )
            bits_left_mask = -1;  /* All one bits */
        else
            bits_left_mask = ( 1 << row_controlP->bits_left ) - 1;

        if (bit_order == MSBFirst)
            bit_shift = 0;
        else
            bit_shift = row_controlP->bits_used;

        switch ( bits_per_item ) {
        case 8:
            row_controlP->carryover_bits = 
                (row_controlP->item.c >> bit_shift) & bits_left_mask;
            break;
        case 16:
            row_controlP->carryover_bits = 
                (row_controlP->item.s >> bit_shift) & bits_left_mask;
            break;
        case 32:
            row_controlP->carryover_bits = 
                (row_controlP->item.l >> bit_shift) & bits_left_mask;
            break;
        }
        row_controlP->bits_carried_over = row_controlP->bits_left;
        row_controlP->bits_used = bits_per_item;
        row_controlP->bits_left = 0;
    } else {
        row_controlP->carryover_bits = 0;
        row_controlP->bits_carried_over = 0;
    }
    DEBUG_PIXEL_4;
    return pixel;
}



static void
reportInfo(int              const cols, 
           int              const rows, 
           int              const padright, 
           xelval           const maxval, 
           enum visualclass const visualclass,
           int              const format, 
           int              const bits_per_pixel,
           int              const bits_per_item, 
           int              const red_mask, 
           int              const green_mask, 
           int              const blue_mask,
           enum byteorder   const byte_order, 
           enum byteorder   const bit_order) {
    
    const char *visualclass_name;
    const char *byte_order_name;
    const char *bit_order_name;
    switch (visualclass) {
    case StaticGray:  visualclass_name="StaticGray";  break;
    case GrayScale:   visualclass_name="Grayscale";   break;
    case StaticColor: visualclass_name="StaticColor"; break;
    case PseudoColor: visualclass_name="PseudoColor"; break;
    case TrueColor:   visualclass_name="TrueColor";   break;
    case DirectColor: visualclass_name="DirectColor"; break;
    default:          visualclass_name="(invalid)";    break;
    }
    switch (byte_order) {
    case MSBFirst: byte_order_name = "MSBFirst";  break;
    case LSBFirst: byte_order_name = "LSBFirst";  break;
    default:       byte_order_name = "(invalid)"; break;
    }
    switch (bit_order) {
    case MSBFirst: bit_order_name = "MSBFirst";  break;
    case LSBFirst: bit_order_name = "LSBFirst";  break;
    default:       bit_order_name = "(invalid)"; break;
    }
    pm_message("%d rows of %d columns with maxval %d",
               rows, cols, maxval);
    pm_message("padright=%d.  visualclass = %s.  format=%d (%c%c)",
               padright, visualclass_name, 
               format, format/256, format%256);
    pm_message("bits_per_pixel=%d; bits_per_item=%d",
               bits_per_pixel, bits_per_item);
    pm_message("byte_order=%s; bit_order=%s",
               byte_order_name, bit_order_name);
    pm_message("red_mask=0x%.8x; green_mask=0x%.8x; blue_mask=0x%.8x",
               red_mask, green_mask, blue_mask);
}



static void 
convertSimpleIndex(FILE *               const ifp,
                   int                  const cols,
                   struct row_control * const row_controlP,
                   const xel *          const colors,
                   int                  const bits_per_pixel,
                   int                  const bits_per_item,
                   enum byteorder       const byte_order,
                   enum byteorder       const bit_order,
                   xel *                const xelrow) {
    
    unsigned int col;
    for (col = 0; col < cols; ++col)
        xelrow[col] = colors[getpix( ifp, row_controlP, 
                                     bits_per_pixel, bits_per_item, 
                                     byte_order, bit_order)];
}



static void
convertDirect(FILE *               const ifp,
              int                  const cols,
              struct row_control * const row_controlP,
              const xel *          const colors,
              int                  const bits_per_pixel,
              int                  const bits_per_item,
              enum byteorder       const byte_order,
              enum byteorder       const bit_order,
              unsigned long        const red_mask,
              unsigned long        const grn_mask,
              unsigned long        const blu_mask,
              xel *                const xelrow) {
        
    unsigned int col;

    for (col = 0; col < cols; ++col) {
        unsigned long pixel;
        /* This is a triplet of indices into the color map, packed
           into this bit string according to red_mask, etc.
        */
        unsigned int red_index, grn_index, blu_index;
        /* These are indices into the color map, unpacked from 'pixel'.
         */
            
        pixel = getpix(ifp, row_controlP, 
                       bits_per_pixel, bits_per_item, 
                       byte_order, bit_order);

        red_index = (pixel & red_mask) >> zero_bits(red_mask);
        grn_index = (pixel & grn_mask) >> zero_bits(grn_mask); 
        blu_index = (pixel & blu_mask) >> zero_bits(blu_mask);

        PPM_ASSIGN(xelrow[col],
                   PPM_GETR(colors[red_index]),
                   PPM_GETG(colors[grn_index]),
                   PPM_GETB(colors[blu_index])
            );
    }
}



static void
convertTrueColor(FILE *               const ifp,
                 int                  const cols,
                 pixval               const maxval,
                 struct row_control * const row_controlP,
                 const xel *          const colors,
                 int                  const bits_per_pixel,
                 int                  const bits_per_item,
                 enum byteorder       const byte_order,
                 enum byteorder       const bit_order,
                 unsigned long        const red_mask,
                 unsigned long        const grn_mask,
                 unsigned long        const blu_mask,
                 xel *                const xelrow) {

    unsigned int col;
    unsigned int red_shift, grn_shift, blu_shift;
    unsigned int red_maxval, grn_maxval, blu_maxval;

    red_shift = zero_bits(red_mask);
    grn_shift = zero_bits(grn_mask);
    blu_shift = zero_bits(blu_mask);

    red_maxval = red_mask >> red_shift;
    grn_maxval = grn_mask >> grn_shift;
    blu_maxval = blu_mask >> blu_shift;

    for (col = 0; col < cols; ++col) {
        unsigned long pixel;

        pixel = getpix(ifp, row_controlP, 
                       bits_per_pixel, bits_per_item, 
                       byte_order, bit_order);

        /* The parsing of 'pixel' used to be done with hardcoded layout
           parameters.  See comments at end of this file.
        */
        PPM_ASSIGN(xelrow[col],
                   ((pixel & red_mask) >> red_shift) * maxval / red_maxval,
                   ((pixel & grn_mask) >> grn_shift) * maxval / grn_maxval,
                   ((pixel & blu_mask) >> blu_shift) * maxval / blu_maxval
            );

    }
}



static void
convertRow(FILE*            const ifp, 
           FILE*            const outfile,
           int              const bits_per_pixel, 
           int              const bits_per_item,
           enum  byteorder  const byte_order, 
           enum  byteorder  const bit_order,
           int              const padright, 
           int              const cols, 
           xelval           const maxval,
           int              const format, 
           unsigned long    const red_mask, 
           unsigned long    const green_mask, 
           unsigned long    const blue_mask, 
           const xel*       const colors, 
           enum visualclass const visualclass) {

    xel* xelrow;
    struct row_control row_control;
    /* Initialize the state of the getpix() row reader */
    row_control.bits_left = 0;
    row_control.carryover_bits = 0;
    row_control.bits_carried_over = 0;
    
    xelrow = pnm_allocrow(cols);

    switch (visualclass) {
    case StaticGray:
    case GrayScale:
    case StaticColor:
    case PseudoColor:
        convertSimpleIndex(ifp, cols, &row_control, colors, 
                           bits_per_pixel, bits_per_item,
                           byte_order, bit_order, xelrow);
        break;
    case DirectColor: 
        convertDirect(ifp, cols, &row_control, colors,
                      bits_per_pixel, bits_per_item,
                      byte_order, bit_order, 
                      red_mask, green_mask, blue_mask,
                      xelrow);
        
        break;
    case TrueColor: 
        convertTrueColor(ifp, cols, maxval, &row_control, colors,
                         bits_per_pixel, bits_per_item,
                         byte_order, bit_order, 
                         red_mask, green_mask, blue_mask,
                         xelrow);
        break;
            
    default:
        pm_error("unknown visual class");
    }
    {
        unsigned int col;
        for (col = 0; col < padright; ++col)
            getpix(ifp, &row_control, bits_per_pixel, bits_per_item,
                   byte_order, bit_order);
    }
    pnm_writepnmrow(outfile, xelrow, cols, maxval, format, 0);
    pnm_freerow(xelrow);
}



int
main( int argc, char *argv[] ) {
    FILE* ifp;
    int rows, cols, format, padright, row;
    int bits_per_pixel;
    int bits_per_item;
    unsigned long red_mask, green_mask, blue_mask;
    xelval maxval;
    enum visualclass visualclass;
    enum byteorder byte_order, bit_order;
    xel *colors;  /* the color map */

    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    debug = cmdline.debug;
    verbose = cmdline.verbose;

    if ( cmdline.input_filename != NULL ) 
        ifp = pm_openr( cmdline.input_filename );
    else
        ifp = stdin;

    getinit(ifp, &cols, &rows, &padright, &maxval, &visualclass, &format, 
            &colors, &bits_per_pixel, &bits_per_item, 
            &red_mask, &green_mask, &blue_mask, &byte_order, &bit_order,
            cmdline.headerdump);
    
    if (verbose) 
        reportInfo(cols, rows, padright, maxval, visualclass,
                   format, bits_per_pixel, bits_per_item,
                   red_mask, green_mask, blue_mask, 
                   byte_order, bit_order);

    pnm_writepnminit( stdout, cols, rows, maxval, format, 0 );
    switch ( PNM_FORMAT_TYPE(format) )
    {
    case PBM_TYPE:
    pm_message( "writing PBM file" );
    break;

    case PGM_TYPE:
    pm_message( "writing PGM file" );
    break;

    case PPM_TYPE:
    pm_message( "writing PPM file" );
    break;

    default:
    pm_error( "shouldn't happen" );
    }

    for ( row = 0; row < rows; ++row ) {
        convertRow(ifp, stdout, bits_per_pixel, bits_per_item,
                   byte_order, bit_order, padright, cols, maxval, format,
                   red_mask, green_mask, blue_mask, colors, visualclass);
    }
    
    pm_close( ifp );
    pm_close( stdout );
    
    exit( 0 );
    }


/* Byte-swapping junk. */

union cheat {
    uint32n l;
    short s;
    unsigned char c[4];
    };

static short
bs_short( short s )
    {
    union cheat u;
    unsigned char t;

    u.s = s;
    t = u.c[0];
    u.c[0] = u.c[1];
    u.c[1] = t;
    return u.s;
    }

static long
bs_long( long l )
    {
    union cheat u;
    unsigned char t;

    u.l = l;
    t = u.c[0];
    u.c[0] = u.c[3];
    u.c[3] = t;
    t = u.c[1];
    u.c[1] = u.c[2];
    u.c[2] = t;
    return u.l;
    }


/* 
   This used to be the way we parsed a direct/true color pixel.  I'm 
   keeping it here in case we find out some applications needs it this way.

   There doesn't seem to be any reason to do this hard-coded stuff when
   the header contains 32 bit masks that tell exactly how to extract the
   3 colors in all cases.

   We know for a fact that 16 bit TrueColor output from XFree86's xwd
   doesn't match these hard-coded shift amounts, so we have replaced
   this whole switch thing.  -Bryan 00.03.01

   switch ( bits_per_pixel )
            {
                
            case 16:
            PPM_ASSIGN( *xP,
            ( ( ul & red_mask )   >> 0    ),
            ( ( ul & green_mask ) >> 5  ),
            ( ( ul & blue_mask )  >> 11) );
            break;

            case 24:
            case 32:
            PPM_ASSIGN( *xP, ( ( ul & 0xff0000 ) >> 16 ),
            ( ( ul & 0xff00 ) >> 8 ),
            ( ul & 0xff ) );
            break;

            default:
            pm_error( "True/Direct only supports 16, 24, and 32 bits" );
            }
*/
