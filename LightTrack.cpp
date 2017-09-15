//LightTrack.cpp
#include "LightTrack.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>                         /* var arg stuff */
#include "Track.h"
#include "Colour.h"
#include "Graphics.h"
#include "data.h"
#include "Vector.h"


#include <string.h>

//extern int main(int n,char **o);            /* real main (well...) */


int Depth = 3 ;   //  跟踪深度 

int Position_Y = -300 ;
int Position_X = 0 ;
int Position_Z = 700 ;

int HW_cmd_show;
HINSTANCE HW_instance;
char HW_class_name[] = "LightTrack windows";
HWND HW_wnd;                                /* window */
HPALETTE HW_palette;                        /* the palette headers */


HDC HW_mem;
HBITMAP HW_bmp;
RECT HW_rect;

#if defined(_RGB_)                          /* paths to data sets */
 #if defined(_32BPP_)
char path[128]="";
 #endif
#endif

struct TR_world *w;


void HWI_null_function(void) {}
void (*HW_application_main)(void) = HWI_null_function;
void (*HW_application_key_handler)(int key_code);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**********************************************************\
 * Quiting with a message.                                *
\**********************************************************/

#define HW_MAX_ERROR_MESSAGE 256

void HW_error(char *s,...)
{
	char str[HW_MAX_ERROR_MESSAGE];
	va_list lst;

	va_start(lst,s);
	vsprintf(str,s,lst);                       /* forming the message */
	va_end(lst);

	MessageBox(NULL,str,"LightTrack",MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL);

	HW_close_event_loop();
	exit(0);                                   /* above might not be enough */
}
/**********************************************************\
 * Quitting the event loop.                               *
\**********************************************************/

void HW_close_event_loop(void)
{
 PostMessage(HW_wnd,WM_CLOSE,0,0L);         /* telling ourselves to quit */
}
/**********************************************************\
 * Implementations for fast memory fill.                  *
\**********************************************************/

void HW_set_int(int *d,long l,int v)
{
 long i;

 for(i=0;i<l;i++) *d++=v;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


#define HW_MAX_CLINE_OPTIONS 20


int WINAPI WinMain(HINSTANCE hInstance, 
				   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine, 
				   int nShowCmd )
{
	WNDCLASS w;
	int n;
	char *start,*end;
	char *o[HW_MAX_CLINE_OPTIONS];

	HW_cmd_show = nShowCmd;
	if ((HW_instance = hPrevInstance) == NULL) 
	{
		w.style = CS_HREDRAW|CS_VREDRAW;
		w.lpfnWndProc = WndProc;
		w.cbClsExtra = 0;
		w.cbWndExtra = 0;
		w.hInstance = hInstance;
		w.hIcon = NULL;
		w.hCursor = NULL;
		w.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		w.lpszMenuName = NULL;
		w.lpszClassName = HW_class_name;

		if (!RegisterClass(&w)) 
		{
			return FALSE;
		}
	}
	
	n = 0;
	o[n++] = "";
	start = lpCmdLine;
	while ((end = strchr(start, ' ')) != NULL) 
	{
		if (n >= HW_MAX_CLINE_OPTIONS)
		{
			HW_error("(LightTrack windows) Way too many command line options.\n");
		}

		if (end != start)
		{
			o[n++] = start;
		}

		*end = 0;
		start = end + 1;
	}

	if (strlen(start) > 0)
	{
		o[n++] = start;
	}

	return(main(n,0));

}

extern int Depth  ;   //  跟踪深度 

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) 
	{
	case WM_PAINT: 
		HW_application_main();
		break;
	case WM_ERASEBKGND:
		return(1L); 
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE)
		{
			PostMessage(HW_wnd,WM_CLOSE,0,0L); //HW_application_key_handler(wParam);
		}
		if (wParam==VK_F1)
		{
			Depth--;
			if (Depth <=0)
			{
				Depth = 0 ;
			}
		}
		if (wParam==VK_F2)
		{
			if (Depth >=5)
			{
				Depth = 5 ;
			}
			else Depth++;			
		}
		if (wParam==VK_UP )
		{
			Position_Y -= 10 ;
		}
		if (wParam==VK_DOWN )
		{
			Position_Y += 10 ;
		}

		if (wParam==VK_LEFT )
		{
			Position_X -= 10 ;
		}

		if (wParam==VK_RIGHT )
		{
			Position_X +=10 ;
		}
 
		 if (wParam=='A' )
		 {
			 Position_Z -=100 ;
		 }
		 
		 if (wParam=='Z' )
		 {
			 Position_Z +=100 ;
		 }

		break;
	default:
		return(DefWindowProc(hWnd,message,wParam,lParam));
	}
	
	return(0L);
}

/**********************************************************\
 * Creating a window.                                     *
\**********************************************************/

#if defined(_RGB_)
void HW_init_screen(char *display_name,
                    char *screen_name
                   )
