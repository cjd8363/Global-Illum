/******************************************************************************
                               pnmcolormap.c
*******************************************************************************

  Create a colormap file (a PPM image containing one pixel of each of a set
  of colors).  Base the set of colors on an input image.

  For PGM input, do the equivalent for grayscale and produce a PGM graymap.

  By Bryan Henderson, San Jose, CA 2001.12.17

  Derived from ppmquant, originally by Jef Poskanzer.

  Copyright (C) 1989, 1991 by Jef Poskanzer.
  Copyright (C) 2001 by Bryan Henderson.

  Permission to use, copy, modify, and distribute this software and its
  documentation for any purpose and without fee is hereby granted, provided
  that the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.  This software is provided "as is" without express or
  implied warranty.

******************************************************************************/

#include <math.h>

#include "pam.h"
#include "pammap.h"
#include "shhopt.h"
#include "mallocvar.h"

#define MAXCOLORS 32767u
#define MAXDEPTH 3

enum methodForLargest {LARGE_NORM, LARGE_LUM};

enum methodForRep {REP_CENTER_BOX, REP_AVERAGE_COLORS, REP_AVERAGE_PIXELS};

typedef struct box* boxVector;
struct box {
    int ind;
    int colors;
    int sum;
};

struct cmdlineInfo {
    /* All the information the user supplied in the command line,
       in a form easy for the program to use.
    */
    const char *inputFilespec;  /* Filespec of input file */
    unsigned int allcolors;  /* boolean: select all colors from the input */
    unsigned int newcolors;    
        /* Number of colors argument; meaningless if allcolors true */
    enum methodForLargest methodForLargest; 
        /* -spreadintensity/-spreadluminosity options */
    enum methodForRep methodForRep;
        /* -center/-meancolor/-meanpixel options */
    unsigned int sort;
    unsigned int square;
    unsigned int verbose;
};



static void
parseCommandLine (int argc, char ** argv,
                  struct cmdlineInfo *cmdlineP) {
/*----------------------------------------------------------------------------
   parse program command line described in Unix standard form by argc
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

    unsigned int spreadbrightness, spreadluminosity;
    unsigned int center, meancolor, meanpixel;

    MALLOCARRAY_NOFAIL(option_def, 100);

    option_def_index = 0;   /* incremented by OPTENT3 */
    OPTENT3(0,   "spreadbrightness", OPT_FLAG,   
            NULL,                       &spreadbrightness, 0);
    OPTENT3(0,   "spreadluminosity", OPT_FLAG,   
            NULL,                       &spreadluminosity, 0);
    OPTENT3(0,   "center",           OPT_FLAG,   
            NULL,                       &center,           0);
    OPTENT3(0,   "meancolor",        OPT_FLAG,   
            NULL,                       &meancolor,        0);
    OPTENT3(0,   "meanpixel",        OPT_FLAG,   
            NULL,                       &meanpixel,        0);
    OPTENT3(0, "sort",     OPT_FLAG,   NULL,                  
            &cmdlineP->sort,       0 );
    OPTENT3(0, "square",   OPT_FLAG,   NULL,                  
            &cmdlineP->square,       0 );
    OPTENT3(0, "verbose",   OPT_FLAG,   NULL,                  
            &cmdlineP->verbose,       0 );

    opt.opt_table = option_def;
    opt.short_allowed = FALSE;  /* We have no short (old-fashioned) options */
    opt.allowNegNum = FALSE;  /* We have no parms that are negative numbers */

    optParseOptions3( &argc, argv, opt, sizeof(opt), 0 );
        /* Uses and sets argc, argv, and some of *cmdline_p and others. */


    if (spreadbrightness && spreadluminosity) 
        pm_error("You cannot specify both -spreadbrightness and "
                 "spreadluminosity.");
    if (spreadluminosity)
        cmdlineP->methodForLargest = LARGE_LUM;
    else
        cmdlineP->methodForLargest = LARGE_NORM;

    if (center + meancolor + meanpixel > 1)
        pm_error("You can specify only one of -center, -meancolor, and "
                 "-meanpixel.");
    if (meancolor)
        cmdlineP->methodForRep = REP_AVERAGE_COLORS;
    else if (meanpixel) 
        cmdlineP->methodForRep = REP_AVERAGE_PIXELS;
    else
        cmdlineP->methodForRep = REP_CENTER_BOX;

    if (argc-1 > 2)
        pm_error("Program takes at most two arguments: number of colors "
                 "and input file specification.  "
                 "You specified %d arguments.", argc-1);
    else {
        if (argc-1 < 2)
            cmdlineP->inputFilespec = "-";
        else
            cmdlineP->inputFilespec = argv[2];

        if (argc-1 < 1)
            pm_error("You must specify the number of colors in the "
                     "output as an argument.");
        else {
            if (strcmp(argv[1], "all") == 0)
                cmdlineP->allcolors = TRUE;
            else {
                char * tail;
                long int const newcolors = strtol(argv[1], &tail, 10);
                if (*tail != '\0')
                    pm_error("The number of colors argument '%s' is not "
                             "a number or 'all'", argv[1]);
                else if (newcolors < 1)
                    pm_error("The number of colors must be positive");
                else if (newcolors == 1)
                    pm_error("The number of colors must be greater than 1.");
                else {
                    cmdlineP->newcolors = newcolors;
                    cmdlineP->allcolors = FALSE;
                }
            }
        }
    }
}
static int __inline__
compareplane(struct tupleint ** const comparandP, 
             struct tupleint ** const comparatorP,
             unsigned int       const plane) {
    return (*comparandP)->tuple[plane] - (*comparatorP)->tuple[plane];
}

