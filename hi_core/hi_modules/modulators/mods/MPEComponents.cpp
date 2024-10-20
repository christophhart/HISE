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

#if USE_BACKEND
MARKDOWN_CHAPTER(MpeHelp)
START_MARKDOWN(Help);
ML("# How to use MPE");
ML("MPE (MIDI Polyphonic Expression) is an extension of the old MIDI standard which allows polyphonic modulation of notes.");
ML("");
ML("## Summary");
ML("If MPE is enabled, the MPE-capable MIDI hardware will assign each note a dedicated MIDI channel cycling from 2-16 (there are other MPE modes called Zones, but they aren't supported in HISE).");
ML("It will then transmit this data on the respective MIDI channel:");
ML("");
ML("| Icon | Name | MIDI message | Description |");
ML("| - | --- | ---- | ---------- |");
ML("| ![](Slide) | Slide | Pitchbend | A horizontal movement on the MIDI controller |");
ML("| ![](Glide) | Glide | CC #74 | A vertical movement on the MIDI controller |");
ML("| ![](Press) | Press | Aftertouch | the force that is applied to the note |");
ML("| ![](Stroke) | Stroke | Velocity | the velocity that you hit the note |");
ML("| ![](Lift) | Lift | Release Velocity | the pressure level when the note is released |");
ML("");
ML("These 5 gesture types are transmitted and can be interpreted by a MPE Modulator, which is an envelope modulator. It will automatically detect the MIDI channel and assign each MIDI message to the respective voices.");
ML("");
ML("## How to use MPE modulators in your instrument");
ML("");
ML("The idea behind the MPE system in HISE is that you **add MPE modulators into every slot that you want to control via MPE**. Then you use this panel to assign and enable each modulator. Don't worry about performance, if the MPE modulators are bypassed, they don't waste (hardly) any CPU cycles. This panel is a floating tile and is supposed to be embedded in your compiled plugin to give the end user the possibility to thorougly tweak and control the MPE mappings. All MPE mappings are stored in a user preset (similar to the MIDI Learn settings) so this can be used as part of the preset design.");
ML(">There is a global **MPE Enabled** state which can be used to toggle *all* MPE modulators at once.");
ML("");
ML("## The MPE Modulator table");
ML("");
ML("Below you see a list of all enabled MPE connections. You can set these attributes for each connection:");
ML("");
ML("| Column | Description |");
ML("| -- | ----------- |");
ML("| **Target**| The modulator name. It will \"uncamelcase\" the modulator ID and remove a trailing \"MPE\", so `SustainGainModMPE` will become `Sustain Gain Mod`. |");
ML("| **Meter** | The current modulation value. |");
ML("| **Gesture** | The type of gesture that controls this modulator: Slide, Glide, Press, Stroke or Lift |");
ML("| **Mode** | By default, the MPE modulation is polyphonic, but you might want to use a monophonic mode (Legato or Retrigger) if the modulation target is not polyphonic itself (eg. when modulating a master effect) or if you want this particular behaviour. | ");
ML("| **Intensity** | This controls the amount of modulation that is applied to the signal. Normally the range is from 0 to 100%, but when the modulation target is pitch, you can control the pitch amount in semitones. |");
ML("| **Curve** | a preview of the modulator's curve that can be edited by clicking on the row. |");
ML("| **Smoothing** | this applies a low pass to the modulation signal to smooth out the edges. |");
ML("| **Default** | The default value that the modulation value will be set to when the voice is started. |");
ML("If you click on a row, it will open a editor for the curve as well as a plotter that shows the last seconds of the modulation signal so you get a visual feedback of how the modulator works. You can right click on the plotter to change the speed / freeze the current state.");
ML("You can press Escape to close the editors and delete to remove the current connection.");
ML("");
ML("## Advanced Tricks");
ML("#### Assign UI sliders to MPE modulators");
ML("A common practice in plugins is to have a right-click context menu with a MIDI learn / assign to controller function. This can be also used for adding / removing MPE connections using a simple trick:");
ML("> If a sliders name (= the `text` property) matches the modulator ID (minus a MPE-suffix), the MPE modulator can be added / removed in the slider's context menu.");
ML("So if we have a slider / knob on our interface that has the name `SustainVolume`, all we need to do is to add a MPE modulator in the gain chain and name it `SustainVolumeMPE`. You can of course add this modulator to the cutoff modulation of a filter, but that would be misleading; the only connection they have is this name and it's your responsibility to match their functionality.");
ML("");
ML("#### Use a MPE modulator as additional envelope");
ML("By setting the default value to 1.0 and the smoothing to a bigger value, the MPE modulator will have some sort of \"attack\", which can be used as additional envelope.");
ML("");
ML("#### Add MPE support for scripts");
ML("The inbuilt script processors in HISE support the MPE protocol, but if you have custom MIDI scripts, changes are big that you have to modify them in order to make them work with MPE enabled.");
ML("");
ML("In 99% of all cases, you just need to store the MIDI channel along with the MIDI note and process that information:");
ML("");
ML("```javascript");
ML("// BEFORE");
ML("local lastNote = Message.getNoteNumber();");
ML("Synth.addNoteOn(1, lastNote, 127, 0);");
ML("");
ML("// AFTER");
ML("local lastNote = Message.getNoteNumber();");
ML("local lastChannel = Message.getChannel();");
ML("Synth.addNoteOn(lastChannel, lastNote, 127, 0);");
ML("```");
ML("");
ML("You can use the API call `Engine.isMpeEnabled()` to find out whether you need to bother about this at all. The most recommended way is to branch using this method and put your original code into the `false` branch like this:");
ML("");
ML("```javascript");
ML("if(Engine.isMpeEnabled())");
ML("{");
ML("	local lastNote = Message.getNoteNumber();");
ML("	local lastChannel = Message.getChannel();");
ML("	Synth.addNoteOn(lastChannel, lastNote, 127, 0);");
ML("}");
ML("else");
ML("{");
ML("	local lastNote = Message.getNoteNumber();");
ML("	Synth.addNoteOn(1, lastNote, 127, 0);");
ML("}");
ML("```");
END_MARKDOWN()
END_MARKDOWN_CHAPTER()
#endif

