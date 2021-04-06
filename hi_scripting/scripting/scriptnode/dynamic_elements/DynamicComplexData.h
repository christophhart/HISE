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
	public ComplexDataUIUpdaterBase::EventListener
{

	dynamic_base(data::base& b, ExternalData::DataType t, int index_);

	virtual ~dynamic_base();;

	void initialise(NodeBase* p) override;

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var data) override;

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
};

}

namespace dynamic
{
using table = data::pimpl::dynamicT<hise::SampleLookupTable>;
using filter = data::pimpl::dynamicT<hise::FilterDataObject>;
using displaybuffer = data::pimpl::dynamicT<hise::SimpleRingBuffer>;

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

	void initialise(NodeBase* n)
	{
		auto fileProvider = new PooledAudioFileDataProvider(n->getScriptProcessor()->getMainController_());

		internalData->setProvider(fileProvider);

		dynamicT<hise::MultiChannelAudioBuffer>::initialise(n);
		
		rangeSyncer.setCallback(getValueTree(), { PropertyIds::MinValue, PropertyIds::MaxValue }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(audiofile::updateRange));
	}
	
	void sourceHasChanged(ComplexDataUIBase* oldSource, ComplexDataUIBase* newSource) override
	{
		if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(newSource))
		{
			auto r = af->getCurrentRange();
			getValueTree().setProperty(PropertyIds::MinValue, r.getStart(), newSource->getUndoManager());
			getValueTree().setProperty(PropertyIds::MaxValue, r.getEnd(), newSource->getUndoManager());
		}
	}

	void updateRange(Identifier id, var newValue)
	{
		if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(currentlyUsedData))
		{
			auto min = (int)getValueTree()[PropertyIds::MinValue];
			auto max = (int)getValueTree()[PropertyIds::MaxValue];
			af->setRange({ min, max });
		}
	}

	valuetree::PropertyListener rangeSyncer;
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

	void timerCallback() override;

	
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
		static_assert(std::is_base_of<juce::Component, ComponentType>(), "not a component type");
		static_assert(std::is_base_of<ObjectType, DynamicDataType>(), "not a dynamic data type");

		addAndMakeVisible(externalButton);

		rebuildSelectorItems();

		addAndMakeVisible(editor);

		int w = 512;
		int h = 130;

		if (auto cd = dynamic_cast<ComponentWithDefinedSize*>(&editor))
		{
			auto a = cd->getFixedBounds();
			w = a.getWidth();
			h = a.getHeight();
		}

		if(AddDragger)
			addAndMakeVisible(dragger = new ModulationSourceBaseComponent(updater));

		setSize(w, h + 41);

		sourceHasChanged(nullptr, b->currentlyUsedData);
	};

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

		if (auto r = m.show())
			getObject()->getValueTree().setProperty(PropertyIds::Index, r - 2, getObject()->parentNode->getUndoManager());
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

	void resized() override
	{
		auto b = getLocalBounds();
		auto top = b.removeFromBottom(28);

		externalButton.setBounds(top.removeFromRight(28).reduced(3));

		if (dragger != nullptr)
		{
			top.removeFromLeft(28);
			dragger->setBounds(top.reduced(2));
		}

		b.removeFromBottom(10);
		b.removeFromTop(3);
		editor.setBounds(b);

		Path src;
		src.addRectangle(b.toFloat());

		PathStrokeType ps(1.0f);
		float d[2] = { 3.0f, 2.0f };
		ps.createDashedStroke(dashPath, src, d, 2);
	}

	void paintOverChildren(Graphics& g) override
	{
		auto idx = getObject()->getIndex();

		if (idx != -1)
		{
			auto b = editor.getBounds().toFloat();

			String x;
			x << "#";
			x << (idx+1);

			g.setColour(Colours::white.withAlpha(0.9f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.fillPath(dashPath);
			g.drawText(x, b.reduced(5.0f), Justification::topLeft);
		}
	}

	void sourceHasChanged(ComplexDataUIBase* oldSource, ComplexDataUIBase* newSource) override
	{
		ignoreUnused(oldSource);
		editor.setComplexDataUIBase(newSource);
		newSource->setGlobalUIUpdater(u);

		rebuildSelectorItems();
		resized();
		repaint();
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
	ComponentType editor;
	ScopedPointer<ModulationSourceBaseComponent> dragger;
};


}

using filter_editor = data::ui::pimpl::editorT<data::dynamic::filter, hise::FilterDataObject, hise::FilterGraph, false>;
using displaybuffer_editor = data::ui::pimpl::editorT<data::dynamic::displaybuffer, hise::SimpleRingBuffer, hise::ModPlotter, true>;

using table_editor_without_mod = data::ui::pimpl::editorT<data::dynamic::table, hise::Table, hise::TableEditor, false>;
using sliderpack_editor_without_mod = data::ui::pimpl::editorT<data::dynamic::sliderpack, hise::SliderPackData, hise::SliderPack, false>;

using table_editor = data::ui::pimpl::editorT<data::dynamic::table, hise::Table, hise::TableEditor, true>;
using sliderpack_editor = data::ui::pimpl::editorT<data::dynamic::sliderpack, hise::SliderPackData, hise::SliderPack, true>;
using audiofile_editor = data::ui::pimpl::editorT<data::dynamic::audiofile, hise::MultiChannelAudioBuffer, hise::MultiChannelAudioBufferDisplay, false>;
using audiofile_editor_with_mod = data::ui::pimpl::editorT<data::dynamic::audiofile, hise::MultiChannelAudioBuffer, hise::MultiChannelAudioBufferDisplay, true>;


}

}

}