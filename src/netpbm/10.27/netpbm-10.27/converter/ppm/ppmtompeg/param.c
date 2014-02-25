/*===========================================================================*
 * param.c              
 *                      
 *  Procedures to read in parameter file  
 *                                    
 *===========================================================================*/

/* COPYRIGHT INFORMATION IS AT THE END OF THIS FILE */

#define _XOPEN_SOURCE 1
    /* This makes sure popen() is in stdio.h.  In GNU libc 2.1.3, 
     _POSIX_C_SOURCE = 2 is sufficient, but on AIX 4.3, the higher level
     _XOPEN_SOURCE is required.  2000.09.09 
    */


/*==============*
 * HEADER FILES *
 *==============*/

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "nstring.h"
#include "mallocvar.h"
#include "all.h"
#include "mtypes.h"
#include "mpeg.h"
#include "motion_search.h"
#include "prototypes.h"
#include "parallel.h"
#include "readframe.h"
#include "fsize.h"
#include "frames.h"
#include "jpeg.h"
#include "input.h"
#include "frametype.h"
#include "param.h"

#include "rate.h"
#include "opts.h"

/*===========*
 * CONSTANTS *
 *===========*/

#define INPUT_ENTRY_BLOCK_SIZE   128

#define FIRST_OPTION           0
#define OPTION_GOP             0
#define OPTION_PATTERN         1
#define OPTION_PIXEL           2
#define OPTION_PQSCALE         3
#define OPTION_OUTPUT          4
#define OPTION_RANGE           5
#define OPTION_PSEARCH_ALG     6
#define OPTION_IQSCALE         7
#define OPTION_INPUT_DIR       8
#define OPTION_INPUT_CONVERT   9
#define OPTION_INPUT          10
#define OPTION_BQSCALE        11
#define OPTION_BASE_FORMAT    12
#define OPTION_SPF            13
#define OPTION_BSEARCH_ALG    14
#define OPTION_REF_FRAME      15
#define LAST_OPTION           15

/* put any non-required options after LAST_OPTION */
#define OPTION_RESIZE	      16
#define OPTION_IO_CONVERT     17
#define OPTION_SLAVE_CONVERT  18
#define OPTION_IQTABLE	      19
#define OPTION_NIQTABLE	      20
#define OPTION_FRAME_RATE     21
#define OPTION_ASPECT_RATIO   22
#define OPTION_YUV_SIZE	      23
#define OPTION_SPECIFICS      24
#define OPTION_DEFS_SPECIFICS 25
#define OPTION_BUFFER_SIZE    26
#define OPTION_BIT_RATE       27
#define OPTION_USER_DATA      28
#define OPTION_YUV_FORMAT     29
#define OPTION_GAMMA          30
#define OPTION_PARALLEL       31
#define OPTION_FRAME_INPUT    32
#define OPTION_GOP_INPUT      33

#define NUM_OPTIONS           33

/*=======================*
 * STRUCTURE DEFINITIONS *
 *=======================*/

typedef struct InputFileEntryStruct {
    char    left[256];
    char    right[256];
    boolean glob;	    /* if FALSE, left is complete name */
    int	    startID;
    int	    endID;
    int	    skip;
    int	    numPadding;	    /* -1 if there is none */
    int	    numFiles;
    boolean repeat;
} InputFileEntry;


/*==================*
 * GLOBAL VARIABLES *
 *==================*/

extern char currentPath[MAXPATHLEN];
char	outputFileName[256];
int	outputWidth, outputHeight;
char inputConversion[1024];
char ioConversion[1024];
char slaveConversion[1024];
char yuvConversion[256];
char specificsFile[256],specificsDefines[1024]="";
boolean GammaCorrection=FALSE;
float   GammaValue;
char userDataFileName[256]={0};
boolean specificsOn = FALSE;
char currentGOPPath[MAXPATHLEN];
char currentFramePath[MAXPATHLEN];
boolean keepTempFiles;

static const char * const optionText[LAST_OPTION+1] = { 
    "GOP_SIZE", "PATTERN", "PIXEL", "PQSCALE",
    "OUTPUT", "RANGE", "PSEARCH_ALG", "IQSCALE", "INPUT_DIR",
    "INPUT_CONVERT", "INPUT", "BQSCALE", "BASE_FILE_FORMAT",
    "SLICES_PER_FRAME", "BSEARCH_ALG", "REFERENCE_FRAME"};
static boolean optionSeen[NUM_OPTIONS+1];
    /* optionSeen[x] means we have seen option x in the parameter file we've
       been reading.
    */

int numMachines;
char	machineName[MAX_MACHINES][256];
char	userName[MAX_MACHINES][256];
char	executable[MAX_MACHINES][1024];
char	remoteParamFile[MAX_MACHINES][1024];
boolean	remote[MAX_MACHINES];
int mult_seq_headers = 0;  /* 0 for none, N for header/N GOPs */