juce::Path MPEPanel::Factory::createPath(const String& id) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);
	Path p;

	LOAD_EPATH_IF_URL("delete", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
	LOAD_EPATH_IF_URL("bypass", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	LOAD_EPATH_IF_URL("stroke", MpeIcons::stroke);
	LOAD_EPATH_IF_URL("press", MpeIcons::press);
	LOAD_EPATH_IF_URL("glide", MpeIcons::glide);
	LOAD_EPATH_IF_URL("lift", MpeIcons::lift);
	LOAD_EPATH_IF_URL("slide", MpeIcons::slide);

	return p;
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


	listbox.setWantsKeyboardFocus(true);
	listbox.setModel(&m);
	listbox.setRowHeight(36);
	listbox.setColour(ListBox::ColourIds::backgroundColourId, Colours::transparentBlack);
	

	setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white.withAlpha(0.5f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::textColour, Colours::white);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.1f));

	addAndMakeVisible(currentTable);
	addAndMakeVisible(listbox);

	

#if USE_BACKEND

	helpButton = new MarkdownHelpButton();

	helpButton->attachTo(&enableMPEButton, MarkdownHelpButton::OverlayRight);

	helpButton->setHelpText<PathProvider<Factory>>(MpeHelp::Help());
	helpButton->setPopupWidth(600);
#endif
	

	updateTableColours();

	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);
}

void MPEPanel::updateTableColours()
{
	currentTable.setUseFlatDesign(true);
	currentTable.setColour(TableEditor::ColourIds::bgColour, laf.fillColour.withAlpha(0.05f));
	currentTable.setColour(TableEditor::ColourIds::fillColour, laf.fillColour);
	currentTable.setColour(TableEditor::ColourIds::lineColour, laf.lineColour);
	listbox.getViewport()->getVerticalScrollBar().setColour(ScrollBar::ColourIds::thumbColourId, laf.fillColour);
}

void MPEPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	laf.bgColour = findPanelColour(FloatingTileContent::PanelColourId::bgColour);
	laf.lineColour = findPanelColour(FloatingTileContent::PanelColourId::itemColour1);
	laf.textColour = findPanelColour(FloatingTileContent::PanelColourId::textColour);
	laf.fillColour = findPanelColour(FloatingTileContent::PanelColourId::itemColour2);

	laf.font = getFont();

	listbox.setRowHeight(roundToInt(getFont().getHeight() * 2.2f));

	updateTableColours();
}

bool MPEPanel::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey)
	{
		setCurrentMod(nullptr);
		listbox.deselectAllRows();
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
			Rectangle<int> rectangles[9] = { tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(80),
				tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(50),
				tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(100),
				tableHeader.removeFromLeft(80),
				tableHeader };

			g.setColour(laf.fillColour.withAlpha(0.1f));

			for (int i = 0; i < 9; i++)
			{
				g.fillRect(rectangles[i].reduced(1));
			}


			g.setColour(laf.textColour);
			g.setFont(laf.font);


			g.drawText("Target", rectangles[0], Justification::centred);
			
			g.drawText("Gesture", rectangles[1], Justification::centred);
			
			g.drawText("Mode", rectangles[2], Justification::centred);
			
			g.drawText("Curve", rectangles[3], Justification::centred);
			g.drawText("Intensity", rectangles[4], Justification::centred);
			g.drawText("Smoothing", rectangles[5], Justification::centred);
			g.drawText("Default", rectangles[6], Justification::centred);
			g.drawText("Meter", rectangles[7], Justification::centred);
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

			g.drawText("Curve", bottomBar.removeFromLeft(getWidth() / 2), Justification::centred);
			g.drawText("Plot", bottomBar.removeFromLeft(getWidth() / 2), Justification::centred);
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
			addAndMakeVisible(currentPlotter = new Plotter(getMainController()->getGlobalUIUpdater()));

			newMod->setPlotter(currentPlotter);

			currentPlotter->setFont(getFont());

			currentPlotter->setColour(Plotter::ColourIds::textColour, laf.textColour);
			currentPlotter->setColour(Plotter::ColourIds::pathColour, laf.fillColour.withMultipliedAlpha(1.05f));
			currentPlotter->setColour(Plotter::ColourIds::pathColour2, laf.fillColour);
			currentPlotter->setColour(Plotter::ColourIds::backgroundColour, laf.fillColour.withAlpha(0.05f));
			currentTable.setColour(TableEditor::ColourIds::bgColour, laf.fillColour.withAlpha(0.05f));
		}

		if(newMod != nullptr)
			ProcessorHelpers::connectTableEditor(currentTable, newMod);
        
		repaint();
		resized();
	}
}

void MPEPanel::mpeModeChanged(bool isEnabled)
{
    notifier.isEnabled = isEnabled;
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
		return SafeFunctionCall::OK;
	};

	getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);

}

