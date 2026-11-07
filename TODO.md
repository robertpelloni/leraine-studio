# Granular Task List (TODO)

## Immediate & Short-Term Tasks
- [x] Consolidate editor tools into a singular unified dashboard (`Program::DisplayDashboard`).
- [x] Fix compilation issues and duplicate enum definitions in `chart.h`.
- [x] Add explanatory tooltips `(?)` across all dashboard options and tools.
- [x] Provide clean quit and session management hooks in UI.
- [x] Implement Chart Validation & Integrity Engine (`Chart::ValidateChart()`) with UI diagnostics report in dashboard.
- [x] Add automated unit tests for chart integrity checks (`TestChartValidation`).
- [x] Add dynamic shortcut configuration mapping and test suite (`ShortcutMenuModule`, `TestShortcutMappings`).
- [x] Add export validation warnings for unsnapped notes (`TestUnsnappedNoteValidation`).
- [x] Implement user skin selector and query helpers in dashboard (`Skin::GetLoadedKeyAmount`, `Skin::GetSnapColor`, `TestSkinSnapColorTable`).
- [x] Implement stereo vs monochannel waveform visualization mode and unit test (`TestWaveformBounds`).
- [x] Add FFT-based audio spectrum analysis in `AudioModule` and real-time dashboard visualizer (`TestAudioFFTData`).
- [x] Provide user control over waveform color theme in configuration (`TestWaveformColorConfiguration`).
- [x] Implement multi-chart difficulty tab switcher in Unified Dashboard (`TestMultiDifficultyScan`).
- [x] Implement waveform alpha transparency control in Configuration and Dashboard (`TestWaveformAlphaConfiguration`).
- [x] Implement note count and distribution breakdown analytics (`TestChartNoteStatistics`).
- [x] Implement waveform resolution multiplier in Configuration and Dashboard (`TestWaveformResolutionMultiplier`).
- [x] Implement bulk note filter clearing by note type (`TestChartClearNotesByType`).
- [ ] Implement FFT spectrogram timeline visualization in `WaveFormModule`.
