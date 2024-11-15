/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

#include "avir/avir.h"

namespace gin {

#if JUCE_INTEL
 #define GIN_USE_SSE 1
#endif

//==============================================================================
// Run a for loop split between each core.
// for (int i = 0; i < 10; i++) becomes multiThreadedFor (0, 10, 1, threadPool, [&] (int i) {});
// Make sure each iteration of the loop is independant

void multiThreadedFor(int start, int end, int interval, juce::ThreadPool* threadPool, std::function<void(int idx)> callback)
{
	if (threadPool == nullptr)
	{
		for (int i = start; i < end; i += interval)
			callback(i);
	}
	else
	{
		int num = threadPool->getNumThreads();

		juce::WaitableEvent wait;
		juce::Atomic<int> threadsRunning(num);

		for (int i = 0; i < num; i++)
		{
			threadPool->addJob([i, &callback, &wait, &threadsRunning, start, end, interval, num]
				{
					for (int j = start + interval * i; j < end; j += interval * num)
						callback(j);

					int stillRunning = --threadsRunning;
					if (stillRunning == 0)
						wait.signal();
				});
		}

		wait.wait();
	}
}

template <typename T>
inline uint8_t toByte (T v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return uint8_t (v);
}

inline uint8_t getIntensity (uint8_t r, uint8_t g, uint8_t b)
{
    return (uint8_t)((7471 * b + 38470 * g + 19595 * r) >> 16);
}

inline uint8_t computeAlpha (uint8_t la, uint8_t ra)
{
    return (uint8_t)(((la * (256 - (ra + (ra >> 7)))) >> 8) + ra);
}

template <class T>
inline T blend (const T& c1, const T& c2)
{
    int a = c1.getAlpha();
    int invA = 255 - a;

    int r = ((c2.getRed()   * invA) + (c1.getRed()   * a)) / 256;
    int g = ((c2.getGreen() * invA) + (c1.getGreen() * a)) / 256;
    int b = ((c2.getBlue()  * invA) + (c1.getBlue()  * a)) / 256;
    uint8_t a2 = computeAlpha (c2.getAlpha(), c1.getAlpha());

    T res;
    res.setARGB (a2, toByte (r), toByte (g), toByte (b));
    return res;
}

template <class T1, class T2>
inline T2 convert (const T1& in)
{
    T2 out;
    out.setARGB (in.getAlpha(), in.getRed(), in.getGreen(), in.getBlue());
    return out;
}

//==============================================================================
template <class T>
void applyVignette (juce::Image& img, float amountIn, float radiusIn, float fallOff, juce::ThreadPool* threadPool)
{
	jassertfalse;

#if 0
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    double outA = w * 0.5 * radiusIn;
    double outB = h * 0.5 * radiusIn;

    double inA = outA * fallOff;
    double inB = outB * fallOff;

    double cx = w * 0.5;
    double cy = h * 0.5;

    double amount = 1.0 - amountIn;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    Ellipse<double> outE { outA, outB };
    Ellipse<double> inE  { inA,  inB  };

    multiThreadedFor (0, h, 1, threadPool, [&] (int y)
    {
        uint8_t* p = data.getLinePointer (y);

        double dy = y - cy;

        for (int x = 0; x < w; x++)
        {
            double dx = x - cx;

            bool outside = outE.isPointOutside ({dx, dy});
            bool inside  = inE.isPointInside ({dx, dy});

            T* s = (T*)p;

            if (outside)
            {
                uint8_t r = toByte (0.5 + (s->getRed() * amount));
                uint8_t g = toByte (0.5 + (s->getGreen() * amount));
                uint8_t b = toByte (0.5 + (s->getBlue() * amount));
                uint8_t a = s->getAlpha();

                s->setARGB (a, r, g, b);
            }
            else if (! inside)
            {
                double angle = std::atan2 (dy, dx);

                auto p1 = outE.pointAtAngle (angle);
                auto p2 = inE.pointAtAngle (angle);

                auto l1 = juce::Line<double> ({dx,dy}, p2);
                auto l2 = juce::Line<double> (p1, p2);

                double factor = 1.0 - (amountIn * juce::jlimit (0.0, 1.0, l1.getLength() / l2.getLength()));

                uint8_t r = toByte (0.5 + (s->getRed()   * factor));
                uint8_t g = toByte (0.5 + (s->getGreen() * factor));
                uint8_t b = toByte (0.5 + (s->getBlue()  * factor));
                uint8_t a = s->getAlpha();

                s->setARGB (a, r, g, b);
            }

            p += data.pixelStride;
        }
    });
#endif
}

void applyVignette (juce::Image& img, float amountIn, float radiusIn, float fallOff, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyVignette<juce::PixelARGB> (img, amountIn, radiusIn, fallOff, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyVignette<juce::PixelRGB>  (img, amountIn, radiusIn, fallOff, threadPool);
    else jassertfalse;
}

template <class T>
void applySepia (juce::Image& img, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    multiThreadedFor (0, h, 1, threadPool, [&] (int y)
    {
        uint8_t* p = data.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            juce::PixelARGB* s = (juce::PixelARGB*)p;

            uint8_t r = s->getRed();
            uint8_t g = s->getGreen();
            uint8_t b = s->getBlue();
            uint8_t a = s->getAlpha();

            uint8_t ro = toByte ((r * .393) + (g *.769) + (b * .189));
            uint8_t go = toByte ((r * .349) + (g *.686) + (b * .168));
            uint8_t bo = toByte ((r * .272) + (g *.534) + (b * .131));

            s->setARGB (a, ro, go, bo);

            p += data.pixelStride;
        }
    });
}

void applySepia (juce::Image& img, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applySepia<juce::PixelARGB> (img, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applySepia<juce::PixelRGB>  (img, threadPool);
    else jassertfalse;
}



template <class T>
void applyGreyScale (juce::Image& img, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
    {
        uint8_t* p = data.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            T* s = (T*)p;

            uint8_t r = s->getRed();
            uint8_t g = s->getGreen();
            uint8_t b = s->getBlue();
            uint8_t a = s->getAlpha();

            uint8_t ro = toByte (r * 0.30 + 0.5);
            uint8_t go = toByte (g * 0.59 + 0.5);
            uint8_t bo = toByte (b * 0.11 + 0.5);

            s->setARGB (a,
                        toByte (ro + go + bo),
                        toByte (ro + go + bo),
                        toByte (ro + go + bo));

            p += data.pixelStride;
        }
    });
}

