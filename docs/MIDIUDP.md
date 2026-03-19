# MIDI over UDP (MUDP-v1)

A simple, stateless protocol for transmitting MIDI messages over UDP to the ESP32 chime controller.

## Overview

MUDP-v1 is designed for:
- **Raw MIDI semantics** - carries full MIDI messages, not high-level events
- **Minimal overhead** - simple 4-byte header + message data
- **Stateless operation** - UDP-friendly, no connection state required
- **Message batching** - multiple MIDI messages per packet
- **ESP32-optimized** - trivial to parse, no dynamic allocation needed

## Packet Format

### Header (4 bytes fixed)
```
byte 0: 0x4D        // 'M'  magic byte
byte 1: 0x55        // 'U'  magic byte  
byte 2: version     // 0x01 for MUDP-v1
byte 3: count       // number of MIDI messages in packet (1–255)
```

### Message Records (variable length)

Each message consists of:
```
byte 0: status      // full MIDI status byte (0x80–0xEF)
byte 1: data1       // first data byte
byte 2: data2       // second data byte (if applicable)
```

### Data Length Rules

| Status Nibble | Meaning            | Total Bytes |
|---------------|-------------------|-------------|
| 0x8n          | Note Off          | 3           |
| 0x9n          | Note On           | 3           |
| 0xAn          | Poly Pressure     | 3           |
| 0xBn          | Control Change    | 3           |
| 0xCn          | Program Change    | 2           |
| 0xDn          | Channel Pressure  | 2           |
| 0xEn          | Pitch Bend        | 3           |

**Important**: Always send full status bytes. No running status.

## Example Packets

### Single Note On (middle C, ch.1, vel 100)
```
4D 55 01 01   90 3C 64
└─┘ └─┘ └─┘ └─┘ └──────┘
 M   U  ver cnt  Note On
```

### Batch: Note On + Note Off
```
4D 55 01 02
  90 3C 64    // Note On
  80 3C 00    // Note Off
```

### Program Change + Note On
```
4D 55 01 02
  C0 05       // Program Change (2 bytes)
  90 40 7F    // Note On (3 bytes)
```

## Network Configuration

**Default UDP Port**: 21928

The ESP32 listens on this port when WiFi is connected. If WiFi is not available at startup, the UDP receiver will automatically start when WiFi connects.

## Testing

Use the provided Python test script:

```bash
python test_midiudp.py <esp32_ip> [port]
```

Example:
```bash
python test_midiudp.py 192.168.1.100
```

## Monitoring

Check receiver status via HTTP API:

```bash
curl http://<esp32_ip>/status
```

Returns JSON with:
```json
{
  "midiUdp": {
    "listening": true,
    "port": 21928,
    "packetsReceived": 142,
    "messagesReceived": 278,
    "packetsDropped": 0
  }
}
```

## Implementation Notes

### ESP32 Parsing

The protocol is designed for efficient linear parsing:

```cpp
uint8_t *p = packet + 4;
for (int i = 0; i < count; i++) {
    uint8_t status = *p++;
    uint8_t type = status & 0xF0;
    uint8_t d1 = *p++;
    uint8_t d2 = 0;
    
    if (type != 0xC0 && type != 0xD0) {
        d2 = *p++;
    }
    
    handle_midi(status, d1, d2);
}
```

### Error Handling

Packets are dropped (with counter increment) if:
- Packet size < 4 bytes
- Magic bytes don't match (not 'MU')
- Version is not 0x01
- Message count is 0
- Invalid status byte (not 0x80-0xEF)
- Truncated message (insufficient bytes)

### Network Resilience

- UDP receiver automatically starts when WiFi connects
- Automatically stops when WiFi disconnects
- Restarts automatically when WiFi reconnects
- No state is lost since protocol is stateless

## Supported MIDI Messages

Currently implemented on ESP32:
- **Note On** (0x90) - triggers chime strike with velocity scaling
- **Note Off** (0x80) - tracked but no damper yet
- **Control Change 123** (0xB0 + data1=123) - All Notes Off

Not yet implemented (but parsed correctly):
- Polyphonic Aftertouch
- Other Control Changes
- Program Change
- Channel Pressure
- Pitch Bend

## Future Extensions

The protocol reserves space for future timing extensions without breaking compatibility. Possible additions:
- Optional timestamp field in header
- Delta-time between messages
- Timing mode flag in header byte 2 (version field has room for mode bits)

All extensions will maintain backward compatibility with MUDP-v1 parsers.
