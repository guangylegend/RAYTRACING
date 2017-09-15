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




//��άʸ�����Ǻ�
#define V_LNG_VECTOR 3                      /* X Y Z */

//��ʸ��
float *V_zero(float *vec);

//ʸ���ĸ�ֵ
float *V_vector_coordinates(float *vector,float x,float y,float z);

//ʸ���ļ���(from & to)
float *V_vector_points(float *vector,float *from,float *to);

//to what?����ʸ��V Set
float *V_set(float *what,float *to);

//ʸ������
float *V_multiply(float *result,float *vector,float m);

//���������Ļ�
float V_scalar_product(float *a,float *b);

//�����ʲô����???���ǵ���
float* V_vector_product(float *product,float *a,float *b);

//ʸ����
float *V_sum(float *sm,float *a,float *b);

//ʸ����
float *V_difference(float *differ,float *a,float *b);

//ʸ����λ��
float *V_unit_vector(float *vector,float *from,float *to);

//����һ��ƽ���ʽ��ϵ��
float *V_plane(float *plane,float *normal,float *origine);

//�����Ƿ���ƽ����
float V_vertex_on_plane(float *plane,float *vertex);

/**********************************************************/

#endif
