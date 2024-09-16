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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace data
{
using namespace snex;

namespace pimpl
{
struct dynamic_base : public base,
	public ExternalDataHolder,
	public ExternalDataHolderWithForcedUpdate::ForcedUpdateListener,
	public ComplexDataUIUpdaterBase::EventListener
{

	dynamic_base(data::base& b, ExternalData::DataType t, int index_);

	virtual ~dynamic_base();;

	void initialise(NodeBase* p) override;

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var data) override;

	void forceRebuild(ExternalData::DataType dt_, int index) override;

	void updateExternalData();

	void updateData(Identifier id, var newValue);

	virtual ComplexDataUIBase* getInternalData() = 0;

	int getNumDataObjects(ExternalData::DataType t) const override;

	void setExternalData(data::base& n, const ExternalData& b, int index);

	Table* getTable(int index) override { return dynamic_cast<Table*>(currentlyUsedData); }
	SliderPackData* getSliderPack(int index) override { return dynamic_cast<SliderPackData*>(currentlyUsedData); }
	MultiChannelAudioBuffer* getAudioFile(int index) override { return dynamic_cast<MultiChannelAudioBuffer*>(currentlyUsedData); }
	FilterDataObject* getFilterData(int index) override { return dynamic_cast<FilterDataObject*>(currentlyUsedData); }
	SimpleRingBuffer* getDisplayBuffer(int index) override { return dynamic_cast<SimpleRingBuffer*>(currentlyUsedData); }

	bool removeDataObject(ExternalData::DataType t, int index) { return true; }

	void setIndex(int index, bool forceUpdate);

	WeakReference<NodeBase> parentNode;

	data::base* dataNode = nullptr;

	ComplexDataUIBase* currentlyUsedData = nullptr;
	ComplexDataUIBase::SourceWatcher sourceWatcher;

	int getIndex() const { return (int)cTree[PropertyIds::Index]; }

	ValueTree getValueTree() { return cTree; }

private:

	WeakReference<ExternalDataHolderWithForcedUpdate> forcedUpdateSource;

	Identifier getDataIndexPropertyId() const
	{
		String s;
		s << ExternalData::getDataTypeName(dt);
		s << String(index);
		s << "Index";
		return Identifier(s);
	}

	Identifier getDataStringPropertyId() const
	{
		String s;
		s << ExternalData::getDataTypeName(dt);
		s << String(index);
		s << "Data";
		return Identifier(s);
	}

	const ExternalData::DataType dt;
	const int index;

	ValueTree cTree;

	valuetree::PropertyListener dataUpdater;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic_base);
};

template <typename T> struct dynamicT : public dynamic_base
{
	static const int NumTables = ExternalData::getDataTypeForClass<T>() == ExternalData::DataType::Table ? 1 : 0;
	static const int NumSliderPacks = ExternalData::getDataTypeForClass<T>() == ExternalData::DataType::SliderPack ? 1 : 0;
	static const int NumAudioFiles = ExternalData::getDataTypeForClass<T>() == ExternalData::DataType::AudioFile ? 1 : 0;
	static const int NumFilters = ExternalData::getDataTypeForClass<T>() == ExternalData::DataType::FilterCoefficients ? 1 : 0;
	static const int NumDisplayBuffers = ExternalData::getDataTypeForClass<T>() == ExternalData::DataType::DisplayBuffer ? 1 : 0;

	dynamicT(data::base& t, int index=0) :
		dynamic_base(t, ExternalData::getDataTypeForClass<T>(), index)
	{
		internalData = new T();
		setIndex(-1, true);
	}

	virtual ~dynamicT() {}

	ComplexDataUIBase* getInternalData() override { return internalData.get(); }

	ReferenceCountedObjectPtr<T> internalData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamicT);
};

}

