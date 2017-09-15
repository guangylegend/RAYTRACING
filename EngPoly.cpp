/** 3DGPL *************************************************\
 * ()                                                     *
 * Functions for polygonal objects.                       *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model;                   *
 *  _Z_BUFFER_               Depth array;                 *
 *  _PAINTER_                Back to front rendering.     *
 *                                                        *
 * Defines:                                               *
 *  M_init_polygon_object    Compute BSP tree and normals;*
 *  M_shade_polygon_object   Shadings using normals;      *
 *  M_render_polygon_object  Renders a polygonal solid.   *
 *                                                        *
 * Internals:                                             *
 *  MI_add_tuple             A result of a split;         *
 *  MI_split_polygon         Called after a split;        *
 *  MI_order_polygons        Creating one BSP tree node;  *
 *  MI_render_polygons       Rendering polygons in order. *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//EngPoly.cpp

#include "LightTrack.h"           /* HW_copy_ints */
#include "Graphics.h"           /* 2-D rendering */
#include "Trans.h"                 /* 3-D transformations */
#include "Clipper.h"             /* xyz clipping */
#include "Engine.h"               /* 3-D engine */
#include "Colour.h"               /* light related */
#include <stdlib.h>                         /* NULL */

int M_no_polygons;                          /* used in bsp tree building */
struct M_polygon **M_polygons;              /* static polygons */
int M_no_vertices;
int *M_vertices;                            /* static vertices X Y Z */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Finding an index to a vertex.               *
 * ---------                                             *
 * RETURNS: Index the passed vertex in the array.        *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int MI_add_tuple(int *vertex)
{
 int i;

 for(i=0;i<M_no_vertices;i++)               /* comparing X Y Z only */
 {
  if((M_vertices[i*T_LNG_VECTOR]==vertex[0])&&
     (M_vertices[i*T_LNG_VECTOR+1]==vertex[1])&&
     (M_vertices[i*T_LNG_VECTOR+2]==vertex[2])
    ) return(i);
 }
 if(i>=M_MAX_OBJECT_VERTICES)
  HW_error("(Engine) Not enough internal storage for vertices.\n");
                                            /* X Y Z to one Tx Ty to another */
 HW_copy_int(vertex,M_vertices+M_no_vertices*T_LNG_VECTOR,3);

 return(M_no_vertices++);                   /* index of where inserted */
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Called after a split has occured.           *
 * ---------                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void MI_split_polygon(struct M_polygon *root,struct M_polygon *old,
                      struct M_polygon *new1,struct M_polygon *new2
                     )
{
 int tmp1[M_MAX_POLYGON_VERTICES*M_LNG_POLYGON_VERTEX];
 int tmp2[M_MAX_POLYGON_VERTICES*M_LNG_POLYGON_VERTEX];
 int i;

 for(i=0;i<=old->m_no_edges;i++)
 {
  HW_copy_int(M_vertices+old->m_vertices[i*M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
              tmp1+i*5,3
             );                             /* X Y Z */
  HW_copy_int(old->m_vertices+i*M_LNG_POLYGON_VERTEX+M_IDX_POLYGON_TEXTURE,
              tmp1+i*5+3,2
             );                             /* Tx Ty */
 }
 new1->m_type=old->m_type&M_NOT_QUAD;       /* can't be any longer M_QUAD */
 new1->m_colour=old->m_colour;
#if defined(_CI_)
 new1->m_intensity=old->m_intensity;        /* I */
#endif
#if defined(_RGB_)
 new1->m_red=old->m_red;                    /* R G B */
 new1->m_green=old->m_green;
 new1->m_blue=old->m_blue;
#endif
 new1->m_log_texture_space_size=old->m_log_texture_space_size;
 new1->m_texture=old->m_texture;
 new1->m_no_edges=
  C_polygon_xyz_clipping(tmp1,tmp2,
                         M_vertices+root->m_vertices[0]*T_LNG_VECTOR,
                         M_vertices+root->m_vertices[M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
                         M_vertices+root->m_vertices[M_LNG_POLYGON_VERTEX*2]*T_LNG_VECTOR,
                         5,old->m_no_edges
                        );                  /* 5 == X Y Z Tx Ty */
 new1->m_vertices=(int*)malloc(sizeof(int)*(new1->m_no_edges+1)*M_LNG_POLYGON_VERTEX);
 if(new1->m_vertices==NULL) HW_error("(Engine) Not enough memory.\n");

 for(i=0;i<=new1->m_no_edges;i++)           /* back into indexing */
 {                                          /* getting vertex Index */
  new1->m_vertices[i*M_LNG_POLYGON_VERTEX]=MI_add_tuple(tmp2+i*5);
  HW_copy_int(tmp2+i*5+3,new1->m_vertices+i*M_LNG_POLYGON_VERTEX+M_IDX_POLYGON_TEXTURE,2);
 }                                          /* getting Tx Ty */

 new2->m_type=old->m_type&M_NOT_QUAD;       /* can't be any longer */
 new2->m_colour=old->m_colour;
#if defined(_CI_)
 new2->m_intensity=old->m_intensity;
#endif
#if defined(_RGB_)
 new2->m_red=old->m_red;
 new2->m_green=old->m_green;
 new2->m_blue=old->m_blue;
#endif
 new2->m_log_texture_space_size=old->m_log_texture_space_size;
 new2->m_texture=old->m_texture;
 new2->m_no_edges=
  C_polygon_xyz_clipping(tmp1,tmp2,
                         M_vertices+root->m_vertices[M_LNG_POLYGON_VERTEX*2]*T_LNG_VECTOR,
                         M_vertices+root->m_vertices[M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
                         M_vertices+root->m_vertices[0]*T_LNG_VECTOR,
                         5,old->m_no_edges
                        );                  /* 5 == X Y Z Tx Ty */

 new2->m_vertices=(int*)malloc(sizeof(int)*(new2->m_no_edges+1)*M_LNG_POLYGON_VERTEX);
 if(new2->m_vertices==NULL) HW_error("(Engine) Not enough memory.\n");

 for(i=0;i<=new2->m_no_edges;i++)           /* back into indexing */
 {                                          /* getting Indx Tx Ty */
  new2->m_vertices[i*M_LNG_POLYGON_VERTEX]=MI_add_tuple(tmp2+i*5);
  HW_copy_int(tmp2+i*5+3,new2->m_vertices+i*M_LNG_POLYGON_VERTEX+M_IDX_POLYGON_TEXTURE,2);
 }                                          /* from X Y Z [I]|[RGB] Tx Ty */

 for(i=0;i<M_no_polygons;i++)
 {
  if(M_polygons[i]==old)                    /* searching for the one to kill */
  {
   free(M_polygons[i]->m_vertices);         /* killing the old one */
   free(M_polygons[i]);
   M_polygons[i]=new1;                      /* puting pointer to a new one */
   break;
  }
 }
 if(M_no_polygons>=M_MAX_OBJECT_POLYGONS)
  HW_error("(Engine) Can't handle this many polygons.");
 M_polygons[M_no_polygons++]=new2;          /* split produced two polygons */
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Creates a BSP tree order node.              *
 * ---------                                             *
 * RECURSIVE: Calls itself twice.                        *
 * ----------                                            *
 * RETURNS: Pointer to a created order structure.        *
 * --------                                              *
 * Minimizes on number of splits and secondary on        *
 * disbalance in nodes.                                  *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct M_polygon_object_order *MI_order_polygons(struct M_polygon **polygons,
                                                 int no_polygons
                                                )
{
 struct M_polygon *new1,*new2;              /* for splits */
 struct M_polygon_object_order *tmp;
 struct M_polygon *positive[M_MAX_OBJECT_POLYGONS];
 struct M_polygon *negative[M_MAX_OBJECT_POLYGONS];
 static int indices[M_MAX_OBJECT_POLYGONS]; /* static for the recursion sake */
 static int balances[M_MAX_OBJECT_POLYGONS];
 static int splits[M_MAX_OBJECT_POLYGONS];
 static int plane[4];                       /* coefs of a plane equation */
 int i,j,k,l,balance,bbal=0,split,itmp,no_positive=0,no_negative=0;

 if(no_polygons==0) return(NULL);           /* base case */

 for(i=0;i<no_polygons;i++) { indices[i]=i; balances[i]=0; splits[i]=0; }
 for(i=0;i<no_polygons;i++)                 /* calculate balance and splits */
 {
  T_plane(M_vertices+polygons[i]->m_vertices[0]*T_LNG_VECTOR,
          M_vertices+polygons[i]->m_vertices[M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
          M_vertices+polygons[i]->m_vertices[M_LNG_POLYGON_VERTEX*2]*T_LNG_VECTOR,
          plane
         );
  for(j=0;j<no_polygons;j++)                /* for all polygons */
  {
   if(i==j) continue;                       /* not for the 1 assumed in root */
   for(k=0,balance=0,split=0;k<polygons[j]->m_no_edges;k++)
   {
    for(l=0;l<polygons[i]->m_no_edges;l++)  /* check shared vertices directly */
     if(polygons[i]->m_vertices[l*M_LNG_POLYGON_VERTEX]==
        polygons[j]->m_vertices[k*M_LNG_POLYGON_VERTEX]) { bbal=0; break; }

    if(l==polygons[i]->m_no_edges)          /* if not shared */
     bbal=T_vertex_on_plane(M_vertices+     /* check plane equation */
                            polygons[j]->m_vertices[k*M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
                            plane
                           );
    if(bbal==0) continue;                   /* ignore those on the plane */
    if(bbal>0) bbal=1; else bbal=-1;
    if(balance==0) balance=bbal;            /* initial point */
    else                                    /* other then initial point */
    {
     if(balance!=bbal) { split=1; balance=0; break; }
    }
   }
   balances[i]+=balance;                    /* parameters assuming i in root */
   splits[i]+=split;
  }
 }                                          /* balances and splits found */

 for(i=0;i<no_polygons;i++) balances[i]=abs(balances[i]);

 for(i=no_polygons-1;i>0;i--)               /* sorting seeking best balance */
 {
  for(j=0;j<i;j++)
  {
   if(balances[j]>balances[j+1])            /* to have least first */
   {
    itmp=balances[j]; balances[j]=balances[j+1]; balances[j+1]=itmp;
    itmp=indices[j]; indices[j]=indices[j+1]; indices[j+1]=itmp;
    itmp=splits[j]; splits[j]=splits[j+1]; splits[j+1]=itmp;
   }
  }
 }

 for(i=no_polygons-1;i>0;i--)               /* sorting seeking less splits */
 {
  for(j=0;j<i;j++)
  {
   if(splits[j]>splits[j+1])                /* to have least first */
   {
    itmp=balances[j]; balances[j]=balances[j+1]; balances[j+1]=itmp;
    itmp=indices[j]; indices[j]=indices[j+1]; indices[j+1]=itmp;
    itmp=splits[j]; splits[j]=splits[j+1]; splits[j+1]=itmp;
   }
  }
 }

 tmp=(struct M_polygon_object_order*)
      malloc(sizeof(struct M_polygon_object_order));
 if(tmp==NULL) HW_error("(Engine) Not enough memory.\n");

 tmp->m_root=polygons[indices[0]];          /* the one which is best */

 T_plane(M_vertices+tmp->m_root->m_vertices[0]*T_LNG_VECTOR,
         M_vertices+tmp->m_root->m_vertices[M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
         M_vertices+tmp->m_root->m_vertices[M_LNG_POLYGON_VERTEX*2]*T_LNG_VECTOR,
         plane
        );                                  /* plane equation for the root */
 for(i=0;i<no_polygons;i++)
 {
  if(tmp->m_root==polygons[i]) continue;    /* not for the 1 in the root */

  for(j=0,balance=0,split=0;j<polygons[i]->m_no_edges;j++)
  {
   for(l=0;l<tmp->m_root->m_no_edges;l++)   /* check shared vertices directly */
    if(tmp->m_root->m_vertices[l*M_LNG_POLYGON_VERTEX]==
       polygons[i]->m_vertices[j*M_LNG_POLYGON_VERTEX]) { bbal=0; break; }

   if(l==tmp->m_root->m_no_edges)           /* if not shared */
    bbal=T_vertex_on_plane(M_vertices+
                           polygons[i]->m_vertices[j*M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
                           plane
                          );
   if(bbal==0) continue;                    /* ignore those on the plane */
   if(bbal>0) bbal=1; else bbal=-1;
   if(balance==0) balance=bbal;             /* initial point */
   else if(balance!=bbal) { split=1; balance=0; break; }
  }

  if(split)                                 /* what to do to a polygon */
  {
   new1=(struct M_polygon*)malloc(sizeof(struct M_polygon));
   new2=(struct M_polygon*)malloc(sizeof(struct M_polygon));
   if((new1==NULL)||(new2==NULL)) HW_error("(Fatal) Not enough memory.\n");

   MI_split_polygon(tmp->m_root,polygons[i],new1,new2);
   if((no_positive>=M_MAX_OBJECT_POLYGONS)||(no_negative>=M_MAX_OBJECT_POLYGONS))
    HW_error("(Fatal) Not enoght internal storage for polygons.\n");
   positive[no_positive++]=new1;
   negative[no_negative++]=new2;
  }
  else
  {
   if(balance>0)                            /* entirely in subspaces */
   {
    if(no_positive>=M_MAX_OBJECT_POLYGONS)
     HW_error("(Fatal) Not enoght internal storage for polygons.\n");
    positive[no_positive++]=polygons[i];
   }
   else
   {
    if(no_negative>=M_MAX_OBJECT_POLYGONS)
     HW_error("(Fatal) Not enoght internal storage for polygons.\n");
    negative[no_negative++]=polygons[i];
   }
  }
 }                                          /* constructing subspaces */

 tmp->m_positive=MI_order_polygons(positive,no_positive);
 tmp->m_negative=MI_order_polygons(negative,no_negative);
 return(tmp);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Renders polygons in order.                  *
 * ---------                                             *
 * RECURSIVE: Calls itself twice.                        *
 * ----------                                            *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void MI_render_polygons(struct M_polygon_object_order *order,
                        int *vertices
                       )
{
 int plane[4];

 if(order!=NULL)                            /* base case */
 {
  T_plane(vertices+order->m_root->m_vertices[0]*T_LNG_VECTOR,
          vertices+order->m_root->m_vertices[M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
          vertices+order->m_root->m_vertices[M_LNG_POLYGON_VERTEX*2]*T_LNG_VECTOR,
          plane
         );                                 /* happens in the view space */
  if(plane[3]>0)                            /* viewer is at (0,0,0) */
  {                                         /* direct order of sub-spaces */
   MI_render_polygons(order->m_negative,vertices);
   M_render_polygon(order->m_root,vertices,NULL,NULL);
   MI_render_polygons(order->m_positive,vertices);
  }
  else                                      /* reversed order of sub spaces */
  {
   MI_render_polygons(order->m_positive,vertices);
   M_render_polygon(order->m_root,vertices,NULL,NULL);
   MI_render_polygons(order->m_negative,vertices);
  }
 }
}

/**********************************************************\
 * Constructing a BSP tree for a polygonal object, and    *
 * computing the normals.                                 *
\**********************************************************/

void M_init_polygon_object(struct M_polygon_object *object)
{
 int tmp_normals[M_MAX_OBJECT_VERTICES*T_LNG_VECTOR];
 int normals[M_MAX_OBJECT_VERTICES*T_LNG_VECTOR];
 int i,j,k,n[5],num,norm_idx;

#if defined(_PAINTER_)
 int tmp_vertices[M_MAX_OBJECT_VERTICES*M_LNG_POLYGON_VERTEX];
 struct M_polygon *tmp_polygons[M_MAX_OBJECT_POLYGONS];

 for(i=0;i<object->m_no_polygons;i++) tmp_polygons[i]=object->m_polygons[i];
 HW_copy_int(object->m_vertices,tmp_vertices,
             object->m_no_vertices*T_LNG_VECTOR
            );
 M_polygons=tmp_polygons; M_no_polygons=object->m_no_polygons;
 M_vertices=tmp_vertices; M_no_vertices=object->m_no_vertices;

 object->m_order=MI_order_polygons(object->m_polygons,object->m_no_polygons);

 free(object->m_polygons);                  /* number of polygons/vertices */
 free(object->m_vertices);                  /* was likely to change anyway */
 object->m_polygons=(struct M_polygon**)malloc(sizeof(struct M_polygon*)*
                                               M_no_polygons
                                              );
 object->m_vertices=(int*)malloc(sizeof(int)*M_no_vertices*T_LNG_VECTOR);
 if((object->m_polygons==NULL)||(object->m_vertices==NULL))
  HW_error("(Engine) Not enough memory.\n");

 for(i=0;i<M_no_polygons;i++) object->m_polygons[i]=tmp_polygons[i];
 HW_copy_int(M_vertices,object->m_vertices,M_no_vertices*T_LNG_VECTOR);

 object->m_no_polygons=M_no_polygons;       /* new numbers */
 object->m_no_vertices=M_no_vertices;
#endif

 M_vertices=tmp_normals; M_no_vertices=0;   /* nothing added yet */

 for(i=0;i<object->m_no_polygons;i++)       /* for all polygons */
 {
  T_normal_plane(object->m_vertices+object->m_polygons[i]->m_vertices[0]*T_LNG_VECTOR,
                 object->m_vertices+object->m_polygons[i]->m_vertices[M_LNG_POLYGON_VERTEX]*T_LNG_VECTOR,
                 object->m_vertices+object->m_polygons[i]->m_vertices[M_LNG_POLYGON_VERTEX*2]*T_LNG_VECTOR,
                 normals+i*T_LNG_VECTOR
                );
  if(object->m_polygons[i]->m_type&M_PLANNAR)
  {                                         /* same normal in flat polygons */
   object->m_polygons[i]->m_normals=(int*)malloc(sizeof(int)*(object->m_polygons[i]->m_no_edges+1));
   if(object->m_polygons[i]->m_normals==NULL)
    HW_error("(Engine) Not enough memory.\n");
   object->m_polygons[i]->m_normals[0]=MI_add_tuple(normals+i*T_LNG_VECTOR);
   for(j=1;j<=object->m_polygons[i]->m_no_edges;j++)
    object->m_polygons[i]->m_normals[j]=object->m_polygons[i]->m_normals[0];
  }
 }

 for(i=0;i<object->m_no_vertices;i++)       /* for all vertices */
 {
  n[0]=n[1]=n[2]=0;
  for(num=0,j=0;j<object->m_no_polygons;j++)/* check all polygons which */
  {
   if(object->m_polygons[j]->m_type&M_CURVED)
   {                                        /* emulate curved surfaces */
    if(object->m_polygons[j]->m_normals==NULL)
    {
     object->m_polygons[j]->m_normals=(int*)malloc(sizeof(int)*(object->m_polygons[j]->m_no_edges+1));
     if(object->m_polygons[j]->m_normals==NULL)
      HW_error("(Engine) Not enough memory.\n");
     HW_set_int(object->m_polygons[j]->m_normals,object->m_polygons[j]->m_no_edges+1,0);
    }

    for(k=0;k<=object->m_polygons[j]->m_no_edges;k++)
    {
     if(object->m_polygons[j]->m_vertices[k*M_LNG_POLYGON_VERTEX]==i)
     {                                      /* if the vertex belong to it */
      object->m_polygons[j]->m_normals[k]=-1;
      n[0]+=normals[j*T_LNG_VECTOR];        /* mark the normal */
      n[1]+=normals[j*T_LNG_VECTOR+1];      /* polygon's normal */
      n[2]+=normals[j*T_LNG_VECTOR+2];
      num++;
     }
    }
   }
  }
  if(num!=0)                                /* for curved only */
  {
   n[0]/=num; n[1]/=num; n[2]/=num;
   norm_idx=MI_add_tuple(n);

   for(j=0;j<object->m_no_polygons;j++)
   {
    if(object->m_polygons[j]->m_type&M_CURVED)
    {                                        /* emulate curved surfaces */
     for(k=0;k<=object->m_polygons[j]->m_no_edges;k++)
     {
      if(object->m_polygons[j]->m_normals[k]==-1)
       object->m_polygons[j]->m_normals[k]=norm_idx;
     }
    }
   }
  }
 }
 object->m_normals=(int*)malloc(sizeof(int)*M_no_vertices*T_LNG_VECTOR);
 object->m_no_normals=M_no_vertices;        /* store the normals */
 HW_copy_int(M_vertices,object->m_normals,M_no_vertices*T_LNG_VECTOR);
}

/**********************************************************\
 * Shading a polygonal shape, setting both flat and       *
 * interpolative intensities.                             *
\**********************************************************/

void M_shade_polygon_object(struct M_polygon_object *object,
                            int x,int y,int z,
                            int alp,int bet,int gam,
                            int no_lights,
                            struct M_light **lights
                           )
{
 int i;
 int tmp_vertices[M_MAX_OBJECT_VERTICES*M_LNG_POLYGON_VERTEX];
 int tmp_normals[M_MAX_OBJECT_VERTICES*M_LNG_POLYGON_VERTEX];

 T_set_self_rotation(alp,bet,gam);          /* shading happens in world space */
 T_self_rotation(object->m_normals,tmp_normals,object->m_no_normals);
 T_self_rotation(object->m_vertices,tmp_vertices,object->m_no_vertices);
 T_translation(tmp_vertices,tmp_vertices,object->m_no_vertices,x,y,z);

 if((object->m_no_vertices>M_MAX_OBJECT_VERTICES)||
    (object->m_no_normals>M_MAX_OBJECT_VERTICES)
   )
  HW_error("(Engine) Can't handle this many vertices or normals in polygons.");

 for(i=0;i<object->m_no_polygons;i++)
 {
  M_shade_polygon(object->m_polygons[i],tmp_vertices,tmp_normals,
                  no_lights,lights
                 );
 }
}

/**********************************************************\
 * Rendering a generic polygonal shape in perspective.    *
\**********************************************************/

void M_render_polygon_object(struct M_polygon_object *object,
                             int x,int y,int z,
                             int alp,int bet,int gam
                            )
{
 int tmp_vertices[M_MAX_OBJECT_VERTICES*M_LNG_POLYGON_VERTEX];
 int i;

 if(object->m_no_vertices>=M_MAX_OBJECT_VERTICES)
  HW_error("(Engine) Can't handle this many vertices in polygons.");

 T_set_self_rotation(alp,bet,gam);          /* transformations */
 T_cancatinate_self_world(x+M_camera_x,y+M_camera_y,z+M_camera_z);
 T_cancatinated_rotation(object->m_vertices,tmp_vertices,object->m_no_vertices);

 if(object->m_order!=NULL)                  /* when order struct exists */
 {
  MI_render_polygons(object->m_order,tmp_vertices);
 }
 else                                       /* just rendering all polygons */
 {                                          /* if no BSP tree structure */
  for(i=0;i<object->m_no_polygons;i++)
  {
   M_render_polygon(object->m_polygons[i],tmp_vertices,NULL,NULL);
  }
 }
}

/**********************************************************/
