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


namespace hise {
using namespace juce;


juce::Path MPEPanel::Factory::createPath(const String& id) const
{
	if (id == "Delete")
	{
		Path deletePath;
		deletePath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));

		return deletePath;
	}
	else if (id == "Bypass")
	{
		Path p;

		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::bypassShape, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::bypassShape));
		return p;
	}
	else if (id == "Stroke")
	{
		static const unsigned char pathData[] = { 110,109,0,12,223,66,128,179,176,67,108,0,21,212,66,128,235,177,67,108,255,131,214,66,64,74,179,67,108,0,0,2,67,192,223,204,67,108,0,190,24,67,64,74,179,67,108,128,245,25,67,128,235,177,67,108,0,122,20,67,128,179,176,67,108,0,66,19,67,128,18,178,67,108,
			0,0,2,67,192,124,197,67,108,0,124,225,66,128,18,178,67,108,0,12,223,66,128,179,176,67,99,101,0,0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}
	else if (id == "Press")
	{
		static const unsigned char pathData[] = { 110,109,0,0,27,67,0,115,212,67,98,138,219,11,67,0,115,212,67,0,18,255,66,5,156,218,67,0,18,255,66,64,46,226,67,98,0,18,255,66,123,192,233,67,138,219,11,67,192,233,239,67,0,0,27,67,192,233,239,67,98,118,36,42,67,192,233,239,67,0,119,54,67,123,192,233,
			67,0,119,54,67,64,46,226,67,98,0,119,54,67,5,156,218,67,118,36,42,67,0,115,212,67,0,0,27,67,0,115,212,67,99,109,0,0,27,67,0,158,214,67,98,127,204,39,67,0,158,214,67,128,32,50,67,0,200,219,67,128,32,50,67,64,46,226,67,98,128,32,50,67,128,148,232,67,127,
			204,39,67,192,190,237,67,0,0,27,67,192,190,237,67,98,129,51,14,67,192,190,237,67,128,223,3,67,128,148,232,67,128,223,3,67,64,46,226,67,98,128,223,3,67,0,200,219,67,129,51,14,67,0,158,214,67,0,0,27,67,0,158,214,67,99,109,0,0,27,67,64,174,218,67,108,0,
			0,27,67,64,174,218,67,108,20,64,26,67,166,176,218,67,108,164,128,25,67,216,183,218,67,108,40,194,24,67,207,195,218,67,108,28,5,24,67,134,212,218,67,108,248,73,23,67,240,233,218,67,108,52,145,22,67,1,4,219,67,108,70,219,21,67,168,34,219,67,108,162,40,
			21,67,208,69,219,67,108,187,121,20,67,100,109,219,67,108,1,207,19,67,75,153,219,67,108,225,40,19,67,103,201,219,67,108,198,135,18,67,155,253,219,67,108,22,236,17,67,197,53,220,67,108,52,86,17,67,193,113,220,67,108,130,198,16,67,104,177,220,67,108,90,
			61,16,67,147,244,220,67,108,21,187,15,67,21,59,221,67,108,6,64,15,67,194,132,221,67,108,124,204,14,67,107,209,221,67,108,192,96,14,67,223,32,222,67,108,24,253,13,67,234,114,222,67,108,196,161,13,67,88,199,222,67,108,252,78,13,67,244,29,223,67,108,248,
			4,13,67,134,118,223,67,108,230,195,12,67,213,208,223,67,108,240,139,12,67,167,44,224,67,108,57,93,12,67,194,137,224,67,108,224,55,12,67,234,231,224,67,108,251,27,12,67,226,70,225,67,108,159,9,12,67,111,166,225,67,108,213,0,12,67,83,6,226,67,108,0,0,12,
			67,64,46,226,67,108,0,0,12,67,64,46,226,67,108,0,0,12,67,64,46,226,67,108,205,4,12,67,54,142,226,67,108,47,19,12,67,238,237,226,67,108,31,43,12,67,44,77,227,67,108,140,76,12,67,178,171,227,67,108,97,119,12,67,68,9,228,67,108,130,171,12,67,167,101,228,
			67,108,208,232,12,67,158,192,228,67,108,33,47,13,67,240,25,229,67,108,73,126,13,67,99,113,229,67,108,22,214,13,67,192,198,229,67,108,79,54,14,67,208,25,230,67,108,183,158,14,67,94,106,230,67,108,11,15,15,67,54,184,230,67,108,3,135,15,67,39,3,231,67,108,
			82,6,16,67,0,75,231,67,108,167,140,16,67,148,143,231,67,108,172,25,17,67,182,208,231,67,108,6,173,17,67,62,14,232,67,108,88,70,18,67,3,72,232,67,108,63,229,18,67,224,125,232,67,108,86,137,19,67,180,175,232,67,108,51,50,20,67,95,221,232,67,108,106,223,
			20,67,194,6,233,67,108,142,144,21,67,196,43,233,67,108,44,69,22,67,77,76,233,67,108,208,252,22,67,72,104,233,67,108,6,183,23,67,164,127,233,67,108,85,115,24,67,80,146,233,67,108,71,49,25,67,66,160,233,67,108,96,240,25,67,113,169,233,67,108,40,176,26,
			67,214,173,233,67,108,0,0,27,67,64,174,233,67,108,0,0,27,67,64,174,233,67,108,0,0,27,67,64,174,233,67,108,235,191,27,67,218,171,233,67,108,92,127,28,67,168,164,233,67,108,215,61,29,67,177,152,233,67,108,228,250,29,67,250,135,233,67,108,8,182,30,67,144,
			114,233,67,108,204,110,31,67,127,88,233,67,108,186,36,32,67,217,57,233,67,108,93,215,32,67,176,22,233,67,108,68,134,33,67,28,239,232,67,108,254,48,34,67,54,195,232,67,108,30,215,34,67,25,147,232,67,108,58,120,35,67,229,94,232,67,108,234,19,36,67,187,
			38,232,67,108,203,169,36,67,191,234,231,67,108,126,57,37,67,24,171,231,67,108,165,194,37,67,238,103,231,67,108,234,68,38,67,107,33,231,67,108,249,191,38,67,190,215,230,67,108,131,51,39,67,21,139,230,67,108,63,159,39,67,162,59,230,67,108,231,2,40,67,151,
			233,229,67,108,60,94,40,67,40,149,229,67,108,3,177,40,67,140,62,229,67,108,7,251,40,67,251,229,228,67,108,25,60,41,67,172,139,228,67,108,16,116,41,67,218,47,228,67,108,199,162,41,67,191,210,227,67,108,32,200,41,67,151,116,227,67,108,4,228,41,67,158,21,
			227,67,108,97,246,41,67,17,182,226,67,108,43,255,41,67,46,86,226,67,108,0,0,42,67,64,46,226,67,108,0,0,42,67,64,46,226,67,108,0,0,42,67,64,46,226,67,108,51,251,41,67,74,206,225,67,108,209,236,41,67,146,110,225,67,108,226,212,41,67,84,15,225,67,108,117,
			179,41,67,206,176,224,67,108,160,136,41,67,60,83,224,67,108,126,84,41,67,218,246,223,67,108,49,23,41,67,227,155,223,67,108,224,208,40,67,145,66,223,67,108,184,129,40,67,30,235,222,67,108,235,41,40,67,193,149,222,67,108,178,201,39,67,177,66,222,67,108,
			74,97,39,67,35,242,221,67,108,246,240,38,67,75,164,221,67,108,255,120,38,67,90,89,221,67,108,176,249,37,67,129,17,221,67,108,91,115,37,67,237,204,220,67,108,86,230,36,67,203,139,220,67,108,252,82,36,67,67,78,220,67,108,170,185,35,67,126,20,220,67,108,
			195,26,35,67,160,222,219,67,108,173,118,34,67,204,172,219,67,108,208,205,33,67,34,127,219,67,108,152,32,33,67,190,85,219,67,108,117,111,32,67,188,48,219,67,108,215,186,31,67,51,16,219,67,108,50,3,31,67,56,244,218,67,108,253,72,30,67,220,220,218,67,108,
			173,140,29,67,48,202,218,67,108,187,206,28,67,62,188,218,67,108,162,15,28,67,15,179,218,67,108,219,79,27,67,170,174,218,67,108,0,0,27,67,64,174,218,67,108,0,0,27,67,64,174,218,67,99,101,0,0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}
	else if (id == "Glide")
	{
		static const unsigned char pathData[] = { 110,109,0,114,144,67,64,217,222,67,108,128,178,135,67,64,174,228,67,108,128,178,144,67,64,174,234,67,108,0,0,145,67,64,174,234,67,108,0,0,145,67,64,174,232,67,108,128,77,145,67,64,174,232,67,108,128,77,139,67,64,174,228,67,108,0,142,145,67,64,131,224,
			67,108,0,114,144,67,64,217,222,67,99,109,0,142,150,67,64,217,222,67,108,0,114,149,67,64,131,224,67,108,128,178,155,67,64,174,228,67,108,0,114,149,67,64,217,232,67,108,0,142,150,67,64,131,234,67,108,128,77,159,67,64,174,228,67,108,0,142,150,67,64,217,
			222,67,99,101,0,0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}
	else if (id == "Lift")
	{
		static const unsigned char pathData[] = { 110,109,0,0,67,67,128,51,174,67,108,0,66,44,67,0,201,199,67,108,128,10,43,67,0,40,201,67,108,0,134,48,67,192,95,202,67,108,0,190,49,67,0,1,201,67,108,0,0,67,67,192,150,181,67,108,0,66,84,67,0,1,201,67,108,0,122,85,67,192,95,202,67,108,128,245,90,67,0,
			40,201,67,108,0,190,89,67,0,201,199,67,108,0,0,67,67,128,51,174,67,99,101,0,0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));

		return path;

	}
	else if (id == "Slide")
	{
		static const unsigned char pathData[] = { 110,109,128,138,166,67,82,122,193,67,108,128,181,160,67,210,186,184,67,108,128,181,154,67,210,186,193,67,108,128,181,154,67,82,8,194,67,108,128,181,156,67,82,8,194,67,108,128,181,156,67,210,85,194,67,108,128,181,160,67,210,85,188,67,108,128,224,164,67,
			82,150,194,67,108,128,138,166,67,82,122,193,67,99,109,128,138,166,67,82,150,199,67,108,128,224,164,67,82,122,198,67,108,128,181,160,67,210,186,204,67,108,128,138,156,67,82,122,198,67,108,128,224,154,67,82,150,199,67,108,128,181,160,67,210,85,208,67,108,
			128,138,166,67,82,150,199,67,99,101,0,0 };

		Path path;
		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}

	return Path();
}

MPEPanel::LookAndFeel::LookAndFeel()
{
	bgColour = Colours::transparentBlack;
	font = GLOBAL_BOLD_FONT();
	textColour = Colours::white;
	fillColour = Colours::white.withAlpha(0.5f);
	lineColour = Colours::white.withAlpha(0.1f);
}



void MPEPanel::LookAndFeel::drawLinearSlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/, const Slider::SliderStyle, Slider &s)
{
	const bool isBiPolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

	float leftX;
	float actualWidth;

	const float max = (float)s.getMaximum();
	const float min = (float)s.getMinimum();

	g.setColour(lineColour);
	g.drawRect(0, 0, width, height);

	if (isBiPolar)
	{
		const float value = ((float)s.getValue() - min) / (max - min);

		leftX = 2.0f + (value < 0.5f ? value * (float)(width - 2) : 0.5f * (float)(width - 2));
		actualWidth = fabs(0.5f - value) * (float)(width - 2);
	}
	else
	{
		const double value = s.getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());

		//const float value = ((float)s.getValue() - min) / (max - min);

		leftX = 2;
		actualWidth = (float)proportion * (float)(width - 4);
	}

	Colour c = fillColour;

	g.setGradientFill(ColourGradient(c.withMultipliedAlpha(1.1f),
		0.0f, 0.0f,
		c.withMultipliedAlpha(0.9f),
		0.0f, (float)height,
		false));

	g.fillRect(leftX, 2.0f, actualWidth, (float)(height - 4));

	if (s.isEnabled())
	{
		g.setColour(textColour);
		g.setFont(font);
		g.drawText(s.getTextFromValue(s.getValue()), 0, 0, width, height, Justification::centred);
	}
}

