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
		"Disabled"
	};
}

ParameterProperties::ParameterProperties()
{
	reset();
}

void ParameterProperties::reset()
{
	for(auto& m: modulationModes)
		m = ParameterMode::Disabled;

	anyConnected = false;

	for(auto& i: modulationConnectState)
		i = 0;

	for(auto& i: modulationToParameter)
		i = -1;

	for(auto& i: parameterToModulation)
		i = -1;

	numUsedModulationSlots = 0;
}

void ParameterProperties::setConnected(int parameterIndex, bool isConnected)
{
	if(isPositiveAndBelow(parameterIndex, NumMaxModulationSlots))
	{
		auto mi = getModulationChainIndex(parameterIndex);

		if(modulationModes[parameterIndex] != ParameterMode::Disabled)
		{
			modulationConnectState[mi] = isConnected ? 1 : 0;
			anyConnected |= isConnected;
		}
	}
}

void ParameterProperties::setModulationMode(int parameterIndex, ParameterMode m)
{
	if(isPositiveAndBelow(parameterIndex, NumMaxModulationSlots))
	{
		if(m != ParameterMode::Disabled)
		{
			modulationModes[parameterIndex] = m;
			parameterToModulation[parameterIndex] = numUsedModulationSlots;
			modulationToParameter[numUsedModulationSlots] = parameterIndex;
			numUsedModulationSlots++;
		}
		else
		{
			// Cannot reset the modulation without messing up the chain order...
			jassert(modulationModes[parameterIndex] == ParameterMode::Disabled);
		}
	}
}

void ParameterProperties::fromValueTree(const ValueTree& v)
{
	jassert(v.getType() == PropertyIds::Network);
	modulationBlocksize = (int)v.getProperty(PropertyIds::ModulationBlockSize, 0);

	if(modulationBlocksize != 0)
		modulationBlocksize = jlimit(8, 1024, nextPowerOfTwo(modulationBlocksize));

	reset();

	auto pTree = v.getChildWithName(PropertyIds::Node).getChildWithName(PropertyIds::Parameters);

	jassert(pTree.isValid());

	for(auto c: pTree)
	{
		auto idx = pTree.indexOf(c);

		if(isPositiveAndBelow(idx, NumMaxModulationSlots))
		{
			auto n = c[PropertyIds::ExternalModulation];
			auto isConnected = c.getChildWithName(PropertyIds::Connections).getNumChildren() > 0;

			setModulationMode(idx, getModeFromVar(n));
			setConnected(idx, isConnected);
		}
	}
}

bool ParameterProperties::isUsed(int modulationIndex) const
{
	if(modulationIndex == -1)
		return false;

	return modulationToParameter[modulationIndex] != -1;
}

bool ParameterProperties::isConnected(int modulationIndex) const
{
	if(anyConnected && isPositiveAndBelow(modulationIndex, numUsedModulationSlots))
		return (bool)modulationConnectState[modulationIndex] && isUsed(modulationIndex);

	return false;
}

int ParameterProperties::getBlockSize(int fullBlocksize) const
{
	return (modulationBlocksize == 0 || !anyConnected) ? fullBlocksize : jmin(fullBlocksize, modulationBlocksize);
}

void ParameterProperties::writeToStream(OutputStream& output) const
{
	output.writeByte(58);
	output.writeCompressedInt(modulationBlocksize);
	static_assert(sizeof(std::array<ParameterMode, NumMaxModulationSlots>) == 16, "not 16 byte");
	output.write(modulationModes.data(), NumMaxModulationSlots);
	output.write(modulationConnectState.data(), NumMaxModulationSlots);
}

void ParameterProperties::readFromStream(InputStream& input)
{
	auto ok = input.readByte();

	if(ok == 58)
	{
		reset();
		modulationBlocksize = input.readCompressedInt();
		ParameterMode modes[NumMaxModulationSlots];
		input.read(modes, NumMaxModulationSlots);
		char bf[NumMaxModulationSlots];
		input.read(bf, NumMaxModulationSlots);
			
		for(int pi = 0; pi < NumMaxModulationSlots; pi++)
		{
			setModulationMode(pi, modes[pi]);
			auto mi = getModulationChainIndex(pi);
			setConnected(pi, bf[mi] == 1);
		}
	}
}

int ParameterProperties::getParameterIndex(int modulationIndex) const
{
	if(modulationIndex == -1 || numUsedModulationSlots == 0)
		return -1;

	auto pIndex = (int)modulationToParameter[modulationIndex];

	if(pIndex == -1)
		return -1;

	jassert(pIndex != -1);
	jassert(modulationIndex == (int)parameterToModulation[pIndex]);
	return pIndex;
}

int ParameterProperties::getModulationChainIndex(int parameterIndex) const
{
	if(isPositiveAndBelow(parameterIndex, NumMaxModulationSlots) && numUsedModulationSlots > 0)
	{
		auto mIndex = (int)parameterToModulation[parameterIndex];

		if(mIndex == -1)
			return -1;

		jassert(parameterIndex == (int)modulationToParameter[mIndex]);
		return mIndex;
	}

	return -1;
}

ParameterMode ParameterProperties::getModeFromVar(const var& value)
{
	auto n = value.toString();
	auto mi = getModulationModeNames().indexOf(n);
	return mi != -1 ? (ParameterMode)mi : ParameterMode::Disabled;
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
