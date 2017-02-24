#if defined(SUPPORT_X11)
#include <map>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#if !defined(GLES)
	#include <GL/gl.h>
	#include <GL/glx.h>
#endif

#include "types.h"
#include "cfg/cfg.h"
#include "linux-dist/x11.h"
#include "linux-dist/main.h"
#include <iostream>

#if FEAT_HAS_NIXPROF
#include "profiler/profiler.h"
#include "sync_net.h"
#endif

#if defined(TARGET_PANDORA)
	#define DEFAULT_FULLSCREEN    1
	#define DEFAULT_WINDOW_WIDTH  800
#else
	#define DEFAULT_FULLSCREEN    0
	#define DEFAULT_WINDOW_WIDTH  640
#endif
#define DEFAULT_WINDOW_HEIGHT   480

//map<int, int> x11_keymap;
//int x11_dc_buttons = 0xFFFF;
int x11_keyboard_input = 0;

int x11_width;
int x11_height;

int ndcid = 0;
void* x11_glc;
bool x11_fullscreen = false;

void* x11_vis;

extern DCPads MainDCPADMap;
extern char X11KeyboardNum;

/*Эмуляция осей джостика с клавиатуры*/
bool APadEmu[4]={false, false, false, false};
char APadEl=0;
s8 tempX,tempY;

enum
{
	_NET_WM_STATE_REMOVE =0,
	_NET_WM_STATE_ADD = 1,
	_NET_WM_STATE_TOGGLE =2
};

void x11_window_set_fullscreen(bool fullscreen)
{
		XEvent xev;
		xev.xclient.type         = ClientMessage;
		xev.xclient.window       = (Window)x11_win;
		xev.xclient.message_type = XInternAtom((Display*)x11_disp, "_NET_WM_STATE", False);
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 2;    // _NET_WM_STATE_TOGGLE
		xev.xclient.data.l[1] = XInternAtom((Display*)x11_disp, "_NET_WM_STATE_FULLSCREEN", True);
		xev.xclient.data.l[2] = 0;    // no second property to toggle
		xev.xclient.data.l[3] = 1;
		xev.xclient.data.l[4] = 0;

		printf("x11: setting fullscreen to %d\n", fullscreen);
		XSendEvent((Display*)x11_disp, DefaultRootWindow((Display*)x11_disp), False, SubstructureNotifyMask, &xev);
}

