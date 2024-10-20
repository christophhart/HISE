#include "cached_blur.h"
#include "internal/rendered_single_channel_shadow.h"
#include "internal/implementations.h"

namespace melatonin
{

    CachedBlur::CachedBlur (size_t r) : radius (r)
    {
        jassert (radius > 0);
    }

    void CachedBlur::update (const juce::Image& newSource)
    {
        if (newSource != src)
        {
            jassert (newSource.isValid());
            src = newSource;

            // the first time the blur is created, a copy is needed
            // so we are passing correct dimensions, etc to the blur algo
            dst = src.createCopy();
            melatonin::blur::argb (src, dst, radius);
        }
    }

    juce::Image& CachedBlur::render (juce::Image& newSource)
    {
        update (newSource);
        return dst;
    }

    juce::Image& CachedBlur::render()
    {
        // You either need to have called update or rendered with a src!
        jassert (dst.isValid());
        return dst;
    }

}
