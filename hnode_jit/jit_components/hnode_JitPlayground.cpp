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

namespace hnode {
namespace jit {
using namespace juce;


JitPlayground::JitPlayground() :
	editor(doc, &tokeniser),
	assembly(assemblyDoc, &assemblyTokeniser),
	console(consoleContent, &consoleTokeniser)
{
	setLookAndFeel(&laf);

	addAndMakeVisible(editor);
	addAndMakeVisible(console);

	editor.setFont(GLOBAL_MONOSPACE_FONT());
	editor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF333333));
	editor.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);

	editor.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xFF555555));

	

	testSignal.setColour(ComboBox::ColourIds::backgroundColourId, Colour(0xFF444444));

	testSignal.setColour(ComboBox::ColourIds::textColourId, Colours::white);
	testSignal.setColour(ComboBox::ColourIds::arrowColourId, Colours::white);
	testSignal.setColour(ComboBox::ColourIds::outlineColourId, Colours::transparentBlack);

	editor.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

	editor.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));

	addAndMakeVisible(graph);

	assembly.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xFF555555));
	assembly.setFont(GLOBAL_MONOSPACE_FONT());
	assembly.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF333333));
	assembly.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);

	assembly.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

	assembly.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));

	assembly.setReadOnly(true);

	console.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xFF555555));
	console.setFont(GLOBAL_MONOSPACE_FONT());
	console.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF333333));
	console.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);

	console.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

	console.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));

	console.setReadOnly(true);

	console.setLineNumbersShown(false);

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

	testSignal.addItemList({ "Noise", "Ramp", "Fast Ramp" }, 1);
	addAndMakeVisible(testSignal);
	testSignal.addListener(this);

	editor.setColourScheme(scheme);

	addAndMakeVisible(assembly);

	addAndMakeVisible(resultLabel);
	resultLabel.setFont(Font("Consolas", 14.0f, Font::plain));
	resultLabel.setColour(juce::Label::ColourIds::backgroundColourId, Colour(0xFF444444));
	resultLabel.setColour(juce::Label::ColourIds::textColourId, Colours::white);
	resultLabel.setEditable(false);
}

void JitPlayground::paint(Graphics& g)
{

}

void JitPlayground::resized()
{
	auto area = getLocalBounds();

	resultLabel.setBounds(area.removeFromBottom(30));

	auto left = area.removeFromRight(600);

	console.setBounds(left.removeFromBottom(300));

	graph.setBounds(left.removeFromBottom(100));
	testSignal.setBounds(left.removeFromBottom(30));

	assembly.setBounds(left);
	editor.setBounds(area);
}




#define JIT_MEMBER_WRAPPER_0(R, C, N)					  static R N(void* o) { return static_cast<C*>(o)->N(); };
#define JIT_MEMBER_WRAPPER_1(R, C, N, T1)				  static R N(void* o, T1 a1) { return static_cast<C*>(o)->N(a1); };
#define JIT_MEMBER_WRAPPER_2(R, C, N, T1, T2)			  static R N(void* o, T1 a1, T2 a2, ) { return static_cast<C*>(o)->N(a1, a2); };
#define JIT_MEMBER_WRAPPER_3(R, C, N, T1, T2, T3)		  static R N(void* o, T1 a1, T2 a2, T3 a3, ) { return static_cast<C*>(o)->N(a1, a2, a3); };
#define JIT_MEMBER_WRAPPER_4(R, C, N, T1, T2, T3, T4)	  static R N(void* o, T1 a1, T2 a2, T3 a3, T4 a4) { return static_cast<C*>(o)->N(a1, a2, a3, a4); };
#define JIT_MEMBER_WRAPPER_5(R, C, N, T1, T2, T3, T4, T5) static R N(void* o, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) { return static_cast<C*>(o)->N(a1, a2, a3, a4, a5); };

#define ADD_JIT_INFO(ReturnType, FunctionName, ...) if( functionId == Identifier(#FunctionName)) { f->function = Wrappers::FunctionName; f->returnType = ReturnType; f->args = { __VA_ARGS__ }; return f;}



struct Funky: public JitCallableObject
{
public:

	Funky(Identifier id) :
		JitCallableObject(id)
	{};

	float getMaxFunk(int i1)
	{
		return i1 + value;
	}

	int get5(float f1, float f2, float f3, float f4, float f5)
	{
		return (int)(f1*f2*f3*f4*f5);
	}

	float getValue() const
	{
		return value;
	}

	struct Wrappers
	{
		JIT_MEMBER_WRAPPER_0(float, Funky, getValue);
		JIT_MEMBER_WRAPPER_1(float, Funky, getMaxFunk, int);
		JIT_MEMBER_WRAPPER_5(int, Funky, get5, float, float, float, float, float)
			
	};

	void registerAllObjectFunctions(GlobalScope* memory)
	{
		auto f = JitCallableObject::createMemberFunctionForJitCode("getValue");
		f->returnType = Types::ID::Float;
		f->function = reinterpret_cast<void*>(Wrappers::getValue);
		f->args = {};

		addFunction(f);
	}

private:

