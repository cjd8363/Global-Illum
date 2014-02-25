/* libpgm2.c - pgm utility library part 2
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

#include "pgm.h"
#include "libpgm.h"

static void pgm_writepgmrowplain ARGS((FILE* file, gray* grayrow, int cols, gray maxval));
static void pgm_writepgmrowraw ARGS((FILE* file, gray* grayrow, int cols, gray maxval));

void
pgm_writepgminit(FILE * const fileP, 
                 int    const cols, 
                 int    const rows, 
                 gray   const maxval, 
                 int    const forceplain) {

    bool const plainFormat = forceplain || pm_plain_output;

    if (maxval > PGM_OVERALLMAXVAL && !plainFormat) 
        pm_error("too-large maxval passed to ppm_writepgminit(): %d.\n"
                 "Maximum allowed by the PGM format is %d.",
                 maxval, PGM_OVERALLMAXVAL);

    fprintf(fileP, "%c%c\n%d %d\n%d\n", 
            PGM_MAGIC1, 
            plainFormat || maxval >= 1<<16 ? PGM_MAGIC2 : RPGM_MAGIC2, 
            cols, rows, maxval );
#ifdef VMS
    if (!plainFormat)
        set_outfile_binary();
#endif
}



static void
putus(unsigned short const n, 
      FILE *         const fileP) {

    if (n >= 10)
        putus(n / 10, fileP);
    putc(n % 10 + '0', fileP);
}



void
pgm_writerawsample(FILE *file, const gray val, const gray maxval) {

    if (maxval < 256) {
        /* Samples fit in one byte, so write just one byte */
        int rc;
        rc = putc(val, file);
        if (rc == EOF)
            pm_error("Error writing single byte sample to file");
    } else {
        /* Samples are too big for one byte, so write two */
        int n_written;
        unsigned char outval[2];
        /* We could save a few instructions if we exploited the internal
           format of a gray, i.e. its endianness.  Then we might be able
           to skip the shifting and anding.
           */
        outval[0] = val >> 8;
        outval[1] = val & 0xFF;
        n_written = fwrite(&outval, 2, 1, file);
        if (n_written == 0) 
            pm_error("Error writing double byte sample to file");
    }
}



static void
pgm_writepgmrowraw(FILE *file, gray *grayrow, int cols, gray maxval ) {
    int col;

    for (col = 0; col < cols; ++col) {
#ifdef DEBUG
    if (grayrow[col] > maxval)
        pm_error( "value out of bounds (%u > %u)", grayrow[col], maxval);
#endif /*DEBUG*/
    pgm_writerawsample(file, grayrow[col], maxval);
    }
}



static void
pgm_writepgmrowplain(FILE * const fileP,
                     gray * const grayrow, 
                     int    const cols, 
                     gray   const maxval) {

    int col, charcount;
    gray* gP;

    charcount = 0;
    for (col = 0, gP = grayrow; col < cols; ++col, ++gP) {
        if (charcount >= 65) {
            putc('\n', fileP);
            charcount = 0;
        } else if (charcount > 0) {
            putc(' ', fileP);
            ++charcount;
        }
#ifdef DEBUG
        if (*gP > maxval)
            pm_error("value out of bounds (%u > %u)", *gP, maxval);
#endif /*DEBUG*/
        putus((unsigned short)*gP, fileP);
        charcount += 3;
    }
    if (charcount > 0)
        putc('\n', fileP);
}



void
pgm_writepgmrow(FILE* const fileP, 
                gray* const grayrow, 
                int   const cols, 
                gray  const maxval, 
                int   const forceplain) {

    if (forceplain || pm_plain_output || maxval >= 1<<16)
        pgm_writepgmrowplain(fileP, grayrow, cols, maxval);
    else
        pgm_writepgmrowraw(fileP, grayrow, cols, maxval);
}



#if __STDC__
void
pgm_writepgm( FILE* file, gray** grays, int cols, int rows, gray maxval, int forceplain )
#else /*__STDC__*/
void
pgm_writepgm( file, grays, cols, rows, maxval, forceplain )
    FILE* file;
    gray** grays;
    int cols, rows;
    gray maxval;
    int forceplain;
#endif /*__STDC__*/
    {
    int row;

    pgm_writepgminit( file, cols, rows, maxval, forceplain );

    for ( row = 0; row < rows; ++row )
     pgm_writepgmrow( file, grays[row], cols, maxval, forceplain );
    }
