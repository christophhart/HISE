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

namespace hise {
using namespace juce;

void TableEditor::removeProcessorConnection()
{
	if (LookupTableProcessor *ltp = dynamic_cast<LookupTableProcessor*>(connectedProcessor.get()))
	{
		ltp->removeTableChangeListener(this);
	}
}


bool TableEditor::isInMainPanelInternal() const
{
	return findParentComponentOfClass<ScriptContentComponent>() == nullptr;
}

void TableEditor::connectToLookupTableProcessor(Processor *p)
{
	if (p == connectedProcessor) return;

	if (auto ltp = dynamic_cast<LookupTableProcessor*>(connectedProcessor.get()))
	{
		ltp->removeTableChangeListener(this);
	}

	if (p == nullptr)
	{
		connectedProcessor = nullptr;
		createDragPoints();
		refreshGraph();
	}

	if (LookupTableProcessor * ltp = dynamic_cast<LookupTableProcessor*>(p))
	{
		connectedProcessor = p;


		fontToUse = p->getMainController()->getFontFromString("Default", 14.0f);

		ltp->addTableChangeListener(this);
	}
}

void TableEditor::updateFromProcessor(SafeChangeBroadcaster* b)
{
	if (dynamic_cast<LookupTableProcessor::TableChangeBroadcaster*>(b) != nullptr)
	{
		LookupTableProcessor::TableChangeBroadcaster * tcb = dynamic_cast<LookupTableProcessor::TableChangeBroadcaster*>(b);

		if (tcb->table == editedTable)
		{
			setDisplayedIndex(tcb->tableIndex);
		}
	}
}



}