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

namespace hise
{
using namespace juce;

#if USE_BACKEND
void DAWClockController::LAF::drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
    g.setFont(GLOBAL_BOLD_FONT().withHeight(s.getHeight() - 2.0f));
    g.setColour(Colours::white.withAlpha(s.isMouseOverOrDragging() ? 0.9f : 0.7f));
    
    auto v = (int)s.getValue();
    
    if(s.getName() == "denom")
        v = nextPowerOfTwo(v);
    
    g.drawText(String(v), s.getLocalBounds().toFloat(), Justification::centred);
}

void DAWClockController::BackendAudioRenderer::handleAsyncUpdate()
{
	if(PresetHandler::showYesNoWindow("Write audio file", "Do you want to write the exported audio data to a file"))
	{
		FileChooser fc("Select file", File(), "*.wav", true);

		if(fc.browseForFileToSave(true))
		{
			auto targetFile = fc.getResult();

			WavAudioFormat afm;
    
			targetFile.deleteFile();
			targetFile.create();

			FileOutputStream* fis = new FileOutputStream(targetFile);
			StringPairArray metadata;
			ScopedPointer<AudioFormatWriter> writer = afm.createWriterFor(fis, parent.getMainController()->getMainSynthChain()->getSampleRate(), buffer.getNumChannels(), 16, metadata, 5);

			if (writer != nullptr)
				writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());

			targetFile.revealToUser();
		}
	}
    
    parent.exportProgress = -1.0f;
    parent.repaint();
}

void DAWClockController::BackendAudioRenderer::callUpdateCallback(bool isFinished, double progress)
{
	parent.exportProgress = progress;
            
	SafeAsyncCall::repaint(&parent);

	if(isFinished)
	{
		int numChannels = this->channels.size();
		auto numSamples = this->channels.getFirst()->size;

		buffer.setSize(numChannels, numSamples);

		for(int i = 0; i < numChannels; i++)
			FloatVectorOperations::copy(buffer.getWritePointer(i), this->channels[i]->buffer.getReadPointer(0), numSamples);

		triggerAsyncUpdate();
	}
}


struct AudioTimelineObject : public TimelineObjectBase
{
	AudioTimelineObject(const File& f) :
		TimelineObjectBase(f)
	{};

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("Audio"); }

	Colour getColour() const override
	{
		return Colour(EFFECT_PROCESSOR_COLOUR);
	}

	void initialise(double sampleRate) override
	{
		AudioFormatManager afm;
		afm.registerBasicFormats();
		
		ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(f);

		if(reader != nullptr)
		{
			content.setSize(2, (int)reader->lengthInSamples);

			reader->read(&content, 0, (int)reader->lengthInSamples, 0, true, true);

			if (sampleRate != reader->sampleRate)
			{
				LagrangeInterpolator interpolators[2];
				auto ratio = sampleRate / reader->sampleRate;

				AudioSampleBuffer newBuffer(2, reader->lengthInSamples * ratio);
				
				interpolators[0].process(1.0 / ratio, content.getReadPointer(0), newBuffer.getWritePointer(0), newBuffer.getNumSamples());
				interpolators[1].process(1.0 / ratio, content.getReadPointer(1), newBuffer.getWritePointer(1), newBuffer.getNumSamples());

				std::swap(content, newBuffer);
			}

		}
		reader = nullptr;
	}

	void draw(Graphics& g, Rectangle<float> bounds) override
	{
		if (bounds != lastArea)
			rebuildPeaks(bounds);

		g.setColour(Colours::white.withAlpha(0.4f));
		g.fillRectList(peaks);
	}

	Result addEventsForBouncing(HiseEventBuffer& , ExternalClockSimulator*) override
	{
		return Result::fail("Can't bounce audio clips. Use MIDI clips for this feature");
	}

	void process(AudioSampleBuffer& buffer, MidiBuffer& mb, double ppqOffsetFromStart, ExternalClockSimulator* clock) override
	{
		auto sampleOffset = clock->getSamplesDelta(ppqOffsetFromStart);

		if (sampleOffset < 0)
		{
			int numToCopy = buffer.getNumSamples() + sampleOffset;

			auto targetOffset = -sampleOffset;
			auto contentOffset = 0;

			if (numToCopy > 0)
			{
				FloatVectorOperations::copy(buffer.getWritePointer(0, targetOffset), content.getReadPointer(0, contentOffset), numToCopy);
				FloatVectorOperations::copy(buffer.getWritePointer(1, targetOffset), content.getReadPointer(1, contentOffset), numToCopy);
			}

			
		}
		else
		{
			auto contentOffset = sampleOffset;
			auto targetOffset = 0;
			auto numToCopy = jmin(buffer.getNumSamples(), content.getNumSamples() - contentOffset);

			if (numToCopy > 0)
			{
				FloatVectorOperations::copy(buffer.getWritePointer(0, targetOffset), content.getReadPointer(0, contentOffset), numToCopy);
				FloatVectorOperations::copy(buffer.getWritePointer(1, targetOffset), content.getReadPointer(1, contentOffset), numToCopy);
			}
		}
	}

	void rebuildPeaks(Rectangle<float> bounds)
	{
		int samplesPerPixel = roundToInt(content.getNumSamples() / bounds.getWidth());

		float x = 0.0f;

		peaks.clear();

		for (int i = 0; i < content.getNumSamples(); i += samplesPerPixel)
		{
			int numToDo = jmin(samplesPerPixel, content.getNumSamples() - i);
			float mag = content.getMagnitude(i, numToDo);

			float h = mag * bounds.getHeight();
			float y = (bounds.getHeight() - h) * 0.5f;

			peaks.addWithoutMerging({ x, y, 1.0f, h });
			x += 1.0f;
		}

		lastArea = bounds;


	}

	double getPPQLength(double sampleRate, double bpm) const override
	{
		auto numSamples = content.getNumSamples();
		auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);
		return (double)numSamples / samplesPerQuarter;
	}

	AudioSampleBuffer content;

	RectangleList<float> peaks;
	Rectangle<float> lastArea;
};

