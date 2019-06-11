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


scriptnode::DspHelpers::ParameterCallback DspHelpers::getFunctionFrom0To1ForRange(NormalisableRange<double> range, bool inverted, const ParameterCallback& originalFunction)
{
	if (RangeHelpers::isIdentity(range))
	{
		if (!inverted)
			return originalFunction;
		else
		{
			return [originalFunction](double normalisedValue)
			{
				originalFunction(1.0 - normalisedValue);
			};
		}
	}

	if (inverted)
	{
		return [range, originalFunction](double normalisedValue)
		{
			normalisedValue = 1.0 - normalisedValue;
			auto v = range.convertFrom0to1(normalisedValue);
			originalFunction(v);
		};
	}
	else
	{
		return [range, originalFunction](double normalisedValue)
		{
			auto v = range.convertFrom0to1(normalisedValue);
			originalFunction(v);
		};
	}
}

scriptnode::DspHelpers::ParameterCallback DspHelpers::wrapIntoConversionLambda(const Identifier& converterId, const ParameterCallback& originalFunction, NormalisableRange<double> range, bool inverted)
{
	using namespace ConverterIds;

	if (converterId == Identity)
		return getFunctionFrom0To1ForRange(range, inverted, originalFunction);
	if (converterId == Decibel2Gain)
	{
		return [originalFunction](double newValue)
		{
			double gain = Decibels::decibelsToGain(newValue);
			originalFunction(gain);
		};
	}
	if (converterId == Gain2Decibel)
	{
		return [originalFunction](double newValue)
		{
			double db = Decibels::gainToDecibels(newValue);
			originalFunction(db);
		};
	}
	if (converterId == SubtractFromOne)
	{
		auto withRange = getFunctionFrom0To1ForRange(range, false, originalFunction);

		return [withRange](double normedValue)
		{
			double v = jlimit(0.0, 1.0, 1.0 - normedValue);
			withRange(v);
		};
	}
	if (converterId == DryAmount)
	{
		return [originalFunction](double newValue)
		{
			auto invGain = jlimit(0.0, 1.0, 1.0 - newValue);
			auto invDb = Decibels::gainToDecibels(invGain);

			originalFunction(invDb);
		};
	}
	if (converterId == WetAmount)
	{
		return [originalFunction](double newValue)
		{
			auto db = Decibels::gainToDecibels(newValue);
			originalFunction(db);
		};
	}

	return getFunctionFrom0To1ForRange(range, inverted, originalFunction);
}


double DspHelpers::findPeak(const ProcessData& data)
{
	double max = 0.0;

	for (auto channel : data)
	{
		auto r = FloatVectorOperations::findMinAndMax(channel, data.size);
		auto thisMax = jmax<float>(std::abs(r.getStart()), std::abs(r.getEnd()));
		max = jmax(max, (double)thisMax);
	}

	return max;
}

void ProcessData::copyToFrameDynamic(float* frame) const
{
	for (int i = 0; i < numChannels; i++)
		frame[i] = *data[i];
}

void ProcessData::copyFromFrameAndAdvanceDynamic(const float* frame)
{
	for (int i = 0; i < numChannels; i++)
		*data[i]++ = frame[i];
}

void ProcessData::advanceChannelPointers(int sampleAmount/*=1*/)
{
	for (int i = 0; i < numChannels; i++)
		data[i] += sampleAmount;
}

scriptnode::ProcessData ProcessData::copyToRawArray(float** channelData, float* uninitialisedData, bool clearArray/*=true*/)
{
	for (int i = 0; i < numChannels; i++)
	{
		channelData[i] = uninitialisedData;

		if (clearArray)
			memset(channelData[i], 0, sizeof(float)*size);

		uninitialisedData += size;
	}

	ProcessData rd(channelData, numChannels, size);
	rd.eventBuffer = eventBuffer;
	return rd;
}

scriptnode::ProcessData ProcessData::copyTo(AudioSampleBuffer& buffer, int index)
{
	auto channelOffset = index * numChannels;

	int numChannelsToCopy = jmin(buffer.getNumChannels() - channelOffset, numChannels);
	int numSamplesToCopy = jmin(buffer.getNumSamples(), size);

	for (int i = 0; i < numChannelsToCopy; i++)
		buffer.copyFrom(i + channelOffset, 0, data[i], numSamplesToCopy);

	return referTo(buffer, index);
}


scriptnode::ProcessData ProcessData::referTo(AudioSampleBuffer& buffer, int index) const
{
	ProcessData d;

	auto channelOffset = index * numChannels;

	d.numChannels = jmin(buffer.getNumChannels() - channelOffset, numChannels);
	d.size = jmin(buffer.getNumSamples(), size);
	d.data = buffer.getArrayOfWritePointers() + channelOffset;
	d.eventBuffer = eventBuffer;

	return d;
}

scriptnode::ProcessData& ProcessData::operator+=(const ProcessData& other)
{
	jassert(numChannels == other.numChannels);
	jassert(size == other.size);

	for (int i = 0; i < numChannels; i++)
		FloatVectorOperations::add(data[i], other.data[i], size);

	return *this;
}

juce::String CodeHelpers::createIncludeFile()
{
	if (!includeDirectory.isDirectory())
		return {};

	auto includeFile = includeDirectory.getChildFile("includes.cpp");
	Array<File> cppFiles = includeDirectory.findChildFiles(File::findFiles, false, "*.cpp");
	String s;

	for (auto cppFile : cppFiles)
	{
		if (cppFile == includeFile)
			continue;

		s << "#include \"" << cppFile.getFullPathName().replaceCharacter('\\', '/') << "\"\n";
	}

	includeFile.replaceWithText(s);

	String hc;

	hc << "#include \"" << includeFile.getFullPathName().replaceCharacter('\\', '/') << "\"";

	return hc;
}

File CodeHelpers::includeDirectory;

void CodeHelpers::setIncludeDirectory(String filePath)
{
	if (filePath.isNotEmpty() && File::isAbsolutePath(filePath))
		includeDirectory = File(filePath);
	else
		includeDirectory = {};
}

juce::File CodeHelpers::getIncludeDirectory()
{
	return includeDirectory;
}

void CodeHelpers::initCustomCodeFolder(Processor* p)
{
	auto s = dynamic_cast<GlobalSettingManager*>(p->getMainController())->getSettingsObject().getSetting(HiseSettings::Compiler::CustomNodePath);
	setIncludeDirectory(s);
}

void CodeHelpers::addFileToCustomFolder(const String& filename, const String& content)
{
	if (getIncludeDirectory().isDirectory())
	{
		auto file = getIncludeDirectory().getChildFile(filename).withFileExtension(".cpp");

		String code;

		code << "/* Autogenerated code. Might get overwritten, so don't edit it. */\n";
		code << "using namespace juce;\n";
		code << "using namespace hise;\n\n";
		code << CppGen::Emitter::wrapIntoNamespace(content, "custom");

		file.replaceWithText(CppGen::Emitter::wrapIntoNamespace(code, "scriptnode"));

		PresetHandler::showMessageWindow("Node exported as C++ code", "The node was exported as C++ class as 'custom." + filename + "'. Recompile HISE to use the hardcoded version");

		createIncludeFile();
	}
	else
	{
		PresetHandler::showMessageWindow("Custom Path is not set", "You need to set the custom path folder in the HISE settings", PresetHandler::IconType::Error);
	}
}

}

