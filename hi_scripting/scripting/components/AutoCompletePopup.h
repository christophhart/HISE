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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#ifndef AUTOCOMPLETEPOPUP_H_INCLUDED
#define AUTOCOMPLETEPOPUP_H_INCLUDED

namespace hise { using namespace juce;


class DspObjectDebugger : public Component,
						  public DspInstance::Listener,
						  public Timer
{
public:

	struct Header : public Component
	{
		String getText()
		{
			String nl = "\n";

			String s;
			s << "#### " << parent.obj->getDebugValue() << nl;
			
			return s;
		}

		Header(DspObjectDebugger& parent_):
			parent(parent_),
			p(getText())
		{
			p.parse();
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF333333));
			
			p.draw(g, getLocalBounds().toFloat());
		}
		
		void resized() override
		{
			p.getHeightForWidth((float)getWidth());
		}

		DspObjectDebugger& parent;

		MarkdownRenderer p;
	};


	struct ParameterSetter : public Component,
							 public Slider::Listener,
							 public Label::Listener
	{
		ParameterSetter(DspObjectDebugger& parent_, int index_):
			parent(parent_),
			index(index_)
		{
			addAndMakeVisible(s);
			addAndMakeVisible(l);
			addAndMakeVisible(minLabel);
			addAndMakeVisible(maxLabel);

			s.setLookAndFeel(&plaf);

			s.setSliderStyle(Slider::LinearBar);
			s.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
			s.addListener(this);
			s.setRange(0.0, 1.0, 0.01);
			s.setColour(Slider::ColourIds::thumbColourId, Colours::grey);

			l.setFont(GLOBAL_BOLD_FONT());
			l.setColour(Label::ColourIds::backgroundColourId, Colours::white);
			l.addListener(this);
			
			minLabel.setFont(GLOBAL_BOLD_FONT());
			minLabel.setColour(Label::ColourIds::backgroundColourId, Colours::white);
			minLabel.addListener(this);
			maxLabel.setFont(GLOBAL_BOLD_FONT());
			maxLabel.setColour(Label::ColourIds::backgroundColourId, Colours::white);
			maxLabel.addListener(this);

			l.setEditable(true);
			minLabel.setEditable(true);
			maxLabel.setEditable(true);

			update(parent.obj->getParameter(index));
		}

		void labelTextChanged(Label* label) override
		{
			float v = label->getText().getFloatValue();
			FloatSanitizers::sanitizeFloatNumber(v);

			if (label == &l)
			{
				parent.obj->setParameter(index, v);
			}
			else if (label == &minLabel)
			{
				s.setRange(v, s.getMaximum(), 0.01);
				s.repaint();
			}
			else if (label == &maxLabel)
			{
				s.setRange(s.getMinimum(), v, 0.01);
				s.repaint();
			}
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromTop(18);
			l.setBounds(b.removeFromLeft(60));
			b.removeFromLeft(10);

			maxLabel.setBounds(b.removeFromRight(40));
			b.removeFromRight(5);
			minLabel.setBounds(b.removeFromRight(40));
			b.removeFromRight(10);
			s.setBounds(b);
		}

		void sliderValueChanged(Slider* slider) override
		{
			parent.obj->setParameter(index, (float)slider->getValue());
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			
			auto b = getLocalBounds().removeFromTop(18).toFloat();

			g.drawText(parent.obj->getConstantName(index).toString(), b, Justification::centred);
		}

		static String getStringFromVar(var value)
		{
            return value.toString();
		}

		void update(var newValue)
		{
			l.setText(newValue.toString(), dontSendNotification);

			if ((double)newValue > s.getMaximum())
			{
				s.setRange(s.getMinimum(), newValue, 0.01);
			}
				
			minLabel.setText(String(s.getMinimum()), dontSendNotification);
			maxLabel.setText(String(s.getMaximum()), dontSendNotification);

			s.setValue(newValue, dontSendNotification);
		}
		
		Label minLabel;
		Label maxLabel;

		HiPropertyPanelLookAndFeel plaf;
		Slider s;
		Label l;

		DspObjectDebugger& parent;
		const int index;


	};

	struct Analyser: public Component,
					 public ComboBox::Listener,
					 public Button::Listener,
					 public Timer
	{
		enum ColourId
		{
			bgColour = 12,
			fillColour,
			lineColour,
			numColourIds
		};

		Analyser(DspObjectDebugger& parent_) :
			parent(parent_),
			showInput("Show Input"),
			showOutput("Show Output"),
			oscIn("Osc IN", oscInBuffer),
			oscOut("Osc OUT", oscOutBuffer),
			specIn("FFT IN", specInBuffer),
			specOut("FFT OUT", specOutBuffer)
		{
			setOpaque(true);

			addAndMakeVisible(oscIn);
			addAndMakeVisible(oscOut);
			addAndMakeVisible(specIn);
			addAndMakeVisible(specOut);
			addAndMakeVisible(selector);
			addAndMakeVisible(showInput);
			addAndMakeVisible(showOutput);
			
			selector.addListener(this);
			showInput.addListener(this);
			showOutput.addListener(this);

			showInput.setClickingTogglesState(true);
			showOutput.setClickingTogglesState(true);

			showInput.setToggleState(true, dontSendNotification);
			showOutput.setToggleState(true, dontSendNotification);

			selector.addItemList(
				{
					"No Analysis",
					"Oscillope",
					"FFT Analyser",
					"Osc & FFT"
				}, 1);

			selector.setSelectedItemIndex(0, dontSendNotification);

			lengthSelector.addItem("1024", 1024);
			lengthSelector.addItem("2048", 2048);
			lengthSelector.addItem("4096", 4096);
			lengthSelector.addItem("8192", 8192);
			lengthSelector.addItem("16384", 16384);
			lengthSelector.addItem("32768", 32768);

			lengthSelector.addListener(this);

			lengthSelector.setSelectedId(8192, sendNotificationSync);

		}

		void comboBoxChanged(ComboBox* cb) override
		{
			if (cb == &selector)
			{
				resized();
			}
			else
			{
				specInBuffer.setAnalyserBufferSize(cb->getSelectedId());
				specOutBuffer.setAnalyserBufferSize(cb->getSelectedId());
				oscInBuffer.setAnalyserBufferSize(cb->getSelectedId());
				oscOutBuffer.setAnalyserBufferSize(cb->getSelectedId());
			}
		}

		void buttonClicked(Button*) override
		{
			resized();
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF333333));
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Analyser", getLocalBounds().removeFromTop(18).toFloat(), Justification::centred);
		}

		void resized() override
		{
			auto b = getLocalBounds();

			b.removeFromTop(18);

			auto topRow = b.removeFromTop(24);
			auto buttonWidth = (getWidth() - 140) / 2;

			showInput.setBounds(topRow.removeFromLeft(buttonWidth));
			showOutput.setBounds(topRow.removeFromRight(buttonWidth));
			selector.setBounds(topRow);

			int index = selector.getSelectedItemIndex();

			specInVisible = (index == 2 || index == 3) && showInput.getToggleState();
			specOutVisible = (index == 2 || index == 3) && showOutput.getToggleState();
			oscInVisible = (index == 1 || index == 3) && showInput.getToggleState();
			oscOutVisible = (index == 1 || index == 3) && showOutput.getToggleState();

			specIn.setVisible(specInVisible);
			specOut.setVisible(specOutVisible);
			oscIn.setVisible(oscInVisible);
			oscOut.setVisible(oscOutVisible);

			if (oscInVisible || oscOutVisible)
			{
				auto oscBounds = (specInVisible || specOutVisible) ? b.removeFromTop(b.getHeight() / 2) : b;

				if(oscInVisible)
					oscIn.setBounds(oscOutVisible ? oscBounds.removeFromLeft(oscBounds.getWidth() / 2).reduced(5) : oscBounds.reduced(5));

				oscOut.setBounds(oscBounds.reduced(5));
			}

			if(specInVisible)
				specIn.setBounds(specOutVisible ? b.removeFromLeft(b.getWidth() / 2).reduced(5) : b.reduced(5));

			specOut.setBounds(b.reduced(5));

			if (specInVisible || specOutVisible || oscInVisible || oscOutVisible)
				startTimer(50);
			else
				stopTimer();

		}

		struct Oscilloscope: public Component,
							 public OscilloscopeBase
		{
			Oscilloscope(const String& name, const AnalyserRingBuffer& buffer):
				Component(name),
				OscilloscopeBase(buffer)
			{

			}

			Colour getColourForAnalyserBase(int colourId) override
			{
				if (colourId == Analyser::ColourId::bgColour)
					return Colour(0xFF222222);
				if (colourId == Analyser::ColourId::fillColour)
					return Colour(0xFF888888);
				if (colourId == Analyser::ColourId::lineColour)
					return Colour(0xFFAAAAAA);

				return Colours::transparentBlack;
			}

			void paint(Graphics& g) override
			{
				OscilloscopeBase::drawWaveform(g);
			}


		};

		struct Spectrograph: public Component,
							 public FFTDisplayBase
		{
			Spectrograph(const String& name, const AnalyserRingBuffer& b) :
				Component(name),
				FFTDisplayBase(b)
			{

			}

			Colour getColourForAnalyserBase(int colourId) override
			{
				if (colourId == Analyser::ColourId::bgColour)
					return Colour(0xFF222222);
				if (colourId == Analyser::ColourId::fillColour)
					return Colour(0xFF888888);
				if (colourId == Analyser::ColourId::lineColour)
					return Colour(0xFFAAAAAA);

				return Colours::transparentBlack;
			}

			double getSamplerate() const override { return 44100.0; }

			void paint(Graphics& g) override
			{
				FFTDisplayBase::drawSpectrum(g);
			}
		};

		void timerCallback() override
		{
			repaint();
		}

		DspObjectDebugger& parent;

		Oscilloscope oscIn;
		Oscilloscope oscOut;

		Spectrograph specIn;
		Spectrograph specOut;

		ComboBox selector;

		ComboBox lengthSelector;
		TextButton showInput;
		TextButton showOutput;

		HiPropertyPanelLookAndFeel plaf;

		AnalyserRingBuffer oscInBuffer;
		AnalyserRingBuffer oscOutBuffer;
		AnalyserRingBuffer specInBuffer;
		AnalyserRingBuffer specOutBuffer;

		bool specInVisible  = false;
		bool specOutVisible = false;
		bool oscInVisible   = false;
		bool oscOutVisible  = false;
	};

	DspObjectDebugger(DspInstance* instance):
		obj(instance),
		header(*this),
		analyser(*this),
		resizer(this, nullptr)
	{
		addAndMakeVisible(header);
		addAndMakeVisible(analyser);
		addAndMakeVisible(resizer);

		buildSliders();
		obj->addListener(this);

		peakMeterIn.setType(VuMeter::Type::StereoVertical);
		peakMeterOut.setType(VuMeter::Type::StereoVertical);

		addAndMakeVisible(peakMeterIn);
		addAndMakeVisible(peakMeterOut);

		peakLIn = peakLOut = peakRIn = peakLOut = 0.0f;
		startTimer(50);
	}

	~DspObjectDebugger()
	{
		obj->removeListener(this);
	}

	void timerCallback() override
	{
		peakMeterIn.setPeak(peakLIn, peakRIn);
		peakMeterOut.setPeak(peakLOut, peakROut);
		peakLIn = peakLOut = peakRIn = peakROut = 0.0f;
	}

	

	

	void parameterChanged(int parameterIndex) override
	{
		sliders[parameterIndex]->update(obj->getParameter(parameterIndex));
	}

	void blockWasProcessed(const float** data, int numChannels, int numSamples) override
	{
		if (numChannels == 1)
		{
			auto range = FloatVectorOperations::findMinAndMax(data[0], numSamples);
			auto maxValue = jmax(std::abs(range.getStart()), std::abs(range.getEnd()));

			peakLOut = jmax(peakLOut, maxValue);
			peakROut = jmax(peakROut, maxValue);
			
		}
		else
		{
			auto range_l = FloatVectorOperations::findMinAndMax(data[0], numSamples);
			auto maxValue_l = jmax(std::abs(range_l.getStart()), std::abs(range_l.getEnd()));

			auto range_r = FloatVectorOperations::findMinAndMax(data[1], numSamples);
			auto maxValue_r = jmax(std::abs(range_r.getStart()), std::abs(range_r.getEnd()));

			peakLOut = jmax(peakLOut, maxValue_l);
			peakROut = jmax(peakROut, maxValue_r);
		}

		AudioSampleBuffer b(const_cast<float**>(data), numChannels, numSamples);
		
		if (analyser.specOutVisible)
			analyser.specOutBuffer.pushSamples(b, 0, numSamples);
		if (analyser.oscOutVisible)
			analyser.oscOutBuffer.pushSamples(b, 0, numSamples);
	}

	void preBlockProcessed(const float** data, int numChannels, int numSamples) override
	{
		if (numChannels == 1)
		{
			auto range = FloatVectorOperations::findMinAndMax(data[0], numSamples);
			auto maxValue = jmax(std::abs(range.getStart()), std::abs(range.getEnd()));

			peakMeterIn.setPeak(maxValue, maxValue);

			peakLIn = jmax(peakLIn, maxValue);
			peakRIn = jmax(peakRIn, maxValue);
		}
		else
		{
			auto range_l = FloatVectorOperations::findMinAndMax(data[0], numSamples);
			auto maxValue_l = jmax(std::abs(range_l.getStart()), std::abs(range_l.getEnd()));

			auto range_r = FloatVectorOperations::findMinAndMax(data[1], numSamples);
			auto maxValue_r = jmax(std::abs(range_r.getStart()), std::abs(range_r.getEnd()));

			peakLIn = jmax(peakLIn, maxValue_l);
			peakRIn = jmax(peakRIn, maxValue_r);
		}
			

		AudioSampleBuffer b(const_cast<float**>(data), numChannels, numSamples);

		if (analyser.specInVisible)
			analyser.specInBuffer.pushSamples(b, 0, numSamples);
		if (analyser.oscInVisible)
			analyser.oscInBuffer.pushSamples(b, 0, numSamples);
	}

	
	void buildSliders()
	{
		sliders.clear();

		for (int i = 0; i < (int)obj->getNumParameters(); i++)
		{
			ParameterSetter* p = new ParameterSetter(*this, i);
			addAndMakeVisible(p);
			sliders.add(p);
		}
	}

	void resized() override
	{
		auto b = getLocalBounds();

		header.setBounds(b.removeFromTop(40));

		peakMeterIn.setBounds(b.removeFromLeft(24));
		peakMeterOut.setBounds(b.removeFromRight(24));

		for (auto s : sliders)
		{
			s->setBounds(b.removeFromTop(40));
		}

		analyser.setBounds(b);

		resizer.setBounds(getLocalBounds().removeFromRight(50).removeFromBottom(50));
	}

	OwnedArray<ParameterSetter> sliders;

	ReferenceCountedObjectPtr<DspInstance> obj;

	Header header;

	VuMeter peakMeterIn;
	VuMeter peakMeterOut;

	float peakLIn, peakRIn, peakLOut, peakROut;

	Analyser analyser;
	juce::ResizableCornerComponent resizer;
};





} // namespace hise
#endif  // AUTOCOMPLETE_H_INCLUDED
