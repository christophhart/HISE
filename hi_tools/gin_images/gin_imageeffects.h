/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

#pragma once

namespace gin {

//==============================================================================
/** Apply vignette
 *
 \param amount Amount to darken outside of vignette. 0 no darkening. 1 is black.
 \param radius Size of vignette. 1 size of image. 0 is 0 size.
 \param falloff Relative size of inner boundry of vignette 0,1
 */
void applyVignette (juce::Image& img, float amount, float radius, float falloff, juce::ThreadPool* threadPool = nullptr);

/** Make it look old */
void applySepia (juce::Image& img, juce::ThreadPool* threadPool = nullptr);

/** Converts image to B/W, heavier weighting towards greens */
void applyGreyScale (juce::Image& img, juce::ThreadPool* threadPool = nullptr);

/** Softens an image */
void applySoften (juce::Image& img, juce::ThreadPool* threadPool = nullptr);

/** Sharpens an image */
void applySharpen (juce::Image& img, juce::ThreadPool* threadPool = nullptr);

void applyGamma (juce::Image& img, float gamma, juce::ThreadPool* threadPool = nullptr);

/** Inverts colours of an image */
void applyInvert (juce::Image& img, juce::ThreadPool* threadPool = nullptr);

/** Adjust contrast of an image
 *
 \param contrast Amount to adjust contrast. Negative values increase, positive values increase
*/
void applyContrast (juce::Image& img, float contrast, juce::ThreadPool* threadPool = nullptr);

/** Adjust brightness and contrast of an image
 *
 \param brightness Amount to adjust brightness -100,100
 \param contrast Amount to adjust contrast -100,100
 */
void applyBrightnessContrast (juce::Image& img, float brightness, float contrast, juce::ThreadPool* threadPool = nullptr);

/** Adjust hue, saturation and lightness of an image
 *
 \param hue Amount to adjust hue -180,180
 \param saturation Amount to adjust saturation 0,200
 \param lightness Amount to adjust lightness -100,100
 */
void applyHueSaturationLightness (juce::Image& img, float hue, float saturation, float lightness, juce::ThreadPool* threadPool = nullptr);

/** A very fast blur. This is a compromise between Gaussian Blur and Box blur.
    It creates much better looking blurs than Box Blur, but is 7x faster than some Gaussian Blur
    implementations.
 *
 \param radius from 2 to 254
 */
void applyStackBlur (juce::Image& img, int radius);

/** A very high quality image resize using a bank of sinc
 *  function-based fractional delay filters */
juce::Image applyResize (const juce::Image& img, int width, int height);

juce::Image applyResize (const juce::Image& img, float factor);

/** GradientMap a image. Brightness gets remapped to colour on a gradient.
  */
void applyGradientMap (juce::Image& img, const juce::ColourGradient& gradient, juce::ThreadPool* threadPool = nullptr);

void applyGradientMap (juce::Image& img, const juce::Colour c1, const juce::Colour c2, juce::ThreadPool* threadPool = nullptr);

/** Set an image to a solid colour
  */
void applyColour (juce::Image& img, juce::Colour c, juce::ThreadPool* threadPool = nullptr);


/** Blending modes for applyBlend
 */
enum BlendMode
{
    Normal,
    Lighten,
    Darken,
    Multiply,
    Average,
    Add,
    Subtract,
    Difference,
    Negation,
    Screen,
    Exclusion,
    Overlay,
    SoftLight,
    HardLight,
    ColorDodge,
    ColorBurn,
    LinearDodge,
    LinearBurn,
    LinearLight,
    VividLight,
    PinLight,
    HardMix,
    Reflect,
    Glow,
    Phoenix,
};

/** Blend two images
 */
void applyBlend (juce::Image& dst, const juce::Image& src, BlendMode mode, float alpha = 1.0f, juce::Point<int> position = {}, juce::ThreadPool* threadPool = nullptr);

/** Blend two images
 */
void applyBlend (juce::Image& dst, BlendMode mode, juce::Colour c, juce::ThreadPool* threadPool = nullptr);


}