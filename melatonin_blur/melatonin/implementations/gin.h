/*==============================================================================

BSD 3-Clause License

Copyright (c) 2018, Roland Rabien
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

2023 by Sudara:
Excerpted for testing, benchmarking, and use of rgba algo on windows

==============================================================================*/

#pragma once

#include "juce_graphics/juce_graphics.h"

namespace melatonin::stackBlur
{
    const unsigned short stackblur_mul[255] = { 512, 512, 456, 512, 328, 456, 335, 512, 405, 328, 271, 456, 388, 335, 292, 512, 454, 405, 364, 328, 298, 271, 496, 456, 420, 388, 360, 335, 312, 292, 273, 512, 482, 454, 428, 405, 383, 364, 345, 328, 312, 298, 284, 271, 259, 496, 475, 456, 437, 420, 404, 388, 374, 360, 347, 335, 323, 312, 302, 292, 282, 273, 265, 512, 497, 482, 468, 454, 441, 428, 417, 405, 394, 383, 373, 364, 354, 345, 337, 328, 320, 312, 305, 298, 291, 284, 278, 271, 265, 259, 507, 496, 485, 475, 465, 456, 446, 437, 428, 420, 412, 404, 396, 388, 381, 374, 367, 360, 354, 347, 341, 335, 329, 323, 318, 312, 307, 302, 297, 292, 287, 282, 278, 273, 269, 265, 261, 512, 505, 497, 489, 482, 475, 468, 461, 454, 447, 441, 435, 428, 422, 417, 411, 405, 399, 394, 389, 383, 378, 373, 368, 364, 359, 354, 350, 345, 341, 337, 332, 328, 324, 320, 316, 312, 309, 305, 301, 298, 294, 291, 287, 284, 281, 278, 274, 271, 268, 265, 262, 259, 257, 507, 501, 496, 491, 485, 480, 475, 470, 465, 460, 456, 451, 446, 442, 437, 433, 428, 424, 420, 416, 412, 408, 404, 400, 396, 392, 388, 385, 381, 377, 374, 370, 367, 363, 360, 357, 354, 350, 347, 344, 341, 338, 335, 332, 329, 326, 323, 320, 318, 315, 312, 310, 307, 304, 302, 299, 297, 294, 292, 289, 287, 285, 282, 280, 278, 275, 273, 271, 269, 267, 265, 263, 261, 259 };
    const unsigned char stackblur_shr[255] = { 9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24 };

    static void ginSingleChannel (juce::Image& img, unsigned int radius)
    {
        const unsigned int w = (unsigned int) img.getWidth();
        const unsigned int h = (unsigned int) img.getHeight();

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        radius = juce::jlimit (1u, 254u, radius);

        unsigned char stack[(254 * 2 + 1) * 1];

        unsigned int x, y, xp, yp, i, sp, stack_start;

        unsigned char* stack_ptr = nullptr;
        unsigned char* src_ptr = nullptr;
        unsigned char* dst_ptr = nullptr;

        unsigned long sum, sum_in, sum_out;

        unsigned int wm = w - 1;
        unsigned int hm = h - 1;
        unsigned int w1 = (unsigned int) data.lineStride;
        unsigned int div = (unsigned int) (radius * 2) + 1;
        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];

        for (y = 0; y < h; ++y)
        {
            sum = sum_in = sum_out = 0;

            src_ptr = data.getLinePointer (int (y));

            for (i = 0; i <= radius; ++i)
            {
                stack_ptr = &stack[i];
                stack_ptr[0] = src_ptr[0];
                sum += src_ptr[0] * (i + 1);
                sum_out += src_ptr[0];
            }

            for (i = 1; i <= radius; ++i)
            {
                if (i <= wm)
                    src_ptr += 1;

                stack_ptr = &stack[1 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                sum += src_ptr[0] * (radius + 1 - i);
                sum_in += src_ptr[0];
            }

            sp = radius;
            xp = radius;
            if (xp > wm)
                xp = wm;

            src_ptr = data.getLinePointer (int (y)) + (unsigned int) data.pixelStride * xp;
            dst_ptr = data.getLinePointer (int (y));

            for (x = 0; x < w; ++x)
            {
                dst_ptr[0] = (unsigned char) ((sum * mul_sum) >> shr_sum);
                dst_ptr += 1;

                sum -= sum_out;

                stack_start = sp + div - radius;

                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[1 * stack_start];

                sum_out -= stack_ptr[0];

                if (xp < wm)
                {
                    src_ptr += 1;
                    ++xp;
                }

                stack_ptr[0] = src_ptr[0];

                sum_in += src_ptr[0];
                sum += sum_in;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 1];

                sum_out += stack_ptr[0];
                sum_in -= stack_ptr[0];
            }
        }

