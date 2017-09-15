/** 3DGPL *************************************************\
 * ()                                                     *
 * 2D basic graphics.                                     *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model;                   *
 *  _Z_BUFFER_               Depth array;                 *
 *  _PAINTER_                Back front order.            *
 *                                                        *
 * Defines:                                               *
 *  G_init_graphics          Initializing graphics;       *
 *  G_clear                  Clearing the bitmap;         *
 *                                                        *
 *  G_pixel                  Pixel into the colourmap;    *
 *  G_dot                    Dot into the colourmap;      *
 *  G_line                   Line into a colourmap.       *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//GraphicsBase.cpp

extern "C";


#include "LightTrack.h"           /* hardware specific stuff */
#include "Colour.h"               /* colour and light */
#include "Clipper.h"             /* 2D clipping routines */
#include "Graphics.h"           /* graphics functions */
#include <stdlib.h>                         /* malloc */
#include <limits.h>                         /* INT_MAX */



HW_pixel *G_c_buffer;                       /* the bitmap's bits */
int G_page_start;                           /* always 0 for _MONO_ */
long G_c_buffer_size;                       /* allocated size for clearings */

#if defined(_Z_BUFFER_)
int *G_z_buffer;                            /* Z buffer */
long G_z_buffer_size;                       /* it's size */
#endif

/**********************************************************\
 * Setting page for stereo images.                        *
 *                                                        *
 * SETS: G_page_start                                     *
 * -----                                                  *
\**********************************************************/

#if defined(_STEREO_)
void G_page(int page_no)                    /* between two pages */
{
 if(page_no==G_LEFT_EYE) G_page_start=0;
 else G_page_start=HW_SCREEN_X_SIZE;
}
#endif

/**********************************************************\
 * Allocating space for the colourmap.                    *
 *                                                        *
 * RETURNS: Pointer to the allocated colourmap.           *
 * --------                                               *
 * SETS: G_c_buffer,G_c_buffer_size,G_z_buffer,           *
 * ----- G_z_buffer_size                                  *
\**********************************************************/


//为颜色表分配空间
void G_init_graphics(void)
{
//空白空间的计算
 G_c_buffer_size=HW_SCREEN_LINE_SIZE*HW_SCREEN_Y_SIZE;

//HW_pixel==int
 G_c_buffer=(HW_pixel*)malloc(G_c_buffer_size*sizeof(HW_pixel));


 G_page_start=0;                            /* page 0 by default */
 if(G_c_buffer==NULL) HW_error("(Graphics) Not enough memory.\n");

#if defined(_Z_BUFFER_)                     /* for both pages */
 G_z_buffer_size=HW_SCREEN_LINE_SIZE*HW_SCREEN_Y_SIZE;

//不同的就是int,HW_pixel
 G_z_buffer=(int*)malloc(G_z_buffer_size*sizeof(int));
 if(G_z_buffer==NULL) HW_error("(Graphics) Not enough memory.\n");
#endif
}

/**********************************************************\
 * Clearing the bitmap with the specified colour.         *
\**********************************************************/

#if defined(_CI_)
void G_clear(HW_pixel colour,int intensity)
#endif
#if defined(_RGB_)
void G_clear(HW_pixel colour,int red,int green,int blue)
#endif
{
#if defined(_CI_)
 HW_set_pixel(G_c_buffer,G_c_buffer_size,CL_light(colour,intensity));
#endif
#if defined(_RGB_)
 HW_set_pixel(G_c_buffer,G_c_buffer_size,CL_light(colour,red,green,blue));
#endif
#if defined(_Z_BUFFER_)
 HW_set_int(G_z_buffer,G_z_buffer_size,INT_MAX);
#endif
}

/**********************************************************\
 * Setting a pixel.                                       *
\**********************************************************/

void G_pixel(int *vertex,HW_pixel colour)
{
 long pos;
 pos=vertex[1]*HW_SCREEN_LINE_SIZE+vertex[0]+G_page_start;

 G_c_buffer[pos]=colour;
}

/**********************************************************\
 * Rendering a dot.                                       *
\**********************************************************/