/*===========================================================================*
 *
 * SkipSpacesTabs
 *
 *	skip all spaces and tabs
 *
 * RETURNS:	point to next character not a space or tab
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static const char *
SkipSpacesTabs(const char * const start) {

    const char * p;

    for (p = start; *p == ' ' || *p == '\t'; ++p);

    return p;
}



/*===========================================================================*
 *
 * GetAspectRatio
 *
 * take a character string with the pixel aspect ratio
 * and returns the correct aspect ratio code for use in the Sequence header
 *
 * RETURNS: aspect ratio code as per MPEG-I spec
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static int
GetAspectRatio(const char * const p)
{
  float   ratio;
  int	  ttRatio;
  int     retval;

  sscanf(p, "%f", &ratio);
  ttRatio = (int)(0.5+ratio*10000.0);

  if ( ttRatio == 10000 )	      retval = 1;
  else if ( ttRatio ==  6735 )    retval = 2;
  else if ( ttRatio ==  7031 )    retval = 3;
  else if ( ttRatio ==  7615 )    retval = 4;
  else if ( ttRatio ==  8055 )    retval = 5;
  else if ( ttRatio ==  8437 )    retval = 6;
  else if ( ttRatio ==  8935 )    retval = 7;
  else if ( ttRatio ==  9157 )    retval = 8;
  else if ( ttRatio ==  9815 )    retval = 9;
  else if ( ttRatio == 10255 )    retval = 10;
  else if ( ttRatio == 10695 )    retval = 11;
  else if ( ttRatio == 10950 )    retval = 12;
  else if ( ttRatio == 11575 )    retval = 13;
  else if ( ttRatio == 12015 )    retval = 14;
  else {
    fprintf(stderr,"INVALID ASPECT RATIO: %s frames/sec\n", p);
    exit(1);
  }
  return retval;
}



/*===========================================================================*
 *
 * ReadMachineNames
 *
 *	read a list of machine names for parallel execution
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    machine info updated
 *
 *===========================================================================*/
static void
ReadMachineNames(FILE * const fpointer)
{
  char    input[256];
  const char *charPtr;

  while ( (fgets(input, 256, fpointer) != NULL) &&
	 (strncmp(input, "END_PARALLEL", 12) != 0) ) {
    if ( input[0] == '#' || input[0] == '\n') {
      continue;
    }

    if ( strncmp(input, "REMOTE", 6) == 0 ) {
      charPtr = SkipSpacesTabs(&input[6]);
      remote[numMachines] = TRUE;

      sscanf(charPtr, "%s %s %s %s", machineName[numMachines],
	     userName[numMachines], executable[numMachines],
	     remoteParamFile[numMachines]);
    } else {
      remote[numMachines] = FALSE;

      sscanf(input, "%s %s %s", machineName[numMachines],
	     userName[numMachines], executable[numMachines]);
    }

    numMachines++;
  }
}


/*===========================================================================*
 *
 * GetFrameRate
 *
 * take a character string with the input frame rate 
 * and return the correct frame rate code for use in the Sequence header
 *
 * RETURNS: frame rate code as per MPEG-I spec
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static int
GetFrameRate(const char * const p)
{
  float   rate;
  int	  thouRate;
  int     retval;

  sscanf(p, "%f", &rate);
  thouRate = (int)(0.5+1000.0*rate);

  if ( thouRate == 23976 )	       retval = 1;
  else if ( thouRate == 24000 )    retval = 2;
  else if ( thouRate == 25000 )    retval = 3;
  else if ( thouRate == 29970 )    retval = 4;
  else if ( thouRate == 30000 )    retval = 5;
  else if ( thouRate == 50000 )    retval = 6;
  else if ( thouRate == 59940 )    retval = 7;
  else if ( thouRate == 60000 )    retval = 8;
  else {
    fprintf(stderr,"INVALID FRAME RATE: %s frames/sec\n", p);
    exit(1);
  }
  return retval;
}



static void
mergeInputSource(struct inputSource * const baseSourceP,
                 struct inputSource * const addedSourceP) {

    unsigned int i;

    baseSourceP->ifArraySize += addedSourceP->numInputFileEntries;
    REALLOCARRAY_NOFAIL(baseSourceP->inputFileEntries, 
                        baseSourceP->ifArraySize);
    for (i = 0; i < addedSourceP->numInputFileEntries; ++i)
        baseSourceP->inputFileEntries[baseSourceP->numInputFileEntries++] =
            addedSourceP->inputFileEntries[i];

    free(addedSourceP);
    /* Note the space allocated for the *addedSourceP input file
       entries themselves is still allocated, and used by 
       *baseSourceP.
    */
}



