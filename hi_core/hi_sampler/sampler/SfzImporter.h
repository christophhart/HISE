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

#ifndef SFZIMPORTER_H_INCLUDED
#define SFZIMPORTER_H_INCLUDED

namespace hise { using namespace juce;

#define HISE_REF_PTR_ALIASES(ClassName) using Ptr = ReferenceCountedObjectPtr<ClassName>; \
								using List = ReferenceCountedArray<ClassName>; \
								JUCE_DECLARE_WEAK_REFERENCEABLE(ClassName); \
								using WeakPtr = WeakReference<ClassName>; \
								using WeakList = Array<WeakReference<ClassName>>;


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
		loop_start, ///< the loop start
		loop_end, ///< the loop end
		tune, ///< the fine tune
		pitch_keycenter, ///< the coarse tune
		volume, ///< the volume
		group_volume, ///< the group volume (will be consolidated for every sample with the 'volume' opcode
		pan, ///< the balance
		groupName,
		key,
		default_path,
		lorand,
		hirand,
		seq_length,
		seq_position,
		numSupportedOpcodes
	};

	enum NavigationMode
	{
		ThrowErrorIfNotFound,
		AllowNoParent
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
	ValueTree importSfzFile();

private:

	struct SfzOpcodeTarget: public ReferenceCountedObject
	{
		HISE_REF_PTR_ALIASES(SfzOpcodeTarget);

		SfzOpcodeTarget(SfzOpcodeTarget* parent_) :
			parent(parent_)
		{
			if(parent != nullptr)
				parent->children.add(this);
		};

		void toDbgString(String& s, int& intLevel);

		virtual String getTagVirtual() const = 0;

		bool isRoot() const
		{
			return parent == nullptr;
		}

		template <typename T> T* as(bool throwOnMismatch=true) 
		{ 
			if (auto typed = dynamic_cast<T*>(this))
				return typed;
			else
				throw SfzParsingError(0, "type mismatch");
		}

		template <typename T> WeakPtr findParentTargetOfType()
		{
			if (auto s = dynamic_cast<T*>(this))
				return this;

			if (parent != nullptr)
				return parent->findParentTargetOfType<T>();

			return nullptr;
		}

		template <typename T> WeakPtr findFirstChildOfType()
		{
			if (auto s = dynamic_cast<T*>(this))
				return this;

			for (auto c : children)
			{
				if (auto cc = c->findFirstChildOfType<T>())
					return cc;
			}

			return nullptr;
		}

		virtual ~SfzOpcodeTarget() {};

		void setOpcodeValue(int opcode, var value) { opcodes.set(Identifier(SfzImporter::getOpcodeName((Opcode)opcode)), value); };

		var operator[](Opcode c) const
		{
			Identifier id(getOpcodeName(c));
			return opcodes[id];
		}
		
		NamedValueSet opcodes;
		List children;
		WeakPtr parent;
	};


	struct Region: public SfzOpcodeTarget
	{
		HISE_REF_PTR_ALIASES(Region);

		Region(SfzOpcodeTarget* p):
			SfzOpcodeTarget(p)
		{
#if 0
			for (int i = 0; i < numSupportedOpcodes; i++)
				setOpcodeValue(i, {});
#endif
		}

		int getRRGroup() const;

		String getRelativeFilePath() const;

		static String getTag() { return "<region>"; };
		String getTagVirtual() const override { return getTag(); }
	};

	

	struct Group: public SfzOpcodeTarget
	{	
		HISE_REF_PTR_ALIASES(Group);

		Group(SfzOpcodeTarget* p) :
			SfzOpcodeTarget(p)
		{}

		static String getTag() { return "<group>"; };
		String getTagVirtual() const override { return getTag(); }

		String groupName;
	};


	struct Global: public SfzOpcodeTarget
	{
		HISE_REF_PTR_ALIASES(Global);

		Global(SfzOpcodeTarget* p) :
			SfzOpcodeTarget(p)
		{}

		static String getTag() { return "<global>"; };
		String getTagVirtual() const override { return getTag(); }
	};

	struct Control : public SfzOpcodeTarget
	{
		HISE_REF_PTR_ALIASES(Control);

		Control(SfzOpcodeTarget* p) :
			SfzOpcodeTarget(p)
		{}

		static String getTag() { return "<control>"; }
		String getTagVirtual() const override { return getTag(); }

		String defaultPath;
	};


	void parseTagLine(const String &restOfLine);

	StringArray getOpcodeTokens(const String &line) const;

	void parseOpcode(const String &opcode);

	void parseOpcodes();

	static String getOpcodeName(Opcode opcode) { return String(opcodeNames[opcode]); };

	var combineOpcodeValue(Opcode o, var prevValue, var thisValue);

	var getOpcodeValue(Opcode o, const String &valueString) const;
	
	static int getOpcode(const StringRef &opcodeName)
	{
		for(int i = 0; i < numSupportedOpcodes; i++)
		{
			if(StringRef(opcodeNames[i]) == opcodeName) return i;
		}

		return -1;
	}

#if 0
	File getDefaultPath()
	{
		auto pDir = fileToImport.getParentDirectory();

		if (auto controlToUse = root->findFirstChildOfType<Control>())
		{
			auto defaultPath = controlToUse->opcodes["default_path"].toString().replaceCharacter('\\', '/');

			if (!defaultPath.isEmpty())
			{
				if (File::isAbsolutePath(defaultPath))
					return File(defaultPath);
				else
					return pDir.getChildFile(defaultPath);
			}
		}

		return pDir;
	}
#endif

	static Identifier getSamplerProperty(Opcode opcode);
	
	void applyGlobalOpcodesToRegion();

	void applyValueSetOnRegion(const NamedValueSet &setToApply, Region * r);

	static const char **opcodeNames;

	File fileToImport;

	ModulatorSampler *sampler;

	int currentParseNumber;

	void setIfRoot(SfzOpcodeTarget::Ptr p)
	{
		if (p->isRoot())
			root = p;
	}

	template <typename T> void navigateToParent(NavigationMode navigationMode, const String& errorMessage = {})
	{
		if (currentTarget != nullptr)
			currentTarget = currentTarget->findParentTargetOfType<T>();

		if (currentTarget == nullptr && navigationMode == ThrowErrorIfNotFound)
			throw SfzParsingError(currentParseNumber, "no valid parent for group");
	}

	void debugRoot();

	SfzOpcodeTarget::Ptr currentTarget;
	SfzOpcodeTarget::Ptr root;

	//ScopedPointer<Control> globalControlObject;
	//Global::Ptr globalSfzObject;

	AlertWindowLookAndFeel alaf;

};



} // namespace hise
#endif  // SFZIMPORTER_H_INCLUDED
