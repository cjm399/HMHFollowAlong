
#include <windows.h>
#include <winuser.h>
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static 

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension
{
    int Height;
    int Width;
};

//TODO: this is global for now!
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
                                                       
win32_window_dimension
Win32GetWindowDimension(HWND window)
{
    win32_window_dimension Result;
    
    RECT clientRect;
    GetClientRect(window, &clientRect);
    Result.Width = clientRect.right - clientRect.left;
    Result.Height = clientRect.bottom - clientRect.top;

    return(Result);
}                                                       

internal void WindowGradient(win32_offscreen_buffer Buffer, uint8_t XOffset, uint8_t YOffset)
{    
    uint8_t *Row = (uint8_t *)Buffer.Memory;
    for(int Y = 0; Y < Buffer.Height; Y++)
    {
        uint32_t *Pixel = (uint32_t *) Row;
        for(int X = 0; X < Buffer.Width; X++)
        {
            uint8_t red = 255;
            uint8_t green = X+XOffset;
            uint8_t blue = Y+YOffset;
            *Pixel++ = (((red << 16) | (green << 8)) | blue);
        }
        Row+= Buffer.Pitch;
    }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer * Buffer, int _width, int _height)
{
    Buffer->Height = _height;
    Buffer->Width = _width;
    Buffer->BytesPerPixel = 4;
    
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = _width;
    Buffer->Info.bmiHeader.biHeight = -_height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    SIZE_T bufferSize = Buffer->BytesPerPixel * (_width * _height);
    Buffer->Memory = VirtualAlloc(0, bufferSize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = _width*Buffer->BytesPerPixel;
}

internal void Win32CopyBufferToWindow(HDC DeviceContext,
                                      int WindowWidth, int WindowHeight,
                                      win32_offscreen_buffer Buffer,
                                      int _x, int _y,
                                      int _width, int _height)
{
    StretchDIBits(DeviceContext,
                  /*_x, _y, _width, _height,
                    _x, _y, _width, _height,*/
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer.Width, Buffer.Height,
                  Buffer.Memory,
                  &Buffer.Info,
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
            HDC DeviceContext = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            
            win32_window_dimension dimensions = Win32GetWindowDimension(window);
            Win32CopyBufferToWindow(DeviceContext,
                                    dimensions.Width, dimensions.Height,
                                    GlobalBackBuffer,
                                    x, y,
                                    width, height);
            EndPaint(window, &paint);
        }   break;
            
        default:
            result = DefWindowProc(window,message,wParam,lParam);
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
            HDC DeviceContext = GetDC(window);
            MSG message;
            GlobalRunning = true;
            uint8_t XOffset = 0, YOffset = 0;
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
                WindowGradient(GlobalBackBuffer, XOffset++, YOffset++);

                win32_window_dimension dimensions = Win32GetWindowDimension(window);
                Win32CopyBufferToWindow(DeviceContext,
                                        dimensions.Width, dimensions.Height,
                                        GlobalBackBuffer,
                                        0, 0,
                                        dimensions.Width, dimensions.Height);
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
