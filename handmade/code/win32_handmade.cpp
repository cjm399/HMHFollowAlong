
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static 
#define PI_32 3.14159265358


typedef int32_t bool32;

#include "handmade.cpp"

#include <windows.h>
#include <winuser.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>


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
    int bytesPerSample;
    int wavePeriod;
    float tSine;
    int secondaryBufferSize;
    int latencySampleCount;
    uint32_t secondaryBufferIndex;
    int16_t toneVolume;
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
global_variable uint64_t GlobalPerformanceCountFrequency;


//NOTE(chris): This is a debug function! Make sure to replace later
#if HANDMADE_INTERNAL

internal file_data
DEBUGPlatformReadEntireFile(char *_fileName)
{
    file_data fileData {};
    
    HANDLE fileHandle = CreateFileA(_fileName, GENERIC_READ, FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if(GetFileSizeEx(fileHandle, &fileSize))
        {
            DWORD fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
            fileData.contents = VirtualAlloc(0, fileSize32, 
                                             MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            DWORD bytesRead;
            if(ReadFile(fileHandle, fileData.contents, fileSize32, &bytesRead, 0) && bytesRead == fileSize32)
            {
                //TODO: Logging
                fileData.size = bytesRead;
                OutputDebugStringA("ReadFile!");
            }
            else
            {
                //TODO(chris): Logging
                DEBUGPlatformFreeMemory(fileData.contents);
            }
        }
        if(CloseHandle(fileHandle))
        {
            //TODO(chris): Logging.
        }
        else
        {
            //TODO(chris): Logging.
        }
    }
    else
    {
        //TODO(chris): Logging
    }
    return fileData;
}

internal void
DEBUGPlatformFreeMemory(void *_memory)
{
    if(_memory)
    {
        VirtualFree(_memory, 0, MEM_RELEASE);
    }
}

internal bool32
DEBUGPlatformWriteEntireFile(void* _buffer, uint64_t _bufferSize, char *_fileName)
{
    bool32 result = false;
    HANDLE fileHandle = CreateFileA(_fileName, GENERIC_WRITE, 0, 0,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        DWORD bytesToWrite = SafeTruncateUInt64(_bufferSize);
        if(WriteFile(fileHandle, _buffer, bytesToWrite, &bytesWritten, 0) && bytesWritten == bytesToWrite)
        {
            OutputDebugStringA("Wrote To File!");
            result = true;
            //TODO:Logging
        }
        else
        {
            //TODO:Logging
        }
        if(CloseHandle(fileHandle))
        {
            //TODO:Logging
        }
        else
        {
            //TODO:Logging
        }
    }
    else
    {
        //TODO(chris):Logging
    }
    return result;
}

#endif

internal void Win32LoadXInput()
{
	HMODULE xInputLib = LoadLibraryA("xinput1_4.dll");
	if (!xInputLib)
	{
		xInputLib = LoadLibraryA("xinput1_3.dll");
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
            Assert(!"Keyboard input came through non-dispatch message!");
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
Win32ClearSoundBuffer(win32_sound_output *_soundOutput)
{
    //Lock buffer
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
                                              0,_soundOutput->secondaryBufferSize,
                                              &region1,&region1Size,
                                              &region2, &region2Size,
                                              0)
                  )
        )
    {
        uint8_t *Byte = (uint8_t *)region1;
        for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex)
        {
            *Byte++ = 0;
        }
        
        Byte = (uint8_t *) region2;
        for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex)
        {
            *Byte++ = 0;
        }
        GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *_destBuffer,
                     DWORD _byteToLock, DWORD _bytesToWrite,
                     game_sound_buffer *_sourceBuffer)
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
        int16_t *sourceSample = (int16_t *) _sourceBuffer->samples;
        
        int16_t *destSample = (int16_t *)region1;
        for (DWORD i = 0; i < region1Size / _destBuffer->bytesPerSample; ++i)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++_destBuffer->secondaryBufferIndex;
        }
        
        destSample = (int16_t *)region2;
        for (DWORD i = 0; i < region2Size / _destBuffer->bytesPerSample; ++i)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++_destBuffer->secondaryBufferIndex;
        }
        
        //Unlock the buffer after writing.
        GlobalSecondaryBuffer->Unlock(region1,region1Size,region2,region2Size);
    }
    
}

internal void 
Win32ProcessDigitalButton(game_button_state *_newState, game_button_state *_oldState, 
                          WORD _button, DWORD _buttonBits)
{
    _newState->isDown = ((_button & _buttonBits) == _buttonBits);
    _newState->halfStepCount = (_oldState->isDown == _newState->isDown) ? 0 : 1;
}

internal void
Win32ProcessKeyboardButton(game_button_state *_newState, bool32 _isDown)
{
    _newState->isDown = _isDown;
    ++_newState->halfStepCount;
}

