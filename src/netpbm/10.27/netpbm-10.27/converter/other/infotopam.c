/* infotopam:  A program to convert Amiga Info icon files to PAM files
 * Copyright (C) 2004  Richard Griswold - griswold@acm.org
 *
 * Thanks to the following people on comp.sys.amiga.programmer for tips
 * and pointers on decoding the info file format:
 *
 *   Ben Hutchings
 *   Thomas Richter
 *   Kjetil Svalastog Matheussen
 *   Anders Melchiorsen
 *   Dirk Stoecker
 *   Ronald V.D.
 *
 * The format of the Amiga info file is as follows:
 *
 *   DiskObject header            (78 bytes)
 *   Optional DrawerData header   (56 bytes)
 *   First icon header            (20 bytes)
 *   First icon data
 *   Second icon header           (20 bytes)
 *   Second icon data
 *
 * The DiskObject header contains, among other things, the magic number
 * (0xE310), the object width and height (inside the embedded Gadget header),
 * and the version.
 *
 * Each icon header contains the icon width and height, which can be smaller
 * than the object width and height, and the number of bit-planes.
 *
 * The icon data has the following format:
 *
 *   BIT-PLANE planes, each with HEIGHT rows of (WIDTH +15) / 16 * 2 bytes
 *   length.
 *
 * So if you have a 9x3x2 icon, the icon data will look like this:
 *
 *   aaaa aaaa a000 0000
 *   aaaa aaaa a000 0000
 *   aaaa aaaa a000 0000
 *   bbbb bbbb b000 0000
 *   bbbb bbbb b000 0000
 *   bbbb bbbb b000 0000
 *
 * Where 'a' is a bit for the first bit-plane, 'b' is a bit for the second
 * bit-plane, and '0' is padding.  Thanks again to Ben Hutchings for his
 * very helpful post!
 *
 * This program uses code from "sidplay" and an older "infotoxpm" program I
 * wrote, both of which are released under GPL.
 *
 *-------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*-----------
 * Includes |
 *-------------------------------------------------------------------------*/
#include "pam.h"
#include "shhopt.h"
#include "mallocvar.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------
 * Structs |
 *-------------------------------------------------------------------------*/

/* Struct to hold miscellaneous icon information */
typedef struct IconInfo_ {
  const char  *name;        /* Icon file name */
  FILE        *fp;          /* Input file pointer */

  bool         forceColor;  /* True if convert 1 bitplane icon to color icon */
  unsigned int numColors;   /* Number of colors to override */
  bool         selected;    /* True if converting selected (second) icon */

  bool         drawerData;  /* True if icon has drawer data */
  unsigned int version;     /* Icon version */
  unsigned int width;       /* Width in pixels */
  unsigned int height;      /* Height in pixels */
  unsigned int depth;       /* Bits of color per pixel */
  pixel        colors[4];   /* Colors to use for converted icons */
  unsigned char *icon;      /* Completed icon */

} IconInfo;

/* Header for each icon image */
typedef struct IconHeader_ { /* 20 bytes */
  unsigned char pad0[4];        /* Padding (always seems to be zero) */
  unsigned char iconWidth[2];   /* Width (usually equal to Gadget width) */
  unsigned char iconHeight[2];  
    /* Height (usually equal to Gadget height -1) */
  unsigned char bpp[2];         /* Bits per pixel */
  unsigned char pad1[10];       /* ??? */
} IconHeader;

/*
 * Gadget and DiskObject structs come from the libsidplay 1.36.57 info_.h file
 * http://www.geocities.com/SiliconValley/Lakes/5147/sidplay/linux.html
 */
typedef struct DiskObject_ { /* 78 bytes (including Gadget struct) */
  unsigned char magic[2];          /* Magic number at the start of the file */
  unsigned char version[2];        /* Object version number */
  unsigned char gadget[44];        /* Copy of in memory gadget (44 by */
  unsigned char type;              /* ??? */
  unsigned char pad;               /* Pad it out to the next word boundry */
  unsigned char pDefaultTool[4];   /* Pointer  to default tool */
  unsigned char ppToolTypes[4];    /* Pointer pointer to tool types */
  unsigned char currentX[4];       /* Current X position (?) */
  unsigned char currentY[4];       /* Current Y position (?) */
  unsigned char pDrawerData[4];    /* Pointer to drawer data */
  unsigned char pToolWindow[4];    /* Ptr to tool window - only for tools */
  unsigned char stackSize[4];      /* Stack size - only for tools */
} DiskObject;

