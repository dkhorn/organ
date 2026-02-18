#ifndef MIDIUDP_H
#define MIDIUDP_H

#include <Arduino.h>

/**
 * MIDI over UDP receiver (MUDP-v1 protocol)
 * 
 * Implements a simple, stateless UDP-based MIDI protocol:
 * - Fixed 4-byte header: [0x4D 0x55 version count]
 * - Variable message records with full MIDI status bytes
 * - No running status, always explicit status bytes
 * - Supports batching multiple MIDI messages in one packet
 */
class MIDIoverUDP {
public:
    /**
     * Initialize MIDI UDP receiver
     * @param port UDP port to listen on (default: 21928)
     */
    void begin(uint16_t port = 21928);
    
    /**
     * Process incoming UDP packets
     * Call from main loop
     */
    void update();
    
    /**
     * Stop UDP receiver
     */
    void end();
    
    /**
     * Check if receiver is active
     */
    bool isListening() const;
    
    /**
     * Get current listening port
     */
    uint16_t getPort() const;
    
    /**
     * Get statistics
     */
    uint32_t getPacketsReceived() const { return packetsReceived; }
    uint32_t getMessagesReceived() const { return messagesReceived; }
    uint32_t getPacketsDropped() const { return packetsDropped; }

private:
    void handlePacket(uint8_t* data, size_t length);
    void handleMIDIMessage(uint8_t status, uint8_t data1, uint8_t data2);
    
    uint16_t port;
    bool listening;
    
    // Statistics
    uint32_t packetsReceived;
    uint32_t messagesReceived;
    uint32_t packetsDropped;
    
    // Protocol constants
    static const uint8_t MAGIC_M = 0x4D;  // 'M'
    static const uint8_t MAGIC_U = 0x55;  // 'U'
    static const uint8_t VERSION = 0x01;
    static const size_t MIN_PACKET_SIZE = 4;  // Header only
    static const size_t MAX_PACKET_SIZE = 1024;
};

// Global instance
extern MIDIoverUDP midiUDP;

#endif // MIDIUDP_H
