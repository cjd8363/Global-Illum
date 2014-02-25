/*
 * ppmtobmp.c - Converts from a PPM file to a Microsoft Windows or OS/2
 * .BMP file.
 *
 * Copyright (C) 1992 by David W. Sanderson.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This software is provided "as is"
 * without express or implied warranty.
 *
 */

#define _BSD_SOURCE 1    /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <assert.h>
#include <string.h>
#include "bmp.h"
#include "ppm.h"
#include "shhopt.h"
#include "bitio.h"

#define MAXCOLORS 256

enum colortype {TRUECOLOR, PALETTE};
/*
 * Utilities
 */

static struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    char *input_filename;
    int class;  /* C_WIN or C_OS2 */
    unsigned int bppSpec;
    unsigned int bpp;
} cmdline;


static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdline_p) {
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

    unsigned int windowsSpec, os2Spec;

    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3('w', "windows",   OPT_FLAG, NULL, &windowsSpec,            0);
    OPTENT3('o', "os2",       OPT_FLAG, NULL, &os2Spec,                0);
    OPTENT3(0,   "bpp",       OPT_UINT, &cmdline_p->bpp, 
            &cmdline_p->bppSpec,      0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

    if (windowsSpec && os2Spec) 
        pm_error("Can't specify both -windows and -os2 options.");
    else if (windowsSpec) 
        cmdline_p->class = C_WIN;
    else if (os2Spec)
        cmdline_p->class = C_OS2;
    else 
        cmdline_p->class = C_WIN;


    if (cmdline_p->bppSpec) {
        if (cmdline_p->bpp != 1 && cmdline_p->bpp != 4 && 
            cmdline_p->bpp != 8 && cmdline_p->bpp != 24)
        pm_error("Invalid -bpp value specified: %u.  The only values valid\n"
                 "in the BMP format are 1, 4, 8, and 24 bits per pixel",
                 cmdline_p->bpp);
    }

    if (argc - 1 == 0)
        cmdline_p->input_filename = strdup("-");  /* he wants stdin */
    else if (argc - 1 == 1)
        cmdline_p->input_filename = strdup(argv[1]);
    else 
        pm_error("Too many arguments.  The only argument accepted\n"
                 "is the input file specificaton");

}



static void
PutByte(FILE * const fp, unsigned char const v) {
    if (putc(v, fp) == EOF) 
        pm_error("Write of a byte to a file failed.");

    /* Note:  a Solaris/SPARC user reported on 2003.09.29 that the above
       putc() returned EOF when a former version of this code declared
       v as "char" instead of "unsigned char".  This was apparently due
       to a bug in his C library that caused 255 to look like -1 at some
       critical internal point.
    */
}



static void
PutShort(FILE * const fp, short const v) {
    if (pm_writelittleshort(fp, v) == -1) 
        pm_error("Write of a halfword to a file failed.");
}



static void
PutLong(FILE * const fp, long const v) {
    if (pm_writelittlelong(fp, v) == -1)
        pm_error("Write of a word to a file failed.");
}



/*
 * BMP writing
 */

static int
BMPwritefileheader(FILE *        const fp, 
                   int           const class, 
                   unsigned long const bitcount, 
                   unsigned long const x, 
                   unsigned long const y) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    PutByte(fp, 'B');
    PutByte(fp, 'M');

    /* cbSize */
    PutLong(fp, BMPlenfile(class, bitcount, -1, x, y));
    
    /* xHotSpot */
    PutShort(fp, 0);
    
    /* yHotSpot */
    PutShort(fp, 0);
    
    /* offBits */
    PutLong(fp, BMPoffbits(class, bitcount, -1));
    
    return 14;
}



static int
BMPwriteinfoheader(FILE *        const fp, 
                   int           const class, 
                   unsigned long const bitcount, 
                   unsigned long const x, 
                   unsigned long const y) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
