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

static const char * sfz_opcodeNames[SfzImporter::numSupportedOpcodes] = 
{
		"sample",
		"lokey",
		"hikey",
		"lovel",
		"hivel",
		"offset",
		"end",
		"loop_mode",
		"loop_start",
		"loop_end",
		"tune",
		"pitch_keycenter",
		"volume",
		"group_volume",
		"pan",
		"group_label",
		"key",
		"default_path",
		"lorand",
		"hirand",
		"seq_length",
		"seq_position"
};

Identifier SfzImporter::getSamplerProperty(Opcode opcode)
{
	switch(opcode)
	{
	case sample:			return SampleIds::FileName;
	case lokey:				return SampleIds::LoKey;
	case hikey:				return SampleIds::HiKey;
	case lovel:				return SampleIds::LoVel;
	case hivel:				return SampleIds::HiVel;
	case offset:			return SampleIds::SampleStart;
	case end:				return SampleIds::SampleEnd;
	case loop_mode:			return SampleIds::LoopEnabled;
	case loop_start:			return SampleIds::LoopStart;
	case loop_end:			return SampleIds::LoopEnd;
	case tune:				return SampleIds::Pitch;
	case pitch_keycenter:	return SampleIds::Root;
	case volume:			return SampleIds::Volume;
	case group_volume:		return SampleIds::Volume; // increases the Volume Property.
	case pan:				return SampleIds::Pan;
	case default_path:		
	case lorand:			
	case hirand:			
	case seq_length:
	case seq_position:		return SampleIds::Unused;
    case numSupportedOpcodes: jassertfalse; 
	default:				jassertfalse; 
	};

	return {};
}

int getNoteNumberFromNameOrNumber(const String &data)
{
    if(RegexFunctions::matchesWildcard("[A-Ga-g]#?-?[0-9]", data))
    {
        const String noteName = data.toUpperCase();
        
        for(int i = 0; i < 127; i++)
        {
            if(noteName.contains(MidiMessage::getMidiNoteName(i, true, true, 3)))
            {
                return i;
            }
        }
        return -1;
    }
    
    return data.getIntValue();
}

var SfzImporter::combineOpcodeValue(Opcode o, var prevValue, var thisValue)
{
	return thisValue;
}

var SfzImporter::getOpcodeValue(Opcode o, const String &valueString) const
{
	switch(o)
	{
	case sample:		return valueString.replaceCharacter('\\', '/');
	case loop_mode:		return (valueString == "loop_continuous") ? var(1) : var(0);
    case lokey:
    case hikey:
    case pitch_keycenter: return var(getNoteNumberFromNameOrNumber(valueString));
	case default_path:  return valueString.replaceCharacter('\\', '/');
	case lorand:
	case hirand:		return var(valueString.getDoubleValue());
	default:			return var(valueString.getIntValue());
	}
}

const char **SfzImporter::opcodeNames = sfz_opcodeNames;


void SfzImporter::debugRoot()
{
#if JUCE_DEBUG
	if (root != nullptr)
	{
		String s;
		int il = 0;
		root->toDbgString(s, il);
	}
#endif
}

void SfzImporter::parseTagLine(const String &restOfLine)
{
	StringArray sa = getOpcodeTokens(restOfLine);

	for (int i = 0; i < sa.size(); i++)
	{
		parseOpcode(sa[i]);
	}
}

StringArray SfzImporter::getOpcodeTokens(const String &line) const
{
	String stringToTokenise = line.contains(">") ? line.fromFirstOccurrenceOf(">", false, false) : line; // skip tags

	StringArray tokens = StringArray::fromTokens(stringToTokenise, " ", "");

	tokens.removeEmptyStrings();

	for (int i = 0; i < tokens.size(); i++)
	{
		if (!tokens[i].contains("="))
		{
			if (i == 0) throw SfzParsingError(currentParseNumber, "Invalid token!");

			tokens.set(i - 1, tokens[i - 1] + " " + tokens[i]);
			tokens.remove(i--);

			continue;
		}
	}

	return tokens;
}

void SfzImporter::parseOpcode(const String &opcode)
{
	StringArray opcodeTokens = StringArray::fromTokens(opcode, "=", "");

	if(opcodeTokens.size() == 2)
	{
		const int opcodeIndex = getOpcode(opcodeTokens[0]);

		if(opcodeIndex == Opcode::groupName)
		{
			if(auto g = currentTarget->as<Group>())
				g->groupName = opcodeTokens[1];
			else
				throw SfzParsingError(currentParseNumber, "group name opcode outside of group definition");
		}
		else if(opcodeIndex != -1)
		{
			if(currentTarget != nullptr)
				currentTarget->setOpcodeValue(opcodeIndex, getOpcodeValue((Opcode)opcodeIndex, opcodeTokens[1]));
			else
				throw SfzParsingError(currentParseNumber, "No Region for opcode");
		}
	}
	else
	{
		throw SfzParsingError(currentParseNumber, "No opcode found");
	}
}