namespace dynamic
{
using table = data::pimpl::dynamicT<hise::SampleLookupTable>;
using filter = data::pimpl::dynamicT<hise::FilterDataObject>;

/** This needs some additional functionality:

	1. set the default slider amount to 16
	2. Watch the NumParameters property and resize the slider pack accordingly.
		This is used for multi-output nodes that have some kind of slider pack connection.
*/
struct sliderpack : public data::pimpl::dynamicT<hise::SliderPackData>
{
	sliderpack(data::base& t, int index=0) :
		dynamicT<hise::SliderPackData>(t, index)
	{
		this->internalData->setNumSliders(16);
	}

	virtual ~sliderpack() {};

	void initialise(NodeBase* p) override;

	void updateNumParameters(Identifier id, var newValue);

	valuetree::PropertyListener numParameterSyncer;
};



/** Additional functions:

	1. Act as data provider and resolve references.
	2. Store the sample range
*/
struct audiofile : public data::pimpl::dynamicT<hise::MultiChannelAudioBuffer>,
				   public ComplexDataUIBase::SourceListener
{
	audiofile(data::base& t, int index=0):
		dynamicT<hise::MultiChannelAudioBuffer>(t, index)
	{
		sourceWatcher.addSourceListener(this);
	}

	~audiofile()
	{
		sourceWatcher.removeSourceListener(this);
	}

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var data) override;

	void initialise(NodeBase* n);
	
	void sourceHasChanged(ComplexDataUIBase* oldSource, ComplexDataUIBase* newSource) override;

	void updateRange(Identifier id, var newValue);

	valuetree::PropertyListener rangeSyncer;
	bool allowRangeChange = false;
};

struct displaybuffer : public data::pimpl::dynamicT<hise::SimpleRingBuffer>
{
	displaybuffer(data::base& t, int index = 0);

	void initialise(NodeBase* n) override;

	SimpleRingBuffer* getCurrentRingBuffer()
	{
		return dynamic_cast<SimpleRingBuffer*>(currentlyUsedData);
	}

	void updateProperty(Identifier id, const var& newValue);

	valuetree::PropertyListener propertyListener;

	JUCE_DECLARE_WEAK_REFERENCEABLE(displaybuffer);
};

}

namespace ui
{
namespace pimpl
{
struct editor_base : public ScriptnodeExtraComponent<data::pimpl::dynamic_base>,
	public ComplexDataUIBase::SourceListener
{
	editor_base(ObjectType* b, PooledUIUpdater* updater);

	virtual ~editor_base();;

	static void showProperties(SimpleRingBuffer* obj, Component* c);

	static Colour getColourFromNodeComponent(NodeComponent* nc);

};

struct RingBufferPropertyEditor: public Component
{
	RingBufferPropertyEditor(dynamic::displaybuffer* b, RingBufferComponentBase* e);

	void addItem(const Identifier& id, const StringArray& entries)
	{
		auto v = buffer->getCurrentRingBuffer()->getProperty(id);

		auto c = new Item(buffer, id, entries, v);
		items.add(c);
		addAndMakeVisible(c);
	}

	SimpleRingBuffer* getCurrentRingBuffer()
	{
		return dynamic_cast<SimpleRingBuffer*>(buffer->currentlyUsedData);
	}

	struct Item: public Component,
				 public ComboBox::Listener
	{
		Item(dynamic::displaybuffer* b_, Identifier id_, const StringArray& entries, const String& value);

		void resized() override
		{
			auto w = f.getStringWidthFloat(id.toString()) + 10.0f;

			auto b = getLocalBounds();
			b.removeFromLeft(w);

			cb.setBounds(b.reduced(1.0f));
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().removeFromLeft(cb.getX());

			g.setFont(f);
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawText(id.toString(), b.toFloat(), Justification::centred);
		}

		void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
		{
			auto um = db->parentNode->getUndoManager();
			db->getValueTree().setProperty(id, cb.getText(), um);
		}
		
		Font f;
		Identifier id;
		ScriptnodeComboBoxLookAndFeel laf;
		ComboBox cb;
		dynamic::displaybuffer* db;
	};

	void paint(Graphics& g) override
	{
	}

