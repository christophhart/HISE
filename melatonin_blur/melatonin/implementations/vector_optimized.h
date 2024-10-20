#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "melatonin_vector/melatonin_vector.h"

namespace melatonin::stackBlur
{

    // Working with vectors, we need to convert uint8 values (0-255) to<->from floats
    // The only thing really different in the passes is the way we access the underlying image data
    template <Orientation orientation>
    static inline void convertToFloats (juce::Image::BitmapData& data, size_t batchOffset, size_t pixelOffset, std::vector<float>& destination, size_t vectorSize)
    {
                if constexpr (orientation == Orientation::Horizontal)
        {
            vector::convertToFloats (data.getLinePointer (batchOffset) + (unsigned int) data.pixelStride * pixelOffset, data.lineStride, destination, vectorSize);
        }
        else
        {
            vector::convertToFloats (data.getLinePointer (pixelOffset) + (unsigned int) data.pixelStride * batchOffset, data.pixelStride, destination, vectorSize);
        }
    }

    template <Orientation orientation>
    static inline void convertToUInt8s (juce::Image::BitmapData& data, size_t batchOffset, size_t offset, std::vector<float>& source, size_t vectorSize)
    {
                if constexpr (orientation == Orientation::Horizontal)
        {
            vector::convertToUInt8s (source, data.getLinePointer (batchOffset) + (unsigned int) data.pixelStride * offset, data.lineStride, vectorSize);
        }
        else
        {
            vector::convertToUInt8s (source, data.getLinePointer (offset) + (unsigned int) data.pixelStride * batchOffset, data.pixelStride, vectorSize);
        }
    }

