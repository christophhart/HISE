/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
using namespace juce;

namespace ui
{

struct OptimizationProperties : public WorkbenchComponent
{
	struct Item : public Component
	{
		Item(const String& id_) :
			id(id_)
		{

		}

		void paint(Graphics& g) override
		{
			if (active)
			{
				g.fillAll(Colours::white.withAlpha(0.2f));
			}

			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);
			g.drawText(id, getLocalBounds().toFloat(), Justification::centred);
		}

		void mouseDown(const MouseEvent& e) override
		{
			active = !active;

			findParentComponentOfClass<OptimizationProperties>()->resetOptimisations();
			repaint();
		}

		bool active = true;
		String id;
	};

	OptimizationProperties(WorkbenchData* data) :
		WorkbenchComponent(data)
	{
		data->addListener(this);

		for (auto o : OptimizationIds::getAllIds())
			addOptimization(o);

		recompiled(getWorkbench());

		setSize(200, items.size() * 20);
	}

	~OptimizationProperties()
	{
		getWorkbench()->removeListener(this);
	}

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("Optimisations"); };

	int getFixedHeight() const override
	{
		return items.size() * 20;
	}

	void recompiled(WorkbenchData::Ptr r) override
	{
		for (auto i : items)
			i->active = getGlobalScope().getOptimizationPassList().contains(i->id);
	}

	void resized() override
	{
		auto b = getLocalBounds();

		for (auto i : items)
			i->setBounds(b.removeFromTop(20));
	}

	void resetOptimisations();

	void addOptimization(const String& id)
	{
		auto i = new Item(id);
		addAndMakeVisible(i);
		items.add(i);
	}

	OwnedArray<Item> items;
};

struct TestDataComponentBase : public WorkbenchComponent,
	public ButtonListener,
	public ComboBoxListener,
	public WorkbenchData::TestData::TestListener
{
	TestDataComponentBase(WorkbenchData::Ptr data) :
		WorkbenchComponent(data)
	{
		data->addListener(this);
		data->getTestData().addListener(this);
	};

	~TestDataComponentBase()
	{
		if (auto wb = getWorkbench())
		{
			wb->removeListener(this);
			wb->getTestData().removeListener(this);
		}
	}

	struct Icons : public PathFactory
	{
		Path createPath(const String& p) const override;

		String getId() const override { return "GraphPaths"; };
	};

	HiseShapeButton* addButton(const String& name, const String& offName = {})
	{
		auto c = new HiseShapeButton(name, this, iconFactory, offName);

		if (offName.isNotEmpty())
			c->setToggleModeWithColourChange(true);

		addAndMakeVisible(c);
		buttons.add(c);
		return c;
	}

	TextButton* addTextButton(const String& n)
	{
		auto c = new TextButton(n);

		c->setClickingTogglesState(true);
		c->setLookAndFeel(&blaf);
		addAndMakeVisible(c);
		c->addListener(this);
		buttons.add(c);

		return c;
	}

	ComboBox* addComboBox(const StringArray& items)
	{
		auto cb = new ComboBox();

		cb->setColour(HiseColourScheme::ComponentBackgroundColour, Colours::transparentBlack);
		cb->setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
		cb->setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
		cb->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
		cb->setColour(HiseColourScheme::ComponentTextColourId, Colours::white);

		cb->addItemList(items, 1);
		cb->setLookAndFeel(&plaf);
		cb->addListener(this);
		cb->setSelectedItemIndex(0, dontSendNotification);

		addAndMakeVisible(cb);
		buttons.add(cb);

		return cb;
	}

	void addSpacer()
	{
		Component* c = new Component();

		addAndMakeVisible(c);
		buttons.add(c);
	}

	void resized()
	{
		auto b = getLocalBounds();
		auto bh = 24;
		auto buttonRow = b.removeFromTop(bh);

		for (auto bt : buttons)
		{
			auto w = bh;

			if (auto tb = dynamic_cast<TextButton*>(bt))
			{
				w = tb->getBestWidthForHeight(bh) + 10;
			}
			else if (auto sb = dynamic_cast<HiseShapeButton*>(bt))
				w = bh;
			else if (auto cb = dynamic_cast<ComboBox*>(bt))
				w = 128;
			else
				w = 10;

			bt->setBounds(buttonRow.removeFromLeft(w).reduced(2));
			buttonRow.removeFromLeft(5);
		}
	}

	void setTestBuffer(NotificationType triggerRecompilation = dontSendNotification)
	{
		auto wb = getWorkbench();

		wb->getTestData().rebuildTestSignal();

		if (triggerRecompilation != dontSendNotification)
			wb->triggerPostCompileActions();
	}

	hise::PopupLookAndFeel plaf;
	BlackTextButtonLookAndFeel blaf;
	Icons iconFactory;

	OwnedArray<Component> buttons;

	JitCompiledNode::Ptr currentNode;

	
};


