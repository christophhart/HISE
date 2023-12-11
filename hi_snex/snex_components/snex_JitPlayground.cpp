/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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






namespace snex {
namespace jit {
using namespace juce;

SnexPlayground::SnexPlayground(ui::WorkbenchData* data, bool isTestMode) :
	ui::WorkbenchComponent(data, true),
	bpProvider(getGlobalScope()),
	mclDoc(doc),
	editor(mclDoc),
	assembly(assemblyDoc, &assemblyTokeniser),
	console(consoleContent, &consoleTokeniser),
	snexIcon(factory.createPath("snex")),
	showInfo("optimise", this, factory),
	showAssembly("asm", this, factory),
	showConsole("console", this, factory),
	showWatch("watch", this, factory),
	bugButton("debug", this, factory),
	spacerAssembly("Assembly"),
	spacerConsole("Console"),
	spacerInfo("Info"),
	spacerWatch("Data Table"),
	compileButton("Compile"),
	resumeButton("Resume"),
	conditionUpdater(*this),
	testMode(isTestMode)
{
	doc.replaceAllContent(getWorkbench()->getCode());
	doc.clearUndoHistory();
	
	watchTable.setHolder(getWorkbench());

	editor.addBreakpointListener(this);

	stateViewer = new ui::OptimizationProperties(getWorkbench());

	auto& ed = editor.editor;

	ed.setPopupLookAndFeel(new hise::PopupLookAndFeel());

	ed.setLanguageManager(new debug::SnexLanguageManager(doc, getGlobalScope()));

	setName("SNEX Editor");

	ed.addPopupMenuFunction([this](mcl::TextEditor& te, PopupMenu& m, const MouseEvent& e) { addPopupMenuItems(te, m, e); }, BIND_MEMBER_FUNCTION_2(SnexPlayground::performPopupMenu));

	auto& d = doc;
	ed.setGotoFunction([&d](int lineNumber, const String& token)
	{
		mcl::Selection selection;

		GlobalScope s;
		jit::Compiler c(s);

		Types::SnexObjectDatabase::registerObjects(c, 2);

		c.compileJitObject(d.getAllContent());
		auto l = c.getNamespaceHandler().getDefinitionLine(lineNumber, token);

		if (l != -1)
			return l;

		Preprocessor pp(d.getAllContent());
		pp.process();
		for (auto l : pp.getAutocompleteData())
		{
			if (l.name.upToFirstOccurrenceOf("(", false, false) == token)
				return l.lineNumber + 1;
		}

		return -1;
	});

	addAndMakeVisible(editor);
	addAndMakeVisible(console);

	if (true)
	{
		

		assembly.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0));
		assembly.setFont(GLOBAL_MONOSPACE_FONT());
		assembly.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0));
		assembly.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
		assembly.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
		assembly.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));
		assembly.setReadOnly(true);
		assemblyDoc.replaceAllContent("; no assembly generated");
		assembly.setOpaque(false);

		console.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xFF555555));
		console.setFont(GLOBAL_MONOSPACE_FONT());
		console.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0x22000000));
		console.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
		console.setOpaque(false);
		console.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

		console.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));
		console.setReadOnly(true);
		console.setLineNumbersShown(false);
		
	}

	CodeEditorComponent::ColourScheme scheme;

	scheme.set("Error", Colour(0xffFF8888));
	scheme.set("Comment", Colour(0xFF88CC88));
	scheme.set("Keyword", Colour(0xFFBBBBFF));
	scheme.set("Operator", Colour(0xFFBBBBBB));
	scheme.set("Identifier", Colour(0xFFDDDDDD));
	scheme.set("Integer", Colour(0xFFFFBBBB));
	scheme.set("Float", Colour(0xFFFFBBBB));
	scheme.set("String", Colour(0xFFFF0000));
	scheme.set("Bracket", Colour(0xFFDDDDDD));
	scheme.set("Punctuation", Colour(0xFFBBBBBB));
	scheme.set("Preprocessor Text", Colour(0xFFBBBB88));
	scheme.set("Deactivated", Colour(0xFF777777));

	ed.colourScheme = scheme;
	ed.setShowNavigation(false);

	addAndMakeVisible(assembly);

	addAndMakeVisible(resultLabel);
	resultLabel.setFont(GLOBAL_MONOSPACE_FONT());
	resultLabel.setColour(juce::Label::ColourIds::backgroundColourId, Colour(0x22444444));
	resultLabel.setColour(juce::Label::ColourIds::textColourId, Colours::white);
	resultLabel.setEditable(false);

	if (true)
	{
		addAndMakeVisible(stateViewer);
		addAndMakeVisible(showInfo);
		addAndMakeVisible(showAssembly);
		addAndMakeVisible(showConsole);
		addAndMakeVisible(bugButton);
		addAndMakeVisible(watchTable);
		addAndMakeVisible(showWatch);
		addAndMakeVisible(spacerWatch);

		addAndMakeVisible(spacerAssembly);
		addAndMakeVisible(spacerConsole);
		addAndMakeVisible(spacerInfo);
	}

	addAndMakeVisible(compileButton);
	addAndMakeVisible(resumeButton);

	getGlobalScope().getBreakpointHandler().addListener(this);

	stateViewer->setVisible(false);
	assembly.setVisible(false);
	console.setVisible(false);
	watchTable.setVisible(false);

	showInfo.setClickingTogglesState(true);
	showAssembly.setClickingTogglesState(true);
	showConsole.setClickingTogglesState(true);
	showWatch.setClickingTogglesState(true);

	showInfo.setLookAndFeel(&blaf);
	showAssembly.setLookAndFeel(&blaf);
	showConsole.setLookAndFeel(&blaf);
	showWatch.setLookAndFeel(&blaf);
	compileButton.setLookAndFeel(&blaf);
	resumeButton.setLookAndFeel(&blaf);

	showWatch.onClick = [this]()
	{
		watchTable.setVisible(showWatch.getToggleState());
		resized();
	};

	showInfo.onClick = [this]()
	{
		stateViewer->setVisible(showInfo.getToggleState());
		resized();
	};

	showConsole.onClick = [this]()
	{
		console.setVisible(showConsole.getToggleState());
		resized();
	};

	showAssembly.onClick = [this]()
	{
		assembly.setVisible(showAssembly.getToggleState());
		resized();
	};

	bugButton.onClick = [this]()
	{
		getWorkbench()->setDebugMode(bugButton.getToggleState(), sendNotification);
	};

	compileButton.onClick = [this]()
	{
		getWorkbench()->setCode(doc.getAllContent(), sendNotification);
	};

	resumeButton.onClick = [this]()
	{
		getGlobalScope().getBreakpointHandler().resume();
	};

	bugButton.setToggleModeWithColourChange(true);
	showAssembly.setToggleModeWithColourChange(true);
	showConsole.setToggleModeWithColourChange(true);
	showInfo.setToggleModeWithColourChange(true);
	showWatch.setToggleModeWithColourChange(true);

	debugModeChanged(getWorkbench()->getGlobalScope().isDebugModeEnabled());

	

	consoleContent.addListener(this);

	if (isTestMode)
	{
		auto f = [this]()
		{
			editor.grabKeyboardFocus();
		};

		MessageManager::callAsync(f);
	}

	getWorkbench()->addListener(this);
}

SnexPlayground::~SnexPlayground()
{
	getGlobalScope().getBreakpointHandler().removeListener(this);
	getWorkbench()->removeListener(this);
}



void SnexPlayground::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF1d1d1d));
	auto b = getLocalBounds();
	GlobalHiseLookAndFeel::drawFake3D(g, b.removeFromTop(24));
	GlobalHiseLookAndFeel::drawFake3D(g, b.removeFromBottom(24));
    snexIcon.scaleToFit(10.0f, 0.0f, 50.0f, 24.0f, true);
    g.setColour(Colours::white.withAlpha(0.5f));
    g.fillPath(snexIcon);
    
}

