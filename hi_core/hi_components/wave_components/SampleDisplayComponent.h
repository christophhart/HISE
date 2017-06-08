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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef SAMPLEDISPLAYCOMPONENT_H_INCLUDED
#define SAMPLEDISPLAYCOMPONENT_H_INCLUDED


class ModulatorSampler;
class ModulatorSamplerSound;

class AudioDisplayComponent;

#define EDGE_WIDTH 5

/** An AudioDisplayComponent displays the content of audio data and has some areas that can be dragged and send a change message on Mouse up
*
*	You can create subclasses of this component and populate it with some SampleArea objects (you can nest them if desired)
*/
class AudioDisplayComponent: public Component,
							 public ChangeListener
{
public:

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
		int getXForSample(int sample, bool relativeToAudioDisplayComponent=false) const
		{
			const double proportion = (double)sample / (double)parentWaveform->getTotalSampleAmount();
			const int xInWaveform = (int)(parentWaveform->getWidth() * proportion);
			const int xInParent = getParentComponent()->getLocalPoint(parentWaveform, Point<int>(xInWaveform, 0)).getX();

			return relativeToAudioDisplayComponent ? xInWaveform : xInParent;
		}

		/** Returns the sample index for the given x coordinate. 
		*
		*	If 'relativeToAudioDisplayComponent' is set to true, the x coordinate is relative to the parent AudioDisplayComponent
		*/
		int getSampleForX(int x, bool relativeToAudioDisplayComponent=false) const
		{
			// This will be useless if the width is zero!
			jassert(parentWaveform->getWidth() != 0);

			if(!relativeToAudioDisplayComponent)
				x = parentWaveform->getLocalPoint(getParentComponent(), Point<int>(x, 0)).getX();

			const int widthOfWaveForm = parentWaveform->getWidth();
			const double proportion = (double)x / (double)widthOfWaveForm;
			const int sample = (int)((double)proportion * parentWaveform->getTotalSampleAmount());

			return sample;
		}

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

		ScopedPointer<ResizableEdgeComponent> leftEdge;
		ScopedPointer<ResizableEdgeComponent> rightEdge;

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
		playBackPosition = normalizedPlaybackPosition;
		repaint();
	};

	AudioDisplayComponent(AudioThumbnailCache &cache_):
	playBackPosition(0.0)
	{
		afm.registerBasicFormats();

		preview = new AudioThumbnail(16, afm, cache_);
		preview->addChangeListener(this);
	};

#pragma warning( push )
#pragma warning( disable : 4100)

	virtual void changeListenerCallback(ChangeBroadcaster *b) override
	{
		jassert(b == preview);

		repaint();
	}

#pragma warning( pop )

	/** Removes all listeners. */
	virtual ~AudioDisplayComponent()
	{
		list.clear();

		preview->clear();
		preview->removeAllChangeListeners();
		preview = nullptr;
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

	virtual void resized() override
	{
		refreshSampleAreaBounds();
	}

	virtual void paint(Graphics &g) override
	{
        //ProcessorEditorLookAndFeel::drawNoiseBackground(g, getLocalBounds(), Colours::darkgrey);
        
		g.setColour(Colours::lightgrey.withAlpha(0.1f));
		g.drawRect(getLocalBounds(), 1);

		if(preview->getTotalLength() == 0.0) return;

		drawWaveForm(g);

		drawPlaybackBar(g);
	}

	int getTotalSampleAmount() const
	{
		return (int)(preview->getTotalLength() * getSampleRate());
	}

	SampleArea *getSampleArea(int index) {return areas[index];};

	virtual double getSampleRate() const = 0;

	/** draws the waveform of the audio data
	*
	*	It draws the deactivated region more transparent than the activated region.
	*/
	void drawWaveForm(Graphics &g);

	virtual float getNormalizedPeak() { return 1.0f; };

protected:

	OwnedArray<SampleArea> areas;

	AudioFormatManager afm;
	ScopedPointer<Viewport> displayViewport;
	ScopedPointer<AudioThumbnail> preview;

private:

	double playBackPosition;

	SampleArea *currentArea;
	
	NormalisableRange<double> totalLength;

	ListenerList<Listener> list;

};

