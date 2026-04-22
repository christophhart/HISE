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

namespace scriptnode
{
using namespace juce;
using namespace hise;

namespace modulation
{

VariableStorage EventData::getReadPointer() const
{
	if(type == SourceType::Disabled)
		return {};

	if(thisBlockSize != nullptr)
	{
		if(auto num = *thisBlockSize)
		{
			if(signal != nullptr)
				return block((float*)signal, num);
		}
	}
		
	return VariableStorage(constantValue);
}

StringArray ParameterProperties::getModulationModeNames()
{
	return {
		"Disabled",
		"Combined",
		"Gain",
		"Offset",	
		"Pan", 
		"Pitch"
	};
}

ParameterProperties::ParameterProperties()
{
	static_assert(NumMaxModulationParameterIndex < INT16_MAX, "go easy on the parameter index numbers");
	static_assert(NumMaxModulationSources <= 128, "The number of modulation slots is limited to 128");

	reset();
}

void ParameterProperties::reset()
{
	modulationBlocksize = 0;
	anyConnected = false;
	numUsedModulationSlots = 0;

	for(auto& m: modInfo)
		m = {};

	for(auto& p: parameterInfo)
		p = {};
}

void ParameterProperties::fromConnectionList(const ConnectionList& c)
{
	reset();

	if(c.size() < NumMaxModulationSources)
	{
		for (const auto& con : c)
		{
			setModulationMode(con.connectedParameterIndex, con.modulationMode);
			setConnected(con.connectedParameterIndex, con.connectionMode == ConnectionMode::Parameter);
			setColour(con.connectedParameterIndex, con.modColour);
		}
	}
	else
	{
		// You're trying to create more modulation slots than possible.
		jassertfalse;
	}
}

void ParameterProperties::setConnected(int16 parameterIndex, bool isConnected)
{
	if(isPositiveAndBelow(parameterIndex, NumMaxModulationParameterIndex))
	{
		if(parameterInfo[parameterIndex].mode  != ParameterMode::Disabled)
		{
			auto mi = getModulationChainIndex(parameterIndex);
			modInfo[mi].connectionMode = isConnected ? ConnectionMode::Parameter : ConnectionMode::ModulationNode;
			anyConnected |= isConnected;
		}
	}
}

void ParameterProperties::setModulationMode(int16 parameterIndex, ParameterMode m)
{
	if(isPositiveAndBelow(parameterIndex, NumMaxModulationParameterIndex))
	{
		if(m != ParameterMode::Disabled)
		{
			parameterInfo[parameterIndex].mode = m;
			parameterInfo[parameterIndex].modulationSlotIndex = numUsedModulationSlots;
			modInfo[numUsedModulationSlots].parameterIndex = parameterIndex;

			numUsedModulationSlots++;
		}
		else
		{
			// Cannot reset the modulation without messing up the chain order...
			jassert(parameterInfo[parameterIndex].mode == ParameterMode::Disabled);
		}
	}
}

void ParameterProperties::fromValueTree(const ValueTree& v)
{
	jassert(v.getType() == PropertyIds::Network);

	reset();

	modulationBlocksize = (int)v.getProperty(PropertyIds::ModulationBlockSize, 0);

	if(modulationBlocksize != 0)
		setModulationBlockSize(modulationBlocksize);

	auto pTree = v.getChildWithName(PropertyIds::Node).getChildWithName(PropertyIds::Parameters);

	jassert(pTree.isValid());

	for(auto c: pTree)
	{
		auto idx = pTree.indexOf(c);

		if(isPositiveAndBelow(idx, NumMaxModulationParameterIndex))
		{
			auto n = c[PropertyIds::ExternalModulation];
			auto isConnected = c.getChildWithName(PropertyIds::Connections).getNumChildren() > 0;

			auto getColourFromVar = [](var value)
			{
				if(value.isVoid() || value.isUndefined())
					return HiseModulationColours::ColourId::ExtraMod;

				return static_cast<HiseModulationColours::ColourId>((int)value);
			};

			setModulationMode(idx, getModeFromVar(n));
			setConnected(idx, isConnected);
			setColour(idx, getColourFromVar(c[PropertyIds::ModColour]));
		}
	}
}

bool ParameterProperties::isUsed(int8 modulationIndex) const
{
	if(!isPositiveAndBelow(modulationIndex, NumMaxModulationSources))
		return false;

	return modInfo[modulationIndex].parameterIndex != -1;
}

bool ParameterProperties::isConnected(int8 modulationIndex) const
{
	if(anyConnected && isPositiveAndBelow(modulationIndex, numUsedModulationSlots))
		return isUsed(modulationIndex) && modInfo[modulationIndex].connectionMode == ConnectionMode::Parameter;

	return false;
}

int ParameterProperties::getBlockSize(int fullBlocksize) const
{
	return (modulationBlocksize == 0 || !anyConnected) ? fullBlocksize : jmin(fullBlocksize, modulationBlocksize);
}

void ParameterProperties::writeToStream(OutputStream& output) const
{
	ConnectionInfo::List list;

	for(int i = 0; i < numUsedModulationSlots; i++)
	{
		ConnectionInfo ni;
		ni.connectedParameterIndex = modInfo[i].parameterIndex;
		jassert(parameterInfo[ni.connectedParameterIndex].modulationSlotIndex == i);

		ni.modulationMode = parameterInfo[ni.connectedParameterIndex].mode;
		jassert(ni.modulationMode != ParameterMode::Disabled);

		ni.modColour = modInfo[i].colour;
		ni.connectionMode = modInfo[i].connectionMode;

		list.push_back(ni);
	}

	ConnectionInfo::writeTo(list, output);

	output.writeByte(ConnectionInfo::BeginMetadata);
	output.writeCompressedInt(modulationBlocksize);
}

void ParameterProperties::readFromStream(InputStream& input)
{
	auto list = ConnectionInfo::readFrom(input);
	fromConnectionList(list);

	if(input.readByte() == ConnectionInfo::BeginMetadata)
		modulationBlocksize = input.readCompressedInt();
}

int16 ParameterProperties::getParameterIndex(int8 modulationIndex) const
{
	if(!isPositiveAndBelow(modulationIndex, NumMaxModulationSources) || numUsedModulationSlots == 0)
		return -1;

	auto pIndex = (int)modInfo[modulationIndex].parameterIndex;

	if(pIndex == -1)
		return -1;

	jassert(modulationIndex == parameterInfo[pIndex].modulationSlotIndex);
	return pIndex;
}

int8 ParameterProperties::getModulationChainIndex(int16 parameterIndex) const
{
	if(isPositiveAndBelow(parameterIndex, NumMaxModulationParameterIndex) && numUsedModulationSlots > 0)
	{
		auto mIndex = (int)parameterInfo[parameterIndex].modulationSlotIndex;

		if(mIndex == -1)
			return -1;

		jassert(parameterIndex == (int)modInfo[mIndex].parameterIndex);
		return mIndex;
	}

	return -1;
}

bool ParameterProperties::isAnyConnected() const noexcept
{
	return anyConnected;
}

scriptnode::modulation::ParameterMode ParameterProperties::getParameterMode(int16 parameterIndex) const noexcept
{
	if (isPositiveAndBelow(parameterIndex, NumMaxModulationParameterIndex))
		return parameterInfo[parameterIndex].mode;

	return ParameterMode::Disabled;
}

void ParameterProperties::setColour(int16 parameterIndex, HiseModulationColours::ColourId newColour)
{
	if (isPositiveAndBelow(parameterIndex, NumMaxModulationParameterIndex))
	{
		auto mi = getModulationChainIndex(parameterIndex);

		if (mi != -1)
		{
			if (parameterInfo[parameterIndex].mode != ParameterMode::Disabled)
				modInfo[mi].colour = newColour;
			else
				modInfo[mi].colour = HiseModulationColours::ColourId::ExtraMod;
		}
	}
}

void ParameterProperties::setModulationBlockSize(int newBlockSize)
{
	modulationBlocksize = jlimit(8, 1024, nextPowerOfTwo(newBlockSize));
}

juce::Colour ParameterProperties::getModulationColour(int16 parameterIndex) const
{
	HiseModulationColours colours;

	auto mi = getModulationChainIndex(parameterIndex);

	if (mi != -1)
		return colours.getColour(modInfo[mi].colour);

	return colours.getColour(HiseModulationColours::ColourId::ExtraMod);
}

hise::HiseModulationColours::ColourId ParameterProperties::getModulationColourRaw(int16 parameterIndex) const
{
	auto mi = getModulationChainIndex(parameterIndex);

	if (mi != -1)
		return modInfo[mi].colour;

	return hise::HiseModulationColours::ColourId::ExtraMod;
}

ParameterMode ParameterProperties::getModeFromVar(const var& value)
{
	auto n = value.toString();
	auto mi = getModulationModeNames().indexOf(n);
	return mi != -1 ? static_cast<ParameterMode>(mi) : ParameterMode::Disabled;
}


class ParameterPropertiesTest: public UnitTest
{
public:

