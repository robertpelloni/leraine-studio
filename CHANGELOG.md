# Changelog

All notable changes to this project will be documented in this file.

## [1.1.0] - 2026-11-04
### Added
- **Unified Control Dashboard**: Implemented a comprehensive, single-page interactive control dashboard (`Program::DisplayDashboard`) consolidating playback, audio rates, note transformations, timing tools, stream generator, difficulty analytics, and session options.
- **UI Tooltips & Guidance**: Added detailed explanation markers `(?)` across all dashboard tools, note operations, and settings to maximize usability.
- **Session Governance & Standards**: Added `VISION.md`, `MEMORY.md`, `DEPLOY.md`, `ROADMAP.md`, `TODO.md`, `IDEAS.md`, and `VERSION.md`.

### Fixed
- **Enum Redefinition**: Resolved duplicate definition of `StreamPattern` enum in `source/structures/chart.h`.
- **Method Declaration**: Declared `RemoveTimeSignature` accessor in `Chart` header to resolve compilation mismatch.
- **Audio Method Bindings**: Updated audio speed and playback handlers in UI to match `AudioModule` public API.

## [1.0.1] - 2025-12-25
### Added
- Full ArrowVortex feature parity: StepMania (`.sm`) import/export, stops, scroll velocity multipliers, time signatures, and measure line markers.
- Procedural stream generator with multiple pattern algorithms.
- NPS / difficulty estimator and audio hitsounds.
