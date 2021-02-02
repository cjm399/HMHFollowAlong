#include "handmade.h"

#include <math.h>

internal void 
GameFillSoundBuffer(game_sound_buffer *_buffer, int toneHz)
{
    local_persist float tSine;
    int16_t toneVolume = 3000;
    int wavePeriod = _buffer->samplesPerSecond / toneHz;
    
    int16_t *sampleOut = _buffer->samples;
    
    for (int sampleIndex = 0; sampleIndex < _buffer->sampleCount; ++sampleIndex)
    {
        float sineValue = sinf(tSine);
        int16_t sampleValue = (int16_t)(sineValue * toneVolume);  
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        
        tSine += float(2.0f*PI_32*1.0f/(float)wavePeriod);
    }
}

internal void 
RenderWeirdGradient(game_offscreen_buffer *_buffer, int _xOffset, int _yOffset)
{
    uint8_t *row = (uint8_t *)_buffer->memory;
    for (int Y = 0; Y < _buffer->height; Y++)
    {
        uint32_t *pixel = (uint32_t *)row;
        for (int X = 0; X < _buffer->width; X++)
        {
            uint8_t red = 255;
            uint8_t green = (uint8_t)(X + _xOffset);
            uint8_t blue = (uint8_t)(Y + _yOffset);
            *pixel++ = (((red << 16) | (green << 8)) | blue);
        }
        row += _buffer->pitch;
    }
}

#include <windows.h>

internal void 
GameUpdateAndRender(game_memory* _memory, game_offscreen_buffer *_renderBuffer, game_sound_buffer* _soundBuffer, game_input* _playerInput)
{
    Assert(_memory->permanentMemorySize >= sizeof(game_state));
    
    game_state *gameState = (game_state *)(_memory->permanentStorage);
    
    if(!_memory->isInitialized)
    {
        gameState->toneHz = 256;
        
        file_data data {};
        
        data = DEBUGPlatformReadEntireFile("nonExistingFile.txt");
        
        //NOTE(chris): If file fails, we have 0 size memory.
        if(data.size != 0)
        {
            DEBUGPlatformWriteEntireFile(data.contents, data.size, "testWriteFile.txt");
            DEBUGPlatformFreeMemory(data.contents);
        }
        _memory->isInitialized = true;
    }
    game_controller_input *controllerInput = &_playerInput->player0Input;
    
    gameState->toneHz = 256 + (int)(128.0f * controllerInput->endY);
    if(controllerInput->bottomButton.isDown)
    {
        gameState->yOffset += 1;
    }
    RenderWeirdGradient(_renderBuffer, 0, gameState->yOffset);
    GameFillSoundBuffer(_soundBuffer, gameState->toneHz);
}