/*-------------
 * Prototypes |
 *-------------------------------------------------------------------------*/

static void parseCommandLine( int argc, char *argv[], IconInfo *infoP );
static void getDiskObject( IconInfo *infoP );
static void getIconHeader( IconInfo *infoP );
static void readIconData ( IconInfo *infoP );
static void writeIconData( IconInfo *infoP, struct pam *pamP );

/*-------
 * main |
 *-------------------------------------------------------------------------*/
int main( int argc, char *argv[] ) {
  IconInfo    info;    /* Miscellaneous icon information */
  struct pam  pam;     /* PAM header */
  int         skip;    /* Bytes to skip to read next icon header */

  /* Init PNM library */
  pnm_init( &argc, argv );

  /* Parse command line arguments */
  parseCommandLine( argc, argv, &info );

  /* Open input file */
  info.fp = pm_openr( info.name );

  /* Read disk object header */
  getDiskObject( &info );

  /* Skip drawer data, if any */
  if ( info.drawerData ) {
    skip = 56;   /* Draw data size */
    if ( fseek( info.fp, skip, SEEK_CUR ) < 0 ) {
      pm_close( info.fp );
      pm_error( "Cannot skip header information in file %s: %s\n",
        info.name, strerror( errno ) );
    }
  }

  /* Get dimensions for first icon */
  getIconHeader( &info );

  /* Skip ahead to next header if converting second icon */
  if ( info.selected ) {
    skip = info.height * ( ( ( info.width + 15 ) / 16 ) * 2 ) * info.depth;

    if ( fseek( info.fp, skip, SEEK_CUR ) < 0 ) {
      pm_close( info.fp );
      pm_error( "Cannot skip to next icon in file %s: %s\n",
        info.name, strerror( errno ) );
    }

    /* Get dimensions for second icon */
    getIconHeader( &info );
  }

  /* Read icon data */
  readIconData( &info );

  /* Print icon info */
  pm_message( "converting %s, version %d, %s icon: %d X %d X %d",
    info.name, info.version, info.selected ? "second" : "first",
    info.width, info.height, info.depth );

  /* Write PAM header */
  pam.size   = sizeof( pam );
  pam.len    = PAM_STRUCT_SIZE( tuple_type );
  pam.file   = stdout;
  pam.height = info.height;
  pam.width  = info.width;
  pam.format = PAM_FORMAT;

  if ( ( info.depth == 1 ) && ( info.forceColor == FALSE ) ) {
    pam.depth  = 1;
    pam.maxval = 1;
    strcpy( pam.tuple_type, "BLACKANDWHITE" );
  } else {
    pam.depth  = 3;
    pam.maxval = 0xFF;
    strcpy( pam.tuple_type, "RGB" );
  }
  pnm_writepaminit( &pam );

  /* Write icon data */
  writeIconData( &info, &pam );

  /* Close input file and return */
  pm_close( pam.file );
  pm_close( info.fp );
  return 0;
}

/*-------------------
 * parseCommandLine |
 *-------------------------------------------------------------------------
 * Get command line arguments
 *-------------------------------------------------------------------------*/
