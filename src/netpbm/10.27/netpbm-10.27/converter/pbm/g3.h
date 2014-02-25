/* G3 fax format declarations
*/

#ifndef _G3_H_
#define _G3_H_

/* G3 is nearly universal as the format for fax transmissions in the
   US.  Its full name is CCITT Group 3 (G3).  It is specified in
   Recommendations T.4 and T.30 and in EIA Standards EIA-465 and
   EIA-466.  It dates to 1993.

   G3 faxes are 204 dots per inch (dpi) horizontally and 98 dpi (196
   dpi optionally, in fine-detail mode) vertically.  Since G3 neither
   assumes error free transmission nor retransmits when errors occur,
   the encoding scheme used is differential only over small segments
   never exceeding 2 lines at standard resolution or 4 lines for
   fine-detail. (The incremental G3 encoding scheme is called
   two-dimensional and the number of lines so encoded is specified by
   a parameter called k.)

   G3 specifies much more than the format of the bit stream, which is
   the subject of this header file.  It also specifies layers
   underneath the bit stream.

   There is also the newer G4.  
*/

/* We have two sets of declarations here.  One,
   based on the 'tableentry' type is used by g3topbm.c, while the
   other, based on the 'g3TableEntry' type, is used by pbmtog3.c.
   The g3TableEntry stuff is newer; Before Netpbm 10.23, both programs
   used the tableentry stuff.  G3topbm probably should be updated and
   the 'tableentry' stuff retired.  */

typedef struct g3TableEntry {
    int code;
    int length;
} g3TableEntry;
	
static struct g3TableEntry mtable[] = {
    { 0x00, 0 },	{ 0x00, 0 },   /* dummy to simplify pointer math */
    { 0x1b, 5 },	{ 0x0f, 10 },   /* white 64 , black 64 */
    { 0x12, 5 },	{ 0xc8, 12 },
    { 0x17, 6 },	{ 0xc9, 12 },
    { 0x37, 7 },	{ 0x5b, 12 },
    { 0x36, 8 },	{ 0x33, 12 },
    { 0x37, 8 },	{ 0x34, 12 },
    { 0x64, 8 },	{ 0x35, 12 },
    { 0x65, 8 },	{ 0x6c, 13 },
    { 0x68, 8 },	{ 0x6d, 13 },
    { 0x67, 8 },	{ 0x4a, 13 },
    { 0xcc, 9 },	{ 0x4b, 13 },
    { 0xcd, 9 },	{ 0x4c, 13 },
    { 0xd2, 9 },	{ 0x4d, 13 },
    { 0xd3, 9 },	{ 0x72, 13 },
    { 0xd4, 9 },	{ 0x73, 13 },
    { 0xd5, 9 },	{ 0x74, 13 },
    { 0xd6, 9 },	{ 0x75, 13 },
    { 0xd7, 9 },	{ 0x76, 13 },
    { 0xd8, 9 },	{ 0x77, 13 },
    { 0xd9, 9 },	{ 0x52, 13 },
    { 0xda, 9 },	{ 0x53, 13 },
    { 0xdb, 9 },	{ 0x54, 13 },
    { 0x98, 9 },	{ 0x55, 13 },
    { 0x99, 9 },	{ 0x5a, 13 },
    { 0x9a, 9 },	{ 0x5b, 13 },
    { 0x18, 6 },	{ 0x64, 13 },
    { 0x9b, 9 },	{ 0x65, 13 },
    { 0x08, 11 },	{ 0x08, 11 },        /* extable len = 1792 */
    { 0x0c, 11 },	{ 0x0c, 11 },
    { 0x0d, 11 },	{ 0x0d, 11 },
    { 0x12, 12 },	{ 0x12, 12 },
    { 0x13, 12 },	{ 0x13, 12 },
    { 0x14, 12 },	{ 0x14, 12 },
    { 0x15, 12 },	{ 0x15, 12 },
    { 0x16, 12 },	{ 0x16, 12 },
    { 0x17, 12 },	{ 0x17, 12 },
    { 0x1c, 12 },	{ 0x1c, 12 },
    { 0x1d, 12 },	{ 0x1d, 12 },
    { 0x1e, 12 },	{ 0x1e, 12 },
    { 0x1f, 12 },	{ 0x1f, 12 },
};

