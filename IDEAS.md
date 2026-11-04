# Innovative Concepts & Architectural Ideas

## 1. Real-time Audio Spectrum / Spectrogram View
- Integrate FFT spectrum computation using `BASS_ChannelGetData(..., BASS_DATA_FFT2048)`.
- Render a 2D spectrogram directly behind the vertical scrolling timefield or minimap to visually locate drum transients, synth leads, and vocal plosives.

## 2. Warped Time Rendering for SVs
- Provide an alternate timeline projection mode where the vertical distance between notes corresponds to perceived visual speed (scaled by `ScrollVelocityMultiplier`) rather than strict linear milliseconds.

## 3. WebAssembly / Portable Web Port
- Port core engine (`leraine_lib`) with WebAssembly (`emscripten`) and WebGL / WebAudio to offer zero-install chart editing directly within modern web browsers.

## 4. Collaborative Live Charting
- Networked state synchronization using lightweight delta CRDTs or WebSocket messages to enable simultaneous multi-charter editing sessions.
