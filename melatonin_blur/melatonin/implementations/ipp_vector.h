#pragma once
#include "juce_gui_basics/juce_gui_basics.h"

#pragma warning(push)
#pragma warning(disable: 4018 4267)

#if _IPP_SEQUENTIAL_STATIC

#include "ipp_vector.h"
#include <ipp.h>




#if IPP_VERSION_MAJOR >= 2021 && IPP_VERSION_MINOR >= 10
    #include <ipp/ippcore_tl.h>
    #include <ipp/ippcv.h>
    #include <ipp/ippi.h>
    #include <ipp/ippi_l.h>
    #include <ipp/ipps.h>
#else
    #include <ippcore_tl.h>
    #include <ippcv.h>
    #include <ippi.h>
    #include <ippi_l.h>
    #include <ipps.h>
#endif

// This is a port of the cross-platform stack blur implementation in tests/implementations/vector.h
// This one doesn't rely on melatonin::vector, only IPP
// It's consistently faster than any of IPP's own image implementations
// such as ippiFilterBorder, ippiFilterSeparableBorder, etc
namespace melatonin::blur
{
    template <typename T>
    class IPPAligned
    {
    public:
        // required by the interface
        using value_type = T;

        // needs to be defined so nested vectors work
        template <typename U>
        struct rebind
        {
            using other = IPPAligned<U>;
        };

        IPPAligned() = default;
        IPPAligned (const IPPAligned&) = default;

        template <typename U>
        explicit IPPAligned (const IPPAligned<U>&)
        {
        }

        value_type* allocate (std::size_t numObjects)
        {
            return (value_type*) ippsMalloc_8u (numObjects * sizeof (value_type));
        }

        void deallocate (value_type* p, std::size_t)
        {
            ippsFree (p);
        }
    };

    static void inline ippVectorSingleChannel (juce::Image& img, unsigned int radius)
    {
        const int w = static_cast<int> (img.getWidth());
        const int h = static_cast<int> (img.getHeight());

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

        // we're going to be using vectors, allocated once
        // and used for both horizontal and vertical passes
        // so we need to know the max size we need
        int vectorSize = std::max (w, h);

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

        // use standard allocator for outside vector, inside vectors are aligned
        std::vector<std::vector<float, IPPAligned<float>>> queue;
        for (auto i = 0u; i < radius * 2 + 1; ++i)
        {
            queue.emplace_back (vectorSize, 0);
        }

        // one sum for each column
        std::vector<float, IPPAligned<float>> stackSumVector (vectorSize, 0);

        // Sum of values in the right half of the queue
        std::vector<float, IPPAligned<float>> sumInVector (vectorSize, 0);

        // Sum of values in the left half of the queue
        std::vector<float, IPPAligned<float>> sumOutVector (vectorSize, 0);

        // little helper for prefilling and conversion
        std::vector<float, IPPAligned<float>> tempPixelVector (vectorSize, 0);

        // HORIZONTAL PASS — all rows at once, progressing to the right
        // Time to fill our vector with columns of floats
        // vdsp pretty much only works with floats
        // our "source vector" is uint8_t
        // single channel means the pixel "stride" is 1, but we're populating with columns first
        // so it's data.lineStride which is "12" for a 10 pixel wide single channel image

        // to start with, populate a fresh vector with float values of the leftmost pixels
        // A 255 uint8 value will literally become 255.0f
        for (auto i = 0; i < h; ++i)
            tempPixelVector[(uint8_t) i] = (float) data.getLinePointer (i)[0];

        // Now pre-fill the left half of the queue with this leftmost pixel value
        for (auto i = 0u; i <= radius; ++i)
        {
            // these initialize the left side AND middle of the stack
            ippsCopy_32f (tempPixelVector.data(), queue[i].data(), h);
            ippsAdd_32f_I (tempPixelVector.data(), sumOutVector.data(), h);
            ippsAddProductC_32f (tempPixelVector.data(), (float) i + 1, stackSumVector.data(), h);
        }

        // Fill the right half of the queue with the next pixel values
        // zero is the center pixel here, it was added above (radius + 1) times to the sum
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

            ippsCopy_32f (tempPixelVector.data(), queue[radius + i].data(), h);
            ippsAdd_32f_I (tempPixelVector.data(), sumInVector.data(), h);
            ippsAddProductC_32f (tempPixelVector.data(), (float) (radius + 1 - i), stackSumVector.data(), h);
        }