struct MidiTimelineObject : public TimelineObjectBase,
							public ControlledObject,
						    public TempoListener
{
	MidiTimelineObject(const File& f, MainController* mc) :
		TimelineObjectBase(f),
		ControlledObject(mc)
	{
		getMainController()->addTempoListener(this);
	};

	~MidiTimelineObject()
	{
		if (auto mc = getMainController())
		{
			mc->allNotesOff();
			mc->removeTempoListener(this);
		}
	}

	void onTransportChange(bool isPlaying, double ppqPosition) override
	{
		if (!isPlaying)
		{
			getMainController()->allNotesOff();
		}
	}

	void loopWrap() override
	{
		clearOnNextBuffer = true;
	}

	void onResync(double ppqPosition) override
	{
		clearOnNextBuffer = true;
	}

	Result addEventsForBouncing(HiseEventBuffer& b, ExternalClockSimulator* clock) override;

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("Midi"); }

	void initialise(double ) override
	{
		FileInputStream fis(f);
        
        content.readFrom(fis);

		auto scaleFactor = 960.0 / jmax(1.0, (double)content.getTimeFormat());

		content.setTicksPerQuarterNote(960);

		if(auto t = content.getTrack(0))
		{
			for(auto e: *t)
			{
				e->message.setTimeStamp(e->message.getTimeStamp() * scaleFactor);
			}
		}
	}

	Colour getColour() const override
	{
		return Colour(MIDI_PROCESSOR_COLOUR);
	}

	void draw(Graphics& g, Rectangle<float> bounds) override
	{
		if (bounds != lastBounds)
			rebuildEvents(bounds);
		
		g.setColour(Colours::white.withAlpha(0.4f));
		g.fillRectList(midiEvents);

	}

	double getPPQLength(double sampleRate, double bpm) const override
	{
		return (double)content.getLastTimestamp() / (double)content.getTimeFormat();
	}

	void process(AudioSampleBuffer& buffer, MidiBuffer& mb, double ppqOffsetFromStart, ExternalClockSimulator* clock) override
	{
		if (clearOnNextBuffer)
		{
			for (auto e : pendingNoteOffs)
			{
				mb.addEvent(e->message, 0);
			}

			pendingNoteOffs.clear();
			clearOnNextBuffer = false;
		}

		if (auto t = content.getTrack(0))
		{
			auto ticksPerQuarter = (double)content.getTimeFormat();

			auto idx = t->getNextIndexAtTime(ppqOffsetFromStart * ticksPerQuarter);
			auto ppqDelta = clock->getPPQDelta(buffer.getNumSamples() + 1);

			Range<double> timestampRange(ppqOffsetFromStart * ticksPerQuarter, (ppqOffsetFromStart + ppqDelta) * ticksPerQuarter);

			for (int i = idx; i < t->getNumEvents(); i++)
			{
				auto e = t->getEventPointer(i);

				auto ts = e->message.getTimeStamp();

				if (timestampRange.contains(ts))
				{
					auto timestampPPQ = e->message.getTimeStamp() / ticksPerQuarter;
					auto timestampSamples = clock->getSamplesDelta(timestampPPQ - ppqOffsetFromStart);

					mb.addEvent(e->message, timestampSamples);

					if (e->message.isNoteOff())
						pendingNoteOffs.remove(e);

					if (e->noteOffObject != nullptr)
					{
						pendingNoteOffs.insert(e->noteOffObject);
					}
				}
				else
					break;
			}

		}
	}

	void rebuildEvents(Rectangle<float> bounds)
	{
		lastBounds = bounds;

		midiEvents.clear();

		if (auto s = content.getTrack(0))
		{
			int max_ = 0;
			int min_ = 128;

			for (auto e : *s)
			{
				max_ = jmax(max_, e->message.getNoteNumber());
				min_ = jmin(min_, e->message.getNoteNumber());
			}
			
			auto numNotes = (float)(max_ - min_);

			if (numNotes == 0.0f)
				return;

			for (auto e : *s)
			{
				if (e->message.isNoteOn() && e->noteOffObject != nullptr)
				{
					auto nn = e->message.getNoteNumber();
					nn -= min_;

					auto x = (float)(e->message.getTimeStamp() / content.getLastTimestamp());
					auto w = (float)(e->noteOffObject->message.getTimeStamp() / content.getLastTimestamp()) - x;

					if (x >= 1.0)
						break;

					x *= bounds.getWidth();
					w *= bounds.getWidth();

					auto y = (float)(numNotes - 1 - nn) / numNotes * bounds.getHeight();
					auto h = jmax(1.0f, bounds.getHeight() / numNotes);

					midiEvents.addWithoutMerging({ x, y, w, h });
				}
			}
		}
	}

	MidiFile content;
	RectangleList<float> midiEvents;

	hise::UnorderedStack<MidiMessageSequence::MidiEventHolder*> pendingNoteOffs;
	bool clearOnNextBuffer;

	Rectangle<float> lastBounds;
};

Result MidiTimelineObject::addEventsForBouncing(HiseEventBuffer& b, ExternalClockSimulator* clock)
{
	if (auto t = content.getTrack(0))
	{
		auto ticksPerQuarter = (double)content.getTimeFormat();

		auto idx = 0;
			
		for (int i = idx; i < t->getNumEvents(); i++)
		{
			auto e = t->getEventPointer(i);

			const auto timestampPPQ = e->message.getTimeStamp() / ticksPerQuarter;
			const auto timestampSamples = clock->getSamplesDelta(timestampPPQ);

            if(b.size() == HISE_EVENT_BUFFER_SIZE-1)
                return Result::ok();
            
            HiseEvent he(e->message);
            he.setTimeStamp(timestampSamples);
            
            if(!he.isEmpty())
                b.addEvent(he);
		}
	}
    
    return Result::ok();
}


