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

#pragma once

namespace hise { using namespace juce;

// TODO: - add 1234 snapshots, move fader to bottom, add envelope text labels, add fade to stereo / spectrum 2d (disable here)
struct MainTopBar::ClickablePeakMeter::PopupComponent: public Component,
					   public ControlledObject,
					   public Thread,
					   public ButtonListener,
					   public PathFactory
{
	enum ToolbarCommand
	{
		Freeze,
		EditProperties,
		ShowChannels,
		numToolbarCommands
	};

	enum class TimeDomainMode
	{
		Milliseconds = 1,
		Samples,
		Frequency
	};

	enum class Mode
	{
		Spectrogram,
		Oscilloscope,
		Envelope,
		FFT,
		PitchTracking,
		StereoField,
		CPU,
		numModes
	};

	static constexpr int XAxis = 15;
	static constexpr int YAxis = 40;

	struct ModeObject: public SimpleRingBuffer::PropertyObject,
							   public ControlledObject
	{
		ModeObject(BackendProcessor* bp, Mode m_);;
		var getProperty(const Identifier& id) const override;
		bool validateInt(const Identifier& id, int& v) const override;

		const Mode mode;
	};

	PopupComponent(ClickablePeakMeter* parent_);;
	~PopupComponent() override;



	Path createPath(const String& url) const override;

	void performCommand(ToolbarCommand t, bool shouldBeOn);
	void addCommand(const String& t, bool isToggle, const String& tooltip);
	Rectangle<int> getContentArea() const;
	void mouseMove(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override
	{
		currentDragInfo.active = false;
		repaint();
	}

	void mouseDown(const MouseEvent& e) override
	{
		if(e.mods.isRightButtonDown() && contentArea.contains(e.getPosition()) &&
		   currentMode != Mode::StereoField && currentMode != Mode::FFT)
		{
			PopupMenu m;
			m.setLookAndFeel(&glaf);
			m.addSectionHeader("Set time domain");
			m.addItem((int)TimeDomainMode::Milliseconds + 1, "Milliseconds", true, currentTimeDomain == TimeDomainMode::Milliseconds);
			m.addItem((int)TimeDomainMode::Samples + 1, "Samples", true, currentTimeDomain == TimeDomainMode::Samples);
			m.addItem((int)TimeDomainMode::Frequency + 1, "Frequency", true, currentTimeDomain == TimeDomainMode::Frequency);

			if(auto r = m.show())
			{
				currentTimeDomain = (TimeDomainMode)(r-1);
				repaint();
			}
		}
	}

	void setMode(Mode newMode);

	struct DragInfo
	{
		Point<int> position;
		Range<float> normalisedDistance;
		bool isXDrag = false;
		bool active = false;
		Line<float> line;
	};

	DragInfo currentDragInfo;

	static float getDecibelForY(float y);
	static float getYValue(float v);

	void paintBackground(Graphics& g) const;
	
	void refresh(bool isPost, const AudioSampleBuffer& b);
	void buttonClicked(Button* b) override;
	void rebuildPeakMeters();

	static String getDecibelText(std::pair<float, float> gainFactor);
	String getHoverText() const;

	float getHoverValue(bool isX, float normPos) const;

	String getHoverText(bool isX, float value) const;

	void run() override;
	
	void paint(Graphics& g) override;
	void resized() override;

	Rectangle<int> getAxisArea(bool getX) const;

	struct InfoBase
	{
		InfoBase(BackendProcessor* bp_, bool isPost_):
		  bp(bp_),
		  isPost(isPost_),
		  c(isPost_ ? Colour(SIGNAL_COLOUR) : JUCE_LIVE_CONSTANT_OFF(Colour(0xff9d629a)))
		{};

		virtual ~InfoBase() {};

		virtual void draw(Graphics& g, float baseAlpha) = 0;
		virtual void calculate(const AudioSampleBuffer& b, Rectangle<int> contentArea) = 0;

		void setFreeze(bool shouldFreeze)
		{
			freeze = shouldFreeze;
		}

		struct Editor: public Component,
		               public ComboBox::Listener
		{
			static constexpr int RowHeight = 32;

			Editor(SimpleRingBuffer::PropertyObject::Ptr p0_, SimpleRingBuffer::PropertyObject::Ptr p1_):
			  p0(p0_),
			  p1(p1_)
			{
				for(auto prop: p0->getPropertyList())
					addProperty(prop, getItemList(prop.toString()));

				setSize(110 + 128 + 20, properties.size() * RowHeight + 20);
			}

			static Array<var> getItemList(const String& id)
			{
				if(id == "BufferLength")
					return { 1024, 2048, 4096, 8192, 16384 };
				if(id == "FFTSize")
					return { 128, 512, 1024, 2048, 4096, 8192 };
				if(id == "WindowType")
					return FFTHelpers::getAvailableWindowTypeNames();
				if(id == "Overlap")
					return { 0.0, 0.25, 1 / 3.0, 0.5, 2 / 3.0, 0.75 };
				if(id == "Oversampling")
					return { 1.0, 2.0, 4.0, 8.0 };
				if(id == "Gamma")
					return { 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
				if(id == "UsePeakDecay")
					return { false, true };	
				if(id == "Decay")
					return { 0.1, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 };	

				return {};
			}

			void paint(Graphics& g) override
			{
				g.setColour(Colour(0xFF181818).withAlpha(0.8f));
				g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

				auto b = getLocalBounds().reduced(10);

				auto l = b.removeFromLeft(100);

				g.setFont(GLOBAL_BOLD_FONT());
				g.setColour(Colours::white.withAlpha(0.7f));

				for(auto p: properties)
				{
					g.drawText(p->first.toString(), l.removeFromTop(RowHeight).toFloat(), Justification::right);
				}
			}

			void resized() override
			{
				auto b = getLocalBounds().reduced(10);

				b.removeFromLeft(110);

				for(auto p: properties)
				{
					p->second.setBounds(b.removeFromTop(RowHeight));
				}
			}

			void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
			{
				for(auto pr: properties)
				{
					if(&pr->second == comboBoxThatHasChanged)
					{
						auto newValue = getItemList(pr->first.toString())[comboBoxThatHasChanged->getSelectedItemIndex()];
						p0->setProperty(pr->first, newValue);
						p1->setProperty(pr->first, newValue);
						return;
					}
				}
			}

			void addProperty(const Identifier& id, const Array<var>& items)
			{
				if(items.isEmpty())
					return;

				auto c = new std::pair<Identifier, ComboBox>();

				int idx = 1;

				for(auto& v: items)
					c->second.addItem(v.toString(), idx++);

				c->second.setText(p0->getProperty(id).toString(), dontSendNotification);

				c->second.addListener(this);
				c->second.setLookAndFeel(&laf);
				laf.setDefaultColours(c->second);
				c->first = id;
				addAndMakeVisible(c->second);
				
				properties.add(c);
			}

			

			SimpleRingBuffer::PropertyObject::Ptr p0, p1;
			GlobalHiseLookAndFeel laf;
			OwnedArray<std::pair<Identifier, ComboBox>> properties;
		};

		virtual Component* createEditor(SimpleRingBuffer::PropertyObject::Ptr other)
		{
			return new Editor(rbo, other);
		}

		AnalyserInfo::Ptr info;
		bool freeze = false;
		BackendProcessor* bp;
		Colour c;
		const bool isPost;
		SimpleRingBuffer::PropertyObject::Ptr rbo;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InfoBase);
	};

	struct Spec2DInfo: public InfoBase,
					   public Spectrum2D::Holder
	{
		Spec2DInfo(BackendProcessor* bp, bool isPost);

		void draw(Graphics& g, float baseAlpha) override;
		void calculate(const AudioSampleBuffer& b, Rectangle<int> contentArea) override;

		Spectrum2D::Parameters::Ptr getParameters() const override;

#if JUCE_DEBUG
		int counter = 0;
#endif
		
		float getYPosition(float input) const override
		{
			return 1.0f - std::pow(input, 0.125f);
		}

		Spectrum2D::Parameters::Ptr parameters;
		Image img;
		Rectangle<int> contentArea;
	};

	struct FFTInfo: public InfoBase
	{
		FFTInfo(BackendProcessor* bp, bool isPost):
		  InfoBase(bp, isPost)
		{
			rbo = new analyse::Helpers::FFT(bp);
			rbo->setProperty(scriptnode::PropertyIds::IsProcessingHiseEvent, false);
			rbo->setProperty("ShowCpuUsage", false);
			rbo->setProperty("Overlap", 0.75);
			rbo->setProperty("Decay", 0.1);
			rbo->setProperty("BufferLength", 8192);
			rbo->setProperty("WindowType", "Kaiser");
			rbo->setProperty("UsePeakDecay", true);
			
		}

		void calculate(const AudioSampleBuffer& b, Rectangle<int> contentArea) override;
		void draw(Graphics& g, float baseAlpha) override;

	private:

		std::array<Path, 4> fftPaths;
	};

	struct OscInfo: public InfoBase
	{
		OscInfo(BackendProcessor* bp, bool isPost):
		  InfoBase(bp, isPost)
		{
			rbo = new ModeObject(bp, Mode::Oscilloscope);
		}

		int* zIndex = nullptr;

		void draw(Graphics& g, float baseAlpha) override;
		void calculate(const AudioSampleBuffer& b, Rectangle<int> contentArea) override;
		
		std::array<Path, 4> cyclePaths;
	};

	struct PitchTrackInfo: public InfoBase
	{
		PitchTrackInfo(BackendProcessor* bp, bool isPost):
		  InfoBase(bp, isPost)
		{
			rbo = new ModeObject(bp, Mode::PitchTracking);
			rbo->setProperty("BufferLength", bp->getMainSynthChain()->getSampleRate() * 4);
			rbo->setProperty("NumChannels", 2);
		}

		void draw(Graphics& g, float baseAlpha) override;
		void calculate(const AudioSampleBuffer& b, Rectangle<int> contentArea) override;

		PopupComponent* parent = nullptr;

		// TODO: maybe use a FIFO and only calculate a new value per callback...

		Path pitchPath;
		Path prePath;
	};

	struct CpuInfo: public InfoBase
	{
		CpuInfo(BackendProcessor* bp, bool isPost):
		  InfoBase(bp, isPost)
		{
			rbo = new ModeObject(bp, Mode::CPU);
			rbo->setProperty("BufferLength", bp->getMainSynthChain()->getSampleRate() * 4);
			rbo->setProperty("NumChannels", 1);
		}

		std::array<Path, 2> cpuPaths;

		void draw(Graphics& g, float baseAlpha) override;
		void calculate(const AudioSampleBuffer& b, Rectangle<int> contentArea) override;

		Rectangle<int> area;
		
	};

	struct EnvInfo: public InfoBase
	{
		EnvInfo(BackendProcessor* bp, bool isPost):
		  InfoBase(bp, isPost)
		{
			rbo = new ModeObject(bp, Mode::Envelope);
			rbo->setProperty("BufferLength", bp->getMainSynthChain()->getSampleRate() * 4);
			rbo->setProperty("NumChannels", 2);
		}

		std::array<std::array<Path, 2>, 2> envPaths;

		void draw(Graphics& g, float baseAlpha) override;
		void calculate(const AudioSampleBuffer& b, Rectangle<int> contentArea) override;

		Rectangle<int> area;
		
	};

	struct StereoInfo: public InfoBase
	{
		StereoInfo(BackendProcessor* bp, bool isPost):
		  InfoBase(bp, isPost)
		{
			rbo = new analyse::Helpers::GonioMeter(bp);
			rbo->setProperty(scriptnode::PropertyIds::IsProcessingHiseEvent, false);
			rbo->setProperty("ShowCpuUsage", false);

			FloatVectorOperations::clear(correllations.data(), (int)correllations.size());
		}

		void calculate(const AudioSampleBuffer& b, Rectangle<int>) override;
		void draw(Graphics& g, float baseAlpha) override;

	private:
		void calculate(float left, float right);

		Rectangle<int> area;

		float smoothedCorrellation = 1.0f;
		float smoothedPan = 0.0f;
		float smoothedGain = 0.0f;
		float min_c, max_c = 0.0f;
		float coeff = 0.999f;
		std::array<float, 16> correllations;
		std::array<span<float, 101>, 5> panPos;
		
	};
	
private:

	String getTimeDomainValue(double valueAsSeconds) const;


	Rectangle<int> labelAreas[2];

	CriticalSection lock;

	AnalyserInfo::Ptr analyserData;

	struct ButtonLookAndFeel: public GlobalHiseLookAndFeel
	{
		ButtonLookAndFeel():
		  dark(Colour(0xFF141414)),
		  bright(Colour(SIGNAL_COLOUR).interpolatedWith(Colour(0xFFDDDDDD), 0.8f))
		{}

		void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/, bool over, bool down);

		void drawButtonText(Graphics& g, TextButton& b, bool over, bool down) override;

		Colour dark, bright;

	};

	GlobalHiseLookAndFeel glaf;
	ChainBarButtonLookAndFeel blaf;

	//ReferenceCountedArray<SimpleRingBuffer::PropertyObject> modeProperties;

	ScopedPointer<Component> currentEditor;
	Mode currentMode = Mode::numModes;
	int currentChannelIndex = 0;

	ScopedPointer<Component> gonioMeter[2];

	Rectangle<float> titleArea;

	Point<float> currentHoverValue;
	
	int zeroCrossingIndex = -1;

	std::array<OwnedArray<InfoBase>, 2> infos;

	std::pair<float, float> maxPeaks;
	std::pair<float, float> currentPeaks;
	Rectangle<float> peakTextArea;

	bool suspended = false;

	TimeDomainMode currentTimeDomain = TimeDomainMode::Milliseconds;

	ScopedPointer<VuMeter> peakMeter;
	Rectangle<int> contentArea;
	juce::ResizableCornerComponent resizer;
	Component::SafePointer<ClickablePeakMeter> parent;
	OwnedArray<TextButton> modes;
	OwnedArray<HiseShapeButton> toolbar;
	ReferenceCountedArray<AnalyserInfo> analyserInfo;
	Result ok;

	Slider alphaSlider;
	LookAndFeel_V4 slaf;
};

} // namespace hise


