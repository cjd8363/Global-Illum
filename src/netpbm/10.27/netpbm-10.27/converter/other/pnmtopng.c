/*
** pnmtopng.c -
** read a portable anymap and produce a Portable Network Graphics file
**
** derived from pnmtorast.c (c) 1990,1991 by Jef Poskanzer and some
** parts derived from ppmtogif.c by Marcel Wijkstra <wijkstra@fwi.uva.nl>
**
** Copyright (C) 1995-1998 by Alexander Lehmann <alex@hal.rhein-main.de>
**                        and Willem van Schaik <willem@schaik.com>
** Copyright (C) 1999,2001 by Greg Roelofs <newt@pobox.com>
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/* A performance note: This program reads one row at a time because
   the whole image won't fit in memory always.  When you realize that
   in a Netpbm xel array a one bit pixel can take 96 bits of memory,
   it's easy to see that an ordinary fax could deplete your virtual
   memory and even if it didn't, it might deplete your real memory and
   iterating through the array would cause thrashing.  This program
   iterates through the image multiple times.  

   So instead, we read the image into memory one row at a time, into a
   single row buffer.  We use Netpbm's pm_openr_seekable() facility to
   access the file.  That facility copies the file into a temporary
   file if it isn't seekable, so we always end up with a file that we
   can rewind and reread multiple times.

   This shouldn't cause I/O delays because the entire image ought to fit
   in the system's I/O cache (remember that the file is a lot smaller than
   the xel array you'd get by doing a pnm_readpnm() of it).

   However, it does introduce some delay because of all the system calls 
   required to read the file.  A future enhancement might read the entire
   file into an xel array in some cases, and read one row at a time in 
   others, depending on the needs of the particular use.

   We do still read the entire alpha mask (if there is one) into a
   'gray' array, rather than access it one row at a time.  

   Before May 2001, we did in fact read the whole image into an xel array,
   and we got complaints.  Before April 2000, it wasn't as big a problem
   because xels were only 24 bits.  Now they're 96.
*/
   
#define GRR_GRAY_PALETTE_FIX

#ifndef PNMTOPNG_WARNING_LEVEL
#  define PNMTOPNG_WARNING_LEVEL 0   /* use 0 for backward compatibility, */
#endif                               /*  2 for warnings (1 == error) */

#include <string.h>	/* strcat() */
#include <limits.h>
#include <png.h>	/* includes zlib.h and setjmp.h */
#define VERSION "2.37.6 (21 July 2001) +netpbm"
#include "pnm.h"
#include "pngtxt.h"
#include "mallocvar.h"
#include "nstring.h"

struct zlib_compression {
    /* These are parameters that describe a form of zlib compression. 
       In each of them, -1 means "don't care," and a nonnegative value is
       as defined by zlib.
    */
    int level;
    int mem_level;
    int strategy;
    int window_bits;
    int method;
    int buffer_size;
};

struct chroma {
    float wx;
    float wy;
    float rx;
    float ry;
    float gx;
    float gy;
    float bx;
    float by;
};

typedef struct cahitem {
    xel color;
    gray alpha;
    int value;
    struct cahitem * next;
} cahitem;

typedef cahitem ** coloralphahash_table;

typedef struct _jmpbuf_wrapper {
  jmp_buf jmpbuf;
} jmpbuf_wrapper;

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef NONE
#  define NONE 0
#endif
#define MAXCOLORS 256
#define MAXPALETTEENTRIES 256

/* PALETTEMAXVAL is the maxval used in a PNG palette */
#define PALETTEMAXVAL 255

#define PALETTEOPAQUE 255
#define PALETTETRANSPARENT 0

/* function prototypes */
#ifdef __STDC__
static void pnmtopng_error_handler (png_structp png_ptr, png_const_charp msg);
int main (int argc, char *argv[]);
#endif

static int verbose = FALSE;

static jmpbuf_wrapper pnmtopng_jmpbuf_struct;
static int errorlevel;



static png_color_16
xelToPngColor_16(xel const input, 
                 xelval const maxval, 
                 xelval const pngMaxval) {
    png_color_16 retval;

    xel scaled;
    
    PPM_DEPTH(scaled, input, maxval, pngMaxval);

    retval.red   = PPM_GETR(scaled);
    retval.green = PPM_GETG(scaled);
    retval.blue  = PPM_GETB(scaled);
    retval.gray  = PNM_GET1(scaled);

    return retval;
}



static void
closestColorInPalette(pixel          const targetColor, 
                      pixel                palette_pnm[],
                      unsigned int   const paletteSize,
                      unsigned int * const bestIndexP,
                      unsigned int * const bestMatchP) {
    
    unsigned int paletteIndex;
    unsigned int bestIndex;
    unsigned int bestMatch;

    bestMatch = UINT_MAX;
    for (paletteIndex = 0; paletteIndex < paletteSize; ++paletteIndex) {
        unsigned int const dist = 
            PPM_DISTANCE(palette_pnm[paletteIndex], targetColor);

        if (dist < bestMatch) {
            bestMatch = dist;
            bestIndex = paletteIndex;
        }
    }
    if (bestIndexP != NULL)
        *bestIndexP = bestIndex;
    if (bestMatchP != NULL)
        *bestMatchP = bestMatch;
}



/* We really ought to make this hash function actually depend upon
   the "a" argument; we just don't know a decent prime number off-hand.
*/
#define HASH_SIZE 20023
#define hashpixelalpha(p,a) ((((long) PPM_GETR(p) * 33023 + \
                               (long) PPM_GETG(p) * 30013 + \
                               (long) PPM_GETB(p) * 27011 ) \
                              & 0x7fffffff ) % HASH_SIZE )

static coloralphahash_table
alloccoloralphahash(void)  {
    coloralphahash_table caht;
    int i;

    MALLOCARRAY(caht,HASH_SIZE);
    if (caht == NULL)
        pm_error( "out of memory allocating hash table" );

    for (i = 0; i < HASH_SIZE; ++i)
        caht[i] = NULL;

    return caht;
}


static void
freecoloralphahash(coloralphahash_table const caht) {
    int i;

    for (i = 0; i < HASH_SIZE; ++i) {
        cahitem * p;
        cahitem * next;
        for (p = caht[i]; p; p = next) {
            next = p->next;
            free(p);
        }
    }
    free(caht);
}



static void
addtocoloralphahash(coloralphahash_table const caht,
                    pixel *              const colorP,
                    gray *               const alphaP,
                    int                  const value) {

    int hash;
    cahitem * itemP;

    MALLOCVAR(itemP);
    if (itemP == NULL)
        pm_error("Out of memory building hash table");
    hash = hashpixelalpha(*colorP, *alphaP);
    itemP->color = *colorP;
    itemP->alpha = *alphaP;
    itemP->value = value;
    itemP->next = caht[hash];
    caht[hash] = itemP;
}



static int
lookupColorAlpha(coloralphahash_table const caht,
                 pixel const * const colorP,
                 gray * const alphaP) {

    int hash;
    cahitem * p;

    hash = hashpixelalpha(*colorP, *alphaP);
    for (p = caht[hash]; p; p = p->next)
        if (PPM_EQUAL(p->color, *colorP) && p->alpha == *alphaP)
            return p->value;

    return -1;
}



#ifdef __STDC__
static void pnmtopng_error_handler (png_structp png_ptr, png_const_charp msg)
#else
static void pnmtopng_error_handler (png_ptr, msg)
png_structp png_ptr;
png_const_charp msg;
#endif
{
  jmpbuf_wrapper  *jmpbuf_ptr;

  /* this function, aside from the extra step of retrieving the "error
   * pointer" (below) and the fact that it exists within the application
   * rather than within libpng, is essentially identical to libpng's
   * default error handler.  The second point is critical:  since both
   * setjmp() and longjmp() are called from the same code, they are
   * guaranteed to have compatible notions of how big a jmp_buf is,
   * regardless of whether _BSD_SOURCE or anything else has (or has not)
   * been defined. */

  fprintf(stderr, "pnmtopng:  fatal libpng error: %s\n", msg);
  fflush(stderr);

  jmpbuf_ptr = png_get_error_ptr(png_ptr);
  if (jmpbuf_ptr == NULL) {         /* we are completely hosed now */
    fprintf(stderr,
      "pnmtopng:  EXTREMELY fatal error: jmpbuf unrecoverable; terminating.\n");
    fflush(stderr);
    exit(99);
  }

  longjmp(jmpbuf_ptr->jmpbuf, 1);
}


/* The following variables belong to getChv() and freeChv() */
static bool getChv_computed = FALSE;
static colorhist_vector getChv_chv;
static int getChv_colors;



static void
getChv(FILE *             const ifP, 
       pm_filepos         const imagepos,
       int                const cols, 
       int                const rows, 
       xelval             const maxval,
       int                const format, 
       int                const maxColors, 
       colorhist_vector * const chvP,
       unsigned int *     const colorsP) {
/*----------------------------------------------------------------------------
   Return a list of all the colors in a libpnm image and the number of
   times they occur.  The image is in the seekable file 'ifP', whose
   raster starts at position 'imagepos' of the file.  The image's properties
   are 'cols', 'rows', 'maxval', and 'format'.

   Return the number of colors as *colorsP.  Return the details of the 
   colors in newly malloc'ed storage, and its address as *chvP.  If
   there are more than 'maxColors' colors, though, just return NULL as
   *chvP and leave *colorsP undefined.

   Don't spend the time to read the file if this subroutine has been called
   before.  In that case, just assume the inputs are all the same and return
   the previously computed information.

   *chvP is in static program storage.
-----------------------------------------------------------------------------*/
    if (!getChv_computed) {
        if (verbose) 
            pm_message ("Finding colors in input image...");

        pm_seek2(ifP, &imagepos, sizeof(imagepos));
        getChv_chv = ppm_computecolorhist2(ifP, cols, rows, maxval, format, 
                                           MAXCOLORS, &getChv_colors);
        
        if (getChv_chv)
            pm_message ("%d colors found", getChv_colors);

        getChv_computed = TRUE;
    }
    *chvP = getChv_chv;
    *colorsP = getChv_colors;
}



