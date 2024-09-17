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

#ifndef SAMPLEDISPLAYCOMPONENT_H_INCLUDED
#define SAMPLEDISPLAYCOMPONENT_H_INCLUDED



namespace hise { using namespace juce;

class ModulatorSampler;
class ModulatorSamplerSound;

class AudioDisplayComponent;

#define EDGE_WIDTH 8

class HiseAudioThumbnail: public ComponentWithMiddleMouseDrag,
						  public AsyncUpdater,
                          public Spectrum2D::Holder
{
public:

	using RectangleListType = RectangleList<int>;

	enum class DisplayMode
	{
		SymmetricArea,
		DownsampledCurve,
		numDisplayModes
	};
    
    
    
    struct RenderOptions
    {
        bool operator==(const RenderOptions& r)
        {
            return memcmp(this, &r, sizeof(RenderOptions)) == 0;
        }
        
        DisplayMode displayMode = DisplayMode::SymmetricArea;
        float manualDownSampleFactor = -1.0f;
		int multithreadThreshold = 44100;
        bool drawHorizontalLines = false;
        bool scaleVertically = false;
        float displayGain = 1.0f;
        bool useRectList = false;
        int forceSymmetry = 0;
		bool dynamicOptions = false;
    };
    
	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods();;

		virtual void drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area);
		virtual void drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path);
		virtual void drawHiseThumbnailRectList(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const RectangleListType& rectList);
		virtual void drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area);
        virtual void drawThumbnailRange(Graphics& g, HiseAudioThumbnail& te, Rectangle<float> area, int areaIndex, Colour c, bool areaEnabled);

		virtual void drawThumbnailRuler(Graphics& g, HiseAudioThumbnail& te, int xPosition);
        
        virtual RenderOptions getThumbnailRenderOptions(HiseAudioThumbnail& te, const RenderOptions& defaultRenderOptions);
	};

	struct DefaultLookAndFeel : public LookAndFeel_V3,
		public LookAndFeelMethods
	{} defaultLaf;

	static Image createPreview(const AudioSampleBuffer* buffer, int width);


	HiseAudioThumbnail();;

	~HiseAudioThumbnail();

	void setBufferAndSampleRate(double sampleRate, var bufferL, var bufferR = var(), bool synchronously = false);

	void setBuffer(var bufferL, var bufferR = var(), bool synchronously=false);

	void fillAudioSampleBuffer(AudioSampleBuffer& b);

	AudioSampleBuffer getBufferCopy(Range<int> sampleRange) const;

	void paint(Graphics& g) override;

    int getNextZero(int samplePos) const;
    
	void drawSection(Graphics &g, bool enabled);

    
    
	double getTotalLength() const;

	bool shouldScaleVertically() const;;

	void setShouldScaleVertically(bool shouldScale);;

	void setDisplayGain(float gainToApply, NotificationType notify=sendNotification);

	Spectrum2D::Parameters::Ptr getParameters() const override;;

	void setReader(AudioFormatReader* r, int64 actualNumSamples=-1);

	void clear();

	void resized() override;

	void lookAndFeelChanged() override;

	void setDisplayMode(DisplayMode newDisplayMode);;

	void setManualDownsampleFactor(float newDownSampleFactor);

	void setDrawHorizontalLines(bool shouldDrawHorizontalLines);

	void handleAsyncUpdate();

	void setRebuildOnResize(bool shouldRebuild);

	void setSpectrumAndWaveformAlpha(float wAlpha, float sAlpha);
    
	void setRange(const int left, const int right);

	bool isEmpty() const noexcept;

	using AudioDataProcessor = LambdaBroadcaster<var, var>;

	AudioDataProcessor& getAudioDataProcessor();;

	float waveformAlpha = 1.0f;
	float spectrumAlpha = 0.0f;

