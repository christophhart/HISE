/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/
#ifndef BACKENDPANELTYPES_H_INCLUDED
#define BACKENDPANELTYPES_H_INCLUDED

namespace hise { using namespace juce;

class ApplicationCommandButtonPanel : public FloatingTileContent,
	public Component
{
public:


	ApplicationCommandButtonPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		addAndMakeVisible(b = new ShapeButton("Icon", Colours::white.withAlpha(0.3f), Colours::white.withAlpha(0.5f), Colours::white));

		b->setVisible(false);
	}

	SET_PANEL_NAME("Icon");

	void resized() override
	{
		b->setBounds(getLocalBounds());
	}

	void setCommand(int commandID);

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

private:

	ScopedPointer<ShapeButton> b;
};

} // namespace hise

#endif  // BACKENDPANELTYPES_H_INCLUDED