static void freeChv(void) {

    if (getChv_computed)
        if (getChv_chv)
            ppm_freecolorhist(getChv_chv);

    getChv_computed = FALSE;
}



static void
meaningful_bits_pgm(FILE *         const ifp, 
                    pm_filepos     const imagepos, 
                    int            const cols,
                    int            const rows,
                    xelval         const maxval,
                    int            const format,
                    unsigned int * const retvalP) {
/*----------------------------------------------------------------------------
   In the PGM raster with maxval 'maxval' at file offset 'imagepos'
   in file 'ifp', the samples may be composed of groups of 1, 2, 4, or 8
   bits repeated.  This would be the case if the image were converted
   at some point from a 2 bits-per-pixel image to an 8-bits-per-pixel
   image, for example.

   If this is the case, we find out and find out how small these repeated
   groups of bits are and return the number of bits.
-----------------------------------------------------------------------------*/
    int mayscale;
    int x, y;
    xel * xelrow;

    xelrow = pnm_allocrow(cols);

    *retvalP = pm_maxvaltobits(maxval);

    if (maxval == 65535) {
        mayscale = TRUE;   /* initial assumption */
        pm_seek2(ifp, &imagepos, sizeof(imagepos));
        for (y = 0 ; y < rows && mayscale ; y++) {
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (x = 0 ; x < cols && mayscale ; x++) {
                xel const p = xelrow[x];
                if ( (PNM_GET1 (p)&0xff)*0x101 != PNM_GET1 (p) )
                    mayscale = FALSE;
            }
        }
        if (mayscale)
            *retvalP = 8;
    }
    if (maxval == 255 || *retvalP == 8) {
        mayscale = TRUE;  /* initial assumption */
        pm_seek2(ifp, &imagepos, sizeof(imagepos));
        for (y = 0 ; y < rows && mayscale ; y++) {
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (x = 0 ; x < cols && mayscale ; x++) {
                if ((PNM_GET1 (xelrow[x]) & 0xf) * 0x11 
                    != PNM_GET1 (xelrow[x]))
                    mayscale = FALSE;
            }
        }
        if (mayscale)
            *retvalP = 4;
    }

    if (maxval == 15 || *retvalP == 4) {
        mayscale = TRUE;   /* initial assumption */
        pm_seek2(ifp, &imagepos, sizeof(imagepos));
        for (y = 0 ; y < rows && mayscale ; y++) {
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (x = 0 ; x < cols && mayscale ; x++) {
                if ((PNM_GET1 (xelrow[x])&3) * 0x5 != PNM_GET1 (xelrow[x]))
                    mayscale = FALSE;
            }
        }
        if (mayscale) {
            *retvalP = 2;
        }
    }

    if (maxval == 3 || *retvalP == 2) {
        mayscale = TRUE;   /* initial assumption */
        pm_seek2(ifp, &imagepos, sizeof(imagepos));
        for (y = 0 ; y < rows && mayscale ; y++) {
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (x = 0 ; x < cols && mayscale ; x++) {
                if ((PNM_GET1 (xelrow[x])&1) * 0x3 != PNM_GET1 (xelrow[x]))
                    mayscale = FALSE;
            }
        }
        if (mayscale) {
            *retvalP = 1;
        }
    }
    pnm_freerow(xelrow);
}


static void
meaningful_bits_ppm(FILE *         const ifp, 
                    pm_filepos     const imagepos, 
                    int            const cols,
                    int            const rows,
                    xelval         const maxval,
                    int            const format,
                    unsigned int * const retvalP) {
/*----------------------------------------------------------------------------
   In the PPM raster with maxval 'maxval' at file offset 'imagepos'
   in file 'ifp', the samples may be composed of groups of 8
   bits repeated twice.  This would be the case if the image were converted
   at some point from a 8 bits-per-pixel image to an 16-bits-per-pixel
   image, for example.

   We return the smallest number of bits we can take from the right of
   a sample without losing information (8 or all).
-----------------------------------------------------------------------------*/
    int mayscale;
    int x, y;
    xel * xelrow;

    xelrow = pnm_allocrow(cols);

    *retvalP = pm_maxvaltobits(maxval);

    if (maxval == 65535) {
        mayscale = TRUE;   /* initial assumption */
        pm_seek2(ifp, &imagepos, sizeof(imagepos));
        for (y = 0 ; y < rows && mayscale ; y++) {
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (x = 0 ; x < cols && mayscale ; x++) {
                xel const p = xelrow[x];
                if ( (PPM_GETR (p)&0xff)*0x101 != PPM_GETR (p) ||
                     (PPM_GETG (p)&0xff)*0x101 != PPM_GETG (p) ||
                     (PPM_GETB (p)&0xff)*0x101 != PPM_GETB (p) )
                    mayscale = FALSE;
            }
        }
        if (mayscale)
            *retvalP = 8;
    }
    pnm_freerow(xelrow);
}



static void
alpha_trans(FILE *     const ifp, 
            pm_filepos const imagepos, 
            int        const cols, 
            int        const rows, 
            xelval     const maxval, int const format, 
            gray **    const alpha_mask, gray alpha_maxval,
            bool *     const alpha_can_be_transparency_indexP, 
            pixel*     const alpha_transcolorP) {
/*----------------------------------------------------------------------------
  Check if the alpha mask can be represented by a single transparency
  value (i.e. all colors fully opaque except one fully transparent;
  the transparent color may not also occur as fully opaque.
  we have to do this before any scaling occurs, since alpha is only
  possible with 8 and 16-bit.
-----------------------------------------------------------------------------*/
    xel * xelrow;
    bool retval;
        /* Our eventual return value -- alpha mask can be represented by
           a simple transparency index.
        */
    bool found_transparent_pixel;
        /* We found a pixel in the image where the alpha mask says it is
           transparent.
        */
    pixel transcolor;
        /* Color of the transparent pixel mentioned above. */
    int const pnm_type = PNM_FORMAT_TYPE(format);
    
    xelrow = pnm_allocrow(cols);

    {
        int row;
        /* Find a candidate transparent color -- the color of any pixel in the
           image that the alpha mask says should be transparent.
        */
        found_transparent_pixel = FALSE;  /* initial assumption */
        pm_seek2(ifp, &imagepos, sizeof(imagepos));
        for (row = 0 ; row < rows && !found_transparent_pixel ; ++row) {
            int col;
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (col = 0 ; col < cols && !found_transparent_pixel; ++col) {
                if (alpha_mask[row][col] == 0) {
                    found_transparent_pixel = TRUE;
                    transcolor = xeltopixel(xelrow[col]);
                }
            }
        }
    }

    if (found_transparent_pixel) {
        int row;
        pm_seek2(ifp, &imagepos, sizeof(imagepos));
        retval = TRUE;  /* initial assumption */
        
        for (row = 0 ; row < rows && retval == TRUE; ++row) {
            int col;
            pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
            for (col = 0 ; col < cols && retval == TRUE; ++col) {
                if (alpha_mask[row][col] == 0) { /* transparent */
                    /* If we have a second transparent color, we're
                       disqualified
                    */
                    if (pnm_type == PPM_TYPE) {
                        if (!PPM_EQUAL (xelrow[col], transcolor))
                            retval = FALSE;
                    } else {
                        if (PNM_GET1 (xelrow[col]) != PNM_GET1 (transcolor))
                            retval = FALSE;
                    }
                } else if (alpha_mask[row][col] != alpha_maxval) {
                    /* Here's an area of the mask that is translucent.  That
                       disqualified us.
                    */
                    retval = FALSE;
                } else 
                    /* Here's an area of the mask that is opaque.  If it's
                       the same color as our candidate transparent color,
                       that disqualifies us.
                    */
                    if (pnm_type == PPM_TYPE) {
                        if (PPM_EQUAL (xelrow[col], transcolor))
                            retval = FALSE;
                    } else {
                        if (PNM_GET1 (xelrow[col]) == PNM_GET1 (transcolor))
                            retval = FALSE;
                    }
            }
        }  
    }
    pnm_freerow(xelrow);

    *alpha_can_be_transparency_indexP = retval;
    *alpha_transcolorP = transcolor;
}



static void
findRedundantBits(FILE *         const ifp, 
                  int            const imagepos, 
                  int            const cols,
                  int            const rows,
                  xelval         const maxval,
                  int            const format,
                  bool           const alpha,
                  bool           const force,
                  unsigned int * const meaningfulBitsP) {
/*----------------------------------------------------------------------------
   Find out if we can use just a subset of the bits from each input
   sample.  Often, people create an image with e.g. 8 bit samples from
   one that has e.g. only 4 bit samples by scaling by 256/16, which is
   the same as repeating the bits.  E.g.  1011 becomes 10111011.  We
   detect this case.  We return as *meaningfulBitsP the minimum number
   of bits, starting from the least significant end, that contain
   original information.
-----------------------------------------------------------------------------*/
  if (!alpha && PNM_FORMAT_TYPE(format) == PGM_TYPE && !force) 
      meaningful_bits_pgm(ifp, imagepos, cols, rows, maxval, format,
                          meaningfulBitsP);
  else if (PNM_FORMAT_TYPE(format) == PPM_TYPE && !force)
      meaningful_bits_ppm(ifp, imagepos, cols, rows, maxval, format,
                          meaningfulBitsP);
  else 
      *meaningfulBitsP = pm_maxvaltobits(maxval);

  if (verbose && *meaningfulBitsP != pm_maxvaltobits(maxval))
      pm_message("Using only %d rightmost bits of input samples.  The "
                 "rest are redundant.", *meaningfulBitsP);
}



