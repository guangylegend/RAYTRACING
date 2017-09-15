#if !defined(_COLOUR_H_)
#define _COLOUR_H_

/** 3DGPL *************************************************\
 * ()                                                     *
 * Header for colour related stuff.                       *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model;                   *
 *  _8BPP_                   8 bits per pixel;            *
 *  _16BPP_                  16 bits per pixel;           *
 *  _32BPP_                  32 bits per pixel.           *
 *                                                        *
 * Files:                                                 *
 *  colour.c                 Hardware specific stuff.     *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/

#include "LightTrack.h"           /* HW_pixel */

#if defined(_CI_)                           /* palette based model */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * CI colour model.                                      *
 * Represents each colour as an index (C) and its        *
 * intensity (I). Limited number of colours is placed    *
 * into the hardware palette and a square lookup table   *
 * is maintained mapping colour indices and intensities  *
 * to colours in the palette.                            *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define CL_LNG_COLOUR           1           /* intensity */

#define CL_COLOUR_LEVELS      256           /* number of colours */
#define CL_LIGHT_LEVELS        32           /* gradations of light intensity */
#define CL_LOG_COLOUR_LEVELS    8           
#define CL_LIGHT_MASK        0x1f           /* for clamping light */

#define CL_clamp_light(colour) (((colour)>CL_LIGHT_MASK)?CL_LIGHT_MASK:(colour))

/**********************************************************\
 * A colour.                                              *
 *                                                        *
 * +----------+                                           *
 * | cl_red   |                                           *
 * | cl_green |                                           *
 * | cl_blue  |                                           *
 * +----------+                                           *
 *                                                        *
\**********************************************************/

struct CL_colour                            /* describes colour */
{
 int cl_red;
 int cl_green;
 int cl_blue;                               /* intensity components */
};

/**********************************************************\
 * A palette.                                             *
 *                                                        *
 * +---------------+                                      *
 * | cl_no_colours |                                      *
 * |               |   +-----------+--  -+                *
 * | cl_colours------->| CL_colour | ... |                *
 * |               |   +-----------+-----+                *
 * +---------------+                                      *
 *                                                        *
\**********************************************************/

struct CL_palette                           /* describes a palette */
{
 int cl_no_colours;
 struct CL_colour *cl_colours;              /* actual colours */
 HW_pixel *cl_intensity_table;
};

extern struct CL_palette *CL_colours;
void CL_init_colour(struct CL_palette *palette);
HW_pixel CL_light(HW_pixel colour,int light);

#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * RGB colour model.                                     *
 * Each colour is represented as three intensities one   *
 * for each of the pure components red, green and blue.  *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if defined(_RGB_)                          /* component based model */

#define CL_LNG_COLOUR           3           /* R G B */

#if defined(_8BPP_)                         /* 8 bits per pixel */
#define CL_COLOUR_LEVELS        8           /* number of colours */
#define CL_LIGHT_LEVELS       256           /* gradations of light intensity */
#define CL_COLOUR_MASK_RED   0x07           /* for clamping colours */
#define CL_COLOUR_MASK_GREEN 0x03           
#define CL_COLOUR_MASK_BLUE  0x07           
#define CL_LIGHT_MASK        0xff           /* for clamping light */
#define CL_RED_SHIFT            5           /* offset for red */
#define CL_GREEN_SHIFT          3           /* offset for green */
#define CL_BLUE_SHIFT           0           /* offset for blue */
#endif

#if defined(_16BPP_)                        /* 16 bits per pixel */
#define CL_COLOUR_LEVELS       32           /* number of colours */
#define CL_LIGHT_LEVELS       256           /* gradations of light intensity */
#define CL_COLOUR_MASK_RED   0x1f           /* for clamping colours */
#define CL_COLOUR_MASK_GREEN 0x1f           
#define CL_COLOUR_MASK_BLUE  0x1f           
#define CL_LIGHT_MASK        0xff           /* for clamping light */
#define CL_RED_SHIFT           11           /* offset for red */
#define CL_GREEN_SHIFT          6           /* offset for green */
#define CL_BLUE_SHIFT           0           /* offset for blue */
#endif

#if defined(_32BPP_)                        /* 32 bits per pixel */
#define CL_COLOUR_LEVELS      256           /* number of colours */
#define CL_LIGHT_LEVELS       256           /* gradations of light intensity */
#define CL_COLOUR_MASK_RED   0xff           /* for clamping colours */
#define CL_COLOUR_MASK_GREEN 0xff           
#define CL_COLOUR_MASK_BLUE  0xff           
#define CL_LIGHT_MASK        0xff           /* for clamping light */
#define CL_RED_SHIFT           16           /* offset for red */
#define CL_GREEN_SHIFT          8           /* offset for green */
#define CL_BLUE_SHIFT           0           /* offset for blue */
#endif

#define CL_clamp_red(colour)   (((colour)>CL_COLOUR_MASK_RED)?CL_COLOUR_MASK_RED:(colour))
#define CL_clamp_green(colour) (((colour)>CL_COLOUR_MASK_GREEN)?CL_COLOUR_MASK_GREEN:(colour))
#define CL_clamp_blue(colour)  (((colour)>CL_COLOUR_MASK_BLUE)?CL_COLOUR_MASK_BLUE:(colour))
#define CL_clamp_light(colour) (((colour)>CL_LIGHT_MASK)?CL_LIGHT_MASK:(colour))
#define CL_red(colour)   (((colour)>>CL_RED_SHIFT)&CL_COLOUR_MASK_RED)
#define CL_green(colour) (((colour)>>CL_GREEN_SHIFT)&CL_COLOUR_MASK_GREEN)
#define CL_blue(colour)  (((colour)>>CL_BLUE_SHIFT)&CL_COLOUR_MASK_BLUE)

void CL_init_colour(void);
HW_pixel CL_colour(int red_light,
                   int green_light,
                   int blue_light
                  );
HW_pixel CL_light(HW_pixel colour,int red_light,
                                  int green_light,
                                  int blue_light
                 );
#endif

/**********************************************************/

#endif
