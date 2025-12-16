#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>

// PC Speaker frequencies (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047

// Duration constants (ms)
#define DUR_WHOLE    1000
#define DUR_HALF     500
#define DUR_QUARTER  250
#define DUR_EIGHTH   125
#define DUR_SIXTEENTH 62

// Sound effect types
typedef enum {
    SFX_BEEP,
    SFX_CLICK,
    SFX_ERROR,
    SFX_SUCCESS,
    SFX_STARTUP,
    SFX_SHUTDOWN,
    SFX_NOTIFICATION,
    SFX_KEYPRESS
} sound_effect_t;

// Note structure for melodies
typedef struct {
    uint16_t frequency;
    uint16_t duration;
} note_t;

// Initialize audio system
void audio_init(void);

// Play a tone at given frequency for duration (ms)
void audio_play_tone(uint16_t frequency, uint16_t duration_ms);

// Stop current tone
void audio_stop(void);

// Play a predefined sound effect
void audio_play_effect(sound_effect_t effect);

// Play a melody (array of notes, terminated by {0, 0})
void audio_play_melody(const note_t* melody);

// Set master volume (0-100) - affects duration/intensity simulation
void audio_set_volume(uint8_t volume);

// Get current volume
uint8_t audio_get_volume(void);

// Enable/disable audio
void audio_enable(bool enable);
bool audio_is_enabled(void);

// Simple beep
void audio_beep(void);

// Delay function (milliseconds)
void audio_delay_ms(uint32_t ms);

#endif // AUDIO_H