void SfzImporter::parseOpcodes()
{
	StringArray fileData;

	fileToImport.readLines(fileData);

	for (int i = 0; i < fileData.size(); i++)
	{
		String currentLine = fileData[i].trimStart().upToFirstOccurrenceOf("//", false, false);

		currentParseNumber++;

		if (currentLine.isEmpty() || !currentLine.containsNonWhitespaceChars()) continue; // Skip empty lines
		if (currentLine.startsWith("//")) continue; // Skip comments

		if (currentLine.startsWith(Control::getTag()))
		{
            if (currentTarget == nullptr)
                setIfRoot(currentTarget = new Global(currentTarget.get()));
            
			currentTarget = new Control(currentTarget.get());

			setIfRoot(currentTarget);
		}
		if (currentLine.startsWith(Global::getTag()))
		{
			navigateToParent<Control>(AllowNoParent);
			
			currentTarget = new Global(currentTarget.get());

			setIfRoot(currentTarget);
		}
		else if (currentLine.startsWith(Group::getTag()))
		{
			if (currentTarget == nullptr)
				setIfRoot(currentTarget = new Global(currentTarget.get()));

			navigateToParent<Global>(ThrowErrorIfNotFound);

			if (i < fileData.size() - 1)
			{
				auto g = new Group(currentTarget.get());
				currentTarget = g;
				auto groupIndex = currentTarget->parent->children.size();
				g->groupName = "Group " + String(groupIndex);

			}
			else
			{
				throw SfzParsingError(i, "Empty Group");
			}
		}
		else if (currentLine.startsWith(Region::getTag()))
		{
            if(currentTarget == nullptr)
            {
                setIfRoot(currentTarget = new Global(nullptr));
                auto g = new Group(currentTarget.get());
                g->groupName = "Group 1";
                currentTarget = g;
            }
            
			navigateToParent<Group>(ThrowErrorIfNotFound, "No Group for region");

			currentTarget = new Region(currentTarget.get());
		}

		parseTagLine(currentLine);
	}
}


void SfzImporter::applyGlobalOpcodesToRegion()
{
	auto currentControl = currentTarget->findParentTargetOfType<Control>();
	auto currentGlobal = currentTarget->findParentTargetOfType<Global>();

	for (auto c: currentGlobal->children)
	{
		for (auto r : c->children)
		{
			// Apply Control properties
			if (currentControl != nullptr)
				applyValueSetOnRegion(currentControl->opcodes, r->as<Region>());

			// Apply global properties
			if (currentGlobal != nullptr)
				applyValueSetOnRegion(currentGlobal->opcodes, r->as<Region>());

			// Apply group properties
			applyValueSetOnRegion(c->opcodes, r->as<Region>());
		}
	}
}


void SfzImporter::applyValueSetOnRegion(const NamedValueSet &setToApply, Region * r)
{
	for (int i = 0; i < setToApply.size(); i++)
	{
		Identifier opcodeId = setToApply.getName(i);

		if (setToApply[opcodeId].isUndefined())
		{
			jassertfalse;
			continue;
		}
			

		if (!r->opcodes.contains(opcodeId))
			r->opcodes.set(opcodeId, setToApply[opcodeId]);	
		else
		{
			auto prevValue = r->opcodes[opcodeId];
			auto thisValue = setToApply[opcodeId];
			//
			auto combinedValue = combineOpcodeValue((Opcode)getOpcode(opcodeId.toString()), prevValue, thisValue);
			r->opcodes.set(opcodeId, combinedValue);
		}
	}
}

SfzImporter::SfzImporter(ModulatorSampler *sampler_, const File &sfzFileToImport) :
sampler(sampler_),
fileToImport(sfzFileToImport),
currentTarget(nullptr),
currentParseNumber(0)
{
	
}