void MPEPanel::LookAndFeel::positionComboBoxText(ComboBox &c, Label &labelToPosition)
{
	if (c.getHeight() < 20)
	{
		labelToPosition.setBounds(5, 2, c.getWidth() - 25, c.getHeight() - 4);

	}
	else
	{
		labelToPosition.setBounds(5, 5, c.getWidth() - 25, c.getHeight() - 10);

	}


	labelToPosition.setFont(getComboBoxFont(c));
	labelToPosition.setColour(Label::ColourIds::textColourId, textColour);
}

void MPEPanel::LookAndFeel::drawComboBox(Graphics &g, int width, int height, bool /*isButtonDown*/, int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, ComboBox &c)
{
	Factory f;
	auto path = f.createPath(c.getText());

	path.scaleToFit((float)width - 20.0f, (float)(height - 12) * 0.5f, 12.0f, 12.0f, true);

	g.setColour(textColour);
	g.fillPath(path);
}


void MPEPanel::LookAndFeel::drawButtonBackground(Graphics& g, Button&b, const Colour& /*backgroundColour*/, bool isMouseOverButton, bool /*isButtonDown*/)
{
	if (b.getToggleState())
	{
		g.setColour(fillColour.withMultipliedAlpha(0.5f));
		g.fillAll();
	}

	if (isMouseOverButton)
	{
		g.setColour(lineColour.withMultipliedAlpha(0.2f));
		g.fillAll();
	}

	if (b.getName() == "Enable MPE Mode")
	{
		MPEPanel::Factory f;
		auto p = f.createPath("Bypass");

		g.setColour(textColour.withMultipliedAlpha(b.getToggleState() ? 1.0f : 0.4f));

		const float margin = 4.0f;

		float s = (float)(b.getHeight()) - 2 * margin;

		p.scaleToFit(margin, margin, s, s, true);

		g.fillPath(p);

	}


}




