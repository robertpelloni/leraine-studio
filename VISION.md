# Leraine Studio — Core Vision & Architecture Philosophy

## 1. Executive Summary
**Leraine Studio** is a cross-platform, high-performance vertical scrolling rhythm game (VSRG) chart editor built with C++17, SFML, Dear ImGui, and BASS Audio Engine.

The editor synthesizes the strengths of three landmark rhythmic tools:
1. **osu!mania Editor**: Intuitive note placement, rapid keyboard-centric workflow, and responsive timeline interaction.
2. **ArrowVortex**: Precision timing grids, multi-format synchronization, measure line visual anchors, and comprehensive StepMania (`.sm`) support.
3. **DDreamStudio**: Advanced timing tools, real-time tap BPM analysis, waveform peak snapping, and algorithmic tempo detection.

## 2. Foundational Pillars
- **Zero Data Loss Guarantee**: Strict preservation of timing data, stops, scroll velocity multipliers, time signatures, and background/foreground tags (`#BGCHANGES`, `#FGCHANGES`).
- **Unified Control Center**: All critical workflows, pattern generation tools, audio settings, and analytics are accessible from a singular consolidated dashboard.
- **Deterministic Audio & Visual Synchronization**: Frame-accurate audio scrub, hitsound synthesis, and beat snap alignment.
- **Extensible Modularity**: Loosely coupled architecture with specialized modules managed via `ModuleManager`.
