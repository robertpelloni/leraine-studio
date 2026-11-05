# Changelog

All notable changes to this project will be documented in this file.

## [1.5.0] - 2026-11-04
### Added
- **Skin Query Helpers & Color Mapping**: Implemented `Skin::GetLoadedKeyAmount()` and `Skin::GetSnapColor()` for querying skin metrics and fallback beat snap coloration.
- **Dashboard Skin Information**: Integrated skin asset path inspection and direct skin folder selection picker into the Unified Dashboard.
- **Automated Skin Snap Tests**: Added `TestSkinSnapColorTable()` in `tests/test_main.cpp`.

## [1.4.0] - 2026-11-04
### Added
- **Unsnapped Note Diagnostics**: Enhanced `Chart::ValidateChart()` to detect notes with missing or invalid beat snap metadata (`BeatSnap <= 0`), populating diagnostic warning messages and updating the Dashboard integrity report.
- **Automated Unsnapped Note Tests**: Added `TestUnsnappedNoteValidation()` to `tests/test_main.cpp`.

## [1.3.0] - 2026-11-04
### Added
- **Shortcut Registry & Mapping Engine**: Added `ShortcutDefinition` struct, dynamic shortcut lookup, modification, and query formatting in `ShortcutMenuModule` (`RegisterShortcut`, `UpdateShortcut`, `GetShortcutString`).
- **Dashboard Shortcuts Reference**: Integrated quick-launch hotkey reference viewer into the Unified Dashboard.
- **Automated Shortcut Unit Tests**: Added `TestShortcutMappings()` into `tests/test_main.cpp`.

## [1.2.0] - 2026-11-04
### Added
- **Chart Validation & Integrity Engine**: Implemented `Chart::ValidateChart()` and `ChartValidationResult` verifying absence of overlapping notes on identical columns/timestamps, bounds checking key indices against `KeyAmount`, and validating hold/roll begin vs end timestamps.
- **Dashboard Integrity Diagnostics**: Added "Run Chart Integrity Check" action in the Unified Dashboard under Session Management, providing real-time pass/fail status and granular diagnostic error/warning reports.
- **Automated Validation Unit Tests**: Added `TestChartValidation()` to test suite in `tests/test_main.cpp`.

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
