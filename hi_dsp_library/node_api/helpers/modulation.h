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

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

namespace modulation
{

// These are the types of modulators available in HISE
enum class SourceType
{
	Disabled,
	VoiceStart,
	TimeVariant,
	Envelope
};

enum class ClearState : int8
{
	Playing,
	PendingReset1, // Leave one buffer through to avoid glitches at fast release times
	PendingReset2, // Leave another buffer through to avoid glitches at fast release times
	Reset
};

// These are the available modulation modes that can be used to define how a given parameter
// should be modulated
enum class ParameterMode: int8
{
	Disabled = 0, ///< indicates no modulation of this parameter
	ScaleAdd,     ///< this parameter can be modulated with scale & unipolar / bipolar add
	ScaleOnly,    ///< this parameter will be modulated with scaling only (like the gain modulation in HISE)
	AddOnly,      ///< this parameter will be modulated with unipolar / bipolar offset only (Offset mode in HISE)
	Pan,          ///< this parameter will be modulated with unipolar / bipolar scale around the zero position (like HISE's StereoFX)
	Pitch,        ///< a special mode that automatically converts the mod value to a pitch factor from 0.5 ... 2.0 (like HISE's Pitch mod).
	numModulationModes
};

enum class ConnectionMode: int8
{
	Disabled = 0,
	Parameter,
	ModulationNode,
	numConnectionModes
};

enum class TargetMode // this defines how the modulation signal is applied inside scriptnode
{
	Gain,		// Applies the infamous HISE intensity scale modulation
	Unipolar,	// Adds the modulation to the base value
	Bipolar,    // Adds (or subtracts) the modulation to the base value bipolarly
	Pitch,      // Does nothing and allows values > 1.0
	Raw,        // Does nothing (as the modulation signal is already calculated properly).
	Aux      // Only applies the intensity to the signal
};

// The maximum number of parameter indexes that can be used with the modulation system
static constexpr int NumMaxModulationParameterIndex = 256;

// The maximum number of modulators that can be fetched as source signal
static constexpr int NumMaxModulationSources = HISE_NUM_MODULATORS_PER_CHAIN;

/** Subclass all classes that provide modulation signals from this. */
struct Host
{
	virtual ~Host() {};
};

/** A POD class that holds the information to the modulation signal for a given voice. */
struct EventData
{
	// an alias that is called on a host to return the event data for the given HISE event. */
	using QueryFunction = EventData(Host*, const HiseEvent&, bool);

	/** This will query the addresses provided in this object and return a VariableStorage object
	 *
	 * - if the modulation signal is invalid: Types::ID::Void
	 * - if the modulation signal is constant: Types::ID::Float with the constant value
	 * - if the modulation signal is dynamic: Types::ID::Block with the modulation signal reference
	 */
	VariableStorage getReadPointer() const;

	SourceType    type          = SourceType::Disabled; // defines the source type of modulation
	const float*  signal        = nullptr;              // a pointer to the start of the modulation signal
	float         constantValue = 0.0f;			        // the constant modulation value if no dynamic signal is available
	const int*    thisBlockSize = nullptr;		        // the (current) size of the modulation signal array
	ClearState*	  resetFlag     = nullptr;				// the voice state for envelope modulation signals

};


/** A small helper class that contains the downsamples modulation signal coming from HISE.
 *  This object is passed around across DLL boundaries using the runtime_target system. */
struct SignalSource
{
	// A alias for a function pointer that queries the 

	operator bool() const noexcept { return sampleRate_cr > 0.0; }

	EventData getEventData(int slotIndex, const HiseEvent& e, bool wantsPolyphonicSignal) const
	{
		if(isPositiveAndBelow(slotIndex, NumMaxModulationSources))
		{
			auto& s = modValueFunctions[slotIndex];

			if(s.first != nullptr)
				return s.second(s.first, e, wantsPolyphonicSignal);
		}

		return {};
	}

	std::array<std::pair<Host*, EventData::QueryFunction*>, NumMaxModulationSources> modValueFunctions;

	// the downsampled sample rate
	double sampleRate_cr = -1.0;

	// the downsampled (maximum) block size
	int numSamples_cr = 0;
};

struct ConnectionInfo
{
	static constexpr char BeginList = 59;
	static constexpr char BeginConnection = 60;
	static constexpr char EndList = 61;
	static constexpr char BeginMetadata = 62;

	using List = std::vector<ConnectionInfo>;

	int16 connectedParameterIndex;
	ParameterMode modulationMode;
	HiseModulationColours::ColourId modColour = HiseModulationColours::ColourId::ExtraMod;
	ConnectionMode connectionMode = ConnectionMode::ModulationNode;

