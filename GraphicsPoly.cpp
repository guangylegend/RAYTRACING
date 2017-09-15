/** 3DGPL *************************************************\
 * ()                                                     *
 * 2D graphics and 2D clipping extentions for shaded      *
 * and textured polygons (has to be linked with regular   *
 * routines).                                             *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model;                   *
 *  _Z_BUFFER_               Depth array;                 *
 *  _PAINTER_                Back front order.            *
 *                                                        *
 * Defines:                                               *
 *  G_flat_polygon           Regular polygon;             *
 *  G_shaded_polygon         Shaded polygon;              *
 *  G_lin_textured_polygon   Textured polygon (approx);   *
 *  G_prp_textured_polygon   Textured polygon (true).     *
 *                                                        *
 * Internals:                                             *
 *  GI_scan                  Scanning an edge;            *
 *  GI_boarder_array_init    Init left/right boundaries.  *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//GraphicsPoly.cpp

#include "LightTrack.h"           /* hardware specific stuff */
#include "Colour.h"               /* colour and light */
#include "Clipper.h"             /* 2D clipping */
#include "Graphics.h"           /* 2D macros */
#include "Engine.h"               /* M_LNG_SHADED */
#include <limits.h>                         /* INT_MAX and INT_MIN */

extern HW_pixel *G_c_buffer;                /* the bitmap's bits */
extern int G_page_start;                    /* always 0 for _MONO_ */
#if defined(_Z_BUFFER_)
extern int *G_z_buffer;                     /* Z buffer */
#endif

#define G_LINEAR 32                         /* interpolate for */

int G_miny,G_maxy;                          /* vertical boundaries */

int G_x_start[HW_SCREEN_Y_SIZE];            /* polygon's */
int G_x_end[HW_SCREEN_Y_SIZE];              /* horizontal boundaries */
HW_32_bit G_0_start[HW_SCREEN_Y_SIZE];      /* [32-G_P].[G_P] values */
HW_32_bit G_0_end[HW_SCREEN_Y_SIZE];        /* the thingie is to work faster */
HW_32_bit G_1_start[HW_SCREEN_Y_SIZE];      /* then multidimensional array */
HW_32_bit G_1_end[HW_SCREEN_Y_SIZE];        /* hope so, */
HW_32_bit G_2_start[HW_SCREEN_Y_SIZE];
HW_32_bit G_2_end[HW_SCREEN_Y_SIZE];        /* space for interpolating */
HW_32_bit G_3_start[HW_SCREEN_Y_SIZE];      /* Z R G B Tx Ty */
HW_32_bit G_3_end[HW_SCREEN_Y_SIZE];
HW_32_bit G_4_start[HW_SCREEN_Y_SIZE];
HW_32_bit G_4_end[HW_SCREEN_Y_SIZE];
HW_32_bit G_5_start[HW_SCREEN_Y_SIZE];
HW_32_bit G_5_end[HW_SCREEN_Y_SIZE];

HW_32_bit *G_start[C_MAX_DIMENSIONS]=
{G_0_start,G_1_start,G_2_start,G_3_start,G_4_start,G_5_start};
HW_32_bit *G_end[C_MAX_DIMENSIONS]=
{G_0_end,G_1_end,G_2_end,G_3_end,G_4_end,G_5_end};

#if defined(_CI_)
 #if defined(_Z_BUFFER_)                    /* Z I Tx Ty */
  #define G_Z_INDX_START  G_0_start         /* indexed colour, z-buffer */
  #define G_I_INDX_START  G_1_start
  #define G_TX_INDX_START G_2_start
  #define G_TY_INDX_START G_3_start

  #define G_Z_INDX_END  G_0_end             /* indexed colour, z-buffer */
  #define G_I_INDX_END  G_1_end
  #define G_TX_INDX_END G_2_end
  #define G_TY_INDX_END G_3_end
 #endif
 #if defined(_PAINTER_)                     /* I Tx Ty */
  #define G_I_INDX_START  G_0_start         /* indexed colour, painter */
  #define G_TX_INDX_START G_1_start
  #define G_TY_INDX_START G_2_start

  #define G_I_INDX_END  G_0_end             /* indexed colour, painter */
  #define G_TX_INDX_END G_1_end
  #define G_TY_INDX_END G_2_end
 #endif