static struct g3TableEntry ttable[] = {
    { 0x35, 8 },	{ 0x37, 10 },       /* white 0 , black 0 */
    { 0x07, 6 },	{ 0x02, 3 },
    { 0x07, 4 },	{ 0x03, 2 },
    { 0x08, 4 },	{ 0x02, 2 },
    { 0x0b, 4 },	{ 0x03, 3 },
    { 0x0c, 4 },	{ 0x03, 4 },
    { 0x0e, 4 },	{ 0x02, 4 },
    { 0x0f, 4 },	{ 0x03, 5 },
    { 0x13, 5 },	{ 0x05, 6 },
    { 0x14, 5 },	{ 0x04, 6 },
    { 0x07, 5 },	{ 0x04, 7 },
    { 0x08, 5 },	{ 0x05, 7 },
    { 0x08, 6 },	{ 0x07, 7 },
    { 0x03, 6 },	{ 0x04, 8 },
    { 0x34, 6 },	{ 0x07, 8 },
    { 0x35, 6 },	{ 0x18, 9 },
    { 0x2a, 6 },	{ 0x17, 10 },
    { 0x2b, 6 },	{ 0x18, 10 },
    { 0x27, 7 },	{ 0x08, 10 },
    { 0x0c, 7 },	{ 0x67, 11 },
    { 0x08, 7 },	{ 0x68, 11 },
    { 0x17, 7 },	{ 0x6c, 11 },
    { 0x03, 7 },	{ 0x37, 11 },
    { 0x04, 7 },	{ 0x28, 11 },
    { 0x28, 7 },	{ 0x17, 11 },
    { 0x2b, 7 },	{ 0x18, 11 },
    { 0x13, 7 },	{ 0xca, 12 },
    { 0x24, 7 },	{ 0xcb, 12 },
    { 0x18, 7 },	{ 0xcc, 12 },
    { 0x02, 8 },	{ 0xcd, 12 },
    { 0x03, 8 },	{ 0x68, 12 },
    { 0x1a, 8 },	{ 0x69, 12 },
    { 0x1b, 8 },	{ 0x6a, 12 },
    { 0x12, 8 },	{ 0x6b, 12 },
    { 0x13, 8 },	{ 0xd2, 12 },
    { 0x14, 8 },	{ 0xd3, 12 },
    { 0x15, 8 },	{ 0xd4, 12 },
    { 0x16, 8 },	{ 0xd5, 12 },
    { 0x17, 8 },	{ 0xd6, 12 },
    { 0x28, 8 },	{ 0xd7, 12 },
    { 0x29, 8 },	{ 0x6c, 12 },
    { 0x2a, 8 },	{ 0x6d, 12 },
    { 0x2b, 8 },	{ 0xda, 12 },
    { 0x2c, 8 },	{ 0xdb, 12 },
    { 0x2d, 8 },	{ 0x54, 12 },
    { 0x04, 8 },	{ 0x55, 12 },
    { 0x05, 8 },	{ 0x56, 12 },
    { 0x0a, 8 },	{ 0x57, 12 },
    { 0x0b, 8 },	{ 0x64, 12 },
    { 0x52, 8 },	{ 0x65, 12 },
    { 0x53, 8 },	{ 0x52, 12 },
    { 0x54, 8 },	{ 0x53, 12 },
    { 0x55, 8 },	{ 0x24, 12 },
    { 0x24, 8 },	{ 0x37, 12 },
    { 0x25, 8 },	{ 0x38, 12 },
    { 0x58, 8 },	{ 0x27, 12 },
    { 0x59, 8 },	{ 0x28, 12 },
    { 0x5a, 8 },	{ 0x58, 12 },
    { 0x5b, 8 },	{ 0x59, 12 },
    { 0x4a, 8 },	{ 0x2b, 12 },
    { 0x4b, 8 },	{ 0x2c, 12 },
    { 0x32, 8 },	{ 0x5a, 12 },
    { 0x33, 8 },	{ 0x66, 12 },
    { 0x34, 8 },	{ 0x67, 12 },       /* white 63 , black 63 */
};


