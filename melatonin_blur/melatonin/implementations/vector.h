#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "melatonin_vector/melatonin_vector.h"

namespace melatonin::stackBlur
{
    // It's hard to find a clear example of this algorithm online
    // since they all have various "crusty" optimizations
    // This one is optimized for readability and simplicity
    static void vectorSingleChannel (juce::Image& img, unsigned int radius)
    {
        const auto w = static_cast<unsigned int> (img.getWidth());
        const auto h = static_cast<unsigned int> (img.getHeight());

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        // Ensure radius is within bounds
        radius = juce::jlimit (1u, 254u, radius);

        // This tracks the start of the circular buffer
        unsigned int queueIndex = 0;

        // the "stack" is all in our head, maaaan
        // think of it as a *weighted* sum of the values in the queue
        // each time the queue moves, we add the rightmost values of the queue to the stack
        // and remove the left values of the queue from the stack
        // the blurred pixel is then calculated by dividing by the number of "values" in the weighted stack sum
        auto sizeOfStack = float ((radius + 1) * (radius + 1));


        size_t vectorSize = std::max (w, h);

        // The "queue" represents the current values within the sliding kernel's radius.
        /* Here the queue is rotated to optimize for vector mem access in main loop
         *
         * Radius of 2:
         *
         *                                    rows: 0  1  2  3... end
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


        // HORIZONTAL PASS — all rows at once, progressing to the right
        // Time to fill our vector with columns of floats
        // vdsp pretty much only works with floats
        // our "source vector" is uint8_t
        // single channel means the pixel "stride" is 1, but we're populating with columns first
        // so it's data.lineStride which is "12" for a 10 pixel wide single channel image

        // to start with, populate a fresh vector with float values of the leftmost pixel
        // A 255 uint8 value will literally become 255.0f

        vector::convertToFloats (data.getLinePointer (0), data.lineStride, tempPixelVector, vectorSize);

        // Now pre-fill the left half of the queue with this leftmost pixel value
        for (auto i = 0u; i <= radius; ++i)
        {
            // these init left side AND middle of the stack
            vector::copy (tempPixelVector, queue[i]);
            vector::add (sumOutVector, tempPixelVector);
            vector::accumulateScalarVectorProduct (tempPixelVector, (float) i + 1, stackSumVector);
        }


        // Fill the right half of the queue with the next pixel values
        // zero is the center pixel here, it was added above (radius + 1) times to the sum
        for (auto i = 1u; i <= radius; ++i)
        {
            if (i <= w - 1)
            {
                vector::convertToFloats (data.getLinePointer (0) + (unsigned int) data.pixelStride * i, data.lineStride, tempPixelVector, h);
            }
            // edge case where queue is bigger than image width!
            // for example vertical test where width = 1
            else
            {
                vector::convertToFloats (data.getLinePointer (0) + (unsigned int) data.pixelStride * (w - 1), data.lineStride, tempPixelVector, h);
            }

            vector::copy (tempPixelVector, queue[radius + i]);
            vector::add (sumInVector, tempPixelVector);
            vector::accumulateScalarVectorProduct (tempPixelVector, (float) (radius + 1 - i), stackSumVector);
        }

        for (auto x = 0u; x < w; ++x)
        {

            // calculate the blurred value from the stack
            // it first goes in a temporary location...
            vector::divide (stackSumVector, sizeOfStack, tempPixelVector);

            // ...before being placed back in our image data as uint8
            vector::convertToUInt8s (tempPixelVector, data.getLinePointer (0) + (unsigned int) data.pixelStride * x, data.lineStride, h);

            // remove the outgoing sum from the stack
            vector::subtract (stackSumVector, sumOutVector);

            // remove the leftmost value from sumOutVector
            vector::subtract (sumOutVector, queue[queueIndex]);

            // Conveniently, after advancing the index of a circular buffer
            // the old "start" (aka queueIndex) will be the new "end",
            // grab the new uint8 vals, stick it in temp buffer
            if (x + radius + 1 < w)
            {
                // grab pixels from each row, offset by x+radius+1
                vector::convertToFloats (data.getLinePointer (0) + (unsigned int) data.pixelStride * (x + radius + 1), data.lineStride, queue[queueIndex], h);
            }
            else
            {
                vector::convertToFloats (data.getLinePointer (0) + (unsigned int) data.pixelStride * (w - 1), data.lineStride, queue[queueIndex], h);
            }

            // Also add the incoming value to the sumInVector
            vector::add (sumInVector, queue[queueIndex]);

            // Advance the queue index by 1 position
            // the new incoming element is now the "last" in the queue
            if (++queueIndex == queue.size())
                queueIndex = 0;

            // Put into place the next incoming sums
            vector::add (stackSumVector, sumInVector);

            // Add the current center pixel to sumOutVector
            auto middleIndex = (queueIndex + radius) % queue.size();
            vector::add (sumOutVector, queue[middleIndex]);

            // *remove* the new center pixel from sumInVector
            vector::subtract (sumInVector, queue[middleIndex]);
        }

        // VERTICAL PASS: all columns at once, progressing to bottom
        // This pass is nicely optimized for vectors, since it uses data.pixelStride
        // clear everything
        vector::clear (stackSumVector);
        vector::clear (sumInVector);
        vector::clear (sumOutVector);
        queueIndex = 0;

        // populate our temp vector with float values of the topmost pixel
        // A 255 uint8 value will literally become 255.0f
        vector::convertToFloats (data.getLinePointer (0), data.pixelStride, tempPixelVector, w);

        // Now pre-fill the left half of the queue with the topmost pixel values
        // queue is already populated from the horizontal pass
        for (auto i = 0u; i <= radius; ++i)
        {
            // these init left side AND middle of the stack
            vector::copy (tempPixelVector, queue[i]);
            vector::add (sumOutVector, tempPixelVector);
            vector::accumulateScalarVectorProduct (tempPixelVector, (float) i + 1, stackSumVector);
        }


        // Fill the right half of the queue with pixel values from the next rows
        for (auto i = 1u; i <= radius; ++i)
        {
            if (i <= h - 1)
            {
                vector::convertToFloats (data.getLinePointer (i), data.pixelStride, tempPixelVector, w);
            }
            // edge case where queue is bigger than image width!
            // for example vertical test where width = 1
            else
            {
                vector::convertToFloats (data.getLinePointer (h - 1), data.pixelStride, tempPixelVector, w);
            }

            vector::copy (tempPixelVector, queue[radius + i]);
            vector::add (sumInVector, tempPixelVector);
            vector::accumulateScalarVectorProduct (tempPixelVector, (float) (radius + 1 - i), stackSumVector);
        }

        for (auto y = 0u; y < h; ++y)
        {
            // calculate the blurred value vector from the stack
            // it first goes in a temporary location...
            vector::divide (stackSumVector, sizeOfStack, tempPixelVector);

            // ...before being placed back in our image data as uint8
            vector::convertToUInt8s (tempPixelVector, data.getLinePointer (y), data.pixelStride, w);

            // remove the outgoing sum from the stack
            vector::subtract (stackSumVector, sumOutVector);

            // remove the leftmost value from sumOutVector
            vector::subtract (sumOutVector, queue[queueIndex]);

            // grab the incoming value (or the bottom most pixel if we're near the bottom)
            // and stick it in our queue
            if (y + radius + 1 < h)
            {
                // grab pixels from each row, offset by x+radius+1
                vector::convertToFloats (data.getLinePointer (y + radius + 1), data.pixelStride, queue[queueIndex], h);
            }
            else
            {
                vector::convertToFloats (data.getLinePointer (h - 1), data.pixelStride, queue[queueIndex], h);
            }

            // Also add the incoming value to the sumInVector
            vector::add (sumInVector, queue[queueIndex]);

            // Advance the queue index by 1 position
            // the new incoming element is now the "last" in the queue
            if (++queueIndex == queue.size())
                queueIndex = 0;

            // Put into place the next incoming sums
            vector::add (stackSumVector, sumInVector);

            // Add the current center pixel to sumOutVector
            auto middleIndex = (queueIndex + radius) % queue.size();
            vector::add (sumOutVector, queue[middleIndex]);

            // *remove* the new center pixel from sumInVector
            vector::subtract (sumInVector, queue[middleIndex]);
        }
    }
}