static int
compareplane0(const void * const comparandP, const void * const comparatorP) {
    return compareplane((struct tupleint **)comparandP,
                        (struct tupleint **)comparatorP,
                        0);
}

static int
compareplane1(const void * const comparandP, const void * const comparatorP) {
    return compareplane((struct tupleint **)comparandP,
                        (struct tupleint **)comparatorP,
                        1);
}

static int
compareplane2(const void * const comparandP, const void * const comparatorP) {
    return compareplane((struct tupleint **)comparandP,
                        (struct tupleint **)comparatorP,
                        2);
}



typedef int qsort_comparison_fn(const void *, const void *);

/* This ought to be an array of functions, but the compiler complains at
   that for some reason.
*/
static struct {qsort_comparison_fn * fn;} samplecompare[MAXDEPTH];

static void
initSampleCompare(void) {
    samplecompare[0].fn = compareplane0;
    samplecompare[1].fn = compareplane1;
    samplecompare[2].fn = compareplane2;
}



static int
sumcompare(const void * const b1, const void * const b2) {
    return(((boxVector)b2)->sum - ((boxVector)b1)->sum);
}




/*
** Here is the fun part, the median-cut colormap generator.  This is based
** on Paul Heckbert's paper "Color Image Quantization for Frame Buffer
** Display", SIGGRAPH '82 Proceedings, page 297.
*/

static tupletable
newColorMap(unsigned int const newcolors,
            unsigned int const depth) {

    tupletable colormap;
    unsigned int i;
    struct pam pam;

    pam.depth = depth;

    colormap = pnm_alloctupletable(&pam, newcolors);

    for (i = 0; i < newcolors; ++i) {
        unsigned int plane;
        for (plane = 0; plane < depth; ++plane) 
            colormap[i]->tuple[plane] = 0;
    }
    return colormap;
}



static boxVector
newBoxVector(int const colors, int const sum, int const newcolors) {

    boxVector bv;
    MALLOCARRAY(bv, newcolors);
    if (bv == NULL)
        pm_error("out of memory allocating box vector table");

    /* Set up the initial box. */
    bv[0].ind = 0;
    bv[0].colors = colors;
    bv[0].sum = sum;

    return bv;
}