void SnexPlayground::resized()
{
	auto area = getLocalBounds();

    auto top = area.removeFromTop(24);

	auto bottom = area.removeFromBottom(24);

	compileButton.setBounds(bottom.removeFromRight(128));
	resumeButton.setBounds(bottom.removeFromRight(128));
	bottom.removeFromRight(10);
    resultLabel.setBounds(bottom);

	bool infoVisible = stateViewer->isVisible();
	bool consoleVisible = console.isVisible();
	bool assemblyVisible = assembly.isVisible();
	bool watchVisible = watchTable.isVisible();

	spacerWatch.setVisible(watchVisible);
	spacerInfo.setVisible(infoVisible);
	spacerAssembly.setVisible(assemblyVisible);
	spacerConsole.setVisible(consoleVisible);

	auto topRight = top.removeFromRight(top.getHeight() * 5);
	
	
	auto buttonWidth = topRight.getHeight();

#if SNEX_MIR_BACKEND
    showInfo.setVisible(false);
#else
	showInfo.setBounds(topRight.removeFromLeft(buttonWidth).reduced(2));
#endif
	showWatch.setBounds(topRight.removeFromLeft(buttonWidth).reduced(2));
	showAssembly.setBounds(topRight.removeFromLeft(buttonWidth).reduced(2));
	showConsole.setBounds(topRight.removeFromLeft(buttonWidth).reduced(2));
	
	bugButton.setBounds(topRight.removeFromLeft(buttonWidth).reduced(2));
	
	top.removeFromLeft(100);

	if (infoVisible || consoleVisible || assemblyVisible || watchVisible)
	{
		auto left = area.removeFromRight(480);

		if (infoVisible)
		{
			spacerInfo.setBounds(left.removeFromTop(20));
			stateViewer->setBounds(left.removeFromTop(stateViewer->getHeight()));
		}

		if (watchVisible)
		{
			spacerWatch.setBounds(left.removeFromTop(20));
			watchTable.setBounds(left.removeFromTop(500));
		}

		if (assemblyVisible)
		{
			spacerAssembly.setBounds(left.removeFromTop(20));
			auto h = consoleVisible ? left.getHeight() / 2 : left.getHeight();
			assembly.setBounds(left.removeFromTop(h));
		}

		if (consoleVisible)
		{
			spacerConsole.setBounds(left.removeFromTop(20));
			console.setBounds(left);
		}
	}
	
	editor.setBounds(area);
}


void SnexPlayground::recalculateInternal()
{
#if 0

	int mode = jmax(0, graph.processingMode.getSelectedItemIndex());
	auto bestCallback = cData.getBestCallback(mode);
	juce::String rs;

	if (bestCallback != CallbackTypes::Inactive)
	{
		int numChannels = jmax(1, graph.channelMode.getSelectedId());
		int numSamples = jmax(16, graph.bufferLength.getText().getIntValue());

		b.setSize(numChannels, numSamples);
		b.clear();
		createTestSignal();

		currentSampleIndex.store(0);

		cData.prepare((double)numSamples, numSamples, numChannels);

		try
		{
			auto start = Time::getMillisecondCounterHiRes();

			if (mode == CallbackCollection::FrameProcessing)
			{
				switch (bestCallback)
				{
				case CallbackTypes::Channel:
				{
					for (int i = 0; i < b.getNumSamples(); i++)
					{
						for (int c = 0; c < b.getNumChannels(); c++)
						{
							block bl(b.getWritePointer(c, i), 1);
							cData.callbacks[CallbackTypes::Channel].callVoidUnchecked(bl, c);
						}
					}

					break;
				}
				case CallbackTypes::Frame:
				{
					float* l = b.getWritePointer(0);
					float* r = numChannels > 1 ? b.getWritePointer(1) : nullptr;

					for (int i = 0; i < b.getNumSamples(); i++)
					{
						currentSampleIndex.store(i);

						float data[2];

						data[0] = *l;

						if (r != nullptr)
							data[1] = *r;

						block bl(data, numChannels);
						cData.callbacks[CallbackTypes::Frame].callVoidUnchecked(bl);

						*l++ = data[0];

						if (r != nullptr)
							*r++ = data[1];
					}
					break;
				}
				case CallbackTypes::Sample:
				{
					float* l = b.getWritePointer(0);
					float* r = numChannels > 1 ? b.getWritePointer(1) : nullptr;

					for (int i = 0; i < b.getNumSamples(); i++)
					{
						currentSampleIndex.store(i);

						float value = *l;
						float result = cData.callbacks[CallbackTypes::Sample].callUnchecked<float>(value);
						*l++ = result;

						if (r != nullptr)
						{
							value = *r;
							result = cData.callbacks[CallbackTypes::Sample].callUnchecked<float>(value);
							*r++ = result;
						}
					}
					break;
				}
				}
			}
			else
			{
				switch (bestCallback)
				{
				case CallbackTypes::Channel:
				{
					for (int i = 0; i < numChannels; i++)
					{
						block bl(b.getWritePointer(i), b.getNumSamples());
						cData.callbacks[CallbackTypes::Channel].callVoidUnchecked(bl, i);
					}

					break;
				}
				case CallbackTypes::Frame:
				{
					float* l = b.getWritePointer(0);
					float* r = numChannels > 1 ? b.getWritePointer(1) : nullptr;

					for (int i = 0; i < b.getNumSamples(); i++)
					{
						float data[2];

						data[0] = *l;

						if (r != nullptr)
							data[1] = *r;

						block bl(data, numChannels);
						cData.callbacks[CallbackTypes::Frame].callVoidUnchecked(bl);

						*l++ = data[0];

						if (r != nullptr)
							*r++ = data[1];
					}
					break;
				}
				case CallbackTypes::Sample:
				{
					for (int c = 0; c < b.getNumChannels(); c++)
					{
						float *ptr = b.getWritePointer(c);

						for (int i = 0; i < b.getNumSamples(); i++)
						{
							float value = *ptr;
							float result = cData.callbacks[CallbackTypes::Sample].callUnchecked<float>(value);
							*ptr++ = result;
						}
					}

					break;
				}
				}


			}

			auto stop = Time::getMillisecondCounterHiRes();
			auto duration = stop - start;

			auto signalLength = (double)b.getNumSamples() / 44100.0 * 1000.0;

			auto speedFactor = signalLength / duration;

			rs << "Compiled OK. Speed: " << juce::String(speedFactor, 1) << "x realtime";
		}
		catch (Types::OutOfBoundsException& exception)
		{
			juce::String error;

			b.clear();

			error << "Out of bounds buffer access: " << juce::String(exception.index);
			rs = error;
		}
	}
	else
	{
		rs = "processFrame was not found.";
	}

	auto f = [this, rs]()
	{
		resultLabel.setText(rs, dontSendNotification);
		graph.setBuffer(b);
	};


	MessageManager::callAsync(f);

#endif
}

void SnexPlayground::logMessage(ui::WorkbenchData::Ptr p, int level, const juce::String& s)
{
	if (level == jit::BaseCompiler::Error)
		editor.editor.setError(s);

	if (level == BaseCompiler::MessageType::Blink)
	{
		editor.sendBlinkMessage(s.getIntValue());
	}

	auto m = p->convertToLogMessage(level, s);

	if (m.isEmpty())
		return;

	if (console.isVisible())
	{
		consoleContent.insertText(consoleContent.getNumCharacters(), m);
		consoleContent.clearUndoHistory();
	}

	if (level == jit::BaseCompiler::Warning)
	{
		editor.editor.addWarning(s);
	}
}

static void addToSubMenu(File currentFile, Array<File>& addedFiles, PopupMenu& m, const File& f)
{
	if (f.isDirectory())
	{
		PopupMenu sub;

		auto l = f.findChildFiles(File::findFilesAndDirectories, false);

		for (auto& c : l)
			addToSubMenu(currentFile, addedFiles, sub, c);

		m.addSubMenu(f.getFileName(), sub);

		return;
	}

	if (f.getFileExtension() == ".h")
	{
		addedFiles.add(f);
		m.addItem(addedFiles.size() + 1, f.getFileNameWithoutExtension(), true, f == currentFile);
	}
}




