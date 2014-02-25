#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "ppm.h"
#include "frame.h"

struct InputFileEntry;

struct inputSource {
/*----------------------------------------------------------------------------
   This describes the source of data for the program.
   Typically, the source data is a bunch of raw frames in PPM format.
   But sometimes, it is a bunch of already encoded frames or GOPs.
-----------------------------------------------------------------------------*/
    bool                     stdinUsed;
    struct InputFileEntry ** inputFileEntries;
    unsigned int             numInputFileEntries;
    unsigned int             ifArraySize;
    int numInputFiles;
        /* This is the number of input files available.  If we're
           reading from a pipe, it's infinity.  (At the moment, we
           approximate "reading from a pipe" as "reading from Standard
           Input).  
        */
};


void
GetNthInputFileName(struct inputSource * const inputSourceP,
                    unsigned int         const n,
                    const char **        const fileName);

void
ReadNthFrame(struct inputSource * const inputSourceP,
             unsigned int         const frameNumber,
             bool                 const remoteIO,
             bool                 const childProcess,
             bool                 const separateConversion,
             const char *         const slaveConversion,
             const char *         const inputConversion,
             MpegFrame *          const frameP);


void
JM2JPEG(struct inputSource * const inputSourceP);

void
AddInputFiles(struct inputSource * const inputSourceP,
              const char *         const input);

void
SetStdinInput(struct inputSource * const inputSourceP);

void
CreateInputSource(struct inputSource ** const inputSourcePP);

void
DestroyInputSource(struct inputSource * const inputSourceP);

#endif
