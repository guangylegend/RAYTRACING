#if !defined(_CLIPPER_H_)
#define _CLIPPER_H_

/** 3DGPL *************************************************\
 * ()                                                     *
 * Header for clipping functions.                         *
 *                                                        *
 * Files:                                                 *
 *  clipp-2d.c               3-D volume and Z clipping;   *
 *  clipp-3d.c               2-D plane clipping.          *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/

#include "LightTrack.h"           /* screen dimensions */

#define C_MAX_DIMENSIONS               8    /* X Y Z R G B Tx Ty */

#define C_Z_CLIPPING_MIN              10    /* where z plane is */
#define C_X_CLIPPING_MIN               0    /* clipping cube */
#define C_X_CLIPPING_MAX HW_SCREEN_X_MAX
#define C_Y_CLIPPING_MIN               0
#define C_Y_CLIPPING_MAX HW_SCREEN_Y_MAX
#define C_TOLERANCE                    1    /* when to stop xyz clipping */

void C_init_clipping(int minx,int miny,int maxx,int maxy);
void C_get_bounds(int *minx,int *miny,int *maxx,int *maxy);
int C_set_bounds(int minx,int miny,int maxx,int maxy);
int C_line_x_clipping(int **vertex1,int **vertex2,int dimension);
int C_line_y_clipping(int **vertex1,int **vertex2,int dimension);
int C_polygon_x_clipping(int *from,int *to,int dimension,int length);

int C_volume_clipping(int *min,int *max);
int C_line_z_clipping(int **vertex1,int **vertex2,int dimension);
int C_polygon_z_clipping(int *from,int *to,int dimension,int length);
int C_line_xyz_clipping(int **vertex1,int **vertex2,
                        int *by1,int *by2,int *by3,
                        int dimension
                       );
int C_polygon_xyz_clipping(int *from,int *to,
                           int *by1,int *by2,int *by3,
                           int dimension,int length
                          );

/**********************************************************/

#endif