    template <Orientation orientation>
    static inline void stackBlurPass (juce::Image::BitmapData& data, unsigned int radius)
    {
        TRACE_EVENT_BEGIN ("component", "pass");
        // This tracks the start of the circular buffer
        unsigned int queueIndex = 0;

        // the "stack" is all in our head, maaaan
        // think of it as a *weighted* sum of the values in the queue
        // each time the queue moves, we add the rightmost values of the queue to the stack
        // and remove the left values of the queue from the stack
        // the blurred pixel is then calculated by dividing by the number of "values" in the weighted stack sum
        auto sizeOfStack = float ((radius + 1) * (radius + 1));

        size_t totalVectorSize; // how many we're vectorizing
        size_t dimensionSize; // how many pixels we're looping over and calculating the blur for
        size_t vectorSize = 64; // we don't want vectors with 800 pixels in em, do fixed chunks

        if constexpr (orientation == Orientation::Horizontal)
        {
            totalVectorSize = (size_t) data.height;
            dimensionSize = (size_t) data.width;
        }
        else
        {
            totalVectorSize = (size_t) data.width;
            dimensionSize = (size_t) data.height;
        }

        TRACE_EVENT_BEGIN ("component", "initial allocations");
        // The "queue" represents the current values within the sliding kernel's radius.
        /* Here the queue is rotated to optimize for vector mem access in main loop
         *
         * Radius of 2:
         *
         *                                    rows: 0  1  2  3...
         * prefilled with leftmost pixel ->       q [] [] [] []
         *                         ditto ->       u []
         *                leftmost pixel ->       e []
         *                                        u []
         *                                        e []
         *
         *     We need to convert all the pixels in the queue to floats anyway
         */
        std::vector<std::vector<float>> queue;
        for (auto i = 0u; i < radius * 2 + 1; ++i)
        {
            queue.emplace_back (vectorSize, 0);
        }

        // one for each column
        std::vector<float> stackSumVector (vectorSize, 0);

        // Sum of values in the right half of the queue
        std::vector<float> sumInVector (vectorSize, 0);

        // Sum of values in the left half of the queue
        std::vector<float> sumOutVector (vectorSize, 0);

        // little helper for prefilling and conversion
        std::vector<float> tempPixelVector (vectorSize, 0);

        TRACE_EVENT_END ("component"); // initial allocations

        // HORIZONTAL PASS — all rows at once, progressing to the right
        // Time to fill our vector with columns of floats
        // vdsp pretty much only works with floats
        // our "source vector" is uint8_t
        // single channel means the pixel "stride" is 1, but we're populating with columns first
        // so it's data.lineStride which is "12" for a 10 pixel wide single channel image

        // to start with, populate a fresh vector with float values of the leftmost pixel
        // A 255 uint8 value will literally become 255.0f

        for (auto offset = 0; offset < totalVectorSize; offset += vectorSize)
        {
            TRACE_EVENT_BEGIN ("component", "batch");
            // vectorSize might be less at the end
            vectorSize = std::min (totalVectorSize - offset, vectorSize);

            convertToFloats<orientation> (data, offset, 0, tempPixelVector, vectorSize);

            TRACE_EVENT_BEGIN ("component", "initial queue");
            // Now pre-fill the left half of the queue with this leftmost pixel value
            // TODO: this doesn't need to be in a loop, can be unrolled and replace vec initializations
            for (auto i = 0u; i <= radius; ++i)
            {
                // these init left side AND middle of the stack
                vector::copy (tempPixelVector, queue[i], vectorSize);
                vector::add (sumOutVector, tempPixelVector, vectorSize);
                vector::accumulateScalarVectorProduct (tempPixelVector, (float) i + 1, stackSumVector, vectorSize);
            }

            // Fill the right half of the queue with the next pixel values
            // zero is the center pixel here, it was added above (radius + 1) times to the sum
            for (auto i = 1u; i <= radius; ++i)
            {
                auto pixelOffset = (i <= dimensionSize - 1) ? i : dimensionSize - 1;
                convertToFloats<orientation> (data, offset, pixelOffset, tempPixelVector, vectorSize);

                vector::copy (tempPixelVector, queue[radius + i], vectorSize);
                vector::add (sumInVector, tempPixelVector, vectorSize);
                vector::accumulateScalarVectorProduct (tempPixelVector, (float) (radius + 1 - i), stackSumVector, vectorSize);
            }

            TRACE_EVENT_END ("component"); // initial queue

            for (auto pixel = 0u; pixel < dimensionSize; ++pixel)
            {
                TRACE_EVENT_BEGIN ("component", "pixel");
                // calculate the blurred value from the stack
                // it first goes in a temporary location...
                vector::divide (stackSumVector, sizeOfStack, tempPixelVector, vectorSize);

                // ...before being placed back in our image data as uint8
                convertToUInt8s<orientation> (data, offset, pixel, tempPixelVector, vectorSize);

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
                convertToFloats<orientation> (data, offset, nextPixelIndex, queue[queueIndex], vectorSize);

                // Also add the incoming value to the sumInVector
                vector::add (sumInVector, queue[queueIndex], vectorSize);

                // Advance the queue index by 1 position
                // the new incoming element is now the "last" in the queue
                if (++queueIndex == queue.size())
                    queueIndex = 0;

                // Put into place the next incoming sums
                vector::add (stackSumVector, sumInVector, vectorSize);

                // Add the current center pixel to sumOutVector
                auto middleIndex = (queueIndex + radius) %  queue.size();
                vector::add (sumOutVector, queue[middleIndex], vectorSize);

                // *remove* the new center pixel from sumInVector
                vector::subtract (sumInVector, queue[middleIndex], vectorSize);
                TRACE_EVENT_END ("component"); // pixel
            }
            TRACE_EVENT_END ("component"); // batch
        }
        TRACE_EVENT_END ("component"); // pass
    }
    // It's hard to find a clear example of this algorithm online
    // since they all have various "crusty" optimizations
    // This one is optimized for readability and simplicity
    static void vectorOptimizedSingleChannel (juce::Image& img, unsigned int radius)
    {

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        juce::jlimit (1u, 254u, radius);




        stackBlurPass<Orientation::Horizontal> (data, radius);
        stackBlurPass<Orientation::Vertical> (data, radius);
    }
}
