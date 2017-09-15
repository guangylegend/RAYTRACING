#if ((!defined(_CI_)) && (!defined(_RGB_)))
 #error Either _CI_ or _RGB_ must be declared.
#endif
#if ((!defined(_Z_BUFFER_)) && (!defined(_PAINTER_)))
 #error Either _Z_BUFFER_ or _PAINTER_ must be declared.
#endif
#if !defined(_ENGINE_H_)
#define _ENGINE_H_

/** 3DGPL *************************************************\
 * ()                                                     *
 * Header for the polygonal 3D engine.                    *
 *                                                        *
 * Ifdefs:                                                *
 *  _CI_                     Colour/Intensity model;      *
 *  _RGB_                    RGB model;                   *
 *  _Z_BUFFER_               Depth array;                 *
 *  _PAINTER_                Back-front order.            *
 *                                                        *
 * Files:                                                 *
 *  eng-base.c               Polymorphic polygon;         *
 *  eng-poly.c               Polygonal object;            *
 *  eng-bcub.c               Bicubic patch;               *
 *  eng-grup.c               Group of objects;            *
 *  eng-volm.c               Indoor volume object;        *
 *  eng-surf.c               Landscape object.            *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/

#include "Graphics.h"           /* G_MAX_POLYGON_VERTICES */
#include "Trans.h"                 /* T_LNG_VECTOR */
#include "Colour.h"               /* CL_LNG_COLOUR */

extern unsigned char M_camera_gam;          /* needed for surfaces */
extern int M_camera_x,M_camera_y,M_camera_z;
extern int M_camera_log_focus;              /* camera parameters */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * The following semi-internal structures are            *
 * used by the structures representing various 3D        *
 * objects:                                              *
 *                                                       *
 *  M_texture                Texture and its size;       *
 *  M_polygon                A polygon;                  *
 *  M_light                  Ambient, point or direct;   *
 *  M_bicubic                A patch;                    *
 *  M_polygon_object_order   A BSP tree;                 *
 *  M_gate                   Gate between volumes;       *
 *  M_surface_cell           Element of the surface.     *
 *                                                       *
 * This external structures modeling various 3D objects: *
 *                                                       *
 *  M_polygon_object         Polygonal solid;            *
 *  M_bicubic_object         Bicubic solid;              *
 *  M_group                  Set of above models;        *
 *  M_volume_object          Interconnected volumes;     *
 *  M_surface_object         A landscape.                *
 *                                                       *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if defined(_CI_)                           /* colour/intensity model */
 #define M_IDX_TEXTURE     4                /* where Tx is */

 #define M_LNG_FLAT        3                /* X Y Z */
 #define M_LNG_SHADED      4                /* X Y Z I */
 #define M_LNG_TEXTURED    6                /* X Y Z I Tx Ty */
#endif
#if defined(_RGB_)                          /* RGB model */
 #define M_IDX_TEXTURE     6                /* where Tx is */

 #define M_LNG_FLAT        3                /* X Y Z */
 #define M_LNG_SHADED      6                /* X Y Z R G B */
 #define M_LNG_TEXTURED    8                /* X Y Z R G B Tx Ty */
#endif

#define M_P              G_P                /* fixed point precision */
#define M_PB               8                /* for bicubic rasterization */

#define M_WIRE          0x01                /* a wire frame */
#define M_FLAT          0x02                /* constant colour polygon */
#define M_SHADED        0x04                /* Gouraud shaded polygon */
#define M_TEXTURED      0x08                /* texture mapped polygon */

extern int M_force_linear_tmapping;         /* when the polygons are small */

void M_init_rendering(int type);            /* highest drawable polygon type */
void M_set_camera(unsigned char alp,unsigned char bet,unsigned char gam,
                  int x,int y,int z,int log_focus
                 );                         /* parameters of the camera */
#ifdef _PAINTER_
void M_sort_elements(int *vertices,int dimension,int *indices,int number);
#endif

/**********************************************************\
 * A light source, either a point light source or a       *
 * directional light source. In the former cases the      *
 * parameter describes the position in the latter case    *
 * the orientation vector.                                *
 *                                                        *
 *  +------------------------+                            *
 *  | m_type                 | M_AMBIENT|M_POINT|M_DIRECT *
 *  |                  +---+---+---+                      *
 *  | m_parameter      | X | Y | Z |                      *
 *  |                  +---+---+---+                      *
 *  | [m_intensity] or       |                            *
 *  | [m_red m_green m_blue] |                            *
 *  +------------------------+                            *
 *                                                        *
\**********************************************************/

