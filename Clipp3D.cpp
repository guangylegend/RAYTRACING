/** 3DGPL *************************************************\
 * ()                                                     *
 * 3-D volume and Z clipping.                             *
 *                                                        *
 * Defines:                                               *
 *  C_volume_clipping        Out of view-volume elements; *
 *                                                        *
 *  C_line_z_clipping        Clipping a line;             *
 *  C_polygon_z_clipping     View plane cliping;          *
 *  C_line_xyz_clipping      Arbitrry clipping of a line; *
 *  C_polygon_xyz_clipping   Arbitrry plane cliping.      *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/

//Clipp3D.cpp

#include "LightTrack.h"           /* HW_copy_int stuff */
#include "Trans.h"                 /* T_plane etc. */
#include "Clipper.h"             /* clipping constants */
#include <limits.h>                         /* INT_MAX & INT_MIN */

int C_3D_clipping;                          /* type of performed clipping */

/**********************************************************\
 * Accept/reject volume clipping.                         *
 *                                                        *
 * RETURNS:  0 when extend is for sure outside the        *
 * --------    view volume, just discard it;              *
 *          -1 when further clipping is required;         *
 *           1 when element has a fair chance to be       *
 *             inside the view volume, and should be      *
 *             clipped by 2-D clipping when rasterizing.  *
 *                                                        *
 *               | z                                      *
 *          \    |    /      Clipping volume is a pyramid *
 *           \   |   /       with 90 degree angle,        *
 *            \  |  /        so the real view volume      *
 *        -x>z \ | /  x>z    better be inside it.         * 
 *              \|/                                       *
 *       --------+--------- x                             *
 *       z<C_Z_CLIPPING_MIN                               *
 *                                                        *
\**********************************************************/

int C_volume_clipping(int *min,int *max)
{
 if((max[2]<min[0])||(max[2]<min[1])||(max[2]<-max[0])||
   (max[2]<-max[1])||(max[2]<=C_Z_CLIPPING_MIN))
 {
  return(0);                                /* outside */
 }
 else
 {
  if(min[2]<C_Z_CLIPPING_MIN) return(-1);   /* partly behind clipping plane */
  else return(1);
 }
}

/**********************************************************\
 * Line clipping using binary search technique.           *
 *                                                        *
 * RETURNS: 0 element was outside the view volume;        *
 * -------- 1 element was clipped to the view volume.     *
 *                                                        *
 * SETS: C_3D_clipping to 1 if first vertex was clipped;  *
 * ----- 0 otherwise.                                     *
\**********************************************************/

int C_line_z_clipping(int **vertex1,int **vertex2,int dimension)
{
 register int i;
 register int whereto;
 register int *l,*r,*m,*t;                  /* left right midle and tmp */
 static int c_store0[C_MAX_DIMENSIONS];     /* static stores for clipped vxes */
 static int c_store1[C_MAX_DIMENSIONS];
 static int c_store2[C_MAX_DIMENSIONS];
 int **vmn,**vmx;                           /* so that *vmn[3] < *vmx[3] */
 int swap;                                  /* were coordinates swaped? */

 C_3D_clipping=0;                           /* default no clipping yet */

 if((*vertex1)[2]<(*vertex2)[2])            /* only Z counts 0=X,1=Y,2=Z,... */
 { swap=0; vmn=vertex1; vmx=vertex2; }      /* so that *vmn[2] < *vmx[2] */
 else
 { swap=1; vmn=vertex2; vmx=vertex1; }

 if((*vmx)[2]<=C_Z_CLIPPING_MIN) return(0); /* compleately outside */
 else
 {
  if((*vmn)[2]<=C_Z_CLIPPING_MIN)           /* clipping */
  {
   HW_copy_int(*vmn,m=c_store0,dimension);  /* copying old vertices */
   HW_copy_int(*vmx,r=c_store1,dimension);
   l=c_store2;

   whereto=1;
   while(m[2]!=C_Z_CLIPPING_MIN)
   {
    if(whereto==1) { t=l; l=m; m=t; }
    else           { t=r; r=m; m=t; }
    for(i=0;i<dimension;i++) m[i]=(l[i]+r[i])>>1;
    whereto=m[2]<C_Z_CLIPPING_MIN;
   }
   *vmn=m;                                  /* that is why m[] is static */
   C_3D_clipping=swap^1;
  }
  return(1);                                /* partialy or not clipped */
 }
}

/***********************************************************\
 * Creating a z-clipped polygon.                           *
 *                                                         *
 * RETURNS: number of elements in the clipped polygon.     *
 * -------- (0 when compleately behind view plane)         *
 *                                                         *
 *         |            1-2-3-4-5-6-1  -> 2-2'-5'-6-1-2    *
 *         |5'                                             *
 *      5*-*-----*6     If first vertex in the line is     *
 *      /  |      \     clipped, both vertices are copyed, *
 *    4*   |       *1   If no clipping or second one is    *
 *      \  |      /     clipped then only second one is    *
 *      3*-*-----*2     copyed. If both vertices are       *
 *         |2'          clipped, neither one is copyed.    *
 *                                                         *
\***********************************************************/

