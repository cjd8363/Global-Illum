/*=========================================================================
                             ppmcolormask
===========================================================================

  This program produces a PBM mask of areas containing a certain color.

  By Bryan Henderson, Olympia WA; April 2000.

  Contributed to the public domain by its author.
=========================================================================*/

#include <string.h>
#include "ppm.h"
#include "pbm.h"
#include "shhopt.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char * inputFilename;
    pixel        maskColor;
    unsigned int verbose;
};



static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that many of the strings that this function returns in the
   *cmdlineP structure are actually in the supplied argv array.  And
   sometimes, one of these strings is actually just a suffix of an entry
   in argv!
-----------------------------------------------------------------------------*/
    optEntry * option_def;
        /* Instructions to OptParseOptions3 on how to parse our options. */
    optStruct3 opt;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "verbose",    OPT_FLAG,   NULL, &cmdlineP->verbose,        0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and all of *cmdlineP. */

    if (argc - 1 == 0)
        pm_error("You must specify the color to mask as an argument.");
    cmdlineP->maskColor = ppm_parsecolor(argv[1], PPM_MAXMAXVAL);

    if (argc - 1 == 1)
        cmdlineP->inputFilename = "-";  /* he wants stdin */
    else if (argc - 1 == 2)
        cmdlineP->inputFilename = argv[2];
    else 
        pm_error("Too many arguments.  The only arguments accepted "
                 "are the mask color and optional input file specificaton");
}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;

    FILE * ifP;

    /* Parameters of input image: */
    int rows, cols;
    pixval maxval;
    int format;

    ppm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilename);

    ppm_readppminit(ifP, &cols, &rows, &maxval, &format);
    pbm_writepbminit(stdout, cols, rows, 0);
    {
        pixel * const inputRow = ppm_allocrow(cols);
        bit * const maskRow = pbm_allocrow(cols);
        {
            int row;
            for (row = 0; row < rows; ++row) {
                int col;
                ppm_readppmrow(ifP, inputRow, cols, maxval, format);
                for (col = 0; col < cols; ++col) {
                    if (PPM_EQUAL(inputRow[col], cmdline.maskColor)) 
                        maskRow[col] = PBM_BLACK;
                    else 
                        maskRow[col] = PBM_WHITE;
                }
                pbm_writepbmrow(stdout, maskRow, cols, 0);
            }
        }
        pbm_freerow(maskRow);
        ppm_freerow(inputRow);
    }
    pm_close(ifP);

    return 0;
}



