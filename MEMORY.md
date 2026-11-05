# Leraine Studio Architecture & System Memory

## 1. System Structure
- **Core Library (`leraine_lib`)**:
  - `source/structures/chart.h` / `chart.cpp`: Central data structure storing notes, time slices, BPM points, stop points, scroll velocities, and time signatures. Undo/Redo is implemented via `TimeSliceHistory` and `TimeSliceFuture` stacks.
  - `source/modules/`: High-cohesion subsystems initialized and dispatched through `ModuleManager`.
    - `AudioModule`: Audio playback, BASS/BASS_FX engine interface, tempo detection, metronome synthesis, and waveform peak detection.
    - `BeatModule`: Beat calculations, measure line generation, dynamic snap divisor handling, and snap color reassignment.
    - `ChartParserModule`: Parses and serializes `.osu` and StepMania `.sm` file formats.
    - `EditModule`: Manages edit modes (`SelectEditMode`, `NoteEditMode`, `BpmEditMode`).
    - `TimefieldRenderModule`: Renders tracks, receptors, notes, holds, rolls, lifts, fakes, and timing markers.
    - `WaveFormModule`: Audio amplitude envelope rendering.
    - `MiniMapModule`: Timeline minimap overview and scrub navigation.
- **Application (`source/main.cpp`, `source/program/`)**:
  - `Program`: Manages SFML window loop, event handling, main menu bar, and the Unified Dashboard (`DisplayDashboard`).

## 2. Timing Conventions
- `Time` is represented in milliseconds (`int` or `long long`).
- In `.sm` files, `#OFFSET` represents the start time of the first beat in seconds. Positive offset translates to a positive millisecond timestamp.
- Divisors in stream generation and quantization represent measure divisors (4 = quarter note, 16 = 16th note).
