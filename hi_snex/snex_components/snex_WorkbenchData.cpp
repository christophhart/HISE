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

	s << "struct " << mainClass << nl;

	s << "{" << nl;
	s << "\t" << "void prepare(PrepareSpecs ps)" << nl;
	s << emptyBody;
	s << "\t" << "void reset()" << nl;
	s << emptyBody;
	s << "\t" << "template <typename ProcessDataType> void process(ProcessDataType& data)" << nl;
	s << emptyBody;
	s << "\t" << "template <int C> void processFrame(span<float, C>& data)" << nl;
	s << emptyBody;
	s << "\t" << "void handleEvent(HiseEvent& e)" << nl;
	s << emptyBody;

	s << "};" << nl;

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
	case jit::BaseCompiler::AsmJitMessage: m << "OUTPUT "; break;
	case jit::BaseCompiler::Blink:	m << "BLINK at line ";
	default: break;
	}

	m << getInstanceId() << ":";

	m << s;

	return m;
}

void ui::WorkbenchData::handleBlinks()
{
	for (auto p : pendingBlinks)
	{
		for (auto l : listeners)
		{
			if (l.get() != nullptr)
				l->logMessage(this, BaseCompiler::MessageType::Blink, String(p));
		}
	}

	pendingBlinks.clearQuick();
}

bool ui::WorkbenchData::handleCompilation()
{
	if (getGlobalScope().getBreakpointHandler().shouldAbort())
		return true;

	if (compileHandler != nullptr)
	{
		auto s = getCode();

		if (codeProvider != nullptr)
			codeProvider->preprocess(s);

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->preprocess(s);
		}

		if (getGlobalScope().getBreakpointHandler().shouldAbort())
			return true;

		lastCompileResult = compileHandler->compile(s);

		MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(WorkbenchData::postCompile));

		compileHandler->postCompile(lastCompileResult);

		// Might get deleted in the meantime...
		if (compileHandler != nullptr)
		{
			MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(WorkbenchData::postPostCompile));
		}
	}

	return true;
}

void ui::WorkbenchManager::workbenchChanged(WorkbenchData::Ptr oldWorkBench, WorkbenchData::Ptr newWorkbench)
{
	jassert(oldWorkBench.get() == currentWb.get());

	currentWb = newWorkbench.get();

	if (currentWb != nullptr)
		logMessage(currentWb.get(), jit::BaseCompiler::VerboseProcessMessage, "Switched to " + currentWb->getInstanceId());
}



void ui::ValueTreeCodeProvider::timerCallback()
{
	auto f = snex::JitFileTestCase::getTestFileDirectory().getChildFile("node.xml");

	if (ScopedPointer<XmlElement> xml = XmlDocument::parse(f))
	{
		ValueTree v = ValueTree::fromXml(*xml);

		if (!lastTree.isEquivalentTo(v))
		{
			lastTree = v;
			getParent()->triggerRecompile();

			

			
			

		}
	}
}

juce::Identifier ui::ValueTreeCodeProvider::getInstanceId() const
{
	return Identifier(lastTree[scriptnode::PropertyIds::ID].toString());
}

void ui::ValueTreeCodeProvider::rebuild() const
{
	snex::cppgen::ValueTreeBuilder vb(lastTree);
	lastResult = vb.createCppCode();
}

}