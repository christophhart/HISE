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

namespace hise { using namespace juce;

struct VoiceCpuBpmComponent::InternalSleepListener : public ControlledObject,
													 public JavascriptThreadPool::SleepListener	
{
	InternalSleepListener(VoiceCpuBpmComponent& parent_):
	ControlledObject(parent_.getMainController()),
		parent(parent_)
	{
		getMainController()->getJavascriptThreadPool().addSleepListener(this);
	}

	~InternalSleepListener()
	{
		getMainController()->getJavascriptThreadPool().removeSleepListener(this);
	}

	void sleepStateChanged(const Identifier& callback, int lineNumber, bool isSleeping) override
	{
		if (isSleeping)
		{
			sleepText = callback + "(" + String(lineNumber) + ")";
		}
		else
			sleepText = "";

		for (int i = 0; i < parent.getNumChildComponents(); i++)
			parent.getChildComponent(i)->setVisible(!isSleeping);
	}

	VoiceCpuBpmComponent& parent;
	String sleepText;
};


VoiceCpuBpmComponent::VoiceCpuBpmComponent(MainController *mc_) :
	PreloadListener(mc_->getSampleManager()),
	ControlledObject(mc_),
	il(new InternalSleepListener(*this))
{
	if (mc_ != nullptr)
	{
		mainControllers.add(mc_);

		preloadActive = mc_->getSampleManager().isPreloading();

		getMainController()->getDebugLogger().addListener(this);

	}


	addAndMakeVisible(cpuSlider = new VuMeter());

	cpuSlider->setColour(VuMeter::backgroundColour, Colours::transparentBlack);
	cpuSlider->setColour(VuMeter::ColourId::ledColour, Colours::white.withAlpha(0.45f));
	cpuSlider->setColour(VuMeter::ColourId::outlineColour, Colours::white.withAlpha(0.4f));

	cpuSlider->setOpaque(false);

	addAndMakeVisible(voiceLabel = new Label());

	voiceLabel->setColour(Label::ColourIds::outlineColourId, Colours::white.withAlpha(0.4f));
	voiceLabel->setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.7f));
	voiceLabel->setColour(Label::ColourIds::backgroundColourId, Colours::transparentBlack);
	voiceLabel->setFont(GLOBAL_FONT().withHeight(10.0f));

	voiceLabel->setEditable(false);

	addAndMakeVisible(bpmLabel = new Label());

	bpmLabel->setColour(Label::ColourIds::outlineColourId, Colours::white.withAlpha(0.4f));
	bpmLabel->setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.7f));
	bpmLabel->setColour(Label::ColourIds::backgroundColourId, Colours::transparentBlack);
	bpmLabel->setFont(GLOBAL_FONT().withHeight(10.0f));

	bpmLabel->setEditable(false);

	addAndMakeVisible(panicButton = new ShapeButton("Panic", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colour(SIGNAL_COLOUR)));

	Path panicPath;
	panicPath.loadPathFromData(HiBinaryData::FrontendBinaryData::panicButtonShape, sizeof(HiBinaryData::FrontendBinaryData::panicButtonShape));

	panicButton->setShape(panicPath, true, true, false);

	panicButton->addListener(this);


	static const unsigned char midiPathData[] = { 110, 109, 0, 0, 226, 66, 92, 46, 226, 67, 98, 0, 0, 226, 66, 112, 2, 227, 67, 79, 80, 223, 66, 92, 174, 227, 67, 0, 0, 220, 66, 92, 174, 227, 67, 98, 177, 175, 216, 66, 92, 174, 227, 67, 0, 0, 214, 66, 112, 2, 227, 67, 0, 0, 214, 66, 92, 46, 226, 67, 98, 0, 0, 214, 66, 72, 90, 225, 67, 177, 175, 216, 66, 92, 174, 224, 67, 0, 0,
		220, 66, 92, 174, 224, 67, 98, 79, 80, 223, 66, 92, 174, 224, 67, 0, 0, 226, 66, 72, 90, 225, 67, 0, 0, 226, 66, 92, 46, 226, 67, 99, 109, 0, 128, 218, 66, 92, 110, 222, 67, 98, 0, 128, 218, 66, 112, 66, 223, 67, 79, 208, 215, 66, 92, 238, 223, 67, 0, 128, 212, 66, 92, 238, 223, 67, 98, 177, 47, 209, 66, 92, 238, 223, 67, 0,
		128, 206, 66, 112, 66, 223, 67, 0, 128, 206, 66, 92, 110, 222, 67, 98, 0, 128, 206, 66, 72, 154, 221, 67, 177, 47, 209, 66, 92, 238, 220, 67, 0, 128, 212, 66, 92, 238, 220, 67, 98, 79, 208, 215, 66, 92, 238, 220, 67, 0, 128, 218, 66, 72, 154, 221, 67, 0, 128, 218, 66, 92, 110, 222, 67, 99, 109, 0, 128, 203, 66, 92, 142, 220,
		67, 98, 0, 128, 203, 66, 112, 98, 221, 67, 79, 208, 200, 66, 92, 14, 222, 67, 0, 128, 197, 66, 92, 14, 222, 67, 98, 177, 47, 194, 66, 92, 14, 222, 67, 0, 128, 191, 66, 112, 98, 221, 67, 0, 128, 191, 66, 92, 142, 220, 67, 98, 0, 128, 191, 66, 72, 186, 219, 67, 177, 47, 194, 66, 92, 14, 219, 67, 0, 128, 197, 66, 92, 14, 219,
		67, 98, 79, 208, 200, 66, 92, 14, 219, 67, 0, 128, 203, 66, 72, 186, 219, 67, 0, 128, 203, 66, 92, 142, 220, 67, 99, 109, 0, 128, 188, 66, 92, 110, 222, 67, 98, 0, 128, 188, 66, 112, 66, 223, 67, 79, 208, 185, 66, 92, 238, 223, 67, 0, 128, 182, 66, 92, 238, 223, 67, 98, 177, 47, 179, 66, 92, 238, 223, 67, 0, 128, 176, 66,
		112, 66, 223, 67, 0, 128, 176, 66, 92, 110, 222, 67, 98, 0, 128, 176, 66, 72, 154, 221, 67, 177, 47, 179, 66, 92, 238, 220, 67, 0, 128, 182, 66, 92, 238, 220, 67, 98, 79, 208, 185, 66, 92, 238, 220, 67, 0, 128, 188, 66, 72, 154, 221, 67, 0, 128, 188, 66, 92, 110, 222, 67, 99, 109, 0, 0, 181, 66, 92, 46, 226, 67, 98, 0, 0, 181,
		66, 112, 2, 227, 67, 79, 80, 178, 66, 92, 174, 227, 67, 0, 0, 175, 66, 92, 174, 227, 67, 98, 177, 175, 171, 66, 92, 174, 227, 67, 0, 0, 169, 66, 112, 2, 227, 67, 0, 0, 169, 66, 92, 46, 226, 67, 98, 0, 0, 169, 66, 72, 90, 225, 67, 177, 175, 171, 66, 92, 174, 224, 67, 0, 0, 175, 66, 92, 174, 224, 67, 98, 79, 80, 178, 66, 92, 174,
		224, 67, 0, 0, 181, 66, 72, 90, 225, 67, 0, 0, 181, 66, 92, 46, 226, 67, 99, 109, 0, 128, 197, 66, 151, 79, 215, 67, 98, 243, 139, 173, 66, 151, 79, 215, 67, 0, 0, 154, 66, 148, 50, 220, 67, 0, 0, 154, 66, 151, 47, 226, 67, 98, 0, 0, 154, 66, 154, 44, 232, 67, 243, 139, 173, 66, 151, 15, 237, 67, 0, 128, 197, 66, 151, 15, 237,
		67, 98, 12, 116, 221, 66, 151, 15, 237, 67, 0, 0, 241, 66, 154, 44, 232, 67, 0, 0, 241, 66, 151, 47, 226, 67, 98, 0, 0, 241, 66, 148, 50, 220, 67, 13, 116, 221, 66, 151, 79, 215, 67, 0, 128, 197, 66, 151, 79, 215, 67, 99, 109, 0, 128, 197, 66, 151, 79, 218, 67, 98, 209, 247, 214, 66, 151, 79, 218, 67, 0, 0, 229, 66, 163, 209,
		221, 67, 0, 0, 229, 66, 151, 47, 226, 67, 98, 0, 0, 229, 66, 139, 141, 230, 67, 210, 247, 214, 66, 151, 15, 234, 67, 0, 128, 197, 66, 151, 15, 234, 67, 98, 47, 8, 180, 66, 151, 15, 234, 67, 0, 0, 166, 66, 139, 141, 230, 67, 0, 0, 166, 66, 151, 47, 226, 67, 98, 0, 0, 166, 66, 163, 209, 221, 67, 47, 8, 180, 66, 151, 79, 218, 67,
		0, 128, 197, 66, 151, 79, 218, 67, 99, 101, 0, 0 };

	Path midiPath;



	midiPath.loadPathFromData(midiPathData, sizeof(midiPathData));

	addAndMakeVisible(midiButton = new ShapeButton("MIDI Input", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));

	midiButton->setShape(midiPath, true, true, false);

	midiButton->setEnabled(false);

	panicButton->setTooltip("MIDI Panic (all notes off)");

	midiButton->setTooltip("MIDI Activity LED");

	setSize(114, 28);

	startTimer(300);
}



