#include "clockchimes.h"

// MIDI note definitions for readability
#define A4  69
#define As4 70
#define B4  71
#define C5  72
#define Cs5 73
#define D5  74
#define Ds5 75
#define E5  76
#define F5  77
#define Fs5 78
#define G5  79
#define Gs5 80
#define A5  81
#define As5 82
#define B5  83
#define C6  84
#define Cs6 85
#define D6  86
#define Ds6 87
#define E6  88

// Note durations in MIDI ticks (480 ticks per quarter note)
#define WHOLE     1920
#define HALF      960
#define QUARTER   480
#define EIGHTH    240
#define SIXTEENTH 120

#define N1 72
#define N2 74
#define N3 76
#define N4 77
#define N5 79
#define N6 81
#define N7 83
#define N8 84

namespace ClockTunes {

// ============================================================================
// TUNE 1: WESTMINSTER QUARTERS (Big Ben)
// Classic melody: E-C-D-G (quarter), G-D-E-C (half), etc.
// ============================================================================

// Quarter hour: 1st phrase only
const MidiEvent westminster_quarter[] = {
    {0,      0x90, N6, 127},  // E -> D#6
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N4, 127},   // C -> C6
    {QUARTER, 0x80, N4, 0},
    {0,      0x90, N5, 127},   // D -> D6
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N1, 127},   // G -> G5
    {HALF,    0x80, N1, 0},
};
const uint16_t westminster_quarter_len = sizeof(westminster_quarter) / sizeof(MidiEvent);

// Half hour: 2 phrases
const MidiEvent westminster_half[] = {
    // First phrase: E-C-D-G
    {0,      0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N4, 127},
    {QUARTER, 0x80, N4, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N1, 127},
    {HALF,    0x80, N1, 0},
    
    // Second phrase: G-D-E-C
    {QUARTER, 0x90, N1, 127},
    {QUARTER, 0x80, N1, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N4, 127},
    {HALF,    0x80, N4, 0},
};
const uint16_t westminster_half_len = sizeof(westminster_half) / sizeof(MidiEvent);

// Three-quarter hour: 3 phrases
const MidiEvent westminster_three_quarter[] = {
    // First phrase: E-C-D-G
    {0,      0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N4, 127},
    {QUARTER, 0x80, N4, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N1, 127},
    {HALF,    0x80, N1, 0},
    
    // Second phrase: G-D-E-C
    {QUARTER, 0x90, N1, 127},
    {QUARTER, 0x80, N1, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N4, 127},
    {HALF,    0x80, N4, 0},
    
    // Third phrase: E-D-C-G
    {QUARTER, 0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N4, 127},
    {QUARTER, 0x80, N4, 0},
    {0,      0x90, N1, 127},
    {HALF,    0x80, N1, 0},
};
const uint16_t westminster_three_quarter_len = sizeof(westminster_three_quarter) / sizeof(MidiEvent);

// Hour: Complete 4 phrases
const MidiEvent westminster_hour[] = {
    // First phrase: E-C-D-G
    {0,      0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N4, 127},
    {QUARTER, 0x80, N4, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N1, 127},
    {HALF,    0x80, N1, 0},
    
    // Second phrase: G-D-E-C
    {QUARTER, 0x90, N1, 127},
    {QUARTER, 0x80, N1, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N4, 127},
    {HALF,    0x80, N4, 0},
    
    // Third phrase: E-D-C-G
    {QUARTER, 0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N4, 127},
    {QUARTER, 0x80, N4, 0},
    {0,      0x90, N1, 127},
    {HALF,    0x80, N1, 0},
    
    // Fourth phrase: G-E-C-D (resolving)
    {QUARTER, 0x90, N1, 127},
    {QUARTER, 0x80, N1, 0},
    {0,      0x90, N5, 127},
    {QUARTER, 0x80, N5, 0},
    {0,      0x90, N6, 127},
    {QUARTER, 0x80, N6, 0},
    {0,      0x90, N4, 127},
    {HALF,    0x80, N4, 0},
};
const uint16_t westminster_hour_len = sizeof(westminster_hour) / sizeof(MidiEvent);

// ============================================================================
// TUNE 2: WHITTINGTON CHIMES
// More complex pattern with different melodic structure
// ============================================================================

const MidiEvent whittington_quarter[] = {
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
};
const uint16_t whittington_quarter_len = sizeof(whittington_quarter) / sizeof(MidiEvent);

