/* ----------------------------------------------------------------------
 *
 * Create a single image stereogram from a height map.
 * by Scott Pakin <scott+pbm@pakin.org>
 * Adapted to Netbpm conventions by Bryan Henderson.
 *
 * The core of this program is a simple adaptation of the code in
 * "Displaying 3D Images: Algorithms for Single Image Random Dot
 * Stereograms" by Harold W. Thimbleby, Stuart Inglis, and Ian
 * H. Witten in IEEE Computer, 27(10):38-48, October 1994.  See that
 * paper for a thorough explanation of what's going on here.
 *
 * ----------------------------------------------------------------------
 *
 * Copyright (c) 2004 Scott Pakin <scott+pbm@pakin.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---------------------------------------------------------------------- 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

/* Define a few helper macros. */
#define round2int(X) ((int)((X)+0.5))
#define EYESEP (round2int(eyesep*dpi))

typedef sample *(*COORD2COLOR)(int, int);
    /* A type to use for functions that map a 2-D coordinate to a color. */


enum outputType {OUTPUT_BW, OUTPUT_GRAYSCALE, OUTPUT_COLOR};

bool verbose;

/* ---------------------------------------------------------------------- */

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* '-' if stdin */
    unsigned int verbose;
    unsigned int crosseyed;
    unsigned int makemask;
    unsigned int dpi;
    float eyesep;
    unsigned int depth;
    pixval maxval;     /* 0 = unspecified.  Meaningless if patfile non-null */
    int guidesize;     /* 0 = unspecified */
    unsigned int magnifypat; /* Meaningless if patfile null */
    unsigned int xshift;     /* Meaningless if patfile null */
    unsigned int yshift;     /* Meaningless if patfile null */
    const char *patfile;     /* NULL = none */
    enum outputType outputType;  /* Meaningless if patfile non-null */
};



