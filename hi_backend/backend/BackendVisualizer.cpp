#include "BackendVisualizer.h"

#include "hi_tools/hi_tools/MiscToolClasses.h"


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


MainTopBar::ClickablePeakMeter::PopupComponent::ModeObject::ModeObject(BackendProcessor* bp, Mode m_):
	PropertyObject(bp),
	ControlledObject(bp),
	mode(m_)
{
	setProperty("ShowCpuUsage", mode == Mode::CPU);
	setProperty(scriptnode::PropertyIds::IsProcessingHiseEvent, mode == Mode::Oscilloscope);

	if(mode == Mode::Spectrogram)
	{
		setProperty("FFTSize", 4096);
		setProperty("WindowType", FFTHelpers::getAvailableWindowTypeNames()[FFTHelpers::WindowType::BlackmanHarris]);
		setProperty("Oversampling", 2.0);
		setProperty("Gamma", 0.3);
		
	}
}

var MainTopBar::ClickablePeakMeter::PopupComponent::ModeObject::getProperty(const Identifier& id) const
{
	static const Identifier showCpu("ShowCpuUsage");

	if(id == scriptnode::PropertyIds::IsProcessingHiseEvent)
		return mode == Mode::Oscilloscope;
	if(id == showCpu)
		return mode == Mode::CPU;

	return PropertyObject::getProperty(id);
}

bool MainTopBar::ClickablePeakMeter::PopupComponent::ModeObject::validateInt(const Identifier& id, int& v) const
{
	if(id == RingBufferIds::BufferLength)
	{
		switch(mode)
		{
		case Mode::Spectrogram:
			v = getMainController()->getMainSynthChain()->getSampleRate() * 4;
			return true;
		//case Mode::PitchTracking:
			//v = PitchDetection::getNumSamplesNeeded(getMainController()->getMainSynthChain()->getSampleRate(), 50.0);
		case Mode::Oscilloscope:
			v = 4096; 
			return true;
		}
	}
	if(id == RingBufferIds::NumChannels)
	{
		switch(mode)
		{
		case Mode::Spectrogram:
			v = 2;
			return true;
		case Mode::Oscilloscope:
			v = 2;
			return true;
		case Mode::CPU:
			v = 1;
			return true;
		}
	}
			
	return true;
}

