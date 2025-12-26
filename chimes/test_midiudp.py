#!/usr/bin/env python3
"""
Test MIDI over UDP (MUDP-v1) implementation.
Sends various MIDI messages to the ESP32 chime controller.

Usage:
    python test_midiudp.py <esp32_ip> [port]

Default port: 21928
"""

import socket
import time
import sys

# MUDP-v1 protocol constants
MAGIC_M = 0x4D  # 'M'
MAGIC_U = 0x55  # 'U'
VERSION = 0x01

# MIDI message types
NOTE_OFF = 0x80
NOTE_ON = 0x90
POLY_AFTERTOUCH = 0xA0
CONTROL_CHANGE = 0xB0
PROGRAM_CHANGE = 0xC0
CHANNEL_PRESSURE = 0xD0
PITCH_BEND = 0xE0

def create_mudp_packet(messages):
    """
    Create a MUDP-v1 packet from a list of MIDI messages.
    
    Args:
        messages: List of tuples (status, data1, data2)
                  For 2-byte messages (Program Change, Channel Pressure), data2 is ignored
    
    Returns:
        bytes: Complete MUDP packet
    """
    if not messages or len(messages) > 255:
        raise ValueError("Message count must be 1-255")
    
    packet = bytearray()
    
    # Header: [MAGIC_M, MAGIC_U, VERSION, count]
    packet.append(MAGIC_M)
    packet.append(MAGIC_U)
    packet.append(VERSION)
    packet.append(len(messages))
    
    # Add messages
    for status, data1, data2 in messages:
        packet.append(status)
        packet.append(data1)
        
        # Check if this message type needs data2
        msg_type = status & 0xF0
        if msg_type not in [PROGRAM_CHANGE, CHANNEL_PRESSURE]:
            packet.append(data2)
    
    return bytes(packet)

def send_note(sock, addr, note, velocity=100, duration=0.5, channel=0):
    """Send a single MIDI note with Note On followed by Note Off."""
    # Note On
    packet_on = create_mudp_packet([
        (NOTE_ON | channel, note, velocity)
    ])
    sock.sendto(packet_on, addr)
    print(f"Sent: Note On  - Note {note:3d} Vel {velocity:3d}")
    
    time.sleep(duration)
    
    # Note Off
    packet_off = create_mudp_packet([
        (NOTE_OFF | channel, note, 0)
    ])
    sock.sendto(packet_off, addr)
    print(f"Sent: Note Off - Note {note:3d}")

def send_batch_notes(sock, addr, notes, velocity=100, channel=0):
    """Send multiple notes in a single packet (chord)."""
    messages = [(NOTE_ON | channel, note, velocity) for note in notes]
    packet = create_mudp_packet(messages)
    sock.sendto(packet, addr)
    print(f"Sent: Batch Note On - Notes {notes} Vel {velocity}")

def test_single_notes(sock, addr):
    """Test single note messages."""
    print("\n=== Test 1: Single Notes ===")
    
    # Test middle C area (MIDI notes 60-72)
    for note in range(60, 73):
        send_note(sock, addr, note, velocity=100, duration=0.3)
        time.sleep(0.2)

def test_batch_messages(sock, addr):
    """Test batching multiple messages in one packet."""
    print("\n=== Test 2: Batch Messages ===")
    
    # Send a chord (C major: C-E-G)
    send_batch_notes(sock, addr, [60, 64, 67], velocity=100)
    time.sleep(1.0)
    
    # Note off for all
    packet_off = create_mudp_packet([
        (NOTE_OFF, 60, 0),
        (NOTE_OFF, 64, 0),
        (NOTE_OFF, 67, 0)
    ])
    sock.sendto(packet_off, addr)
    print("Sent: Batch Note Off")
    time.sleep(0.5)

def test_velocity_scaling(sock, addr):
    """Test different velocities."""
    print("\n=== Test 3: Velocity Scaling ===")
    
    note = 69  # A440
    for velocity in [20, 40, 60, 80, 100, 127]:
        send_note(sock, addr, note, velocity=velocity, duration=0.4)
        time.sleep(0.3)

def test_all_notes_off(sock, addr):
    """Test All Notes Off control change."""
    print("\n=== Test 4: All Notes Off ===")
    
    # Send several notes on
    send_batch_notes(sock, addr, [60, 62, 64, 65, 67], velocity=100)
    time.sleep(0.5)
    
    # Send All Notes Off (CC 123)
    packet = create_mudp_packet([
        (CONTROL_CHANGE, 123, 0)
    ])
    sock.sendto(packet, addr)
    print("Sent: All Notes Off (CC 123)")
    time.sleep(0.5)

def test_program_change(sock, addr):
    """Test 2-byte message (Program Change)."""
    print("\n=== Test 5: Program Change (2-byte message) ===")
    
    # Send program change + note
    packet = create_mudp_packet([
        (PROGRAM_CHANGE, 5, 0),  # data2 ignored for program change
        (NOTE_ON, 64, 100)
    ])
    sock.sendto(packet, addr)
    print("Sent: Program Change 5 + Note On 64")
    time.sleep(1.0)
    
    packet_off = create_mudp_packet([
        (NOTE_OFF, 64, 0)
    ])
    sock.sendto(packet_off, addr)
    print("Sent: Note Off 64")

def main():
    if len(sys.argv) < 2:
        print("Usage: python test_midiudp.py <esp32_ip> [port]")
        print("Example: python test_midiudp.py 192.168.1.100")
        sys.exit(1)
    
    esp32_ip = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 21928
    
    addr = (esp32_ip, port)
    
    print(f"MIDI/UDP Test Script")
    print(f"Target: {esp32_ip}:{port}")
    print(f"Protocol: MUDP-v1")
    print("=" * 50)
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        # Run tests
        test_single_notes(sock, addr)
        time.sleep(1.0)
        
        test_batch_messages(sock, addr)
        time.sleep(1.0)
        
        test_velocity_scaling(sock, addr)
        time.sleep(1.0)
        
        test_all_notes_off(sock, addr)
        time.sleep(1.0)
        
        test_program_change(sock, addr)
        
        print("\n=== All tests complete ===")
        print(f"\nCheck status at: http://{esp32_ip}/status")
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
