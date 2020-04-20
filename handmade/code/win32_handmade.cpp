#include <windows.h>
#include <winuser.h>

LRESULT CALLBACK
MainWindowCallback(
    HWND windowHandle,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT result = 0;
    switch(message)
    {
        case WM_SIZE:
            OutputDebugStringA("WM_SIZE\n");
            break;
            
        case WM_DESTROY:
            OutputDebugStringA("WM_DESTROY\n");
            break;
            
        case WM_CLOSE:
            OutputDebugStringA("WM_CLOSE\n");
            break;
            
        case WM_ACTIVATEAPP:
            OutputDebugStringA("WM_ACTIVATEAPP\n");
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC DeviceContext = BeginPaint(windowHandle, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            static DWORD stroke = WHITENESS;
            PatBlt(DeviceContext, x, y, width, height, stroke);
            EndPaint(windowHandle, &paint);

            if(stroke == WHITENESS)
            {
                stroke = BLACKNESS;
            }
            else
            {
                stroke = WHITENESS;
            }
            

        }   break;
            
        default:
            result = DefWindowProc(windowHandle,message,wParam,lParam);
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
    
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowCallback;
    windowClass.hInstance = instance;
    //windowClass.hIcon;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";    

    ATOM uniqueClassId = RegisterClassExA(&windowClass);
    if(uniqueClassId)
    {
        //create window and look at the queue
        HWND windowHandle = CreateWindowExA(
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

        if(windowHandle)
        {
            MSG message;
            for(;;)
            {
                
                BOOL messageResult = GetMessageA(&message,0,0,0);
                if(messageResult > 0)
                {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
                else
                {
                    break;
                }
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