ValueTree SfzImporter::importSfzFile()
{
	parseOpcodes();

	debugRoot();

	applyGlobalOpcodesToRegion();

	debugRoot();

	ValueTree v("samplemap");

	v.setProperty("RelativePath", 0, nullptr);
	
	v.setProperty("FileName", fileToImport.getFullPathName(), nullptr);
	
	v.setProperty("SaveMode", 1, nullptr);

	auto firstGlobal = root->findFirstChildOfType<Global>();

	
	int id = 0;

	int groupAmount = 0;

	for (int i = 0; i < firstGlobal->children.size(); i++)
	{
		const int rrGroup = i+1;

		groupAmount = jmax(rrGroup, groupAmount);

		if(rrGroup == -1) continue;

		for (int j = 0; j < firstGlobal->children[i]->children.size(); j++)
		{
			Region *region = firstGlobal->children[i]->children[j]->as<Region>();

			ValueTree sample("sample");

			id++;

			sample.setProperty(SampleIds::ID, id, nullptr);

			sample.setProperty(SampleIds::LoVel, 0, nullptr);
			sample.setProperty(SampleIds::HiVel, 127, nullptr);

			for(int k = 0; k < Opcode::numSupportedOpcodes; k++)
			{
				Identifier opcode = Identifier(getOpcodeName((Opcode)k));

				if (k == Opcode::groupName) continue;

				auto opValue = region->opcodes[opcode];

				bool isEmpty = opValue.isUndefined() || opValue.isVoid();

				if (!isEmpty)
				{
					if (k == Opcode::key)
					{
						auto v = region->opcodes[opcode];

						sample.setProperty(SampleIds::Root, opValue, nullptr);
						sample.setProperty(SampleIds::LoKey, opValue, nullptr);
						sample.setProperty(SampleIds::HiKey, opValue, nullptr);

						continue;
					}

					auto p = getSamplerProperty((Opcode)k);

					if (p == SampleIds::Unused)
						continue;

					

					if (p == SampleIds::FileName)
					{
						auto path = region->getRelativeFilePath();
						auto dir = fileToImport.getParentDirectory();

						auto f = dir.getChildFile(path);

						if (sampler != nullptr)
						{
							PoolReference ref(sampler->getMainController(), f.getFullPathName(), FileHandlerBase::Samples);
							sample.setProperty(p, ref.getReferenceString(), nullptr);
						}
						else
						{
							sample.setProperty(p, f.getFullPathName(), nullptr);
						}
					}
					else
					{
						sample.setProperty(p, opValue, nullptr);
					}
				}
			}

			Identifier groupVolumeOpcode = Identifier(getOpcodeName(group_volume));

			if ( !region->opcodes[groupVolumeOpcode].isUndefined() )
			{
				int zoneValue = sample.getProperty(SampleIds::Volume, var(0));

				int groupValue = region->opcodes[groupVolumeOpcode];

				int combinedLevel = zoneValue + groupValue;

				sample.setProperty(SampleIds::Volume, combinedLevel, nullptr);

			}

			if (auto customRRGroup = region->getRRGroup())
			{
				sample.setProperty(SampleIds::RRGroup, customRRGroup, nullptr);
				groupAmount = jmax(groupAmount, customRRGroup);
			}
			else
				sample.setProperty(SampleIds::RRGroup, rrGroup, nullptr);

			v.addChild(sample, -1, nullptr);
		}

	}

	v.setProperty("RRGroupAmount", jmax(1, groupAmount), nullptr);

	if (sampler != nullptr)
	{
		sampler->getSampleMap()->loadUnsavedValueTree(v);
		sampler->refreshPreloadSizes();
		sampler->refreshMemoryUsage();
	}

	return v;
};

void SfzImporter::SfzOpcodeTarget::toDbgString(String& s, int& intLevel)
{
	String intendation;
	for (int i = 0; i < intLevel; i++)
		intendation << ' ';

	s << intendation << getTagVirtual() << "\n";

	for (const auto& kv : opcodes)
		s << intendation << '-' << kv.name << ":" << kv.value.toString() << "\n";

	intLevel++;

	for (auto c : children)
		c->toDbgString(s, intLevel);

	intLevel--;
}

int SfzImporter::Region::getRRGroup() const
{
	Range<double> randomRange((*this)[Opcode::lorand], (*this)[Opcode::hirand]);

	if (!randomRange.isEmpty())
	{
		auto numRR = 1.0 / randomRange.getLength();
		return roundToInt(randomRange.getStart() * numRR) + 1;
	}

	if (auto slength = (int)(*this)[Opcode::seq_length])
	{
		return (int)(*this)[Opcode::seq_position];
	}
	
	return 0;
}

String SfzImporter::Region::getRelativeFilePath() const
{
	String s;

	s << (*this)[Opcode::default_path].toString().replaceCharacter('\\', '/');

	if (!s.endsWithChar('/'))
		s << '/';

	auto fn = (*this)[Opcode::sample].toString().replaceCharacter('\\', '/');

	if (fn.startsWithChar('/'))
		fn = fn.fromFirstOccurrenceOf("/", false, false);

	s << fn;

	if(s.startsWithChar('/'))
		s = s.fromFirstOccurrenceOf("/", false, false);

	return s;
}

} // namespace hise
