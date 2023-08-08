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

#ifndef SAMPLEMAPBROWSER_H_INCLUDED
#define SAMPLEMAPBROWSER_H_INCLUDED

namespace hise { using namespace juce;


class SampleMapBrowser : public Component,
	public FloatingTileContent
{
public:

	enum SpecialPanelIds
	{
		SamplerId = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		SampleList,
		numPropertyIds
	};

	SET_PANEL_NAME("SampleMapBrowser");

	SampleMapBrowser(FloatingTile* parent);
	~SampleMapBrowser();

	int getNumDefaultableProperties() const override;
	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

	void resized() override;

private:

	struct ValueTreeHelpers
	{
		static ValueTree createEntry(const String& displayName, const String& id = String());
		static void createEntryWithHierarchy(ValueTree& v, const Array<var>& columns, const String& id);
	};

	class ColumnListBoxModel : public ListBoxModel
	{
	public:
		ColumnListBoxModel(SampleMapBrowser* parent_, int index_);

		int getNumRows() override;

		void setData(const ValueTree& newData);
		void listBoxItemClicked(int row, const MouseEvent &) override;
		void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
		void returnKeyPressed(int row) override;
		void setHighlightColourAndFont(Font f, Colour highlightColour_);

	private:

		Colour highlightColour;
		Font font;

		SampleMapBrowser* parent;
		ValueTree data;
		int index;
	};


	class Column : public Component
	{
	public:

		Column(SampleMapBrowser* parent, int index);

		void resized() override;

		void setData(const ValueTree& newData);

	private:

		ScopedPointer<ColumnListBoxModel> model;
		ScopedPointer<ListBox> table;

	};

	void rebuildValueTree();
	void rebuildColumns();

	String samplerId;
	Array<var> sampleList;
	ValueTree columnData;
	WeakReference<Processor> scriptProcessor;
	OwnedArray<Column> columns;
	int numColumns;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleMapBrowser)
};


} // namespace hise

#endif  // SAMPLEMAPBROWSER_H_INCLUDED
