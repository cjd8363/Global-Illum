/* pnmrotate.c - read a portable anymap and rotate it by some angle
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

#include <math.h>
#include <limits.h>

#include "pnm.h"
#include "shhopt.h"
#include "mallocvar.h"

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /*M_PI*/

#define SCALE 4096
#define HALFSCALE 2048

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespecs of input file */
    float angle;                /* Angle to rotate, in radians */
    unsigned int noantialias;
    const char *background;  /* NULL if none */
    unsigned int debugStage1;
    unsigned int debugStage2;
};


enum rotationDirection {CLOCKWISE, COUNTERCLOCKWISE};

struct shearParm {
    /* These numbers tell how to shear a pixel, but I haven't figured out 
       yet exactly what each means.
    */
    long fracnew0;
    long omfracnew0;
    int intnew0;
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def = malloc(100*sizeof(optEntry));
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int backgroundSpec;
    unsigned int option_def_index;

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "background",  OPT_STRING, &cmdlineP->background, 
            &backgroundSpec,      0);
    OPTENT3(0, "noantialias", OPT_FLAG,   NULL, 
            &cmdlineP->noantialias, 0);
    OPTENT3(0, "debug_stage1", OPT_FLAG,   NULL, 
            &cmdlineP->debugStage1, 0);
    OPTENT3(0, "debug_stage2", OPT_FLAG,   NULL, 
            &cmdlineP->debugStage2, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = TRUE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!backgroundSpec)
        cmdlineP->background = NULL;

    if (argc-1 < 1)
        pm_error("You must specify at least one argument:  the angle "
                 "to rotate.");
    else {
        int rc;
        float angleArg;

        rc = sscanf(argv[1], "%f", &angleArg);

        if (rc != 1)
            pm_error("Invalid angle argument: '%s'.  Must be a floating point "
                     "number of degrees.", argv[1]);
        else if (angleArg < -90.0 || angleArg > 90.0)
            pm_error("angle must be between -90 and 90, inclusive.  "
                     "You specified %f.  "
                     "Use 'pamflip' for other rotations.", angleArg);
        else {
            /* Convert to radians */
            cmdlineP->angle = angleArg * M_PI / 180.0;

            if (argc-1 < 2)
                cmdlineP->inputFilespec = "-";
            else {
                cmdlineP->inputFilespec = argv[2];
                
                if (argc-1 > 2)
                    pm_error("Program takes at most two arguments "
                             "(angle and filename).  You specified %d",
                             argc-1);
            }
        }
    }
}



static void
computeNewFormat(bool const antialias, 
                 int const format, xelval const maxval,
                 int * const newformatP, xelval * const newmaxvalP) {

    if (antialias && PNM_FORMAT_TYPE(format) == PBM_TYPE) {
        *newformatP = PGM_TYPE;
        *newmaxvalP = PGM_MAXMAXVAL;
        pm_message( "promoting from PBM to PGM - "
                    "use -noantialias to avoid this" );
    } else {
        *newformatP = format;
        *newmaxvalP = maxval;
    }
}



static xel
backgroundColor(const char * const backgroundColorName,
                xel *        const topRow,
                int          const cols,
                xelval       const maxval,
                int          const format) {

    xel retval;

    if (backgroundColorName) {
        retval = ppm_parsecolor(backgroundColorName, maxval);

        switch(PNM_FORMAT_TYPE(format)) {
        case PGM_TYPE:
            if (!PPM_ISGRAY(retval))
                pm_error("Image is PGM (grayscale), "
                         "but you specified a non-gray "
                         "background color '%s'", backgroundColorName);

            break;
        case PBM_TYPE:
            if (!PNM_EQUAL(retval, pnm_whitexel(maxval, format)) &&
                !PNM_EQUAL(retval, pnm_blackxel(maxval, format)))
                pm_error("Image is PBM (black and white), "
                         "but you specified '%s', which is neither black "
                         "nor white, as background color", 
                         backgroundColorName);
            break;
        }
    }
    else 
        retval = pnm_backgroundxelrow(topRow, cols, maxval, format);

    return retval;
}



