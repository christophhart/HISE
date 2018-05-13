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

#ifndef SAMPLEPOOLTABLE_H_INCLUDED
#define SAMPLEPOOLTABLE_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSamplerSoundPool;

/** A table component containing all samples of a ModulatorSampler.
*	@ingroup debugComponents
*/
class SamplePoolTable      : public Component,
                             public TableListBoxModel,
							 public SafeChangeListener
{
public:

	enum ColumnId
	{
		FileName = 1,
		Memory,
		State,
		References,
		numColumns
	};

	SamplePoolTable(BackendRootWindow *rootWindow) ;

	SET_GENERIC_PANEL_ID("SamplePoolTable");

	~SamplePoolTable();

	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		setName(getHeadline());
		table.updateContent();
		if(getParentComponent() != nullptr) getParentComponent()->repaint();
	}

    int getNumRows() override;

    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;

	void selectedRowsChanged(int /*lastRowSelected*/) override;

    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override;
    
	String getHeadline() const;

    void resized() override;

	void mouseDown(const MouseEvent &e) override;

private:
    TableListBox table;     // the table component itself
    Font font;

	ModulatorSamplerSoundPool *pool;
	ScopedPointer<TableHeaderLookAndFeel> laf;
	PopupLookAndFeel plaf;

    int numRows;            // The number of rows of data we've got

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePoolTable)
};

class MainController;

struct PoolTableHelpers
{
	template <class DataType> void doubleClickCallback(MainController* mc, const SharedPoolBase<DataType>& pool, int rowNumber)
	{
		

		

		if (auto editor = mc->getLastActiveEditor())
		{
			auto fileName = pool->getFileNameForId(pool->getIdForIndex(rowNumber));
			auto ref = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFileReference(fileName, ProjectHandler::SubDirectories::AudioFiles);

			editor->insertTextAtCaret(ref);
		}
	}

	
};

/** A table component containing all samples of a ModulatorSampler.
*	@ingroup components
*/
template <class DataType> class ExternalFileTableBase: public Component,
							 public ControlledObject,
                             public TableListBoxModel,
							 public SafeChangeListener,
							 public DragAndDropContainer,
							 public ExpansionHandler::Listener
{
public:

	enum ColumnId
	{
		FileName = 1,
		Memory,
		Type,
		References,
		numColumns
	};

	// ========================================================================================================= ExternalFileTable
	ExternalFileTableBase(BackendRootWindow* rootWindow) :
		ControlledObject(rootWindow->getMainController()),
		font(GLOBAL_FONT()),
		selectedRow(-1)
	{
		getMainController()->getExpansionHandler().addListener(this);

		setName(getHeadline());

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
		table.getHeader().addColumn("Type", Type, 50);
		table.getHeader().addColumn("# Ref", References, 40);
		
		updatePool();

	}

	~ExternalFileTableBase()
	{
		getMainController()->getExpansionHandler().removeListener(this);

		pool->removeChangeListener(this);
	}

	void expansionPackLoaded(Expansion* currentExpansion) override
	{
		updatePool();
	}

	

	void updatePool()
	{
		if (pool != nullptr)
		{
			pool->removeChangeListener(this);
			
			pool = getMainController()->getExpansionHandler().template getCurrentPool<DataType>();

			pool->addChangeListener(this);

			table.updateContent();
		}
	}

	static Identifier getGenericPanelId()
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());
		
		return Identifier(ProjectHandler::getIdentifier(type) + "PoolTable");
	}

	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		setName(getHeadline());
		table.updateContent();
		if(getParentComponent() != nullptr) getParentComponent()->repaint();
	}

	int getNumRows() override
	{
		return pool->getNumLoadedFiles();
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

	String getHeadline() const
	{
		String x;

		const int numSamples = pool->getNumLoadedFiles();

		x << "External Files: " << String(numSamples) << " " << pool->getFileTypeName().toString();

		return x;
	}

	void resized() override
	{
		table.setBounds(getLocalBounds());

		table.getHeader().setColumnWidth(FileName, getWidth() - 200);
		table.getHeader().setColumnWidth(Memory, 100);
		table.getHeader().setColumnWidth(References, 100);
	}

	void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent&)
	{
		auto mc = pool->getMainController();

		if (auto editor = mc->getLastActiveEditor())
		{
			auto ref = pool->getReference(rowNumber);

			if (ref.isValid())
			{
				editor->insertTextAtCaret(ref.getReferenceString());
			}
		}
	}

	String getTextForTableCell(int rowNumber, int columnNumber)
	{

		StringArray info = pool->getTextDataForId(rowNumber);

		if ((columnNumber - 1) < info.size())
		{
			return info[columnNumber - 1];
		}
		else return String();
	}

	var getDragSourceDescription(const SparseSet< int > &set) override
	{
		var id;

		if(set.getNumRanges() > 0)
		{
			const int index = set[0];

			id = pool->getReference(index).getReferenceString();
		}

		return id;
	};

protected:



	WeakReference<SharedPoolBase<DataType>> pool;

private:
    TableListBox table;     // the table component itself
    Font font;

	int selectedRow;

	var currentlyDraggedId;

	ScopedPointer<TableHeaderLookAndFeel> laf;
	PopupLookAndFeel plaf;

    int numRows;            // The number of rows of data we've got

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExternalFileTableBase)
};

struct PoolTableSubTypes
{
	using AudioFilePoolTable = ExternalFileTableBase<AudioSampleBuffer>;
	using ImageFilePoolTable = ExternalFileTableBase<Image>;
};



} // namespace hise

#endif  // SAMPLEPOOLTABLE_H_INCLUDED