static void
parseCommandLine(int                 argc, 
                 char **             argv,
                 struct cmdlineInfo *cmdlineP ) {
/*----------------------------------------------------------------------------
   Parse program command line described in Unix standard form by argc
   and argv.  Return the information in the options as *cmdlineP.  

   If command line is internally inconsistent (invalid options, etc.),
   issue error message to stderr and abort program.

   Note that the strings we return are stored in the storage that
   was passed to us as the argv array.  We also trash *argv.
-----------------------------------------------------------------------------*/
    optEntry *option_def;
        /* Instructions to optParseOptions3 on how to parse our options.
         */
    optStruct3 opt;

    unsigned int option_def_index;

    unsigned int patfileSpec, dpiSpec, eyesepSpec, depthSpec,
        maxvalSpec, guidesizeSpec, magnifypatSpec, xshiftSpec, yshiftSpec;

    unsigned int blackandwhite, grayscale, color;

    
    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0, "verbose",         OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,       0);
    OPTENT3(0, "crosseyed",       OPT_FLAG,   NULL,                  
            &cmdlineP->crosseyed,     0);
    OPTENT3(0, "makemask",        OPT_FLAG,   NULL,                  
            &cmdlineP->makemask,      0);
    OPTENT3(0, "blackandwhite",   OPT_FLAG,   NULL,
            &blackandwhite,           0);
    OPTENT3(0, "grayscale",       OPT_FLAG,   NULL,
            &grayscale,               0);
    OPTENT3(0, "color",           OPT_FLAG,   NULL,
            &color,                   0);
    OPTENT3(0, "dpi",             OPT_UINT,   &cmdlineP->dpi,
            &dpiSpec,                 0);
    OPTENT3(0, "eyesep",          OPT_FLOAT,  &cmdlineP->eyesep,
            &eyesepSpec,              0);
    OPTENT3(0, "depth",           OPT_UINT,   &cmdlineP->depth,
            &depthSpec,               0);
    OPTENT3(0, "maxval",          OPT_UINT,   &cmdlineP->maxval,
            &maxvalSpec,              0);
    OPTENT3(0, "guidesize",       OPT_INT,    &cmdlineP->guidesize,
            &guidesizeSpec,           0);
    OPTENT3(0, "magnifypat",      OPT_UINT,   &cmdlineP->magnifypat,
            &magnifypatSpec,          0);
    OPTENT3(0, "xshift",          OPT_UINT,   &cmdlineP->xshift,
            &xshiftSpec,              0);
    OPTENT3(0, "yshift",          OPT_UINT,   &cmdlineP->yshift,
            &yshiftSpec,              0);
    OPTENT3(0, "patfile",         OPT_STRING, &cmdlineP->patfile,
            &patfileSpec,             0);

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0);
        /* Uses and sets argc, argv, and some of *cmdlineP and others. */


    if (blackandwhite + grayscale + color == 0)
        cmdlineP->outputType = OUTPUT_BW;
    else if (blackandwhite + grayscale + color > 1)
        pm_error("You may specify only one of -blackandwhite, -grayscale, "
                 "and -color");
    else {
        if (blackandwhite)
            cmdlineP->outputType = OUTPUT_BW;
        else if (grayscale)
            cmdlineP->outputType = OUTPUT_GRAYSCALE;
        else {
            assert(color);
            cmdlineP->outputType = OUTPUT_COLOR;
        }
    }
    if (!patfileSpec)
        cmdlineP->patfile = NULL;

    if (!dpiSpec)
        cmdlineP->dpi = 96;
    else if (cmdlineP->dpi < 1) 
        pm_error ("The argument to -dpi must be a positive integer");
       
    if (!eyesepSpec)
        cmdlineP->eyesep = 2.5;
    else if (cmdlineP->eyesep <= 0.0)
        pm_error ("The argument to -eyesep must be a positive number");

    if (!depthSpec)
        cmdlineP->depth = (1.0/3.0);
    else if (cmdlineP->depth < 0.0 || cmdlineP->depth > 1.0)
        pm_error ("The argument to -depth must be a number from 0.0 to 1.0");

    if (!maxvalSpec)
        cmdlineP->maxval = 0;

    if (!guidesizeSpec)
        cmdlineP->guidesize = 0;

    if (!magnifypatSpec)
        cmdlineP->magnifypat = 1;
    else if (cmdlineP->magnifypat < 1)
        pm_error ("The argument to -magnifypat must be a positive integer");

    if (!xshiftSpec)
        cmdlineP->xshift = 0;

    if (!yshiftSpec)
        cmdlineP->yshift = 0;

    if (xshiftSpec && !cmdlineP->makemask)
        pm_error("-xshift is valid only with -makemask");
    if (yshiftSpec && !cmdlineP->makemask)
        pm_error("-yshift is valid only with -makemask");

    if (cmdlineP->makemask && cmdlineP->patfile)
        pm_error("You may not specify both -makemask and -patfile");

    if (cmdlineP->patfile && blackandwhite)
        pm_error("-blackandwhite is not valid with -patfile");
    if (cmdlineP->patfile && grayscale)
        pm_error("-grayscale is not valid with -patfile");
    if (cmdlineP->patfile && color)
        pm_error("-color is not valid with -patfile");
    if (cmdlineP->patfile && maxvalSpec)
        pm_error("-maxval is not valid with -patfile");


    if (argc-1 < 1)
        cmdlineP->inputFilespec = "-";
    else if (argc-1 == 1)
        cmdlineP->inputFilespec = argv[1];
    else
        pm_error("Too many non-option arguments: %d.  Only argument is "
                 "input file name", argc-1);
}



static int
separation(double       const s,
           unsigned int const mu,
           double       const eyesep,
           unsigned int const dpi,
           sample       const maxval
           ) {
    
    return round2int((1.0-mu*(s))*EYESEP/(2.0-mu*(s)));
}



