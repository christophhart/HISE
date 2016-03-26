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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef SFZIMPORTER_H_INCLUDED
#define SFZIMPORTER_H_INCLUDED



/** Handles the importing of SFZ sample files.
*	@ingroup sampler
*
*/
class SfzImporter
{
public:

	/** All supported opcodes. This is far from being complete, but it tries to fetch everything that a ModulatorSamplerSound::Property can handle. */
	enum Opcode
	{
		sample = 0, ///< the sample file name (absolute)
		lokey, ///< the lowest key
		hikey, ///< the highest key
		lovel, ///< the lowest velocity
		hivel, ///< the highest velocity
		offset, ///< the sample start
		end, ///< the sample end
		loop_mode, ///< the loop mode (it will only support yes / no)
		loopstart, ///< the loop start
		loopend, ///< the loop end
		tune, ///< the fine tune
		pitch_keycenter, ///< the coarse tune
		volume, ///< the volume
		group_volume, ///< the group volume (will be consolidated for every sample with the 'volume' opcode
		pan, ///< the balance
		groupName,
		key,
		numSupportedOpcodes
	};

	struct SfzParsingError
	{
		SfzParsingError(int lineNumber_, String error):
			lineNumber(lineNumber_),
			message(error)
		{};

		SfzParsingError& operator=(const SfzParsingError &)
		{
			return *this;
		}

		String getErrorMessage() { return "Line " + String(lineNumber) + ": " + message; }

		const int lineNumber;
		const String message;

	};

	SfzImporter(ModulatorSampler *sampler, const File &sfzFileToImport);

	/** imports a SFZ file into the given ModulatorSampler. 
	*
	*	It will parse the file with all supported opcodes and replace the SampleMap of the ModulatorSampler with the data from the SFZ file.
	*
	*	If multiple groups are found, there will be a dialog box to consolidate groups or ignore them as round robin groups of the ModulatorSampler.
	*/
	void importSfzFile();

private:

	struct SfzOpcodeTarget
	{
		virtual ~SfzOpcodeTarget() {};

		void setOpcodeValue(int opcode, var value) { opcodes.set(Identifier(SfzImporter::getOpcodeName((Opcode)opcode)), value); };

		NamedValueSet opcodes;
	};


	struct Region: public SfzOpcodeTarget
	{
		Region()
		{
			for (int i = 0; i < numSupportedOpcodes; i++)
			{
				setOpcodeValue(i, var::undefined());
			}
		}

		static String getTag() { return "<region>"; };
	};


	struct Group: public SfzOpcodeTarget
	{	
		static String getTag() { return "<group>"; };

		void addRegion(Region *r) {	regions.add(r);	}

		String groupName;

		OwnedArray<Region> regions;
	};


	struct Global: public SfzOpcodeTarget
	{
		static String getTag() { return "<global>"; };

		OwnedArray<Group> groups;
	};


	void parseTagLine(const String &restOfLine);

	StringArray getOpcodeTokens(const String &line) const;

	void parseOpcode(const String &opcode);

	void parseOpcodes();

	static String getOpcodeName(Opcode opcode) { return String(opcodeNames[opcode]); };

	var getOpcodeValue(Opcode o, const String &valueString) const;
	
	static int getOpcode(const StringRef &opcodeName)
	{
		for(int i = 0; i < numSupportedOpcodes; i++)
		{
			if(StringRef(opcodeNames[i]) == opcodeName) return i;
		}

		return -1;
	}

	static ModulatorSamplerSound::Property getSamplerProperty(Opcode opcode);
	
	void applyGlobalOpcodesToRegion();

	void applyValueSetOnRegion(const NamedValueSet &setToApply, Region * r);

	static const char **opcodeNames;

	const File &fileToImport;

	ModulatorSampler *sampler;

	int currentParseNumber;

	SfzOpcodeTarget *currentTarget;

	ScopedPointer<Global> globalSfzObject;

	AlertWindowLookAndFeel alaf;

};




#endif  // SFZIMPORTER_H_INCLUDED