void MPEPanel::LookAndFeel::drawButtonText(Graphics& g, TextButton& b, bool /*isMouseOverButton*/, bool /*isButtonDown*/)
{
	g.setColour(textColour);
	g.setFont(font);
	g.drawText(b.getButtonText(), 0, 0, b.getWidth(), b.getHeight(), Justification::centred);
}

MPEPanel::MPEPanel(FloatingTile* parent) :
	FloatingTileContent(parent),
	m(*this),
	notifier(*this),
	currentTable(parent->getMainController()->getControlUndoManager(), nullptr),

	enableMPEButton("Enable MPE Mode")

{
	addAndMakeVisible(enableMPEButton);

	enableMPEButton.setLookAndFeel(&laf);
	enableMPEButton.setClickingTogglesState(true);
	enableMPEButton.addListener(this);



	listbox.setModel(&m);
	listbox.setRowHeight(36);
	listbox.setColour(ListBox::ColourIds::backgroundColourId, Colours::transparentBlack);

	setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white.withAlpha(0.5f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::textColour, Colours::white);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.1f));

	addAndMakeVisible(currentTable);
	addAndMakeVisible(listbox);

	updateTableColours();

	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);
}

void MPEPanel::updateTableColours()
{
	currentTable.setUseFlatDesign(true);
	currentTable.setColour(TableEditor::ColourIds::bgColour, laf.fillColour.withAlpha(0.05f));
	currentTable.setColour(TableEditor::ColourIds::fillColour, laf.fillColour);
	currentTable.setColour(TableEditor::ColourIds::lineColour, laf.lineColour);
}

void MPEPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	laf.bgColour = findPanelColour(FloatingTileContent::PanelColourId::bgColour);
	laf.lineColour = findPanelColour(FloatingTileContent::PanelColourId::itemColour1);
	laf.textColour = findPanelColour(FloatingTileContent::PanelColourId::textColour);
	laf.fillColour = findPanelColour(FloatingTileContent::PanelColourId::itemColour2);

	laf.font = getFont();

	listbox.setRowHeight(roundDoubleToInt(getFont().getHeight() * 2.2f));

	updateTableColours();
}

bool MPEPanel::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey)
	{
		setCurrentMod(nullptr);
		return true;
	}

	return false;
}

