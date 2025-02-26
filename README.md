## Handmade Hero starter 


This is my starter project for the Handmade Hero series. 

- https://mollyrocket.com/#handmade 
- https://davidgow.net/handmadepenguin

I'll be using it to get good, and probably as a starting points for my own projects in the future. 

---

#### Made with 

- C++ 20 kinda but mostly just C
- Linux 
- SDL 3.2.4
- GNU Make 4.3

It's not pretty, but it sure works great. Not sticking to common practice very much, just doing what feels good. 

Sticking with make because I want to get a feel for building compiler commands myself, once I feel confident I may switch to cmake or meson, but keeping it simple for now. Less is more.  

I've done the basic platform layer quite faithfully to the series so far. I'm avoiding C++ features and libs, trying to maintain C compatibility as much as possible for the platform-independant code, perhaps I'll switch to only C if I don't use anything from C++. The platform layer has lots of linux stuff as well as SDL3. 

I haven't done sound yet for a few reasons, my brain is not quite big enough, SDL3 sound is quite different from SDL2 that was used for the handmade penguin port, I'll figure it out soon enough. 

---

### Build modes 

To build you'll need 
- Platform essentials as usual, g++ or clang++ 
- SDL3 built in static mode, mine is in /usr/local
- GNU make, or add your own shell scripts 

```bash

# Build the game dll
make 

# Build the platform layer with special features
make platform 

# Or build the whole lot in static mode with less debug stuff
make static 

# Then run the program 
./main

# Also there's 
make clean 

```

There's not much to see at this point, the screen is initialised to Red, up and down inputs will change the Alpha value for all pixels. 

---

### Dev features 

When built shared, some things are enabled 

**Hot reloading** 

The platform layer must be stopped and restarted when changed, but the core game can be hot reloaded by re-running `make`, the platform layer will detect changes, re-import the shared object and use the updated code. 

**Save states**

Hold ALT and press 7, 8, 9 or 0 to select one of four save-state slots

Press L to create a restore point and start recording inputs

Press L again to stop recording and start replaying the inputs 

Press L a third time to stop playback 

The data is just the whole memory of the program dumped as binary into `tmp/playback_[n].dat` and each one is 604MB in size 