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
using namespace juce;


void debug::MathFunctionProvider::addTokens(mcl::TokenCollection::List& l)
{
	ComplexType::Ptr bt = new DynType(TypeInfo(Types::ID::Float));
	FunctionClass::Ptr fc = new snex::jit::MathFunctions(false, bt);

	Array<NamespacedIdentifier> functions;
	fc->getAllFunctionNames(functions);

	for (auto& f : functions)
	{
		Array<FunctionData> matches;
		fc->addMatchingFunctions(matches, f);

		for (auto m : matches)
		{
			l.add(new MathFunction(m));
		}
	}

	for (int i = 0; i < l.size(); i++)
	{
		if (l[i]->tokenContent.isEmpty())
		{
			l.remove(i--);
			continue;
		}

		for (int j = i + 1; j < l.size(); j++)
		{
			if (*l[j] == *l[i])
			{
				auto s = l[j]->tokenContent;
				l.remove(j--);
			}
		}
	}
}

debug::SymbolProvider::ComplexMemberToken::ComplexMemberToken(SymbolProvider& parent_, ComplexType::Ptr p_, FunctionData& f) :
	Token(f.getSignature()),
	parent(parent_),
	p(p_)
{
	f.getOrResolveReturnType(p);

	c = FourColourScheme::getColour(FourColourScheme::Method);
	tokenContent = f.getSignature({}, false);
	priority = 80;
	codeToInsert = f.getCodeToInsert();
	markdownDescription = f.description;
}

bool debug::SymbolProvider::ComplexMemberToken::matches(const String& input, const String& previousToken, int lineNumber) const
{
	if (auto st = dynamic_cast<SpanType*>(p.get()))
	{
		if (st->getElementSize() == 19)
			jassertfalse;
	}

	if (previousToken.endsWith("."))
	{
		try
		{
			auto typeInfo = parent.nh->parseTypeFromTextInput(previousToken.upToLastOccurrenceOf(".", false, false), lineNumber);

			auto codeToSearch = input.length() == 1 ? getCodeToInsert(input) : tokenContent;

			if (typeInfo.getTypedIfComplexType<ComplexType>() == p.get())
				return matchesInput(input, codeToSearch);
		}
		catch (ParserHelpers::Error& )
		{
			return false;
		}
	}

	return false;
}

void debug::PreprocessorMacroProvider::addTokens(mcl::TokenCollection::List& tokens)
{
	Preprocessor p(doc.getAllContent());

	p.addDefinitionsFromScope(GlobalScope::getDefaultDefinitions());

	try
	{
		p.process();
	}
	catch (ParserHelpers::Error& e)
	{
		DBG(e.toString());
		ignoreUnused(e);
	}

	for (auto ad : p.getAutocompleteData())
	{
		tokens.add(new PreprocessorToken(ad.name, ad.codeToInsert, ad.description, ad.lineNumber));
	}
}

void debug::SymbolProvider::addTokens(mcl::TokenCollection::List& tokens)
{
	ApiDatabase::Instance db;
	c.reset();
	Types::SnexObjectDatabase::registerObjects(c, 2);

	c.compileJitObject(doc.getAllContent());

	auto ct = c.getNamespaceHandler().getComplexTypeList();

	nh = c.getNamespaceHandlerReference();

	for (auto c : ct)
	{
		if (FunctionClass::Ptr fc = c->getFunctionClass())
		{
			TokenCollection::List l;

			for (auto id : fc->getFunctionIds())
			{
				Array<FunctionData> fData;
				fc->addMatchingFunctions(fData, id);

				for (auto& f : fData)
					l.add(new ComplexMemberToken(*this, c, f));
			}

			if (auto st = dynamic_cast<StructType*>(c))
			{
				for (auto ni : l)
					db->addDocumentation(ni, st->id, ni->getCodeToInsert(""));
			}

			tokens.addArray(l);
		}
	}

	auto l2 = c.getNamespaceHandler().getTokenList();

	for (auto ni : l2)
		db->addDocumentation(ni, NamespacedIdentifier(ni->tokenContent), {});

	tokens.addArray(l2);
}

void debug::TemplateProvider::addTokens(mcl::TokenCollection::List& tokens)
{
	GlobalScope s;
	Compiler c(s);
	Types::SnexObjectDatabase::registerObjects(c, 2);

	ApiDatabase::Instance db;

	for (auto id : c.getNamespaceHandler().getTemplateClassTypes())
	{
		auto nt = new TemplateToken(id);
		tokens.add(nt);

		db->addDocumentation(nt, id.id.id, {});	
	}
}