void MPEPanel::cancelRefresh()
{
	notifier.cancelRefresh(true);
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
		parent.enableMPEButton.setToggleState(isEnabled, dontSendNotification);
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
				m->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);
			}

			return SafeFunctionCall::OK;
		};

		mod->getMainController()->getKillStateHandler().killVoicesAndCall(mod, f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
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

		auto xml = XmlDocument::parse(clipboardContent);

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
			mod->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);

		}
		else if (result == 4)
		{
			auto exportedData = mod->exportAsValueTree().createXml();
			SystemClipboard::copyTextToClipboard(exportedData->createDocument(""));

		}
		else if (result == 5)
		{
			if (xml != nullptr && xml->getTagName() == "Processor" && xml->hasAttribute("ID"))
			{
				xml->setAttribute("ID", mod->getId());
				ValueTree v = ValueTree::fromXml(*xml);

				mod->restoreFromValueTree(v);
				mod->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);
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
			Component::SafePointer<ListBox> lb = findParentComponentOfClass<ListBox>();

			jassert(lb != nullptr);

			auto f = [lb](Processor* p)
			{
				auto& data = p->getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData();

				WeakReference<MPEModulator> m = dynamic_cast<MPEModulator*>(p);

				data.addConnection(m.get());

				auto updateF = [lb, m]()
				{
					if (lb.getComponent() != nullptr)
					{
						auto index = lb->getModel()->getNumRows() - 2;

						auto par = lb->findParentComponentOfClass<MPEPanel>();

						if (par != nullptr)
						{
							par->cancelRefresh();
							par->setCurrentMod(m);
							lb.getComponent()->selectRow(index);
							lb.getComponent()->getViewport()->setViewPositionProportionately(0.0, 1.0);
						}
							
					}
				};

				new DelayedFunctionCaller(updateF, 51);

				return SafeFunctionCall::OK;
			};

			mod->getMainController()->getKillStateHandler().killVoicesAndCall(mod, f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
		}
	}
}

MPEPanel::Model::Row::Row(MPEModulator* mod_, LookAndFeel& laf_) :
	OtherListener(mod_, dispatch::library::ProcessorChangeEvent::Custom),
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

	if (mod->getMode() == Modulation::PanMode)
	{
		modeSelector.addItem("Polyphonic Bipolar", 4);
		modeSelector.addItem("Legato Bipolar", 5);
		modeSelector.addItem("Retrigger Bipolar", 6);
	}



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

	auto mode = mod->getMode();

	if (mode == Modulation::GainMode)
	{
		intensity.setMode(HiSlider::NormalizedPercentage);
		defaultValue.setMode(HiSlider::NormalizedPercentage);
	}
	else if (mode == Modulation::PitchMode)
	{
		intensity.setMode(HiSlider::Linear, -12.0, 12.0, 0.0, 0.01);
		intensity.setTextValueSuffix(" st.");

		defaultValue.setMode(HiSlider::Linear, -12.0, 12.0, 0.0, 0.01);
		defaultValue.setTextValueSuffix(" st.");
	}
	else if (mode == Modulation::PanMode)
	{
		intensity.setMode(HiSlider::Pan);
		defaultValue.setMode(HiSlider::Pan);
	}
	else if (mode == Modulation::GlobalMode)
	{
		intensity.setMode(HiSlider::NormalizedPercentage);
		defaultValue.setMode(HiSlider::NormalizedPercentage);
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

    ProcessorHelpers::connectTableEditor(curvePreview, mod);

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

	otherChange(mod);
}


MPEPanel::Model::Row::~Row()
{
}

void MPEPanel::Model::Row::resized()
{
	auto ar = getLocalBounds();

	int buttonWidth = getHeight();

	const int margin = 2;


	ar.removeFromLeft(100);

	
	selector.setBounds(ar.removeFromLeft(80));
	modeSelector.setBounds(ar.removeFromLeft(100).reduced(margin));
	curvePreview.setBounds(ar.removeFromLeft(50).reduced(margin));
	intensity.setBounds(ar.removeFromLeft(100).reduced(margin));
	
	smoothingTime.setBounds(ar.removeFromLeft(100).reduced(margin));
	defaultValue.setBounds(ar.removeFromLeft(100).reduced(margin));
	output.setBounds(ar.removeFromLeft(80).reduced(margin));
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
		const bool isBipolarPan = mod->isBipolar() && mod->getMode() == Modulation::PanMode;

		int valueToUse = isMonophonic ? (shouldRetrigger ? 3 : 2) : 1;

		if (isBipolarPan)
			valueToUse += 3;


		modeSelector.setSelectedId(valueToUse, dontSendNotification);

		intensity.setEnabled(enabled);
		smoothingTime.setEnabled(enabled);
		selector.setEnabled(enabled);

		repaint();
	}
}