static void
shearX(xel * const inRow, 
       xel * const outRow, 
       int   const cols, 
       int   const format,
       xel   const bgxel,
       bool  const antialias,
       float const new0,
       int   const tempcols) {

    int const intnew0 = (int) new0;

    if ( antialias ) {
        long const fracnew0 = ( new0 - intnew0 ) * SCALE;
        long const omfracnew0 = SCALE - fracnew0;

        int col;
        xel * nxP;
        xel * xP;
        xel prevxel;

        for (col = 0; col < tempcols; ++col)
            outRow[col] = bgxel;
            
        prevxel = bgxel;
        for (col = 0, nxP = &(outRow[intnew0]), xP = inRow;
             col < cols; ++col, ++nxP, ++xP) {
            switch (PNM_FORMAT_TYPE(format)) {
            case PPM_TYPE:
                PPM_ASSIGN(*nxP,
                           (fracnew0 * PPM_GETR(prevxel) 
                            + omfracnew0 * PPM_GETR(*xP) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETG(prevxel) 
                            + omfracnew0 * PPM_GETG(*xP) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETB(prevxel) 
                            + omfracnew0 * PPM_GETB(*xP) 
                            + HALFSCALE) / SCALE );
                break;
                
            default:
                PNM_ASSIGN1(*nxP,
                            (fracnew0 * PNM_GET1(prevxel) 
                             + omfracnew0 * PNM_GET1(*xP) 
                             + HALFSCALE) / SCALE );
                break;
            }
            prevxel = *xP;
        }
        if (fracnew0 > 0 && intnew0 + cols < tempcols) {
            switch (PNM_FORMAT_TYPE(format)) {
            case PPM_TYPE:
                PPM_ASSIGN(*nxP,
                           (fracnew0 * PPM_GETR(prevxel) 
                            + omfracnew0 * PPM_GETR(bgxel) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETG(prevxel) 
                            + omfracnew0 * PPM_GETG(bgxel) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETB(prevxel) 
                            + omfracnew0 * PPM_GETB(bgxel) 
                            + HALFSCALE) / SCALE );
                break;
                    
            default:
                PNM_ASSIGN1(*nxP,
                            (fracnew0 * PNM_GET1(prevxel) 
                             + omfracnew0 * PNM_GET1(bgxel) 
                             + HALFSCALE) / SCALE );
                break;
            }
        }
    } else {
        int col;
        xel * nxP;
        
        for (col = 0, nxP = outRow; col < intnew0; ++col, ++nxP )
            *nxP = bgxel;
        for (col = 0; col < cols; ++col, ++nxP)
            *nxP = inRow[col];
        for (col = intnew0 + cols; col < tempcols; ++col, ++nxP)
            *nxP = bgxel;
    }
}



static void 
shearYNoAntialias(xel **           const inxels,
                  xel **           const outxels,
                  int              const cols,
                  int              const inrows,
                  int              const outrows,
                  int              const format,
                  xel              const bgxel,
                  struct shearParm const shearParm[]) {
/*----------------------------------------------------------------------------
   Shear the image in 'inxels' ('cols' x 'inrows') vertically into
   'outxels' ('cols' x 'outrows'), both format 'format'.  shearParm[X]
   tells how much to shear pixels in Column X and 'bgxel' is what to
   use for background where there is none of the input in the output.

   We do not do any antialiasing.  We simply move whole pixels.

   We go row by row instead of column by column to save real memory.  Going
   row by row, the working set is only a few pages, whereas going row by
   row, it would be one page per output row plus one page per input row.
-----------------------------------------------------------------------------*/
    int inrow;

    for (inrow = 0; inrow < inrows; ++inrow) {
        int col;
        for (col = 0; col < cols; ++col) {
            int const outrow = inrow + shearParm[col].intnew0;
            if (outrow >= 0 && outrow < outrows)
                outxels[outrow][col] = inxels[inrow][col];
        }
    }
    {
        /* Now fill in the pixels that we didn't fill above, with background */
        int col;
        for (col = 0; col < cols; ++col) {
            int startBgRow, endBgRow;
            int outrow;
            
            if (shearParm[col].intnew0 > 0 ) {
                startBgRow = 0;
                endBgRow   = shearParm[col].intnew0;
            } else {
                startBgRow = outrows + shearParm[col].intnew0;
                endBgRow   = outrows;
            }
            for (outrow = startBgRow; outrow < endBgRow; ++outrow)
                outxels[outrow][col] = bgxel;
        }
    }
}