static void
findBoxBoundaries(tupletable   const colorfreqtable,
                  unsigned int const depth,
                  unsigned int const boxStart,
                  unsigned int const boxSize,
                  sample             minval[],
                  sample             maxval[]) {
/*----------------------------------------------------------------------------
  Go through the box finding the minimum and maximum of each
  component - the boundaries of the box.
-----------------------------------------------------------------------------*/
    unsigned int plane;
    unsigned int i;

    for (plane = 0; plane < depth; ++plane) 
        minval[plane] = maxval[plane] = colorfreqtable[boxStart]->tuple[plane];
    
    for (i = 1; i < boxSize; ++i) {
        unsigned int plane;
        for (plane = 0; plane < depth; ++plane) {
            sample const v = colorfreqtable[boxStart + i]->tuple[plane];
            if (v < minval[plane]) minval[plane] = v;
            if (v > maxval[plane]) maxval[plane] = v;
        } 
    }
}



static unsigned int
largestByNorm(sample minval[], sample maxval[], unsigned int const depth) {
    
    unsigned int largestDimension;
    unsigned int plane;

    sample largestSpreadSoFar = 0;  
    largestDimension = 0;
    for (plane = 0; plane < depth; ++plane) {
        sample const spread = maxval[plane]-minval[plane];
        if (spread > largestSpreadSoFar) {
            largestDimension = plane;
            largestSpreadSoFar = spread;
        }
    }
    return largestDimension;
}



static unsigned int 
largestByLuminosity(sample minval[], sample maxval[], 
                    unsigned int const depth) {
/*----------------------------------------------------------------------------
   This subroutine presumes that the tuple type is either 
   BLACKANDWHITE, GRAYSCALE, or RGB (which implies pamP->depth is 1 or 3).
   To save time, we don't actually check it.
-----------------------------------------------------------------------------*/
    unsigned int retval;

    if (depth == 1)
        retval = 0;
    else {
        /* An RGB tuple */
        unsigned int largestDimension;
        unsigned int plane;
        double largestSpreadSoFar;

        largestSpreadSoFar = 0.0;

        for (plane = 0; plane < 3; ++plane) {
            double const spread = 
                pnm_lumin_factor[plane] * (maxval[plane]-minval[plane]);
            if (spread > largestSpreadSoFar) {
                largestDimension = plane;
                largestSpreadSoFar = spread;
            }
        }
        retval = largestDimension;
    }
    return retval;
}



static void
centerBox(int          const boxStart,
          int          const boxSize,
          tupletable   const colorfreqtable, 
          unsigned int const depth,
          tuple        const newTuple) {

    unsigned int plane;

    for (plane = 0; plane < depth; ++plane) {
        int minval, maxval;
        unsigned int i;

        minval = maxval = colorfreqtable[boxStart]->tuple[plane];
        
        for (i = 1; i < boxSize; ++i) {
            int const v = colorfreqtable[boxStart + i]->tuple[plane];
            minval = MIN( minval, v);
            maxval = MAX( maxval, v);
        }
        newTuple[plane] = (minval + maxval) / 2;
    }
}



static void
averageColors(int          const boxStart,
              int          const boxSize,
              tupletable   const colorfreqtable, 
              unsigned int const depth,
              tuple        const newTuple) {

    unsigned int plane;

    for (plane = 0; plane < depth; ++plane) {
        sample sum;
        int i;

        sum = 0;

        for (i = 0; i < boxSize; ++i) 
            sum += colorfreqtable[boxStart+i]->tuple[plane];

        newTuple[plane] = sum / boxSize;
    }
}



static void
averagePixels(int          const boxStart,
              int          const boxSize,
              tupletable   const colorfreqtable, 
              unsigned int const depth,
              tuple        const newTuple) {

    unsigned int n;
        /* Number of tuples represented by the box */
    unsigned int plane;
    unsigned int i;

    /* Count the tuples in question */
    n = 0;  /* initial value */
    for (i = 0; i < boxSize; ++i)
        n += colorfreqtable[boxStart + i]->value;


    for (plane = 0; plane < depth; ++plane) {
        sample sum;
        int i;

        sum = 0;

        for (i = 0; i < boxSize; ++i) 
            sum += colorfreqtable[boxStart+i]->tuple[plane]
                * colorfreqtable[boxStart+i]->value;

        newTuple[plane] = sum / n;
    }
}



