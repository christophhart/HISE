#pragma once
#include "juce_dsp/juce_dsp.h"
#include "juce_graphics/juce_graphics.h"

/*
 * This implementation could be improved.
 * On single channel, it's sometimes better, sometimes worse than Gin.
 * On ARGB it's about 1.5x worse than Gin, currently.
 *
 * One thing it certainly is: ugly and hard to read. If you want to understand what's going on,
 * I recommend looking at the other vector implementations which are more straightforward.
 */
namespace melatonin::blur
{
    static void juceFloatVectorSingleChannel (juce::Image& img, int radius)
    {
        const int w = static_cast<int> (img.getWidth());
        const int h = static_cast<int> (img.getHeight());

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        // Ensure radius is within bounds
        radius = juce::jlimit (1, 254, radius);

        // This tracks the start of the circular buffer
        unsigned int queueIndex = 0;

        // the "stack" is all in our head, maaaan
        // think of it as a *weighted* sum of the values in the queue
        // each time the queue moves, we add the rightmost values of the queue to the stack
        // and remove the left values of the queue from the stack
        // the blurred pixel is then calculated by dividing by the number of "values" in the weighted stack sum
        // in the end, we're working with a divisor, since FloatVectorOps doesn't do division
        auto divisor = 1.0f / float ((radius + 1) * (radius + 1));

        // we're going to be using vectors, allocated once
        // and used for both horizontal and vertical passes
        // so we need to know the max size we need
        auto vectorSize = (size_t) std::max (w, h);

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
        for (auto i = 0; i < radius * 2 + 1; ++i)
        {
            queue.emplace_back (vectorSize, 0.0f);
        }

        // one sum for each column
        std::vector<float> stackSumVector (vectorSize, 0);

        // Sum of values in the right half of the queue
        std::vector<float> sumInVector (vectorSize, 0);

        // Sum of values in the left half of the queue
        std::vector<float> sumOutVector (vectorSize, 0);

        // little helper for prefilling and conversion
        std::vector<float> tempPixelVector (vectorSize, 0);

        // HORIZONTAL PASS — all rows at once, progressing to the right
        // Time to fill our vector with columns of floats
        // single channel means the pixel "stride" is 1, but we're populating with columns first
        // so it's data.lineStride which is "12" for a 10 pixel wide single channel image

        // to start with, populate a fresh vector with float values of the leftmost pixels
        // A 255 uint8 value will literally become 255.0f
        for (auto i = 0; i < h; ++i)
            tempPixelVector[(uint8_t) i] = (float) data.getLinePointer (i)[0];

        // Now pre-fill the left half of the queue with this leftmost pixel value
        for (size_t i = 0; i <= size_t (radius); ++i)
        {
            // these initialize the left side AND middle of the stack
            juce::FloatVectorOperations::copy (queue[i].data(), tempPixelVector.data(), h);
            juce::FloatVectorOperations::add (sumOutVector.data(), tempPixelVector.data(), h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector.data(), tempPixelVector.data(), (float) i + 1, h);
        }

        // Fill the right half of the queue with the next pixel values
        // zero is the center pixel here, it was already added above (radius + 1) times to the sum
        for (auto i = 1; i <= radius; ++i)
        {
            if (i <= w - 1)
            {
                for (int row = 0; row < h; ++row)
                    tempPixelVector[(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * i];
            }
            // edge case where queue is bigger than image width!
            // for example in our vertical tests where width=1
            else
            {
                for (int row = 0; row < h; ++row)
                    tempPixelVector[(size_t) row] = data.getLinePointer (row)[data.pixelStride * (w - 1)];
            }

            juce::FloatVectorOperations::copy (queue[radius + i].data(), tempPixelVector.data(), h);
            juce::FloatVectorOperations::add (sumInVector.data(), tempPixelVector.data(), h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector.data(), tempPixelVector.data(), (float) (radius + 1 - i), h);
        }

        for (int x = 0; x < w; ++x)
        {
            // calculate the blurred value from the stack
            // it first goes in a temporary location...
            juce::FloatVectorOperations::copy (tempPixelVector.data(), stackSumVector.data(), h);
            juce::FloatVectorOperations::multiply (tempPixelVector.data(), divisor, h);

            // ...before being placed back in our image data as uint8
            // unfortunately not a great way to vectorize this with juce
            for (auto i = 0; i < h; ++i)
                data.getLinePointer (i)[(size_t) data.pixelStride * x] = (unsigned char) tempPixelVector[(size_t) i];

            // remove the outgoing sum from the stack
            juce::FloatVectorOperations::subtract (stackSumVector.data(), sumOutVector.data(), h);

            // remove the leftmost value from sumOutVector
            juce::FloatVectorOperations::subtract (sumOutVector.data(), queue[queueIndex].data(), h);

            // Conveniently, after advancing the index of a circular buffer
            // the old "start" (aka queueIndex) will be the new "end",
            // grab the new uint8 vals, stick it in temp buffer
            if (x + radius + 1 < w)
            {
                // grab incoming pixels for each row (they are offset by x+radius+1)
                for (int row = 0; row < h; ++row)
                    queue[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * (x + radius + 1)];
            }
            else
            {
                for (int row = 0; row < h; ++row)
                    queue[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * ((size_t) w - 1)];
            }

            // Also add the incoming value to the sumInVector
            juce::FloatVectorOperations::add (sumInVector.data(), queue[queueIndex].data(), h);

            // Advance the queue index by 1 position
            // the new incoming element is now the "last" in the queue
            if (++queueIndex == queue.size())
                queueIndex = 0;

            // Put into place the next incoming sums
            juce::FloatVectorOperations::add (stackSumVector.data(), sumInVector.data(), h);

            // Add the current center pixel to sumOutVector
            auto middleIndex = (queueIndex + radius) % queue.size();
            juce::FloatVectorOperations::add (sumOutVector.data(), queue[middleIndex].data(), h);

            // *remove* the new center pixel from sumInVector
            juce::FloatVectorOperations::subtract (sumInVector.data(), queue[middleIndex].data(), h);
        }

        // VERTICAL PASS: this does all columns at once, progressing from top to bottom
        // This pass is VERY nicely optimized for vectors, since it uses data.pixelStride
        // clear our reusable vectors first
        std::fill (stackSumVector.begin(), stackSumVector.end(), 0.0f);
        std::fill (sumInVector.begin(), sumInVector.end(), 0.0f);
        std::fill (sumOutVector.begin(), sumOutVector.end(), 0.0f);
        queueIndex = 0;

        // populate our temp vector with float values of the topmost pixels
        for (size_t i = 0; i < static_cast<size_t>(w); ++i)
            tempPixelVector[i] = (float) data.getLinePointer (0)[i];

        // Now pre-fill the left half of the queue with the topmost pixel values
        // (queue is already initialized from the horizontal pass)
        for (size_t i = 0; i <= static_cast<size_t>(radius); ++i)
        {
            // these init left side AND middle of the stack
            juce::FloatVectorOperations::copy (queue[i].data(), tempPixelVector.data(), w);
            juce::FloatVectorOperations::add (sumOutVector.data(), tempPixelVector.data(), w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector.data(), tempPixelVector.data(), (float) i + 1, w);
        }

        // Fill the right half of the queue with pixel values from the next rows
        for (auto i = 1; i <= radius; ++i)
        {
            if (i <= h - 1)
            {
                for (size_t col = 0; col < (size_t) w; ++col)
                    tempPixelVector[col] = (float) data.getLinePointer (i)[col];
            }
            // edge case where queue is bigger than image width!
            // for example vertical test where width = 1
            else
            {
                for (size_t col = 0; col < (size_t) w; ++col)
                    tempPixelVector[col] = (float) data.getLinePointer (h - 1)[col];
            }

            juce::FloatVectorOperations::copy (queue[radius + i].data(), tempPixelVector.data(), w);
            juce::FloatVectorOperations::add (sumInVector.data(), tempPixelVector.data(), w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector.data(), tempPixelVector.data(), (float) (radius + 1 - i), w);
        }

        for (auto y = 0; y < h; ++y)
        {
            // calculate the blurred value vector from the stack
            // it first goes in a temporary location...
            juce::FloatVectorOperations::copy (tempPixelVector.data(), stackSumVector.data(), w);
            juce::FloatVectorOperations::multiply (tempPixelVector.data(), divisor, w);

            // ...before being placed back in our image data as uint8
            for (size_t i = 0; i < static_cast<size_t>(w); ++i)
                data.getLinePointer (y)[i] = (uint8_t) tempPixelVector[i];

            // remove the outgoing sum from the stack
            juce::FloatVectorOperations::subtract (stackSumVector.data(), sumOutVector.data(), w);

            // remove the leftmost value from sumOutVector
            juce::FloatVectorOperations::subtract (sumOutVector.data(), queue[queueIndex].data(), w);

            // grab the incoming value (or the bottom most pixel if we're near the bottom)
            // and stick it in our queue
            if (y + radius + 1 < h)
            {
                // grab pixels from each row, offset by x+radius+1
                for (size_t col = 0; col < (size_t) w; ++col)
                    queue[queueIndex][col] = (float) data.getLinePointer ((int) (y + radius + 1))[col];
            }
            else
            {
                // bottom of image, grab bottom pixel
                for (size_t col = 0; col < (size_t) w; ++col)
                    queue[queueIndex][col] = (float) data.getLinePointer (h - 1)[col];
            }

            // Also add the incoming value to the sumInVector
            juce::FloatVectorOperations::add (sumInVector.data(), queue[queueIndex].data(), w);

            // Advance the queue index by 1 position
            // the new incoming element is now the "last" in the queue
            if (++queueIndex == queue.size())
                queueIndex = 0;

            // Put into place the next incoming sums
            juce::FloatVectorOperations::add (stackSumVector.data(), sumInVector.data(), w);

            // Add the current center pixel to sumOutVector
            auto middleIndex = (queueIndex + radius) % queue.size();
            juce::FloatVectorOperations::add (sumOutVector.data(), queue[middleIndex].data(), w);

            // *remove* the new center pixel from sumInVector
            juce::FloatVectorOperations::subtract (sumInVector.data(), queue[middleIndex].data(), w);
        }
    }

    // The ARGB channel is byte order agnostic
    // it just performs stack blur on 4 channels without caring what they are
    [[maybe_unused]] static void juceFloatVectorARGB (juce::Image& img, int radius)
    {
        const int w = static_cast<int> (img.getWidth());
        const int h = static_cast<int> (img.getHeight());

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        // Ensure radius is within bounds
        radius = juce::jlimit (1, 254, radius);

        // This tracks the start of the circular buffer
        unsigned int queueIndex = 0;

        // the "stack" is all in our head, maaaan
        // think of it as a *weighted* sum of the values in the queue
        // each time the queue moves, we add the rightmost values of the queue to the stack
        // and remove the left values of the queue from the stack
        // the blurred pixel is then calculated by dividing by the number of "values" in the weighted stack sum
        // in the end, we're working with a divisor, since FloatVectorOps doesn't do division
        auto divisor = 1.0f / float ((radius + 1) * (radius + 1));

        // we're going to be using vectors, allocated once
        // and used for both horizontal and vertical passes
        // so we need to know the max size we need
        auto vectorSize = (size_t) std::max (w, h);

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
         *
         *     For ARGB version, we're going to have 4 queues, one for each channel
         */
        std::vector<std::vector<float>> queue0;
        std::vector<std::vector<float>> queue1;
        std::vector<std::vector<float>> queue2;
        std::vector<std::vector<float>> queue3;

        for (auto i = 0; i < radius * 2 + 1; ++i)
        {
            queue0.emplace_back (vectorSize, 0.0f);
            queue1.emplace_back (vectorSize, 0.0f);
            queue2.emplace_back (vectorSize, 0.0f);
            queue3.emplace_back (vectorSize, 0.0f);
        }

        // one sum for each column for each channel
        std::vector<float> stackSumVector0 (vectorSize, 0);
        std::vector<float> stackSumVector1 (vectorSize, 0);
        std::vector<float> stackSumVector2 (vectorSize, 0);
        std::vector<float> stackSumVector3 (vectorSize, 0);

        // Sum of values in the right half of the queue
        std::vector<float> sumInVector0 (vectorSize, 0);
        std::vector<float> sumInVector1 (vectorSize, 0);
        std::vector<float> sumInVector2 (vectorSize, 0);
        std::vector<float> sumInVector3 (vectorSize, 0);

        // Sum of values in the left half of the queue
        std::vector<float> sumOutVector0 (vectorSize, 0);
        std::vector<float> sumOutVector1 (vectorSize, 0);
        std::vector<float> sumOutVector2 (vectorSize, 0);
        std::vector<float> sumOutVector3 (vectorSize, 0);

        // little helper for prefilling and conversion
        std::vector<float> tempPixelVector0 (vectorSize, 0);
        std::vector<float> tempPixelVector1 (vectorSize, 0);
        std::vector<float> tempPixelVector2 (vectorSize, 0);
        std::vector<float> tempPixelVector3 (vectorSize, 0);

        // HORIZONTAL PASS — all rows at once (aka 1 vertical col) progressing to the right
        // Time to fill our vector with columns of floats
        // single channel means the pixel "stride" is 1, but we're populating with columns first
        // so it's data.lineStride which is "12" for a 10 pixel wide single channel image

        // to start with, populate a fresh vector with float values of the leftmost pixels
        // A 255 uint8 value will literally become 255.0f
        for (auto i = 0; i < h; ++i)
        {
            tempPixelVector0[(uint8_t) i] = (float) data.getLinePointer (i)[0];
            tempPixelVector1[(uint8_t) i] = (float) data.getLinePointer (i)[1];
            tempPixelVector2[(uint8_t) i] = (float) data.getLinePointer (i)[2];
            tempPixelVector3[(uint8_t) i] = (float) data.getLinePointer (i)[3];
        }

        // Now pre-fill the left half of the queue with this leftmost pixel value
        for (size_t i = 0u; i <= static_cast<size_t>(radius); ++i)
        {
            // these initialize the left side AND middle of the stack
            juce::FloatVectorOperations::copy (queue0[i].data(), tempPixelVector0.data(), h);
            juce::FloatVectorOperations::copy (queue1[i].data(), tempPixelVector1.data(), h);
            juce::FloatVectorOperations::copy (queue2[i].data(), tempPixelVector2.data(), h);
            juce::FloatVectorOperations::copy (queue3[i].data(), tempPixelVector3.data(), h);
            juce::FloatVectorOperations::add (sumOutVector0.data(), tempPixelVector0.data(), h);
            juce::FloatVectorOperations::add (sumOutVector1.data(), tempPixelVector1.data(), h);
            juce::FloatVectorOperations::add (sumOutVector2.data(), tempPixelVector2.data(), h);
            juce::FloatVectorOperations::add (sumOutVector3.data(), tempPixelVector3.data(), h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector0.data(), tempPixelVector0.data(), (float) i + 1, h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector1.data(), tempPixelVector1.data(), (float) i + 1, h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector2.data(), tempPixelVector2.data(), (float) i + 1, h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector3.data(), tempPixelVector3.data(), (float) i + 1, h);
        }

        // Fill the right half of the queue with the next pixel values
        // zero is the center pixel here, it was already added above (radius + 1) times to the sum
        for (auto i = 1; i <= radius; ++i)
        {
            if (i <= w - 1)
            {
                for (int row = 0; row < h; ++row)
                {
                    tempPixelVector0[(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * i];
                    tempPixelVector1[(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * i + 1];
                    tempPixelVector2[(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * i + 2];
                    tempPixelVector3[(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * i + 3];
                }
            }
            else
            {
                // edge case where queue is bigger than image width!
                // for example in our vertical tests where width=1
                for (int row = 0; row < h; ++row)
                {
                    tempPixelVector0[(size_t) row] = data.getLinePointer (row)[data.pixelStride * (w - 1)];
                    tempPixelVector1[(size_t) row] = data.getLinePointer (row)[data.pixelStride * (w - 1) + 1];
                    tempPixelVector2[(size_t) row] = data.getLinePointer (row)[data.pixelStride * (w - 1) + 2];
                    tempPixelVector3[(size_t) row] = data.getLinePointer (row)[data.pixelStride * (w - 1) + 3];
                }
            }

            juce::FloatVectorOperations::copy (queue0[radius + i].data(), tempPixelVector0.data(), h);
            juce::FloatVectorOperations::copy (queue1[radius + i].data(), tempPixelVector1.data(), h);
            juce::FloatVectorOperations::copy (queue2[radius + i].data(), tempPixelVector2.data(), h);
            juce::FloatVectorOperations::copy (queue3[radius + i].data(), tempPixelVector3.data(), h);
            juce::FloatVectorOperations::add (sumInVector0.data(), tempPixelVector0.data(), h);
            juce::FloatVectorOperations::add (sumInVector1.data(), tempPixelVector1.data(), h);
            juce::FloatVectorOperations::add (sumInVector2.data(), tempPixelVector2.data(), h);
            juce::FloatVectorOperations::add (sumInVector3.data(), tempPixelVector3.data(), h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector0.data(), tempPixelVector0.data(), (float) (radius + 1 - i), h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector1.data(), tempPixelVector1.data(), (float) (radius + 1 - i), h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector2.data(), tempPixelVector2.data(), (float) (radius + 1 - i), h);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector3.data(), tempPixelVector3.data(), (float) (radius + 1 - i), h);
        }

        // horizontal pass main loop
        for (auto x = 0; x < w; ++x)
        {
            // calculate the blurred value from the stack
            // it first goes in a temporary location...
            juce::FloatVectorOperations::copy (tempPixelVector0.data(), stackSumVector0.data(), h);
            juce::FloatVectorOperations::copy (tempPixelVector1.data(), stackSumVector1.data(), h);
            juce::FloatVectorOperations::copy (tempPixelVector2.data(), stackSumVector2.data(), h);
            juce::FloatVectorOperations::copy (tempPixelVector3.data(), stackSumVector3.data(), h);
            juce::FloatVectorOperations::multiply (tempPixelVector0.data(), divisor, h);
            juce::FloatVectorOperations::multiply (tempPixelVector1.data(), divisor, h);
            juce::FloatVectorOperations::multiply (tempPixelVector2.data(), divisor, h);
            juce::FloatVectorOperations::multiply (tempPixelVector3.data(), divisor, h);

            // ...before being placed back in our image data as uint8
            for (auto i = 0; i < h; ++i)
            {
                data.getLinePointer (i)[(size_t) data.pixelStride * x] = (unsigned char) tempPixelVector0[(size_t) i];
                data.getLinePointer (i)[(size_t) data.pixelStride * x + 1] = (unsigned char) tempPixelVector1[(size_t) i];
                data.getLinePointer (i)[(size_t) data.pixelStride * x + 2] = (unsigned char) tempPixelVector2[(size_t) i];
                data.getLinePointer (i)[(size_t) data.pixelStride * x + 3] = (unsigned char) tempPixelVector3[(size_t) i];
            }

            // remove the outgoing sum from the stack
            juce::FloatVectorOperations::subtract (stackSumVector0.data(), sumOutVector0.data(), h);
            juce::FloatVectorOperations::subtract (stackSumVector1.data(), sumOutVector1.data(), h);
            juce::FloatVectorOperations::subtract (stackSumVector2.data(), sumOutVector2.data(), h);
            juce::FloatVectorOperations::subtract (stackSumVector3.data(), sumOutVector3.data(), h);

            // remove the leftmost value from sumOutVector
            juce::FloatVectorOperations::subtract (sumOutVector0.data(), queue0[queueIndex].data(), h);
            juce::FloatVectorOperations::subtract (sumOutVector1.data(), queue1[queueIndex].data(), h);
            juce::FloatVectorOperations::subtract (sumOutVector2.data(), queue2[queueIndex].data(), h);
            juce::FloatVectorOperations::subtract (sumOutVector3.data(), queue3[queueIndex].data(), h);

            // Conveniently, after advancing the index of a circular buffer
            // the old "start" (aka queueIndex) will be the new "end",
            // grab the new uint8 vals, stick it in temp buffer
            if (x + radius + 1 < w)
            {
                // grab incoming pixels for each row (they are offset by x+radius+1)
                for (int row = 0; row < h; ++row)
                {
                    queue0[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * (x + radius + 1)];
                    queue1[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * (x + radius + 1) + 1];
                    queue2[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * (x + radius + 1) + 2];
                    queue3[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * (x + radius + 1) + 3];
                }
            }
            else
            {
                for (int row = 0; row < h; ++row)
                {
                    queue0[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * ((size_t) w - 1)];
                    queue1[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * ((size_t) w - 1) + 1];
                    queue2[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * ((size_t) w - 1) + 2];
                    queue3[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * ((size_t) w - 1) + 3];
                }
            }

            // Add the incoming value to the sumInVector
            juce::FloatVectorOperations::add (sumInVector0.data(), queue0[queueIndex].data(), h);
            juce::FloatVectorOperations::add (sumInVector1.data(), queue1[queueIndex].data(), h);
            juce::FloatVectorOperations::add (sumInVector2.data(), queue2[queueIndex].data(), h);
            juce::FloatVectorOperations::add (sumInVector3.data(), queue3[queueIndex].data(), h);

            // Advance the queue index by 1 position
            // the new incoming element is now the "last" in the queue
            // For RGBA we just need one index, it's the same for each vector
            if (++queueIndex == queue0.size())
                queueIndex = 0;

            // Put into place the next incoming sums
            juce::FloatVectorOperations::add (stackSumVector0.data(), sumInVector0.data(), h);
            juce::FloatVectorOperations::add (stackSumVector1.data(), sumInVector1.data(), h);
            juce::FloatVectorOperations::add (stackSumVector2.data(), sumInVector2.data(), h);
            juce::FloatVectorOperations::add (stackSumVector3.data(), sumInVector3.data(), h);

            // Add the current center pixel to sumOutVector
            auto middleIndex = (queueIndex + radius) % queue0.size();
            juce::FloatVectorOperations::add (sumOutVector0.data(), queue0[middleIndex].data(), h);
            juce::FloatVectorOperations::add (sumOutVector1.data(), queue1[middleIndex].data(), h);
            juce::FloatVectorOperations::add (sumOutVector2.data(), queue2[middleIndex].data(), h);
            juce::FloatVectorOperations::add (sumOutVector3.data(), queue3[middleIndex].data(), h);

            // *remove* the new center pixel from sumInVector
            juce::FloatVectorOperations::subtract (sumInVector0.data(), queue0[middleIndex].data(), h);
            juce::FloatVectorOperations::subtract (sumInVector1.data(), queue1[middleIndex].data(), h);
            juce::FloatVectorOperations::subtract (sumInVector2.data(), queue2[middleIndex].data(), h);
            juce::FloatVectorOperations::subtract (sumInVector3.data(), queue3[middleIndex].data(), h);
        }

        // VERTICAL PASS: this does all columns at once (ie, an entire row at once), progressing from top to bottom
        // This pass is VERY nicely optimized for vectors, since it uses data.pixelStride
        // clear our reusable vectors first
        memset (stackSumVector0.data(), 0, (size_t) w);
        memset (stackSumVector1.data(), 0, (size_t) w);
        memset (stackSumVector2.data(), 0, (size_t) w);
        memset (stackSumVector3.data(), 0, (size_t) w);
        memset (sumInVector0.data(), 0, (size_t) w);
        memset (sumInVector1.data(), 0, (size_t) w);
        memset (sumInVector2.data(), 0, (size_t) w);
        memset (sumInVector3.data(), 0, (size_t) w);
        memset (sumOutVector0.data(), 0, (size_t) w);
        memset (sumOutVector1.data(), 0, (size_t) w);
        memset (sumOutVector2.data(), 0, (size_t) w);
        memset (sumOutVector3.data(), 0, (size_t) w);
        queueIndex = 0;

        // populate our temp vector with float values of the topmost pixels
        // this *could* be vectorized easily, but we'd need to do a conversion step somehow
        for (size_t i = 0; i < static_cast<size_t>(w); ++i)
        {
            tempPixelVector0[i] = (float) data.getLinePointer (0)[i * data.pixelStride];
            tempPixelVector1[i] = (float) data.getLinePointer (0)[i * data.pixelStride + 1];
            tempPixelVector2[i] = (float) data.getLinePointer (0)[i * data.pixelStride + 2];
            tempPixelVector3[i] = (float) data.getLinePointer (0)[i * data.pixelStride + 3];
        }

        // Now pre-fill the left half of the queue with the topmost pixel values
        // (queue is already initialized from the horizontal pass)
        for (size_t i = 0; i <= static_cast<size_t>(radius); ++i)
        {
            // these init left side AND middle of the stack
            juce::FloatVectorOperations::copy (queue0[i].data(), tempPixelVector0.data(), w);
            juce::FloatVectorOperations::copy (queue1[i].data(), tempPixelVector1.data(), w);
            juce::FloatVectorOperations::copy (queue2[i].data(), tempPixelVector2.data(), w);
            juce::FloatVectorOperations::copy (queue3[i].data(), tempPixelVector3.data(), w);
            juce::FloatVectorOperations::add (sumOutVector0.data(), tempPixelVector0.data(), w);
            juce::FloatVectorOperations::add (sumOutVector1.data(), tempPixelVector1.data(), w);
            juce::FloatVectorOperations::add (sumOutVector2.data(), tempPixelVector2.data(), w);
            juce::FloatVectorOperations::add (sumOutVector3.data(), tempPixelVector3.data(), w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector0.data(), tempPixelVector0.data(), (float) i + 1, w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector1.data(), tempPixelVector1.data(), (float) i + 1, w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector2.data(), tempPixelVector2.data(), (float) i + 1, w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector3.data(), tempPixelVector3.data(), (float) i + 1, w);
        }

        // Fill the right half of the queue with pixel values from the next rows
        for (auto i = 1; i <= radius; ++i)
        {
            if (i <= h - 1)
            {
                for (size_t col = 0; col < (size_t) w; ++col)
                {
                    tempPixelVector0[col] = (float) data.getLinePointer (i)[col * data.pixelStride];
                    tempPixelVector1[col] = (float) data.getLinePointer (i)[col * data.pixelStride + 1];
                    tempPixelVector2[col] = (float) data.getLinePointer (i)[col * data.pixelStride + 2];
                    tempPixelVector3[col] = (float) data.getLinePointer (i)[col * data.pixelStride + 3];
                }
            }
            // edge case where queue is bigger than image width!
            // for example vertical test where width = 1
            else
            {
                for (size_t col = 0; col < (size_t) w; ++col)
                {
                    tempPixelVector0[col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride];
                    tempPixelVector1[col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride + 1];
                    tempPixelVector2[col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride + 2];
                    tempPixelVector3[col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride + 3];
                }
            }

            juce::FloatVectorOperations::copy (queue0[radius + i].data(), tempPixelVector0.data(), w);
            juce::FloatVectorOperations::copy (queue1[radius + i].data(), tempPixelVector1.data(), w);
            juce::FloatVectorOperations::copy (queue2[radius + i].data(), tempPixelVector2.data(), w);
            juce::FloatVectorOperations::copy (queue3[radius + i].data(), tempPixelVector3.data(), w);
            juce::FloatVectorOperations::add (sumInVector0.data(), tempPixelVector0.data(), w);
            juce::FloatVectorOperations::add (sumInVector1.data(), tempPixelVector1.data(), w);
            juce::FloatVectorOperations::add (sumInVector2.data(), tempPixelVector2.data(), w);
            juce::FloatVectorOperations::add (sumInVector3.data(), tempPixelVector3.data(), w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector0.data(), tempPixelVector0.data(), (float) (radius + 1 - i), w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector1.data(), tempPixelVector1.data(), (float) (radius + 1 - i), w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector2.data(), tempPixelVector2.data(), (float) (radius + 1 - i), w);
            juce::FloatVectorOperations::addWithMultiply (stackSumVector3.data(), tempPixelVector3.data(), (float) (radius + 1 - i), w);
        }

        for (auto y = 0; y < h; ++y)
        {
            // calculate the blurred value vector from the stack
            // it first goes in a temporary location...
            juce::FloatVectorOperations::copy (tempPixelVector0.data(), stackSumVector0.data(), w);
            juce::FloatVectorOperations::copy (tempPixelVector1.data(), stackSumVector1.data(), w);
            juce::FloatVectorOperations::copy (tempPixelVector2.data(), stackSumVector2.data(), w);
            juce::FloatVectorOperations::copy (tempPixelVector3.data(), stackSumVector3.data(), w);
            juce::FloatVectorOperations::multiply (tempPixelVector0.data(), divisor, w);
            juce::FloatVectorOperations::multiply (tempPixelVector1.data(), divisor, w);
            juce::FloatVectorOperations::multiply (tempPixelVector2.data(), divisor, w);
            juce::FloatVectorOperations::multiply (tempPixelVector3.data(), divisor, w);

            // ...before being placed back in our image data as uint8
            // manually iterate across the row here, no easy way to do this in juce
            for (size_t i = 0; i < static_cast<size_t>(w); ++i)
            {
                data.getLinePointer (y)[i * data.pixelStride] = (unsigned char) tempPixelVector0[i];
                data.getLinePointer (y)[i * data.pixelStride + 1] = (unsigned char) tempPixelVector1[i];
                data.getLinePointer (y)[i * data.pixelStride + 2] = (unsigned char) tempPixelVector2[i];
                data.getLinePointer (y)[i * data.pixelStride + 3] = (unsigned char) tempPixelVector3[i];
            }

            // remove the outgoing sum from the stack
            juce::FloatVectorOperations::subtract (stackSumVector0.data(), sumOutVector0.data(), w);
            juce::FloatVectorOperations::subtract (stackSumVector1.data(), sumOutVector1.data(), w);
            juce::FloatVectorOperations::subtract (stackSumVector2.data(), sumOutVector2.data(), w);
            juce::FloatVectorOperations::subtract (stackSumVector3.data(), sumOutVector3.data(), w);

            // remove the leftmost value from sumOutVector
            juce::FloatVectorOperations::subtract (sumOutVector0.data(), queue0[queueIndex].data(), w);
            juce::FloatVectorOperations::subtract (sumOutVector1.data(), queue1[queueIndex].data(), w);
            juce::FloatVectorOperations::subtract (sumOutVector2.data(), queue2[queueIndex].data(), w);
            juce::FloatVectorOperations::subtract (sumOutVector3.data(), queue3[queueIndex].data(), w);

            // grab the incoming value (or the bottom most pixel if we're near the bottom)
            // and stick it in our queue
            if (y + radius + 1 < h)
            {
                // grab pixels from each row, offset by x+radius+1
                for (size_t col = 0; col < (size_t) w; ++col)
                {
                    queue0[queueIndex][col] = (float) data.getLinePointer ((int) (y + radius + 1))[col * data.pixelStride];
                    queue1[queueIndex][col] = (float) data.getLinePointer ((int) (y + radius + 1))[col * data.pixelStride + 1];
                    queue2[queueIndex][col] = (float) data.getLinePointer ((int) (y + radius + 1))[col * data.pixelStride + 2];
                    queue3[queueIndex][col] = (float) data.getLinePointer ((int) (y + radius + 1))[col * data.pixelStride + 3];
                }
            }
            else
            {
                // bottom of image, grab bottom pixel
                for (size_t col = 0; col < (size_t) w; ++col)
                {
                    queue0[queueIndex][col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride];
                    queue1[queueIndex][col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride + 1];
                    queue2[queueIndex][col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride + 2];
                    queue3[queueIndex][col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride + 3];
                }
            }

            // Also add the incoming value to the sumInVector
            juce::FloatVectorOperations::add (sumInVector0.data(), queue0[queueIndex].data(), w);
            juce::FloatVectorOperations::add (sumInVector1.data(), queue1[queueIndex].data(), w);
            juce::FloatVectorOperations::add (sumInVector2.data(), queue2[queueIndex].data(), w);
            juce::FloatVectorOperations::add (sumInVector3.data(), queue3[queueIndex].data(), w);

            // Advance the queue index by 1 position
            // the new incoming element is now the "last" in the queue
            if (++queueIndex == queue0.size())
                queueIndex = 0;

            // Put into place the next incoming sums
            juce::FloatVectorOperations::add (stackSumVector0.data(), sumInVector0.data(), w);
            juce::FloatVectorOperations::add (stackSumVector1.data(), sumInVector1.data(), w);
            juce::FloatVectorOperations::add (stackSumVector2.data(), sumInVector2.data(), w);
            juce::FloatVectorOperations::add (stackSumVector3.data(), sumInVector3.data(), w);

            // Add the current center pixel to sumOutVector
            auto middleIndex = (queueIndex + radius) % queue0.size();
            juce::FloatVectorOperations::add (sumOutVector0.data(), queue0[middleIndex].data(), w);
            juce::FloatVectorOperations::add (sumOutVector1.data(), queue1[middleIndex].data(), w);
            juce::FloatVectorOperations::add (sumOutVector2.data(), queue2[middleIndex].data(), w);
            juce::FloatVectorOperations::add (sumOutVector3.data(), queue3[middleIndex].data(), w);

            // *remove* the new center pixel from sumInVector
            juce::FloatVectorOperations::subtract (sumInVector0.data(), queue0[middleIndex].data(), w);
            juce::FloatVectorOperations::subtract (sumInVector1.data(), queue1[middleIndex].data(), w);
            juce::FloatVectorOperations::subtract (sumInVector2.data(), queue2[middleIndex].data(), w);
            juce::FloatVectorOperations::subtract (sumInVector3.data(), queue3[middleIndex].data(), w);
        }
    }
}
