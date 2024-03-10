![Figma - 2023-11-09 42@2x](https://github.com/sudara/melatonin_blur/assets/472/0cb16190-bce7-4d9a-8a7c-d15846946354)

![](https://github.com/sudara/melatonin_blur/actions/workflows/tests.yml/badge.svg)

Melatonin Blur is a batteries-included, cross-platform CPU blur and shadow compositing library for
the [JUCE C++ framework](https://juce.com/).

*Batteries-included* means it aims to give you everything out of the box:

* 👩‍🎨 Figma/CSS-accurate drop and inner shadows on paths!
* 🔠 Drop and Inner Text shadows!
* 💅🏼 Supports both filled and stroked paths!
* 🌇 ARGB image blurs!
* 🚀 Fast! (see [benchmarks](#more-benchmarks)).
* 🔎 Retina-friendly (context scale-aware)!
* 🍰 Trivial to layer multiple shadows!
* ⚙️ Behind-the-scenes multi-layer caching!
* 😎 Debug optimized for high quality of life!
* 🤖 Over 1000 correctness tests passing on mac/windows!
* 🚂 Compatible down to macOS 10.13 (progressive speedups on recent versions)

The goal: modern vector interfaces in JUCE (100s of shadows) without resorting to deprecated solutions with lower quality of life (looking at you, OpenGL on macOS!).

https://github.com/sudara/melatonin_blur/assets/472/3e1d6c9a-aab9-422f-a262-6b63cbca5b71

Melatonin Blur provides a 10-30x speedup over using Stack Blur alone.

<img src="https://github.com/sudara/melatonin_blur/assets/472/598115b8-9c9d-42d8-b868-e921978cda17" width="550" />

On macOS, it depends on the built-in Accelerate framework.

On Windows, it *optionally* depends on the Intel IPP library. If IPP is not present, it will fall back to a JUCE FloatVectorOperations implementation for single channel (shadows, etc) and Gin's Stack Blur for ARGB.

Interested in how the blurring
works? [I wrote an in-depth article about re-implementing Stack Blur 15+ times](https://melatonin.dev/blog/implementing-marios-stack-blur-15-times-in-cpp/).

## Installation

Melatonin Blur is a JUCE Module.

If you are new to JUCE modules, don't be scared! They are easy to set up.

### CMake option 1: Submodules

```git
git submodule add -b main https://github.com/sudara/melatonin_blur.git modules/melatonin_blur

# To update down the road:
# git submodule update --remote --merge modules/melatonin_blur
```

### CMake option 2: FetchContent

```cmake
Include(FetchContent)
FetchContent_Declare(melatonin_blur
    GIT_REPOSITORY https://github.com/sudara/melatonin_blur.git
    GIT_TAG origin/main
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_blur)
FetchContent_MakeAvailable(melatonin_blur)
```

### Tell CMake about the module

Add  *before* your `juce_add_plugin` call:

```cmake
juce_add_module("modules/melatonin_blur")

```

### Link your plugin target

Link *after* the `juce_add_plugin` call:

```cmake
target_link_libraries("YourProject" PRIVATE melatonin_blur)

```

### Via Projucer

Download (via git like above, or via the UI here) and "Add a module from a specified folder" and you're done!

<p align="center">
<img src="https://github.com/sudara/melatonin_inspector/assets/472/010d9bf3-f8dc-4fc1-9039-69ba42ff856c" width="500"/>
</p>

### Don't forget to include the header!

```cpp
#include <melatonin_blur/melatonin_blur.h>
```

### IPP on Windows

If you aren't already using it, Intel IPP might feel like an annoying dependency. Understandable! [I wrote a blog post describing how to set it up locally and in CI](https://melatonin.dev/blog/using-intel-performance-primitives-ipp-with-juce-and-cmake/).

It's not too bad! It's fantastic tool to have for dsp as well (albeit with an annoying API!) and it'll speed up the single channel (shadows, etc) on Windows. And don't worry, without IPP you'll still get Stack Blur performance + shadow caching.

## Usage

### Drop Shadows

Drop shadows work on a `juce::Path`.

Add a `melatonin::DropShadow` as a member of your `juce::Component`, specifying the blur radius like so:

```cpp
melatonin::DropShadow shadow = { juce::Colours::black, 8 };
```

In `paint` of your component, call `shadow.render(g, path)`, passing in the graphics context and the path to render. 

**Remember to render drop shadows *before* rendering the path!**

### Color, Radius, Offset, Spread

Melatonin Blur comes with a test suite that verifies compatibility with design programs like Figma and CSS. It's not pixel-to-pixel accurate (Figma and Adobe XD both seem to use cheaper blurs than CSS), but the feature set should be 1:1.

You can specify the color, radius, offset and spread of each blur, passing them in a struct like so:

```cpp
struct ShadowParameters
{
    // one single color per shadow
    const juce::Colour color = {};
    const int radius = 1;
    const juce::Point<int> offset = { 0, 0 };

    // Spread literally just expands or contracts the path size
    // Inverted for inner shadows
    const int spread = 0;
}
```

Or just as parameters, like so:

```cpp
melatonin::DropShadow shadow = { juce::Colours::black, 8, { 4,4 }, 10 };
```

### Full Drop Shadow Example

```cpp

class MySlider : public juce::Component
{
public:
    
    void paint (juce::Graphics& g) override
    {
        // drop shadows get painted *before* the path
        shadow.render (g, valueTrack);
        
        g.setColour (juce::Colours::red);
        path.fillPath (valueTrack);
    }
    
    void resized()
    {
        valueTrack.clear();
        valueTrack.addRoundedRectangle (10, 10, 100, 20, 2);
    }
    
private:
    juce::Path valueTrack;
    melatonin::DropShadow shadow = { juce::Colours::black, 8, { -2, 0 } };
}
```

The `juce::Path` itself doesn't *have* to be a member variable to still take advantage of the caching. The path is passed in on `render` (instead of on construction). This frees you up to recalculate the path in `paint` (i.e. there are times when `resized` won't be called, such as when animating), while still retaining the cached shadow when the data is identical:

```cpp
class MySlider : public juce::Component
{
public:
    
    void paint (juce::Graphics& g) override
    {
        juce::Path valueTrack;
        valueTrack.addRoundedRectangle (10, 10, 100, 20, 2);
        
        shadow.render (g, valueTrack);
        
        g.setColour (juce::Colours::red);
        path.fillPath (valueTrack);
    }
    
private:
    melatonin::DropShadow shadow = {{ juce::Colours::black, 8, { -2, 0 } }};
}

```

### Inner Shadows

Inner shadows function identically and have the same API as drop shadows. Just call `melatonin::InnerShadow`.

**Remember, inner shadows are rendered *after* the path is rendered**.

```cpp
class MySlider : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colours::red);
        path.fillPath (valueTrack);
        
        // inner shadows get painted *after* the path
        innerShadow.render (g, valueTrack);
    }
    
    void resized()
    {
        valueTrack.clear();
        valueTrack.addRoundedRectangle (10, 10, 100, 20, 2);
    }
private:
        melatonin::InnerShadow innerShadow = { juce::Colours::black, 3 };
}
```

### Multiple Shadows

You can easily stack shadows by feeding `melatonin::DropShadow` or `melatonin::InnerShadow` multiple sets of parameters.

```cpp
melatonin::DropShadow thumbShadow {
    { juce::Colours::black, 16, { 0, 0 } },
    { juce::Colours::gray, 8, { 0, 0 } },
    { juce::Colours::blue, 3, { 0, 0 } }};
```

Shadows are rendered in the order they are specified.

### Updating and Animating Shadows

Starting with version 1.2, caching of the underlying single channel blurs is position, color, offset and opacity agnostic.

Translation: you can animate these things at 60fps and it's very cheap. Only some image compositing, no blur recalculation.

This is great for `juce::Path`s that look the same each paint but move around (such as slider thumbs).

```cpp
// these are all "free" operations that use cached single channel blurs
path.translate(10, 0);
shadow.setColor(juce::Colours::red);
shadow.setOffset({ 10, 0 });
shadow.setOpacity(0.5f);

// these are "expensive", requiring a recalculation of the underlying blur
shadow.setRadius(10);
shadow.setSpread(10);
```

### Default Constructors for Inner/DropShadow

Don't know your colors at compile time?

That's fine, there's a default constructor, you can update the color in the paint method.

### Stroked Paths

Stroked paths are supported for both inner and drop shadows.

Instead of calling `shadow.render(g, path)`, you'll need to call `shadow.render(g, path, pathStrokeType)`, passing in the same stroke type as you used to stroke the path.

### Text shadows

![AudioPluginHost - 2023-12-23 45@2x](https://github.com/sudara/melatonin_blur/assets/472/858e305f-f223-4df7-ad72-ace7e69865aa)

Text shadows use the same `melatonin::DropShadow` and `melatonin::InnerShadow` classes as paths:

```cpp
class MySlider : public juce::Component
{
public:
    
    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colours::black);
        g.setFont (juce::Font (20));
        shadow.render (g, "Hello World", getLocalBounds(), juce::Justification::centred);
        g.drawText ("Hello World", getLocalBounds(), juce::Justification::centred);
    }
    
private:
    melatonin::DropShadow shadow = {{ juce::Colours::red, 8, { -2, 0 } }};
}
```

Right now, the API just mirrors `g.drawText`, so it's not particularly DRY. I'm open to suggestions on how to improve this. Ellipses aren't supported, but you can open a PR. I will never support `drawFittedText` — it's downright evil and should be deprecated and removed from JUCE :)

Just like `g.drawText`, you can pass in a `juce::Rectangle<int>`, `juce::Rectangle<float>`, or the `x, y, w, h` as bounds arguments.

Text shadows are cached. As with path shadows, repainting is cheap and changing their color, offset, opacity is free. However, changing the font, text, justification or bounds will require re-rendering (both the glyphs and underlying blur).

### Full Color Blurs

These are still "expensive" (up to a few ms) for larger images on first render, but caching makes them trivial to re-render.

Just add a `melatonin::CachedBlur` member to your component, specifying the radius:

```cpp 
melatonin::CachedBlur blur { 48 };
```

In your paint call, you can use `blur.render(mySourceImage)` as a juce::Image, like so:

```
g.drawImageAt (blur.render (mySourceImage), 0, 0);
```

or something fancier like:

```cpp
g.drawImageTransformed (blur.render (mySourceImage), backgroundTransform);
```

Alternatively, if you have specific times you are updating the blur (such as capturing a screenshot when someone clicks to open a modal window), you can call `blur.update` with the new image:

```cpp
void updateBackgroundBlur()
{
    blur.update (getParentComponent()->createComponentSnapshot (getContentBoundsInParent()));
}
```

You can then call `render()` without needing to pass an image argument:

```cpp
g.drawImageAt (blur.render(), 0, 0);
```

I've got plans to add some more background blur helpers for these use cases!

## Motivation

Blurs are essential to modern design. Layered drop shadows, frosted glass effects, blurred backgrounds — you won't see a nice looking app in the 2020s without them.

Designers tend to work in vector based tools such as Figma. Shadows are a big part of their workflow. It's how they bring depth and life to 2D interfaces. Melatonin Blur lets you take a designer's work in CSS/Figma and quickly translate it. No need to export image strips and so on like it's the 1990s!

For example, I have a slider that look like this:

![Figma - 2023-11-09 05](https://github.com/sudara/melatonin_blur/assets/472/1b84cad0-6044-444a-a2bd-ac8d33142eb9)

Every single part of this slider is a vector path with shadows. The background track (2 inner), to the level indicator (3 inner, 2 drop) to the knob (3 drop shadow). They all need to be rendered fast enough that many of these can be happily animated at 60fps without freezing up the UI on older machines.

Stack Blur [via the Gin implementation](https://github.com/FigBug/Gin) first made this feel technically possible for me. Thanks Roland!

However, performance is death by 1000 cuts. I was still finding myself building little caching helpers. Sometimes these shadows add up, and I didn't feel "safe", in particular on Windows. On larger images (above 500px in a dimension) Stack Blur can takes milliseconds of CPU time (which is unacceptable for responsive UI targeting 60fps). I was also seeing some sluggishness in Debug mode, which drives me crazy!

So I started to get curious about the Stack Blur algorithm. I kept thinking I could make it:

* **Faster**. The original stack blur algorithm was made for an environment without access to SIMD or intel/apple vendor libs, in 2004. For larger images, I was seeing images take `ms`. I wanted to see them in the `µs`. In Debug, Stack Blur is sluggish and can't always provide 60fps, even on a fast machine.
* **Cleaner**. The Gin Stack Blur implementation comes from a long line of ports originating
  with [Mario's js implementation](https://web.archive.org/web/20110707030516/http://www.quasimondo.com/StackBlurForCanvas/StackBlur.js).
  That means there's no templates, no code reuse, there's multiplication and left shift tables to avoid division, and all kinds of trickery that I felt was needed with modern C++ and modern compilers.
* **Understandable**. The concept of the "stack" in Stack Blur — what is it? I had a hard time finding resources that made it easy to understand. The original notes on the algorithm don't align with how implementations worked in
  practice. So I was interested in understanding how the algorithm works.
* **Fully Tested**. This is critical when iteratively re-implementing algorithms, and I felt like it was a must-have.
* **Benchmarked**. The only way to effectively compare implementations was to test on Windows and MacOS.
* **Batteries included**.

Thanks to being arrogant and setting a somewhat ridiculously high bar, I implemented Stack Blur probably 25+ times over the course of a few weeks. Enough where I can do it in my sleep. There are 15 reference implementations in this repo that pass the tests, but most didn't bring the performance improvements I was looking for. I still have a few more  implementations that I'd like to try, but I've already invested ridiculous amounts of time into what I've been calling my *C++ performance final exam* — in short, I would like to move on with my life!

## More Benchmarks

Benchmarks are REALLY messy things.

* It's easy to make something *seem* like it performs 10x faster than an alternative by toying with inputs and cherry-picking results.
* It's easy to get motivated by one big number (like a 5-10x speedup on one input) and imagine that it's possible on all inputs — especially with C++ compilers, it's rarely consistently predictable.
* Initial "wow!" level improvements tend to degrade as you get closer to real world use cases (due to things like cache locality etc)
* Even with a high `n`, Benchmarks are noisy and can vary per run (musn't run other CPU intensive things while running them).
* Using benchmark averages obscure outliers (and outliers matter, especially in dsp, but even in UI).
* Results differ on different machines. (I swear a fresh restart of my Apple M1 made vImage run faster!)

So, here are some cherry-picked benchmarks. The Windows machine is an AMD Ryzen 9 and the mac is a M1 MacBook Pro. In all cases, the image dimensions are square (e.g. 50x50px) and the times are `µs` (microseconds, or a millionth of a second) averaged over 100 runs. That means that when you see a number like 1000, it means 1ms. Please open issues if you are seeing discrepancies or want to contribute to the benchmarks.

### Cached Drop Shadows

My #1 performance goal with this library was for drop-shadows to be screaming fast.

99% of the time I'm rendering single channel shadows for vector UI.

Caching does most of the heavy lifting here, giving a 10-30x improvement over using just StackBlur:

<img src="https://github.com/sudara/melatonin_blur/assets/472/598115b8-9c9d-42d8-b868-e921978cda17" width="550" />

On Windows, with IPP as a dependency:

<img src="https://github.com/sudara/melatonin_blur/assets/472/d660ef4c-8807-4c4d-b9eb-cfb5a28655bd" width="750" />

Note: I haven't been including JUCE's DropShadow class. That's in part because it's not compatible with design programs like Figma or standards like CSS. But it also performs 20-30x worse than Stack Blur and up to 500x worse than Melatonin Blur.

To show this clearly, the time axis (in `µs`) has to be logarithmic:
<img src="https://github.com/sudara/melatonin_blur/assets/472/74353ab2-9eeb-40c1-a1df-1e0dbc26fdd0" width="550" />

### Single Channel Blurs (Uncached Shadows)

My #2 performance goal was for single channel blurs underlying shadows themselves to take `µs`, not `ms`. You can also think of these as the timings for the *first* time a shadow is built with a blur. Optimizing these larger image sizes ensures that drop shadows won't be a cause for dropped frames on their first render (and can even be animated).

Stack Blur (and in particular the Gin implementation) is already *very* optimized, especially for smaller dimensions. It's hard to beat the raw blur performance on smaller images like a 32x32px (although caching the blur is still very much worth it). However, as image dimensions scale, Stack Blur gets into the `ms`, even on single channels.

Melatonin Blur stays under `1ms` for the initial render of most realistic image sizes and radii.

<img src="https://github.com/sudara/melatonin_blur/assets/472/56b4c60c-835d-412c-bcb9-df0431247b46" width="750" />

On Windows (with IPP):

<img src="https://github.com/sudara/melatonin_blur/assets/472/ce0dc3c7-3d30-413e-af3a-77b741c6c1fe" width="750" />

### Optimized for Debug Too

Debug is where we spend 95% of our day! Nothing worse than clicking around a janky low FPS UI, uncertain of how it will perform in Release.

Because it directly talks to vendor vector libraries and the caching is still in play, Melatonin Blur is *almost* as fast in Debug as it is in Release. Individual drop shadows are up to 30x-50x faster than a Debug Stack Blur, and timings will stay in µs, not ms. The following chart is again on a logarithmic scale:

<img src="https://github.com/sudara/melatonin_blur/assets/472/9e6e0551-d6ca-4df6-a842-de09a9a6f5f3" width="550" />

### Full Color Blurs (ARGB)

ARGB blurs are often used to blur a whole window or a big part of the screen.

The blur itself usually has to be a good 32px or 48px something to look nice. The image dimensions are large. The radii are large. And there are 4 channels. This is rough on performance.

Initial Melatonin ARGB blurs usually break into the `ms` once image dimensions go above 500x500 with radii above 16. However, caching gives up to a 30x speedup on repaint:

<img src="https://github.com/sudara/melatonin_blur/assets/472/b520cd39-5cbb-4d1d-b69f-8c66d80acd89" width="750" />

ARGB on Windows just about killed me. I tried many implementations,
and [still have a few left to try](https://github.com/sudara/melatonin_blur/issues/2). Nothing consistenly outperforms
Gin (vImage can be 2x on larger images but suffers on larger radii), so Gin is being used as the blur implementation backing Windows ARGB.

What you need to know:

* Debug will also be fast thanks to caching.
* You won't be able to animate RGBA blurs.
* Very large blurs (over 1000x1000px with large radii) may cause UI sluggishness. Please measure!

## FAQ

### Shouldn't this kind of thing be done on the GPU?

Yes.

### So uhh... why did you spend your life on this.

Don't ask.

### Seriously, why?

In JUCE, graphics options are limited, as it's a cross-platform C++ framework. Yes, you could spin up an OpenGL context (deprecated and crusty on MacOS), but you lose a lot of conveniences working with the normal JUCE rendering.

JUCE + Stack Blur got me very close to having what I needed to happily make modern plugin UIs. This library was the last piece of the puzzle.

### What's tested?

Tests were necessary to verify implementations were correct. Horizontal and vertical blur passes of Stack Blur are
tested in isolation. Most of the implementations in this repository pass the tests.

<img src="https://github.com/sudara/melatonin_blur/assets/472/b1490023-d9c9-440e-a6b0-d814e548b4b8" width="300" />

### How can I run the benchmarks on my machine

It could be fun to codesign and release the benchmark binaries... In general I sort of daydream about JUCE hiring me to release a benchmark plugin utility for JUCE that compares dsp and UI performance across different machines (and reports to the cloud so we can all benefit from results).

## Interesting facts learned along the way

Things I learned the hard way:

### There's no such thing as a native RGB packed image on macOS, it's all RGBA

[see this forum thread](https://forum.juce.com/t/ask-for-rgb-get-back-argb/9265).

This basically just means: under the hood, images are always 32 bytes per pixel, even if you only are using 24.

(And there's no need for an RGB blur on macOS, it's all ARGB).

### A JUCE `Image` is stored premultiplied

I ran into one gotcha when first testing the RGB(A) version of the algorithm, making sure that each channel was behaving correctly if the other channels were all at 0 and it was at 255. I couldn't get the darn tests to pass!

It turns out that JUCE, like most frameworks handling image compositing, pre-multiplies the alpha when it stores a pixel.

In other words, if the 8-bit red channel is at max, at 255, but the alpha is at a third, at 85, the red component of the pixel is actually stored pre-multiplied by the alpha value. As 85, not 255.

The alpha information is kept in the pixel, and when you ask JUCE for the color, it will unpremultiplied it so you see "red" as 255 and alpha at 85.

This is done both for functional and performance reasons, you can [read more here](https://learn.microsoft.com/en-us/windows/apps/develop/win2d/premultiplied-alpha) and find some [interesting stuff on the JUCE forum](https://forum.juce.com/t/softwareimagerenderer-produces-premultiplied-argb/8991) about it.

### Actual pixels are stored as `BGRA` in memory

All modern consumer MacOS and Windows machines are little endian. After banging my head for a couple hours on strangely failing tests, I realized JUCE (and most platforms it abstracts by) stores pixels
in [BGRA order in memory for these machines](https://github.com/juce-framework/JUCE/blob/a7d1e61a5511875dc8f345816fca94042ce074fb/modules/juce_graphics/native/juce_CoreGraphicsContext_mac.mm#L165).

Funny, because on the web, we think in terms of `RGBA`.
On desktop, we think in terms of `ARGB`.
But the computers think in `BGRA`.

### Vendor and Vector implementations

Originally I split work between "naive" (pixel by pixel) and vector implementations of Mario's Stack Blur algorithm. I learned some interesting things via the vector implementation attempts.

Image vector algos seem to need to be converted to floating point to efficiently work with vector operations on Intel and Mac. There are sometimes 8-bit functions available, but kernels have to be at least 16-bit (to calculate sums or to perform convolution). So in bulk, there's always some need for conversion.

I tried batching the vector operations (aka, entire rows or cols of pixels) to be 32/64/100, thinking this would be a more efficient for larger images. It's not. It's slower. My best guess is vdsp/ipp under the hood juggle data and memory better than I can manually. However, the fallback implementation could still benefit from this method.

Explicit Alignment for vector implementation. Any attempts for me to do this seemed to worsen performance on Apple ARM! This is the second time I've run into this on ARM, I will have to investigate further.

It was definitely harder to get classes to perform as well as their free function counterparts. For non-vector implementations, generalizing pixel access via function pointers performs worse than using `if constexpr` in the templated function.

Vector implementations reuse vectors based on the largest dimension size. Meaning, it won't reallocate before the vertical pass. This was always faster than allocating twice, despite the vector sizes being unideal for one of the passes.

### Ugly verbose C++ code (like the original stack blur implementation) can unfortunately still outperform modern C++ code

I started out with everything in one big ole free function. 0 code reuse, as a tribute to the original.

This might seem like a strange thing to say, or even a no-brainer to a seasoned C++ developer, but I was surprised by how much of a negative impact things like templates, classes, and function calls regularly had on performance in these implementations.

In reality, I think the truth is more subtle: compilers have a lot of history with C, and are very good at optimizing a linear mess of instructions. There are more considerations when moving to templates and classes (such as memory layout, the resulting impact on cache locality, "seeing through" the indirection created by classes and function calls). The compiler can't always make the same straightforward assumptions, if only because there is more complexity for the compiler itself. My conclusion is that the human has to work *harder* at optimizing modern C++ code than with naive "risky" pointer juggling C-ish implementations.

In other words, this project humbled me. I love to advocate for clarity, readability and maintainability. I tend to despise spaghetti code and esoteric C++ tricks. In the rare times when the absolute priority is raw speed, I still 100% believe readability and maintainability are possible, but there's definitely a price to be paid for composability and reusability. I paid that price, both in terms of raw hours, as well as an end result I wish I could be prouder of.

## Acknowledgements

* Mars, for being my reliable rubber duck! Inner Shadow caching and compositing geometry broke my brain.
* Roland Rabien for JUCE Stack Blur workhorse via [Gin](https://github.com/figbug/gin).
* LukeM1 on the forums
  for [figuring out the `drawImageAt` optimization](https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76).
* Ecstasy on the Discord for the motivation and feedback around stroked paths and default constructors.
