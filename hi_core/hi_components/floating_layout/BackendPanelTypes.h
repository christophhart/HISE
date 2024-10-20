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
#ifndef BACKENDPANELTYPES_H_INCLUDED
#define BACKENDPANELTYPES_H_INCLUDED

namespace hise { using namespace juce;

#if USE_BACKEND

class ExternalClockSimulator;

struct TimelineObjectBase : public ReferenceCountedObject
{
	enum class Type
	{
		Unknown,
		Midi,
		Audio,
		Metronome,
		numTypes
	};

	static Type getTypeFromFile(const File& f);

	using Ptr = ReferenceCountedObjectPtr<TimelineObjectBase>;

	virtual ~TimelineObjectBase() {};

	virtual Identifier getTypeId() const = 0;

	virtual Result addEventsForBouncing(HiseEventBuffer& b, ExternalClockSimulator* clock) = 0;

	virtual void process(AudioSampleBuffer& buffer, MidiBuffer& mb, double ppqOffsetFromStart, ExternalClockSimulator* clock) { jassertfalse; }

	virtual void draw(Graphics& g, Rectangle<float> bounds) = 0;

	virtual void initialise(double sampleRate) = 0;

	virtual void loopWrap() {};

	virtual double getPPQLength(double sampleRate, double bpm) const = 0;

	virtual Colour getColour() const = 0;

	double startPPQ = 0.0;
	const Type type;
	File f;

	TimelineObjectBase(const File& f_) :
		type(getTypeFromFile(f_)),
		f(f_)
	{};

};

struct TimelineMetronome : public TimelineObjectBase
{
	TimelineMetronome() :
		TimelineObjectBase(File())
	{}

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("Metronome"); }

	void process(AudioSampleBuffer& buffer, MidiBuffer& mb, double ppqOffsetFromStart,
		ExternalClockSimulator* clock) override;

	void draw(Graphics& g, Rectangle<float> bounds) override {}

	void initialise(double sampleRate) override;

	Colour getColour() const override { return Colours::transparentBlack; }

	Result addEventsForBouncing(HiseEventBuffer& b, ExternalClockSimulator* clock) override { return Result::ok(); }
	
	double getPPQLength(double sampleRate, double bpm) const override
	{
		auto numSamples = loClick.getNumSamples();
		auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);
		return (double)numSamples / samplesPerQuarter;
	}

	bool enabled = true;

	AudioSampleBuffer loClick, hiClick;
};

class ExternalClockSimulator: public juce::AudioPlayHead
{
public:
    
	ExternalClockSimulator();

	double getPPQDelta(int numSamples) const;

	int getSamplesDelta(double ppqDelta) const;

	void sendLoopMessage();

	void addTimelineData(AudioSampleBuffer& bufferData, MidiBuffer& mb);

	void addPostTimelineData(AudioSampleBuffer& bufferData, MidiBuffer& mb);
	
	void process(int numSamples);

	int getLoopBeforeWrap(int numSamples);

	bool getCurrentPosition (CurrentPositionInfo& result) override;

	void prepareToPlay(double newSampleRate);

	HiseEventBuffer getHiseEventBufferForBounce()
	{
        
		HiseEventBuffer allEvents;

        if(timelineObjects.isEmpty())
            throw Result::fail("no MIDI clips to render.  Make sure to add a MIDI clip before trying to bounce audio.");
        
		for(auto to : timelineObjects)
		{
			auto ok = to->addEventsForBouncing(allEvents, this);

			if(ok.failed())
				throw ok;
			//if(auto mb = dynamic_cast<)
		}

		return allEvents;
	}

	bool bypassed = false;

	bool isLooping = false;
    bool isPlaying = false;
	bool metronomeEnabled = false;
    
    int nom = 4;
    int denom = 4;
    
    double bpm = 120.0;
    double ppqPos = 0.0;
    Range<double> ppqLoop = { 8.0, 16.0 };
    
    double sampleRate;

	TimelineObjectBase::Ptr metronome;

	ReferenceCountedArray<TimelineObjectBase> timelineObjects;

	CriticalSection lock;

    JUCE_DECLARE_WEAK_REFERENCEABLE(ExternalClockSimulator);
};

