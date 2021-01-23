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
        for (int i = 0; i < region1Size / _destBuffer->bytesPerSample; ++i)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++_destBuffer->secondaryBufferIndex;
        }
        
        destSample = (int16_t *)region2;
        for (int i = 0; i < region2Size / _destBuffer->bytesPerSample; ++i)
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
                          XINPUT_GAMEPAD *_gamePad, DWORD _buttonBits)
{
    _newState->isDown = ((_gamePad->wButtons & _buttonBits) == _buttonBits);
    _newState->halfStepCount = (_oldState->isDown == _newState->isDown) ? 0 : 1;
}

int WINAPI WinMain(HINSTANCE _instance,
                   HINSTANCE _previousInstance,
                   LPSTR _commandLine,
                   int _showCode)

{
    //NOTE(chris): Can initialize perfCountFrquency once at start.
    LARGE_INTEGER perfCountFrqResult;
    QueryPerformanceFrequency(&perfCountFrqResult);
    int64_t perfCountFrq = perfCountFrqResult.QuadPart;
    
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
            Win32InitSound(window,soundOutput.samplesPerSecond,soundOutput.secondaryBufferSize);
            Win32ClearSoundBuffer(&soundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            int16_t *soundSamples = (int16_t *)VirtualAlloc(0, 
                                                            soundOutput.secondaryBufferSize,
                                                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            game_memory gameMemory{};
            gameMemory.permanentMemorySize = Mebibytes(64);
            gameMemory.transientMemorySize = Gibibytes(4);
            int totalMemorySize = gameMemory.permanentMemorySize + gameMemory.transientMemorySize;
            
#if HANDMADE_INTERNAL
            LPVOID baseAddress = (LPVOID)Tebibytes(2);
#else
            LPVOID baseAddress = 0;
#endif
            
            gameMemory.permanentStorage = VirtualAlloc(baseAddress, totalMemorySize, 
                                                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            gameMemory.transientStorage = ((uint8_t *)gameMemory.permanentStorage 
                                           + gameMemory.permanentMemorySize);
            
            LARGE_INTEGER startTimer;
            QueryPerformanceCounter(&startTimer);
            uint64_t startClock  = __rdtsc();
            
            game_input input[2];
            game_input *gameInput = &input[0];
            game_input *oldInput = &input[1];
            
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
                int maxControllerCount = XUSER_MAX_COUNT;
                if(maxControllerCount > ArrayCount(gameInput->allInput))
                {
                    maxControllerCount = ArrayCount(gameInput->allInput);
                }
                
                for (DWORD controllerIndex = 0;
                     controllerIndex < maxControllerCount;
                     ++controllerIndex)
                {
                    game_controller_input *oldController = &oldInput->allInput[controllerIndex];
                    game_controller_input *newController = &gameInput->allInput[controllerIndex];
                    
                    XINPUT_STATE inputState;
                    ZeroMemory(&inputState, sizeof(XINPUT_STATE));
                    DWORD inputReturnCode = XInputGetState(controllerIndex, &inputState);
                    if (inputReturnCode == ERROR_SUCCESS)
                    {
                        XINPUT_GAMEPAD *gamePad = &inputState.Gamepad;
                        
                        Win32ProcessDigitalButton(&newController->dUp, &oldController->dUp, gamePad, XINPUT_GAMEPAD_DPAD_UP);
                        Win32ProcessDigitalButton(&newController->dDown, &oldController->dDown, gamePad, XINPUT_GAMEPAD_DPAD_DOWN);
                        Win32ProcessDigitalButton(&newController->dRight, &oldController->dRight, gamePad, XINPUT_GAMEPAD_DPAD_RIGHT);
                        Win32ProcessDigitalButton(&newController->dLeft, &oldController->dLeft, gamePad, XINPUT_GAMEPAD_DPAD_LEFT);
                        Win32ProcessDigitalButton(&newController->start, &oldController->start, gamePad, XINPUT_GAMEPAD_START);
                        Win32ProcessDigitalButton(&newController->back, &oldController->back, gamePad, XINPUT_GAMEPAD_BACK);
                        Win32ProcessDigitalButton(&newController->leftShoulder, &oldController->leftShoulder, gamePad, XINPUT_GAMEPAD_LEFT_SHOULDER);
                        Win32ProcessDigitalButton(&newController->rightShoulder, &oldController->rightShoulder, gamePad, XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        Win32ProcessDigitalButton(&newController->bottomButton, &oldController->bottomButton, gamePad, XINPUT_GAMEPAD_A);
                        Win32ProcessDigitalButton(&newController->rightButton, &oldController->rightButton, gamePad, XINPUT_GAMEPAD_B);
                        Win32ProcessDigitalButton(&newController->leftButton, &oldController->leftButton, gamePad, XINPUT_GAMEPAD_X);
                        Win32ProcessDigitalButton(&newController->topButton, &oldController->topButton, gamePad, XINPUT_GAMEPAD_Y);
                        
                        float xOffset = gamePad->sThumbLX;
                        float yOffset = gamePad->sThumbLY;
                        
                        //NOTE(chris):Normalize stick from -1 to 1
                        if(xOffset >= 0)
                        {
                            xOffset /= 32767.0f;
                        }
                        else
                        {
                            xOffset /= 32768.0f;
                        }
                        if(yOffset >= 0)
                        {
                            yOffset /= 32767.0f;
                        }
                        else
                        {
                            yOffset /= 32768.0f;
                        }
                        
                        newController->startX = oldController->endX;
                        newController->endX = xOffset;
                        
                        newController->startY = oldController->endY;
                        newController->endY = yOffset;
                        newController->isAnalog = false;
                    }
                    else
                    {
                        //TODO Add error handling, figure out what to show user.
                    }
                }
                
                DWORD playCursor;
                DWORD writeCursor;
                DWORD targetCursor;
                DWORD lockByte;
                DWORD bytesToWrite;
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
                
                win32_window_dimension dimensions = Win32GetWindowDimension(window);
                Win32CopyBufferToWindow(deviceContext,
                                        dimensions.width, dimensions.height,
                                        &GlobalBackBuffer);
                
                
                LARGE_INTEGER endTimer;
                QueryPerformanceCounter(&endTimer);
                
                int64_t counterElapsed = endTimer.QuadPart - startTimer.QuadPart;
                double msPerFrame = ((1000.0 * (double)counterElapsed) / (double)(perfCountFrq));
                double fpsCounter  = 1000.0/msPerFrame;
                uint64_t currClock = __rdtsc();
                uint64_t clockDiff = currClock - startClock;
                double mcpf = (double)clockDiff / 1000000.0;
                
                char Buffer[256];
                sprintf(Buffer, "%.02f ms/f, %.02f f/s, %.02f mc/f\n", msPerFrame, fpsCounter, mcpf);
                OutputDebugStringA(Buffer);
                
                startTimer = endTimer;
                startClock = currClock;
                
                game_input* tmp = gameInput;
                gameInput = oldInput;
                oldInput = tmp;
                
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
