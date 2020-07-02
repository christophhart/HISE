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

ScriptWatchTable::ScriptWatchTable() :
	rebuilder(this),
	ApiComponentBase(nullptr)
{
	setOpaque(true);

	setName(getHeadline());

    addAndMakeVisible (table = new TableListBox());
    table->setModel (this);
	table->getHeader().setLookAndFeel(&laf);
	table->getHeader().setSize(getWidth(), 22);
    table->setOutlineThickness (0);
	table->getViewport()->setScrollBarsShown(true, false, false, false);
    
	table->setColour(ListBox::backgroundColourId, JUCE_LIVE_CONSTANT_OFF(Colour(0x04ffffff)));

	table->getHeader().addColumn("Type", Type, 30, 30, 30);
	table->getHeader().addColumn("Data Type", DataType, 100, 100, 100);
	table->getHeader().addColumn("Name", Name, 100, 60, 200);
	table->getHeader().addColumn("Value", Value, 180, 150, -1);

	table->getHeader().setStretchToFitActive(true);
	

	table->addMouseListener(this, true);

	addAndMakeVisible(fuzzySearchBox = new TextEditor());
	fuzzySearchBox->addListener(this);
	fuzzySearchBox->setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
    fuzzySearchBox->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	fuzzySearchBox->setFont(GLOBAL_FONT());
	fuzzySearchBox->setSelectAllWhenFocused(true);

	rebuildLines();
}


ScriptWatchTable::~ScriptWatchTable()
{
	rebuilder.cancelPendingUpdate();

	allVariableLines.clear();
	table = nullptr;
}

void ScriptWatchTable::timerCallback()
{
	if(table != nullptr) refreshChangeStatus();
}

int ScriptWatchTable::getNumRows() 
{
	return filteredIndexes.size();
};

void ScriptWatchTable::rebuildLines()
{
	allVariableLines.clear();

	if (auto pr = getProviderBase())
	{
		for (int i = 0; i < pr->getNumDebugObjects(); i++)
		{
			auto di = pr->getDebugInformation(i);
			Info f;
			f.type = di->getType();
			f.dataType = di->getTextForDataType();
			f.name = di->getTextForName();
			f.value = di->getTextForValue();

			allVariableLines.add(f);
		}

		applySearchFilter();
	}
}

void ScriptWatchTable::applySearchFilter()
{
	const String filterText = fuzzySearchBox->getText();

	if (filterText.isNotEmpty())
	{
		filteredIndexes.clear();

		for (int i = 0; i < allVariableLines.size(); i++)
		{
			if (allVariableLines[i].name.contains(filterText))
				filteredIndexes.add(i);
		}
	}
	else
	{
		filteredIndexes.clear();

		filteredIndexes.ensureStorageAllocated(allVariableLines.size());

		for (int i = 0; i < allVariableLines.size(); i++)
		{
			filteredIndexes.add(i);
		}
	}

	table->updateContent();

	if (resizeOnChange)
	{
		auto preferredHeight = 24 + table->getHeaderHeight() + table->getNumRows() * table->getRowHeight();

		setSize(getWidth(), preferredHeight);
	}

	repaint();
}


void ScriptWatchTable::refreshChangeStatus()
{
	if(auto provider = getProviderBase())
	{
		BigInteger lastChanged = changed;
		changed = 0;

		for (int i = 0; i < filteredIndexes.size(); i++)
		{
			const int indexInAllLines = filteredIndexes[i];

			auto *info = provider->getDebugInformation(indexInAllLines);

			if (info != nullptr)
			{
				const String currentValue = info->getTextForValue();
				const String oldValue = allVariableLines[indexInAllLines].value;

				if (currentValue != oldValue && isPositiveAndBelow(indexInAllLines, allVariableLines.size()))
				{
					allVariableLines.getReference(indexInAllLines).value = currentValue;
					changed.setBit(i, true);
				}
			}
		}

		if (lastChanged != changed || changed != 0) repaint();
	}
};


void ScriptWatchTable::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		auto info = getDebugInformationForRow(table->getSelectedRow(0));

		if (info != nullptr)
		{
			info->rightClickCallback(e, table);
		}
	}
}