const MidiEvent whittington_half[] = {
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
};
const uint16_t whittington_half_len = sizeof(whittington_half) / sizeof(MidiEvent);

const MidiEvent whittington_three_quarter[] = {
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, G5, 127},
    {HALF,    0x80, G5, 0},
};
const uint16_t whittington_three_quarter_len = sizeof(whittington_three_quarter) / sizeof(MidiEvent);

const MidiEvent whittington_hour[] = {
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, G5, 127},
    {HALF,    0x80, G5, 0},
    
    {QUARTER, 0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, Ds6, 127},
    {HALF,    0x80, Ds6, 0},
};
const uint16_t whittington_hour_len = sizeof(whittington_hour) / sizeof(MidiEvent);

// ============================================================================
// TUNE 3: ST. MICHAEL'S CHIMES
// Simpler, more contemplative pattern
// ============================================================================

const MidiEvent stmichael_quarter[] = {
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
};
const uint16_t stmichael_quarter_len = sizeof(stmichael_quarter) / sizeof(MidiEvent);

const MidiEvent stmichael_half[] = {
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {HALF,    0x80, G5, 0},
};
const uint16_t stmichael_half_len = sizeof(stmichael_half) / sizeof(MidiEvent);

const MidiEvent stmichael_three_quarter[] = {
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {HALF,    0x80, G5, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, G5, 127},
    {HALF,    0x80, G5, 0},
};
const uint16_t stmichael_three_quarter_len = sizeof(stmichael_three_quarter) / sizeof(MidiEvent);

const MidiEvent stmichael_hour[] = {
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {HALF,    0x80, G5, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, G5, 127},
    {HALF,    0x80, G5, 0},
    
    {QUARTER, 0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
};
const uint16_t stmichael_hour_len = sizeof(stmichael_hour) / sizeof(MidiEvent);

// ============================================================================
// TUNE 4: WINCHESTER CHIMES
// Ascending pattern with brighter character
// ============================================================================

const MidiEvent winchester_quarter[] = {
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
};
const uint16_t winchester_quarter_len = sizeof(winchester_quarter) / sizeof(MidiEvent);

const MidiEvent winchester_half[] = {
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, A5, 127},
    {QUARTER, 0x80, A5, 0},
    {0,      0x90, D6, 127},
    {HALF,    0x80, D6, 0},
};
const uint16_t winchester_half_len = sizeof(winchester_half) / sizeof(MidiEvent);

const MidiEvent winchester_three_quarter[] = {
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, A5, 127},
    {QUARTER, 0x80, A5, 0},
    {0,      0x90, D6, 127},
    {HALF,    0x80, D6, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, B5, 127},
    {QUARTER, 0x80, B5, 0},
    {0,      0x90, Ds6, 127},
    {HALF,    0x80, Ds6, 0},
};
const uint16_t winchester_three_quarter_len = sizeof(winchester_three_quarter) / sizeof(MidiEvent);

const MidiEvent winchester_hour[] = {
    {0,      0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
    
    {QUARTER, 0x90, D6, 127},
    {QUARTER, 0x80, D6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, A5, 127},
    {QUARTER, 0x80, A5, 0},
    {0,      0x90, D6, 127},
    {HALF,    0x80, D6, 0},
    
    {QUARTER, 0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, B5, 127},
    {QUARTER, 0x80, B5, 0},
    {0,      0x90, Ds6, 127},
    {HALF,    0x80, Ds6, 0},
    
    {QUARTER, 0x90, C6, 127},
    {QUARTER, 0x80, C6, 0},
    {0,      0x90, Ds6, 127},
    {QUARTER, 0x80, Ds6, 0},
    {0,      0x90, G5, 127},
    {QUARTER, 0x80, G5, 0},
    {0,      0x90, C6, 127},
    {HALF,    0x80, C6, 0},
};
const uint16_t winchester_hour_len = sizeof(winchester_hour) / sizeof(MidiEvent);

// ============================================================================
// TUNE 5: TEST TUNE (Westminster rhythm, single note E6)
// Same timing as Westminster but uses only E6 (MIDI 88)
// ============================================================================

#define E6  88

// Quarter hour: 1st phrase only
const MidiEvent testtune_quarter[] = {
    {0,      0x90, E6, 127},
    {HALF,    0x80, E6, 0},
};
const uint16_t testtune_quarter_len = sizeof(testtune_quarter) / sizeof(MidiEvent);

// Half hour: 2 phrases
const MidiEvent testtune_half[] = {
    // First phrase
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, E6, 127},
    {HALF,    0x80, E6, 0},
};
const uint16_t testtune_half_len = sizeof(testtune_half) / sizeof(MidiEvent);