#endif
{
	PAINTSTRUCT ps;

	HW_wnd = CreateWindow(HW_class_name,
						screen_name,
						WS_SYSMENU,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						HW_SCREEN_X_SIZE,
						HW_SCREEN_Y_SIZE+GetSystemMetrics(SM_CYCAPTION),
						NULL,
						NULL,
						HW_instance,
						NULL);

	HW_mem=CreateCompatibleDC(BeginPaint(HW_wnd,&ps));
	if(((GetDeviceCaps(ps.hdc,PLANES))!=1)||
	((GetDeviceCaps(ps.hdc,BITSPIXEL))!=sizeof(HW_pixel)*8)
	)

	//HW_error("%d",(GetDeviceCaps(ps.hdc,BITSPIXEL)));

	HW_error("(Hardware) I'd rather have %d bit screen.",
		   sizeof(HW_pixel)*8
		  );

	HW_bmp=CreateCompatibleBitmap(ps.hdc,HW_SCREEN_X_SIZE,HW_SCREEN_Y_SIZE);
	SelectObject(HW_mem,HW_bmp);

	EndPaint(HW_wnd,&ps);

	HW_rect.left=HW_rect.top=0;

	//窗口的大小
	HW_rect.right=HW_SCREEN_X_SIZE;
	HW_rect.bottom=HW_SCREEN_Y_SIZE;

	ShowWindow(HW_wnd,HW_cmd_show);            /* generate messages */
	UpdateWindow(HW_wnd);
}



//////////////////////////////////////////////////////////////////////////////////
/**********************************************************\
 * Rendering the bitmap into the window.                  *
\**********************************************************/

//复制图像位图到物理设备存储器
void HW_blit(void)
{
	PAINTSTRUCT ps;

	//
	BeginPaint(HW_wnd,&ps);

	SelectPalette(ps.hdc,HW_palette,FALSE);
	RealizePalette(ps.hdc);
	SetMapMode(ps.hdc,MM_TEXT);
	SetBitmapBits(HW_bmp,G_c_buffer_size*sizeof(HW_pixel),G_c_buffer);
	BitBlt(ps.hdc,0,0,HW_SCREEN_X_SIZE,HW_SCREEN_Y_SIZE,HW_mem,0,0,SRCCOPY);

	EndPaint(HW_wnd,&ps);
}

void app_main(void)                         /* rendering loop */
{
	//光线跟踪场景中设置摄像机
	//TR_set_camera(0,0,500, 0,0,0, 1,0,0, 0,1,0);Position_X
	TR_set_camera(Position_X,Position_Y,-1.0*Position_Z, 0,0,0, 1,0,0, 0,1,0);


	//关键中的关键，光线跟踪窗口中的pixel
	TR_trace_world(w,Depth);




	{
		char str[100]="Now depth =  ";
	    char * p = str;	char strDepth[10];
	    itoa(Depth,strDepth,10);
	    p=strcat(str,strDepth);

	    G_text(270,10,p,
		CL_colour(CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1),
		CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1);

	}

	{
		char str[100]="F1 - Sub Depth ; f2 - add depth ";
		char * p = str;	
		
		G_text(270,30,p,
			CL_colour(CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1),
			CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1);
		
	}

	G_text(270,50,"Esc  - exit program ",
		CL_colour(CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1),
		CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1);

	G_text(270,70,"arrow(left,right,up,down) - change view",
		CL_colour(CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1),
		CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1);

	G_text(270,90,"\'a\' or \'z\' ->  - change distance ",
		CL_colour(CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1,CL_COLOUR_LEVELS-1),
		CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1,CL_LIGHT_LEVELS-1);
	
	HW_blit();
}

void app_handler(int kk)                    /* event handler */
{
	HW_close_event_loop();
}

void HW_close_screen(void)
{
	DeleteDC(HW_mem);
	DeleteObject(HW_bmp);
}


int main(int n, char **o)
{
	char *display;
	if (n == 2) 
		display = o[1];
	else
		display = NULL;

	strcat(path,"tracer.dat");

	w = (struct TR_world *)D_data(path);

	CL_init_colour();

	TR_init_rendering(TR_SPECULAR|TR_SHADOW|TR_REFLECT);
	
	TR_init_world(w);

	G_init_graphics();

	HW_init_screen(display,"LightTrack");

	HW_init_event_loop(app_main,app_handler);

	HW_close_screen();


	return(1);
}



/**********************************************************\
 * Running the event loop.                                *
\**********************************************************/

//事件循环函数，第一个函数指针被不停执行，第二个响应外部时间
void HW_init_event_loop(void (*application_main)(void),
                        void (*application_key_handler)(int key_code)
                       )
{
	MSG msg;

	HW_application_main=application_main;
	HW_application_key_handler=application_key_handler;

	while(1)
	{
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))  /* this IS sensitive part! */
		{
			if(msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			InvalidateRect(HW_wnd,&HW_rect,TRUE);
			UpdateWindow(HW_wnd);
		}
	}
}