#if defined(_CI_)
void G_dot(int *vertex,HW_pixel colour,int intensity)
#endif
#if defined(_RGB_)
void G_dot(int *vertex,HW_pixel colour,int red,int green,int blue)
#endif
{
 long pos;

 if( (vertex[0]>=0)&&(vertex[0]<HW_SCREEN_X_SIZE) &&
     (vertex[1]>=0)&&(vertex[1]<HW_SCREEN_Y_SIZE) )
 {
  pos=vertex[1]*HW_SCREEN_LINE_SIZE+vertex[0]+G_page_start;

#if defined(_Z_BUFFER_)
  if(vertex[2]<G_z_buffer[pos])             /* doing Z check */
#endif
#if defined(_CI_)
   G_c_buffer[pos]=CL_light(colour,intensity);
#endif
#if defined(_RGB_)
   G_c_buffer[pos]=CL_light(colour,red,green,blue);
#endif
 }
}

/**********************************************************\
 * Rendering a line.                                      *
\**********************************************************/

#if defined(_CI_)
void G_line(int *vertex1,int *vertex2,
            HW_pixel colour,int intensity
	   )
#endif
#if defined(_RGB_)
void G_line(int *vertex1,int *vertex2,
            HW_pixel colour,int red,int green,int blue
           )
#endif
{
 register int inc_ah,inc_al;
 register int i;
 int *v1,*v2,pos;
 int dx,dy,long_d,short_d;
 int d,add_dh,add_dl;
 int inc_xh,inc_yh,inc_xl,inc_yl;
 register HW_pixel *adr_c=G_c_buffer;
#if defined(_Z_BUFFER_)
 int *adr_z=G_z_buffer;
 HW_32_bit cur_z,inc_z;
#endif
#if defined(_CI_)
 HW_pixel hwcolour=CL_light(colour,intensity);
#endif
#if defined(_RGB_)
 HW_pixel hwcolour=CL_light(colour,red,green,blue);
#endif

 v1=(int*)vertex1;
 v2=(int*)vertex2;

 if(C_line_x_clipping(&v1,&v2,2))           /* horizontal clipping */
 {
  if(C_line_y_clipping(&v1,&v2,2))          /* vertical clipping */
  {
   dx=v2[0]-v1[0]; dy=v2[1]-v1[1];          /* ranges */

   if(dx<0){dx=-dx; inc_xh=-1; inc_xl=-1;}  /* making sure dx and dy >0 */
   else    {        inc_xh=1;  inc_xl=1; }  /* adjusting increments */

   if(dy<0){dy=-dy;inc_yh=-HW_SCREEN_LINE_SIZE;
                   inc_yl=-HW_SCREEN_LINE_SIZE;
           }                                /* to get to the neighboring */
   else    {       inc_yh= HW_SCREEN_LINE_SIZE;
                   inc_yl= HW_SCREEN_LINE_SIZE;
           }
   if(dx>dy){long_d=dx;short_d=dy;inc_yl=0;}/* long range,&make sure either */
   else     {long_d=dy;short_d=dx;inc_xl=0;}/* x or y is changed in L case */

   inc_ah=inc_xh+inc_yh;
   inc_al=inc_xl+inc_yl;                    /* increments for point address */

   d=2*short_d-long_d;                      /* initial value of d */
   add_dl=2*short_d;                        /* d adjustment for H case */
   add_dh=2*(short_d-long_d);               /* d adjustment for L case */
   pos=v1[1]*HW_SCREEN_LINE_SIZE+v1[0]+G_page_start;
   adr_c+=pos;                              /* address of first point */

#if defined(_Z_BUFFER_)
   adr_z+=pos;                              /* same for z-buffer */
   cur_z=v1[2];
   if(long_d!=0) inc_z=(v2[2]-cur_z)/long_d;
#endif

   for(i=0;i<=long_d;i++)                   /* for all points in long range */
   {
#if defined(_Z_BUFFER_)
    if(cur_z<*adr_z)
#endif
     *adr_c=hwcolour;                       /* rendering */

    if(d>=0)
    {
     d+=add_dh;
     adr_c+=inc_ah;                         /* new in the colour-buffer */
#if defined(_Z_BUFFER_)
     adr_z+=inc_ah;                         /* new in Z-buffer */
#endif
    }                                       /* previous point was H type */
    else
    {
     d+=add_dl;
     adr_c+=inc_al;                         /* new in the colour-buffer */
#if defined(_Z_BUFFER_)
     adr_z+=inc_al;                         /* new in Z-buffer */
#endif
    }                                       /* previous point was L type */
#if defined(_Z_BUFFER_)
    cur_z+=inc_z;
#endif
   }
  }
 }
}

/**********************************************************/