#define M_AMBIENT 0x1                       /* ambient light source */
#define M_POINT   0x2                       /* point light source */
#define M_DIRECT  0x4                       /* directional light source */

struct M_light                              /* describes a light source */
{
 int m_type;                                /* M_AMBIENT,M_POINT or M_DIRECT */

 int m_parameter[T_LNG_VECTOR];             /* location of the light */

#if defined(_CI_)
 int m_intensity;                           /* ambient intensity */
#endif
#if defined(_RGB_)
 int m_red;                                 /* intensity as RGB */
 int m_green;
 int m_blue;
#endif
};

void M_shade_vertex(int *intensity,
                    int *vertex,int *normal,
                    int no_lights,struct M_light **lights
                   );

/**********************************************************\
 * Texture, basically only square textures with the size  *
 * which is a power of two are allowed.                   *
 *                                                        *
 *  +--------------------+        +-------------+         *
 *  | m_texture_bytes------------>|texture bytes|         *
 *  | m_log_texure_size  |        |     ...     |         *
 *  +--------------------+        +-------------+         *
 *                                                        *
\**********************************************************/

struct M_texture
{
 HW_pixel *m_texture_bytes;                 /* points raw data */
 int m_log_texture_size;                    /* log base 2 of texture size */
};

/**********************************************************\
 * Polygon, either actual plane patch or part of some     *
 * curved surface approximation. Has a colour or a        *
 * texture. The illumination intensities for the hole     *
 * patch or per vertex are set by object shading          *
 * function. Vertices point to an array each element of   *
 * which contains info relative to a vertex. NB whatever  *
 * the type of the polygon is ALL parameters must be      *
 * present. Texture coordinates are related to            *
 * log_texture_...                                        *
 *                                                        *
 *  +--------------------------+                          *
 *  | m_type                   |   M_PLANNAR | M_CURVED   *
 *  |                          |                          *
 *  | m_colour                 |                          *
 *  |                          |                          *
 *  | [ m_intensity ] or       |                          *
 *  | [ m_red m_green m_blue ] |                          *
 *  |                          |                          *
 *  | m_log_texture_space_size | +-------------------+    *
 *  | m_texture----------------->|M_texture structure|    *
 *  |                          | +-------------------+    *
 *  |                          |                          *
 *  | m_no_edges               | +-----+----------+--+--+ *
 *  | m_vertices---------------->|Index|[I]or[RGB]|Tx|Ty| *
 *  |                          | |- - - - - - - - - - - | *
 *  |                          | |           ...        | *
 *  |                          | +----------------------+ *
 *  |                          |                          *
 *  |                          | +-----+                  *
 *  | m_normals----------------->|Index|                  *
 *  +--------------------------+ |- - -|                  *
 *                               | ... |                  *
 *                               +-----+                  *
\**********************************************************/

#define M_PLANNAR              0x01         /* polygon models a plane */
#define M_CURVED               0x02         /* polygon models a surface */

#define M_QUAD_XY              0x04         /* easy to find texture vectors */
#define M_QUAD_MINUS_XY        0x08         /* same but order reversed */
#define M_QUAD                 0x0c         /* any of two above */
#define M_NOT_QUAD             0xf3         /* to get rid of M_QUAD in splits */

#define M_MAX_POLYGON_VERTICES G_MAX_POLYGON_VERTICES

#if defined(_CI_)
 #define M_IDX_POLYGON_TEXTURE 2            /* where texture is in the list */
 #define M_LNG_POLYGON_VERTEX  4            /* Idx I Tx Ty */
#endif
#if defined(_RGB_)
 #define M_IDX_POLYGON_TEXTURE 4            /* where texture is in the list */
 #define M_LNG_POLYGON_VERTEX  6            /* Idx R G B Tx Ty */
#endif

struct M_polygon                            /* describes one polygon */
{
 int m_type;                                /* M_PLANNAR | M_CURVED */

 HW_pixel m_colour;                         /* only for M_AMBIENT */

#if defined(_CI_)
 int m_intensity;                           /* ambient intensity */
#endif
#if defined(_RGB_)
 int m_red;                                 /* intensity as RGB */
 int m_green;
 int m_blue;
#endif

 int m_log_texture_space_size;              /* mapping scale */
 struct M_texture *m_texture;               /* raw data with sizes */