	void resized() override
	{
		auto b = getLocalBounds();

		for (auto i : items)
		{
			auto pos = b.removeFromLeft(i->getWidth());
			i->setBounds(pos);
		}
	}

	

	WeakReference<dynamic::displaybuffer> buffer;
	RingBufferComponentBase* editor;

	OwnedArray<Component> items;
};

struct complex_ui_laf : public ScriptnodeComboBoxLookAndFeel,
						public TableEditor::LookAndFeelMethods,
						public SliderPack::LookAndFeelMethods,
						public HiseAudioThumbnail::LookAndFeelMethods,
						public FilterGraph::LookAndFeelMethods,
						public RingBufferComponentBase::LookAndFeelMethods,
						public AhdsrGraph::LookAndFeelMethods
{
	complex_ui_laf() = default;

	void drawTableBackground(Graphics& g, TableEditor& te, Rectangle<float> area, double rulerPosition) override;
	void drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness) override;
	void drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged) override;
	void drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition) override;
	void drawTableValueLabel(Graphics& g, TableEditor& te, Font f, const String& text, Rectangle<int> textBox) override;

	void drawFilterBackground(Graphics &g, FilterGraph& fg) override;
	void drawFilterGridLines(Graphics &g, FilterGraph& fg, const Path& gridPath) override;
	void drawFilterPath(Graphics& g, FilterGraph& fg, const Path& p) override;

	bool shouldClosePath() const override { return false; }

	void drawLinearSlider(Graphics&, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle, Slider&) override;

	void drawSliderPackBackground(Graphics& g, SliderPack& s) override;
	void drawSliderPackFlashOverlay(Graphics& g, SliderPack& s, int sliderIndex, Rectangle<int> sliderBounds, float intensity) override;

	void drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area) override;
	void drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path) override;
	void drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area) override;

	void drawButtonBackground(Graphics&, Button&, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	void drawButtonText(Graphics&, TextButton&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	Colour getNodeColour(Component* c);

	void drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac, Rectangle<float> area) override;
	void drawOscilloscopePath(Graphics& g, RingBufferComponentBase& ac, const Path& p) override;
	void drawGonioMeterDots(Graphics& g, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index) override;
	void drawAnalyserGrid(Graphics& g, RingBufferComponentBase& ac, const Path& p) override;

	void drawAhdsrBackground(Graphics& g, AhdsrGraph& graph) override;
	void drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive) override;
	void drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> p) override;

	Colour nodeColour;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(complex_ui_laf);
};