private:

    RenderOptions currentOptions;
	bool optionsInitialised = false;
    
	//float manualDownSampleFactor = -1.0f;

	AudioDataProcessor sampleProcessor;

	//DisplayMode displayMode = DisplayMode::SymmetricArea;
	AudioSampleBuffer downsampledValues;

    //bool useRectList = false;
    
	void createCurvePathForCurrentView(bool isLeft, Rectangle<int> area);

    
    
	float applyDisplayGain(float value);

	double sampleRate = 44100.0;

	//float displayGain = 1.0f;

	//bool scaleVertically = false;
	bool rebuildOnResize = true;
	bool repaintOnUpdate = false;
	bool rebuildOnUpdate = false;

	void refresh();

	void rebuildPaths(bool synchronously = false);

	class LoadingThread : public Thread
	{
	public:

		LoadingThread(HiseAudioThumbnail* parent_);;

		void run() override;;

		void scalePathFromLevels(Path &lPath, RectangleListType& rects, Rectangle<float> bounds, const float* data, const int numSamples, bool scaleVertically);

		void calculatePath(Path &p, float width, const float* l_, int numSamples, RectangleListType& rects, bool isLeft);

	private:

        AudioSampleBuffer tempBuffer;

		WeakReference<HiseAudioThumbnail> parent;

	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseAudioThumbnail);

	CriticalSection lock;

	LoadingThread loadingThread;

	Spectrum2D::Parameters::Ptr spectrumParameters;

	bool specDirty = true;

	ScopedPointer<AudioFormatReader> currentReader;

	ScopedPointer<ScrollBar> scrollBar;

	AudioSampleBuffer ab;

	var lBuffer;
	var rBuffer;

	bool isClear = true;
	//bool drawHorizontalLines = false;

	Path leftWaveform, rightWaveform;

	RectangleListType leftPeaks, rightPeaks;

	int leftBound = -1;
	int rightBound = -1;

	double lengthInSeconds = 0.0;
    
    Image spectrum;
};

/** An AudioDisplayComponent displays the content of audio data and has some areas that can be dragged and send a change message on Mouse up
*
*	You can create subclasses of this component and populate it with some SampleArea objects (you can nest them if desired)
*/
class AudioDisplayComponent: public ComponentWithMiddleMouseDrag,
                             public SettableTooltipClient
{
public:

	enum ColourIds
	{
		bgColour = 0,
		outlineColour,
		fillColour,
		textColour,
		numColourIds,
	};

	enum AreaTypes
	{
		PlayArea = 0,
		SampleStartArea,
		LoopArea,
		LoopCrossfadeArea,
		numAreas
	};

		/** A rectangle that represents a range of samples. */
	class SampleArea: public Component
	{
	public:

		/** Creates a new SampleArea.
		*
		*	@param area the AreaType that will be used.
		*	@param parentWaveform the waveform that owns this area.
		*/
		SampleArea(int areaType, AudioDisplayComponent *parentWaveform_);

		~SampleArea();

		/** Returns the sample range (0 ... numSamples). */
		Range<int> getSampleRange() const;

		/** Sets the sample range that this SampleArea represents. */
		void setSampleRange(Range<int> r);;

        bool isAreaEnabled() const;

		/** Returns the x-coordinate of the given sample within its parent.
		*
		*	If a SampleArea is a child of another SampleArea, you can still get the absolute x value by passing 'true'.a
		*/
		int getXForSample(int sample, bool relativeToAudioDisplayComponent=false) const;

		/** Returns the sample index for the given x coordinate. 
		*
		*	If 'relativeToAudioDisplayComponent' is set to true, the x coordinate is relative to the parent AudioDisplayComponent
		*/
		int getSampleForX(int x, bool relativeToAudioDisplayComponent=false) const;

		/** Sets the current SampleArea. */
		void mouseDown(const MouseEvent &e) override;

		/** Updates the position by using a boundary check for legal bounds. */
		void mouseDrag(const MouseEvent &e) override;

		/** Sends a change message to all registered listeners of the parent AudioDisplayComponent. */
		void mouseUp(const MouseEvent &e) override;

		void paint(Graphics &g) override;

		/** This can be used to limit the sample area bounds. */
		void checkBounds();

		void resized() override;

		/** You can set a constrainer on the boundaries of the SampleArea. If you don't want a constrainer (which is the default),
		*	simply pass two empty ranges. */
		void setAllowedPixelRanges(Range<int> leftRangeInSamples, Range<int> rightRangeInSamples);

		/** This toggles the area enabled (which is not the same as Component::setEnabled()) */
		void setAreaEnabled(bool shouldBeEnabled);

		void toggleEnabled();

		/** Returns the hardcoded colour depending on the AreaType. */
		static Colour getAreaColour(AreaTypes a);

		bool leftEdgeClicked;

		class AreaEdge : public ResizableEdgeComponent,
			public SettableTooltipClient
		{
		public:

			AreaEdge(Component* componentToResize, ComponentBoundsConstrainer* constrainer, Edge edgeToResize);;
		};

		ScopedPointer<AreaEdge> leftEdge;
		ScopedPointer<AreaEdge> rightEdge;

		void setReversed(bool isReversed);

		void setGamma(float newGamma);

	private:

		float gamma = 1.0f;
		bool reversed = false;
		bool useConstrainer;

		bool areaEnabled;

		int prevDragWidth;

		struct EdgeLookAndFeel: public LookAndFeel_V3
		{
			EdgeLookAndFeel(SampleArea *areaParent);;

			void drawStretchableLayoutResizerBar (Graphics &g, Component&, int w, int h, bool isVerticalBar, bool isMouseOver, bool isMouseDragging) override;

			const SampleArea *parentArea;
		};

		ScopedPointer<EdgeLookAndFeel> edgeLaf;
		

		AudioDisplayComponent *parentWaveform;
		int area;

		Range<int> range;

		Range<int> leftEdgeRangeInPixels;
		Range<int> rightEdgeRangeInPixels;
	};

	/** draws a vertical ruler to display the current playing position. */
	void drawPlaybackBar(Graphics &g);
	
	/** Sets the playback position for (0 ... PlayArea::numSamples) 
	*
	*	If you need this functionality, use a timer callback to call this periodically.
	*/
	void setPlaybackPosition(double normalizedPlaybackPosition);;



	AudioDisplayComponent();;

	/** Removes all listeners. */
	virtual ~AudioDisplayComponent();;

	/** Acts as listener and gets a callback whenever a area was changed. */
	class Listener
	{
	public:
        
        virtual ~Listener();;

		/** overwrite this method and handle the new area (eg. set the sample properties...) */
		virtual void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea) = 0;
	};

	/** Adds an AreaListener that will be informed whenever a Area was dragged. */
	void addAreaListener(Listener *l);;

	void removeAreaListener(Listener* l);

	void refreshSampleAreaBounds(SampleArea* areaToSkip=nullptr);

	/** Overwrite this method and update the ranges of all SampleAreas of the AudioDisplayComponent. 
	*
	*	Remember to call refreshSampleAreaBounds() at the end of your method.<w
	*/
	virtual void updateRanges(SampleArea *areaToSkip=nullptr) = 0;
	
	/** Sets the current Area .*/
	void setCurrentArea(SampleArea *area);

	void sendAreaChangedMessage();

	void resized() override;

	virtual void paintOverChildren(Graphics &g) override;

	HiseAudioThumbnail* getThumbnail();

	int getTotalSampleAmount() const;

	void setIsOnInterface(bool isOnInterface);

	SampleArea *getSampleArea(int index);;

	virtual double getSampleRate() const = 0;

	
	virtual float getNormalizedPeak();;

	void mouseMove(const MouseEvent& e);

	float getHoverPosition() const;