 int m_no_edges;                            /* number of edges in the polygn */
 int *m_vertices;                           /* array of indices */
 int *m_normals;                            /* normals for the vertices */
};

void M_shade_polygon(struct M_polygon *polygon,int *vertices,
                     int *normals,int no_lights,
                     struct M_light **lights
                    );
void M_render_polygon(struct M_polygon *polygon,int *vertices,
                      int *colours,int *textures
                     );

/**********************************************************\
 * Represents a single bicubic Bezier patch. It is shaded *
 * using descretely precalculated normals (appears to be  *
 * not that expensive compared to other methods). And     *
 * rasterized through turning the controls finding the    *
 * mesh and rasterizing the small polygons one at a time. *
 *                                                        *
 *  +--------------------------+                          *
 *  | m_log_render_size        |                          *
 *  |                          |                          *
 *  | m_colour                 |                          *
 *  | m_log_texture_space_size | +-------------------+    *
 *  | m_texture----------------->|M_texture structure|    *
 *  |                          | +-------------------+    *
 *  |                          |                          *
 *  |                   +---+---+---+                     *
 *  | m_controls        | X | Y | Z |                     *
 *  |                   |- - - - - -|                     *
 *  |                   | ...16...  |                     *
 *  |                   +-----------+                     *
 *  |                          |                          *
 *  |                          | +---+---+---+            *
 *  | m_normals----------------->| X | Y | Z |            *
 *  |                          | |- - - - - -|            *
 *  |                          | |    ...    |            *
 *  |                          | +-----------+            *
 *  |                          |                          *
 *  |                          | +------------+           *
 *  | m_intensities------------->| I or R G B |           *
 *  +--------------------------+ |- - - - - - |           *
 *                               |     ...    |           *
 *                               +------------+           *
 *                                                        *
\**********************************************************/

#define M_MAX_RENDER_SIZE 33                /* 2**5+1 */

struct M_bicubic                            /* describes one polygon */
{
 int m_log_render_size;                     /* rasterized into polygons */

 HW_pixel m_colour;                         /* for M_SHADED and M_FLAT */
 int m_log_texture_space_size;              /* mapping scale */
 struct M_texture *m_texture;               /* raw data with sizes */

 int m_controls[16*T_LNG_VECTOR];           /* array of indices */
 int *m_normals;                            /* normals for shading */
 int *m_intensities;                        /* shading intensities */
};

void M_init_bicubic(struct M_bicubic *bicubic);
void M_shade_bicubic(struct M_bicubic *bicubic,
                     int x,int y,int z,
                     int alp,int bet,int gam,
                     int no_lights,
                     struct M_light **lights
                    );
void M_render_bicubic(struct M_bicubic *bicubic,
                      int x,int y,int z,
                      int alp,int bet,int gam
                     );

/**********************************************************\
 * Polygon order object, a BSP tree.                      *
 *                                                        *
 *  +-----------------+    +-----------+                  *
 *  | m_root-------------->| M_polygon |                  *
 *  |                 |    +-----------+                  *
 *  |                 |    +------------------------+     *
 *  | m_positive---------->| M_polygon_object_order |     *
 *  |                 |    +------------------------+     *
 *  |                 |    +------------------------+     *
 *  | m_negative---------->| M_polygon_object_order |     *
 *  +-----------------+    +------------------------+     *
 *                                                        *
\**********************************************************/

struct M_polygon_object_order
{
 struct M_polygon *m_root;                  /* root polygon */
 struct M_polygon_object_order *m_positive; /* one sub tree */
 struct M_polygon_object_order *m_negative; /* another one */
};

/**********************************************************\
 * Gate between volume objects, like a polygon specified  *
 * by a chain of vertices and pointing to the gate which  *
 * it leads to.                                           *
 *                                                        *
 *  +-------------------+      +-----------------+        *
 *  | m_volume---------------->| M_volume_object |        *
 *  |                   |      +-----------------+        *
 *  |                   |                                 *
 *  | m_no_edges        |      +-------+                  *
 *  | m_vertices-------------->| Index |                  *
 *  |                   |      |       |                  *
 *  +-------------------+      |- - - -|                  *
 *                             |  ...  |                  *
 *                             +-------+                  *
 *                                                        *
\**********************************************************/

struct M_gate
{
 struct M_volume_object *m_volume;          /* gate into this volume */

 int m_no_edges;
 int *m_vertices;                           /* shape of the gate */
};

int M_set_gate(struct M_gate *gate,int *vertices);