----------------------------------------------------------------------------*/
    long cbFix;

    switch (class) {
    case C_WIN: {
        cbFix = 40;
        PutLong(fp, cbFix);

        PutLong(fp, x);         /* cx */
        PutLong(fp, y);         /* cy */
        PutShort(fp, 1);        /* cPlanes */
        PutShort(fp, bitcount); /* cBitCount */

        /*
         * We've written 16 bytes so far, need to write 24 more
         * for the required total of 40.
         */

        PutLong(fp, 0);   /* Compression */
        PutLong(fp, 0);   /* ImageSize */
        PutLong(fp, 0);   /* XpixelsPerMeter */
        PutLong(fp, 0);   /* YpixelsPerMeter */
        PutLong(fp, 0);   /* ColorsUsed */
        PutLong(fp, 0);   /* ColorsImportant */
    }
    break;
    case C_OS2: {
        cbFix = 12;
        PutLong(fp, cbFix);

        PutShort(fp, x);        /* cx */
        PutShort(fp, y);        /* cy */
        PutShort(fp, 1);        /* cPlanes */
        PutShort(fp, bitcount); /* cBitCount */
    }
    break;
    default:
        pm_error(er_internal, "BMPwriteinfoheader");
    }

    return cbFix;
}



static int
BMPwritergb(FILE * const fp, 
            int    const class, 
            pixval const R, 
            pixval const G, 
            pixval const B) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    switch (class) {
    case C_WIN:
        PutByte(fp, B);
        PutByte(fp, G);
        PutByte(fp, R);
        PutByte(fp, 0);
        return 4;
    case C_OS2:
        PutByte(fp, B);
        PutByte(fp, G);
        PutByte(fp, R);
        return 3;
    default:
        pm_error(er_internal, "BMPwritergb");
    }
    return -1;
}



static int
BMPwritecolormap(FILE *        const fp, 
                 int           const class, 
                 int           const bpp,
                 int           const colors, 
                 unsigned char const R[], 
                 unsigned char const G[], 
                 unsigned char const B[]) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    long const ncolors = (1 << bpp);

    int  nbyte;
    int  i;

    nbyte = 0;
    for (i = 0; i < colors; ++i)
        nbyte += BMPwritergb(fp,class,R[i],G[i],B[i]);

    for (; i < ncolors; ++i)
        nbyte += BMPwritergb(fp,class,0,0,0);

    return nbyte;
}



static int
BMPwriterow_palette(FILE *          const fp, 
                    const pixel *   const row, 
                    unsigned long   const cx, 
                    unsigned short  const bpp, 
                    colorhash_table const cht) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    BITSTREAM    b;
    int retval;
    
    b = pm_bitinit(fp, "w");
    if (b == NULL)
        retval = -1;
    else {
        unsigned int nbyte;
        unsigned int x;
        bool         error;
        
        nbyte = 0;      /* initial value */
        error = FALSE;  /* initial value */
        
        for (x = 0; x < cx && !error; ++x) {
            int rc;
            rc = pm_bitwrite(b, bpp, ppm_lookupcolor(cht, &row[x]));
            if (rc == -1)
                error = TRUE;
            else
                nbyte += rc;
        }
        if (error)
            retval = -1;
        else {
            int rc;

            rc = pm_bitfini(b);
            if (rc == -1)
                retval = -1;
            else {
                nbyte += rc;
                
                /* Make sure we write a multiple of 4 bytes.  */
                while (nbyte % 4 != 0) {
                    PutByte(fp, 0);
                    ++nbyte;
                }
                retval = nbyte;
            }
        }
    }
    return retval;
}



static int
BMPwriterow_truecolor(FILE *        const fp, 
                      const pixel * const row, 
                      unsigned long const cols,
                      pixval        const maxval) {
/*----------------------------------------------------------------------------
  Write a row of a truecolor BMP image to the file 'fp'.  The row is 
  'row', which is 'cols' columns long.

  Return the number of bytes written.

  On error, issue error message and exit program.
-----------------------------------------------------------------------------*/
    /* This works only for 24 bits per pixel.  To implement this for the
       general case (which is only hypothetical -- this program doesn't
       write any truecolor images except 24 bit and apparently no one
       else does either), you would move this function into 
       BMPwriterow_palette, which writes arbitrary bit strings.  But
       that would be a lot slower and less robust.
    */

    int nbyte;  /* Number of bytes we have written to file so far */
    int col;  
        
    nbyte = 0;  /* initial value */
    for (col = 0; col < cols; col++) {
        /* We scale to the BMP maxval, which is always 255. */
        PutByte(fp, PPM_GETB(row[col]) * 255 / maxval);
        PutByte(fp, PPM_GETG(row[col]) * 255 / maxval);
        PutByte(fp, PPM_GETR(row[col]) * 255 / maxval);
        nbyte += 3;
    }

    /*
     * Make sure we write a multiple of 4 bytes.
     */
    while (nbyte % 4) {
        PutByte(fp, 0);
        nbyte++;
    }
    
    return nbyte;
}



