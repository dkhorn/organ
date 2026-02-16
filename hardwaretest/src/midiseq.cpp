// midiseq.cpp - Simple non-blocking MIDI sequencer
#include "midiseq.h"
#include "midinote.h"
#include <Arduino.h>

static const MidiEvent* sequence = nullptr;
static uint16_t num_events = 0;
static uint16_t current_event = 0;
static uint32_t ticks_per_quarter = 480;
static uint32_t microsec_per_tick = 0;
static uint32_t next_event_time = 0;
static bool playing = false;
static bool paused = false;
static int8_t transpose = 0;  // Transpose in semitones (half-steps)
static uint8_t velocity_scale = 127;  // Maximum velocity (127 = no scaling)

static uint16_t base_tempo_bpm = 120;
static float tempo_scale = 1.0f;
static float velocity_scale_factor = 1.0f;

// Calculate microseconds per tick based on tempo
static void calculate_timing(uint16_t tempo_bpm) {
  // MIDI timing: microseconds per quarter note = 60,000,000 / BPM
  uint32_t usec_per_quarter = 60000000UL / tempo_bpm;
  microsec_per_tick = usec_per_quarter / ticks_per_quarter;
}

void midiseq_begin() {
  sequence = nullptr;
  num_events = 0;
  current_event = 0;
  playing = false;
  paused = false;
}

void midiseq_load(const MidiEvent* events, uint16_t count, 
                  uint16_t tpq, uint16_t tempo_bpm, int8_t transpose_semitones, uint8_t max_velocity) {
  // Stop current playback
  midiseq_stop();
  
  // Load new sequence
  sequence = events;
  num_events = count;
  current_event = 0;
  ticks_per_quarter = tpq;
  transpose = transpose_semitones;
  velocity_scale = max_velocity;
  
  calculate_timing(tempo_bpm);
}

void midiseq_play() {
  if (!sequence || num_events == 0) return;
  
  current_event = 0;
  next_event_time = micros();
  playing = true;
  paused = false;
  
  // Serial.printf("Starting MIDI playback: %d events, %d us/tick\n", 
  //               num_events, microsec_per_tick);
}

void midiseq_stop() {
  playing = false;
  paused = false;
  current_event = 0;
  
  // Send all notes off
  all_off();
}

void midiseq_pause() {
  paused = true;
}

void midiseq_resume() {
  if (playing && paused) {
    paused = false;
    // Reset timing to avoid jump
    next_event_time = micros();
  }
}

bool midiseq_is_playing() {
  return playing && !paused;
}

void midiseq_set_tempo(uint16_t tempo_bpm) {
  base_tempo_bpm = tempo_bpm;
  calculate_timing((uint16_t)(tempo_bpm * tempo_scale));
  // Serial.printf("Tempo changed to %d BPM (%d us/tick)\n", 
  //               tempo_bpm, microsec_per_tick);
}

void midiseq_set_tempo_scale(float scale) {
  if (scale < 0.1f) scale = 0.1f;
  if (scale > 4.0f) scale = 4.0f;
  tempo_scale = scale;
  calculate_timing((uint16_t)(base_tempo_bpm * tempo_scale));
}

void midiseq_set_velocity_scale(float scale) {
  if (scale < 0.0f) scale = 0.0f;
  if (scale > 2.0f) scale = 2.0f;
  velocity_scale_factor = scale;
}

void midiseq_set_transpose(int8_t semitones) {
  if (semitones < -12) semitones = -12;
  if (semitones > 12) semitones = 12;
  transpose = semitones;
}

void midiseq_loop() {
  if (!playing || paused || !sequence) return;
  
  uint32_t now = micros();
  
  // Process all events that are due
  while (current_event < num_events && (int32_t)(now - next_event_time) >= 0) {
    const MidiEvent* evt = &sequence[current_event];
    
    // Handle MIDI event
    uint8_t status = evt->status & 0xF0;  // Upper nibble is message type
    uint8_t channel = evt->status & 0x0F; // Lower nibble is channel (unused for now)
    
    switch (status) {
      case 0x90: // Note On
        if (evt->data2 > 0) {  // Velocity > 0 means note on
          int16_t transposed_note = (int16_t)evt->data1 + transpose;
          if (transposed_note >= 0 && transposed_note <= 127) {
            // Scale velocity: first by max_velocity, then by velocity_scale_factor
            uint16_t vel = (velocity_scale == 127) ? evt->data2 : 
                          ((uint16_t)evt->data2 * velocity_scale) / 127;
            vel = (uint16_t)(vel * velocity_scale_factor);
            if (vel > 127) vel = 127;
            note_on(transposed_note, (uint8_t)vel);
          }
          // Serial.printf("Note ON: %d vel=%d\n", evt->data1, evt->data2);
        } else {  // Velocity = 0 is actually note off
          int16_t transposed_note = (int16_t)evt->data1 + transpose;
          if (transposed_note >= 0 && transposed_note <= 127) {
            note_off(transposed_note, evt->data2);
          }
          // Serial.printf("Note OFF: %d\n", evt->data1);
        }
        break;
        
      case 0x80: // Note Off
        {
          int16_t transposed_note = (int16_t)evt->data1 + transpose;
          if (transposed_note >= 0 && transposed_note <= 127) {
            note_off(transposed_note, evt->data2);
          }
        }
        // Serial.printf("Note OFF: %d\n", evt->data1);
        break;
        
      // Other MIDI messages could be handled here:
      // 0xA0 = Polyphonic aftertouch
      // 0xB0 = Control change
      // 0xC0 = Program change
      // 0xD0 = Channel aftertouch
      // 0xE0 = Pitch bend
      
      default:
        // Ignore unknown messages
        break;
    }
    
    // Advance to next event
    current_event++;
    
    // Calculate next event time
    if (current_event < num_events) {
      uint32_t delta_usec = sequence[current_event].delta_ticks * microsec_per_tick;
      next_event_time += delta_usec;
    }
  }
  
  // Check if sequence finished
  if (current_event >= num_events) {
    // Serial.println("MIDI sequence finished");
    midiseq_stop();
  }
}