static void parseCommandLine( int argc, char *argv[], IconInfo *infoP ) {
  unsigned int numColorArgs,  /* Number of arguments for overriding colors */
               colorIdx,      /* Color index */
               i;             /* Argument index */
  const char  * const colors[4] = { 
      /* Pixel colors based on original Amiga colors */
      "#0055AA",    /*   Blue      0,  85, 170 */
      "#FFFFFF",    /*   White   255, 255, 255 */
      "#000020",    /*   Black     0,   0,  32 */
      "#FF8A00"     /*   Orange  255, 138,   0 */
  };

  /* Option entry variables */
  optEntry     *option_def;
  optStruct3    opt;
  unsigned int  option_def_index;
  unsigned int numColorsSpec, forceColorSpec, selectedSpec;
  
  MALLOCARRAY_NOFAIL(option_def, 100);

  /* Set command line options */
  option_def_index = 0;   /* Incremented by OPTENT3 */
  OPTENT3(0, "forcecolor", OPT_FLAG, NULL, &forceColorSpec, 0);
  OPTENT3(0, "numcolors",  OPT_UINT, &infoP->numColors, &numColorsSpec, 0);
  OPTENT3(0, "selected",   OPT_FLAG, NULL, &selectedSpec,   0);

  /* Initialize the iconInfo struct */
  infoP->name = NULL;
  infoP->fp = NULL;
  infoP->drawerData = FALSE;
  infoP->version = 0;
  infoP->width = 0;
  infoP->height = 0;
  infoP->depth = 0;
  infoP->icon = NULL;
  for ( colorIdx = 0; colorIdx < 4; colorIdx++ ) {
    infoP->colors[colorIdx] = ppm_parsecolor( (char*) colors[colorIdx], 0xFF );
  }

  /* Initialize option structure */
  opt.opt_table     = option_def;
  opt.short_allowed = FALSE;  /* No short (old-fashioned) options */
  opt.allowNegNum   = FALSE;  /* No negative number parameters */

  /* Parse the command line */
  optParseOptions3( &argc, argv, opt, sizeof( opt ), 0 );

  infoP->forceColor = forceColorSpec;
  infoP->selected = selectedSpec;
  if (!numColorsSpec)
      infoP->numColors = 0;

  /* Get colors and file name */
  numColorArgs = infoP->numColors * 2;
  if ( ( argc - 1 != numColorArgs ) && ( argc - 1 != numColorArgs + 1 ) ) {
    pm_error( "Wrong number of arguments for number of colors.  For %d\n"
      "colors, you need %d color arguments, with possibly one more argument\n"
      "for the input file name.\n", infoP->numColors, numColorArgs );
  }

  /* Convert color arguments */
  for ( i = 1; i < numColorArgs; i += 2 ) {
    char        *endptr;        /* End pointer for strtol() */
    unsigned int colorIdx;

    /* Get color index from argument */
    endptr = NULL;
    colorIdx = strtoul( argv[i], &endptr, 0 );

    if ( *endptr != '\0' ) {
      pm_error( "%s is not a valid color index\n", argv[i] );
    }

    /* Check color index range (current 0 to 3) */
    if ( ( colorIdx < 0 ) || ( colorIdx > 3 ) ) {
      pm_error( "%d is not a valid color index (minimum 0, maximum 3)\n",
        colorIdx );
    }

    /* Convert the color for this color index */
    infoP->colors[colorIdx] = ppm_parsecolor( argv[i+1], 0xFF );
  }

  /* Set file name */
  if ( i == argc ) {
    infoP->name = "-";  /* Read from standard input */
  } else {
    infoP->name = argv[i];
  }
}

/*----------------
 * getDiskObject |
 *-------------------------------------------------------------------------
 * Get fields from disk object portion of info file
 *-------------------------------------------------------------------------*/
static void getDiskObject( IconInfo *infoP ) {
  DiskObject  dobj;      /* Disk object structure */
  int         dobjSize,  /* Disk object structure size */
              rc;        /* Return code from fread() */

  /* Read the disk object header */
  dobjSize = sizeof( dobj );
  if ( ( rc = fread( &dobj, sizeof( char ), dobjSize, infoP->fp ) ) < 0 ) {
    pm_close( infoP->fp );
    pm_error( "Cannot read disk object header for file %s: %s\n",
      infoP->name, strerror( errno ) );
  }
  if ( rc != dobjSize ) {
    pm_close( infoP->fp );
    pm_error( "Cannot read entire disk object header for file %s\n"
      "Only read 0x%X of 0x%X bytes\n", infoP->name, rc, dobjSize );
  }

  /* Check magic number */
  if ( ( dobj.magic[0] != 0xE3 ) && ( dobj.magic[1] != 0x10 ) ) {
    pm_close( infoP->fp );
    pm_error( "Wrong magic number for file %s\n"
      "Expected 0xE310, but got 0x%X%X\n",
      infoP->name, dobj.magic[0], dobj.magic[1] );
  }

  /* Set version info and have drawer data flag */
  infoP->version     = ( dobj.version[0]     <<  8 ) +
                       ( dobj.version[1]           );
  infoP->drawerData  = ( dobj.pDrawerData[0] << 24 ) +
                       ( dobj.pDrawerData[1] << 16 ) +
                       ( dobj.pDrawerData[2] <<  8 ) +
                       ( dobj.pDrawerData[3]       ) ? TRUE : FALSE;
}

