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

#define EDGE_WIDTH 5

#ifndef HISE_USE_SYMMETRIC_WAVEFORMS
#define HISE_USE_SYMMETRIC_WAVEFORMS 0
#endif


class HiseAudioThumbnail: public Component,
						  public AsyncUpdater
{
public:

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods() {};

		virtual void drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area);
		virtual void drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path);
		virtual void drawHiseThumbnailRectList(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const RectangleList<float>& rectList);
		virtual void drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area);
	};

	

	static Image createPreview(const AudioSampleBuffer* buffer, int width)
	{
		jassert(buffer != nullptr);

		HiseAudioThumbnail thumbnail;

		thumbnail.setSize(width, 150);

		auto data = const_cast<float**>(buffer->getArrayOfReadPointers());

		VariantBuffer::Ptr l = new VariantBuffer(data[0], buffer->getNumSamples());

		var lVar = var(l.get());
		var rVar;

		thumbnail.lBuffer = var(l.get());

		if (data[1] != nullptr)
		{
			VariantBuffer::Ptr r = new VariantBuffer(data[1], buffer->getNumSamples());
			thumbnail.rBuffer = var(r.get());
		}

		thumbnail.setDrawHorizontalLines(true);

		thumbnail.loadingThread.run();

		return thumbnail.createComponentSnapshot(thumbnail.getLocalBounds());
	}


	HiseAudioThumbnail();;

	~HiseAudioThumbnail();

	void setBufferAndSampleRate(double sampleRate, var bufferL, var bufferR = var(), bool synchronously = false);

	void setBuffer(var bufferL, var bufferR = var(), bool synchronously=false);

	void paint(Graphics& g) override;

	void drawSection(Graphics &g, bool enabled);

	double getTotalLength() const
	{
		return lengthInSeconds;
	}
	
	bool shouldScaleVertically() const { return scaleVertically; };

	void setShouldScaleVertically(bool shouldScale)
	{
		scaleVertically = shouldScale;
	};

	void setReader(AudioFormatReader* r, int64 actualNumSamples=-1);

	void clear();

	void resized() override
	{
		if (rebuildOnResize)
			rebuildPaths();
		else
		{
			repaint();
		}
			
	}

	void setDrawHorizontalLines(bool shouldDrawHorizontalLines)
	{
		drawHorizontalLines = shouldDrawHorizontalLines;
		repaint();
	}

	void handleAsyncUpdate()
	{
		if (rebuildOnUpdate)
		{
			loadingThread.stopThread(-1);
			
			

			loadingThread.startThread(5);
				
			repaint();
			rebuildOnUpdate = false;
		}

		if (repaintOnUpdate)
		{
			repaint();
			repaintOnUpdate = false;
		}
		
	}

	void setRebuildOnResize(bool shouldRebuild)
	{
		rebuildOnResize = shouldRebuild;
	}

	void setRange(const int left, const int right);
private:

	double sampleRate = 44100.0;

	bool scaleVertically = false;
	bool rebuildOnResize = true;
	bool repaintOnUpdate = false;
	bool rebuildOnUpdate = false;

	void refresh()
	{
		repaintOnUpdate = true;
		triggerAsyncUpdate();
	}

	void rebuildPaths(bool synchronously = false)
	{
		if (synchronously)
		{
			isClear = true;
			
			loadingThread.run();

			Component::SafePointer<Component> thisSafe = this;

			auto f = [thisSafe]()
			{
				if (thisSafe.getComponent() != nullptr)
					thisSafe.getComponent()->repaint();
			};

			MessageManager::callAsync(f);
		}
		else
		{
			rebuildOnUpdate = true;
			triggerAsyncUpdate();
		}
	}

	class LoadingThread : public Thread
	{
	public:

		LoadingThread(HiseAudioThumbnail* parent_) :
			Thread("Thumbnail Generator"),
			parent(parent_)
		{
		};

		void run() override;;

		void scalePathFromLevels(Path &lPath, RectangleList<float>& rects, Rectangle<float> bounds, const float* data, const int numSamples, bool scaleVertically);

		void calculatePath(Path &p, float width, const float* l_, int numSamples, RectangleList<float>& rects);

	private:

		

		WeakReference<HiseAudioThumbnail> parent;

	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseAudioThumbnail);

	CriticalSection lock;

	LoadingThread loadingThread;

	ScopedPointer<AudioFormatReader> currentReader;

	ScopedPointer<ScrollBar> scrollBar;

	var lBuffer;
	var rBuffer;

	bool isClear = true;
	bool drawHorizontalLines = false;

	Path leftWaveform, rightWaveform;

	RectangleList<float> leftPeaks, rightPeaks;

	int leftBound = -1;
	int rightBound = -1;

	double lengthInSeconds = 0.0;
};