void SnexPlayground::mouseDown(const MouseEvent& event)
{
	if (testMode && event.getMouseDownX() < 50)
	{
		hise::PopupLookAndFeel claf;
		PopupMenu m;
		m.setLookAndFeel(&claf);

		m.addItem(100000, "Export all tests into big file");
		m.addSectionHeader("Load test file");
		
		
		Array<File> addedFiles;

		auto root = JitFileTestCase::getTestFileDirectory();

		m.addItem(90000, root.getFullPathName(), false, false);

		auto allFiles = root.findChildFiles(File::findDirectories, false);
		auto cppTestDIr = root.getChildFile("CppTest");
		for (int i = 0; i < allFiles.size(); i++)
		{
			if (allFiles[i].getFullPathName().contains("CppTest"))
				allFiles.remove(i--);
		}

		for(auto c: allFiles)
			addToSubMenu(currentTestFile, addedFiles, m, c);
		
		if (auto r = m.show())
		{
			if (r == 100000)
			{
				if (AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, "Create include file", "Do you want to create an include file?"))
				{
					OwnedArray<JitFileTestCase> testCases;

					String s;

					s << "#include <JuceHeader.h>\n";
					s << "using namespace juce;\n";
					s << "using namespace snex;\n";
					s << "using namespace snex::Types;\n";
					s << "using namespace scriptnode;\n";
					s << "using namespace Interleaver;\n";
					s << "static constexpr int NumChannels = 2;\n";
					s << "hmath Math;\n";

					for (auto f : addedFiles)
					{
						testCases.add(new JitFileTestCase(nullptr, getGlobalScope(), f));
					}

					for (auto t : testCases)
						s << t->convertToIncludeableCpp();

					s << "struct TestFileCppTest : public juce::UnitTest\n";
					s << "{\n";
					s << "    TestFileCppTest() : juce::UnitTest(\"TestFileCpp\") {};\n";
					s << "\n";
					s << "    void runTest() override\n";
					s << "    {\n";
					s << "        beginTest(\"Testing CPP files\");\n\n";
					
					for (auto t : testCases)
						s << t->convertToCppTestCode();

					s << "    };\n";
					s << "};\n\n";
					s << "static TestFileCppTest cppTest;";

					root.getChildFile("CppTest/Source/include.h").replaceWithText(s);
				}
			}

			currentTestFile = addedFiles[r - 2];

			doc.replaceAllContent({});
			doc.clearUndoHistory();

			testProvider = new TestCodeProvider(*this, currentTestFile);
			getWorkbench()->setCodeProvider(testProvider, sendNotification);
		}
	}
}

bool SnexPlayground::keyPressed(const KeyPress& k)
{
	if (k.getKeyCode() == KeyPress::F5Key)
	{
		saveTest = k.getModifiers().isShiftDown();
		compileButton.triggerClick();
		return true;
	}
	if (k.getKeyCode() == KeyPress::F10Key)
	{
		resumeButton.triggerClick();
		return true;
	}

	return false;
}

void SnexPlayground::createTestSignal()
{
#if 0
	auto d = b.getWritePointer(0);

	int signalType = graph.testSignal.getSelectedItemIndex();

	for (int i = 0; i < b.getNumSamples(); i++)
	{
		switch (signalType)
		{
		case 0: d[i] = Random::getSystemRandom().nextFloat(); break;
		case 1:	d[i] = (float)i / (float)b.getNumSamples(); break;
		case 2:	d[i] = fmod((float)i / (float)b.getNumSamples() * 30.0f, 1.0f); break;
		}
	}

	if (b.getNumChannels() > 1)
	{
		FloatVectorOperations::copy(b.getWritePointer(1), b.getWritePointer(0), b.getNumSamples());
	}
#endif
}

void SnexPlayground::buttonClicked(Button* b)
{

}

String SnexPlayground::TestCodeProvider::getTestTemplate()
{
	juce::String s;
	juce::String nl = "\n";
	String emptyBracket;
	emptyBracket << "{" << nl << "\t" << nl << "}" << nl << nl;

	s << "/*" << nl;
	s << "BEGIN_TEST_DATA" << nl;
	s << "  f: main" << nl;
	s << "  ret: int" << nl;
	s << "  args: int" << nl;
	s << "  input: 12" << nl;
	s << "  output: 12" << nl;
	s << "  error: \"\"" << nl;
	s << "  filename: \"\"" << nl;
	s << "END_TEST_DATA" << nl;
	s << "*/" << nl;

	s << nl;
	s << "int main(int input)" << nl;
	s << emptyBracket;

	return s;
}

String SnexPlayground::TestCodeProvider::loadCode() const
{
	String s;

	bool replaceContentInEditor = false;
	
	auto currentCode = parent.doc.getAllContent();

	if (currentCode.isNotEmpty())
	{
		s = currentCode;
		replaceContentInEditor = false;
	}
	else if (f.existsAsFile())
	{
		s = f.loadFileAsString();
		replaceContentInEditor = true;
	}
	else
	{
		s = getTestTemplate();
		replaceContentInEditor = true;
	}

	if (replaceContentInEditor)
	{
		auto unconstThis = const_cast<SnexPlayground*>(&parent);

		MessageManager::callAsync([unconstThis, s]()
		{
			unconstThis->doc.clearUndoHistory();
			unconstThis->doc.replaceAllContent(s.removeCharacters("\r"));
		});
	}

	return s;
}


bool SnexPlayground::TestCodeProvider::saveCode(const String& s)
{
	if (parent.saveTest)
	{
		JitFileTestCase newTest(parent.getGlobalScope(), s);
		f = newTest.save();
	}

	return true;
}

void SnexPlayground::recompiled(ui::WorkbenchData::Ptr p)
{
	auto r = p->getLastResult();

	if (p->isCppPreview())
	{
		doc.replaceAllContent(p->getCode());
		return;
	}

	assemblyDoc.replaceAllContent(getWorkbench()->getLastAssembly());

	if (r.compiledOk())
	{
		editor.editor.setError({});
		resized();
		resultLabel.setText("OK", dontSendNotification);
	}
	else
	{
		editor.editor.setError(r.compileResult.getErrorMessage());
		resultLabel.setText(r.compileResult.getErrorMessage(), dontSendNotification);
	}
}

void SnexPlayground::postPostCompile(ui::WorkbenchData::Ptr wb)
{
	auto& result = wb->getTestData();

    if(!wb->getLastResult().compiledOk())
    {
        auto m = wb->getLastResult().compileResult.getErrorMessage();
        resultLabel.setText(m, dontSendNotification);
        
        editor.editor.setError(m);
        
    }
	else if (!result.testWasOk())
	{
		resultLabel.setText(result.getErrorMessage(), dontSendNotification);
	}

	getWorkbench()->rebuild();
}

int AssemblyTokeniser::readNextToken(CodeDocument::Iterator& source)
{
    auto c = source.nextChar();

    auto commentChar = (juce_wchar)((SNEX_MIR_BACKEND && !SNEX_INCLUDE_NMD_ASSEMBLY) ? '#' : ';');
    
    if (c == commentChar)
    {
        source.skipToEndOfLine(); return Comment;
    }
    if (CharacterFunctions::isDigit(c))
    {
        while (!CharacterFunctions::isWhitespace(c) && !source.isEOF())
            c = source.nextChar();

        return Number;
    }
    if (c == 'L' || c == '[')
    {
        while (!CharacterFunctions::isWhitespace(c) && !source.isEOF())
            c = source.nextChar();

        return Location;
    }
#if SNEX_MIR_BACKEND && !SNEX_INCLUDE_NMD_ASSEMBLY
    
    if(c == 'l')
    {
        if(source.peekNextChar() == 'o')
        {
            source.skipToEndOfLine(); return Local;
        }
    }
    
    if(c == 'f')
    {
        if(CharacterFunctions::isWhitespace(source.peekNextChar()))
        {
            return Type;
        }
    }
    if(c == 'i')
    {
        auto s = source.peekNextChar();
        if(s == '6' || s == '3')
        {
            source.skip();
            auto s2 = source.nextChar();
            
            if((s == '6' && s2 == '4') ||
                (s == '3' && s2 == '2'))
            {
                return Type;
            }
            
        }
        
        if(CharacterFunctions::isWhitespace(source.peekNextChar()))
        {
            return Instruction;
        }
    }
    if(c == 'r' || c == 'x')
    {
        auto next = source.nextChar();
        auto next2 = source.peekNextChar();
        auto nextMatch = (c == 'r' && next == 'e' && next2 == 'g') ||
        (c == 'x' && next == 'm' && next2 == 'm');
        
        if(nextMatch)
        {
            while (!CharacterFunctions::isWhitespace(c) && !source.isEOF())
                c = source.nextChar();
            
            return Register;
        }
    }
#endif
   
    if (CharacterFunctions::isLowerCase(c))
    {
        while (!CharacterFunctions::isWhitespace(c) && !source.isEOF())
            c = source.nextChar();

        return Instruction;
    }
    if (c == '.')
    {
        while (!CharacterFunctions::isWhitespace(c) && !source.isEOF())
            c = source.nextChar();

        return Label;
    }
    
	


	return Unknown;
}