        for (auto x = 0u; x < w; ++x)
        {
            // calculate the blurred value from the stack
            // it first goes in a temporary location...
            ippsDivC_32f (stackSumVector.data(), sizeOfStack, tempPixelVector.data(), h);

            // ...before being placed back in our image data as uint8
            for (auto i = 0; i < h; ++i)
                data.getLinePointer (i)[(size_t) data.pixelStride * x] = (unsigned char) tempPixelVector[(size_t) i];

            // remove the outgoing sum from the stack
            ippsSub_32f_I (sumOutVector.data(), stackSumVector.data(), h);

            // remove the leftmost value from sumOutVector
            ippsSub_32f_I (queue[queueIndex].data(), sumOutVector.data(), h);

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
            ippsAdd_32f_I (queue[queueIndex].data(), sumInVector.data(), h);

            // Advance the queue index by 1 position
            // the new incoming element is now the "last" in the queue
            if (++queueIndex == queue.size())
                queueIndex = 0;

            // Put into place the next incoming sums
            ippsAdd_32f_I (sumInVector.data(), stackSumVector.data(), h);

            // Add the current center pixel to sumOutVector
            auto middleIndex = (queueIndex + radius) % queue.size();
            ippsAdd_32f_I (queue[middleIndex].data(), sumOutVector.data(), h);

            // *remove* the new center pixel from sumInVector
            ippsSub_32f_I (queue[middleIndex].data(), sumInVector.data(), h);
        }

        // VERTICAL PASS: this does all columns at once, progressing to bottom
        // This pass is nicely optimized for vectors, since it uses data.pixelStride
        // clear everything
        ippsZero_32f (stackSumVector.data(), w);
        ippsZero_32f (sumInVector.data(), w);
        ippsZero_32f (sumOutVector.data(), w);
        queueIndex = 0;

        // populate our temp vector with float values of the topmost pixels
        // this can be a straight convert since we're not jumping image rows
        // or dealing with multi-channel right now
        ippsConvert_8u32f (data.getLinePointer (0), tempPixelVector.data(), w);

        // Now pre-fill the left half of the queue with the topmost pixel values
        // queue is already populated from the horizontal pass
        for (auto i = 0u; i <= radius; ++i)
        {
            // these init left side AND middle of the stack
            ippsCopy_32f (tempPixelVector.data(), queue[i].data(), w);
            ippsAdd_32f_I (tempPixelVector.data(), sumOutVector.data(), w);
            ippsAddProductC_32f (tempPixelVector.data(), (float) i + 1, stackSumVector.data(), w);
        }

        // Fill the right half of the queue with pixel values from the next rows
        for (auto i = 1u; i <= radius; ++i)
        {
            if (i <= h - 1)
            {
                ippsConvert_8u32f (data.getLinePointer (i), tempPixelVector.data(), w);
            }
            // edge case where queue is bigger than image width!
            // for example vertical test where width = 1
            else
            {
                ippsConvert_8u32f (data.getLinePointer (h - 1), tempPixelVector.data(), w);
            }

            ippsCopy_32f (tempPixelVector.data(), queue[radius + i].data(), w);
            ippsAdd_32f_I (tempPixelVector.data(), sumInVector.data(), w);
            ippsAddProductC_32f (tempPixelVector.data(), (float) (radius + 1 - i), stackSumVector.data(), w);
        }

