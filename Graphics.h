
#if ((!defined(_CI_)) && (!defined(_RGB_)))
 #error Either _CI_ or _RGB_ must be declared.
#endif
#if ((!defined(_Z_BUFFER_)) && (!defined(_PAINTER_)))
 #error Either _Z_BUFFER_ or _PAINTER_ must be declared.
#endif
#if ((!defined(_STEREO_)) && (!defined(_MONO_)))
 #error Either _STEREO_ or _MONO_ must be declared.
#endif

#if !defined(_GRAPHICS_H_)
#define _GRAPHICS_H_

/** 3DGPL *************************************************\
 * ()                                                     *
 * Header for 2D graphics stuff.                          *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model;                   *
 *  _Z_BUFFER_               Depth array;                 *
 *  _PAINTER_                Back to front order;         *
 *  _STEREO_                 Interlaced 2 page image;     *
 *  _MONO_                   Single page.                 * 
 *                                                        *
 * Files:                                                 *
 *  grp-base.c               Basic graphics;              *
 *  grp-text.c               Text;                        *
 *  grp-poly.c               Polygon rendering.           * 
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/

#include "LightTrack.h"           /* HW_pixel */

#define G_MAX_POLYGON_VERTICES  16          /* points in a polygon */
#define G_Z_MAPPING_SWITCH    1024          /* when switch linear/non-linear */
#define G_P                     16          /* fixed point precision */

#if defined(_CI_)
 #if defined(_Z_BUFFER_)                    /* Z I Tx Ty */
  #define G_LNG_FLAT     3                  /* X Y Z */
  #define G_LNG_SHADED   4                  /* X Y Z I */
  #define G_LNG_TEXTURED 6                  /* X Y Z I Tx Ty */
 #endif
 #if defined(_PAINTER_)                     /* I Tx Ty */
  #define G_LNG_FLAT     2                  /* X Y */
  #define G_LNG_SHADED   3                  /* X Y I */
  #define G_LNG_TEXTURED 5                  /* X Y I Tx Ty */
 #endif
#endif
#if defined(_RGB_)
 #if defined(_Z_BUFFER_)                    /* Z R G B Tx Ty */
  #define G_LNG_FLAT     3                  /* X Y Z */
  #define G_LNG_SHADED   6                  /* X Y Z R G B */
  #define G_LNG_TEXTURED 8                  /* X Y Z R G B Tx Ty */
 #endif
 #if defined(_PAINTER_)                     /* R G B Tx Ty */
  #define G_LNG_FLAT     2                  /* X Y */
  #define G_LNG_SHADED   5                  /* X Y R G B */
  #define G_LNG_TEXTURED 7                  /* X Y R G B Tx Ty */
 #endif
#endif

extern HW_pixel *G_c_buffer;
extern long G_c_buffer_size;
void G_init_graphics(void);

#if defined(_STEREO_)
 #define G_LEFT_EYE  0x0                    /* render image for left eye */
 #define G_RIGHT_EYE 0x1                    /* render image for right eye */
void G_page(int page_no);
#endif
#if defined(_CI_)
void G_clear(HW_pixel colour,int intensity);
void G_pixel(int *vertex,HW_pixel colour);
void G_dot(int *vertex,HW_pixel colour,int intensity);
void G_line(int *vertex1,int *vertex2,
            HW_pixel colour,int intensity
	   );
void G_text(int x,int y,char *txt,
            HW_pixel colour,int intensity
	   );
void G_flat_polygon(int *edges,int length,
                    HW_pixel colour,int intensity
                   );
#endif
#if defined(_RGB_)
void G_clear(HW_pixel colour,int red,int green,int blue);
void G_pixel(int *vertex,HW_pixel colour);
void G_dot(int *vertex,HW_pixel colour,int red,int green,int blue);
void G_line(int *vertex1,int *vertex2,
            HW_pixel colour,int red,int green,int blue
           );
void G_text(int x,int y,char *txt,
            HW_pixel colour,int red,int green,int blue
           );
void G_flat_polygon(int *edges,int length,
                    HW_pixel colour,int red,int green,int blue
                   );
#endif
void G_shaded_polygon(int *edges,int length,HW_pixel colour);
void G_lin_textured_polygon(int *edges,int length,
                            HW_pixel *texture,
                            int log_texture_size
                           );
void G_prp_textured_polygon(int *edges,int length,
                            int *O,int *u,int *v,
                            HW_pixel *texture,
                            int log_texture_size,
                            int log_texture_scale
                           );

/*********************************************************/

#endif