protected:

	struct DefaultLookAndFeel : public LookAndFeel_V3,
								public HiseAudioThumbnail::LookAndFeelMethods
	{

	} defaultLaf;

	OwnedArray<SampleArea> areas;

	AudioFormatManager afm;
	ScopedPointer<Viewport> displayViewport;
	//ScopedPointer<AudioThumbnail> preview;

	ScopedPointer<HiseAudioThumbnail> preview;

	bool onInterface = false;

private:

	float hoverPosition = 0.0f;

	double playBackPosition;

	SampleArea *currentArea;
	
	NormalisableRange<double> totalLength;

	ListenerList<Listener> list;
};


/** This is a multichannel buffer type used by SNEX and scriptnode for audio files. 

	The buffer will contain two versions of the data, one is used as read-only resource
	and the other one is used for the data.

	The buffer has also the capability to host multiple files that are mapped in an XYZ
	coordinate system (like the samplemaps).
*/
struct MultiChannelAudioBuffer : public ComplexDataUIBase
{
	struct SampleReference : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<SampleReference>;

		SampleReference(bool ok = true, const String& ref = String());;

		operator bool();

		bool operator==(const SampleReference& other) const;

		AudioSampleBuffer buffer;
		Result r;
		String reference = {};
		Range<int> loopRange = {};
		double sampleRate = 0.0;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleReference);
	};

	struct DataProvider: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<DataProvider>;

		virtual ~DataProvider();

		/** Override this function and load the content and process the string to be displayed. */
		virtual SampleReference::Ptr loadFile(const String& referenceString) = 0;

		virtual File parseFileReference(const String& b64) const = 0;

		/** This directory will be used as default directory when opening files. */
		virtual File getRootDirectory();

		/** Allows you to change the default root directory. */
		virtual void setRootDirectory(const File& rootDirectory);

	protected:

		/** Use this to load a file that you have resolved to an absolute path. */
		MultiChannelAudioBuffer::SampleReference::Ptr loadAbsoluteFile(const File& f, const String& refString);

		AudioFormatManager afm;

	private:

		File rootDir;

		JUCE_DECLARE_WEAK_REFERENCEABLE(DataProvider);
	};

	struct XYZItem
	{
		using List = Array<XYZItem>;

		bool matches(int n, int v, int r);

		Range<int> veloRange;
		Range<int> keyRange;
		double root;
		int rrGroup;
		SampleReference::Ptr data;
	};

	struct XYZPool : public DataProvider
	{
		int indexOf(const String& ref) const;

		SampleReference::Ptr loadFile(const String& ref) override;

		File parseFileReference(const String& b64) const override
		{
			if(File::isAbsolutePath(b64))
				return File(b64);

			return File();
		}

		ReferenceCountedArray<SampleReference> pool;
	};

	struct XYZProviderBase : public ReferenceCountedObject
	{
		XYZProviderBase(XYZPool* pool_);

		virtual ComplexDataUIBase::EditorBase* createEditor(MultiChannelAudioBuffer* ed) = 0;

		SampleReference::Ptr loadFileFromReference(const String& f);

		void removeFromPool(SampleReference::Ptr p);

		virtual Identifier getId() const = 0;

		String getWildcard() const;

		virtual bool parse(const String& v, XYZItem::List& list) = 0;

		virtual DataProvider* getDataProvider() = 0;

		SampleReference::Ptr getPooledItem(int idx) const;

	protected:
		
		ReferenceCountedObjectPtr<XYZPool> pool;
	};

	struct XYZProviderFactory
	{
		struct Item
		{
			Identifier id;
			std::function<XYZProviderBase*()> f;
		};

		static Identifier parseID(const String& referenceString);

		void registerXYZProvider(const Identifier& id, const std::function<XYZProviderBase*()>& f);

		XYZProviderBase* create(const Identifier& id);

		Array<Identifier> getIds();

	private:

		Array<Item> items;
	};
	
	/** A Listener will be notified about changes to the sample content. 
	
		By default the content change notifications are synchronous
		(because you might want to update stuff for the processing), 
		but you can change the notification to be asynchronous in order
		to do UI stuff (there's no guarantee from which thread the events
		might be executed).
	*/
	struct Listener: private ComplexDataUIUpdaterBase::EventListener
	{
		Listener();
		virtual ~Listener();

		/** This will be called (synchronously while holding the data write lock) whenever the data is relocated. */
		virtual void bufferWasLoaded() = 0;

		/** This will be called (synchronously but without holding the lock) whenever the (non-original) data has been modified. */
		virtual void bufferWasModified() = 0;

		/** This will be called asynchronously whenever the sample index has been changed. The index is relative to the data buffer. */
		virtual void sampleIndexChanged(int newSampleIndex);;

	private:

		friend struct MultiChannelAudioBuffer;

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var v) override;
	};

	void addListener(Listener* l);

	void removeListener(Listener* l);

	String toBase64String() const override;

	void setXYZProvider(const Identifier& id);

	bool fromBase64String(const String& b64) override;

	
	/** Set the range of the buffer. The notification to the listeners will always be synchronous. */
	void setRange(Range<int> sampleRange);

	void loadFromEmbeddedData(SampleReference::Ptr r);

	void loadBuffer(const AudioSampleBuffer& b, double sr);

	void setLoopRange(Range<int> newLoopRange, NotificationType n);

	var getChannelBuffer(int channelIndex, bool getFullContent);

	void setProvider(DataProvider::Ptr p);

	Range<int> getCurrentRange() const;

	Range<int> getTotalRange() const;

	Range<int> getLoopRange(bool subtractStart = false) const;

	double sampleRate = 0.0;

	AudioSampleBuffer& getBuffer();
	const AudioSampleBuffer& getBuffer() const;

	DataProvider::Ptr getProvider();

	bool isEmpty() const;

	bool isNotEmpty() const;

	float** getDataPtrs();

	Array<Identifier> getAvailableXYZProviders();

	Identifier getCurrentXYZId() const;

	bool isXYZ() const;

	void registerXYZProvider(const Identifier& id, const std::function<XYZProviderBase*()> & f);

	ComplexDataUIBase::EditorBase* createEditor();

	const XYZItem::List& getXYZItems() const;
	XYZItem::List& getXYZItems();

	SampleReference::Ptr getFirstXYZData();

	void setDisabledXYZProviders(const Array<Identifier>& ids);

	struct ScopedUndoActivator
	{
		ScopedUndoActivator(MultiChannelAudioBuffer& parent, bool value=true):
		  state(parent.useUndoManagerForLoadOperation, value)
		{}

		ScopedValueSetter<bool> state;
	};