/** An AudioDisplayComponent displays the content of audio data and has some areas that can be dragged and send a change message on Mouse up
*
*	You can create subclasses of this component and populate it with some SampleArea objects (you can nest them if desired)
*/
class AudioDisplayComponent: public Component
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

#if 0 // sometime in the future...
		class ValuePopup : public Component,
			public Timer
		{
		public:

			ValuePopup(Component* c_) :
				c(c_)
			{
				updateText();
				startTimer(30);
			}

			void updateText()
			{
				if (auto area = dynamic_cast<SampleArea*>(c.getComponent()))
				{
					auto oldText = currentText;

					auto range = area->getSampleRange();

					currentText = String(range.getStart()) + " - " + String(range.getEnd());

					if (currentText != oldText)
					{
						auto f = GLOBAL_BOLD_FONT();
						int newWidth = f.getStringWidth(currentText) + 20;

						setSize(newWidth, 20);

						repaint();
					}
				}
			}

			void timerCallback() override
			{
				updateText();
			}

			void paint(Graphics& g) override
			{

				auto ar = Rectangle<float>(1.0f, 1.0f, (float)getWidth() - 2.0f, (float)getHeight() - 2.0f);

				g.setGradientFill(ColourGradient(itemColour, 0.0f, 0.0f, itemColour2, 0.0f, (float)getHeight(), false));
				g.fillRoundedRectangle(ar, 2.0f);

				g.setColour(bgColour);
				g.drawRoundedRectangle(ar, 2.0f, 2.0f);

				if (dynamic_cast<Slider*>(c.getComponent()) != nullptr)
				{
					g.setFont(GLOBAL_BOLD_FONT());
					g.setColour(textColour);
					g.drawText(currentText, getLocalBounds(), Justification::centred);
				}
			}

			Colour bgColour;
			Colour itemColour;
			Colour itemColour2;
			Colour textColour;

			String currentText;

			Component::SafePointer<Component> c;
		};