/*----------------
 * getIconHeader |
 *-------------------------------------------------------------------------
 * Get fields from icon header portion of info file
 *-------------------------------------------------------------------------*/
static void getIconHeader( IconInfo *infoP ) {
  IconHeader  ihead;      /* Icon header structure */
  int         iheadSize,  /* Icon header structure size */
              rc;         /* Return code from fread() */

  /* Read icon header */
  iheadSize = sizeof( ihead );
  if ( ( rc = fread( &ihead, sizeof( char ), iheadSize, infoP->fp ) ) < 0 ) {
    pm_close( infoP->fp );
    pm_error( "Cannot read icon header for file %s: %s\n",
      infoP->name, strerror( errno ) );
  }
  if ( rc != iheadSize  ) {
    pm_close( infoP->fp );
    pm_error( "Cannot read the entire icon header for file %s\n"
      "Only read 0x%X of 0x%X bytes\n", infoP->name, rc, iheadSize  );
  }

  /* Get icon width, heigh, and bitplanes */
  infoP->width  = ( ihead.iconWidth[0]  << 8 ) + ihead.iconWidth[1];
  infoP->height = ( ihead.iconHeight[0] << 8 ) + ihead.iconHeight[1];
  infoP->depth  = ( ihead.bpp[0]        << 8 ) + ihead.bpp[1];

  /* Check number of bit planes */
  if ( ( infoP->depth > 2 ) || ( infoP->depth < 1 ) ) {
    pm_close( infoP->fp );
    pm_error( "%d bitplanes is not supported in file %s\n"
      "Only 1 or 2 bitplanes are supported!  Please send this file to\n"
      "griswold@acm.org so I can add support for higher color icons!\n",
      infoP->depth, infoP->name );
  }
}

/*---------------
 * readIconData |
 *-------------------------------------------------------------------------
 * Read icon data from file
 *-------------------------------------------------------------------------*/
static void readIconData( IconInfo *infoP ) {
  int             bitplane, /* Bitplane index */
                  i, j,     /* Indicies */
                  rc,       /* Return code */
                  bpsize,   /* Bitplane size in bytes, with padding */
                  toread;   /* Number of bytes left to read */
  unsigned char  *buff,     /* Buffer to hold bits for 1 bitplane */
                 *buffp;    /* Buffer point for reading data */

  /* Calculate bitplane width in bits and size in bytes */
  bpsize  = infoP->height * ( ( ( infoP->width + 15 ) / 16 ) * 2 );
  
  MALLOCARRAY( buff, bpsize );
  if ( buff == NULL )
      pm_error( "Cannot allocate memory to hold icon pixels" );
  MALLOCARRAY( infoP->icon, bpsize * 8 );

  if ( infoP->icon == NULL )
    pm_error( "Cannot allocate memory to hold icon" );

  /* Initialize to zero */
  memset( buff,        0, bpsize     );
  memset( infoP->icon, 0, bpsize * 8 );

  /* Each bitplane is stored independently in the icon file.  This loop reads 
   * one bitplane at a time into buff.  Since fread() may not read all of the 
   * bitplane on the first call, the inner loop continues until all bytes are 
   * read.  The buffer pointer, bp, points to the next byte in buff to fill 
   * in.  When the inner loop is done, bp points to the end of buff. 
   *
   * After reading in the entire bitplane, the second inner loop splits the 
   * eight pixels in each byte of the bitplane into eight separate bytes in 
   * the icon buffer.  The existing contents of each byte in icon are left 
   * shifted by one to make room for the next bit. 
   *
   * Each byte in the completed icon contains a value from 0 to 2^depth 
   * (0 to 1 for depth of 1 and 0 to 3 for a depth of 3).  This is an index 
   * into the colors array in the info struct.
   */
  for ( bitplane = 0; bitplane < infoP->depth; bitplane++ ) {
    /* Read bitplane into buffer */
    for ( toread = bpsize, buffp = ( unsigned char * ) buff;
          toread > 0;
          toread -= rc, buffp += rc ) {
      if ( ( rc = fread( buffp, sizeof( char ), toread, infoP->fp ) ) < 0 ) {
        free( infoP->icon ); free( buff );
        pm_close( infoP->fp );
        pm_error( "Cannot read from file %s: %s",
          infoP->name, strerror( errno ) );
      }
      if ( rc == 0 ) {
        free( infoP->icon ); free( buff );
        pm_close( infoP->fp );
        pm_error( "Premature end-of-file.  Still have 0x%X bytes to read \n",
          toread );
      }
    }

    /* Add bitplane to existing icon image */
    for ( i = j = 0; i < bpsize; i++, j += 8 ) {
      infoP->icon[j  ] = ( infoP->icon[j  ] << 1 ) | (   buff[i]        & 1 );
      infoP->icon[j+1] = ( infoP->icon[j+1] << 1 ) | ( ( buff[i] >> 1 ) & 1 );
      infoP->icon[j+2] = ( infoP->icon[j+2] << 1 ) | ( ( buff[i] >> 2 ) & 1 );
      infoP->icon[j+3] = ( infoP->icon[j+3] << 1 ) | ( ( buff[i] >> 3 ) & 1 );
      infoP->icon[j+4] = ( infoP->icon[j+4] << 1 ) | ( ( buff[i] >> 4 ) & 1 );
      infoP->icon[j+5] = ( infoP->icon[j+5] << 1 ) | ( ( buff[i] >> 5 ) & 1 );
      infoP->icon[j+6] = ( infoP->icon[j+6] << 1 ) | ( ( buff[i] >> 6 ) & 1 );
      infoP->icon[j+7] = ( infoP->icon[j+7] << 1 ) | ( ( buff[i] >> 7 ) & 1 );

    }
  }

  free( buff );
}