static void
readOrderedPalette(FILE * const   pfp,
                   xel            ordered_palette[], 
                   unsigned int * const ordered_palette_size_p) {

    xel ** xels;
    int cols, rows;
    xelval maxval;
    int format;
    
    if (verbose)
        pm_message ("reading ordered palette (colormap)...");

    xels = pnm_readpnm (pfp, &cols, &rows, &maxval, &format);
    
    if (PNM_FORMAT_TYPE(format) != PPM_TYPE) 
        pm_error("ordered palette must be a PPM file, not type %d", format);

    *ordered_palette_size_p = rows * cols;
    if (*ordered_palette_size_p > MAXCOLORS) 
        pm_error("ordered-palette image contains %d pixels.  Maximum is %d",
                 *ordered_palette_size_p, MAXCOLORS);
    if (verbose)
      pm_message ("%d colors found", *ordered_palette_size_p);

    {
        int j;
        int row;
        j = 0;  /* initial value */
        for (row = 0; row < rows; ++row) {
            int col;
            for (col = 0; col < cols; ++col) 
                ordered_palette[j++] = xels[row][col];
        }
    }
    pnm_freearray(xels, rows);
}        



static void
compute_nonalpha_palette(colorhist_vector const chv,
                         int              const colors,
                         pixval           const maxval,
                         FILE *           const pfp,
                         pixel                  palette_pnm[],
                         unsigned int *   const paletteSizeP,
                         gray                   trans_pnm[],
                         unsigned int *   const transSizeP) {
/*----------------------------------------------------------------------------
   Compute the palette corresponding to the color set 'chv'
   (consisting of 'colors' distinct colors) assuming a pure-color (no
   transparency) palette.

   If 'pfp' is non-null, assume it's a PPM file and read the palette
   from that.  Make sure it contains the same colors as the palette
   we computed ourself would have.  Caller supplied the file because he
   wants the colors in a particular order in the palette.
-----------------------------------------------------------------------------*/
    unsigned int colorIndex;
    
    xel ordered_palette[MAXCOLORS];
    unsigned int ordered_palette_size;

    if (pfp) {
        readOrderedPalette(pfp, ordered_palette, &ordered_palette_size);

        if (colors != ordered_palette_size) 
            pm_error("sizes of ordered palette (%d) "
                     "and existing palette (%d) differ",
                     ordered_palette_size, colors);
        
        /* Make sure the ordered palette contains all the colors in
           the image 
        */
        for (colorIndex = 0; colorIndex < colors; colorIndex++) {
            int j;
            bool found;
            
            found = FALSE;
            for (j = 0; j < ordered_palette_size && !found; ++j) {
                if (PNM_EQUAL(ordered_palette[j], chv[colorIndex].color)) 
                    found = TRUE;
            }
            if (!found) 
                pm_error("failed to find color (%d, %d, %d), which is in the "
                         "input image, in the ordered palette",
                         PPM_GETR(chv[colorIndex].color),
                         PPM_GETG(chv[colorIndex].color),
                         PPM_GETB(chv[colorIndex].color));
        }
        /* OK, the ordered palette passes muster as a palette; go ahead
           and return it as the palette.
        */
        for (colorIndex = 0; colorIndex < colors; ++colorIndex)
            palette_pnm[colorIndex] = ordered_palette[colorIndex];
    } else {
        for (colorIndex = 0; colorIndex < colors; ++colorIndex) 
            palette_pnm[colorIndex] = chv[colorIndex].color;
    }
    *paletteSizeP = colors;
    *transSizeP = 0;
}



static void
computeUnsortedAlphaPalette(FILE *           const ifP,
                            int              const cols,
                            int              const rows,
                            xelval           const maxval,
                            int              const format,
                            pm_filepos       const imagepos,
                            gray **          const alpha_mask,
                            unsigned int     const maxPaletteEntries,
                            colorhist_vector const chv,
                            int              const colors,
                            gray *                 alphas_of_color[],
                            unsigned int           alphas_first_index[],
                            unsigned int           alphas_of_color_cnt[]) {
/*----------------------------------------------------------------------------
   Read the image at position 'imagepos' in file *ifP, which is a PNM
   described by 'cols', 'rows', 'maxval', and 'format'.

   Using the alpha mask 'alpha_mask' and color map 'chv' (of size 'colors')
   for the image, construct a palette of (color index, alpha) ordered pairs 
   for the image, as follows.

   The alpha/color palette is the set of all ordered pairs of
   (color,alpha) in the PNG, including the background color.  The
   actual palette is an array with up to 'maxPaletteEntries elements.  Each
   array element contains a color index from the color palette and
   an alpha value.  All the elements with the same color index are
   contiguous.  alphas_first_index[x] is the index in the
   alpha/color palette of the first element that has color index x.
   alphas_of_color_cnt[x] is the number of elements that have color
   index x.  alphas_of_color[x][y] is the yth alpha value that
   appears with color index x (in order of appearance).
   alpha_color_pair_count is the total number of elements, i.e. the
   total number of combinations color and alpha.
-----------------------------------------------------------------------------*/
    colorhash_table cht;
    int color_index;
    int row;
    xel * xelrow;

    cht = ppm_colorhisttocolorhash (chv, colors);

    for (color_index = 0 ; color_index < colors + 1 ; ++color_index) {
        /* TODO: It sure would be nice if we didn't have to allocate
           256 words here for what is normally only 0 or 1 different
           alpha values!  Maybe we should do some sophisticated reallocation.
        */
        MALLOCARRAY(alphas_of_color[color_index], maxPaletteEntries);
        if (alphas_of_color[color_index] == NULL)
            pm_error ("out of memory allocating alpha/palette entries");
        alphas_of_color_cnt[color_index] = 0;
    }
 
    pm_seek2(ifP, &imagepos, sizeof(imagepos));

    xelrow = pnm_allocrow(cols);

    for (row = 0 ; row < rows ; ++row) {
        int col;
        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);
        pnm_promoteformatrow(xelrow, cols, maxval, format, maxval, PPM_TYPE);
        for (col = 0 ; col < cols ; ++col) {
            int i;
            int const color = ppm_lookupcolor(cht, &xelrow[col]);
            for (i = 0 ; i < alphas_of_color_cnt[color] ; ++i) {
                if (alpha_mask[row][col] == alphas_of_color[color][i])
                    break;
            }
            if (i == alphas_of_color_cnt[color]) {
                alphas_of_color[color][i] = alpha_mask[row][col];
                alphas_of_color_cnt[color]++;
            }
        }
    }
    {
        int i;
        alphas_first_index[0] = 0;
        for (i = 1 ; i < colors ; i++)
            alphas_first_index[i] = alphas_first_index[i-1] +
                alphas_of_color_cnt[i-1];
    }
    pnm_freerow(xelrow);
    ppm_freecolorhash(cht);
}



static void
sortAlphaPalette(gray *alphas_of_color[],
                 unsigned int alphas_first_index[],
                 unsigned int alphas_of_color_cnt[],
                 unsigned int const colors,
                 unsigned int mapping[],
                 unsigned int * const transSizeP) {
/*----------------------------------------------------------------------------
   Remap the palette indices so opaque entries are last.

   alphas_of_color[], alphas_first_index[], and alphas_of_color_cnt[]
   describe an unsorted PNG (alpha/color) palette.  We generate
   mapping[] such that mapping[x] is the index into the sorted PNG
   palette of the alpha/color pair whose index is x in the unsorted
   PNG palette.  This mapping sorts the palette so that opaque entries
   are last.
-----------------------------------------------------------------------------*/
    unsigned int bot_idx;
    unsigned int top_idx;
    unsigned int colorIndex;
    
    /* We start one index at the bottom of the palette index range
       and another at the top.  We run through the unsorted palette,
       and when we see an opaque entry, we map it to the current top
       cursor and bump it down.  When we see a non-opaque entry, we map 
       it to the current bottom cursor and bump it up.  Because the input
       and output palettes are the same size, the two cursors should meet
       right when we process the last entry of the unsorted palette.
    */    
    bot_idx = 0;
    top_idx = alphas_first_index[colors-1] + alphas_of_color_cnt[colors-1] - 1;
    
    for (colorIndex = 0;  colorIndex < colors;  ++colorIndex) {
        unsigned int j;
        for (j = 0; j < alphas_of_color_cnt[colorIndex]; ++j) {
            unsigned int const paletteIndex = 
                alphas_first_index[colorIndex] + j;
            if (alphas_of_color[colorIndex][j] == PALETTEOPAQUE)
                mapping[paletteIndex] = top_idx--;
                else
                    mapping[paletteIndex] = bot_idx++;
        }
    }
    /* indices should have just crossed paths */
    if (bot_idx != top_idx + 1) {
        pm_error ("internal inconsistency: "
                  "remapped bot_idx = %u, top_idx = %u",
                  bot_idx, top_idx);
    }
    *transSizeP = bot_idx;
}