static void
shearYColAntialias(xel ** const inxels, 
                   xel ** const outxels,
                   int    const col,
                   int    const inrows,
                   int    const outrows,
                   int    const format,
                   xel    const bgxel,
                   struct shearParm shearParm[]) {
/*-----------------------------------------------------------------------------
  Shear a column vertically.
-----------------------------------------------------------------------------*/
    long const fracnew0   = shearParm[col].fracnew0;
    long const omfracnew0 = shearParm[col].omfracnew0;
    int  const intnew0    = shearParm[col].intnew0;
        
    int outrow;

    xel prevxel;
    int inrow;
        
    /* Initialize everything to background color */
    for (outrow = 0; outrow < outrows; ++outrow)
        outxels[outrow][col] = bgxel;

    prevxel = bgxel;
    for (inrow = 0; inrow < inrows; ++inrow) {
        int const outrow = inrow + intnew0;

        if (outrow >= 0 && outrow < outrows) {
            xel * const nxP = &(outxels[outrow][col]);
            xel const x = inxels[inrow][col];
            switch ( PNM_FORMAT_TYPE(format) ) {
            case PPM_TYPE:
                PPM_ASSIGN(*nxP,
                           (fracnew0 * PPM_GETR(prevxel) 
                            + omfracnew0 * PPM_GETR(x) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETG(prevxel) 
                            + omfracnew0 * PPM_GETG(x) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETB(prevxel) 
                            + omfracnew0 * PPM_GETB(x) 
                            + HALFSCALE) / SCALE );
                break;
                        
            default:
                PNM_ASSIGN1(*nxP,
                            (fracnew0 * PNM_GET1(prevxel) 
                             + omfracnew0 * PNM_GET1(x) 
                             + HALFSCALE) / SCALE );
                break;
            }
            prevxel = x;
        }
    }
    if (fracnew0 > 0 && intnew0 + inrows < outrows) {
        xel * const nxP = &(outxels[intnew0 + inrows][col]);
        switch (PNM_FORMAT_TYPE(format)) {
        case PPM_TYPE:
            PPM_ASSIGN(*nxP,
                       (fracnew0 * PPM_GETR(prevxel) 
                        + omfracnew0 * PPM_GETR(bgxel) 
                        + HALFSCALE) / SCALE,
                       (fracnew0 * PPM_GETG(prevxel) 
                        + omfracnew0 * PPM_GETG(bgxel) 
                        + HALFSCALE) / SCALE,
                       (fracnew0 * PPM_GETB(prevxel) 
                        + omfracnew0 * PPM_GETB(bgxel) 
                        + HALFSCALE) / SCALE);
            break;
                
        default:
            PNM_ASSIGN1(*nxP,
                        (fracnew0 * PNM_GET1(prevxel) 
                         + omfracnew0 * PNM_GET1(bgxel) 
                         + HALFSCALE) / SCALE);
            break;
        }
    }
} 