struct DAWClockController::Ruler: public Component,
                                  public ControlledObject,
                                  public juce::FileDragAndDropTarget
{
	struct DraggableObject: public Component
	{
		

		void updatePosition(double ppq, int pixelPos)
		{
			data->startPPQ = ppq;
			setTopLeftPosition(pixelPos, LoopHeight);
		}

		void paint(Graphics& g) override
		{
			auto c = data->getColour();

			auto b = getLocalBounds().toFloat();

			g.setColour(c.withAlpha(0.6f));
			g.fillRoundedRectangle(b, 4.0f);

			g.drawRoundedRectangle(b, 4.0f, 2.0f);
			g.setFont(GLOBAL_BOLD_FONT());

			data->draw(g, b);
		}

		DraggableObject(TimelineObjectBase::Ptr obj):
			data(obj)
		{
			Component::setInterceptsMouseClicks(false, false);
		}

		TimelineObjectBase::Ptr data;
	};

    static constexpr int LoopHeight = 17;
    
    Ruler(ExternalClockSimulator* clock_, MainController* mc):
	  ControlledObject(mc),
      clock(clock_)
    {
        setOpaque(true);

		if (getTimelineFile().existsAsFile())
		{
			auto xml = XmlDocument::parse(getTimelineFile());

			auto v = ValueTree::fromXml(*xml);

			clock->isLooping = (bool)v["Loop"];
			clock->ppqLoop = { (double)v["LoopStart"], (double)v["LoopEnd"] };

			numBars = jmax(1, (int)v["NumBars"]);
			grid = (bool)v["Grid"];

			for (auto c : v)
			{
				auto f_ = c["File"];
				auto p = c["StartPosition"];

				File f(f_);

				if (f.existsAsFile())
				{
					auto obj = getOrCreate(f);
					obj->startPPQ = p;
				}
			}
		}

		for (auto o : clock->timelineObjects)
		{
			auto newObj = new DraggableObject(o);
            
            updatePosition(newObj);
            
			addAndMakeVisible(newObj);
			existingObjects.add(newObj);
		}
    };
    
	bool isInterestedInFileDrag(const StringArray& files) override 
	{ 
		auto type = TimelineObjectBase::getTypeFromFile(File(files[0]));

		return type != TimelineObjectBase::Type::Unknown;
	}

	OwnedArray<DraggableObject> existingObjects;

	ScopedPointer<DraggableObject> currentObject;

	TimelineObjectBase::Ptr getOrCreate(const File& f)
	{
		for (auto to : clock->timelineObjects)
		{
			if (to->f == f)
				return to;
		}

		auto type = TimelineObjectBase::getTypeFromFile(f);

		TimelineObjectBase::Ptr newObj;

		if (type == TimelineObjectBase::Type::Audio)
			newObj = new AudioTimelineObject(f);
		else
			newObj = new MidiTimelineObject(f, getMainController());

		newObj->initialise(clock->sampleRate);

		clock->timelineObjects.add(newObj);

		return newObj;
		
	}

	virtual void fileDragEnter(const StringArray& files, int x, int y)
	{
		auto f = File(files[0]);

		auto newObj = getOrCreate(f);

		newObj->startPPQ = pixelToPPQ(x);

		addAndMakeVisible(currentObject = new DraggableObject(newObj));

		updatePosition(currentObject);

		Component::setMouseCursor(MouseCursor::StandardCursorType::CopyingCursor);
	}

	void updatePosition(DraggableObject* d)
	{
		auto ppqLength = d->data->getPPQLength(clock->sampleRate, clock->bpm);
		auto x = (int)PPQToPixel(d->data->startPPQ);
		d->setBounds(x, LoopHeight, PPQToPixel(ppqLength), getHeight() - LoopHeight);
	}

	virtual void fileDragMove(const StringArray& files, int x, int y)
	{
		auto ppq = pixelToPPQ(x);
		
		if (currentObject != nullptr)
			currentObject->updatePosition(ppq, PPQToPixel(ppq));	
	};

	virtual void fileDragExit(const StringArray& files)
	{
		currentObject = nullptr;
		setMouseCursor(MouseCursor::NormalCursor);
	}

	
	virtual void filesDropped(const StringArray& files, int x, int y)
	{
		existingObjects.add(currentObject.release());

		setMouseCursor(MouseCursor::NormalCursor);
	}

    void setPositionFromEvent(const MouseEvent& e)
    {
        if(e.getPosition().getY() > LoopHeight)
        {
            clock->ppqPos = pixelToPPQ(e.getPosition().getX());
        }
        else
        {
            auto thisPos = pixelToPPQ(e.getPosition().getX());
            
            auto distToStart = hmath::abs(clock->ppqLoop.getStart() - thisPos);
            auto distToEnd = hmath::abs(clock->ppqLoop.getEnd() - thisPos);
            
            if(distToStart < distToEnd && thisPos < clock->ppqLoop.getEnd())
                clock->ppqLoop.setStart(thisPos);
            else
                clock->ppqLoop.setEnd(thisPos);
        }
    }

	void mouseDoubleClick(const MouseEvent& e) override
	{
		currentObject = nullptr;
		existingObjects.clear();
		ScopedLock sl(clock->lock);
		clock->timelineObjects.clear();
	}
    
    void mouseDrag(const MouseEvent& e) override
    {
        setPositionFromEvent(e);
    }
    
	File getTimelineFile() const
	{
		return ProjectHandler::getAppDataDirectory(nullptr).getChildFile("Timeline.xml");
	}

    void mouseDown(const MouseEvent& e) override
    {
		if (e.mods.isRightButtonDown())
		{
			PopupMenu m;
			PopupLookAndFeel plaf;
			m.setLookAndFeel(&plaf);

			auto mc = getMainController();

			static constexpr int SyncOffset = 9000;

			m.addSectionHeader("Sync Mode");

#define ADD_SYNC_MODE(x) m.addItem(SyncOffset + (int)MasterClock::SyncModes::x, #x, true, mc->getMasterClock().getSyncMode() == MasterClock::SyncModes::x);

			ADD_SYNC_MODE(Inactive);
			ADD_SYNC_MODE(ExternalOnly);
			ADD_SYNC_MODE(InternalOnly);
			ADD_SYNC_MODE(PreferExternal);
			ADD_SYNC_MODE(PreferInternal);

#undef ADD_SYNC_MODE

			m.addSeparator();

			m.addItem(1, "Clear all objects", !clock->timelineObjects.isEmpty(), false);
			m.addItem(2, "Save timelime as default", true, false);
			m.addItem(3, "Reset default timeline", getTimelineFile().existsAsFile(), false);

			auto result = m.show();

			if (result == 1)
			{
				
				existingObjects.clear();
				currentObject = nullptr;

				ScopedLock sl(clock->lock);
				clock->timelineObjects.clear();
			}
			if (result == 2)
			{
				ValueTree v("Timeline");

				v.setProperty("Loop", clock->isLooping, nullptr);
				v.setProperty("LoopStart", clock->ppqLoop.getStart(), nullptr);
				v.setProperty("LoopEnd", clock->ppqLoop.getEnd(), nullptr);
				v.setProperty("NumBars", numBars, nullptr);
				v.setProperty("Grid", grid, nullptr);

				for (auto to : existingObjects)
				{
					auto f = to->data->f.getFullPathName();
					auto startPos = to->data->startPPQ;

					ValueTree c("Object");
					c.setProperty("File", f, nullptr);
					c.setProperty("StartPosition", startPos, nullptr);

					v.addChild(c, -1, nullptr);
				}

				getTimelineFile().replaceWithText(v.createXml()->createDocument(""));
			}
			if (result == 3)
			{
				getTimelineFile().deleteFile();
			}

			if (result >= SyncOffset)
			{
				auto newMode = (MasterClock::SyncModes)(result - SyncOffset);

				mc->getMasterClock().setSyncMode(newMode);
			}

			return;
		}

        setPositionFromEvent(e);
    }
    
    float PPQToPixel(double ppqPos) const
    {
        float numQuarters = numBars * clock->nom;
        
        return (float)ppqPos / (float)numQuarters * (float)getWidth();
    }
    
    float pixelToPPQ(int xPos) const
    {
        auto xNormalized = (double)xPos / (double)getWidth();
        
        float numQuarters = numBars * clock->nom;
        
        auto v = xNormalized * numQuarters;
        
        if(grid)
            v = hmath::round(v);
        
        return jmax<float>(0.0f, v);
    }
    
	void resized() override
	{
		if (currentObject != nullptr)
			updatePosition(currentObject);

		for (auto d : existingObjects)
			updatePosition(d);
	}

	void setNumBars(int newValue)
	{
		numBars = newValue;
		resized();
	}

    void paint(Graphics& g) override
    {
        g.setGradientFill(ColourGradient(Colour(0xFF303030), 0.0f, 0.0f, Colour(0xFF262626), 0.0f, (float)getHeight(), false));
        g.fillAll();
        auto b = getLocalBounds().toFloat();
        
        auto top = b.removeFromTop((float)LoopHeight);
        
		if (existingObjects.isEmpty())
		{
			g.setColour(Colours::white.withAlpha(0.2f));

			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Drop audio or MIDI files here", b, Justification::centred);
		}

        g.setColour(Colours::white.withAlpha(0.1f));
        g.fillRect(top);
        g.setColour(Colour(0xFF555555));
        g.drawRect(b, 1.0f);
        
        g.setColour(Colours::black.withAlpha(0.6f));
        g.drawHorizontalLine(b.getY() + 1, 1.0f, (float)getWidth() - 1.0f);
        
        float barWidth = (float)getWidth() / (float)numBars;
        float beatWidth = barWidth / (float)clock->nom;
        
        float numQuarters = numBars * clock->nom;
        
        auto rulerPos = (float)PPQToPixel(clock->ppqPos);
        
        g.setColour(Colours::white.withAlpha(clock->isPlaying ? 1.0f : 0.7f));
        g.fillRect(rulerPos-0.5f, b.getY()+2, 2.0f, b.getHeight()-4);
        
        g.setColour(Colours::white.withAlpha(clock->isPlaying ? 0.2f : 0.1f));
        g.fillRect(rulerPos-3.5f, b.getY()+2, 8.0f, b.getHeight()-4);
        
        for(int i = 0; i < numQuarters; i++)
        {
            auto x = b.getX();
            
            g.setColour(Colour(0xFF555555).withAlpha(i % clock->nom == 0 ? 0.7f : 0.2f));
            b.removeFromLeft(beatWidth);
            g.drawVerticalLine(x, 0, b.getBottom());
        }
        
        DAWClockController::Icons f;
        
        auto ls = f.createPath("loopStart");
        auto le = f.createPath("loopEnd");
        
        auto lsX = PPQToPixel(clock->ppqLoop.getStart());
        auto leX = PPQToPixel(clock->ppqLoop.getEnd());
        
        f.scalePath(ls, top.withWidth(LoopHeight).withX(lsX - LoopHeight + 1.0f));
        f.scalePath(le, top.withWidth(LoopHeight).withX(leX - 1.0f));
        
        
        
        g.setColour(Colours::white.withAlpha(clock->isLooping ? 0.8f : 0.3f));
        
        g.fillPath(ls);
        g.fillPath(le);
        
        if(clock->isLooping)
        {
            g.setColour(Colours::white.withAlpha(0.05f));
            g.fillRect(lsX, b.getY(), leX - lsX, b.getHeight());
        }
    }
    
    
    bool grid = true;
    int numBars = 8;
    
    WeakReference<ExternalClockSimulator> clock;
};



