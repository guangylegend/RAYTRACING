#if !defined(_VECTOR_)
#define _VECTOR_

/** 3DGPL *************************************************\
 * ()                                                     *
 * Header for operations on vectors used by the ray       *
 * tracer.                                                *
 *                                                        *
 * Files:                                                 *
 *  vector.c                 Operations on vectors.       *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/




//三维矢量，呵呵
#define V_LNG_VECTOR 3                      /* X Y Z */

//零矢量
float *V_zero(float *vec);

//矢量的赋值
float *V_vector_coordinates(float *vector,float x,float y,float z);

//矢量的计算(from & to)
float *V_vector_points(float *vector,float *from,float *to);

//to what?设置矢量V Set
float *V_set(float *what,float *to);

//矢量倍增
float *V_multiply(float *result,float *vector,float m);

//两个向量的积
float V_scalar_product(float *a,float *b);

//这个是什么计算???不记得了
float* V_vector_product(float *product,float *a,float *b);

//矢量和
float *V_sum(float *sm,float *a,float *b);

//矢量差
float *V_difference(float *differ,float *a,float *b);

//矢量单位化
float *V_unit_vector(float *vector,float *from,float *to);

//计算一个平面等式的系数
float *V_plane(float *plane,float *normal,float *origine);

//检查点是否在平面上
float V_vertex_on_plane(float *plane,float *vertex);

/**********************************************************/

#endif
