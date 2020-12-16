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
	BITMAPINFO info;
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


struct win32_sound_output
{
    int samplesPerSecond;
    int toneHz;
    int16_t toneVolume;
    uint32_t secondaryBufferIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    float tSine;
    int latencySampleCount;
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

internal void Win32InitSound(HWND _window, int32_t _samplesPerSecond, int32_t _bufferSize)
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
			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = _samplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
			waveFormat.nAvgBytesPerSec =  waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;
			waveFormat.cbSize = 0;
            
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(_window, DSSCL_PRIORITY)))
			{
				//NOTE(chris): Set buffer struct to 0. Primary size 0.
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
				{
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
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
			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = _bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
            
			HRESULT error = DirectSound->CreateSoundBuffer(&bufferDescription, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(error))
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
Win32GetWindowDimension(HWND _window)
{
	win32_window_dimension result;
    
	RECT clientRect;
	GetClientRect(_window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;
    
	return(result);
}

internal void 
WindowGradient(win32_offscreen_buffer *_buffer, uint8_t _xOffset, uint8_t _yOffset)
{
	uint8_t *row = (uint8_t *)_buffer->memory;
	for (int Y = 0; Y < _buffer->height; Y++)
	{
		uint32_t *pixel = (uint32_t *)row;
		for (int X = 0; X < _buffer->width; X++)
		{
			uint8_t red = 255;
			uint8_t green = X + _xOffset;
			uint8_t blue = Y + _yOffset;
			*pixel++ = (((red << 16) | (green << 8)) | blue);
		}
		row += _buffer->pitch;
	}
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *_buffer,
                      int _width, int _height)
{
	_buffer->height = _height;
	_buffer->width = _width;
	int bytesPerPixel = 4;
    
	if (_buffer->memory)
	{
		VirtualFree(_buffer->memory, 0, MEM_RELEASE);
	}
    
	_buffer->info.bmiHeader.biSize = sizeof(_buffer->info.bmiHeader);
	_buffer->info.bmiHeader.biWidth = _width;
	_buffer->info.bmiHeader.biHeight = -_height;
	_buffer->info.bmiHeader.biPlanes = 1;
	_buffer->info.bmiHeader.biBitCount = 32;
	_buffer->info.bmiHeader.biCompression = BI_RGB;
    
	SIZE_T _bufferSize = bytesPerPixel * (_width * _height);
	_buffer->memory = VirtualAlloc(0, _bufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	_buffer->pitch = _width * bytesPerPixel;
}

internal void
Win32CopyBufferToWindow(HDC _deviceContext,
                        int _windowWidth, int _windowHeight,
                        win32_offscreen_buffer *_buffer)
{
	StretchDIBits(_deviceContext,
                  /*_x, _y, _width, _height,
                    _x, _y, _width, _height,*/
                  0, 0, _windowWidth, _windowHeight,
                  0, 0, _buffer->width, _buffer->height,
                  _buffer->memory,
                  &_buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(
                        HWND _window,
                        UINT _message,
                        WPARAM _wParam,
                        LPARAM _lParam)
{
	LRESULT result = 0;
	switch (_message)
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
            uint32_t VKCode = _wParam;
            bool32 altKeyWasDown = (_lParam & (1 << 29));
            if ((VKCode == VK_F4) && altKeyWasDown)
            {
                GlobalRunning = false;
            }
        } break;
        
        
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(_window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            
            win32_window_dimension dimensions = Win32GetWindowDimension(_window);
            Win32CopyBufferToWindow(deviceContext,
                                    dimensions.width, dimensions.height,
                                    &GlobalBackBuffer);
            EndPaint(_window, &paint);
        }   break;
        
        default:
		result = DefWindowProc(_window, _message, _wParam, _lParam);
		break;
	}
	return (result);
}

internal void
Win32FillSoundBuffer(win32_sound_output *_soundOutput, DWORD _byteToLock, DWORD _bytesToWrite)
{
    //Lock buffer
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
                                              _byteToLock,_bytesToWrite,
                                              &region1,&region1Size,
                                              &region2, &region2Size,
                                              0)
                  )
        )
    {
        int16_t *region1Output = (int16_t *)region1;
        
        for (int i = 0; i < region1Size / _soundOutput->bytesPerSample; ++i)
        {
            float sineValue = sinf(_soundOutput->tSine);
            int16_t sampleValue = (int16_t)(sineValue * _soundOutput->toneVolume);  
            *region1Output++ = sampleValue;
            *region1Output++ = sampleValue;
            
            _soundOutput->tSine += 2.0f*PI_32*1.0f/(float)_soundOutput->wavePeriod;
            ++_soundOutput->secondaryBufferIndex;
        }
        
        int16_t *region2Output = (int16_t *)region2;
        
        for (int i = 0; i < region2Size / _soundOutput->bytesPerSample; ++i)
        {
            float sineValue = sinf(_soundOutput->tSine);
            int16_t sampleValue = (int16_t)(sineValue * _soundOutput->toneVolume);
            *region2Output++ = sampleValue;
            *region2Output++ = sampleValue;
            
            _soundOutput->tSine += 2.0f*PI_32*1.0f/(float)_soundOutput->wavePeriod;
            ++_soundOutput->secondaryBufferIndex;
        }
        
        //Unlock the buffer after writing.
        GlobalSecondaryBuffer->Unlock(region1,region1Size,region2,region2Size);
    }
    
}

int WINAPI WinMain(HINSTANCE _instance,
                   HINSTANCE _previousInstance,
                   LPSTR _commandLine,
                   int _showCode)

{
    
	WNDCLASSEXA windowClass = {};
    
	Win32LoadXInput();
	//win32_window_dimension dimensions = Win32GetWindowDimension(window);
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
    
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = _instance;
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
                                      _instance,
                                      0);
        
		if (window)
		{
			HDC deviceContext = GetDC(window);
			MSG message;
            
			GlobalRunning = true;
            
			//Graphics Test
			uint8_t xOffset = 0, yOffset = 0;
            
			//Audio Test
            win32_sound_output soundOutput = {};
            soundOutput.samplesPerSecond = 48000;
            soundOutput.toneHz = 256;
            soundOutput.toneVolume = 1000;
            soundOutput.bytesPerSample = sizeof(int16_t) * 2; //16 bits per channel, 2 channels.
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
            soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
            
			//NOTE(chris): To initialize Direct sound, we need to have a window, so wait until we have a window to initialize!
			Win32InitSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
            Win32FillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);
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
                        
						bool32 up = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool32 down = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool32 left = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool32 right = (gamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool32 start = (gamePad->wButtons & XINPUT_GAMEPAD_START);
						bool32 back = (gamePad->wButtons & XINPUT_GAMEPAD_BACK);
						bool32 leftShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool32 rightShoulder = (gamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool32 aButton = (gamePad->wButtons & XINPUT_GAMEPAD_A);
						bool32 bButton = (gamePad->wButtons & XINPUT_GAMEPAD_B);
						bool32 xButton = (gamePad->wButtons & XINPUT_GAMEPAD_X);
						bool32 yButton = (gamePad->wButtons & XINPUT_GAMEPAD_Y);
                        
						int16_t stickX = gamePad->sThumbLX;
						int16_t stickY = gamePad->sThumbLY;
                        
						xOffset += gamePad->sThumbLX / 4096;
						yOffset += gamePad->sThumbLY / 4096;
                        
                        //soundOutput.toneHz = 256 +  yOffset;
                        //soundOutput.wavePeriod =soundOutput.samplesPerSecond / soundOutput.toneHz;
					}
					else
					{
						//TODO Add error handling, figure out what to show user.
					}
				}
                
                DWORD playCursor;
                DWORD writeCursor;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                {
                    DWORD lockByte = (soundOutput.secondaryBufferIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                    DWORD bytesToWrite;
                    DWORD targetCursor =((playCursor + (soundOutput.latencySampleCount*soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize);
                    
                    if (lockByte > targetCursor)
                    {
                        bytesToWrite = (soundOutput.secondaryBufferSize - lockByte);
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - lockByte;
                    }
                    
                    Win32FillSoundBuffer(&soundOutput, lockByte, bytesToWrite);
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
