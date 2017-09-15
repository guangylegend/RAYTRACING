/** 3DGPL *************************************************\
 * ()                                                     *
 * Functions for the base of polygonal 3D engine.         *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model;                   *
 *  _Z_BUFFER_               Depth array;                 *
 *  _PAINTER_                Back to front order.         *
 *                                                        *
 * Defines:                                               *
 *  M_init_rendering         Set type of rendering;       *
 *  M_shade_vertex           Shading a single vertex;     *
 *  M_sort_elements          Polygons in _PAINTER_;       *
 *                                                        *
 *  M_shade_polygon          Shading using normals;       *
 *  M_render_polygon         Rendering in perspective;    *
 *                                                        *
 *  M_set_gate               Set gate's clipping bounds.  *
 *                                                        *
 * Internals:                                             *
 *  MI_construct_tuples      Gets X Y Z R G B Tx Ty;      *
 *  MI_tmapping_vectors      Computes U V for tmapping.   *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//EngBase.cpp

#include "Graphics.h"           /* 2-D rendering */
#include "Trans.h"                 /* 3-D transformations */
#include "Clipper.h"             /* 2-D/3-D clipping */
#include "Engine.h"               /* 3-D engine */
#include <stdlib.h>                         /* NULL */
#include <limits.h>                         /* INT_MIN and INT_MAX */

