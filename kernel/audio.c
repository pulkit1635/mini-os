#include "audio.h"
#include "io.h"

// PIT (Programmable Interval Timer) ports
#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43

// PC Speaker port
#define SPEAKER_PORT    0x61

// PIT frequency
#define PIT_FREQUENCY   1193180

// Audio state
static bool audio_enabled = true;
static uint8_t master_volume = 80;

// Timer tick counter for delays
static volatile uint32_t tick_count = 0;

void audio_init(void) {
    audio_enabled = true;
    master_volume = 80;
    audio_stop();
}

// Simple delay using busy wait (approximate)
void audio_delay_ms(uint32_t ms) {
    // Rough approximation - actual timing depends on CPU speed
    // This uses a busy loop calibrated for ~1ms per iteration on typical systems
    for (uint32_t i = 0; i < ms; i++) {
        for (volatile uint32_t j = 0; j < 10000; j++) {
            __asm__ volatile("nop");
        }
    }
}

void audio_play_tone(uint16_t frequency, uint16_t duration_ms) {
    if (!audio_enabled || frequency == 0) return;
    
    // Calculate PIT divisor
    uint32_t divisor = PIT_FREQUENCY / frequency;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;
    
    // Set PIT channel 2 to square wave mode
    outb(PIT_COMMAND, 0xB6);  // Channel 2, lobyte/hibyte, square wave
    
    // Set frequency divisor
    outb(PIT_CHANNEL2, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL2, (uint8_t)((divisor >> 8) & 0xFF));
    
    // Enable speaker
    uint8_t speaker_state = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, speaker_state | 0x03);
    
    // Adjust duration based on volume (lower volume = shorter duration for perceived quieter sound)
    uint16_t adjusted_duration = (duration_ms * master_volume) / 100;
    if (adjusted_duration < 10) adjusted_duration = 10;
    
    // Wait for duration
    audio_delay_ms(adjusted_duration);
    
    // Stop tone
    audio_stop();
}

void audio_stop(void) {
    // Disable speaker
    uint8_t speaker_state = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, speaker_state & 0xFC);
}

void audio_beep(void) {
    audio_play_tone(1000, 100);
}

void audio_play_effect(sound_effect_t effect) {
    if (!audio_enabled) return;
    
    switch (effect) {
        case SFX_BEEP:
            audio_play_tone(800, 100);
            break;
            
        case SFX_CLICK:
            audio_play_tone(1500, 20);
            break;
            
        case SFX_ERROR:
            audio_play_tone(200, 150);
            audio_delay_ms(50);
            audio_play_tone(150, 200);
            break;
            
        case SFX_SUCCESS:
            audio_play_tone(523, 100);  // C5
            audio_delay_ms(30);
            audio_play_tone(659, 100);  // E5
            audio_delay_ms(30);
            audio_play_tone(784, 150);  // G5
            break;
            
        case SFX_STARTUP: {
            // Windows-like startup sound
            audio_play_tone(523, 150);  // C5
            audio_delay_ms(30);
            audio_play_tone(659, 150);  // E5
            audio_delay_ms(30);
            audio_play_tone(784, 150);  // G5
            audio_delay_ms(30);
            audio_play_tone(1047, 300); // C6
            break;
        }
            
        case SFX_SHUTDOWN:
            audio_play_tone(784, 150);  // G5
            audio_delay_ms(50);
            audio_play_tone(523, 150);  // C5
            audio_delay_ms(50);
            audio_play_tone(392, 300);  // G4
            break;
            
        case SFX_NOTIFICATION:
            audio_play_tone(880, 100);  // A5
            audio_delay_ms(50);
            audio_play_tone(1109, 150); // C#6
            break;
            
        case SFX_KEYPRESS:
            audio_play_tone(2000, 10);
            break;
    }
}

void audio_play_melody(const note_t* melody) {
    if (!audio_enabled || !melody) return;
    
    for (int i = 0; melody[i].frequency != 0 || melody[i].duration != 0; i++) {
        if (melody[i].frequency == 0) {
            // Rest
            audio_delay_ms(melody[i].duration);
        } else {
            audio_play_tone(melody[i].frequency, melody[i].duration);
        }
        // Small gap between notes
        audio_delay_ms(20);
    }
}

void audio_set_volume(uint8_t volume) {
    if (volume > 100) volume = 100;
    master_volume = volume;
}

uint8_t audio_get_volume(void) {
    return master_volume;
}

void audio_enable(bool enable) {
    audio_enabled = enable;
    if (!enable) {
        audio_stop();
    }
}

bool audio_is_enabled(void) {
    return audio_enabled;
}

// Predefined melodies
const note_t MELODY_SCALE[] = {
    {NOTE_C4, DUR_QUARTER},
    {NOTE_D4, DUR_QUARTER},
    {NOTE_E4, DUR_QUARTER},
    {NOTE_F4, DUR_QUARTER},
    {NOTE_G4, DUR_QUARTER},
    {NOTE_A4, DUR_QUARTER},
    {NOTE_B4, DUR_QUARTER},
    {NOTE_C5, DUR_HALF},
    {0, 0}  // End marker
};

const note_t MELODY_TWINKLE[] = {
    {NOTE_C4, DUR_QUARTER}, {NOTE_C4, DUR_QUARTER},
    {NOTE_G4, DUR_QUARTER}, {NOTE_G4, DUR_QUARTER},
    {NOTE_A4, DUR_QUARTER}, {NOTE_A4, DUR_QUARTER},
    {NOTE_G4, DUR_HALF},
    {NOTE_F4, DUR_QUARTER}, {NOTE_F4, DUR_QUARTER},
    {NOTE_E4, DUR_QUARTER}, {NOTE_E4, DUR_QUARTER},
    {NOTE_D4, DUR_QUARTER}, {NOTE_D4, DUR_QUARTER},
    {NOTE_C4, DUR_HALF},
    {0, 0}
};

const note_t MELODY_BEEP_BOOP[] = {
    {800, 100}, {0, 50}, {1200, 100}, {0, 50},
    {800, 100}, {0, 50}, {1200, 100},
    {0, 0}
};