	ParameterPropertiesTest():
	  UnitTest("Testing modulation::ParameterProperties", "modulation")
	{};

	void testReset()
	{
		beginTest("testing reset");

		ParameterProperties prop;
		prop.setModulationMode(8, ParameterMode::Pan);
		prop.setConnected(8, true);

		expect(prop.isUsed(0));
		expect(prop.getModulationChainIndex(8) == 0);
		expect(prop.getParameterIndex(0) == 8);
		expect(prop.isUsed(0));
		expect(prop.isConnected(0));
		expect(prop.anyConnected);
		expect(prop.numUsedModulationSlots == 1);

		prop.reset();

		expect(!prop.isUsed(0));
		expect(prop.getModulationChainIndex(8) == -1);
		expect(prop.getModulationChainIndex(0) == -1);
		expect(!prop.isUsed(0));
		expect(!prop.isConnected(0));
		expect(!prop.anyConnected);
		expect(prop.numUsedModulationSlots == 0);
	}

	void testReadWrite()
	{
		beginTest("testing read write");

		ParameterProperties p1;
		ParameterProperties p2;

		p1.setModulationMode(1, ParameterMode::AddOnly);
		p1.setModulationMode(3, ParameterMode::ScaleOnly);
		p1.setConnected(3, true);
		p1.setModulationMode(4, ParameterMode::Pan);
		p1.setConnected(4, false);

		expect(p1.getModulationChainIndex(0) == -1);
		expect(p1.getModulationChainIndex(1) == 0);
		expect(p1.getModulationChainIndex(3) == 1);
		expect(p1.getModulationChainIndex(4) == 2);

		expect(p1.getParameterIndex(0) == 1);
		expect(p1.getParameterIndex(1) == 3);
		expect(p1.getParameterIndex(2) == 4);
		expect(p1.getParameterIndex(3) == -1);

		expect(!p1.isConnected(0));
		expect(p1.isConnected(1));
		expect(!p1.isConnected(2));
		expect(!p1.isConnected(3));
		expect(!p1.isConnected(4));

		MemoryOutputStream mos;
		p1.modulationBlocksize = 128;
		p1.writeToStream(mos);

		mos.flush();
		MemoryInputStream mis(mos.getMemoryBlock());
		p2.setModulationMode(0, ParameterMode::ScaleAdd);
		p2.readFromStream(mis);
		

		expect(p2.getModulationChainIndex(0) == -1);
		expect(p2.getModulationChainIndex(1) == 0);
		expect(p2.getModulationChainIndex(3) == 1);
		expect(p2.getModulationChainIndex(4) == 2);

		expect(p2.getParameterIndex(0) == 1);
		expect(p2.getParameterIndex(1) == 3);
		expect(p2.getParameterIndex(2) == 4);
		expect(p2.getParameterIndex(3) == -1);

		expect(!p2.isConnected(0));
		expect(p2.isConnected(1));
		expect(!p2.isConnected(2));
		expect(!p2.isConnected(3));
		expect(!p2.isConnected(4));

		expect(p2.modulationBlocksize == 128);
	}

	void runTest() override
	{
		testReset();
		testReadWrite();
	}
};

static ParameterPropertiesTest ptest;

} // namespace modulation
} // namespace scriptnode