        for (auto y = 0u; y < h; ++y)
        {
            // calculate the blurred value vector from the stack
            // it first goes in a temporary location...
            ippsDivC_32f (stackSumVector.data(), sizeOfStack, tempPixelVector.data(), w);

            // ...before being placed back in our image data as uint8
            ippsConvert_32f8u_Sfs (tempPixelVector.data(), data.getLinePointer (y), w, ippRndNear, 0);

            // remove the outgoing sum from the stack
            ippsSub_32f_I (sumOutVector.data(), stackSumVector.data(), w);

            // remove the leftmost value from sumOutVector
            ippsSub_32f_I (queue[queueIndex].data(), sumOutVector.data(), w);

            // grab the incoming value (or the bottom most pixel if we're near the bottom)
            // and stick it in our queue
            if (y + radius + 1 < h)
            {
                // grab pixels from each row, offset by x+radius+1
                ippsConvert_8u32f (data.getLinePointer (y + radius + 1), queue[queueIndex].data(), w);
            }
            else
            {
                ippsConvert_8u32f (data.getLinePointer (h - 1), queue[queueIndex].data(), w);
            }

            // Also add the incoming value to the sumInVector
            ippsAdd_32f_I (queue[queueIndex].data(), sumInVector.data(), w);

            // Advance the queue index by 1 position
            // the new incoming element is now the "last" in the queue
            if (++queueIndex == queue.size())
                queueIndex = 0;

            // Put into place the next incoming sums
            ippsAdd_32f_I (sumInVector.data(), stackSumVector.data(), w);

            // Add the current center pixel to sumOutVector
            auto middleIndex = (queueIndex + radius) % queue.size();
            ippsAdd_32f_I (queue[middleIndex].data(), sumOutVector.data(), w);

            // *remove* the new center pixel from sumInVector
            ippsSub_32f_I (queue[middleIndex].data(), sumInVector.data(), w);
        }
    }

    static void inline ippVectorARGB (juce::Image& img, unsigned int radius)
    {
        const int w = static_cast<int> (img.getWidth());
        const int h = static_cast<int> (img.getHeight());

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

        // we're going to be using vectors, allocated once
        // and used for both horizontal and vertical passes
        // so we need to know the max size we need
        int vectorSize = std::max (w, h);

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

        // use standard allocator for outside vector, inside vectors are aligned
        std::vector<std::vector<float, IPPAligned<float>>> queue;
        for (auto i = 0u; i < radius * 2 + 1; ++i)
        {
            queue.emplace_back (vectorSize, 0);
        }

        // one sum for each column
        std::vector<float, IPPAligned<float>> stackSumVector (vectorSize, 0);

        // Sum of values in the right half of the queue
        std::vector<float, IPPAligned<float>> sumInVector (vectorSize, 0);

        // Sum of values in the left half of the queue
        std::vector<float, IPPAligned<float>> sumOutVector (vectorSize, 0);

        // little helper for prefilling and conversion
        std::vector<float, IPPAligned<float>> tempPixelVector (vectorSize, 0);

        // loop around horizontal pass 4x, once for each channel
        // this is faster than allocating 4x and doing channels inline
        for (auto channel = 0; channel < 4; ++channel)
        {
            // HORIZONTAL PASS — all rows at once, progressing to the right
            // Time to fill our vector with columns of floats
            // vdsp pretty much only works with floats
            // our "source vector" is uint8_t
            // single channel means the pixel "stride" is 1, but we're populating with columns first
            // so it's data.lineStride which is "12" for a 10 pixel wide single channel image

            // to start with, populate a fresh vector with float values of the leftmost pixels
            // A 255 uint8 value will literally become 255.0f
            for (auto i = 0; i < h; ++i)
                tempPixelVector[(uint8_t) i] = (float) data.getLinePointer (i)[channel];

            // Now pre-fill the left half of the queue with this leftmost pixel value
            for (auto i = 0u; i <= radius; ++i)
            {
                // these initialize the left side AND middle of the stack
                ippsCopy_32f (tempPixelVector.data(), queue[i].data(), h);
                ippsAdd_32f_I (tempPixelVector.data(), sumOutVector.data(), h);
                ippsAddProductC_32f (tempPixelVector.data(), (float) i + 1, stackSumVector.data(), h);
            }

            // Fill the right half of the queue with the next pixel values
            // zero is the center pixel here, it was added above (radius + 1) times to the sum
            for (auto i = 1u; i <= radius; ++i)
            {
                if (i <= w - 1)
                {
                    for (int row = 0; row < h; ++row)
                        tempPixelVector[(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * i + channel];
                }
                // edge case where queue is bigger than image width!
                // for example in our vertical tests where width=1
                else
                {
                    for (int row = 0; row < h; ++row)
                        tempPixelVector[(size_t) row] = data.getLinePointer (row)[data.pixelStride * (w - 1) + channel];
                }

                ippsCopy_32f (tempPixelVector.data(), queue[radius + i].data(), h);
                ippsAdd_32f_I (tempPixelVector.data(), sumInVector.data(), h);
                ippsAddProductC_32f (tempPixelVector.data(), (float) (radius + 1 - i), stackSumVector.data(), h);
            }

            // main loop of horizontal pass
            for (auto x = 0u; x < w; ++x)
            {
                // calculate the blurred value from the stack
                // it first goes in a temporary location...
                ippsDivC_32f (stackSumVector.data(), sizeOfStack, tempPixelVector.data(), h);

                // ...before being placed back in our image data as uint8
                for (auto i = 0; i < h; ++i)
                    data.getLinePointer (i)[(size_t) data.pixelStride * x + channel] = (unsigned char) tempPixelVector[(size_t) i];

                // remove the outgoing sum from the stack
                ippsSub_32f_I (sumOutVector.data(), stackSumVector.data(), h);

                // remove the leftmost value from sumOutVector
                ippsSub_32f_I (queue[queueIndex].data(), sumOutVector.data(), h);

                // Conveniently, after advancing the index of a circular buffer
                // the old "start" (aka queueIndex) will be the new "end",
                // grab the new uint8 vals, stick it in temp buffer
                if (x + radius + 1 < w)
                {
                    // grab incoming pixels for each row (they are offset by x+radius+1)
                    for (int row = 0; row < h; ++row)
                        queue[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * (x + radius + 1) + channel];
                }
                else
                {
                    for (int row = 0; row < h; ++row)
                        queue[queueIndex][(size_t) row] = data.getLinePointer (row)[(size_t) data.pixelStride * ((size_t) w - 1) + channel];
                }

                // Also add the incoming value to the sumInVector
                ippsAdd_32f_I (queue[queueIndex].data(), sumInVector.data(), h);

                // Advance the queue index by 1 position
                // the new incoming element is now the "last" in the queue
                if (++queueIndex == queue.size())
                    queueIndex = 0;

                // Put into place the next incoming sums
                ippsAdd_32f_I (sumInVector.data(), stackSumVector.data(), h);

                // Add the current center pixel to sumOutVector
                auto middleIndex = (queueIndex + radius) % queue.size();
                ippsAdd_32f_I (queue[middleIndex].data(), sumOutVector.data(), h);

                // *remove* the new center pixel from sumInVector
                ippsSub_32f_I (queue[middleIndex].data(), sumInVector.data(), h);
            }

            // cleanup after each channel
            ippsZero_32f (stackSumVector.data(), w);
            ippsZero_32f (sumInVector.data(), w);
            ippsZero_32f (sumOutVector.data(), w);
            queueIndex = 0;
        }

        // loop around each vertical pass 4x, once for each channel
        for (auto channel = 0; channel < 4; ++channel)
        {
            // VERTICAL PASS: this does all columns at once, progressing to bottom
            // This pass is nicely optimized for vectors, since it uses data.pixelStride

            // populate our temp vector with float values of the topmost pixels
            // this must be a manual loop
            for (auto i = 0; i < w; ++i)
                tempPixelVector[(uint8_t) i] = (float) data.getLinePointer (0)[i * data.pixelStride + channel];

            // Now pre-fill the left half of the queue with the topmost pixel values
            // queue is already populated from the horizontal pass
            for (auto i = 0u; i <= radius; ++i)
            {
                // these init left side AND middle of the stack
                ippsCopy_32f (tempPixelVector.data(), queue[i].data(), w);
                ippsAdd_32f_I (tempPixelVector.data(), sumOutVector.data(), w);
                ippsAddProductC_32f (tempPixelVector.data(), (float) i + 1, stackSumVector.data(), w);
            }

            // Fill the right half of the queue with pixel values from the next rows
            for (auto i = 1u; i <= radius; ++i)
            {
                if (i <= h - 1)
                {
                    for (auto col = 0; col < w; ++col)
                        tempPixelVector[(uint8_t) col] = (float) data.getLinePointer (i)[col * data.pixelStride + channel];
                }
                // edge case where queue is bigger than image width!
                // for example vertical test where width = 1
                else
                {
                    for (auto col = 0; col < w; ++col)
                        tempPixelVector[(uint8_t) col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride + channel];
                }

                ippsCopy_32f (tempPixelVector.data(), queue[radius + i].data(), w);
                ippsAdd_32f_I (tempPixelVector.data(), sumInVector.data(), w);
                ippsAddProductC_32f (tempPixelVector.data(), (float) (radius + 1 - i), stackSumVector.data(), w);
            }

            for (auto y = 0u; y < h; ++y)
            {
                // calculate the blurred value vector from the stack
                // it first goes in a temporary location...
                ippsDivC_32f (stackSumVector.data(), sizeOfStack, tempPixelVector.data(), w);

                // ...before being placed back in our image data as uint8
                for (auto col = 0u; col < w; ++col)
                {
                    data.getLinePointer (y)[col * data.pixelStride + channel] = (unsigned char) tempPixelVector[col];
                }

                // remove the outgoing sum from the stack
                ippsSub_32f_I (sumOutVector.data(), stackSumVector.data(), w);

                // remove the leftmost value from sumOutVector
                ippsSub_32f_I (queue[queueIndex].data(), sumOutVector.data(), w);

                // grab the incoming value (or the bottom most pixel if we're near the bottom)
                // and stick it in our queue
                if (y + radius + 1 < h)
                {
                    // grab pixels from each row, offset by x+radius+1
                    for (size_t col = 0; col < (size_t) w; ++col)
                        queue[queueIndex][col] = (float) data.getLinePointer ((int) (y + radius + 1))[col * data.pixelStride + channel];
                }
                else
                {
                    // we're at the bottom of image, grab bottom row
                    for (size_t col = 0; col < (size_t) w; ++col)
                        queue[queueIndex][col] = (float) data.getLinePointer (h - 1)[col * data.pixelStride + channel];
                }

                // Also add the incoming value to the sumInVector
                ippsAdd_32f_I (queue[queueIndex].data(), sumInVector.data(), w);

                // Advance the queue index by 1 position
                // the new incoming element is now the "last" in the queue
                if (++queueIndex == queue.size())
                    queueIndex = 0;

                // Put into place the next incoming sums
                ippsAdd_32f_I (sumInVector.data(), stackSumVector.data(), w);

                // Add the current center pixel to sumOutVector
                auto middleIndex = (queueIndex + radius) % queue.size();
                ippsAdd_32f_I (queue[middleIndex].data(), sumOutVector.data(), w);

                // *remove* the new center pixel from sumInVector
                ippsSub_32f_I (queue[middleIndex].data(), sumInVector.data(), w);
            }
            // cleanup before next chan
            ippsZero_32f (stackSumVector.data(), w);
            ippsZero_32f (sumInVector.data(), w);
            ippsZero_32f (sumOutVector.data(), w);
            queueIndex = 0;
        }
    }
}

#endif

#pragma warning(pop)
