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
	public RingBufferComponentBase
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

			Updater(Broadcaster& p);

			void timerCallback() override;
			void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;

			std::atomic<bool> changeFlag;
			Broadcaster& parent;
		};

		struct BroadcasterPropertyObject : public SimpleRingBuffer::PropertyObject
		{
			BroadcasterPropertyObject(Broadcaster* br_):
				PropertyObject(nullptr),
				br(br_)
			{};

			bool validateInt(const Identifier& id, int& v) const override;
			Path createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double startValue) const override;
			void transformReadBuffer(AudioSampleBuffer& b) override;

			WeakReference<Broadcaster> br;
		};

	public:

		Broadcaster() :
			updater(*this)
		{}

		virtual ~Broadcaster() {};

		/** If you want to display a complex UI in the waveform, just connect the updaters here. */
		void connectWaveformUpdaterToComplexUI(ComplexDataUIBase* d, bool enableUpdate);
		void suspendStateChanged(bool shouldBeSuspended) override;
		void triggerWaveformUpdate() { updater.changeFlag = true; };
		void addWaveformListener(WaveformComponent* listener);
		void removeWaveformListener(WaveformComponent* listener);
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

	WaveformComponent(Processor *p, int index = 0);
	~WaveformComponent();

	

	void setBypassed(bool shouldBeBypassed);
	

	void paint(Graphics &g);
	void resized() override;
	void refresh() override;

	Colour getColourForAnalyserBase(int colourId) override;

	class Panel : public PanelWithProcessorConnection
	{
	public:

		SET_PANEL_NAME("Waveform");

		Panel(FloatingTile* parent);
		Identifier getProcessorTypeId() const override;
		Component* createContentComponent(int /*index*/) override;
		void fillModuleList(StringArray& moduleList) override;
	};

protected:

	void setTableValues(const float* values, int numValues, float normalizeValue_);

private:

	friend class Broadcaster;

	static Path getPathForBasicWaveform(WaveformType t);
	void rebuildPath();

	bool bypassed = false;
	int index = 0;
	Path path;
	WeakReference<Processor> processor;
	float const *tableValues;
	int tableLength;
	float normalizeValue;
	WeakReference<Broadcaster> broadcaster;
};

class SamplerSoundWaveform;



struct SamplerDisplayWithTimeline : public Component
{
	static constexpr int TimelineHeight = 24;

	enum class TimeDomain
	{
		Samples,
		Milliseconds,
		Seconds
	};

	struct Properties
	{
		double sampleLength;
		double sampleRate;
		TimeDomain currentDomain = TimeDomain::Seconds;
	};

	SamplerDisplayWithTimeline(ModulatorSampler* sampler);

	SamplerSoundWaveform* getWaveform();
	const SamplerSoundWaveform* getWaveform() const;
	void resized() override;
	void mouseDown(const MouseEvent& e) override;
	static String getText(const Properties& p, float normalisedX);

	static Colour getColourForEnvelope(Modulation::Mode m);

	void paint(Graphics& g) override;

	void setEnvelope(Modulation::Mode m, ModulatorSamplerSound* sound, bool setVisible);

	Properties props;

	ScopedPointer<TableEditor> tableEditor;
	SampleLookupTable table;
	Modulation::Mode envelope = Modulation::Mode::numModes;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SamplerDisplayWithTimeline);
};

struct SamplerTools
{
    enum class Mode
    {
        Nothing,
        Zoom,
        Preview,
        PlayArea,
        SampleStartArea,
        LoopArea,
        LoopCrossfadeArea,
		ReleaseStart,
        GainEnvelope,
        PitchEnvelope,
        FilterEnvelope,
        ToolModes
    };
    
    static Colour getToolColour(Mode m);

    void toggleMode(Mode newMode);

    void setMode(Mode newMode);

    Mode currentMode = Mode::Nothing;
    LambdaBroadcaster<Mode> broadcaster;
};