static void
shearFinal(xel * const inRow, 
           xel * const outRow, 
           int   const incols, 
           int   const outcols,
           int   const format,
           xel   const bgxel,
           bool  const antialias,
           float const new0,
           int   const x2shearjunk) {

    long const fracnew0 = ( new0 - (int) new0 ) * SCALE; 
    long const omfracnew0 = SCALE - fracnew0; 
    int  const intnew0 = (int) new0 - x2shearjunk;

    int col;

    for (col = 0; col < outcols; ++col)
        outRow[col] = bgxel;
        
    if (antialias) {
        xel prevxel;
        int col;

        prevxel = bgxel;
        for (col = 0; col < incols; ++col) {
            int const new = intnew0 + col;
            if (new >= 0 && new < outcols) {
                xel * const nxP = &(outRow[new]);
                xel const x = inRow[col];
                switch (PNM_FORMAT_TYPE(format)) {
                case PPM_TYPE:
                    PPM_ASSIGN(*nxP,
                               (fracnew0 * PPM_GETR(prevxel) 
                                + omfracnew0 * PPM_GETR(x) 
                                + HALFSCALE) / SCALE,
                               (fracnew0 * PPM_GETG(prevxel) 
                                + omfracnew0 * PPM_GETG(x) 
                                + HALFSCALE) / SCALE,
                               (fracnew0 * PPM_GETB(prevxel) 
                                + omfracnew0 * PPM_GETB(x) 
                                + HALFSCALE) / SCALE);
                    break;
                    
                default:
                    PNM_ASSIGN1(*nxP,
                                (fracnew0 * PNM_GET1(prevxel) 
                                 + omfracnew0 * PNM_GET1(x) 
                                 + HALFSCALE) / SCALE );
                    break;
                }
                prevxel = x;
            }
        }
        if (fracnew0 > 0 && intnew0 + incols < outcols) {
            xel * const nxP = &(outRow[intnew0 + incols]);
            switch (PNM_FORMAT_TYPE(format)) {
            case PPM_TYPE:
                PPM_ASSIGN(*nxP,
                           (fracnew0 * PPM_GETR(prevxel) 
                            + omfracnew0 * PPM_GETR(bgxel) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETG(prevxel) 
                            + omfracnew0 * PPM_GETG(bgxel) 
                            + HALFSCALE) / SCALE,
                           (fracnew0 * PPM_GETB(prevxel) 
                            + omfracnew0 * PPM_GETB(bgxel) 
                            + HALFSCALE) / SCALE);
                break;
                
            default:
                PNM_ASSIGN1(*nxP,
                            (fracnew0 * PNM_GET1(prevxel) 
                             + omfracnew0 * PNM_GET1(bgxel) 
                             + HALFSCALE) / SCALE );
                break;
            }
        }
    } else {
        int col;
        for (col = 0; col < incols; ++col) {
            int const new = intnew0 + col;
            if (new >= 0 && new < outcols)
                outRow[new] = inRow[col];
        }
    }
}



static void
shearImageY(xel ** const inxels,
            xel ** const outxels,
            int    const cols,
            int    const inrows,
            int    const outrows,
            int    const format,
            xel    const bgxel,
            bool   const antialias,
            enum rotationDirection const direction,
            float  const yshearfac,
            int    const yshearjunk) {
    
    struct shearParm * shearParm;  /* malloc'ed */
    int col;

    MALLOCARRAY(shearParm, cols);
    if (shearParm == NULL)
        pm_error("Unable to allocate memory for shearParm");

    for ( col = 0; col < cols; ++col ) {
        float const new0 = yshearfac * 
            (direction == CLOCKWISE ? col : cols - col);

        shearParm[col].fracnew0   = ( new0 - (int) new0 ) * SCALE;
        shearParm[col].omfracnew0 = SCALE - shearParm[col].fracnew0;
        shearParm[col].intnew0    = (int) new0 - yshearjunk;
    }
    if (!antialias)
        shearYNoAntialias(inxels, outxels, cols, inrows, outrows, format,
                          bgxel, shearParm);
    else {
        /* TODO: do this row-by-row, same as for noantialias, to save
           real memory.
        */
        for ( col = 0; col < cols; ++col) 
            shearYColAntialias(inxels, outxels, col, inrows, outrows, format, 
                               bgxel, shearParm);
    }
    free(shearParm);
}



