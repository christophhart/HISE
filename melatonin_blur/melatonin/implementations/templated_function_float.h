#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "melatonin_vector/melatonin_vector.h"

namespace melatonin::stackBlur
{
    template <Orientation orientation>
    static inline void convertToFloats (juce::Image::BitmapData& data, size_t rowOrColumnNumber, std::vector<float>& destination, size_t vectorSize)
    {
                if constexpr (orientation == Orientation::Horizontal)
        {
            melatonin::vector::convertToFloats (data.getLinePointer (rowOrColumnNumber), data.pixelStride, destination, vectorSize);
        }
        else
        {
            melatonin::vector::convertToFloats (data.getLinePointer (0) + (unsigned int) data.pixelStride * rowOrColumnNumber, data.lineStride, destination, vectorSize);
        }
    }

    template <Orientation orientation>
    static inline void convertToUInt8s (juce::Image::BitmapData& data, size_t rowOrColumnNumber, std::vector<float>& source, size_t vectorSize)
    {
                if constexpr (orientation == Orientation::Horizontal)
        {
            melatonin::vector::convertToUInt8s (source, data.getLinePointer (rowOrColumnNumber), data.pixelStride, vectorSize);
        }
        else
        {
            melatonin::vector::convertToUInt8s (source, data.getLinePointer (0) + (unsigned int) data.pixelStride * rowOrColumnNumber, data.lineStride, vectorSize);
        }
    }

    template <Orientation orientation>
    inline static void templatedFloatPass (juce::Image::BitmapData& data, size_t dimensionSize, int numberOfLines, size_t radius)
    {
        // The "queue" represents the current values within the sliding kernel's radius.
        // Audio people: yes, it's a circular buffer
        // It efficiently handles the sliding window nature of the kernel.
        std::vector<float> tempPixelVector (dimensionSize + radius + 1);

        float sumIn, sumOut, stackSum;
        auto sizeOfStack = (float) ((radius + 1) * (radius + 1));

        for (int lineNumber = 0; lineNumber < numberOfLines; ++lineNumber)
        {
            // grab our image pixels
            convertToFloats<orientation> (data, lineNumber, tempPixelVector, dimensionSize);

            // extend the end of the vector to include the right/bottom most x radius
            // start off filling our temp pixel vector with radius numbers of leftmost
            auto endmost = (float) *getPixel<orientation> (data, dimensionSize - 1, lineNumber);
            for (auto i = dimensionSize; i < dimensionSize + radius; ++i)
            {
                tempPixelVector[i] = endmost;
            }

            // pre-emptively divide by stack size
            // this lets us avoid it in pixel loop
            melatonin::vector::divide (tempPixelVector, sizeOfStack);

            // clear everything
            stackSum = sumIn = stackSum = 0;

            // Pre-fill the left half of the queue with the leftmost pixel value
            // should be 512 / 4 for radius 1
            sumOut = tempPixelVector[0] * (radius + 1);
            for (auto i = 0u; i <= radius; ++i)
            {
                stackSum += tempPixelVector[0] * (i + 1);
            }

            // Fill the right half of the queue with the actual pixel values
            // zero is the center pixel here, it's added to the sum radius + 1 times
            for (auto i = 1u; i <= radius; ++i)
            {
                float value = tempPixelVector[i];

                sumIn += value; // should be 255 / 4 for radius 1
                stackSum += value * (radius + 1 - i);
            }

            // "pixel" is the center pixel that we are calculating for
            for (int pixel = 0; pixel < dimensionSize; ++pixel)
            {
                // the sum for this pixel is already calculated
                *getPixel<orientation> (data, pixel, lineNumber) = (uint8_t) stackSum;

                // if the next pixel won't alter our stackSum
                // move on without even advancing the queue
                auto nextValue = tempPixelVector[pixel + radius + 1];

                // disable martin optimization for now, so tests pass
                //                if (tempPixelVector[pixel] == nextValue)
                //                    continue;

                // remove the outgoing sum from the stack
                stackSum -= sumOut; // is 510 here

                // remove the leftmost value from sumOut
                int leftmost = ((pixel - (int) radius > 0) ? (pixel - (int) radius) : 0);
                sumOut -= tempPixelVector[leftmost]; // is 255 here

                // add the next incoming value to sumIn
                sumIn += nextValue; // is 510 here

                // Put into place the next incoming sums
                stackSum += sumIn; // 1020 here

                // Add the new center pixel to sumOut
                sumOut += tempPixelVector[pixel + 1]; // 510 here

                // *remove* the new center pixel from sumIn
                sumIn -= tempPixelVector[pixel + 1]; // 255 here
            }
        }
    }

    static void templatedFloatSingleChannel (juce::Image& img, unsigned int radius)
    {
        const auto w = static_cast<unsigned int> (img.getWidth());
        const auto h = static_cast<unsigned int> (img.getHeight());

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        templatedFloatPass<Orientation::Horizontal> (data, w, h, radius);
        templatedFloatPass<Orientation::Vertical> (data, h, w, radius);
    }
}