/**********************************************************\
 * Surface cell, specifies a single cell of a landscape   *
 * object. Can model either plannar area or curved. In    *
 * the latter case modeled by two triangles instead of    *
 * a single rectangle.                                    *
 *                                                        *
 *  +--------------------------------+                    *
 *  | m_type                         | M_PLANNAR|M_CURVED *
 *  |                                |                    *
 *  | m_colour_1                     |                    *
 *  | [ m_intensity_1 ] or           |                    *
 *  | [ m_red_1 m_green_1 m_blue_1 ] |                    *
 *  |                                |                    *
 *  | m_colour_2                     |                    *
 *  | [ m_intensity_2 ] or           |                    *
 *  | [ m_red_2 m_green_2 m_blue_2 ] |                    *
 *  |                                |    +-----------+   *
 *  | m_texture-------------------------->| M_texture |   *
 *  |                                |    | structure |   *
 *  |                                |    +-----------+   *
 *  +--------------------------------+                    *
 *                                                        *
\**********************************************************/

struct M_surface_cell
{
 int m_type;                                /* M_PLANNAR | M_CURVED */

 HW_pixel m_colour_1;                       /* base colour */
 HW_pixel m_colour_2;                       /* same for polyg #2 if M_CURVED */

#if defined(_CI_)
 int m_intensity_1;                         /* flat intensity */
 int m_intensity_2;
#endif
#if defined(_RGB_)
 int m_red_1,m_green_1,m_blue_1;            /* same for RGB model */
 int m_red_2,m_green_2,m_blue_2;
#endif

 struct M_texture *m_texture;               /* if any */
};

/**********************************************************\
 * Polygonal object, a set of polygons on the common      *
 * vertex set. Also carries the set of normals, which     *
 * are, like the vertices, referenced by their respective *
 * polygons. When a BSP tree pointer is non NULL, it is   *
 * being used in hidden surface removal.                  *
 *                                                        *
 *  +-----------------+                                   *
 *  | m_type          |   M_POLYGON_OBJECT                *
 *  |                 |   +------------------------+      *
 *  | m_order- - - - - - >| M_polygon_object_order |      *
 *  |                 |   +------------------------+      *
 *  |                 |                                   *
 *  | m_no_polygons   |   +---+---+  - -+                 *
 *  | m_polygons--------->| o | o | ... |                 *
 *  |                 |   +-|-+-|-+ -  -+                 *
 *  |                 |     V   V                         *
 *  |              +---------+ +---------+                *
 *  |              |M_polygon| |M_polygon|                *
 *  |              +---------+ +---------+                *
 *  |                 |                                   *
 *  | m_no_vertices   |   +---+---+---+                   *
 *  | m_vertices--------->| X | Y | Z |                   *
 *  |                 |   |- - - - - -|                   *
 *  |                 |   |    ...    |                   *
 *  |                 |   +-----------+                   *
 *  |                 |                                   *
 *  | m_no_normals    |   +---+---+---+                   *
 *  | m_normals---------->| X | Y | Z |                   *
 *  +-----------------+   |- - - - - -|                   *
 *                        |    ...    |                   *
 *                        +-----------+                   *
 *                                                        *
\**********************************************************/

#define M_POLYGON_OBJECT 0x1                /* to id it in a group */

#define M_MAX_OBJECT_VERTICES 1024          /* size of tmp structures */
#define M_MAX_OBJECT_POLYGONS  256

struct M_polygon_object
{
 int m_type;                                /* always M_POLYGON_OBJECT */
 struct M_polygon_object_order *m_order;    /* BSP tree, if any */

 int m_no_polygons;
 struct M_polygon **m_polygons;             /* array of polygons */

 int m_no_vertices;
 int *m_vertices;                           /* array of coordinates */

 int m_no_normals;
 int *m_normals;                            /* array of coordinates */
};

void M_init_polygon_object(struct M_polygon_object *object);
void M_shade_polygon_object(struct M_polygon_object *object,
                            int x,int y,int z,
                            int alp,int bet,int gam,
                            int no_lights,
                            struct M_light **lights
                           );               /* object moves and rotates */
void M_render_polygon_object(struct M_polygon_object *object,
                             int x,int y,int z,
                             int alp,int bet,int gam
                            );

