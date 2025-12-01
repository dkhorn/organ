# Organ Shared Libraries

This directory contains shared code libraries used across multiple PlatformIO projects in the workspace.

## Structure

```
shared/
  OrganCommon/     - WiFi, OTA, and telnet logging functionality
  OrganPins/       - Hardware pin definitions
```

## Usage in PlatformIO Projects

### 1. Add to platformio.ini

Add this line to your `platformio.ini`:

```ini
lib_extra_dirs = ../shared
```

### 2. Include in your code

```cpp
#include <OrganCommon.h>
#include <OrganPins.h>
```

### 3. Use the libraries

**OrganCommon** provides:
- `organcommon_setup(wifi_ssid, wifi_pass, ota_hostname, ota_password, app_version)` - Initialize WiFi, OTA, and telnet
- `organcommon_loop()` - Handle OTA updates and telnet connections
- `Log` - Global Print object that outputs to both Serial and Telnet

**OrganPins** provides:
- Pin constants: `PIN_MOSI`, `PIN_SCK`, `PIN_LATCH`, `PIN_OE_N`, `PIN_CLR_N`, `PIN_BUTTON`

## Example

```cpp
#include <Arduino.h>
#include <OrganCommon.h>
#include <OrganPins.h>

void setup() {
  organcommon_setup("YourWiFi", "password", "hostname", "otapass", "1.0.0");
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Log.println("Setup complete!");
}

void loop() {
  organcommon_loop();
  // Your code here
}
```

## Adding New Shared Libraries

To create a new shared library:

1. Create directory: `shared/YourLibName/src/`
2. Add `library.json` with library metadata
3. Add your `.h` and `.cpp` files in `src/`
4. Projects using `lib_extra_dirs = ../shared` will automatically find it

## Projects Using Shared Libraries

- **hardwaretest** - Uses OrganCommon and OrganPins
- **chimes** - Will be migrated to use shared libraries (stable, not changed yet)
