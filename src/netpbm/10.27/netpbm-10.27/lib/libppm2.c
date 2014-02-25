/* libppm2.c - ppm utility library part 2
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

#include "ppm.h"
#include "libppm.h"



void
ppm_writeppminit(FILE*  const fileP, 
                 int    const cols, 
                 int    const rows, 
                 pixval const maxval, 
                 int    const forceplain) {

    bool const plainFormat = forceplain || pm_plain_output;

    if (maxval > PPM_OVERALLMAXVAL && !plainFormat) 
        pm_error("too-large maxval passed to ppm_writeppminit(): %d."
                 "Maximum allowed by the PPM format is %d.",
                 maxval, PPM_OVERALLMAXVAL);

	fprintf(fileP, "%c%c\n%d %d\n%d\n", 
            PPM_MAGIC1, 
            plainFormat || maxval >= 1<<16 ? PPM_MAGIC2 : RPPM_MAGIC2, 
            cols, rows, maxval );
#ifdef VMS
    if (!plainFormat)
        set_outfile_binary();
#endif
}



static void
putus(unsigned short n, FILE *file ) {
    if ( n >= 10 )
	putus( n / 10, file );
    (void) putc( n % 10 + '0', file );
    }

static void
ppm_writeppmrowraw(FILE *file, pixel *pixelrow, int cols, pixval maxval ) {
    register int col;
    register pixval val;

    for ( col = 0; col < cols; ++col )
	{
	val = PPM_GETR( pixelrow[col] );
#ifdef DEBUG
	if ( val > maxval )
	    pm_error( "r value out of bounds (%u > %u)", val, maxval );
#endif /*DEBUG*/
	pgm_writerawsample( file, val, maxval );
	val = PPM_GETG( pixelrow[col] );
#ifdef DEBUG
	if ( val > maxval )
	    pm_error( "g value out of bounds (%u > %u)", val, maxval );
#endif /*DEBUG*/
	pgm_writerawsample( file, val, maxval );
	val = PPM_GETB( pixelrow[col] );
#ifdef DEBUG
	if ( val > maxval )
	    pm_error( "b value out of bounds (%u > %u)", val, maxval );
#endif /*DEBUG*/
	pgm_writerawsample( file, val, maxval );
        }
    }

static void
ppm_writeppmrowplain(FILE *file, pixel *pixelrow, int cols, pixval maxval ) {
    register int col, charcount;
    register pixel* pP;
    register pixval val;

    charcount = 0;
    for ( col = 0, pP = pixelrow; col < cols; ++col, ++pP )
	{
	if ( charcount >= 65 )
	    {
	    (void) putc( '\n', file );
	    charcount = 0;
	    }
	else if ( charcount > 0 )
	    {
	    (void) putc( ' ', file );
	    (void) putc( ' ', file );
	    charcount += 2;
	    }
	val = PPM_GETR( *pP );
#ifdef DEBUG
	if ( val > maxval )
	    pm_error( "r value out of bounds (%u > %u)", val, maxval );
#endif /*DEBUG*/
	putus( val, file );
	(void) putc( ' ', file );
	val = PPM_GETG( *pP );
#ifdef DEBUG
	if ( val > maxval )
	    pm_error( "g value out of bounds (%u > %u)", val, maxval );
#endif /*DEBUG*/
	putus( val, file );
	(void) putc( ' ', file );
	val = PPM_GETB( *pP );
#ifdef DEBUG
	if ( val > maxval )
	    pm_error( "b value out of bounds (%u > %u)", val, maxval );
#endif /*DEBUG*/
	putus( val, file );
	charcount += 11;
	}
    if ( charcount > 0 )
	(void) putc( '\n', file );
    }

void
ppm_writeppmrow(FILE *  const fileP, 
                pixel * const pixelrow, 
                int     const cols, 
                pixval  const maxval, 
                int     const forceplain) {

    if (forceplain || pm_plain_output || maxval >= 1<<16) 
        ppm_writeppmrowplain(fileP, pixelrow, cols, maxval);
    else 
        ppm_writeppmrowraw(fileP, pixelrow, cols, maxval);
}



void
ppm_writeppm(FILE *  const file, 
             pixel** const pixels, 
             int     const cols, 
             int     const rows, 
             pixval  const maxval, 
             int     const forceplain)  {
    int row;
    
    ppm_writeppminit(file, cols, rows, maxval, forceplain);
    
    for (row = 0; row < rows; ++row)
        ppm_writeppmrow(file, pixels[row], cols, maxval, forceplain);
}
