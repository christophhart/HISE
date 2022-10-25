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

GlobalScriptCompileBroadcaster::GlobalScriptCompileBroadcaster() :
	useBackgroundCompiling(false),
	enableGlobalRecompile(true),
	globalEditBroadcaster(new ScriptComponentEditBroadcaster())
{
	createDummyLoader();
}



void GlobalScriptCompileBroadcaster::sendScriptCompileMessage(JavascriptProcessor *processorThatWasCompiled)
{
	if (!enableGlobalRecompile) return;

	for (int i = 0; i < listenerListStart.size(); i++)
	{
		if (listenerListStart[i].get() != nullptr)
		{
			listenerListStart[i].get()->scriptWasCompiled(processorThatWasCompiled);
		}
		else
		{
			listenerListStart.remove(i--);
		}
	}

	for (int i = 0; i < listenerListEnd.size(); i++)
	{
		if (listenerListEnd[i].get() != nullptr)
		{
			listenerListEnd[i].get()->scriptWasCompiled(processorThatWasCompiled);
		}
		else
		{
			listenerListEnd.remove(i--);
		}
	}
}

double GlobalScriptCompileBroadcaster::getCompileTimeOut() const noexcept
{
	return jmax(2.0, (double)dynamic_cast<const GlobalSettingManager*>(this)->getSettingsObject().getSetting(HiseSettings::Scripting::CompileTimeout));

}

bool GlobalScriptCompileBroadcaster::isCallStackEnabled() const noexcept
{
	return (bool)dynamic_cast<const GlobalSettingManager*>(this)->getSettingsObject().getSetting(HiseSettings::Scripting::EnableCallstack);
}

void GlobalScriptCompileBroadcaster::updateCallstackSettingForExistingScriptProcessors()
{
	auto shouldBeEnabled = isCallStackEnabled();

	ModulatorSynthChain *mainChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

	Processor::Iterator<JavascriptProcessor> iter(mainChain);

	while (auto jp = iter.getNextProcessor())
	{
		jp->setCallStackEnabled(shouldBeEnabled);
	}
}

void GlobalScriptCompileBroadcaster::fillExternalFileList(Array<File> &files, StringArray &processors)
{
	ModulatorSynthChain *mainChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

	Processor::Iterator<JavascriptProcessor> iter(mainChain);

	while (JavascriptProcessor *sp = iter.getNextProcessor())
	{
		for (int i = 0; i < sp->getNumWatchedFiles(); i++)
		{
			if (!files.contains(sp->getWatchedFile(i)))
			{
				files.add(sp->getWatchedFile(i));
				processors.add(dynamic_cast<Processor*>(sp)->getId());
			}



		}
	}
}

void GlobalScriptCompileBroadcaster::setExternalScriptData(const ValueTree &collectedExternalScripts)
{
	externalScripts = collectedExternalScripts;
}

String GlobalScriptCompileBroadcaster::getExternalScriptFromCollection(const String &fileName)
{
    static const String deviceWildcard = "{DEVICE}";

    String realFileName = fileName;
    
    if(realFileName.contains(deviceWildcard))
    {
        realFileName = realFileName.replace(deviceWildcard, HiseDeviceSimulator::getDeviceName());
    }
    
	for (int i = 0; i < externalScripts.getNumChildren(); i++)
	{
    const String thisName = externalScripts.getChild(i).getProperty("FileName").toString().replace("\\", "/");
        
		if (thisName == realFileName)
		{
			return externalScripts.getChild(i).getProperty("Content").toString();
		}
	}

	// Hitting this assert means you try to get a script that wasn't exported.
	jassertfalse;
	return String();
}

ExternalScriptFile::Ptr GlobalScriptCompileBroadcaster::getExternalScriptFile(const File& fileToInclude)
{
	for (int i = 0; i < includedFiles.size(); i++)
	{
		if (includedFiles[i]->getFile() == fileToInclude)
			return includedFiles[i];
	}

	includedFiles.add(new ExternalScriptFile(fileToInclude));

	return includedFiles.getLast();
}

void ExternalScriptFile::setRuntimeErrors(const Result& r)
{
	runtimeErrors.clearQuick();

	if (!r.wasOk())
	{
		StringArray sa = StringArray::fromLines(r.getErrorMessage());

		for (const auto& s : sa)
			runtimeErrors.add(RuntimeError(s));
	}

	runtimeErrorBroadcaster.sendMessage(sendNotification, &runtimeErrors);
}

ExternalScriptFile::RuntimeError::RuntimeError(const String& e)
{
	file = e.upToFirstOccurrenceOf("(", false, false);
	lineNumber = e.fromFirstOccurrenceOf("(", false, false).getIntValue();

	auto tokens = StringArray::fromTokens(e.fromFirstOccurrenceOf(")", false, false), ":", "");
	tokens.removeEmptyStrings();
	
	bool isWarning = tokens[0].trim() == "warning";

	errorLevel = isWarning ? ErrorLevel::Warning : ErrorLevel::Error;

	errorMessage = tokens[1].trim();

	if (errorMessage.isEmpty())
		errorLevel = ErrorLevel::Invalid;

}

bool ExternalScriptFile::RuntimeError::matches(const String& fileNameWithoutExtension) const
{
	return file.compareIgnoreCase(fileNameWithoutExtension) == 0;
}

String ExternalScriptFile::RuntimeError::toString() const
{
	/** Invert this: 
	auto s = e.fromFirstOccurrenceOf("Line ", false, false);
	auto l = s.getIntValue() - 1;
	auto c = s.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false).getIntValue();
	errorMessage = s.fromFirstOccurrenceOf(": ", false, false);
	*/

	String e;
	e << "Line " << String(lineNumber) << "(-1): " << errorMessage;
	return e;
}

} // namespace hise