#endif

		/** Creates a new SampleArea.
		*
		*	@param area the AreaType that will be used.
		*	@param parentWaveform the waveform that owns this area.
		*/
		SampleArea(int areaType, AudioDisplayComponent *parentWaveform_);

		~SampleArea();

		/** Returns the sample range (0 ... numSamples). */
		Range<int> getSampleRange() const {	return range; }

		/** Sets the sample range that this SampleArea represents. */
		void setSampleRange(Range<int> r)
		{ 
			range = r; 

			repaint();
		};

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
		void setAllowedPixelRanges(Range<int> leftRangeInSamples, Range<int> rightRangeInSamples)
		{
			useConstrainer = !(leftRangeInSamples.isEmpty() && rightRangeInSamples.isEmpty());
			
			if(!useConstrainer) return;
			

			leftEdgeRangeInPixels = Range<int>(getXForSample(leftRangeInSamples.getStart(), false),
											   getXForSample(leftRangeInSamples.getEnd(), false));

			rightEdgeRangeInPixels = Range<int>(getXForSample(rightRangeInSamples.getStart(), false),
											   getXForSample(rightRangeInSamples.getEnd(), false));
		}

		/** This toggles the area enabled (which is not the same as Component::setEnabled()) */
		void setAreaEnabled(bool shouldBeEnabled)
		{
			areaEnabled = shouldBeEnabled;

			leftEdge->setInterceptsMouseClicks(areaEnabled, false);
			rightEdge->setInterceptsMouseClicks(areaEnabled, false);

			repaint();
		}

		void toggleEnabled()
		{
			setAreaEnabled(!areaEnabled);

			repaint();
		}

		/** Returns the hardcoded colour depending on the AreaType. */
		Colour getAreaColour() const;

		bool leftEdgeClicked;

		class AreaEdge : public ResizableEdgeComponent,
			public SettableTooltipClient
		{
		public:

			AreaEdge(Component* componentToResize, ComponentBoundsConstrainer* constrainer, Edge edgeToResize) :
				ResizableEdgeComponent(componentToResize, constrainer, edgeToResize)
			{};
		};

		ScopedPointer<AreaEdge> leftEdge;
		ScopedPointer<AreaEdge> rightEdge;

	private:

		bool useConstrainer;

		bool areaEnabled;

		int prevDragWidth;

		struct EdgeLookAndFeel: public LookAndFeel_V3
		{
			EdgeLookAndFeel(SampleArea *areaParent): parentArea(areaParent) {};

			void drawStretchableLayoutResizerBar (Graphics &g, int w, int h, bool isVerticalBar, bool isMouseOver, bool isMouseDragging) override;

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
	void setPlaybackPosition(double normalizedPlaybackPosition)
	{
		if(playBackPosition != normalizedPlaybackPosition)
        {
            playBackPosition = normalizedPlaybackPosition;
            repaint();
        }
	};



	AudioDisplayComponent():
	playBackPosition(0.0)
	{
		afm.registerBasicFormats();

		addAndMakeVisible(preview = new HiseAudioThumbnail());

		preview->setLookAndFeel(&defaultLaf);
	};

	/** Removes all listeners. */
	virtual ~AudioDisplayComponent()
	{
		preview = nullptr;

		list.clear();		
	};

	/** Acts as listener and gets a callback whenever a area was changed. */
	class Listener
	{
	public:
        
        virtual ~Listener() {};

		/** overwrite this method and handle the new area (eg. set the sample properties...) */
		virtual void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea) = 0;
	};

	/** Adds an AreaListener that will be informed whenever a Area was dragged. */
	void addAreaListener(Listener *l)
	{
		list.add(l);
	};

	void removeAreaListener(Listener* l)
	{
		list.remove(l);
	}

	void refreshSampleAreaBounds(SampleArea* areaToSkip=nullptr)
	{
		bool somethingVisible = getTotalSampleAmount() != 0;

		for(int i=0; i < areas.size(); i++)
		{
			if(areas[i] == areaToSkip) continue;

			areas[i]->setVisible(somethingVisible);

			Range<int> sampleRange = areas[i]->getSampleRange();

			const int x = areas[i]->getXForSample(sampleRange.getStart(), false);
			const int right = areas[i]->getXForSample(sampleRange.getEnd(), false);

			areas[i]->leftEdge->setTooltip(String(sampleRange.getStart()));
			areas[i]->rightEdge->setTooltip(String(sampleRange.getEnd()));

			if (i == 0)
			{
				preview->setRange(x, right);
			}

			areas[i]->setBounds(x,0,right-x, getHeight());
		}

		

		repaint();
	}

	/** Overwrite this method and update the ranges of all SampleAreas of the AudioDisplayComponent. 
	*
	*	Remember to call refreshSampleAreaBounds() at the end of your method.<w
	*/
	virtual void updateRanges(SampleArea *areaToSkip=nullptr) = 0;
	
	/** Sets the current Area .*/
	void setCurrentArea(SampleArea *area)
	{
		currentArea = area;
	}

	void sendAreaChangedMessage()
	{
		list.call(&Listener::rangeChanged, this, areas.indexOf(currentArea));
		repaint();
	}

	void resized() override
	{
		preview->setBounds(getLocalBounds());
		preview->resized();
		refreshSampleAreaBounds();
	}

	virtual void paint(Graphics &g) override;

	HiseAudioThumbnail* getThumbnail()
	{
		return preview;
	}

	int getTotalSampleAmount() const
	{
		return (int)(preview->getTotalLength() * getSampleRate());
	}

	void setIsOnInterface(bool isOnInterface)
	{
		onInterface = isOnInterface;
	}

	SampleArea *getSampleArea(int index) {return areas[index];};

	virtual double getSampleRate() const = 0;

	
	virtual float getNormalizedPeak() { return 1.0f; };

	void mouseMove(const MouseEvent& e)
	{
		auto xNormalised = (float)e.getPosition().getX() / (float)getWidth();

		hoverPosition = xNormalised * (float)getTotalSampleAmount();
	}

	float getHoverPosition() const
	{
		return hoverPosition;
	}

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
*/
struct MultiChannelAudioBuffer : public ComplexDataUIBase
{
	struct DataProvider: public ReferenceCountedObject
	{
		struct LoadResult
		{
			LoadResult() :
				r(Result::ok())
			{};

