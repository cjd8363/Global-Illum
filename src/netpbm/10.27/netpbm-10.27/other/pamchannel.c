/*----------------------------------------------------------------------------
                               pamchannel
------------------------------------------------------------------------------
  Part of the Netpbm package.

  Extract specified channels (planes) from a PAM image


  By Bryan Henderson, San Jose CA 2000.08.05

  Contributed to the public domain by its author 2000.08.05.
-----------------------------------------------------------------------------*/

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

#define MAX_CHANNELS 16
    /* The most channels we allow user to specify */

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_filespec;  /* Filespecs of input files */
    const char *tupletype;       /* Tuple type for output PAM */
    int n_channel;
        /* The number of channels to extract.  At least 1, at most 16. */
    unsigned int channel_to_extract[MAX_CHANNELS];
        /* The channel numbers to extract, in order. */
};


static void
parseCommandLine(int argc, char ** argv,
                 struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the file spec array we return is stored in the storage that
   was passed to us as the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;
    extern struct pam pam;  /* Just so we can look at field sizes */

    unsigned int option_def_index;
    unsigned int infileSpec, tupletypeSpec;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "infile",     OPT_STRING, &cmdlineP->input_filespec, 
            &infileSpec, 0);
    OPTENT3(0, "tupletype",  OPT_STRING, &cmdlineP->tupletype, 
            &tupletypeSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We may have parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!infileSpec)
        cmdlineP->input_filespec = "-";

    if (!tupletypeSpec)
        cmdlineP->tupletype = "";
    else
        if (strlen(cmdlineP->tupletype)+1 > sizeof(pam.tuple_type))
            pm_error("Tuple type name specified is too long.  Maximum of "
                     "%u characters allowed.", sizeof(pam.tuple_type));

    cmdlineP->n_channel = 0;  /* initial value */
    { 
        int argn;
        for (argn = 1; argn < argc; argn++) {
            int n;
            char *endptr;

            if (cmdlineP->n_channel >= MAX_CHANNELS) 
                pm_error("You may not specify more than %d channels.",
                         MAX_CHANNELS);
            
            n = strtol(argv[argn], &endptr, 10);
            if (n < 0)
                pm_error("Channel numbers cannot be negative.  "
                         "You specified %d", n);
            if (endptr == NULL)
                pm_error("non-numeric channel number argument: '%s'",
                         argv[argn]);
            
            cmdlineP->channel_to_extract[cmdlineP->n_channel++] = n;
        }
    }
    if (cmdlineP->n_channel < 1)
        pm_error("You must specify at least one channel to extract.");
}



static void
validate_channels(int          const n_channel, 
                  unsigned int const channels[], 
                  int          const depth) {

    int i;

    for (i = 0; i < n_channel; i++) 
        if (channels[i] > depth-1)
            pm_error("You specified channel number %d.  The highest numbered\n"
                     "channel in the input image is %d.",
                     channels[i], depth-1);

}



int
main(int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE* ifp;
    struct pam inpam;   /* Input PAM image */
    struct pam outpam;  /* Output PAM image */

    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);
    
    ifp = pm_openr(cmdline.input_filespec);

    pnm_readpaminit(ifp, &inpam, PAM_STRUCT_SIZE(tuple_type));

    validate_channels(cmdline.n_channel, cmdline.channel_to_extract, 
                      inpam.depth);

    outpam = inpam;     /* Initial value */
    outpam.file = stdout;
    outpam.depth = cmdline.n_channel;
    outpam.format = PAM_FORMAT;
    strcpy(outpam.tuple_type, cmdline.tupletype);
    
    pnm_writepaminit(&outpam);

    {
        tuple *inrow;
        tuple *outrow;
        
        inrow = pnm_allocpamrow(&inpam);      
        outrow = pnm_allocpamrow(&outpam);
        { 
            int row;
            
            for (row = 0; row < inpam.height; row++) {
                int col;
                pnm_readpamrow(&inpam, inrow);
                for (col = 0; col < inpam.width; col ++) {
                    int sample;
                    for (sample = 0; sample < cmdline.n_channel; sample++) 
                        outrow[col][sample] = 
                            inrow[col][cmdline.channel_to_extract[sample]];
                }
                pnm_writepamrow(&outpam, outrow);
            }
        }
        pnm_freepamrow(outrow);
        pnm_freepamrow(inrow);        
    }
    return 0;
}

