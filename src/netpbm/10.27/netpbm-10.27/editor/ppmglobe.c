/*
 * This code written 2003
 * by Max Gensthaler <Max@Gensthaler.de>
 * Distributed under the Gnu Public License (GPL)
 *
 * Gensthaler called it 'ppmglobemap'.
 *
 * Translations of comments and C dialect by Bryan Henderson May 2003.
 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "ppm.h"


#ifdef M_PI
#define PI M_PI
#else
#define PI 3.1415926535897932384626433832795028841971693993751
#endif

int main(int argc, char *argv[])
{
    pixel **src;
    pixel **dst;
    int srcw, srch;
    int dstw, dsth;
    pixval srcmv, dstmv;
    int stripWidth;
    int y;

    ppm_init( &argc, argv );

    if(argc-1 != 1)
        pm_error("Program takes 1 argument: the strip count.  "
                 "You specified %d", argc-1);
  
    src = ppm_readppm(stdin, &srcw, &srch, &srcmv);

    stripWidth = srcw / atoi(argv[1]);

    dstw = srcw;
    dsth = srch;
    dstmv = srcmv;

    dst = ppm_allocarray(dstw, dsth);

    for(y=0 ; y<dsth ; y++)
    {
        double const factor = sin( PI * y / dsth );
        int const stripBorder = (int)( (stripWidth*(1.0-factor)/2.0) + 0.5);
            /* Distance from edge of strip in the original picture to
               edge of strip in the scaled picture in the current line 
            */

        int x;

        for(x = 0 ; x < dstw ; x++)
        {
            if( x % stripWidth < stripBorder
                || x % stripWidth > stripWidth - stripBorder)
            {
                PPM_ASSIGN(dst[y][x], 0, 0, 0);
            } else {
                int const origX = (int)((x / stripWidth) * stripWidth) + 
                    (int)( (int)(x % stripWidth - stripBorder) / factor);
                dst[y][x] = src[y][origX];
            }
        }
    }

    ppm_writeppm(stdout, dst, dstw, dsth, dstmv, 0);

    exit(0);
}
