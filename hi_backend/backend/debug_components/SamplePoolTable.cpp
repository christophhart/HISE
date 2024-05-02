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

namespace hise { using namespace juce;

SamplePoolTable::SamplePoolTable(BackendRootWindow* rootWindow) :
	font (GLOBAL_FONT()),
	pool(rootWindow->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool())
{
	
	setName(getHeadline());

    // Create our table component and add it to this component..
    addAndMakeVisible (table);
    table.setModel (this);

	pool->addChangeListener(this);

	laf = new TableHeaderLookAndFeel();

	table.getHeader().setLookAndFeel(laf);

	table.getHeader().setSize(getWidth(), 22);

    // give it a border
    table.setColour (ListBox::outlineColourId, Colours::black.withAlpha(0.5f));
	table.setColour(ListBox::backgroundColourId, HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));

    table.setOutlineThickness (0);

	table.getViewport()->setScrollBarsShown(true, false, false, false);

	table.getHeader().setInterceptsMouseClicks(true, true);

	table.getHeader().addColumn("File Name", ColumnId::FileName, 900 - 32 - 200);
	table.getHeader().addColumn("Memory", ColumnId::Memory, 60);
	table.getHeader().addColumn("State", ColumnId::State, 100);
	table.getHeader().addColumn("#Ref", ColumnId::References, 100);

	table.addMouseListener(this, true);
}


SamplePoolTable::~SamplePoolTable()
{
	table.getHeader().setLookAndFeel(nullptr);

	pool->removeChangeListener(this);

	

}


int SamplePoolTable::getNumRows()
{
	return pool->getNumSoundsInPool();
};



	
void SamplePoolTable::paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) 
{
	if(rowNumber % 2) g.fillAll(Colours::white.withAlpha(0.05f));

    if (rowIsSelected)
        g.fillAll (Colour(0x44000000));
}

void SamplePoolTable::selectedRowsChanged(int /*lastRowSelected*/) {};

void SamplePoolTable::paintCell (Graphics& g, int rowNumber, int columnId,
                int width, int height, bool /*rowIsSelected*/) 
{
	g.setColour (Colours::white.withAlpha(.8f));
    
	if (pool->isFileBeingUsed(rowNumber))
	{
		g.setFont(font.boldened());
	}
	else
	{
		g.setFont(font);
	}
	

	String text = pool->getTextForPoolTable(columnId, rowNumber);

    g.drawText (text, 2, 0, width - 4, height, Justification::centredLeft, true);

    //g.setColour (Colours::black.withAlpha (0.2f));
    //g.fillRect (width - 1, 0, 1, height);
}

String SamplePoolTable::getHeadline() const
{
        
    String memory = String(int64(pool->getMemoryUsageForAllSamples() / 1024 / 1024));
        
	String x;
        
        
        
	x << "Global Sample Pool Table - " << String(pool->getNumSoundsInPool()) << " samples  " << memory << " MB";
	return x;
}

    
void SamplePoolTable::resized() 
{
    table.setBounds(getLocalBounds());

	table.getHeader().setColumnWidth(ColumnId::FileName, getWidth() - 170);
	table.getHeader().setColumnWidth(ColumnId::Memory, 60);
	table.getHeader().setColumnWidth(ColumnId::State, 70);
	table.getHeader().setColumnWidth(ColumnId::References, 40);

}



void SamplePoolTable::mouseDown(const MouseEvent &e)
{
	if (e.mods.isLeftButtonDown()) return;

	PopupMenu m;

	m.setLookAndFeel(&plaf);

	enum 
	{
		ResolveMissingSamples = 1,
		DeleteMissingSamples,
		numOperations
	};

	m.addSectionHeader("Missing Sample Handling");
	m.addItem(ResolveMissingSamples, "Resolve Missing Sample References");
	m.addItem(DeleteMissingSamples, "Delete Missing Samples");

	const int result = m.show();

	switch (result)
	{
	case ResolveMissingSamples:	pool->resolveMissingSamples(this); break;
	}


};

} // namespace hise