static tupletable
colormapFromBv(unsigned int      const newcolors,
               boxVector         const bv, 
               unsigned int      const boxes,
               tupletable        const colorfreqtable, 
               unsigned int      const depth,
               enum methodForRep const methodForRep) {
    /*
    ** Ok, we've got enough boxes.  Now choose a representative color for
    ** each box.  There are a number of possible ways to make this choice.
    ** One would be to choose the center of the box; this ignores any structure
    ** within the boxes.  Another method would be to average all the colors in
    ** the box - this is the method specified in Heckbert's paper.  A third
    ** method is to average all the pixels in the box. 
    */
    tupletable colormap;
    unsigned int bi;

    colormap = newColorMap(newcolors, depth);

    for (bi = 0; bi < boxes; ++bi) {
        switch (methodForRep) {
        case REP_CENTER_BOX: 
            centerBox(bv[bi].ind, bv[bi].colors, colorfreqtable, depth, 
                      colormap[bi]->tuple);
            break;
        case REP_AVERAGE_COLORS:
            averageColors(bv[bi].ind, bv[bi].colors, colorfreqtable, depth,
                          colormap[bi]->tuple);
            break;
        case REP_AVERAGE_PIXELS:
            averagePixels(bv[bi].ind, bv[bi].colors, colorfreqtable, depth,
                          colormap[bi]->tuple);
            break;
        default:
            pm_error("Internal error: invalid value of methodForRep: %d",
                     methodForRep);
        }
    }
    return colormap;
}



static unsigned int
freqTotal(tupletable const freqtable, unsigned int const entries) {
    
    unsigned int i;
    unsigned int sum;

    sum = 0;

    for (i = 0; i < entries; ++i)
        sum += freqtable[i]->value;

    return sum;
}


static void
splitBox(boxVector             const bv, 
         unsigned int *        const boxesP, 
         unsigned int          const bi,
         tupletable            const colorfreqtable, 
         unsigned int          const depth,
         enum methodForLargest const methodForLargest) {
/*----------------------------------------------------------------------------
   Split Box 'bi' in the box vector bv (so that bv contains one more box
   than it did as input).  Split it so that each new box represents about
   half of the pixels in the distribution given by 'colorfreqtable' for 
   the colors in the original box, but with distinct colors in each of the
   two new boxes.

   Assume the box contains at least two colors.
-----------------------------------------------------------------------------*/
    unsigned int const boxStart = bv[bi].ind;
    unsigned int const boxSize = bv[bi].colors;
    unsigned int const sm = bv[bi].sum;

    sample minval[MAXDEPTH];
    sample maxval[MAXDEPTH];
    unsigned int largestDimension;
    /* number of the plane with the largest spread */
    unsigned int medianIndex;
    int lowersum;
    /* Number of pixels whose value is "less than" the median */

    findBoxBoundaries(colorfreqtable, depth, boxStart, boxSize, 
                      minval, maxval);

    /*
            ** Find the largest dimension, and sort by that component.  I have
            ** included two methods for determining the "largest" dimension;
            ** first by simply comparing the range in RGB space, and second
            ** by transforming into luminosities before the comparison. 
            */
    switch (methodForLargest) {
    case LARGE_NORM: 
        largestDimension = largestByNorm(minval, maxval, depth);
        break;
    case LARGE_LUM: 
        largestDimension = largestByLuminosity(minval, maxval, depth);
        break;
    }
                                                    
    /* TODO: I think this sort should go after creating a box,
       not before splitting.  Because you need the sort to use
       the REP_CENTER_BOX method of choosing a color to
       represent the final boxes 
    */

    qsort((char*) &colorfreqtable[boxStart], boxSize, 
          sizeof(colorfreqtable[boxStart]), 
          samplecompare[largestDimension].fn);
            
    {
        /* Now find the median based on the counts, so that
           about half the pixels (not colors, pixels) are in
           each subdivision. 
                */
        unsigned int i;

        lowersum = colorfreqtable[boxStart]->value; /* initial value */
        for (i = 1; i < boxSize - 1 && lowersum < sm/2; ++i) {
            lowersum += colorfreqtable[boxStart + i]->value;
        }
        medianIndex = i;
    }
    /* Split the box, and sort to bring the biggest boxes to the top.  */

    bv[bi].colors = medianIndex;
    bv[bi].sum = lowersum;
    bv[*boxesP].ind = boxStart + medianIndex;
    bv[*boxesP].colors = boxSize - medianIndex;
    bv[*boxesP].sum = sm - lowersum;
    ++(*boxesP);
    qsort((char*) bv, *boxesP, sizeof(struct box), sumcompare);
}