struct DAWClockController: public Component,
                           public ControlledObject,
                           public PooledUIUpdater::SimpleTimer,
                           public Slider::Listener
{
    struct Icons: public PathFactory
    {
        Path createPath(const String& url) const override;
    };
    
    DAWClockController(MainController* mc);
      
    virtual ~DAWClockController()
    {
	    exporter = nullptr;
    };
    
    bool keyPressed(const KeyPress& k) override;
    
    void timerCallback() override;
    
    void resized() override;
    
    void paint(Graphics& g) override;
    
    void sliderValueChanged(Slider* s) override;
    
    struct LAF: public LookAndFeel_V3
    {
        void drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;
    } laf;

	struct BackendAudioRenderer: public AudioRendererBase,
                                 public AsyncUpdater
	{
		BackendAudioRenderer(DAWClockController& parent_):
		  AudioRendererBase(parent_.getMainController()),
		  parent(parent_)
		{
			
		};

		DAWClockController& parent;

        AudioSampleBuffer buffer;
        
		bool init()
		{
            this->skipCallbacks = false;
            this->sendArtificialTransportMessages = true;
            
			try
			{
				eventBuffers.add(new HiseEventBuffer(parent.clock->getHiseEventBufferForBounce()));
				initAfterFillingEventBuffer();
				return true;
			}
			catch(Result& r)
			{
				PresetHandler::showMessageWindow("Export failed", r.getErrorMessage(), PresetHandler::IconType::Error);
				return false;
			}
		}

        void handleAsyncUpdate() override;

		void callUpdateCallback(bool isFinished, double progress) override;
	};

	float exportProgress = -1.0f;

	ScopedPointer<BackendAudioRenderer> exporter;

    WeakReference<ExternalClockSimulator> clock;
    Icons f;
    
    HiseShapeButton bypass, play, stop, loop, grid, rewind, metronome, exportButton;
    
    Slider bpm, nom, denom, length;
    
    Label position;
    
    
    
    struct Ruler;
    
    ScopedPointer<Component> ruler;
};
#endif

class ApplicationCommandButtonPanel : public FloatingTileContent,
	public Component
{
public:


	ApplicationCommandButtonPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		addAndMakeVisible(b = new ShapeButton("Icon", Colours::white.withAlpha(0.3f), Colours::white.withAlpha(0.5f), Colours::white));

		b->setVisible(false);
	}

	SET_PANEL_NAME("Icon");

	void resized() override
	{
		b->setBounds(getLocalBounds());
	}

	void setCommand(int commandID);

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

private:

	ScopedPointer<ShapeButton> b;
};

struct PoolTableHelpers
{
	class Factory : public PathFactory
	{
		String getId() const override { return "Pool Table Icons"; }

		Path createPath(const String& name) const override;
	};

	static Image getPreviewImage(const AudioSampleBuffer* buffer, float width);
	static Image getPreviewImage(const Image* img, float width);
	static Image getPreviewImage(const ValueTree* v, float width);
	static Image getPreviewImage(const MidiFileReference* v, float width);
};



class MainController;

/** A table component containing all samples of a ModulatorSampler.
*	@ingroup components
*/
template <class DataType> class ExternalFileTableBase : public Component,
														public FloatingTileContent,
