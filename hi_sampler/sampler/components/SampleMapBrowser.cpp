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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace hise { using namespace juce;

ValueTree SampleMapBrowser::ValueTreeHelpers::createEntry(const String& displayName, const String& id /*= String()*/)
{
	const bool isDirectory = id.isEmpty();

	ValueTree v(isDirectory ? "Directory" : "SampleMap");

	static const Identifier n("Name");
	static const Identifier i("ID");

	if (!isDirectory)
		v.setProperty(i, id, nullptr);

	v.setProperty(n, displayName, nullptr);

	return v;
}

void SampleMapBrowser::ValueTreeHelpers::createEntryWithHierarchy(ValueTree& v, const Array<var>& columns, const String& id)
{
	static const Identifier n("Name");

	ValueTree parent = v;

	for (int i = 0; i < columns.size(); i++)
	{
		var thisName = columns[i];

		const bool isLast = columns.size() == i + 1;

		auto existing = parent.getChildWithProperty(n, thisName);

		if (existing.isValid())
		{
			parent = existing;
		}
		else
		{
			parent.addChild(createEntry(thisName, isLast ? id : String()), -1, nullptr);
			parent = parent.getChild(parent.getNumChildren() - 1);
		}
	}
}

SampleMapBrowser::ColumnListBoxModel::ColumnListBoxModel(SampleMapBrowser* parent_, int index_) :
	parent(parent_),
	index(index_),
	font(GLOBAL_BOLD_FONT()),
	highlightColour(Colour(SIGNAL_COLOUR))
{

}

int SampleMapBrowser::ColumnListBoxModel::getNumRows()
{
	return data.getNumChildren();
}

void SampleMapBrowser::ColumnListBoxModel::setData(const ValueTree& newData)
{
	data = newData;
}

void SampleMapBrowser::ColumnListBoxModel::listBoxItemClicked(int row, const MouseEvent &)
{
	auto nextColumnn = parent->columns[index + 1];

	if (nextColumnn != nullptr)
	{
		nextColumnn->setData(data.getChild(row));
	}

	auto value = data.getChild(row).getProperty("ID");

	if (auto popup = parent->findParentComponentOfClass<FloatingTilePopup>())
	{
		if (auto attachedComponent = popup->getAttachedComponent())
		{
			if (auto content = attachedComponent->findParentComponentOfClass<ScriptContentComponent>())
			{
				

				auto sc = content->getScriptComponentFor(attachedComponent);
				auto pwsc = const_cast<ProcessorWithScriptingContent*>(dynamic_cast<const ProcessorWithScriptingContent*>(content->getScriptProcessor()));

				if (sc != nullptr)
				{
					sc->setValue(value);
					pwsc->controlCallback(sc, value);
				}
			}
		}	
	}
	else
	{
		auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(parent->scriptProcessor.get());



		if (pwsc == nullptr)
			return;


		auto content = parent->findParentComponentOfClass<ScriptContentComponent>();

		if (content != nullptr)
		{
			auto sc = content->getScriptComponentFor(parent->getParentShell());

			if (sc != nullptr)
			{
				sc->setValue(value);
				pwsc->controlCallback(sc, value);
			}
		}
	}
}

void SampleMapBrowser::ColumnListBoxModel::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	if (rowNumber < data.getNumChildren())
	{
		auto text = data.getChild(rowNumber).getProperty("Name");

		Rectangle<int> area(0, 1, width, height - 2);

		g.setColour(rowIsSelected ? highlightColour.withAlpha(0.3f) : Colour(0x00222222));
		g.fillRect(area);
		g.setColour(Colours::white.withAlpha(0.4f));
		if (rowIsSelected) g.drawRect(area, 1);

		g.setColour(Colours::white);
		g.setFont(font.withHeight(16.0f));
		g.drawText(text, 10, 0, width - 20, height, Justification::centredLeft);
	}
}

void SampleMapBrowser::ColumnListBoxModel::returnKeyPressed(int /*row*/)
{

}