static void
compute_alpha_palette(FILE *         const ifP, 
                      int            const cols,
                      int            const rows,
                      xelval         const maxval,
                      int            const format,
                      pm_filepos     const imagepos,
                      gray **        const alpha_mask,
                      pixel                palette_pnm[],
                      gray                 trans_pnm[],
                      unsigned int * const paletteSizeP,
                      unsigned int * const transSizeP,
                      bool *         const tooBigP) {
/*----------------------------------------------------------------------------
   Return the palette of color/alpha pairs for the image indicated by
   'ifP', 'cols', 'rows', 'maxval', 'format', and 'imagepos'.
   alpha_mask[] is the Netpbm-style alpha mask for the image.

   Return the palette as the arrays palette_pnm[] and trans_pnm[].
   The ith entry in the palette is the combination of palette[i],
   which defines the color, and trans[i], which defines the
   transparency.

   Return the number of entries in the palette as *paletteSizeP.

   The palette is sorted so that the opaque entries are last, and we return
   *transSizeP as the number of non-opaque entries.

   palette[] and trans[] are allocated by the caller to at least 
   MAXPALETTEENTRIES elements.

   If there are more than MAXPALETTEENTRIES color/alpha pairs in the image, 
   don't return any palette information -- just return *tooBigP == TRUE.
-----------------------------------------------------------------------------*/
    colorhist_vector chv;
    unsigned int colors;

    gray *alphas_of_color[MAXPALETTEENTRIES];
    unsigned int alphas_first_index[MAXPALETTEENTRIES];
    unsigned int alphas_of_color_cnt[MAXPALETTEENTRIES];
 
    getChv(ifP, imagepos, cols, rows, maxval, format, MAXCOLORS, 
           &chv, &colors);

    computeUnsortedAlphaPalette(ifP, cols, rows, maxval, format, imagepos,
                                alpha_mask, MAXPALETTEENTRIES, chv, colors,
                                alphas_of_color,
                                alphas_first_index,
                                alphas_of_color_cnt);

    *paletteSizeP = 
        alphas_first_index[colors-1] + alphas_of_color_cnt[colors-1];
    if (*paletteSizeP > MAXPALETTEENTRIES) {
        *tooBigP = TRUE;
    } else {
        unsigned int mapping[MAXPALETTEENTRIES];
            /* Sorting of the alpha/color palette.  mapping[x] is the
               index into the sorted PNG palette of the alpha/color
               pair whose index is x in the unsorted PNG palette.
               This mapping sorts the palette so that opaque entries
               are last.  
            */

        *tooBigP = FALSE;

        /* Make the opaque palette entries last */
        sortAlphaPalette(alphas_of_color, alphas_first_index,
                         alphas_of_color_cnt, colors,
                         mapping, transSizeP);

        {
            unsigned int colorIndex;

            for (colorIndex = 0; colorIndex < colors; ++colorIndex) {
                unsigned int j;
                for (j = 0; j < alphas_of_color_cnt[colorIndex]; ++j) {
                    unsigned int const paletteIndex = 
                        alphas_first_index[colorIndex] + j;
                    palette_pnm[mapping[paletteIndex]] = chv[colorIndex].color;
                    trans_pnm[mapping[paletteIndex]] = 
                    alphas_of_color[colorIndex][j];
                }
            }
        }
    }
    { 
        unsigned int colorIndex;
        for (colorIndex = 0; colorIndex < colors + 1; ++colorIndex)
            free(alphas_of_color[colorIndex]);
    }
} 




static void
makeOneColorTransparentInPalette(xel            const transColor, 
                                 bool           const exact,
                                 pixel                palette_pnm[],
                                 unsigned int   const paletteSize,
                                 gray                 trans_pnm[],
                                 unsigned int * const transSizeP) {
/*----------------------------------------------------------------------------
   Find the color 'transColor' in the color/alpha palette defined by
   palette_pnm[], paletteSize, trans_pnm[] and *transSizeP.  

   Make that entry fully transparent.

   Rearrange the palette so that that entry is first.  (The PNG compressor
   can do a better job when the opaque entries are all last in the 
   color/alpha palette).

   If the specified color is not there and exact == TRUE, return
   without changing anything, but issue a warning message.  If it's
   not there and exact == FALSE, just find the closest color.

   We assume every entry in the palette is opaque upon entry.
-----------------------------------------------------------------------------*/
    unsigned int transparentIndex;
    unsigned int distance;
    
    if (*transSizeP != 0)
        pm_error("Internal error: trying to make a color in the palette "
                 "transparent where there already is one.");

    closestColorInPalette(transColor, palette_pnm, paletteSize, 
                          &transparentIndex, &distance);

    if (distance != 0 && exact) {
        pm_message ("specified transparent color not present in palette; "
                    "ignoring -transparent");
        errorlevel = PNMTOPNG_WARNING_LEVEL;
    } else {        
        /* Swap this with the first entry in the palette */
        pixel tmp;
    
        tmp = palette_pnm[transparentIndex];
        palette_pnm[transparentIndex] = palette_pnm[0];
        palette_pnm[0] = tmp;
        
        /* Make it transparent */
        trans_pnm[0] = PGM_TRANSPARENT;
        *transSizeP = 1;
        if (verbose) {
            pixel const p = palette_pnm[0];
            pm_message("Making all occurences of color (%u, %u, %u) "
                       "transparent.",
                       PPM_GETR(p), PPM_GETG(p), PPM_GETB(p));
        }
    }
}



static void
findOrAddBackgroundInPalette(pixel          const backColor, 
                             pixel                palette_pnm[], 
                             unsigned int * const paletteSizeP,
                             unsigned int * const backgroundIndexP) {
/*----------------------------------------------------------------------------
  Add the background color 'backColor' to the palette, unless
  it's already in there.  If it's not present and there's no room to
  add it, choose a background color that's already in the palette,
  as close to 'backColor' as possible.

  If we add an entry to the palette, make it opaque.  But in searching the 
  existing palette, ignore transparency.

  Note that PNG specs say that transparency of the background is meaningless;
  i.e. a viewer must ignore the transparency of the palette entry when 
  using the background color.

  Return the palette index of the background color as *backgroundIndexP.
-----------------------------------------------------------------------------*/
    int backgroundIndex;  /* negative means not found */
    unsigned int paletteIndex;

    backgroundIndex = -1;
    for (paletteIndex = 0; 
         paletteIndex < *paletteSizeP; 
         ++paletteIndex) 
        if (PPM_EQUAL(palette_pnm[paletteIndex], backColor))
            backgroundIndex = paletteIndex;

    if (backgroundIndex >= 0) {
        /* The background color is already in the palette. */
        *backgroundIndexP = backgroundIndex;
        if (verbose) {
            pixel const p = palette_pnm[*backgroundIndexP];
            pm_message ("background color (%u, %u, %u) appears in image.",
                        PPM_GETR(p), PPM_GETG(p), PPM_GETB(p));
        }
    } else {
        /* Try to add the background color, opaque, to the palette. */
        if (*paletteSizeP < MAXCOLORS) {
            /* There's room, so just add it to the end of the palette */

            /* Because we're not expanding the transparency palette, this
               entry is not in it, and is thus opaque.
            */
            *backgroundIndexP = (*paletteSizeP)++;
            palette_pnm[*backgroundIndexP] = backColor;
            if (verbose) {
                pixel const p = palette_pnm[*backgroundIndexP];
                pm_message ("added background color (%u, %u, %u) to palette.",
                            PPM_GETR(p), PPM_GETG(p), PPM_GETB(p));
            }
        } else {
            closestColorInPalette(backColor, palette_pnm, *paletteSizeP,
                                  backgroundIndexP, NULL);
            errorlevel = PNMTOPNG_WARNING_LEVEL;
            if (verbose) {
                pixel const p = palette_pnm[*backgroundIndexP];
                pm_message ("no room in palette for background color; "
                            "using closest match (%u, %u, %u) instead",
                            PPM_GETR(p), PPM_GETG(p), PPM_GETB(p));
            }
        }
    }
}



static void 
buildColorLookup(pixel                   palette_pnm[], 
                 unsigned int      const paletteSize,
                 colorhash_table * const chtP) {
/*----------------------------------------------------------------------------
   Create a colorhash_table out of the palette described by
   palette_pnm[] (which has 'paletteSize' entries) so one can look up
   the palette index of a given color.

   Where the same color appears twice in the palette, the lookup table
   finds an arbitrary one of them.  We don't consider transparency of
   palette entries, so if the same color appears in the palette once
   transparent and once opaque, the lookup table finds an arbitrary one
   of those two.
-----------------------------------------------------------------------------*/
    colorhash_table const cht = ppm_alloccolorhash();
    unsigned int paletteIndex;

    for (paletteIndex = 0; paletteIndex < paletteSize; ++paletteIndex) {
        ppm_addtocolorhash(cht, &palette_pnm[paletteIndex], paletteIndex);
    }
    *chtP = cht;
}


static void 
buildColorAlphaLookup(pixel              palette_pnm[], 
                      unsigned int const paletteSize,
                      gray               trans_pnm[], 
                      unsigned int const transSize,
                      gray         const alphaMaxval,
                      coloralphahash_table * const cahtP) {
    
    coloralphahash_table const caht = alloccoloralphahash();

    unsigned int paletteIndex;

    for (paletteIndex = 0; paletteIndex < paletteSize; ++paletteIndex) {
        gray paletteTrans;

        if (paletteIndex < transSize)
            paletteTrans = alphaMaxval;
        else
            paletteTrans = trans_pnm[paletteIndex];


        addtocoloralphahash(caht, &palette_pnm[paletteIndex],
                            &trans_pnm[paletteIndex], paletteIndex);
    }
    *cahtP = caht;
}



static void
tryAlphaPalette(FILE *         const ifP,
                int            const cols,
                int            const rows,
                xelval         const maxval,
                int            const format,
                pm_filepos     const imagepos,
                gray **        const alpha_mask,
                FILE *         const pfP,
                pixel *        const palette_pnm,
                unsigned int * const paletteSizeP,
                gray *         const trans_pnm,
                unsigned int * const transSizeP,
                const char **  const impossibleReasonP) {
    
    bool tooBig;
    if (pfP)
        pm_error("This program is not capable of generating "
                 "a PNG with transparency when you specify "
                 "the palette with -palette.");

    compute_alpha_palette(ifP, cols, rows, maxval, format, 
                          imagepos,  alpha_mask, palette_pnm, trans_pnm, 
                          paletteSizeP, transSizeP, &tooBig);
    if (tooBig) {
        asprintfN(impossibleReasonP,
                  "too many color/transparency pairs "
                  "(more than the PNG maximum of %u", 
                  MAXPALETTEENTRIES);
    } else
        *impossibleReasonP = NULL;
} 



