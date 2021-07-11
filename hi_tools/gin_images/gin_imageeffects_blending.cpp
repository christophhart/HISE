/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

namespace gin {

inline uint8_t channelBlendNormal (int A, int)          { return ((uint8_t)(A)); }
inline uint8_t channelBlendLighten (int A, int B)       { return ((uint8_t)((B > A) ? B : A)); }
inline uint8_t channelBlendDarken (int A, int B)        { return ((uint8_t)((B > A) ? A : B)); }
inline uint8_t channelBlendMultiply (int A, int B)      { return ((uint8_t)((A * B) / 255)); }
inline uint8_t channelBlendAverage (int A, int B)       { return ((uint8_t)((A + B) / 2)); }
inline uint8_t channelBlendAdd (int A, int B)           { return ((uint8_t)(juce::jmin (255, (A + B)))); }
inline uint8_t channelBlendSubtract (int A, int B)      { return ((uint8_t)((A + B < 255) ? 0 : (A + B - 255))); }
inline uint8_t channelBlendDifference (int A, int B)    { return ((uint8_t)(std::abs (A - B))); }
inline uint8_t channelBlendNegation (int A, int B)      { return ((uint8_t)(255 - std::abs (255 - A - B))); }
inline uint8_t channelBlendScreen (int A, int B)        { return ((uint8_t)(255 - (((255 - A) * (255 - B)) >> 8))); }
inline uint8_t channelBlendExclusion (int A, int B)     { return ((uint8_t)(A + B - 2 * A * B / 255)); }
inline uint8_t channelBlendOverlay (int A, int B)       { return ((uint8_t)((B < 128) ? (2 * A * B / 255) : (255 - 2 * (255 - A) * (255 - B) / 255))); }
inline uint8_t channelBlendSoftLight (int A, int B)     { return ((uint8_t)((B < 128) ? (2 * ((A >> 1) + 64)) * ((float)B / 255) : (255 - (2 * (255 - ((A >> 1) + 64)) * (float)(255 - B) / 255)))); }
inline uint8_t channelBlendHardLight (int A, int B)     { return (channelBlendOverlay (B,A)); }
inline uint8_t channelBlendColorDodge (int A, int B)    { return ((uint8_t)((B == 255) ? B : juce::jmin (255, ((A << 8 ) / (255 - B))))); }
inline uint8_t channelBlendColorBurn (int A, int B)     { return ((uint8_t)((B == 0) ? B : juce::jmax (0, (255 - ((255 - A) << 8 ) / B)))); }
inline uint8_t channelBlendLinearDodge (int A, int B)   { return (channelBlendAdd (A, B)); }
inline uint8_t channelBlendLinearBurn (int A, int B)    { return (channelBlendSubtract (A, B)); }
inline uint8_t channelBlendLinearLight (int A, int B)   { return ((uint8_t)(B < 128) ? channelBlendLinearBurn (A,(2 * B)) : channelBlendLinearDodge (A, (2 * (B - 128)))); }
inline uint8_t channelBlendVividLight (int A, int B)    { return ((uint8_t)(B < 128) ? channelBlendColorBurn (A,(2 * B)) : channelBlendColorDodge (A, (2 * (B - 128)))); }
inline uint8_t channelBlendPinLight (int A, int B)      { return ((uint8_t)(B < 128) ? channelBlendDarken (A,(2 * B)) : channelBlendLighten (A, (2 * (B - 128)))); }
inline uint8_t channelBlendHardMix (int A, int B)       { return ((uint8_t)((channelBlendVividLight (A, B) < 128) ? 0:255)); }
inline uint8_t channelBlendReflect (int A, int B)       { return ((uint8_t)((B == 255) ? B : juce::jmin (255, (A * A / (255 - B))))); }
inline uint8_t channelBlendGlow (int A, int B)          { return (channelBlendReflect (B, A)); }
inline uint8_t channelBlendPhoenix (int A, int B)       { return ((uint8_t)(juce::jmin (A, B) - juce::jmax (A, B) + 255)); }

inline uint8_t channelBlendAlpha (uint8_t A, uint8_t B, float O)
{
    return ((uint8_t)(O * A + (1 - O) * B));
}

template <class T, uint8_t (*F)(int, int)>
void applyBlend (juce::Image& dst, const juce::Image& src, float alpha, juce::Point<int> position, juce::ThreadPool* threadPool)
{
    auto rcLower = juce::Rectangle<int> (0, 0, dst.getWidth(), dst.getHeight());
    auto rcUpper = juce::Rectangle<int> (position.x, position.y, src.getWidth(), src.getHeight());

    auto rcOverlap = rcLower.getIntersection (rcUpper);
    if (rcOverlap.isEmpty())
        return;

    int w = rcOverlap.getWidth();
    int h = rcOverlap.getHeight();

    int cropX = position.x < 0 ? -position.x : 0;
    int cropY = position.y < 0 ? -position.y : 0;

    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image::BitmapData srcData (src, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readWrite);

    multiThreadedFor (0, h, 1, threadPool, [&] (int y)
                           {
                               uint8_t* pSrc = srcData.getLinePointer (cropY + y);
                               uint8_t* pDst = dstData.getLinePointer (rcOverlap.getY() + y);

                               pSrc += srcData.pixelStride * cropX;
                               pDst += dstData.pixelStride * rcOverlap.getX();

                               for (int x = 0; x < w; x++)
                               {
                                   T* ac = (T*)pSrc;
                                   T* bc = (T*)pDst;

                                   uint8_t ar = ac->getRed();
                                   uint8_t ag = ac->getGreen();
                                   uint8_t ab = ac->getBlue();
                                   uint8_t aa = ac->getAlpha();

                                   uint8_t br = bc->getRed();
                                   uint8_t bg = bc->getGreen();
                                   uint8_t bb = bc->getBlue();
                                   uint8_t ba = bc->getAlpha();

                                   if (ba == 255)
                                   {
                                       float pixelAlpha = alpha * aa / 255.0f;

                                       br = channelBlendAlpha (F (ar, br), br, pixelAlpha);
                                       bg = channelBlendAlpha (F (ag, bg), bg, pixelAlpha);
                                       bb = channelBlendAlpha (F (ab, bb), bb, pixelAlpha);
                                   }
                                   else
                                   {
                                       float srcAlpha = alpha * aa / 255.0f;
                                       float dstAlpha = ba / 255.0f;

                                       float outAlpha = srcAlpha + dstAlpha * (1.0f - srcAlpha);

                                       if (outAlpha == 0.0)
                                       {
                                           br = 0;
                                           bg = 0;
                                           bb = 0;
                                       }
                                       else
                                       {
                                           uint8_t r = F (ar, br);
                                           uint8_t g = F (ag, bg);
                                           uint8_t b = F (ab, bb);

                                           br = uint8_t ((r * srcAlpha + br * dstAlpha * (1.0f - srcAlpha)) / outAlpha);
                                           bg = uint8_t ((g * srcAlpha + bg * dstAlpha * (1.0f - srcAlpha)) / outAlpha);
                                           bb = uint8_t ((b * srcAlpha + bb * dstAlpha * (1.0f - srcAlpha)) / outAlpha);
                                       }
                                   }

                                   bc->setARGB (ba, br, bg, bb);

                                   pSrc += srcData.pixelStride;
                                   pDst += dstData.pixelStride;
                               }
                           });
}

template <class T>
void applyBlend (juce::Image& dst, const juce::Image& src, BlendMode mode, float alpha, juce::Point<int> position, juce::ThreadPool* threadPool)
{
    switch (mode)
    {
        case Normal: applyBlend<T, channelBlendNormal> (dst, src, alpha, position, threadPool); break;
        case Lighten: applyBlend<T, channelBlendLighten> (dst, src, alpha, position, threadPool); break;
        case Darken: applyBlend<T, channelBlendDarken> (dst, src, alpha, position, threadPool); break;
        case Multiply: applyBlend<T, channelBlendMultiply> (dst, src, alpha, position, threadPool); break;
        case Average: applyBlend<T, channelBlendAverage> (dst, src, alpha, position, threadPool); break;
        case Add: applyBlend<T, channelBlendAdd> (dst, src, alpha, position, threadPool); break;
        case Subtract: applyBlend<T, channelBlendSubtract> (dst, src, alpha, position, threadPool); break;
        case Difference: applyBlend<T, channelBlendDifference> (dst, src, alpha, position, threadPool); break;
        case Negation: applyBlend<T, channelBlendNegation> (dst, src, alpha, position, threadPool); break;
        case Screen: applyBlend<T, channelBlendScreen> (dst, src, alpha, position, threadPool); break;
        case Exclusion: applyBlend<T, channelBlendExclusion> (dst, src, alpha, position, threadPool); break;
        case Overlay: applyBlend<T, channelBlendOverlay> (dst, src, alpha, position, threadPool); break;
        case SoftLight: applyBlend<T, channelBlendSoftLight> (dst, src, alpha, position, threadPool); break;
        case HardLight: applyBlend<T, channelBlendHardLight> (dst, src, alpha, position, threadPool); break;
        case ColorDodge: applyBlend<T, channelBlendColorDodge> (dst, src, alpha, position, threadPool); break;
        case ColorBurn: applyBlend<T, channelBlendColorBurn> (dst, src, alpha, position, threadPool); break;
        case LinearDodge: applyBlend<T, channelBlendLinearDodge> (dst, src, alpha, position, threadPool); break;
        case LinearBurn: applyBlend<T, channelBlendLinearBurn> (dst, src, alpha, position, threadPool); break;
        case LinearLight: applyBlend<T, channelBlendLinearLight> (dst, src, alpha, position, threadPool); break;
        case VividLight: applyBlend<T, channelBlendVividLight> (dst, src, alpha, position, threadPool); break;
        case PinLight: applyBlend<T, channelBlendPinLight> (dst, src, alpha, position, threadPool); break;
        case HardMix: applyBlend<T, channelBlendHardMix> (dst, src, alpha, position, threadPool); break;
        case Reflect: applyBlend<T, channelBlendReflect> (dst, src, alpha, position, threadPool); break;
        case Glow: applyBlend<T, channelBlendGlow> (dst, src, alpha, position, threadPool); break;
        case Phoenix: applyBlend<T, channelBlendPhoenix> (dst, src, alpha, position, threadPool); break;
    }
}

void applyBlend (juce::Image& dst, const juce::Image& src, BlendMode mode, float alpha, juce::Point<int> position, juce::ThreadPool* threadPool)
{
    if (src.getFormat() != dst.getFormat())
    {
        juce::Image copy = src.createCopy();
        copy = copy.convertedToFormat (dst.getFormat());

        if (src.getFormat() == juce::Image::ARGB)          applyBlend<juce::PixelARGB> (dst, copy, mode, alpha, position, threadPool);
        else if (src.getFormat() == juce::Image::RGB)      applyBlend<juce::PixelRGB>  (dst, copy, mode, alpha, position, threadPool);
        else jassertfalse;
    }
    else
    {
        if (src.getFormat() == juce::Image::ARGB)          applyBlend<juce::PixelARGB> (dst, src, mode, alpha, position, threadPool);
        else if (src.getFormat() == juce::Image::RGB)      applyBlend<juce::PixelRGB>  (dst, src, mode, alpha, position, threadPool);
        else jassertfalse;
    }
}

template <class T, uint8_t (*F)(int, int)>
void applyBlend (juce::Image& dst, juce::Colour c, juce::ThreadPool* threadPool)
{
    int w = dst.getWidth();
    int h = dst.getHeight();

    threadPool = (w >= 256 || h >= 256) ? threadPool : nullptr;

    juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readWrite);