DAWClockController::DAWClockController(MainController* mc):
  SimpleTimer(mc->getGlobalUIUpdater()),
  ControlledObject(mc),
  clock(&dynamic_cast<BackendProcessor*>(mc)->externalClockSim),
  bypass("bypass", nullptr, f),
  play("play", nullptr, f),
  stop("stop", nullptr, f),
  loop("loop", nullptr, f),
  grid("grid", nullptr, f),
  rewind("rewind", nullptr, f),
  exportButton("export", nullptr, f),
  metronome("metronome", nullptr, f)
{
	addAndMakeVisible(bypass);
    addAndMakeVisible(play);
    addAndMakeVisible(stop);
    addAndMakeVisible(rewind);
    addAndMakeVisible(loop);
    addAndMakeVisible(bpm);
    addAndMakeVisible(nom);
    addAndMakeVisible(denom);
	addAndMakeVisible(metronome);
    addAndMakeVisible(position);
	addAndMakeVisible(exportButton);
    addAndMakeVisible(ruler = new Ruler(clock, mc));
    
    addAndMakeVisible(grid);
    addAndMakeVisible(length);

	bypass.setToggleModeWithColourChange(true);
    play.setToggleModeWithColourChange(true);
    stop.setToggleModeWithColourChange(true);
    loop.setToggleModeWithColourChange(true);
    grid.setToggleModeWithColourChange(true);
	metronome.setToggleModeWithColourChange(true);
    
    denom.setName("denom");
    
    auto setupSlider = [this](Slider& s)
    {
        s.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
        s.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
        s.setLookAndFeel(&laf);
        s.addListener(this);
    };
    
    nom.setRange(1.0, 16.0, 1.0);
    denom.setRange(1, 16.0, 1.0);
    bpm.setRange(30.0, 240.0, 1.0);
    length.setRange(1, 128.0, 1.0);
    
    length.setValue(dynamic_cast<Ruler*>(ruler.get())->numBars, dontSendNotification);

	bypass.onClick = [this]()
	{
		clock->bypassed = !clock->bypassed;
	};

    play.onClick = [this]()
    {
        clock->isPlaying = true;
    };
    
    stop.onClick = [this]()
    {
        clock->isPlaying = false;
    };

	metronome.onClick = [this]()
	{
		clock->metronomeEnabled = !clock->metronomeEnabled;
	};
    
    loop.onClick = [this]()
    {
        clock->isLooping = loop.getToggleState();
    };
    
    rewind.onClick = [this]()
    {
        clock->ppqPos = 0.0;
    };
    
    grid.onClick = [this]()
    {
        dynamic_cast<Ruler*>(ruler.get())->grid = grid.getToggleState();
    };

	exportButton.onClick = [this]()
	{
        exporter = new BackendAudioRenderer(*this);
        exporter->init();
	};

    grid.setToggleStateAndUpdateIcon(true);
    
    setupSlider(length);
    setupSlider(bpm);
    setupSlider(nom);
    setupSlider(denom);
    
    position.setEditable(false);
    position.setFont(GLOBAL_BOLD_FONT().withHeight(17.0f));
    position.setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.5f));

	bypass.setTooltip("Simulate the bypass state of the plugin");
    play.setTooltip("Start the external DAW playback simulator [Space]");
    stop.setTooltip("Stop the external DAW playback simulator [Space]");
    loop.setTooltip("Toggle the loop playback");
    bpm.setTooltip("Set the external DAW tempo");
    rewind.setTooltip("Rewind to 1|1|0 [Backspace]");
    grid.setTooltip("Enable the magnetic grid for the playback ruler");
    length.setTooltip("Set the length of the playback ruler");
	exportButton.setTooltip("Bounce the MIDI clips as audio to simulate offline bounce process");
	metronome.setTooltip("Enable a metronome that plays a click on each beat");
}