static struct {
    /* The state of the randomColor generator.  This is private to
       randomColor() and initRandomColor().
    */
    struct pam pam;
    unsigned int magnifypat;
    tuple * currentRow;
    unsigned int prevy;
} randomState;



static void
initRandomColor(const struct pam * const pamP,
                unsigned int       const magnifypat) {

    randomState.currentRow = pnm_allocpamrow(pamP);
    randomState.pam = *pamP;
    randomState.magnifypat = magnifypat;
    randomState.prevy = -magnifypat;

    srand(time(NULL));
}



/* Return a random RGB value. */
static tuple
randomColor(int const x, 
            int const y) {

    /* Every time we start a new row, we select a new sequence of random
       colors. 
    */
    if (y/randomState.magnifypat != randomState.prevy/randomState.magnifypat) {
        unsigned int col;

        for (col = 0; col < randomState.pam.width; ++col) {
            tuple const thisTuple =
                randomState.currentRow[col];

            unsigned int plane;

            for (plane = 0; plane < randomState.pam.depth; ++plane) {
                unsigned int const modulus = randomState.pam.maxval + 1;

                thisTuple[plane] = rand() % modulus;
                thisTuple[plane] = rand() % modulus;
                thisTuple[plane] = rand() % modulus;
            }
        }
    }
    
    /* Return the appropriate column from the pregenerated color row. */
    randomState.prevy = y;
    return randomState.currentRow[x/randomState.magnifypat];
}



static struct {
    /* This is the state of the patternPixel generator.  It is
       private to functions of the patternPixel generator.
    */
    struct pam   pam;
    tuple **     tuples;
    unsigned int xshift;
    unsigned int yshift;
    unsigned int magnifypat;
} patternPixelState;



static void
initPatternPixel(const struct pam * const pamP,
                 tuple **           const tuples,
                 unsigned int       const xshift,
                 unsigned int       const yshift,
                 unsigned int       const magnifypat) {

    patternPixelState.pam = *pamP;
    patternPixelState.tuples = tuples;
    patternPixelState.xshift = xshift;
    patternPixelState.yshift = yshift;
    patternPixelState.magnifypat = magnifypat;
}



/* Return a pixel from the pattern file. */
static sample *
patternPixel(int const x, 
             int const y) {

#define pp patternPixelState
    int patx, paty;

    paty = ((y - pp.yshift) / pp.magnifypat) % pp.pam.height;

    if (paty < 0)
        paty += pp.pam.height;

    patx = ((x - pp.xshift) / pp.magnifypat) % pp.pam.width;
    
    if (patx < 0)
        patx += pp.pam.width;

    return pp.tuples[paty][patx];
#undef pp
}



static void
setupColorFunc(struct cmdlineInfo const cmdline,
               const struct pam * const outPamP,
               tuple **           const patTuples,
               COORD2COLOR *      const colorFuncP) {

    if (cmdline.patfile) {
        initPatternPixel(outPamP, patTuples, 
                         cmdline.xshift, cmdline.yshift,
                         cmdline.magnifypat);
        *colorFuncP = patternPixel;
    } else {
        initRandomColor(outPamP, cmdline.magnifypat);
        *colorFuncP = randomColor;
    }
}



/* ---------------------------------------------------------------------- */


static void
makeWhiteRow(const struct pam * const pamP,
             tuple *            const tuplerow) {

    unsigned int col;

    for (col = 0; col < pamP->width; ++col) {
        unsigned int plane;
        for (plane = 0; plane < pamP->depth; ++plane)
            tuplerow[col][plane] = pamP->maxval;
    }
}



static void
writeRowCopies(const struct pam *  const outPamP,
               const tuple *       const outrow,
               unsigned int        const copyCount) {

    unsigned int i;
    for (i = 0; i < copyCount; ++i)
        pnm_writepamrow(outPamP, outrow);
}