void MPEPanel::Model::Row::paint(Graphics& g)
{
	auto ar = getLocalBounds();
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
	auto result = comboBoxThatHasChanged->getSelectedId() - 1;

	const bool shouldBeBipolar = result >= 3;
	result = result % 3;

	if(mod->getMode() != Modulation::GainMode)
		mod->setIsBipolar(shouldBeBipolar);

	if (result == 0)
	{
		mod->setAttribute(EnvelopeModulator::Parameters::Monophonic, false, sendNotificationSync);
	}
	else if (result == 1)
	{
		mod->setAttribute(EnvelopeModulator::Parameters::Monophonic, true, dontSendNotification);
		mod->setAttribute(EnvelopeModulator::Parameters::Retrigger, false, sendNotificationSync);
	}
	else if (result == 2)
	{
		mod->setAttribute(EnvelopeModulator::Parameters::Monophonic, true, dontSendNotification);
		mod->setAttribute(EnvelopeModulator::Parameters::Retrigger, true, sendNotificationSync);
	}
}

bool MPEPanel::Model::Row::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::upKey || key == KeyPress::downKey)
	{
		if (auto lb = findParentComponentOfClass<ListBox>())
			return lb->keyPressed(key);
	}

	return false;
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
				m->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);
			}

			return SafeFunctionCall::OK;
		};

		findParentComponentOfClass<MPEPanel>()->setCurrentMod(nullptr);

		mod->getMainController()->getKillStateHandler().killVoicesAndCall(mod, f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
	}
}

MPEKeyboard::MPEKeyboard(MainController* mc) :
	state(mc->getKeyboardState()),
	pendingMessages(1024),
	channelRange({2, 16})
{
	suspend(false);

	state.addListener(this);

	setLookAndFeel(&dlaf);

	setColour(bgColour, Colours::black);
	setColour(waveColour, Colours::white.withAlpha(0.5f));
	setColour(keyOnColour, Colours::white);
	setColour(dragColour, Colour(SIGNAL_COLOUR));
}

MPEKeyboard::~MPEKeyboard()
{
	setLookAndFeel(nullptr);
	state.removeListener(this);
}

void MPEKeyboard::handleAsyncUpdate()
{
	if (!pendingMessages.isEmpty())
	{
		MidiMessage m;

		auto f = [this](MidiMessage& m)
		{
			if (!appliesToRange(m))
				return MultithreadedQueueHelpers::OK;

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

			return MultithreadedQueueHelpers::OK;
		};

		pendingMessages.clear(f);

		repaint();
	}
}

void MPEKeyboard::handleNoteOn(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity)
{
	pendingMessages.push(MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity));
	triggerAsyncUpdate();
}

void MPEKeyboard::handleNoteOff(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity)
{
	pendingMessages.push(MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity));
	triggerAsyncUpdate();
}

void MPEKeyboard::handleMessage(const MidiMessage& m)
{
	MidiMessage copy(m);
	pendingMessages.push(std::move(copy));
	triggerAsyncUpdate();
}

void MPEKeyboard::mouseDown(const MouseEvent& e)
{
	auto n = Note::fromMouseEvent(*this, e, nextChannelIndex);
    
    pressedNotes.insert(n);
    
	n.sendNoteOn(state);

	nextChannelIndex++;

	if (nextChannelIndex > channelRange.getEnd())
		nextChannelIndex = channelRange.getStart();

	repaint();
}

void MPEKeyboard::mouseUp(const MouseEvent& e)
{
    bool found = false;
    
	for (int i = 0; i < pressedNotes.size(); i++)
	{
		if (pressedNotes[i] == e)
		{
			pressedNotes[i].sendNoteOff(state);
			pressedNotes.removeElement(i--);
            found = true;
			break;
		}
	}
    
    if(!found)
    {
        DBG("Kill all voices to be sure");
        
        for(int i = 0; i < pressedNotes.size(); i++)
        {
            pressedNotes[i].sendNoteOff(state);
        }
        
        pressedNotes.clear();
    }
    
    
	repaint();
}

