#pragma once
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin::stackBlur
{
    enum class Orientation {
        Horizontal,
        Vertical
    };

    template <Orientation orientation>
    inline static uint8_t* getPixel (juce::Image::BitmapData& data, int x, int y)
    {
        if constexpr (orientation == Orientation::Horizontal)
            return data.getPixelPointer (x, y);
        else
            return data.getPixelPointer (y, x);
    }

    template <Orientation orientation>
    inline static void templatedPass (juce::Image::BitmapData& data, size_t dimensionSize, int numberOfLines, size_t radius)
    {
        // The "queue" represents the current values within the sliding kernel's radius.
        // Audio people: yes, it's a circular buffer
        // It efficiently handles the sliding window nature of the kernel.
        std::vector<unsigned char> queue ((radius * 2) + 1);

        // This tracks the start of the circular buffer
        unsigned int queueIndex = 0;

        // the "stack" is all in our head, maaaan
        // think of it as a *weighted* sum of the values in the queue
        // each time the queue moves, we add the rightmost values of the queue to the stack
        // and remove the left values of the queue from the stack
        // the blurred pixel is then calculated by dividing by the number of "values" in the weighted stack sum
        unsigned int stackSum = 0;
        unsigned int sizeOfStack = (radius + 1) * (radius + 1);

        // Sum of values in the right half of the queue
        unsigned int sumIn = 0;

        // Sum of values in the left half of the queue
        unsigned long sumOut = 0;

        for (int lineNumber = 0; lineNumber < numberOfLines; ++lineNumber)
        {
            // clear everything
            stackSum = sumIn = sumOut = queueIndex = 0;

            // Pre-fill the left half of the queue with the leftmost pixel value
            for (auto i = 0u; i <= radius; ++i)
            {
                queue[i] = *getPixel<orientation> (data, 0, lineNumber);

                // these init left side AND middle of the stack
                sumOut += queue[i];
                stackSum += queue[i] * (i + 1);
            }

            // Fill the right half of the queue with the actual pixel values
            // zero is the center pixel here, it's added to the sum radius + 1 times
            for (auto i = 1u; i <= radius; ++i)
            {
                // edge case where queue is bigger than image width!
                // for example vertical test where width = 1
                uint8_t value;
                if (i <= dimensionSize - 1)
                    value = *getPixel<orientation> (data, i, lineNumber);
                else
                    value = *getPixel<orientation> (data, dimensionSize - 1, lineNumber);

                queue[radius + i] = value; // look ma, no bounds checkin'
                sumIn += value;
                stackSum += value * (radius + 1 - i);
            }

            for (auto pixel = 0u; pixel < dimensionSize; ++pixel)
            {
                auto nextIndex = pixel + radius + 1;
                uint8_t nextValue;
                if (nextIndex < dimensionSize)
                    nextValue = *getPixel<orientation> (data, nextIndex, lineNumber);
                else
                    nextValue = *getPixel<orientation> (data, dimensionSize - 1, lineNumber);

                // calculate the blurred value from the stack
                *getPixel<orientation> (data, pixel, lineNumber) = static_cast<unsigned char> (stackSum / sizeOfStack);

                // if the incoming pixel isn't going to alter our stack
                // move on without even advancing the queue
//                if (*getPixel<orientation> (data, pixel, lineNumber) == nextValue)
//                    continue;

                // remove the outgoing sum from the stack
                stackSum -= sumOut;

                // remove the leftmost value from sumOut
                sumOut -= queue[queueIndex];

                queue[queueIndex] = nextValue;

                // Also add the new incoming value to the sumIn
                sumIn += queue[queueIndex];

                // Advance the queue index by 1 position
                // the new incoming element is now the "last" in the queue
                if (++queueIndex == queue.size())
                    queueIndex = 0;

                // Put into place the next incoming sums
                stackSum += sumIn;

                // Add the current center pixel to sumOut
                auto middleIndex = (queueIndex + radius) % queue.size();
                sumOut += queue[middleIndex];

                // *remove* the new center pixel from sumIn
                sumIn -= queue[middleIndex];
            }
        }
    }

    static void singleChannelTemplated (juce::Image& img, unsigned int radius)
    {
        const auto w = static_cast<unsigned int> (img.getWidth());
        const auto h = static_cast<unsigned int> (img.getHeight());

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        // Ensure radius is within bounds
        radius = juce::jlimit (1u, 254u, radius);

        templatedPass<Orientation::Horizontal> (data, w, h, radius);
        templatedPass<Orientation::Vertical> (data, h, w, radius);
    }
}