VoiceCpuBpmComponent::~VoiceCpuBpmComponent()
{
	getMainController()->getDebugLogger().removeListener(this);
}

void VoiceCpuBpmComponent::buttonClicked(Button *)
{
	for (int i = 0; i < mainControllers.size(); i++)
	{
		if (mainControllers[i].get() == nullptr)
		{
			mainControllers.remove(i--);
			continue;
		}

		mainControllers[i]->allNotesOff(true);
	}

}


void VoiceCpuBpmComponent::timerCallback()
{
	if (preloadActive)
	{
		repaint();
	}
	else
	{
		cpuSlider->setColour(VuMeter::backgroundColour, findColour(Slider::backgroundColourId));
		voiceLabel->setColour(Label::ColourIds::backgroundColourId, findColour(Slider::backgroundColourId));

		double totalUsage = 0.0;
		int totalVoiceAmount = 0;

		for (int i = 0; i < mainControllers.size(); i++)
		{
			if (mainControllers[i].get() == nullptr)
			{
				mainControllers.remove(i--);
				continue;
			}

			totalUsage += mainControllers[i]->getCpuUsage() / 100.0f;
			totalVoiceAmount += mainControllers[i]->getNumActiveVoices();
		}

		cpuSlider->setPeak(((float)totalUsage));
		voiceLabel->setText(String(totalVoiceAmount), dontSendNotification);

		if (mainControllers.size() != 0)
		{
			MainController* mc = mainControllers.getFirst();

			bpmLabel->setText(String(mc->getBpm(), 0), dontSendNotification);

			bpmLabel->setText(String(mc->getBpm(), 0), dontSendNotification);

			const bool midiFlag = mc->checkAndResetMidiInputFlag();

			Colour c = midiFlag ? Colour(SIGNAL_COLOUR) : Colours::white.withAlpha(0.6f);

			midiButton->setColours(c, c, c);
			midiButton->repaint();
		}

		repaint();
	}
}