CodeEditorComponent::ColourScheme AssemblyTokeniser::getDefaultColourScheme()
{
	CodeEditorComponent::ColourScheme scheme;

	scheme.set("Unknown", Colour(0xFFBBBBBB));
	scheme.set("Comment", Colour(0xFF88CC88));
	scheme.set("Location", Colour(0xFFDDBB77));
	scheme.set("Number", Colour(0xFFFFBBBB));
	scheme.set("Label", Colours::white);
	scheme.set("Instruction", Colour(0xFFBBBBFF));
    scheme.set("Register", Colour(0xFFBBDDEE));
    scheme.set("Type", Colour(0xFFBBDDFF));
    scheme.set("Local", Colour(0xFFAAAAAA));
    

	return scheme;
}


    namespace Icons
    {
        static const unsigned char snex[] = { 110,109,117,147,31,66,0,0,0,0,98,209,162,38,66,188,116,19,60,51,179,45,66,182,243,125,61,125,191,52,66,14,45,50,62,98,35,91,68,66,117,147,216,62,115,232,83,66,152,110,114,63,8,44,99,66,139,108,231,63,98,125,63,113,66,43,135,38,64,111,18,127,66,168,198,
            107,64,164,48,134,66,141,151,162,64,98,215,35,138,66,184,30,189,64,106,252,141,66,199,75,219,64,88,185,145,66,57,180,252,64,108,115,40,146,66,211,77,0,65,98,250,190,146,66,164,112,3,65,133,235,146,66,0,0,4,65,252,105,147,66,147,24,8,65,98,174,135,148,
            66,16,88,17,65,10,23,149,66,4,86,30,65,139,236,148,66,209,34,43,65,98,29,218,148,66,205,204,48,65,137,193,148,66,147,24,50,65,180,136,148,66,188,116,55,65,108,6,1,137,66,160,26,203,65,98,158,175,136,66,203,161,205,65,84,163,136,66,41,92,206,65,33,48,
            136,66,104,145,208,65,98,29,218,135,66,88,57,210,65,188,116,135,66,20,174,211,65,150,3,135,66,72,225,212,65,98,115,232,133,66,84,227,215,65,193,138,132,66,39,49,217,65,201,54,131,66,6,129,216,65,98,51,243,130,66,53,94,216,65,33,176,130,66,233,38,216,
            65,152,110,130,66,47,221,215,65,98,215,227,129,66,137,65,215,65,16,88,129,66,2,43,214,65,129,213,128,66,150,67,213,65,98,86,142,128,66,168,198,212,65,43,71,128,66,186,73,212,65,131,0,128,66,205,204,211,65,98,96,229,126,66,10,215,210,65,193,202,125,66,
            84,227,209,65,27,175,124,66,170,241,208,65,98,133,107,121,66,27,47,206,65,215,35,118,66,250,126,203,65,4,214,114,66,158,239,200,65,98,250,254,107,66,215,163,195,65,104,17,101,66,217,206,190,65,74,12,94,66,80,141,186,65,98,137,193,80,66,250,126,178,65,
            190,31,67,66,213,120,172,65,217,78,53,66,139,108,169,65,98,154,25,48,66,162,69,168,65,66,224,42,66,92,143,167,65,209,162,37,66,186,73,167,65,98,158,239,34,66,221,36,167,65,100,59,32,66,233,38,167,65,43,135,29,66,162,69,167,65,98,223,79,25,66,201,118,
            167,65,147,24,21,66,207,247,167,65,109,231,16,66,158,239,168,65,98,166,155,10,66,66,96,170,65,236,81,4,66,205,204,172,65,70,182,252,65,233,38,177,65,98,215,163,247,65,0,0,179,65,45,178,242,65,63,53,181,65,25,4,238,65,96,229,183,65,98,72,225,234,65,45,
            178,185,65,23,217,231,65,70,182,187,65,74,12,229,65,12,2,190,65,98,188,116,226,65,221,36,192,65,98,16,224,65,31,133,194,65,133,235,221,65,160,26,197,65,98,209,34,214,65,176,114,206,65,10,215,209,65,233,38,218,65,117,147,207,65,219,249,229,65,98,217,206,
            206,65,0,0,234,65,150,67,206,65,111,18,238,65,96,229,205,65,221,36,242,65,98,33,176,205,65,213,120,244,65,18,131,205,65,217,206,246,65,164,112,205,65,221,36,249,65,98,102,102,205,65,188,116,250,65,213,120,205,65,180,200,251,65,43,135,205,65,147,24,253,
            65,98,178,157,205,65,236,81,255,65,156,196,205,65,162,197,0,66,12,2,206,65,72,225,1,66,98,233,38,207,65,209,34,7,66,51,51,210,65,4,86,12,66,233,38,216,65,131,192,16,66,98,68,139,217,65,186,201,17,66,135,22,219,65,168,198,18,66,143,194,220,65,63,181,19,
            66,98,59,223,227,65,8,172,23,66,209,34,237,65,18,131,26,66,84,227,246,65,86,142,28,66,98,57,180,250,65,41,92,29,66,178,157,254,65,43,7,30,66,180,72,1,66,215,163,30,66,98,35,91,2,66,55,9,31,66,29,90,2,66,55,9,31,66,145,109,3,66,133,107,31,66,98,156,68,
            7,66,162,197,32,66,209,34,11,66,86,14,34,66,6,1,15,66,248,83,35,66,98,254,84,19,66,125,191,36,66,252,169,23,66,221,36,38,66,250,254,27,66,61,138,39,66,98,254,212,39,66,248,83,43,66,8,172,51,66,154,25,47,66,12,130,63,66,84,227,50,66,98,88,57,65,66,164,
            112,51,66,170,241,66,66,231,251,51,66,246,168,68,66,61,138,52,66,98,227,165,71,66,12,130,53,66,184,158,74,66,55,137,54,66,123,148,77,66,141,151,55,66,98,47,221,85,66,74,140,58,66,68,11,94,66,223,207,61,66,55,9,102,66,231,123,65,66,98,199,203,108,66,147,
            152,68,66,139,108,115,66,6,1,72,66,254,212,121,66,199,203,75,66,98,109,231,130,66,84,227,82,66,92,143,136,66,66,96,91,66,145,45,141,66,16,216,101,66,98,141,87,143,66,125,191,106,66,119,62,145,66,160,26,112,66,178,221,146,66,174,199,117,66,98,135,150,
            148,66,236,209,123,66,244,253,149,66,166,27,129,66,154,25,151,66,109,103,132,66,98,184,222,152,66,2,171,137,66,84,227,153,66,252,41,143,66,184,94,154,66,164,176,148,66,98,135,150,154,66,133,43,151,66,182,179,154,66,246,168,153,66,76,183,154,66,227,37,
            156,66,98,106,188,154,66,33,112,159,66,16,152,154,66,219,185,162,66,219,57,154,66,250,254,165,66,98,242,146,153,66,31,197,171,66,225,58,152,66,94,122,177,66,193,10,150,66,184,222,182,66,98,10,87,149,66,29,154,184,66,80,141,148,66,74,76,186,66,145,173,
            147,66,51,243,187,66,98,43,71,146,66,29,154,190,66,246,168,144,66,84,35,193,66,141,215,142,66,37,134,195,66,98,184,30,140,66,147,24,199,66,63,245,136,66,242,82,202,66,100,123,133,66,252,41,205,66,98,252,105,131,66,35,219,206,66,238,60,129,66,252,105,
            208,66,213,248,125,66,154,217,209,66,98,14,45,122,66,223,15,211,66,143,66,118,66,27,47,212,66,137,65,114,66,76,55,213,66,98,68,139,111,66,127,234,213,66,186,201,108,66,242,146,214,66,0,0,106,66,39,49,215,66,98,221,36,85,66,92,207,219,66,12,130,62,66,
            227,37,222,66,16,216,39,66,61,10,223,66,98,23,217,35,66,176,50,223,66,23,217,31,66,223,79,223,66,16,216,27,66,78,98,223,66,98,96,229,22,66,88,121,223,66,170,241,17,66,0,128,223,66,238,252,12,66,88,121,223,66,98,119,190,244,65,190,95,223,66,43,135,207,
            65,225,122,222,66,0,0,171,65,109,167,220,66,98,244,253,132,65,6,193,218,66,178,157,63,65,135,214,215,66,55,137,241,64,102,230,211,66,98,221,36,190,64,213,120,210,66,20,174,139,64,246,232,208,66,8,172,52,64,88,57,207,66,98,190,159,26,64,43,199,206,66,
            156,196,0,64,242,82,206,66,123,20,206,63,172,220,205,66,98,129,149,179,63,190,159,205,66,86,14,157,63,27,111,205,66,225,122,132,63,66,32,205,66,98,104,145,109,63,188,244,204,66,70,182,83,63,156,196,204,66,109,231,59,63,104,145,204,66,98,39,49,136,62,
            92,143,203,66,111,18,131,58,195,53,202,66,0,0,0,0,254,212,200,66,98,0,0,0,0,154,25,200,66,158,239,39,61,8,236,199,66,217,206,247,61,201,54,199,66,108,78,98,152,64,225,250,168,66,98,182,243,153,64,219,185,168,66,100,59,155,64,76,119,168,66,135,22,157,
            64,213,56,168,66,98,47,221,160,64,225,186,167,66,143,194,165,64,43,71,167,66,178,157,171,64,66,224,166,66,98,6,129,177,64,219,121,166,66,236,81,184,64,203,33,166,66,217,206,191,64,35,219,165,66,98,141,151,210,64,121,41,165,66,125,63,233,64,20,238,164,
            66,190,159,254,64,195,53,165,66,98,139,108,1,65,25,68,165,66,31,133,3,65,141,87,165,66,80,141,5,65,158,111,165,66,98,150,67,9,65,166,155,165,66,213,120,11,65,168,198,165,66,82,184,14,65,125,255,165,66,98,84,227,17,65,76,55,166,66,111,18,21,65,20,110,
            166,66,162,69,24,65,221,164,166,66,98,68,139,28,65,145,237,166,66,254,212,32,65,195,53,167,66,209,34,37,65,238,124,167,66,98,203,161,55,65,139,172,168,66,78,98,74,65,199,203,169,66,139,108,93,65,141,215,170,66,98,233,38,129,65,184,222,172,66,86,14,148,
            65,72,161,174,66,162,69,167,65,215,35,176,66,98,123,20,180,65,227,37,177,66,25,4,193,65,205,12,178,66,160,26,206,65,74,204,178,66,98,229,208,226,65,100,251,179,66,59,223,247,65,74,204,180,66,55,137,6,66,8,44,181,66,98,254,84,10,66,86,78,181,66,209,34,
            14,66,78,98,181,66,170,241,17,66,233,102,181,66,98,76,55,22,66,8,108,181,66,244,125,26,66,47,93,181,66,131,192,30,66,39,49,181,66,98,0,128,37,66,133,235,180,66,125,63,44,66,184,94,180,66,63,181,50,66,129,85,179,66,98,248,211,54,66,139,172,178,66,254,
            212,58,66,217,206,177,66,31,133,62,66,133,171,176,66,98,70,182,66,66,66,96,175,66,201,118,70,66,82,184,173,66,158,111,73,66,131,192,171,66,98,109,231,75,66,53,30,170,66,217,206,77,66,180,72,168,66,240,39,79,66,154,89,166,66,98,86,142,80,66,141,87,164,
            66,59,95,81,66,244,61,162,66,168,198,81,66,66,32,160,66,98,170,241,81,66,244,61,159,66,68,11,82,66,35,91,158,66,117,19,82,66,207,119,157,66,98,203,33,82,66,172,28,156,66,37,6,82,66,143,194,154,66,166,155,81,66,2,107,153,66,98,68,11,81,66,47,157,151,66,
            164,240,79,66,35,219,149,66,150,67,78,66,225,58,148,66,98,178,157,76,66,203,161,146,66,158,111,74,66,27,47,145,66,121,233,71,66,109,231,143,66,98,18,131,68,66,145,45,142,66,43,135,64,66,12,194,140,66,248,83,60,66,92,143,139,66,98,137,193,54,66,82,248,
            137,66,168,198,48,66,12,194,136,66,27,175,42,66,156,196,135,66,98,51,51,41,66,168,134,135,66,57,180,39,66,86,78,135,66,63,53,38,66,129,21,135,66,98,96,229,35,66,238,188,134,66,135,150,33,66,215,99,134,66,168,70,31,66,205,12,134,66,98,41,92,25,66,152,
            46,133,66,164,112,19,66,104,81,132,66,37,134,13,66,182,115,131,66,98,174,199,10,66,199,11,131,66,55,9,8,66,215,163,130,66,199,75,5,66,100,59,130,66,98,63,53,251,65,242,18,129,66,10,215,235,65,217,206,127,66,55,137,220,65,53,94,125,66,98,236,81,214,65,
            66,96,124,66,160,26,208,65,53,94,123,66,133,235,201,65,242,82,122,66,98,231,251,195,65,236,81,121,66,98,16,190,65,180,72,120,66,76,55,184,65,240,39,119,66,98,31,133,182,65,29,218,118,66,254,212,180,65,74,140,118,66,209,34,179,65,113,61,118,66,98,135,
            22,172,65,145,237,116,66,147,24,165,65,80,141,115,66,51,51,158,65,80,13,114,66,98,199,75,144,65,55,9,111,66,119,190,130,65,135,150,107,66,154,153,107,65,215,163,103,66,98,102,102,80,65,250,126,99,66,68,139,54,65,193,202,94,66,156,196,30,65,238,124,89,
            66,98,203,161,5,65,66,224,83,66,94,186,221,64,141,151,77,66,82,184,182,64,76,183,70,66,98,51,51,155,64,47,221,65,66,170,241,130,64,94,186,60,66,225,122,92,64,66,96,55,66,98,217,206,47,64,154,153,49,66,188,116,11,64,111,146,43,66,201,118,222,63,109,103,
            37,66,98,68,139,108,63,37,6,26,66,168,198,11,63,51,51,14,66,219,249,254,62,72,97,2,66,98,193,202,225,62,37,6,233,65,160,26,79,63,135,22,205,65,188,116,227,63,102,102,178,65,98,104,145,21,64,158,239,162,65,4,86,70,64,170,241,147,65,221,36,130,64,242,210,
            133,65,98,205,204,160,64,70,182,111,65,242,210,197,64,129,149,85,65,88,57,240,64,193,202,61,65,98,55,137,11,65,25,4,40,65,2,43,33,65,88,57,20,65,53,94,56,65,43,135,2,65,98,90,100,75,65,0,0,232,64,90,100,95,65,164,112,205,64,197,32,116,65,55,137,181,64,
            98,135,22,130,65,61,10,163,64,4,86,138,65,221,36,146,64,143,194,146,65,131,192,130,64,98,176,114,177,65,164,112,21,64,236,81,210,65,4,86,158,63,227,165,243,65,37,6,33,63,98,37,134,4,66,106,188,116,62,236,81,15,66,244,253,84,61,197,32,26,66,66,96,229,
            59,98,170,241,27,66,111,18,3,59,137,193,29,66,111,18,131,186,117,147,31,66,0,0,0,0,99,109,199,91,130,67,170,177,220,66,108,150,35,70,67,170,177,220,66,98,78,194,69,67,78,162,220,66,240,167,69,67,252,169,220,66,61,74,69,67,14,109,220,66,98,215,3,69,67,
            125,63,220,66,137,193,68,67,231,251,219,66,227,133,68,67,221,164,219,66,98,86,14,68,67,76,247,218,66,182,179,67,67,119,254,217,66,37,134,67,67,221,228,216,66,98,174,103,67,67,246,40,216,66,68,107,67,67,188,244,215,66,150,99,67,67,170,49,215,66,108,150,
            99,67,67,72,225,146,64,98,68,107,67,67,33,176,134,64,174,103,67,67,90,100,131,64,37,134,67,67,41,92,111,64,98,238,156,67,67,143,194,93,64,184,190,67,67,125,63,77,64,61,234,67,67,162,69,62,64,98,6,65,68,67,78,98,32,64,113,189,68,67,193,202,9,64,61,74,
            69,67,8,172,252,63,98,240,167,69,67,164,112,237,63,78,194,69,67,100,59,239,63,150,35,70,67,31,133,235,63,108,219,73,130,67,31,133,235,63,98,35,91,130,67,8,172,236,63,74,108,130,67,182,243,237,63,145,125,130,67,100,59,239,63,98,94,170,130,67,16,88,249,
            63,168,182,130,67,213,120,249,63,131,224,130,67,240,167,6,64,98,27,63,131,67,86,14,29,64,37,134,131,67,176,114,72,64,125,159,131,67,150,67,123,64,98,160,170,131,67,47,221,136,64,82,168,131,67,59,223,139,64,88,169,131,67,41,92,151,64,108,102,70,131,67,
            168,198,158,65,98,98,64,131,67,70,182,161,65,137,65,131,67,6,129,162,65,229,48,131,67,211,77,165,65,98,100,11,131,67,203,161,171,65,43,183,130,67,190,159,176,65,33,80,130,67,227,165,178,65,98,176,34,130,67,55,137,179,65,4,22,130,67,152,110,179,65,201,
            230,129,67,240,167,179,65,108,170,241,91,67,240,167,179,65,108,170,241,91,67,215,35,46,66,108,213,216,128,67,215,35,46,66,98,154,9,129,67,143,66,46,66,168,22,129,67,51,51,46,66,162,69,129,67,14,173,46,66,98,231,139,129,67,90,100,47,66,29,202,129,67,211,
            205,48,66,162,245,129,67,8,172,50,66,98,68,11,130,67,166,155,51,66,41,28,130,67,221,164,52,66,141,39,130,67,113,189,53,66,98,233,54,130,67,63,53,55,66,254,52,130,67,178,157,55,66,213,56,130,67,215,35,57,66,108,213,56,130,67,166,155,117,66,98,254,52,130,
            67,203,33,119,66,233,54,130,67,61,138,119,66,141,39,130,67,12,2,121,66,98,197,16,130,67,57,52,123,66,150,227,129,67,227,37,125,66,174,167,129,67,12,130,126,66,98,219,137,129,67,27,47,127,66,180,104,129,67,70,182,127,66,162,69,129,67,180,8,128,66,98,168,
            22,129,67,162,69,128,66,154,9,129,67,119,62,128,66,213,216,128,67,211,77,128,66,108,170,241,91,67,211,77,128,66,108,170,241,91,67,236,81,179,66,108,205,172,130,67,236,81,179,66,98,94,218,130,67,59,95,179,66,135,230,130,67,147,88,179,66,143,18,131,67,
            211,141,179,66,98,150,51,131,67,195,181,179,66,242,82,131,67,164,240,179,66,158,111,131,67,238,60,180,66,98,141,183,131,67,106,252,180,66,74,236,131,67,90,36,182,66,203,1,132,67,51,115,183,66,98,47,13,132,67,227,37,184,66,35,11,132,67,4,86,184,66,139,
            12,132,67,74,12,185,66,108,133,187,131,67,139,108,215,66,98,227,181,131,67,121,41,216,66,43,183,131,67,172,92,216,66,201,166,131,67,104,17,217,66,98,203,129,131,67,252,169,218,66,113,45,131,67,145,237,219,66,195,197,130,67,164,112,220,66,98,16,152,130,
            67,127,170,220,66,100,139,130,67,84,163,220,66,199,91,130,67,170,177,220,66,99,109,82,40,148,67,170,177,220,66,108,76,183,137,67,170,177,220,66,98,63,165,137,67,8,172,220,66,242,146,137,67,8,172,220,66,6,129,137,67,197,160,220,66,98,238,92,137,67,61,
            138,220,66,252,57,137,67,178,93,220,66,121,25,137,67,41,28,220,66,98,213,248,136,67,160,218,219,66,2,219,136,67,162,133,219,66,6,193,136,67,59,31,219,66,98,45,114,136,67,2,235,217,66,199,75,136,67,121,41,216,66,127,90,136,67,188,116,214,66,98,125,95,
            136,67,90,228,213,66,219,105,136,67,141,87,213,66,154,121,136,67,248,211,212,66,98,104,129,136,67,111,146,212,66,199,139,136,67,4,86,212,66,188,148,136,67,10,23,212,66,108,207,23,154,67,47,93,91,66,108,49,232,137,67,80,141,195,64,108,178,205,137,67,119,
            190,183,64,98,109,199,137,67,238,124,179,64,223,191,137,67,41,92,175,64,225,186,137,67,61,10,171,64,98,229,176,137,67,4,86,162,64,106,172,137,67,174,71,153,64,113,173,137,67,88,57,144,64,98,66,176,137,67,12,2,115,64,233,214,137,67,160,26,71,64,49,24,
            138,67,139,108,39,64,98,45,50,138,67,229,208,26,64,223,79,138,67,137,65,16,64,98,112,138,67,39,49,8,64,98,197,144,138,67,197,32,0,64,117,179,138,67,125,63,245,63,76,215,138,67,178,157,239,63,98,55,233,138,67,205,204,236,63,100,251,138,67,205,204,236,
            63,80,13,139,67,31,133,235,63,108,102,198,149,67,31,133,235,63,98,43,215,149,67,8,172,236,63,240,231,149,67,242,210,237,63,213,248,149,67,160,26,239,63,98,88,9,150,67,33,176,242,63,29,26,150,67,184,30,245,63,61,42,150,67,231,251,249,63,98,225,90,150,
            67,246,40,4,64,49,136,150,67,215,163,16,64,86,174,150,67,16,88,33,64,98,66,208,150,67,137,65,48,64,63,213,150,67,45,178,53,64,223,239,150,67,119,190,71,64,108,133,235,161,67,55,137,23,66,108,68,59,173,67,143,194,69,64,108,88,89,173,67,35,219,49,64,98,
            63,101,173,67,49,8,44,64,66,112,173,67,45,178,37,64,205,124,173,67,137,65,32,64,98,209,162,173,67,158,239,15,64,158,207,173,67,168,198,3,64,190,255,173,67,154,153,249,63,98,127,42,174,67,86,14,237,63,37,54,174,67,141,151,238,63,78,98,174,67,31,133,235,
            63,108,100,139,184,67,31,133,235,63,98,113,157,184,67,205,204,236,63,158,175,184,67,145,237,236,63,137,193,184,67,178,157,239,63,98,129,229,184,67,125,63,245,63,82,8,185,67,39,49,0,64,213,40,185,67,236,81,8,64,98,88,73,185,67,176,114,16,64,10,103,185,
            67,111,18,27,64,39,129,185,67,119,190,39,64,98,190,207,185,67,123,20,78,64,168,246,185,67,12,2,131,64,82,232,185,67,113,61,158,64,98,182,227,185,67,150,67,167,64,121,217,185,67,49,8,176,64,252,201,185,67,137,65,184,64,98,78,194,185,67,29,90,188,64,16,
            184,185,67,197,32,192,64,27,175,185,67,98,16,196,64,108,240,39,169,67,43,135,92,66,108,174,151,186,67,4,22,212,66,108,242,178,186,67,242,210,212,66,98,121,185,186,67,10,23,213,66,104,193,186,67,23,89,213,66,168,198,186,67,184,158,213,66,98,6,209,186,
            67,127,42,214,66,4,214,186,67,231,187,214,66,63,213,186,67,211,77,215,66,98,117,211,186,67,231,187,216,66,14,173,186,67,184,30,218,66,199,107,186,67,59,31,219,66,98,170,81,186,67,31,133,219,66,215,51,186,67,160,218,219,66,84,19,186,67,41,28,220,66,98,
            176,242,185,67,178,93,220,66,190,207,185,67,61,138,220,66,199,171,185,67,197,160,220,66,98,186,153,185,67,8,172,220,66,141,135,185,67,8,172,220,66,96,117,185,67,170,177,220,66,108,90,116,174,67,170,177,220,66,98,49,72,174,67,96,165,220,66,106,60,174,
            67,133,171,220,66,170,17,174,67,88,121,220,66,98,104,209,173,67,20,46,220,66,43,151,173,67,154,153,219,66,68,107,173,67,223,207,218,66,98,66,96,173,67,178,157,218,66,43,87,173,67,96,101,218,66,14,77,173,67,164,48,218,66,108,59,111,161,67,98,16,145,66,
            108,178,77,149,67,231,59,218,66,108,125,47,149,67,23,217,218,66,98,33,16,149,67,117,83,219,66,154,9,149,67,94,122,219,66,57,228,148,67,147,216,219,66,98,61,186,148,67,143,66,220,66,225,138,148,67,49,136,220,66,186,89,148,67,215,163,220,66,98,121,73,148,
            67,14,173,220,66,213,56,148,67,14,173,220,66,82,40,148,67,170,177,220,66,99,109,199,75,216,66,8,172,220,66,108,57,180,181,66,8,172,220,66,98,39,241,180,66,172,156,220,66,106,188,180,66,215,163,220,66,6,1,180,66,233,102,220,66,98,109,231,178,66,68,11,
            220,66,152,238,177,66,135,86,219,66,6,65,177,66,109,103,218,66,98,252,233,176,66,33,240,217,66,102,166,176,66,133,107,217,66,213,120,176,66,184,222,216,66,98,231,59,176,66,84,35,216,66,150,67,176,66,152,238,215,66,57,52,176,66,8,44,215,66,108,57,52,176,
            66,250,126,146,64,98,150,67,176,66,211,77,134,64,231,59,176,66,61,10,131,64,213,120,176,66,141,151,110,64,98,123,212,176,66,188,116,75,64,55,137,177,66,29,90,44,64,82,120,178,66,141,151,22,64,98,33,240,178,66,70,182,11,64,57,116,179,66,248,83,3,64,6,
            1,180,66,209,34,251,63,98,106,188,180,66,109,231,235,63,39,241,180,66,242,210,237,63,57,180,181,66,231,251,233,63,108,57,52,222,66,231,251,233,63,98,201,118,222,66,209,34,235,63,219,185,222,66,127,106,236,63,238,252,222,66,104,145,237,63,98,119,62,223,
            66,233,38,241,63,6,129,223,66,129,149,243,63,137,193,223,66,236,81,248,63,98,18,131,224,66,150,67,3,64,76,55,225,66,178,157,15,64,92,207,225,66,197,32,32,64,98,10,87,226,66,23,217,46,64,133,107,226,66,88,57,52,64,4,214,226,66,221,36,70,64,98,207,119,
            228,66,109,231,139,64,160,26,230,66,57,180,180,64,113,189,231,66,6,129,221,64,98,14,237,244,66,238,124,137,65,254,20,1,67,201,118,219,65,117,179,7,67,76,183,22,66,98,246,72,11,67,121,233,44,66,119,222,14,67,160,26,67,66,123,116,18,67,193,74,89,66,98,
            92,175,21,67,180,72,109,66,127,234,24,67,84,163,128,66,240,39,28,67,59,159,138,66,98,41,124,28,67,197,160,139,66,229,208,28,67,203,161,140,66,227,37,29,67,209,162,141,66,98,166,123,29,67,221,164,142,66,39,209,29,67,233,166,143,66,43,39,30,67,115,168,
            144,66,98,242,18,30,67,90,228,136,66,59,255,29,67,66,32,129,66,199,235,29,67,82,184,114,66,98,227,229,29,67,57,180,109,66,59,223,29,67,33,176,104,66,160,218,29,67,8,172,99,66,108,29,218,29,67,162,197,98,66,108,29,218,29,67,250,126,146,64,108,143,226,
            29,67,106,188,132,64,98,154,249,29,67,252,169,113,64,219,249,29,67,111,18,107,64,168,38,30,67,86,14,85,64,98,225,122,30,67,168,198,43,64,33,16,31,67,109,231,11,64,131,192,31,67,209,34,251,63,98,53,30,32,67,109,231,235,63,147,56,32,67,242,210,237,63,29,
            154,32,67,231,251,233,63,108,16,248,49,67,231,251,233,63,98,154,89,50,67,242,210,237,63,182,115,50,67,109,231,235,63,170,209,50,67,209,34,251,63,98,207,23,51,67,248,83,3,64,29,90,51,67,70,182,11,64,195,149,51,67,141,151,22,64,98,145,13,52,67,29,90,44,
            64,240,103,52,67,188,116,75,64,129,149,52,67,141,151,110,64,98,248,179,52,67,61,10,131,64,98,176,52,67,211,77,134,64,16,184,52,67,250,126,146,64,108,16,184,52,67,8,44,215,66,98,98,176,52,67,152,238,215,66,248,179,52,67,84,35,216,66,129,149,52,67,184,
            222,216,66,98,184,126,52,67,133,107,217,66,238,92,52,67,33,240,217,66,170,49,52,67,109,103,218,66,98,160,218,51,67,135,86,219,66,53,94,51,67,68,11,220,66,170,209,50,67,233,102,220,66,98,182,115,50,67,215,163,220,66,154,89,50,67,172,156,220,66,16,248,
            49,67,8,172,220,66,108,33,112,29,67,8,172,220,66,98,76,23,29,67,59,159,220,66,190,255,28,67,96,165,220,66,121,169,28,67,176,114,220,66,98,115,40,28,67,233,38,220,66,182,179,27,67,98,144,219,66,166,91,27,67,31,197,218,66,98,227,69,27,67,111,146,218,66,
            182,51,27,67,29,90,218,66,125,31,27,67,90,36,218,66,108,250,62,221,66,127,234,30,66,98,98,80,221,66,14,45,36,66,197,96,221,66,164,112,41,66,164,112,221,66,51,179,46,66,98,238,124,221,66,92,15,51,66,55,137,221,66,133,107,55,66,123,148,221,66,168,198,59,
            66,98,158,175,221,66,199,203,70,66,43,199,221,66,223,207,81,66,199,203,221,66,254,212,92,66,108,199,203,221,66,162,197,94,66,108,199,203,221,66,8,44,215,66,108,225,186,221,66,49,8,216,66,98,205,140,221,66,37,198,216,66,74,140,221,66,100,251,216,66,176,
            50,221,66,2,171,217,66,98,61,138,220,66,63,245,218,66,190,95,219,66,57,244,219,66,250,254,217,66,233,102,220,66,98,150,67,217,66,215,163,220,66,92,15,217,66,172,156,220,66,199,75,216,66,8,172,220,66,99,109,109,215,128,67,72,225,234,64,108,150,227,72,
            67,72,225,234,64,108,150,227,72,67,170,177,209,66,108,61,10,129,67,170,177,209,66,108,211,61,129,67,236,81,190,66,108,170,49,89,67,236,81,190,66,98,33,208,88,67,143,66,190,66,4,182,88,67,186,73,190,66,16,88,88,67,205,12,190,66,98,133,203,87,67,170,177,
            189,66,27,79,87,67,238,252,188,66,16,248,86,67,80,13,188,66,98,205,204,86,67,4,150,187,66,2,171,86,67,104,17,187,66,57,148,86,67,31,133,186,66,98,195,117,86,67,55,201,185,66,88,121,86,67,254,148,185,66,170,113,86,67,236,209,184,66,108,170,113,86,67,166,
            155,117,66,98,88,121,86,67,129,21,116,66,195,117,86,67,14,173,115,66,57,148,86,67,63,53,114,66,98,203,193,86,67,18,3,112,66,41,28,87,67,104,17,110,66,248,147,87,67,63,181,108,66,98,158,207,87,67,49,8,108,66,236,17,88,67,6,129,107,66,16,88,88,67,227,37,
            107,66,98,4,182,88,67,2,171,106,66,33,208,88,67,94,186,106,66,170,49,89,67,166,155,106,66,108,170,241,126,67,166,155,106,66,108,170,241,126,67,215,35,68,66,108,170,49,89,67,215,35,68,66,98,33,208,88,67,31,5,68,66,4,182,88,67,117,19,68,66,16,88,88,67,
            154,153,67,66,98,133,203,87,67,84,227,66,66,27,79,87,67,219,121,65,66,16,248,86,67,160,154,63,66,98,205,204,86,67,8,172,62,66,2,171,86,67,209,162,61,66,57,148,86,67,61,138,60,66,98,195,117,86,67,111,18,59,66,88,121,86,67,252,169,58,66,170,113,86,67,215,
            35,57,66,108,170,113,86,67,240,167,157,65,98,88,121,86,67,166,155,154,65,195,117,86,67,193,202,153,65,57,148,86,67,35,219,150,65,98,203,193,86,67,188,116,146,65,41,28,87,67,104,145,142,65,248,147,87,67,35,219,139,65,98,158,207,87,67,250,126,138,65,236,
            17,88,67,164,112,137,65,16,88,88,67,94,186,136,65,98,4,182,88,67,168,198,135,65,33,208,88,67,96,229,135,65,170,49,89,67,240,167,135,65,108,82,152,128,67,240,167,135,65,108,109,215,128,67,72,225,234,64,99,109,90,4,149,67,72,225,234,64,108,211,157,141,
            67,72,225,234,64,108,129,229,156,67,47,93,85,66,98,41,252,156,67,190,159,86,66,90,4,157,67,121,233,86,66,170,17,157,67,217,78,88,66,98,162,37,157,67,96,101,90,66,31,37,157,67,240,167,92,66,66,16,157,67,106,188,94,66,98,78,2,157,67,197,32,96,66,29,250,
            156,67,121,105,96,66,209,226,156,67,246,168,97,66,108,143,82,140,67,170,177,209,66,108,100,107,147,67,170,177,209,66,108,238,76,160,67,125,255,131,66,98,240,87,160,67,43,199,131,66,236,97,160,67,68,139,131,66,244,109,160,67,135,86,131,66,98,20,158,160,
            67,143,130,130,66,86,222,160,67,139,236,129,66,57,36,161,67,14,173,129,66,98,242,82,161,67,143,130,129,66,92,95,161,67,80,141,129,66,217,142,161,67,86,142,129,66,98,131,160,161,67,154,153,129,66,12,178,161,67,221,164,129,66,182,195,161,67,33,176,129,
            66,98,188,212,161,67,162,197,129,66,70,230,161,67,135,214,129,66,201,246,161,67,39,241,129,66,98,115,40,162,67,12,66,130,66,96,85,162,67,131,192,130,66,23,121,162,67,197,96,131,66,98,254,132,162,67,4,150,131,66,184,142,162,67,111,210,131,66,154,153,162,
            67,68,11,132,66,108,117,51,175,67,170,177,209,66,108,154,217,182,67,170,177,209,66,108,231,91,166,67,193,202,98,66,98,156,68,166,67,55,137,97,66,41,60,166,67,125,63,97,66,53,46,166,67,23,217,95,66,98,88,25,166,67,131,192,93,66,23,25,166,67,225,122,91,
            66,145,45,166,67,72,97,89,66,98,35,59,166,67,219,249,87,66,117,67,166,67,33,176,87,66,127,90,166,67,139,108,86,66,108,63,245,181,67,72,225,234,64,108,72,33,175,67,72,225,234,64,108,119,14,163,67,127,234,49,66,98,117,3,163,67,47,93,50,66,154,249,162,67,
            10,215,50,66,145,237,162,67,143,66,51,66,98,96,213,162,67,160,26,52,66,55,185,162,67,242,210,52,66,252,153,162,67,102,102,53,66,98,225,122,162,67,213,248,53,66,246,88,162,67,90,100,54,66,162,53,162,67,221,164,54,66,98,47,221,161,67,168,70,55,66,119,126,
            161,67,199,203,54,66,197,48,161,67,242,82,53,66,98,203,17,161,67,106,188,52,66,227,245,160,67,6,1,52,66,20,222,160,67,233,38,51,66,98,78,210,160,67,94,186,50,66,147,200,160,67,125,63,50,66,211,189,160,67,199,203,49,66,108,90,4,149,67,72,225,234,64,99,
            101,0,0 };
        
    }
    
    Path SnexPathFactory::createPath(const juce::String& url) const
    {
        Path p;
        
        LOAD_PATH_IF_URL("snex", Icons::snex);
        
		LOAD_PATH_IF_URL("debug", SnexIcons::bugIcon);
		LOAD_PATH_IF_URL("asm", SnexIcons::asmIcon);
		LOAD_PATH_IF_URL("optimise", SnexIcons::optimizeIcon);
		LOAD_PATH_IF_URL("console", SnexIcons::debugPanel);
		LOAD_EPATH_IF_URL("watch", BackendBinaryData::ToolbarIcons::viewPanel);

        return p;
    }

	void BreakpointDataProvider::rebuild()
	{
		infos.clear();

		for (int i = 0; i < handler.getNumEntries(); i++)
		{
			auto e = handler.getEntry(i);
			auto si = new SettableDebugInfo();

			si->typeValue = e.currentValue.getType();
			si->name = e.id.toString();
			si->dataType = e.scope == BaseScope::ScopeType::Class ? "global" : "local";
			si->value = Types::Helpers::getCppValueString(e.currentValue);

			infos.add(si);
		}

#if 0
		if (auto cs = dynamic_cast<ClassScope*>(scope.getCurrentClassScope()))
		{
			for (auto v : cs->getAllVariables())
			{
				bool found = false;

				for (auto existing : infos)
				{
					if (existing->getTextForName() == v.id.toString())
					{
						found = true;
						break;
					}
				}

				if (found)
					continue;

				auto si = new SettableDebugInfo();

				if (cs->rootData->contains(v.id))
				{
					auto value = cs->rootData->getDataCopy(v.id);
					
					si->typeValue = value.getType();
					si->name = v.id.toString();
					si->dataType = "global";
					si->value = Types::Helpers::getCppValueString(value);

					infos.add(si);
				}
			}

			StringArray removeClasses = { "Math", "Message", "Block" };

			for (int i = 0; i < infos.size(); i++)
			{
				if(removeClasses.contains(infos[i]->getTextForName()))
					infos.remove(i--);
			}
		}
#endif

		ApiProviderBase::Holder::rebuild();
	}

	

	void SnexPlayground::PreprocessorUpdater::timerCallback()
	{
		stopTimer();

		auto& ed = parent.editor.editor;

		if (ed.isPreprocessorParsingEnabled())
		{
			snex::jit::Preprocessor pp(parent.doc.getAllContent());
			lastRange = pp.getDeactivatedLines();
			ed.setDeactivatedLines(lastRange);
		}

		if (ed.isLiveParsingEnabled())
		{
			ed.clearWarningsAndErrors();

			GlobalScope s;
			Compiler c(s);
			c.setDebugHandler(this);
			Types::SnexObjectDatabase::registerObjects(c, 2);

			c.compileJitObject(parent.doc.getAllContent());
			ed.tokenCollection->signalRebuild();
			auto r = c.getCompileResult();

			if (!r.wasOk())
				ed.setError(r.getErrorMessage());
		}
	}

	void SnexPlayground::PreprocessorUpdater::logMessage(int level, const juce::String& s)
	{
		if (level == (int)snex::BaseCompiler::Warning)
			parent.editor.editor.addWarning(s);
	}

	snex::ui::WorkbenchData::CompileResult TestCompileThread::compile(const String& s)
	{
		lastTest = new JitFileTestCase(getParent()->getGlobalScope(), s);

		lastResult = {};

		lastResult.compileResult = lastTest->compileWithoutTesting();
		lastResult.assembly = lastTest->assembly;
		lastResult.obj = lastTest->obj;
		lastResult.parameters.clear();
		
		if (lastTest->nodeToTest != nullptr)
		{
			lastResult.parameters.addArray(lastTest->nodeToTest->getParameterList());
			getParent()->getTestData().testSourceData.makeCopyOf(lastTest->getBuffer(true));
		}
		
		return lastResult;
	}

	void TestCompileThread::postCompile(ui::WorkbenchData::CompileResult& lastResult)
	{
		if (lastTest != nullptr && lastResult.compiledOk())
		{
			auto& td = getParent()->getTestData();

			if (lastTest->nodeToTest != nullptr)
			{
				
			}

			if (td.shouldRunTest() && lastTest->nodeToTest != nullptr)
			{
                PrepareSpecs ps;
                ps.sampleRate = 44100.0;
                ps.blockSize = 512;
                ps.numChannels = 2;
				td.initProcessing(ps);
				td.processTestData(getParent());
			}
			else
			{
				lastResult.compileResult = lastTest->testAfterCompilation();
                lastResult.assembly = lastTest->assembly;
			}

#if 0
			lastResult.testResult = lastTest->testAfterCompilation();
			lastResult.cpuUsage = lastTest->cpuUsage;
#endif
		}
	}
}
}