/**********************************************************\
 * Represents a collection of bicubic patches, hidden     *
 * surface elemination with _PAINTER_ option achieved     *
 * by back to front ordering of the patches (may very     *
 * well be wrong for many cases).                         *
 *                                                        *
 *  +--------------------------+                          *
 *  | m_type                   | M_BICUBIC_OBJECT         *
 *  |                          |                          *
 *  | m_no_patches             | +---+---+  - -+          *
 *  | m_patches----------------->| o | o | ... |          *
 *  |                          | +-|-+-|-+ -  -+          *
 *  |                          |   V   V                  *
 *  |                     +---------+ +---------+         *
 *  |                     | M_patch | | M_patch |         *
 *  |                     +---------+ +---------+         *
 *  |                          |                          *
 *  |                          | +---+---+---+            *
 *  | m_centres ---------------->| X | Y | Z |            *
 *  |                          | +- - - - - -+            *
 *  +--------------------------+ |    ...    |            *
 *                               +-----------+            *
 *                                                        *
 *                                                        *
\**********************************************************/

#define M_BICUBIC_OBJECT 0x2                /* to id it in a group */

#define M_MAX_PATCHES    128                /* upper for tmp structures */

struct M_generic_object
{
 int m_type;                                /* polygon or bicubic */
};

struct M_bicubic_object                     /* describes one polygon */
{
 int m_type;                                /* always M_POLYGON_OBJECT */

 int m_no_patches;
 struct M_bicubic **m_patches;              /* pointers to patches */
 int *m_centres;                            /* centres of the patches */
};

void M_init_bicubic_object(struct M_bicubic_object *bicubic);
void M_shade_bicubic_object(struct M_bicubic_object *object,
                            int x,int y,int z,
                            int alp,int bet,int gam,
                            int no_lights,
                            struct M_light **lights
                           );
void M_render_bicubic_object(struct M_bicubic_object *object,
                             int x,int y,int z,
                             int alp,int bet,int gam
                            );

/**********************************************************\
 * Group of objects, set of objects, each specified by    *
 * position and orientation. Also contains a set of light *
 * sources.                                               *
 *                                                        *
 *  +-------------------+                                 *
 *  | m_no_objects      |                                 *
 *  |                   |   +---+---+- -- --+             *
 *  | m_objects ----------->| o | o |  ...  |             *
 *  |                   |   +-|-+-|-+ - -- -+             *
 *  |                   |     V   V                       *
 *  |        +------------------+ +------------------+    *
 *  |        | M_generic_object | | M_generic_object |    *
 *  |        +------------------+ +------------------+    *
 *  |                   |                                 *
 *  |                   |   +---+---+---+                 *
 *  | m_centres ----------->| X | Y | Z |                 *
 *  |                   |   +- - - - - -+                 *
 *  |                   |   |    ...    |                 *
 *  |                   |   +-----------+                 *
 *  |                   |                                 *
 *  |                   |   +---+---+---+                 *
 *  | m_orientations ------>|alp|bet|gam|                 *
 *  |                   |   +- - - - - -+                 *
 *  |                   |   |    ...    |                 *
 *  |                   |   +-----------+                 *
 *  |                   |                                 *
 *  | m_no_lights       |   +---+---+- -- --+             *
 *  | m_lights ------------>| o | o |  ...  |             *
 *  |                   |   +-|-+-|-+ - -- -+             *
 *  +-------------------+     V   V                       *
 *                   +---------+ +---------+              *
 *                   | M_light | | M_light |              *
 *                   +---------+ +---------+              *
 *                                                        *
\**********************************************************/

#define M_MAX_GROUP_OBJECTS 32              /* defines size for tmp structs */

struct M_group
{
 int m_no_objects;
 struct M_generic_object **m_objects;       /* polygon or bicubic objects */
 int *m_centres;                            /* centres of the objects */
 int *m_orientations;                       /* their orientations */

 int m_no_lights;
 struct M_light **m_lights;                 /* light sources */
};

void M_init_group(struct M_group *group);   /* each object in the group */
void M_shade_group(struct M_group *group);  /* group doesn't move */
void M_render_group(struct M_group *object,
                    int x,int y,int z
                   );                       /* render at x y z */