    uint8_t ar = c.getRed();
    uint8_t ag = c.getGreen();
    uint8_t ab = c.getBlue();
    uint8_t aa = c.getAlpha();

    multiThreadedFor(0, h, 1, threadPool, [&] (int y)
                           {
                               uint8_t* pDst = dstData.getLinePointer (y);

                               for (int x = 0; x < w; x++)
                               {
                                   T* bc = (T*)pDst;

                                   uint8_t br = bc->getRed();
                                   uint8_t bg = bc->getGreen();
                                   uint8_t bb = bc->getBlue();
                                   uint8_t ba = bc->getAlpha();

                                   if (ba == 255)
                                   {
                                       float pixelAlpha = aa / 255.0f;

                                       br = channelBlendAlpha (F (ar, br), br, pixelAlpha);
                                       bg = channelBlendAlpha (F (ag, bg), bg, pixelAlpha);
                                       bb = channelBlendAlpha (F (ab, bb), bb, pixelAlpha);
                                   }
                                   else
                                   {
                                       float srcAlpha = aa / 255.0f;
                                       float dstAlpha = ba / 255.0f;

                                       float outAlpha = srcAlpha + dstAlpha * (1.0f - srcAlpha);

                                       if (outAlpha == 0.0)
                                       {
                                           br = 0;
                                           bg = 0;
                                           bb = 0;
                                       }
                                       else
                                       {
                                           uint8_t r = F (ar, br);
                                           uint8_t g = F (ag, bg);
                                           uint8_t b = F (ab, bb);

                                           br = uint8_t ((r * srcAlpha + br * dstAlpha * (1.0f - srcAlpha)) / outAlpha);
                                           bg = uint8_t ((g * srcAlpha + bg * dstAlpha * (1.0f - srcAlpha)) / outAlpha);
                                           bb = uint8_t ((b * srcAlpha + bb * dstAlpha * (1.0f - srcAlpha)) / outAlpha);
                                       }
                                   }

                                   bc->setARGB (ba, br, bg, bb);

                                   pDst += dstData.pixelStride;
                               }
                           });
}

