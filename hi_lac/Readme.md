# HLAC - HISE Lossless Audio Codec

**A lossless audio codec designed with focus on decompression speed.**

## Comparison with FLAC & PCM

Codec | Ratio | Decompression Speed | Main Compression Technique | Error signal encoding
----- | ----- | ------------------- | -------------------------- | ---------------------
PCM   | 100% | 12500x realtime    | - | -
FLAC  | 25%   | 1300 x realtime     | Linear Predictive Coding   | RICE Encoding
HLAC  | 45% - 70% | 8000x - 12000x realtime | Wavetable Encoding / Linear Gradient Encoding | Fixed bit depth encoding

Unlike FLAC and ALAC and almost every other generic purpose lossless codec around, HLAC uses a much simpler algorithm to compress the audio data.

This results in a lower compression size, but highly improves the decoding speed which makes it a suitable canditate for disk streaming in sample based instruments (which usually use uncompressed data otherwise). Another feature of HLAC is it's *Full Dynamics* mode, which removes quantisation noise on the lower end of the dynamics spectrum which might occur at the tail of heavily processed decaying audio material like cymbal hits / piano samples.

For convenience, there are JUCE format readers available which offer a high level interface to the audio format.

## Features

- suitable for monophonic audio signals like samples
- three compression modes
- 4x - 8x faster decompression than FLAC (50% - 80% of uncompressed PCM read speed)
- 45% - 70% compression ratio (about twice as big as FLAC)
- memory mapped file support
- seekable with no overhead (within 4096 sample frame boundary)
- 16 bit only 
- multithreading capable (no global states)
- GPL v3 license