bool DAWClockController::keyPressed(const KeyPress& k)
{
    if(k == KeyPress::spaceKey)
    {
        clock->isPlaying = !clock->isPlaying;
        return true;
    }
    if(k == KeyPress::backspaceKey)
    {
        rewind.triggerClick();
        return true;
    }
    if(k == KeyPress::leftKey)
    {
        clock->ppqPos = jmax(0.0, clock->ppqPos - 1.0f);
        return true;
    }
    if(k == KeyPress::rightKey)
    {
        clock->ppqPos += 1.0;
        return true;
    }
    
    
    return false;
}

void DAWClockController::sliderValueChanged(Slider* s)
{
    if(s == &bpm)
    {
        clock->bpm = roundToInt(s->getValue());
        //getMainController()->setHostBpm(clock->bpm);
		ruler->resized();
    }
    if(s == &nom)
        clock->nom = roundToInt(s->getValue());
    if(s == &denom)
        clock->denom = nextPowerOfTwo(roundToInt(s->getValue()));
    if(s == &length)

        dynamic_cast<Ruler*>(ruler.get())->setNumBars((int)s->getValue());
}

void DAWClockController::timerCallback()
{
    if(!bpm.isMouseButtonDown())
        bpm.setValue(clock->bpm, dontSendNotification);
    
    if(!nom.isMouseButtonDown())
        nom.setValue(clock->nom, dontSendNotification);
    
    if(!denom.isMouseButtonDown())
        denom.setValue(clock->denom, dontSendNotification);
    
    auto pos = clock->ppqPos;
    
    auto ticks = roundToInt(hmath::fmod(pos, 1.0) * 960.0);
    auto quarters = (int)hmath::floor(hmath::fmod(pos, (double)clock->denom));
    auto bars = (int)hmath::floor(pos / clock->denom);
    
    String posString;
    posString << String(bars+1) << " | " << String(quarters+1) << " | " << String(ticks);
    
    position.setText(posString, dontSendNotification);

	
	bypass.setToggleStateAndUpdateIcon(!clock->bypassed);
    play.setToggleStateAndUpdateIcon(clock->isPlaying);
    stop.setToggleStateAndUpdateIcon(!clock->isPlaying);
    loop.setToggleStateAndUpdateIcon(clock->isLooping);
	metronome.setToggleStateAndUpdateIcon(clock->metronomeEnabled);
    
    ruler->repaint();
}

void DAWClockController::paint(Graphics& g)
{
    if(exportProgress != -1.0)
    {
		auto tb = exportButton.getBoundsInParent().toFloat().expanded(3.0f);

        tb = tb.translated(-1.5f, 1.0f);
        
		Path p;
        p.startNewSubPath(tb.getCentreX(), tb.getY());
        p.addArc(tb.getX(), tb.getY(), tb.getWidth(), tb.getHeight(), 0.0, exportProgress * float_Pi * 2.0f);
        g.setColour(Colours::white.withAlpha(0.3f));
        g.strokePath(p, PathStrokeType(2.0f));
    }
}

void DAWClockController::resized()
{
    auto b = getLocalBounds();
    
    static constexpr int TopHeight = 28;
    static constexpr int Margin = 5;
    
    auto top = b.removeFromTop(TopHeight);
    
    top.removeFromLeft(TopHeight);
    
    b.removeFromTop(5);

	bypass.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    play.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    stop.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    rewind.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    loop.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
	metronome.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    top.removeFromLeft(Margin);
    exportButton.setBounds(top.removeFromLeft(TopHeight).reduced(Margin));
    top.removeFromLeft(Margin);

    bpm.setBounds(top.removeFromLeft(TopHeight * 2).reduced(Margin));
    
    auto ts = top.removeFromLeft(TopHeight);
    
    nom.setBounds(ts.removeFromTop(ts.getHeight()/2));
    denom.setBounds(ts);
    
    position.setBounds(top.removeFromLeft(TopHeight * 4));
    

    
    auto r = b.removeFromLeft(TopHeight);
    
    length.setBounds(r.removeFromTop(Ruler::LoopHeight));
    
    grid.setBounds(r.reduced(Margin));
    
    b.removeFromLeft(10);
    ruler->setBounds(b);
}
#endif