template <class T>
void applyBlend (juce::Image& dst, BlendMode mode, juce::Colour c, juce::ThreadPool* threadPool)
{
    switch (mode)
    {
        case Normal: applyBlend<T, channelBlendNormal> (dst, c, threadPool); break;
        case Lighten: applyBlend<T, channelBlendLighten> (dst, c, threadPool); break;
        case Darken: applyBlend<T, channelBlendDarken> (dst, c, threadPool); break;
        case Multiply: applyBlend<T, channelBlendMultiply> (dst, c, threadPool); break;
        case Average: applyBlend<T, channelBlendAverage> (dst, c, threadPool); break;
        case Add: applyBlend<T, channelBlendAdd> (dst, c, threadPool); break;
        case Subtract: applyBlend<T, channelBlendSubtract> (dst, c, threadPool); break;
        case Difference: applyBlend<T, channelBlendDifference> (dst, c, threadPool); break;
        case Negation: applyBlend<T, channelBlendNegation> (dst, c, threadPool); break;
        case Screen: applyBlend<T, channelBlendScreen> (dst, c, threadPool); break;
        case Exclusion: applyBlend<T, channelBlendExclusion> (dst, c, threadPool); break;
        case Overlay: applyBlend<T, channelBlendOverlay> (dst, c, threadPool); break;
        case SoftLight: applyBlend<T, channelBlendSoftLight> (dst, c, threadPool); break;
        case HardLight: applyBlend<T, channelBlendHardLight> (dst, c, threadPool); break;
        case ColorDodge: applyBlend<T, channelBlendColorDodge> (dst, c, threadPool); break;
        case ColorBurn: applyBlend<T, channelBlendColorBurn> (dst, c, threadPool); break;
        case LinearDodge: applyBlend<T, channelBlendLinearDodge> (dst, c, threadPool); break;
        case LinearBurn: applyBlend<T, channelBlendLinearBurn> (dst, c, threadPool); break;
        case LinearLight: applyBlend<T, channelBlendLinearLight> (dst, c, threadPool); break;
        case VividLight: applyBlend<T, channelBlendVividLight> (dst, c, threadPool); break;
        case PinLight: applyBlend<T, channelBlendPinLight> (dst, c, threadPool); break;
        case HardMix: applyBlend<T, channelBlendHardMix> (dst, c, threadPool); break;
        case Reflect: applyBlend<T, channelBlendReflect> (dst, c, threadPool); break;
        case Glow: applyBlend<T, channelBlendGlow> (dst, c, threadPool); break;
        case Phoenix: applyBlend<T, channelBlendPhoenix> (dst, c, threadPool); break;
    }
}

void applyBlend (juce::Image& dst, BlendMode mode, juce::Colour c, juce::ThreadPool* threadPool)
{
    if (dst.getFormat() == juce::Image::ARGB)          applyBlend<juce::PixelARGB> (dst, mode, c, threadPool);
    else if (dst.getFormat() == juce::Image::RGB)      applyBlend<juce::PixelRGB>  (dst, mode, c, threadPool);
    else jassertfalse;
}

}