        for (x = 0; x < w; ++x)
        {
            sum = sum_in = sum_out = 0;

            src_ptr = data.getLinePointer (0) + (unsigned int) data.pixelStride * x;

            for (i = 0; i <= radius; ++i)
            {
                stack_ptr = &stack[i * 1];
                stack_ptr[0] = src_ptr[0];
                sum += src_ptr[0] * (i + 1);
                sum_out += src_ptr[0];
            }

            for (i = 1; i <= radius; ++i)
            {
                if (i <= hm)
                    src_ptr += w1;

                stack_ptr = &stack[1 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                sum += src_ptr[0] * (radius + 1 - i);
                sum_in += src_ptr[0];
            }

            sp = radius;
            yp = radius;
            if (yp > hm)
                yp = hm;

            src_ptr = data.getLinePointer (int (yp)) + (unsigned int) data.pixelStride * x;
            dst_ptr = data.getLinePointer (0) + (unsigned int) data.pixelStride * x;

            for (y = 0; y < h; ++y)
            {
                dst_ptr[0] = (unsigned char) ((sum * mul_sum) >> shr_sum);
                dst_ptr += w1;

                sum -= sum_out;

                stack_start = sp + div - radius;
                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[1 * stack_start];

                sum_out -= stack_ptr[0];

                if (yp < hm)
                {
                    src_ptr += w1;
                    ++yp;
                }

                stack_ptr[0] = src_ptr[0];

                sum_in += src_ptr[0];
                sum += sum_in;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 1];

                sum_out += stack_ptr[0];
                sum_in -= stack_ptr[0];
            }
        }
    }

    static void ginARGB (juce::Image& img, unsigned int radius)
    {
        const unsigned int w = (unsigned int) img.getWidth();
        const unsigned int h = (unsigned int) img.getHeight();

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        radius = juce::jlimit (2u, 254u, radius);

        unsigned char stack[(254 * 2 + 1) * 4];

        unsigned int x, y, xp, yp, i, sp, stack_start;

        unsigned char* stack_ptr = nullptr;
        unsigned char* src_ptr = nullptr;
        unsigned char* dst_ptr = nullptr;

        unsigned long sum_r, sum_g, sum_b, sum_a, sum_in_r, sum_in_g, sum_in_b, sum_in_a,
            sum_out_r, sum_out_g, sum_out_b, sum_out_a;

        unsigned int wm = w - 1;
        unsigned int hm = h - 1;
        unsigned int w4 = (unsigned int) data.lineStride;
        unsigned int div = (unsigned int) (radius * 2) + 1;
        unsigned int mul_sum = stackBlur::stackblur_mul[radius];
        unsigned char shr_sum = stackBlur::stackblur_shr[radius];

        for (y = 0; y < h; ++y)
        {
            sum_r = sum_g = sum_b = sum_a =
                sum_in_r = sum_in_g = sum_in_b = sum_in_a =
                    sum_out_r = sum_out_g = sum_out_b = sum_out_a = 0;

            src_ptr = data.getLinePointer (int (y));

            for (i = 0; i <= radius; ++i)
            {
                stack_ptr = &stack[4 * i];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];
                sum_r += src_ptr[0] * (i + 1);
                sum_g += src_ptr[1] * (i + 1);
                sum_b += src_ptr[2] * (i + 1);
                sum_a += src_ptr[3] * (i + 1);
                sum_out_r += src_ptr[0];
                sum_out_g += src_ptr[1];
                sum_out_b += src_ptr[2];
                sum_out_a += src_ptr[3];
            }

            for (i = 1; i <= radius; ++i)
            {
                if (i <= wm)
                    src_ptr += 4;

                stack_ptr = &stack[4 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];
                sum_r += src_ptr[0] * (radius + 1 - i);
                sum_g += src_ptr[1] * (radius + 1 - i);
                sum_b += src_ptr[2] * (radius + 1 - i);
                sum_a += src_ptr[3] * (radius + 1 - i);
                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_in_a += src_ptr[3];
            }

            sp = radius;
            xp = radius;
            if (xp > wm)
                xp = wm;

            src_ptr = data.getLinePointer (int (y)) + (unsigned int) data.pixelStride * xp;
            dst_ptr = data.getLinePointer (int (y));

            for (x = 0; x < w; ++x)
            {
                dst_ptr[0] = (unsigned char) ((sum_r * mul_sum) >> shr_sum);
                dst_ptr[1] = (unsigned char) ((sum_g * mul_sum) >> shr_sum);
                dst_ptr[2] = (unsigned char) ((sum_b * mul_sum) >> shr_sum);
                dst_ptr[3] = (unsigned char) ((sum_a * mul_sum) >> shr_sum);
                dst_ptr += 4;

                sum_r -= sum_out_r;
                sum_g -= sum_out_g;
                sum_b -= sum_out_b;
                sum_a -= sum_out_a;

                stack_start = sp + div - radius;

                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[4 * stack_start];

                sum_out_r -= stack_ptr[0];
                sum_out_g -= stack_ptr[1];
                sum_out_b -= stack_ptr[2];
                sum_out_a -= stack_ptr[3];

                if (xp < wm)
                {
                    src_ptr += 4;
                    ++xp;
                }

                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];

                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_in_a += src_ptr[3];
                sum_r += sum_in_r;
                sum_g += sum_in_g;
                sum_b += sum_in_b;
                sum_a += sum_in_a;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 4];

                sum_out_r += stack_ptr[0];
                sum_out_g += stack_ptr[1];
                sum_out_b += stack_ptr[2];
                sum_out_a += stack_ptr[3];
                sum_in_r -= stack_ptr[0];
                sum_in_g -= stack_ptr[1];
                sum_in_b -= stack_ptr[2];
                sum_in_a -= stack_ptr[3];
            }
        }

        for (x = 0; x < w; ++x)
        {
            sum_r = sum_g = sum_b = sum_a =
                sum_in_r = sum_in_g = sum_in_b = sum_in_a =
                    sum_out_r = sum_out_g = sum_out_b = sum_out_a = 0;

            src_ptr = data.getLinePointer (0) + (unsigned int) data.pixelStride * x;

            for (i = 0; i <= radius; ++i)
            {
                stack_ptr = &stack[i * 4];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];
                sum_r += src_ptr[0] * (i + 1);
                sum_g += src_ptr[1] * (i + 1);
                sum_b += src_ptr[2] * (i + 1);
                sum_a += src_ptr[3] * (i + 1);
                sum_out_r += src_ptr[0];
                sum_out_g += src_ptr[1];
                sum_out_b += src_ptr[2];
                sum_out_a += src_ptr[3];
            }

            for (i = 1; i <= radius; ++i)
            {
                if (i <= hm)
                    src_ptr += w4;

                stack_ptr = &stack[4 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];
                sum_r += src_ptr[0] * (radius + 1 - i);
                sum_g += src_ptr[1] * (radius + 1 - i);
                sum_b += src_ptr[2] * (radius + 1 - i);
                sum_a += src_ptr[3] * (radius + 1 - i);
                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_in_a += src_ptr[3];
            }

            sp = radius;
            yp = radius;
            if (yp > hm)
                yp = hm;

            src_ptr = data.getLinePointer (int (yp)) + (unsigned int) data.pixelStride * x;
            dst_ptr = data.getLinePointer (0) + (unsigned int) data.pixelStride * x;

            for (y = 0; y < h; ++y)
            {
                dst_ptr[0] = (unsigned char) ((sum_r * mul_sum) >> shr_sum);
                dst_ptr[1] = (unsigned char) ((sum_g * mul_sum) >> shr_sum);
                dst_ptr[2] = (unsigned char) ((sum_b * mul_sum) >> shr_sum);
                dst_ptr[3] = (unsigned char) ((sum_a * mul_sum) >> shr_sum);
                dst_ptr += w4;

                sum_r -= sum_out_r;
                sum_g -= sum_out_g;
                sum_b -= sum_out_b;
                sum_a -= sum_out_a;

                stack_start = sp + div - radius;
                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[4 * stack_start];

                sum_out_r -= stack_ptr[0];
                sum_out_g -= stack_ptr[1];
                sum_out_b -= stack_ptr[2];
                sum_out_a -= stack_ptr[3];

                if (yp < hm)
                {
                    src_ptr += w4;
                    ++yp;
                }

                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];

                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_in_a += src_ptr[3];
                sum_r += sum_in_r;
                sum_g += sum_in_g;
                sum_b += sum_in_b;
                sum_a += sum_in_a;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 4];

                sum_out_r += stack_ptr[0];
                sum_out_g += stack_ptr[1];
                sum_out_b += stack_ptr[2];
                sum_out_a += stack_ptr[3];
                sum_in_r -= stack_ptr[0];
                sum_in_g -= stack_ptr[1];
                sum_in_b -= stack_ptr[2];
                sum_in_a -= stack_ptr[3];
            }
        }
    }

    // these are sudara's old helpers
    static void renderDropShadow (juce::Graphics& g, const juce::Path& path, juce::Colour color, const int radius = 1, const juce::Point<int> offset = { 0, 0 }, int spread = 0)
    {
        if (radius < 1)
            return;

        auto area = (path.getBounds().getSmallestIntegerContainer() + offset)
                        .expanded (radius + spread + 1)
                        .getIntersection (g.getClipBounds().expanded (radius + spread + 1));

        if (area.getWidth() < 2 || area.getHeight() < 2)
            return;

        // spread enlarges or shrinks the path before blurring it
        auto spreadPath = juce::Path (path);
        if (spread != 0)
        {
            area.expand (spread, spread);
            auto bounds = path.getBounds().expanded (static_cast<float>(spread));
            spreadPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
        }

        juce::Image renderedPath (juce::Image::SingleChannel, area.getWidth(), area.getHeight(), true);

        juce::Graphics g2 (renderedPath);
        g2.setColour (juce::Colours::white);
        g2.fillPath ((spread != 0) ? spreadPath : path, juce::AffineTransform::translation ((float) (offset.x - area.getX()), (float) (offset.y - area.getY())));
        ginSingleChannel (renderedPath, static_cast<unsigned int> (radius));

        g.setColour (color);
        g.drawImageAt (renderedPath, area.getX(), area.getY(), true);
    }

    // sudara's old inner helper
    [[maybe_unused]] static void renderInnerShadow (juce::Graphics& g, juce::Path target, juce::Colour shadowColor, int radius = 1, juce::Point<int> offset = { 0, 0 }, int spread = 0)
    {
        // resets the Clip Region when this scope ends
        juce::Graphics::ScopedSaveState saveState (g);

        // invert the path's fill shape and enlarge it,
        // so it casts a shadow
        juce::Path shadowPath (target);
        shadowPath.setUsingNonZeroWinding (false);

        if (spread != 0)
        {
            // A positive spread radius means a smaller projected image (more inner shadow)
            auto bounds = shadowPath.getBounds().expanded (static_cast<float>(-spread));
            shadowPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
        }

        shadowPath.addRectangle (target.getBounds().expanded ((float) (10 + radius + spread)));

        // reduce clip region to avoid the shadow
        // being drawn outside of the shape to cast the shadow on
        g.reduceClipRegion (target);

        renderDropShadow (g, shadowPath, shadowColor, radius, offset);
    }
}