void VoiceCpuBpmComponent::resized()
{
	panicButton->setBounds(0, 0, 12, 12);
	midiButton->setBounds(0, 14, 12, 12);
	voiceLabel->setBounds(17, 13, 30, 14);
	bpmLabel->setBounds(48, 13, 30, 14);
	cpuSlider->setBounds(79, 13, 30, 14);
}



void VoiceCpuBpmComponent::paint(Graphics& g)
{
	if (!preloadActive && il->sleepText.isEmpty())
	{
		if (isOpaque()) g.fillAll(findColour(Slider::ColourIds::backgroundColourId));


		if (isRecording)
		{
			g.setColour(Colours::red.withAlpha(0.3f));
			g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
		}

		g.setColour(Colours::white.withAlpha(0.4f));
		g.setFont(GLOBAL_BOLD_FONT().withHeight(10.0f));

#if JUCE_WINDOWS

		const int x1 = 18;
		const int x2 = 42;
		const int x3 = 74;

		const int y = 1;

#else

		const int x1 = 16;
		const int x2 = 44;
		const int x3 = 76;

		const int y = 3;

#endif

		g.drawText("Voices", x1, y, 50, 11, Justification::left, true);
		g.drawText("BPM", x2, y, 30, 11, Justification::right, true);
		g.drawText("CPU", x3, y, 30, 11, Justification::right, true);
	}
	
}

void VoiceCpuBpmComponent::paintOverChildren(Graphics& g)
{
	if (preloadActive || il->sleepText.isNotEmpty())
	{
		g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId));

		g.setColour(Colour(0xFF444444));

		g.drawRect(Rectangle<int>(0, 0, getWidth(), getHeight()), 1);

		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_FONT());

		auto message = getCurrentErrorMessage();

		if (il->sleepText.isNotEmpty())
			message = il->sleepText;

		if (message.isNotEmpty())
		{
			g.drawText(message, getLocalBounds(), Justification::centred);
		}
		else
		{
			const auto progress = mainControllers.getFirst()->getSampleManager().getPreloadProgress();
			const auto percent = roundToInt(progress*100.0);

			g.drawText("Preloading: " + String(percent) + "%", getLocalBounds(), Justification::centred);
		}


		

	}
	else
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_FONT().withHeight(10.0f));

		double cpuUsage = 0.0;

		for (int i = 0; i < mainControllers.size(); i++)
		{
			if (mainControllers[i].get() == nullptr)
			{
				mainControllers.remove(i--);
				continue;
			}

			cpuUsage += mainControllers[i]->getCpuUsage();
		}

		g.drawText(String(cpuUsage, 1) + "%", cpuSlider->getBounds(), Justification::centred, true);
	}

	
}

void VoiceCpuBpmComponent::setMainControllers(Array<WeakReference<MainController>>& newMainControllers)
{
	mainControllers.swapWith(newMainControllers);
}

void VoiceCpuBpmComponent::preloadStateChanged(bool isPreloading)
{
	preloadActive = isPreloading;

	if (preloadActive)
	{
		startTimer(30);
	}
	else
		startTimer(300);

	repaint();
}

void VoiceCpuBpmComponent::recordStateChanged(DebugLogger::Listener::RecordState state)
{
	isRecording = state == DebugLogger::Listener::RecordState::RecordingMidi ||
		          state == DebugLogger::Listener::RecordState::RecordingAudio;
	repaint();
}

void VoiceCpuBpmComponent::mouseDown(const MouseEvent& e)
{
	if (il->sleepText.isNotEmpty())
	{
		getMainController()->getJavascriptThreadPool().resume();
	}
}

} // namespace hise