static tupletable
mediancut(tupletable            const colorfreqtable, 
          int                   const colors, 
          unsigned int          const depth,
          int                   const newcolors,
          enum methodForLargest const methodForLargest,
          enum methodForRep     const methodForRep) {
/*----------------------------------------------------------------------------
   Compute a set of only 'newcolors' colors that best represent an image
   whose pixels are summarized by the histogram 'colorfreqtable'.  There are
   'colors' colors in that table.  Each tuple in it has depth 'depth'.
   colorfreqtable[i] tells the number of pixels in the subject image have a
   particular color.

   As a side effect, sort 'colorfreqtable'.
-----------------------------------------------------------------------------*/
    tupletable colormap;
    boxVector bv;
    unsigned int bi;
    unsigned int boxes;
    bool multicolorBoxesExist;
        /* There is at least one box that contains at least 2 colors; ergo,
           there is more splitting we can do.
        */

    bv = newBoxVector(colors, freqTotal(colorfreqtable, colors), newcolors);
    boxes = 1;
    multicolorBoxesExist = (colors > 1);

    /* Main loop: split boxes until we have enough. */
    while (boxes < newcolors && multicolorBoxesExist) {
        /* Find the first splittable box. */
        for (bi = 0; bi < boxes && bv[bi].colors < 2; ++bi);
        if (bi >= boxes)
            multicolorBoxesExist = FALSE;
        else 
            splitBox(bv, &boxes, bi, colorfreqtable, depth, methodForLargest);
    }
    colormap = colormapFromBv(newcolors, bv, boxes, colorfreqtable, depth,
                              methodForRep);

    free(bv);
    return colormap;
}




static void
computeHistogram(struct pam *   const pamP, 
                 tupletable *   const colorfreqtableP, 
                 unsigned int * const colorsP,
                 sample *       const freqtableMaxvalP) {
/*----------------------------------------------------------------------------
  Attempt to make a histogram of the colors in the image represented by
  *pamP, whose file is seekable and is now positioned to the raster.

  If at first we don't succeed, rescale the color resolution to half
  the present maxval and try again, repeatedly.  This will eventually
  terminate, with maxval at worst 16, which is half of 32, and 32^3 is
  just slightly larger than MAXCOLORS.
----------------------------------------------------------------------------*/
    tupletable colorfreqtable;
    sample maxval;
    pm_filepos rasterPos;

    pm_message("making histogram...");

    pm_tell2(pamP->file, &rasterPos, sizeof(rasterPos));

    colorfreqtable = NULL;  /* initial value */
    
    for (maxval = pamP->maxval;
         colorfreqtable == NULL && maxval > 0;
         maxval /= 2)
    {
        pm_seek2(pamP->file, &rasterPos, sizeof(rasterPos));
        colorfreqtable = 
            pnm_computetuplefreqtable2(pamP, NULL, MAXCOLORS, maxval, colorsP);
        if (colorfreqtable)
            *freqtableMaxvalP = maxval;
    }

    /* colorfreqtable is now non-null.  The "maxval > 0" in the for loop 
       limit above is just for robustness.
    */

    if (*freqtableMaxvalP != pamP->maxval) 
        pm_message("Too many colors (more than %u)!  "
                   "scaled color resolution from maxval %lu "
                   "to maxval %lu before choosing",
                   MAXCOLORS, pamP->maxval, *freqtableMaxvalP);

    pm_message("%u colors found", *colorsP);
    *colorfreqtableP = colorfreqtable;
}