	float value = 12.0f;
};

void JitPlayground::recalculate()
{
	b.setSize(1, 44100);
	createTestSignal();

	jit::GlobalScope memory;
	memory.allocate("x", 2.0f);

	String s;
	s << doc.getAllContent();

	consoleContent.replaceAllContent({});
	consoleContent.clearUndoHistory();

	Compiler cc(memory);
	cc.setDebugHandler(this);

	memory.addFunctionClass(new Funky("Funky"));

	auto functions = cc.compileJitObject(s);

	auto data = functions["processStereo"];

	assemblyDoc.replaceAllContent(cc.getAssemblyCode());

	


	block bl(b.getWritePointer(0), b.getNumSamples());

	auto start = Time::getMillisecondCounterHiRes();

	try
	{

		data.callVoid(bl);
	}
	catch (Types::OutOfBoundsException& exception)
	{
		String error;

		error << "Out of bounds buffer access: " << String(exception.index);
		resultLabel.setText(error, dontSendNotification);
	}
	

	auto stop = Time::getMillisecondCounterHiRes();

	auto duration = stop - start;

	//auto resultValue = 1.0f;

	String rs;

	rs << "Compiled OK. Time: " << String(duration * 0.1) << "%";

	resultLabel.setText(rs, dontSendNotification);


	//auto r = functions->getVariable("x");

	//auto value = Types::Helpers::getCppValueString(r);

	//logMessage("Result: " + String(result));
	//logMessage("X is " + value);

	graph.setBuffer(b);

	return;
	
#if 0
	Funky ff("ff");

	ff.registerToMemoryPool(&memory);

	hnode::jit::JITCompiler compiler(s);
	
	ScopedPointer<JITScope> scope1 = compiler.compileAndReturnScope(&memory);

	if (scope1 == nullptr)
	{
		resultLabel.setText(compiler.getErrorMessage(), dontSendNotification);
		return;
	}
	else
	{
		String s = scope1->dumpAssembly();
		assemblyDoc.replaceAllContent(s);
	}

	auto data = FunkyFunctionData::create<float, float>("get_1");

	auto fResult = scope1->getCompiledFunction(data);


	if (fResult.wasOk())
	{
		auto start = Time::getMillisecondCounterHiRes();

		auto d = b.getWritePointer(0);

		//for (int i = 0; i < 44100; i++)
//			d[i] = f1(d[i]);

		auto stop = Time::getMillisecondCounterHiRes();

		auto duration = stop - start;

		//auto resultValue = 1.0f;
		auto resultValue = data.call<float>(1.0f);

		graph.setBuffer(b);

		String s;

		s << "Compiled OK. Result for 1.0f: " << String(resultValue) << ". Time: " << String(duration * 0.1) << "%";

		resultLabel.setText(s, dontSendNotification);
	}
	else
	{
		resultLabel.setText(fResult.getErrorMessage(), dontSendNotification);
	}
#endif
}

bool JitPlayground::keyPressed(const KeyPress& k)
{
	if (k.getKeyCode() == KeyPress::F5Key)
	{
		recalculate();


		return true;
	}

	return false;
}

void JitPlayground::createTestSignal()
{
	auto d = b.getWritePointer(0);

	int signalType = testSignal.getSelectedItemIndex();

	for (int i = 0; i < b.getNumSamples(); i++)
	{
		switch (signalType)
		{
		case 0: d[i] = Random::getSystemRandom().nextFloat(); break;
		case 1:	d[i] = (float)i / (float)b.getNumSamples(); break;
		case 2:	d[i] = fmodf((float)i / (float)b.getNumSamples() * 30.0f, 1.0f); break;
		}

	}
}

void Graph::setBuffer(AudioSampleBuffer& b)
{
	p.clear();

	p.startNewSubPath(0.0f, 1.0f);

	int samplesPerPixel = b.getNumSamples() / getWidth();

	for (int i = 0; i < b.getNumSamples(); i += samplesPerPixel)
	{
		int numToDo = jmin(samplesPerPixel, b.getNumSamples() - i);
		auto range = FloatVectorOperations::findMinAndMax(b.getWritePointer(0, i), numToDo);

		if (range.getEnd() > (-1.0f * range.getStart()))
		{
			p.lineTo((float)i, 1.0f - range.getEnd());
		}
		else
			p.lineTo((float)i, 1.0f - range.getStart());
	}

	p.lineTo(b.getNumSamples(), 1.0f);
	p.closeSubPath();

	p.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), false);

	repaint();
}

void Graph::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF666666));
	g.setColour(Colours::white);
	g.fillPath(p);
}

int AssemblyTokeniser::readNextToken(CodeDocument::Iterator& source)
{
	auto c = source.nextChar();

	if (c == ';')
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

	return scheme;
}



}
}