			operator bool() { return r.wasOk(); }

			AudioSampleBuffer buffer;
			Result r;
			String reference = {};
			Range<int> loopRange = {};
			double sampleRate = 0.0;
		};

		using Ptr = ReferenceCountedObjectPtr<DataProvider>;

		virtual ~DataProvider() = default;

		/** Override this function and load the content and process the string to be displayed. */
		virtual LoadResult loadFile(const String& referenceString) = 0;

		/** This directory will be used as default directory when opening files. */
		virtual File getRootDirectory() { return File(); }

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(DataProvider);
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
		Listener() = default;
		virtual ~Listener() = default;

		/** This will be called (synchronously while holding the data write lock) whenever the data is relocated. */
		virtual void bufferWasLoaded() = 0;

		/** This will be called (synchronously but without holding the lock) whenever the (non-original) data has been modified. */
		virtual void bufferWasModified() = 0;

		/** This will be called asynchronously whenever the sample index has been changed. The index is relative to the data buffer. */
		virtual void sampleIndexChanged(int newSampleIndex) {};

	private:

		friend class MultiChannelAudioBuffer;

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var v) override
		{
			switch (d)
			{
			case ComplexDataUIUpdaterBase::EventType::ContentChange:
				bufferWasModified();
				break;
			case ComplexDataUIUpdaterBase::EventType::ContentRedirected:
				bufferWasLoaded();
				break;
			case ComplexDataUIUpdaterBase::EventType::DisplayIndex:
				sampleIndexChanged((int)v);
				break;
			}
		}
	};

	void addListener(Listener* l)
	{
		internalUpdater.addEventListener(l);
	}

	void removeListener(Listener* l)
	{
		internalUpdater.removeEventListener(l);
	}

	String toBase64String() const override 
	{ 
		return referenceString;
	}
	
	bool fromBase64String(const String& b64) override 
	{ 
		if (b64 != referenceString)
		{
			referenceString = b64;

			jassert(provider != nullptr);

			if (provider != nullptr)
			{
				if (auto lr = provider->loadFile(referenceString))
				{
					originalBuffer = lr.buffer;
					auto nb = createNewDataBuffer({ 0, originalBuffer.getNumSamples() });

					referenceString = lr.reference;

					{
						SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
						bufferRange = { 0, originalBuffer.getNumSamples() };
						sampleRate = lr.sampleRate;
						setDataBuffer(nb);
					}

					setLoopRange(lr.loopRange, dontSendNotification);

					return true;
				}
				else
				{
					
					originalBuffer = {};

					SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
					bufferRange = {};
					currentData = {};
					getUpdater().sendContentRedirectMessage();
					return false;
				}
			}
		}
		
		return false; 
	}

	
	/** Set the range of the buffer. The notification to the listeners will always be synchronous. */
	void setRange(Range<int> sampleRange)
	{
		sampleRange.setStart(jmax(0, sampleRange.getStart()));
		sampleRange.setEnd(jmin(originalBuffer.getNumSamples(), sampleRange.getEnd()));

		if (sampleRange != bufferRange)
		{
			{
				auto nb = createNewDataBuffer(sampleRange);

				SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
				bufferRange = sampleRange;
				setDataBuffer(nb);
			}
		}
	}

	void loadBuffer(const AudioSampleBuffer& b, double sr)
	{
		referenceString = "{INTERNAL}";
		
		originalBuffer.makeCopyOf(b);

		auto nb = createNewDataBuffer({ 0, b.getNumSamples() });
		SimpleReadWriteLock::ScopedWriteLock l(getDataLock());
		sampleRate = sr;
		bufferRange = { 0, b.getNumSamples() };
		setDataBuffer(nb);
	}

	void setLoopRange(Range<int> newLoopRange, NotificationType n)
	{
		newLoopRange.setStart(jmax(bufferRange.getStart(), newLoopRange.getStart()));
		newLoopRange.setEnd(jmin(bufferRange.getEnd(), newLoopRange.getEnd()));

		if (newLoopRange != loopRange)
		{
			{
				SimpleReadWriteLock::ScopedWriteLock sl(getDataLock());
				loopRange = newLoopRange;
				
				if(loopRange.getEnd() > 1)
					bufferRange.setEnd(loopRange.getEnd());
			}
			
			if(n != dontSendNotification)
				getUpdater().sendContentChangeMessage(sendNotificationSync, -1);
		}
	}

	var getChannelBuffer(int channelIndex, bool getFullContent)
	{
		auto& bToUse = getFullContent ? originalBuffer : currentData;

		if(isPositiveAndBelow(channelIndex, bToUse.getNumChannels()))
			return var(new VariantBuffer(bToUse.getWritePointer(channelIndex, 0), bToUse.getNumSamples()));

		return {};
	}
	
	void setProvider(DataProvider* p)
	{
		provider = p;
	}

	Range<int> getCurrentRange() const
	{
		return bufferRange;
	}

	Range<int> getTotalRange() const
	{
		return { 0, originalBuffer.getNumSamples() };
	}

	Range<int> getLoopRange() const
	{
		return loopRange;
	}

	double sampleRate = 0.0;

	AudioSampleBuffer& getBuffer() { return currentData; }
	const AudioSampleBuffer& getBuffer() const { return currentData; }

	DataProvider::Ptr getProvider()
	{
		return provider;
	}

	bool isEmpty() const
	{
		return originalBuffer.getNumChannels() == 0 || originalBuffer.getNumSamples() == 0;
	}

	bool isNotEmpty() const
	{
		return originalBuffer.getNumChannels() != 0 || originalBuffer.getNumSamples() != 0;
	}

	float** getDataPtrs()
	{
		return currentData.getArrayOfWritePointers();
	}