// Simple MIDI file parser - loads track 0 only
bool midiseq_load_from_buffer(const uint8_t* data, size_t size) {
  if (!data || size < 14) return false;
  
  // Check MThd header
  if (data[0] != 'M' || data[1] != 'T' || data[2] != 'h' || data[3] != 'd') {
    return false;
  }
  
  // Parse header
  uint16_t format = (data[8] << 8) | data[9];
  uint16_t num_tracks = (data[10] << 8) | data[11];
  uint16_t division = (data[12] << 8) | data[13];
  
  // We only support format 0 (single track) and format 1 (multiple tracks, use first)
  if (format > 1) return false;
  
  // Division gives us ticks per quarter note
  if (division & 0x8000) {
    // SMPTE time division not supported
    return false;
  }
  uint16_t tpq = division & 0x7FFF;
  
  // Find first MTrk
  size_t pos = 14;
  while (pos + 8 < size) {
    if (data[pos] == 'M' && data[pos+1] == 'T' && 
        data[pos+2] == 'r' && data[pos+3] == 'k') {
      // Found track
      uint32_t track_length = (data[pos+4] << 24) | (data[pos+5] << 16) |
                              (data[pos+6] << 8) | data[pos+7];
      pos += 8;
      
      if (pos + track_length > size) return false;
      
      // Parse track events
      const uint8_t* track_data = data + pos;
      size_t track_pos = 0;
      uint8_t running_status = 0;
      
      // Count events first (max estimate)
      static MidiEvent temp_events[1024];
      uint16_t event_count = 0;
      
      while (track_pos < track_length && event_count < 1024) {
        // Read variable-length delta time
        uint32_t delta = 0;
        uint8_t byte;
        do {
          if (track_pos >= track_length) break;
          byte = track_data[track_pos++];
          delta = (delta << 7) | (byte & 0x7F);
        } while (byte & 0x80);
        
        // Read status byte
        if (track_pos >= track_length) break;
        uint8_t status = track_data[track_pos];
        
        if (status & 0x80) {
          // New status byte
          running_status = status;
          track_pos++;
        } else {
          // Running status
          status = running_status;
        }
        
        // Skip non-MIDI events
        if (status == 0xFF) {
          // Meta event
          if (track_pos >= track_length) break;
          uint8_t meta_type = track_data[track_pos++];
          
          // Read length
          uint32_t length = 0;
          do {
            if (track_pos >= track_length) break;
            byte = track_data[track_pos++];
            length = (length << 7) | (byte & 0x7F);
          } while (byte & 0x80);
          
          track_pos += length;
          continue;
        } else if (status == 0xF0 || status == 0xF7) {
          // SysEx - skip
          uint32_t length = 0;
          do {
            if (track_pos >= track_length) break;
            byte = track_data[track_pos++];
            length = (length << 7) | (byte & 0x7F);
          } while (byte & 0x80);
          track_pos += length;
          continue;
        }
        
        // Parse MIDI event
        uint8_t type = status & 0xF0;
        
        // Read data bytes
        uint8_t data1 = 0, data2 = 0;
        if (type != 0xC0 && type != 0xD0) {
          // 2-byte message
          if (track_pos >= track_length) break;
          data1 = track_data[track_pos++];
          if (track_pos >= track_length) break;
          data2 = track_data[track_pos++];
        } else {
          // 1-byte message
          if (track_pos >= track_length) break;
          data1 = track_data[track_pos++];
        }
        
        // Store event (only Note On/Off for now)
        if (type == 0x90 || type == 0x80) {
          temp_events[event_count].delta_ticks = delta;
          temp_events[event_count].status = status;
          temp_events[event_count].data1 = data1;
          temp_events[event_count].data2 = data2;
          event_count++;
        }
      }
      
      // Allocate permanent storage
      if (event_count > 0) {
        MidiEvent* events = (MidiEvent*)malloc(event_count * sizeof(MidiEvent));
        if (events) {
          memcpy(events, temp_events, event_count * sizeof(MidiEvent));
          midiseq_load(events, event_count, tpq, 120, 0, 127);
          midiseq_play();
          return true;
        }
      }
      
      return false;
    }
    pos++;
  }
  
  return false;
}
