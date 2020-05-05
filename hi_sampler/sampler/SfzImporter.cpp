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
		"key"
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

var SfzImporter::getOpcodeValue(Opcode o, const String &valueString) const
{
	switch(o)
	{
	case sample:		return var(fileToImport.getParentDirectory().getFullPathName() + "/" + valueString);
	case loop_mode:		return (valueString == "loop_continuous") ? var(1) : var(0);
    case lokey:
    case hikey:
    case pitch_keycenter: return var(getNoteNumberFromNameOrNumber(valueString));
	default:			return var(valueString.getIntValue());
	}
}

const char **SfzImporter::opcodeNames = sfz_opcodeNames;


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
			if(dynamic_cast<Group*>(currentTarget) != nullptr) dynamic_cast<Group*>(currentTarget)->groupName = opcodeTokens[1];
			else throw SfzParsingError(currentParseNumber, "group name opcode outside of group definition");
		}
		else if(opcodeIndex != -1)
		{
			if(currentTarget != nullptr)
			{
				currentTarget->setOpcodeValue(opcodeIndex, getOpcodeValue((Opcode)opcodeIndex, opcodeTokens[1]));
			}
			else
			{
				throw SfzParsingError(currentParseNumber, "No Region for opcode");
			}
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

		if (currentLine.startsWith(Global::getTag()))
		{
			currentTarget = globalSfzObject;
		}
		else if (currentLine.startsWith(Group::getTag()))
		{
			if (i < fileData.size() - 1)
			{
				globalSfzObject->groups.add(new Group());

				currentTarget = globalSfzObject->groups.getLast();

				globalSfzObject->groups.getLast()->groupName = "Group " + String(globalSfzObject->groups.size());

			}
			else
			{
				throw SfzParsingError(i, "Empty Group");
			}
		}
		else if (currentLine.startsWith(Region::getTag()))
		{
			if (globalSfzObject->groups.size() > 0)
			{
				globalSfzObject->groups.getLast()->addRegion(new Region());
				currentTarget = globalSfzObject->groups.getLast()->regions.getLast();
			}
			else
			{
				throw SfzParsingError(i, "No Group for region");
			}
		}

		parseTagLine(currentLine);
	}
}


void SfzImporter::applyGlobalOpcodesToRegion()
{
	NamedValueSet *globalSet = &globalSfzObject->opcodes;

	for (int i = 0; i < globalSfzObject->groups.size(); i++)
	{
		NamedValueSet *groupSet = &globalSfzObject->groups[i]->opcodes;

		ScopedPointer<XmlElement> xml = new XmlElement("a");

		groupSet->copyToXmlAttributes(*xml);

		for (int j = 0; j < globalSfzObject->groups[i]->regions.size(); j++)
		{
			Region *r = globalSfzObject->groups[i]->regions[j];
			applyValueSetOnRegion(*globalSet, r);
			applyValueSetOnRegion(*groupSet, r);

			
		}
	}
}


void SfzImporter::applyValueSetOnRegion(const NamedValueSet &setToApply, Region * r)
{
	

	for (int i = 0; i < setToApply.size(); i++)
	{
		Identifier opcodeId = setToApply.getName(i);

		if (setToApply[opcodeId].isUndefined()) continue;

		if (r->opcodes[opcodeId].isUndefined())
		{
			r->opcodes.set(opcodeId, setToApply[opcodeId]);	
		}
	}
}

SfzImporter::SfzImporter(ModulatorSampler *sampler_, const File &sfzFileToImport) :
sampler(sampler_),
fileToImport(sfzFileToImport),
currentTarget(nullptr),
currentParseNumber(0)
{
	globalSfzObject = new Global();

	currentTarget = globalSfzObject;
}