	static ConnectionInfo::List readFrom(InputStream& is)
	{
		List l;

		if(is.readByte() == BeginList)
		{
			while (!is.isExhausted())
			{
				auto check = is.readByte();

				if (check == EndList)
					break;

				if (check == BeginConnection)
				{
					ConnectionInfo ni;
					ni.connectedParameterIndex = is.readCompressedInt();
					ni.modulationMode = static_cast<ParameterMode>(is.readByte());
					ni.modColour =		static_cast<HiseModulationColours::ColourId>(is.readByte());
					ni.connectionMode = static_cast<ConnectionMode>(is.readByte());

					l.push_back(ni);
				}
				else
				{
					// corrupted, recompile
					jassertfalse;
				}
			}
		}
		else
		{
			// corrupted, recompile
			jassertfalse;
		}

		return l;
	}

	static void writeTo(const List& list, OutputStream& os)
	{
		os.writeByte(BeginList);

		for(const auto& info: list)
		{
			os.writeByte(BeginConnection);
			os.writeCompressedInt(info.connectedParameterIndex);
			os.writeByte(static_cast<char>(info.modulationMode));
			os.writeByte(static_cast<char>(info.modColour));
			os.writeByte(static_cast<char>(info.connectionMode));
		}

		os.writeByte(EndList);
	}
};

/** This pod structure will hold the information and properties on how the parameters of the Opaque node
 *  are exposed to the modulation system in HISE.
 */
struct ParameterProperties
{
	static StringArray getModulationModeNames();
	static ParameterMode getModeFromVar(const var& value);

	ParameterProperties();

	template <typename NodeClass> void fromNode()
	{
        typename NodeClass::MetadataClass n;

		MemoryOutputStream tm;

		for (auto& s : n.encodedModInfo)
			tm.writeShort(static_cast<uint16>(s));

		auto data = tm.getMemoryBlock();
		MemoryInputStream mis(data, false);
		readFromStream(mis);
	}

	// Clears the state of this object. */
	void reset();

	using ConnectionList = ConnectionInfo::List;

	void fromConnectionList(const ConnectionList& c);

	void fromValueTree(const ValueTree& v);
	bool isUsed(int8 modulationIndex) const;

	/** Checks whether the slot is used and the parameter is connected to something
	 *
	 *  (Note that a parameter can still be used but not be connected if it is controlling
	 *	a modulation chain with a `extra_mod` connection.
	 */
	bool isConnected(int8 modulationIndex) const;

	/** Returns the block size that should be used for chunking.
	 *
	 *  - if nothing is connected, the full block size will be used
	 *	- if the modulationBlockSize is zero, the full block size will be used.
	 */
	int getBlockSize(int fullBlocksize) const;

	void writeToStream(OutputStream& output) const;

	/** Returns the parameter index of the modulation slot or -1 if not assigned. */
	int16 getParameterIndex(int8 modulationIndex) const;

	/** Returns the modulation chain index of the parameter or -1 if not assigned. */
	int8 getModulationChainIndex(int16 parameterIndex) const;

	/** Returns the number of modulation slots used by this setup. You can pass in a limit that will
		cap of the number of slots, which is usually the amount of slots available through the HISE
		preprocessors

		HISE_NUM_HARDCODED_FX_SLOTS=2
		...
	*/
	int getNumUsed(int maxNumber) const noexcept
	{
		return jmin<int>(maxNumber, numUsedModulationSlots);
	}

	/** This checks whether the modulation setup has any connections to parameters.
	
		Note that if you use modulation slots with mod targets that do not control the parameters,
		this will still return false, so it's not guaranteed that anyConnected == (numModulationSlotsUsed != 0)
	*/
	bool isAnyConnected() const noexcept;

	ParameterMode getParameterMode(int16 parameterIndex) const noexcept;

	void setColour(int16 parameterIndex, HiseModulationColours::ColourId newColour);

	void setModulationBlockSize(int newBlockSize);

	Colour getModulationColour(int16 parameterIndex) const;

	HiseModulationColours::ColourId getModulationColourRaw(int16 parameterIndex) const;

private:

	friend class ParameterPropertiesTest;

	/** Sets the connected flag for the given parameterIndex. */
	void setConnected(int16 parameterIndex, bool isConnected);

	void setModulationMode(int16 parameterIndex, ParameterMode m);

	void readFromStream(InputStream& input);

	struct ParameterInfo
	{
		ParameterMode mode = ParameterMode::Disabled;
		int8 modulationSlotIndex = -1;
	};

