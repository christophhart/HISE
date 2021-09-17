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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;







void DspHelpers::increaseBuffer(AudioSampleBuffer& b, const PrepareSpecs& specs)
{
	auto numChannels = specs.numChannels;
	auto numSamples = specs.blockSize;

	if (numChannels != b.getNumChannels() ||
		b.getNumSamples() < numSamples)
	{
		b.setSize(numChannels, numSamples);
	}
}

void DspHelpers::increaseBuffer(snex::Types::heap<float>& b, const PrepareSpecs& specs)
{
	auto numChannels = specs.numChannels;
	auto numSamples = specs.blockSize;
	auto numElements = numChannels * numSamples;

	if (numElements > b.size())
		b.setSize(numElements);
}

void DspHelpers::setErrorIfFrameProcessing(const PrepareSpecs& ps)
{
	if (ps.blockSize == 1)
	{
		Error e;
		e.error = Error::IllegalFrameCall;
		throw e;
	}
}

void DspHelpers::setErrorIfNotOriginalSamplerate(const PrepareSpecs& ps, NodeBase* n)
{
	auto original = n->getRootNetwork()->getOriginalSampleRate();

	if (ps.sampleRate != original)
	{
		Error e;
		e.error = Error::SampleRateMismatch;
		e.expected = (int)original;
		e.actual = (int)ps.sampleRate;
		throw e;
	}
}



scriptnode::DspHelpers::ParameterCallback DspHelpers::getFunctionFrom0To1ForRange(InvertableParameterRange range, const ParameterCallback& originalFunction)
{
	if (RangeHelpers::isIdentity(range))
	{
		if (!range.inv)
			return originalFunction;
		else
		{
			return [originalFunction](double normalisedValue)
			{
				originalFunction(1.0 - normalisedValue);
			};
		}
	}

	return [range, originalFunction](double normalisedValue)
	{
		auto v = range.convertFrom0to1(normalisedValue);
		originalFunction(v);
	};
}

void DspHelpers::validate(PrepareSpecs sp, PrepareSpecs rp)
{
	auto isIdle = [](const PrepareSpecs& p)
	{
		return p.numChannels == 0 && p.sampleRate == 0.0 && p.blockSize == 0;
	};

	if (isIdle(sp) || isIdle(rp))
		return;

	if (sp.numChannels != rp.numChannels)
	{
		Error e;
		e.error = Error::ChannelMismatch;
		e.actual = rp.numChannels;
		e.expected = sp.numChannels;

		throw e;
	}

	if (sp.sampleRate != rp.sampleRate)
	{
		Error e;
		e.error = Error::SampleRateMismatch;
		e.actual = (int)rp.sampleRate;
		e.expected = (int)sp.sampleRate;
		throw e;
	}

	if (sp.blockSize != rp.blockSize)
	{
		Error e;
		e.error = Error::BlockSizeMismatch;
		e.actual = rp.blockSize;
		e.expected = sp.blockSize;
		throw e;
	}
}

void DspHelpers::throwIfFrame(PrepareSpecs ps)
{
	if (ps.blockSize == 1)
	{
		Error e;
		e.error = Error::IllegalFrameCall;
		throw e;
	}
}

#if 0
juce::String CodeHelpers::createIncludeFile(File targetDirectory)
{
	if (!targetDirectory.isDirectory())
		return {};

	auto includeFile = targetDirectory.getChildFile("includes.cpp");
	Array<File> cppFiles = targetDirectory.findChildFiles(File::findFiles, false, "*.cpp");
	String s;

	if(targetDirectory == projectIncludeDirectory)
		s << "#include \"JuceHeader.h\n\n\"";

	for (auto cppFile : cppFiles)
	{
		if (cppFile == includeFile)
			continue;

		auto fileName = 

		s << "#include \"" << cppFile.getFileName().replaceCharacter('\\', '/') << "\"\n";
	}

	String nl = "\n";

	if (targetDirectory == projectIncludeDirectory)
	{
		s << "// project factory" << nl;

		s << "namespace scriptnode" << nl;
		s << "{" << nl;
		s << "using namespace hise;" << nl;
		s << "using namespace juce;" << nl;

		s << "namespace project" << nl;
		s << "{" << nl;
		s << "DEFINE_FACTORY_FOR_NAMESPACE;" << nl;
		s << "}" << nl;
		s << "}" << nl;
	}

	includeFile.replaceWithText(s);

	String hc;

	hc << "#include \"" << includeFile.getFullPathName().replaceCharacter('\\', '/') << "\"";

	return hc;
}


void CodeHelpers::addFileInternal(const String& filename, const String& content, File targetDirectory)
{
	auto file = targetDirectory.getChildFile(filename).withFileExtension(".cpp");

	bool isProjectTarget = targetDirectory == projectIncludeDirectory;

	String namespaceId = isProjectTarget ? "project" : "custom";

	String code;

	code << "/* Autogenerated code. Might get overwritten, so don't edit it. */\n";
	code << "using namespace juce;\n";
	code << "using namespace hise;\n\n";
	code << CppGen::Emitter::wrapIntoNamespace(content, namespaceId);

	file.replaceWithText(CppGen::Emitter::wrapIntoNamespace(code, "scriptnode"));

	if (targetDirectory == getIncludeDirectory())
		PresetHandler::showMessageWindow("Node exported to Global Node Directory", "The node was exported as C++ class as 'custom." + filename + "'. Recompile HISE to use the hardcoded version.");
	else
		PresetHandler::showMessageWindow("Node exported to Project Directory", "The node was exported as C++ class as 'project." + filename + "'. Export your plugin to use the hardcoded version.");

	createIncludeFile(targetDirectory);
}

File CodeHelpers::includeDirectory;
File CodeHelpers::projectIncludeDirectory;

void CodeHelpers::setIncludeDirectory(String filePath)
{
	if (filePath.isNotEmpty() && File::isAbsolutePath(filePath))
		includeDirectory = File(filePath);
	else
		includeDirectory = File();
}

juce::File CodeHelpers::getIncludeDirectory()
{
	return includeDirectory;
}

void CodeHelpers::initCustomCodeFolder(Processor* p)
{
	auto s = dynamic_cast<GlobalSettingManager*>(p->getMainController())->getSettingsObject().getSetting(HiseSettings::Compiler::CustomNodePath);
	setIncludeDirectory(s);

#if HI_ENABLE_CUSTOM_NODE_LOCATION
	projectIncludeDirectory = GET_PROJECT_HANDLER(p).getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile("CustomNodes");

	if (!projectIncludeDirectory.isDirectory())
		projectIncludeDirectory.createDirectory();
#endif
}


bool CodeHelpers::customFolderIsDefined()
{
	return includeDirectory.isDirectory();
}

bool CodeHelpers::projectFolderIsDefined()
{
	return projectIncludeDirectory.isDirectory();
}

void CodeHelpers::addFileToCustomFolder(const String& filename, const String& content)
{
	addFileInternal(filename, content, includeDirectory);

}

void CodeHelpers::addFileToProjectFolder(const String& filename, const String& content)
{
	addFileInternal(filename, content, projectIncludeDirectory);
}
#endif

}