typedef struct tableentry {
    int tabid;
    int code;
    int length;
    int count;
} tableentry;

#define TWTABLE		23
#define MWTABLE		24
#define TBTABLE		25
#define MBTABLE		26
#define EXTABLE		27
#define VRTABLE		28

static struct tableentry twtable[] = {
    { TWTABLE, 0x35, 8, 0 },
    { TWTABLE, 0x07, 6, 1 },
    { TWTABLE, 0x07, 4, 2 },
    { TWTABLE, 0x08, 4, 3 },
    { TWTABLE, 0x0b, 4, 4 },
    { TWTABLE, 0x0c, 4, 5 },
    { TWTABLE, 0x0e, 4, 6 },
    { TWTABLE, 0x0f, 4, 7 },
    { TWTABLE, 0x13, 5, 8 },
    { TWTABLE, 0x14, 5, 9 },
    { TWTABLE, 0x07, 5, 10 },
    { TWTABLE, 0x08, 5, 11 },
    { TWTABLE, 0x08, 6, 12 },
    { TWTABLE, 0x03, 6, 13 },
    { TWTABLE, 0x34, 6, 14 },
    { TWTABLE, 0x35, 6, 15 },
    { TWTABLE, 0x2a, 6, 16 },
    { TWTABLE, 0x2b, 6, 17 },
    { TWTABLE, 0x27, 7, 18 },
    { TWTABLE, 0x0c, 7, 19 },
    { TWTABLE, 0x08, 7, 20 },
    { TWTABLE, 0x17, 7, 21 },
    { TWTABLE, 0x03, 7, 22 },
    { TWTABLE, 0x04, 7, 23 },
    { TWTABLE, 0x28, 7, 24 },
    { TWTABLE, 0x2b, 7, 25 },
    { TWTABLE, 0x13, 7, 26 },
    { TWTABLE, 0x24, 7, 27 },
    { TWTABLE, 0x18, 7, 28 },
    { TWTABLE, 0x02, 8, 29 },
    { TWTABLE, 0x03, 8, 30 },
    { TWTABLE, 0x1a, 8, 31 },
    { TWTABLE, 0x1b, 8, 32 },
    { TWTABLE, 0x12, 8, 33 },
    { TWTABLE, 0x13, 8, 34 },
    { TWTABLE, 0x14, 8, 35 },
    { TWTABLE, 0x15, 8, 36 },
    { TWTABLE, 0x16, 8, 37 },
    { TWTABLE, 0x17, 8, 38 },
    { TWTABLE, 0x28, 8, 39 },
    { TWTABLE, 0x29, 8, 40 },
    { TWTABLE, 0x2a, 8, 41 },
    { TWTABLE, 0x2b, 8, 42 },
    { TWTABLE, 0x2c, 8, 43 },
    { TWTABLE, 0x2d, 8, 44 },
    { TWTABLE, 0x04, 8, 45 },
    { TWTABLE, 0x05, 8, 46 },
    { TWTABLE, 0x0a, 8, 47 },
    { TWTABLE, 0x0b, 8, 48 },
    { TWTABLE, 0x52, 8, 49 },
    { TWTABLE, 0x53, 8, 50 },
    { TWTABLE, 0x54, 8, 51 },
    { TWTABLE, 0x55, 8, 52 },
    { TWTABLE, 0x24, 8, 53 },
    { TWTABLE, 0x25, 8, 54 },
    { TWTABLE, 0x58, 8, 55 },
    { TWTABLE, 0x59, 8, 56 },
    { TWTABLE, 0x5a, 8, 57 },
    { TWTABLE, 0x5b, 8, 58 },
    { TWTABLE, 0x4a, 8, 59 },
    { TWTABLE, 0x4b, 8, 60 },
    { TWTABLE, 0x32, 8, 61 },
    { TWTABLE, 0x33, 8, 62 },
    { TWTABLE, 0x34, 8, 63 },
    };