static int
BMPwritebits(FILE *          const fp, 
             unsigned long   const cx, 
             unsigned long   const cy, 
             enum colortype  const colortype,
             unsigned short  const cBitCount, 
             const pixel **  const pixels, 
             pixval          const maxval,
             colorhash_table const cht) {
/*----------------------------------------------------------------------------
  Return the number of bytes written, or -1 on error.
-----------------------------------------------------------------------------*/
    int  nbyte;
    long y;

    if (cBitCount > 24)
        pm_error("cannot handle cBitCount: %d", cBitCount);

    nbyte = 0;  /* initial value */

    /* The picture is stored bottom line first, top line last */

    for (y = cy - 1; y >= 0; --y) {
        int rc;
        if (colortype == PALETTE)
            rc = BMPwriterow_palette(fp, pixels[y], cx, 
                                     cBitCount, cht);
        else 
            rc = BMPwriterow_truecolor(fp, pixels[y], cx, maxval);

        if (rc == -1)
            pm_error("couldn't write row %ld", y);
        if (rc % 4 != 0)
            pm_error("row had bad number of bytes: %d", rc);
        nbyte += rc;
    }

    return nbyte;
}



static void
BMPEncode(FILE *          const fp, 
          int             const class, 
          enum colortype  const colortype,
          int             const bpp,
          int             const x, 
          int             const y, 
          const pixel **  const pixels, 
          pixval          const maxval,
          int             const colors, 
          colorhash_table const cht, 
          unsigned char   const R[], 
          unsigned char   const G[], 
          unsigned char   const B[]) {
/*----------------------------------------------------------------------------
  Write a BMP file of the given class.
  
  Note that we must have 'colors' in order to know exactly how many
  colors are in the R, G, B, arrays.  Entries beyond those in the
  arrays are undefined.
-----------------------------------------------------------------------------*/
    unsigned long nbyte;

    if (colortype == PALETTE)
        pm_message("Writing %d bits per pixel with a color palette", bpp);
    else
        pm_message("Writing %d bits per pixel truecolor (no palette)", bpp);

    nbyte = 0;  /* initial value */
    nbyte += BMPwritefileheader(fp, class, bpp, x, y);
    nbyte += BMPwriteinfoheader(fp, class, bpp, x, y);
    if (colortype == PALETTE)
        nbyte += BMPwritecolormap(fp, class, bpp, colors, R, G, B);

    if (nbyte != (BMPlenfileheader(class)
                  + BMPleninfoheader(class)
                  + BMPlencolormap(class, bpp, -1)))
        pm_error(er_internal, "BMPEncode 1");

    nbyte += BMPwritebits(fp, x, y, colortype, bpp, pixels, maxval, cht);
    if (nbyte != BMPlenfile(class, bpp, -1, x, y))
        pm_error(er_internal, "BMPEncode 2");
}



