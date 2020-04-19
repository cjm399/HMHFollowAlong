//Some starting code for the project!
#include <windows.h>
LRESULT CALLBACK
MainWindowCallback(
					  HWND   HandleWindow,
					  UINT   Message,
					  WPARAM WParam,
					  LPARAM LParam
)
{
	LRESULT result = 0;
	switch(Message)
	{
		case WM_SIZE:
		{
			OutputDebugStringA("Message\n");
		}break;
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(HandleWindow, &Paint);
			int x = Paint.rcPaint.left;
			int y = Paint.rcPaint.top;
			LONG height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			LONG width = Paint.rcPaint.right - Paint.rcPaint.left;
			PatBlt(DeviceContext, x, y, height, width, WHITENESS);
			EndPaint(HandleWindow, &Paint);
		}break;
		default:
		{
			result = DefWindowProc(HandleWindow, Message, WParam, LParam);
		}
	}
	return(result);
}

int CALLBACK 
WinMain(
		 HINSTANCE Instance, 
		 HINSTANCE PreviuosInstance,
		 LPSTR CommandLine,
		 int ShowCode
)
{
//todo: highlight the word todo!
	WNDCLASS WindowClass= {};
	WindowClass.style = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;	
	WindowClass.lpfnWndProc= MainWindowCallback;
	WindowClass.hInstance= Instance;	
   //	WindowClass.hIcon= ;	
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";	

	if(RegisterClass(&WindowClass))
	{
		HWND WindowHandle =  
			CreateWindowEx(
					0,
					WindowClass.lpszClassName,
	   			"Handmade Hero",
	  				WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					0,
					0,
					Instance,
					0);
		if(WindowHandle)
		{
			MSG Message;
			for(;;)
				{
					BOOL MessageResult = GetMessage(&Message, 0, 0, 0);	
					if(MessageResult > 0)
					{
						TranslateMessage(&Message);
						DispatchMessage(&Message);
					}
					else
					{
						break;
					}
				}
		}
		else
		{
			//TODO: Log
		}
	}
	else
	{
		//TODO: Log
	}
	return(0);
}
