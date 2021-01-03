
#include "handmade.h"

internal void GameUpdateAndRender(game_offscreen_buffer *_buffer, int _xOffset, int _yOffset)
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