void MPEPanel::resized()
{

	auto ar = getParentShell()->getContentBounds();

	const int margin = 2;

	enableMPEButton.setBounds(ar.removeFromTop(32).reduced(margin));

	const bool enabled = enableMPEButton.getToggleState();

	listbox.setVisible(enabled);
	currentTable.setVisible(enabled);

	if (currentPlotter)
		currentPlotter->setVisible(enabled);

	if (enabled)
	{
		updateRectangles();

		if (currentlyEditedMod != nullptr)
		{
			auto r = bottomArea;

			currentTable.setVisible(true);

			currentTable.setBounds(r.removeFromLeft(r.getWidth() / 2).reduced(margin));
			if (currentPlotter)
				currentPlotter->setBounds(r.reduced(margin));
		}
		else
		{
			currentTable.setVisible(false);
		}


		listbox.setBounds(topArea.reduced(margin));
	}



}



void MPEPanel::updateRectangles()
{
	auto ar = getParentShell()->getContentBounds();

	ar.removeFromTop(32);

	topArea = ar.removeFromTop(ar.getHeight() / 2);
	topBar = topArea.removeFromTop(30);
	tableHeader = topArea.removeFromTop(30);

	bottomArea = ar;
	bottomBar = bottomArea.removeFromTop(30);
}



void MPEPanel::paint(Graphics& g)
{
	if (enableMPEButton.getToggleState())
	{
		updateRectangles();

		const bool empty = getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().size() == 0;

		if (!empty)
		{
			const int buttonWidth = listbox.getRowHeight();

			Rectangle<int> rectangles[9] = { tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(80),
				tableHeader.removeFromLeft(80),
				tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(50),
				tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(100),
				tableHeader };

			g.setColour(laf.fillColour.withAlpha(0.1f));

			for (int i = 0; i < 9; i++)
			{
				g.fillRect(rectangles[i].reduced(1));
			}


			g.setColour(laf.textColour);
			g.setFont(laf.font);


			g.drawText("Target", rectangles[0], Justification::centred);
			g.drawText("Meter", rectangles[1], Justification::centred);
			g.drawText("Gesture", rectangles[2], Justification::centred);
			g.drawText("Mode", rectangles[3], Justification::centred);
			g.drawText("Intensity", rectangles[4], Justification::centred);
			g.drawText("Curve", rectangles[5], Justification::centred);
			g.drawText("Smoothing", rectangles[6], Justification::centred);
			g.drawText("Default", rectangles[7], Justification::centred);
		}
		else
		{
			g.setColour(laf.textColour.withMultipliedAlpha(0.4f));
			g.setFont(laf.font);

			g.drawText("No Active Modulations", tableHeader, Justification::centred);
		}

		if (currentlyEditedMod != nullptr)
		{
			g.setColour(laf.textColour);
			g.setFont(laf.font);

			g.drawText("Curve", bottomBar.removeFromLeft(getWidth() / 2), Justification::centredTop);
			g.drawText("Plot", bottomBar.removeFromLeft(getWidth() / 2), Justification::centredTop);
		}
	}
	else
	{
		updateRectangles();

		g.setFont(laf.font);
		g.setColour(laf.textColour.withMultipliedAlpha(0.4f));
		g.drawText("MPE is disabled", tableHeader, Justification::centred);
	}


}



void MPEPanel::setCurrentMod(MPEModulator* newMod)
{
	if (newMod != currentlyEditedMod || newMod == nullptr)
	{
		currentPlotter = nullptr;

		currentlyEditedMod = newMod;

		if (newMod)
		{
			currentTable.setEditedTable(newMod->getTable(0));
			addAndMakeVisible(currentPlotter = new Plotter());

			newMod->setPlotter(currentPlotter);
			currentPlotter->setColour(Plotter::ColourIds::pathColour, laf.fillColour.withMultipliedAlpha(1.05f));
			currentPlotter->setColour(Plotter::ColourIds::pathColour2, laf.fillColour);
			currentPlotter->setColour(Plotter::ColourIds::backgroundColour, laf.fillColour.withAlpha(0.05f));
			currentTable.setColour(TableEditor::ColourIds::bgColour, laf.fillColour.withAlpha(0.05f));
		}


		currentTable.connectToLookupTableProcessor(newMod);

		resized();
	}
}

void MPEPanel::mpeModeChanged(bool isEnabled)
{
	enableMPEButton.setToggleState(isEnabled, dontSendNotification);
	notifier.refresh();
}

void MPEPanel::mpeModulatorAssigned(MPEModulator* /*m*/, bool /*wasAssigned*/)
{
	notifier.refresh();
}

void MPEPanel::mpeDataReloaded()
{
	notifier.refresh();
}

void MPEPanel::mpeModulatorAmountChanged()
{
	notifier.refresh();
}

void MPEPanel::buttonClicked(Button* b)
{
	const bool on = b->getToggleState();

	auto f = [on](Processor* p)
	{
		p->getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().setMpeMode(on);
		return true;
	};

	getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), f, MainController::KillStateHandler::MessageThread);

}

MPEPanel::Notifier::Notifier(MPEPanel& parent_) :
	parent(parent_)
{
	startTimer(50);
}

void MPEPanel::Notifier::timerCallback()
{
	if (refreshPanel)
	{
		parent.setCurrentMod(nullptr);
		parent.listbox.deselectAllRows();
		parent.listbox.updateContent();
		parent.repaint();
		parent.resized();
		refreshPanel = false;
	}
}



MPEPanel::Model::Model(MPEPanel& parent_) :
	parent(parent_),
	data(parent.getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData())
{

}

