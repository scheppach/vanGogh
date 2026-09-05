# Van Gogh

Van Gogh is a Windows VST3 waveshaping and distortion plug-in. Users can draw a transfer curve and apply it to incoming audio.

## Features

- Editable transfer curve with movable control points
- Point insertion, selection, dragging, and deletion
- Linear reset, symmetrization, random, and smooth-random curve generation
- Input and output gain controls
- Optional 20 Hz DC-blocking filter
- Mono and stereo processing
- 4x internal oversampling
- DAW project-state persistence
- Human-readable JSON presets using the `.drawdist` extension

## Requirements

- Windows x64
- A VST3-compatible host
- JUCE 9 for building from source
- C++17-compatible compiler
- CMake

The distributed binary targets Windows x64 VST3 hosts. Development and testing have been performed with Ableton Live on Windows.

## Installation

1. Download the Windows x64 ZIP from the repository's **Releases** page.
2. Extract `Van Gogh.vst3` to a VST3 directory, such as:

   `C:\Program Files\Common Files\VST3\`

3. Rescan plug-ins in the host application.
4. Add **Van Gogh** to an audio track.

## Building from source

The current CMake configuration expects the JUCE source tree at:

```text
third_party/JUCE/
```

JUCE is not included in this repository. Obtain it separately and place it at that location before configuring the project.

From the project root:

```powershell
cmake -B build -S .
cmake --build build --config Release --parallel
```

The generated plug-in is written to:

```text
build/VanGogh_artefacts/Release/VST3/Van Gogh.vst3
```

Use a `Debug` configuration for development if required. Use `Release` for distribution.

## Presets and state

Example presets are provided in [`presets/`](presets/). Presets can also be saved and loaded from the plug-in interface.

A `.drawdist` preset stores:

- The transfer curve
- Input gain
- Output gain
- DC-filter state

The temporary bypass parameter is not part of the preset format and is forced off. The same usable state is stored when a DAW saves its project.

## Limitations

- Windows VST3 only
- Fixed 900 × 650 pixel editor
- No undo/redo
- No built-in preset browser
- Rapid, large curve changes may produce a brief transient while the audio lookup table is updated

## Project status

This is an early first version intended for experimentation and feedback. Interfaces and preset formats may change.

## Licence

The project source code is released under the [MIT License](LICENSE).

JUCE is distributed under its own licence terms. See the JUCE distribution for applicable conditions.
