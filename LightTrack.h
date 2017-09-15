

#include <windows.h>
#include <string.h>                         /* memset memcpy */


#define main _main                          /* to accomodate windows */

//extern "C";



#define HW_set_char(dst,lng,val)  memset(dst,val,lng)
#define HW_copy_char(src,dst,lng) memcpy(dst,src,lng)

void HW_set_int(int *dst,long lng,int val);
#define HW_copy_int(src,dst,lng)  memcpy(dst,src,lng*sizeof(int))

typedef char HW_8_bit;                      /* compiler/mashine independent */
#define HW_set_8_bit(dst,lng,val) HW_set_char(dst,lng,val)
#define HW_copy_8_bit(src,dst,lng) HW_copy_char(src,dst,lng)

typedef short HW_16_bit;
void HW_set_16_bit(HW_16_bit *dst,long lng,HW_16_bit val);
#define HW_copy_16_bit(src,dst,lng) memcpy(dst,src,lng*sizeof(HW_16_bit))

typedef int HW_32_bit;
#define HW_set_32_bit(dst,lng,val) HW_set_int(dst,lng,val)
#define HW_copy_32_bit(src,dst,lng) HW_copy_int(src,dst,lng)

#if defined(_16BPP_)
typedef HW_16_bit HW_pixel;
#define HW_set_pixel(dst,lng,val) HW_set_16_bit(dst,lng,val)
#define HW_copy_pixel(src,dst,lng) HW_copy_16_bit(src,dst,lng)
#endif


#if defined(_32BPP_)
typedef HW_32_bit HW_pixel;
#define HW_set_pixel(dst,lng,val) HW_set_32_bit(dst,lng,val)
#define HW_copy_pixel(src,dst,lng) HW_copy_32_bit(src,dst,lng)
#endif

#define HW_SCREEN_X_SIZE 600
#define HW_SCREEN_Y_SIZE 600                /* number of pixels total */
#if defined(_MONO_)
 #define HW_SCREEN_LINE_SIZE HW_SCREEN_X_SIZE
#endif
#define HW_SCREEN_X_MAX    (HW_SCREEN_X_SIZE-1)
#define HW_SCREEN_Y_MAX    (HW_SCREEN_Y_SIZE-1)
#define HW_SCREEN_X_CENTRE (HW_SCREEN_X_SIZE/2)
#define HW_SCREEN_Y_CENTRE (HW_SCREEN_Y_SIZE/2)

#define HW_KEY_ARROW_LEFT  VK_LEFT
#define HW_KEY_ARROW_RIGHT VK_RIGHT
#define HW_KEY_ARROW_UP    VK_UP
#define HW_KEY_ARROW_DOWN  VK_DOWN
#define HW_KEY_PLUS        VK_ADD
#define HW_KEY_MINUS       VK_SUBTRACT
#define HW_KEY_ENTER       VK_RETURN
#define HW_KEY_SPACE       VK_SPACE
#define HW_KEY_TAB         VK_TAB           /* all i can think of */



int main(int n, char **o);

void HW_error(char *s,...);
void HW_close_event_loop(void);
void HW_init_event_loop(void (*application_main)(void),
                        void (*application_key_handler)(int key_code)
                       );