int MPEPanel::Model::getNumRows()
{
	const bool unconnectedModsAvailable = !data.getListOfUnconnectedModulators(false).isEmpty();

	if (unconnectedModsAvailable)
		return data.size() + 1;
	else return data.size();
}

void MPEPanel::Model::paintListBoxItem(int /*rowNumber*/, Graphics& g, int width, int height, bool rowIsSelected)
{
	if (rowIsSelected)
	{
		g.setColour(parent.laf.fillColour.withAlpha(0.05f));

		g.fillRect(0, 0, width, height);
	}
}

Component* MPEPanel::Model::refreshComponentForRow(int rowNumber, bool /*isRowSelected*/, Component* existingComponentToUpdate)
{
	if (existingComponentToUpdate != nullptr)
		delete existingComponentToUpdate;

	if (rowNumber == data.size())
	{
		return new LastRow(parent);
	}
	if (auto expectedMod = data.getModulator(rowNumber))
	{
		return new Row(expectedMod, parent.laf);
	}
	else
	{
		return nullptr;
	}
}

void MPEPanel::Model::deleteKeyPressed(int lastRowSelected)
{
	if (auto mod = data.getModulator(lastRowSelected))
	{
		auto f = [](Processor* p)
		{
			if (auto m = dynamic_cast<MPEModulator*>(p))
			{
				m->getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeConnection(m);
				m->sendChangeMessage();
			}

			return true;
		};

		mod->getMainController()->getKillStateHandler().killVoicesAndCall(mod, f, MainController::KillStateHandler::MessageThread);
	}

	parent.setCurrentMod(nullptr);
}

void MPEPanel::Model::listBoxItemClicked(int row, const MouseEvent& e)
{
	auto mod = data.getModulator(row);

	if (mod == nullptr)
		return;

	if (e.mods.isRightButtonDown())
	{
		PopupMenu menu;

		menu.setLookAndFeel(&parent.laf);

		menu.addItem(1, "Reset");


		const String clipboardContent = SystemClipboard::getTextFromClipboard();

		const String wildcard = "^[0-9]+\\.+";

		const bool tableInClipboard = clipboardContent.isNotEmpty() && RegexFunctions::matchesWildcard(wildcard, clipboardContent);

		ScopedPointer<XmlElement> xml = XmlDocument::parse(clipboardContent);

		const bool modInClipboard = xml != nullptr;

		menu.addSeparator();

		menu.addItem(2, "Copy Curve Data", true, tableInClipboard);
		menu.addItem(3, "Paste Curve Data", tableInClipboard, false);

		menu.addSeparator();

		menu.addItem(4, "Copy MPE values", true, modInClipboard);
		menu.addItem(5, "Paste values from clipboard", modInClipboard, false);

		auto result = menu.show();

		if (result == 1)
		{
			mod->resetToDefault();
		}
		else if (result == 2)
		{
			SystemClipboard::copyTextToClipboard(mod->getTable(0)->exportData());
		}
		else if (result == 3)
		{
			mod->getTable(0)->restoreData(clipboardContent);
			mod->sendTableIndexChangeMessage(true, mod->getTable(0), 0);
			mod->sendChangeMessage();

		}
		else if (result == 4)
		{
			ScopedPointer<XmlElement> exportedData = mod->exportAsValueTree().createXml();
			SystemClipboard::copyTextToClipboard(exportedData->createDocument(""));

		}
		else if (result == 5)
		{
			if (xml != nullptr && xml->getTagName() == "Processor" && xml->hasAttribute("ID"))
			{
				xml->setAttribute("ID", mod->getId());
				ValueTree v = ValueTree::fromXml(*xml);

				mod->restoreFromValueTree(v);
				mod->sendChangeMessage();
				mod->sendTableIndexChangeMessage(true, mod->getTable(0), 0);
			}
			else
			{
				PresetHandler::showMessageWindow("No Valid Data", "The clipboard contains no valid MPE data.", PresetHandler::IconType::Warning);
			}

		}
	}
	else
	{
		parent.setCurrentMod(mod);
	}

}

MPEPanel::Model::LastRow::LastRow(MPEPanel& parent_) :
	parent(parent_),
	addButton("Add MPE Modulation")
{
	addAndMakeVisible(addButton);
	addButton.setLookAndFeel(&parent_.laf);
	addButton.addListener(this);
}



MPEPanel::Model::LastRow::~LastRow()
{
	addButton.removeListener(this);
}

void MPEPanel::Model::LastRow::resized()
{
	addButton.setBounds(getLocalBounds());
}

void MPEPanel::Model::LastRow::buttonClicked(Button*)
{
	PopupMenu menu;
	menu.setLookAndFeel(&parent.laf);

	auto& mpeData = parent.getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData();

	auto sa = mpeData.getListOfUnconnectedModulators(true);

	for (int i = 0; i < sa.size(); i++)
	{
		menu.addItem(i + 1, sa[i], true, false);
	}

	int result = menu.show();

	if (result > 0)
	{
		auto ugly = mpeData.getListOfUnconnectedModulators(false);
		auto id = ugly[result - 1];

		if (auto mod = mpeData.findMPEModulator(id))
		{
			auto f = [](Processor* p)
			{
				auto& data = p->getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData();

				data.addConnection(dynamic_cast<MPEModulator*>(p));

				return true;
			};

			mod->getMainController()->getKillStateHandler().killVoicesAndCall(mod, f, MainController::KillStateHandler::MessageThread);
		}
	}
}