static void
computePixelWidth(int            const pnm_type,
                  unsigned int   const pnm_meaningful_bits,
                  bool           const alpha,
                  unsigned int * const bitsPerSampleP,
                  unsigned int * const bitsPerPixelP) {

    unsigned int bitsPerSample, bitsPerPixel;

    if (pnm_type == PPM_TYPE || alpha) {
        /* PNG allows only depths of 8 and 16 for a truecolor image 
           and for a grayscale image with an alpha channel.
          */
        if (pnm_meaningful_bits > 8)
            bitsPerSample = 16;
        else 
            bitsPerSample = 8;
    } else {
        /* A grayscale, non-colormapped, no-alpha PNG may have any 
             bit depth from 1 to 16
          */
        if (pnm_meaningful_bits > 8)
            bitsPerSample = 16;
        else if (pnm_meaningful_bits > 4)
            bitsPerSample = 8;
        else if (pnm_meaningful_bits > 2)
            bitsPerSample = 4;
        else if (pnm_meaningful_bits > 1)
            bitsPerSample = 2;
        else
            bitsPerSample = 1;
    }
    if (alpha) {
        if (pnm_type == PPM_TYPE)
            bitsPerPixel = 4 * bitsPerSample;
        else
            bitsPerPixel = 2 * bitsPerSample;
    } else {
        if (pnm_type == PPM_TYPE)
            bitsPerPixel = 3 * bitsPerSample;
        else
            bitsPerPixel = bitsPerSample;
    }
    if (bitsPerPixelP)
        *bitsPerPixelP = bitsPerPixel;
    if (bitsPerSampleP)
        *bitsPerSampleP = bitsPerSample;
}



static unsigned int
paletteIndexBits(unsigned int const nColors) {
/*----------------------------------------------------------------------------
  Return the number of bits that a palette index in the PNG will
  occupy given that the palette has 'nColors' colors in it.  It is 1,
  2, 4, or 8 bits.
  
  If 'nColors' is not a valid PNG palette size, return 0.
-----------------------------------------------------------------------------*/
    unsigned int retval;

    if (nColors < 1)
        retval = 0;
    else if (nColors <= 2)
        retval = 1;
    else if (nColors <= 4)
        retval = 2;
    else if (nColors <= 16)
        retval = 4;
    else if (nColors <= 256)
        retval = 8;
    else
        retval = 0;

    return retval;
}



static void
computeColorMap(FILE *         const ifP,
                pm_filepos     const imagepos,
                int            const cols,
                int            const rows,
                xelval         const maxval,
                int            const format,
                bool           const force,
                FILE *         const pfP,
                bool           const alpha,
                int            const transparent,
                pixel          const transcolor,
                bool           const transexact,
                int            const background,
                pixel          const backcolor,
                gray **        const alpha_mask,
                unsigned int   const pnm_meaningful_bits,
                /* Outputs */
                pixel *        const palette_pnm,
                unsigned int * const paletteSizeP,
                gray *         const trans_pnm,
                unsigned int * const transSizeP,
                unsigned int * const backgroundIndexP,
                const char **  const noColormapReasonP) {
/*---------------------------------------------------------------------------
  Determine whether to do a colormapped or truecolor PNG and if
  colormapped, compute the full PNG palette -- both color and
  transparency.

  If we decide to do truecolor, we return as *noColormapReasonP a text
  description of why, in newly malloced memory.  If we decide to go
  with colormapped, we return *noColormapReasonP == NULL.

  In the colormapped case, we return the palette as arrays
  palette_pnm[] and trans_pnm[], allocated by Caller, with sizes
  *paletteSizeP and *transSizeP.

  If the image is to have a background color, we return the palette index
  of that color as *backgroundIndexP.
-------------------------------------------------------------------------- */
    if (force)
        asprintfN(noColormapReasonP, "You requested no color map");
    else if (maxval > PALETTEMAXVAL)
        asprintfN(noColormapReasonP, "The maxval of the input image (%u) "
                  "exceeds the PNG palette maxval (%u)", 
                  maxval, PALETTEMAXVAL);
    else {
        unsigned int bitsPerPixel;
        computePixelWidth(PNM_FORMAT_TYPE(format), pnm_meaningful_bits, alpha,
                          NULL, &bitsPerPixel);

        if (!pfP && bitsPerPixel == 1)
            /* No palette can beat 1 bit per pixel -- no need to waste time
               counting the colors.
            */
            asprintfN(noColormapReasonP, "pixel is already only 1 bit");
        else {
            /* We'll have to count the colors ('colors') to know if a
               palette is possible and desirable.  Along the way, we'll
               compute the actual set of colors (chv) too, and then create
               the palette itself if we decide we want one.
            */
            colorhist_vector chv;
            unsigned int colors;
            
            getChv(ifP, imagepos, cols, rows, maxval, format, MAXCOLORS, 
                   &chv, &colors);

            if (chv == NULL) {
                asprintfN(noColormapReasonP, 
                          "More than %u colors found -- too many for a "
                          "colormapped PNG", MAXCOLORS);
            } else {
                /* There are few enough colors that a palette is possible */
                if (bitsPerPixel <= paletteIndexBits(colors) && !pfP)
                    asprintfN(noColormapReasonP, 
                              "palette index for %u colors would be "
                              "no smaller than the indexed value (%u bits)", 
                              colors, bitsPerPixel);
                else {
                    unsigned int paletteSize;
                    unsigned int transSize;
                    if (alpha)
                        tryAlphaPalette(ifP, cols, rows, maxval, format,
                                        imagepos, alpha_mask, pfP,
                                        palette_pnm, &paletteSize, 
                                        trans_pnm, &transSize,
                                        noColormapReasonP);

                    else {
                        *noColormapReasonP = NULL;

                        compute_nonalpha_palette(chv, colors, maxval, pfP,
                                                 palette_pnm, &paletteSize, 
                                                 trans_pnm, &transSize);
    
                        if (transparent != -1)
                            makeOneColorTransparentInPalette(
                                transcolor, transexact, 
                                palette_pnm, paletteSize, trans_pnm, 
                                &transSize);
                    }
                    if (!*noColormapReasonP) {
                        if (background > -1)
                            findOrAddBackgroundInPalette(
                                backcolor, palette_pnm, &paletteSize,
                                backgroundIndexP);
                        *paletteSizeP = paletteSize;
                        *transSizeP   = transSize;
                    }
                }
            }
            freeChv();
        }
    }
}



static void computeColorMapLookupTable(
    bool                   const colorMapped,
    pixel                        palette_pnm[],
    unsigned int           const palette_size,
    gray                         trans_pnm[],
    unsigned int           const trans_size,
    bool                   const alpha,
    xelval                 const alpha_maxval,
    colorhash_table *      const chtP,
    coloralphahash_table * const cahtP) {
/*----------------------------------------------------------------------------
   Compute applicable lookup tables for the palette index.  If there's no
   alpha mask, this is just a standard Netpbm colorhash_table.  If there's
   an alpha mask, it is the slower Pnmtopng-specific 
   coloralphahash_table.

   If a lookup table is not applicable to the image, return NULL as
   its address.  (If the image is not colormapped, both will be NULL).
-----------------------------------------------------------------------------*/
    if (colorMapped) {
        if (alpha) {
            buildColorAlphaLookup(palette_pnm, palette_size, 
                                  trans_pnm, trans_size, alpha_maxval, cahtP);
            *chtP = NULL;
        } else { 
            buildColorLookup(palette_pnm, palette_size, chtP);
            *cahtP = NULL;
        }
        if (verbose)
            pm_message("PNG palette has %u entries, %u of them non-opaque",
                       palette_size, trans_size);
    } else {
        *chtP = NULL;
        *cahtP = NULL;
    }
}



static void
computeRasterWidth(bool           const colorMapped,
                   unsigned int   const palette_size,
                   int            const pnm_type,
                   unsigned int   const pnm_meaningful_bits,
                   bool           const alpha,
                   unsigned int * const bitsPerSampleP,
                   unsigned int * const bitsPerPixelP) {
/*----------------------------------------------------------------------------
   Compute the number of bits per raster sample and per raster pixel:
   *bitsPerSampleP and *bitsPerPixelP.  Note that a raster element may be a
   palette index, or a gray value or color with or without alpha mask.
-----------------------------------------------------------------------------*/
    if (colorMapped) {
        /* The raster element is a palette index */
        if (palette_size <= 2)
            *bitsPerSampleP = 1;
        else if (palette_size <= 4)
            *bitsPerSampleP = 2;
        else if (palette_size <= 16)
            *bitsPerSampleP = 4;
        else
            *bitsPerSampleP = 8;
        *bitsPerPixelP = *bitsPerSampleP;
        if (verbose)
            pm_message("Writing %d-bit color indexes", *bitsPerSampleP);
    } else {
        /* The raster element is an explicit pixel -- color and transparency */
        computePixelWidth(pnm_type, pnm_meaningful_bits, alpha,
                          bitsPerSampleP, bitsPerPixelP);

        if (verbose)
            pm_message("Writing %d bits per component per pixel", 
                       *bitsPerSampleP);
    }
}


static void
createPngPalette(pixel              palette_pnm[], 
                 unsigned int const paletteSize, 
                 pixval       const maxval,
                 gray               trans_pnm[],
                 unsigned int const transSize,
                 gray               alpha_maxval,
                 png_color          palette[],
                 png_byte           trans[]) {
/*----------------------------------------------------------------------------
   Create the data structure to be passed to the PNG compressor to represent
   the palette -- the whole palette, color + transparency.

   This is basically just a maxval conversion from the Netpbm-format
   equivalents we get as input.
-----------------------------------------------------------------------------*/
    unsigned int i;

    for (i = 0; i < paletteSize; ++i) {
        pixel p;
        PPM_DEPTH(p, palette_pnm[i], maxval, PALETTEMAXVAL);
        palette[i].red   = PPM_GETR(p);
        palette[i].green = PPM_GETG(p);
        palette[i].blue  = PPM_GETB(p);
#ifdef DEBUG
        pm_message("palette[%u] = (%d, %d, %d)",
                   i, palette[i].red, palette[i].green, palette[i].blue);
#endif
    }

    for (i = 0; i < transSize; ++i) {
        unsigned int const newmv = PALETTEMAXVAL;
        unsigned int const oldmv = alpha_maxval;
        trans[i] = (trans_pnm[i] * newmv + (oldmv/2)) / oldmv;
#ifdef DEBUG
        pm_message("trans[%u] = %d", i, trans[i]);
#endif
    }
}