template <class DynamicDataType, class DataType, class ComponentType, bool AddDragger> struct editorT : public editor_base,
																										public ButtonListener
{
	editorT(PooledUIUpdater* updater, ObjectType* b) :
		editor_base(b, updater),
		u(updater),
		externalButton(getButtonName(), this, f)
	{
		static_assert(std::is_base_of<ComplexDataUIBase, DataType>(), "not a complex data type");
		static_assert(std::is_base_of<ComplexDataUIBase::EditorBase, ComponentType>(), "not a complex data editor type");
		//static_assert(std::is_base_of<juce::Component, ComponentType>(), "not a component type");
		static_assert(std::is_base_of<ObjectType, DynamicDataType>(), "not a dynamic data type");

		addAndMakeVisible(externalButton);

		rebuildSelectorItems();

		

		

		//setSize(w, h + 41);

		newSource = b->currentlyUsedData;
		sourceChangedAsync();

		if (AddDragger)
			addAndMakeVisible(dragger = new ModulationSourceBaseComponent(updater));
		else if constexpr (std::is_same<DynamicDataType, data::dynamic::displaybuffer>())
		{
			if (auto rb = dynamic_cast<SimpleRingBuffer*>(b->currentlyUsedData))
			{
				if(rb->getPropertyObject()->allowModDragger() && AddDragger)
					addAndMakeVisible(dragger = new RingBufferPropertyEditor(dynamic_cast<DynamicDataType*>(b), editor.get()));
			}
		}

		int w = 512;
		int h = 130;

		if (auto cd = dynamic_cast<ComponentWithDefinedSize*>(editor.get()))
		{
			auto a = cd->getFixedBounds();
			w = a.getWidth();
			h = a.getHeight();
		}

		setSize(w, h);
	};

	bool initialised = false;

	void timerCallback() override
	{
		if (auto pc = findParentComponentOfClass<NodeComponent>())
		{
			auto nColour = getColourFromNodeComponent(pc);
			
			getEditorAsComponent()->setColour(HiseColourScheme::ColourIds::ComponentBackgroundColour, nColour);
			
			if (dragger != nullptr)
				dragger->setColour(ModPlotter::ColourIds::pathColour, nColour);

			auto thisScaleFactor = UnblurryGraphics::getScaleFactorForComponent(this, false);

			if (thisScaleFactor != lastScaleFactor)
			{
				lastScaleFactor = thisScaleFactor;
				getEditorAsComponent()->resized();
			}
		}
	}

	String getButtonName() const
	{
		auto dt = ExternalData::getDataTypeForClass(getObject()->currentlyUsedData);
		return ExternalData::getDataTypeName(dt, false).toLowerCase();
	}

	void buttonClicked(Button* b) override
	{
		auto type = ExternalData::getDataTypeForClass<DataType>();

		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		StringArray list;

		list.add("Embedded");

		slotSelector.clear(dontSendNotification);

		if (auto eh = getObject()->parentNode->getRootNetwork()->getExternalDataHolder())
		{
			for (int i = 0; i < eh->getNumDataObjects(type); i++)
			{
				String s;
				s << "External " << ExternalData::getDataTypeName(type) << " Slot #" << String(i + 1);
				list.add(s);
			}

			String s;
			s << "Add new external " << ExternalData::getDataTypeName(type);

			list.add(s);
		}



		auto index = getObject()->getIndex();

		for (int i = 0; i < list.size(); i++)
			m.addItem(i + 1, list[i], true, (i-1) == (index));

		if constexpr (std::is_same<DynamicDataType, data::dynamic::displaybuffer>())
		{
			m.addSeparator();
			m.addItem(9000, "Edit Properties");
			m.addItem(9001, "Show in big popup");
		}

		if constexpr (std::is_same<DynamicDataType, data::dynamic::filter>())
		{
			m.addSeparator();
			m.addItem(9001, "Show in big popup");
		}

		if (auto r = m.show())
		{
			if (r == 9000)
			{
				if (auto obj = dynamic_cast<SimpleRingBuffer*>(getObject()->currentlyUsedData))
				{
					showProperties(obj, &externalButton);
				}

				return;
			}
			if(r == 9001)
			{
#if USE_BACKEND
				if(auto obj = dynamic_cast<FilterDataObject*>(getObject()->currentlyUsedData))
				{
					struct ResizableFilterGraph: public Component
					{
						ResizableFilterGraph(const String& name, FilterDataObject* obj, Colour nColour):
						  resizer(this, nullptr)
						{
							setName("Filter Graph: " + name);

							fg.setComplexDataUIBase(obj);

							auto laf = new complex_ui_laf();
							laf->nodeColour = nColour;
							fg.setSpecialLookAndFeel(laf, true);

							addAndMakeVisible(fg);
							addAndMakeVisible(resizer);

							setSize(768, 400);
						}

						void resized() override
						{
							auto b = getLocalBounds();
							fg.setBounds(b.reduced(10));
							resizer.setBounds(b.removeFromRight(10).removeFromBottom(10));
						}

						FilterGraph fg;
						ResizableCornerComponent resizer;
					};

					auto pc = findParentComponentOfClass<NodeComponent>();
					auto nColour = pc != nullptr ? getColourFromNodeComponent(pc) : Colours::white;

					auto fg = new ResizableFilterGraph(getObject()->parentNode->getId(), obj, nColour);
					
					auto rt = GET_BACKEND_ROOT_WINDOW(this)->getRootFloatingTile();

					rt->showComponentInRootPopup(fg, this, {});
				}


				if (auto obj = dynamic_cast<SimpleRingBuffer*>(getObject()->currentlyUsedData))
				{
					struct ResizableModPlotter: public Component
					{
						ResizableModPlotter(const String& name, SimpleRingBuffer* obj, Colour c):
						  Component(name),
						  resizer(this, nullptr)
						{
							addAndMakeVisible(m);
							addAndMakeVisible(resizer);
							m.setComplexDataUIBase(obj);
							m.setColour(ModPlotter::backgroundColour, Colour(0xFF333333));
							m.setColour(ModPlotter::ColourIds::pathColour, c);
							auto laf = new complex_ui_laf();
							laf->nodeColour = c;
							m.setSpecialLookAndFeel(laf, true);
							
							setSize(768, 300);
						};

						void resized() override
						{
							auto b = getLocalBounds();
							m.setBounds(b.reduced(10));
							resizer.setBounds(b.removeFromRight(10).removeFromBottom(10));
						}

						ModPlotter m;
						ResizableCornerComponent resizer;
					};

					auto pc = findParentComponentOfClass<NodeComponent>();
					auto nColour = pc != nullptr ? getColourFromNodeComponent(pc) : Colours::white;

					auto n = new ResizableModPlotter("Plotter: " + getObject()->parentNode->getId(), obj, nColour);
					auto rt = GET_BACKEND_ROOT_WINDOW(this)->getRootFloatingTile();

					rt->showComponentInRootPopup(n, this, {});
				}
#endif
				return;
			}

			auto network = getObject()->parentNode->getRootNetwork();

			SimpleReadWriteLock::ScopedWriteLock sl(network->getConnectionLock());

			auto& eh = network->getExceptionHandler();
			eh.removeError(getObject()->parentNode, Error::RingBufferMultipleWriters);

			try
			{
				getObject()->getValueTree().setProperty(PropertyIds::Index, r - 2, getObject()->parentNode->getUndoManager());
			}
			catch (scriptnode::Error& e)
			{
				eh.addError(getObject()->parentNode, e);
			}

			if(auto nc = findParentComponentOfClass<NodeComponent>())
				nc->repaint();
		}
			
	}

	void rebuildSelectorItems()
	{
		auto index = getObject()->getIndex();
		externalButton.setToggleStateAndUpdateIcon(index != -1);
	}

	void paint(Graphics& g) override
	{
		
	}

	~editorT() {};

	Component* getEditorAsComponent() { return dynamic_cast<Component*>(editor.get()); }

	void resized() override
	{
		auto b = getLocalBounds();
		
		if (dragger != nullptr && dragger->isVisible())
		{
			auto top = b.removeFromBottom(28);

			externalButton.setBounds(top.removeFromRight(28).reduced(3));

			if(dynamic_cast<ModulationSourceBaseComponent*>(dragger.get()) != nullptr)
				top.removeFromLeft(28);

			dragger->setBounds(top.reduced(2));

			b.removeFromBottom(UIValues::NodeMargin);
		}
		else
		{
			b.removeFromLeft(28);
			externalButton.setBounds(b.removeFromRight(28).removeFromBottom(28).reduced(3));
		}
		
		b.removeFromTop(3);

		if (getEditorAsComponent() == nullptr)
			return;

		getEditorAsComponent()->setBounds(b);

		refreshDashPath();
	}

	void refreshDashPath()
	{
		auto b = getEditorAsComponent()->getBounds();
		Path src;
		src.addRectangle(b.toFloat());

		PathStrokeType ps(1.0f);
		float d[2] = { 3.0f, 2.0f };
		ps.createDashedStroke(dashPath, src, d, 2);
	}

	void paintOverChildren(Graphics& g) override
	{
		if (getObject() == nullptr)
			return;

		auto idx = getObject()->getIndex();

		if (idx != -1)
		{
			auto b = getEditorAsComponent()->getBounds().toFloat();

			String x;
			x << "#";
			x << (idx+1);

			g.setColour(Colours::white.withAlpha(0.9f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.fillPath(dashPath);
			g.drawText(x, b.reduced(5.0f), Justification::topLeft);
		}
	}

	static constexpr bool isDisplayBufferBase() { return std::is_same<ComponentType, RingBufferComponentBase>(); }

	void sourceChangedAsync()
	{
		if (newSource == nullptr)
			return;

		if constexpr (isDisplayBufferBase())
			editor = dynamic_cast<SimpleRingBuffer*>(newSource.get())->getPropertyObject()->createComponent();
		else
			editor = new ComponentType();

		editor->setComplexDataUIBase(newSource);
		editor->setSpecialLookAndFeel(new complex_ui_laf(), true);

		newSource->setGlobalUIUpdater(u);

		addAndMakeVisible(getEditorAsComponent());

		rebuildSelectorItems();

		if (auto te = dynamic_cast<TableEditor*>(editor.get()))
			te->setScrollModifiers(ModifierKeys().withFlags(ModifierKeys::commandModifier | ModifierKeys::shiftModifier));

		if (!getLocalBounds().isEmpty())
		{
			resized();
			repaint();
		}
	}

	void sourceHasChanged(ComplexDataUIBase* oldSource, ComplexDataUIBase* newSource_) override
	{
		ignoreUnused(oldSource);

		newSource = newSource_;

		WeakReference<editorT> safeThis(this);

		auto f = [safeThis]()
		{
			if (safeThis.get() != nullptr)
				safeThis->sourceChangedAsync();
		};

		MessageManager::callAsync(f);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		// must be casted to the derived class or you'll slice the object like a melon!!!
		auto t = static_cast<DynamicDataType*>(obj);

		return new editorT<DynamicDataType, DataType, ComponentType, AddDragger>(updater, t);
	}

	ExternalData::Factory f;
	HiseShapeButton externalButton;

	Path dashPath;

	PooledUIUpdater* u;
	PopupLookAndFeel plaf;
	ComboBox slotSelector;
	WeakReference<ComplexDataUIBase> newSource = nullptr;
	ScopedPointer<ComponentType> editor;
	ScopedPointer<Component> dragger;

	float lastScaleFactor = 1.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(editorT);

	JUCE_DECLARE_WEAK_REFERENCEABLE(editorT);
};


}

using filter_editor = data::ui::pimpl::editorT<data::dynamic::filter, hise::FilterDataObject, hise::FilterGraph, false>;
using displaybuffer_editor = data::ui::pimpl::editorT<data::dynamic::displaybuffer, hise::SimpleRingBuffer, hise::RingBufferComponentBase, true>;
using displaybuffer_editor_nomod = data::ui::pimpl::editorT<data::dynamic::displaybuffer, hise::SimpleRingBuffer, hise::RingBufferComponentBase, false>;

using table_editor_without_mod = data::ui::pimpl::editorT<data::dynamic::table, hise::Table, hise::TableEditor, false>;
using sliderpack_editor_without_mod = data::ui::pimpl::editorT<data::dynamic::sliderpack, hise::SliderPackData, hise::SliderPack, false>;

using table_editor = data::ui::pimpl::editorT<data::dynamic::table, hise::Table, hise::TableEditor, true>;
using sliderpack_editor = data::ui::pimpl::editorT<data::dynamic::sliderpack, hise::SliderPackData, hise::SliderPack, true>;
using audiofile_editor = data::ui::pimpl::editorT<data::dynamic::audiofile, hise::MultiChannelAudioBuffer, hise::XYZMultiChannelAudioBufferEditor, false>;
using audiofile_editor_with_mod = data::ui::pimpl::editorT<data::dynamic::audiofile, hise::MultiChannelAudioBuffer, hise::XYZMultiChannelAudioBufferEditor, true>;


using xyz_audio_editor = data::ui::pimpl::editorT<data::dynamic::audiofile, hise::MultiChannelAudioBuffer, hise::XYZMultiChannelAudioBufferEditor, false>;

}

}

}