MPEPanel::Model::Row::Row(MPEModulator* mod_, LookAndFeel& laf_) :
	mod(mod_),
	curvePreview(nullptr, mod->getTable(0)),
	deleteButton("Delete", Colours::white, Colours::white, Colours::white),
	selector("Gesture"),
	smoothingTime("Smoothing"),
	defaultValue("Default"),
	intensity("Intensity"),
	data(mod_->getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData()),
	laf(laf_)
{
	addAndMakeVisible(selector);
	addAndMakeVisible(deleteButton);
	addAndMakeVisible(smoothingTime);
	addAndMakeVisible(curvePreview);
	addAndMakeVisible(output);
	addAndMakeVisible(intensity);
	addAndMakeVisible(defaultValue);

	addAndMakeVisible(modeSelector);

	modeSelector.addItem("Polyphonic", 1);
	modeSelector.addItem("Legato", 2);
	modeSelector.addItem("Retrigger", 3);



	MPEPanel::Factory f;

	deleteButton.setShape(f.createPath("Delete"), false, true, false);

	deleteButton.addListener(this);


	selector.setup(mod, MPEModulator::SpecialParameters::GestureCC, "Gesture");

	selector.addItem("Press", MPEModulator::Gesture::Press);
	selector.addItem("Slide", MPEModulator::Gesture::Slide);
	selector.addItem("Glide", MPEModulator::Gesture::Glide);
	selector.addItem("Stroke", MPEModulator::Gesture::Stroke);
	selector.addItem("Lift", MPEModulator::Gesture::Lift);

	smoothingTime.setup(mod, MPEModulator::SpecialParameters::SmoothingTime, "Smoothing");
	smoothingTime.setMode(HiSlider::Time, 0.0, 2000.0, 200.0, 0.1);

	defaultValue.setup(mod, MPEModulator::SpecialParameters::DefaultValue, "Default");
	defaultValue.setMode(HiSlider::NormalizedPercentage);





	intensity.setup(mod, MPEModulator::SpecialParameters::SmoothedIntensity, "Intensity");

	if (mod->getMode() == Modulation::GainMode)
	{
		intensity.setMode(HiSlider::NormalizedPercentage);
		defaultValue.setMode(HiSlider::NormalizedPercentage);
	}
	else
	{
		intensity.setMode(HiSlider::Linear, -12.0, 12.0, 0.0, 0.01);
		intensity.setTextValueSuffix(" st.");

		defaultValue.setMode(HiSlider::Linear, -12.0, 12.0, 0.0, 0.01);
		defaultValue.setTextValueSuffix(" st.");
	}


	smoothingTime.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
	intensity.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
	output.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
	defaultValue.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);

	smoothingTime.setSliderStyle(Slider::LinearBar);
	intensity.setSliderStyle(Slider::LinearBar);
	output.setSliderStyle(Slider::LinearBar);
	defaultValue.setSliderStyle(Slider::LinearBar);

	smoothingTime.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	intensity.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	output.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	defaultValue.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);


	intensity.setScrollWheelEnabled(false);
	defaultValue.setScrollWheelEnabled(false);
	smoothingTime.setScrollWheelEnabled(false);

	modeSelector.addListener(this);

	mod->addChangeListener(this);


	curvePreview.connectToLookupTableProcessor(mod);
	curvePreview.setEnabled(false);
	curvePreview.setUseFlatDesign(true);
	curvePreview.setColour(TableEditor::ColourIds::lineColour, Colours::transparentBlack);
	curvePreview.setColour(TableEditor::ColourIds::bgColour, Colours::transparentBlack);
	curvePreview.setColour(TableEditor::ColourIds::fillColour, laf_.fillColour);


	setInterceptsMouseClicks(false, true);

	startTimer(50);

	output.setEnabled(false);

	output.setRange(0.0, 1.0, 0.01);

	selector.setLookAndFeel(&laf_);
	deleteButton.setLookAndFeel(&laf_);
	smoothingTime.setLookAndFeel(&laf_);
	curvePreview.setLookAndFeel(&laf_);
	output.setLookAndFeel(&laf_);
	intensity.setLookAndFeel(&laf_);
	modeSelector.setLookAndFeel(&laf_);
	defaultValue.setLookAndFeel(&laf_);

	changeListenerCallback(nullptr);
}


MPEPanel::Model::Row::~Row()
{
	if (mod != nullptr)
	{
		mod->removeChangeListener(this);
	}
}

void MPEPanel::Model::Row::resized()
{


	auto ar = getLocalBounds();

	int buttonWidth = getHeight();

	const int margin = 2;


	ar.removeFromLeft(100);

	output.setBounds(ar.removeFromLeft(80).reduced(margin));
	selector.setBounds(ar.removeFromLeft(80));
	modeSelector.setBounds(ar.removeFromLeft(100).reduced(margin));
	intensity.setBounds(ar.removeFromLeft(100).reduced(margin));
	curvePreview.setBounds(ar.removeFromLeft(50).reduced(margin));
	smoothingTime.setBounds(ar.removeFromLeft(100).reduced(margin));
	defaultValue.setBounds(ar.removeFromLeft(100).reduced(margin));

	deleteButton.setBounds(ar.removeFromRight(buttonWidth).reduced(margin + 6));
}


void MPEPanel::Model::Row::timerCallback()
{
	if (mod)
		output.setValue(mod->getOutputValue());
}