/* Draw a pair of guide boxes. */
static void 
drawguides(int                const guidesize, 
           const struct pam * const outPamP,
           unsigned int       const depth,
           double             const eyesep,
           unsigned int       const dpi) {
    
    int const far = separation(0, depth, eyesep, dpi, outPamP->maxval); 
        /* Space between the two guide boxes. */
    int const width = outPamP->width;    /* Width of the output image */
    
    tuple *outrow;             /* One row of output data */
    tuple blackTuple;
    int col;

    pnm_createBlackTuple(outPamP, &blackTuple);
    
    outrow = pnm_allocpamrow(outPamP);
             
    /* Leave some blank rows before the guides. */
    makeWhiteRow(outPamP, outrow);
    writeRowCopies(outPamP, outrow, guidesize);

    /* Draw the guides. */
    if ((width - far + guidesize)/2 < 0 || 
        (width + far - guidesize)/2 >= width)
        pm_message("warning: the guide boxes are completely out of bounds "
                   "at %d DPI", dpi);
    else if ((width - far - guidesize)/2 < 0 || 
             (width + far + guidesize)/2 >= width)
        pm_message ("warning: the guide boxes are partially out of bounds "
                    "at %d DPI", dpi);

    for (col = (width - far - guidesize)/2; 
         col < (width - far + guidesize)/2; 
         ++col)
        if (col >= 0 && col < width)
            pnm_assigntuple(outPamP, outrow[col], blackTuple);

    for (col = (width + far - guidesize)/2; 
         col < (width + far + guidesize)/2; 
         ++col)
        if (col >= 0 && col < width)
            pnm_assigntuple(outPamP, outrow[col], blackTuple);

    writeRowCopies(outPamP,outrow, guidesize);

    /* Leave some blank rows after the guides. */
    makeWhiteRow(outPamP, outrow);
    writeRowCopies(outPamP, outrow, guidesize);

    pnm_freerow(outrow);
    pnm_freepamtuple(blackTuple);
}



static void
reportInputAndPattern(const struct pam * const heightPamP,
                      bool               const usingPattern,
                      const struct pam * const patPamP) {

    pm_message ("Input (height) file: %d wide x %d high x %d deep, maxval %lu",
                heightPamP->width, heightPamP->height, heightPamP->depth,
                heightPamP->maxval);

    if (heightPamP->depth > 1)
        pm_message("Ignoring all but first plane of input");

    if (usingPattern) {
        pm_message("Pattern file: tuple type '%s', "
                   "%d wide x %d high x %d deep, maxval %lu",
                   patPamP->tuple_type, patPamP->width, patPamP->height,
                   patPamP->depth, patPamP->maxval);
    }
}



static void
makeStereoRow(tuple *      const inrow,
              unsigned int const width,
              int *        const samebuf,
              unsigned int const mu,
              double       const eyesep,
              unsigned int const dpi,
              sample       const maxval) {

#define Z(X) (1.0-inrow[X][0]/(double)maxval)

    int * const same = samebuf;
        /* Pointer to a pixel to the right forced to have the same color */

    int col;
    
    for (col = 0; col < width; ++col)
        same[col] = col;
    
    for (col = 0; col < width; ++col) {
        int const s = separation(Z(col), mu, eyesep, dpi, maxval);
        int left, right;

        left  = col - s/2;  /* initial value */
        right = left + s;   /* initial value */
        
        if (0 <= left && right < width) {
            int visible;
            int t;
            double zt;
            
            t = 1;  /* initial value */
            
            do {
                zt = Z(col) + 2.0*(2.0 - mu*Z(col))*t/(mu*EYESEP);
                visible = Z(col-t) < zt && Z(col+t) < zt;
                t++;
            } while (visible && zt < 1);
            if (visible) {
                int l;
                
                l = same[left];
                while (l != left && l != right)
                    if (l < right) {
                        left = l;
                        l = same[left];
                    }
                    else {
                        same[left] = right;
                        left = right;
                        l = same[left];
                        right = l;
                    }
                same[left] = right;
            }
        }
    }
}



