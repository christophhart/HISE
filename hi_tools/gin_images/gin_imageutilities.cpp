/*==============================================================================

 Copyright 2019 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

namespace gin {

juce::Image rasterizeSVG (juce::String svgText, int w, int h)
{
    juce::Image img (juce::Image::ARGB, w, h, true);

    if (auto svg = juce::XmlDocument::parse (svgText))
    {
        const juce::MessageManagerLock mmLock;

        auto drawable = juce::Drawable::createFromSVG (*svg);

        juce::Graphics g (img);
        drawable->drawWithin (g, juce::Rectangle<float>(float (w), float (h)), 0, 1.0f);
    }

    return img;
}

juce::Path parseSVGPath ( const juce::String& text )
{
    auto pathFromPoints = [] (juce::String pointsText) -> juce::Path
    {
        auto points = juce::StringArray::fromTokens (pointsText, " ,", "");
        points.removeEmptyStrings();

        jassert (points.size() % 2 == 0);

        juce::Path p;

        for (int i = 0; i < points.size() / 2; i++)
        {
            auto x = points[i * 2].getFloatValue();
            auto y = points[i * 2 + 1].getFloatValue();

            if (i == 0)
                p.startNewSubPath ({ x, y });
            else
                p.lineTo ({ x, y });
        }

        p.closeSubPath();

        return p;
    };

    auto path = juce::Drawable::parseSVGPath (text);

    if (path.isEmpty())
        path = pathFromPoints (text);

    return path;
}

}