/** A component that displays the waveform of a sample.
*
*	It uses a thumbnail data to display the waveform of the selected ModulatorSamplerSound and has some SampleArea
*	objects that allow changing of its sample ranges (playback range, loop range etc.) @see SampleArea.
*
*	It uses a timer to display the current playbar.
*/
class SamplerSoundWaveform : public AudioDisplayComponent,
	public Timer,
    public Processor::DeleteListener
{
public:


	/** Creates a new SamplerSoundWaveform.
	*
	*	@param ownerSampler the ModulatorSampler that the SamplerSoundWaveform should use.
	*/
	SamplerSoundWaveform(ModulatorSampler *ownerSampler);

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

	void paintOverChildren(Graphics &g) override;

	void resized() override;

    void setIsSamplerWorkspacePreview();
    
	/** Sets the currently displayed sound.
	*
	*	It listens for the global sound selection and displays the last selected sound if the selection changes.
	*/
	void setSoundToDisplay(const ModulatorSamplerSound *s, int multiMicIndex = 0);

	void mouseDown(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseMove(const MouseEvent& e) override;

	void mouseExit(const MouseEvent& e) override;

    void processorDeleted(Processor* p) override { stopTimer(); }
    
    void updateChildEditorList(bool) override {};
    
	const ModulatorSamplerSound *getCurrentSound() const { return currentSound.get(); }

	float getNormalizedPeak() override;

	void refresh(NotificationType n);

	void setVerticalZoom(float zf);

	void setClickArea(AreaTypes newArea, bool resetIfSame=true);

	float getCurrentSampleGain() const;

	SamplerDisplayWithTimeline::Properties timeProperties;

	AreaTypes currentClickArea = AreaTypes::numAreas;

	bool releaseStartIsSelected = false;

    bool zeroCrossing = true;
    
private:

	valuetree::PropertyListener gammaListener;

	bool lastActive = false;
	int xPos = -1;
	bool previewHover = false;

	ScopedPointer<LookAndFeel> slaf;

	AudioDisplayComponent::AreaTypes getAreaForModifiers(const MouseEvent& e) const;

	Identifier getSampleIdToChange(AreaTypes a, const MouseEvent& e) const;

	float verticalZoomGain = 1.0f;

    WeakReference<ModulatorSampler> sampler;
	ReferenceCountedObjectPtr<ModulatorSamplerSound> currentSound;

	int numSamplesInCurrentSample;

    bool inWorkspace = false;
	double sampleStartPosition;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerSoundWaveform);
	JUCE_DECLARE_WEAK_REFERENCEABLE(SamplerSoundWaveform);
};


class WavetableSound;

struct WaterfallComponent : public Component,
	public PooledUIUpdater::SimpleTimer,
	public ControlledObject
{
	struct LookAndFeelMethods
	{
        virtual ~LookAndFeelMethods() {};
		virtual void drawWavetableBackground(Graphics& g, WaterfallComponent& wc, bool isEmpty);
		virtual void drawWavetablePath(Graphics& g, WaterfallComponent& wc, const Path& p, int tableIndex, bool isStereo, int currentTableIndex, int numTables);
	};

	struct DefaultLookAndFeel : public LookAndFeel_V3,
		public LookAndFeelMethods
	{};

	WaterfallComponent(MainController* mc, ReferenceCountedObjectPtr<WavetableSound> sound_);

	void rebuildPaths();

	void paint(Graphics& g) override;

	void timerCallback() override;

	void resized() override;

	struct DisplayData
	{
		float modValue = 0.0f;
		ReferenceCountedObjectPtr<WavetableSound> sound;
	};

	void setPerspectiveDisplacement(const Point<float>& newDisplacement);

	Point<float> displacement;

	std::function<DisplayData()> displayDataFunction;

	struct Panel : public PanelWithProcessorConnection
	{
		SET_PANEL_NAME("WavetableWaterfall");

		enum SpecialPanelIds
		{
			Displacement = (int)PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds,
			numSpecialPanelIds
		};

		explicit Panel(FloatingTile* parent);

		Identifier getProcessorTypeId() const override;

		Component* createContentComponent(int index) override;

		int getNumDefaultableProperties() const override;

		Identifier getDefaultablePropertyId(int index) const override;

		var getDefaultProperty(int index) const override;

		void fromDynamicObject(const var& object) override;

		var toDynamicObject() const override;

		void contentChanged() override {};

		void fillModuleList(StringArray& moduleList) override;;
		void fillIndexList(StringArray& indexList) override;;

		bool hasSubIndex() const override { return true; }
	};

private:

	ReferenceCountedObjectPtr<WavetableSound> sound;
	int currentTableIndex = -1;
	int currentBank = -1;
	bool stereo = false;

	Array<Path> paths;

	DefaultLookAndFeel defaultLaf;
};

}