static void
setupOutput(struct pam *       const outPamP,
            const struct pam * const heightPamP,
            sample             const requestedMaxval,
            bool               const outputIsMask,
            enum outputType    const outputType,
            unsigned int       const guidesize,
            FILE *             const outfile) {

    outPamP->size        = sizeof(*outPamP);
    outPamP->len         = PAM_STRUCT_SIZE(tuple_type);
    outPamP->file        = outfile;
    outPamP->plainformat = 0;
    outPamP->height      = heightPamP->height + 3*abs(guidesize);  
    /* Allow room for guides. */
    outPamP->width       = heightPamP->width;
    outPamP->format      = PAM_FORMAT;
  
    if (outputIsMask) {
        outPamP->depth = 1;
        outPamP->maxval = 1;
        sprintf (outPamP->tuple_type, "BLACKANDWHITE");
    } else {
        switch (outputType) {
        case OUTPUT_BW:
            outPamP->depth = 1;
            outPamP->maxval = 1;
            sprintf (outPamP->tuple_type, "BLACKANDWHITE");
            break;

        case OUTPUT_GRAYSCALE:
            outPamP->depth = 1;
            outPamP->maxval = 
                requestedMaxval ? requestedMaxval : heightPamP->maxval;
            sprintf (outPamP->tuple_type, "GRAYSCALE");
            break;

        case OUTPUT_COLOR:
            outPamP->depth = 3;
            outPamP->maxval = 
                requestedMaxval ? requestedMaxval : heightPamP->maxval;
            sprintf (outPamP->tuple_type, "RGB");
            break;
        }
    }
    outPamP->bytes_per_sample = pnm_bytespersample(outPamP->maxval);

    pnm_writepaminit(outPamP);
}



static void
makeMaskRow(const struct pam * const outPamP,
            const tuple *      const outrow,
            const int *        const same) {

    int col;
    for (col = outPamP->width-1; col >= 0; --col) {
        unsigned int plane;
        
        for (plane = 0; plane < outPamP->depth; ++plane)
            outrow[col][plane] = 
                same[col] == col ? 0 : outPamP->maxval;
    }
}



static void
makeImageRow(const struct pam * const outPamP,
             const tuple *      const outrow,
             int                const row,
             COORD2COLOR              gettuple,
             const int *        const same) {

    int col;
    for (col = outPamP->width-1; col >= 0; --col) {
        tuple const newtuple = 
            same[col] == col ? gettuple(col, row) : outrow[same[col]];
        
        unsigned int plane;
        
        for (plane = 0; plane < outPamP->depth; ++plane)
            outrow[col][plane] = newtuple[plane];
    }
}



static void
invertHeightRow(const struct pam * const pamP,
                tuple *            const tupleRow) {

    unsigned int col;

    for (col = 0; col < pamP->width; ++col)
        tupleRow[col][0] = pamP->maxval - tupleRow[col][0];
}



/* Do the bulk of the work.  See the paper cited above for comments
 * and explanation.  All I did was modify the code to use the PBM
 * libraries. */
