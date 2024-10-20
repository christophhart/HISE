#pragma once

namespace melatonin
{
    class CachedBlur
    {
    public:
        explicit CachedBlur (size_t r);

        // we are passing the source by value here
        // (but it's a value object of sorts since its reference counted)
        void update (const juce::Image& newSource);

        juce::Image& render (juce::Image& newSource);
        juce::Image& render();

    private:
        // juce::Images are value objects, reference counted behind the scenes
        // We want to store a reference to the src so we can compare on render
        // And we actually are the owner of the dst
        size_t radius = 0;
        juce::Image src;
        juce::Image dst;
    };
}