void applyGreyScale (juce::Image& img, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyGreyScale<juce::PixelARGB> (img, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyGreyScale<juce::PixelRGB>  (img, threadPool);
    else jassertfalse;
}

template <class T>
void applySoften (juce::Image& img, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image dst (img.getFormat(), w, h, true);

    juce::Image::BitmapData srcData (img, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::writeOnly);

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
    {
        for (int x = 0; x < w; x++)
        {
            int ro = 0, go = 0, bo = 0;
            uint8_t a = 0;

            for (int m = -1; m <= 1; m++)
            {
                for (int n = -1; n <= 1; n++)
                {
                    int cx = juce::jlimit<int> (0, w - 1, x + m);
                    int cy = juce::jlimit<int> (0, h - 1, y + n);

                    T* s = (T*) srcData.getPixelPointer (cx, cy);

                    ro += s->getRed();
                    go += s->getGreen();
                    bo += s->getBlue();
                }
            }

            T* s = (T*) srcData.getPixelPointer (x, y);
            a = s->getAlpha();

            T* d = (T*) dstData.getPixelPointer (x, y);

            d->setARGB (a, toByte (ro / 9), toByte (go / 9), toByte (bo / 9));
        }
    });
    img = dst;
}

void applySoften (juce::Image& img, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applySoften<juce::PixelARGB> (img, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applySoften<juce::PixelRGB>  (img, threadPool);
    else jassertfalse;
}

template <class T>
void applySharpen (juce::Image& img, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image dst (img.getFormat(), w, h, true);

    juce::Image::BitmapData srcData (img, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::writeOnly);

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
    {
        for (int x = 0; x < w; x++)
        {
            auto getPixelPointer = [&] (int cx, int cy) -> T*
            {
                cx = juce::jlimit<int>(0, w - 1, cx);
                cy = juce::jlimit<int>(0, h - 1, cy);

                return (T*) srcData.getPixelPointer (cx, cy);
            };

            int ro = 0, go = 0, bo = 0;
            uint8_t ao = 0;

            T* s = getPixelPointer (x, y);

            ro = s->getRed()   * 5;
            go = s->getGreen() * 5;
            bo = s->getBlue()  * 5;
            ao = s->getAlpha();

            s = getPixelPointer (x, y - 1);
            ro -= s->getRed();
            go -= s->getGreen();
            bo -= s->getBlue();

            s = getPixelPointer (x - 1, y);
            ro -= s->getRed();
            go -= s->getGreen();
            bo -= s->getBlue();

            s = getPixelPointer (x + 1, y);
            ro -= s->getRed();
            go -= s->getGreen();
            bo -= s->getBlue();

            s = getPixelPointer (x, y + 1);
            ro -= s->getRed();
            go -= s->getGreen();
            bo -= s->getBlue();

            T* d = (T*) dstData.getPixelPointer (x, y);

            d->setARGB (ao, toByte (ro), toByte (go), toByte (bo));
        }
    });
    img = dst;
}

