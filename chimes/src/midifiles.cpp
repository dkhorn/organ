#include "midifiles.h"
#include "midiseq.h"
#include "logger.h"
#include <SPIFFS.h>
#include <FS.h>

// Global instance
MIDIFileManager midiFiles;

static const char* MIDI_DIR = "/midi";

bool MIDIFileManager::begin() {
    if (initialized) {
        return true;
    }
    
    Log.println("Initializing MIDI file manager...");
    
    // Mount SPIFFS
    if (!SPIFFS.begin(true)) {  // true = format on fail
        Log.println("SPIFFS mount failed!");
        return false;
    }
    
    // Create MIDI directory if it doesn't exist
    if (!SPIFFS.exists(MIDI_DIR)) {
        SPIFFS.mkdir(MIDI_DIR);
        Log.printf("Created directory: %s\n", MIDI_DIR);
    }
    
    initialized = true;
    
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    Log.printf("SPIFFS: %d KB total, %d KB used, %d KB free\n", 
               total / 1024, used / 1024, (total - used) / 1024);
    
    return true;
}

String MIDIFileManager::makeFullPath(const String& name) {
    return String(MIDI_DIR) + "/" + name;
}

bool MIDIFileManager::validateFilename(const String& name) {
    // Check length (leave room for directory prefix)
    if (name.length() == 0 || name.length() > 31) {
        return false;
    }
    
    // Check for invalid characters
    if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 ||
        name.indexOf(':') >= 0 || name.indexOf('*') >= 0 ||
        name.indexOf('?') >= 0 || name.indexOf('"') >= 0 ||
        name.indexOf('<') >= 0 || name.indexOf('>') >= 0 ||
        name.indexOf('|') >= 0) {
        return false;
    }
    
    return true;
}

bool MIDIFileManager::isMidiFile(const uint8_t* data, size_t size) {
    // Check for MThd header (MIDI file signature)
    if (size < 14) return false;
    return (data[0] == 'M' && data[1] == 'T' && data[2] == 'h' && data[3] == 'd');
}

bool MIDIFileManager::uploadFile(const String& name, const uint8_t* data, size_t size) {
    if (!initialized) {
        Log.println("File manager not initialized");
        return false;
    }
    
    if (!validateFilename(name)) {
        Log.printf("Invalid filename: %s\n", name.c_str());
        return false;
    }
    
    // Validate MIDI file format
    if (!isMidiFile(data, size)) {
        Log.println("Not a valid MIDI file (missing MThd header)");
        return false;
    }
    
    // Check if we have enough space
    size_t freeSpace = getFreeBytes();
    if (size > freeSpace) {
        Log.printf("Insufficient space: need %d bytes, have %d bytes\n", size, freeSpace);
        return false;
    }
    
    String fullPath = makeFullPath(name);
    
    // Write file
    File file = SPIFFS.open(fullPath, FILE_WRITE);
    if (!file) {
        Log.printf("Failed to open file for writing: %s\n", fullPath.c_str());
        return false;
    }
    
    size_t written = file.write(data, size);
    file.close();
    
    if (written != size) {
        Log.printf("Write error: wrote %d of %d bytes\n", written, size);
        SPIFFS.remove(fullPath);
        return false;
    }
    
    Log.printf("Uploaded MIDI file: %s (%d bytes)\n", name.c_str(), size);
    return true;
}

std::vector<MIDIFileManager::FileInfo> MIDIFileManager::listFiles() {
    std::vector<FileInfo> files;
    
    if (!initialized) {
        return files;
    }
    
    File dir = SPIFFS.open(MIDI_DIR);
    if (!dir || !dir.isDirectory()) {
        return files;
    }
    
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            FileInfo info;
            String fullName = file.name();
            // Strip directory prefix
            int lastSlash = fullName.lastIndexOf('/');
            info.name = (lastSlash >= 0) ? fullName.substring(lastSlash + 1) : fullName;
            info.size = file.size();
            files.push_back(info);
        }
        file = dir.openNextFile();
    }
    
    return files;
}

bool MIDIFileManager::deleteFile(const String& name) {
    if (!initialized) {
        return false;
    }
    
    if (!validateFilename(name)) {
        return false;
    }
    
    String fullPath = makeFullPath(name);
    
    if (!SPIFFS.exists(fullPath)) {
        Log.printf("File not found: %s\n", name.c_str());
        return false;
    }
    
    bool result = SPIFFS.remove(fullPath);
    if (result) {
        Log.printf("Deleted MIDI file: %s\n", name.c_str());
    }
    
    return result;
}

bool MIDIFileManager::readFile(const String& name, uint8_t** outData, size_t* outSize) {
    if (!initialized || !outData || !outSize) {
        return false;
    }
    
    if (!validateFilename(name)) {
        return false;
    }
    
    String fullPath = makeFullPath(name);
    
    if (!SPIFFS.exists(fullPath)) {
        Log.printf("File not found: %s\n", name.c_str());
        return false;
    }
    
    File file = SPIFFS.open(fullPath, FILE_READ);
    if (!file) {
        Log.printf("Failed to open file: %s\n", fullPath.c_str());
        return false;
    }
    
    size_t size = file.size();
    uint8_t* buffer = (uint8_t*)malloc(size);
    if (!buffer) {
        file.close();
        Log.println("Failed to allocate memory for file read");
        return false;
    }
    
    size_t bytesRead = file.read(buffer, size);
    file.close();
    
    if (bytesRead != size) {
        free(buffer);
        Log.printf("Read error: read %d of %d bytes\n", bytesRead, size);
        return false;
    }
    
    *outData = buffer;
    *outSize = size;
    return true;
}

bool MIDIFileManager::playFile(const String& name, const PlaybackParams& params) {
    if (!initialized) {
        return false;
    }
    
    // Read file
    uint8_t* data = nullptr;
    size_t size = 0;
    
    if (!readFile(name, &data, &size)) {
        return false;
    }
    
    // Stop any current playback
    midiseq_stop();
    
    // Load MIDI file
    bool success = midiseq_load_from_buffer(data, size);
    
    free(data);
    
    if (!success) {
        Log.printf("Failed to load MIDI file: %s\n", name.c_str());
        return false;
    }
    
    // Set playback parameters AFTER loading (which calls midiseq_play internally)
    midiseq_set_tempo_scale(params.tempoScale);
    midiseq_set_velocity_scale(params.velocityScale);
    midiseq_set_transpose(params.transpose);
    
    Log.printf("Playing MIDI file: %s (tempo=%.2fx, vel=%.2fx, transpose=%+d)\n",
               name.c_str(), params.tempoScale, params.velocityScale, params.transpose);
    
    return true;
}

size_t MIDIFileManager::getTotalBytes() {
    return initialized ? SPIFFS.totalBytes() : 0;
}

size_t MIDIFileManager::getUsedBytes() {
    return initialized ? SPIFFS.usedBytes() : 0;
}

size_t MIDIFileManager::getFreeBytes() {
    if (!initialized) return 0;
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    return (total > used) ? (total - used) : 0;
}
