/* pnmsmooth.c - smooth out an image by replacing each pixel with the 
**               average of its width x height neighbors.
**
** Version 2.0   December 5, 1994
**
** Copyright (C) 1994 by Mike Burns (burns@chem.psu.edu)
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/*
  Written by Mike Burns December 5, 1994 and called Version 2.0.
  Based on ideas from a shell script by Jef Poskanzer, 1989, 1991.
  The shell script had no options.
*/


#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include "pnm.h"
#include "nstring.h"


static void
writeConvolutionImage(FILE *       const cofp,
                      unsigned int const cols,
                      unsigned int const rows,
                      int          const format) {

    xelval const convmaxval = rows * cols * 2;
        /* normalizing factor for our convolution matrix */
    xelval const g = rows * cols + 1;
        /* weight of all pixels in our convolution matrix */
    int row;
    xel *outputrow;

    if (convmaxval > PNM_OVERALLMAXVAL)
        pm_error("The convolution matrix is too large.  "
                 "Width x Height x 2\n"
                 "must not exceed %d and it is %d.",
                 PNM_OVERALLMAXVAL, convmaxval);

    pnm_writepnminit(cofp, cols, rows, convmaxval, format, 0);
    outputrow = pnm_allocrow(cols);

    for (row = 0; row < rows; ++row) {
        unsigned int col;
        for (col = 0; col < cols; ++col)
            PNM_ASSIGN1(outputrow[col], g);
        pnm_writepnmrow(cofp, outputrow, cols, convmaxval, format, 0);
    }
    pm_close(cofp);
    pnm_freerow(outputrow);
}



int
main(int argc, char ** argv) {

    FILE *cofp;
    const char * tempfileName;
    char *pnmfn;
    int argn;
    int format;
    int cols, rows;
    int status;
    bool DUMPFLAG = FALSE;
    const char * const usage = 
        "[-size width height] [-dump dumpfile] [pnmfile]";

    pnm_init( &argc, argv );

    /* set up defaults */
    cols = 3;
    rows = 3;
    format = PGM_FORMAT;
    pnmfn = (char *) 0;     /* initialize to NULL just in case */

    argn = 1;
    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
        if ( pm_keymatch( argv[argn], "-size", 2 ) )
        {
            ++argn;
            if ( argn+1 >= argc )
            {
                pm_message( "incorrect number of arguments for -size option" );
                pm_usage( usage );
            }
            else if ( argv[argn][0] == '-' || argv[argn+1][0] == '-' )
            {
                pm_message( "invalid arguments to -size option: %s %s", 
                            argv[argn], argv[argn+1] );
                pm_usage( usage );
            }
            if ( (cols = atoi(argv[argn])) == 0 )
                pm_error( "invalid width size specification: %s", argv[argn] );
            ++argn;
            if ( (rows = atoi(argv[argn])) == 0 )
                pm_error( "invalid height size specification: %s",argv[argn] );
            if ( cols % 2 != 1 || rows % 2 != 1 )
                pm_error( "the convolution matrix must have an odd number of rows and columns" );
        }
        else if ( pm_keymatch( argv[argn], "-dump", 2 ) )
        {
            ++argn;
            if ( argn >= argc )
            {
                pm_message( "missing argument to -dump option" );
                pm_usage( usage );
            }
            else if ( argv[argn][0] == '-' )
            {
                pm_message( "invalid argument to -dump option: %s", 
                            argv[argn] );
                pm_usage( usage );
            }
            cofp = pm_openw( argv[argn] );
            DUMPFLAG = TRUE;
        }
        else
            pm_usage( usage );
        ++argn;
    }

    /* Only get file name if given on command line to pass through to 
    ** pnmconvol.  If filename is coming from stdin, pnmconvol will read it.
    */
    if ( argn < argc )
    {
        pnmfn = argv[argn];
        ++argn;
    }

    if ( argn != argc )
        pm_usage( usage );


    if (!DUMPFLAG)
        pm_make_tmpfile(&cofp, &tempfileName);
        
    writeConvolutionImage(cofp, cols, rows, format);

    if (!DUMPFLAG) {
        /* fork a Pnmconvol process */
        pid_t rc;

        rc = fork();
        if (rc < 0)
            pm_error("fork() failed.  errno=%d (%s)", errno, strerror(errno));
        else if (rc == 0) {
            /* child process executes following code */

            /* If pnmfile name is not given on command line, then pnmfn will be
            ** (char *) 0 and the arglist will terminate there.
            */
            execlp("pnmconvol", "pnmconvol", tempfileName, pnmfn, NULL);
            pm_error("error executing pnmconvol command");
        } else {
            /* This is the parent */
            pid_t const childPid = rc;

            /* wait for child to finish */
            while (wait(&status) != childPid);
        }
    }
    unlink(tempfileName);
    strfree(tempfileName);

    return 0;
}






