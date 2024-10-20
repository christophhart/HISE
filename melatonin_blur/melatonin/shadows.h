#pragma once
#include "internal/cached_shadows.h"

namespace melatonin
{
    /*  A drop shadow is a path filled by a single color and then blurred.
        These shadows are cached.

        Both DropShadow and InnerShadow take the same parameters and should be
        held as a class member of a juce::Component:

        melatonin::DropShadow shadow = {{ juce::Colours::black, 8, { -2, 0 }, 2 }};

        ShadowParameters is a struct that looks like this:

        struct ShadowParameters
        {
            // one single color per shadow
            const juce::Colour color = {};
            const int radius = 1;
            const juce::Point<int> offset = { 0, 0 };

            // Spread literally just expands or contracts the *path* size
            // Inverted for inner shadows
            const int spread = 0;

            // by default we match the main graphics context's scaling factor
            bool lowQuality = false;
        }
    */
    class DropShadow : public internal::CachedShadows
    {
    public:
        DropShadow() : DropShadow (juce::Colours::black, 0, { 0, 0 }, 0) {}

        // individual arguments
        DropShadow (juce::Colour color, int radius, juce::Point<int> offset = { 0, 0 }, int spread = 0)
            : CachedShadows ({ { color, radius, offset, spread } }) {}

        // single ShadowParameters
        // melatonin::DropShadow ({Colour::fromRGBA (255, 183, 43, 111), pulse (6)}).render (g, button);
        explicit DropShadow (ShadowParameters p) : CachedShadows ({ p }) {}

        // multiple ShadowParameters
        DropShadow (std::initializer_list<ShadowParameters> p) : CachedShadows (p) {}
    };

    // An inner shadow is basically the *inverted* filled path, blurred and clipped to the path
    // so the blur is only visible *inside* the path.
    class InnerShadow : public internal::CachedShadows
    {
    public:
        InnerShadow() : InnerShadow (juce::Colours::black, 0, { 0, 0 }, 0) {}

        // multiple shadows
        InnerShadow (std::initializer_list<ShadowParameters> p) : CachedShadows (p, true) {}

        // single initializer list
        explicit InnerShadow (ShadowParameters p) : CachedShadows ({ p }, true) {}

        // individual arguments
        InnerShadow (juce::Colour color, int radius, juce::Point<int> offset = { 0, 0 }, int spread = 0)
            : CachedShadows ({ { color, radius, offset, spread } }, true) {}
    };

    // Renders a collection of inner and drop shadows plus a path
    class PathWithShadows : public internal::CachedShadows
    {
        // multiple shadows
        PathWithShadows (std::initializer_list<ShadowParameters> p) : CachedShadows (p) {}

        // single ShadowParameters
        // melatonin::DropShadow ({Colour::fromRGBA (255, 183, 43, 111), pulse (6)}).render (g, button);
        explicit PathWithShadows (ShadowParameters p) : CachedShadows ({ p }) {}

        // individual arguments
        PathWithShadows (juce::Colour color, int radius, juce::Point<int> offset = { 0, 0 }, int spread = 0)
            : CachedShadows ({ { color, radius, offset, spread } }) {}
    };
}
