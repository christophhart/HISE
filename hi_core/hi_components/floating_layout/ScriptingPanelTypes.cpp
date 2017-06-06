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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



CodeEditorPanel::CodeEditorPanel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{
	tokeniser = new JavascriptTokeniser();


}

CodeEditorPanel::~CodeEditorPanel()
{
	tokeniser = nullptr;
}


Component* CodeEditorPanel::createContentComponent(int index)
{
	auto p = dynamic_cast<JavascriptProcessor*>(getProcessor());

	const bool isCallback = index < p->getNumSnippets();

	if (isCallback)
	{
		auto pe = new PopupIncludeEditor(p, p->getSnippet(index)->getCallbackName());
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());
		return pe;
	}
	else
	{
		const int fileIndex = index - p->getNumSnippets();

		auto pe = new PopupIncludeEditor(p, p->getWatchedFile(fileIndex));
		getProcessor()->getMainController()->setLastActiveEditor(pe->getEditor(), CodeDocument::Position());

		return pe;
	}
}

Identifier CodeEditorPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

void CodeEditorPanel::fillIndexList(StringArray& indexList)
{
	auto p = dynamic_cast<JavascriptProcessor*>(getConnectedProcessor());

	if (p != nullptr)
	{
		for (int i = 0; i < p->getNumSnippets(); i++)
		{
			indexList.add(p->getSnippet(i)->getCallbackName().toString());
		}

		for (int i = 0; i < p->getNumWatchedFiles(); i++)
		{
			indexList.add(p->getWatchedFile(i).getFileName());
		}
	}
}

void CodeEditorPanel::gotoLocation(Processor* p, const String& fileName, int charNumber)
{
	if (fileName.isEmpty())
	{
		setContentWithUndo(p, 0);
	}
	else
	{
		auto jp = dynamic_cast<JavascriptProcessor*>(p);

		int fileIndex = -1;

		for (int i = 0; i < jp->getNumWatchedFiles(); i++)
		{
			if (jp->getWatchedFile(i).getFullPathName() == fileName)
			{
				fileIndex = i;
				break;
			}
		}

		if (fileIndex != -1)
		{
			setContentWithUndo(p, jp->getNumSnippets() + fileIndex);
		}
		else
			return;
	}

	auto editor = getContent<PopupIncludeEditor>()->getEditor();

	CodeDocument::Position pos(editor->getDocument(), charNumber);
	editor->scrollToLine(jmax<int>(0, pos.getLineNumber()));
}

ConsolePanel::ConsolePanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setInterceptsMouseClicks(false, true);
	addAndMakeVisible(console = new Console(getRootWindow()->getBackendProcessor()));
}

void ConsolePanel::resized()
{
	console->setBounds(getParentShell()->getContentBounds());
}




void ScriptContentPanel::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (processor == dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()))
	{
		resized();
	}
}

Identifier ScriptContentPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

Component* ScriptContentPanel::createContentComponent(int /*index*/)
{
	return new Editor(getConnectedProcessor());
}

struct ScriptContentPanel::Canvas : public ScriptEditHandler,
	public Component
{
	Canvas(Processor* p) :
		processor(p)
	{
		addAndMakeVisible(content = new ScriptContentComponent(dynamic_cast<ProcessorWithScriptingContent*>(p)));
		addAndMakeVisible(overlay = new ScriptingContentOverlay(this));

        
		setSize(content->getContentWidth() + 40, content->getContentHeight() + 40);
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black);

		g.setColour(Colours::white.withAlpha(0.5f));

		g.drawHorizontalLine(20, 10.0f, 20.0f);
		g.drawHorizontalLine(20, (float)getWidth() - 20.0f, (float)getWidth() - 10.0f);

		g.drawHorizontalLine(getHeight() - 20, 10.0f, 20.0f);
		g.drawHorizontalLine(getHeight() - 20, (float)getWidth() - 20.0f, (float)getWidth() - 10.0f);

		g.drawVerticalLine(20, 10.0f, 20.0f);
		g.drawVerticalLine(20, (float)getHeight() - 20.0f, (float)getHeight() - 10.0f);

		g.drawVerticalLine(getWidth() - 20, 10.0f, 20.0f);
		g.drawVerticalLine(getWidth() - 20, (float)getHeight() - 20.0f, (float)getHeight() - 10.0f);
	}

	void selectOnInitCallback() override
	{

	}


	void scriptEditHandlerCompileCallback()
	{
		if (getScriptEditHandlerProcessor())
			getScriptEditHandlerProcessor()->compileScript();
	}

public:

	virtual ScriptContentComponent* getScriptEditHandlerContent() { return content; }

	virtual ScriptingContentOverlay* getScriptEditHandlerOverlay() { return overlay; }

	virtual JavascriptCodeEditor* getScriptEditHandlerEditor()
	{
		if (processor.get())
		{
			return dynamic_cast<JavascriptCodeEditor*>(processor->getMainController()->getLastActiveEditor());
		}

		return nullptr;
	}

	virtual JavascriptProcessor* getScriptEditHandlerProcessor() { return dynamic_cast<JavascriptProcessor*>(processor.get()); }

	void resized() override
	{
		content->setBounds(20, 20, getWidth() - 40, getHeight() - 40);
		overlay->setBounds(20, 20, getWidth() - 40, getHeight() - 40);
	}

private:

	ScopedPointer<ScriptContentComponent> content;
	ScopedPointer<ScriptingContentOverlay> overlay;
	WeakReference<Processor> processor;

};



ScriptContentPanel::Editor::Editor(Processor* p)
{
	addAndMakeVisible(viewport = new Viewport());

	viewport->setViewedComponent(new Canvas(p), true);
}


Identifier ScriptWatchTablePanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

Component* ScriptWatchTablePanel::createContentComponent(int /*index*/)
{
	setStyleProperty(showConnectionBar, false);

	auto swt = new ScriptWatchTable(getRootWindow());

	swt->setScriptProcessor(dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()), nullptr);

	return swt;
}

void ConnectorHelpers::tut(PanelWithProcessorConnection* connector, const Identifier &idToSearch)
{
    auto parentContainer = connector->getParentShell()->getParentContainer();
    
    
    FloatingTile::Iterator<PanelWithProcessorConnection> iter(parentContainer->getParentShell());
    
    while (auto p = iter.getNextPanel())
    {
        if (p == connector)
            continue;
        
        if (p->getProcessorTypeId() != idToSearch)
            continue;
        
        p->setContentWithUndo(connector->getProcessor(), 0);
    }
}

