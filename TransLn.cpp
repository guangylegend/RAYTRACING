/** 3DGPL *************************************************\
 * ()                                                     *
 * 3-D linear algebra stuff.                              *
 *                                                        *
 * Defines:                                               *
 *  T_vector                 Construct from two points;   *
 *  T_norm                   Norm (length) of a vector;   *
 *                                                        *
 *  T_normal_vectors         From two vectors;            *
 *  T_normal_plane           From three points;           *
 *  T_unit_vector            Normalising a vector;        *
 *  T_scalar_product         Computing the product;       *
 *  T_normal_z_negative      Z component of the normal;   *
 *                                                        *
 *  T_plane                  Coefs for plane equation;    *
 *  T_vertex_on_plane        Vertex belongs to a plane?.  *
 *                                                        *
 * Internal:                                              *
 *  TI_sqrt                  Integer square root.         *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//TransLn.cpp

#include "Trans.h"                 /* 3D mathematics */
#include "LightTrack.h"           /* HW_error */
#include <math.h>                           /* float sqrt */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * INTERNAL: Computes square root for a long integer     *
 * --------- using iterative bit setting.                *
 *                                                       *
 * RETURNS: The square root.                             *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if defined(_FIXED_)
unsigned long TI_sqrt(register unsigned long arg)   
{                                           /* using iterative method */
 register int i;
 register unsigned long nprd,msk=0x8000L,val=0,prd=0;

 for(i=15;i>=0;i--)
 {                                          /* iteratively computing the */
  nprd=prd+(val<<(i+1))+(msk<<i);           /* square */
  if(nprd<=arg) { val|=msk; prd=nprd; }     /* this bit must be in the result */
  msk>>=1;                                  /* next, a lower bit */
 }
 return(val);
}
#endif

/**********************************************************\
 * Build a vector from two points.                        *
 *                                                        *
 * RETURNS: Always a pointer to the destanation.          *
 * --------                                               *
\**********************************************************/

int *T_vector(int *from1,int *from2,int *to,int dimension)
{
 to[0]=(*from2++)-(*from1++);
 to[1]=(*from2++)-(*from1++);
 if(dimension==3) to[2]=(*from2)-(*from1);
 return(to);
}

/**********************************************************\
 * Compute norm (length) of the vector.                   *
 *                                                        *
 * RETURNS: Length of the vector.                         *
 * --------                                               *
\**********************************************************/

long T_norm(int *vector)
{
#if defined(_FLOAT_)
 return(sqrt(((float)vector[0])*vector[0]+
             ((float)vector[1])*vector[1]+
             ((float)vector[2])*vector[2]
            )
       );
#endif
#if defined(_FIXED_)
 if((vector[0]>0xffff)||(vector[1]>0xffff)||(vector[2]>0xffff)||
    (vector[0]<-0xffff)||(vector[1]<-0xffff)||(vector[2]<-0xffff)
   )
 {
  return(TI_sqrt(((long)vector[0]>>8)*vector[0]+
                 ((long)vector[1]>>8)*vector[1]+
                 ((long)vector[2]>>8)*vector[2]
                )<<4                        /* to avoid an overflow: */
        );                                  /* sqrt(a/2**8)==sqrt(a)/2**4 */
 }
 else
 {
  return(TI_sqrt(((long)vector[0])*vector[0]+
                 ((long)vector[1])*vector[1]+
                 ((long)vector[2])*vector[2]
                )
        );
 }
#endif
}

/**********************************************************\
 * Computing a normal through a vector product of two     *
 * vectors.                                               *
 *       _  _  _                                          *
 *      |i  j  k |   _              _                     *
 * N=det|Vx Vy Vz| = i*det|Vy Vz| - j*det|Vx Vz| +        *
 *      |Wx Wy Wz|        |Wy Wz|        |Wx Wz|          *
 *   _                                                    *
 * + k*det|Vx Vy|                                         *
 *        |Wx Wy|                                         *
 *                                                        *
\**********************************************************/

void T_normal_vectors(int *v,int *w,int *to)
{
 int n[3];
 long lng;

 n[0]=v[1]*w[2]-v[2]*w[1];
 n[1]=v[2]*w[0]-v[0]*w[2];
 n[2]=v[0]*w[1]-v[1]*w[0];                  /* a vector product */

 lng=T_norm(n);                             /* vector's length */

 if(lng==0) HW_error("(Trans) Found a zero length vector.");

 *to++=(int)((((long)n[0])<<T_LOG_NORMAL_SIZE)/lng);
 *to++=(int)((((long)n[1])<<T_LOG_NORMAL_SIZE)/lng);
 *to  =(int)((((long)n[2])<<T_LOG_NORMAL_SIZE)/lng);
}

