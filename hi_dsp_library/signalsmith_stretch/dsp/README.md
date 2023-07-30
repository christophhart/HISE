# Signalsmith Audio's DSP Library

A C++11 header-only library, providing classes/templates for (mostly audio) signal-processing tasks.

More detail is in the [main project page](https://signalsmith-audio.co.uk/code/dsp/), and the [Doxygen docs](https://signalsmith-audio.co.uk/code/dsp/html/modules.html).

## Basic use

```
git clone https://signalsmith-audio.co.uk/code/dsp.git
```

Just include the header file(s) you need, and start using classes:

```cpp
#include "dsp/delay.h"

using Delay = signalsmith::delay::Delay<float>;
Delay delayLine(1024);
```

You can add a compile-time version-check to make sure you have a compatible version of the library:
```cpp
#include "dsp/envelopes.h"
SIGNALSMITH_DSP_VERSION_CHECK(1, 3, 3)
```

### Development / contributing

Tests (and source-scripts for the above docs) are available in a separate repo:

```
git clone https://signalsmith-audio.co.uk/code/dsp-doc.git
```

The goal (where possible) is to measure/test the actual audio characteristics of the tools (e.g. frequency responses and aliasing levels).

### License

This code is [MIT licensed](LICENSE.txt).  If you'd prefer something else, get in touch.