#endif
#if defined(_RGB_)
 #if defined(_Z_BUFFER_)                    /* Z R G B Tx Ty */
  #define G_Z_INDX_START  G_0_start         /* RGB colour, z-buffer */
  #define G_R_INDX_START  G_1_start
  #define G_G_INDX_START  G_2_start
  #define G_B_INDX_START  G_3_start
  #define G_TX_INDX_START G_4_start
  #define G_TY_INDX_START G_5_start

  #define G_Z_INDX_END  G_0_end             /* RGB colour, z-buffer */
  #define G_R_INDX_END  G_1_end
  #define G_G_INDX_END  G_2_end
  #define G_B_INDX_END  G_3_end
  #define G_TX_INDX_END G_4_end
  #define G_TY_INDX_END G_5_end
 #endif
 #if defined(_PAINTER_)                     /* R G B Tx Ty */
  #define G_R_INDX_START  G_0_start         /* RGB colou, painter */
  #define G_G_INDX_START  G_1_start
  #define G_B_INDX_START  G_2_start
  #define G_TX_INDX_START G_3_start
  #define G_TY_INDX_START G_4_start

  #define G_R_INDX_END  G_0_end             /* RGB colou, painter */
  #define G_G_INDX_END  G_1_end
  #define G_B_INDX_END  G_2_end
  #define G_TX_INDX_END G_3_end
  #define G_TY_INDX_END G_4_end
 #endif
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Scan converting a N dimensional line.       *
 * ---------                                             *
 * SETS: G_x_start,G_x_end,G_start,G_end,G_miny,G_maxy   *
 * -----                                                 *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void GI_scan(int *edges,int dimension,int skip)
{
 HW_32_bit cur_v[C_MAX_DIMENSIONS];         /* initial values for Z+ dims */
 HW_32_bit inc_v[C_MAX_DIMENSIONS];         /* increment for Z+ dimensions */
 int dx,dy,long_d,short_d;
 register int d,add_dh,add_dl;
 register int inc_xh,inc_yh,inc_xl,inc_yl;
 int x,y,i,j;
 int *v1,*v2;                               /* first and second vertices */

 v1=edges; edges+=skip; v2=edges;           /* length ints in each */

 if(C_line_y_clipping(&v1,&v2,dimension))   /* vertical clipping */
 {
  dx=*v2++; dy=*v2++;                       /* extracting 2-D coordinates */
  x=*v1++; y=*v1++;                         /* v2/v1 point remaining dim-2 */
  dimension-=2;

  if(y<G_miny) G_miny=y;
  if(y>G_maxy) G_maxy=y;
  if(dy<G_miny) G_miny=dy;
  if(dy>G_maxy) G_maxy=dy;                  /* updating vertical size */

  dx-=x; dy-=y;                             /* ranges */

  if(dx<0){dx=-dx; inc_xh=-1; inc_xl=-1;}   /* making sure dx and dy >0 */
  else    {        inc_xh=1;  inc_xl=1; }   /* adjusting increments */
  if(dy<0){dy=-dy; inc_yh=-1; inc_yl=-1;}
  else    {        inc_yh=1;  inc_yl=1; }

  if(dx>dy){long_d=dx;short_d=dy;inc_yl=0;} /* long range,&make sure either */
  else     {long_d=dy;short_d=dx;inc_xl=0;} /* x or y is changed in L case */

  d=2*short_d-long_d;                       /* initial value of d */
  add_dl=2*short_d;                         /* d adjustment for H case */
  add_dh=2*(short_d-long_d);                /* d adjustment for L case */

#if defined(_Z_BUFFER_)
  cur_v[0]=((HW_32_bit)v1[0])<<G_P;         /* Z */
  if(long_d>0)
   inc_v[0]=(((HW_32_bit)(v2[0]-v1[0]))<<G_P)/long_d;
  i=1;                                      /* the rest */
#endif
#if defined(_PAINTER_)
  i=0;                                      /* all */
#endif

  for(;i<dimension;i++)                     /* for all remaining dimensions */
  {
   cur_v[i]=((HW_32_bit)v1[i]);
   if(long_d>0)
    inc_v[i]=((HW_32_bit)(v2[i]-v1[i]))/long_d;
  }

  for(i=0;i<=long_d;i++)                    /* for all points in long range */
  {
   if(x<G_x_start[y])                       /* further than rightmost */
   {
    G_x_start[y]=x;                         /* the begining of scan line */
    for(j=0;j<dimension;j++)
     G_start[j][y]=cur_v[j];                /* all other dimensions */
   }

   if(G_x_end[y]<x)                         /* further the leftmost */
   {
    G_x_end[y]=x;                           /* the end of scan line */
    for(j=0;j<dimension;j++)
     G_end[j][y]=cur_v[j];                  /* and for other dimension */
   }

   if(d>=0){x+=inc_xh;y+=inc_yh;d+=add_dh;} /* previous point was H type */
   else    {x+=inc_xl;y+=inc_yl;d+=add_dl;} /* previous point was L type */
   for(j=0;j<dimension;j++)
    cur_v[j]+=inc_v[j];                     /* for all other dimensions */
  }
 }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Initialization of polygon boundaries.       *
 * ---------                                             *
 * SETS: G_miny,G_maxy,G_x_start,G_x_end                 *
 * -----                                                 *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void GI_boarder_array_init(void)
{
 G_miny=INT_MAX;                            /* polygon starts here */
 G_maxy=INT_MIN;                            /* polygon ends here */

 HW_set_int(G_x_start,HW_SCREEN_Y_SIZE,INT_MAX);
 HW_set_int(G_x_end,HW_SCREEN_Y_SIZE,INT_MIN);
}

/**********************************************************\
 * Rendering a polygon.                                   *
 * Accepts a stream of two-tuples X Y (_PAINTER_) or      *
 * three-tuples X Y Z (_Z_BUFFER_).                       *
\**********************************************************/

#if defined(_CI_)
void G_flat_polygon(int *edges,int length,
                       HW_pixel colour,int intensity
                      )
#endif
#if defined(_RGB_)
void G_flat_polygon(int *edges,int length,
                       HW_pixel colour,int red,int green,int blue
                      )
#endif
{
 int new_edges[G_MAX_POLYGON_VERTICES*G_LNG_FLAT];
 int new_length,i;                          /* although no edges there yet */
 long pos;
 register HW_pixel *adr_c;                  /* position in the Colour buffer */
 register int beg,end,span;
#if defined(_CI_)
 HW_pixel hwcolour=CL_light(colour,intensity);
#endif
#if defined(_RGB_)
 HW_pixel hwcolour=CL_light(colour,red,green,blue);
#endif
#if defined(_Z_BUFFER_)
 int *adr_z;                                /* position in the Z buffer */
 register HW_32_bit cur_z,inc_z;            /* current deapth */
#endif

 GI_boarder_array_init();                   /* initializing the arrays */

 new_length=C_polygon_x_clipping(edges,new_edges,G_LNG_FLAT,length);

 for(i=0;i<new_length;i++)                  /* Searching polygon boarders */
  GI_scan(&new_edges[i*G_LNG_FLAT],G_LNG_FLAT,G_LNG_FLAT);

 if(G_miny<G_maxy)                          /* nothing to do? */
 {
  pos=G_miny*HW_SCREEN_LINE_SIZE+G_page_start;

  for(;G_miny<G_maxy;G_miny++,pos+=HW_SCREEN_LINE_SIZE)
  {                                         /* rendering all lines */
   adr_c=G_c_buffer+pos+(beg=G_x_start[G_miny]);
#if defined(_Z_BUFFER_)
   adr_z=G_z_buffer+pos+beg;                /* corresponding position in Z buffer */
#endif
   end=G_x_end[G_miny];                     /* ends here */
   span=end-beg;

#if defined(_Z_BUFFER_)
   cur_z=G_Z_INDX_START[G_miny];
   if(span!=0)
    inc_z=(G_Z_INDX_END[G_miny]-cur_z)/span;

   for(;beg<=end;beg++,adr_c++,adr_z++)     /* render this lines */
   {
    if(*adr_z>(cur_z>>G_P))                 /* Z buffer check */
    {
     *adr_z=cur_z>>G_P;                     /* store new deapth here */
     *adr_c=hwcolour;                       /* flat colour here */
    }
    cur_z+=inc_z;                           /* next value for Z */
   }
#endif
#if defined(_PAINTER_)
   HW_set_pixel(adr_c,span+1,hwcolour);     /* drawing a strip */
#endif
  }
 }
}

/**********************************************************\
 * Rendering an Interpolatively shaded polygon.           *
 * Accepts a stream of three-tuples ( X Y Intensity )     *
 * for _CI_ and 5-tuples ( X Y R G B ) for _RGB_          *
 * similarely it is ( X Y Z etc. ) if _Z_BUFFER_          *
\**********************************************************/

void G_shaded_polygon(int *edges,int length,HW_pixel colour)
{
 int new_edges[G_MAX_POLYGON_VERTICES*G_LNG_SHADED];
 int new_length,i;
 long pos;
 register HW_pixel *adr_c;
 register int beg,end,span;
#if defined(_CI_)
 register HW_32_bit cur_i,inc_i;            /* current colour */
#endif
#if defined(_RGB_)
 register HW_32_bit cur_r,inc_r,cur_g,inc_g,cur_b,inc_b;
#endif
#if defined(_Z_BUFFER_)
 int *adr_z;                                /* position in the Z buffer */
 register HW_32_bit cur_z,inc_z;            /* current depth */
#endif

 GI_boarder_array_init();                   /* initializing the array */

 new_length=C_polygon_x_clipping(edges,new_edges,G_LNG_SHADED,length);

 for(i=0;i<new_length;i++)                  /* Searching polygon boarders */
  GI_scan(&new_edges[i*G_LNG_SHADED],G_LNG_SHADED,G_LNG_SHADED);

 if(G_miny<G_maxy)                          /* nothing to do? */
 {
  pos=G_miny*HW_SCREEN_LINE_SIZE+G_page_start;

  for(;G_miny<G_maxy;G_miny++,pos+=HW_SCREEN_LINE_SIZE)
  {                                         /* rendering all lines */
   adr_c=G_c_buffer+pos+(beg=G_x_start[G_miny]);
#if defined(_Z_BUFFER_)
   adr_z=G_z_buffer+pos+beg;                /* corresponding place in Z buffer */
#endif
   end=G_x_end[G_miny];                     /* ends here */
   span=end-beg;
   if(span==0) span=1;                      /* not to divide by 0 */

#if defined(_CI_)
   cur_i=G_I_INDX_START[G_miny];
   inc_i=(G_I_INDX_END[G_miny]-cur_i)/span;
#endif
#if defined(_RGB_)
   cur_r=G_R_INDX_START[G_miny];
   inc_r=(G_R_INDX_END[G_miny]-cur_r)/span;
   cur_g=G_G_INDX_START[G_miny];
   inc_g=(G_G_INDX_END[G_miny]-cur_g)/span;
   cur_b=G_B_INDX_START[G_miny];
   inc_b=(G_B_INDX_END[G_miny]-cur_b)/span;
#endif
#if defined(_Z_BUFFER_)
   cur_z=G_Z_INDX_START[G_miny];
   inc_z=(G_Z_INDX_END[G_miny]-cur_z)/span;

   for(;beg<=end;beg++,adr_c++,adr_z++)     /* render this lines */
   {
    if(*adr_z>(cur_z>>G_P))                 /* Z buffer check */
    {
     *adr_z=cur_z>>G_P;                     /* store new deapth here */
#endif
#if defined(_PAINTER_)
   for(;beg<=end;beg++,adr_c++)             /* same, no Z check */
   {
    {
#endif
#if defined(_CI_)
     *adr_c=CL_light(colour,cur_i>>G_P);    /* rendering single point */
    }
    cur_i+=inc_i;                           /* incrementing colour */
#endif
#if defined(_RGB_)
     *adr_c=CL_light(colour,cur_r>>G_P,cur_g>>G_P,cur_b>>G_P);
    }
    cur_r+=inc_r;
    cur_g+=inc_g;
    cur_b+=inc_b;                           /* increment RGB */
#endif
#if defined(_Z_BUFFER_)
    cur_z+=inc_z;                           /* increment Z */
#endif
   }
  }
 }
}

/**********************************************************\
 * Rendering a leanerely textured polygon.                *
 * The applied texture is a square colourmap with:        *
 *                                                        *
 * log_texture_size=log texture_size                      *
 *                     2                                  *
 *                                                        *
 * Accepts a stream of 5-tuples ( X Y Intensity Tx Ty)    *
 * for _CI_ and 5-tuples ( X Y R G B Tx Ty) for _RGB_     *
 * similarely it is one more ( X Y Z etc. ) if _Z_BUFFER_ *
\**********************************************************/

void G_lin_textured_polygon(int *edges,int length,
                            HW_pixel *texture,
                            int log_texture_size
                           )
{
 int new_edges[G_MAX_POLYGON_VERTICES*G_LNG_TEXTURED];
 int new_length,i;                          /* although no edges there yet */
 long pos;
 register HW_pixel *adr_c;
 register int beg,end,span;
 register HW_32_bit cur_tx,inc_tx;          /* current position inside */
 register HW_32_bit cur_ty,inc_ty;          /* the texture */
#if defined(_CI_)
 register HW_32_bit cur_i,inc_i;            /* current colour and it's inc */
#endif
#if defined(_RGB_)
 register HW_32_bit cur_r,inc_r,cur_g,inc_g,cur_b,inc_b;
#endif
#if defined(_Z_BUFFER_)
 int *adr_z;                                /* position in the Z buffer */
 register HW_32_bit cur_z,inc_z;            /* current deapth */
#endif
 HW_32_bit txtrmasc=(0x1<<(log_texture_size+G_P))-0x1;

 GI_boarder_array_init();                   /* initializing the array */

 new_length=C_polygon_x_clipping(edges,new_edges,G_LNG_TEXTURED,length);

 for(i=0;i<new_length;i++)
  GI_scan(&new_edges[i*G_LNG_TEXTURED],G_LNG_TEXTURED,G_LNG_TEXTURED);

 if(G_miny<G_maxy)                          /* nothing to do? */
 {
  pos=G_miny*HW_SCREEN_LINE_SIZE+G_page_start;

  for(;G_miny<G_maxy;G_miny++,pos+=HW_SCREEN_LINE_SIZE)
  {                                         /* rendering all lines */
   adr_c=G_c_buffer+pos+(beg=G_x_start[G_miny]);
#if defined(_Z_BUFFER_)
   adr_z=G_z_buffer+pos+beg;                /* corresponding place in Z buffer */
#endif
   end=G_x_end[G_miny];                     /* ends here */
   span=end-beg;
   if(span==0) span=1;                      /* not to divide by 0 */

   cur_tx=G_TX_INDX_START[G_miny];
   inc_tx=(G_TX_INDX_END[G_miny]-cur_tx)/span;
   cur_ty=G_TY_INDX_START[G_miny];
   inc_ty=(G_TY_INDX_END[G_miny]-cur_ty)/span;
#if defined(_CI_)
   cur_i=G_I_INDX_START[G_miny];
   inc_i=(G_I_INDX_END[G_miny]-cur_i)/span;
#endif
#if defined(_RGB_)
   cur_r=G_R_INDX_START[G_miny];
   inc_r=(G_R_INDX_END[G_miny]-cur_r)/span;
   cur_g=G_G_INDX_START[G_miny];
   inc_g=(G_G_INDX_END[G_miny]-cur_g)/span;
   cur_b=G_B_INDX_START[G_miny];
   inc_b=(G_B_INDX_END[G_miny]-cur_b)/span;
#endif
#if defined(_Z_BUFFER_)
   cur_z=G_Z_INDX_START[G_miny];
   inc_z=(G_Z_INDX_END[G_miny]-cur_z)/span;

   for(;beg<=end;beg++,adr_c++,adr_z++)
   {
    cur_tx&=txtrmasc; cur_ty&=txtrmasc;

    if(*adr_z>(cur_z>>G_P))                 /* Z buffer check */
    {
     *adr_z=cur_z>>G_P;
#endif
#if defined(_PAINTER_)
   for(;beg<=end;beg++,adr_c++)             /* render all lines: */
   {
    cur_tx&=txtrmasc;
    cur_ty&=txtrmasc;                       /* wrap around */
    {
#endif
#if defined(_CI_)
     *adr_c=CL_light(*(texture+((cur_ty>>G_P)<<log_texture_size)+(cur_tx>>G_P)),
                      cur_i>>G_P
                     );
     cur_i+=inc_i;
    }
#endif
#if defined(_RGB_)
     *adr_c=CL_light(*(texture+((cur_ty>>G_P)<<log_texture_size)+(cur_tx>>G_P)),
                      cur_r>>G_P,cur_g>>G_P,cur_b>>G_P
                     );
     cur_r+=inc_r;
     cur_g+=inc_g;
     cur_b+=inc_b;
    }
#endif
    cur_tx+=inc_tx;
    cur_ty+=inc_ty;                         /* new position inside texture */
#if defined(_Z_BUFFER_)
    cur_z+=inc_z;
#endif
   }
  }
 }
}

/**********************************************************\
 * Rendering a perspectively textured polygon.            *
 * The applied texture is a square colourmap with:        *
 *                                                        *
 * log_texture_size=log texture_size                      *
 *                     2                                  *
 *                                                        *
 * Accepts a stream of 5-tuples ( X Y Intensity Tx Ty)    *
 * for _CI_ and 5-tuples ( X Y R G B Tx Ty) for _RGB_     *
 * similarely it is one more ( X Y Z etc. ) if _Z_BUFFER_ *
 *                                                        *
 * log_texture_scale allowing scaling the texture, i.e.   *
 * mapping a larger texture onto a smaller polygon or     *
 * smaller texture onto a larger polygon.                 *
\**********************************************************/

void G_prp_textured_polygon(int *edges,int length,
                            int *O,int *u,int *v,
                            HW_pixel *texture,
                            int log_texture_size,
                            int log_texture_scale
                           )
{
 int new_edges[G_MAX_POLYGON_VERTICES*G_LNG_TEXTURED];
 int new_length,i;
 HW_32_bit Vx,Vy,Vz;
 HW_32_bit Ux,Uy,Uz;                        /* extracting vectors */
 HW_32_bit x0,y0,z0;
 HW_32_bit ui,uj,uc;
 HW_32_bit vi,vj,vc;
 HW_32_bit zi,zj,zc;                        /* back to texture coeficients */
 HW_32_bit v0,u0;
 HW_32_bit xx,yy,zz,zzz;
 long pos;
 HW_pixel *adr_c;
 register int beg,end,span;
 HW_32_bit cur_tx,inc_tx,end_tx,cur_ty,inc_ty,end_ty;
#if defined(_CI_)
 register HW_32_bit cur_i,inc_i;            /* current colour and it's inc */
#endif
#if defined(_RGB_)
 register HW_32_bit cur_r,inc_r,cur_g,inc_g,cur_b,inc_b;
#endif
#if defined(_Z_BUFFER_)
 int *adr_z;                                /* position in the Z buffer */
 register HW_32_bit cur_z,inc_z;            /* current deapth */
#endif
 register int x,y;
 HW_32_bit txtrmasc=(0x1<<(log_texture_size+G_P))-0x1;

 GI_boarder_array_init();                   /* initializing the array */

 new_length=C_polygon_x_clipping(edges,new_edges,G_LNG_TEXTURED,length);

 for(i=0;i<new_length;i++)
  GI_scan(&new_edges[i*G_LNG_TEXTURED],G_LNG_SHADED,G_LNG_TEXTURED);

 if(G_miny<G_maxy)                          /* nothing to do? */
 {
  x0=O[0]; y0=O[1]; z0=O[2];                /* X Y Z */
  u0=O[M_LNG_SHADED];
  v0=O[M_LNG_SHADED+1];                     /* world point <-> texture point */

  Vx=v[0]; Vy=v[1]; Vz=v[2];
  Ux=u[0]; Uy=u[1]; Uz=u[2];                /* extracting vectors */

  ui=(Vz*y0)-(Vy*z0);
  uj=(Vx*z0)-(Vz*x0);
  uc=(Vy*x0)-(Vx*y0);
  vi=(Uy*z0)-(Uz*y0);
  vj=(Uz*x0)-(Ux*z0);
  vc=(Ux*y0)-(Uy*x0);
  zi=(Vy*Uz)-(Vz*Uy);
  zj=(Vz*Ux)-(Vx*Uz);
  zc=(Vx*Uy)-(Vy*Ux);                       /* back to texture coefs */

  pos=G_miny*HW_SCREEN_LINE_SIZE+G_page_start;

  for(;G_miny<G_maxy;G_miny++,pos+=HW_SCREEN_LINE_SIZE)
  {                                         /* rendering all lines */
   adr_c=G_c_buffer+pos+(beg=G_x_start[G_miny]);
#if defined(_Z_BUFFER_)
   adr_z=G_z_buffer+pos+beg;                /* corresponding place in Z buffer */
#endif
   end=G_x_end[G_miny];                     /* ends here */
   span=end-beg;
   if(span==0) span=1;                      /* not to divide by 0 */

   beg-=HW_SCREEN_X_CENTRE;
   x=beg;
   end-=HW_SCREEN_X_CENTRE;
   y=G_miny-HW_SCREEN_Y_CENTRE;             /* pure perspective space */

#if defined(_CI_)
   cur_i=G_I_INDX_START[G_miny];
   inc_i=(G_I_INDX_END[G_miny]-cur_i)/span;
#endif
#if defined(_RGB_)
   cur_r=G_R_INDX_START[G_miny];
   inc_r=(G_R_INDX_END[G_miny]-cur_r)/span;
   cur_g=G_G_INDX_START[G_miny];
   inc_g=(G_G_INDX_END[G_miny]-cur_g)/span;
   cur_b=G_B_INDX_START[G_miny];
   inc_b=(G_B_INDX_END[G_miny]-cur_b)/span;
#endif
#if defined(_Z_BUFFER_)
   cur_z=G_Z_INDX_START[G_miny];
   inc_z=(G_Z_INDX_END[G_miny]-cur_z)/span;
#endif

   xx=((y*uj)>>M_camera_log_focus)+uc;
   yy=((y*vj)>>M_camera_log_focus)+vc;
   zz=((y*zj)>>M_camera_log_focus)+zc;      /* valid for the whole run */

   if((zzz=zz+((x*zi)>>M_camera_log_focus))!=0)
   {
    end_tx=((((xx+((x*ui)>>M_camera_log_focus))<<log_texture_scale)/zzz)<<G_P)+u0;
    end_ty=((((yy+((x*vi)>>M_camera_log_focus))<<log_texture_scale)/zzz)<<G_P)+v0;
   } else { end_tx=end_ty=0; }              /* not important actually */

   for(;beg<=end;)
   {
    x+=G_LINEAR; if(x>end) x=end;           /* size of linear run */
    cur_tx=end_tx;
    cur_ty=end_ty;

    if((zzz=zz+((x*zi)>>M_camera_log_focus))!=0)
    {
     end_tx=((((xx+((x*ui)>>M_camera_log_focus))<<log_texture_scale)/zzz)<<G_P)+u0;
     end_ty=((((yy+((x*vi)>>M_camera_log_focus))<<log_texture_scale)/zzz)<<G_P)+v0;
    } else { end_tx=end_ty=0; }             /* not important to what */

    if((span=x-beg)!=0)                     /* ends here */
    {
     inc_tx=(end_tx-cur_tx)/span;
     inc_ty=(end_ty-cur_ty)/span;
    } else { inc_tx=inc_ty=0; }             /* not important to what */

#if defined(_Z_BUFFER_)
    for(;beg<=x;beg++,adr_c++,adr_z++)
    {
     cur_tx&=txtrmasc;
     cur_ty&=txtrmasc;

     if(*adr_z>(cur_z>>G_P))                /* Z buffer check */
     {
      *adr_z=cur_z>>G_P;
#endif
#if defined(_PAINTER_)
    for(;beg<=x;beg++,adr_c++)              /* linear run */
    {
     cur_tx&=txtrmasc;
     cur_ty&=txtrmasc;                      /* wrap around */
     {
#endif
#if defined(_CI_)
      *adr_c=CL_light(*(texture+((cur_ty>>G_P)<<log_texture_size)+(cur_tx>>G_P)),
                       cur_i>>G_P
                      );
      cur_i+=inc_i;
     }
#endif
#if defined(_RGB_)
      *adr_c=CL_light(*(texture+((cur_ty>>G_P)<<log_texture_size)+(cur_tx>>G_P)),
                       cur_r>>G_P,cur_g>>G_P,cur_b>>G_P
                      );
      cur_r+=inc_r;
      cur_g+=inc_g;
      cur_b+=inc_b;
     }
#endif
     cur_tx+=inc_tx;
     cur_ty+=inc_ty;                        /* new position in the texture */
#if defined(_Z_BUFFER_)
     cur_z+=inc_z;
#endif
    }
   }
  }
 }
}

/**********************************************************/
