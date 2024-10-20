#pragma once
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin::stackBlur
{
    // It's hard to find a clear example of this algorithm online
    // since they all have various "crusty" optimizations
    // This one is optimized for readability and simplicity
     static void dequeueSingleChannel (juce::Image& img, unsigned int radius)
    {
        const auto w = static_cast<unsigned int> (img.getWidth());
        const auto h = static_cast<unsigned int> (img.getHeight());

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        // Ensure radius is within bounds
        radius = juce::jlimit (1u, 254u, radius);

        // The "queue" represents the current values within the sliding kernel's radius.
        // Audio people: yes, it's a circular buffer
        // It efficiently handles the sliding window nature of the kernel.
        std::deque<unsigned char> queue;

        // the "stack" is all in our head, maaaan
        // think of it as a *weighted* sum of the values in the queue
        // each time the queue moves, we add the rightmost values of the queue to the stack
        // and remove the left values of the queue from the stack
        // the blurred pixel is then calculated by dividing by the number of "values" in the weighted stack sum
        unsigned long stackSum = 0;
        unsigned int sizeOfStack = (radius + 1) * (radius + 1);

        // Sum of values in the right half of the queue
        unsigned long sumIn = 0;

        // Sum of values in the left half of the queue
        unsigned long sumOut = 0;

        // horizontal pass
        for (auto y = 0u; y < h; ++y)
        {
            auto row = data.getLinePointer (static_cast<int> (y));

            // clear everything
            queue.clear();
            stackSum = sumIn = sumOut = 0;

            // Pre-fill the left half of the queue with the leftmost pixel value
            for (auto i = 0u; i <= radius; ++i)
            {
                queue.push_back (row[0]);

                // these init left side + middle of the stack
                sumOut += row[0];
                stackSum += row[0] * (i + 1);
            }

            // Fill the right half of the queue with the actual pixel values
            // zero is the center pixel here, it's added to the sum radius + 1 times
            for (auto i = 1u; i <= radius; ++i)
            {
                // edge case where queue is bigger than image width!
                // for example vertical test where width = 1
                if (i <= w - 1)
                    queue.push_back (row[i]);
                else
                    queue.push_back (row[w - 1]);

                sumIn += queue.back();
                stackSum += queue.back() * (radius + 1 - i);
            }

            for (auto x = 0u; x < w; ++x)
            {
                // calculate the blurred value from the stack
                auto blurredValue = stackSum / sizeOfStack;
                jassert (blurredValue <= 255);
                row[x] = static_cast<unsigned char> (blurredValue);

                // remove the outgoing sum from the stack
                stackSum -= sumOut;

                // remove the leftmost value from sumOut
                sumOut -= queue.front();
                queue.pop_front(); // would be nice if this actually returned the value! I miss ruby!

                // Now push the incoming value onto the queue
                // if we hit the right edge, use the rightmost value
                unsigned char incomingValue;
                if (x + radius + 1 < w)
                    incomingValue = row[x + radius + 1];
                else
                    incomingValue = row[w - 1];
                queue.push_back (incomingValue);

                // Add the incoming value to the sumIn
                sumIn += incomingValue;

                // Put into place the next incoming sums
                stackSum += sumIn;

                // Before we move the queue/kernel, add the current center pixel to sumOut
                sumOut += queue[radius];

                // *remove* the new center pixel (only after the window shifts)
                sumIn -= queue[radius];
            }
        }

        // vertical pass, loop around each column
        for (auto x = 0u; x < w; ++x)
        {
            // clear everything
            queue.clear();
            stackSum = sumIn = sumOut = 0;

            // Pre-fill the left half of the queue with the topmost pixel value
            auto topMostPixel = data.getLinePointer (0) + (unsigned int) data.pixelStride * x;
            for (auto i = 0u; i <= radius; ++i)
            {
                queue.push_back (topMostPixel[0]);

                // these init left side + middle of the stack
                sumOut += queue.back();
                stackSum += topMostPixel[0] * (i + 1);
            }

            // Fill the right half of the queue (excluding center!) with actual pixel values
            // zero is the topmost/center pixel here (it was added to the sum (radius + 1) times above)
            for (auto i = 1u; i <= radius; ++i)
            {
                if (i <= h - 1)
                {
                    auto pixel = data.getLinePointer (i) + (unsigned int) data.pixelStride * x;
                    queue.push_back (pixel[0]);
                }
                // edge case where queue is bigger than image height!
                // for example where width = 1
                else
                {
                    auto pixel = data.getLinePointer ((h - 1)) + (unsigned int) data.pixelStride * x;
                    queue.push_back (pixel[0]);
                }

                sumIn += queue.back();
                stackSum += queue.back() * (radius + 1 - i);
            }

            for (auto y = 0u; y < h; ++y)
            {
                // calculate the blurred value from the stack
                auto blurredValue = stackSum / sizeOfStack;
                jassert (blurredValue <= 255);
                auto blurredPixel = data.getLinePointer (static_cast<int> (y)) + (unsigned int) data.pixelStride * x;
                blurredPixel[0] = static_cast<unsigned char> (blurredValue);

                // remove outgoing sum from the stack
                stackSum -= sumOut;

                // start crafting the next sumOut by removing the leftmost value from sumOut
                sumOut -= queue.front();
                queue.pop_front(); // would be nice if this actually returned the value! I miss ruby!

                // Now push the incoming value onto the queue
                // if we hit the bottom edge, use the bottommost value
                unsigned char incomingValue;
                if (y + radius + 1 < h)
                    incomingValue = *data.getPixelPointer (static_cast<int> (x), static_cast<int> (y + radius + 1));
                else
                    incomingValue = *data.getPixelPointer (static_cast<int> (x), static_cast<int> (h - 1));
                queue.push_back (incomingValue);

                // Add the incoming value to the sumIn
                sumIn += incomingValue;

                // Put into place the next incoming sums
                stackSum += sumIn;

                // Before we move the queue/kernel, add the current center pixel to sumOut
                sumOut += queue[radius];

                // *remove* the new center pixel (only after the window shifts)
                sumIn -= queue[radius];
            }
        }
    }
}
