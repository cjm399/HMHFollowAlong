/* date = January 2nd 2021 8:56 pm */

#ifndef HANDMADE_H

struct game_sound_buffer
{
    int16_t *samples;
    int sampleCount;
    int samplesPerSecond;
};

struct game_offscreen_buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};

internal void GameFillSoundBuffer(game_sound_buffer *_buffer);

internal void GameUpdateAndRender(game_offscreen_buffer *_buffer, int _xOffset, int _yOffset);

#define HANDMADE_H
#endif //HANDMADE_H
