#include "platform.h"
#include "game_engine_lib.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "wglext.h"
// #########################################################################
//                      Windows Globals
// #########################################################################
static HWND window;

// #########################################################################
//                      Platform Implementations
// #########################################################################
LRESULT CALLBACK windows_window_callback(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch(msg)
    {
        case WM_CLOSE:
        {
            running = false;
            break;
        }

        default:
        {
            // Let windows handle the default input for now
            result = DefWindowProcA(window, msg, wParam, lParam); // default windows procedure address for handling window messages
        }
    }

    return result;
}

bool platform_create_window(int width, int height, char* title)
{
    HINSTANCE instance = GetModuleHandleA(0); // instance we need when we register a window class

    WNDCLASS wc = {};
    wc.hInstance  = instance;
    wc.hIcon = LoadIcon(instance, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);           // here we decide the look of the cursor 
    wc.lpszClassName = title;                           // THIS IS NOT THE TITLE, THIS IS A UNIQUE IDENTIFIER
    wc.lpfnWndProc = windows_window_callback;           // Callback for Input into the Window; long function pointer to our window proc address

    if(!RegisterClassA(&wc))
    {
        return false;
    }

    // WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
    int dwStyle = WS_OVERLAPPEDWINDOW;
    
    // we need access to these functions' proc address for the reason below. But we can't use them without creating a DC in the 1st place
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

    // Black magic going on here
    // Fake window initialization for OpenGL 
    {
        window = CreateWindowExA(0, title,     // this references lpszClassName from wc
                                    title,      // this is the actual Title
                                    dwStyle,
                                    100,
                                    100,
                                    width,
                                    height,
                                    NULL,       // parent
                                    NULL,       // menu
                                    instance,
                                    NULL);      // lpParam
        if (window == NULL)
        {
            return false;
        }
        
        HDC fakeDC = GetDC(window);
        //this is just in case
        if(!fakeDC)
        {
            SM_ASSERT(false, "Failed to get HDC");
            return false;
        }
        PIXELFORMATDESCRIPTOR pfd = {0};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cAlphaBits = 8;
        pfd.cDepthBits = 24;

        int pixelFormat = ChoosePixelFormat(fakeDC, &pfd);
        if(!pixelFormat)
        {
            SM_ASSERT(false, "Failed to choose pixel format!");
            return false;
        }

        if(!SetPixelFormat(fakeDC, pixelFormat, &pfd))
        {
            SM_ASSERT(false, "Failed to set pixel format!");
            return false;
        }

        // Create handle for fake rendering context using the fake  device context
        HGLRC fakeRC = wglCreateContext(fakeDC);
        if(!fakeRC)
        {
            SM_ASSERT(false, "Failed to create rendering context!");
            return false;
        }

        if(!wglMakeCurrent(fakeDC, fakeRC))
        {
            SM_ASSERT(false, "Failed to make current fake render context for fake device context!");
            return false;
        }

        // we need access to these functions' proc address for the reason below. But we can't use them without creating a DC in the 1st place
        wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)platform_load_gl_function("wglChoosePixelFormatARB");
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)platform_load_gl_function("wglCreateContextAttribsARB");
        if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB)
        {
            SM_ASSERT(false, "Failed to load OpenGL functions!");
            return false;
        }
        
        // destroy what we did and recreate again BECAUSE we can't use this rendering context;
        // -> we have to set attributes on the window before we create a rendering context xD (for openGL)
        // ^ this was the reason

        // Clean up
        wglMakeCurrent(fakeDC, 0);
        wglDeleteContext(fakeRC);
        ReleaseDC(window, fakeDC);

        // Can't reuse the same Device Context
        // because we already called "SetPixelFormat"
        DestroyWindow(window);
    }

    // Actual OpenGL initialization
    {
        //  Add border size of the window
        {
            RECT borderRect = {};
            AdjustWindowRectEx(&borderRect, dwStyle,  0, 0);

            width += borderRect.right - borderRect.left;
            height += borderRect.bottom - borderRect.top;
        }

        window = CreateWindowExA(0, title,     // this references lpszClassName from wc
                                    title,      // this is the actual Title
                                    dwStyle,
                                    100,
                                    100,
                                    width,
                                    height,
                                    NULL,       // parent
                                    NULL,       // menu
                                    instance,
                                    NULL);      // lpParam

        if (window == NULL)
        {
            SM_ASSERT(false, "Failed to create Windows Window!");
            return false;
        }
        HDC dc = GetDC(window);
        //this is just in case
        if(!dc)
        {
            SM_ASSERT(false, "Failed to get DC");
            return false;
        }

        const int pixelAttribs[] = 
        {
            WGL_DRAW_TO_WINDOW_ARB, 1,  // GL_TRUE
            WGL_SUPPORT_OPENGL_ARB, 1,
            WGL_DOUBLE_BUFFER_ARB,  1,
            WGL_SWAP_COPY_ARB,      WGL_SWAP_COPY_ARB,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
            WGL_COLOR_BITS_ARB,     32,
            WGL_ALPHA_BITS_ARB,     8,
            WGL_DEPTH_BITS_ARB,     24,
            0   //Terminat with 0 so openGL does not throw error
        };

        //  ARB == ARCHITECTURE REVIEW BOT
        UINT numPixelFormats;
        int pixelFormat = 0;
        if(!wglChoosePixelFormatARB(dc, pixelAttribs,
                                    0,  // Float List
                                    1,  // Max Formats
                                    &pixelFormat,
                                    &numPixelFormats))
        {
            SM_ASSERT(false, "Failed to wglChoosePixelFormatARB");
            return false;
        }
        PIXELFORMATDESCRIPTOR pfd = {0};
        DescribePixelFormat(dc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        if (!SetPixelFormat(dc, pixelFormat, &pfd))
        {
            SM_ASSERT(false, "Failed to SetPixelFormat");
            return true;
        }

        const int contextAttribs[] = 
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
            0,
        };

        HGLRC rc = wglCreateContextAttribsARB(dc, 0, contextAttribs);
        if(!rc)
        {
            SM_ASSERT(0, "failed to create render context for openGL");
            return false;
        }

        if(!wglMakeCurrent(dc, rc))
        {
            SM_ASSERT(false, "Failed to wglMakeCurrent");
            return false;
        }
    }
    ShowWindow(window, SW_SHOW);
    
    return true;
}

void platform_update_window()
{
    // Contains message information from a thread's message queue.
    MSG msg;        // create MSG obj on the stack
    /*
        There is a queue of messages and we need to make sure we empty that queue.
        We peek one message, we fill in the data and we dispatch it via a callback
    */
    while(PeekMessage(&msg, window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);      // Calls the callback defined in up above (wc.lpfnWndProc)
    }
}


void* platform_load_gl_function(char* funName)
{
    PROC proc = wglGetProcAddress(funName);
    if(!proc)
    {
        // in case not the entirety of openGL is loaded, we load it directly from the machine and get its proc addr
        static HMODULE openglDLL = LoadLibraryA("opengl32.dll");
        proc = GetProcAddress(openglDLL, funName);

        if(!proc)
        {
            SM_ASSERT(false, "Failed to load gl function %s", "glCreateProgram");
            return nullptr;
        }
    }
    return (void*)proc;
}