MainTopBar::ClickablePeakMeter::PopupComponent::PopupComponent(ClickablePeakMeter* parent_) :
	ControlledObject(parent_->getMainController()),
	Thread("Visualiser"),
	parent(parent_),
	resizer(this, nullptr),
    ok(Result::ok())
{
	setOpaque(true);
	auto bp = dynamic_cast<BackendProcessor*>(getMainController());

	addAndMakeVisible(alphaSlider);
	alphaSlider.setRange(0.0f, 1.0f);
	alphaSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
	alphaSlider.setSliderStyle(Slider::LinearHorizontal);
	alphaSlider.setDoubleClickReturnValue(true, 0.5f);
	alphaSlider.setValue(0.5, dontSendNotification);

	

	alphaSlider.setColour(Slider::ColourIds::thumbColourId, Colour(0xffdfdfdf));
	alphaSlider.setColour(Slider::ColourIds::backgroundColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
	alphaSlider.setColour(Slider::ColourIds::trackColourId, Colour(0xff9d629a));
	

	alphaSlider.setLookAndFeel(&slaf);

	alphaSlider.onValueChange = [this]()
	{
		this->repaint();
	};

	for(int i = 0; i < 2; i++)
	{
		infos[i].add(new Spec2DInfo(bp, i));

		auto oi = new OscInfo(bp, i);
		oi->zIndex = &zeroCrossingIndex;

		infos[i].add(oi);

		infos[i].add(new EnvInfo(bp, i));
		infos[i].add(new FFTInfo(bp, i));

		auto pi = new PitchTrackInfo(bp, i);
		pi->parent = this;

		infos[i].add(pi);
		infos[i].add(new StereoInfo(bp, i));

		infos[i].add(new CpuInfo(bp, i));
	}

	for(int i = 0; i < (int)Mode::numModes; i++)
	{
		analyserInfo.add(new AnalyserInfo(infos[0][i]->rbo.get(), infos[1][i]->rbo.get()));

		infos[0][i]->info = analyserInfo.getLast();
		infos[1][i]->info = analyserInfo.getLast();
	}
	
	addAndMakeVisible(resizer);
	
	setOpaque(true);
	rebuildPeakMeters();

	addAndMakeVisible(modes.add(new TextButton("Spectral")));
	addAndMakeVisible(modes.add(new TextButton("Osc")));
	addAndMakeVisible(modes.add(new TextButton("Gain")));
	addAndMakeVisible(modes.add(new TextButton("FFT")));
	addAndMakeVisible(modes.add(new TextButton("Pitch")));
	addAndMakeVisible(modes.add(new TextButton("Stereo")));
	addAndMakeVisible(modes.add(new TextButton("CPU")));

	

	

	for (auto b : modes)
	{
		int flag = 0;

		if (b != modes.getFirst())
			flag |= Button::ConnectedEdgeFlags::ConnectedOnLeft;

		if (b != modes.getLast())
			flag |= Button::ConnectedEdgeFlags::ConnectedOnRight;

		b->setConnectedEdges(flag);
		b->setLookAndFeel(&blaf);
		b->setRadioGroupId(91231);
		b->setClickingTogglesState(true);
		b->addListener(this);
	}

	addCommand("freeze", true, "Freezes the visualizer");
	addCommand("edit", true, "Edit FFT properties of the visualizer");
	addCommand("channels", false, "Choose the module that you want to analyse");

	modes[(int)Mode::Spectrogram]->setTooltip("A FFT spectrogram that shows a spectral image of the last 4 seconds.");
	modes[(int)Mode::Oscilloscope]->setTooltip("An oscilloscope that displays a single cycle of the waveform based on the MIDI note input.");
	modes[(int)Mode::FFT]->setTooltip("A spectral analyser.");
	modes[(int)Mode::Envelope]->setTooltip("A time based amplitude envelope of the last 4 seconds with a subtle envelope follower applied to smooth the downfall rate.");
	modes[(int)Mode::PitchTracking]->setTooltip("A pitch detector that plots the root frequency of the last 4 seconds.");
	modes[(int)Mode::StereoField]->setTooltip("A goniometer and other various tools related to stereo imaging (correllation & distribution).");
	modes[(int)Mode::CPU]->setTooltip("A time plot of the CPU usage of the selected module (the PRE plot is the total filtered usage as shown in the CPU meter of HISE).");
	toolbar[(int)ToolbarCommand::EditProperties]->setTooltip("Select the properties of the current analyser mode.");
	toolbar[(int)ToolbarCommand::Freeze]->setTooltip("Freeze the current plot for detailed analysis.");
	toolbar[(int)ToolbarCommand::ShowChannels]->setTooltip("Select the module that you want to analyse and use the PRE / POST graphs to analyse the difference that the module causes.");
	alphaSlider.setTooltip("Blend between the pre and post data graph.");

	setName("Audio Analyser");
	setSize(700, 500);

	setMode(Mode::Oscilloscope);

	startThread(8);
}

MainTopBar::ClickablePeakMeter::PopupComponent::~PopupComponent()
{
	stopThread(2000);

	if(auto bp = dynamic_cast<BackendProcessor*>(getMainController()))
	{
		bp->setAnalysedProcessor(analyserData, false);
	}

	analyserData = nullptr;
	analyserInfo.clear();
	infos[0].clear();
	infos[1].clear();
	
	if(auto p = parent.getComponent())
	{
		p->toggleState = false;
		p->repaint();
	}
}

inline Path MainTopBar::ClickablePeakMeter::PopupComponent::createPath(const String& url) const
{
	Path p;

	LOAD_EPATH_IF_URL("freeze", HnodeIcons::freezeIcon);
	LOAD_PATH_IF_URL("edit", ColumnIcons::threeDots);
	LOAD_EPATH_IF_URL("channels", HiBinaryData::SpecialSymbols::routingIcon);

	return p;
}

void MainTopBar::ClickablePeakMeter::PopupComponent::performCommand(ToolbarCommand t, bool shouldBeOn)
{
	if(t == ToolbarCommand::Freeze)
	{
		for(auto i: infos[0])
			i->setFreeze(shouldBeOn);
		for(auto i: infos[1])
			i->setFreeze(shouldBeOn);

		repaint();
		
	}
	else if (t == ToolbarCommand::ShowChannels)
	{
		PopupMenu m;
		PopupLookAndFeel plaf;

		m.setLookAndFeel(&plaf);

		auto list = ProcessorHelpers::getListOfAllProcessors<RoutableProcessor>(getMainController()->getMainSynthChain());

		int itemId = 1;

		for(auto rp: list)
		{
			auto numChannels = rp->getMatrix().getNumSourceChannels() / 2;

			auto id = dynamic_cast<Processor*>(rp.get())->getId();

			if(numChannels == 1)
			{
				m.addItem(itemId, id);
			}
			else
			{
				PopupMenu sub;

				for(int i = 0; i < numChannels; i++)
				{
					if(rp->getMatrix().getConnectionForSourceChannel(i*2) != -1)
					{
						String s;
						s << "Ch. " << String((i*2+1)) << "/" << String(i*2+2);
						sub.addItem(itemId + i, s, true, currentChannelIndex == i);
					}
				}

				m.addSubMenu(id, sub);
			}

			itemId += 100;
		}

		if(auto result = m.show())
		{
			result -= 1;

			currentChannelIndex = result % 100;
			auto processorIndex = result / 100;

			
			auto p = dynamic_cast<Processor*>(list[processorIndex].get());
			auto bp = dynamic_cast<BackendProcessor*>(p->getMainController());

			analyserData->currentlyAnalysedProcessor = { p, currentChannelIndex };

			if(ok.wasOk())
				bp->setAnalysedProcessor(analyserData, false);

			ok = bp->setAnalysedProcessor(analyserData, true);
		}


	}
	else if (t == ToolbarCommand::EditProperties)
	{
		if(currentEditor != nullptr)
		{
			currentEditor = nullptr;
		}
		else
		{
			currentEditor = infos[0][(int)currentMode]->createEditor(infos[1][(int)currentMode]->rbo);

			addAndMakeVisible(currentEditor);
			currentEditor->setBounds(contentArea.withSizeKeepingCentre(currentEditor->getWidth(), currentEditor->getHeight()));
		}
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::addCommand(const String& t, bool isToggle, const String& tooltip)
{
	auto b = new HiseShapeButton(t, this, *this);

	b->setToggleModeWithColourChange(isToggle);
	b->setTooltip(tooltip);
	addAndMakeVisible(toolbar.add(b));
}

Rectangle<int> MainTopBar::ClickablePeakMeter::PopupComponent::getContentArea() const
{
	auto baseArea = contentArea;
	baseArea.removeFromBottom(30);
	baseArea.removeFromTop(30);
	
	switch(currentMode)
	{
	case Mode::Oscilloscope: 
		baseArea.removeFromLeft(YAxis);
		break;
	case Mode::FFT: 
	case Mode::Envelope:
		baseArea.removeFromTop(XAxis);
	    baseArea.removeFromLeft(YAxis);
		break;
	case Mode::PitchTracking: 
	{
		baseArea.removeFromTop(XAxis);
	    baseArea.removeFromLeft(YAxis);
		auto h = (float)baseArea.getHeight() / 25.0f;
		baseArea.removeFromTop(h / 2);
		baseArea.removeFromBottom(h / 2);
		break;
	}
	case Mode::Spectrogram:
		baseArea.removeFromTop(XAxis);
	    baseArea.removeFromLeft(YAxis);
		break;
	case Mode::CPU:
		baseArea.removeFromTop(XAxis);
		baseArea.removeFromLeft(YAxis);
		break;
	default: break;
	}

	return baseArea;
}

void MainTopBar::ClickablePeakMeter::PopupComponent::mouseMove(const MouseEvent& e)
{
	auto ca = getContentArea().toFloat();

	if(ca.contains(e.position))
	{
		float xnorm = (e.position.getX() - ca.getX()) / ca.getWidth();
		float ynorm;

		if(currentMode == Mode::Envelope)
		{
			auto oscArea = ca;
			auto l = oscArea.removeFromTop(oscArea.getHeight() * 0.5f);
			auto r = oscArea;
			r.removeFromTop(10);
			l.removeFromBottom(10);

			auto pos = e.position.toFloat();

			if(l.contains(pos))
				ynorm = (pos.getY() - l.getY()) / l.getHeight();
			else if(r.contains(pos))
				ynorm = (pos.getY() - r.getY()) / r.getHeight();
			else
				ynorm = 0.0f;
		}	
		else
			 ynorm = (e.position.getY() - ca.getY()) / ca.getHeight();

		currentHoverValue = { xnorm, ynorm };
	}
	else
	{
		currentHoverValue = {};
	}

	repaint();
}

void MainTopBar::ClickablePeakMeter::PopupComponent::mouseDrag(const MouseEvent& e)
{
	if(e.mouseWasDraggedSinceMouseDown())
	{
		auto area = getContentArea();
		
		currentDragInfo.active = area.contains(e.getMouseDownPosition());

		if(!currentDragInfo.active)
			return;

		currentDragInfo.isXDrag = hmath::abs(e.getDistanceFromDragStartX()) > hmath::abs(e.getDistanceFromDragStartY());

		if(e.mods.isAnyModifierKeyDown())
			currentDragInfo.isXDrag = !currentDragInfo.isXDrag;

		currentDragInfo.position = e.getPosition();
		
		auto length = (float)(currentDragInfo.isXDrag ? area.getWidth() : area.getHeight());
		auto offset = currentDragInfo.isXDrag ? area.getX() : area.getY();

		auto startPos = e.getMouseDownPosition();

		auto pixelStart = currentDragInfo.isXDrag ? startPos.getX() : startPos.getY();
		auto pixelEnd = currentDragInfo.isXDrag ? currentDragInfo.position.getX() : currentDragInfo.position.getY();

		float pos0 = (float)(pixelStart - offset) / length;
		float pos1 = (float)(pixelEnd - offset) / length;

		

		

		if(pos0 > pos1)
			std::swap(pos0, pos1);

		currentDragInfo.normalisedDistance = { jlimit(0.0f, 1.0f, pos0), jlimit(0.0f, 1.0f, pos1) };

		auto ls = e.getMouseDownPosition().toFloat();
		auto le = e.getPosition().toFloat();



		if(currentDragInfo.isXDrag)
			le = le.withY(ls.getY());
		else
			le = le.withX(ls.getX());

		if(currentDragInfo.isXDrag)
		{
			ls.setX(jlimit<float>(area.getX(), area.getRight(), ls.getX()));
			le.setX(jlimit<float>(area.getX(), area.getRight(), le.getX()));
		}
		else
		{
			ls.setY(jlimit<float>(area.getY(), area.getBottom(), ls.getY()));
			le.setY(jlimit<float>(area.getY(), area.getBottom(), le.getY()));
		}

		currentDragInfo.line = { ls, le };

		repaint();
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::setMode(Mode newMode)
{
	if(currentMode != newMode)
	{
		toolbar[(int)ToolbarCommand::EditProperties]->setToggleState(false, sendNotificationSync);

		

		auto bp = dynamic_cast<BackendProcessor*>(getMainController());
		currentMode = newMode;
		auto idx = (int)currentMode;

		toolbar[(int)ToolbarCommand::EditProperties]->setVisible(currentMode == Mode::Spectrogram || currentMode == Mode::FFT);

		alphaSlider.setEnabled(currentMode != Mode::StereoField);

		Processor* pToUse = bp->getMainSynthChain();
		
		if(analyserData != nullptr && ok.wasOk())
		{
			ScopedLock sl(lock);
			bp->setAnalysedProcessor(analyserData, false);
			pToUse = analyserData->currentlyAnalysedProcessor.first;
		}
		
		analyserData = analyserInfo[idx];
		analyserData->currentlyAnalysedProcessor = { pToUse, currentChannelIndex };

		{
			ScopedLock sl(lock);
			ok = bp->setAnalysedProcessor(analyserData, true);
		}
		
		modes[idx]->setToggleState(true, dontSendNotification);

		if(currentMode == Mode::StereoField)
		{
			for(int i = 0; i < 2; i++)
			{
				auto r = infos[i][idx]->rbo->createComponent();
				r->setComplexDataUIBase(analyserData->getAnalyserBuffer((bool)i).get());
				r->setUseCustomColours(true);
				addAndMakeVisible(gonioMeter[i] = dynamic_cast<Component*>(r));
				gonioMeter[i]->setColour(RingBufferComponentBase::ColourId::bgColour, Colours::transparentBlack);
				gonioMeter[i]->setColour(RingBufferComponentBase::ColourId::fillColour, infos[i][idx]->c);
				gonioMeter[i]->setColour(RingBufferComponentBase::ColourId::lineColour, Colours::white.withAlpha(0.1f));
			}
		}
		else
		{
			gonioMeter[0] = nullptr;
			gonioMeter[1] = nullptr;
		}

		resized();
	}
}

float MainTopBar::ClickablePeakMeter::PopupComponent::getDecibelForY(float y)
{
	Range<float> dbRange(-100.0f, 0.0f);

	y = 1.0f - y;
	y *= dbRange.getLength();
	y += dbRange.getStart();
	y = jlimit(dbRange.getStart(), dbRange.getEnd(), y);
	return y;
}

float MainTopBar::ClickablePeakMeter::PopupComponent::getYValue(float v)
{
	Range<float> dbRange(-100.0f, 0.0f);
	v = Decibels::gainToDecibels(v);
	v = jlimit<float>(dbRange.getStart(), dbRange.getEnd(), v);
	v -= dbRange.getStart();
	v /= dbRange.getLength();

	return 1.0f - v;
}

void MainTopBar::ClickablePeakMeter::PopupComponent::paintBackground(Graphics& g) const
{
	auto oscArea = getContentArea().toFloat();

	//g.setGradientFill(ColourGradient(Colour(SIGNAL_COLOUR).interpolatedWith(Colour(0xFF111111), JUCE_LIVE_CONSTANT_OFF(0.95f)), oscArea.getCentre(), Colour(0xFF111111), oscArea.getBottomLeft(), true));

	//g.setColour(blaf.bright.withAlpha(0.5f));

	


	//g.drawRoundedRectangle(contentArea.toFloat().reduced(3.0f), 10.0f, 1.0f);

	g.setColour(Colours::white.withAlpha(0.1f));
	g.setColour(Colours::white.withAlpha(0.2f));
	g.setFont(GLOBAL_FONT());

	auto xAxis = getAxisArea(true);
	auto yAxis = getAxisArea(false);

	switch(currentMode)
	{
	case Mode::Oscilloscope:
		{
			for(double d = 0.125; d <= 1.0; d += 0.125)
			{
				g.setColour(Colours::white.withAlpha(d == 0.5 ? 0.1f : 0.05f));
				g.drawHorizontalLine(oscArea.getY() + d * oscArea.getHeight(), oscArea.getX() + 20, oscArea.getRight());
				g.drawVerticalLine(oscArea.getX() + d * oscArea.getWidth(), oscArea.getY(), oscArea.getBottom());
			}

			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText("-3dB", yAxis.getX(), yAxis.getY() + 0.125 * yAxis.getHeight()-10, yAxis.getWidth() - 5, 20, Justification::right);
			g.drawText("-3dB", yAxis.getX(), yAxis.getY() + 0.875 * yAxis.getHeight()-10, yAxis.getWidth() - 5, 20, Justification::right);
			g.drawText("-6dB", yAxis.getX(), yAxis.getY() + 0.25 * yAxis.getHeight()-10, yAxis.getWidth() - 5, 20, Justification::right);
			g.drawText("-6dB", yAxis.getX(), yAxis.getY() + 0.75 * yAxis.getHeight()-10, yAxis.getWidth() - 5, 20, Justification::right);
			g.drawText("-12dB", yAxis.getX(), yAxis.getY() + 0.375 * yAxis.getHeight()-10, yAxis.getWidth() - 5, 20, Justification::right);
			g.drawText("-12dB", yAxis.getX(), yAxis.getY() + 0.625 * yAxis.getHeight()-10, yAxis.getWidth() - 5, 20, Justification::right);
			break;
		}
	case Mode::Envelope:
	{
		auto l = oscArea.removeFromTop(oscArea.getHeight() * 0.5f);
		auto r = oscArea;
		r.removeFromTop(10);
		l.removeFromBottom(10);

		g.setColour(Colours::white.withAlpha(0.05f));
		g.drawRect(l, 1.0f);
		g.drawRect(r, 1.0f);

		g.setFont(GLOBAL_FONT());

		StringArray xLabels = { "-3dB", "-6dB", "-12dB" };

		for(int i = 1; i < 4; i++)
		{
			g.setColour(Colours::white.withAlpha(0.05f));
			auto yOffset = ((float)i / 4.0f) * l.getHeight();
			g.drawHorizontalLine(l.getY() + yOffset, l.getX(), l.getRight());
			g.drawHorizontalLine(r.getY() + yOffset, l.getX(), l.getRight());

			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText(xLabels[i-1], yAxis.getX(), l.getY() + yOffset - 10, yAxis.getWidth() - 10, 20, Justification::right);
			g.drawText(xLabels[i-1], yAxis.getX(), r.getY() + yOffset - 10, yAxis.getWidth() - 10, 20, Justification::right);
		}

		for(int i = 1; i < 8; i++)
		{
			g.setColour(Colours::white.withAlpha(0.05f));
			auto xOffset = ((float)i / 8.0f) * l.getWidth();
			g.drawVerticalLine(l.getX() + xOffset, l.getY(), l.getBottom());
			g.drawVerticalLine(l.getX() + xOffset, r.getY(), r.getBottom());

			auto label = String(roundToInt(4000.0 - (float)i / 2.0f * 1000.0)) + "ms";

			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText(label, l.getX() + xOffset - 30, l.getY() - 20, 60, 20, Justification::centred);
		}

		break;
	}
	case Mode::Spectrogram:
	{
		g.setFont(GLOBAL_FONT());

		Array<float> freqValues = { 100.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 };

		for(auto f: freqValues)
		{
			auto normPos = 1.0f - (f - 20.0f) / 19980.0f;

			normPos = std::pow(normPos, 1.0f / 0.125f);
			auto yOffset = normPos * oscArea.getHeight();

			g.setColour(Colours::white.withAlpha(0.05f));
			g.drawHorizontalLine(oscArea.getY() + yOffset, oscArea.getX(), oscArea.getRight());

			g.setColour(Colours::white.withAlpha(0.2f));
			
			if(f < 1000.0f)
				g.drawText(String(roundToInt(f)) + " Hz", yAxis.getX(), oscArea.getY() + yOffset - 10, yAxis.getWidth(), 20, Justification::right);
			else
				g.drawText(String(roundToInt(f/1000)) + " kHz", yAxis.getX(), oscArea.getY() + yOffset - 10, yAxis.getWidth(), 20, Justification::right);
		}


		for(int i = 1; i < 8; i++)
		{
			g.setColour(Colours::white.withAlpha(0.05f));
			auto xOffset = ((float)i / 8.0f) * oscArea.getWidth();
			g.drawVerticalLine(oscArea.getX() + xOffset, oscArea.getY(), oscArea.getBottom());

			auto label = String(4000.0f - roundToInt((float)i / 2.0f * 1000.0)) + "ms";

			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText(label, oscArea.getX() + xOffset - 30, oscArea.getY() - 20, 60, 20, Justification::centred);
		}

		break;
	}
	case Mode::CPU:
	{
		g.setFont(GLOBAL_FONT());

		StringArray xLabels = { "100%", "75%", "50%", "25%", "0%" };

		for(int i = 0; i < 5; i++)
		{
			g.setColour(Colours::white.withAlpha(0.05f));
			auto yOffset = ((float)i / 4.0f) * oscArea.getHeight();
			g.drawHorizontalLine(oscArea.getY() + yOffset, oscArea.getX(), oscArea.getRight());

			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText(xLabels[i], yAxis.getX(), oscArea.getY() + yOffset - 10, yAxis.getWidth() - 10, 20, Justification::right);
		}

		for(int i = 1; i < 8; i++)
		{
			g.setColour(Colours::white.withAlpha(0.05f));
			auto xOffset = ((float)i / 8.0f) * oscArea.getWidth();
			g.drawVerticalLine(oscArea.getX() + xOffset, oscArea.getY(), oscArea.getBottom());
			
			auto label = String(4000.0f - roundToInt((float)i / 2.0f * 1000.0)) + "ms";

			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText(label, xAxis.getX() + xOffset - 30, xAxis.getY(), 60, xAxis.getHeight(), Justification::centred);
		}

		break;
	}
	case Mode::PitchTracking:
	{
		auto copy = oscArea;

		g.setColour(Colours::white.withAlpha(0.05f));
		g.drawRect(copy, 1.0f);

		auto h = copy.getHeight() / 24.0;

		auto start = dynamic_cast<const BackendProcessor*>(getMainController())->getCurrentAnalyserNoteNumber();

		g.setFont(GLOBAL_FONT());

		if(start == -1)
		{
			g.setColour(Colours::white.withAlpha(0.4f));
			g.drawText("Play a note in order to view the root frequency plot.", oscArea, Justification::centred);
			return;
		}

		static constexpr bool blackKeys[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
		
		g.setColour(Colours::white.withAlpha(0.05f));

		

		yAxis = yAxis.expanded(0.0, h * 0.5);

		auto axisHeight = yAxis.getHeight() / 25.0f;

		yAxis.removeFromRight(10);

		auto first = true;

		if(start != -1)
		{
			for(int i = start - 12; i < start + 13; i++)
			{
				auto thisH = first ? h / 2 : h;

				g.setColour((blackKeys[i % 12] ? Colours::black : Colours::white).withAlpha(0.02f));
				g.fillRect(copy.removeFromBottom(thisH-1));
				copy.removeFromBottom(1);

				auto ta = yAxis.removeFromBottom(axisHeight);

				g.setColour(Colours::white.withAlpha(0.2f));

				if(thisH > 13.0f)
				{
					g.drawText(MidiMessage::getMidiNoteName(i, true, true, 3), ta, Justification::right);
				}
				else if (i % 12 == 0)
				{
					g.drawText(MidiMessage::getMidiNoteName(i, true, true, 3), ta.expanded(0.0f, 10.0f), Justification::right);
				}

				first = false;
			}
		}

		

		for(int i = 1; i < 8; i++)
		{
			g.setColour(Colours::white.withAlpha(0.05f));
			auto xOffset = ((float)i / 8.0f) * oscArea.getWidth();
			g.drawVerticalLine(oscArea.getX() + xOffset, oscArea.getY(), oscArea.getBottom());
			
			auto label = String(4000.0f - roundToInt((float)i / 2.0f * 1000.0)) + "ms";

			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText(label, oscArea.getX() + xOffset - 30, oscArea.getY() - 20, 60, 20, Justification::centred);
		}

		break;
	}
	case Mode::FFT:
	{
		std::array<int, 9> freqs = {30, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
		StringArray fn({"30Hz", "50Hz", "100Hz", "200Hz", "500Hz", "1kHz", "2kHz", "5kHz", "10kHz"});

		auto fftArea = oscArea;


		for(int i = 0; i < fn.size(); i++)
		{
			auto x = fftArea.getX() + FFTHelpers::getPixelValueForLogXAxis(freqs[i], fftArea.getWidth());
			g.setColour(Colours::white.withAlpha(hmath::fmod(hmath::log10((double)freqs[i]), 1.0) == 0.0 ? 0.1f : 0.05f));
			g.drawVerticalLine(x, fftArea.getY(), fftArea.getBottom());
			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText(fn[i], x - 25, xAxis.getY(), 50, xAxis.getHeight(), Justification::centred);
		}

		float gain = 1.0f;

		while(gain > Decibels::decibelsToGain(-80.0f))
		{
			auto yPos = getYValue(gain);

			yPos *= fftArea.getHeight();
			yPos += fftArea.getY() + 0.065f * fftArea.getHeight();
			
			g.setColour(Colours::white.withAlpha(0.05f));
			g.drawHorizontalLine(yPos, fftArea.getX(), fftArea.getRight());
			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawText(String(roundToInt(Decibels::gainToDecibels(gain))) + "dB", yAxis.getX(), yPos - 10, yAxis.getWidth() - 10, 20, Justification::right);

			gain *= 0.25f;
		}


		break;
	}
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::refresh(bool isPost, const AudioSampleBuffer& b)
{
	auto i = infos[(int)isPost][(int)currentMode];

	if(i->freeze)
		return;

	i->calculate(b, getContentArea());
}

void MainTopBar::ClickablePeakMeter::PopupComponent::buttonClicked(Button* b)
{
	auto modeIndex = modes.indexOf(dynamic_cast<TextButton*>(b));

	if(modeIndex != -1)
	{
		if(b->getToggleState())
			setMode((Mode)modeIndex);
		return;
	}

	auto toolbarIndex = toolbar.indexOf(dynamic_cast<HiseShapeButton*>(b));

	if(toolbarIndex != -1)
	{
		performCommand((ToolbarCommand)toolbarIndex, b->getToggleState());
		return;
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::rebuildPeakMeters()
{
	auto pm = new VuMeter(0.0, 0.0, VuMeter::Type::StereoVertical);
	addAndMakeVisible(pm);
	peakMeter = pm;
	pm->setColour(VuMeter::ColourId::backgroundColour, JUCE_LIVE_CONSTANT(Colour(0xff222222)));
	pm->setColour(VuMeter::ColourId::ledColour, JUCE_LIVE_CONSTANT(Colour(0xFFDDDDDD)));
	pm->setColour(VuMeter::ColourId::outlineColour, Colours::transparentBlack);
	
}

void MainTopBar::ClickablePeakMeter::PopupComponent::run()
{
	while(!threadShouldExit())
	{
		std::pair<float, float> thisPeak;

		{
			AudioSampleBuffer* b0;
			AudioSampleBuffer* b1;

			int numSamples = 0;

			{
				ScopedLock sl(lock);
				auto analyserBuffer0 = analyserData->getAnalyserBuffer(false);
				auto analyserBuffer1 = analyserData->getAnalyserBuffer(true);
				b0 = const_cast<AudioSampleBuffer*>(&analyserBuffer0->getReadBuffer());
				b1 = const_cast<AudioSampleBuffer*>(&analyserBuffer1->getReadBuffer());
				numSamples = analyserBuffer0->getMaxLengthInSamples();
				analyserBuffer0->read(*b0);
				analyserBuffer1->read(*b1);
			}
			
			if(threadShouldExit())
				return;

			
			auto numChannelsInBuffer = b1->getNumChannels();

			auto& maxPeak = maxPeaks;
			auto i = currentChannelIndex;
			maxPeak = { b1->getMagnitude(jmin(numChannelsInBuffer-1, i * 2), 0, numSamples), b1->getMagnitude(jmin(numChannelsInBuffer-1, i * 2+1), 0, numSamples) };
			auto peakDuration = jmin(numSamples, roundToInt(getMainController()->getMainSynthChain()->getSampleRate() * 0.05));

			auto& currentPeak = currentPeaks;
			thisPeak = { b1->getMagnitude(jmin(numChannelsInBuffer-1, i * 2), numSamples - peakDuration, peakDuration), b1->getMagnitude(jmin(numChannelsInBuffer-1, i * 2+1), numSamples - peakDuration, peakDuration) };

			FloatSanitizers::sanitizeFloatNumber(maxPeak.first);
			FloatSanitizers::sanitizeFloatNumber(maxPeak.second);
			FloatSanitizers::sanitizeFloatNumber(thisPeak.first);
			FloatSanitizers::sanitizeFloatNumber(thisPeak.second);

			if(thisPeak.first > currentPeak.first)
				currentPeak.first = thisPeak.first;
			else
				currentPeak.first *= 0.9f;

			if(thisPeak.second > currentPeak.second)
				currentPeak.second = thisPeak.second;
			else
				currentPeak.second *= 0.93f;

			
				
			if(threadShouldExit())
				return;

			refresh(false, *b0);

			if(threadShouldExit())
				return;

			refresh(true, *b1);
		}
		

		{
			MessageManagerLock mm(this);

			if(mm.lockWasGained())
			{
				peakMeter->setPeak(thisPeak.first, thisPeak.second);
				repaint();
			}
		}

		wait(15);
	}
}

String MainTopBar::ClickablePeakMeter::PopupComponent::getDecibelText(std::pair<float, float> gainFactor)
{
	auto max = jmax(gainFactor.first, gainFactor.second);
	auto db = Decibels::gainToDecibels(max);

	if(db <= -99.5)
		return "-inf";

	return String(db, 1);
}

String MainTopBar::ClickablePeakMeter::PopupComponent::getHoverText() const
{
	String t;

	t << getHoverText(true, getHoverValue(true, currentHoverValue.getX())) << " | ";
	t << getHoverText(false, getHoverValue(false, currentHoverValue.getY()));

	return t;
}

float MainTopBar::ClickablePeakMeter::PopupComponent::getHoverValue(bool isX, float normPos) const
{
	auto ca = getContentArea();

	switch(currentMode)
	{
	case Mode::Spectrogram:
		if(isX)
			return normPos * 4.0;
		else
			return dynamic_cast<const Spec2DInfo*>(infos[0][(int)Mode::Spectrogram])->getYPosition(normPos);
	case Mode::Oscilloscope:
		if(isX)
			return normPos * (analyserData->getAnalyserBuffer(false)->getMaxLengthInSamples() / getMainController()->getMainSynthChain()->getSampleRate());
		else
			return Decibels::gainToDecibels(2.0f * hmath::abs(normPos - 0.5f));
	case Mode::FFT:
		if(isX)
			return FFTHelpers::getFreqForLogX(normPos * (ca.getWidth()), ca.getWidth());
		else
			return getDecibelForY(normPos + 0.065f);
	case Mode::Envelope:
		if(isX)
			return normPos * 4.0;
		else
			return Decibels::gainToDecibels(1.0f - normPos);
	case Mode::PitchTracking:
		if(isX)
			return normPos * 4.0;
		else
		{
			auto nn = (float)dynamic_cast<const BackendProcessor*>(getMainController())->getCurrentAnalyserNoteNumber();

			if(nn == -1.0f)
				return 0.0f;
			else
				return NormalisableRange<float>(nn -12.0f, nn + 12.0f).convertFrom0to1(1.0f - normPos);
		}
			
	case Mode::CPU:
		if(isX)
			return normPos * 4.0;
		else
			return (1.0 - normPos) * 100.0;
	default:
		return normPos;
	}
}

String MainTopBar::ClickablePeakMeter::PopupComponent::getHoverText(bool isX, float hoverValue) const
{
	switch(currentMode)
	{
	case Mode::Spectrogram:
		if(isX)
			return getTimeDomainValue(hoverValue);
		else
			return String(hoverValue * 19980.0 + 20.0, 1) + "Hz: ";
	case Mode::Oscilloscope:
		if(isX)
			return getTimeDomainValue(hoverValue);
		else
			return String(hoverValue, 1) + "dB";
	case Mode::FFT:
		if(isX)
			return String(hoverValue, 1) + "Hz: ";
		else
			return String(hoverValue, 1) + "dB";
	case Mode::Envelope:
		if(isX)
			return getTimeDomainValue(hoverValue);
		else
			return String(hoverValue, 1) + "dB";
	case Mode::CPU:
		if(isX)
			return getTimeDomainValue(hoverValue);
		else
			return String(roundToInt(hoverValue)) + "%";
	case Mode::PitchTracking:
		if(isX)
			return getTimeDomainValue(hoverValue);
		else
		{
			String s;

			if(hoverValue < 21.0f)
			{
				s << String(hoverValue, 2) << "st";
			}
			else
			{
				s << MidiMessage::getMidiNoteName(roundToInt(hoverValue), true, true, 3);
			}

			return s;
		}
			
	default:
		return String(isX ? "X:" : "Y:") + String(hoverValue, 2);
	}
}


void MainTopBar::ClickablePeakMeter::PopupComponent::paint(Graphics& g)
{
	auto oscArea = getContentArea();

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF242424)));

	g.fillAll();

	auto bb = getLocalBounds().removeFromBottom(75);
	g.setColour(Colours::white.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.02f)));
	g.fillRect(bb);
	GlobalHiseLookAndFeel::drawFake3D(g, bb);

	g.setFont(GLOBAL_BOLD_FONT());

	g.setColour(Colours::white.withAlpha(0.7f));

	g.drawText(getDecibelText(maxPeaks), peakTextArea, Justification::centredTop);
	g.drawText(getDecibelText(currentPeaks), peakTextArea, Justification::centredBottom);

	if(!ok.wasOk())
	{
		g.setFont(GLOBAL_FONT());
		g.drawText(ok.getErrorMessage(), contentArea.toFloat(), Justification::centred);
	}
	else
	{
		paintBackground(g);

		auto preAlpha = jlimit(0.0f, 1.0f, (float)alphaSlider.getValue() * 2.0f);
		auto postAlpha = jlimit(0.0f, 1.0f, (1.0f - (float)alphaSlider.getValue()) * 2.0f);

		float alpha[2] = { std::pow(preAlpha, 1.5f), std::pow(postAlpha, 1.5f) };

		auto firstIndex = preAlpha > postAlpha ? 1 : 0;
		auto secondIndex = 1 - firstIndex;

		if(alpha[firstIndex] > 0.01f)
			infos[firstIndex][(int)currentMode]->draw(g, alpha[firstIndex]);

		if(alpha[secondIndex] > 0.01f)
			infos[secondIndex][(int)currentMode]->draw(g, alpha[secondIndex]);
		
	}

	

	g.setFont(GLOBAL_FONT());

	auto l = labelAreas[0].toFloat();
	auto r = labelAreas[1].toFloat();

	auto preAlpha = jmin(1.0f, (float)alphaSlider.getValue() * 2.0f);
	auto postAlpha = jmin(1.0f, 2.0f - 2.0f * (float)alphaSlider.getValue());
		
	g.setColour(infos[0][0]->c.withAlpha(preAlpha));
	g.fillEllipse(l.removeFromLeft(l.getHeight()).reduced(13));
		g.setColour(Colours::white.withAlpha(0.7f));
	g.drawText("Pre", l, Justification::left);

	g.setColour(infos[1][0]->c.withAlpha(postAlpha));
	g.fillEllipse(r.removeFromRight(r.getHeight()).reduced(13));
		g.setColour(Colours::white.withAlpha(0.7f));
	g.drawText("Post", r, Justification::right);

	g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0f));
	g.setColour(Colours::white.withAlpha(0.8f));

	auto ap = analyserData->currentlyAnalysedProcessor.first;
	String t;
	t << (ap != nullptr ? ap->getId() : "Master Chain");

	int pairIndex = (currentChannelIndex * 2) + 1;
	t << " " << String(pairIndex) << "/" << String(pairIndex+1);
	g.drawText(t, titleArea, Justification::left);

	if(!currentHoverValue.isOrigin())
	{
		g.setColour(Colours::white.withAlpha(0.6f));
		g.setFont(GLOBAL_FONT());
		g.drawText(getHoverText(), contentArea.toFloat().reduced(20.0f, 4.0f), Justification::topRight);
	}

	if(currentDragInfo.active)
	{
		auto f = GLOBAL_FONT();

		String t;

		auto sv = getHoverValue(currentDragInfo.isXDrag, currentDragInfo.normalisedDistance.getStart());
		auto ev = getHoverValue(currentDragInfo.isXDrag, currentDragInfo.normalisedDistance.getEnd());

		if(sv > ev)
			std::swap(ev, sv);

		t << getHoverText(currentDragInfo.isXDrag, ev - sv);

		auto mp = currentDragInfo.line.getPointAlongLineProportionally(0.5f);
		auto ta = Rectangle<float>(mp, mp).withSizeKeepingCentre(f.getStringWidthFloat(t) + 24.0f, 24.0f);

		g.setColour(Colours::white.withAlpha(0.3f));

		if(currentDragInfo.isXDrag)
		{
			g.drawVerticalLine(currentDragInfo.line.getStartX(), oscArea.getY(), oscArea.getBottom());
			g.drawVerticalLine(currentDragInfo.line.getEndX(), oscArea.getY(), oscArea.getBottom());
		}
		else
		{
			g.drawHorizontalLine(currentDragInfo.line.getStartY(), oscArea.getX(), oscArea.getRight());
			g.drawHorizontalLine(currentDragInfo.line.getEndY(), oscArea.getX(), oscArea.getRight());
		}
			

		if(currentDragInfo.isXDrag)
			ta.translate(0.0f, -14.0f);
		else
			ta.translate(-(ta.getWidth() * 0.5f + 10.0f), 0.0f);
			

		g.setColour(Colour(0xDD181818));
		g.fillRoundedRectangle(ta, ta.getHeight() * 0.5f);

		Path p, p2;

		p.startNewSubPath(currentDragInfo.line.getStart());
		p.lineTo(currentDragInfo.line.getEnd());

		float arrowSize = 3.0f + jlimit(0.0f, 6.0f, currentDragInfo.line.getLength() / 20.0f);

		PathStrokeType(1.0f).createStrokeWithArrowheads(p2, p, arrowSize, arrowSize, arrowSize, arrowSize);
		
		g.setColour(Colours::white.withAlpha(0.95f));
		g.drawText(t, ta, Justification::centred);
		g.fillPath(p2);
	}
	
}

void MainTopBar::ClickablePeakMeter::PopupComponent::resized()
{
	auto b = getLocalBounds().reduced(20);

	auto top = b.removeFromTop(24);

	

	toolbar[ToolbarCommand::ShowChannels]->setBounds(top.removeFromLeft(42).reduced(2));

	titleArea = top.toFloat();

	
	
	

	

	b.removeFromTop(10);

	auto bottomBar = b.removeFromBottom(40);

	auto peakArea = b.removeFromLeft(42);

	auto ta = peakArea.removeFromLeft(40);
		
	peakTextArea = ta.removeFromTop(40).reduced(0, 5).toFloat();
	ta.removeFromBottom(30);
	peakMeter->setBounds(ta);
	peakArea.removeFromLeft(2);

	b.removeFromLeft(10);

	auto modeArea = b.removeFromTop(20);

	toolbar[ToolbarCommand::EditProperties]->setBounds(modeArea.removeFromRight(modeArea.getHeight()));
	

	


	toolbar[ToolbarCommand::Freeze]->setBounds(bottomBar.removeFromLeft(bottomBar.getHeight()).reduced(4));

	labelAreas[0] = bottomBar.removeFromLeft(75);
	labelAreas[1] = bottomBar.removeFromRight(75);

	alphaSlider.setBounds(bottomBar);
	
	auto modeWidth = modeArea.getWidth() / modes.size();

	for(auto mb: modes)
		mb->setBounds(modeArea.removeFromLeft(modeWidth));

	b.removeFromTop(10);

	contentArea = b;

	if(gonioMeter[0] != nullptr)
	{
		auto gb = contentArea.reduced(30);
		gb.removeFromBottom(100);
		gb.removeFromBottom(30);
		gonioMeter[0]->setBounds(gb);
		gonioMeter[1]->setBounds(gb);
	}

	if(currentEditor != nullptr)
	{
		currentEditor->setBounds(contentArea.withSizeKeepingCentre(currentEditor->getWidth(), currentEditor->getHeight()));
	}

	resizer.setBounds(getLocalBounds().removeFromRight(15).removeFromBottom(15));
}

Rectangle<int> MainTopBar::ClickablePeakMeter::PopupComponent::getAxisArea(bool getX) const
{
	auto c = getContentArea();

	if(getX)
		return c.translated(0, -XAxis).withHeight(XAxis);
	else
		return c.translated(-YAxis, 0).withWidth(YAxis);
}

MainTopBar::ClickablePeakMeter::PopupComponent::Spec2DInfo::Spec2DInfo(BackendProcessor* bp, bool isPost):
  InfoBase(bp, isPost)
{
    rbo = new ModeObject(bp, Mode::Spectrogram);

	parameters = new Spectrum2D::Parameters();
	parameters->order = 12;
	parameters->gammaPercent = 30;
	parameters->Spectrum2DSize = std::pow(2, parameters->order);
	parameters->currentWindowType = FFTHelpers::WindowType::BlackmanHarris;
	parameters->lut->setColourScheme(isPost ? Spectrum2D::LookupTable::ColourScheme::hiseColours : Spectrum2D::LookupTable::ColourScheme::preColours);
	parameters->oversamplingFactor = 2;
}

void MainTopBar::ClickablePeakMeter::PopupComponent::Spec2DInfo::draw(Graphics& g, float baseAlpha)
{
	if(img.isValid())
	{
		g.setColour(Colours::white.withAlpha(baseAlpha));
		Spectrum2D::draw(g, img, contentArea, Graphics::mediumResamplingQuality);
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::Spec2DInfo::calculate(const AudioSampleBuffer& b,
	Rectangle<int> ca)
{
#if JUCE_DEBUG
	if(counter++ % 3 != 0)
		return;
#endif

	contentArea = ca;
	Spectrum2D spectrum(this, b);
	spectrum.parameters = parameters;

	auto l = (int)rbo->getProperty("FFTSize");
	auto logl = std::log2(l);

	parameters->order = logl;
	parameters->gammaPercent = roundToInt((double)rbo->getProperty("Gamma") * 100.0);
	parameters->Spectrum2DSize = l;
	parameters->currentWindowType = (FFTHelpers::WindowType)FFTHelpers::getAvailableWindowTypeNames().indexOf(rbo->getProperty("WindowType"));
	parameters->oversamplingFactor = rbo->getProperty("Oversampling");

	spectrum.useAlphaChannel = true;
	auto sb = spectrum.createSpectrumBuffer();
	
	auto newImage = spectrum.createSpectrumImage(sb);

	MessageManagerLock mm(Thread::getCurrentThread());

	if(mm.lockWasGained())
		std::swap(newImage, img);
}

Spectrum2D::Parameters::Ptr MainTopBar::ClickablePeakMeter::PopupComponent::Spec2DInfo::getParameters() const
{
	return parameters;
}

void MainTopBar::ClickablePeakMeter::PopupComponent::FFTInfo::calculate(const AudioSampleBuffer& b,
                                                                        Rectangle<int> contentArea)
{
	

	rbo->transformReadBuffer(*const_cast<AudioSampleBuffer*>(&b));
	auto np = rbo->createPath({}, {}, contentArea.withPosition(0.0f, 0.0f).toFloat(), 0.0);
	np.applyTransform(AffineTransform::translation(contentArea.getX(), contentArea.getY()));

	MessageManagerLock mm(Thread::getCurrentThread());

	if(mm.lockWasGained())
	{
		for(auto i = fftPaths.size()-1; i >= 1 ; i--)
			std::swap(fftPaths[i], fftPaths[i-1]);

		fftPaths[0] = np;
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::FFTInfo::draw(Graphics& g, float baseAlpha)
{
	float alpha = baseAlpha;
	auto& p =  fftPaths[0];
	g.setColour(c.interpolatedWith(Colours::white, alpha * 0.8f).withAlpha(alpha * alpha));
	g.strokePath(p, PathStrokeType(0.5f + (JUCE_LIVE_CONSTANT_OFF(1.0f) / alpha)));
	g.setColour(c.withAlpha(0.1f * baseAlpha));
	g.fillPath(p);
}

void MainTopBar::ClickablePeakMeter::PopupComponent::OscInfo::draw(Graphics& g, float baseAlpha)
{
	float alpha = baseAlpha;
	
	for(auto& p: cyclePaths)
	{
		g.setColour(c.interpolatedWith(Colours::white, alpha * 0.7f).withAlpha(alpha));
		g.strokePath(p, PathStrokeType(1.0f + (JUCE_LIVE_CONSTANT_OFF(0.5f) / alpha)));
		alpha *= jmax(0.1f, JUCE_LIVE_CONSTANT_OFF(0.4f));

		if(freeze)
			break;
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::OscInfo::calculate(const AudioSampleBuffer& b,
                                                                        Rectangle<int> contentArea)
{
	auto cb = contentArea.toFloat();

	

	Path cyclePath;

	auto analyserBuffer = info->getAnalyserBuffer(true);

	cyclePath.clear();
	auto w =  cb.getWidth();
	auto numSamples = analyserBuffer->getMaxLengthInSamples();
	auto samplesPerPixel = 1;//jmax(1, roundToInt((double)(numSamples-1) / (double)(w)));
	auto pixelPerSample = jmax(0.5f, (float)(w) / (float)(numSamples-1));

	cyclePath.startNewSubPath(cb.getX(), cb.getCentreY());

	auto maxGain = b.getMagnitude(0, 0, numSamples);

	AudioSampleBuffer zero(1, numSamples);

	cyclePath.preallocateSpace(w * 3);

	if(!isPost || *zIndex == -1)
	{
		if(maxGain == 0.0f)
			*zIndex = -1;
		else
		{
			auto zeroCrossing = -1;

			for(int i = 0; i < numSamples - 1; i++)
			{
				auto thisSample = b.getSample(0, i);
				auto nextSample = b.getSample(0, i+1);

				if(hmath::sign(thisSample) != hmath::sign(nextSample) && nextSample > 0.0f)
				{
					zeroCrossing = i;
					break;
				}
			}

			*zIndex = zeroCrossing;
		}
	}

	if(*zIndex == -1)
	{
		FloatVectorOperations::copy(zero.getWritePointer(0), b.getReadPointer(0), zero.getNumSamples());
	}
	else
	{
		auto numAfter = zero.getNumSamples() - *zIndex;
		auto numBefore = *zIndex;

		if(numAfter > 0)
			FloatVectorOperations::copy(zero.getWritePointer(0), b.getReadPointer(0, numBefore), numAfter);

		if(numBefore > 0)
			FloatVectorOperations::copy(zero.getWritePointer(0, numAfter), b.getReadPointer(0), numBefore);
	}

	auto gf = 1.0f;

	if(maxGain > 1.0f)
		gf = 1.0f / maxGain;

	float x = (float)cb.getX();

	for(int i = 0; i < numSamples; i += samplesPerPixel)
	{
		auto range = zero.findMinMax(0, i, jmin(samplesPerPixel, numSamples - i));

		float gainValue = 0.0f;

		if(hmath::abs(range.getStart()) > hmath::abs(range.getEnd()))
			gainValue = range.getStart();
		else
			gainValue = range.getEnd();

		gainValue *= gf * -0.5f;

		auto y = cb.getCentreY() + gainValue * cb.getHeight();
		cyclePath.lineTo(x, y);
		x = jmin(cb.getRight(), x + pixelPerSample);
	}

	cyclePath.lineTo(cb.getRight(), cb.getCentreY());

	MessageManagerLock mm(Thread::getCurrentThread());

	if(mm.lockWasGained())
	{
		for(auto i = cyclePaths.size()-1; i >= 1; i--)
			std::swap(cyclePaths[i], cyclePaths[i-1]);

		cyclePaths[0] = cyclePath;
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::PitchTrackInfo::draw(Graphics& g, float baseAlpha)
{
	if(!isPost)
		return;

	baseAlpha = parent->alphaSlider.getValue();

	auto preAlpha = jlimit(0.0f, 1.0f, (float)baseAlpha * 2.0f);
	auto postAlpha = jlimit(0.0f, 1.0f, (1.0f - (float)baseAlpha) * 2.0f);

	preAlpha = std::pow(preAlpha, 1.5f);
	postAlpha = std::pow(postAlpha, 1.5f);

	g.setColour(Colours::pink.interpolatedWith(Colours::white, preAlpha * 0.7f).withAlpha(preAlpha));
	g.strokePath(prePath, PathStrokeType(1.0f + (JUCE_LIVE_CONSTANT_OFF(0.5f) / preAlpha)));

	g.setColour(c.interpolatedWith(Colours::white, postAlpha * 0.7f).withAlpha(postAlpha));
	g.strokePath(pitchPath, PathStrokeType(1.0f + (JUCE_LIVE_CONSTANT_OFF(0.5f) / postAlpha)));
}

void MainTopBar::ClickablePeakMeter::PopupComponent::PitchTrackInfo::calculate(const AudioSampleBuffer& b,
	Rectangle<int> contentArea)
{
	

	if(!isPost)
		return;

	PitchDetection pitchDetection;

	auto sr = bp->getMainSynthChain()->getSampleRate();

	auto maxStepSize = pitchDetection.getNumSamplesNeeded(sr, 50.0);
	auto minStepSize = nextPowerOfTwo(pitchDetection.getNumSamplesNeeded(sr, 8000.0));

	auto stepSize = maxStepSize;

	float normalisedPos = 0.0f;

	Array<Point<float>> dataPoints;

	Array<Point<float>> prePoints;

	auto xOffset = (float)contentArea.getX();
	auto yOffset = (float)contentArea.getY();
	auto xLength = (float)contentArea.getWidth();
	auto yLength = (float)contentArea.getHeight();

	dataPoints.ensureStorageAllocated(b.getNumSamples() / minStepSize);

	bool wasZero = true;

	auto nn = bp->getCurrentAnalyserNoteNumber();

	float prePitch = nn != -1 ? (float)MidiMessage::getMidiNoteInHertz(nn) : 0.0f;

	Range<float> allowedRange = { prePitch * 0.5f, prePitch * 2.0f };

	float lastPitch = 0.0f;

	NormalisableRange<float> nr((float)nn - 12.0f, (float)nn + 12.0f);

	for(int i = 0; i < b.getNumSamples(); i += stepSize)
	{
		auto numThisTime = jmin(b.getNumSamples() - i, stepSize);
		normalisedPos = (float)i / (float)b.getNumSamples();

		FloatSanitizers::sanitizeFloatNumber(normalisedPos);

		if(FloatSanitizers::isNotSilence(b.getMagnitude(0, i, numThisTime)) || !isPost)
		{
			if(wasZero)
				stepSize = maxStepSize;

			numThisTime = jmin(b.getNumSamples() - i, stepSize);

			auto pitch = (float)pitchDetection.detectPitch(b, i, numThisTime, sr);

			if(pitch != 0.0f && allowedRange.contains(pitch))
			{
				dataPoints.add({ normalisedPos, pitch });
				lastPitch = pitch;
			}
			else
			{
				dataPoints.add({ normalisedPos, lastPitch });
			}
			
			prePoints.add({ normalisedPos, prePitch });

			if(pitch != 0.0)
			{
				stepSize = jmin(maxStepSize, pitchDetection.getNumSamplesNeeded(sr, pitch));
				wasZero = false;
			}
		}
		else
		{
			dataPoints.add({ normalisedPos, 0.0f });
			prePoints.add({ normalisedPos, 0.0f });
			stepSize = minStepSize;
			wasZero = true;
		}
	}

	bool startNew = true;

	Path path0, path1;

	auto tolerance = JUCE_LIVE_CONSTANT(0.02);

	for(int i = 1; i < dataPoints.size() - 1; i++)
	{
		auto thisY = dataPoints[i].getY();

		if(thisY != 0.0f)
		{
			auto prev = dataPoints[i-1].getY();
			auto next = dataPoints[i+1].getY();

			if(prev != 0.0f && next != 0.0f)
			{
				auto distPrev = hmath::abs(prev - thisY);
				auto distNext = hmath::abs(next - thisY);
				auto distBetween = hmath::abs(next - prev);

				if(distPrev > thisY * tolerance &&
				   distNext > thisY * tolerance &&
				   distBetween < thisY * tolerance)
				{
					dataPoints.getReference(i).setY(prev);
				}
				else
				{
					dataPoints.getReference(i).setY(thisY);
				}
			}
		}
	}

	for(const auto& p: prePoints)
	{
		if(p.getY() == 0.0)
			startNew = true;
		else
		{
			auto x = p.getX() * xLength + xOffset;
			auto y = (1.0f - nr.convertTo0to1((float)nn)) * yLength + yOffset;

			FloatSanitizers::sanitizeFloatNumber(x);
			FloatSanitizers::sanitizeFloatNumber(y);

			if(startNew)
				path0.startNewSubPath(x, y);
			else
				path0.lineTo(x, y);

			startNew = false;
		}
	}

	startNew = true;

	for(const auto& p: dataPoints)
	{
		if(p.getY() == 0.0)
		{
			startNew = true;
		}
		else
		{
			auto freq = p.getY();
			auto noteNumber = hmath::log(freq / 440.0f) * 12.0f / hmath::log(2.0f) + 69.0f;

			if(!nr.getRange().expanded(1.0f).contains(noteNumber))
			{
				if(noteNumber < nn)
					noteNumber = (float)nn + hmath::fmod(noteNumber - (float)nn, 12.0f);
				else
					noteNumber = hmath::fmod(noteNumber - (float)nn, 12.0f) + (float)nn;
			}

			noteNumber = jlimit<float>((float)nn - 12.0f, (float)nn + 12.0f, noteNumber);
			
			auto y = (1.0f - nr.convertTo0to1(noteNumber)) * yLength + yOffset;
			auto x = p.getX() * xLength + xOffset;

			if(startNew)
				path1.startNewSubPath(x, y);
			else
				path1.lineTo(x, y);

			startNew = false;
		}
	}

	MessageManagerLock mm(Thread::getCurrentThread());
	prePath = path0;
	pitchPath = path1;
}

void MainTopBar::ClickablePeakMeter::PopupComponent::CpuInfo::draw(Graphics& g, float baseAlpha)
{
	float alpha = baseAlpha;

	bool first = true;

	for(auto& p: cpuPaths)
	{
		g.setColour(c.interpolatedWith(Colours::white, alpha * 0.7f).withAlpha(alpha));
		g.strokePath(p, PathStrokeType(1.0f + (JUCE_LIVE_CONSTANT_OFF(0.5f) / alpha)));
		g.strokePath(p, PathStrokeType(1.0f + (JUCE_LIVE_CONSTANT_OFF(0.5f) / alpha)));

		if(first)
		{
			g.setColour(c.withAlpha(0.1f * baseAlpha));
			g.fillPath(p);
			g.fillPath(p);

			first = false;
			break;
		}

		alpha *= jmax(0.1f, JUCE_LIVE_CONSTANT_OFF(0.4f));

		if(freeze)
			break;
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::CpuInfo::calculate(const AudioSampleBuffer& b,
	Rectangle<int> contentArea)
{
	area = contentArea;

	if(area.isEmpty())
		return;
	
	auto numPointsToCalculate = roundToInt(area.getWidth() / 2);
	auto numSamplesPerPixel = roundToInt(b.getNumSamples() / numPointsToCalculate);

	Path path;

	path.clear();
	path.startNewSubPath(area.getX(), area.getBottom());

	path.preallocateSpace(numPointsToCalculate * 3);

	int x = 0;

	for(int i = 0; i < b.getNumSamples(); i += numSamplesPerPixel)
	{
		auto numToDo = jmin(b.getNumSamples() - i, numSamplesPerPixel);

		auto max = b.getMagnitude(0, i, numToDo);

		max = jlimit(0.0f, 1.0f, max);
		auto xPos = area.getX() + (float)x;
		auto yPos = area.getY() + (1.0f - max) * area.getHeight();
		path.lineTo(xPos, yPos);
		x += 2;
	}

	path.lineTo(area.getRight(), area.getBottom());
	path.closeSubPath();
	
	MessageManagerLock mm(Thread::getCurrentThread());

	if(mm.lockWasGained())
	{
		for(auto i = cpuPaths.size()-1; i >= 1; i--)
			std::swap(cpuPaths[i], cpuPaths[i-1]);

		std::swap(cpuPaths[0], path);
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::EnvInfo::draw(Graphics& g, float baseAlpha)
{
	float alpha = baseAlpha;

	bool first = true;

	for(auto& p: envPaths)
	{
		g.setColour(c.interpolatedWith(Colours::white, alpha * 0.7f).withAlpha(alpha));
		g.strokePath(p[0], PathStrokeType(1.0f + (JUCE_LIVE_CONSTANT_OFF(0.5f) / alpha)));
		g.strokePath(p[1], PathStrokeType(1.0f + (JUCE_LIVE_CONSTANT_OFF(0.5f) / alpha)));
		

		if(first)
		{
			g.setColour(c.withAlpha(0.1f * baseAlpha));
			g.fillPath(p[0]);
			g.fillPath(p[1]);

			first = false;
			break;
		}

		alpha *= jmax(0.1f, JUCE_LIVE_CONSTANT_OFF(0.4f));

		if(freeze)
			break;
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::EnvInfo::calculate(const AudioSampleBuffer& b,
	Rectangle<int> contentArea)
{
	area = contentArea;
	if(area.isEmpty())
		return;
	
	span<float, 2> frame;

	dynamics::envelope_follower follower[2];

	follower[0].setAttack(0.0);
	follower[0].setRelease(12.0);
	follower[0].setProcessSignal(true);
	follower[1].setAttack(0.0);
	follower[1].setRelease(12.0);
	follower[1].setProcessSignal(true);

	auto numPointsToCalculate = roundToInt(area.getWidth() / 2);
	auto numSamplesPerPixel = roundToInt(b.getNumSamples() / numPointsToCalculate);

	PrepareSpecs ps;
	ps.sampleRate = bp->getMainSynthChain()->getSampleRate();
	ps.numChannels = 1;
	ps.blockSize = 1;

	follower[0].prepare(ps);
	follower[1].prepare(ps);

	follower[0].reset();
	follower[1].reset();

	auto x = area.toFloat();

	Rectangle<float> areas[2];
	areas[0] = x.removeFromTop(x.getHeight() * 0.5f);
	areas[1] = x;

	areas[0].removeFromBottom(10);
	areas[1].removeFromTop(10);

	std::array<Path, 2> np;

	for(int c = 0; c < b.getNumChannels(); c++)
	{
		auto& path = np[c];
		auto& area = areas[c];
		path.clear();
		path.startNewSubPath(area.getX(), area.getBottom());

		auto ptr = b.getReadPointer(c);
		path.preallocateSpace(numPointsToCalculate * 3);

		int x = 0;

		for(int i = 0; i < b.getNumSamples(); i += numSamplesPerPixel)
		{
			auto numToDo = jmin(b.getNumSamples() - i, numSamplesPerPixel);

			float max = 0.0f;
			
			for(int s = 0; s < numToDo; s++)
			{
				span<float, 1> thisFrame = { ptr[i + s]};
				follower[c].processFrame(thisFrame);
				max = jmax(max, thisFrame[0]);
			}

			max = jlimit(0.0f, 1.0f, max);
			

			auto xPos = area.getX() + (float)x;
			auto yPos = area.getY() + (1.0f - max) * area.getHeight();

			path.lineTo(xPos, yPos);

			x += 2;
		}

		path.lineTo(area.getRight(), area.getBottom());
		path.closeSubPath();
	}
	
	area = contentArea;

	MessageManagerLock mm(Thread::getCurrentThread());

	if(mm.lockWasGained())
	{
		for(auto i = envPaths.size()-1; i >= 1; i--)
			std::swap(envPaths[i], envPaths[i-1]);

		std::swap(envPaths[0], np);
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::StereoInfo::calculate(const AudioSampleBuffer& b, Rectangle<int> ca)
{
	area = ca;

	for(auto i = panPos.size()-1; i >= 1; i--)
	{
		std::swap(panPos[i-1], panPos[i]);
	}

	min_c = 1.0f;
	max_c = -1.0f;

	panPos[0].clear();

	auto prevCorrellation = smoothedCorrellation;

	smoothedCorrellation = 0.0f;

	auto rChannel = jmin(b.getNumChannels()-1, 1);

	for(int i = 0; i < b.getNumSamples(); i++)
	{
		calculate(b.getSample(0, i), b.getSample(rChannel, i));
	}

	for(auto i = correllations.size()-1; i >= 1; i--)
	{
		std::swap(correllations[i-1], correllations[i]);
	}

	smoothedCorrellation /= (float)b.getNumSamples();

	correllations[0] = 0.8f * prevCorrellation + 0.2f * smoothedCorrellation;
}

void MainTopBar::ClickablePeakMeter::PopupComponent::StereoInfo::draw(Graphics& g, float baseAlpha)
{
	auto c2 = c;//.interpolatedWith(Colour(0xFFDDDDDD), 0.8f);

	auto copy = area.reduced(20);

	if(copy.isEmpty())
		return;

	copy = copy.removeFromBottom(100);

	copy.removeFromTop(18);
	auto cArea = copy.removeFromTop(24).toFloat();
	auto pArea = copy.removeFromBottom(24).toFloat();

	g.setColour(Colours::white.withAlpha(0.1f));
	g.drawRect(cArea, 1);
	g.drawRect(pArea, 1);

	g.setColour(Colours::white.withAlpha(0.4f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Correllation", cArea.translated(0, -20.0f), Justification::centred);
	g.drawText("Stereo Meter", pArea.translated(0, -20.0f), Justification::centred);

	g.setFont(GLOBAL_FONT());
	g.drawText("1.0 (mono)", cArea.translated(0, -20.0f), Justification::left);
	g.drawText("-1.0 (off-phase)", cArea.translated(0, -20.0f), Justification::right);

	g.drawText("100L", pArea.translated(0, -20.0f), Justification::left);
	g.drawText("100R", pArea.translated(0, -20.0f), Justification::right);

	g.drawText("50L", pArea.translated(0, -20.0f).reduced(pArea.getWidth() * 0.25f, 0), Justification::left);
	g.drawText("50R", pArea.translated(0, -20.0f).reduced(pArea.getWidth() * 0.25f, 0), Justification::right);

	pArea = pArea.reduced(2);
	cArea = cArea.reduced(2);

	auto pw = pArea.getWidth() / 101.0f;

	auto xPos = hmath::round((0.5f * smoothedPan + 0.5f) * 100.0f) / 100.0f;

	if(isPost)
	{
		pArea = pArea.removeFromBottom(pArea.getHeight()/2 - 1);
		cArea = cArea.removeFromBottom(cArea.getHeight()/2 - 1);
	}
	else
	{
		pArea = pArea.removeFromTop(pArea.getHeight()/2 - 1);
		cArea = cArea.removeFromTop(cArea.getHeight()/2 - 1);
	}

	auto psArea = pArea.withX(pArea.getX() + (pArea.getWidth()-pw) * xPos).withWidth(pw-1);

	g.setColour(c2.withAlpha(0.2f + 0.8f * jlimit(0.0f, 1.0f, hmath::pow(smoothedGain, 0.8f))));
	g.fillRect(psArea);
			
	for(int i = 0; i < 101; i++)
	{
		float alpha = 0.0f;

		for(auto& p: panPos)
		{
			alpha += p[i] * JUCE_LIVE_CONSTANT_OFF(0.02f);
		}
				
		if(alpha != 0.0f)
		{
			alpha = jlimit(0.0f, 1.0f, alpha);
			alpha = hmath::pow(alpha, JUCE_LIVE_CONSTANT_OFF(0.8f));
			g.setColour(c2.withAlpha(jlimit(0.0f, 1.0f, alpha * 0.8f)));
			Rectangle<float> ar(pArea.getX() + (float)i/100.0f * (pArea.getWidth() - pw), pArea.getY(), pw-1.0f, pArea.getHeight());
			g.fillRect(ar);
		}
		else
		{
			g.setColour(Colours::white.withAlpha(0.05f));
			Rectangle<float> ar(pArea.getX() + (float)i/100.0f * (pArea.getWidth() - pw), pArea.getY(), pw-1.0f, pArea.getHeight());
			g.fillRect(ar);
		}
	}

	float alpha = 1.0f;

	auto cw = cArea.getWidth() / 101.0;;

	for(auto& co: correllations)
	{
		g.setColour(c2.withAlpha(alpha * 0.7f));

		auto normX = 1.0f - (co * 0.5 + 0.5);
		auto normXStep = hmath::round(normX * 101.0) / 101.0;
		auto x = cArea.getX() + (cArea.getWidth() - cw) * normXStep;
		g.fillRect(x, cArea.getY(), cw-1.0f, cArea.getHeight());

		alpha *= 0.7f;
	}
}

void MainTopBar::ClickablePeakMeter::PopupComponent::StereoInfo::calculate(float left, float right)
{
	auto normGain = hmath::max(hmath::abs(left), hmath::abs(right));

	if(normGain > smoothedGain)
		smoothedGain = normGain;
	else
		smoothedGain *= coeff;

	if(normGain > 0.0f)
	{
		auto thisPan = hmath::abs(left) * -1.0f + hmath::abs(right);
		thisPan /= normGain;
		auto thisCorellation = 1.0 - (hmath::abs(left - right) / normGain);

		auto idx = jlimit(0, 100, roundToInt(101.0f * (thisPan * 0.5f + 0.5f)));

		panPos[0][idx] += normGain;

		smoothedPan = coeff * smoothedPan + (1.0f - coeff) * thisPan;

		smoothedCorrellation += thisCorellation;

		min_c = jmin<float>(min_c, smoothedCorrellation);
		max_c = jmax<float>(max_c, smoothedCorrellation);
	}
}


String MainTopBar::ClickablePeakMeter::PopupComponent::getTimeDomainValue(double valueAsSeconds) const
{
	switch(currentTimeDomain)
	{
	case TimeDomainMode::Samples: return String(roundToInt(getMainController()->getMainSynthChain()->getSampleRate() * valueAsSeconds)) + "samples";
	case TimeDomainMode::Frequency: return String(1.0 / (valueAsSeconds), 1) + " Hz";
	case TimeDomainMode::Milliseconds: return String(roundToInt(valueAsSeconds * 1000.0)) + " ms";
	}

	jassertfalse;
	return {};
}

void MainTopBar::ClickablePeakMeter::PopupComponent::ButtonLookAndFeel::drawButtonBackground(Graphics& g,
                                                                                             Button& button, const Colour& colour, bool over, bool down)
{
	Path p;

	float r = 0.0f;

	if(!button.getToggleState())
		r += 1.0f;

	if(down)
		r += 1.0f;

	auto a = button.getLocalBounds().toFloat().reduced(r);
	auto leftRound = !button.isConnectedOnLeft();
	auto rightRound = !button.isConnectedOnRight();

	float alpha = 0.8f;
	if(over)
		alpha += 0.1f;
	if(down)
		alpha += 0.1f;
			
	g.setColour(bright.withAlpha(alpha));
	p.addRoundedRectangle(a.getX(), a.getY(), a.getWidth(), a.getHeight(), a.getHeight() * 0.5f, a.getHeight() * 0.5f, leftRound, rightRound, leftRound, rightRound);

	if(button.getToggleState())
		g.fillPath(p);
	else
		g.strokePath(p, PathStrokeType(1.0f));
}

void MainTopBar::ClickablePeakMeter::PopupComponent::ButtonLookAndFeel::drawButtonText(Graphics& g, TextButton& b,
	bool over, bool down)
{
	g.setFont(GLOBAL_BOLD_FONT());

	float alpha = 0.8f;
	if(over)
		alpha += 0.1f;
	if(down)
		alpha += 0.1f;

	g.setColour(Colour(b.getToggleState() ? dark : bright).withAlpha(alpha));
	g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
}
} // namespace hise