void applySharpen (juce::Image& img, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applySharpen<juce::PixelARGB> (img, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applySharpen<juce::PixelRGB>  (img, threadPool);
    else jassertfalse;
}

template <class T>
void applyGamma (juce::Image& img, float gamma, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
    {
        uint8_t* p = data.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            T* s = (T*)p;

            uint8_t r = s->getRed();
            uint8_t g = s->getGreen();
            uint8_t b = s->getBlue();
            uint8_t a = s->getAlpha();

            uint8_t ro = toByte (std::pow (r / 255.0, gamma) * 255.0 + 0.5);
            uint8_t go = toByte (std::pow (g / 255.0, gamma) * 255.0 + 0.5);
            uint8_t bo = toByte (std::pow (b / 255.0, gamma) * 255.0 + 0.5);

            s->setARGB (a, ro, go, bo);

            p += data.pixelStride;
        }
    });
}

void applyGamma (juce::Image& img, float gamma, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyGamma<juce::PixelARGB> (img, gamma, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyGamma<juce::PixelRGB>  (img, gamma, threadPool);
    else jassertfalse;
}

template <class T>
void applyInvert (juce::Image& img, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
    {
        uint8_t* p = data.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            T* s = (T*)p;

            uint8_t r = s->getRed();
            uint8_t g = s->getGreen();
            uint8_t b = s->getBlue();
            uint8_t a = s->getAlpha();

            uint8_t ro = 255 - r;
            uint8_t go = 255 - g;
            uint8_t bo = 255 - b;

            s->setARGB (a, ro, go, bo);

            p += data.pixelStride;
        }
    });
}

void applyInvert (juce::Image& img, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyInvert<juce::PixelARGB> (img, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyInvert<juce::PixelRGB>  (img, threadPool);
    else jassertfalse;
}

template <class T>
void applyContrast (juce::Image& img, float contrast, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    contrast = (100.0f + contrast) / 100.0f;
    contrast = contrast * contrast;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
    {
        uint8_t* p = data.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            T* s = (T*)p;

            uint8_t r = s->getRed();
            uint8_t g = s->getGreen();
            uint8_t b = s->getBlue();
            uint8_t a = s->getAlpha();

            double ro = (double) r / 255.0;
            ro = ro - 0.5;
            ro = ro * contrast;
            ro = ro + 0.5;
            ro = ro * 255.0;

            double go = (double) g / 255.0;
            go = go - 0.5;
            go = go * contrast;
            go = go + 0.5;
            go = go * 255.0;

            double bo = (double) b / 255.0;
            bo = bo - 0.5;
            bo = bo * contrast;
            bo = bo + 0.5;
            bo = bo * 255.0;

            ro = toByte (ro);
            go = toByte (go);
            bo = toByte (bo);

            s->setARGB (a, toByte (ro), toByte (go), toByte (bo));

            p += data.pixelStride;
        }
    });
}

