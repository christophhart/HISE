#pragma once

#include "../JuceLibraryCode/JuceHeader.h"


class DarkSplitter  : public StretchableLayoutResizerBar
{
public:
    DarkSplitter (StretchableLayoutManager* layout, const int index, const bool vertical)
        : StretchableLayoutResizerBar (layout, index, vertical)
    { }
	void paint (Graphics& g)
	{
		float alpha = 0.5f;
		if (isMouseOver() || isMouseButtonDown())
		{
			g.fillAll (Colour (0x190000ff));
			alpha = 1.0f;
		} else 
			g.fillAll (Colour (0x101010ff));

		const float cx = getWidth() * 0.5f;
		const float cy = getHeight() * 0.5f;
		const float cr = jmin (getWidth(), getHeight()) * 0.4f;

		g.setGradientFill (ColourGradient (Colours::white.withAlpha (alpha), cx + cr * 0.1f, cy + cr,
										   Colours::black.withAlpha (alpha), cx, cy - cr * 4.0f,
										   true));
		g.fillEllipse (cx - cr, cy - cr, cr * 2.0f, cr * 2.0f);
	}
};