static void
computeColorMapFromInput(struct pam *          const pamP, 
                         bool                  const allColors, 
                         int                   const reqColors, 
                         enum methodForLargest const methodForLargest,
                         enum methodForRep     const methodForRep,
                         tupletable *          const colormapP,
                         int *                 const colorsP,
                         sample *              const colormapMaxvalP) {
/*----------------------------------------------------------------------------
   Produce a colormap containing the best colors to represent the
   image represented by *pamP, whose file is seekable and now
   positioned to the raster.  Figure it out using the median cut
   technique.

   The colormap will have 'reqcolors' or fewer colors in it, unless
   'allcolors' is true, in which case it will have all the colors that
   are in the input.

   The colormap has the same maxval as the input, unless the input contains
   more than MAXCOLORS colors.  In that case, we compute the map from a
   maxval-reduced version of the input.  The colormap has that reduced 
   maxval.  Note that in this case, 'allColors' does not cause the colormap
   to have all the colors from the input; rather, it contains all the colors
   from the input once the input's color resolution is reduced to the new
   maxval.

   Put the colormap in newly allocated storage as a tupletable 
   and return its address as *colormapP.  Return the number of colors in
   it as *colorsP and its maxval as *colormapMaxvalP.
-----------------------------------------------------------------------------*/
    tupletable colorfreqtable;
    unsigned int colors;
    sample colormapMaxval;

    computeHistogram(pamP, &colorfreqtable, &colors, &colormapMaxval);
    
    if (allColors) {
        *colormapP = colorfreqtable;
        *colorsP = colors;
    } else {
        if (colors <= reqColors) {
            pm_message("Image already has few enough colors (<=%d).  "
                       "Keeping same colors.", reqColors);
            *colormapP = colorfreqtable;
            *colorsP = colors;
        } else {
            pm_message("choosing %d colors...", reqColors);
            *colormapP = mediancut(colorfreqtable, colors, pamP->depth, 
                                   reqColors,
                                   methodForLargest, methodForRep);
            *colorsP = reqColors;
            pnm_freetupletable(pamP, colorfreqtable);
        }
    }
    *colormapMaxvalP = colormapMaxval;
}



static void
sortColormap(tupletable   const colormap, 
             sample       const depth, 
             unsigned int const colormapSize) {
/*----------------------------------------------------------------------------
   Sort the colormap in place, in order of ascending Plane 0 value, 
   the Plane 1 value, etc.

   Use insertion sort.
-----------------------------------------------------------------------------*/
    int i;
    
    pm_message("Sorting %d colors...", colormapSize);

    for (i = 0; i < colormapSize; ++i) {
        int j;
        for (j = i+1; j < colormapSize; ++j) {
            unsigned int plane;
            bool iIsGreater, iIsLess;

            iIsGreater = FALSE; iIsLess = FALSE;
            for (plane = 0; 
                 plane < depth && !iIsGreater && !iIsLess; 
                 ++plane) {
                if (colormap[i]->tuple[plane] > colormap[j]->tuple[plane])
                    iIsGreater = TRUE;
                else if (colormap[i]->tuple[plane] < colormap[j]->tuple[plane])
                    iIsLess = TRUE;
            }            
            if (iIsGreater) {
                for (plane = 0; plane < depth; ++plane) {
                    sample const temp = colormap[i]->tuple[plane];
                    colormap[i]->tuple[plane] = colormap[j]->tuple[plane];
                    colormap[j]->tuple[plane] = temp;
                }
            }
        }    
    }
}



