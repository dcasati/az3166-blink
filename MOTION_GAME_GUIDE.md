# Motion Detector Game - How to Play!

## What Just Uploaded

The **Motion Detector Game** is now running on your AZ3166! This is a fun, interactive game that counts how many times you shake or move the board.

## Build Results

- **Status**: ✓ SUCCESS - Uploaded to board!
- **RAM Usage**: 17.0% (44,520 bytes)
- **Flash Usage**: 21.1% (221,276 bytes)

## What You'll See

### On the Screen:

**When it starts:**
```
Line 1: Motion Game!
Line 2: Shake me!
Line 3: Moves:
Line 4: Count: 0
```

**After shaking:**
```
Line 1: Motion Game!
Line 2: Flat (or "Tilted!" if board is tilted)
Line 3: Moves:
Line 4: Count: 5 (increases each shake)
```

### On the LED:

The LED flashes different colors each time you move it:
- **Red** - Every 3rd movement (moves 3, 6, 9, 12...)
- **Green** - Every 3rd + 1 movement (moves 1, 4, 7, 10...)
- **Blue** - Every 3rd + 2 movement (moves 2, 5, 8, 11...)
- **Rainbow** - Every 10th movement! (10, 20, 30...)

The LED turns off when the board is still.

### In Serial Monitor:

```
Motion Detector Game Starting!
Movement detected! Count: 1
Movement detected! Count: 2
Movement detected! Count: 3
...
```

## How to Play

### 1. Basic Play:
- **Shake the board** - Watch the count go up!
- **Each shake** changes the LED color
- **Keep shaking** to see different colors

### 2. Try to Get to 10:
- Shake the board 10 times
- On the 10th shake, you get a **RAINBOW!**
- The screen shows "10 moves!"

### 3. Tilt Detection:
- **Hold board flat** - Screen shows "Flat"
- **Tilt the board** - Screen shows "Tilted!"
- Tilt angle must be more than 45 degrees

### 4. Color Pattern Game:
Try to predict the color:
- Shake 1 time → GREEN
- Shake 2 times → BLUE
- Shake 3 times → RED
- Shake 4 times → GREEN
- Pattern repeats: Green, Blue, Red, Green, Blue, Red...

## Game Challenges

### Challenge 1: Rainbow Hunter
**Goal**: Get 3 rainbows (30 total shakes)
**Difficulty**: Easy
**Reward**: Pretty rainbow effects!

### Challenge 2: Color Predictor
**Goal**: Call out the color before shaking
**Difficulty**: Medium
**How**: Remember the pattern (Green, Blue, Red, repeat)

### Challenge 3: Speed Shaker
**Goal**: Get to 20 shakes as fast as possible
**Difficulty**: Hard
**How**: Shake rapidly but consistently

### Challenge 4: Gentle Touch
**Goal**: Move it just enough to trigger, but gently
**Difficulty**: Expert
**How**: Slow, gentle movements still count!

## How the Game Works

### Motion Detection:
- Uses the **accelerometer** sensor
- Detects when movement changes gravity
- Compares total acceleration to 1g (gravity)
- Triggers when acceleration is less than 0.8g or more than 1.2g

### Counting Logic:
```cpp
// Only counts NEW movements
if (isMovingNow && !wasMoving) {
    moveCount++;  // This is a new shake!
}
```

### Color Selection:
```cpp
if (moveCount % 3 == 0) {      // Divisible by 3
    EasyDisplay::setLED("red");
} else if (moveCount % 3 == 1) { // Remainder 1
    EasyDisplay::setLED("green");
} else {                         // Remainder 2
    EasyDisplay::setLED("blue");
}
```

### Rainbow Bonus:
```cpp
if (moveCount % 10 == 0) {      // Every 10th movement
    EasyDisplay::write(2, "10 moves!");
    EasyDisplay::rainbow();
}
```

## Modify the Game

Your son can easily change the game!

### Make it Easier (Count Faster):
```cpp
// Change the sensitivity - lower number = more sensitive
// In the code, find this line and change 0.8 and 1.2:
return (totalAccel < 0.9 || totalAccel > 1.1);  // More sensitive!
```

### Change the Rainbow Frequency:
```cpp
// Show rainbow every 5 moves instead of 10
if (moveCount % 5 == 0) {
    EasyDisplay::rainbow();
}
```

### Add More Colors:
```cpp
// Use 4 colors instead of 3
if (moveCount % 4 == 0) {
    EasyDisplay::setLED("red");
} else if (moveCount % 4 == 1) {
    EasyDisplay::setLED("green");
} else if (moveCount % 4 == 2) {
    EasyDisplay::setLED("blue");
} else {
    EasyDisplay::setLED("yellow");  // NEW color!
}
```

### Change What It Counts:
```cpp
// Count tilts instead of movements
if (EasySensors::isTilted() && !wasTilted) {
    moveCount++;
    EasyDisplay::write(2, "Tilted!");
}
```

### Add Sound Effects (in Serial):
```cpp
if (isMovingNow && !wasMoving) {
    moveCount++;
    Serial.println("BOOM! Movement detected!");  // Fun message
}
```

## What Your Son Will Learn

### Programming Concepts:
1. **Variables** - `moveCount`, `wasMoving`
2. **If statements** - Color selection logic
3. **Boolean logic** - Movement detection
4. **Modulo operator** - `%` for patterns
5. **State tracking** - Remembering previous movement

### Electronics Concepts:
1. **Accelerometer** - Measures movement
2. **LED control** - Visual feedback
3. **Display updates** - Showing information
4. **Sensor polling** - Checking sensors repeatedly

## Troubleshooting

### Not Counting Movements:
- Shake harder or faster
- Try a quick flick motion
- Board needs to go from still to moving

### Counting Too Much:
- Movements are too continuous
- Try discrete shakes with pauses
- Make movements more distinct

### LED Not Changing:
- Check that movement is being detected
- Look at Serial Monitor for "Movement detected!"
- Make sure LED isn't broken (try other examples)

### Tilt Always Says "Flat":
- Tilt more than 45 degrees
- Try standing board on its edge
- Move board from flat to tilted position

## Next Steps

### 1. Play the Game:
- Try all the challenges
- See how high you can count
- Practice the color pattern

### 2. Modify It:
- Change colors
- Adjust sensitivity
- Add your own features

### 3. Create Your Own Version:
- Make a "Tilt Counter" game
- Create a "Spin Detector"
- Invent new challenges

## Fun Facts About This Code

- Uses only **221 KB** of Flash memory
- Checks motion **10 times per second** (every 100ms)
- Can count over **32,000 movements** before resetting
- Rainbow uses **6 colors** in sequence
- Tilt detection checks if angle > **45 degrees**

## To Monitor the Game

If you want to see the Serial output:
```bash
platformio device monitor --baud 115200
```

Press **Ctrl+C** to stop monitoring.

---

**Have fun shaking and tilting your board!** 🎮

The game is ready to play - just start shaking!