void SfzImporter::importSfzFile()
{
	parseOpcodes();

	applyGlobalOpcodesToRegion();


	ValueTree v("samplemap");

	v.setProperty("RelativePath", 0, nullptr);
	
	v.setProperty("FileName", fileToImport.getFullPathName(), nullptr);
	
	v.setProperty("SaveMode", 1, nullptr);

	OwnedArray<SfzGroupSelectorComponent> groupSelectors;

	if (globalSfzObject->groups.size() > 1)
	{
		AlertWindow w("Group Import Settings", String(), AlertWindow::AlertIconType::NoIcon);

		ScopedPointer<Viewport> viewport = new Viewport();

		Component *c = new Component();

		viewport->setViewedComponent(c, true);

		int y = 0;

		

		for (int i = 0; i < globalSfzObject->groups.size(); i++)
		{
			SfzGroupSelectorComponent *g = new SfzGroupSelectorComponent();

			g->setData(i, globalSfzObject->groups[i]->groupName, globalSfzObject->groups.size());

			c->addAndMakeVisible(g);

			g->setBounds(0, y, 500, 30);

			groupSelectors.add(g);

			y = g->getBottom();
		}

		c->setSize(500, y);

		viewport->setSize(530, 200);
		viewport->setVisible(true);

		w.setLookAndFeel(&alaf);

		w.setColour(AlertWindow::backgroundColourId, Colour(0xff222222));
		w.setColour(AlertWindow::textColourId, Colours::white);
		w.addButton("OK", 1, KeyPress(KeyPress::returnKey));
		w.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

		w.addCustomComponent(viewport);

		if (w.runModalLoop() == 0) return;

		
	}
	else
	{
		groupSelectors.add(new SfzGroupSelectorComponent());

		groupSelectors.getLast()->setFixedReturnValue();
	}

	jassert(groupSelectors.size() == globalSfzObject->groups.size());
	
    
	int id = 0;

	for (int i = 0; i < globalSfzObject->groups.size(); i++)
	{
		const int rrGroup = groupSelectors[i]->getGroupIndex();

		if(rrGroup == -1) continue;

		for (int j = 0; j < globalSfzObject->groups[i]->regions.size(); j++)
		{
			Region *region = globalSfzObject->groups[i]->regions[j];

			ValueTree sample("sample");

			id++;

			sample.setProperty(SampleIds::ID, id, nullptr);

			sample.setProperty(SampleIds::LoVel, 0, nullptr);
			sample.setProperty(SampleIds::HiVel, 127, nullptr);

			for(int k = 0; k < Opcode::numSupportedOpcodes; k++)
			{
				Identifier opcode = Identifier(getOpcodeName((Opcode)k));

				if (k == Opcode::groupName) continue;

				if ( !region->opcodes[opcode].isUndefined() )
				{
					if (k == Opcode::key)
					{
						sample.setProperty(SampleIds::Root, region->opcodes[opcode], nullptr);
						sample.setProperty(SampleIds::LoKey, region->opcodes[opcode], nullptr);
						sample.setProperty(SampleIds::HiKey, region->opcodes[opcode], nullptr);

						continue;
					}
					
					

					auto p = getSamplerProperty((Opcode)k);

					if (p == SampleIds::FileName)
					{
						auto path = region->opcodes[opcode];
						PoolReference ref(sampler->getMainController(), path, FileHandlerBase::Samples);
						sample.setProperty(p, ref.getReferenceString(), nullptr);
					}
					else
					{
						sample.setProperty(p, region->opcodes[opcode], nullptr);
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

			// Add the group properties
			sample.setProperty(SampleIds::RRGroup, rrGroup, nullptr);

			v.addChild(sample, -1, nullptr);
		}

	}

	int groupAmount = 0;

	for (int i = 0; i < groupSelectors.size(); i++)
	{
		groupAmount = jmax(groupAmount, groupSelectors[i]->getGroupIndex());
	}

	sampler->setRRGroupAmount(groupAmount);

	sampler->getSampleMap()->loadUnsavedValueTree(v);

	//sampler->getSampleMap()->setRelativeSaveMode(true);

	sampler->refreshPreloadSizes();
	sampler->refreshMemoryUsage();
};

} // namespace hise