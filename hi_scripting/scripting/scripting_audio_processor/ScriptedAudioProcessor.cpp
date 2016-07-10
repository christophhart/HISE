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



ScriptingAudioProcessor::ScriptingAudioProcessor() :
samplesPerBlock(0),
callbackResult(Result::ok()),
mc(nullptr),
libraryLoader(new DspFactory::LibraryLoader())
{
	compileScript();
}

ScriptingAudioProcessorEditor::ScriptingAudioProcessorEditor(AudioProcessor *p) :
AudioProcessorEditor(p),
tokenizer(new JavascriptTokeniser())
{
	addAndMakeVisible(controls = new GenericEditor(*p));
	addAndMakeVisible(codeEditor = new CodeEditorComponent(dynamic_cast<ScriptingAudioProcessor*>(p)->getDocument(), tokenizer));
	addAndMakeVisible(compileButton = new TextButton("Compile"));

	codeEditor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
	codeEditor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
	codeEditor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
	codeEditor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
	codeEditor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
	codeEditor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
	codeEditor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));

	compileButton->addListener(this);

	setSize(1000, controls->getHeight() + 600);
}

void ScriptingAudioProcessorEditor::resized()
{
	controls->setBounds(0, 0, getWidth(), jmax<int>(controls->getHeight(), 50));
	codeEditor->setBounds(0, controls->getBottom(), getWidth(), getHeight() - controls->getBottom());
	compileButton->setBounds(0, 0, 80, 20);
}

void ScriptingAudioProcessorEditor::buttonClicked(Button *b)
{
	if (b == compileButton)
	{
		dynamic_cast<ScriptingAudioProcessor*>(getAudioProcessor())->compileScript();
	}
}