void applyContrast (juce::Image& img, float contrast, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyContrast<juce::PixelARGB> (img, contrast, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyContrast<juce::PixelRGB>  (img, contrast, threadPool);
    else jassertfalse;
}

template <class T>
void applyBrightnessContrast (juce::Image& img, float brightness, float contrast, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    double multiply = 1;
    double divide = 1;

    if (contrast < 0)
    {
        multiply = contrast + 100;
        divide = 100;
    }
    else if (contrast > 0)
    {
        multiply = 100;
        divide = 100 - contrast;
    }
    else
    {
        multiply = 1;
        divide = 1;
    }

    uint8_t* rgbTable = new uint8_t[65536];

    if (divide == 0)
    {
        for (int intensity = 0; intensity < 256; intensity++)
        {
            if (intensity + brightness < 128)
                rgbTable[intensity] = 0;
            else
                rgbTable[intensity] = 255;
        }
    }
    else if (divide == 100)
    {
        for (int intensity = 0; intensity < 256; intensity++)
        {
            int shift = int ((intensity - 127) * multiply / divide + 127 - intensity + brightness);

            for (int col = 0; col < 256; col++)
            {
                int index = (intensity * 256) + col;
                rgbTable[index] = toByte (col + shift);
            }
        }
    }
    else
    {
        for (int intensity = 0; intensity < 256; intensity++)
        {
            int shift = int ((intensity - 127 + brightness) * multiply / divide + 127 - intensity);

            for (int col = 0; col < 256; col++)
            {
                int index = (intensity * 256) + col;
                rgbTable[index] = toByte (col + shift);
            }
        }
    }

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
    {
        uint8_t* p = data.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            T* s = (T*)p;

            uint8_t r = s->getRed();
            uint8_t g = s->getGreen();
            uint8_t b = s->getBlue();
            uint8_t a = s->getAlpha();

            if (divide == 0)
            {
                int i = getIntensity (toByte (r), toByte (g), toByte (b));
                uint8_t c = rgbTable[i];

                s->setARGB (a, c, c, c);
            }
            else
            {
                int i = getIntensity (toByte (r), toByte (g), toByte (b));
                int shiftIndex = i * 256;

                uint8_t ro = rgbTable[shiftIndex + r];
                uint8_t go = rgbTable[shiftIndex + g];
                uint8_t bo = rgbTable[shiftIndex + b];

                ro = toByte (ro);
                go = toByte (go);
                bo = toByte (bo);

                s->setARGB (a, ro, go, bo);
            }

            p += data.pixelStride;
        }
    });

    delete[] rgbTable;
}

void applyBrightnessContrast (juce::Image& img, float brightness, float contrast, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyBrightnessContrast<juce::PixelARGB> (img, brightness, contrast, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyBrightnessContrast<juce::PixelRGB>  (img, brightness, contrast, threadPool);
    else jassertfalse;
}

template <class T>
void applyHueSaturationLightness (juce::Image& img, float hueIn, float saturation, float lightness, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    if (saturation > 100)
        saturation = ((saturation - 100) * 3) + 100;
    saturation = (saturation * 1024) / 100;

    hueIn /= 360.0f;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
    {
        uint8_t* p = data.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            T* s = (T*)p;

            uint8_t r = s->getRed();
            uint8_t g = s->getGreen();
            uint8_t b = s->getBlue();
            uint8_t a = s->getAlpha();

            int intensity = getIntensity (toByte (r), toByte (g), toByte (b));
            int ro = toByte (int (intensity * 1024 + (r - intensity) * saturation) >> 10);
            int go = toByte (int (intensity * 1024 + (g - intensity) * saturation) >> 10);
            int bo = toByte (int (intensity * 1024 + (b - intensity) * saturation) >> 10);

            juce::Colour c (toByte (ro), toByte (go), toByte (bo));
            float hue = c.getHue();
            hue += hueIn;

            while (hue < 0.0f)  hue += 1.0f;
            while (hue >= 1.0f) hue -= 1.0f;

            c = juce::Colour::fromHSV (hue, c.getSaturation(), c.getBrightness(), float (a));
            ro = c.getRed();
            go = c.getGreen();
            bo = c.getBlue();

            ro = toByte (ro);
            go = toByte (go);
            bo = toByte (bo);

            s->setARGB (a, toByte (ro), toByte (go), toByte (bo));

            if (lightness > 0)
            {
                auto blended = blend (juce::PixelARGB (toByte ((lightness * 255) / 100 * (a / 255.0)), 255, 255, 255), convert<T, juce::PixelARGB> (*s));
                *s = convert<juce::PixelARGB, T> (blended);
            }
            else if (lightness < 0)
            {
                auto blended = blend (juce::PixelARGB (toByte ((-lightness * 255) / 100 * (a / 255.0)), 0, 0, 0), convert<T, juce::PixelARGB> (*s));
                *s = convert<juce::PixelARGB, T> (blended);
            }

            p += data.pixelStride;
        }
    });
}