static struct tableentry mwtable[] = {
    { MWTABLE, 0x1b, 5, 64 },
    { MWTABLE, 0x12, 5, 128 },
    { MWTABLE, 0x17, 6, 192 },
    { MWTABLE, 0x37, 7, 256 },
    { MWTABLE, 0x36, 8, 320 },
    { MWTABLE, 0x37, 8, 384 },
    { MWTABLE, 0x64, 8, 448 },
    { MWTABLE, 0x65, 8, 512 },
    { MWTABLE, 0x68, 8, 576 },
    { MWTABLE, 0x67, 8, 640 },
    { MWTABLE, 0xcc, 9, 704 },
    { MWTABLE, 0xcd, 9, 768 },
    { MWTABLE, 0xd2, 9, 832 },
    { MWTABLE, 0xd3, 9, 896 },
    { MWTABLE, 0xd4, 9, 960 },
    { MWTABLE, 0xd5, 9, 1024 },
    { MWTABLE, 0xd6, 9, 1088 },
    { MWTABLE, 0xd7, 9, 1152 },
    { MWTABLE, 0xd8, 9, 1216 },
    { MWTABLE, 0xd9, 9, 1280 },
    { MWTABLE, 0xda, 9, 1344 },
    { MWTABLE, 0xdb, 9, 1408 },
    { MWTABLE, 0x98, 9, 1472 },
    { MWTABLE, 0x99, 9, 1536 },
    { MWTABLE, 0x9a, 9, 1600 },
    { MWTABLE, 0x18, 6, 1664 },
    { MWTABLE, 0x9b, 9, 1728 },
    };

static struct tableentry tbtable[] = {
    { TBTABLE, 0x37, 10, 0 },
    { TBTABLE, 0x02, 3, 1 },
    { TBTABLE, 0x03, 2, 2 },
    { TBTABLE, 0x02, 2, 3 },
    { TBTABLE, 0x03, 3, 4 },
    { TBTABLE, 0x03, 4, 5 },
    { TBTABLE, 0x02, 4, 6 },
    { TBTABLE, 0x03, 5, 7 },
    { TBTABLE, 0x05, 6, 8 },
    { TBTABLE, 0x04, 6, 9 },
    { TBTABLE, 0x04, 7, 10 },
    { TBTABLE, 0x05, 7, 11 },
    { TBTABLE, 0x07, 7, 12 },
    { TBTABLE, 0x04, 8, 13 },
    { TBTABLE, 0x07, 8, 14 },
    { TBTABLE, 0x18, 9, 15 },
    { TBTABLE, 0x17, 10, 16 },
    { TBTABLE, 0x18, 10, 17 },
    { TBTABLE, 0x08, 10, 18 },
    { TBTABLE, 0x67, 11, 19 },
    { TBTABLE, 0x68, 11, 20 },
    { TBTABLE, 0x6c, 11, 21 },
    { TBTABLE, 0x37, 11, 22 },
    { TBTABLE, 0x28, 11, 23 },
    { TBTABLE, 0x17, 11, 24 },
    { TBTABLE, 0x18, 11, 25 },
    { TBTABLE, 0xca, 12, 26 },
    { TBTABLE, 0xcb, 12, 27 },
    { TBTABLE, 0xcc, 12, 28 },
    { TBTABLE, 0xcd, 12, 29 },
    { TBTABLE, 0x68, 12, 30 },
    { TBTABLE, 0x69, 12, 31 },
    { TBTABLE, 0x6a, 12, 32 },
    { TBTABLE, 0x6b, 12, 33 },
    { TBTABLE, 0xd2, 12, 34 },
    { TBTABLE, 0xd3, 12, 35 },
    { TBTABLE, 0xd4, 12, 36 },
    { TBTABLE, 0xd5, 12, 37 },
    { TBTABLE, 0xd6, 12, 38 },
    { TBTABLE, 0xd7, 12, 39 },
    { TBTABLE, 0x6c, 12, 40 },
    { TBTABLE, 0x6d, 12, 41 },
    { TBTABLE, 0xda, 12, 42 },
    { TBTABLE, 0xdb, 12, 43 },
    { TBTABLE, 0x54, 12, 44 },
    { TBTABLE, 0x55, 12, 45 },
    { TBTABLE, 0x56, 12, 46 },
    { TBTABLE, 0x57, 12, 47 },
    { TBTABLE, 0x64, 12, 48 },
    { TBTABLE, 0x65, 12, 49 },
    { TBTABLE, 0x52, 12, 50 },
    { TBTABLE, 0x53, 12, 51 },
    { TBTABLE, 0x24, 12, 52 },
    { TBTABLE, 0x37, 12, 53 },
    { TBTABLE, 0x38, 12, 54 },
    { TBTABLE, 0x27, 12, 55 },
    { TBTABLE, 0x28, 12, 56 },
    { TBTABLE, 0x58, 12, 57 },
    { TBTABLE, 0x59, 12, 58 },
    { TBTABLE, 0x2b, 12, 59 },
    { TBTABLE, 0x2c, 12, 60 },
    { TBTABLE, 0x5a, 12, 61 },
    { TBTABLE, 0x66, 12, 62 },
    { TBTABLE, 0x67, 12, 63 },
    };

