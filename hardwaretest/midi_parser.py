#!/usr/bin/env python3
"""
Parse raw MIDI hex data and convert to MidiEvent format for ESP32 chimes
"""

import sys

def parse_variable_length(data, pos):
    """Parse MIDI variable length quantity"""
    value = 0
    while True:
        byte = data[pos]
        pos += 1
        value = (value << 7) | (byte & 0x7F)
        if not (byte & 0x80):
            break
    return value, pos

def parse_midi_hex(hex_string):
    """Parse MIDI hex string and extract note events"""
    # Remove whitespace and convert to bytes
    hex_clean = hex_string.replace(' ', '').replace('\n', '')
    data = bytes.fromhex(hex_clean)
    
    print(f"Total MIDI data: {len(data)} bytes\n")
    
    # Parse header
    if data[0:4] != b'MThd':
        print("ERROR: Not a valid MIDI file")
        return
    
    header_length = int.from_bytes(data[4:8], 'big')
    midi_format = int.from_bytes(data[8:10], 'big')
    num_tracks = int.from_bytes(data[10:12], 'big')
    ticks_per_quarter = int.from_bytes(data[12:14], 'big')
    
    print(f"MIDI Format: {midi_format}")
    print(f"Tracks: {num_tracks}")
    print(f"Ticks per quarter: {ticks_per_quarter}\n")
    
    # Find track data
    pos = 8 + header_length
    
    events = []
    running_status = 0
    
    for track_num in range(num_tracks):
        if data[pos:pos+4] != b'MTrk':
            print(f"ERROR: Invalid track header at {pos}")
            break
            
        track_length = int.from_bytes(data[pos+4:pos+8], 'big')
        print(f"Track {track_num}: {track_length} bytes")
        
        pos += 8
        track_end = pos + track_length
        
        while pos < track_end:
            # Parse delta time
            delta, pos = parse_variable_length(data, pos)
            
            # Parse event
            status = data[pos]
            
            # Running status
            if status < 0x80:
                status = running_status
            else:
                pos += 1
                running_status = status
            
            event_type = status & 0xF0
            
            if event_type == 0x90:  # Note On
                note = data[pos]
                velocity = data[pos + 1]
                pos += 2
                if velocity > 0:
                    events.append((delta, 0x90, note, velocity))
                    print(f"  Note ON:  delta={delta:5d} note={note:3d} vel={velocity:3d}")
                else:
                    events.append((delta, 0x80, note, 64))
                    print(f"  Note OFF: delta={delta:5d} note={note:3d}")
                    
            elif event_type == 0x80:  # Note Off
                note = data[pos]
                velocity = data[pos + 1]
                pos += 2
                events.append((delta, 0x80, note, velocity))
                print(f"  Note OFF: delta={delta:5d} note={note:3d}")
                
            elif event_type in [0xA0, 0xB0, 0xE0]:  # 2-byte events
                pos += 2
                
            elif event_type in [0xC0, 0xD0]:  # 1-byte events
                pos += 1
                
            elif status == 0xFF:  # Meta event
                meta_type = data[pos]
                pos += 1
                length, pos = parse_variable_length(data, pos)
                pos += length
                
            elif status == 0xF0 or status == 0xF7:  # SysEx
                length, pos = parse_variable_length(data, pos)
                pos += length
    
    print(f"\nTotal note events: {len(events)}\n")
    
    # Generate C code
    print("// Generated MidiEvent array:")
    print(f"static const MidiEvent SONG_IMPORTED[] = {{")
    
    for delta, status, note, vel in events:
        status_str = "0x90" if status == 0x90 else "0x80"
        print(f"  {{{delta:5d}, {status_str}, {note:3d}, {vel:3d}}},")
    
    print("};")
    print(f"\n// Song info:")
    print(f"// Events: {len(events)}")
    print(f"// Ticks per quarter: {ticks_per_quarter}")
    print(f"// MIDI note range: {min(e[2] for e in events)} - {max(e[2] for e in events)}")
    print(f"// Your chime range: 69-88 (A to E, 20 notes)")

if __name__ == "__main__":
    hex_data = """
    4d546864000000060000000103c04d54726b0000023f8736bf79008f0a9f3879815c8f3879149f3a77815c8f3a77149f3d79815c8f3d79149f3a7b815c8f3a7b149f417985148f41795abf00000abf20000acf41813e9f417883388f4178289f3f78822cbf076400bf0a4000bf0b7f0abf5b7f00bf5d148718bf65000abf64000abf060c0abf657f0abf647f5a8f3f78509f387c815c8f387c149f3a78815c8f3a78149f3d78815c8f3d78149f3a7c815c8f3a7c149f3f7b85148f3f7b822c9f3f7783388f3f77289f3d7985148f3d793c9f3c78815c8f3c78149f3a7883388f3a78289f3879815c8f3879149f3a7b815c8f3a7b149f3d7b815c8f3d7b149f3a7b815c8f3a7b149f3d7786708f3d77509f3f7783388f3f77289f3c7985148f3c793c9f3a77815c8f3a77149f387983388f3879289f387983388f3879289f3f7b8a508f3f7b509f3d788e308f3d78509f3879815c8f3879149f3a7c815c8f3a7c149f3d7c815c8f3d7c149f3a7a815c8f3a7a149f417985148f4179822c9f417c83388f417c289f3f788a508f3f78509f3879815c8f3879149f3a7b815c8f3a7b149f3d7c815c8f3d7c149f3a79815c8f3a79149f447886708f4478509f3c7b83388f3c7b289f3d7b85148f3d7b3c9f3c7a815c8f3c7a149f3a7983388f3a79289f387a815c8f387a149f3a7b815c8f3a7b149f3d77815c8f3d77149f3a78815c8f3a78149f3d7986708f3d79509f3f7783388f3f77289f3c7885148f3c783c9f3a7b815c8f3a7b149f387785148f3877822c9f387883388f3878289f3f7b86708f3f7b509f3d7b90208f3d7b8620b07b0000ff2f00
    """
    
    parse_midi_hex(hex_data)
