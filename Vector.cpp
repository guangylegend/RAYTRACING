/** 3DGPL *************************************************\
 * ()                                                     *
 * Header for operations on vectors used by the ray       *
 * tracer.                                                *
 *                                                        *
 * Defines:                                               *
 *  V_zero                   Initialize a vector;         *
 *  V_vector_coordinates     Set the vector's coordinates;*
 *  V_vector_points          Make a vector from points;   *
 *  V_set                    Copy a vector;               *
 *  V_multiply               Multiply by a scalar;        *
 *  V_scalar_product         Find a scalar product;       *
 *  V_vector_product         Find a vector product;       *
 *  V_sum                    Sum two vectors;             *
 *  V_difference             Compute difference;          *
 *  V_unit_vector            Compute unit vector;         *
 *  V_plane                  Coeffs of a plane equation;  *
 *  V_vertex_on_plane        Checks if in the plane.      *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//Vector.cpp

#include <math.h>                           /* sqrt */
#include "vector.h"               /* self definition */

/**********************************************************\
 * Initialize a vector to zero.                           *
 *                                                        *
 * RETURNS: Initializeed vector.                          *
 * --------                                               *
\**********************************************************/

float *V_zero(float *vec)
{
//Ò»¸öfloatÐÍµÄÖ¸ÕëºÍÒ»¸öÊ¸Á¿ÓÐÊ²Ã´¹ØÏµå

 vec[0]=0; vec[1]=0; vec[2]=0; return(vec);
}

/**********************************************************\
 * Sets the coordinates of a vector.                      *
 *                                                        *
 * RETURNS: Resulting vector.                             *
 * --------                                               *
\**********************************************************/

float *V_vector_coordinates(float *vector,float x,float y,float z)
{
 vector[0]=x;
 vector[1]=y;
 vector[2]=z;
 return(vector);
}

/**********************************************************\
 * Computes a vector base on the coordinates of the       *
 * beginning and the end.                                 *
 *                                                        *
 * RETURNS: Resulting vector.                             *
 * --------                                               *
\**********************************************************/

float *V_vector_points(float *vector,float *from,float *to)
{
 vector[0]=to[0]-from[0];
 vector[1]=to[1]-from[1];
 vector[2]=to[2]-from[2];
 return(vector);
}

/**********************************************************\
 * Copy a vector from another vector.                     *
 *                                                        *
 * RETURNS: Resulting vector.                             *
 * --------                                               *
\**********************************************************/

float *V_set(float *what,float *to)
{
 what[0]=to[0]; what[1]=to[1]; what[2]=to[2];
 return(what);
}

/**********************************************************\
 * Multiply a vector by a scalar number.                  *
 *                                                        *
 * RETURNS: Resulting vector.                             *
 * --------                                               *
\**********************************************************/

float *V_multiply(float *result,float *vector,float m)
{
 result[0]=vector[0]*m; result[1]=vector[1]*m; result[2]=vector[2]*m;
 return(result);
}

/**********************************************************\
 * Computes a scalar product of two vectors.              *
 *                                                        *
 * RETURNS: Scalar product.                               *
 * --------                                               *
\**********************************************************/

float V_scalar_product(float *a,float *b)
{
 return(a[0]*b[0]+a[1]*b[1]+a[2]*b[2]);
}

/**********************************************************\
 * Computes a vector product of two vectors.              *
 *                                                        *
 * RETURNS: Vector product.                               *
 * --------                                               *
\**********************************************************/

float* V_vector_product(float *product,float *a,float *b)
{
 product[0]=a[1]*b[2]-a[2]*b[1];
 product[1]=a[2]*b[0]-a[0]*b[2];
 product[2]=a[0]*b[1]-a[1]*b[0];

 return(product);
}

/**********************************************************\
 * Computes a sum of two vectors.                         *
 *                                                        *
 * RETURNS: Resulting vector.                             *
 * --------                                               *
\**********************************************************/

float *V_sum(float *sm,float *a,float *b)
{
 sm[0]=a[0]+b[0]; sm[1]=a[1]+b[1]; sm[2]=a[2]+b[2];
 return(sm);
}

/**********************************************************\
 * Computes difference of two vectors.                    *
 *                                                        *
 * RETURNS: Resulting vector.                             *
 * --------                                               *
\**********************************************************/

float *V_difference(float *differ,float *a,float *b)
{
 differ[0]=a[0]-b[0]; differ[1]=a[1]-b[1]; differ[2]=a[2]-b[2];
 return(differ);
}

/**********************************************************\
 * Computes a unit length vector base on the coordinates  *
 * of the beginning and the end.                          *
 *                                                        *
 * RETURNS: Resulting vector.                             *
 * --------                                               *
\**********************************************************/

float *V_unit_vector(float *vector,float *from,float *to)
{
 float lng;
 vector[0]=to[0]-from[0]; vector[1]=to[1]-from[1]; vector[2]=to[2]-from[2];
 lng=sqrt(vector[0]*vector[0]+vector[1]*vector[1]+vector[2]*vector[2]);
 vector[0]/=lng; vector[1]/=lng; vector[2]/=lng;
 //·µ»ØµÄÊÇµ¥Î»»¯¹ýµÄ·½ÏòÊ¸Á¿
 return(vector);
}

/**********************************************************\
 * Computes coefficients of a plane equation.             *
 *                                                        *
 * RETURNS: Pointer to the coefficients.                  *
 * --------                                               *
\**********************************************************/

float *V_plane(float *plane,float *normal,float *origine)
{
 plane[0]=normal[0];
 plane[1]=normal[1];
 plane[2]=normal[2];
 plane[3]=-(origine[0]*normal[0]+origine[1]*normal[1]+origine[2]*normal[2]);

 return(plane);
}

/**********************************************************\
 * Checking if a vertex belongs to the plane.             *
 *                                                        *
 * RETURNS: 0   when the vertex belongs to the plane;     *
 * -------- "+" when in the normal pointed                *
 *              halfplane;                                *
 *          "-" otherwise.                                *
\**********************************************************/

float V_vertex_on_plane(float *plane,float *vertex)
{
 return(vertex[0]*plane[0]+
        vertex[1]*plane[1]+
        vertex[2]*plane[2]+
        plane[3]
       );
}

/**********************************************************/
