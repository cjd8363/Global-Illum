/* escp2topbm.c - read an Epson ESC/P2 printer file and
**                 create a pbm file from the raster data,
**                 ignoring all other data.
**                 Can be regarded as a simple raster printer emulator
**                 with a RLE run length decoder.
**                 This program was made primarily for the test of pbmtoescp2
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

#include "pbm.h"

/* RLE decoder */
static unsigned int 
dec_epson_rle(const unsigned int k, 
              unsigned const char * in, 
              unsigned char * out)
{
    unsigned int  i, pos=0, dpos=0;

    while (dpos < k)
    {
        if (in[pos]<128)
        {
            for(i=0;i<in[pos]+1;i++) out[dpos+i]=in[pos+i+1];     
                /* copy through */
            pos+=i+1;
        }
        else
        {
            for (i=0; i<257-in[pos]; i++) out[dpos+i]=in[pos+1];  
                /* inflate this run */
            pos+=2;
        }
        dpos+=i;
    }
    return pos;        /* return number of treated input bytes */
}



int
main(int argc, char* argv[])
{
    FILE *ifp;
    unsigned int i, k, l, len, pos, opos, width, height, size;
    unsigned char *input, *output;

    pbm_init( &argc, argv );

    height = len = pos = opos = 0;
    size = 4096;        /* arbitrary value */

    if ((input  = malloc(size)) == NULL ||
        (output = malloc(size)) == NULL )
        pm_error("Cannot allocate memory");

    if ( argc-1 > 1 )
        pm_error("Too many arguments (%d).  Only argument is filename",
                 argc-1);

    if ( argc == 2 )
        ifp = pm_openr( argv[1] );
    else
        ifp = stdin;

    /* read the whole file */
    for (i=0;!feof(ifp);i++)          
    {
        input = (unsigned char*) realloc(input,(i+1)*size);
        if(!input) pm_error("Cannot allocate memory");
        l = fread (input+i*size,1,size,ifp);
        len+=l;
    }

    /* filter out raster data */
    while ( pos < len )           
    {
        /* only ESC. sequences are regarded  */
        if(input[pos]=='\x1b' && input[pos+1]=='.') 
        {
            height += input[pos+5];
            width = input[pos+7]*256+input[pos+6];
            k = input[pos+5]*((input[pos+7]*256+input[pos+6]+7)/8);
            output = (unsigned char*) realloc(output,opos+k);
            if(!output) pm_error("Cannot allocate memory");

            switch (input[pos+2]){
            case 0:
                /* copy the data block */
                memcpy(output+opos,input+pos+8,k);        
                pos+=k+8;opos+=k;
                break;
            case 1:
                /* inflate the data block */
                l = dec_epson_rle(k,input+pos+8,output+opos);  
                pos+=l+8;opos+=k;
                break;
            default:
                pm_error("unknown compression mode");
                break;
            }
        }
        else
            pos++;      /* skip bytes outside the ESC. sequence */
    }

    pbm_writepbminit(stdout,width,height,FALSE);
    fwrite(output,opos,1,stdout);
    free (input); free (output);
    fclose(stdout); fclose(ifp);

    return 0;
}
