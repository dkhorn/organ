#include "midiudp.h"
#include "midihandler.h"
#include "logger.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// Global instance
MIDIoverUDP midiUDP;

// UDP socket
static WiFiUDP udp;

void MIDIoverUDP::begin(uint16_t port) {
    this->port = port;
    this->listening = false;
    this->packetsReceived = 0;
    this->messagesReceived = 0;
    this->packetsDropped = 0;
    
    // Only start listening if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        if (udp.begin(port)) {
            listening = true;
            Log.printf("MIDI/UDP listening on port %d\n", port);
        } else {
            Log.printf("MIDI/UDP failed to start on port %d\n", port);
        }
    } else {
        Log.println("MIDI/UDP: WiFi not connected, will start when connected");
    }
}

void MIDIoverUDP::update() {
    // If not listening but WiFi is now connected, try to start
    if (!listening && WiFi.status() == WL_CONNECTED) {
        if (udp.begin(port)) {
            listening = true;
            Log.printf("MIDI/UDP now listening on port %d\n", port);
        }
    }
    
    // If listening but WiFi disconnected, stop
    if (listening && WiFi.status() != WL_CONNECTED) {
        udp.stop();
        listening = false;
        Log.println("MIDI/UDP stopped (WiFi disconnected)");
    }
    
    if (!listening) return;
    
    // Check for incoming packets
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        // Allocate buffer on stack (max 1KB)
        uint8_t buffer[MAX_PACKET_SIZE];
        size_t len = packetSize > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : packetSize;
        
        // Read packet
        int bytesRead = udp.read(buffer, len);
        if (bytesRead > 0) {
            handlePacket(buffer, bytesRead);
        }
    }
}

void MIDIoverUDP::handlePacket(uint8_t* data, size_t length) {
    Log.printf("MIDI/UDP: Received packet of %d bytes\r\n", length);
    // Validate minimum packet size
    if (length < MIN_PACKET_SIZE) {
        packetsDropped++;
        Log.printf("MIDI/UDP: Packet too small (%d bytes)\n", length);
        return;
    }
    
    // Validate magic bytes
    if (data[0] != MAGIC_M || data[1] != MAGIC_U) {
        packetsDropped++;
        Log.printf("MIDI/UDP: Invalid magic bytes: 0x%02X 0x%02X\n", data[0], data[1]);
        return;
    }
    
    // Check version
    uint8_t version = data[2];
    if (version != VERSION) {
        packetsDropped++;
        Log.printf("MIDI/UDP: Unsupported version: %d\n", version);
        return;
    }
    
    // Get message count
    uint8_t count = data[3];
    if (count == 0) {
        packetsDropped++;
        Log.println("MIDI/UDP: Empty packet (count=0)");
        return;
    }
    
    // Parse messages
    uint8_t* p = data + 4;
    size_t remaining = length - 4;
    
    for (int i = 0; i < count; i++) {
        // Need at least status + data1
        if (remaining < 2) {
            packetsDropped++;
            Log.printf("MIDI/UDP: Truncated packet at message %d\n", i);
            return;
        }
        
        uint8_t status = *p++;
        remaining--;
        
        // Validate status byte (must be 0x80-0xEF)
        if (status < 0x80 || status > 0xEF) {
            packetsDropped++;
            Log.printf("MIDI/UDP: Invalid status byte: 0x%02X\n", status);
            return;
        }
        
        uint8_t type = status & 0xF0;
        uint8_t d1 = *p++;
        remaining--;
        
        uint8_t d2 = 0;
        
        // Check if this message type needs a second data byte
        if (type != 0xC0 && type != 0xD0) {  // Not Program Change or Channel Pressure
            if (remaining < 1) {
                packetsDropped++;
                Log.printf("MIDI/UDP: Truncated packet (missing data2) at message %d\n", i);
                return;
            }
            d2 = *p++;
            remaining--;
        }
        
        // Handle the MIDI message
        handleMIDIMessage(status, d1, d2);
        messagesReceived++;
    }
    
    packetsReceived++;
}

void MIDIoverUDP::handleMIDIMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    // Delegate to common MIDI handler
    handle_midi_message(status, data1, data2);
}

void MIDIoverUDP::end() {
    if (listening) {
        udp.stop();
        listening = false;
        Log.println("MIDI/UDP stopped");
    }
}

bool MIDIoverUDP::isListening() const {
    return listening;
}

uint16_t MIDIoverUDP::getPort() const {
    return port;
}