public TableListBoxModel,
public PoolBase::Listener,
public DragAndDropContainer,
public ButtonListener,
public ExpansionHandler::Listener
{
public:

	enum ColumnId
	{
		FileName = 1,
		Memory,
		References,
		numColumns
	};

	class CustomSnapshotTableListBox : public TableListBox
	{
	public:

		CustomSnapshotTableListBox(ExternalFileTableBase& parent_) :
			parent(parent_)
		{}

		ScaledImage createSnapshotOfRows(const SparseSet<int>& rows, int& imageX, int& imageY) override
		{
			imageX = 256;
			imageY = 128;

			if (auto ref = parent.pool->getReference(rows.getTotalRange().getStart()))
			{
				auto m = parent.pool->getWeakReferenceToItem(ref);
				return ScaledImage(PoolTableHelpers::getPreviewImage(m.getData(), 256));
			}
			else
			{
				return ScaledImage(PoolHelpers::getEmptyImage(imageX, imageY));
			}
		}

		ExternalFileTableBase& parent;
	};

	class PreviewComponent : public Component
	{
	public:

		class TypedImageProvider : public MarkdownParser::ImageProvider
		{
		public:

			TypedImageProvider(MarkdownParser* parent, PoolEntry<DataType>* d) :
				ImageProvider(parent),
				data(d)
			{};

			Image getImage(const MarkdownLink& /*imageURL*/, float width) override
			{
				if (data == nullptr)
					return Image();

				return PoolTableHelpers::getPreviewImage(&data->data, width);
			}

			ImageProvider* clone(MarkdownParser* newParser) const override
			{
				return new TypedImageProvider(newParser, data);
			}

			Identifier getId() const override { RETURN_STATIC_IDENTIFIER("PoolPreviewImageProvider"); };

			MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Autogenerated; }

		private:

			WeakReference<PoolEntry<DataType>> data;
		};

		PreviewComponent(PoolEntry<DataType>* d) :
			data(d),
			p(getMarkdownDescription())
		{
			p.setDefaultTextSize(14.0f);
			p.setTextColour(Colours::white);

			auto ip = new TypedImageProvider(&p, data.get());

			p.setImageProvider(ip);

			p.parse();

			setSize(256, (int)p.getHeightForWidth(256.0f) + 20);
		}

		void paint(Graphics& g) override
		{
			p.draw(g, getLocalBounds().reduced(10).toFloat());
		}

		String getMarkdownDescription()
		{
			if (data == nullptr)
				return {};

			String s;

			String nl = "  \n";

			s << "### File" << nl;

			if (data->ref.isEmbeddedReference())
				s << "**File:** " << "Embedded" << nl;
			else
				s << "**File:** " << data->ref.getFile().getFullPathName() << nl;

			s << "**Reference:** `" << data->ref.getReferenceString() << "`" << nl;
			s << "**Hashcode:** " << data->ref.getHashCode() << nl;

			var metadata = data->additionalData;

			if (auto md = metadata.getDynamicObject())
			{
				s << "### Metadata" << nl;

				for (const auto& v : md->getProperties())
				{
					s << "**" << v.name.toString() << "**: " << v.value.toString() << nl;
				}
			}

			s << "### Preview" << nl;

			s << "![preview](/images/preview)" << nl;

			return s;
		}

		WeakReference < PoolEntry<DataType>> data;
		MarkdownRenderer p;
	};

	// ========================================================================================================= ExternalFileTable
	ExternalFileTableBase(FloatingTile* parent) :
		FloatingTileContent(parent),
		table(*this),
		font(GLOBAL_FONT()),
		selectedRow(-1),
		previewButton("Preview", this, factory),
		reloadButton("Reload", this, factory)
	{
		addAndMakeVisible(&previewButton);

		getMainController()->getExpansionHandler().addListener(this);

		// Create our table component and add it to this component..
		addAndMakeVisible(table);
		table.setModel(this);

		laf = new TableHeaderLookAndFeel();

		table.getHeader().setLookAndFeel(laf);

		table.getHeader().setSize(getWidth(), 22);

		// give it a border
		table.setColour(ListBox::outlineColourId, Colours::grey);
		table.setColour(ListBox::backgroundColourId, HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));

		table.setOutlineThickness(0);

		table.getViewport()->setScrollBarsShown(true, false, false, false);

		//table.getHeader().setInterceptsMouseClicks(false, false);

		table.getHeader().addColumn("File Name", FileName, 60);
		table.getHeader().addColumn("Size", Memory, 50);
		table.getHeader().addColumn("References", References, 50);

		updatePool();
	}

	~ExternalFileTableBase()
	{
		getMainController()->getExpansionHandler().removeListener(this);

		pool->removeListener(this);
	}

	void expansionPackLoaded(Expansion* /*currentExpansion*/) override
	{
		updatePool();
	}

	void updatePool()
	{
		if (pool != nullptr)
		{
			pool->removeListener(this);
		}

		if (auto e = getMainController()->getExpansionHandler().getCurrentExpansion())
			pool = e->pool->template getPool<DataType>();
		else
			pool = getMainController()->getCurrentFileHandler().pool->template getPool<DataType>();

		pool->addListener(this);

		table.updateContent();

	}

	void buttonClicked(Button* /*b*/) override
	{

	}

	static Identifier getPanelId()
	{
		DataType* d = nullptr;

		return PoolHelpers::getPrettyName(d);
	}

	Identifier getIdentifierForBaseClass() const override
	{
		return getPanelId();
	}

	void poolEntryAdded() override
	{
		poolEntryRemoved();
	}

	void poolEntryRemoved() override
	{
		table.updateContent();
		repaint();
	}

	void poolEntryChanged(PoolReference referenceThatWasChanged)
	{
		poolEntryRemoved();
	}

	int getNumRows() override
	{
		return pool != nullptr ? pool->getNumLoadedFiles() : 0;
	}

	void paintRowBackground(Graphics& g, int rowNumber, int, int, bool rowIsSelected) override
	{
		if (rowNumber % 2) g.fillAll(Colours::white.withAlpha(0.05f));

		if (rowIsSelected)
			g.fillAll(Colour(0x44000000));
	}

	void selectedRowsChanged(int i) override
	{
		selectedRow = i;
	}

	void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool) override
	{
		g.setColour(Colours::white.withAlpha(.8f));
		g.setFont(font);

		String text = getTextForTableCell(rowNumber, columnId);

		g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);

		//g.setColour (Colours::black.withAlpha (0.2f));
		//g.fillRect (width - 1, 0, 1, height);
	}

	void resized() override
	{
		auto area = getParentContentBounds();

		auto top = area.removeFromTop(32).reduced(4);

		previewButton.setBounds(top.removeFromRight(28));
		reloadButton.setBounds(top.removeFromRight(28));

		table.setBounds(area);

		table.getHeader().setColumnWidth(FileName, getWidth() - 100);
		table.getHeader().setColumnWidth(Memory, 50);
		table.getHeader().setColumnWidth(References, 50);
	}

	void paint(Graphics& g)
	{
		auto area = getParentContentBounds();

		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());
		auto top = area.removeFromTop(32).reduced(4);
		g.drawText(pool->getStatistics(), top, Justification::left);
	}

	void cellDoubleClicked(int rowNumber, int /*columnId*/, const MouseEvent&)
	{
		if (pool == nullptr)
			return;

		auto mc = pool->getMainController();

		if (auto editor = mc->getLastActiveEditor())
		{
			auto ref = pool->getReference(rowNumber);

			if (ref.isValid())
			{
				CommonEditorFunctions::insertTextAtCaret(editor, ref.getReferenceString());
			}
		}
	}

	void cellClicked(int rowNumber, int /*columnId*/, const MouseEvent& e) override
	{
		if (e.mods.isRightButtonDown())
		{
			enum class MenuItems
			{
				Properties = 1,
				ShowInFolder,
				LoadAllData,
				ReloadFromDisk,
				numMenuItems
			};

			PopupMenu m;
			m.setLookAndFeel(&plaf);

			m.addItem((int)MenuItems::Properties, "Properties");

#if JUCE_WINDOWS
			m.addItem((int)MenuItems::ShowInFolder, "Show in Explorer");
#else
			m.addItem((int)MenuItems::ShowInFolder, "Show in Finder");
#endif

			m.addItem((int)MenuItems::ReloadFromDisk, "Reload File");

			DataType* d = nullptr;

			m.addItem((int)MenuItems::LoadAllData, "Load all " + PoolHelpers::getPrettyName(d) + " into pool");

			MenuItems result = (MenuItems)m.show();

			switch (result)
			{
			case MenuItems::Properties:
			{
				auto ref = pool.get()->getReference(rowNumber);
				auto mod = pool.get()->getWeakReferenceToItem(ref);
				auto pc = new PreviewComponent(mod.get());
				auto b = table.getRowPosition(rowNumber, true);
				auto b2 = table.getScreenPosition();
				Rectangle<int> b3({ b.getX() + b2.getX(), b.getY() + b2.getY(), b.getWidth(), b.getHeight() });

				CallOutBox::launchAsynchronously(std::unique_ptr<Component>(pc), b3, nullptr);
				break;
			}
			case MenuItems::LoadAllData:
			{
				pool->loadAllFilesFromProjectFolder();
				break;
			}
			case MenuItems::ShowInFolder:
			{
				auto ref = pool.get()->getReference(rowNumber);
				ref.getFile().revealToUser();
				break;
			}
			case MenuItems::ReloadFromDisk:
			{
				auto ref = pool.get()->getReference(rowNumber);

				pool->loadFromReference(ref, PoolHelpers::ForceReloadWeak);

			}
			case MenuItems::numMenuItems:
			default:
				break;
			}
		}


	}

	String getTextForTableCell(int rowNumber, int columnNumber)
	{
		if (pool == nullptr)
			return {};

		StringArray info = pool->getTextDataForId(rowNumber);

		if ((columnNumber - 1) < info.size())
		{
			return info[columnNumber - 1];
		}
		else
			return {};
	}

	var getDragSourceDescription(const SparseSet< int > &set) override
	{
		if (pool == nullptr)
			return {};

		if (set.getNumRanges() > 0)
		{
			int index = set.getTotalRange().getStart();

			return pool->getReference(index).createDragDescription();
		}

		return var();
	};

protected:


	WeakReference<SharedPoolBase<DataType>> pool;



private:

	PopupLookAndFeel plaf;

	CustomSnapshotTableListBox table;     // the table component itself

	PoolTableHelpers::Factory factory;

	HiseShapeButton reloadButton;
	HiseShapeButton previewButton;



	Font font;

	int selectedRow;

	var currentlyDraggedId;

	ScopedPointer<TableHeaderLookAndFeel> laf;

	int numRows;            // The number of rows of data we've got

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExternalFileTableBase)
};

struct PoolTableSubTypes
{
	using AudioFilePoolTable = ExternalFileTableBase<AudioSampleBuffer>;
	using ImageFilePoolTable = ExternalFileTableBase<Image>;
	using SampleMapPoolTable = ExternalFileTableBase<ValueTree>;
	using MidiFilePoolTable = ExternalFileTableBase<MidiFileReference>;
};

} // namespace hise

#endif  // BACKENDPANELTYPES_H_INCLUDED