static void 
colormapToSquare(struct pam * const pamP,
                 tupletable   const colormap,
                 unsigned int const colormapSize, 
                 tuple ***    const outputRasterP) {
    {
        unsigned int const intsqrt = (int)sqrt((float)colormapSize);
        if (intsqrt * intsqrt == colormapSize) 
            pamP->width = intsqrt;
        else 
            pamP->width = intsqrt + 1;
            overflow_add(intsqrt, 1);
    }
    {
        unsigned int const intQuotient = colormapSize / pamP->width;
        if (pamP->width * intQuotient == colormapSize)
            pamP->height = intQuotient;
        else
            pamP->height = intQuotient + 1;
    }
    {
        tuple ** outputRaster;
        unsigned int row;
        unsigned int colormapIndex;
        
        outputRaster = pnm_allocpamarray(pamP);

        colormapIndex = 0;  /* initial value */
        
        for (row = 0; row < pamP->height; ++row) {
            unsigned int col;
            for (col = 0; col < pamP->width; ++col) {
                unsigned int plane;
                for (plane = 0; plane < pamP->depth; ++plane) {
                    outputRaster[row][col][plane] = 
                        colormap[colormapIndex]->tuple[plane];
                }
                if (colormapIndex < colormapSize-1)
                    colormapIndex++;
            }
        }
        *outputRasterP = outputRaster;
    } 
}



static void 
colormapToSingleRow(struct pam * const pamP,
                    tupletable   const colormap,
                    unsigned int const colormapSize, 
                    tuple ***    const outputRasterP) {

    tuple ** outputRaster;
    unsigned int col;
    
    pamP->width = colormapSize;
    pamP->height = 1;
    
    outputRaster = pnm_allocpamarray(pamP);
    
    for (col = 0; col < pamP->width; ++col) {
        int plane;
        for (plane = 0; plane < pamP->depth; ++plane)
            outputRaster[0][col][plane] = colormap[col]->tuple[plane];
    }
    *outputRasterP = outputRaster;
}



static void
colormapToImage(struct pam * const inpamP, 
                struct pam * const outpamP, 
                tupletable   const colormap,
                unsigned int const colormapSize, 
                sample       const colormapMaxval,
                bool         const sort,
                bool         const square,
                tuple ***    const outputRasterP) {
/*----------------------------------------------------------------------------
   Create a tuple array and pam structure for an image which includes
   one pixel of each of the colors in the colormap 'colormap'.

   May rearrange the contents of 'colormap'.
-----------------------------------------------------------------------------*/
    outpamP->size             = sizeof(*outpamP);
    outpamP->len              = PAM_STRUCT_SIZE(tuple_type);
    outpamP->format           = inpamP->format;
    outpamP->plainformat      = FALSE;
    outpamP->depth            = inpamP->depth;
    outpamP->maxval           = colormapMaxval;
    outpamP->bytes_per_sample = inpamP->bytes_per_sample;
    strcpy(outpamP->tuple_type, inpamP->tuple_type);

    if (sort)
        sortColormap(colormap, outpamP->depth, colormapSize);

    if (square) 
        colormapToSquare(outpamP, colormap, colormapSize, outputRasterP);
    else 
        colormapToSingleRow(outpamP, colormap, colormapSize, outputRasterP);
}



int
main(int argc, char * argv[] ) {

    struct cmdlineInfo cmdline;
    FILE* ifP;
    struct pam inpam, outpam;
    tuple** colormapRaster;
    int newcolors;
    tupletable colormap;
    sample colormapMaxval;

    pnm_init(&argc, argv);

    initSampleCompare();
 
    parseCommandLine(argc, argv, &cmdline);

    ifP = pm_openr_seekable(cmdline.inputFilespec);

    pnm_readpaminit(ifP, &inpam, PAM_STRUCT_SIZE(tuple_type));
    
    computeColorMapFromInput(&inpam,
                             cmdline.allcolors, cmdline.newcolors, 
                             cmdline.methodForLargest, 
                             cmdline.methodForRep,
                             &colormap, &newcolors, &colormapMaxval);

    pm_close(ifP);

    colormapToImage(&inpam, &outpam, colormap, newcolors, 
                    colormapMaxval, cmdline.sort,
                    cmdline.square, &colormapRaster);

    if (cmdline.verbose)
        pm_message("Generating %u x %u image", outpam.width, outpam.height);

    outpam.file = stdout;
    
    pnm_writepam(&outpam, colormapRaster);
    
    pnm_freepamarray(colormapRaster, &outpam);

    pm_close(stdout);

    return 0;
    
}
