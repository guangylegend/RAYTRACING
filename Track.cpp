/** 3DGPL *************************************************\
 * (RGB hardware)                                         *
 * Simple 3D ray tracing engine.                          *
 *                                                        *
 * Defines:                                               *
 *  TR_sphere_init           Actualy, does nothing;       *
 *  TR_sphere_intersect      Closest of two;              *
 *  TR_sphere_normal         From a point;                *
 *  TR_polygon_init          Compute the plane equation;  *
 *  TR_polygon_intersect     t of the intersection;       *
 *  TR_polygon_normal        Always constant;             *
 *  TR_init_world            Init all objects;            *
 *  TR_trace_world           Building an image.           *
 *                                                        *
 * Internals:                                             *
 *  TRI_make_ray_point       Construct from two points;   *
 *  TRI_make_ray_vector      From a point and a vector;   *
 *  TRI_on_ray               Point from ray coef;         *
 *  TRI_illuminate           Local illumination;          *
 *  TRI_shadow_ray           If a lightsource is shadowed;*
 *  TRI_direct_ray           Computes one pixel's colour. *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/

#include "Graphics.h"           /* G_pixel */
#include "Colour.h"               /* colour related */
#include "Vector.h"               /* linear algebra related */
#include "Track.h"                 /* self definition */
#include <math.h>                           /* sqrt */
#include <stdlib.h>                         /* malloc */