static void
analyze_colors(const pixel **    const pixels, 
               int               const cols, 
               int               const rows, 
               pixval            const maxval, 
               int *             const colors_p, 
               int *             const minimum_bpp_p,
               colorhash_table * const cht_p, 
               unsigned char           Red[], 
               unsigned char           Green[], 
               unsigned char           Blue[]) {
/*----------------------------------------------------------------------------
  Look at the colors in the image 'pixels' and compute values to use in
  representing those colors in a BMP image.  

  First of all, count the distinct colors.  Return as *minimum_bpp_p
  the minimum number of bits per pixel it will take to represent all
  the colors in BMP format.

  If there are few enough colors to represent with a palette, also
  return the number of colors as *colors_p and a suitable palette
  (colormap) and a hash table in which to look up indexes into that
  palette as Red[], Green[], Blue[], and *cht_p.  Use only the first
  *colors_p entries of the Red[], etc. array.

  If there are too many colors for a palette, return *cht_p == NULL.

  Issue informational messages.
-----------------------------------------------------------------------------*/

    /* Figure out the colormap. */
    colorhist_vector chv;
    int i;

    pm_message("analyzing colors...");
    chv = ppm_computecolorhist((pixel**)pixels, cols, rows, MAXCOLORS, 
                               colors_p);
    if (chv == NULL) {
        pm_message("More than %d colors found", MAXCOLORS);
        *minimum_bpp_p = 24;
        *cht_p = NULL;
    } else {
        unsigned int const minbits = pm_maxvaltobits(*colors_p-1);

        pm_message("%d colors found", *colors_p);

        /* Only 1, 4, 8, and 24 are defined in the BMP spec we
           implement and other bpp's have in fact been seen to confuse
           viewers.  There is an extended BMP format that has 16 bpp
           too, but this program doesn't know how to generate that
           (see Bmptopnm.c, though).  
        */
        if (minbits == 1)
            *minimum_bpp_p = 1;
        else if (minbits <= 4)
            *minimum_bpp_p = 4;
        else if (minbits <= 8)
            *minimum_bpp_p = 8;
        else
            *minimum_bpp_p = 24;

        /*
         * Now scale the maxval to 255 as required by BMP format.
         */
        for (i = 0; i < *colors_p; ++i) {
            Red[i] = (pixval) PPM_GETR(chv[i].color) * 255 / maxval;
            Green[i] = (pixval) PPM_GETG(chv[i].color) * 255 / maxval;
            Blue[i] = (pixval) PPM_GETB(chv[i].color) * 255 / maxval;
        }
    
        /* And make a hash table for fast lookup. */
        *cht_p = ppm_colorhisttocolorhash(chv, *colors_p);
        ppm_freecolorhist(chv);
    }
}



static void
choose_colortype_bpp(struct cmdline_info const cmdline,
                     int                 const colors, 
                     int                 const minimum_bpp,
                     enum colortype *    const colortype_p, 
                     int *               const bits_per_pixel_p) {

    if (!cmdline.bppSpec) {
        /* User has no preference as to bits per pixel.  Choose the
           smallest number possible for this image.
        */
        *bits_per_pixel_p = minimum_bpp;
    } else {
        if (cmdline.bpp < minimum_bpp)
            pm_error("There are too many colors in the image to "
                     "represent in the\n"
                     "number of bits per pixel you requested: %d.\n"
                     "You may use Ppmquant to reduce the number of "
                     "colors in the image.",
                     cmdline.bpp);
        else
            *bits_per_pixel_p = cmdline.bpp;
    }

    assert(*bits_per_pixel_p == 1 || 
           *bits_per_pixel_p == 4 || 
           *bits_per_pixel_p == 8 || 
           *bits_per_pixel_p == 24);

    if (*bits_per_pixel_p > 8) 
        *colortype_p = TRUECOLOR;
    else {
        *colortype_p = PALETTE;
    }
}



int
main(int argc, char **argv) {
    FILE *ifp;

    int rows;
    int cols;
    int colors;
    pixval maxval;
    int minimum_bpp;
    int bits_per_pixel;
    enum colortype colortype;

    pixel** pixels;
    colorhash_table cht;
    /* 'Red', 'Green', and 'Blue' are the color map for the BMP file,
       with indices the same as in 'cht', above.
    */
    unsigned char Red[MAXCOLORS], Green[MAXCOLORS], Blue[MAXCOLORS];

    ppm_init(&argc, argv);

    parse_command_line(argc, argv, &cmdline);

    ifp = pm_openr(cmdline.input_filename);

    pixels = ppm_readppm(ifp, &cols, &rows, &maxval);

    pm_close(ifp);


    analyze_colors((const pixel**)pixels, cols, rows, maxval, 
                   &colors, &minimum_bpp, &cht, Red, Green, Blue);

    choose_colortype_bpp(cmdline, colors, minimum_bpp, &colortype, 
                         &bits_per_pixel);

    BMPEncode(stdout, cmdline.class, colortype, bits_per_pixel,
              cols, rows, (const pixel**)pixels, maxval, colors, cht,
              Red, Green, Blue);

    if (cht) 
        ppm_freecolorhash(cht);

    pm_close(stdout);

    return 0;
}