private:

	void setDataBuffer(AudioSampleBuffer& newBuffer)
	{
		// Never call this without holding the lock
		jassert(getDataLock().writeAccessIsLocked());

		std::swap(currentData, newBuffer);
		getUpdater().sendContentRedirectMessage();
	}

	AudioSampleBuffer createNewDataBuffer(Range<int> newRange)
	{
		if (newRange.isEmpty())
		{
			return {};
		}
		
		SimpleReadWriteLock::ScopedReadLock l(getDataLock());

		AudioSampleBuffer newDataBuffer(originalBuffer.getNumChannels(), newRange.getLength());

		for (int i = 0; i < newDataBuffer.getNumChannels(); i++)
			newDataBuffer.copyFrom(i, 0, originalBuffer.getReadPointer(i, newRange.getStart()), newDataBuffer.getNumSamples());

		return newDataBuffer;
	}

	friend class DataProvider;

	Range<int> bufferRange;
	Range<int> loopRange;
	
	String referenceString;
	
	AudioSampleBuffer originalBuffer;
	AudioSampleBuffer currentData;
	DataProvider::Ptr provider;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MultiChannelAudioBuffer);
};

#if 0
struct MultiChannelAudioBufferDisplay : public AudioDisplayComponent,
										public ComplexDataUIBase::EditorBase,
										public MultiChannelAudioBuffer::Listener,
										public AudioDisplayComponent::Listener
{
	MultiChannelAudioBufferDisplay()
	{
		addAreaListener(this);

		areas.add(new SampleArea(AreaTypes::PlayArea, this));
		addAndMakeVisible(areas[0]);
		areas[0]->setAreaEnabled(true);
	}

	

	void updateRanges(SampleArea *areaToSkip/* =nullptr */) override
	{
		areas[0]->setSampleRange(connectedFile->getCurrentRange());
		refreshSampleAreaBounds(areaToSkip);
	}

	double getSampleRate() const override
	{
		if (connectedFile != nullptr)
			return connectedFile->sampleRate;
	}

	

	void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea) override
	{
		if (auto ar = getSampleArea(changedArea))
		{
			if (connectedFile != nullptr)
			{
				ar->getSampleRange();
			}
		}
	}

	

	WeakReference<MultiChannelAudioBuffer> connectedFile;
};
#endif


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

	virtual void setSpecialLookAndFeel(LookAndFeel* l, bool shouldOwn=false)
	{
		preview->setLookAndFeel(l);
		EditorBase::setSpecialLookAndFeel(l, shouldOwn);
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

	void setShowFileName(bool shouldShowFileName)
	{
		showFileName = shouldShowFileName;
		repaint();
	}

	void rangeChanged(AudioDisplayComponent *, int ) override
	{
		auto range = areas[0]->getSampleRange();

		if (connectedBuffer != nullptr)
			connectedBuffer->setRange(range);
	}

	void setBackgroundColour(Colour c) { bgColour = c; };

	void mouseDown(const MouseEvent &e) override;

	void mouseDoubleClick(const MouseEvent&)
	{
		if (connectedBuffer != nullptr)
			connectedBuffer->fromBase64String({});
	}

	void paint(Graphics &g) override;

	void paintOverChildren(Graphics& g) override;

	/** Returns the currently loaded file name. */
	const String &getCurrentlyLoadedFileName() const
	{
		if (connectedBuffer != nullptr)
		{
			auto a = connectedBuffer->toBase64String();

			if (a == "-1")
				return {};

			return a;
		}

		return {};
	}

	/** Returns only 44100.0 (this will have no impact, but must be overriden. */
	double getSampleRate() const override
	{
		if (connectedBuffer != nullptr)
			return connectedBuffer->sampleRate;

		return 0.0;
	}

	void setShowLoop(bool shouldShowLoop)
	{
		if (showLoop != shouldShowLoop)
		{
			showLoop = shouldShowLoop;
			repaint();
		}
	}

	void bufferWasLoaded() override
	{
		if (connectedBuffer != nullptr)
			preview->setBufferAndSampleRate(connectedBuffer->sampleRate, connectedBuffer->getChannelBuffer(0, true), connectedBuffer->getChannelBuffer(1, true));
		else
			preview->setBuffer({}, {});
		
		updateRanges(nullptr);
	}

	void bufferWasModified() override
	{
		updateRanges(nullptr);
	}

	void sampleIndexChanged(int newSampleIndex) override
	{
		if (connectedBuffer != nullptr)
		{
			auto s = connectedBuffer->getCurrentRange().getLength();
			AudioDisplayComponent::setPlaybackPosition((double)newSampleIndex / s);
			repaint();
		}
	}

	void setComplexDataUIBase(ComplexDataUIBase* newData) override
	{
		if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(newData))
			setAudioFile(af);
	}

	void setAudioFile(MultiChannelAudioBuffer* af)
	{
		if (af != connectedBuffer)
		{
			if (connectedBuffer != nullptr)
				connectedBuffer->removeListener(this);

			connectedBuffer = af;
			bufferWasLoaded();

			if (connectedBuffer != nullptr)
				connectedBuffer->addListener(this);
		}
	}

protected:

	WeakReference<MultiChannelAudioBuffer> connectedBuffer;

	bool over = false;

	bool showLoop = false;
	bool showFileName = true;

	Path loopPath;

	Range<int> xPositionOfLoop;

	Colour bgColour;

	bool itemDragged;
};


} // namespace hise

#endif  // SAMPLEDISPLAYCOMPONENT_H_INCLUDED
