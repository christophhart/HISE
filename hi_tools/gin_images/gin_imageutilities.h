/*==============================================================================

 Copyright 2019 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

#pragma once

namespace gin {

/** Converts an SVG to Image.

    WARNING: This will lock the image thread
*/
juce::Image rasterizeSVG (juce::String svgText, int w, int h);

/** Like Drawable::parseSVGPath but works with list of points */
juce::Path parseSVGPath ( const juce::String& txt );

}