void MPEKeyboard::mouseDrag(const MouseEvent& event)
{
	for (auto& s : pressedNotes)
		s.updateNote(*this, event);

	repaint();
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
    n.pressureValue = e.isPressureValid() ? (int)(e.pressure * 127.0f) : 0;

	n.startPoint = { (int)p.getPositionForNote(n.noteNumber).getCentreX(), e.getMouseDownY() };

    DBG("Finger: " + String(n.fingerIndex));

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
	glideValue = jlimit<int>(0, 127, 64 + roundToInt(normalisedGlide * 127.0f));

    if(e.isPressureValid())
    {
        
        pressureValue = jlimit<int>(0, 127, (int)(e.pressure * 127.0f));
        
        p.state.injectMessage(MidiMessage::channelPressureChange(assignedMidiChannel, pressureValue));
    }
    
    
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

void MPEKeyboard::DefaultLookAndFeel::drawKeyboard(MPEKeyboard& keyboard, Graphics& g)
{
	Colour c1 = keyboard.findColour(bgColour).withMultipliedAlpha(1.1f);
	Colour c2 = keyboard.findColour(bgColour).withMultipliedAlpha(0.9f);

	g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)keyboard.getHeight(), false));
	g.fillAll();

	static const int whiteWave[24] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0,
		0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

	for (int i = 0; i < 24; i++)
	{
		auto noteNumber = keyboard.lowKey + i;

		auto l = keyboard.getPositionForNote(noteNumber);

		if (keyboard.isShowingOctaveNumbers() && noteNumber % 12 == 0)
		{
			g.setFont(Font((float)l.getWidth() / 2.5f));
			g.setColour(keyboard.findColour(waveColour));
			g.drawText(MidiMessage::getMidiNoteName(noteNumber, true, true, 3), l.withHeight(l.getHeight() - 10), Justification::centredBottom);
		}

		const float radius = keyboard.getWidthForNote() * 0.2f;

		if (whiteWave[i] == 1)
		{
			g.setColour(keyboard.findColour(waveColour));
			g.drawLine(l.getCentreX(), radius, l.getCentreX(), l.getHeight() - 2.0f*radius, 4.0f);
		}
		else if (whiteWave[i] == 0)
		{
			l.reduce(4, 3);

			g.setColour(keyboard.findColour(waveColour).withMultipliedAlpha(0.1f));
			g.fillRoundedRectangle(l, radius);
		}
	}
}


void MPEKeyboard::paint(Graphics& g)
{
	MPEKeyboardLookAndFeel* laf = dynamic_cast<MPEKeyboardLookAndFeel*>(&getLookAndFeel());

	if (laf == nullptr)
		laf = &dlaf;

	laf->drawKeyboard(*this, g);

	for (const auto& n : pressedNotes)
	{
		if (!n.isVisible(*this))
			continue;

		auto area = getPositionForNote(n.noteNumber);

		laf->drawNote(*this, n, g, area);
	}
}


void MPEKeyboard::DefaultLookAndFeel::drawNote(MPEKeyboard& keyboard, const Note& n, Graphics& g, Rectangle<float> area)
{
	g.setColour(keyboard.findColour(keyOnColour).withAlpha(0.1f + 0.9f * ((float)n.pressureValue / 127.0f)));

	area.reduce(4.0f, 3.0f);
	auto radius = keyboard.getWidthForNote() * 0.2f;

	g.fillRoundedRectangle(area, radius);
	g.setColour(keyboard.findColour(dragColour));

	auto l = Line<int>(n.startPoint, n.dragPoint);

	g.drawLine((float)l.getStartX(), (float)l.getStartY(), (float)l.getEndX(), (float)l.getEndY(), 2.0f);

	Rectangle<float> r((float)n.dragPoint.getX(), (float)n.dragPoint.getY(), 0.0f, 0.0f);
	r = r.withSizeKeepingCentre(10, 10);

	g.setColour(Colours::white.withAlpha((float)n.strokeValue / 127.0f));
	g.drawEllipse(r, 2.0f);
}

}
