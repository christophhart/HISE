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



ScriptingObject::ScriptingObject(ProcessorWithScriptingContent *p) :
processor(p),
thisAsProcessor(dynamic_cast<Processor*>(p))
{};

ProcessorWithScriptingContent *ScriptingObject::getScriptProcessor()
{
	return processor;
};

const ProcessorWithScriptingContent *ScriptingObject::getScriptProcessor() const
{
	return processor;
};


bool ScriptingObject::checkArguments(const String &callName, int numArguments, int expectedArgumentAmount)
{
	if (numArguments < expectedArgumentAmount)
	{
		String x;
		x << "Call to " << callName << " - Too few arguments: " << String(numArguments) << ", (Expected: " << String(expectedArgumentAmount) << ")";

		reportScriptError(x);
		return false;
	}

	return true;
}

int ScriptingObject::checkValidArguments(const var::NativeFunctionArgs &args)
{
	for (int i = 0; i < args.numArguments; i++)
	{
		if (args.arguments[i].isUndefined())
		{
			reportScriptError("Argument " + String(i) + " is undefined!");
			RETURN_IF_FRONTEND(i);
		}
	}

	return -1;
};



bool ScriptingObject::checkIfSynchronous(const Identifier &methodName) const
{
	const JavascriptMidiProcessor *sp = dynamic_cast<const JavascriptMidiProcessor*>(getScriptProcessor());

	if (sp == nullptr) return true; // HardcodedScriptProcessors are always synchronous

	if (sp->isDeferred())
	{
		reportScriptError("Illegal call of " + methodName.toString() + " (Can only be called in synchronous mode)");
	}

	return !sp->isDeferred();
}

void ScriptingObject::reportIllegalCall(const String &callName, const String &allowedCallback) const
{
	String x;
	x << "Call of " << callName << " outside of " << allowedCallback << " callback";

	reportScriptError(x);
};