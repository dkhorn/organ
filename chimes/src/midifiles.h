#ifndef MIDIFILES_H
#define MIDIFILES_H

#include <Arduino.h>
#include <vector>

/**
 * MIDI File Manager
 * Stores and manages MIDI files in SPIFFS filesystem
 */
class MIDIFileManager {
public:
    struct FileInfo {
        String name;
        size_t size;
    };
    
    struct PlaybackParams {
        float velocityScale;    // 0.0-2.0 (1.0 = no scaling)
        float tempoScale;       // 0.1-4.0 (1.0 = no scaling)
        int8_t transpose;       // -12 to +12 semitones
    };
    
    /**
     * Initialize SPIFFS and file manager
     * Returns true if successful
     */
    bool begin();
    
    /**
     * Upload a MIDI file
     * @param name Filename (without path, max 31 chars)
     * @param data MIDI file data
     * @param size Size in bytes
     * @return true if successful
     */
    bool uploadFile(const String& name, const uint8_t* data, size_t size);
    
    /**
     * List all MIDI files
     */
    std::vector<FileInfo> listFiles();
    
    /**
     * Delete a file by name
     */
    bool deleteFile(const String& name);
    
    /**
     * Read a file's contents
     * @param name Filename
     * @param outData Buffer to store data (caller must free)
     * @param outSize Size of data read
     * @return true if successful
     */
    bool readFile(const String& name, uint8_t** outData, size_t* outSize);
    
    /**
     * Play a MIDI file
     * @param name Filename
     * @param params Playback parameters
     * @return true if playback started successfully
     */
    bool playFile(const String& name, const PlaybackParams& params);
    
    /**
     * Get filesystem usage info
     */
    size_t getTotalBytes();
    size_t getUsedBytes();
    size_t getFreeBytes();
    
private:
    bool initialized;
    String makeFullPath(const String& name);
    bool validateFilename(const String& name);
    bool isMidiFile(const uint8_t* data, size_t size);
};

// Global instance
extern MIDIFileManager midiFiles;

#endif // MIDIFILES_H