juce::Image PoolTableHelpers::getPreviewImage(const AudioSampleBuffer* buffer, float width)
{
	if (buffer == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	return HiseAudioThumbnail::createPreview(buffer, (int)width);
}

juce::Image PoolTableHelpers::getPreviewImage(const Image* img, float width)
{
	if (img == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	auto ratio = (float)img->getWidth() / (float)img->getHeight();

	if (img->getWidth() > width)
	{
		return img->rescaled((int)width, (int)(width / ratio));
	}
	else
	{
		if (img->getHeight() < 1600)
		{
			int heightToUse = jmin<int>(500, img->getHeight());

			return img->rescaled((int)((float)heightToUse * ratio), heightToUse);
		}
		else
		{
			// most likely a filmstrip, so crop it to show the first two strips...
			return img->getClippedImage({ 0, 0, img->getWidth(), img->getWidth() * 2 });

		}
	}
}

juce::Image PoolTableHelpers::getPreviewImage(const ValueTree* v, float width)
{
	if (v == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	Array<Rectangle<int>> zones;

	auto totalArea = Rectangle<int>(0, 0, (int)width, 128);

	for (const auto& data : *v)
	{
		auto d = StreamingHelpers::getBasicMappingDataFromSample(data);

		int x = jmap((int)d.lowKey, 0, 128, 0, totalArea.getWidth());
		int w = jmap((int)(1 + d.highKey - d.lowKey), 0, 128, 0, totalArea.getWidth());
		int y = jmap((int)d.highVelocity, 128, 0, 0, totalArea.getHeight());
		int h = jmap((int)(1 + d.highVelocity - d.lowVelocity), 0, 128, 0, totalArea.getHeight() - 1);

		zones.add({ x, y, w, h });
	}

	Image img(Image::ARGB, (int)width, 128, true);

	Graphics g(img);

	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRect(totalArea, 1);

	for (auto z : zones)
	{
		g.fillRect(z);
		g.drawRect(z);
	}

	return img;
}

juce::Image PoolTableHelpers::getPreviewImage(const MidiFileReference* v, float width)
{
	auto f = v->getFile();

#if 0

	MemoryOutputStream mos;
	f.writeTo(mos);

	MemoryBlock mb = mos.getMemoryBlock();
	MemoryInputStream mis(mb, true);
#endif

	HiseMidiSequence seq;
	seq.loadFrom(f);

	auto l = seq.getRectangleList({ 0.0f, 0.0f, width, 200.0f });

	Image img(Image::PixelFormat::ARGB, (int)width, 200, true);
	Graphics g(img);

	g.setColour(Colours::white);

	for (auto note : l)
		g.fillRect(note);

	return img;
}

juce::Path PoolTableHelpers::Factory::createPath(const String& name) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(name);

	Path p;

	LOAD_EPATH_IF_URL("preview", HiBinaryData::FrontendBinaryData::infoButtonShape);
	LOAD_PATH_IF_URL("reload", ColumnIcons::moveIcon);

	return p;
}

#if USE_BACKEND
namespace ClockIcons
{
static const unsigned char play[] = { 110,109,48,200,67,68,112,182,151,67,98,184,219,68,68,40,55,153,67,4,128,69,68,60,173,155,67,4,128,69,68,240,76,158,67,98,4,128,69,68,180,238,160,67,184,219,68,68,184,98,163,67,48,200,67,68,132,229,164,67,98,192,23,56,68,196,67,181,67,224,196,25,68,160,
189,223,67,72,37,13,68,216,110,241,67,98,142,12,12,68,220,247,242,67,140,158,10,68,176,39,243,67,230,109,9,68,156,235,241,67,98,74,62,8,68,132,175,240,67,4,128,7,68,124,59,238,67,4,128,7,68,64,141,235,67,98,4,128,7,68,140,67,201,67,4,128,7,68,112,9,97,
67,4,128,7,68,0,107,31,67,98,4,128,7,68,160,147,26,67,148,43,8,68,88,40,22,67,18,62,9,68,136,238,19,67,98,136,79,10,68,152,176,17,67,46,154,11,68,240,7,18,67,224,151,12,68,32,207,20,67,98,0,221,24,68,176,47,55,67,20,236,55,68,184,23,135,67,48,200,67,
68,112,182,151,67,99,101,0,0 };

static const unsigned char stop[] = { 110,109,0,128,69,68,4,15,89,67,98,0,128,69,68,236,58,75,67,70,177,66,68,0,0,64,67,62,60,63,68,0,0,64,67,108,118,196,13,68,0,0,64,67,98,112,79,10,68,0,0,64,67,0,128,7,68,236,58,75,67,0,128,7,68,4,15,89,67,108,0,128,7,68,18,119,207,67,98,0,128,7,68,32,
97,214,67,112,79,10,68,0,0,220,67,118,196,13,68,0,0,220,67,108,62,60,63,68,0,0,220,67,98,70,177,66,68,0,0,220,67,0,128,69,68,32,97,214,67,0,128,69,68,18,119,207,67,108,0,128,69,68,4,15,89,67,99,101,0,0 };

static const unsigned char loop[] = { 110,109,100,113,22,68,130,145,190,67,98,28,151,26,68,168,120,198,67,24,27,32,68,82,230,202,67,244,217,37,68,82,230,202,67,98,252,8,50,68,82,230,202,67,44,249,59,68,146,94,183,67,172,47,60,68,134,0,159,67,108,32,85,69,68,234,81,159,67,98,28,8,69,68,158,
169,193,67,136,6,55,68,56,49,221,67,244,217,37,68,56,49,221,67,98,196,173,29,68,56,49,221,67,224,213,21,68,76,216,214,67,128,248,15,68,214,129,203,67,108,216,218,7,68,152,190,219,67,108,0,128,7,68,146,118,172,67,108,76,35,31,68,64,44,173,67,108,100,113,
22,68,130,145,190,67,99,109,104,196,54,68,248,165,121,67,98,32,168,50,68,104,154,106,67,192,71,45,68,92,51,98,67,104,175,39,68,92,51,98,67,98,168,127,27,68,92,51,98,67,116,143,17,68,224,162,132,67,244,88,17,68,238,0,157,67,108,132,51,8,68,22,174,156,
67,98,140,128,8,68,196,172,116,67,28,130,22,68,144,157,61,67,104,175,39,68,144,157,61,67,98,92,180,47,68,144,157,61,67,92,105,55,68,36,219,73,67,72,61,61,68,104,194,95,67,108,44,37,69,68,228,34,64,67,108,0,128,69,68,120,89,143,67,108,184,220,45,68,88,
162,142,67,108,104,196,54,68,248,165,121,67,99,101,0,0 };

static const unsigned char grid[] = { 110,109,96,140,22,68,64,118,86,67,108,96,140,22,68,144,107,68,67,108,120,41,17,68,144,107,68,67,108,120,41,17,68,240,165,102,67,108,248,127,7,68,240,165,102,67,108,248,127,7,68,136,49,124,67,108,120,41,17,68,136,49,124,67,108,120,41,17,68,24,157,152,
67,108,216,154,8,68,24,157,152,67,108,216,154,8,68,228,98,163,67,108,120,41,17,68,228,98,163,67,108,120,41,17,68,52,231,189,67,108,216,154,8,68,52,231,189,67,108,216,154,8,68,0,173,200,67,108,120,41,17,68,0,173,200,67,108,120,41,17,68,0,0,220,67,108,
96,140,22,68,0,0,220,67,108,96,140,22,68,12,173,204,67,108,152,123,22,68,12,173,204,67,108,152,123,22,68,64,118,86,67,108,96,140,22,68,64,118,86,67,99,109,104,49,41,68,168,178,85,67,108,104,49,41,68,240,255,63,67,108,128,206,35,68,240,255,63,67,108,128,
206,35,68,240,165,102,67,108,80,22,24,68,240,165,102,67,108,80,22,24,68,136,49,124,67,108,128,206,35,68,136,49,124,67,108,128,206,35,68,24,157,152,67,108,80,22,24,68,24,157,152,67,108,80,22,24,68,228,98,163,67,108,128,206,35,68,228,98,163,67,108,128,
206,35,68,52,231,189,67,108,80,22,24,68,52,231,189,67,108,80,22,24,68,0,173,200,67,108,128,206,35,68,0,173,200,67,108,128,206,35,68,48,202,217,67,108,104,49,41,68,48,202,217,67,108,104,49,41,68,220,73,204,67,108,184,48,41,68,220,73,204,67,108,184,48,
41,68,168,178,85,67,108,104,49,41,68,168,178,85,67,99,109,40,204,42,68,240,165,102,67,108,40,204,42,68,136,49,124,67,108,144,115,54,68,136,49,124,67,108,144,115,54,68,24,157,152,67,108,40,204,42,68,24,157,152,67,108,40,204,42,68,228,98,163,67,108,144,
115,54,68,228,98,163,67,108,144,115,54,68,52,231,189,67,108,40,204,42,68,52,231,189,67,108,40,204,42,68,0,173,200,67,108,144,115,54,68,0,173,200,67,108,144,115,54,68,48,202,217,67,108,120,214,59,68,48,202,217,67,108,120,214,59,68,0,173,200,67,108,96,
219,59,68,0,173,200,67,108,96,219,59,68,52,231,189,67,108,120,214,59,68,52,231,189,67,108,120,214,59,68,228,98,163,67,108,96,219,59,68,228,98,163,67,108,96,219,59,68,24,157,152,67,108,120,214,59,68,24,157,152,67,108,120,214,59,68,136,49,124,67,108,96,
219,59,68,136,49,124,67,108,96,219,59,68,240,165,102,67,108,120,214,59,68,240,165,102,67,108,120,214,59,68,240,255,63,67,108,144,115,54,68,240,255,63,67,108,144,115,54,68,240,165,102,67,108,40,204,42,68,240,165,102,67,99,109,32,118,61,68,52,231,189,67,
108,32,118,61,68,0,173,200,67,108,248,127,69,68,0,173,200,67,108,248,127,69,68,52,231,189,67,108,32,118,61,68,52,231,189,67,99,109,32,118,61,68,24,157,152,67,108,32,118,61,68,228,98,163,67,108,248,127,69,68,228,98,163,67,108,248,127,69,68,24,157,152,
67,108,32,118,61,68,24,157,152,67,99,109,32,118,61,68,240,165,102,67,108,32,118,61,68,136,49,124,67,108,8,101,68,68,136,49,124,67,108,8,101,68,68,240,165,102,67,108,32,118,61,68,240,165,102,67,99,101,0,0 };

static const unsigned char loopEnd[] = { 110,109,0,128,7,68,64,40,54,67,108,0,128,69,68,128,42,58,67,108,0,111,8,68,224,235,224,67,108,0,128,7,68,64,40,54,67,99,101,0,0 };

static const unsigned char loopStart[] = { 110,109,0,128,69,68,160,37,54,67,108,0,128,7,68,64,45,58,67,108,248,144,68,68,64,237,224,67,108,0,128,69,68,160,37,54,67,99,101,0,0 };


static const unsigned char metronome[] = { 110,109,164,19,27,68,112,185,121,67,98,164,19,27,68,112,185,121,67,200,80,30,68,16,244,85,67,40,88,33,68,24,130,52,67,98,20,33,34,68,248,210,43,67,32,49,36,68,104,10,38,67,0,128,38,68,104,10,38,67,98,222,206,40,68,104,10,38,67,234,222,42,68,248,210,43,
67,212,167,43,68,24,130,52,67,98,234,120,52,68,204,240,138,67,0,128,69,68,208,250,232,67,0,128,69,68,208,250,232,67,108,0,128,7,68,208,250,232,67,108,194,73,22,68,244,79,151,67,108,32,151,26,68,84,139,173,67,108,146,169,18,68,128,81,217,67,98,44,161,
33,68,128,81,217,67,220,85,58,68,128,81,217,67,220,85,58,68,128,81,217,67,108,0,128,38,68,80,137,87,67,108,4,97,31,68,20,24,147,67,108,164,19,27,68,112,185,121,67,99,109,252,119,23,68,232,162,108,67,98,44,140,24,68,208,113,104,67,58,56,25,68,152,146,
98,67,58,56,25,68,240,20,92,67,98,58,56,25,68,240,88,79,67,12,162,22,68,56,0,69,67,12,115,19,68,56,0,69,67,98,126,67,16,68,56,0,69,67,78,173,13,68,240,88,79,67,78,173,13,68,240,20,92,67,98,78,173,13,68,208,113,104,67,112,28,16,68,48,139,114,67,122,41,
19,68,216,34,115,67,108,228,102,36,68,168,167,210,67,108,178,86,41,68,168,167,210,67,108,252,119,23,68,232,162,108,67,99,101,0,0 };

static const unsigned char rewind[] = { 110,109,212,254,24,68,202,91,153,67,98,48,52,24,68,204,119,154,67,232,186,23,68,240,72,156,67,232,186,23,68,206,56,158,67,98,232,186,23,68,50,42,160,67,48,52,24,68,208,249,161,67,212,254,24,68,90,23,163,67,98,236,159,33,68,176,44,175,67,172,2,56,68,10,
136,206,67,18,85,65,68,158,151,219,67,98,82,36,66,68,194,185,220,67,128,50,67,68,18,221,220,67,162,18,68,68,186,243,219,67,98,138,243,68,68,98,10,219,67,0,128,69,68,194,58,217,67,0,128,69,68,38,64,215,67,98,0,128,69,68,66,240,189,67,0,128,69,68,76,217,
120,67,0,128,69,68,76,104,72,67,98,0,128,69,68,88,213,68,67,92,1,69,68,56,146,65,67,182,54,68,68,148,237,63,67,98,20,108,67,68,224,69,62,67,250,119,66,68,92,134,62,67,118,189,65,68,96,147,64,67,98,156,174,56,68,40,244,89,67,238,192,33,68,218,22,141,67,
212,254,24,68,202,91,153,67,99,109,150,181,19,68,192,76,64,67,98,150,181,19,68,200,71,63,67,100,129,19,68,236,115,62,67,38,64,19,68,236,115,62,67,108,112,245,7,68,236,115,62,67,98,246,180,7,68,236,115,62,67,0,128,7,68,200,71,63,67,0,128,7,68,192,76,64,
67,108,0,128,7,68,102,195,217,67,98,0,128,7,68,90,68,218,67,246,180,7,68,70,174,218,67,112,245,7,68,70,174,218,67,108,38,64,19,68,70,174,218,67,98,100,129,19,68,70,174,218,67,150,181,19,68,90,68,218,67,150,181,19,68,102,195,217,67,108,150,181,19,68,192,
76,64,67,99,101,0,0 };

}

Path DAWClockController::Icons::createPath(const String& url) const
{
    Path p;
    
    LOAD_PATH_IF_URL("play", ClockIcons::play);
    LOAD_PATH_IF_URL("stop", ClockIcons::stop);
    LOAD_PATH_IF_URL("loop", ClockIcons::loop);
    LOAD_PATH_IF_URL("grid", ClockIcons::grid);
    LOAD_PATH_IF_URL("loopStart", ClockIcons::loopStart);
    LOAD_PATH_IF_URL("loopEnd", ClockIcons::loopEnd);
    LOAD_PATH_IF_URL("rewind", ClockIcons::rewind);
	LOAD_PATH_IF_URL("metronome", ClockIcons::metronome);
	LOAD_EPATH_IF_URL("export", HnodeIcons::exportIcon);
	LOAD_EPATH_IF_URL("bypass", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);

    return p;
}

TimelineObjectBase::Type TimelineObjectBase::getTypeFromFile(const File& f)
{
	if (f.getFileExtension() == ".wav" ||
		f.getFileExtension() == ".aif")
	{
		return Type::Audio;
	}
	if (f.getFileExtension() == ".mid" ||
		f.getFileExtension() == ".midi")
	{
		return Type::Midi;
	}

	return Type::Unknown;
}

void TimelineMetronome::process(AudioSampleBuffer& buffer, MidiBuffer& mb, double ppqOffsetFromStart,
                                ExternalClockSimulator* clock)
{
	if (!enabled)
		return;

	Range hiRange(0.0, 1.0 / (double)clock->nom);


	auto normPos = hmath::fmod(clock->ppqPos, (double)clock->nom) * ((double)clock->nom / (double)clock->denom);

	auto useHi = hiRange.contains(normPos);
	

	auto& bufferToUse = useHi ? hiClick : loClick;

	

	auto sampleOffset = clock->getSamplesDelta(ppqOffsetFromStart);

	

	if (sampleOffset < 0)
	{
		int numToCopy = bufferToUse.getNumSamples() + sampleOffset;
		
		auto targetOffset = -sampleOffset;
		auto contentOffset = 0;

		numToCopy = jmin(numToCopy, buffer.getNumSamples() - targetOffset);

		if (numToCopy > 0)
		{
			FloatVectorOperations::add(buffer.getWritePointer(0, targetOffset), bufferToUse.getReadPointer(0, contentOffset), numToCopy);
			FloatVectorOperations::add(buffer.getWritePointer(1, targetOffset), bufferToUse.getReadPointer(1, contentOffset), numToCopy);
		}
	}
	else
	{
		auto contentOffset = sampleOffset;
		auto targetOffset = 0;
		auto numToCopy = jmin(buffer.getNumSamples(), bufferToUse.getNumSamples() - contentOffset);

		if (numToCopy > 0)
		{
			FloatVectorOperations::add(buffer.getWritePointer(0, targetOffset), bufferToUse.getReadPointer(0, contentOffset), numToCopy);
			FloatVectorOperations::add(buffer.getWritePointer(1, targetOffset), bufferToUse.getReadPointer(1, contentOffset), numToCopy);
		}
	}

	
}

void TimelineMetronome::initialise(double sampleRate)
{
	auto numSamples = sampleRate * 0.4;

	auto cycleLength = sampleRate / 220.0;
		
	auto delta = hmath::PI * 2.0 / cycleLength;

	hiClick.setSize(2, numSamples);
	loClick.setSize(2, numSamples);

	double uptime = 0.0;
	auto env = 1.0f;

	for (int i = 0; i < numSamples; i++)
	{
		auto hiSample = 0.5f * (float)hmath::sin(uptime * 2.0) + 0.1f * (hmath::random() * 2.0f - 1.0f);
		auto loSample = 0.5f * (float)hmath::sin(uptime) + 0.1f * (hmath::random() * 2.0f - 1.0f);

		hiSample *= env;
		loSample *= env;

		env *= 0.998f;

		hiClick.setSample(0, i, hiSample);
		hiClick.setSample(1, i, hiSample);
		loClick.setSample(0, i, loSample);
		loClick.setSample(1, i, loSample);

		uptime += delta;
	}
}

ExternalClockSimulator::ExternalClockSimulator()
{
	metronome = new TimelineMetronome();
}

double ExternalClockSimulator::getPPQDelta(int numSamples) const
{
	auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);
        
	return (double)numSamples / samplesPerQuarter;
}

int ExternalClockSimulator::getSamplesDelta(double ppqDelta) const
{
	auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);
	return roundToInt(samplesPerQuarter * ppqDelta);
}