int
main(int argc, char *argv[]) { 

    struct cmdlineInfo cmdline;
    FILE* ifP;
    xel* xelrow;
    xel** temp1xels;
    xel** temp2xels;
    xel* newxelrow;
    xel bgxel;
    int rows, cols, format;
    int newformat, newrows;
    int tempcols, newcols;
    int yshearjunk, x2shearjunk;
    int row;
    xelval maxval, newmaxval;
    float xshearfac, yshearfac;
    enum rotationDirection direction;

    pnm_init( &argc, argv );

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);

    pnm_readpnminit(ifP, &cols, &rows, &maxval, &format);
    xelrow = pnm_allocrow(cols);
    
    computeNewFormat(!cmdline.noantialias, format, maxval, 
                     &newformat, &newmaxval);

    xshearfac = tan(cmdline.angle / 2.0);
    if (xshearfac < 0.0)
        xshearfac = -xshearfac;
    yshearfac = sin(cmdline.angle);
    if (yshearfac < 0.0)
        yshearfac = -yshearfac;
    overflow2(rows, xshearfac);
    overflow_add(cols, 1);
    overflow_add(rows * xshearfac, cols);
    tempcols = rows * xshearfac + cols + 0.999999;
    yshearjunk = (tempcols - cols) * yshearfac;
    newrows = tempcols * yshearfac + rows + 0.999999;
    x2shearjunk = (newrows - rows - yshearjunk) * xshearfac;
    newrows -= 2 * yshearjunk;

    if(newrows * xshearfac + tempcols + 0.999999 - 2 * x2shearjunk > INT_MAX)
        pm_error("image too large");

    newcols = newrows * xshearfac + tempcols + 0.999999 - 2 * x2shearjunk;
    direction = cmdline.angle > 0 ? COUNTERCLOCKWISE : CLOCKWISE;

    /* First shear X from input file into temp1xels. */
    temp1xels = pnm_allocarray(tempcols, rows);
    for (row = 0; row < rows; ++row) {
        float const new0 = xshearfac * (cmdline.angle > 0 ? row : (rows-row));

        pnm_readpnmrow(ifP, xelrow, cols, maxval, format);

        pnm_promoteformatrow(xelrow, cols, maxval, format, 
                             newmaxval, newformat);

        if (row == 0)
            bgxel = backgroundColor(cmdline.background, 
                                    xelrow, cols, newmaxval, format);

        shearX(xelrow, temp1xels[row], cols, newformat, bgxel, 
               !cmdline.noantialias, new0, tempcols);
    }
    pm_close(ifP);
    pnm_freerow(xelrow);

    if (cmdline.debugStage1) {
        pnm_writepnm(stdout, temp1xels, tempcols, rows, 
                     newmaxval, newformat, 0);
        exit(1);
    }
  
    /* Now inverse shear Y from temp1xels into temp2xels. */
    temp2xels = pnm_allocarray(tempcols, newrows);
    shearImageY(temp1xels, temp2xels, tempcols, rows, newrows, newformat,
                bgxel, !cmdline.noantialias, direction, yshearfac, yshearjunk);

    pnm_freearray(temp1xels, rows);

    if (cmdline.debugStage2) {
        pnm_writepnm(stdout, temp2xels, tempcols, newrows, 
                     newmaxval, newformat, 0);
        exit(1);
    }
  
    /* Finally, shear X from temp2xels to output file. */
    pnm_writepnminit(stdout, newcols, newrows, newmaxval, newformat, 0);
    newxelrow = pnm_allocrow(newcols);
    for (row = 0; row < newrows; ++row) {
        float const new0 = xshearfac * (cmdline.angle > 0 ? row : newrows-row);

        shearFinal(temp2xels[row], newxelrow, tempcols, newcols, newformat, 
                   bgxel, !cmdline.noantialias, new0, x2shearjunk);

        pnm_writepnmrow(stdout, newxelrow, newcols, newmaxval, newformat, 0);
    }
    
    pnm_freerow(newxelrow);
    pnm_freearray(temp2xels, newrows);
    pm_close(stdout);
    
    exit( 0 );
}