int M_force_linear_tmapping=0;              /* when the polygons are small */
int M_rendering_type;                       /* types that can be rendered */
unsigned char M_camera_gam;                 /* needed for surfaces */
int M_camera_x,M_camera_y,M_camera_z;
int M_camera_log_focus;                     /* camera parameters */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Constructing polygon list.                  *
 * ---------                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CI_construct_tuples(int *from,register int *to,
                         int *vertices,int *colours,
                         int *textures,
                         int dimension,int length,int *min,int *max
                        )
{
 register int i,index;

 HW_set_int(min,3,INT_MAX);                 /* X Y Z */
 HW_set_int(max,3,INT_MIN);                 /* initializing searching */

 for(i=0;i<length;i++)
 {
  HW_copy_int(vertices+((index=(*from++))*T_LNG_VECTOR),to,3);

  if(*to>max[0]) max[0]=*to;                /* determining polygon's extend */
  if(*to<min[0]) min[0]=*to;
  to++;
  if(*to>max[1]) max[1]=*to;
  if(*to<min[1]) min[1]=*to;
  to++;
  if(*to>max[2]) max[2]=*to;
  if(*to<min[2]) min[2]=*to;                /* by searching max/min */
  to++;

#if defined(_CI_)
  if(dimension>=M_LNG_SHADED)
  {
   if(colours==NULL)
   {
    *to++=(*(from++))<<M_P;                 /* store I */
   }
   else
   {
    *to++=colours[index*CL_LNG_COLOUR]<<M_P;
    from++;                                 /* skip empty space for I */
   }
   if(dimension==M_LNG_TEXTURED)
   {
    if(textures==NULL)
    {
     *to++=(*(from++))<<M_P;
     *to++=(*(from++))<<M_P;                 /* store Tx,Ty */
    }
    else
    {
     *to++=textures[index*2]<<M_P;
     *to++=textures[index*2+1]<<M_P;
     from+=2;
    }
   }
   else from+=2;                            /* Tx Ty */
  }
  else from+=3;                             /* I Tx Ty */
#endif
#if defined(_RGB_)
  if(dimension>=M_LNG_SHADED)
  {
   if(colours==NULL)
   {
    *to++=(*(from++))<<M_P;                 /* store R G B */
    *to++=(*(from++))<<M_P;
    *to++=(*(from++))<<M_P;
   }
   else
   {
    HW_copy_int(colours+(index*CL_LNG_COLOUR),to,3);
    *to++<<=M_P;
    *to++<<=M_P;
    *to++<<=M_P;
    from+=3;                                /* skip empty space for R G B */
   }
   if(dimension==M_LNG_TEXTURED)
   {
    if(textures==NULL)
    {
     *to++=(*(from++))<<M_P;
     *to++=(*(from++))<<M_P;                /* store Tx,Ty */
    }
    else
    {
     *to++=textures[index*2]<<M_P;
     *to++=textures[index*2+1]<<M_P;
     from+=2;
    }
   }
   else from+=2;                            /* Tx Ty */
  }
  else from+=5;                             /* R G B Tx Ty */
#endif
 }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Finding u,v texture orientation vectors.    *
 * ---------                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void MI_tmapping_vectors(int type,int *p1,int *p2,int *p3,
                         int log_texture_space_size,
                         int *u,int *v
                        )
{
 int a[T_MAX_MATRIX_SIZE][T_MAX_MATRIX_SIZE];
 int b[T_MAX_MATRIX_SIZE][T_MAX_MATRIX_SIZE];
 int x[T_MAX_MATRIX_SIZE][T_MAX_MATRIX_SIZE];

 if(type)                                   /* regular shape */
 {
  if(type&M_QUAD_XY)
  {
   T_vector(p1,p2,u,3);                     /* X Y Z */
   T_vector(p2,p3,v,3);                     /* direct order */
  }
  else
  {
   T_vector(p2,p1,u,3);                     /* X Y Z */
   T_vector(p3,p2,v,3);                     /* reversed order */
  }
 }
 else                                       /* non regular shape */
 {
  T_vector(p1+M_IDX_TEXTURE,p2+M_IDX_TEXTURE,a[0],2);
  T_vector(p2+M_IDX_TEXTURE,p3+M_IDX_TEXTURE,a[1],2);
  T_vector(p1,p2,b[0],3);                   /* X Y Z */
  T_vector(p2,p3,b[1],3);                   /* setting system of equations */

  T_linear_solve(a,b,x,2,3,M_P,log_texture_space_size);

  HW_copy_int(x[0],u,3);                    /* X Y Z */
  HW_copy_int(x[1],v,3);
 }
}

/**********************************************************\
 * Set rendering options.                                 *
 *                                                        *
 * SETS: M_rendering_type                                 *
 * -----                                                  *
\**********************************************************/

void M_init_rendering(int type)
{
 M_rendering_type=type;                     /* allowed rendering methods */
}

/**********************************************************\
 * Set the camera parameters, orientation and position    *
 * of the projection plane and perspective focus.         *
 *                                                        *
 * SETS: M_rendering_type                                 *
 * -----                                                  *
\**********************************************************/

void M_set_camera(unsigned char alp,unsigned char bet,unsigned char gam,
                  int x,int y,int z,int log_focus
                 )
{
 T_set_world_rotation((unsigned char)256-alp,(unsigned char)256-bet,
                      (unsigned char)256-gam
                     );
 M_camera_gam=(unsigned char)256-gam;
 M_camera_x=-x;
 M_camera_y=-y;
 M_camera_z=-z;
 M_camera_log_focus=log_focus;              /* other parameter of the camera */
}

/**********************************************************\
 * Shading a single vertex using a list of light sources  *
 * and determining contribution of each light source.     *
\**********************************************************/

void M_shade_vertex(int *intensity,
                    int *vertex,int *normal,
                    int no_lights, struct M_light **lights
                   )
{
 int j,light_vector[T_LNG_VECTOR],prd;
 struct M_light *light;

#if defined(_CI_)
 intensity[0]=0;                            /* intensity */
#endif
#if defined(_RGB_)
 intensity[0]=0;                            /* RGB */
 intensity[1]=0;
 intensity[2]=0;
#endif

 for(j=0;j<no_lights;j++)
 {
  light=lights[j];
  switch(light->m_type)
  {
   case M_AMBIENT:                          /* uniform illumination */
   {
#if defined(_CI_)
    intensity[0]+=light->m_intensity;
#endif
#if defined(_RGB_)
    intensity[0]+=light->m_red;
    intensity[1]+=light->m_green;
    intensity[2]+=light->m_blue;
#endif
   }
   break;

   case M_POINT:                            /* depends on the unit vector */
   {
    T_unit_vector(vertex,light->m_parameter,light_vector);
    prd=T_scalar_product(light_vector,normal);
    if(prd<0) break;

#if defined(_CI_)
    intensity[0]+=(prd*light->m_intensity)>>T_LOG_NORMAL_SIZE;
#endif
#if defined(_RGB_)
    intensity[0]+=(prd*light->m_red)>>T_LOG_NORMAL_SIZE;
    intensity[1]+=(prd*light->m_green)>>T_LOG_NORMAL_SIZE;
    intensity[2]+=(prd*light->m_blue)>>T_LOG_NORMAL_SIZE;
#endif
   }
   break;

   case M_DIRECT:                           /* depends on the normal */
   {
    prd=-T_scalar_product(light->m_parameter,normal);
    if(prd<0) break;

#if defined(_CI_)
    intensity[0]+=(prd*light->m_intensity)>>T_LOG_NORMAL_SIZE;
#endif
#if defined(_RGB_)
    intensity[0]+=(prd*light->m_red)>>T_LOG_NORMAL_SIZE;
    intensity[1]+=(prd*light->m_green)>>T_LOG_NORMAL_SIZE;
    intensity[2]+=(prd*light->m_blue)>>T_LOG_NORMAL_SIZE;
#endif
   }
   break;
  }
 }
}

/**********************************************************\
 * Sorting at the same time an array of vertices and an   *
 * array of indices.                                      *
\**********************************************************/

#if defined(_PAINTER_)
void M_sort_elements(int *vertices,int dimension,int *indices,int number)
{
 int i,j,tmp;

 for(i=number-1;i>0;i--)                    /* buble sorting indexes */
 {
  for(j=0;j<i;j++)
  {
   if(vertices[indices[j]*dimension]<vertices[indices[(j+1)]*dimension])
   {                                        /* render the ones further away */
    tmp=indices[j]; indices[j]=indices[j+1]; indices[j+1]=tmp;
   }                                        /* earlier */
  }
 }
}
#endif

/**********************************************************\
 * Shading a single polygon using a list of light sources *
 * for each vertex at the same time computing flat        *
 * illumination for the whole polygon to use for low      *
 * rendering options.                                     *
\**********************************************************/

void M_shade_polygon(struct M_polygon *poly,int *vertices,
                     int *normals, int no_lights,
                     struct M_light **lights
                    )
{
 int i,*vertex,*normal_idxs;

#if defined(_CI_)
 poly->m_intensity=0;
#endif
#if defined(_RGB_)
 poly->m_red=0;
 poly->m_green=0;
 poly->m_blue=0;                            /* init flat intensity */
#endif

 vertex=poly->m_vertices;
 normal_idxs=poly->m_normals;
 for(i=0;i<=poly->m_no_edges;i++,vertex+=M_LNG_POLYGON_VERTEX,normal_idxs++)
 {
  M_shade_vertex(vertex+1,vertices+vertex[0]*T_LNG_VECTOR,
                 normals+(*normal_idxs*T_LNG_VECTOR),
                 no_lights,lights
                );

#if defined(_CI_)
  poly->m_intensity+=vertex[1];
#endif
#if defined(_RGB_)
  poly->m_red+=vertex[1];
  poly->m_green+=vertex[2];
  poly->m_blue+=vertex[3];                  /* polygon's illumination */
#endif
 }

#if defined(_CI_)
 poly->m_intensity/=(poly->m_no_edges+1);
#endif
#if defined(_RGB_)
 poly->m_red/=(poly->m_no_edges+1);
 poly->m_green/=(poly->m_no_edges+1);       /* computing intensities for */
 poly->m_blue/=(poly->m_no_edges+1);        /* dirty rendering options */
#endif
}

/**********************************************************\
 * Rendering a generic polygon in perspective.            *
 * Firstly vertex tuples are being created, they are      *
 * of length M_LNG_WHATEVER, they are being pyramide      *
 * clipped and Z clipped, hence tuples are being sent     *
 * into perspective acquiring length of G_LNG_WHATEVER,   *
 * (Z might get discarded for _PAINTER_, that's why),     *
 * culling is being done, and then method dependent       *
 * (AMBIENT, SHADED) rendering are being done.            *
 * Specifically for perspective a function to get         *
 * mapping vectors would be invoked.                      *
\**********************************************************/

void M_render_polygon(struct M_polygon *poly,
                      int *vertices,int *colours,
                      int *textures
                     )
{
 int original1[M_MAX_POLYGON_VERTICES*C_MAX_DIMENSIONS];
 int original2[M_MAX_POLYGON_VERTICES*C_MAX_DIMENSIONS];
 int perspective[M_MAX_POLYGON_VERTICES*C_MAX_DIMENSIONS];
 int vector1[T_LNG_VECTOR],vector2[T_LNG_VECTOR];
 int *clipped,number,cnd;
 int i,min[3],max[3];                       /* extends X Y Z */

 switch(M_rendering_type)
 {
  case M_WIRE:                              /* X Y Z, a wireframe */
  {
   number=poly->m_no_edges;
   CI_construct_tuples((int*)poly->m_vertices,original1,
                       vertices,colours,textures,
                       M_LNG_FLAT,number+1,min,max
                      );
   if((cnd=C_volume_clipping(min,max))!=0)  /* using extends */
   {
    if(cnd==-1)
    {
     number=C_polygon_z_clipping(original1,clipped=original2,
                                 M_LNG_FLAT,number
                                );
    }
    else clipped=original1;                 /* source is of M_LNG_FLAT */
    T_perspective(clipped,perspective,M_LNG_FLAT,number+1,M_camera_log_focus);
                                            /* result is of G_LNG_FLAT */
    if(T_normal_z_negative(perspective,perspective+G_LNG_FLAT,
                                       perspective+G_LNG_FLAT*2
                          )
      )
    {
     for(i=0;i<number;i++)
     {
#if defined(_CI_)
      G_line(perspective+i*G_LNG_FLAT,perspective+(i+1)*G_LNG_FLAT,
             poly->m_colour,poly->m_intensity
            );
#endif
#if defined(_RGB_)
      G_line(perspective+i*G_LNG_FLAT,perspective+(i+1)*G_LNG_FLAT,
             poly->m_colour,poly->m_red,poly->m_green,poly->m_blue
            );
#endif
     }
    }
   }
  } break;

  case M_FLAT:                              /* X Y Z, flat shaded polygon */
  {
   number=poly->m_no_edges;
   CI_construct_tuples((int*)poly->m_vertices,original1,
                       vertices,colours,textures,
                       M_LNG_FLAT,number+1,min,max
                      );
   if((cnd=C_volume_clipping(min,max))!=0)  /* using extends */
   {
    if(cnd==-1)
    {
     number=C_polygon_z_clipping(original1,clipped=original2,
                                 M_LNG_FLAT,number
                                );
    }
    else clipped=original1;                 /* source is of M_LNG_FLAT */
    T_perspective(clipped,perspective,M_LNG_FLAT,number+1,M_camera_log_focus);
                                            /* result is of G_LNG_FLAT */
    if(T_normal_z_negative(perspective,perspective+G_LNG_FLAT,
                                       perspective+G_LNG_FLAT*2
                          )
      )
    {
#if defined(_CI_)
     G_flat_polygon(perspective,number,poly->m_colour,poly->m_intensity);
#endif
#if defined(_RGB_)
     G_flat_polygon(perspective,number,poly->m_colour,poly->m_red,
                       poly->m_green,poly->m_blue
                      );
#endif
    }
   }
  } break;

  case M_TEXTURED:                          /* X Y Z [I] or [R G B] Tx Ty */
  {                                         /* texture mapped polygon */
   if(poly->m_texture!=NULL)
   {
    number=poly->m_no_edges;
    CI_construct_tuples((int*)poly->m_vertices,original1,
                        vertices,colours,textures,
                        M_LNG_TEXTURED,number+1,min,max
                       );
    if((cnd=C_volume_clipping(min,max))!=0) /* using extends */
    {
     if(cnd==-1) number=C_polygon_z_clipping(original1,clipped=original2,
                                             M_LNG_TEXTURED,number
                                            );
     else clipped=original1;                /* source is of M_LNG_TEXTURED */

     T_perspective(clipped,perspective,M_LNG_TEXTURED,number+1,M_camera_log_focus);
                                            /* result is of G_LNG_TEXTURED */
     if(T_normal_z_negative(perspective,perspective+G_LNG_TEXTURED,
                                        perspective+G_LNG_TEXTURED*2
                           )
       )
     {
      if((M_force_linear_tmapping)||
         (clipped[2]<G_Z_MAPPING_SWITCH)    /* non-linear one */
        )
      {                                     /* original is of M_LNG_TEXTURED */
       MI_tmapping_vectors(poly->m_type&M_QUAD,
                           original1,
                           original1+M_LNG_TEXTURED,
                           original1+M_LNG_TEXTURED*2,
                           poly->m_log_texture_space_size,
                           vector1,vector2
                          );
       G_prp_textured_polygon(perspective,number,clipped,
                              vector1,vector2,
                              poly->m_texture->m_texture_bytes,
                              poly->m_texture->m_log_texture_size,
                              poly->m_log_texture_space_size
                             );
      }
      else                                  /* linear mapping */
      {
       G_lin_textured_polygon(perspective,number,
                              poly->m_texture->m_texture_bytes,
                              poly->m_texture->m_log_texture_size
                             );
      }
     }
    }
    break;                                  /* if no texture do shaded */
   }
  }

  case M_SHADED:                            /* X Y Z [I] or [R G B] */
  {                                         /* Gourand shaded polygons */
   number=poly->m_no_edges;
   CI_construct_tuples((int*)poly->m_vertices,original1,
                       vertices,colours,textures,
                       M_LNG_SHADED,number+1,min,max
                      );
   if((cnd=C_volume_clipping(min,max))!=0)  /* using extends */
   {
    if(cnd==-1)
    {
     number=C_polygon_z_clipping(original1,clipped=original2,
                                 M_LNG_SHADED,number
                                );
    }
    else clipped=original1;                 /* source is of M_LNG_SHADED */

    T_perspective(clipped,perspective,M_LNG_SHADED,number+1,M_camera_log_focus);
                                            /* result is of G_LNG_SHADED */
    if(T_normal_z_negative(perspective,perspective+G_LNG_SHADED,
                                       perspective+G_LNG_SHADED*2
                          )
      )
    {
     G_shaded_polygon(perspective,number,poly->m_colour);
    }
   }
  }
  break;
 }
}

/**********************************************************\
 * Setting clipping limits for the extend of a            *
 * perspective projection of a gate to the screen.        *
 *                                                        *
 * Firstly vertex tuples are created, they are pyramide   *
 * clipped and Z clipped, hence tuples are sent into      *
 * perspective, further the extends are found and the     *
 * clipping bounds set.                                   *
 *                                                        *
 * RETURNS: 1 on success;                                 *
 * -------- 0 when clipping limits can't be set.          *
\**********************************************************/

int M_set_gate(struct M_gate *gate,int *vertices)
{
 int original1[M_MAX_POLYGON_VERTICES*C_MAX_DIMENSIONS];
 int original2[M_MAX_POLYGON_VERTICES*C_MAX_DIMENSIONS];
 int perspective[M_MAX_POLYGON_VERTICES*C_MAX_DIMENSIONS];
 int *tmp,*clipped,number,cnd;
 int min[3],max[3];                         /* extends X Y Z */
 int nminx,nminy,nmaxx,nmaxy;               /* new clipping bounds */
 int i,*to;

 number=gate->m_no_edges;

 HW_set_int(min,3,INT_MAX);                 /* X Y Z */
 HW_set_int(max,3,INT_MIN);                 /* initializing searching */

 for(to=original1,i=0;i<=number;i++)
 {
  HW_copy_int(vertices+(gate->m_vertices[i]*T_LNG_VECTOR),to,3);

  if(*to>max[0]) max[0]=*to;                /* determining polygon's extend */
  if(*to<min[0]) min[0]=*to;
  to++;
  if(*to>max[1]) max[1]=*to;
  if(*to<min[1]) min[1]=*to;
  to++;
  if(*to>max[2]) max[2]=*to;
  if(*to<min[2]) min[2]=*to;                /* by searching max/min */
  to++;
 }

 if((cnd=C_volume_clipping(min,max))!=0)    /* using extends */
 {
  if(cnd==-1)
  {
   number=C_polygon_z_clipping(original1,clipped=original2,M_LNG_FLAT,number);
  }
  else clipped=original1;                   /* source is of length 3 */

  T_perspective(clipped,perspective,M_LNG_FLAT,number+1,M_camera_log_focus);
                                            /* result is of G_LNG_FLAT */
  if(!T_normal_z_negative(perspective,perspective+G_LNG_FLAT,
                                      perspective+G_LNG_FLAT*2)
                         )
   return(0);                               /* gate is back culled */

  nminx=nminy=INT_MAX;
  nmaxx=nmaxy=INT_MIN;                      /* initializing searching */
  for(tmp=perspective,i=0;i<number;i++)
  {
   if(*tmp>nmaxx) nmaxx=*tmp;               /* determining gate extend */
   if(*tmp<nminx) nminx=*tmp;               /* X */
   tmp++;
   if(*tmp>nmaxy) nmaxy=*tmp;               /* Y */
   if(*tmp<nminy) nminy=*tmp;
   tmp++;
#if defined(_Z_BUFFER_)
   tmp++;                                   /* Z if present */
#endif
  }
  return(C_set_bounds(nminx,nminy,nmaxx,nmaxy));
 }
 else return(0);                            /* gate is behind the viewer */
}

/**********************************************************/