struct Graph : public TestDataComponentBase
{
	enum class MouseMode
	{
		Scroll,
		Zoom,
		numMouseModes
	};

	enum class GraphType
	{
		Signal,
		FFT,
		Spectrograph,
		numGraphTypes
	};

	struct Helpers
	{
		enum WindowType
		{
			Rectangle,
			Hamming,
			Hann,
			BlackmanHarris,
			Triangle,
			FlatTop,
			numWindowType
		};

		Array<WindowType> getAvailableWindowTypes()
		{
			return { Hamming, Hann, BlackmanHarris, Triangle, FlatTop };
		}

		static String getWindowType(WindowType w)
		{
			switch (w)
			{
			case Rectangle: return "Rectangle";
			case Hamming: return "Hamming";
			case Hann: return "Hann";
			case BlackmanHarris: return "Blackman Harris";
			case Triangle: return "Triangle";
			case FlatTop: return "FlatTop";
			default: return {};
			}
		}

		static void applyWindow(WindowType t, AudioSampleBuffer& b);
	};

	struct InternalGraph : public Component,
		public Timer,
		public TooltipClient
	{

		InternalGraph(Graph& parent_) :
			parent(parent_)
		{};

		void paint(Graphics& g) override;

		void drawTestEvent(Graphics& g, bool isParameter, int index);



		void timerCallback() override
		{
			stopTimer();
			repaint();
		}

		void setBuffer(const AudioSampleBuffer& b);

		GraphType getCurrentGraphType() const;

		void calculatePath(Path& p, const AudioSampleBuffer& b, int channel);

		void resizePath();

		void mouseMove(const MouseEvent& e) override
		{
			currentPoint = e.getPosition();
			getTooltip();
			startTimer(1200);
			repaint();
		}

		String getTooltip() override;

		float getYPosition(float level) const;

		float getXPosition(float normalisedIndex) const;

		void rebuildSpectrumRectangles();

		void mouseExit(const MouseEvent&) override
		{
			stopTimer();
			currentPoint = {};
			repaint();
		}

		void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override
		{
			if (e.mods.isAnyModifierKeyDown())
			{
				zoomFactor = jlimit(1.0f, 32.0f, zoomFactor + (float)wheel.deltaY * 5.0f);
				findParentComponentOfClass<Graph>()->resized();
				setBuffer(lastBuffer);
			}
			else
				getParentComponent()->mouseWheelMove(e, wheel);
		}

		bool isHiresMode() const
		{
			return pixelsPerSample > 10.0f;
		}

		int getYPixelForSample(int sample)
		{
			auto x = (float)getPixelForSample(sample);
			Line<float> line(x, 0.0f, x, (float)getHeight());
			return roundToInt(l.getClippedLine(line, true).getEndY());
		}

		int getPixelForSample(int sample)
		{
			if (lastBuffer.getNumSamples() == 0)
				return 0;

			Path::Iterator iter(l);

			auto asFloat = (float)sample / (float)lastBuffer.getNumSamples();
			asFloat *= (float)getWidth();

			while (iter.next())
			{
				if (iter.x1 >= asFloat)
					return roundToInt(iter.x1);
			}

			return 0;
		}

		Graph& parent;

		AudioSampleBuffer lastBuffer;

		float pixelsPerSample = 1;

		Point<int> currentPoint;

		int numSamples = 0;
		int currentPosition = 0;
		Path l;
		Path r;

		Image spectroImage;

		Range<float> leftPeaks;
		Range<float> rightPeaks;
		bool stereoMode = false;
		
		float zoomFactor = 1.0f;

	} internalGraph;

