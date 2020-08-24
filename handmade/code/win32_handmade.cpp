
#include <windows.h>
#include <winuser.h>
#include <stdint.h>
#include <xinput.h>

#define internal static 
#define local_persist static 
#define global_variable static 

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct win32_window_dimension
{
    int height;
    int width;
};

#define X_Input_Get_State(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_Input_Get_State(x_input_get_state);
X_Input_Get_State(XInputGetStateStub)
{
    return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_Input_Set_State(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_Input_Set_State(x_input_set_state);
X_Input_Set_State(XInputSetStateStub)
{
    return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;


internal void Win32LoadXInput()
{
    HMODULE xInputLib = LoadLibraryA("xinput1_3.dll");
    if(xInputLib)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(xInputLib, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(xInputLib, "XInputSetState");
    }
}


internal win32_window_dimension
Win32GetWindowDimension(HWND window)
{
    win32_window_dimension result;
    
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;

    return(result);
}                                                       

internal void WindowGradient(win32_offscreen_buffer *buffer, uint8_t xOffset, uint8_t yOffset)
{    
    uint8_t *row = (uint8_t *)buffer->memory;
    for(int Y = 0; Y < buffer->height; Y++)
    {
        uint32_t *pixel = (uint32_t *) row;
        for(int X = 0; X < buffer->width; X++)
        {
            uint8_t red = 255;
            uint8_t green = X+xOffset;
            uint8_t blue = Y+yOffset;
            *pixel++ = (((red << 16) | (green << 8)) | blue);
        }
        row+= buffer->pitch;
    }
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *buffer,
                      int _width, int _height)
{
    buffer->height = _height;
    buffer->width = _width;
    int bytesPerPixel = 4;
    
    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    
    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = _width;
    buffer->Info.bmiHeader.biHeight = -_height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    SIZE_T bufferSize = bytesPerPixel * (_width * _height);
    buffer->memory = VirtualAlloc(0, bufferSize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = _width*bytesPerPixel;
}

internal void
Win32CopyBufferToWindow(HDC deviceContext,
                        int windowWidth, int windowHeight,
                        win32_offscreen_buffer *buffer)
{
    StretchDIBits(deviceContext,
                  /*_x, _y, _width, _height,
                    _x, _y, _width, _height,*/
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}    

LRESULT CALLBACK
Win32MainWindowCallback(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT result = 0;
    switch(message)
    {
        case WM_SIZE:
        {
        } break;
            
        case WM_DESTROY:{
            //TODO: Send a message to the user?
            GlobalRunning = false;            
        } break;
            
        case WM_CLOSE:{

//TODO: Try to recreate window? Handle as error?
            GlobalRunning = false;
        } break;
            
        case WM_ACTIVATEAPP:{
            
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            
            win32_window_dimension dimensions = Win32GetWindowDimension(window);
            Win32CopyBufferToWindow(deviceContext,
                                    dimensions.width, dimensions.height,
                                    &GlobalBackBuffer);
            EndPaint(window, &paint);
        }   break;
            
        default:
            result = DefWindowProc(window, message, wParam, lParam);
            break;
    }
    return (result);
}


int WINAPI WinMain(HINSTANCE instance,
                   HINSTANCE previousInstance,
                   LPSTR commandLine,
                   int showCode)
    
{

    WNDCLASSEXA windowClass = {};

    Win32LoadXInput();
    //win32_window_dimension dimensions = Win32GetWindowDimension(window);
    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);    

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    //windowClass.hIcon;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";    

    ATOM uniqueClassId = RegisterClassExA(&windowClass);
    if(uniqueClassId)
    {
        //create window and look at the queue
        HWND window = CreateWindowExA(
            0,
            "HandmadeHeroWindowClass",
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0);

        if(window)
        {
            HDC deviceContext = GetDC(window);
            MSG message;
            GlobalRunning = true;
            uint8_t xOffset = 0, yOffset = 0;
            while(GlobalRunning)
            {
                
                while(PeekMessage(&message, window, 0, 0, PM_REMOVE))
                {
                    if(message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                    
                }
                for(DWORD controllerIndex = 0;
                    controllerIndex < XUSER_MAX_COUNT;
                    ++controllerIndex)
                {
                    
                    XINPUT_STATE inputState;
                    DWORD inputReturnCode = XInputGetState(controllerIndex, &inputState);
                    if(inputReturnCode == ERROR_SUCCESS)
                    {
                        XINPUT_GAMEPAD *gamePad = &inputState.Gamepad;
                        xOffset += gamePad->sThumbLX >> 12;
                        yOffset += gamePad->sThumbLY >> 12;
                    }
                    else
                    {
                        //TODO Add error handling, figure out what to show user.
                    } 
                }
                
                WindowGradient(&GlobalBackBuffer, xOffset, yOffset);

                win32_window_dimension dimensions = Win32GetWindowDimension(window);
                Win32CopyBufferToWindow(deviceContext,
                                        dimensions.width, dimensions.height,
                                        &GlobalBackBuffer);
            }            
        }
        else
        {
            //TODO: Loggin error info
        }
        
    }
    else
    {
        uniqueClassId;
        //TODO: Logging error info
    }
    
    return(0);
}