/* Forward declaration for recursively called function */
static void
ReadInputFileNames(FILE *               const fpointer, 
                   const char *         const endInput,
                   struct inputSource * const inputSourceP);

static void
expandBackTickLine(const char *         const input,
                   struct inputSource * const inputSourceP) {

    FILE *fp;
    char cmd[300];
    const char * start;
    const char * end;
    char cdcmd[110];

    start = &input[1];
    end = &input[strlen(input)-1];

    while (*end != '`') {
        end--;
    }

    end--;

    if (optionSeen[OPTION_INPUT_DIR])
        sprintf(cdcmd,"cd %s;",currentPath);
    else
        strcpy(cdcmd,"");

    {
        char tmp[300];
        strncpy(tmp,start,end-start+1);
        sprintf(cmd,"(%s %s)", cdcmd, tmp);
    }

    fp = popen(cmd, "r");
    if (fp == NULL) {
        pm_error("Unable to resolve backtick entry in input file list.  "
                 "Could not open piped command: '%s'", cmd);
    } else {
        struct inputSource subInputSource;
        ReadInputFileNames(fp, "HOPE-THIS_ISNT_A_FILENAME.xyz5555",
                           &subInputSource);

        mergeInputSource(inputSourceP, &subInputSource);
    }
}



static void
ReadInputFileNames(FILE *               const fpointer, 
                   const char *         const endInput,
                   struct inputSource * const inputSourceP) {
/*----------------------------------------------------------------------------
   Read a list of input file names from the parameter file.  Add 
   the information to *inputSourceP.

   If inputSourceP == NULL, read off the list, but ignore it.
-----------------------------------------------------------------------------*/
    char input[256];
    bool endStatementRead;

    /* read param file up through endInput statement.  Each line until
       endInput is a file specification.
    */
    endStatementRead = FALSE;

    while (!endStatementRead) {
        char * rc;
        rc = fgets(input, sizeof(input), fpointer);

        if (feof(fpointer))
            pm_error("End of file before section end statement '%s'",
                     endInput);
        else if (rc == NULL)
            pm_error("Error reading file names from parameter file.");
        else {
            if (strncmp(input, endInput, strlen(endInput)) == 0)
                endStatementRead = TRUE;
            else if ((input[0] == '#') || (input[0] == '\n')) { 
                /* It's a comment or blank line.  Ignore it */
            } else if (input[0] == '`' ) {
                expandBackTickLine(input, inputSourceP);
            } else {
                /* get rid of trailing whitespace including newline */
                while (ISSPACE(input[strlen(input)-1]))
                    input[strlen(input)-1] = '\0';
                if (inputSourceP)
                    AddInputFiles(inputSourceP, input);
            }
        }
    }
}



static void
initOptionSeen(void) {

    unsigned int index;
    
    for (index = FIRST_OPTION; index < NUM_OPTIONS; ++index)
        optionSeen[index] = FALSE;
}



static void
verifyNoMissingEncodeFramesOption(void) {
    if (!optionSeen[OPTION_GOP])
        pm_error("GOP_SIZE option missing");
    if (!optionSeen[OPTION_PATTERN])
        pm_error("PATTERN option missing");
    if (!optionSeen[OPTION_PIXEL])
        pm_error("PIXEL option missing");
    if (!optionSeen[OPTION_PQSCALE])
        pm_error("PQSCALE option missing");
    if (!optionSeen[OPTION_OUTPUT])
        pm_error("OUTPUT option missing");
    if (!optionSeen[OPTION_RANGE])
        pm_error("RANGE option missing");
    if (!optionSeen[OPTION_PSEARCH_ALG])
        pm_error("PSEARCH_ALG option missing");
    if (!optionSeen[OPTION_IQSCALE])
        pm_error("IQSCALE option missing");
    if (!optionSeen[OPTION_INPUT_DIR])
        pm_error("INPUT_DIR option missing");
    if (!optionSeen[OPTION_INPUT_CONVERT])
        pm_error("INPUT_CONVERT option missing");
    if (!optionSeen[OPTION_BQSCALE])
        pm_error("BQSCALE option missing");
    if (!optionSeen[OPTION_BASE_FORMAT])
        pm_error("BASE_FILE_FORMAT option missing");
    if (!optionSeen[OPTION_SPF])
        pm_error("SLICES_PER_FRAME option missing");
    if (!optionSeen[OPTION_BSEARCH_ALG])
        pm_error("BSEARCH_ALG option missing");
    if (!optionSeen[OPTION_REF_FRAME])
        pm_error("REFERENCE_FRAME option missing");
}



static void
verifyNoMissingCombineGopsOption(void) {

    if (!optionSeen[OPTION_GOP_INPUT])
        pm_error("GOP_INPUT option missing");
    if (!optionSeen[OPTION_YUV_SIZE])
        pm_error("YUV_SIZE option missing");
    if (!optionSeen[OPTION_OUTPUT])
        pm_error("OUTPUT option missing");
}



