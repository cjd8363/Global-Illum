/* libppm1.c - ppm utility library part 1
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/* See libpbm.c for the complicated explanation of this 32/64 bit file
   offset stuff.
*/
#define _FILE_OFFSET_BITS 64
#define _LARGE_FILES  

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "ppm.h"
#include "libppm.h"
#include "pgm.h"
#include "libpgm.h"
#include "pbm.h"
#include "libpbm.h"
#include "fileio.h"

void
ppm_init( argcP, argv )
    int* argcP;
    char* argv[];
    {
    pgm_init( argcP, argv );
    }

void
ppm_nextimage(FILE * const fileP, 
              int *  const eofP) {
    pm_nextimage(fileP, eofP);
}



void
ppm_readppminitrest(FILE *   const file, 
                    int *    const colsP, 
                    int *    const rowsP, 
                    pixval * const maxvalP) {
    unsigned int maxval;

    /* Read size. */
    *colsP = (int)pm_getuint(file);
    *rowsP = (int)pm_getuint(file);

    /* Read maxval. */
    maxval = pm_getuint(file);
    if (maxval > PPM_OVERALLMAXVAL)
        pm_error("maxval of input image (%u) is too large.  "
                 "The maximum allowed by the PPM is %u.",
                 maxval, PPM_OVERALLMAXVAL); 
    if (maxval == 0)
        pm_error("maxval of input image is zero.");

    *maxvalP = maxval;
}



void
ppm_readppminit(FILE *   const fileP, 
                int *    const colsP, 
                int *    const rowsP, 
                pixval * const maxvalP, 
                int *    const formatP) {
    /* Check magic number. */
    *formatP = pm_readmagicnumber(fileP);
    switch (PPM_FORMAT_TYPE(*formatP)) {
	case PPM_TYPE:
        ppm_readppminitrest(fileP, colsP, rowsP, maxvalP);
        break;

	case PGM_TYPE:
        pgm_readpgminitrest(fileP, colsP, rowsP, maxvalP);
        break;

	case PBM_TYPE:
        pbm_readpbminitrest(fileP, colsP, rowsP);
        *maxvalP = 1;
        break;

	default:
        pm_error("bad magic number - not a ppm, pgm, or pbm file");
	}
}



void
ppm_readppmrow(FILE*  const fileP, 
               pixel* const pixelrow, 
               int    const cols, 
               pixval const maxval, 
               int    const format) {

    switch (format) {
	case PPM_FORMAT: {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            pixval const r = pm_getuint(fileP);
            pixval const g = pm_getuint(fileP);
            pixval const b = pm_getuint(fileP);
            PPM_ASSIGN(pixelrow[col], r, g, b);
	    }
    }
    break;

	case RPPM_FORMAT: {
        unsigned int col;
        for (col = 0; col < cols; ++col) {
            pixval const r = pgm_getrawsample(fileP, maxval);
            pixval const g = pgm_getrawsample(fileP, maxval);
            pixval const b = pgm_getrawsample(fileP, maxval);
            PPM_ASSIGN(pixelrow[col], r, g, b);
	    }
    }
	break;

	case PGM_FORMAT:
	case RPGM_FORMAT: {
        gray * const grayrow = pgm_allocrow(cols);
        unsigned int col;

        pgm_readpgmrow(fileP, grayrow, cols, maxval, format);
        for (col = 0; col < cols; ++col) {
            pixval const g = grayrow[col];
            PPM_ASSIGN(pixelrow[col], g, g, g);
	    }
        pgm_freerow(grayrow);
    }
    break;

	case PBM_FORMAT:
	case RPBM_FORMAT: {
        bit * const bitrow = pbm_allocrow(cols);
        unsigned int col;

        pbm_readpbmrow(fileP, bitrow, cols, format);
        for (col = 0; col < cols; ++col) {
            pixval const g = (bitrow[col] == PBM_WHITE) ? maxval : 0;
            PPM_ASSIGN(pixelrow[col], g, g, g);
	    }
        pbm_freerow(bitrow);
    }
    break;

	default:
        pm_error("Invalid format code");
	}
}



pixel**
ppm_readppm(FILE *   const fileP, 
            int *    const colsP, 
            int *    const rowsP, 
            pixval * const maxvalP) {
    pixel** pixels;
    int row;
    int format;

    ppm_readppminit(fileP, colsP, rowsP, maxvalP, &format);

    pixels = ppm_allocarray(*colsP, *rowsP);

    for (row = 0; row < *rowsP; ++row)
        ppm_readppmrow(fileP, pixels[row], *colsP, *maxvalP, format);

    return pixels;
}



void
ppm_check(FILE *               const fileP, 
          enum pm_check_type   const check_type, 
          int                  const format, 
          int                  const cols, 
          int                  const rows, 
          pixval               const maxval,
          enum pm_check_code * const retval_p) {

    if (rows < 0)
        pm_error("Invalid number of rows passed to ppm_check(): %d", rows);
    if (cols < 0)
        pm_error("Invalid number of columns passed to ppm_check(): %d", cols);
    
    if (check_type != PM_CHECK_BASIC) {
        if (retval_p) *retval_p = PM_CHECK_UNKNOWN_TYPE;
    } else if (PPM_FORMAT_TYPE(format) == PBM_TYPE) {
        pbm_check(fileP, check_type, format, cols, rows, retval_p);
    } else if (PPM_FORMAT_TYPE(format) == PGM_TYPE) {
        pgm_check(fileP, check_type, format, cols, rows, maxval, retval_p);
    } else if (format != RPPM_FORMAT) {
        if (retval_p) *retval_p = PM_CHECK_UNCHECKABLE;
    } else {        
        pm_filepos const bytes_per_row = cols * 3 * (maxval > 255 ? 2 : 1);
        pm_filepos const need_raster_size = rows * bytes_per_row;
        
        pm_check(fileP, check_type, need_raster_size, retval_p);
    }
}