void ScriptWatchTable::mouseDoubleClick(const MouseEvent &e)
{
	if (auto info = getDebugInformationForRow(table->getSelectedRow(0)))
	{
		info->doubleClickCallback(e, this);
	}
}

void ScriptWatchTable::paint(Graphics &g)
{
	g.setColour(Colour(0xff353535));
	g.fillRect(0.0f, 0.0f, (float)getWidth(), 25.0f);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.5f), 0.0f, 25.0f,
		Colours::transparentBlack, 0.0f, 30.0f, false));
	g.fillRect(0.0f, 25.0f, (float)getWidth(), 25.0f);

	g.setColour(Colour(0xFF3D3D3D));

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff262626)));
	g.fillRect(0, 25, getWidth(), getHeight());

	g.setColour(Colours::white.withAlpha(0.6f));

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(searchIcon, sizeof(searchIcon));
	path.applyTransform(AffineTransform::rotation(float_Pi));

	const float xOffset = 0.0f;

	path.scaleToFit(xOffset + 4.0f, 4.0f, 16.0f, 16.0f, true);

	g.fillPath(path);
}

DebugInformationBase* ScriptWatchTable::getDebugInformationForRow(int rowIndex)
{
	auto realIndex = filteredIndexes[rowIndex];

	if(auto p = getProviderBase())
		return p->getDebugInformation(realIndex);

	return nullptr;
}

void ScriptWatchTable::setHolder(ApiProviderBase::Holder* h)
{
	deregisterAtHolder();
	holder = h;
	registerAtHolder();

	setName(getHeadline());

	if(holder.get() != nullptr)
	{
		rebuildLines();
		startTimer(400);
	}
	else
	{
		allVariableLines.clear();
		table->updateContent();
		stopTimer();
		repaint();
	}

	if(getParentComponent() != nullptr) 
		getParentComponent()->repaint();

}
	
void ScriptWatchTable::paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) 
{
	if(rowNumber % 2) g.fillAll(Colours::white.withAlpha(0.05f));

    if (rowIsSelected)
        g.fillAll (Colour(0x44000000));
}

void ScriptWatchTable::selectedRowsChanged(int /*lastRowSelected*/) {};

void ScriptWatchTable::paintCell (Graphics& g, int rowNumber, int columnId,
                int width, int height, bool /*rowIsSelected*/) 
{
	g.setColour(Colours::black.withAlpha(0.1f));

	g.drawHorizontalLine(0, 0.0f, (float)width);
	
	g.setColour (Colours::white.withAlpha(.8f));
    g.setFont (GLOBAL_FONT());

	const int indexInAllLines = filteredIndexes[rowNumber];

	

    if(auto pr = getProviderBase())
	{
		auto info = allVariableLines[indexInAllLines];

        String text;

		if (columnId == Type)
		{

			const float alpha = 0.4f;
			const float brightness = 0.6f;
			const float h = jmin<float>((float)height, (float)width) - 4.0f;
			const Rectangle<float> area(((float)width - h) / 2.0f, 2.0f, h, h);

			Colour colour;

			char c;

			pr->getColourAndLetterForType(info.type, colour, c);

			g.setColour(colour);

			g.fillRoundedRectangle(area, 5.0f);
			g.setColour(Colours::white.withAlpha(0.4f));
			g.drawRoundedRectangle(area, 5.0f, 1.0f);
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);

			String type;
			type << c;
			g.drawText(type, area, Justification::centred);
		}
		else
		{
			if (columnId == ColumnId::DataType)
				text << info.dataType;
			else if (columnId == ColumnId::Name)
				text << info.name;
			else
				text << info.value;

			g.setColour(changed[rowNumber] ? Colours::orangered : Colours::white);
			g.setFont(GLOBAL_MONOSPACE_FONT());
			g.drawText(text, 5, 0, width - 10, height, Justification::centredLeft, true);
		}
	}
}

String ScriptWatchTable::getHeadline() const
{  
	String x;
        
	x << "Watch Script Variable";
	return x;
}

    
void ScriptWatchTable::resized()
{
	table->getHeader().resizeAllColumnsToFit(getWidth());

	table->setBounds(0, 24, getWidth(), jmax<int>(0, getHeight() - 24));
	fuzzySearchBox->setBounds(24, 0, getWidth()-24, 23);
}

} // namespace hise