/**********************************************************\
 * Indoor volume object, similar in many respects to      *
 * polygonal object, except that it contains a set of     *
 * gates to the volumes which it is connected to.         *
 *                                                        *
 *  +--------------------+                                *
 *  | m_no_polygons      |    +---+---+  - -+             *
 *  | m_polygons------------->| o | o | ... |             *
 *  |                    |    +-|-+-|-+ -  -+             *
 *  |                    |      V   V                     *
 *  |                  +---------+ +---------+            *
 *  |                  |M_polygon| |M_polygon|            *
 *  |                  +---------+ +---------+            *
 *  |                    |                                *
 *  | m_no_gates         |    +---+---+ -  -+             *
 *  | m_gates---------------->| o | o | ... |             *
 *  |                    |    +-|-+-|-+  - -+             *
 *  |                    |      V   V                     *
 *  |                    | +------+ +------+              *
 *  |                    | |M_gate| |M_gate|              *
 *  |                    | +------+ +------+              *
 *  |                    |                                *
 *  | m_no_vertices      |    +---+---+---+               *
 *  | m_vertices------------->| X | Y | Z |               *
 *  |                    |    |- - - - - -|               *
 *  |                    |    |    ...    |               *
 *  |                    |    +-----------+               *
 *  |                    |                                *
 *  | m_no_normals       |    +---+---+---+               *
 *  | m_normals-------------->| X | Y | Z |               *
 *  +--------------------+    |- - - - - -|               *
 *                            |    ...    |               *
 *                            +-----------+               *
 *                                                        *
\**********************************************************/

#define M_MAX_VOLUME_VERTICES 256           /* size of tmp structures */
#define M_MAX_VOLUMES          64           /* can be rendered at once */

struct M_volume_object
{
 int m_no_polygons;
 struct M_polygon **m_polygons;             /* points an array */

 int m_no_gates;
 struct M_gate **m_gates;                   /* gateways to other volumes */

 int m_no_vertices;
 int *m_vertices;                           /* points 3 element array */

 int m_no_normals;
 int *m_normals;                            /* points 3 element array */
};

void M_init_volume_object(struct M_volume_object *object);
void M_shade_volume_object(struct M_volume_object *object,
                           int no_lights,
                           struct M_light **lights
                          );                /* volume doesn't move */
void M_render_volume_object(struct M_volume_object *object,int depth);

/**********************************************************\
 * Surface object, a height field with corresponding      *
 * sets of vertices and normals.                          *
 *                                                        *
 *  +--------------------+ +------+---------+---------+   *
 *  | m_orders------------>|m_cell|m_vertex1|m_vertex2|   *
 *  |                    | +- - - - - - - - - - - - - +   *
 *  | m_total_size       | |            ...           |   *
 *  | m_display_size     | +--------------------------+   *
 *  | m_cell_length      |                                *
 *  |                    | +---+----------------+         *
 *  | m_vertices---------->| Y | [I] or [R G B] |         *
 *  |                    | |- - - - - - - - - - |         *
 *  |                    | |         ...        |         *
 *  |                    | +--------------------+         *
 *  |                    |                                *
 *  |                    | +----------------+             *
 *  | m_cells------------->|M_surface_cell  |             *
 *  |                    | |structures      |             *
 *  |                    | |- - - - - - - - |             *
 *  |                    | |       ...      |             *
 *  |                    | +----------------+             *
 *  |                    |                                *
 *  |                    | +---+---+---+                  *
 *  | m_normals----------->| X | Y | Z |                  *
 *  +--------------------+ |- - - - - -|                  *
 *                         |    ...    |                  *
 *                         +-----------+                  *
 *                                                        *
\**********************************************************/

#define M_TEXTURE_LENGTH       127          /* size of texture in the cell */
#define M_LOG_TEXTURE_LENGTH     7          /* log of the number above */

#define M_MAX_SURFACE_VERTICES  20          /* maximum display size */

#if defined(_CI_)
 #define M_LNG_SURFACE_VERTEX    2          /* Y Intensity */
#endif
#if defined(_RGB_)
 #define M_LNG_SURFACE_VERTEX    4          /* Y R G B */
#endif

struct M_surface_object
{
 int *m_orders;                             /* array of order indices */

 int m_total_size;                          /* dimension of the struct */
 int m_display_size;                        /* how many of those displayed */
 int m_cell_length;                         /* size for one cell */

 int *m_vertices;                           /* square array of vertices */
 struct M_surface_cell *m_cells;            /* square array of cells */
 int *m_normals;
};

void M_init_surface_object(struct M_surface_object *object);
void M_shade_surface_object(struct M_surface_object *object,
                            int no_lights,
                            struct M_light **lights
                           );               /* surface doesn't move */
void M_render_surface_object(struct M_surface_object *object,
                             int xcell,int zcell
                            );              /* render at [xc,zc] cell */

/**********************************************************/

#endif
