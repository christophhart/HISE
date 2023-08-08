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

#ifndef EXPANSION_FLOATING_TILES_H_INCLUDED
#define EXPANSION_FLOATING_TILES_H_INCLUDED

namespace hise {
using namespace juce;

class ExpansionEditBar : public FloatingTileContent,
						 public Component,
						 public ButtonListener,
						 public ExpansionHandler::Listener,
						 public ComboBox::Listener
{
public:

	SET_PANEL_NAME("ExpansionEditBar");

	ExpansionEditBar(FloatingTile* parent);;

	void refreshExpansionList();

	~ExpansionEditBar();

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF333333));
	}

	void resized() override;

	void buttonClicked(Button* b) override;

	int getFixedHeight() const override { return 28; }

	ScopedPointer<hise::PathFactory> factory;

	OwnedArray<HiseShapeButton> buttons;

	ScopedPointer<ComboBox> expansionSelector;

	void expansionPackCreated(Expansion* /*e*/) override { refreshExpansionList(); }
	void expansionPackLoaded(Expansion* /*e*/) override;

	void comboBoxChanged(ComboBox* /*comboBoxThatHasChanged*/) override;

private:

	HiseShapeButton* getButton(const String& id)
	{
		for (auto b : buttons)
		{
			if (b->getName() == id)
				return b;
		}

		jassertfalse;
		return nullptr;
	}
};


}

#endif