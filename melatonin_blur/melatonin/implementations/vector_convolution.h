#pragma once

#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "melatonin_vector/melatonin_vector.h"
#include "melatonin_vector/melatonin/utilities.h"

namespace melatonin
{
    class ConvolutionBlur
    {
    public:
        // Lets us efficiently reuse the same code for horizontal and vertical passes
        enum class Orientation {
            Horizontal,
            Vertical
        };

        ConvolutionBlur (juce::Image& i, unsigned int r) : image (i), radius (r), data (image, juce::Image::BitmapData::readWrite)
        {
            juce::jlimit (1u, 254u, radius);

            sizeOfStack = float ((radius + 1) * (radius + 1));

            // we want to allocate our vectors once, and reuse them
            size_t largestDimensionSize = (size_t) std::max (image.getWidth(), image.getHeight());

            TRACE_EVENT_BEGIN ("component", "initial allocations");
            for (auto i = 0u; i < radius * 2 + 1; ++i)
            {
                kernel.emplace_back(largestDimensionSize);
            }
            TRACE_EVENT_END ("component"); // initial allocations

            // TODO, determine template arguments and call right function here
            singleChannel ();
        }

        // A stack blur is a horizontal and then a vertical pass over the image data
        template <Orientation orientation>
        inline void blurPass()
        {
            TRACE_EVENT_BEGIN ("component", "pass");
            // This tracks the start of the circular buffer

            unsigned int queueIndex = 0;

            size_t dimensionSize; // how many pixels we're looping over and calculating the blur for
            size_t vectorSize;

            if constexpr (orientation == Orientation::Horizontal)
            {
                vectorSize = (size_t) data.height;
                dimensionSize = (size_t) data.width;
            }
            else
            {
                vectorSize = (size_t) data.width;
                dimensionSize = (size_t) data.height;
            }

            // to start with, populate a fresh vector with float values of the leftmost pixel
            // A 255 uint8 value will literally become 255.0f
            convertToFloats<orientation> (data, 0, tempPixelVector, vectorSize);

            for (auto pixel = 0u; pixel < dimensionSize; ++pixel)
            {

                TRACE_EVENT_BEGIN ("component", "pixel");
                // calculate the blurred value from the stack
                // it first goes in a temporary location...
                vector::divide (stackSumVector, sizeOfStack, tempPixelVector, vectorSize);

                // ...before being placed back in our image data as uint8
                convertToUInt8s<orientation> (data, pixel, tempPixelVector, vectorSize);

                // remove the outgoing sum from the stack
                vector::subtract (stackSumVector, sumOutVector, vectorSize);

                // remove the leftmost value from sumOutVector
                vector::subtract (sumOutVector, queue[queueIndex], vectorSize);

                // Conveniently, after advancing the index of a circular buffer
                // the old "start" (aka queueIndex) will be the new "end",
                // grab the new uint8 vals, stick it in temp buffer
                auto nextPixelIndex = pixel + radius + 1;
                // until we hid the end of the row, then just grab the last pixel
                if (nextPixelIndex >= dimensionSize)
                    nextPixelIndex = dimensionSize - 1;
                convertToFloats<orientation> (data, nextPixelIndex, queue[queueIndex], vectorSize);

                // Also add the incoming value to the sumInVector
                vector::add (sumInVector, queue[queueIndex], vectorSize);

                // Advance the queue index by 1 position
                // the new incoming element is now the "last" in the queue
                if (++queueIndex == queue.size())
                    queueIndex = 0;

                // Put into place the next incoming sums
                vector::add (stackSumVector, sumInVector, vectorSize);

                // Add the current center pixel to sumOutVector
                auto middleIndex = (queueIndex + radius) % queue.size();
                vector::add (sumOutVector, queue[middleIndex], vectorSize);

                // *remove* the new center pixel from sumInVector
                vector::subtract (sumInVector, queue[middleIndex], vectorSize);
                TRACE_EVENT_END ("component"); // pixel
            }
            TRACE_EVENT_END ("component"); // batch
        }

        // It's hard to find a clear example of this algorithm online
        // since they all have various "crusty" optimizations
        // This one is optimized for readability and simplicity
        void singleChannel ()
        {

            blurPass<Orientation::Horizontal> ();
            blurPass<Orientation::Vertical> ();
        }

    private:
        juce::Image& image;
        unsigned int radius;
        juce::Image::BitmapData data;

        // the number of virtual "values" in the weighted stack sum
        float sizeOfStack;

        // Sum of values in the right half of the queue
        std::vector<float> kernel;

        // copy of a row/col the kernel is operating on
        std::vector<float> tempPixelVector;

        // Working with vectors, we need to convert uint8 values (0-255) to<->from floats
        // The only thing really different in the passes is the way we access the underlying image data
        template <Orientation orientation>
        static inline void convertToFloats (juce::Image::BitmapData& data, size_t pixelOffset, std::vector<float>& destination, size_t vectorSize)
        {
                        if constexpr (orientation == Orientation::Horizontal)
            {
                vector::convertToFloats (data.getLinePointer (0) + (unsigned int) data.pixelStride * pixelOffset, data.lineStride, destination, vectorSize);
            }
            else
            {
                vector::convertToFloats (data.getLinePointer (pixelOffset), data.pixelStride, destination, vectorSize);
            }
        }

        template <Orientation orientation>
        static inline void convertToUInt8s (juce::Image::BitmapData& data, size_t offset, std::vector<float>& source, size_t vectorSize)
        {
                        if constexpr (orientation == Orientation::Horizontal)
            {
                vector::convertToUInt8s (source, data.getLinePointer (0) + (unsigned int) data.pixelStride * offset, data.lineStride, vectorSize);
            }
            else
            {
                vector::convertToUInt8s (source, data.getLinePointer (offset), data.pixelStride, vectorSize);
            }
        }
    };
}
