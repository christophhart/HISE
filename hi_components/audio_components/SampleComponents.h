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

namespace hise {
using namespace juce;

class WaveformComponent : public Component,
	public RingBufferComponentBase,
	public SafeChangeListener
{
public:

	struct WaveformFactory : public PathFactory
	{
		String getId() const override { return "Waveform Icons"; }

		Path createPath(const String& id) const override;
	};

	enum InterpolationMode
	{
		Truncate,
		LinearInterpolation,
		numInterpolationModes
	};

	using ScaleFunction = std::function<float(float)>;

	static float identity(float input) { return input; }

	enum WaveformType
	{
		Sine = 1,
		Triangle,
		Saw,
		Square,
		Noise,
		numWaveformTypes
	};

	class Broadcaster : public SuspendableTimer::Manager
	{
		class Updater : public SuspendableTimer,
			public ComplexDataUIUpdaterBase::EventListener
		{
		public:

			Updater(Broadcaster& p) :
				parent(p)
			{
				startTimer(30);
			}

			void timerCallback() override
			{
				if (changeFlag)
				{
					changeFlag = false;

					parent.updateData();
				}
			}

			void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override
			{
				if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
					parent.triggerWaveformUpdate();
			}

			std::atomic<bool> changeFlag;

			Broadcaster& parent;
		};

		struct BroadcasterPropertyObject : public SimpleRingBuffer::PropertyObject
		{
			BroadcasterPropertyObject(Broadcaster* br_):
				PropertyObject(nullptr),
				br(br_)
			{};

			bool validateInt(const Identifier& id, int& v) const override
			{
				if (id == RingBufferIds::BufferLength)
					return SimpleRingBuffer::toFixSize<128>(v);

				if (id == RingBufferIds::NumChannels)
					return SimpleRingBuffer::toFixSize<1>(v);
                
                return true;
			}

			void transformReadBuffer(AudioSampleBuffer& b) override
			{
				if (br != nullptr)
				{
					const float* d[1] = { nullptr };
					int numSamples = 0;
					float nv;
					br->getWaveformTableValues(0, d, numSamples, nv);

					if (numSamples == 128)
						FloatVectorOperations::copy(b.getWritePointer(0), d[0], numSamples);
				}
			}

			WeakReference<Broadcaster> br;
		};

	public:

		Broadcaster() :
			updater(*this)
		{}

		virtual ~Broadcaster() {};

		/** If you want to display a complex UI in the waveform, just connect the updaters here. */
		void connectWaveformUpdaterToComplexUI(ComplexDataUIBase* d, bool enableUpdate);

		void suspendStateChanged(bool shouldBeSuspended) override
		{
			updater.suspendTimer(shouldBeSuspended);
		}

		void triggerWaveformUpdate() { updater.changeFlag = true; };

		void addWaveformListener(WaveformComponent* listener)
		{
			listener->broadcaster = this;
			listeners.addIfNotAlreadyThere(listener);
		}

		void removeWaveformListener(WaveformComponent* listener)
		{
			listener->broadcaster = nullptr;
			listeners.removeAllInstancesOf(listener);
		}

		void updateData();

		ScaleFunction scaleFunction = identity;

		InterpolationMode interpolationMode = LinearInterpolation;

		virtual void getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue) = 0;

		virtual int getNumWaveformDisplays() const { return 1; }

	protected:

	private:

		Updater updater;
		Array<Component::SafePointer<WaveformComponent>> listeners;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Broadcaster)
	};


	enum ColourIds
	{
		bgColour = 1024,
		fillColour,
		lineColour,
		numColourIds
	};

	WaveformComponent(Processor *p, int index = 0);

	~WaveformComponent();

	void changeListenerCallback(SafeChangeBroadcaster* /*b*/) override
	{
		setBypassed(processor->isBypassed());
	}

	void setBypassed(bool shouldBeBypassed)
	{
		if (bypassed != shouldBeBypassed)
		{
			bypassed = shouldBeBypassed;
			rebuildPath();
		}
	}

	void setUseFlatDesign(bool shouldUseFlatDesign)
	{
		useFlatDesign = shouldUseFlatDesign;
		repaint();
	}

	void paint(Graphics &g);

	void refresh() override;

	Colour getColourForAnalyserBase(int colourId) override { return Colours::transparentBlack; }

	void resized() override
	{
		rebuildPath();
	}

	class Panel : public PanelWithProcessorConnection
	{
	public:

		SET_PANEL_NAME("Waveform");

		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{
			setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);
			setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white);
			setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.5f));
		}

		Identifier getProcessorTypeId() const override;

		Component* createContentComponent(int /*index*/) override;

		void fillModuleList(StringArray& moduleList) override;
	};

protected:

	void setTableValues(const float* values, int numValues, float normalizeValue_);



private:

	bool bypassed = false;

	int index = 0;

	friend class Broadcaster;

	static Path getPathForBasicWaveform(WaveformType t);

	void rebuildPath();

	Path path;

	bool useFlatDesign = false;

	WeakReference<Processor> processor;



	float const *tableValues;

	int tableLength;

	float normalizeValue;

	WeakReference<Broadcaster> broadcaster;

};


/** A component that displays the waveform of a sample.
*
*	It uses a thumbnail data to display the waveform of the selected ModulatorSamplerSound and has some SampleArea
*	objects that allow changing of its sample ranges (playback range, loop range etc.) @see SampleArea.
*
*	It uses a timer to display the current playbar.
*/
class SamplerSoundWaveform : public AudioDisplayComponent,
	public Timer
{
public:

	/** Creates a new SamplerSoundWaveform.
	*
	*	@param ownerSampler the ModulatorSampler that the SamplerSoundWaveform should use.
	*/
	SamplerSoundWaveform(const ModulatorSampler *ownerSampler);

	~SamplerSoundWaveform();


	/** used to display the playing positions / sample start position. */
	void timerCallback() override;

	/** draws a vertical ruler at the position where the sample was recently started. */
	void drawSampleStartBar(Graphics &g);

	/** enables the range (makes it possible to drag the edges). */
	void toggleRangeEnabled(AreaTypes type);

	/** Call this whenever the sample ranges change.
	*
	*	If you only want to refresh the sample area (while dragging), use refreshSampleAreaBounds() instead.
	*/
	void updateRanges(SampleArea *areaToSkip = nullptr) override;

	void updateRange(AreaTypes area, bool refreshBounds);

	double getSampleRate() const override;

	void paint(Graphics &g) override;

	void resized() override;

	/** Sets the currently displayed sound.
	*
	*	It listens for the global sound selection and displays the last selected sound if the selection changes.
	*/
	void setSoundToDisplay(const ModulatorSamplerSound *s, int multiMicIndex = 0);

	const ModulatorSamplerSound *getCurrentSound() const { return currentSound.get(); }


	float getNormalizedPeak() override;

private:

	const ModulatorSampler *sampler;
	ReferenceCountedObjectPtr<ModulatorSamplerSound> currentSound;

	int numSamplesInCurrentSample;


	double sampleStartPosition;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerSoundWaveform)
};


}
