# CAN Bus Protocol

Specification for CAN bus communication with organ console.

## Overview

The organ console communicates over CAN bus using a simple protocol that mirrors MIDI message semantics. Each CAN message carries the equivalent of a MIDI note_on/note_off or control change event.

## Message Format

CAN 2.0A Standard Frame (11-bit identifier):
- **CAN ID**: 11-bit identifier encoding message type and channel
- **DLC**: Data Length Code (always 3 bytes for our protocol)
- **Data**: 3 bytes containing event information

### CAN ID Structure (11 bits)

```
Bits 10-8: Message Type (3 bits)
  000 = Note Off
  001 = Note On
  010-111 = Reserved

Bits 7-0: Channel (8 bits)
  0 = Great manual
  1 = Swell manual
  2 = Choir manual
  3 = Pedal
  4 = Stops
  5 = Couplers
  6-255 = Reserved for future use
```

**CAN ID Calculation**: `CAN_ID = (msg_type << 8) | channel`

### Data Bytes

For all Note On/Off messages (keys, stops, couplers, pistons):
- **Byte 0**: Note number (0-127, MIDI compatible)
- **Byte 1**: Velocity (0-127, MIDI compatible)
- **Byte 2**: Reserved (0x00)

## Message Types

### Key Events

**Note On** (CAN ID = 0x100 + channel):
```
CAN ID: 0x100 (Great), 0x101 (Swell), 0x102 (Choir), 0x103 (Pedal)
Data[0]: MIDI note number (36-96 for manuals, 24-59 for pedal)
Data[1]: Velocity (1-127)
Data[2]: 0x00
```

**Note Off** (CAN ID = 0x000 + channel):
```
CAN ID: 0x000 (Great), 0x001 (Swell), 0x002 (Choir), 0x003 (Pedal)
Data[0]: MIDI note number
Data[1]: Release velocity (typically 64)
Data[2]: 0x00
```

### Stop Messages

**Stop Draw/Cancel** (CAN ID = Note On/Off on Stop Channel):
```
Note On (Draw):
  CAN ID: 0x104 (Note On, Channel 4 = Stops)
  Data[0]: Stop note number (as mapped in input_map.yaml)
  Data[1]: Velocity (1-127, typically 127)
  Data[2]: 0x00

Note Off (Cancel):
  CAN ID: 0x004 (Note Off, Channel 4 = Stops)
  Data[0]: Stop note number
  Data[1]: Release velocity (typically 64)
  Data[2]: 0x00
```

Note: Stops use the same Note On/Off mechanism as keys, just on a dedicated channel (4).
The stop note numbers map to specific stops as defined in input_map.yaml.

### Piston Messages

**Piston Press** (CAN ID = Note On on division channel):
```
Note On:
  CAN ID: 0x100 (Great), 0x101 (Swell), 0x102 (Choir), 0x103 (Pedal)
  Data[0]: Piston note number (from piston range in input_map.yaml)
  Data[1]: Velocity (typically 127)
  Data[2]: 0x00
```

Note: Pistons use Note On messages in the piston note ranges defined for each division.
For example, Great pistons below the manual use notes 24-35 (C1-B1) on channel 0.

## Examples

### Playing Middle C on Great Manual

```
Note On:
  CAN ID: 0x100 (Note On, Channel 0 = Great)
  Data: [60, 80, 0]  (Note 60 = Middle C, Velocity 80)

Note Off:
  CAN ID: 0x000 (Note Off, Channel 0 = Great)
  Data: [60, 64, 0]  (Note 60, Release velocity 64)
```

### Activating Great Principal 8' (Stop Note 36)

```
Draw (Note On):
  CAN ID: 0x104 (Note On, Channel 4 = Stops)
  Data: [36, 127, 0]  (Note 36, Velocity 127)

Cancel (Note Off):
  CAN ID: 0x004 (Note Off, Channel 4 = Stops)
  Data: [36, 64, 0]   (Note 36, Release velocity 64)
```

### Pressing Great Piston 3 (Note 26 = C1 + 2)

```
CAN ID: 0x100 (Note On, Channel 0 = Great)
Data: [26, 127, 0]  (Note 26 in piston range, Velocity 127)
```

## Error Handling

- Invalid CAN IDs are logged and ignored
- Out-of-range note numbers are clamped or rejected
- Velocity of 0 in Note On is treated as Note Off
- Malformed messages (wrong DLC) are logged and discarded

## Hardware Requirements

- USB CAN interface (e.g., PEAK USB-to-CAN, CANable)
- SocketCAN driver support in Linux
- Console must send standard CAN 2.0A frames at 250 kbit/s or 500 kbit/s