internal float
Win32ProcessXInputStickValue(SHORT _stickValue, SHORT _deadZone)
{
    float result = 0;
    
    if(_stickValue > _deadZone)
    {
        result =  (float)(_stickValue + _deadZone) / (32767.0f - _deadZone);
    }
    else if(_stickValue < -_deadZone)
    {
        result = (float)(_stickValue + _deadZone) / (32768.0f - _deadZone);
    }
    
    return result;
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline float
Win32GetSecondsElapsed(LARGE_INTEGER _start, LARGE_INTEGER _end)
{
    return (float)(_end.QuadPart - _start.QuadPart) / (float)GlobalPerformanceCountFrequency;
}

int WINAPI WinMain(HINSTANCE _instance,
                   HINSTANCE _previousInstance,
                   LPSTR _commandLine,
                   int _showCode)

{
    //NOTE(chris): Can initialize perfCountFrquency once at start.
    LARGE_INTEGER perfCountFrqResult;
    QueryPerformanceFrequency(&perfCountFrqResult);
    GlobalPerformanceCountFrequency = perfCountFrqResult.QuadPart;
    
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
            Win32InitSound(window,soundOutput.samplesPerSecond,soundOutput.secondaryBufferSize);
            Win32ClearSoundBuffer(&soundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            int16_t *soundSamples = (int16_t *)VirtualAlloc(0, 
                                                            soundOutput.secondaryBufferSize,
                                                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            game_memory gameMemory{};
            gameMemory.permanentMemorySize = Mebibytes(64);
            gameMemory.transientMemorySize = Gibibytes(4);
            uint64_t totalMemorySize = gameMemory.permanentMemorySize + gameMemory.transientMemorySize;
            
#if HANDMADE_INTERNAL
            LPVOID baseAddress = (LPVOID)Tebibytes(2);
#else
            LPVOID baseAddress = 0;
#endif
            
            gameMemory.permanentStorage = VirtualAlloc(baseAddress, (size_t)totalMemorySize, 
                                                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            gameMemory.transientStorage = ((uint8_t *)gameMemory.permanentStorage 
                                           + gameMemory.permanentMemorySize);
            
            game_input input[2];
            game_input *gameInput = &input[0];
            game_input *oldInput = &input[1];
            
            ZeroMemory(&input[0], sizeof(input));
            
            Assert(&gameInput->player0Input.TERMINAL - &gameInput->player0Input.buttonInputs[0] == ArrayCount(gameInput->player0Input.buttonInputs));
            
            int monitorRefreshRate = 60;
            int gameRefreshRate = monitorRefreshRate / 2;
            float targetSecondsPerFrame = 1.0f / gameRefreshRate;
            //NOTE(chris): Sets the ms between windows scheduler looking at if we should wake up.
            timeBeginPeriod(1);
            
            LARGE_INTEGER lastTimer;
            QueryPerformanceCounter(&lastTimer);
            uint64_t startClock  = __rdtsc();
            
            while (GlobalRunning)
            {
                game_controller_input *newKeyboardController = GetControllerInput(gameInput, 0);
                game_controller_input *oldKeyboardController = GetControllerInput(oldInput, 0);
                
                *newKeyboardController = {};
                newKeyboardController->isAnalog = true;
                newKeyboardController->isConnected = true;
                for(int buttonIndex = 0; buttonIndex < ArrayCount(newKeyboardController->buttonInputs);
                    ++buttonIndex)
                {
                    newKeyboardController->buttonInputs[buttonIndex].isDown = oldKeyboardController->buttonInputs[buttonIndex].isDown;
                }
                
                while (PeekMessage(&message, window, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }
                    switch(message.message)
                    {
                        case WM_SYSKEYDOWN:
                        case WM_SYSKEYUP:
                        case WM_KEYDOWN:
                        case WM_KEYUP:
                        {
                            uint32_t VKCode = (uint32_t)message.wParam;
                            bool32 altKeyWasDown = (message.lParam & (1 << 29));
                            bool32 wasDown = ((message.lParam & (1 << 30)) != 0);
                            bool32 isDown = ((message.lParam & (1 << 31)) == 0);
                            
                            if(wasDown != isDown)
                            {
                                switch(VKCode)
                                {
                                    case 'W':
                                    {
                                        newKeyboardController->dUp.isDown = isDown;
                                        newKeyboardController->sUp.isDown = isDown;
                                        break;
                                    }
                                    case 'S':
                                    {
                                        newKeyboardController->dDown.isDown = isDown;
                                        newKeyboardController->sDown.isDown = isDown;
                                        break;
                                    }
                                    case 'A':
                                    {
                                        newKeyboardController->dLeft.isDown = isDown;
                                        newKeyboardController->sLeft.isDown = isDown;
                                        break;
                                    }
                                    case 'D':
                                    {
                                        newKeyboardController->dRight.isDown = isDown;
                                        newKeyboardController->sRight.isDown = isDown;
                                        break;
                                    }
                                    case 'E':
                                    {
                                        Win32ProcessKeyboardButton(&newKeyboardController->rightShoulder, isDown);
                                    }
                                    case 'Q':
                                    {
                                        Win32ProcessKeyboardButton(&newKeyboardController->leftShoulder, isDown);
                                        break;
                                    }
                                    case VK_UP:
                                    {
                                        Win32ProcessKeyboardButton(&newKeyboardController->actionTop, isDown);
                                        break;
                                    }
                                    case VK_DOWN:
                                    {
                                        Win32ProcessKeyboardButton(&newKeyboardController->actionBottom, isDown);
                                        break;
                                    }
                                    case VK_RIGHT:
                                    {
                                        Win32ProcessKeyboardButton(&newKeyboardController->actionRight, isDown);
                                        break;
                                    }
                                    case VK_LEFT:
                                    {
                                        Win32ProcessKeyboardButton(&newKeyboardController->actionLeft, isDown);
                                        break;
                                    }
                                    case VK_F4:
                                    {
                                        if (altKeyWasDown)
                                        {
                                            GlobalRunning = false;
                                        }
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        default:
                        {
                            TranslateMessage(&message);
                            DispatchMessageA(&message);
                        }
                    }
                    
                }
                //TODO(chris): Should we poll this more frequent?
                //TODO(chris): This is bad performance! Only poll plugged in devices to prevent stall
                DWORD maxControllerCount = XUSER_MAX_COUNT;
                if(maxControllerCount > (GetControllerCount(gameInput) - 1))
                {
                    maxControllerCount = GetControllerCount(gameInput) - 1;
                }
                
                for (DWORD controllerIndex = 0;
                     controllerIndex < maxControllerCount;
                     ++controllerIndex)
                {
                    //Need to skip the keyboard controller.
                    DWORD realControllerIndex = controllerIndex + 1;
                    
                    game_controller_input *oldController = GetControllerInput(oldInput, realControllerIndex);
                    
                    game_controller_input *newController = GetControllerInput(gameInput, realControllerIndex);
                    
                    XINPUT_STATE inputState;
                    ZeroMemory(&inputState, sizeof(XINPUT_STATE));
                    DWORD inputReturnCode = XInputGetState(realControllerIndex, &inputState);
                    if (inputReturnCode == ERROR_SUCCESS)
                    {
                        newController->isConnected = true;
                        newController->isAnalog = false;
                        XINPUT_GAMEPAD *gamePad = &inputState.Gamepad;
                        
                        //RealSticks
                        float rStickX = Win32ProcessXInputStickValue(gamePad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                        float rStickY = Win32ProcessXInputStickValue(gamePad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                        newController->avgRSX = rStickX;
                        newController->avgRSY = rStickY;
                        
                        //DPAD
                        Win32ProcessDigitalButton(&newController->dUp, &oldController->dUp, gamePad->wButtons, XINPUT_GAMEPAD_DPAD_UP);
                        Win32ProcessDigitalButton(&newController->dDown, &oldController->dDown, gamePad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
                        Win32ProcessDigitalButton(&newController->dRight, &oldController->dRight, gamePad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
                        Win32ProcessDigitalButton(&newController->dLeft, &oldController->dLeft, gamePad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
                        
                        //To Handle DPad as same as stick.
                        if(newController->dUp.isDown)
                        {
                            newController->isAnalog = true;
                            newController->avgRSY = 1;
                        }
                        if(newController->dDown.isDown)
                        {
                            newController->isAnalog = true;
                            newController->avgRSY = -1;
                        }
                        if(newController->dLeft.isDown)
                        {
                            newController->isAnalog = true;
                            newController->avgRSX = -1;
                        }
                        if(newController->dRight.isDown)
                        {
                            newController->isAnalog = true;
                            newController->avgRSX = 1;
                        }
                        
                        //Start/Back
                        Win32ProcessDigitalButton(&newController->start, &oldController->start, gamePad->wButtons, XINPUT_GAMEPAD_START);
                        Win32ProcessDigitalButton(&newController->back, &oldController->back, gamePad->wButtons, XINPUT_GAMEPAD_BACK);
                        //Shoulders
                        Win32ProcessDigitalButton(&newController->leftShoulder, &oldController->leftShoulder, gamePad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
                        Win32ProcessDigitalButton(&newController->rightShoulder, &oldController->rightShoulder, gamePad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        //Action Buttons
                        Win32ProcessDigitalButton(&newController->actionBottom, &oldController->actionBottom, gamePad->wButtons, XINPUT_GAMEPAD_A);
                        Win32ProcessDigitalButton(&newController->actionRight, &oldController->actionRight, gamePad->wButtons, XINPUT_GAMEPAD_B);
                        Win32ProcessDigitalButton(&newController->actionLeft, &oldController->actionLeft, gamePad->wButtons, XINPUT_GAMEPAD_X);
                        Win32ProcessDigitalButton(&newController->actionTop, &oldController->actionTop, gamePad->wButtons, XINPUT_GAMEPAD_Y);
                        
                        //Fake Sticks
                        float fakeStickThreshold = .4f;
                        Win32ProcessDigitalButton(&newController->sLeft, 
                                                  &oldController->sLeft,
                                                  (newController->avgRSX <-fakeStickThreshold)? 1 : 0,
                                                  1);
                        Win32ProcessDigitalButton(&newController->sRight, 
                                                  &oldController->sRight,
                                                  (newController->avgRSX >fakeStickThreshold)? 1 : 0,
                                                  1);
                        Win32ProcessDigitalButton(&newController->sUp, 
                                                  &oldController->sUp,
                                                  (newController->avgRSY >fakeStickThreshold)? 1 : 0,
                                                  1);
                        Win32ProcessDigitalButton(&newController->sDown, 
                                                  &oldController->sDown,
                                                  (newController->avgRSY <-fakeStickThreshold)? 1 : 0,
                                                  1);
                    }
                    else
                    {
                        newController->isConnected = false;
                        //TODO Add error handling, figure out what to show user.
                    }
                }
                
                DWORD playCursor;
                DWORD writeCursor;
                DWORD targetCursor;
                DWORD lockByte = 0;
                DWORD bytesToWrite = 0;
                bool32 isSoundValid = false;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                {
                    lockByte = (soundOutput.secondaryBufferIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                    
                    targetCursor = ((playCursor + (soundOutput.latencySampleCount*soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize);
                    
                    if (lockByte > targetCursor)
                    {
                        bytesToWrite = (soundOutput.secondaryBufferSize - lockByte);
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - lockByte;
                    }
                    isSoundValid = true;
                }
                
                game_sound_buffer soundBuffer = {};
                soundBuffer.samples = soundSamples;
                soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
                soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                
                if(isSoundValid)
                {
                    Win32FillSoundBuffer(&soundOutput, lockByte, bytesToWrite, &soundBuffer);
                }
                
                game_offscreen_buffer graphicsBuffer{};
                graphicsBuffer.width = GlobalBackBuffer.width;
                graphicsBuffer.height = GlobalBackBuffer.height;
                graphicsBuffer.pitch = GlobalBackBuffer.pitch;
                graphicsBuffer.memory = GlobalBackBuffer.memory;
                
                GameUpdateAndRender(&gameMemory, &graphicsBuffer, &soundBuffer, gameInput);
                
                LARGE_INTEGER workTimer = Win32GetWallClock();
                float totalFrameTime = Win32GetSecondsElapsed(lastTimer, workTimer);
                
                float remainingFrameTime = targetSecondsPerFrame - totalFrameTime;
                
                
                if(totalFrameTime < targetSecondsPerFrame)
                {
                    if(remainingFrameTime > 0)
                    {
                        Sleep((DWORD)(remainingFrameTime * 1000.0f));
                    }
                    while(totalFrameTime < targetSecondsPerFrame)
                    {
                        totalFrameTime = Win32GetSecondsElapsed(lastTimer, Win32GetWallClock());
                    }
                }
                else
                {
                    //TODO(chris): LOG MISSED FRAME TIMING!
                    OutputDebugStringA("MISSED FRAME TIME\n");
                }
                
                win32_window_dimension dimensions = Win32GetWindowDimension(window);
                Win32CopyBufferToWindow(deviceContext,
                                        dimensions.width, dimensions.height,
                                        &GlobalBackBuffer);
                
                
                game_input* tmp = gameInput;
                gameInput = oldInput;
                oldInput = tmp;
                
                LARGE_INTEGER endTimer = Win32GetWallClock();
                
                double msPerFrame = 1000.0f * Win32GetSecondsElapsed(lastTimer, endTimer);
                double fpsCounter  = 1000.0/msPerFrame;
                uint64_t currClock = __rdtsc();
                uint64_t clockDiff = currClock - startClock;
                double mcpf = (double)clockDiff / 1000000.0;
                
                char Buffer[256];
                _snprintf_s(Buffer, sizeof(Buffer),
                            "%.02f ms/f, %.02f f/s, %.02f mc/f\n", msPerFrame, fpsCounter, mcpf);
                OutputDebugStringA(Buffer);
                lastTimer= endTimer;
                startClock = currClock;
                
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