void input_x11_handle()
{
	if (x11_win && x11_keyboard_input)  //если клаву не обявили то сюда не попадем
	{
		//Handle X11
		XEvent e;
                                    

		if(XCheckWindowEvent((Display*)x11_disp, (Window)x11_win, KeyPressMask | KeyReleaseMask, &e))
		{
			switch(e.type)
			{
				case KeyPress:
                                                                                { 
                                                                                    //вверх вниз, влево, вправо
                                                                                    for (APadEl=0; APadEl<4; APadEl++)
                                                                                    {
                                                                                        if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].SPad[APadEl])
                                                                                           APadEmu[APadEl]=true;
                                                                                    }
                                                                                    
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Triggers[0])
                                                                                       rt[X11KeyboardNum] =255;
                                                                                    
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Triggers[1])
                                                                                       lt[X11KeyboardNum] =255;
                                                                                    
                                                                                    //DPAD
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].DPad[0])
                                                                                       kcode[X11KeyboardNum] &= ~DC_DPAD_UP;
                                                                                    
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].DPad[1])
                                                                                       kcode[X11KeyboardNum] &=~DC_DPAD_DOWN;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].DPad[2])
                                                                                       kcode[X11KeyboardNum] &=~DC_DPAD_LEFT;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].DPad[3])
                                                                                       kcode[X11KeyboardNum] &=~DC_DPAD_RIGHT;
                                                                                   
                                                                                    //Кнопки...
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[0])
                                                                                       kcode[X11KeyboardNum] &=~DC_BTN_A;
                                                                                    
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[1])
                                                                                       kcode[X11KeyboardNum] &=~DC_BTN_B;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[2])
                                                                                       kcode[X11KeyboardNum] &=~DC_BTN_Y;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[3])
                                                                                       kcode[X11KeyboardNum] &=~DC_BTN_X;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[4])
                                                                                       kcode[X11KeyboardNum] &=~DC_BTN_START; 

                                                                                   break;
                                                                                }
				case KeyRelease:
                                    
                                                                                {
                                                                                    //вверх вниз, влево, вправо
                                                                                    for (APadEl=0; APadEl<4; APadEl++)
                                                                                    {
                                                                                        if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].SPad[APadEl])
                                                                                           APadEmu[APadEl]=false;
                                                                                    }
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Triggers[0])
                                                                                       rt[X11KeyboardNum] =0;
                                                                                    
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Triggers[1])
                                                                                       lt[X11KeyboardNum] =0;
                                                                                    
                                                                                    
                                                                                     //DPAD
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].DPad[0])
                                                                                       kcode[X11KeyboardNum] |=DC_DPAD_UP;
                                                                                    
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].DPad[1])
                                                                                       kcode[X11KeyboardNum] |=DC_DPAD_DOWN;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].DPad[2])
                                                                                       kcode[X11KeyboardNum] |=DC_DPAD_LEFT;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].DPad[3])
                                                                                       kcode[X11KeyboardNum] |=DC_DPAD_RIGHT;
                                                                                   
                                                                                    //Кнопки...
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[0])
                                                                                       kcode[X11KeyboardNum] |=DC_BTN_A;
                                                                                    
                                                                                    if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[1])
                                                                                       kcode[X11KeyboardNum] |=DC_BTN_B;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[2])
                                                                                       kcode[X11KeyboardNum] |=DC_BTN_Y;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[3])
                                                                                       kcode[X11KeyboardNum] |=DC_BTN_X;
                                                                                    
                                                                                     if (e.xkey.keycode==MainDCPADMap[X11KeyboardNum].Buttons[4])
                                                                                       kcode[X11KeyboardNum] |=DC_BTN_START; 
                                                                                    
                                                                                    
                                                                                    break;
                                                                                }
 			}
                                            //Обновление информации об осях
                                            tempX=0;
                                            tempY=0;
                                            
                                            if (APadEmu[0] ^ MainDCPADMap[X11KeyboardNum].InvertY) tempY=-127; 
                                            if (APadEmu[1] ^ MainDCPADMap[X11KeyboardNum].InvertY) tempY+=127;
                                            if (APadEmu[2] ^ MainDCPADMap[X11KeyboardNum].InvertX) tempX=-127; 
                                            if (APadEmu[3] ^ MainDCPADMap[X11KeyboardNum].InvertX) tempX+=127;
                                            
                                            joyx[X11KeyboardNum]=tempX;                                           
                                            joyy[X11KeyboardNum]=tempY;
                                            //обновили)                        
		}
	}
}

void input_x11_init()
{
    	//x11_keyboard_input = cfgLoadInt("input", "enable_x11_keyboard", 1);
    
                            //обновить инфу о клаве   
                            x11_keyboard_input=false;
                             for (int i=0; i<4; i++)                              
                                 if (MainDCPADMap[i].DeviceEvNumber==1) 
                                 { 
                                    X11KeyboardNum=i;
                                    x11_keyboard_input=true;
                                    break;
                                 }
}