static int 
convertpnm(FILE * const ifp, 
           FILE * const afp, 
           FILE * const pfp, 
           FILE * const tfp,
           float  const gamma,
           int    const interlace,
           int    const downscale,
           bool   const transparent_opt,
           char * const transstring,
           bool   const alpha_opt,
           char * const alpha_file,
           int    const background,
           char * const backstring,
           int    const hist,
           struct chroma const chroma,
           int    const phys_x,
           int    const phys_y,
           int    const phys_unit,
           int    const text,
           int    const ztxt,
           char * const text_file,
           int    const mtime,
           char * const date_string,
           char * const time_string,
           int    const filterSet,
           int    const force,
           struct zlib_compression zlib_compression
    ) {

  xel p;
  int rows, cols, format;
  xelval maxval;
      /* The maxval of the input image */
  xelval png_maxval;
      /* The maxval of the samples in the PNG output 
         (must be 1, 3, 7, 15, 255, or 65535)
      */
  pixel transcolor;
      /* The color that is to be transparent, with maxval equal to that
         of the input image.
      */
  int transexact;  
    /* boolean: the user wants only the exact color he specified to be
       transparent; not just something close to it.
    */
  int transparent;
  bool alpha;
    /* There will be an alpha mask */
  unsigned int pnm_meaningful_bits;
  pixel backcolor;
      /* The background color, with maxval equal to that of the input
         image.
      */
  png_struct *png_ptr;
  png_info *info_ptr;

  bool colorMapped;
  pixel palette_pnm[MAXCOLORS];
  png_color palette[MAXCOLORS];
      /* The color part of the color/alpha palette passed to the PNG
         compressor 
      */
  unsigned int palette_size;

  gray trans_pnm[MAXCOLORS];
  png_byte  trans[MAXCOLORS];
      /* The alpha part of the color/alpha palette passed to the PNG
         compressor 
      */
  unsigned int trans_size;

  colorhash_table cht;
  coloralphahash_table caht;

  unsigned int background_index;
      /* Index into palette[] of the background color. */

  png_uint_16 histogram[MAXCOLORS];
  png_byte *line;
  png_byte *pp;
  int pass;
  gray alpha_maxval;
  int alpha_rows;
  int alpha_cols;
  int alpha_can_be_transparency_index;
  const char * noColormapReason;
      /* The reason that we shouldn't make a colormapped PNG, or NULL if
         we should.  malloc'ed null-terminated string.
      */
  unsigned int depth;
      /* The number of bits per sample in the (uncompressed) png 
         raster -- if the raster contains palette indices, this is the
         number of bits in the index.
      */
  unsigned int fulldepth;
      /* The total number of bits per pixel in the (uncompressed) png
         raster, including all channels 
      */
  int x, y;
  pm_filepos imagepos;  
      /* file position in input image file of start of image (i.e. after
         the header)
      */
  xel *xelrow;    /* malloc'ed */
      /* The row of the input image currently being processed */

  /* these variables are declared static because gcc wasn't kidding
   * about "variable XXX might be clobbered by `longjmp' or `vfork'"
   * (stack corruption observed on Solaris 2.6 with gcc 2.8.1, even
   * in the absence of any other error condition) */
  static int pnm_type;
  static xelval maxmaxval;
  static gray **alpha_mask;
  

  /* these guys are initialized to quiet compiler warnings: */
  maxmaxval = 255;
  alpha_mask = NULL;
  depth = 0;
  errorlevel = 0;

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
    &pnmtopng_jmpbuf_struct, pnmtopng_error_handler, NULL);
  if (png_ptr == NULL) {
    pm_closer (ifp);
    pm_error ("cannot allocate main libpng structure (png_ptr)");
  }

  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
    pm_closer (ifp);
    pm_error ("cannot allocate libpng info structure (info_ptr)");
  }

  if (setjmp (pnmtopng_jmpbuf_struct.jmpbuf)) {
    png_destroy_write_struct (&png_ptr, &info_ptr);
    pm_closer (ifp);
    pm_error ("setjmp returns error condition (1)");
  }

  pnm_readpnminit (ifp, &cols, &rows, &maxval, &format);
  pm_tell2(ifp, &imagepos, sizeof(imagepos));
  pnm_type = PNM_FORMAT_TYPE (format);

  xelrow = pnm_allocrow(cols);

  if (verbose) {
    if (pnm_type == PBM_TYPE)    
      pm_message ("reading a PBM file (maxval=%d)", maxval);
    else if (pnm_type == PGM_TYPE)    
      pm_message ("reading a PGM file (maxval=%d)", maxval);
    else if (pnm_type == PPM_TYPE)    
      pm_message ("reading a PPM file (maxval=%d)", maxval);
  }

  if (pnm_type == PGM_TYPE)
    maxmaxval = PGM_OVERALLMAXVAL;
  else if (pnm_type == PPM_TYPE)
    maxmaxval = PPM_OVERALLMAXVAL;

  if (transparent_opt) {
    char * transstring2;  
      /* Same as transstring, but with possible leading '=' removed */
    if (transstring[0] == '=') {
      transexact = 1;
      transstring2 = transstring+1;
    } else {
      transexact = 0;
      transstring2 = transstring;
    }  
    /* We do this funny PPM_DEPTH thing instead of just passing 'maxval'
       to ppm_parsecolor() because ppm_parsecolor() does a cheap maxval
       scaling, and this is more precise.
    */
    PPM_DEPTH (transcolor, ppm_parsecolor(transstring2, maxmaxval),
               maxmaxval, maxval);
  }
  if (alpha_opt) {
    pixel alpha_transcolor;

    if (verbose)
      pm_message ("reading alpha-channel image...");
    alpha_mask = pgm_readpgm (afp, &alpha_cols, &alpha_rows, &alpha_maxval);

    if (alpha_cols != cols || alpha_rows != rows) {
      png_destroy_write_struct (&png_ptr, &info_ptr);
      pm_closer (ifp);
      pm_error ("dimensions for image and alpha mask do not agree");
    }
    alpha_trans(ifp, imagepos, cols, rows, maxval, format, 
                alpha_mask, alpha_maxval,
                &alpha_can_be_transparency_index, &alpha_transcolor);

    if (alpha_can_be_transparency_index && !force) {
      if (verbose)
        pm_message ("converting alpha mask to transparency index");
      alpha = FALSE;
      transparent = 2;
      transcolor = alpha_transcolor;
    } else {
      alpha = TRUE;
      transparent = -1;
    }
  } else {
      /* Though there's no alpha_mask, we still need an alpha_maxval for
         use with trans[], which can have stuff in it if the user specified
         a transparent color.
      */
      alpha = FALSE;
      alpha_maxval = 255;
      transparent = transparent_opt ? 1 : -1;
  }
  /* gcc 2.7.0 -fomit-frame-pointer causes stack corruption here */
  if (background > -1) 
      PPM_DEPTH(backcolor, ppm_parsecolor (backstring, maxmaxval), 
                maxmaxval, maxval);;

  /* first of all, check if we have a grayscale image written as PPM */

  if (pnm_type == PPM_TYPE && !force) {
    int isgray = TRUE;

    pm_seek2(ifp, &imagepos, sizeof(imagepos));
    for (y = 0 ; y < rows && isgray ; y++) {
      pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
      for (x = 0 ; x < cols && isgray ; x++) {
        p = xelrow[x];
        if (PPM_GETR (p) != PPM_GETG (p) || PPM_GETG (p) != PPM_GETB (p))
          isgray = FALSE;
      }
    }
    if (isgray)
      pnm_type = PGM_TYPE;
  }

  /* handle `odd' maxvalues */

    if (maxval > 65535 && !downscale) {
      png_destroy_write_struct (&png_ptr, &info_ptr);
      pm_closer (ifp);
      pm_error ("can only handle files up to 16-bit "
                "(use -downscale to override");
    }

  findRedundantBits(ifp, imagepos, cols, rows, maxval, format, alpha,
                    force, &pnm_meaningful_bits);
  
  computeColorMap(ifp, imagepos, cols, rows, maxval, format,
                  force, pfp, alpha, transparent, transcolor, transexact, 
                  background, backcolor,
                  alpha_mask, pnm_meaningful_bits,
                  palette_pnm, &palette_size, trans_pnm, &trans_size,
                  &background_index, &noColormapReason);

  if (noColormapReason) {
      if (pfp)
          pm_error("You specified a particular palette, but this image "
                   "cannot be represented by any palette.  %s",
                   noColormapReason);
      if (verbose)
          pm_message("Not using color map.  %s", noColormapReason);
      strfree(noColormapReason);
      colorMapped = FALSE;
  } else
      colorMapped = TRUE;
  
  computeColorMapLookupTable(colorMapped, palette_pnm, palette_size,
                             trans_pnm, trans_size, alpha, alpha_maxval,
                             &cht, &caht);

  computeRasterWidth(colorMapped, palette_size, pnm_type, 
                     pnm_meaningful_bits, alpha,
                     &depth, &fulldepth);
  if (verbose)
    pm_message ("writing a%s %d-bit %s%s file%s",
                fulldepth == 8 ? "n" : "", fulldepth,
                colorMapped ? "palette": 
                (pnm_type == PPM_TYPE ? "RGB" : "gray"),
                alpha ? (colorMapped ? "+transparency" : "+alpha") : "",
                interlace? " (interlaced)" : "");

  /* now write the file */

  png_maxval = pm_bitstomaxval(depth);

  if (setjmp (pnmtopng_jmpbuf_struct.jmpbuf)) {
    png_destroy_write_struct (&png_ptr, &info_ptr);
    pm_closer (ifp);
    pm_error ("setjmp returns error condition (2)");
  }

  png_init_io (png_ptr, stdout);
  info_ptr->width = cols;
  info_ptr->height = rows;
  info_ptr->bit_depth = depth;

  if (colorMapped)
    info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
  else if (pnm_type == PPM_TYPE)
    info_ptr->color_type = PNG_COLOR_TYPE_RGB;
  else
    info_ptr->color_type = PNG_COLOR_TYPE_GRAY;

  if (alpha && info_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
    info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;

  info_ptr->interlace_type = interlace;

  /* gAMA chunk */
  if (gamma != -1.0) {
    info_ptr->valid |= PNG_INFO_gAMA;
    info_ptr->gamma = gamma;
  }

  /* cHRM chunk */
  if (chroma.wx != -1.0) {
    info_ptr->valid |= PNG_INFO_cHRM;
    info_ptr->x_white = chroma.wx;
    info_ptr->y_white = chroma.wy;
    info_ptr->x_red = chroma.rx;
    info_ptr->y_red = chroma.ry;
    info_ptr->x_green = chroma.gx;
    info_ptr->y_green = chroma.gy;
    info_ptr->x_blue = chroma.bx;
    info_ptr->y_blue = chroma.by;
  }

  /* pHYS chunk */
  if (phys_unit != -1.0) {
    info_ptr->valid |= PNG_INFO_pHYs;
    info_ptr->x_pixels_per_unit = phys_x;
    info_ptr->y_pixels_per_unit = phys_y;
    info_ptr->phys_unit_type = phys_unit;
  }

  if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {

    /* creating PNG palette  (PLTE and tRNS chunks) */

    createPngPalette(palette_pnm, palette_size, maxval,
                     trans_pnm, trans_size, alpha_maxval, 
                     palette, trans);
    info_ptr->valid |= PNG_INFO_PLTE;
    info_ptr->palette = palette;
    info_ptr->num_palette = palette_size;
    if (trans_size > 0) {
        info_ptr->valid |= PNG_INFO_tRNS;
        info_ptr->trans = trans;
        info_ptr->num_trans = trans_size;   /* omit opaque values */
    }
    /* creating hIST chunk */
    if (hist) {
        colorhist_vector chv;
        unsigned int colors;
        colorhash_table cht;
        
        getChv(ifp, imagepos, cols, rows, maxval, format, MAXCOLORS, 
               &chv, &colors);

        cht = ppm_colorhisttocolorhash (chv, colors);
                
        { 
            unsigned int i;
            for (i = 0 ; i < MAXCOLORS; ++i) {
                int const chvIndex = ppm_lookupcolor(cht, &palette_pnm[i]);
                if (chvIndex == -1)
                    histogram[i] = 0;
                else
                    histogram[i] = chv[chvIndex].value;
            }
        }

        ppm_freecolorhash(cht);

        info_ptr->valid |= PNG_INFO_hIST;
        info_ptr->hist = histogram;
        if (verbose)
            pm_message ("histogram created");
    }
  } else { /* color_type != PNG_COLOR_TYPE_PALETTE */
    if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY ||
        info_ptr->color_type == PNG_COLOR_TYPE_RGB) {
        if (transparent > 0) {
            info_ptr->valid |= PNG_INFO_tRNS;
            info_ptr->trans_values = 
                xelToPngColor_16(transcolor, maxval, png_maxval);
        }
    } else {
        /* This is PNG_COLOR_MASK_ALPHA.  Transparency will be handled
           by the alpha channel, not a transparency color.
        */
    }
    if (verbose) {
        if (info_ptr->valid && PNG_INFO_tRNS) 
            pm_message("Transparent color {gray, red, green, blue} = "
                       "{%d, %d, %d, %d}",
                       info_ptr->trans_values.gray,
                       info_ptr->trans_values.red,
                       info_ptr->trans_values.green,
                       info_ptr->trans_values.blue);
        else
            pm_message("No transparent color");
    }
  }

  /* bKGD chunk */
  if (background > -1) {
      info_ptr->valid |= PNG_INFO_bKGD;
      if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
          info_ptr->background.index = background_index;
      } else {
          info_ptr->background = 
              xelToPngColor_16(backcolor, maxval, png_maxval);
          if (verbose)
              pm_message("Writing bKGD chunk with background color "
                         " {gray, red, green, blue} = {%d, %d, %d, %d}",
                         info_ptr->background.gray, 
                         info_ptr->background.red, 
                         info_ptr->background.green, 
                         info_ptr->background.blue ); 
      }
  }

  if (!colorMapped && (png_maxval != maxval || 
                       (alpha && png_maxval != alpha_maxval))) {
    /* We're writing in a bit depth that doesn't match the maxval of the
       input image and the alpha mask.  So we write an sBIT chunk to tell
       what the original image's maxval was.  The sBit chunk doesn't let
       us specify any maxval -- only powers of two minus one.  So we pick
       the power of two minus one which is greater than or equal to the
       actual input maxval.
    */
    /* sBIT chunk */
    int sbitval;

    info_ptr->valid |= PNG_INFO_sBIT;

    sbitval = pm_maxvaltobits(maxval);

    if (info_ptr->color_type & PNG_COLOR_MASK_COLOR) {
      info_ptr->sig_bit.red   = sbitval;
      info_ptr->sig_bit.green = sbitval;
      info_ptr->sig_bit.blue  = sbitval;
    } else {
      info_ptr->sig_bit.gray = sbitval;
    }
    if (verbose)
        pm_message("Writing sBIT chunk with sbitval = %d", sbitval);

    if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA) {
      info_ptr->sig_bit.alpha = pm_maxvaltobits(alpha_maxval);
    }
  }

  /* tEXT and zTXT chunks */
  if ((text) || (ztxt)) {
    pnmpng_read_text (info_ptr, tfp, ztxt, verbose);
  }

  /* tIME chunk */
  if (mtime) {
    struct tm time_struct;
    info_ptr->valid |= PNG_INFO_tIME;
    sscanf (date_string, "%d-%d-%d", &time_struct.tm_year,
                                     &time_struct.tm_mon,
                                     &time_struct.tm_mday);
    if (time_struct.tm_year > 1900)
      time_struct.tm_year -= 1900;
    time_struct.tm_mon--; /* tm has monthes 0..11 */
    sscanf (time_string, "%d:%d:%d", &time_struct.tm_hour,
                                     &time_struct.tm_min,
                                     &time_struct.tm_sec);
    png_convert_from_struct_tm (&info_ptr->mod_time, &time_struct);
  }

  /* explicit filter-type (or none) required */
  if (filterSet != 0)
  {
    png_set_filter (png_ptr, 0, filterSet);
  }

  /* zlib compression-level (or none) required */
  if (zlib_compression.level >= 0)
  {
    png_set_compression_level (png_ptr, zlib_compression.level);
  }

  /* write the png-info struct */
  png_write_info (png_ptr, info_ptr);

  if ((text) || (ztxt))
    /* prevent from being written twice with png_write_end */
    info_ptr->num_text = 0;

  if (mtime)
    /* prevent from being written twice with png_write_end */
    info_ptr->valid &= ~PNG_INFO_tIME;

  /* let libpng take care of, e.g., bit-depth conversions */
  png_set_packing (png_ptr);

  /* Write the raster */
 
  /* max: 3 color channels, one alpha channel, 16-bit */
  MALLOCARRAY(line, cols * 8);
  if (line == NULL)
  {
    png_destroy_write_struct (&png_ptr, &info_ptr);
    pm_closer (ifp);
    pm_error ("out of memory allocating PNG row buffer");
  }

  for (pass = 0 ; pass < png_set_interlace_handling (png_ptr) ; pass++) {
      unsigned int row;
      pm_seek2(ifp, &imagepos, sizeof(imagepos));
      for (row = 0; row < rows; ++row) {
          unsigned int col;
          pnm_readpnmrow(ifp, xelrow, cols, maxval, format);
          pnm_promoteformatrow(xelrow, cols, maxval, format, maxval, PPM_TYPE);
          pp = line;
          for (col = 0 ; col < cols ; ++col) {
              xel p_png;
              xel const p = xelrow[col];
              PPM_DEPTH(p_png, p, maxval, png_maxval);
              if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY ||
                  info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
                  if (depth == 16)
                      *pp++ = PNM_GET1 (p_png) >> 8;
                  *pp++ = PNM_GET1 (p_png)&0xff;
              } else if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
                  unsigned int paletteIndex;
                  if (alpha)
                      paletteIndex = lookupColorAlpha(caht, &p, 
                                                      &alpha_mask[row][col]);
                  else
                      paletteIndex = ppm_lookupcolor(cht, &p);
                  *pp++ = paletteIndex;
              } else if (info_ptr->color_type == PNG_COLOR_TYPE_RGB ||
                         info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
                  if (depth == 16)
                      *pp++ = PPM_GETR (p_png) >> 8;
                  *pp++ = PPM_GETR (p_png)&0xff;
                  if (depth == 16)
                      *pp++ = PPM_GETG (p_png) >> 8;
                  *pp++ = PPM_GETG (p_png)&0xff;
                  if (depth == 16)
                      *pp++ = PPM_GETB (p_png) >> 8;
                  *pp++ = PPM_GETB (p_png)&0xff;
              } else {
                  png_destroy_write_struct (&png_ptr, &info_ptr);
                  pm_closer (ifp);
                  pm_error (" (can't happen) undefined color_type");
              }
              if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA) {
                  int const png_alphaval = (int)
                      alpha_mask[row][col] * (float) png_maxval / maxval +0.5;
                  if (depth == 16)
                      *pp++ = png_alphaval >> 8;
                  *pp++ = png_alphaval & 0xff;
              }
          }
          png_write_row (png_ptr, line);
      }
  }
  png_write_end (png_ptr, info_ptr);


