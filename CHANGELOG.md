# Changelog

All notable changes to this project will be documented in this file.

## [1.18.0] - 2026-11-04
### Added
- **Audio Waveform Width Scaling**: Added configurable waveform width scale (`WaveformWidthScale`, default 1.0x, range 0.5x-2.0x) in `Configuration` with YAML serialization.
- **Waveform Width Hooks**: Added `GetWidthScale()` and `SetWidthScale(float InScale)` in `WaveFormModule` with bounds clamping.
- **Dashboard Width Scale Slider**: Added interactive `ImGui::SliderFloat` width scale slider to the Unified Dashboard View & Grid Settings panel.
- **Automated Width Scale Tests**: Added `TestWaveformWidthScale()` in `tests/test_main.cpp`.

## [1.17.0] - 2026-11-04
### Added
- **Bulk Column Note Shifting**: Implemented `Chart::ShiftNotesByColumn(int InColumnDelta)` allowing whole-chart notes to be transposed left or right across columns with single-step Undo/Redo history tracking.
- **Dashboard Column Shift Buttons**: Added "Shift Left (-1 Col)" and "Shift Right (+1 Col)" triggers in the Note Editing & Transformations panel of the Unified Dashboard.
- **Automated Column Shifting Tests**: Added `TestChartShiftNotesByColumn()` in `tests/test_main.cpp`.

## [1.16.0] - 2026-11-04
### Added
- **Audio Waveform Inversion Mode**: Added `InvertWaveform` boolean flag to `Configuration` with YAML serialization.
- **Waveform Inversion Hooks**: Added `IsInverted()` and `SetInverted(bool InInvert)` in `WaveFormModule` to flip stereo channels across the timeline axis.
- **Dashboard Invert Checkbox**: Added interactive "Invert Waveform" checkbox toggle to the Unified Dashboard View & Grid Settings panel.
- **Automated Inversion Tests**: Added `TestWaveformInversion()` in `tests/test_main.cpp`.

## [1.15.0] - 2026-11-04
### Added
- **Chart Duration & Playable Range Analytics**: Implemented `Chart::GetFirstNoteTime()`, `Chart::GetLastNoteTime()`, and `Chart::GetPlayableDuration()` tracking exact song range and active playing span.
- **Dashboard Playable Range Readout**: Integrated formatted mm:ss duration and timestamp range into the NPS & Difficulty Analytics panel of the Unified Dashboard.
- **Automated Duration Tests**: Added `TestChartDurationAnalytics()` in `tests/test_main.cpp`.

## [1.14.0] - 2026-11-04
### Added
- **Column Note Distribution Analytics**: Added `Chart::GetColumnNoteDistribution()` calculating total note counts across each individual column lane.
- **Dashboard Column Distribution Histogram**: Integrated real-time column density histogram visualization into the NPS & Difficulty Analytics panel of the Unified Dashboard.
- **Automated Column Distribution Tests**: Added `TestColumnNoteDistribution()` in `tests/test_main.cpp`.

## [1.13.0] - 2026-11-04
### Added
- **Bulk Note Filter Clearing**: Added `Chart::ClearNotesByType(Note::EType InType)` allowing selective removal of specific note types (e.g. mines, lifts, fakes, or holds) across the entire active chart.
- **Dashboard Note Clearing Tool**: Integrated "Clear All Notes of Type" dropdown filter and action button into the Note Editing & Transformations panel of the Unified Dashboard.
- **Automated Clearing Tests**: Added `TestChartClearNotesByType()` in `tests/test_main.cpp`.

## [1.12.0] - 2026-11-04
### Added
- **Waveform Resolution Multiplier**: Added configurable waveform resolution scaling (`WaveformResolutionMultiplier`, default 1.0x, range 0.25x-4.0x) in `Configuration` with YAML serialization.
- **Waveform Module Resolution Hooks**: Added `GetResolutionMultiplier()` and `SetResolutionMultiplier(float InMult)` in `WaveFormModule` with safe bounds clamping.
- **Dashboard Resolution Slider**: Added `ImGui::SliderFloat` waveform resolution multiplier control in the Unified Dashboard View & Grid Settings panel.
- **Automated Resolution Tests**: Added `TestWaveformResolutionMultiplier()` in `tests/test_main.cpp`.

