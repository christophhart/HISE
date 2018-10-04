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

class PluginPreviewWindow::Content::ScriptDeleteListener : public Processor::DeleteListener
{
public:

	ScriptDeleteListener(Content* parent_, Processor* processor) :
		parent(parent_),
		p(processor)
	{
		if(p != nullptr)
			p->addDeleteListener(this);

	}

	~ScriptDeleteListener()
	{
		if(p != nullptr)
			p->removeDeleteListener(this);
	}

	void processorDeleted(Processor* /*deletedProcessor*/) override
	{
		
		auto popup = parent->findParentComponentOfClass<FloatingTilePopup>();
		
		if(popup)
			popup->deleteAndClose();
	}

	void updateChildEditorList(bool /*forceUpdate*/) override {};


public:

	WeakReference<Processor> p;

	Content* parent;

};

PluginPreviewWindow::PluginPreviewWindow(BackendProcessorEditor *editor_) :
DocumentWindow("Preview: : " + editor_->getMainSynthChain()->getId(), Colours::black, DocumentWindow::closeButton, true),
editor(editor_)
{
	
	setContentOwned(new Content(editor), true);

	setUsingNativeTitleBar(true);

	centreWithSize(getContentComponent()->getWidth(), getContentComponent()->getHeight());

	setResizable(false, false);

	setVisible(true);
}


void PluginPreviewWindow::closeButtonPressed()
{
	if (editor.getComponent() != nullptr)
	{
		editor->setPluginPreviewWindow(nullptr);
	}
}

PluginPreviewWindow::Content* PluginPreviewWindow::createContent(BackendProcessorEditor* editor)
{
	return new Content(editor);
}

PluginPreviewWindow::Content::Content(BackendProcessorEditor *editor_) :
editor(editor_),
mainSynthChain(editor->getMainSynthChain())
{

	setName("Interface Preview");

	setOpaque(true);

	Processor::Iterator<JavascriptMidiProcessor> iter(mainSynthChain);

	while (auto m = iter.getNextProcessor())
	{
		if (m->isFront())
		{
			scriptProcessor = m;
			break;
		}
	}

	if (scriptProcessor != nullptr)
	{
		addAndMakeVisible(content = new ScriptContentComponent(scriptProcessor));

		deleteListener = new ScriptDeleteListener(this, scriptProcessor);
	}
	else
	{
		jassertfalse;
	}


	setSize(content->getContentWidth(), content->getContentHeight());
    
}

PluginPreviewWindow::Content::~Content()
{
	content = nullptr;
	editor = nullptr;
}

void PluginPreviewWindow::Content::resized()
{
#if HISE_IOS
	container->setBounds(0, 0, content->getContentWidth(), content->getContentHeight());
#else
	content->setBounds(2, 0, content->getContentWidth(), content->getContentHeight());
#endif
}

} // namespace hise
