/* date = January 2nd 2021 8:56 pm */

#ifndef HANDMADE_H

struct game_offscreen_buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};

internal void GameUpdateAndRender(game_offscreen_buffer *_buffer, int _xOffset, int _yOffset);

#define HANDMADE_H
#endif //HANDMADE_H