void ExternalClockSimulator::sendLoopMessage()
{
	if(metronome != nullptr)
		metronome->loopWrap();

	for (auto to : timelineObjects)
		to->loopWrap();
}

void ExternalClockSimulator::addTimelineData(AudioSampleBuffer& bufferData, MidiBuffer& mb)
{
	if (!isPlaying)
		return;

	auto thisPPQ = getPPQDelta(bufferData.getNumSamples());

	Range<double> thisRange(ppqPos, ppqPos + thisPPQ);

	ScopedLock sl(lock);

	

	for (auto to : timelineObjects)
	{
		auto l = to->getPPQLength(sampleRate, bpm);

		Range<double> toRange(to->startPPQ, to->startPPQ + l);

		if (!toRange.getIntersectionWith(thisRange).isEmpty())
		{
			auto offset = ppqPos - to->startPPQ;
			to->process(bufferData, mb, offset, this);
		}
	}
}

void ExternalClockSimulator::addPostTimelineData(AudioSampleBuffer& bufferData, MidiBuffer& mb)
{
	if (!isPlaying || !metronomeEnabled)
		return;

	auto normedPPQ = hmath::fmod(ppqPos + 0.5, 1.0) - 0.5;

	auto thisPPQ = getPPQDelta(bufferData.getNumSamples());

	Range thisRange(normedPPQ, normedPPQ + thisPPQ);

	auto l = metronome->getPPQLength(sampleRate, bpm);

	Range<double> floorRange(0.0, l);
	
	if (!floorRange.getIntersectionWith(thisRange).isEmpty())
	{
		auto offset = normedPPQ - thisPPQ;
		metronome->process(bufferData, mb, offset, this);
	}
}