void x11_window_create()
{
	if (cfgLoadInt("pvr", "nox11", 0) == 0)
	{
		XInitThreads();
		// X11 variables
		Window       x11Window = 0;
		Display*     x11Display = 0;
		long         x11Screen = 0;
		XVisualInfo* x11Visual = 0;
		Colormap     x11Colormap = 0;

		/*
		Step 0 - Create a NativeWindowType that we can use it for OpenGL ES output
		*/
		Window sRootWindow;
		XSetWindowAttributes sWA;
		unsigned int ui32Mask;
		int i32Depth;

		// Initializes the display and screen
		x11Display = XOpenDisplay(NULL);
		if (!x11Display && !(x11Display = XOpenDisplay(":0")))
		{
			printf("Error: Unable to open X display\n");
			return;
		}
		x11Screen = XDefaultScreen(x11Display);

		// Gets the window parameters
		sRootWindow = RootWindow(x11Display, x11Screen);

		int depth = CopyFromParent;

		#if !defined(GLES)
			// Get a matching FB config
			static int visual_attribs[] =
			{
				GLX_X_RENDERABLE    , True,
				GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
				GLX_RENDER_TYPE     , GLX_RGBA_BIT,
				GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
				GLX_RED_SIZE        , 8,
				GLX_GREEN_SIZE      , 8,
				GLX_BLUE_SIZE       , 8,
				GLX_ALPHA_SIZE      , 8,
				GLX_DEPTH_SIZE      , 24,
				GLX_STENCIL_SIZE    , 8,
				GLX_DOUBLEBUFFER    , True,
				//GLX_SAMPLE_BUFFERS  , 1,
				//GLX_SAMPLES         , 4,
				None
			};

			int glx_major, glx_minor;

			// FBConfigs were added in GLX version 1.3.
			if (!glXQueryVersion(x11Display, &glx_major, &glx_minor) ||
					((glx_major == 1) && (glx_minor < 3)) || (glx_major < 1))
			{
				printf("Invalid GLX version");
				exit(1);
			}

			int fbcount;
			GLXFBConfig* fbc = glXChooseFBConfig(x11Display, x11Screen, visual_attribs, &fbcount);
			if (!fbc)
			{
				printf("Failed to retrieve a framebuffer config\n");
				exit(1);
			}
			printf("Found %d matching FB configs.\n", fbcount);

			GLXFBConfig bestFbc = fbc[0];
			XFree(fbc);

			// Get a visual
			XVisualInfo *vi = glXGetVisualFromFBConfig(x11Display, bestFbc);
			printf("Chosen visual ID = 0x%x\n", vi->visualid);


			depth = vi->depth;
			x11Visual = vi;

			x11Colormap = XCreateColormap(x11Display, RootWindow(x11Display, x11Screen), vi->visual, AllocNone);
		#else
			i32Depth = DefaultDepth(x11Display, x11Screen);
			x11Visual = new XVisualInfo;
			XMatchVisualInfo(x11Display, x11Screen, i32Depth, TrueColor, x11Visual);
			if (!x11Visual)
			{
				printf("Error: Unable to acquire visual\n");
				return;
			}
			x11Colormap = XCreateColormap(x11Display, sRootWindow, x11Visual->visual, AllocNone);
		#endif

		sWA.colormap = x11Colormap;

		// Add to these for handling other events
		sWA.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
		ui32Mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;
                                    
                                                 
		/*x11_width = cfgLoadInt("x11", "width", DEFAULT_WINDOW_WIDTH);
		x11_height = cfgLoadInt("x11", "height", DEFAULT_WINDOW_HEIGHT);
		x11_fullscreen = (cfgLoadInt("x11", "fullscreen", DEFAULT_FULLSCREEN) > 0); */

		if (x11_width <= 0 || x11_height <= 0)   //FIXED!!
		{
			x11_width = XDisplayWidth(x11Display, x11Screen);
			x11_height = XDisplayHeight(x11Display, x11Screen);
		}

		// Creates the X11 window
		x11Window = XCreateWindow(x11Display, RootWindow(x11Display, x11Screen), (ndcid%3)*640, (ndcid/3)*480, x11_width, x11_height,
			0, depth, InputOutput, x11Visual->visual, ui32Mask, &sWA);

		if(x11_fullscreen)
		{

			// fullscreen
			Atom wmState = XInternAtom(x11Display, "_NET_WM_STATE", False);
			Atom wmFullscreen = XInternAtom(x11Display, "_NET_WM_STATE_FULLSCREEN", False);
			XChangeProperty(x11Display, x11Window, wmState, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wmFullscreen, 1);

			XMapRaised(x11Display, x11Window);
		}
		else
		{
			XMapWindow(x11Display, x11Window);
		}

		#if !defined(GLES)
			#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
			#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
			typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

			glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
			glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
			verify(glXCreateContextAttribsARB != 0);
			int context_attribs[] =
			{
				GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
				GLX_CONTEXT_MINOR_VERSION_ARB, 1,
				GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
				GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
				None
			};

			x11_glc = glXCreateContextAttribsARB(x11Display, bestFbc, 0, True, context_attribs);
			XSync(x11Display, False);

			if (!x11_glc)
			{
				die("Failed to create GL3.1 context\n");
			}
		#endif

		XFlush(x11Display);

		//(EGLNativeDisplayType)x11Display;
		x11_disp = (void*)x11Display;
		x11_win = (void*)x11Window;
		x11_vis = (void*)x11Visual->visual;
	}
	else
	{
		printf("Not creating X11 window ..\n");
	}
}

void x11_window_set_text(const char* text)
{
	if (x11_win)
	{
		XChangeProperty((Display*)x11_disp, (Window)x11_win,
			XInternAtom((Display*)x11_disp, "WM_NAME", False),     //WM_NAME,
			XInternAtom((Display*)x11_disp, "UTF8_STRING", False), //UTF8_STRING,
			8, PropModeReplace, (const unsigned char *)text, strlen(text));
	}
}

void x11_window_destroy()
{
	// close XWindow
	if (x11_win)
	{
		XDestroyWindow((Display*)x11_disp, (Window)x11_win);
		x11_win = 0;
	}
	if (x11_disp)
	{
		XCloseDisplay((Display*)x11_disp);
		x11_disp = 0;
	}
}
#endif