private:

	bool useUndoManagerForLoadOperation = false;

	Array<Identifier> deactivatedXYZIds;

	SharedResourcePointer<XYZProviderFactory> factory;

	void setDataBuffer(AudioSampleBuffer& newBuffer);

	AudioSampleBuffer createNewDataBuffer(Range<int> newRange);

	friend struct DataProvider;

	Range<int> bufferRange;
	Range<int> loopRange;
	
	String referenceString;
	
	AudioSampleBuffer originalBuffer;
	AudioSampleBuffer currentData;
	DataProvider::Ptr provider;

	XYZItem::List xyzItems;
	ReferenceCountedObjectPtr<XYZProviderBase> xyzProvider;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MultiChannelAudioBuffer);
};


struct XYZMultiChannelAudioBufferEditor : public ComplexDataUIBase::EditorBase,
										  public Component,
										  public ButtonListener
{
	void setComplexDataUIBase(ComplexDataUIBase* newData) override;

	void addButton(const Identifier& id, const Identifier& currentId);

	void buttonClicked(Button* b);

	void rebuildButtons();

	void rebuildEditor();

	void paint(Graphics& g) override;

	void resized() override;

	OwnedArray<TextButton> buttons;

	ScopedPointer<Component> currentEditor;

	WeakReference<MultiChannelAudioBuffer> currentBuffer;
};