void ExternalClockSimulator::process(int numSamples)
{
	if (bpm == -1.0)
		bpm = 120.0;

	if(isPlaying)
	{
		auto ppqDelta = getPPQDelta(numSamples);
            
		ppqPos += ppqDelta;
            
		if(isLooping && !ppqLoop.isEmpty() && ppqPos > ppqLoop.getEnd())
		{
			auto posAfterStart = ppqPos - ppqLoop.getStart();
			ppqPos = ppqLoop.getStart() + hmath::fmod(posAfterStart, ppqLoop.getLength());
		}
	}
}

int ExternalClockSimulator::getLoopBeforeWrap(int numSamples)
{
	if(isPlaying && isLooping && !ppqLoop.isEmpty())
	{
		auto beforeInside = ppqLoop.contains(ppqPos);
            
		if(!beforeInside)
			return 0;
            
		auto afterPos = ppqPos + getPPQDelta(numSamples);
            
		auto afterInside = ppqLoop.contains(afterPos);
            
		if(!afterInside)
		{
			return getSamplesDelta(afterPos - ppqLoop.getEnd());
		}
            
		return 0;
	}
        
	return 0;
}

bool ExternalClockSimulator::getCurrentPosition(CurrentPositionInfo& result)
{
	result.bpm = bpm;
	result.timeSigNumerator = nom;
	result.timeSigDenominator = denom;
	result.timeInSamples = TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter) * ppqPos;
	result.timeInSeconds = TempoSyncer::getTempoInMilliSeconds(bpm, TempoSyncer::Quarter) * ppqPos;
	result.ppqPosition = ppqPos;
	result.ppqPositionOfLastBarStart = hmath::floor(ppqPos / 4.0) * 4.0;
	result.isPlaying = isPlaying;
	result.isRecording = false;
	result.ppqLoopStart = ppqLoop.getStart();
	result.ppqLoopEnd = ppqLoop.getEnd();
	result.isLooping = isLooping;
        
	return true;
}

void ExternalClockSimulator::prepareToPlay(double newSampleRate)
{
	sampleRate = newSampleRate;

	metronome->initialise(newSampleRate);

	for (auto o : timelineObjects)
		o->initialise(newSampleRate);
}
#endif



}