static struct tableentry mbtable[] = {
    { MBTABLE, 0xf, 10, 64 },
    { MBTABLE, 0xc8, 12, 128 },
    { MBTABLE, 0xc9, 12, 192 },
    { MBTABLE, 0x5b, 12, 256 },
    { MBTABLE, 0x33, 12, 320 },
    { MBTABLE, 0x34, 12, 384 },
    { MBTABLE, 0x35, 12, 448 },
    { MBTABLE, 0x6c, 13, 512 },
    { MBTABLE, 0x6d, 13, 576 },
    { MBTABLE, 0x4a, 13, 640 },
    { MBTABLE, 0x4b, 13, 704 },
    { MBTABLE, 0x4c, 13, 768 },
    { MBTABLE, 0x4d, 13, 832 },
    { MBTABLE, 0x72, 13, 896 },
    { MBTABLE, 0x73, 13, 960 },
    { MBTABLE, 0x74, 13, 1024 },
    { MBTABLE, 0x75, 13, 1088 },
    { MBTABLE, 0x76, 13, 1152 },
    { MBTABLE, 0x77, 13, 1216 },
    { MBTABLE, 0x52, 13, 1280 },
    { MBTABLE, 0x53, 13, 1344 },
    { MBTABLE, 0x54, 13, 1408 },
    { MBTABLE, 0x55, 13, 1472 },
    { MBTABLE, 0x5a, 13, 1536 },
    { MBTABLE, 0x5b, 13, 1600 },
    { MBTABLE, 0x64, 13, 1664 },
    { MBTABLE, 0x65, 13, 1728 },
    };

static struct tableentry extable[] = {
    { EXTABLE, 0x08, 11, 1792 },
    { EXTABLE, 0x0c, 11, 1856 },
    { EXTABLE, 0x0d, 11, 1920 },
    { EXTABLE, 0x12, 12, 1984 },
    { EXTABLE, 0x13, 12, 2048 },
    { EXTABLE, 0x14, 12, 2112 },
    { EXTABLE, 0x15, 12, 2176 },
    { EXTABLE, 0x16, 12, 2240 },
    { EXTABLE, 0x17, 12, 2304 },
    { EXTABLE, 0x1c, 12, 2368 },
    { EXTABLE, 0x1d, 12, 2432 },
    { EXTABLE, 0x1e, 12, 2496 },
    { EXTABLE, 0x1f, 12, 2560 },
    };

static __inline__ void *
g3tableDummyReference(void) {
/*----------------------------------------------------------------------------
   This is a trick to stop the compiler from warning about unused static
   variables, because this header file is used in multiple programs, so
   it's perfectly all right if all the tables are not used.
-----------------------------------------------------------------------------*/
    if (0)
        return &mtable;
    if (0)
        return &ttable;
    if (0)
        return &twtable;
    if (0)
        return &mwtable;
    if (0)
        return &tbtable;
    if (0)
        return &mbtable;
    if (0)
        return &extable;
    return NULL;
}

#endif /*_G3_H_*/
