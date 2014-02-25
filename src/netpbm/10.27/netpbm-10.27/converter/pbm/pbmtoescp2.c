/* pbmtoescp2.c - read a portable bitmap and produce Epson ESC/P2 raster
**                 graphics output data for Epson Stylus printers
**
** Copyright (C) 2003 by Ulrich Walcher (u.walcher@gmx.de)
**                       and Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/* I used the Epson ESC/P Reference Manual (1997) in writing this. */

#include "pbm.h"
#include "shhopt.h"

static char const esc = 033;

/* compress l data bytes from src to dest and return the compressed length */
static unsigned int
enc_epson_rle(const unsigned int l, 
              const unsigned char * src, 
              unsigned char * dest)
{
    unsigned int i;      /* index */
    unsigned int state;  /* run state */
    unsigned int pos;    /* source position */
    unsigned int dpos;   /* destination position */

    pos = dpos = state  = 0;
    while ( pos < l )
    {
        for (i=0; i<128 && pos+i<l; i++)
            /* search for begin of a run, smallest useful run is 3
               equal bytes 
            */
            if(src[pos+i]==src[pos+i+1] && src[pos+i]==src[pos+i+2])
            {
                state=1;                       /* set run state */
                break;
            }
	if(i)
	{
        /* set counter byte for copy through */
        dest[dpos] = i-1;       
        /* copy data bytes before run begin or end cond. */
        memcpy(dest+dpos+1,src+pos,i);    
        pos+=i; dpos+=i+1;                 /* update positions */
	}
    if (state)
    {
        for (i=0; src[pos+i]==src[pos+i+1] && i<128 && pos+i<l; i++);
        /* found the runlength i */
        dest[dpos]   = 257-i;           /* set counter for byte repetition */
        dest[dpos+1] = src[pos];        /* set byte to be repeated */
        pos+=i; dpos+=2; state=0;       /* update positions, reset run state */
        }
    }
    return dpos;
}



int
main(int argc, char* argv[]) {

    FILE* ifp;
    int rows, cols;
    int format;
    unsigned int row, idx, len;
    unsigned int cpr = 1;       /* set default mode: compressed */
    unsigned int res=360;       /* set default resolution: 360dpi */
    unsigned int h, v;
    unsigned char *bytes, *cprbytes;

    pbm_init( &argc, argv );

    {
        optStruct3 opt;
        unsigned int option_def_index = 0;
        optEntry *option_def = malloc(100*sizeof(optEntry));
        opt.opt_table = option_def;
        opt.short_allowed = FALSE;
        opt.allowNegNum = FALSE;
        OPTENT3(0, "compress",     OPT_UINT,    &cpr,       NULL, 0);
        OPTENT3(0, "resolution",   OPT_UINT,    &res,       NULL, 0);

        optParseOptions3(&argc, argv, opt, sizeof(opt), 0);

        if ( argc-1 > 1 )
            pm_error( "Too many arguments: %d.  "
                      "Only argument is the filename", argc-1 );
        if ( cpr != 0 && cpr != 1 )
            pm_error( "Invalid -compress value: %u.  Only 0 and 1 are valid.",
                      cpr );
        if ( res != 360 && res != 180)
            pm_error( "Invalid -resolution value: %u.  "
                      "Only 180 and 360 are valid.", res );
        
        if (argc-1 == 1)
            ifp = pm_openr(argv[1]);
        else
            ifp = stdin;
    }

    pbm_readpbminit(ifp, &cols, &rows, &format);

    if ((bytes    = malloc(24*pbm_packed_bytes(cols)+2)) == NULL ||
        (cprbytes = malloc(2*24*pbm_packed_bytes(cols))) == NULL )
            pm_error("Cannot allocate memory");

    h = v = 3600/res;

    /* Set raster graphic mode. */
    printf("%c%c%c%c%c%c", esc, '(', 'G', 1, 0, 1);

    /* Set line spacing in units of 1/360 inches. */
    printf("%c%c%c", esc, '+', 24*h/10);

    /* Write out raster stripes 24 rows high. */
    for (row = 0; row < rows; row += 24) {
        unsigned int const m = (rows-row<24) ? rows%24 : 24;
        printf("%c%c%c%c%c%c%c%c", esc, '.', cpr, v, h, m, cols%256, cols/256);
        /* Read pbm rows, each padded to full byte */
        for (idx = 0; idx < 24 && row+idx < rows; ++idx)
            pbm_readpbmrow_packed(ifp,bytes+idx*pbm_packed_bytes(cols),
                                  cols,format);
        /* Write raster data. */
        if (cpr) {
            /* compressed */
            len = enc_epson_rle(m * pbm_packed_bytes(cols), bytes, cprbytes);
            fwrite(cprbytes,len,1,stdout);
        }
        else
            /* uncompressed */
            fwrite(bytes,pbm_packed_bytes(cols),m,stdout);    

        if (rows-row >= 24) putchar('\n');
    }
    free(bytes); free (cprbytes);
    pm_close(ifp);

    /* Reset printer. */
    printf("%c%c", esc, '@');

    return 0;
}
