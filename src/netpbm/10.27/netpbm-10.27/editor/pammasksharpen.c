#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  
    const char *maskFilespec;  
    unsigned int verbose;
};



static void
parseCommandLine(int argc, char ** const argv,
                 struct cmdlineInfo * const cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to OptParseOptions2 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;
    
    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENTRY */
    OPTENT3(0, "verbose",         OPT_FLAG,   NULL, &cmdlineP->verbose,   0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (argc-1 < 1)
        pm_error("You must specify at least one argument:  The name "
                 "of the mask image file");
    else {
        cmdlineP->maskFilespec = argv[1];
        if (argc-1 < 2)
            cmdlineP->inputFilespec = "-";
        else {
            cmdlineP->inputFilespec = argv[2];
        
            if (argc-1 > 2)
                pm_error("There are at most two arguments:  mask file name "
                         "and input file name.  You specified %d", argc-1);
        }
    }
}        



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    struct pam inpam;
    struct pam maskpam;
    struct pam outpam;
    FILE* ifP;
    FILE* maskfP;
    tuple* inputTuplerow;
    tuple* maskTuplerow;
    tuple* outputTuplerow;
    unsigned int row;
    
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.inputFilespec);
    maskfP = pm_openr(cmdline.maskFilespec);

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));
    pnm_readpaminit(maskfP, &maskpam, PAM_STRUCT_SIZE(tuple_type));

    if (inpam.width  != maskpam.width || 
        inpam.height != maskpam.height ||
        inpam.depth  != maskpam.depth)
        pm_error("The mask image must be the same dimensions as the "
                 "input image.  The mask is %dx%dx%d, but the input is "
                 "%dx%dx%d.",
                 maskpam.width, maskpam.height, maskpam.depth,
                 inpam.width,   inpam.height,   inpam.depth);
    if (inpam.maxval != maskpam.maxval)
        pm_error("The mask image must have the same maxval as the "
                 "input image.  The input image has maxval %u, "
                 "but the mask image has maxval %u",
                 (unsigned)inpam.maxval, (unsigned)maskpam.maxval);

    outpam = inpam;
    outpam.file = stdout;

    inputTuplerow  = pnm_allocpamrow(&inpam);
    maskTuplerow   = pnm_allocpamrow(&maskpam);
    outputTuplerow = pnm_allocpamrow(&outpam);

    pnm_writepaminit(&outpam);

    for (row = 0; row < outpam.height; ++row) {
        unsigned int col;
        pnm_readpamrow(&inpam,   inputTuplerow);
        pnm_readpamrow(&maskpam, maskTuplerow);
        
        for (col = 0; col < outpam.width; ++col) {
            unsigned int plane;
            
            for (plane = 0; plane < outpam.depth; ++plane) {
                unsigned int const rawResult = 
                    2 * inputTuplerow[col][plane] - maskTuplerow[col][plane];

                outputTuplerow[col][plane] = MIN(255, MAX(0, rawResult));
            }
        }
        pnm_writepamrow(&outpam, outputTuplerow);
    }

    pm_close(ifP);
    pm_close(maskfP);

    pnm_freepamrow(inputTuplerow);
    pnm_freepamrow(maskTuplerow);
    pnm_freepamrow(outputTuplerow);
    
    return 0;
}
