#pragma once
#include "Accelerate/Accelerate.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "vImage.h"

namespace melatonin::blur
{
    static inline void vImageARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
        jassert (srcImage.getFormat() == juce::Image::PixelFormat::ARGB);

        auto kernel = createFloatKernel (radius);

        const auto w = (unsigned int) srcImage.getWidth();
        const auto h = (unsigned int) srcImage.getHeight();
        juce::Image::BitmapData srcData (srcImage, juce::Image::BitmapData::readWrite);
        juce::Image::BitmapData dstData (dstImage, juce::Image::BitmapData::readWrite);

        // vImageSepConvolve isn't happy operating in-place
        vImage_Buffer src = { srcData.getLinePointer (0), h, w, (size_t) srcData.lineStride };
        vImage_Buffer dst = { dstData.getLinePointer (0), h, w, (size_t) dstData.lineStride };
        
        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wunguarded-availability-new")
        vImageSepConvolve_ARGB8888 (&src, &dst, nullptr, 0, 0, kernel.data(), (unsigned int) kernel.size(), kernel.data(), (unsigned int) kernel.size(), 0, Pixel_8888 { 0, 0, 0, 0 }, kvImageEdgeExtend);
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE
    }
}
