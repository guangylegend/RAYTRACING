/** 3DGPL *************************************************\
 * ()                                                     *
 * Handling of colour and lighting.                       *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model;                   *
 *                                                        *
 * Defines:                                               *
 *  CL_init_colours          Initialization;              *
 *  CL_colour                Composing a colour;          *
 *  CL_light                 Lighting a colour.           *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//Color.cpp

#include "Colour.h"               /* colour models. */

int CL_div[CL_COLOUR_LEVELS][CL_LIGHT_LEVELS];
HW_pixel *CL_intensities;                   /* intensity table */

#if defined(_CI_)
struct CL_palette *CL_colours;              /* saved for hardware access */
#endif

/**********************************************************\
 * Initializing internal lookup tables.                   *
 *                                                        *
 * SETS: CL_intensity (when _CI_) CL_div (when _RGB_)     *
 * -----                                                  *
\**********************************************************/

#if defined(_CI_)
void CL_init_colour(struct CL_palette *palette)
{
 CL_colours=palette;
 CL_intensities=palette->cl_intensity_table;/* for palette based models */
}
#endif

//初始化内部查找表，具体不是特别清楚
#if defined(_RGB_)
void CL_init_colour(void)
{
 int i,j;                                   /* for RGB modes */

 for(i=0;i<CL_COLOUR_LEVELS;i++)
 {
  for(j=0;j<CL_LIGHT_LEVELS;j++)
  {
	//CL_LIGHT_MASK is for clamping light???? 0xff???
   CL_div[i][j]=(i*j)/CL_LIGHT_MASK;
  }
 }
}
#endif

/**********************************************************\
 * Composing a colour.                                    *
 *                                                        *
 * RETURNS: Packed colour, storable in a colourmap.       *
 * --------                                               *
\**********************************************************/

#if defined(_RGB_)
HW_pixel CL_colour(int red_light,
                   int green_light,
                   int blue_light
                  )
{
 return((CL_clamp_red(red_light)<<CL_RED_SHIFT) |
        (CL_clamp_green(green_light)<<CL_GREEN_SHIFT) |
        (CL_clamp_blue(blue_light)<<CL_BLUE_SHIFT)
       );
}
#endif

/**********************************************************\
 * Lighting a single pixel.                               *
 *                                                        *
 * RETURNS: Packed colour, storable in a colourmap.       *
 * --------                                               *
\**********************************************************/

#if defined(_CI_)
HW_pixel CL_light(HW_pixel colour,int light)
{
 return(CL_intensities[colour+(CL_clamp_light(light)<<CL_LOG_COLOUR_LEVELS)]);
}
#endif
#if defined(_RGB_)
HW_pixel CL_light(HW_pixel colour,int red_light,
                                  int green_light,
                                  int blue_light
                  )
{
 return((CL_div[CL_red(colour)][CL_clamp_light(red_light)]<<CL_RED_SHIFT)       |
        (CL_div[CL_green(colour)][CL_clamp_light(green_light)]<<CL_GREEN_SHIFT) |
        (CL_div[CL_blue(colour)][CL_clamp_light(blue_light)]<<CL_BLUE_SHIFT)
       );
}
#endif

/**********************************************************/