mcl::FoldableLineRange::List debug::SnexLanguageManager::createLineRange(const CodeDocument& doc)
{
	auto lineRanges = LanguageManager::createLineRange(doc);

	snex::Preprocessor p(doc.getAllContent());

	auto s = p.getDeactivatedLines();

	for (int i = 0; i < s.getNumRanges(); i++)
	{
		auto r = s.getRange(i);
		lineRanges.add(new mcl::FoldableLineRange(doc, { r.getStart() - 2, r.getEnd() }));
	}

	return lineRanges;
}

struct LiveCodePopup::Data : public juce::DeletedAtShutdown
{
	static int64 getHash(const char* filename, int lineNumber)
	{
		return String(filename).hashCode() * lineNumber;
	}

	struct Item : public ReferenceCountedObject,
                  public LiveCodePopup::ItemBase
	{
		using List = ReferenceCountedArray<Item>;
		using Ptr = ReferenceCountedObjectPtr<Item>;

		Item(Types::ID returnType, const String& expression, const char* file_, int lineNumber_, const Array<Argument>& args) :
			r(Result::ok()),
			file(file_),
			lineNumber(lineNumber_),
			hash(getHash(file_, lineNumber))
		{
			memory.setDebugMode(true);

			originalCode << "// this function will be evaluated live\n";

			f.id = NamespacedIdentifier::fromString("getLiveValue");
			f.returnType = returnType;

			for (auto& a : args)
				f.args.add(std::get<0>(a));

			originalCode << f.getSignature({}, false);
			originalCode << "\n{\n\treturn " << expression << ";\n}\n";

			rebuild(originalCode);

			setNumLastValuesToStore(4096);
		}

		void setNumLastValuesToStore(int numValues)
		{
			lastValues.calloc(numValues);
			numLastValues = numValues;
			lastValueIndex = 0;
		}

		void rebuild(const String& code)
		{
			lastCode = code;
			snex::jit::Compiler c(memory);

#if SNEX_MIR_BACKEND
			mir::MirCompiler::setLibraryFunctions(c.getFunctionMap());
#endif
			obj = c.compileJitObject(lastCode);

			for (auto& s : memory.getPreprocessorDefinitions())
			{
				if (s.name == "NUM_DATA_POINTS")
				{
					setNumLastValuesToStore(s.value.getIntValue());
				}
			}

			r = c.getCompileResult();
			auto compiledFunction = obj["getLiveValue"];

			f.function = compiledFunction.function;
		}

		VariableStorage evaluate(const Array<Argument>& args) override
		{
			if (f)
			{
				int numArgs = 0;
				VariableStorage argValues[4];

				for (const auto& a : args)
				{
					argValues[numArgs++] = std::get<1>(a);
				}

				lastValue = f.callDynamic(argValues, numArgs);

				if (isPositiveAndBelow(lastValueIndex, numLastValues))
				{
					lastValues[lastValueIndex++] = lastValue.toDouble();
				}

				return lastValue;
			}
			else
			{
				return lastValue;
			}

		}

		GlobalScope memory;
		String originalCode;
		String lastCode;
		VariableStorage lastValue;
		FunctionData f;
		snex::JitObject obj;
		Result r;

		HeapBlock<double> lastValues;
		int numLastValues = 0;
		int lastValueIndex = -1;

		String file;
		int lineNumber;
		int64 hash;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Item);
	};

	struct Editor : public Component,
		public Timer,
		public snex::DebugHandler
	{
		Editor(Item::Ptr item_) :
			item(item_),
			codeDoc(),
			doc(codeDoc),
			editor(doc),
			resetOriginal("Reset"),
			resetCode("Reset"),
			resetGraph("Reset"),
			originalEditor(originalCode, &tokeniser),
			applyButton("Apply"),
			breakButton("Break")
		{
			codeDoc.replaceAllContent(item->lastCode);

			auto fullCode = StringArray::fromLines(File(item->file).loadFileAsString());

			GlobalHiseLookAndFeel::setTextEditorColours(cycleLength);

			addAndMakeVisible(cycleLength);
			cycleLength.setText("-1", dontSendNotification);

			addAndMakeVisible(breakButton);

			originalCode.setDisableUndo(true);
			originalCode.replaceAllContent(fullCode.joinIntoString("\n"));
			originalEditor.setReadOnly(true);
			originalEditor.setColourScheme(editor.colourScheme);

			originalEditor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF282829));
			originalEditor.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colours::white.withAlpha(0.05f));
			originalEditor.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white.withAlpha(0.3f));
			originalEditor.getScrollbar(true).setVisible(false);

			originalEditor.setScrollbarThickness(13);

			fader.addScrollBarToAnimate(originalEditor.getScrollbar(true));
			fader.addScrollBarToAnimate(originalEditor.getScrollbar(false));

			addAndMakeVisible(originalEditor);
			addAndMakeVisible(editor);

			int styleFlags = 0;

			styleFlags |= ComponentPeer::StyleFlags::windowHasCloseButton;
			styleFlags |= ComponentPeer::StyleFlags::windowIsResizable;
			styleFlags |= ComponentPeer::StyleFlags::windowHasTitleBar;
			styleFlags |= ComponentPeer::StyleFlags::windowAppearsOnTaskbar;
			styleFlags |= ComponentPeer::StyleFlags::windowHasMaximiseButton;
			styleFlags |= ComponentPeer::StyleFlags::windowHasMinimiseButton;

			applyButton.setLookAndFeel(&alaf);
			errorLabel.setFont(GLOBAL_MONOSPACE_FONT());
			errorLabel.setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.6f));

			addAndMakeVisible(applyButton);
			addAndMakeVisible(errorLabel);
			addAndMakeVisible(breakButton);

			addAndMakeVisible(resetOriginal);
			addAndMakeVisible(resetCode);
			addAndMakeVisible(resetGraph);

			resetOriginal.setLookAndFeel(&alaf);
			resetCode.setLookAndFeel(&alaf);
			resetGraph.setLookAndFeel(&alaf);

			setName("Live Expression Editor");
			setOpaque(true);
			setVisible(true);

			setAlwaysOnTop(true);

			setBounds(100, 100, 700, 700);

			applyButton.onClick = [&]()
			{
				item->rebuild(codeDoc.getAllContent());
				checkError();
			};

			resetOriginal.onClick = [&]()
			{
				originalEditor.scrollToKeepLinesOnScreen({ item->lineNumber - 3, item->lineNumber + 3 });
			};

			resetGraph.onClick = [&]()
			{
				clearLogger();
			};

			resetCode.onClick = [&]()
			{
				codeDoc.replaceAllContent(item->originalCode);
				applyButton.triggerClick(sendNotificationAsync);
			};

			breakButton.onClick = [&]()
			{
				setVisible(false);
				jassertfalse;
				setVisible(true);
			};

			checkError();

			addToDesktop(styleFlags);

			startTimer(300);

			toFront(true);

			editor.grabKeyboardFocusAsync();

			item->memory.addDebugHandler(this);
		}

		~Editor()
		{
			item->memory.removeDebugHandler(this);
			item = nullptr;
		}

		int hurbel = 0;
		int numMessages = 0;

		void clearLogger() override
		{
			numMessages = 0;
			lastLogValueIndex = -1;
			logValueIndex = 0;
			logValues.clearQuick();

			auto maxLength = cycleLength.getText().getIntValue();

			if (maxLength > 0)
			{
				logValues.insertMultiple(0, 0.0f, maxLength);
			}
		}

		void logMessage(int level, const juce::String& s) override
		{
			if (level == 5) // AsmJitMessage
			{
				auto floatValue = s.fromFirstOccurrenceOf(":", false, false).getFloatValue();

				numMessages++;

				auto maxLength = cycleLength.getText().getIntValue();

				if (maxLength <= 0)
					logValues.add(floatValue);
				else
				{
					logValues.set(logValueIndex++, floatValue);

					if (logValueIndex >= maxLength)
						logValueIndex = 0;
				}
			}
		}

		int numLogValues = 0;
		Array<float> logValues;
		int logValueIndex = 0;
		int lastLogValueIndex = 0;
		Path logPath;
		Rectangle<float> pathBounds;
		float minLogValue = 0.0f;
		float maxLogValue = 0.0f;

		void timerCallback() override
		{
			if (numLogValues != logValues.size() || lastLogValueIndex != logValueIndex)
			{
				lastLogValueIndex = logValueIndex;
				numLogValues = logValues.size();

				logPath.clear();
				logPath.startNewSubPath(0.0f, logValues[0]);

				minLogValue = std::numeric_limits<float>::max();
				maxLogValue = std::numeric_limits<float>::min();

				for (int i = 1; i < numLogValues; i++)
				{
					minLogValue = jmin(minLogValue, logValues[i]);
					maxLogValue = jmax(maxLogValue, logValues[i]);

					logPath.lineTo((float)i, -1.0f * logValues[i]);
				}

				if (!logPath.getBounds().isEmpty())
					logPath.scaleToFit(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(), false);

				repaint();
			}
		}

		void checkError()
		{
			auto r = item->r;
			errorLabel.setText(r.wasOk() ? "OK" : r.getErrorMessage(), dontSendNotification);
		}

		bool keyPressed(const KeyPress& k) override
		{
			if (k == KeyPress::F5Key)
			{
				applyButton.triggerClick(sendNotificationAsync);
				return true;
			}

			return false;
		}

		void userTriedToCloseWindow() override
		{
			removeFromDesktop();

			MessageManager::callAsync([this]()
			{
				getInstance()->editors.removeObject(this, true);
			});
		}

		void resized() override
		{
			auto b = getLocalBounds();

			auto bottomBar = b.removeFromBottom(24);

			applyButton.setBounds(bottomBar.removeFromRight(80));
			breakButton.setBounds(bottomBar.removeFromLeft(80));
			errorLabel.setBounds(bottomBar);

			originalLabel = b.removeFromTop(24).toFloat();
			originalLabel.removeFromLeft(10.0f);
			resetOriginal.setBounds(originalLabel.toNearestInt().removeFromRight(50).reduced(2));
			originalEditor.setBounds(b.removeFromTop(7 * originalEditor.getLineHeight()));
			originalEditor.scrollToKeepLinesOnScreen({ item->lineNumber - 3, item->lineNumber + 3 });

			editor.setBounds(b.removeFromBottom(160));
			codeLabel = b.removeFromBottom(24).toFloat();
			codeLabel.removeFromLeft(10.0f);
			resetCode.setBounds(codeLabel.toNearestInt().removeFromRight(50).reduced(2));

			pathLabel = b.removeFromTop(24).toFloat();
			pathLabel.removeFromLeft(10.0f);
			pathBounds = b.toFloat();

			if (!logPath.getBounds().isEmpty())
			{
				logPath.scaleToFit(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(), false);
			}

			resetGraph.setBounds(pathLabel.removeFromRight(50).reduced(2).toNearestInt());
			cycleLength.setBounds(pathLabel.removeFromRight(50).reduced(2).toNearestInt());






			repaint();
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF222222));
			g.setColour(Colours::white.withAlpha(0.8f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Original code preview", originalLabel, Justification::left);
			g.drawText("Live expression code", codeLabel, Justification::left);
			g.drawText("Logged value graph", pathLabel, Justification::left);
			g.drawText("Max sample length: ", pathLabel, Justification::right);

			g.setColour(Colour(0xFF282829));
			g.fillRect(pathBounds);

			g.setColour(Colours::white.withAlpha(0.2f));

			if (logValues.size() == 1)
			{
				g.drawText(String(logValues[0]), pathBounds, Justification::centred);
			}
			else if (!logPath.isEmpty())
			{
				g.drawText("0", pathBounds, Justification::left);
				g.drawText(String(logValues.size()), pathBounds.translated(0.0f, 10.f), Justification::right);
				g.drawText(String(logValues.size()), pathBounds.translated(0.0f, 10.f), Justification::left);
				g.drawText(String(minLogValue, 2), pathBounds, Justification::bottomLeft);
				g.drawText(String(maxLogValue, 2), pathBounds, Justification::topLeft);

				g.drawText(String(minLogValue, 2), pathBounds, Justification::bottomRight);
				g.drawText(String(maxLogValue, 2), pathBounds, Justification::topRight);

				g.drawHorizontalLine(pathBounds.getCentreY(), 0.0f, (float)getWidth());

				g.setColour(Colours::white.withAlpha(0.8f));
				g.strokePath(logPath, PathStrokeType(1.0f));
			}
			else
			{
				g.drawText("No path. Use Console.print(value) to log values", pathBounds, Justification::centred);
			}
		}

		Rectangle<float> originalLabel, codeLabel, pathLabel;


		hise::AlertWindowLookAndFeel alaf;
		juce::CodeDocument codeDoc;
		mcl::TextDocument doc;
		mcl::TextEditor editor;
		TextButton applyButton;
		Label errorLabel;
		Item::Ptr item;

		TextButton resetOriginal;
		TextButton resetCode;
		TextButton resetGraph;
		TextButton breakButton;

		CodeDocument originalCode;
		juce::CPlusPlusCodeTokeniser tokeniser;
		CodeEditorComponent originalEditor;

		ScrollbarFader fader;
		TextEditor cycleLength;

	};

	Item::Ptr getItem(const String& expression, const char* file, int lineNumber, Types::ID returnType, const Array<Argument>& args)
	{
		auto thisHash = getHash(file, lineNumber);

		for (auto s : items)
		{
			if (s->hash == thisHash)
				return s;
		}

		items.add(new Item(returnType, expression, file, lineNumber, args));
		auto l = items.getLast();

		MessageManager::callAsync([this, l]()
		{
			editors.add(new Editor(l));
		});

		return l;
	}

	Item::List items;

	OwnedArray<Component> editors;
};

LiveCodePopup::Data* LiveCodePopup::instance = nullptr;

LiveCodePopup::ItemBase* LiveCodePopup::getItem(const String& expression, const char* file, int lineNumber, Types::ID returnType, const Array<Argument>& args)
{
    return getInstance()->getItem(expression, file, lineNumber, returnType, args).get();
}

snex::LiveCodePopup::Data* LiveCodePopup::getInstance()
{
	if (instance == nullptr)
		instance = new Data();

	return instance;
}

}