/** A component that displays the waveform of a sample.
*	@ingroup components
*
*	It uses a thumbnail data to display the waveform of the selected ModulatorSamplerSound and has some SampleArea 
*	objects that allow changing of its sample ranges (playback range, loop range etc.) @see SampleArea.
*
*	It uses a timer to display the current playbar.
*/
class SamplerSoundWaveform: public AudioDisplayComponent,
							public Timer
{
public:

	enum AreaTypes
	{
		PlayArea = 0,
		SampleStartArea,
		LoopArea,
		LoopCrossfadeArea,
		numAreas
	};

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
	void updateRanges(SampleArea *areaToSkip=nullptr) override;

	double getSampleRate() const override;

	void paint(Graphics &g) override;

	/** Sets the currently displayed sound.
	*
	*	It listens for the global sound selection and displays the last selected sound if the selection changes. 
	*/
	void setSoundToDisplay(const ModulatorSamplerSound *s);

	const ModulatorSamplerSound *getCurrentSound() const { return currentSound.get(); }


	float getNormalizedPeak() override;

private:

	const ModulatorSampler *sampler;
	WeakReference<ModulatorSamplerSound> currentSound;

	int numSamplesInCurrentSample;

	
	double sampleStartPosition;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerSoundWaveform)
};


/** A waveform component to display the content of a pooled AudioSampleBuffer.
*
*	Features:
*
*	- draggable SampleArea which can define the range of used samples.
*	- drag and drop of pooled samples of files (only .wav)
*	- right click to open a file dialog to load a .wav file
*	- playback position display
*	- designed to interact with AudioSampleProcessor & AudioSampleBufferPool
*/
class AudioSampleBufferComponent: public AudioDisplayComponent,
								  public FileDragAndDropTarget,
								  public SafeChangeBroadcaster,
								  public SafeChangeListener,
								  public DragAndDropTarget,
								  public DragAndDropContainer,
                                  public Timer
{
public:

	enum AreaTypes
	{
		PlayArea = 0,
		numAreas
	};

	bool isInterestedInFileDrag (const StringArray &files) override
	{
		return files.size() == 1 && isAudioFile(files[0]);
	}

	static bool isAudioFile(const String &s);

	bool isInterestedInDragSource (const SourceDetails &dragSourceDetails) override;;
 	
	void itemDragExit (const SourceDetails &) override
	{
		itemDragged = false;
		repaint();
	};
	
	void itemDragEnter (const SourceDetails &dragSourceDetails) override
	{
		itemDragged = isInterestedInDragSource (dragSourceDetails);
		repaint();
	};

	void itemDropped (const SourceDetails &dragSourceDetails) override
	{
		const String s =  dragSourceDetails.description.toString();

		if(s.isNotEmpty())
		{
			currentFileName = s;
			itemDragged = false;
			sendSynchronousChangeMessage();
		}
	};
 	
	void filesDropped(const StringArray &fileNames, int , int ) override
	{
		if(fileNames.size() > 0)
		{
			currentFileName = fileNames[0];
			sendSynchronousChangeMessage();
		}
	};

	AudioSampleBufferComponent(AudioThumbnailCache &cache);

	/** Call this when you want the component to display the content of the given AudioSampleBuffer. 
	*
	*	It repaints the waveform, resets the range and calls rangeChanged for all registered AreaListeners.
	*/
	void setAudioSampleBuffer(const AudioSampleBuffer *b, const String &fileName)
	{
		if(b != nullptr)
		{
			currentFileName = fileName;

			buffer = b;

			preview->reset(b->getNumChannels(), 44100.0, 0);
			preview->addBlock(0, *b, 0, b->getNumSamples());

			preview->reset(buffer->getNumChannels(), 44100.0, 0);
			preview->addBlock(0, *buffer, 0, buffer->getNumSamples());
		
			updateRanges();

			setCurrentArea(getSampleArea(0));
			sendAreaChangedMessage();
		}
	}

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	void updateRanges(SampleArea *areaToSkip=nullptr) override
	{
		areas[PlayArea]->setSampleRange(Range<int>(0, buffer == nullptr ? 0 : buffer->getNumSamples()));
		refreshSampleAreaBounds(areaToSkip);
	}

	/** Call this whenever you need to set the range from outside. */
	void setRange(Range<int> newRange)
	{
		getSampleArea(0)->setSampleRange(newRange);
		refreshSampleAreaBounds();
	}

	void setBackgroundColour(Colour c) { bgColour = c; };

	void mouseDrag(const MouseEvent &) override
	{
		startDragging(currentFileName, this, Image(), true);
	}

	void mouseDown(const MouseEvent &e) override;

	void timerCallback() override
	{
		repaint();
	}

	void resized() override
	{
		refreshSampleAreaBounds();
	}

	void paint(Graphics &g) override;

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

private:

	Colour bgColour;

	String currentFileName;
	bool itemDragged;

	const AudioSampleBuffer *buffer;
};


#endif  // SAMPLEDISPLAYCOMPONENT_H_INCLUDED
