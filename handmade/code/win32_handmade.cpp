#include <windows.h>
#include <winuser.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static 
#define local_persist static 
#define global_variable static 
#define PI_32 3.14159265358


typedef int32_t bool32;


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

//NOTE(chris): GetState with Stub function
#define X_Input_Get_State(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_Input_Get_State(x_input_get_state);
X_Input_Get_State(XInputGetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_


//NOTE(chris): Set State with Stub function
#define X_Input_Set_State(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_Input_Set_State(x_input_set_state);
X_Input_Set_State(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;


internal void Win32LoadXInput()
{
	HMODULE xInputLib = LoadLibraryA("xinput1_4.dll");
	if (!xInputLib)
	{
		HMODULE xInputLib = LoadLibraryA("xinput1_3.dll");
	}
	if (xInputLib)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(xInputLib, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(xInputLib, "XInputSetState");
	}
}

internal void Win32InitSound(HWND window, int32_t SamplesPerSecond, int32_t BufferSize)
{
	//Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    
	if (DSoundLibrary)
	{
		//Get a DirectSound obj -- set coop
		direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = WaveFormat.nChannels * WaveFormat.wBitsPerSample / 8;
			WaveFormat.nAvgBytesPerSec =  WaveFormat.nBlockAlign * WaveFormat.nSamplesPerSec;
			WaveFormat.cbSize = 0;
            
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
			{
				//NOTE(chris): Set buffer struct to 0. Primary size 0.
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &primaryBuffer, 0)))
				{
					if (SUCCEEDED(primaryBuffer->SetFormat(&WaveFormat)))
					{
						//We have set the format of the primary buffer!
                        OutputDebugStringA("We set the primary buffer!");
					}
					else 
					{
						//TODO: Diagnostic
					}
				}
			}
			else
			{
				//TODO(chris): Diagnostic
			}
			//Create a secondary (virtual) buffer
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
            
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{
				OutputDebugStringA("Secondary Sound Buffer Created Successfully.");
			}
			else 
			{
				//TODO: Diagnostic.
			}
		}
		else
		{
			//TODO(chris): Diagnostic
		}
	}
	else
	{
		//TODO(chris): Diagnostic
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
	for (int Y = 0; Y < buffer->height; Y++)
	{
		uint32_t *pixel = (uint32_t *)row;
		for (int X = 0; X < buffer->width; X++)
		{
			uint8_t red = 255;
			uint8_t green = X + xOffset;
			uint8_t blue = Y + yOffset;
			*pixel++ = (((red << 16) | (green << 8)) | blue);
		}
		row += buffer->pitch;
	}
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *buffer,
                      int _width, int _height)
{
	buffer->height = _height;
	buffer->width = _width;
	int bytesPerPixel = 4;
    
	if (buffer->memory)
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
	buffer->memory = VirtualAlloc(0, bufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = _width * bytesPerPixel;
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
	switch (message)
	{
        case WM_SIZE:
        {
        } break;
        
        case WM_DESTROY: {
            //TODO: Send a message to the user?
            GlobalRunning = false;
        } break;
        
        case WM_CLOSE: {
            
            //TODO: Try to recreate window? Handle as error?
            GlobalRunning = false;
        } break;
        
        case WM_ACTIVATEAPP: {
            
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32_t VKCode = wParam;
            bool32 altKeyWasDown = (lParam & (1 << 29));
            if ((VKCode == VK_F4) && altKeyWasDown)
            {
                GlobalRunning = false;
            }
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

struct win32_SoundOutput
{
    int SamplesPerSecond;
    int ToneHz;
    int16_t ToneVolume;
    uint32_t SecondaryBufferIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    float tSine;
    int LatencySampleCount;
};

internal void
win32_FillSoundOutput(win32_SoundOutput *SoundOutput, DWORD _byteToLock, DWORD _bytesToWrite)
{
    //Lock buffer
    void *Region1;
    DWORD Region1Size;
    void *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
                                              _byteToLock,_bytesToWrite,
                                              &Region1,&Region1Size,
                                              &Region2, &Region2Size,
                                              0)
                  )
        )
    {
        int16_t *Region1Sample = (int16_t *)Region1;
        
        for (int i = 0; i < Region1Size / SoundOutput->BytesPerSample; ++i)
        {
            float SineValue = sinf(SoundOutput->tSine);
            int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);  
            *Region1Sample++ = SampleValue;
            *Region1Sample++ = SampleValue;
            
            SoundOutput->tSine += 2.0f*PI_32*1.0f/(float)SoundOutput->WavePeriod;
            ++SoundOutput->SecondaryBufferIndex;
        }
        
        int16_t *Region2Sample = (int16_t *)Region2;
        
        for (int i = 0; i < Region2Size / SoundOutput->BytesPerSample; ++i)
        {
            float SineValue = sinf(SoundOutput->tSine);
            int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
            *Region2Sample++ = SampleValue;
            *Region2Sample++ = SampleValue;
            
            SoundOutput->tSine += 2.0f*PI_32*1.0f/(float)SoundOutput->WavePeriod;
            ++SoundOutput->SecondaryBufferIndex;
        }
        
        //Unlock the buffer after writing.
        GlobalSecondaryBuffer->Unlock(Region1,Region1Size,Region2,Region2Size);
    }
    
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
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = instance;
	//windowClass.hIcon;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";
    
	ATOM uniqueClassId = RegisterClassExA(&windowClass);
	if (uniqueClassId)
	{
		//create window and look at the queue
		HWND window = CreateWindowExA(
                                      0,
                                      "HandmadeHeroWindowClass",
                                      "Handmade Hero",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      0,
                                      0,
                                      instance,
                                      0);
        
		if (window)
		{
			HDC deviceContext = GetDC(window);
			MSG message;
            
			GlobalRunning = true;
            
			//Graphics Test
			uint8_t xOffset = 0, yOffset = 0;
            
			//Audio Test
			
            win32_SoundOutput SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.ToneHz = 256;
            SoundOutput.ToneVolume = 1000;
            SoundOutput.BytesPerSample = sizeof(int16_t) * 2; //16 bits per channel, 2 channels.
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
            
			//NOTE(chris): To initialize Direct sound, we need to have a window, so wait until we have a window to initialize!
			Win32InitSound(window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            win32_FillSoundOutput(&SoundOutput, 0, SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample);
            //PlayBuffer
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
			while (GlobalRunning)
			{
                
				while (PeekMessage(&message, window, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}
					TranslateMessage(&message);
					DispatchMessageA(&message);
                    
				}
				//TODO(chris): Should we poll this more frequent?
				//TODO(chris): This is bad performance! Only poll plugged in devices to prevent stall
				for (DWORD controllerIndex = 0;
                     controllerIndex < XUSER_MAX_COUNT;
                     ++controllerIndex)
				{
                    
					XINPUT_STATE inputState;
					DWORD inputReturnCode = XInputGetState(controllerIndex, &inputState);
					if (inputReturnCode == ERROR_SUCCESS)
					{
						XINPUT_GAMEPAD *gamePad = &inputState.Gamepad;
                        
						bool32 Up = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool32 Down = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool32 Left = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool32 Right = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool32 Start = (gamePad->wButtons & XINPUT_GAMEPAD_START);
						bool32 Back = (gamePad->wButtons & XINPUT_GAMEPAD_BACK);
						bool32 LeftShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool32 RightShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool32 AButton = (gamePad->wButtons & XINPUT_GAMEPAD_A);
						bool32 BButton = (gamePad->wButtons & XINPUT_GAMEPAD_B);
						bool32 XButton = (gamePad->wButtons & XINPUT_GAMEPAD_X);
						bool32 YButton = (gamePad->wButtons & XINPUT_GAMEPAD_Y);
                        
						int16_t StickX = gamePad->sThumbLX;
						int16_t StickY = gamePad->sThumbLY;
                        
						xOffset += gamePad->sThumbLX >> 12;
						yOffset += gamePad->sThumbLY >> 12;
					}
					else
					{
						//TODO Add error handling, figure out what to show user.
					}
				}
                
                DWORD PlayCursor;
                DWORD WriteCursor;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD LockByte = (SoundOutput.SecondaryBufferIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                    DWORD BytesToWrite;
                    DWORD TargetCursor =((PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
                    
                    if (LockByte > TargetCursor)
                    {
                        BytesToWrite = (SoundOutput.SecondaryBufferSize - LockByte);
                        BytesToWrite += TargetCursor;
                    }
                    else
                    {
                        BytesToWrite = TargetCursor - LockByte;
                    }
                    
                    win32_FillSoundOutput(&SoundOutput, LockByte, BytesToWrite);
                }
				WindowGradient(&GlobalBackBuffer, ++xOffset, ++yOffset);
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
