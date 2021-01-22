/* date = January 2nd 2021 8:56 pm */

#ifndef HANDMADE_H

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

struct game_button_state
{
    uint8_t halfStepCount;
    bool32 isDown;
};

struct game_controller_input
{
    float startX;
    float startY;
    
    float minX;
    float minY;
    
    float maxX;
    float maxY;
    
    float endX;
    float endY;
    
    union{
        struct{
            game_button_state bottomButton;
            game_button_state rightButton;
            game_button_state leftButton;
            game_button_state topButton;
            game_button_state leftShoulder;
            game_button_state rightShoulder;
            game_button_state start;
            game_button_state back;
            game_button_state dDown;
            game_button_state dRight;
            game_button_state dLeft;
            game_button_state dUp;
        };
        game_button_state *buttonInputs[12];
    };
};

struct game_input
{
    union{
        struct
        {
            game_controller_input player0Input;
            game_controller_input player1Input;
            game_controller_input player2Input;
            game_controller_input player3Input;
        };
        game_controller_input allInput[4];
    };
};

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

internal void GameFillSoundBuffer(game_sound_buffer *_buffer, int toneHz);

internal void GameUpdateAndRender(game_offscreen_buffer *_renderBuffer, game_sound_buffer *_soundBuffer, game_input *_playerInput);

#define HANDMADE_H
#endif //HANDMADE_H