void applyHueSaturationLightness (juce::Image& img, float hue, float saturation, float lightness, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyHueSaturationLightness<juce::PixelARGB> (img, hue, saturation, lightness, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyHueSaturationLightness<juce::PixelRGB>  (img, hue, saturation, lightness, threadPool);
    else jassertfalse;
}

juce::Image applyResize (const juce::Image& src, int width, int height)
{
    juce::Image dst (src.getFormat(), width, height, true);

    juce::Image::BitmapData srcData (src, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readWrite);

    int channels = 0;
    if (src.getFormat() == juce::Image::ARGB)               channels = 4;
    else if (src.getFormat() == juce::Image::RGB)           channels = 3;
    else if (src.getFormat() == juce::Image::SingleChannel) channels = 1;
    else                                                    return {};

    // JUCE images may have padding at the end of each scan line.
    // Avir expects the image data to be packed. So we need to
    // pack and unpack the image data before and after resizing.
    juce::HeapBlock<uint8_t> srcPacked (src.getWidth() * src.getHeight() * channels);
    juce::HeapBlock<uint8_t> dstPacked (dst.getWidth() * dst.getHeight() * channels);

    uint8_t* rawSrc = srcPacked.getData();
    uint8_t* rawDst = dstPacked.getData();

    for (int y = 0; y < src.getHeight(); y++)
        memcpy (rawSrc + y * src.getWidth() * channels,
                srcData.getLinePointer (y),
                (size_t) (src.getWidth() * channels));

   #if 0 && GIN_USE_SSE
    avir::CImageResizer<avir::fpclass_float4> imageResizer (8);
    imageResizer.resizeImage (rawSrc, src.getWidth(), src.getHeight(), 0,
                                rawDst, dst.getWidth(), dst.getHeight(), channels, 0);
   #else
    avir::CImageResizer<> imageResizer (8);
    imageResizer.resizeImage (rawSrc, src.getWidth(), src.getHeight(), 0,
                                    rawDst, dst.getWidth(), dst.getHeight(), channels, 0);
   #endif

    for (int y = 0; y < dst.getHeight(); y++)
        memcpy (dstData.getLinePointer (y),
                rawDst + y * dst.getWidth() * channels,
                (size_t) (dst.getWidth() * channels));

    return dst;
}

juce::Image applyResize (const juce::Image& src, float factor)
{
    return applyResize (src,
                        juce::roundToInt (factor * src.getWidth()),
                        juce::roundToInt (factor * src.getHeight()));
}

template <class T>
void applyGradientMap (juce::Image& img, const juce::ColourGradient& gradient, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    multiThreadedFor (0, h, 1, threadPool, [&] (int y)
                           {
                               uint8_t* p = data.getLinePointer (y);

                               for (int x = 0; x < w; x++)
                               {
                                   T* s = (T*)p;

                                   uint8_t r = s->getRed();
                                   uint8_t g = s->getGreen();
                                   uint8_t b = s->getBlue();
                                   uint8_t a = s->getAlpha();

                                   uint8_t ro = toByte (r * 0.30 + 0.5);
                                   uint8_t go = toByte (g * 0.59 + 0.5);
                                   uint8_t bo = toByte (b * 0.11 + 0.5);

                                   float proportion = float (ro + go + bo) / 256.0f;

                                   auto c = gradient.getColourAtPosition (proportion);

                                   s->setARGB (a,
                                               c.getRed(),
                                               c.getGreen(),
                                               c.getBlue());

                                   p += data.pixelStride;
                               }
                           });
}

void applyGradientMap (juce::Image& img, const juce::ColourGradient& gradient, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyGradientMap<juce::PixelARGB> (img, gradient, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyGradientMap<juce::PixelRGB>  (img, gradient, threadPool);
    else jassertfalse;
}

void applyGradientMap (juce::Image& img, const juce::Colour c1, const juce::Colour c2, juce::ThreadPool* threadPool)
{
    juce::ColourGradient g;
    g.addColour (0.0, c1);
    g.addColour (1.0, c2);

    applyGradientMap (img, g, threadPool);
}

template <class T>
void applyColour (juce::Image& img, juce::Colour c, juce::ThreadPool* threadPool)
{
    const int w = img.getWidth();
    const int h = img.getHeight();
    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    uint8_t r = c.getRed();
    uint8_t g = c.getGreen();
    uint8_t b = c.getBlue();
    uint8_t a = c.getAlpha();

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

    multiThreadedFor (0, h, 1, threadPool, [&] (int y)
                           {
                               uint8_t* p = data.getLinePointer (y);

                               for (int x = 0; x < w; x++)
                               {
                                   T* s = (T*)p;
                                   s->setARGB (a, r, g, b);
                                   p += data.pixelStride;
                               }
                           });
}

void applyColour (juce::Image& img, juce::Colour c, juce::ThreadPool* threadPool)
{
    if (img.getFormat() == juce::Image::ARGB)          applyColour<juce::PixelARGB> (img, c, threadPool);
    else if (img.getFormat() == juce::Image::RGB)      applyColour<juce::PixelRGB>  (img, c, threadPool);
    else jassertfalse;
}

}