/** Rewrite AudioDisplayComponent:

	- add Locking on MultiChannelAudioBuffer level
	- make it use the MultiChannelAudioFile
	- remove all Listeners & replace with one
	- remove inheritance stuff and replace with one provider class

*/

/** A waveform component to display the content of a pooled AudioSampleBuffer.
*	@ingroup hise_ui
*
*	Features:
*
*	- draggable SampleArea which can define the range of used samples.
*	- drag and drop of pooled samples of files (only .wav)
*	- right click to open a file dialog to load a .wav file
*	- playback position display
*	- designed to interact with AudioSampleProcessor & AudioSampleBufferPool
*/
class MultiChannelAudioBufferDisplay: public AudioDisplayComponent,
									  public FileDragAndDropTarget,
									  public DragAndDropTarget,
									  public ComplexDataUIBase::EditorBase,
									  public MultiChannelAudioBuffer::Listener,
									  public AudioDisplayComponent::Listener
{
public:

	enum AreaTypes
	{
		PlayArea = 0,
		numAreas
	};

	struct BufferLookAndFeel : public LookAndFeel_V3,
							   public HiseAudioThumbnail::LookAndFeelMethods
	{

	};

	virtual void setSpecialLookAndFeel(LookAndFeel* l, bool shouldOwn=false);

	

	void setLoadWithLeftClick(bool newValue)
	{
		loadWithLeftClick = newValue;
	}

	MultiChannelAudioBufferDisplay();
	virtual ~MultiChannelAudioBufferDisplay();

	void itemDragEnter(const SourceDetails& dragSourceDetails) override;;
	void itemDragExit(const SourceDetails& /*dragSourceDetails*/) override;;
	
	bool isInterestedInFileDrag (const StringArray &files) override;

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	static bool isAudioFile(const String &s);
	
	void filesDropped(const StringArray &fileNames, int, int);

	void updateRanges(SampleArea *areaToSkip=nullptr) override;

	/** Call this whenever you need to set the range from outside. */
	void setRange(Range<int> newRange);

	void setShowFileName(bool shouldShowFileName);

	void rangeChanged(AudioDisplayComponent *, int ) override;

	void setBackgroundColour(Colour c);;

	void mouseDown(const MouseEvent &e) override;

	void mouseDoubleClick(const MouseEvent&);

	void paint(Graphics &g) override;

	void paintOverChildren(Graphics& g) override;

	/** Returns the currently loaded file name. */
	String getCurrentlyLoadedFileName() const;

	/** Returns only 44100.0 (this will have no impact, but must be overriden. */
	double getSampleRate() const override;

	void setShowLoop(bool shouldShowLoop);

	void bufferWasLoaded() override;

	void bufferWasModified() override;

	void sampleIndexChanged(int newSampleIndex) override;

	void setComplexDataUIBase(ComplexDataUIBase* newData) override;

	void setAudioFile(MultiChannelAudioBuffer* af);

	MultiChannelAudioBuffer* getBuffer();

protected:

	WeakReference<MultiChannelAudioBuffer> connectedBuffer;

	bool over = false;

	bool loadWithLeftClick = false;
	bool showLoop = false;
	bool showFileName = true;

	Path loopPath;

	Range<float> xPositionOfLoop;

	Colour bgColour;

	bool itemDragged;
};


} // namespace hise

#endif  // SAMPLEDISPLAYCOMPONENT_H_INCLUDED
