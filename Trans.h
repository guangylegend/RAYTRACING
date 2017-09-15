#if ((!defined(_FLOAT_)) && (!defined(_FIXED_)))
 #error Either _FLOAT_ or _FIXED_ must be declared.
#endif
#if ((!defined(_Z_BUFFER_)) && (!defined(_PAINTER_)))
 #error Either _Z_BUFFER_ or _PAINTER_ must be declared.
#endif
#if !defined(_TRANS_H_)
#define _TRANS_H_

/** 3DGPL *************************************************\
 * ()                                                     *
 * Header for basic 3-d math.                             *
 *                                                        *
 * Ifdefs:                                                *
 *  _FLOAT_                 Use float implementation;     *
 *  _FIXED_                 Use fixed implementation;     *
 *  _Z_BUFFER_              Hidden surface removal;       *
 *  _PAINTER_               Back to front rendering;      *
 *                                                        *
 * Files:                                                 *
 *  trans-bs.c              3-D transformations;          *
 *  trans-ln.c              Linear algebra stuff.         *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/

#define T_LNG_VECTOR              4         /* 3 rounded up to power of 2 */
#define T_MAX_MATRIX_SIZE         8         /* for Gaussian elimination */
#define T_LOG_NORMAL_SIZE         7         /* fixed precision for normals */
#define T_ONE_OVER_Z_CONST -0x7ffff         /* for Z_BUFFER ordering */
#define T_P                      15         /* fixed point precision */

void T_init_math(void);
int *T_vector(int *from1,int *from2,int *to,int dimension);
long T_norm(int *vector);

void T_normal_vectors(int *from1,int *from2,int *to);
void T_normal_plane(int *from1,int *from2,int *from3,int *to);
void T_unit_vector(int *from1,int *from2,int *to);
int T_scalar_product(int *from1,int *from2);

int T_normal_z_negative(int *from1,int *from2,int *from3);
void T_plane(int *from1,int *from2,int *from3,int *to);
int T_vertex_on_plane(int *vertex,int *plane);

void T_translation(int *from,int *to,int length,
                   int addx,int addy,int addz
                  );
void T_scaling(int *from,int *to,int length,
               int mulx,int muly,int mulz
              );
void T_set_self_rotation(unsigned char alp,unsigned char bet,
                         unsigned char gam
                        );
void T_self_rotation(int *from,int *to,int length);
void T_set_world_rotation(unsigned char alp,unsigned char bet,
                          unsigned char gam
                         );
void T_world_rotation(int *from,int *to,int length);
void T_cancatinate_self_world(int addx,int addy,int addz);
void T_cancatinated_rotation(int *from,int *to,int length);
void T_perspective(int *from,int *to,
                   int dimension,int length,int log_focus
                  );

void T_linear_solve(int ia[T_MAX_MATRIX_SIZE][T_MAX_MATRIX_SIZE],
                    int ib[T_MAX_MATRIX_SIZE][T_MAX_MATRIX_SIZE],
                    int ix[T_MAX_MATRIX_SIZE][T_MAX_MATRIX_SIZE],
                    int n,int m,
                    int log_source_multiplyer,
                    int log_result_multiplyer
                   );

/**********************************************************/

#endif