/**********************************************************\
 * Computing a normal through a vector product of the     *
 * vectors built on three points passed.                  *
 *       _  _  _                                          *
 *      |i  j  k |   _              _                     *
 * N=det|Vx Vy Vz| = i*det|Vy Vz| - j*det|Vx Vz| +        *
 *      |Wx Wy Wz|        |Wy Wz|        |Wx Wz|          *
 *   _                                                    *
 * + k*det|Vx Vy|                                         *
 *        |Wx Wy|                                         *
 *                                                        *
\**********************************************************/

void T_normal_plane(int *from1,int *from2,int *from3,
                    int *to
                   )
{
 int v[3],w[3],n[3];
 long lng;

 T_vector(from1,from2,v,3);
 T_vector(from1,from3,w,3);

 n[0]=v[1]*w[2]-v[2]*w[1];
 n[1]=v[2]*w[0]-v[0]*w[2];
 n[2]=v[0]*w[1]-v[1]*w[0];                  /* a vector product */

 lng=T_norm(n);                             /* vector's length */

 if(lng==0) HW_error("(Trans) Found a zero length vector.");

 *to++=(int)((((long)n[0])<<T_LOG_NORMAL_SIZE)/lng);
 *to++=(int)((((long)n[1])<<T_LOG_NORMAL_SIZE)/lng);
 *to  =(int)((((long)n[2])<<T_LOG_NORMAL_SIZE)/lng);
}

/**********************************************************\
 * Computing a unit length vector based on two points.    *
\**********************************************************/

void T_unit_vector(int *from1,int *from2,int *to)
{
 int v[3];
 long lng;

 T_vector(from1,from2,v,3);                 /* vector V */

 lng=T_norm(v);                             /* length */

 if(lng==0) HW_error("(Trans) Found a zero length vector.");

 *to++=(int)((((long)v[0])<<T_LOG_NORMAL_SIZE)/lng);
 *to++=(int)((((long)v[1])<<T_LOG_NORMAL_SIZE)/lng);
 *to  =(int)((((long)v[2])<<T_LOG_NORMAL_SIZE)/lng);
}

/**********************************************************\
 * Computing the scalar product of two unit vectors.      *
 * which were assumed to be built with T_LOG_NORMAL_SIZE  *
 * precision.                                             *
 *                                                        *
 * RETURNS: The scalar product.                           *
 * --------                                               *
\**********************************************************/

int T_scalar_product(int *vector1,int *vector2)
{
 return(((long)vector1[0]*(long)vector2[0]+
         (long)vector1[1]*(long)vector2[1]+
         (long)vector1[2]*(long)vector2[2]
        )>>T_LOG_NORMAL_SIZE
       );
}

/**********************************************************\
 * Z component of the normal.                             *
 *                                                        *
 * RETURNS: 1 if z component negative, 0 otherwise.       *
 * --------                                               *
\**********************************************************/

int T_normal_z_negative(int *from1,int *from2,int *from3)
{
 return((((long)(from2[0]-from1[0]))*(from3[1]-from1[1])-
         ((long)(from2[1]-from1[1]))*(from3[0]-from1[0]))<0
       );                                   /* component of vector product */
}

/**********************************************************\
 * Computing coeficients for a plane equation.            *
 *                                                        *
 *        ^ N---------+   (N)(x-x0)==0                    *
 *        |    *x   /   Nx(x-x0)+Ny(y-y0)+Nz(z-z0)==0     *
 *      / |  /    /                                       *
 *    /   |/x0  /   x*Nx+y*Ny+z*Nz-(Nx*x0+Ny*y0+Nz*z0)==0 *
 *  +----------+                                          *
 *                                                        *
\**********************************************************/

void T_plane(int *from1,int *from2,int *from3,int *to)
{
 int n[3];
 T_normal_plane(from1,from2,from3,n);       /* normal based on three points */

 *to++=n[0]; *to++=n[1]; *to++=n[2];        /* plane coeficients */
 *to=-(from1[0]*n[0]+from1[1]*n[1]+from1[2]*n[2]);
}

/**********************************************************\
 * Putting vertex into a plane equation Ax+By+Cz+D        *
 *                                                        *
 * RETURNS: 0   if a point belongs to a plane;            *
 * -------- "+" if a point is on the normal pointed side; *
 *          "-" if on the other side.                     *
\**********************************************************/

int T_vertex_on_plane(int *vertex,int *plane)
{
 return(vertex[0]*plane[0]+
        vertex[1]*plane[1]+
        vertex[2]*plane[2]+
        plane[3]
       );
}

/**********************************************************/
