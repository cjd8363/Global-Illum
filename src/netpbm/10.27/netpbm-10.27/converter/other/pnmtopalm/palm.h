#ifndef PALM_H_INCLUDED
#define PALM_H_INCLUDED

#define PALM_IS_COMPRESSED_FLAG     0x8000
#define PALM_HAS_COLORMAP_FLAG      0x4000
#define PALM_HAS_TRANSPARENCY_FLAG  0x2000
#define PALM_INDIRECT_BITMAP        0x1000  /* Palm says internal use only */
#define PALM_FOR_SCREEN             0x0800  /* Palm says internal use only */
#define PALM_DIRECT_COLOR_FLAG      0x0400
#define PALM_INDIRECT_COLORMAP      0x0200  /* Palm says internal use only */ 
#define PALM_NO_DITHER_FLAG         0x0100  /* rather mysterious */

#define PALM_COMPRESSION_SCANLINE   0x00
#define PALM_COMPRESSION_RLE        0x01
#define PALM_COMPRESSION_PACKBITS   0x02
#define PALM_COMPRESSION_END        0x03    /* Palm says internal use only */
#define PALM_COMPRESSION_BEST       0x64    /* Palm says internal use only */
#define PALM_COMPRESSION_NONE       0xFF    /* Palm says internal use only */

#define PALM_DENSITY_LOW             72
#define PALM_DENSITY_ONEANDAHALF    108
#define PALM_DENSITY_DOUBLE         144
#define PALM_DENSITY_TRIPLE         216
#define PALM_DENSITY_QUADRUPLE      288

#define PALM_FORMAT_INDEXED     0x00
#define PALM_FORMAT_565         0x01
#define PALM_FORMAT_565LE       0x02    /* Palm says internal use only */
#define PALM_FORMAT_INDEXEDLE   0x03    /* Palm says internal use only */

typedef unsigned long Color_s, *Color;

typedef struct {
  unsigned int nentries;    /* number of allocated Colors */
  unsigned int ncolors;     /* number of actually used Colors */
  Color color_entries;      /* pointer to vector of Colors */
} Colormap_s, *Colormap;

int 
palmcolor_compare_indices(const void *, 
                          const void *);

int 
palmcolor_compare_colors(const void *, 
                         const void *);

Colormap 
palmcolor_build_custom_8bit_colormap(unsigned int rows, 
                                     unsigned int cols, 
                                     pixel **     pixels);

Colormap 
palmcolor_build_default_8bit_colormap(void);

Colormap 
palmcolor_read_colormap(FILE *);

#endif