#if 0
  /* The following code may be intended to solve some segfault problem
     that arises with png_destroy_write_struct().  The latter is the
     method recommended in the libpng documentation and this program 
     will not compile under Cygwin because the Windows DLL for libpng
     does not contain png_write_destroy() at all.  Since the author's
     comment below does not make it clear what the segfault issue is,
     we cannot consider it.  -Bryan 00.09.15
*/

  png_write_destroy (png_ptr);
  /* flush first because free(png_ptr) can segfault due to jmpbuf problems
     in png_write_destroy */
  fflush (stdout);
  free (png_ptr);
  free (info_ptr);
#else
  png_destroy_write_struct(&png_ptr, &info_ptr);
#endif

  pnm_freerow(xelrow);

  if (cht)
      ppm_freecolorhash(cht);
  if (caht)
      freecoloralphahash(caht);

  return errorlevel;
}



static void
displayVersion() {

    fprintf(stderr,"pnmtopng version %s.\n", VERSION);

    /* We'd like to display the version of libpng with which we're
       linked, as we do for zlib, but it isn't practical.
       While libpng is capable of telling you what it's level
       is, different versions of it do it two different ways: with
       png_libpng_ver or with png_get_header_ver.  So we have to be
       compiled for a particular version just to find out what
       version it is! It's not worth having a link failure, much
       less a compile failure, if we choose wrong.
       png_get_header_ver is not in anything older than libpng 1.0.2a
       (Dec 1998).  png_libpng_ver is not there in libraries built
       without USE_GLOBAL_ARRAYS.  Cygwin versions are normally built
       without USE_GLOBAL_ARRAYS.  -bjh 2002.06.17.
    */
    fprintf(stderr, "   Compiled with libpng %s.\n",
            PNG_LIBPNG_VER_STRING);
    fprintf(stderr, "   Compiled with zlib %s; using zlib %s.\n",
            ZLIB_VERSION, zlib_version);
    fprintf(stderr,    
            "   Compiled with %d-bit netpbm support "
            "(PPM_OVERALLMAXVAL = %d).\n",
            pm_maxvaltobits (PPM_OVERALLMAXVAL), PPM_OVERALLMAXVAL);
    fprintf(stderr, "\n");
}