int TR_rendering_type;                      /* rendering options */
float TR_viewer[V_LNG_VECTOR];              /* position of the viewer */
float TR_screen[V_LNG_VECTOR];              /* origine of the screen */
float TR_screen_u[V_LNG_VECTOR];            /* screen orientation vectors */
float TR_screen_v[V_LNG_VECTOR];

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Constructing a ray from two points.                   *
 *                                                       *
 * RETURNS: Constructed ray.                             *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct TR_ray *TRI_make_ray_point(struct TR_ray *r,float *from,
                                                   float *to
                                 )
{
 //ÉèÖÃ¹âÏßÊ¸Á¿µÄÆðÊ¼µã
 V_set(r->tr_start,from);
 //¼ÆËã¹âÏßµÄÊ¸Á¿·½Ïò
 V_difference(r->tr_codirected,to,from);
 return(r);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Constructing a ray from a point and a vector.         *
 *                                                       *
 * RETURNS: Constructed ray.                             *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct TR_ray *TRI_make_ray_vector(struct TR_ray *r,float *from,
                                                    float *vector
                                  )
{
 V_set(r->tr_start,from);
 V_set(r->tr_codirected,vector);
 return(r);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Returns point at distance t from the origine.         *
 *                                                       *
 * RETURNS: Constructed vertex.                          *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

float *TRI_on_ray(float *point,struct TR_ray *r,float t)
{
 point[0]=r->tr_start[0]+r->tr_codirected[0]*t;
 point[1]=r->tr_start[1]+r->tr_codirected[1]*t;
 point[2]=r->tr_start[2]+r->tr_codirected[2]*t;
 return(point);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Computing illumination of a intersected surface point.*
 *                                                       *
 * RETURNS: An RGB triple.                               *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

float *TRI_illuminate(float *light,struct TR_point_light *l,
                                   struct TR_matter *material,
                                   float *normal,
                                   float *where,
                                   float *viewer
                    )
{
 int i;
 float lightvector[V_LNG_VECTOR],viewvector[V_LNG_VECTOR],reflect[V_LNG_VECTOR];
 float diffuseratio,specularratio,specularfun;

 V_unit_vector(lightvector,where,l->tr_centre);
 V_unit_vector(viewvector,where,viewer);

 if((diffuseratio=V_scalar_product(normal,lightvector))>0)
 {
  if(TR_rendering_type&(TR_DIFFUSE|TR_SPECULAR))
  {
   light[0]+=l->tr_intensity[0]*material->tr_diffuse[0]*diffuseratio;
   light[1]+=l->tr_intensity[1]*material->tr_diffuse[1]*diffuseratio;
   light[2]+=l->tr_intensity[2]*material->tr_diffuse[2]*diffuseratio;
  }
                                            /* diffuse term */
  if(TR_rendering_type&TR_SPECULAR)
  {
   reflect[0]=2*diffuseratio*normal[0]-lightvector[0];
   reflect[1]=2*diffuseratio*normal[1]-lightvector[1];
   reflect[2]=2*diffuseratio*normal[2]-lightvector[2];

   if((specularratio=V_scalar_product(reflect,viewvector))>0)
   {
    for(specularfun=1,i=0;i<material->tr_exponent;i++) specularfun*=specularratio;
    light[0]+=l->tr_intensity[0]*material->tr_specular*specularfun;
    light[1]+=l->tr_intensity[1]*material->tr_specular*specularfun;
    light[2]+=l->tr_intensity[2]*material->tr_specular*specularfun;
   }                                        /* specular term */
  }
 }
 return(light);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Casting a ray towards a lightsource to find out if    *
 * it is hidden by other objects or not.                 *
 *                                                       *
 * RETURNS: 1 light source visible; 0 otherwise.         *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int TRI_shadow_ray(struct TR_world *w,
                   struct TR_point_light *l,
                   float *point,
                   int cur_obj
                  )
{
 float t=0.0;
 int i;
 struct TR_ray r;

 TRI_make_ray_point(&r,point,l->tr_centre);
 for(i=0;i<w->tr_no_objects;i++)            /* finding intersection */
 {
  if(i!=cur_obj)
  {
   switch(w->tr_objects[i]->tr_type)
   {
    case TR_SPHERE:  t=TR_sphere_intersect(&r,(struct TR_sphere*)w->tr_objects[i]);
                     break;

    case TR_POLYGON: t=TR_polygon_intersect(&r,(struct TR_polygon*)w->tr_objects[i]);
                     break;
   }
   if((t>0)&&(t<=1)) return(1);             /* first intersection is enough */
  }
 }

 return(0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Casting a ray into the world, recursing to compute    *
 * environmental reflections.                            *
 *                                                       *
 * RETURNS: Illumination for the pixel.                  *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

float *TRI_direct_ray(float *light,struct TR_world *w,
                                   struct TR_ray *r,
                                   int cur_obj,
                                   int depth
                     )
{
 int i,min=0,no_inter=0;
 float objt[TR_MAX_SPHERES],t=0.0;
 int obj[TR_MAX_SPHERES];
 struct TR_ray rr;
 float where[V_LNG_VECTOR];                 /* current intersection */
 float normal[V_LNG_VECTOR];                /* of the current intersection */
 float viewer[V_LNG_VECTOR],reflect[V_LNG_VECTOR],rlight[V_LNG_VECTOR];

 if(depth!=0)
 {
  for(i=0;i<w->tr_no_objects;i++)           /* finding intersection */
  {
   if(i!=cur_obj)                           /* with itself, no sense */
   {
    switch(w->tr_objects[i]->tr_type)
    {
     case TR_SPHERE:  t=TR_sphere_intersect(r,(struct TR_sphere*)w->tr_objects[i]);
	 					//Í¬ÇòÇó½»
                      break;

     case TR_POLYGON: t=TR_polygon_intersect(r,(struct TR_polygon*)w->tr_objects[i]);
						//Í¬¶à±ßÐÎÇó½»
                      break;
    }
    if(t>0)                                 /* not behind the ray */
    {
     objt[no_inter]=t; obj[no_inter++]=i;   /* a valid intersection */
    }
   }
  }

  if(no_inter!=0)                           /* if some objects intersected */
  {
   for(i=1;i<no_inter;i++)
    if(objt[min]>objt[i]) min=i;            /* finding closest intersection */

   light[0]+=w->tr_objects[obj[min]]->tr_material.tr_ambient[0]*w->tr_ambient[0];
   light[1]+=w->tr_objects[obj[min]]->tr_material.tr_ambient[1]*w->tr_ambient[1];
   light[2]+=w->tr_objects[obj[min]]->tr_material.tr_ambient[2]*w->tr_ambient[2];

   TRI_on_ray(where,r,objt[min]);           /* intersection's coordinate */

   switch(w->tr_objects[obj[min]]->tr_type)
   {
	//
	case TR_SPHERE:  TR_sphere_normal(normal,where,(struct TR_sphere*)w->tr_objects[obj[min]]);
                     break;

    case TR_POLYGON: TR_polygon_normal(normal,where,(struct TR_polygon*)w->tr_objects[obj[min]]);
                     break;
   }

   for(i=0;i<w->tr_no_point_lights;i++)     /* illumination from each light */
   {
    if((!TRI_shadow_ray(w,w->tr_point_lights[i],where,obj[min]))||
       (!(TR_rendering_type&TR_SHADOW))
      )
     TRI_illuminate(light,w->tr_point_lights[i],
                    &w->tr_objects[obj[min]]->tr_material,
                    normal,where,TR_viewer
                   );
   }

   if(TR_rendering_type&TR_REFLECT)
   {
    V_unit_vector(viewer,where,TR_viewer);
    V_multiply(reflect,normal,V_scalar_product(normal,viewer)*2);
    V_difference(reflect,reflect,viewer);
    TRI_make_ray_vector(&rr,where,reflect); /* prepare recursive ray */

    TRI_direct_ray(V_zero(rlight),w,&rr,obj[min],depth-1);
    light[0]+=rlight[0]*w->tr_objects[obj[min]]->tr_material.tr_reflect;
    light[1]+=rlight[1]*w->tr_objects[obj[min]]->tr_material.tr_reflect;
    light[2]+=rlight[2]*w->tr_objects[obj[min]]->tr_material.tr_reflect;
   }
  }
 }
 return(light);
}

/**********************************************************\
 * Setting the rendering type.                            *
\**********************************************************/

void TR_init_rendering(int type)
{
 TR_rendering_type=type;
}

/**********************************************************\
 * Setting the camera parameter, where TR_viewer stores   *
 * position of the viewer's eye, TR_screen origine of the *
 * projection plane, TR_screen_u and TR_screen_v          *
 * orientation of the projection plane in the world       *
 * space.                                                 *
\**********************************************************/
//ÉãÏñ»ú²ÎÊýµÄÉèÖÃ£¬
//TR_viever´æ´¢ÕÚÊÓ´°eyeµÄÎ»ÖÃ£

//TR_screenÊÇÍ¶Ó°Æ½Ãæ??
//TR_screen_u & TR_sreen_v ÊÇÊÀ½ç¿Õ¼äÔÚÍ¶Ó°Æ½ÃæÉÏµÄÍ¶Ó°
void TR_set_camera(float viewer_x,float viewer_y,float viewer_z,
                   float screen_x,float screen_y,float screen_z,
                   float screen_ux,float screen_uy,float screen_uz,
                   float screen_vx,float screen_vy,float screen_vz
                  )
{
 V_vector_coordinates(TR_viewer,viewer_x,viewer_y,viewer_z);
 V_vector_coordinates(TR_screen,screen_x,screen_y,screen_z);
 V_vector_coordinates(TR_screen_u,screen_ux,screen_uy,screen_uz);
 V_vector_coordinates(TR_screen_v,screen_vx,screen_vy,screen_vz);
}

/**********************************************************\
 * Does nothing, for symmetry's sake.                     *
\**********************************************************/

void TR_sphere_init(struct TR_sphere *s)
{
 s->tr_type=TR_SPHERE;
}

/**********************************************************\
 * Finding intersection of a ray with a sphere.           *
 *                                                        *
 * RETURNS: Distance from the origine of the ray.         *
 * --------                                               *
\**********************************************************/

float TR_sphere_intersect(struct TR_ray *r,struct TR_sphere *s)
{
 float a,b,c,det;
 float d[V_LNG_VECTOR],t1,t2;

 a=V_scalar_product(r->tr_codirected,r->tr_codirected);
 b=2*V_scalar_product(r->tr_codirected,V_difference(d,r->tr_start,s->tr_centre));
 c=V_scalar_product(d,d)-s->tr_radius*s->tr_radius;

//¸ùµÄÅÐ¶Ï
 det=b*b-4*a*c;

 if(det<0) return(-1);                      /* no intersection */
 if(det==0) return(-b/(2*a));               /* one intersection */
 t1=(-b+sqrt(det))/(2*a);
 t2=(-b-sqrt(det))/(2*a);
 if(t1<t2) return(t1); else return(t2);     /* closest intersection */
}

/**********************************************************\
 * Computes sphere's normal for a point on a shpere.      *
 *                                                        *
 * RETURNS: The normal vector.                            *
 * --------                                               *
\**********************************************************/

float *TR_sphere_normal(float *normal,float *where,
                        struct TR_sphere *s
                       )
{
 V_unit_vector(normal,s->tr_centre,where);  /* from the centre */
 return(normal);
}

/**********************************************************\
 * Computes plane equation and the equations delimiting   *
 * the edges.                                             *
\**********************************************************/

//¼ÆËãÆ½Ãæ·½³ÌºÍ±ßµÄÍ¶Ó°¹«Ê½
//¶à±ßÐÎµÄ³õÊ¼»¯
void TR_polygon_init(struct TR_polygon *p)
{
 int i;
 float a[V_LNG_VECTOR],b[V_LNG_VECTOR];
//Ö¸¶¨ÀàÐÍ
 p->tr_type=TR_POLYGON;

//Îªedges·ÖÅäÄÚ´æ¿Õ¼ä
 p->tr_edges=(float*)malloc((p->tr_no_vertices)*4*sizeof(float));

 V_vector_points(a,&p->tr_vertices[V_LNG_VECTOR*2],&p->tr_vertices[V_LNG_VECTOR]);
 V_vector_points(b,&p->tr_vertices[V_LNG_VECTOR],&p->tr_vertices[0]);
 V_vector_product(p->tr_normal,a,b);        /* normal to the plane */

 V_zero(a);                                 /* making it unit length */
 V_unit_vector(p->tr_normal,a,p->tr_normal);

 for(i=0;i<p->tr_no_vertices;i++)           /* finding equations for edges */
 {
  V_vector_points(a,&p->tr_vertices[i*V_LNG_VECTOR],&p->tr_vertices[(i+1)*V_LNG_VECTOR]);
  V_vector_product(b,p->tr_normal,a);
  V_plane(&p->tr_edges[i*4],b,&p->tr_vertices[i*V_LNG_VECTOR]);
 }
}

/**********************************************************\
 * Finding intersection of a ray with a polygon.           *
 *                                                        *
 * RETURNS: Distance from the origine of the ray.         *
 * --------                                               *
\**********************************************************/

float TR_polygon_intersect(struct TR_ray *r,struct TR_polygon *p)
{
 float a[V_LNG_VECTOR],t,s1,s2;
 int i;

 V_difference(a,p->tr_vertices,r->tr_start);
 s1=V_scalar_product(a,p->tr_normal);
 s2=V_scalar_product(r->tr_codirected,p->tr_normal);

 if(s2==0) return(-1); else t=s1/s2;
 if(t<0) return(-1);

 TRI_on_ray(a,r,t);

 for(i=0;i<p->tr_no_vertices;i++)
  if(V_vertex_on_plane(&p->tr_edges[i*4],a)>0) return(-1);

 return(t);
}

/**********************************************************\
 * Returns polygon's normal.                              *
 *                                                        *
 * RETURNS: The Normal vector.                            *
 * --------                                               *
\**********************************************************/

float *TR_polygon_normal(float *normal,float *where,
                         struct TR_polygon *p
                        )
{
 normal[0]=p->tr_normal[0];
 normal[1]=p->tr_normal[1];
 normal[2]=p->tr_normal[2];
 return(normal);
}

/**********************************************************\
 * Initialisez all entities in the world.                 *
\**********************************************************/

void TR_init_world(struct TR_world *w)
{
 int i;

 for(i=0;i<w->tr_no_objects;i++)
 {
  switch(w->tr_objects[i]->tr_type)
  {
  //¶ÔÓÚÐéÄâ»·¾³ÖÐµÄsphereÎïÌå»òÊÇpolygonÎïÌå½øÐÐ³õÊ¼»¯
   case TR_SPHERE:  TR_sphere_init((struct TR_sphere*)w->tr_objects[i]);
   					//½ö½ö¶ÔÓÚÎïÌåµsphereµÄÊý¾Ý½á¹¹ÀàÐÍ½øÐÐÁËÖ¸¶¨
   					//ÒòÎªÊÇ¶Ô³ÆµÄÇòÌå£¬ËùÒÔ»á¼òµ¥Ð©
                    break;
   case TR_POLYGON: TR_polygon_init((struct TR_polygon*)w->tr_objects[i]);
   					//ÉÔÎ¢¸´ÔÓÐ©£¬ÆäÖÐÉæ¼°µ½ÒÔÏÂ¼¸¸ö·½ÃæµÄ¹¤×÷
   					//???
                    break;
  }
 }
}

/**********************************************************\
 * Ray tracing a scene for all pixels.                    *
\**********************************************************/

void TR_trace_world(struct TR_world *w,int depth)
{
 int i,j;
 float *c,x,y;
 float light[V_LNG_VECTOR];
 float point[V_LNG_VECTOR];
 int coord[2];
 struct TR_ray r;

 for(i=0;i<HW_SCREEN_X_SIZE;i++)
 {
  for(j=0;j<HW_SCREEN_Y_SIZE;j++)           /* for each pixel on screen */
  {
   coord[0]=i;
   coord[1]=j;                              /* screen coordinates */

   x=i-HW_SCREEN_X_SIZE/2;
   y=j-HW_SCREEN_Y_SIZE/2;                  /* plane coordinates */

   point[0]=TR_screen_u[0]*x+TR_screen_v[0]*y+TR_screen[0];
   point[1]=TR_screen_u[1]*x+TR_screen_v[1]*y+TR_screen[1];
   point[2]=TR_screen_u[2]*x+TR_screen_v[2]*y+TR_screen[2];

//´ÓÁ½¸öµã(point and TR_viewer)¹¹½¨ray
   TRI_make_ray_point(&r,TR_viewer,point);

//¹Ø¼üÖÐµÄ¹Ø¼ü£¬¼ÆËã»·¾³¹â£¬·µ»ØpixelµÄÕÕÃ÷¶È   
   c=TRI_direct_ray(V_zero(light),w,&r,-1,depth);

//Setting a pixel   
   G_pixel(coord,CL_colour((int)(c[0]*CL_COLOUR_LEVELS),
                           (int)(c[1]*CL_COLOUR_LEVELS),
                           (int)(c[2]*CL_COLOUR_LEVELS)
                          )
          );
  }
 }
}

/**********************************************************/
