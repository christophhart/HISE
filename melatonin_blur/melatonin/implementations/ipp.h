#pragma once

// graveyard of failed IPP attempts (they were too slow)

#if false

    // tried this strategy to avoid loops in the ipp implementation
    // results were worse!
    // could be due to line stride, which needs investigating
    static void convertToFloatFirst
    {
        // rather than converting each row/col into floats, do it once upfront
        std::vector<Ipp32f, IPPAligned<float>> srcFloats (data.width * data.height);
        ippiConvert_8u32f_C1R (data.getLinePointer (0), (int) data.lineStride, srcFloats.data(), data.width * sizeof (Ipp32f), { data.width, data.height });

        // make conversion easy
        int pixelStride = sizeof (Ipp32f);
        int lineStride = data.width * pixelStride;
        IppiSize oneColumn = { 1, data.height }; // these are pixel sizes
        IppiSize oneRow = { data.width, 1 };

        ippiCopy_32f_C1R(&srcFloats[i], lineStride, &tempPixelVector[0], sizeof (Ipp32f), oneColumn);
        ippiCopy_32f_C1R(&srcFloats[x+radius+1], lineStride, &queue[queueIndex][0], sizeof (Ipp32f), oneColumn);
    };


    // decent with lower radii, sux on higher radii
    // plus only 3 channel... didn't pass correctness, wolud have to convert to 3 chan first
    static inline void ippGaussian (juce::Image& img, size_t radius)
    {
            IppiSize roiSize = { data.width, data.height };
            int specSize = 0;
            int tempBufferSize = 0;
            Ipp8u borderValue = 0;
            ippiFilterGaussianGetBufferSize (roiSize, radius * 2 + 1, ipp8u, 3, &specSize, &tempBufferSize);
            auto* pSpec = (IppFilterGaussianSpec*) ippsMalloc_8u (specSize);
            auto* pBuffer = ippsMalloc_8u (tempBufferSize);
            ippiFilterGaussianInit (roiSize, radius * 2 + 1, 10, ippBorderRepl, ipp8u, 3, pSpec, pBuffer);
            ippiFilterGaussianBorder_8u_C3R(data.getLinePointer (0), (int) data.lineStride, data.getLinePointer (0), (int) data.lineStride, roiSize, &borderValue, pSpec, pBuffer);
    }

    // Used for IPP flavors of blur
    static inline std::vector<Ipp16s> createIntegerKernel (size_t radius)
    {
        // The kernel size is always odd
        size_t kernelSize = radius * 2 + 1;

        std::vector<Ipp16s> kernel (kernelSize);

        // Manufacture the stack blur-esque kernel
        // For example, for radius of 2:
        // 1/9 2/9 3/9 2/9 1/9
        for (size_t i = 0; i < kernelSize; ++i)
        {
            auto distance = (size_t) std::abs ((int) i - (int) radius);
            kernel[i] = radius + 1 - distance;
        }

        return kernel;
    }

  // this method was too slow with larger radii -- i'm assuming it's actually a 2D kernel
    static inline void seperableFilterSingleChannel (juce::Image& img, size_t radius)
    {
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();
        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        auto kernel = createFloatKernel (radius);

        IppiSize roiSize = { data.width, data.height };

        // first, we need the image as floats
        std::vector<Ipp32f> srcFloats (data.width * data.height);
        ippiConvert_8u32f_C1R (data.getLinePointer (0), (int) data.lineStride, srcFloats.data(), data.width * sizeof (Ipp32f), roiSize);

        int specSize;
        int bufferSize; // out variables in 2023, lets goooo

        // docs specify this should be the "rectangular" kernel size, which is sketchy
        // as we have 2 dimensions, but each is a vector...
        // so, for whatever reason, we have to pretend we have a 2D kernel here
        IppiSize kernelSize = { (int) kernel.size(), (int) kernel.size() };

        // init filter
        ippiFilterSeparableGetSpecSize (kernelSize, ipp32f, 1, &specSize);
        auto pSpec = (IppiFilterSeparableSpec*) ippsMalloc_8u (specSize);
        ippiFilterSeparableInit_32f (kernel.data(), kernel.data(), kernelSize, ipp32f, 1, pSpec);

        // init buffer
        ippiFilterSeparableGetBufferSize (roiSize, kernelSize, ipp32f, ipp32f, 1, &bufferSize);
        auto* pBuffer = ippsMalloc_8u (bufferSize);

        // Intel's API: Now Level 99 cryptic!
        // C1 = 1 channel
        // R = Region of Interest (vs. whole image)
        // L = platform specific, aka 64-bit (why of course!)
        ippiFilterSeparable_32f_C1R (srcFloats.data(), roiSize.width * sizeof (ipp32f), srcFloats.data(), roiSize.width * sizeof (ipp32f), roiSize, ippBorderRepl, 0, pSpec, pBuffer);

        // copy the floats back to our uint8_t image
        // for now lets round to 0, and assume that's fastest and accurate enough
        ippiConvert_32f8u_C1R (srcFloats.data(), data.width * sizeof (Ipp32f), data.getLinePointer (0), data.lineStride, roiSize, ippRndNear);

        // I'll tell my kids about that day in 2023 I was forced to manually free memory
        ippsFree (pSpec);
        ippsFree (pBuffer);
    }

    static inline void filterBorder(juce::Image& img, size_t radius)
    {
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();
        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        const auto kernel = createIntegerKernel (radius);

        IppiSize roiSize = { data.width, data.height };
        int divisor = (radius + 1) * (radius + 1);
        IppiSize kernelSize = { (int) kernel.size(), 1 };

        // out variables in 2023, lets goooo
        int specSize;
        int bufferSize;

        // do intel's dirty laundry: init filter and temp buffer
        ippiFilterBorderGetSize (kernelSize, roiSize, ipp8u, ipp16s, 1, &specSize, &bufferSize);
        auto filterSpec = (IppiFilterBorderSpec*) ippsMalloc_8u (specSize);
        auto* pBuffer = ippsMalloc_8u (bufferSize);

        // No idea why this must be 16s, but there's an example for FilterBorder that uses this with 8u src and dst
        // https://www.intel.com/content/www/us/en/docs/ipp/developer-reference/2021-9/filterborder-002.html
        ippiFilterBorderInit_16s (kernel.data(), kernelSize, divisor, ipp8u, 1, ippRndZero, filterSpec);

        // horizontal pass
        ippiFilterBorderSetMode(ippAlgHintFast,0,filterSpec);
        ippiFilterBorder_8u_C1R (data.getLinePointer (0), data.lineStride, data.getLinePointer (0), data.lineStride, roiSize, ippBorderRepl, nullptr, filterSpec, pBuffer);

        // vertical pass
        kernelSize = { 1, (int) kernel.size() };
        ippiFilterBorderGetSize (kernelSize, roiSize, ipp8u, ipp16s, 1, &specSize, &bufferSize);
        auto filterSpec2 = (IppiFilterBorderSpec*) ippsMalloc_8u (specSize);
        auto* pBuffer2 = ippsMalloc_8u (bufferSize);
        ippiFilterBorderInit_16s (kernel.data(), kernelSize, divisor, ipp8u, 1, ippRndZero, filterSpec2);
        ippiFilterBorderSetMode(ippAlgHintFast,0,filterSpec2);
        ippiFilterBorder_8u_C1R(data.getLinePointer (0), data.lineStride, data.getLinePointer (0), data.lineStride, roiSize, ippBorderRepl, nullptr, filterSpec2, pBuffer2);

        // I'll tell my kids about that day in 2023 I was forced to manually free memory
        ippsFree (filterSpec);
        ippsFree (pBuffer);
        ippsFree (filterSpec2);
        ippsFree (pBuffer2);
    }

    // fastest out of the IPP methods
    static void floatFilterBorder (juce::Image& img, size_t radius)
    {
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();
        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        // FilterRowBorderPipeline doesn't operate on ARGB.
        // FilterSeparable is slow, I think it's secretly 2D?
        // Intel states there are FilterBorder optimized branches for h=1 and w=1
        // So we'll do two passes of that
        // https://community.intel.com/t5/Intel-Integrated-Performance/Can-t-find-FilterSeparable-function/m-p/1164075

        const auto kernel = createFloatKernel (radius);
        IppiSize roiSize = { data.width, data.height };
        IppiSize kernelSize = { (int) kernel.size(), 1 };

        // work with floats for our image so that vector operations are fast
        std::vector<Ipp32f> srcFloats (data.width * data.height);
        ippiConvert_8u32f_C1R (data.getLinePointer (0), (int) data.lineStride, srcFloats.data(), data.width * sizeof (Ipp32f), roiSize);

        // out variables in 2023, lets goooo
        int specSize;
        int bufferSize;

        // do intel's dirty laundry: init filter and temp buffer
        ippiFilterBorderGetSize (kernelSize, roiSize, ipp32f, ipp32f, 1, &specSize, &bufferSize);
        auto filterSpec = (IppiFilterBorderSpec*) ippsMalloc_8u (specSize);
        auto* pBuffer = ippsMalloc_8u (bufferSize);

        ippiFilterBorderInit_32f (kernel.data(), kernelSize, ipp32f, 1, ippRndNear, filterSpec);

        // horizontal pass
        ippiFilterBorder_32f_C1R (srcFloats.data(), roiSize.width * sizeof (ipp32f), srcFloats.data(), roiSize.width * sizeof (ipp32f), roiSize, ippBorderRepl, nullptr, filterSpec, pBuffer);

        // vertical pass
        kernelSize = { 1, (int) kernel.size() };
        ippiFilterBorderGetSize (kernelSize, roiSize, ipp32f, ipp32f, 1, &specSize, &bufferSize);
        ippsFree (filterSpec);
        filterSpec = (IppiFilterBorderSpec*) ippsMalloc_8u (specSize);
        ippiFilterBorderInit_32f (kernel.data(), kernelSize, ipp32f, 1, ippRndNear, filterSpec);
        ippiFilterBorder_32f_C1R(srcFloats.data(), roiSize.width * sizeof (ipp32f), srcFloats.data(), roiSize.width * sizeof (ipp32f), roiSize, ippBorderRepl, nullptr, filterSpec, pBuffer);

        // copy the floats back to our uint8_t image
        // for now lets round to 0, and assume that's fastest and accurate enough
        ippiFilterBorderSetMode(ippAlgHintFast,0,filterSpec);
        ippiConvert_32f8u_C1R (srcFloats.data(), data.width * sizeof (Ipp32f), data.getLinePointer (0), data.lineStride, roiSize, ippRndNear);

        // I'll tell my kids about that day in 2023 I was forced to manually free memory
        ippsFree (filterSpec);
        ippsFree (pBuffer);
    }
#endif