static void 
produceStereogram(struct cmdlineInfo const cmdline,
                  struct pam *       const heightPamP,
                  const struct pam * const patPamP,
                  tuple **           const patTuples,
                  FILE *             const outfile) {

    tuple *inrow;    /* One row of input data */
    tuple *outrow;   /* One row of output data */
    struct pam outPam;    /* PAM information for the output file */
    int *same;       /* Working storage for makeStereoRow() */
    COORD2COLOR colorFunc; 
        /* Function to map an (x,y) cooordinate to a color */
    unsigned int row;

    inrow = pnm_allocpamrow (heightPamP);
    MALLOCARRAY_NOFAIL(same, heightPamP->width);

    /* Describe the input and pattern files. */
    if (verbose)
        reportInputAndPattern(heightPamP, !!patTuples, patPamP);

    setupOutput(&outPam, heightPamP, cmdline.maxval,
                cmdline.makemask, cmdline.outputType,
                cmdline.guidesize, stdout);

    setupColorFunc(cmdline, &outPam, patTuples, &colorFunc);

    outrow = pnm_allocpamrow(&outPam);

    if (verbose)
        pm_message("Unique depth levels possible: %d", 
                   separation(0, cmdline.depth, cmdline.eyesep, 
                              cmdline.dpi, outPam.maxval) - 
                   separation(1, cmdline.depth, cmdline.eyesep, 
                              cmdline.dpi, outPam.maxval) + 1);

    /* Draw guide boxes at the top, if desired. */
    if (cmdline.guidesize < 0)
        drawguides(-cmdline.guidesize, &outPam, 
                   cmdline.depth, cmdline.eyesep, cmdline.dpi);

    /* See the paper cited above for code comments.  All I (Scott) did was
     * transcribe the code and make minimal changes for Netpbm.  And some
     * style changes by Bryan to match Netpbm style.
     */
    for (row = 0; row < heightPamP->height; ++row) {
        pnm_readpamrow(heightPamP, inrow);

        if (cmdline.crosseyed)
            invertHeightRow(heightPamP, inrow);

        makeStereoRow(inrow, heightPamP->width, same, 
                      cmdline.depth, cmdline.eyesep, cmdline.dpi,
                      outPam.maxval);

        if (cmdline.makemask)
            makeMaskRow(&outPam, outrow, same);
        else
            makeImageRow(&outPam, outrow, row, colorFunc, same);

        pnm_writepamrow(&outPam, outrow);
    }

    /* Draw guide boxes at the bottom, if desired. */
    if (cmdline.guidesize > 0)
        drawguides(cmdline.guidesize, &outPam,
                   cmdline.depth, cmdline.eyesep, cmdline.dpi);

    free(same);
    pnm_freepamrow(outrow);
    pnm_freepamrow(inrow);
}



static void
reportParameters(struct cmdlineInfo const cmdline) {

    unsigned int const pixelEyesep = round2int(cmdline.eyesep * cmdline.dpi);

    pm_message("Eye separation: %f inch * %d DPI = %u pixels",
               cmdline.eyesep, cmdline.dpi, pixelEyesep);

    pm_message("Pattern magnification: %d", cmdline.magnifypat);
    pm_message("\"Optimal\" pattern width: %u / (%d * 2) = %u pixels",
               pixelEyesep, cmdline.magnifypat, 
               pixelEyesep/(cmdline.magnifypat*2));
    if (cmdline.patfile)
        pm_message("Pattern shift: (%d, %d)", 
                   cmdline.xshift, cmdline.yshift);
}



int main (int argc, char *argv[]) {

    struct cmdlineInfo cmdline;
    FILE * infile;
    struct pam heightPam;
    struct pam patPam;      /* Description of pattern image */
    tuple **patTuples;      /* Pattern image raster */

    /* Parse the command line. */
    pnm_init(&argc, argv);

    parseCommandLine(argc, argv, &cmdline);

    verbose = cmdline.verbose;
    infile = pm_openr(cmdline.inputFilespec);
    if (cmdline.patfile) {
        FILE * patfile;
        patfile = pm_openr(cmdline.patfile);
        patTuples = pnm_readpam(patfile, &patPam, PAM_STRUCT_SIZE(tuple_type));
        pm_close(patfile);
    } else
        patTuples = NULL;

    if (verbose)
        reportParameters(cmdline);

    pnm_readpaminit(infile, &heightPam, PAM_STRUCT_SIZE(tuple_type));

    produceStereogram(cmdline, &heightPam, &patPam, patTuples, stdout);

    if (patTuples)
        pnm_freepamarray(patTuples, &patPam);

    pm_close(infile);

    return 0;
}

