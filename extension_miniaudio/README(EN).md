# Defold Miniaudio Extension

[한국어 문서 보기](./README(KO).md)


**Defold Miniaudio Extension** is a high-performance audio solution for the Defold engine.
It is built on top of the [**miniaudio**](https://miniaud.io) library and adopts a **memory-based streaming approach using `sys.load_resource`**, which completely resolves file path issues that often occur after bundling.

***

## Key Features

- **Full Bundling Support**
Loads and plays audio files directly from memory instead of relying on file paths. Works reliably on **Android, iOS, and HTML5**.
- **High-Resolution Audio**
Supports **16-bit**, **24-bit**, and **32-bit Float** formats for superior sound quality.
- **Advanced Controls**
    - **BGM:** Looping, Pause/Resume, Fade In/Out
    - **SFX:** Pooled playback with up to **8 concurrent channels**, stereo panning, and overlapping playback
    - **Volume:** Real-time master and per-track volume control
- **Optimized Performance**
Uses C++ heap memory allocation to minimize Lua GC overhead.
- **Easy Integration**
Includes a ready-to-use Lua wrapper. Simply `require` and start playing.

***

## Installation

### 1. Folder Structure

Download this repository and copy the `extension_miniaudio` folder into your project root.

```
Project Root
├── extension_miniaudio/
│   ├── include/
│   ├── src/
│   ├── ext.manifest
│   └── miniaudio.lua       <-- Main Lua module
├── assets/
│   └── sounds/             <-- Your audio files
└── game.project
```


### 2. game.project Configuration

You must configure Defold to include your sound assets in the bundle.

1. Open the `game.project` file.
2. In the `[project]` section, add your sound folder path to the `custom_resources` field.

Example:

```
[project]
custom_resources = /assets/sounds
```

You can separate multiple paths using commas.

***

## Quick Start

Here’s a minimal example to get started:

```lua
-- 1. Require the module
local ma = require "extension_miniaudio.miniaudio"

function init(self)
    -- 2. Initialize the engine (required)
    ma.init()

    -- 3. Play background music
    ma.play_bgm("/assets/sounds/bgm_main.ogg")
end

function on_input(self, action_id, action)
    if action_id == hash("touch") and action.pressed then
        -- 4. Play a sound effect
        ma.play_sfx("/assets/sounds/click.wav")
    end
end
```


***

## API Reference

### 1. Initialization

```lua
ma.init()
```

Initializes the audio engine.
Call this **once** at game start (e.g., in `main.script`).

***

### 2. Background Music (BGM)

Only one BGM track can be active at a time. Looping is automatic.

```lua
ma.play_bgm(path)
```

- **path** *(string)* — Path to the audio file (e.g., `"/assets/music.ogg"`)
- Stops the current BGM, frees memory, and starts the new one.

```lua
ma.stop_bgm()
```

- Stops the BGM completely and resets its playback position.

```lua
ma.pause_bgm()
```

- Pauses the BGM at the current position.

```lua
ma.resume_bgm()
```

- Resumes playback from the paused position.

```lua
ma.set_bgm_volume(volume)
```

- **volume** *(number)* — `0.0` (mute) to `1.0` (max)
- Changes BGM volume instantly.

```lua
ma.fade_bgm(target_volume, duration)
```

- **target_volume** *(number)* — Range: `0.0` ~ `1.0`
- **duration** *(number)* — Duration in seconds
- Smoothly changes BGM volume over the given duration (for fade in/out).

***

### 3. Sound Effects (SFX)

SFX playback uses a **pooled system** that allows multiple overlapping sounds.

```lua
ma.play_sfx(path, [pan])
```

- **path** *(string)* — Sound file path
- **pan** *(number, optional)* — Stereo panning
    - `-1.0`: Full left
    - `0.0`: Center (default)
    - `1.0`: Full right
- Plays the sound effect. If `pan` is omitted, plays at center.

```lua
ma.set_sfx_volume(volume)
```

- **volume** *(number)* — `0.0` ~ `1.0`
- Sets the master volume for all SFX channels.

***

## Advanced Guide

### Smooth BGM Transition (Crossfade)

Implement seamless BGM switching without abrupt stops:

```lua
-- 1. Fade out current BGM (over 2 seconds)
ma.fade_bgm(0.0, 2.0)

-- 2. Play new music after fade-out
timer.delay(2.0, false, function()
    ma.play_bgm("/assets/sounds/boss_theme.ogg")
    ma.set_bgm_volume(0.0) -- Start silent

    -- 3. Fade in (over 3 seconds)
    ma.fade_bgm(1.0, 3.0)
end)
```


***

### Positional Sound (Panning)

Adjust panning dynamically based on on-screen position:

```lua
-- Map screen position (-1.0 left ~ 1.0 right)
local screen_width = sys.get_config("display.width")
local pan_value = (action.x / screen_width) * 2.0 - 1.0

ma.play_sfx("/assets/sounds/explosion.wav", pan_value)
```


***

## Technical Details

The extension stores binary audio data loaded via `sys.load_resource()`
in **C++ heap memory (malloc)**.

- **Stability**: Prevents Lua's garbage collector from freeing sound data mid-playback.
- **Efficiency**: Frees memory immediately when switching BGMs, preventing memory leaks.

***

## License

This project is licensed under the **MIT License**.
See the `LICENSE` file for full details.

***