/*----------------
 * writeIconData |
 *-------------------------------------------------------------------------
 * Write icon data to file
 *-------------------------------------------------------------------------*/
static void writeIconData( IconInfo *infoP, struct pam *pamP ) {
  tuple    *row;      /* Output row */
  int       i, j;     /* Indicies */
  unsigned  char colorIdx; /* Color index */
  unsigned int bpwidth;  /* Bitplane width */

  /* Width of each row in icon, including padding */
  bpwidth = ( ( infoP->width + 15 ) / 16 ) * 16;

  /* Allocate row */
  row = pnm_allocpamrow( pamP );

  /* Write icon image to output file */
  /* Put if check outside for loop to reduce number of times check is made */
  if ( infoP->depth == 1 ) {
    if ( infoP->forceColor ) {
      /* Convert 1 bitplane icon into color PAM */
      for ( i = 0; i < infoP->height; i++ ) {
        for ( j = 0; j < infoP->width; j++ ) {
          /* 1 is black and 0 is white */
          colorIdx = infoP->icon[ i * bpwidth + j ] ? 2 : 1;
          row[j][PAM_RED_PLANE] = PPM_GETR( infoP->colors[colorIdx] );
          row[j][PAM_GRN_PLANE] = PPM_GETG( infoP->colors[colorIdx] );
          row[j][PAM_BLU_PLANE] = PPM_GETB( infoP->colors[colorIdx] );
        }
        pnm_writepamrow( pamP, row );
      }
    } else {
      /* Convert 1 bitplane icon into bitmap PAM */
      for ( i = 0; i < infoP->height; i++ ) {
        for ( j = 0; j < infoP->width; j++ ) {
          /* 1 is black and 0 is white */
          row[j][0] = infoP->icon[ i * bpwidth + j ] ? 0 : 1;
        }
        pnm_writepamrow( pamP, row );
      }
    }
  } else {
    /* Convert color icon into color PAM */
    for ( i = 0; i < infoP->height; i++ ) {
      for ( j = 0; j < infoP->width; j++ ) {
        colorIdx = infoP->icon[ i * bpwidth + j ];
        row[j][PAM_RED_PLANE] = PPM_GETR( infoP->colors[colorIdx] );
        row[j][PAM_GRN_PLANE] = PPM_GETG( infoP->colors[colorIdx] );
        row[j][PAM_BLU_PLANE] = PPM_GETB( infoP->colors[colorIdx] );
      }
      pnm_writepamrow( pamP, row );
    }
  }

  /* Clean up allocated memory */
  pnm_freepamrow( row );
  free( infoP->icon );
}
