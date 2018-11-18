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



class HiseAudioThumbnail: public Component,
						  public AsyncUpdater
{
public:

	static Image createPreview(const AudioSampleBuffer* buffer, int width)
	{
		jassert(buffer != nullptr);

		HiseAudioThumbnail thumbnail;

		thumbnail.setSize(width, 150);

		auto data = const_cast<float**>(buffer->getArrayOfReadPointers());

		VariantBuffer::Ptr l = new VariantBuffer(data[0], buffer->getNumSamples());

		var lVar = var(l);
		var rVar;

		thumbnail.lBuffer = var(l);

		if (data[1] != nullptr)
		{
			VariantBuffer::Ptr r = new VariantBuffer(data[1], buffer->getNumSamples());
			thumbnail.rBuffer = var(r);
		}

		thumbnail.setDrawHorizontalLines(true);

		thumbnail.loadingThread.run();

		return thumbnail.createComponentSnapshot(thumbnail.getLocalBounds());
	}


	HiseAudioThumbnail();;

	~HiseAudioThumbnail();

	void setBuffer(var bufferL, var bufferR = var());

	void paint(Graphics& g) override;

	void drawSection(Graphics &g, bool enabled);

	double getTotalLength() const
	{
		return lengthInSeconds;
	}
	
	void setReader(AudioFormatReader* r, int64 actualNumSamples=-1);

	void clear();

	void resized() override
	{
		rebuildPaths();
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
				

			rebuildOnUpdate = false;
		}

		if (repaintOnUpdate)
		{
			repaint();
			repaintOnUpdate = false;
		}
		
	}

	void setRange(const int left, const int right);
private:

	bool repaintOnUpdate = false;
	bool rebuildOnUpdate = false;

	void refresh()
	{
		repaintOnUpdate = true;
		triggerAsyncUpdate();
	}

	void rebuildPaths()
	{
		rebuildOnUpdate = true;
		triggerAsyncUpdate();
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

		void scalePathFromLevels(Path &lPath, Rectangle<float> bounds, const float* data, const int numSamples);

		void calculatePath(Path &p, float width, const float* l_, int numSamples);

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

	void refreshSampleAreaBounds(SampleArea* areaToSkip=nullptr)
	{
		if(getTotalSampleAmount() == 0) return;

		for(int i=0; i < areas.size(); i++)
		{
			if(areas[i] == areaToSkip) continue;

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

protected:

	OwnedArray<SampleArea> areas;

	AudioFormatManager afm;
	ScopedPointer<Viewport> displayViewport;
	//ScopedPointer<AudioThumbnail> preview;

	ScopedPointer<HiseAudioThumbnail> preview;

	bool onInterface = false;

private:

	

	double playBackPosition;

	SampleArea *currentArea;
	
	NormalisableRange<double> totalLength;

	ListenerList<Listener> list;

};

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
class AudioSampleBufferComponentBase: public AudioDisplayComponent,
								  public FileDragAndDropTarget,
								  public SafeChangeBroadcaster,
								  public SafeChangeListener,
								  public DragAndDropTarget,
								  public Timer
{
public:

	enum AreaTypes
	{
		PlayArea = 0,
		numAreas
	};

	AudioSampleBufferComponentBase(SafeChangeBroadcaster* p);
	virtual ~AudioSampleBufferComponentBase();

	virtual void updateProcessorConnection() = 0;
	virtual File getDefaultDirectory() const = 0;
	virtual void loadFile(const File& f) = 0;

	void itemDragEnter(const SourceDetails& dragSourceDetails) override;;
	void itemDragExit(const SourceDetails& /*dragSourceDetails*/) override;;
	
	bool isInterestedInFileDrag (const StringArray &files) override;

	static bool isAudioFile(const String &s);
	
	void filesDropped(const StringArray &fileNames, int, int);

	void setAudioSampleProcessor(SafeChangeBroadcaster* newProcessor);

	virtual void updatePlaybackPosition() = 0;
	
	/** Call this when you want the component to display the content of the given AudioSampleBuffer. 
	*
	*	It repaints the waveform, resets the range and calls rangeChanged for all registered AreaListeners.
	*/
	void setAudioSampleBuffer(const AudioSampleBuffer *b, const String &fileName, NotificationType notifyListeners);

	virtual void newBufferLoaded() = 0;

	void changeListenerCallback(SafeChangeBroadcaster* /*b*/) override
	{
		newBufferLoaded();

		repaint();
	}

	void updateRanges(SampleArea *areaToSkip=nullptr) override;

	/** Call this whenever you need to set the range from outside. */
	void setRange(Range<int> newRange);

	void setShowFileName(bool shouldShowFileName)
	{
		showFileName = shouldShowFileName;
		repaint();
	}

	void setBackgroundColour(Colour c) { bgColour = c; };

	void mouseDown(const MouseEvent &e) override;

	void timerCallback() override
	{
		repaint();
	}

	void paint(Graphics &g) override;

	void paintOverChildren(Graphics& g) override;

	/** Returns the currently loaded file name. */
	const String &getCurrentlyLoadedFileName() const
	{
		return currentFileName;
	}

	/** Returns only 44100.0 (this will have no impact, but must be overriden. */
	double getSampleRate() const override
	{
		return 44100.0;
	}

	void setShowLoop(bool shouldShowLoop)
	{
		if (showLoop != shouldShowLoop)
		{
			showLoop = shouldShowLoop;
			repaint();
		}
	}

protected:

	bool over = false;

	bool showLoop = false;
	bool showFileName = true;

	Path loopPath;

	Range<int> xPositionOfLoop;

	WeakReference<SafeChangeBroadcaster> connectedProcessor;

	Colour bgColour;

	String currentFileName;
	bool itemDragged;

	const AudioSampleBuffer *buffer;
};


} // namespace hise

#endif  // SAMPLEDISPLAYCOMPONENT_H_INCLUDED