	Viewport viewport;

	Graph(WorkbenchData* data) :
		TestDataComponentBase(data),
		currentResult(Result::ok()),
		internalGraph(*this)
	{
		processButton = addButton("process", "bypass");
		addSpacer();
		graphType = addComboBox({ "Time Domain", "FFT Spectrum", "2D Spectrum" });
		logScaleButton = addTextButton("Log Freq");
		logPeakButton = addTextButton("Log Peak");

		processButton->setToggleStateAndUpdateIcon(true);

		StringArray sa;

		for (int i = 0; i < Helpers::WindowType::numWindowType; i++)
		{
			sa.add(Helpers::getWindowType((Helpers::WindowType)i));
		}

		windowType = addComboBox(sa);
		
		markerButton = addButton("markers", "markers");

		markerButton->setToggleStateAndUpdateIcon(true);

		addAndMakeVisible(viewport);
		viewport.setViewedComponent(&internalGraph, false);

		updateFFTComponents();
	}

	void updateFFTComponents()
	{
		auto shouldBeEnabled = currentGraphType != GraphType::Signal;

		windowType->setEnabled(shouldBeEnabled);
		logPeakButton->setEnabled(shouldBeEnabled);
		logScaleButton->setEnabled(shouldBeEnabled);

	}

	~Graph()
	{
		getWorkbench()->removeListener(this);
	}

	void comboBoxChanged(ComboBox* cb) override;

	void buttonClicked(Button* b) override;;

	void postPostCompile(WorkbenchData::Ptr wb);

	void paint(Graphics& g);

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("SnexGraph"); }

	void resized() override
	{
		TestDataComponentBase::resized();

		auto b = getLocalBounds();
		b.removeFromTop(24);

		b.removeFromRight(32);
		
		internalGraph.setBounds(0, 0, viewport.getWidth() * internalGraph.zoomFactor, viewport.getMaximumVisibleHeight());
		viewport.setBounds(b);
		internalGraph.setBounds(0, 0, viewport.getWidth() * internalGraph.zoomFactor, viewport.getMaximumVisibleHeight());
		
		internalGraph.resizePath();

		repaint();
	}

	void testEventsChanged() override
	{
		internalGraph.repaint();
	}

	void refreshDisplayedBuffer();

	void processFFT(const AudioSampleBuffer& originalSource);

	void setCurrentPosition(int newPos)
	{
		internalGraph.currentPosition = newPos;
		repaint();
	}

	bool use2DSpectrum = true;

	int Spectrum2DSize = 128;

	Helpers::WindowType currentWindowType = Helpers::Rectangle;

	
	ComboBox* windowType;
	ComboBox* graphType;
	TextButton* logScaleButton;
	TextButton* logPeakButton;
	HiseShapeButton* processButton;
	HiseShapeButton* markerButton;
	AudioSampleBuffer fftSource;


	double cpuUsage = 0.0;
	Result currentResult;

	GraphType currentGraphType = GraphType::Signal;
};