void MPEPanel::Model::Row::updateEnableState()
{
	if (mod)
	{
		auto enabled = !mod->isBypassed();

		const bool isMonophonic = mod->getAttribute(EnvelopeModulator::Monophonic) > 0.5f;
		const bool shouldRetrigger = mod->getAttribute(EnvelopeModulator::Retrigger) > 0.5f;

		modeSelector.setSelectedId(isMonophonic ? (shouldRetrigger ? 3 : 2) : 1, dontSendNotification);

		intensity.setEnabled(enabled);
		smoothingTime.setEnabled(enabled);
		selector.setEnabled(enabled);

		repaint();
	}
}


void MPEPanel::Model::Row::changeListenerCallback(SafeChangeBroadcaster* /*b*/)
{
	smoothingTime.updateValue();
	selector.updateValue();
	intensity.updateValue();
	defaultValue.updateValue();
	updateEnableState();
}

void MPEPanel::Model::Row::paint(Graphics& g)
{
	auto ar = getLocalBounds();

	const int margin = 2;

	ar = ar.removeFromLeft(100);

	if (mod)
	{
		g.setFont(laf.getFont());

		auto enabled = !mod->isBypassed();

		Colour on = laf.textColour;
		Colour off = on.withMultipliedAlpha(0.4f);

		g.setColour(enabled ? on : off);
		g.drawText(data.getPrettyName(mod->getId()), ar, Justification::centred);
	}
}



void MPEPanel::Model::Row::buttonClicked(Button* b)
{
	if (mod == nullptr)
		return;

	if (b == &deleteButton)
	{
		deleteThisRow();
	}
}

void MPEPanel::Model::Row::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	auto result = comboBoxThatHasChanged->getSelectedId();

	if (result == 1)
	{
		mod->setAttribute(EnvelopeModulator::Parameters::Monophonic, false, sendNotification);
	}
	else if (result == 2)
	{
		mod->setAttribute(EnvelopeModulator::Parameters::Monophonic, true, dontSendNotification);
		mod->setAttribute(EnvelopeModulator::Parameters::Retrigger, false, sendNotification);
	}
	else if (result == 3)
	{
		mod->setAttribute(EnvelopeModulator::Parameters::Monophonic, true, dontSendNotification);
		mod->setAttribute(EnvelopeModulator::Parameters::Retrigger, true, sendNotification);
	}
}

void MPEPanel::Model::Row::deleteThisRow()
{
	if (mod != nullptr)
	{
		auto f = [](Processor* p)
		{
			if (auto m = dynamic_cast<MPEModulator*>(p))
			{
				m->getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeConnection(m);
				m->sendChangeMessage();
			}

			return true;
		};

		findParentComponentOfClass<MPEPanel>()->setCurrentMod(nullptr);

		mod->getMainController()->getKillStateHandler().killVoicesAndCall(mod, f, MainController::KillStateHandler::MessageThread);
	}
}

MPEKeyboard::MPEKeyboard(MidiKeyboardState& state_) :
	state(state_),
	pendingMessages(1024)
{
	state.addListener(this);
	startTimer(30);

	setColour(bgColour, Colours::black);
	setColour(waveColour, Colours::white.withAlpha(0.5f));
	setColour(keyOnColour, Colours::white);
	setColour(dragColour, Colour(SIGNAL_COLOUR));
}

MPEKeyboard::~MPEKeyboard()
{
	state.removeListener(this);
}

void MPEKeyboard::timerCallback()
{
	if (!pendingMessages.isEmpty())
	{
		MidiMessage m;

		while (pendingMessages.pop(m))
		{
			if (m.isNoteOn())
			{
				pressedNotes.insert(Note::fromMidiMessage(*this, m));
			}
			else if (m.isNoteOff())
			{
				for (int i = 0; i < pressedNotes.size(); i++)
				{
					if (pressedNotes[i] == m)
						pressedNotes.removeElement(i--);
				}
			}
			else
			{
				for (auto& n : pressedNotes)
					n.updateNote(*this, m);
			}

		}

		repaint();
	}
}

void MPEKeyboard::handleNoteOn(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity)
{
	auto m = MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);

	pendingMessages.push(std::move(m));
}

void MPEKeyboard::handleNoteOff(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity)
{
	auto m = MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity);

	pendingMessages.push(std::move(m));
}

void MPEKeyboard::handleMessage(const MidiMessage& m)
{
	MidiMessage copy(m);

	pendingMessages.push(std::move(copy));
}

void MPEKeyboard::mouseDown(const MouseEvent& e)
{
	auto n = Note::fromMouseEvent(*this, e, nextChannelIndex);
	n.sendNoteOn(state);

	pressedNotes.insert(n);

	nextChannelIndex++;

	if (nextChannelIndex > 15)
		nextChannelIndex = 1;

	repaint();
}

void MPEKeyboard::mouseUp(const MouseEvent& e)
{
	for (int i = 0; i < pressedNotes.size(); i++)
	{
		if (pressedNotes[i] == e)
		{
			pressedNotes[i].sendNoteOff(state);
			pressedNotes.removeElement(i--);
			break;
		}

	}

	repaint();
}

void MPEKeyboard::mouseDrag(const MouseEvent& event)
{
	for (auto& s : pressedNotes)
		s.updateNote(*this, event);

	repaint();
}

void MPEKeyboard::paint(Graphics& g)
{
	Colour c1 = findColour(bgColour).withMultipliedAlpha(1.1f);
	Colour c2 = findColour(bgColour).withMultipliedAlpha(0.9f);

	g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false));
	g.fillAll();

	static const int whiteWave[24] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0,
		0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

	for (int i = 0; i < 24; i++)
	{
		auto l = getPositionForNote(lowKey + i);

		const float radius = getWidthForNote() * 0.2f;

		if (whiteWave[i] == 1)
		{
			g.setColour(findColour(waveColour));
			g.drawLine(l.getCentreX(), radius, l.getCentreX(), l.getHeight() - 2.0f*radius, 4.0f);
		}
		else if (whiteWave[i] == 0)
		{
			l.reduce(4, 3);

			g.setColour(findColour(waveColour).withMultipliedAlpha(0.1f));
			g.fillRoundedRectangle(l, radius);
		}
	}

	for (const auto& n : pressedNotes)
		n.draw(*this, g);
}