## [1.11.0] - 2026-11-04
### Added
- **Note Count & Distribution Analytics**: Added `ChartNoteStatistics` struct and `Chart::GetNoteStatistics()` calculating counts for total notes, common taps, holds, rolls, mines, lifts, and fakes.
- **Dashboard Note Breakdown Table**: Integrated a granular note distribution breakdown into the NPS & Difficulty Analytics panel of the Unified Dashboard.
- **Automated Note Statistics Tests**: Added `TestChartNoteStatistics()` in `tests/test_main.cpp`.

## [1.10.0] - 2026-11-04
### Added
- **Waveform Alpha Transparency Control**: Added configurable alpha channel (`WaveformAlpha`) in `Configuration` and YAML serializer.
- **Waveform Opacity Hooks**: Added `GetAlpha()` and `SetAlpha(uint8_t InAlpha)` in `WaveFormModule` to modulate audio waveform opacity dynamically.
- **Dashboard Waveform Alpha Slider**: Added `ImGui::SliderInt` opacity slider to the Unified Dashboard View & Grid Settings panel.
- **Automated Waveform Alpha Tests**: Added `TestWaveformAlphaConfiguration()` in `tests/test_main.cpp`.

## [1.9.0] - 2026-11-04
### Added
- **Multi-Chart Difficulty Tab Switcher**: Added difficulty tab bar to the Unified Dashboard under Session Management, allowing seamless hot-switching between multiple difficulties in the current song folder/file.
- **Difficulty Tracking**: Stored `_AvailableDifficulties` and `_CurrentChartFilePath` in `Program` across chart scan and load lifecycle.
- **Automated Multi-Difficulty Tests**: Added `TestMultiDifficultyScan()` in `tests/test_main.cpp`.

## [1.8.0] - 2026-11-04
### Added
- **Waveform Color Customization Engine**: Added configurable RGB waveform color fields (`WaveformColorRed`, `WaveformColorGreen`, `WaveformColorBlue`) to `Configuration` with YAML serialization.
- **Waveform Module Color Hooks**: Implemented `GetFrontColor()`, `SetFrontColor()`, `GetBackColor()`, and `SetBackColor()` in `WaveFormModule`.
- **Dashboard Waveform Color Picker**: Added interactive `ImGui::ColorEdit3` picker to the Unified Dashboard under View & Grid Settings with instant preview and auto-saving.
- **Automated Waveform Color Tests**: Added `TestWaveformColorConfiguration()` in `tests/test_main.cpp`.

## [1.7.0] - 2026-11-04
### Added
- **FFT Audio Spectrum Analysis**: Implemented `AudioModule::GetFFTSpectrum` utilizing `BASS_ChannelGetData` with `BASS_DATA_FFT2048` to sample real-time frequency distribution bins.
- **Dashboard Spectrum Visualizer**: Integrated real-time frequency spectrum histogram in the Unified Dashboard under Playback & Audio Controls.
- **Automated Audio Spectrum Tests**: Added `TestAudioFFTData` unit test in `tests/test_main.cpp`.

## [1.6.0] - 2026-11-04
### Added
- **Waveform Channel Mode Control**: Implemented Stereo vs. Monochannel waveform rendering modes and state inspection in `WaveFormModule` (`IsStereoMode`, `SetStereoMode`, `HasWaveFormData`, `GetSongLength`).
- **Dashboard Waveform Settings**: Added stereo waveform channel toggle in View & Grid Settings inside the Unified Dashboard with `(?)` guidance marker.
- **Automated Waveform Tests**: Added `TestWaveformBounds()` in `tests/test_main.cpp`.

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