struct TestDataComponent : public TestDataComponentBase
{
	TestDataComponent(WorkbenchData::Ptr p) :
		TestDataComponentBase(p),
		addEvent("add", this, f),
		addParameter("add", this, f)
	{
		addButton("new-file");
		addButton("open-file");
		addButton("save-file");

		signalType = addComboBox(WorkbenchData::TestData::getSignalTypeList());
		signalLength = addComboBox({ "1024", "2048", "4096", "8192", "16384", "32768" });

		addAndMakeVisible(addEvent);
		addAndMakeVisible(addParameter);
		
		addSpacer();

		compareButton = addButton("compare");
		testIcon = addButton("test", "test");

		eventViewport.setViewedComponent(&eventHolder, false);
		parameterViewport.setViewedComponent(&parameterHolder, false);
		addAndMakeVisible(eventViewport);
		addAndMakeVisible(parameterViewport);

		p->addListener(this);
	};

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("TestData"); };

	~TestDataComponent()
	{
		if (getWorkbench() != nullptr)
			getWorkbench()->removeListener(this);
	}

	Graph::Icons f;

	void buttonClicked(Button* b) override;

	void comboBoxChanged(ComboBox* cb) override;

	struct Item : public Component,
				  public ButtonListener,
				  public TextEditor::Listener
	{
		Item(WorkbenchData::TestData& d, int i, bool isParameter_);

		void buttonClicked(Button* b) override
		{
			if (b == &deleteButton)
			{
				data.removeTestEvent(index, isParameter);
			}
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().reduced(1).toFloat();

			g.setColour(Colours::white.withAlpha(0.1f));
			g.drawRoundedRectangle(b, 2.0f, 1.0f);
			g.fillRoundedRectangle(b, 2.0f);

			g.setColour(Colours::white.withAlpha(0.8f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(String(index), b.removeFromLeft(getHeight()).toFloat(), Justification::centred);
		}

		void textEditorReturnKeyPressed(TextEditor&);

#if 0
		void rebuildAsync()
		{
			auto p = findParentComponentOfClass<TestDataComponent>();
			MessageManager::callAsync([p]()
			{
				p->getWorkbench()->triggerPostCompileActions();
				p->rebuild();
			});
		}
#endif

		void resized() override
		{
			auto b = getLocalBounds();
			deleteButton.setBounds(b.removeFromRight(getHeight()).reduced(3));
			b.removeFromLeft(getHeight());
			jsonEditor.setBounds(b.reduced(2));
		}

		Graph::Icons f;

		TextEditor jsonEditor;
		HiseShapeButton deleteButton;

		OwnedArray<Component> components;

		WorkbenchData::TestData& data;
		bool isParameter = false;
		const int index;
	};

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(24);

		g.fillAll(Colour(0xFF262626));

		GlobalHiseLookAndFeel::drawFake3D(g, top);

		top = b.removeFromTop(24);

		g.setColour(Colours::white.withAlpha(0.1f));
		g.fillRect(top);

		g.setColour(Colours::black.withAlpha(0.15f));
		g.drawVerticalLine(getWidth() / 2, 0.0f, 24.0f);

		g.setColour(Colours::black.withAlpha(0.05f));
		g.drawVerticalLine(getWidth() / 2, 24.0f, (float)getHeight());

		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.9f));
		g.drawText("Parameters", top.removeFromLeft(top.getWidth() / 2).toFloat(), Justification::centred);
		g.drawText("Events", top.toFloat(), Justification::centred);
	}

	void testSignalChanged() override
	{
		auto& td = getWorkbench()->getTestData();

		signalLength->setText(String(td.testSignalLength), dontSendNotification);
		signalType->setSelectedItemIndex((int)td.currentTestSignalType, dontSendNotification);
	}

	void testEventsChanged() override
	{
		eventItems.clear();
		parameterItems.clear();

		auto& td = getWorkbench()->getTestData();

		for (int i = 0; i < td.getNumTestEvents(false); i++)
			eventItems.add(new Item(td, i, false));

		for (int i = 0; i < td.getNumTestEvents(true); i++)
			parameterItems.add(new Item(td, i, true));

		int Height = 24;

		parameterHolder.setSize(parameterViewport.getWidth() - parameterViewport.getScrollBarThickness(), parameterItems.size() * Height);
		eventHolder.setSize(eventViewport.getWidth() - eventViewport.getScrollBarThickness(), eventItems.size() * Height);

		auto pb = parameterHolder.getLocalBounds();
		auto eb = eventHolder.getLocalBounds();

		for (auto p : parameterItems)
		{
			parameterHolder.addAndMakeVisible(p);
			p->setBounds(pb.removeFromTop(Height));
		}
			
		for (auto e : eventItems)
		{
			eventHolder.addAndMakeVisible(e);
			e->setBounds(eb.removeFromTop(Height));
		}
	}

	void resized() override
	{
		TestDataComponentBase::resized();

		auto b = getLocalBounds();
		b.removeFromTop(24);
		auto r = b.removeFromTop(24);

		auto l = r.removeFromLeft(getWidth() / 2);

		addParameter.setBounds(l.removeFromLeft(24).reduced(3));
		addEvent.setBounds(r.removeFromLeft(24).reduced(3));

		parameterViewport.setBounds(b.removeFromLeft(b.getWidth() / 2));
		eventViewport.setBounds(b);
	}

	Component parameterHolder;
	Component eventHolder;

	Viewport parameterViewport;
	Viewport eventViewport;

	OwnedArray<Item> eventItems;
	OwnedArray<Item> parameterItems;
	HiseShapeButton addParameter;
	HiseShapeButton addEvent;

	ComboBox* signalLength;
	ComboBox* signalType;
	HiseShapeButton* compareButton;
	HiseShapeButton* testIcon;
};

