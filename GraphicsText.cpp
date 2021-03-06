/** 3DGPL *************************************************\
 * ()                                                     *
 * 2D graphics and 2D clipping.                           *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model.                   *
 *                                                        *
 * Defines:                                               *
 *  G_text                   Rendering a text string.     *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//GraphicsText.cpp

#include "LightTrack.h"           /* hardware specific stuff */
#include "Colour.h"               /* colour and light */
#include "Graphics.h"           /* graphics functions */

extern HW_pixel *G_c_buffer;                /* the bitmap's bits */
extern int G_page_start;                    /* always 0 for _MONO_ */

unsigned char G_font[354]=                  /* The 8x6 font */
{
 0x00,0x00,0x00,0x00,0x00,0x00, 0x30,0x30,0x30,0x00,0x30,0x00,
 0xd8,0x90,0x00,0x00,0x00,0x00, 0x6c,0xfe,0x6c,0xfe,0x6c,0x00,
 0x7e,0xd0,0x7c,0x16,0xfc,0x00, 0xcc,0x18,0x30,0x60,0xcc,0x00,
 0x60,0x90,0x74,0xd8,0x6c,0x00, 0x30,0x20,0x00,0x00,0x00,0x00,
 0x60,0xc0,0xc0,0xc0,0x60,0x00, 0x18,0x0c,0x0c,0x0c,0x18,0x00,
 0x6c,0x38,0xfe,0x38,0x6c,0x00, 0x00,0x30,0xfc,0x30,0x00,0x00,
 0x00,0x00,0x00,0x30,0x20,0x00, 0x00,0x00,0x7c,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0xc0,0x00, 0x0c,0x18,0x30,0x60,0xc0,0x00,
 0x78,0x84,0x84,0x84,0x78,0x00, 0x04,0x0c,0x14,0x04,0x04,0x00,
 0x78,0x84,0x18,0x20,0x7c,0x00, 0x78,0x04,0x38,0x04,0x78,0x00,
 0x84,0x84,0xfc,0x04,0x04,0x00, 0xfc,0x80,0xf8,0x04,0xf8,0x00,
 0x30,0x40,0xf8,0x84,0x78,0x00, 0xfc,0x04,0x08,0x10,0x10,0x00,
 0x78,0x84,0x78,0x84,0x78,0x00, 0x78,0x84,0x7c,0x08,0x30,0x00,
 0x00,0x30,0x00,0x30,0x00,0x00, 0x00,0x30,0x00,0x30,0x20,0x00,
 0x30,0x60,0xc0,0x60,0x30,0x00, 0x00,0x7c,0x00,0x7c,0x00,0x00,
 0x30,0x18,0x0c,0x18,0x30,0x00, 0x78,0xcc,0x18,0x00,0x18,0x00,
 0x78,0xcc,0xdc,0xc0,0x7c,0x00, 0x30,0x48,0x84,0xfc,0x84,0x00,
 0xf8,0x84,0xf8,0x84,0xf8,0x00, 0x7c,0x80,0x80,0x80,0x7c,0x00,
 0xf8,0x84,0x84,0x84,0xf8,0x00, 0xfc,0x80,0xf0,0x80,0xfc,0x00,
 0xfc,0x80,0xf0,0x80,0x80,0x00, 0x7c,0x80,0x84,0x84,0x7c,0x00,
 0x84,0x84,0xfc,0x84,0x84,0x00, 0x10,0x10,0x10,0x10,0x10,0x00,
 0x04,0x04,0x04,0x84,0x78,0x00, 0x84,0x88,0xf0,0x88,0x84,0x00,
 0x80,0x80,0x80,0x80,0xfc,0x00, 0x82,0xc6,0xaa,0x92,0x82,0x00,
 0x84,0xc4,0xa4,0x94,0x8c,0x00, 0x78,0x84,0x84,0x84,0x78,0x00,
 0xf8,0x84,0xf8,0x80,0x80,0x00, 0x78,0x84,0x84,0x78,0x1c,0x00,
 0xf8,0x84,0xf8,0x90,0x88,0x00, 0x7c,0x80,0x78,0x04,0xf8,0x00,
 0xfe,0x10,0x10,0x10,0x10,0x00, 0x84,0x84,0x84,0x84,0x78,0x00,
 0x84,0x84,0x84,0x48,0x30,0x00, 0x82,0x82,0x92,0xaa,0x44,0x00,
 0x84,0x48,0x30,0x48,0x84,0x00, 0x88,0x88,0x50,0x20,0x20,0x00,
 0xf8,0x10,0x20,0x40,0xf8,0x00
};

/**********************************************************\
 * Rendering a text string, no clipping is performed !    *
\**********************************************************/

#if defined(_CI_)
void G_text(int x,int y,char *txt,
            HW_pixel colour,int intensity
	   )
#endif
#if defined(_RGB_)
void G_text(int x,int y,char *txt,
            HW_pixel colour,int red,int green,int blue
           )
#endif
{
 int nm,i,j;
 unsigned char cur;
 HW_pixel *adr,*l_adr;
#if defined(_CI_)
 register HW_pixel hwcolour=CL_light(colour,intensity);
#endif
#if defined(_RGB_)
 register HW_pixel hwcolour=CL_light(colour,red,green,blue);
#endif

 adr=G_c_buffer+G_page_start+HW_SCREEN_LINE_SIZE*y+x;

 while(*txt!='\0')                          /* until the end of the string */
 {
  nm=((*txt++)-' ');                        /* starting from space */
  if(nm>58) nm-=' ';                        /* to uppercase 58='Z'-' ' */
  nm*=6;                                    /* 6 bytes per char */
  for(i=0,l_adr=adr;i<6;i++,l_adr+=HW_SCREEN_LINE_SIZE)
  {
   cur=G_font[nm++];
   for(j=0;j<8;j++)
    if(cur&(0x80>>j)) *(l_adr+j)=hwcolour;
  }
  adr+=8;                                   /* next word */
 }
}

/**********************************************************/