static void
verifyNoMissingCombineFramesOption(void) {

    if (!optionSeen[OPTION_GOP])
        pm_error("GOP_SIZE option missing");
    if (!optionSeen[OPTION_YUV_SIZE])
        pm_error("YUV_SIZE option missing");
    if (!optionSeen[OPTION_OUTPUT])
        pm_error("OUTPUT option missing");
}



static void
verifyNoMissingOption(int  const function) {
/*----------------------------------------------------------------------------
  Verify that the parameter file contains every option it is supposed to. 
  Abort program if not.
-----------------------------------------------------------------------------*/
    switch(function) {
    case ENCODE_FRAMES:
        verifyNoMissingEncodeFramesOption();
        break;
    case COMBINE_GOPS:
        verifyNoMissingCombineGopsOption();
        break;
    case COMBINE_FRAMES:
        verifyNoMissingCombineFramesOption();
        break;
    default:
        pm_error("Internal error - impossible value for 'function': %d",
                 function);
    }
}



static void
processIqtable(FILE * const fpointer) {

    unsigned int row;
    unsigned int col;
    char input[256];

    for (row = 0; row < 8; ++row) {
        const char * charPtr;

        fgets(input, 256, fpointer);
        charPtr = input;
        if (8 != sscanf(charPtr,"%d %d %d %d %d %d %d %d",
                        &qtable[row*8+0],  &qtable[row*8+1],
                        &qtable[row*8+2],  &qtable[row*8+3],
                        &qtable[row*8+4],  &qtable[row*8+5],
                        &qtable[row*8+6],  &qtable[row*8+7])) {
            pm_error("Line %d of IQTABLE doesn't have 8 elements!", 
                     row);
        }
        for (col = 0; col < 8; ++col) {
            if ((qtable[row*8+col]<1) || (qtable[row*8+col]>255)) {
                pm_message(
                    "Warning:  IQTable Element %1d,%1d (%d) "
                    "corrected to 1-255.",
                    row+1, col+1, qtable[row*8+col]);
                qtable[row*8+col] = (qtable[row*8+col]<1)?1:255;
            }
        }
    }
            
    if (qtable[0] != 8) {
        pm_message("Warning:  IQTable Element 1,1 reset to 8, "
                   "since it must be 8.");
        qtable[0] = 8;
    }
    customQtable = qtable;
}



static void
setInputSource(int                   const function,
               struct inputSource ** const inputSourcePP,
               struct inputSource *  const inputSourceP,
               struct inputSource *  const gopInputSourceP,
               struct inputSource *  const frameInputSourceP) {

    switch (function) {
    case ENCODE_FRAMES:
        *inputSourcePP = inputSourceP;
        DestroyInputSource(gopInputSourceP);
        DestroyInputSource(frameInputSourceP);
        break;
    case COMBINE_GOPS:
        *inputSourcePP = gopInputSourceP;
        DestroyInputSource(inputSourceP);
        DestroyInputSource(frameInputSourceP);
        break;
    case COMBINE_FRAMES:
        *inputSourcePP = frameInputSourceP;
        DestroyInputSource(inputSourceP);
        DestroyInputSource(gopInputSourceP);
        break;
    default:
      pm_error("INTERNAL ERROR: invalid 'function' %d", function);
    }
}



/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/


/*===========================================================================*
 *
 * ReadParamFile
 *
 *	read the parameter file
 *	function is ENCODE_FRAMES, COMBINE_GOPS, or COMBINE_FRAMES, and
 *	    will slightly modify the procedure's behavior as to what it
 *	    is looking for in the parameter file
 *
 * SIDE EFFECTS:    sets parameters accordingly, as well as machine info for
 *		    parallel execution and input file names
 *
 *===========================================================================*/