struct TestComplexDataManager : public TestDataComponentBase,
								public hise::ComplexDataUIUpdaterBase::EventListener
{
	TestComplexDataManager(WorkbenchData::Ptr d) :
		TestDataComponentBase(d)
	{
		addButton("add");
		selector = addComboBox({});

		updateComboBox();

		addButton("delete");
	};

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("TestComplexData"); };

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override
	{
		if (getWorkbench()->getGlobalScope().isDebugModeEnabled())
		{
			if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
				getWorkbench()->triggerPostCompileActions();
		}
	}

	void testEventsChanged()
	{
		updateComboBox();
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF262626));

		auto b = getLocalBounds();
		GlobalHiseLookAndFeel::drawFake3D(g, b.removeFromTop(24));
	}

	void updateComboBox()
	{
		selector->setTextWhenNoChoicesAvailable("No data");

		auto& td = getWorkbench()->getTestData();

		auto currentId = selector->getSelectedId();

		selector->clear(dontSendNotification);

		ExternalData::forEachType([&](ExternalData::DataType type)
		{
			for (int i = 0; i < td.getNumDataObjects(type); i++)
			{
				String s = ExternalData::getDataTypeName(type);
				s << " " << String(i + 1);
				selector->addItem(s, getComboboxIndex(type, i));
			}
		});

		selector->setSelectedId(currentId, dontSendNotification);
	}

	void buttonClicked(Button* b) override
	{
		auto n = b->getName();

		if (n == "add")
		{
			PopupLookAndFeel plaf;
			PopupMenu m;
			m.setLookAndFeel(&plaf);

			auto& td = getWorkbench()->getTestData();

			ExternalData::forEachType([&](ExternalData::DataType t)
			{
				String s;
				s << "Add " << ExternalData::getDataTypeName(t) << " slot";
				m.addItem(getComboboxIndex(t, td.getNumDataObjects(t)), s);
			});

			int r = m.show();

			if (r != 0)
			{
				setComponent(r);
				updateComboBox();
				selector->setSelectedId(r, dontSendNotification);
			}
		}
		if (n == "delete")
		{
			ExternalData::DataType t;
			int i;

			if (getDataTypeAndIndex(selector->getSelectedId(), t, i))
			{
				getWorkbench()->getTestData().removeDataObject(t, i);
				currentDataComponent = nullptr;
				updateComboBox();
			}
		}
	}

	static ExternalData::DataType getDataType(int comboBoxIndex)
	{
		return (ExternalData::DataType)(comboBoxIndex / 1000 - 1);
	}

	bool getDataTypeAndIndex(const int comboBoxIndex, ExternalData::DataType& d, int& dataIndex)
	{
		d = getDataType(comboBoxIndex);

		if (d != ExternalData::DataType::numDataTypes)
		{
			dataIndex = comboBoxIndex % 1000;
			return true;
		}

		return false;
	}

	int getComboboxIndex(ExternalData::DataType t, int index)
	{
		return index + (((int)t + 1) * 1000);
	}

	void setComponent(int cbIndex)
	{
		ExternalData::DataType d;
		int i;

		if (getDataTypeAndIndex(cbIndex, d, i))
		{
			auto& td = getWorkbench()->getTestData();

			auto obj = td.getComplexBaseType(d, i);

			obj->getUpdater().addEventListener(this);

			currentDataComponent = dynamic_cast<Component*>(ExternalData::createEditor(obj));

#if 0
			if (d == ExternalData::DataType::Table)
			{
				auto t = td.getTable(i);
				auto te = new TableEditor(nullptr, t);
				t->addRulerListener(this);
				currentDataComponent = te;
			}
			if (d == ExternalData::DataType::SliderPack)
			{
				auto t = td.getSliderPack(i);
				auto sp = new SliderPack(t);
				sp->addListener(this);
				currentDataComponent = sp;
			}
			if (d == ExternalData::DataType::AudioFile)
			{
				auto t = td.getAudioFile(i);
				auto b = new hise::MultiChannelAudioBufferDisplay();
				b->setAudioFile(t);
				currentDataComponent = b;
			}
			if (d == ExternalData::DataType::FilterCoefficients)
			{
				auto t = td.getFilterData(i);
				auto b = new hise::FilterGraph();
				b->setComplexDataUIBase(t);
				currentDataComponent = b;
			}
			if (d == ExternalData::DataType::DisplayBuffer)
			{
				auto t = td.getDisplayBuffer(i);
				auto b = new hise::ModPlotter();
				b->setComplexDataUIBase(t);
				currentDataComponent = b;
			}
#endif
		}

		if (currentDataComponent != nullptr)
		{
			addAndMakeVisible(currentDataComponent);
			resized();
		}
	}

	void comboBoxChanged(ComboBox* cb) override
	{
		auto cbIndex = cb->getSelectedId();
		setComponent(cbIndex);
	}

	void resized() override
	{
		TestDataComponentBase::resized();
		auto b = getLocalBounds();
		b.removeFromTop(24);

		if (currentDataComponent != nullptr)
			currentDataComponent->setBounds(b);
	}

	ScopedPointer<Component> currentDataComponent;
	ComboBox* selector;
};


