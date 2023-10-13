#include"game_engine_lib.h"


// #########################################################################
//                      Platform Globals
// #########################################################################
static bool running = true;

// #########################################################################
//                      Platform Functions
// #########################################################################
bool platform_create_window(int width, int height, char *title);
void platform_update_window();

// #########################################################################
//                      Windows Platform
// #########################################################################
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

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

#endif



int main()
{
    platform_create_window(1200, 720, "CelesteClone");

    int i = 0;
    while(running)
    {
        // Update
        platform_update_window();

        SM_TRACE("Test");
        SM_WARN("Warn");
        SM_ERROR("Error");
        SM_ASSERT(false, "Assertion not hit");
    }


    return 0;
}