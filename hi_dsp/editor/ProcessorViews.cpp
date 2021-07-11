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

ViewInfo::ViewInfo(ModulatorSynthChain *synthChain, Processor *rootProcessor_, const String &viewName_):
		rootProcessor(rootProcessor_),
		mainSynthChain(synthChain),
		viewName(viewName_),
		changed(false)
{
	Processor::Iterator<Processor> iter(synthChain, false);

	Processor *p;

	while((p = iter.getNextProcessor()) != nullptr)
	{
		ProcessorView *pv = new ProcessorView();

		pv->processorId = p->getId();
		pv->editorState = p->getCompleteEditorState();

		Chain *c = dynamic_cast<Chain*>(p);

		const bool isChainOfModulatorSynth = c != nullptr && c->getParentProcessor() == nullptr; // ModulatorSynthChains have nullptr as ParentProcessor...

		if(c != nullptr && !isChainOfModulatorSynth)
		{
			pv->parentId = c->getParentProcessor()->getId();
		}
		else
			pv->parentId = String();

		states.add(pv);
	}
};



void ViewInfo::restoreFromValueTree(const ValueTree &v)
{
	// Restore the root processor
	const String rootId = v.getProperty("r", "");

	rootProcessor = ProcessorHelpers::getFirstProcessorWithName(mainSynthChain, rootId);

	// Restore the name

	viewName = v.getProperty("n", "UnnamedView");

	// Restore the view list

	states.clear();

	for(int i = 0; i < v.getNumChildren(); i++)
	{
		ValueTree child = v.getChild(i);

		ProcessorView *view = new ProcessorView();

		view->processorId = child.getProperty("i", "");
		view->parentId = child.getProperty("p", "");
		view->editorState = XmlDocument::parse(child.getProperty("s", "")).release();

		states.add(view);
	}
};

ValueTree ViewInfo::exportAsValueTree() const 
{
	// This must be an initialized ViewInfo!
	jassert(rootProcessor != nullptr);

	if (rootProcessor.get() != nullptr)
	{
		ValueTree v("vi");

		v.setProperty("n", viewName, nullptr);
		v.setProperty("m", mainSynthChain->getId(), nullptr);
		v.setProperty("r", rootProcessor->getId(), nullptr);

		for (int i = 0; i < states.size(); i++)
		{
			ProcessorView *view = states[i];

			ValueTree child("v");
			child.setProperty("s", view->editorState != nullptr ? view->editorState->createDocument("") : "", nullptr);
			child.setProperty("p", view->parentId, nullptr);
			child.setProperty("i", view->processorId, nullptr);



			v.addChild(child, -1, nullptr);
		}

		return v;
	}
	else
	{
		debugError(mainSynthChain, "Error at saving view!");
		
		ValueTree v("vi");
		
		return v;
	}

	
};

void ViewInfo::restoreViewInfoList(ModulatorSynthChain *synthChain, const String &encodedViewInfo, OwnedArray<ViewInfo> &infoArray)
{
	MemoryBlock block;

	block.fromBase64Encoding(encodedViewInfo);

	ValueTree infoList = ValueTree::readFromData(block.getData(), block.getSize());

	infoArray.clear();

	for(int i = 0; i < infoList.getNumChildren(); i++)
	{
		ViewInfo *info = new ViewInfo(synthChain);
		info->restoreFromValueTree(infoList.getChild(i));

		infoArray.add(info);
	}
};

String ViewInfo::exportViewInfoList(const OwnedArray<ViewInfo> &infoArray)
{
	MemoryBlock block;

	ValueTree l("list");

	for(int i = 0; i < infoArray.size(); i++)
	{
		ValueTree v = infoArray[i]->exportAsValueTree();
		
		l.addChild(v, -1, nullptr);

	}

	MemoryOutputStream mos(block, true);

	l.writeToStream(mos);

	String x = block.toBase64Encoding();

	return x;
};

void ViewInfo::restore()
{
	if(mainSynthChain != nullptr)
	{
		Processor::Iterator<Processor> iter(mainSynthChain, false);

		Processor *p;

		while((p = iter.getNextProcessor()) != nullptr)
		{
			for(int i = 0; i < states.size(); i++)
			{
				Chain *c = dynamic_cast<Chain*>(p);

				bool chainHasCorrectParent = true;

				const bool isChainOfModulatorSynth = c != nullptr && c->getParentProcessor() == nullptr; // ModulatorSynthChains have nullptr as ParentProcessor...

				if(c != nullptr && !isChainOfModulatorSynth)
				{
					chainHasCorrectParent = c->getParentProcessor()->getId() == states[i]->parentId;
				}

				const bool processorIsCorrect =  p->getId() == states[i]->processorId;

				if(chainHasCorrectParent && processorIsCorrect)
				{
					p->restoreCompleteEditorState(states[i]->editorState);
					break;
				}
			}
		}

		markAsUnchanged();
	}
}

ViewManager::ViewManager(ModulatorSynthChain *chain_, UndoManager *undoManager):
	chain(chain_),
	root(chain_),
    currentViewInfo(-1),
	viewUndoManager(undoManager)
{};

void ViewManager::clearAllViews()
{
    viewInfos.clear();
    currentViewInfo = -1;
    root = chain;
}

bool ViewBrowsing::perform()
{
	oldRoot = viewManager->getRootProcessor();
	viewManager->setRootProcessor(newRoot);

	BackendProcessorEditor *bpe = dynamic_cast<BackendProcessorEditor*>(editor.getComponent());

	if (bpe != nullptr)
	{
		bpe->setRootProcessor(newRoot);
		return true;
	}
	
	return false;
}

bool ViewBrowsing::undo()
{
	if (oldRoot.get() != nullptr)
	{
		BackendProcessorEditor *bpe = dynamic_cast<BackendProcessorEditor*>(editor.getComponent());

		if (bpe != nullptr)
		{
			bpe->setRootProcessor(oldRoot, scrollY);
			return true;
		}
		else return false;
	}
	else
	{
		return false;
	}
}

} // namespace hise