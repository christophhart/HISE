#include "rendered_single_channel_shadow.h"
#include "implementations.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin::internal
{
    RenderedSingleChannelShadow::RenderedSingleChannelShadow (ShadowParameters p) : parameters (p) {}

    juce::Image& RenderedSingleChannelShadow::render (juce::Path& originAgnosticPath, float scale, bool stroked)
    {
        jassert (scale > 0);
        scaledPathBounds = (originAgnosticPath.getBounds() * scale).getSmallestIntegerContainer();
        updateScaledShadowBounds (scale);

        // explicitly support 0 radius shadows and edge spread cases
        if (parameters.radius < 1 || scaledShadowBounds.isEmpty())
            singleChannelRender = juce::Image();

        // We can't modify our original path as it would break cache.
        // Remember, the origin of the path will always be 0,0
        auto shadowPath = juce::Path (originAgnosticPath);

        if (!stroked && parameters.spread != 0)
        {
            // expand the actual path itself
            // note: this is 1x, it'll be upscaled as needed by fillPath
            auto bounds = originAgnosticPath.getBounds().expanded (parameters.inner ? (float) -parameters.spread : (float) parameters.spread);
            shadowPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false);
        }

        // inner shadows are rendered by inverting the path, drop shadowing and clipping to the original path
        if (parameters.inner)
        {
            shadowPath.setUsingNonZeroWinding (false);

            // The outside of our path will be filled with shadow color
            // which will then cast a blurred shadow inside the path's area.
            // We need a radius amount of pixels for a strong shadow at the path's edge
            shadowPath.addRectangle (shadowPath.getBounds().expanded ((float) scaledRadius));
        }

        // each shadow is its own single channel image associated with a color
        juce::Image renderedSingleChannel (juce::Image::SingleChannel, scaledShadowBounds.getWidth(), scaledShadowBounds.getHeight(), true);

        // boot up a graphics context to give us access to fillPath, etc
        juce::Graphics g2 (renderedSingleChannel);

        // ensure we're working at the correct scale
        g2.addTransform (juce::AffineTransform::scale (scale));

        // cache at full opacity (later composited with the correct color/opacity)
        g2.setColour (juce::Colours::white);

        // we're still working @1x until fillPath happens
        // blurContextBounds x/y is negative (relative to path @ 0,0) and we must render in positive space
        // Note that offset isn't used here,
        auto unscaledPosition = -scaledShadowBounds.getPosition().toFloat() / scale;

        g2.fillPath (shadowPath, juce::AffineTransform::translation (unscaledPosition));

        // perform the blur with the fastest algorithm available
        melatonin::blur::singleChannel (renderedSingleChannel, (size_t) scaledRadius);

        singleChannelRender = renderedSingleChannel;
        return singleChannelRender;
    }

    // Offset is added on the fly, it's not actually a part of the render
    // and can change without invalidating cache
    juce::Rectangle<int> RenderedSingleChannelShadow::getScaledBounds()
    {
        return scaledShadowBounds + scaledOffset;
    }

    juce::Rectangle<int> RenderedSingleChannelShadow::getScaledPathBounds()
    {
        return scaledPathBounds;
    }

    const juce::Image& RenderedSingleChannelShadow::getImage()
    {
        return singleChannelRender;
    }

    bool RenderedSingleChannelShadow::updateRadius (int radius)
    {
        if (juce::approximatelyEqual (radius, parameters.radius))
            return false;

        parameters.radius = radius;
        return true;
    }

    bool RenderedSingleChannelShadow::updateSpread (int spread)
    {
        if (juce::approximatelyEqual (spread, parameters.spread))
            return false;

        parameters.spread = spread;
        return true;
    }

    bool RenderedSingleChannelShadow::updateOffset (juce::Point<int> offset, float scale)
    {
        if (offset == parameters.offset)
            return false;

        parameters.offset = offset;
        scaledOffset = (parameters.offset * scale).roundToInt();
        return true;
    }

    bool RenderedSingleChannelShadow::updateColor (juce::Colour color)
    {
        if (color == parameters.color)
            return false;

        parameters.color = color;
        return true;
    }

    bool RenderedSingleChannelShadow::updateOpacity (float opacity)
    {
        if (juce::approximatelyEqual (opacity, parameters.color.getFloatAlpha()))
            return false;

        parameters.color = parameters.color.withAlpha (opacity);
        return true;
    }

    // this doesn't re-render, just re-calculates position stuff
    void RenderedSingleChannelShadow::updateScaledShadowBounds (float scale)
    {
        // By default, match the main graphics context's scaling factor.
        // This lets us render retina / high quality shadows.
        // We can only use an integer numbers for blurring (hence the rounding)
        scaledSpread = juce::roundToInt ((float) parameters.spread * scale);
        scaledRadius = juce::roundToInt ((float) parameters.radius * scale);
        scaledOffset = (parameters.offset * scale).roundToInt();

        // account for our scaled radius and spread
        // one might think that inner shadows don't need to expand with radius
        // since they are clipped to path bounds
        // however, when there's offset, and we are making position-agnostic shadows!
        if (parameters.inner)
        {
            scaledShadowBounds = scaledPathBounds.expanded (scaledRadius - scaledSpread, scaledRadius - scaledSpread);
        }
        else
            scaledShadowBounds = scaledPathBounds.expanded (scaledRadius + scaledSpread, scaledRadius + scaledSpread);

        // TODO: Investigate/test if this is ever relevant / how to apply to position agnostic
        // I'm guessing reduces the clip size in the edge case it doesn't overlap the main context?
        // It comes from JUCE's shadow classes
        //.getIntersection (g.getClipBounds().expanded (s.radius + s.spread + 1));

        // if the scale isn't an integer, we'll be dealing with subpixel compositing
        // for example a 4.5px image centered in a canvas technically has the width of 6 pixels
        // (the outer 2 pixels will be 25%-ish opacity)
        // this is a problem because we're going to be blurring the image
        // and don't want to cut our blurs off early
        if (!juce::approximatelyEqual (scale - std::floor (scale), 0.0f))
        {
            // lazily add a buffer all around the image for sub-pixel-ness
            scaledShadowBounds.expand (1, 1);
        }
    }
}