void
ReadParamFile(const char *    const fileName, 
              int             const function,
              struct params * const paramP) {

  FILE *fpointer;
  char    input[256];
  const char *charPtr;
  boolean yuvUsed = FALSE;
  int row, col;
  struct inputSource * inputSourceP;
      /* Contents of INPUT section */
  struct inputSource * frameInputSourceP;
      /* Contents of FRAME_INPUT section */
  struct inputSource * gopInputSourceP;
      /* Contents of GOP_INPUT section */

  fpointer = fopen(fileName, "r");
  if (fpointer == NULL)
      pm_error("Error:  Cannot open parameter file:  %s", fileName);

  CreateInputSource(&inputSourceP);
  CreateInputSource(&frameInputSourceP);
  CreateInputSource(&gopInputSourceP);

  /* should set defaults */
  numMachines = 0;
  sprintf(currentPath, ".");
  sprintf(currentGOPPath, ".");
  sprintf(currentFramePath, ".");
#ifndef HPUX
  SetRemoteShell("rsh");
#else
  SetRemoteShell("remsh");
#endif
  keepTempFiles = FALSE;
  paramP->warnOverflow = FALSE;
  paramP->warnUnderflow = FALSE;

  initOptionSeen();

  while ( fgets(input, 256, fpointer) != NULL ) {
    /* skip comments */
    if ( input[0] == '#' ) {            
      continue;
    }

    {
      int len = strlen(input);
      if (input[len-1] == '\n') {
	len--;
        input[len] = '\0'; /* get rid of newline */
	/* Junk whitespace */
	while ((len >= 0) && ((input[len-1] == ' ') || (input[len-1] == '\t'))) {
	  input[--len] = '\0';
	}
      }
    }

    if (strlen(SkipSpacesTabs(input)) == 0) continue;

    switch(input[0]) {
    case 'A':
      if ( strncmp(input, "ASPECT_RATIO", 12) == 0 ) {
        charPtr = SkipSpacesTabs(&input[12]);
        aspectRatio = GetAspectRatio(charPtr);
        optionSeen[OPTION_ASPECT_RATIO] = TRUE;
      }
      break;

    case 'B':
      if ( strncmp(input, "BQSCALE", 7) == 0 ) {
        charPtr = SkipSpacesTabs(&input[7]);
        SetBQScale(atoi(charPtr));
        optionSeen[OPTION_BQSCALE] = TRUE;
      } else if ( strncmp(input, "BASE_FILE_FORMAT", 16) == 0 ) {
        charPtr = SkipSpacesTabs(&input[16]);
        SetFileFormat(charPtr);
        if ( (strncmp(charPtr,"YUV",3) == 0) || 
            (strcmp(charPtr,"Y") == 0) ) {
          yuvUsed = TRUE;
        }
        optionSeen[OPTION_BASE_FORMAT] = TRUE;
      } else if ( strncmp(input, "BSEARCH_ALG", 11) == 0 ) {
        charPtr = SkipSpacesTabs(&input[11]);
        SetBSearchAlg(charPtr);
        optionSeen[OPTION_BSEARCH_ALG] = TRUE;
      } else if ( strncmp(input, "BIT_RATE", 8) == 0 ) {
        charPtr = SkipSpacesTabs(&input[8]);
        setBitRate(charPtr);
        optionSeen[OPTION_BIT_RATE] = TRUE;
      } else if ( strncmp(input, "BUFFER_SIZE", 11) == 0 ) {
        charPtr = SkipSpacesTabs(&input[11]);
        setBufferSize(charPtr);
        optionSeen[OPTION_BUFFER_SIZE] = TRUE;                  
      }
      break;

    case 'C':
      if ( strncmp(input, "CDL_FILE", 8) == 0 ) {
        charPtr = SkipSpacesTabs(&input[8]);
        strcpy(specificsFile, charPtr);
        specificsOn=TRUE;
        optionSeen[OPTION_SPECIFICS] = TRUE;
      } else if ( strncmp(input, "CDL_DEFINES", 11) == 0 ) {
        charPtr = SkipSpacesTabs(&input[11]);
        strcpy(specificsDefines, charPtr);
        optionSeen[OPTION_DEFS_SPECIFICS] = TRUE;
      }
      break;

    case 'F':
      if (strncmp(input, "FRAME_INPUT_DIR", 15) == 0) {
          charPtr = SkipSpacesTabs(&input[15]);
          if ( strncmp(charPtr, "stdin", 5) == 0 ||
               strncmp(charPtr, "STDIN", 5) == 0 ) {
              SetStdinInput(frameInputSourceP);
          }
          strcpy(currentFramePath, charPtr);
      } else if ( strncmp(input, "FRAME_INPUT", 11) == 0 ) {
          ReadInputFileNames(fpointer, "FRAME_END_INPUT", 
                             frameInputSourceP->stdinUsed ? 
                             NULL : frameInputSourceP);
          optionSeen[OPTION_FRAME_INPUT] = TRUE;
      } else if ( strncmp(input, "FORCE_I_ALIGN", 13) == 0 ) {
        forceIalign = TRUE;
      } else if ( strncmp(input, "FORCE_ENCODE_LAST_FRAME", 23) == 0 ) {
        /* NO-OP.  We used to drop trailing B frames by default and you
           needed this option to change the last frame to I so you could
           encode all the frames.  Now we just do that all the time.  
           Why wouldn't we?
        */
      } else if ( strncmp(input, "FRAME_RATE", 10) == 0 ) {
        charPtr = SkipSpacesTabs(&input[10]);
        frameRate = GetFrameRate(charPtr);
        frameRateRounded = (int) VidRateNum[frameRate];
        if ( (frameRate % 3) == 1) {
          frameRateInteger = FALSE;
        }
        optionSeen[OPTION_FRAME_RATE] = TRUE;
      }
      break;

    case 'G':
        if ( strncmp(input, "GOP_SIZE", 8) == 0 ) {
            charPtr = SkipSpacesTabs(&input[8]);
            SetGOPSize(atoi(charPtr));
            optionSeen[OPTION_GOP] = TRUE;
        } else if ( strncmp(input, "GOP_INPUT_DIR", 13) == 0 ) {
            charPtr = SkipSpacesTabs(&input[13]);
            if ( strncmp(charPtr, "stdin", 5) == 0 ||
                 strncmp(charPtr, "STDIN", 5) == 0 ) {
                SetStdinInput(inputSourceP);
            }
            strcpy(currentGOPPath, charPtr);
        } else if ( strncmp(input, "GOP_INPUT", 9) == 0 ) {
            ReadInputFileNames(fpointer, "GOP_END_INPUT", 
                               gopInputSourceP->stdinUsed ? 
                               NULL : gopInputSourceP);
            optionSeen[OPTION_GOP_INPUT] = TRUE;
        } else if ( strncmp(input, "GAMMA", 5) == 0) {
            charPtr = SkipSpacesTabs(&input[5]);
            GammaCorrection = TRUE;
            sscanf(charPtr,"%f",&GammaValue);
            optionSeen[OPTION_GAMMA] = TRUE;
        }
        break;

    case 'I':
        if ( strncmp(input, "IQSCALE", 7) == 0 ) {
            charPtr = SkipSpacesTabs(&input[7]);
            SetIQScale(atoi(charPtr));
            optionSeen[OPTION_IQSCALE] = TRUE;
        } else if ( strncmp(input, "INPUT_DIR", 9) == 0 ) {
            charPtr = SkipSpacesTabs(&input[9]);
            if ( strncmp(charPtr, "stdin", 5) == 0 ||
                 strncmp(charPtr, "STDIN", 5) == 0 )
                SetStdinInput(paramP->inputSourceP);
            strcpy(currentPath, charPtr);
            optionSeen[OPTION_INPUT_DIR] = TRUE;
        } else if ( strncmp(input, "INPUT_CONVERT", 13) == 0 ) {
            charPtr = SkipSpacesTabs(&input[13]);
            strcpy(inputConversion, charPtr);
            optionSeen[OPTION_INPUT_CONVERT] = TRUE;
        } else if ( strcmp(input, "INPUT") == 0 ) { /* yes, strcmp */
            ReadInputFileNames(fpointer, "END_INPUT", 
                               inputSourceP->stdinUsed ?
                               NULL : inputSourceP);
            optionSeen[OPTION_INPUT] = TRUE;
        } else if ( strncmp(input, "IO_SERVER_CONVERT", 17) == 0 ) {
            charPtr = SkipSpacesTabs(&input[17]);
            strcpy(ioConversion, charPtr);
            optionSeen[OPTION_IO_CONVERT] = TRUE;
        } else if ( strncmp(input, "IQTABLE", 7) == 0 ) {
            processIqtable(fpointer);

            optionSeen[OPTION_IQTABLE] = TRUE;
        }
        break;

    case 'K':
        if (strncmp(input, "KEEP_TEMP_FILES", 15) == 0)
            keepTempFiles = TRUE;
        break;

    case 'N':
      if ( strncmp(input, "NIQTABLE", 8) == 0 ) {
        for ( row = 0; row < 8; row ++ ) {
          fgets(input, 256, fpointer);
          charPtr = input;
          if (8!=sscanf(charPtr,"%d %d %d %d %d %d %d %d",
                        &niqtable[row*8+0],  &niqtable[row*8+1],
                        &niqtable[row*8+2],  &niqtable[row*8+3],
                        &niqtable[row*8+4],  &niqtable[row*8+5],
                        &niqtable[row*8+6],  &niqtable[row*8+7])) {
            fprintf(stderr, "Line %d of NIQTABLE doesn't have 8 elements!\n", 
                    row);
            exit(1);
          }
          for ( col = 0; col < 8; col++ ) {
            if ((niqtable[row*8+col]<1) || (niqtable[row*8+col]>255)) {
              fprintf(stderr,
                      "Warning:  NIQTable Element %1d,%1d (%d) "
                      "corrected to 1-255.\n",
                      row+1, col+1, niqtable[row*8+col]);
              niqtable[row*8+col]=(niqtable[row*8+col]<1)?1:255;
            }}
        }

        customNIQtable = niqtable;
        optionSeen[OPTION_NIQTABLE] = TRUE;
      }
      break;

    case 'O':
      if ( strncmp(input, "OUTPUT", 6) == 0 ) {
        charPtr = SkipSpacesTabs(&input[6]);
        if ( whichGOP == -1 ) {
          strcpy(outputFileName, charPtr);
        } else {
          sprintf(outputFileName, "%s.gop.%d",
                  charPtr, whichGOP);
        }

        optionSeen[OPTION_OUTPUT] = TRUE;
      }
      break;

    case 'P':
      if ( strncmp(input, "PATTERN", 7) == 0 ) {
        charPtr = SkipSpacesTabs(&input[7]);
        SetFramePattern(charPtr);
        optionSeen[OPTION_PATTERN] = TRUE;
      } else if ( strncmp(input, "PIXEL", 5) == 0 ) {
        charPtr = SkipSpacesTabs(&input[5]);
        SetPixelSearch(charPtr);
        optionSeen[OPTION_PIXEL] = TRUE;
      } else if ( strncmp(input, "PQSCALE", 7) == 0 ) {
        charPtr = SkipSpacesTabs(&input[7]);
        SetPQScale(atoi(charPtr));
        optionSeen[OPTION_PQSCALE] = TRUE;
      } else if ( strncmp(input, "PSEARCH_ALG", 11) == 0 ) {
        charPtr = SkipSpacesTabs(&input[11]);
        SetPSearchAlg(charPtr);
        optionSeen[OPTION_PSEARCH_ALG] = TRUE;
      } else if ( strncmp(input, "PARALLEL_TEST_FRAMES", 20) == 0 ) {
        SetParallelPerfect(FALSE);
        charPtr = SkipSpacesTabs(&input[20]);
        parallelTestFrames = atoi(charPtr);
      } else if ( strncmp(input, "PARALLEL_TIME_CHUNKS", 20) == 0 ) {
        SetParallelPerfect(FALSE);
        charPtr = SkipSpacesTabs(&input[20]);
        parallelTimeChunks = atoi(charPtr);
      } else if ( strncmp(input, "PARALLEL_CHUNK_TAPER", 20) == 0 ) {
        SetParallelPerfect(FALSE);
        parallelTimeChunks = -1;
      } else if ( strncmp(input, "PARALLEL_PERFECT", 16) == 0 ) {
        SetParallelPerfect(TRUE);
      } else if ( strncmp(input, "PARALLEL", 8) == 0 ) {
        ReadMachineNames(fpointer);
        optionSeen[OPTION_PARALLEL] = TRUE;
      }
      break;

    case 'R':
      if ( strncmp(input, "RANGE", 5) == 0 ) {
        int num_ranges=0,a,b;
        charPtr = SkipSpacesTabs(&input[5]);
        optionSeen[OPTION_RANGE] = TRUE;
        num_ranges=sscanf(charPtr,"%d %d",&a,&b);
        if (num_ranges==2) {
          SetSearchRange(a,b);
        } else if (sscanf(charPtr,"%d [%d]",&a,&b)==2) {
          SetSearchRange(a,b);
        } else SetSearchRange(a,a);
      } else if ( strncmp(input, "REFERENCE_FRAME", 15) == 0 ) {
        charPtr = SkipSpacesTabs(&input[15]);
        SetReferenceFrameType(charPtr);
        optionSeen[OPTION_REF_FRAME] = TRUE;
      } else if ( strncmp(input, "RSH", 3) == 0 ) {
        charPtr = SkipSpacesTabs(&input[3]);
        SetRemoteShell(charPtr);
      } else if ( strncmp(input, "RESIZE", 6) == 0 ) {
        charPtr = SkipSpacesTabs(&input[6]);                    
        sscanf(charPtr, "%dx%d", &outputWidth, &outputHeight);
        outputWidth &= ~(DCTSIZE * 2 - 1);
        outputHeight &= ~(DCTSIZE * 2 - 1);
        optionSeen[OPTION_RESIZE] = TRUE;
      }
      break;

    case 'S':
      if ( strncmp(input, "SLICES_PER_FRAME", 16) == 0 ) {
        charPtr = SkipSpacesTabs(&input[16]);
        SetSlicesPerFrame(atoi(charPtr));
        optionSeen[OPTION_SPF] = TRUE;
      } else if ( strncmp(input, "SLAVE_CONVERT", 13) == 0 ) {
        charPtr = SkipSpacesTabs(&input[13]);
        strcpy(slaveConversion, charPtr);
        optionSeen[OPTION_SLAVE_CONVERT] = TRUE;
      } else if ( strncmp(input, "SPECIFICS_FILE", 14) == 0 ) {
        charPtr = SkipSpacesTabs(&input[14]);
        strcpy(specificsFile, charPtr);
        specificsOn=TRUE;
        optionSeen[OPTION_SPECIFICS] = TRUE;
      } else if ( strncmp(input, "SPECIFICS_DEFINES", 16) == 0 ) {
        charPtr = SkipSpacesTabs(&input[17]);
        strcpy(specificsDefines, charPtr);
        optionSeen[OPTION_DEFS_SPECIFICS] = TRUE;
      } else if (strncmp(input, "SEQUENCE_SIZE", 13) == 0) {
        charPtr = SkipSpacesTabs(&input[13]);
        mult_seq_headers = atoi(charPtr);
      } else if (strncmp(input, "SIZE", 4) == 0 ) {
        charPtr = SkipSpacesTabs(&input[4]);
        sscanf(charPtr, "%dx%d", &yuvWidth, &yuvHeight);
        realWidth = yuvWidth;
        realHeight = yuvHeight;
        Fsize_Validate(&yuvWidth, &yuvHeight);
        optionSeen[OPTION_YUV_SIZE] = TRUE;
      }
      break;

    case 'T':
      if ( strncmp(input, "TUNE", 4) == 0) {
        tuneingOn = TRUE;
        charPtr = SkipSpacesTabs(&input[4]);
	ParseTuneParam(charPtr);
      }
      break;

    case 'U':
        if (strncmp(input, "USER_DATA", 9) == 0) {
            charPtr = SkipSpacesTabs(&input[9]);
            strcpy(userDataFileName, charPtr);
            optionSeen[OPTION_USER_DATA] = TRUE;
        }
        break;
        
    case 'W':
        if (strncmp(input, "WARN_UNDERFLOW", 14) == 0)
            paramP->warnUnderflow = TRUE;
        if (strncmp(input, "WARN_OVERFLOW", 13) == 0)
            paramP->warnOverflow = TRUE;
        break;
        
    case 'Y':
        if (strncmp(input, "YUV_SIZE", 8) == 0) {
            charPtr = SkipSpacesTabs(&input[8]);
            sscanf(charPtr, "%dx%d", &yuvWidth, &yuvHeight);
            realWidth = yuvWidth;
            realHeight = yuvHeight;
            Fsize_Validate(&yuvWidth, &yuvHeight);
            optionSeen[OPTION_YUV_SIZE] = TRUE;
        }
        else if (strncmp(input, "Y_SIZE", 6) == 0) {
            charPtr = SkipSpacesTabs(&input[6]);
            sscanf(charPtr, "%dx%d", &yuvWidth, &yuvHeight);
            realWidth = yuvWidth;
            realHeight = yuvHeight;
            Fsize_Validate(&yuvWidth, &yuvHeight);
            optionSeen[OPTION_YUV_SIZE] = TRUE;
        }
        else if (strncmp(input, "YUV_FORMAT", 10) == 0) {
            charPtr = SkipSpacesTabs(&input[10]);
            strcpy(yuvConversion, charPtr);
            optionSeen[OPTION_YUV_FORMAT] = TRUE;
        }
        break;
    }
  }

  fclose(fpointer);

  setInputSource(function, &paramP->inputSourceP,
                 inputSourceP, gopInputSourceP, frameInputSourceP);

  verifyNoMissingOption(function);

  /* error checking */

  if (!paramP->inputSourceP->stdinUsed &&
      paramP->inputSourceP->numInputFiles == 0)
      pm_error("You have not specified any input.  See the "
               "INPUT_DIR, INPUT, GOP_INPUT_DIR, GOP_INPUT, "
               "FRAME_INPUT_DIR, and FRAME_INPUT options");

  if (yuvUsed) {
      if (!optionSeen[OPTION_YUV_SIZE])
          pm_error("YUV format used but YUV_SIZE not given");
      
      if (!optionSeen[OPTION_YUV_FORMAT]) {
          strcpy (yuvConversion, "EYUV");
          pm_message("WARNING:  YUV format not specified; "
                     "defaulting to Berkeley YUV (EYUV)");
      }
  }

  if (paramP->inputSourceP->stdinUsed && optionSeen[OPTION_PARALLEL] )
      pm_error("stdin reading for parallel execution not enabled yet.");

  if (optionSeen[OPTION_PARALLEL] && !optionSeen[OPTION_YUV_SIZE])
      pm_error("Specify SIZE WxH for parallel encoding");

  if (optionSeen[OPTION_IO_CONVERT] != optionSeen[OPTION_SLAVE_CONVERT])
      pm_error("Must have either both IO_SERVER_CONVERT and SLAVE_CONVERT "
               "or neither");
      
  if (optionSeen[OPTION_DEFS_SPECIFICS] && !optionSeen[OPTION_SPECIFICS])
      pm_error("Does not make sense to define Specifics file options, "
               "but no specifics file!");

  SetIOConvert(optionSeen[OPTION_IO_CONVERT]);

  SetResize(optionSeen[OPTION_RESIZE]);

  if (function == ENCODE_FRAMES) {
      SetFCode();

      if (psearchAlg == PSEARCH_TWOLEVEL)
          SetPixelSearch("HALF");
  }
}


/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */
