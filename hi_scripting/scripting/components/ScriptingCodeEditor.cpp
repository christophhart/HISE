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

#include <regex>

namespace hise { using namespace juce;


DebugConsoleTextEditor::DebugConsoleTextEditor(const String& name, Processor* p) :
	TextEditor(name),
	processor(p)
{
	setMultiLine(false);
	setReturnKeyStartsNewLine(false);
	setScrollbarsShown(false);
	setPopupMenuEnabled(false);
	setColour(TextEditor::textColourId, Colours::white);
	setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	setColour(TextEditor::backgroundColourId, Colour(0x00ffffff));
	setColour(TextEditor::highlightColourId, Colour(0x40ffffff));
	setColour(TextEditor::shadowColourId, Colour(0x00000000));
	setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::white.withAlpha(0.1f));
	setText(String());

	setFont(GLOBAL_MONOSPACE_FONT());
	setLookAndFeel(&laf2);
	setColour(Label::ColourIds::textColourId, Colours::white);

	addListener(this);

	p->getMainController()->addScriptListener(this);

	scriptWasCompiled(dynamic_cast<JavascriptProcessor*>(p));
}

DebugConsoleTextEditor::~DebugConsoleTextEditor()
{
	setLookAndFeel(nullptr);

	if (processor != nullptr)
	{
		processor->getMainController()->removeScriptListener(this);
	}
}

void DebugConsoleTextEditor::scriptWasCompiled(JavascriptProcessor *jp)
{
	if (dynamic_cast<Processor*>(jp) == processor)
	{
		auto r = jp->getLastErrorMessage();



		if (r.wasOk()) setText("Compiled OK", dontSendNotification);
		else
        {
            fullErrorMessage = r.getErrorMessage().upToFirstOccurrenceOf("\n", false, false);
            
            setText(fullErrorMessage.upToFirstOccurrenceOf("{", false, false), dontSendNotification);
        }

		setColour(TextEditor::backgroundColourId, r.wasOk() ? Colours::green.withBrightness(0.1f) : Colours::red.withBrightness((0.1f)));
	}
}

bool DebugConsoleTextEditor::keyPressed(const KeyPress& k)
{
	if (k == KeyPress::upKey)
	{
		currentHistoryIndex = jmax<int>(0, currentHistoryIndex - 1);

		setText(history[currentHistoryIndex], dontSendNotification);

		return true;
	}
	else if (k == KeyPress::downKey)
	{
		currentHistoryIndex = jmin<int>(history.size() - 1, currentHistoryIndex + 1);
		setText(history[currentHistoryIndex], dontSendNotification);
	}

	return TextEditor::keyPressed(k);
}

void DebugConsoleTextEditor::mouseDown(const MouseEvent& e)
{
	if(!getText().containsChar('>'))
        setText("> ", dontSendNotification);

	TextEditor::mouseDown(e);
}

void DebugConsoleTextEditor::mouseDoubleClick(const MouseEvent& /*e*/)
{
	gotoText();
}

void DebugConsoleTextEditor::gotoText()
{
	DebugableObject::Helpers::gotoLocation(processor->getMainController()->getMainSynthChain(), fullErrorMessage);
}

void DebugConsoleTextEditor::addToHistory(const String& s)
{
	if (!history.contains(s))
	{
		history.add(s);
		currentHistoryIndex = history.size() - 1;
	}
	else
	{
		history.move(history.indexOf(s), history.size() - 1);
	}
}

void DebugConsoleTextEditor::textEditorReturnKeyPressed(TextEditor& /*t*/)
{
	String codeToEvaluate = getText();

	addToHistory(codeToEvaluate);

	if (codeToEvaluate.startsWith("> "))
	{
		codeToEvaluate = codeToEvaluate.substring(2);
	}

	auto jsp = dynamic_cast<JavascriptProcessor*>(processor.get());

	if(codeToEvaluate.startsWith("goto "))
	{
		auto d = codeToEvaluate.substring(5, 1000000);
		auto tokens = StringArray::fromTokens(d, "@", "");

		DebugableObject::Location loc;
		loc.charNumber = tokens[1].getIntValue();
		loc.fileName = tokens[0];

		DebugableObject::Helpers::gotoLocation(this, jsp, loc);
		return;
	}
    
    processor->getMainController()->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::Compilation,
                                                                     jsp, [codeToEvaluate](JavascriptProcessor* p)
    {
        Result r = Result::ok();
        HiseJavascriptEngine* engine = p->getScriptEngine();
        
        var returnValue = engine->evaluate(codeToEvaluate, &r);

        auto pr = dynamic_cast<Processor*>(p);
        
        if (r.wasOk())
        {
            pr->getMainController()->writeToConsole("> " + returnValue.toString(), 0, nullptr);
        }
        else
        {
            debugToConsole(pr, r.getErrorMessage());
        }
        
        return r;
    });
}


struct InteractiveEditor : public MarkdownCodeComponentBase,
	public ControlledObject
{
	void initialiseEditor() override
	{
		if (getMainController() != nullptr && Helpers::createProcessor(syntax))
		{
			jp = new JavascriptMidiProcessor(getMainController(), "TestProcessor");
			jp->setOwnerSynth(getMainController()->getMainSynthChain());

			if (Helpers::createContent(syntax))
			{
				scriptContent = new ScriptContentComponent(jp);
				addAndMakeVisible(scriptContent);
			}
				
			usedDocument = jp->getSnippet(0);
			usedDocument->replaceAllContent(ownedDoc->getAllContent());

			editor = new JavascriptCodeEditor(*usedDocument, tok, jp, "onInit");

			editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(fontSize));

			addBottomRow();
		}
		else
		{
			MarkdownCodeComponentBase::initialiseEditor();

			if (syntax == EditableFloatingTile)
			{
				editor->setReadOnly(false);

				auto data = JSON::parse(ownedDoc->getAllContent());

				parent->addAndMakeVisible(this);

				addAndMakeVisible(floatingTile = new FloatingTile(getMainController(), nullptr, {}));
				floatingTile->setOpaque(true);

				floatingTile->setContent(data);

				addBottomRow();

				if (auto pc = dynamic_cast<PanelWithProcessorConnection*>(floatingTile->getCurrentFloatingPanel()))
				{
					floatingTileProcessor = pc->createDummyProcessorForDocumentation(getMainController());
					pc->setContentWithUndo(floatingTileProcessor, 0);
					floatingTile->getCurrentFloatingPanel()->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF363636));
				}
			}
		}
	}

	InteractiveEditor(SyntaxType syntax, String code, float width, float fontsize, MainController* mc, Component* parent_, MarkdownParser* parser) :
		MarkdownCodeComponentBase(syntax, code, width, fontsize, parser),
		ControlledObject(mc),
		parent(parent_),
		copyButton("copy", this, f)
	{
		initialiseEditor();
		createChildComponents();

		addAndMakeVisible(copyButton);

		if (syntax == ScriptContent)
		{
			showContentOnly = true;
			editor->setVisible(false);
			runButton->triggerClick();
		}

		setWantsKeyboardFocus(true);
	}

	~InteractiveEditor()
	{
		floatingTile = nullptr;
		floatingTileProcessor = nullptr;
		scriptContent = nullptr;
		editor = nullptr;
		jp = nullptr;
		tok = nullptr;
		ownedDoc = nullptr;
	}

	void addBottomRow()
	{
		addAndMakeVisible(runButton = new TextButton("Run"));
		addAndMakeVisible(resultLabel = new Label());
		runButton->setLookAndFeel(&blaf);
		runButton->addListener(this);
		resultLabel->setColour(Label::ColourIds::backgroundColourId, Colour(0xff363636));
		resultLabel->setColour(Label::ColourIds::textColourId, Colours::white);
		resultLabel->setFont(GLOBAL_MONOSPACE_FONT());
		resultLabel->setText("Click Run to evaluate this code", dontSendNotification);
	}

	bool keyPressed(const KeyPress& key) override
	{
		if (key == KeyPress::F5Key && runButton != nullptr)
		{
			runButton->triggerClick();
			return true;
		}

		return false;
	}

	void buttonClicked(Button* b) override // TODO: => base class
	{
		MarkdownCodeComponentBase::buttonClicked(b);

		if (b == &copyButton)
		{
			SystemClipboard::copyTextToClipboard(editor->getDocument().getAllContent());
			return;
		}


		if (jp != nullptr)
		{
			auto f2 = [this](const JavascriptProcessor::SnippetResult& result)
			{
				Component::SafePointer<InteractiveEditor> tmp = this;

				auto fc = [tmp, result]()
				{
					if (tmp.getComponent() == nullptr)
						return;

					tmp.getComponent()->isActive = true;
					String s;

					if (result.r.ok())
					{
						auto consoleContent = tmp.getComponent()->jp->getMainController()->getConsoleHandler().getConsoleData()->getAllContent();
						s = consoleContent.removeCharacters("\n").fromFirstOccurrenceOf(":", false, false);
					}
					else
						s = "Error: " + result.r.getErrorMessage();



					tmp.getComponent()->resultLabel->setText(s, dontSendNotification);
					tmp.getComponent()->repaint();

					tmp.getComponent()->updateHeightInParent();
				};

				new DelayedFunctionCaller(fc, 100);

			};

			jp->getMainController()->getConsoleHandler().clearConsole();
			jp->compileScript(f2);
		}
		else if (floatingTile != nullptr)
		{
			auto value = JSON::parse(editor->getDocument().getAllContent());

			floatingTile->setContent(value);

			if (auto pc = dynamic_cast<PanelWithProcessorConnection*>(floatingTile->getCurrentFloatingPanel()))
			{
				pc->setContentWithUndo(floatingTileProcessor, 0);
			}
		}
	}

	bool autoHideEditor() const override
	{
		return !isExpanded && (floatingTile != nullptr || usedDocument->getNumLines() > 20);
	}

	int getPreferredHeight() const
	{
		if (showContentOnly)
		{
			return scriptContent->getContentHeight() + 20;
		}
		else
		{
			int y = scriptContent != nullptr ? (scriptContent->getContentHeight() + 20) : 0;

			if (floatingTile != nullptr)
			{
				int size = floatingTile->getCurrentFloatingPanel()->getPreferredHeight();

				if (size == 0)
					size = 400;

				y += size;
				y += 2 * getGutterWidth();
			}

			if (runButton != nullptr)
				y += (autoHideEditor() ? 2 : 1) * editor->getLineHeight();

			y += MarkdownCodeComponentBase::getPreferredHeight();

			return y;
		}
	}



	void resized() override
	{
		if (showContentOnly)
		{
			scriptContent->setBounds(10, 10, getWidth(), scriptContent->getContentHeight());
			editor->setVisible(false);
			expandButton->setVisible(false);
			runButton->setVisible(false);
			resultLabel->setVisible(false);
		}
		else
		{
			int y = 0;

			if (scriptContent != nullptr)
			{

				scriptContent->setBounds(10, 10, getWidth(), scriptContent->getContentHeight());
				y += scriptContent->getContentHeight() + 20;
			}

			if (floatingTile != nullptr)
			{
				int size = floatingTile->getCurrentFloatingPanel()->getPreferredHeight();;

				if (size == 0)
					size = 400;

				y += size;

				auto gWidth = getGutterWidth();

				floatingTile->setBounds(gWidth, gWidth, getWidth() - 2 * gWidth, size);

				y += 2 * gWidth;
			}

			editor->scrollToLine(0);

			if (autoHideEditor())
			{
				o.setVisible(true);
				expandButton->setVisible(true);

				editor->setSize(getWidth(), 2 * editor->getLineHeight());
				editor->setTopLeftPosition(0, y + editor->getLineHeight() / 2);

				auto b = editor->getBounds();

				b.removeFromLeft(getGutterWidth());

				expandButton->setBounds(b.withSizeKeepingCentre(130, editor->getLineHeight()));
				editor->setEnabled(false);

				auto ob = editor->getBounds();
				ob.removeFromLeft(getGutterWidth());

				o.setBounds(ob);
			}
			else
			{
				y += editor->getLineHeight();

				editor->setSize(getWidth(), getEditorHeight());
				editor->setTopLeftPosition(0, y);

				o.setVisible(false);

				expandButton->setVisible(false);
				editor->setEnabled(true);
			}

			if (runButton != nullptr)
			{
				auto ar = getLocalBounds();
				ar = ar.removeFromBottom(editor->getLineHeight());
				runButton->setBounds(ar.removeFromLeft(getGutterWidth()));
				resultLabel->setBounds(ar);
			}

			copyButton.setBounds(Rectangle<int>(0, y, 24, 24).reduced(4));
		}
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xff363636));

		auto b = getLocalBounds();
		b.removeFromLeft(getGutterWidth());

		g.setColour(editor->findColour(CodeEditorComponent::ColourIds::backgroundColourId));
		g.fillRect(b);

		if (floatingTile != nullptr)
		{
			g.setColour(Colour(0xff363636));
			g.fillRect(0, 0, getWidth(), floatingTile->getHeight() + 2 * getGutterWidth());
		}

		if (scriptContent != nullptr)
		{
			auto h = getLocalBounds().removeFromTop(scriptContent->getContentHeight() + 20);

			g.setColour(Colour(0xff363636));
			g.fillRect(h);

			if (!isActive)
			{
				g.setFont(GLOBAL_BOLD_FONT());
				g.setColour(Colours::white.withAlpha(0.5f));
				g.drawText("Press run to show the result", h.toFloat(), Justification::centred);
			}

		}

	}

	ScopedPointer<ScriptContentComponent> scriptContent;
	ScopedPointer<JavascriptMidiProcessor> jp;

	Component* parent = nullptr;

	Rectangle<float> pathBounds;

	ScopedPointer<FloatingTile> floatingTile;
	ScopedPointer<Processor> floatingTileProcessor;

	ScopedPointer<TextButton> runButton;
	ScopedPointer<Label> resultLabel;

	bool isActive = false;
	bool showContentOnly = false;

	HiseShapeButton copyButton;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractiveEditor);
};

#if HI_MARKDOWN_ENABLE_INTERACTIVE_CODE
hise::MarkdownCodeComponentBase* MarkdownCodeComponentFactory::createInteractiveEditor(MarkdownParser* parent, MarkdownCodeComponentBase::SyntaxType syntax, const String& code, float width)
{
	auto mainController = dynamic_cast<MainController*>(parent->getHolder());
	auto parentComponent = dynamic_cast<MarkdownRenderer*>(parent)->getTargetComponent();

	if (mainController == nullptr)
	{
		if (auto cObj = dynamic_cast<ControlledObject*>(parent->getHolder()))
			mainController = cObj->getMainController();
	}
		
	jassert(mainController != nullptr);
	
	return new InteractiveEditor(syntax, code, width, parent->getStyleData().fontSize, mainController, parentComponent, parent);
}
#endif


} // namespace hise