// Three-quarter hour: 3 phrases
const MidiEvent testtune_three_quarter[] = {
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, E6, 127},
    {HALF,    0x80, E6, 0},
};
const uint16_t testtune_three_quarter_len = sizeof(testtune_three_quarter) / sizeof(MidiEvent);

// Hour: Complete 4 phrases
const MidiEvent testtune_hour[] = {
    // First phrase
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, E6, 127},
    {QUARTER, 0x80, E6, 0},
    {0,      0x90, E6, 127},
    {HALF,    0x80, E6, 0},
};
const uint16_t testtune_hour_len = sizeof(testtune_hour) / sizeof(MidiEvent);

} // namespace ClockTunes

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

namespace ClockTunes {

// Tune names
static const char* TUNE_NAMES[] = {
    "Disabled",
    "Westminster",
    "Whittington",
    "St. Michael's",
    "Winchester",
    "Test Tune"
};

const char* getTuneName(uint8_t tuneNumber) {
    if (tuneNumber < sizeof(TUNE_NAMES) / sizeof(TUNE_NAMES[0])) {
        return TUNE_NAMES[tuneNumber];
    }
    return "Unknown";
}

uint8_t getNumTunes() {
    return (sizeof(TUNE_NAMES) / sizeof(TUNE_NAMES[0])) - 1;  // Exclude "Disabled"
}

const MidiEvent* getSequence(uint8_t tuneNumber, uint8_t quarter, uint16_t* outLength) {
    if (!outLength || quarter < 1 || quarter > 4) {
        return nullptr;
    }
    
    const MidiEvent* sequence = nullptr;
    uint16_t length = 0;
    
    switch (tuneNumber) {
        case 1:  // Westminster
            switch (quarter) {
                case 1: sequence = westminster_quarter; 
                        length = westminster_quarter_len; break;
                case 2: sequence = westminster_half; 
                        length = westminster_half_len; break;
                case 3: sequence = westminster_three_quarter; 
                        length = westminster_three_quarter_len; break;
                case 4: sequence = westminster_hour; 
                        length = westminster_hour_len; break;
            }
            break;
            
        case 2:  // Whittington
            switch (quarter) {
                case 1: sequence = whittington_quarter; 
                        length = whittington_quarter_len; break;
                case 2: sequence = whittington_half; 
                        length = whittington_half_len; break;
                case 3: sequence = whittington_three_quarter; 
                        length = whittington_three_quarter_len; break;
                case 4: sequence = whittington_hour; 
                        length = whittington_hour_len; break;
            }
            break;
            
        case 3:  // St. Michael's
            switch (quarter) {
                case 1: sequence = stmichael_quarter; 
                        length = stmichael_quarter_len; break;
                case 2: sequence = stmichael_half; 
                        length = stmichael_half_len; break;
                case 3: sequence = stmichael_three_quarter; 
                        length = stmichael_three_quarter_len; break;
                case 4: sequence = stmichael_hour; 
                        length = stmichael_hour_len; break;
            }
            break;
            
        case 4:  // Winchester
            switch (quarter) {
                case 1: sequence = winchester_quarter; 
                        length = winchester_quarter_len; break;
                case 2: sequence = winchester_half; 
                        length = winchester_half_len; break;
                case 3: sequence = winchester_three_quarter; 
                        length = winchester_three_quarter_len; break;
                case 4: sequence = winchester_hour; 
                        length = winchester_hour_len; break;
            }
            break;
            
        case 5:  // Test Tune
            switch (quarter) {
                case 1: sequence = testtune_quarter; 
                        length = testtune_quarter_len; break;
                case 2: sequence = testtune_half; 
                        length = testtune_half_len; break;
                case 3: sequence = testtune_three_quarter; 
                        length = testtune_three_quarter_len; break;
                case 4: sequence = testtune_hour; 
                        length = testtune_hour_len; break;
            }
            break;
            
        default:
            return nullptr;
    }
    
    *outLength = length;
    return sequence;
}

} // namespace ClockTunes