struct ParameterList : public WorkbenchComponent,
					   public SliderListener,
					   public WorkbenchData::TestData::TestListener
{
	ParameterList(WorkbenchData* data) :
		WorkbenchComponent(data)
	{
		data->addListener(this);
		data->getTestData().addListener(this);
		rebuild();
	};

	~ParameterList()
	{
		getWorkbench()->removeListener(this);
		getWorkbench()->getTestData().removeListener(this);
	}

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("SnexParameterList"); }

	Array<FunctionData> functions;

	void recompiled(WorkbenchData::Ptr wb) override
	{
		rebuild();
	}

	void testEventsChanged() override
	{
		for (auto s : sliders)
			s->setEnabled(getWorkbench()->getTestData().getNumTestEvents(true) == 0);
	}

	void rebuild();

	void sliderValueChanged(Slider* slider) override;

	void paint(Graphics& g) override
	{
		if (sliders.isEmpty())
		{
			g.setColour(Colours::white.withAlpha(0.3f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("No parameters available", getLocalBounds().toFloat(), Justification::centred);
		}
	}


	void resized() override
	{
		auto numColumns = jmax(1, getWidth() / 150);
		auto numRows = sliders.size() / numColumns + 1;

		auto wTotal = 150 * sliders.size();
		auto hTotal = 48;

		int xOffset = (getWidth() - wTotal) / 2;
		int yOffset = (getHeight() - hTotal) / 2;
		int i = 0;

		auto y = yOffset;

		for (int row = 0; row < numRows; row++)
		{
			auto x = xOffset;
			

			for (int column = 0; column < numColumns; column++)
			{
				if (auto s = sliders[i])
				{
					sliders[i++]->setTopLeftPosition(x, y);
					x += 150;
				}
				else
					break;
			}

			y += 50;
		}
	}

	WorkbenchData::CompileResult lastResult;
	WeakReference<JitCompiledNode> currentNode;
	hise::GlobalHiseLookAndFeel laf;
	juce::OwnedArray<juce::Slider> sliders;
};


}


}