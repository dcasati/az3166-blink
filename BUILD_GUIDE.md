# Quick Build Guide - SuperEasy Example

## What Just Happened

The SuperEasy example built successfully! Here's what it does:

### The Program Shows:

1. **Startup**
   - Displays "Hello!" and "I'm ready!" on screen
   - Shows a rainbow LED effect
   - Green LED for 2 seconds

2. **Main Loop (repeats every 2 seconds)**
   - Reads the temperature
   - Shows "Temperature:" on line 1
   - Shows "Temp: XX.X" on line 2
   - Changes LED based on temperature:
     - RED if hot (over 25°C)
     - BLUE if cold (under 20°C)  
     - GREEN if just right (20-25°C)
   - Blinks YELLOW twice if board is moving

## Build Results

- **RAM Usage**: 17.0% (44,512 bytes)
- **Flash Usage**: 21.1% (220,900 bytes)
- **Build Time**: 2.62 seconds
- **Status**: SUCCESS ✓

## Libraries Used

- **EasyDisplay** - Screen and LED control
- **EasySensors** - Temperature and motion detection

## Next Steps

### 1. Upload to Board
```bash
platformio run --target upload --environment mxchip_az3166
```

### 2. Monitor Serial Output
```bash
platformio device monitor
```

### 3. Or Upload + Monitor Together
```bash
platformio run --target upload --target monitor --environment mxchip_az3166
```

## What to Expect When Running

### On the Screen:
```
Line 1: Temperature:
Line 2: Temp: 24.5
Line 3: It's hot! (or "It's cold!" or "Just right!")
```

### On the LED:
- Green at startup (2 seconds)
- Rainbow effect at startup
- Red/Blue/Green based on temperature
- Yellow blink when you shake it

### In Serial Monitor:
```
Hello! The board is starting...
Temperature is: 24.5 degrees
Hey! Someone is moving me! (when shaken)
```

## Test It!

1. **Temperature Test**: 
   - Watch the screen show temperature
   - LED changes color based on temperature

2. **Motion Test**:
   - Shake or move the board
   - LED blinks yellow twice
   - Serial says "Hey! Someone is moving me!"

3. **Rainbow Test**:
   - Watch the startup rainbow effect
   - Red → Yellow → Green → Cyan → Blue → Purple

## Modify It!

Your son can easily change things:

### Change Temperature Limits:
```cpp
// Make "hot" be 23 degrees instead of 25
if (temp > 23) {
    EasyDisplay::setLED("red");
}
```

### Change Colors:
```cpp
// Use purple instead of red for hot
EasyDisplay::setLED("purple");
```

### Change Timing:
```cpp
// Check every 1 second instead of 2
delay(1000);
```

### Add More Lines:
```cpp
// Show humidity too
float humidity = EasySensors::getHumidity();
EasyDisplay::writeNumber(4, "Humid: ", humidity);
```

## Available Files to Build

To switch to a different example:

### Example 1 - SuperEasy (Current):
```bash
cp examples/01_SuperEasy/SuperEasy.cpp src/main.cpp
platformio run --environment mxchip_az3166
```

### Example 2 - WiFi and Internet:
```bash
cp examples/02_WiFiAndInternet/WiFiAndInternet.cpp src/main.cpp
# Edit WiFi credentials first!
platformio run --environment mxchip_az3166
```

### Example 3 - Motion Game:
```bash
cp examples/03_MotionGame/MotionGame.cpp src/main.cpp
platformio run --environment mxchip_az3166
```

### Back to Modular Version:
```bash
cp src/main_modular.cpp.bak src/main.cpp
platformio run --environment mxchip_az3166
```

## Troubleshooting

### If Build Fails:
1. Check that all library files exist in `lib/` folder
2. Run: `platformio run --environment mxchip_az3166 --verbose`
3. Look for specific error messages

### If Upload Fails:
1. Make sure board is plugged in
2. Check USB cable
3. Try different USB port
4. Press reset button on board

### If Nothing Shows:
1. Check Serial Monitor (115200 baud)
2. Press reset button on board
3. Make sure board has power (LED should be on)

## Quick Commands

```bash
# Build only
platformio run --environment mxchip_az3166

# Upload only
platformio run --target upload --environment mxchip_az3166

# Monitor only
platformio device monitor

# Clean build
platformio run --target clean --environment mxchip_az3166

# Build + Upload + Monitor (all in one!)
platformio run --target upload --target monitor --environment mxchip_az3166
```

## For Your Son

Tell him:
1. "The board is ready to run!"
2. "It will show the temperature and change colors"
3. "Try shaking it to see yellow blinks!"
4. "Look at the screen to see the numbers"

Have fun programming! 🚀