void SampleMapBrowser::ColumnListBoxModel::setHighlightColourAndFont(Font f, Colour highlightColour_)
{
	font = f;
	highlightColour = highlightColour_;
}

SampleMapBrowser::Column::Column(SampleMapBrowser* parent, int index)
{
	model = new ColumnListBoxModel(parent, index);
	addAndMakeVisible(table = new ListBox());

	table->setModel(model);

	table->setColour(ListBox::ColourIds::backgroundColourId, Colours::white.withAlpha(0.15f));
	table->setRowHeight(30);
	table->setWantsKeyboardFocus(true);

	if (HiseDeviceSimulator::isMobileDevice())
		table->setRowSelectedOnMouseDown(false);

	table->getViewport()->setScrollOnDragEnabled(true);
}

void SampleMapBrowser::Column::resized()
{
	table->setBounds(getLocalBounds());
}

void SampleMapBrowser::Column::setData(const ValueTree& newData)
{
	model->setData(newData);
	table->updateContent();
}

SampleMapBrowser::SampleMapBrowser(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	auto content = parent->findParentComponentOfClass<ScriptContentComponent>();

	if (content != nullptr)
	{
		scriptProcessor = const_cast<Processor*>(dynamic_cast<const Processor*>(content->getScriptProcessor()));
	}

	setDefaultPanelColour(PanelColourId::bgColour, Colours::black.withAlpha(0.97f));
	setDefaultPanelColour(PanelColourId::itemColour1, Colour(SIGNAL_COLOUR));
}

SampleMapBrowser::~SampleMapBrowser()
{
	columns.clear();
	sampleList.clear();
}

int SampleMapBrowser::getNumDefaultableProperties() const
{
	return (int)SpecialPanelIds::numPropertyIds;
}

var SampleMapBrowser::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, SpecialPanelIds::SamplerId, samplerId);
	storePropertyInObject(obj, SpecialPanelIds::SampleList, sampleList);;

	return obj;
}

void SampleMapBrowser::fromDynamicObject(const var& )
{
}

Identifier SampleMapBrowser::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::SamplerId, "SamplerId");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::SampleList, "SampleList");

	jassertfalse;
	return{};
}

var SampleMapBrowser::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::SamplerId, "");
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::SampleList, Array<var>());

	jassertfalse;
	return{};
}

void SampleMapBrowser::resized()
{
	if (columns.size() != 0)
	{
		const int widthPerColumn = getWidth() / columns.size();

		for (int i = 0; i < columns.size(); i++)
		{
			auto r = Rectangle<int>(i * widthPerColumn, 0, widthPerColumn, getHeight());

			columns[i]->setBounds(r.reduced(3));
		}
	}
}

void SampleMapBrowser::rebuildValueTree()
{
	columnData = ValueTree("ColumnData");
	numColumns = 0;

	for (int i = 0; i < sampleList.size(); i++)
	{
		if (sampleList[i].isString())
		{
			columnData.addChild(ValueTreeHelpers::createEntry(sampleList[i], sampleList[i]), -1, nullptr);

			numColumns = jmax<int>(numColumns, 1);
		}
		else if (sampleList[i].isObject())
		{
			var columnArray = sampleList[i].getProperty("Columns", var());

			numColumns = jmax<int>(numColumns, columnArray.size());

			var id = sampleList[i].getProperty("ID", var());

			if (columnArray.isArray() && id.isString())
			{
				ValueTreeHelpers::createEntryWithHierarchy(columnData, *columnArray.getArray(), id);
			}
			else
			{
				throw String("Invalid sample map browser data");
			}
		}
	}
}

void SampleMapBrowser::rebuildColumns()
{
	rebuildValueTree();

	for (int i = 0; i < numColumns; i++)
	{
		auto l = new Column(this, i);

		addAndMakeVisible(l);
		columns.add(l);
	}

	columns.getFirst()->setData(columnData);
}

} // namespace hise