int 
main(int argc, char *argv[]) {

  float gamma = -1.0;
  int interlace = FALSE;
  int downscale = FALSE;
  bool transparent = FALSE;
  char *transstring;
  bool alpha = FALSE;
  char *alpha_file;
  int background = -1;
  char *backstring;
  int hist = FALSE;
  struct chroma chroma = {-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0};
  int phys_x = -1.0;
  int phys_y = -1.0;
  int phys_unit = -1.0;
  int text = FALSE;
  int ztxt = FALSE;
  char *text_file;
  int mtime = FALSE;
  char *date_string;
  char *time_string;
  int force = FALSE;
  int filterSet = 0;
  struct zlib_compression zlib_compression;

  FILE *ifp, *tfp, *afp, *pfp;
  char *palette_file;
      /* Name of user-supplied palette, file; NULL if none */
  int argn, errorlevel=0;

  pnm_init (&argc, argv);
  argn = 1;

  zlib_compression.level = -1;  /* initial value */
  /* TODO: switch this to shhopt, and make options for the rest of
     zlib_compression.
  */
  
  palette_file = NULL;   /* Initial value */

  while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0') {
    if (pm_keymatch (argv[argn], "-verbose", 2)) {
      verbose = TRUE;
    } else
    if (pm_keymatch (argv[argn], "-downscale", 2)) {
      downscale = TRUE;
    } else
    if (pm_keymatch (argv[argn], "-interlace", 2)) {
      interlace = TRUE;
    } else
    if (pm_keymatch (argv[argn], "-alpha", 2)) {
      if (transparent)
        pm_error ("-alpha and -transparent are mutually exclusive");
      alpha = TRUE;
      if (++argn < argc)
        alpha_file = argv[argn];
      else
        pm_error("-alpha requires a value");
    } else
    if (pm_keymatch (argv[argn], "-transparent", 3)) {
      if (alpha)
        pm_error ("-alpha and -transparent are mutually exclusive");
      transparent = TRUE;
      if (++argn < argc)
        transstring = argv[argn];
      else
        pm_error("-transparent requires a value");
    } else
    if (pm_keymatch (argv[argn], "-background", 2)) {
      background = 1;
      if (++argn < argc)
        backstring = argv[argn];
      else
        pm_error("-background requires a value");
    } else
    if (pm_keymatch (argv[argn], "-gamma", 2)) {
      if (++argn < argc)
        sscanf (argv[argn], "%f", &gamma);
      else
        pm_error("-gamma requires a value");
    } else
    if (pm_keymatch (argv[argn], "-hist", 2)) {
      hist = TRUE;
    } else
    if (pm_keymatch (argv[argn], "-chroma", 3)) {
      if (++argn < argc)
        sscanf (argv[argn], "%f", &chroma.wx);
      else
        pm_error("-chroma requires 6 values");
      if (++argn < argc)
        sscanf (argv[argn], "%f", &chroma.wy);
      else
        pm_error("-chroma requires 6 values");
      if (++argn < argc)
        sscanf (argv[argn], "%f", &chroma.rx);
      else
        pm_error("-chroma requires 6 values");
      if (++argn < argc)
        sscanf (argv[argn], "%f", &chroma.ry);
      else
        pm_error("-chroma requires 6 values");
      if (++argn < argc)
        sscanf (argv[argn], "%f", &chroma.gx);
      else
        pm_error("-chroma requires 6 values");
      if (++argn < argc)
        sscanf (argv[argn], "%f", &chroma.gy);
      else
        pm_error("-chroma requires 6 values");
      if (++argn < argc)
        sscanf (argv[argn], "%f", &chroma.bx);
      else
        pm_error("-chroma requires 6 values");
      if (++argn < argc)
        sscanf (argv[argn], "%f", &chroma.by);
      else
        pm_error("-chroma requires 6 values");
    } else
    if (pm_keymatch (argv[argn], "-phys", 3)) {
      if (++argn < argc)
        sscanf (argv[argn], "%d", &phys_x);
      else
        pm_error("-phys requires 3 values");
      if (++argn < argc)
        sscanf (argv[argn], "%d", &phys_y);
      else
        pm_error("-phys requires 3 values");
      if (++argn < argc)
        sscanf (argv[argn], "%d", &phys_unit);
      else
        pm_error("-phys requires 3 values");
    } else
    if (pm_keymatch (argv[argn], "-text", 3)) {
      text = TRUE;
      if (++argn < argc)
        text_file = argv[argn];
      else
        pm_error("-text requires a value");
    } else
    if (pm_keymatch (argv[argn], "-ztxt", 2)) {
      ztxt = TRUE;
      if (++argn < argc)
        text_file = argv[argn];
      else
        pm_error("-ztxt requires a value");
    } else
    if (pm_keymatch (argv[argn], "-time", 3)) {
      mtime = TRUE;
      if (++argn < argc) {
        date_string = argv[argn];
        if (++argn < argc)
          time_string = argv[argn];
        else
          pm_error("-time requires 2 values");
      } else {
        pm_error("-time requires 2 values");
      }
    } else 
    if (pm_keymatch (argv[argn], "-palette", 3)) {
      if (++argn < argc)
        palette_file = argv[argn];
      else
        pm_error("-palette requires a value");
    } else
    if (pm_keymatch (argv[argn], "-filter", 3)) {
      if (++argn < argc)
      {
        int filter;
        sscanf (argv[argn], "%d", &filter);
        if ((filter < 0) || (filter > 4))
          pm_error("-filter is obsolete.  Use -nofilter, -sub, -up, -avg, "
                   "and -paeth options instead.");
        else
          switch (filter) {
          case 0: filterSet = PNG_FILTER_NONE;  break;
          case 1: filterSet = PNG_FILTER_SUB;   break;
          case 2: filterSet = PNG_FILTER_UP;    break;
          case 3: filterSet = PNG_FILTER_AVG;   break;
          case 4: filterSet = PNG_FILTER_PAETH; break;
          }
      }
      else
        pm_error("-filter is obsolete.  Use -nofilter, -sub, -up, -avg, "
                 "and -paeth options instead.");
    } else
    if (pm_keymatch (argv[argn], "-nofilter", 4))
      filterSet |= PNG_FILTER_NONE;
    else if (pm_keymatch (argv[argn], "-sub", 3))
      filterSet |= PNG_FILTER_SUB;
    else if (pm_keymatch (argv[argn], "-up", 3))
      filterSet |= PNG_FILTER_UP;
    else if (pm_keymatch (argv[argn], "-avg", 3))
      filterSet |= PNG_FILTER_AVG;
    else if (pm_keymatch (argv[argn], "-paeth", 3))
      filterSet |= PNG_FILTER_PAETH;
    else
    if (pm_keymatch (argv[argn], "-compression", 3)) {
      if (++argn < argc)
      {
        sscanf (argv[argn], "%d", &zlib_compression.level);
        if ((zlib_compression.level < 0) || (zlib_compression.level > 9))
        {
          pm_error("zlib compression must be between 0 (none) and 9 (max)");
        }
      }
      else
        pm_error("-compression requires a value");
    } else
    if (pm_keymatch (argv[argn], "-force", 3)) {
      force = TRUE;
    } else
    if (pm_keymatch (argv[argn], "-libversion", 3))
      displayVersion();  /* exits program */
    else
      pm_error("Unrecognized option: '%s'", argv[argn]);
    argn++;
  }

  { 
      const char *input_file;
      if (argn == argc)
          input_file = "-";
      else {
          input_file = argv[argn];
          argn++;
      }
      ifp = pm_openr_seekable(input_file);
  }

  if (argn != argc)
    pm_error("Too many arguments.");

  if (alpha)
    afp = pm_openr (alpha_file);
  else
    afp = NULL;

  if (palette_file)
    pfp = pm_openr (palette_file);
  else
    pfp = NULL;

  if ((text) || (ztxt))
    tfp = pm_openr (text_file);
  else
    tfp = NULL;

  errorlevel = convertpnm(ifp, afp, pfp, tfp,
                          gamma, interlace, downscale, transparent,
                          transstring, alpha, alpha_file, background,
                          backstring, hist, chroma, phys_x, phys_y,
                          phys_unit, text, ztxt, text_file, mtime,
                          date_string, time_string, filterSet,
                          force, zlib_compression);

  if (afp)
    pm_closer (afp);
  if (pfp)
    pm_closer (pfp);
  if (tfp)
    pm_closer (tfp);

  pm_closer (ifp);
  pm_closew (stdout);

  return errorlevel;
}
