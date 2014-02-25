/******************************************************************************
                               pnmsplit
*******************************************************************************
  Split a Netpbm format input file into multiple Netpbm format output files
  with one image per output file.

  By Bryan Henderson, Olympia WA; June 2000

  Contributed to the public domain by its author.
******************************************************************************/

#define _BSD_SOURCE 1      /* Make sure strdup() is in string.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#include <string.h>
#include <stdio.h>
#include "pnm.h"
#include "shhopt.h"
#include "nstring.h"
#include "mallocvar.h"

struct cmdline_info {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *input_file;
    const char *output_file_pattern;
    unsigned int debug;
    unsigned int padname;
} cmdline;



static void
parse_command_line(int argc, char ** argv,
                   struct cmdline_info *cmdlineP) {
/*----------------------------------------------------------------------------
   Note that the pointers we place into *cmdlineP are sometimes to storage
   in the argv array.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to OptParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int padnameSpec;

    unsigned int option_def_index;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "debug",   OPT_FLAG, NULL, &cmdlineP->debug, 0);
    OPTENT3(0,   "padname", OPT_UINT, &cmdlineP->padname, &padnameSpec, 0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3(&argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */

    if (!padnameSpec)
        cmdlineP->padname = 0;

    if (argc - 1 < 1) 
        cmdlineP->input_file = "-";
    else
        cmdlineP->input_file = argv[1];
    if (argc -1 < 2)
        cmdlineP->output_file_pattern = "image%d";
    else
        cmdlineP->output_file_pattern = argv[2];

    if (!strstr(cmdlineP->output_file_pattern, "%d"))
        pm_error("output file spec pattern parameter must include the "
                 "string '%%d',\n"
                 "to stand for the image sequence number.\n"
                 "You specified '%s'.", cmdlineP->output_file_pattern);
}



static void
extract_one_image(FILE *infile, const char outputfilename[]) {

    FILE *outfile;
    xelval maxval;
    int rows, cols, format;
    enum pm_check_code check_retval;
    
    int row;
    xel *xelrow;

    pnm_readpnminit(infile, &cols, &rows, &maxval, &format);

    pnm_check(infile, PM_CHECK_BASIC, format, cols, rows, maxval, 
              &check_retval);

    outfile = pm_openw(outputfilename);
    pnm_writepnminit(outfile, cols, rows, maxval, format, 0);

    xelrow = pnm_allocrow(cols);
    for (row = 0; row < rows; row++) {
        pnm_readpnmrow(infile, xelrow, cols, maxval, format);
        pnm_writepnmrow(outfile, xelrow, cols, maxval, format, 0);
    }
    pnm_freerow(xelrow);
    pm_close(outfile);
}



static void
compute_output_name(char          const output_file_pattern[], 
                    unsigned int  const padCount,
                    unsigned int  const image_seq,
                    const char ** const output_name_p) {
/*----------------------------------------------------------------------------
   Compute the name of an output file given the pattern
   output_file_pattern[] and the image sequence number 'image_seq'.
   output_file_pattern[] contains at least one instance of the string
   "%d" and we substitute the ASCII decimal representation of
   image_seq for the firstone of them to generate the output file
   name.  We add leading zeroes as necessary to bring the number up to
   at least 'padCount' characters.
-----------------------------------------------------------------------------*/
    char *before_sub, *after_sub;
    const char * filenameFormat;
        /* A format string for asprintfN for the file name */

    before_sub = strdup(output_file_pattern);
    *(strstr(before_sub, "%d")) = '\0';

    after_sub = strstr(output_file_pattern, "%d") + 2;

    /* Make filenameFormat something like "%s%04u%s" */
    asprintfN(&filenameFormat, "%%s%%0%ud%%s", padCount);

    asprintfN(output_name_p, filenameFormat, before_sub, image_seq, after_sub);

    strfree(filenameFormat);

    free(before_sub);
}



int
main(int argc, char *argv[]) {

    FILE* ifP;
    int eof;  /* No more images in input */
    unsigned int image_seq;  
        /* Sequence of current image in input file.  First = 0 */

    pnm_init( &argc, argv );

    parse_command_line(argc, argv, &cmdline);

    ifP = pm_openr(cmdline.input_file);

    eof = FALSE;
    for (image_seq = 0; !eof; image_seq++) {
        const char *output_file_name;  /* malloc'ed */
        compute_output_name(cmdline.output_file_pattern, cmdline.padname, 
                            image_seq,
                            &output_file_name);
        pm_message("WRITING %s\n", output_file_name);
        extract_one_image(ifP, output_file_name);
        strfree(output_file_name);
        pnm_nextimage(ifP, &eof);
    }

    pm_close(ifP);
    
    return 0;
}
