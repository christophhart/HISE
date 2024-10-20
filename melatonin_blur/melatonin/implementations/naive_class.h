#pragma once
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin
{
    class NaiveStackBlur
    {
    public:
        /*
         * Efficiently reuse the same code for horizontal and vertical passes
         * Tried several different approaches to this
         * But ultimately I think class members result in poorer cache locality
         */
        enum class Orientation {
            Horizontal,
            Vertical
        };

        NaiveStackBlur (juce::Image& i, unsigned int r) : radius (r), data (i, juce::Image::BitmapData::readWrite)
        {
            singleChannel();
        }

        inline void singleChannel()
        {
            auto height = (size_t) data.height;
            auto width = (size_t) data.width;
            stackBlurPass<Orientation::Horizontal> (width, height);
            stackBlurPass<Orientation::Vertical> (height, width);
        }

    private:
        const uint8_t radius;
        juce::Image::BitmapData data;

        // aka our "divisor"
        // can go above uint8_t (since the radius can be larger)
        const unsigned int sizeOfStack = (radius + 1) * (radius + 1);

        // The "queue" represents the current values within the sliding kernel's radius.
        std::vector<uint8_t> queue = std::vector<uint8_t> ((radius * 2) + 1);

        // This tracks the start of the circular buffer
        uint8_t queueIndex = 0;

        unsigned long stackSum = 0;

        // Sum of values in the right half of the queue
        unsigned long sumIn = 0;

        // Sum of values in the left half of the queue
        unsigned long sumOut = 0;

        // A stack blur is a horizontal and then a vertical pass over the image data
        template <Orientation orientation>
        inline void stackBlurPass (size_t dimensionSize, size_t numberOfLines)
        {
            for (auto lineNumber = 0u; lineNumber < numberOfLines; ++lineNumber)
            {
                // clear everything
                stackSum = sumIn = sumOut = queueIndex = 0;

                // Pre-fill the left half of the queue with the leftmost pixel value
                for (auto i = 0u; i <= radius; ++i)
                {
                    auto value = *getPixel<orientation> (lineNumber, 0);
                    queue[i] = value;

                    // these init left side AND middle of the stack
                    sumOut += value;
                    stackSum += value * (i + 1);
                }

                // Fill the right half of the queue with actual pixel values
                // zero is the center pixel here, it's added to the sum radius + 1 times
                for (auto i = 1u; i <= radius; ++i)
                {
                    // edge case where queue is bigger than image width!
                    // for example vertical test where width = 1
                    if (i <= dimensionSize - 1)
                        queue[radius + i] = *getPixel<orientation> (lineNumber, i);
                    else
                        queue[radius + i] = *getPixel<orientation> (lineNumber, dimensionSize - 1);

                    sumIn += queue[radius + i];
                    stackSum += queue[radius + i] * (radius + 1 - i);
                }

                for (auto pixel = 0u; pixel < dimensionSize; ++pixel)
                {
                    // calculate the blurred value from the stack
                    *getPixel<orientation> (lineNumber, pixel) = static_cast<uint8_t> (stackSum / sizeOfStack);

                    // remove the outgoing sum from the stack
                    stackSum -= sumOut;

                    // remove the leftmost value from sumOut
                    sumOut -= queue[queueIndex];

                    // Conveniently, after advancing the index of a circular buffer
                    // the old "start" (aka queueIndex) will be the new "end",
                    // This means we can just replace it with the incoming pixel value
                    // if we hit the right edge, use the rightmost pixel value
                    auto nextIndex = pixel + radius + 1;
                    if (nextIndex < dimensionSize)
                        queue[queueIndex] = *getPixel<orientation> (lineNumber, nextIndex);
                    else
                        queue[queueIndex] = *getPixel<orientation> (lineNumber, dimensionSize - 1);

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

        template <Orientation orientation>
        inline uint8_t* getPixel (int lineNumber, int pixelNumber)
        {
            if constexpr (orientation == Orientation::Horizontal)
                return &data.getLinePointer (lineNumber)[pixelNumber];
            else
                return data.getPixelPointer (lineNumber, pixelNumber);
        }
    };
}