int C_polygon_z_clipping(int *from,register int *to,
                         int dimension,int length
                        )
{
 register int i;
 int *v1,*v2,new_lng=0;
 int *first_vrtx=to;                        /* begining of the source */

 for(i=0;i<length;i++)                      /* for all edges */
 {
  v1=(int*)from; from+=dimension; v2=(int*)from;

  if(C_line_z_clipping(&v1,&v2,dimension))  /* clipping */
  {
   if(C_3D_clipping)                        /* depends which one was clipped */
   {
    HW_copy_int(v1,to,dimension); to+=dimension;
    HW_copy_int(v2,to,dimension); to+=dimension;
    new_lng+=2;                             /* first point clipped */
   }
   else
   {
    HW_copy_int(v2,to,dimension); to+=dimension;
    new_lng++;                              /* second point clipped */
   }
  }
 }
 HW_copy_int(first_vrtx,to,dimension);      /* looping the polygon vertices */

 return(new_lng);
}

/**********************************************************\
 * Line clipping using binary search technique.           *
 *                                                        *
 * RETURNS: 0 when line is compleately outside;           *
 * -------- 1 otherwise.                                  *
 *                                                        *
 * SETS:  C_3D_clipping to 1 when first or both points    *
 * -----  were changed; 0 otherwise.                      *
\**********************************************************/

int C_line_xyz_clipping(int **vertex1,int **vertex2,
                        int *by1,int *by2,int *by3,
                        int dimension
                       )
{
 int v1,v2,plane[4],tmp[3];                 /* plane equation */
 register int i;
 register int whereto;
 register int *l,*r,*m,*t;                  /* left right midle and tmp */
 static int c_store0[C_MAX_DIMENSIONS];     /* static stores for clipped vxes */
 static int c_store1[C_MAX_DIMENSIONS];
 static int c_store2[C_MAX_DIMENSIONS];
 int **vmn,**vmx;                           /* so that *vmn[3] < *vmx[3] */
 int swap,itmp;                             /* were coordinates swaped? */

 C_3D_clipping=0;                           /* default no clipping yet */

 T_plane(by1,by2,by3,plane);
 v1=T_vertex_on_plane(*vertex1,plane);
 v2=T_vertex_on_plane(*vertex2,plane);

 if(v1<v2)
 { swap=0; vmn=vertex1; vmx=vertex2; }
 else
 { swap=1; vmn=vertex2; vmx=vertex1; itmp=v1; v1=v2; v2=itmp; }

 if(v2<=0) return(0);                       /* compleately outside */
 else
 {
  if(v1<=0)                                 /* clipping */
  {
   HW_copy_int(*vmn,l=c_store2,dimension);  /* copying old vertices */
   HW_copy_int(*vmn,m=c_store0,dimension);  
   HW_copy_int(*vmx,r=c_store1,dimension);

   whereto=1;
   while((T_vertex_on_plane(m,plane)!=0)&&  /* while not on the plane and */
         (T_norm(T_vector(l,r,tmp,3))>C_TOLERANCE)
        )                                   /* not close enough */
   {
    if(whereto==1) { t=l; l=m; m=t; }
    else           { t=r; r=m; m=t; }
    for(i=0;i<dimension;i++) m[i]=(l[i]+r[i])>>1;
    whereto=T_vertex_on_plane(m,plane)<0;
   }
   *vmn=m;                                  /* that is why m[] is static */
   C_3D_clipping=swap^1;
  }
  return(1);                                /* partialy or not clipped */
 }
}

/**********************************************************\
 * Creating a polygon clipped by any plane.               *
 *                                                        *
 * RETURNS: number of vertices in the clipped polygon.    *
 * -------- (0 when compleately clipped)                  *
 *                                                        *
 *                       1-2-3-4-5-6-1  -> 2'-3'-5'-6'    *
 *        \ 5'                                            *
 *      5*-*------*6     If first vertex in the line is   *
 *      /   \       \    being clipped both vertices are  *
 *    4*     \       *1  copyed If no clipping or second  *
 *      \     \ 3'  /    one is clipped then only second  *
 *      3*-----*--*2     one is copyed. If both vertices  *
 *              \        are clipped, neither is copyed.  *
 *                                                        *
\**********************************************************/

int C_polygon_xyz_clipping(int *from,register int *to,
                           int *by1,int *by2,int *by3,
                           int dimension,int length
                          )
{
 register int i;
 int *v1,*v2,new_lng=0;
 int *first_vrtx=to;                        /* begining of the source */

 for(i=0;i<length;i++)                      /* for all edges */
 {
  v1=(int*)from; from+=dimension; v2=(int*)from;

  if(C_line_xyz_clipping(&v1,&v2,by1,by2,by3,dimension))
  {                                         /* clipping */
   if(C_3D_clipping)                        /* depends which one was clipped */
   {
    HW_copy_int(v1,to,dimension); to+=dimension;
    HW_copy_int(v2,to,dimension); to+=dimension;
    new_lng+=2;                             /* first point clipped */
   }
   else
   {
    HW_copy_int(v2,to,dimension); to+=dimension;
    new_lng++;                              /* second point clipped */
   }
  }
 }
 HW_copy_int(first_vrtx,to,dimension);      /* looping the polygon vertices */

 return(new_lng);
}

/**********************************************************/
