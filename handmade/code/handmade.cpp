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
        
        tSine += 2.0f*PI_32*1.0f/(float)wavePeriod;
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
            uint8_t green = X + _xOffset;
            uint8_t blue = Y + _yOffset;
            *pixel++ = (((red << 16) | (green << 8)) | blue);
        }
        row += _buffer->pitch;
    }
}

internal void 
GameUpdateAndRender(game_offscreen_buffer *_renderBuffer, game_sound_buffer* _soundBuffer, game_input* _playerInput)
{
    local_persist int toneHz = 256;
    local_persist int yOffset = 0;
    
    game_controller_input *controllerInput = &_playerInput->player0Input;
    
    toneHz = 256 + (int)(128.0f * controllerInput->endY);
    if(controllerInput->bottomButton.isDown)
    {
        yOffset += 1;
    }
    RenderWeirdGradient(_renderBuffer, 0, yOffset);
    GameFillSoundBuffer(_soundBuffer, toneHz);
}
