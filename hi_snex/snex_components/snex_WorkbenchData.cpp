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


String ui::WorkbenchData::getDefaultNodeTemplate(const Identifier& mainClass)
{
	auto emitCommentLine = [](juce::String& code, const juce::String& comment)
	{
		code << "/** " << comment << " */\n";
	};

	String s;
	String nl = "\n";
	String emptyBody = "\t{\n\t\t\n\t}\n\t\n";

	s << "#define NUM_CHANNELS 2" << nl;

	s << "struct " << mainClass << nl;

	s << "{" << nl;
	s << "\t" << "void prepare(PrepareSpecs ps)" << nl;
	s << emptyBody;
	s << "\t" << "void reset()" << nl;
	s << emptyBody;
	s << "\t" << "void process(ProcessData<NUM_CHANNELS>& data)" << nl;
	s << emptyBody;
	s << "\t" << "void processFrame(span<float, NUM_CHANNELS>& data)" << nl;
	s << emptyBody;
	s << "\t" << "void handleEvent(HiseEvent& e)" << nl;
	s << emptyBody;
	
	s << "};" << nl;

	return s;
}

String ui::WorkbenchData::getTestTemplate()
{
	auto emitCommentLine = [](juce::String& code, const juce::String& comment)
	{
		code << "/** " << comment << " */\n";
	};

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



String ui::WorkbenchData::convertToLogMessage(int level, const String& s)
{
	juce::String m;

	switch (level)
	{
	case jit::BaseCompiler::Error:  m << "ERROR: "; break;
	case jit::BaseCompiler::Warning:  m << "WARNING: "; break;
	case jit::BaseCompiler::PassMessage: return {};// m << "PASS: "; break;
	case jit::BaseCompiler::ProcessMessage: return {};// m << "- "; break;
	case jit::BaseCompiler::VerboseProcessMessage: m << "-- "; break;
	default: break;
	}

	if (getConnectedFile().existsAsFile())
		m << getConnectedFile().getFileName() << ":";

	m << s;

	return m;
}

void ui::WorkbenchManager::workbenchChanged(WorkbenchData::Ptr oldWorkBench, WorkbenchData::Ptr newWorkbench)
{
	jassert(oldWorkBench.get() == currentWb.get());

	currentWb = newWorkbench.get();

	if (currentWb != nullptr && currentWb->getConnectedFile().existsAsFile())
	{
		logMessage(currentWb.get(), jit::BaseCompiler::VerboseProcessMessage, "Switched to " + currentWb->getConnectedFile().getFullPathName());
	}
}

}