	struct ModInfo
	{
		ConnectionMode connectionMode = ConnectionMode::Disabled;
		int16 parameterIndex = -1;
		HiseModulationColours::ColourId colour = HiseModulationColours::ColourId::ExtraMod;
	};

	// this contains the modulation mode and slot index of every parameter indexes (0 ... 1024)
	std::array<ParameterInfo, NumMaxModulationParameterIndex> parameterInfo;

	// this contains the parameter index, connection mode and colour of every modulation slot
	std::array<ModInfo, NumMaxModulationSources> modInfo;

	int numUsedModulationSlots = 0;
	bool anyConnected = false;
	int modulationBlocksize = 0;
};

// This namespace contains classes that can be used to tailor the modulation target nodes
// to a fix behaviour to improve the performance
namespace config
{

static constexpr int PitchModulation = 90001;
static constexpr int GainModulation = 90002;
static constexpr int CustomOffset = 5000;

using ExtraIndexer = runtime_target::indexers::fix_hash<CustomOffset>;
using PitchIndexer = runtime_target::indexers::fix_hash<PitchModulation>;
using GainIndexer = runtime_target::indexers::fix_hash<GainModulation>;

/** A index type that allows dynamic changing of the modulation properties. */
template <bool EnableBuffer> struct dynamic_internal
{
	~dynamic_internal() = default;

	static constexpr bool shouldEnableDisplayBuffer() { return EnableBuffer; }
	static constexpr bool isNormalisedModulation() { return true; }
	bool shouldProcessSignal() const { return processSignal; }
	TargetMode getMode() const { return mode; }
	static constexpr bool useMidPositionAsZero() { return false; }
	static constexpr void setUseMidPositionAsZero(bool) {}

	void setProcessSignal(bool shouldProcess) { processSignal = shouldProcess; }
	void setMode(TargetMode m) { mode = m; }

	SN_EMPTY_INITIALISE;
	SN_EMPTY_PREPARE;

	bool processSignal = false;
	TargetMode mode = TargetMode::Gain;
};

using dynamic = dynamic_internal<false>;
using dynamic_with_display = dynamic_internal<true>;

/** A special index type for the extra modulators. */
template <bool EnableBuffer> struct extra_config_internal
{
	virtual ~extra_config_internal() = default;

	static constexpr bool shouldEnableDisplayBuffer() { return EnableBuffer; }
	static constexpr bool isNormalisedModulation() { return true; }
	static constexpr TargetMode getMode() { return TargetMode::Raw; };
	static constexpr bool useMidPositionAsZero() { return false; }
	static constexpr void setUseMidPositionAsZero(bool) {}

	// overriden by a scriptnode config class that validates MIDI processing
	virtual SN_EMPTY_PREPARE; 
	virtual SN_EMPTY_INITIALISE;

	void setProcessSignal(bool shouldProcess) { processSignal = shouldProcess; };
	bool shouldProcessSignal() const { return processSignal; }

	bool processSignal = false;
};

using extra_config = extra_config_internal<false>;
using extra_config_with_display = extra_config_internal<true>;

/** A special index type for the pitch modulator. */
template <bool EnableBuffer> struct pitch_config_internal
{
	static constexpr bool shouldEnableDisplayBuffer() { return EnableBuffer; }
	static constexpr bool isNormalisedModulation() { return false; }
	static constexpr TargetMode getMode() { return TargetMode::Pitch; };
	void setMode(TargetMode ) {};
	static constexpr bool useMidPositionAsZero() { return true; }
	static constexpr void setUseMidPositionAsZero(bool) {}

	SN_EMPTY_PREPARE;
	SN_EMPTY_INITIALISE;

	void setProcessSignal(bool shouldProcess) { processSignal = shouldProcess; };
	bool shouldProcessSignal() const { return processSignal; }

	bool processSignal = false;
};

using pitch_config = pitch_config_internal<false>;
using pitch_config_with_display = pitch_config_internal<true>;

/** This is used by the C++ code creator if the modulation properties are static. */
template <bool UseProcess, TargetMode Mode, bool EnableDisplayBuffer=false> struct constant
{
	static constexpr bool shouldEnableDisplayBuffer() { return EnableDisplayBuffer; }
	static constexpr bool isNormalisedModulation() { return true; }
	static constexpr bool shouldProcessSignal() { return UseProcess; }
	static constexpr TargetMode getMode() { return Mode; };
	static constexpr bool useMidPositionAsZero() { return false; }
	static constexpr void setUseMidPositionAsZero(bool) {}

	SN_EMPTY_PREPARE;
	SN_EMPTY_INITIALISE;

	void setProcessSignal(bool) {};
	void setMode(TargetMode m) {};
	
};

} // namespace config
} // namespace modulation
} // namespace scriptnode