juce::Rectangle<float> MPEKeyboard::getPositionForNote(int noteNumber) const
{
	int normalised = noteNumber - lowKey;
	float widthPerWave = getWidthForNote();

	if (normalised > 24 || normalised < 0)
		return {};

	static const int whiteWave[24] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0,
		0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

	return { normalised * widthPerWave, 0.0f, widthPerWave, whiteWave[normalised] ? (float)getHeight()*0.5f : (float)getHeight() };
}

void MPEKeyboard::buttonClicked(Button* b)
{
	if (b == &octaveUp)
	{
		lowKey = jmin<int>(108, lowKey + 12);
	}
	else
	{
		lowKey = jmax<int>(0, lowKey - 12);
	}

	repaint();
}

hise::MPEKeyboard::Note MPEKeyboard::Note::fromMouseEvent(const MPEKeyboard& p, const MouseEvent& e, int channelIndex)
{
	Note n;

	n.isArtificial = true;
	n.fingerIndex = e.source.getIndex();
	n.assignedMidiChannel = channelIndex;
	n.noteNumber = p.getNoteForPosition(e.getMouseDownPosition());
	n.glideValue = 64;
	n.slideValue = 8192;
	n.strokeValue = 127;
	n.liftValue = 127;
	n.pressureValue = 0;

	n.startPoint = { (int)p.getPositionForNote(n.noteNumber).getCentreX(), e.getMouseDownY() };


	n.dragPoint = n.startPoint;

	return n;
}

hise::MPEKeyboard::Note MPEKeyboard::Note::fromMidiMessage(const MPEKeyboard& p, const MidiMessage& m)
{
	Note n;

	n.isArtificial = false;
	n.fingerIndex = -1;
	n.assignedMidiChannel = m.getChannel();
	n.noteNumber = m.getNoteNumber();
	n.strokeValue = m.getVelocity();
	n.slideValue = 8192;
	n.liftValue = 0;
	n.glideValue = 64;
	n.pressureValue = 0;

	auto sp = p.getPositionForNote(n.noteNumber).getCentre();


	n.startPoint = { (int)sp.getX(), (int)sp.getY() };
	n.dragPoint = n.startPoint;

	return n;
}

void MPEKeyboard::Note::updateNote(const MPEKeyboard& p, const MouseEvent& e)
{
	if (*this != e)
		return;

	dragPoint = e.getPosition();

	float pitchBendValue = (float)e.getDistanceFromDragStartX() / p.getWidthForNote();
	slideValue = jlimit<int>(0, 8192 * 2, 8192 + (int)(pitchBendValue / 24.0f * 4096.0f));
	float normalisedGlide = -0.5f * (float)e.getDistanceFromDragStartY() / (float)p.getHeight();
	glideValue = jlimit<int>(0, 127, 64 + roundFloatToInt(normalisedGlide * 127.0f));

	p.state.injectMessage(MidiMessage::pitchWheel(assignedMidiChannel, slideValue));
	p.state.injectMessage(MidiMessage::controllerEvent(assignedMidiChannel, 74, glideValue));
}

void MPEKeyboard::Note::updateNote(const MPEKeyboard& p, const MidiMessage& m)
{
	if (*this != m)
		return;

	if (m.isPitchWheel())
	{
		slideValue = m.getPitchWheelValue();
		float normalisedSlideValue = (float)(slideValue - 8192) / 4096.0f;
		float slideOctaveValue = normalisedSlideValue * 24.0f;
		float slideDistance = slideOctaveValue * p.getWidthForNote();
		auto newX = (float)startPoint.getX() + slideDistance;

		dragPoint.setX((int)newX);
	}

	else if (m.isChannelPressure())
		pressureValue = m.getChannelPressureValue();
	else if (m.isControllerOfType(74))
	{
		glideValue = m.getControllerValue();

		auto distance = (float)(glideValue - 64) / 32.0f;
		auto newY = (float)startPoint.getY() - distance * (float)startPoint.getY();

		dragPoint.setY((int)newY);
	}

	else if (m.isNoteOff())
		liftValue = m.getVelocity();
}

void MPEKeyboard::Note::draw(const MPEKeyboard& p, Graphics& g) const
{
	if (!isVisible(p))
		return;

	auto area = p.getPositionForNote(noteNumber);

	g.setColour(p.findColour(keyOnColour).withAlpha((float)pressureValue / 127.0f));

	area.reduce(4.0f, 3.0f);
	auto radius = p.getWidthForNote() * 0.2f;

	g.fillRoundedRectangle(area, radius);
	g.setColour(p.findColour(dragColour));

	auto l = Line<int>(startPoint, dragPoint);

	g.drawLine((float)l.getStartX(), (float)l.getStartY(), (float)l.getEndX(), (float)l.getEndY(), 2.0f);

	Rectangle<float> r((float)dragPoint.getX(), (float)dragPoint.getY(), 0.0f, 0.0f);
	r = r.withSizeKeepingCentre(10, 10);

	g.setColour(Colours::white.withAlpha((float)strokeValue / 127.0f));
	g.drawEllipse(r, 2.0f);
}

}