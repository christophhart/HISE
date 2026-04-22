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

#pragma once

namespace hise { using namespace juce;

class WavetableSynth;

struct WavetableMonolithHeader
{
	static void writeProjectInfo(OutputStream& output, const String& projectName, const String& key);

	static bool checkProjectName(InputStream& input, const String& projectName, const String& key);

	void write(OutputStream& output)
	{
		output.writeByte((uint8)name.length() + 1);
		output.writeString(name);
		output.writeInt64(offset);
		output.writeInt64(length);
	}

	static Array<WavetableMonolithHeader> readHeader(InputStream& input, const String& projectName, const String& encryptionKey);

	String name;
	int64 offset;
	int64 length;
};

class WavetableSound: public ModulatorSynthSound
{
public:

	/** Creates a new wavetable sound.
	*
	*	You have to supply a ValueTree with the following properties:
	*
	*	- 'data' the binary data
	*	- 'amount' the number of wavetables
	*	- 'noteNumber' the noteNumber
	*	- 'sampleRate' the sample rate
	*
	*/
	WavetableSound(const ValueTree &wavetableData, Processor* parent);;

	bool appliesToNote (int midiNoteNumber) override   { return midiNotes[midiNoteNumber]; }
    bool appliesToChannel (int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity (int /*midiChannel*/) override  { return true; }

	/** Returns a read pointer to the wavetable with the given index.
	*
	*	Make sure you don't get off bounds, it will return a nullptr if the index is bigger than the wavetable amount.
	*/
	const float *getWaveTableData(int channelIndex, int wavetableIndex) const;

	float getUnnormalizedMaximum() const
	{
		return unnormalizedMaximum;
	}

	int getWavetableAmount() const
	{
		return wavetableAmount;
	}

	int getTableSize() const
	{
		return wavetableSize;
	};

	float getMaxLevel() const
	{
		return maximum;
	}

	/** Call this in the prepareToPlay method to calculate the pitch error. It also resamples if the playback frequency is different. */
	void calculatePitchRatio(double playBackSampleRate);

	double getPitchRatio(double midiNoteNumber) const
	{
        double p = pitchRatio;
        
        p *= hmath::pow(2.0, (midiNoteNumber - noteNumber) / 12.0);
        
		return p;
	}
    
    Range<double> getFrequencyRange() const
    {
        return frequencyRange;
    }

	void normalizeTables();

	float getUnnormalizedGainValue(int tableIndex)
	{
		jassert(isPositiveAndBelow(tableIndex, wavetableAmount));
		
		return unnormalizedGainValues[tableIndex];
	}

	String getMarkdownDescription() const;

	int getRootNote() const { return noteNumber; };

	bool isStereo() const { return stereo; };

	float isReversed() const { return reversed; };

	struct RenderData
	{
		using TableIndexFunction = std::function<float(int)>;
		RenderData(AudioSampleBuffer& b_, int startSample_, int numSamples_, double uptimeDelta_, const float* voicePitchValues_, bool hqMode_) :
			b(b_),
			startSample(startSample_),
			numSamples(numSamples_),
			voicePitchValues(voicePitchValues_),
			uptimeDelta(uptimeDelta_),
			hqMode(hqMode_)
		{};

		AudioSampleBuffer& b;
		int startSample;
		int numSamples;
		const float* voicePitchValues;
		const double uptimeDelta;
		const bool hqMode;
		bool dynamicPhase = false;

		void render(WavetableSound* currentSound, double& voiceUptime, const TableIndexFunction& tf);

		float calculateSample(const float* lowerTable, const float* upperTable, const span<int, 4>& i, float alpha, float tableAlpha) const;
	};

private:

	float reversed = 0.0f;
	bool stereo = false;

	size_t memoryUsage = 0;
	size_t storageSize = 0;

	float maximum;
	float unnormalizedMaximum;
	HeapBlock<float> unnormalizedGainValues;

    Range<double> frequencyRange;
    
	BigInteger midiNotes;
	int noteNumber;

	AudioSampleBuffer wavetables;
	AudioSampleBuffer emptyBuffer;

	double sampleRate;
	double pitchRatio;
    double playbackSampleRate;

	int wavetableSize;
	int wavetableAmount;
	bool dynamicPhase = false;
};

class WavetableSynth;

class WavetableSynthVoice: public ModulatorSynthVoice
{
public:

	WavetableSynthVoice(ModulatorSynth *ownerSynth);

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	int getSmoothSize() const;

	void startNote (int midiNoteNumber, float /*velocity*/, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override;;

	void resetVoice() override;

	void calculateBlock(int startSample, int numSamples) override;;

	void setRefreshMipmap(bool refreshMipmap_)
	{
		refreshMipmap = refreshMipmap_;
	}

	void setHqMode(bool useHqMode)
	{
		hqMode = useHqMode;
	};

    bool updateSoundFromPitchFactor(double pitchFactor, WavetableSound* soundToUse);
    
private:

	WavetableSynth *wavetableSynth;

	int octaveTransposeFactor;

	WavetableSound *currentSound = nullptr;

	int tableSize;
	
	float currentGainValue;
    int noteNumberAtStart = 0;
    double startFrequency = 0.0;
	
	bool hqMode = true;
	bool refreshMipmap = false;

	float const *currentTable;
};


/** A two-dimensional wavetable synthesiser.
	@ingroup synthTypes
*/
class WavetableSynth: public ModulatorSynth,
					  public hise::AudioSampleProcessor,
					  public MultiChannelAudioBuffer::Listener,
					  public WaveformComponent::Broadcaster
{
public:

	SET_PROCESSOR_NAME("WavetableSynth", "Wavetable Synthesiser", "");

	static ProcessorMetadata createMetadata();

	/** An object that will perform the post processing after a wavetable is loaded. */
	struct PostFXProcessor
	{
		enum class Type
		{
			Identity = 0,
			Custom,
			Sin,
			Warp,
			FM1,
			FM2,
			FM3,
			FM4,
			Sync,
			Root,
			Clip,
			Tanh,
			Bitcrush,
			SampleAndHold,
			Fold,
			Normalise,
			Phase,
			numTypes
		};

		static StringArray getTypeNames()
		{
			return {
				"Identity",
				"Custom",
				"Sin",
				"Warp",
				"FM1",
				"FM2",
				"FM3",
				"FM4",
				"Sync",
				"Root",
				"Clip",
				"Tanh",
				"Bitcrush",
				"SampleAndHold",
				"Fold",
				"Normalise",
				"Phase"
			};
		}

		using Function = std::function<void(VariantBuffer::Ptr, float)>;

		struct Functions
		{
			static void Identity(VariantBuffer::Ptr p, float parameter)
			{
				
			}

			static void Root(VariantBuffer::Ptr p, float amount)
			{
				auto uptime = 0.0;
				auto uptimeDelta = (2.0 * double_Pi) / (double)(p->size);
				
				for(auto& s: *p)
				{
					auto v = amount * std::sin(uptime);
					s += v;
					uptime += uptimeDelta;
				}
			}

			static void Clip(VariantBuffer::Ptr p, float amount)
			{
				amount = 1.0 - jmin(0.99f, hmath::abs(amount));
				
				for(auto& s: *p)
					s = hmath::range(s, -amount, amount);
			}

			static void Tanh(VariantBuffer::Ptr p, float amount)
			{
				for(auto& s: *p)
					s = hmath::tanh(s * amount * 10.0) * amount + s * (1.0 - amount);
			}

			static void Bitcrush(VariantBuffer::Ptr p, float amount)
			{
				for(auto& s: *p)
					s = s - hmath::fmod(s, (amount + 0.01f) / 4.0f);
			}

			static void Fold(VariantBuffer::Ptr p, float amount)
			{
				amount = jlimit(0.0f, 1.0f, 1.0f - amount);
				for(auto& s: *p)
				{
					if (s > amount)
						s = 2.0f * amount - s;
						
					else if (s < -amount)
						s = -2.0 * amount - s;
				}
			}

			static void Warp(VariantBuffer::Ptr p, float amount)
			{
				auto uptime = 0.0;
			    auto uptimeDelta = 1.0;

				auto temp = (float*)alloca(p->size * sizeof(float));
				auto data = p->begin();
				auto N = p->size;

			    FloatVectorOperations::copy(temp, data, N);

			    for(int i = 0; i < N; i++)
			    {
					auto L = (double)(N-1);
					auto alpha = hmath::fmod(uptime, 1.0);
					auto lo = (int)(uptime);
					auto hi = jmin(lo + 1, (N-1));
					
					data[i] = alpha * temp[lo] + (1.0 - alpha) * temp[hi];
					auto normX = (double)i / L;
					uptimeDelta = 1.0 + (amount - 0.5) * (normX * 4.0 - 2.0);
				    uptime = hmath::range(uptime + uptimeDelta, 0.0, L);
			    }
			}

			static void Phase(VariantBuffer::Ptr p, float amount)
			{
				auto offset = jlimit(0, p->size-1, roundToInt(amount * p->size));
				std::rotate(p->begin(), p->begin() + offset, p->end());
			}

			static void Sync(VariantBuffer::Ptr p, float amount)
			{
				auto uptime = 0.0;
			    auto uptimeDelta = jmax(0.01, 1.0 - amount);

				auto temp = (float*)alloca(p->size * sizeof(float));
				auto data = p->begin();
				auto N = p->size;

			    FloatVectorOperations::copy(temp, data, N);

			    for(int i = 0; i < N; i++)
			    {
					auto L = (double)(N-1);
					auto alpha = hmath::fmod(uptime, 1.0);
					auto lo = (int)(uptime);
					auto hi = jmin(lo + 1, (N-1));
					
					data[i] = alpha * temp[lo] + (1.0 - alpha) * temp[hi];
				    uptime = hmath::range(uptime + uptimeDelta, 0.0, L);
			    }
			}

			static void SampleAndHold(VariantBuffer::Ptr p, float amount)
			{
				auto N = p->size;
				auto holdAmount = jlimit(1, N, roundToInt(amount * N/ 8));

				auto ptr = p->begin();

				auto lastValue = ptr[0];

				for(int i = 0; i < N; i++)
				{
					auto newValue = i% holdAmount == 0;

					if(newValue)
						lastValue = ptr[i];

					ptr[i] = lastValue;
				}
			}

			static void Normalise(VariantBuffer::Ptr p, float amount)
			{
				if(amount != 0.0)
				{
					auto peak = p->buffer.getMagnitude(0, p->size);

					auto fullNormalise = 1.0f / peak;
					auto gain = Interpolator::interpolateLinear(1.0f, fullNormalise, jlimit(0.0f, 1.0f, amount));
					FloatVectorOperations::multiply(p->begin(), gain, p->size);
				}
			}

			template<int Factor> static void FM(VariantBuffer::Ptr p, float amount)
			{
				auto uptime = 0.0;
			    auto uptimeDelta = 1.0;

				const auto sinFactor = (double)Factor * (double_Pi * 2.0) / (double)p->size ;

				auto temp = (float*)alloca(p->size * sizeof(float));
				auto data = p->begin();
				auto N = p->size;

			    FloatVectorOperations::copy(temp, data, N);

			    for(int i = 0; i < N; i++)
			    {
					auto alpha = hmath::fmod(uptime, 1.0);
					auto lo = (int)(uptime);
					auto hi = (lo + 1) % N;
					
					data[i] = alpha * temp[lo] + (1.0 - alpha) * temp[hi];

					uptimeDelta = 1.0 + amount * Math.sin((double)i * sinFactor);
					uptime += uptimeDelta;

					if(uptime < 0.0)
						uptime += N;

				    uptime = hmath::fmod(uptime, (double)N);
			    }
			}

			static void FM1(VariantBuffer::Ptr p, float amount) { FM<1>(p, amount); }
			static void FM2(VariantBuffer::Ptr p, float amount) { FM<2>(p, amount); }
			static void FM3(VariantBuffer::Ptr p, float amount) { FM<3>(p, amount); }
			static void FM4(VariantBuffer::Ptr p, float amount) { FM<4>(p, amount); }
			
			static void Sin(VariantBuffer::Ptr p, float amount)
			{
				for(float& s: *p)
					s = hmath::sin(s * (1.0 + amount * 2.0f * float_Pi));
			}
		};

		static Function getFunction(Type t)
		{
			switch(t)
			{
			case Type::Identity: return Functions::Identity;
			case Type::Sin: return Functions::Sin;
			case Type::Warp: return Functions::Warp;
			case Type::FM1: return Functions::FM1;
			case Type::FM2: return Functions::FM2;
			case Type::FM3: return Functions::FM3;
			case Type::FM4: return Functions::FM4;
			case Type::Root: return Functions::Root;
			case Type::Sync: return Functions::Sync;
			case Type::Bitcrush: return Functions::Bitcrush;
			case Type::SampleAndHold: return Functions::SampleAndHold;
			case Type::Tanh: return Functions::Tanh;
			case Type::Clip: return Functions::Clip;
			case Type::Fold: return Functions::Fold;
			case Type::Normalise: return Functions::Normalise;
			case Type::Phase: return Functions::Phase;
			}

			return {};
		}

		var toVar() const
		{
			auto o = new DynamicObject();

			var obj(o);

			scriptnode::RangeHelpers::storeDoubleRange(obj, parameterRange, RangeHelpers::IdSet::ScriptComponents);

			o->setProperty("Type", getTypeNames()[(int)type]);
			o->setProperty("TableProcessor", tableParent);
			o->setProperty("TableIndex", tableIndex);

			return obj;
		}

		void fromVar(MainController* mc, const var& obj)
		{
			parameterRange = scriptnode::RangeHelpers::getDoubleRange(obj, RangeHelpers::IdSet::ScriptComponents);

			auto typeName = obj.getProperty("Type", "").toString();

			auto idx = getTypeNames().indexOf(typeName);

			if(idx != -1)
				type = (Type)idx;

			if(type != Type::Identity && type != Type::Custom)
				cf = getFunction(type);

			tableParent = obj.getProperty("TableProcessor", tableParent);
			tableIndex = obj.getProperty("TableIndex", tableIndex);

			if(tableIndex != -1 && tableParent.isNotEmpty())
			{
				auto p = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), tableParent);

				if(auto ep = dynamic_cast<ProcessorWithExternalData*>(p))
				{
					auto numTables = ep->getNumDataObjects(ExternalData::DataType::Table);

					if(isPositiveAndBelow(tableIndex, numTables))
					{
						connectedTable = ep->getTable(tableIndex);
					}
				}
			}
		}

		Type type = Type::Identity;
		WeakReference<Table> connectedTable;
		String tableParent;
		int tableIndex = -1;
		InvertableParameterRange parameterRange;
		Function cf;

		void apply(VariantBuffer::Ptr cycle, double normalisedIndex, NotificationType n=dontSendNotification)
		{
			if(cf)
			{
				if(auto ws = dynamic_cast<SampleLookupTable*>(connectedTable.get()))
					normalisedIndex = jlimit(0.0, 1.0, (double)ws->getInterpolatedValue(normalisedIndex, n));

				auto v = parameterRange.convertFrom0to1(normalisedIndex, true);
				cf(cycle, (float)v);
			}
		}
	};

	enum EditorStates
	{
		TableIndexChainShow = ModulatorSynth::numEditorStates,
		TableIndexBipolarShow
	};

	enum SpecialParameters
	{
		HqMode = ModulatorSynth::numModulatorSynthParameters,
		LoadedBankIndex,
		TableIndexValue,
		RefreshMipmap,
		numSpecialParameters
	};

	enum ChainIndex
	{
		Gain = 0,
		Pitch = 1,
		TableIndex = 2,
		TableIndexBipolar
	};

	enum class WavetableInternalChains
	{
		TableIndexModulation = ModulatorSynth::numInternalChains,
		TableIndexBipolarChain,
		numInternalChains
	};

	WavetableSynth(MainController *mc, const String &id, int numVoices);;

	void loadWaveTable(const ValueTree& v)
	{
		clearSounds();

		currentlyLoadedWavetableTree = v;

		jassert(v.isValid());
        
		for(int i = 0; i < v.getNumChildren(); i++)
		{
			auto s = new WavetableSound(v.getChild(i), this);

			s->calculatePitchRatio(getSampleRate());

			reversed = s->isReversed();

			addSound(s);
		}
	}

	void getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue) override;
	
	void restoreFromValueTree(const ValueTree &v) override
	{
		ModulatorSynth::restoreFromValueTree(v);

		auto idx = (int)v["LoadedBankIndex"];

		if(idx == AudioFileIndex)
		{
			currentBankIndex = idx;
			AudioSampleProcessor::restoreFromValueTree(v);
		}
		else
		{
			loadAttribute(LoadedBankIndex, "LoadedBankIndex");
		}

		

		loadAttribute(HqMode, "HqMode");
		loadAttributeWithDefault(TableIndexValue);
		loadAttributeWithDefault(RefreshMipmap);
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = ModulatorSynth::exportAsValueTree();

		saveAttribute(HqMode, "HqMode");
		saveAttribute(LoadedBankIndex, "LoadedBankIndex");

		if(currentBankIndex == AudioFileIndex)
		{
			AudioSampleProcessor::saveToValueTree(v);
		}

		saveAttribute(TableIndexValue, "TableIndexValue");
		saveAttribute(RefreshMipmap, "RefreshMipMap");

		return v;
	}

	int getNumChildProcessors() const override { return (int)WavetableInternalChains::numInternalChains;	};

	int getNumInternalChains() const override {return (int)WavetableInternalChains::numInternalChains; };

	virtual Processor *getChildProcessor(int processorIndex) override
	{
		jassert(processorIndex < (int)WavetableInternalChains::numInternalChains);

		switch(processorIndex)
		{
		case GainModulation:	return gainChain;
		case PitchModulation:	return pitchChain;
		case (int)WavetableInternalChains::TableIndexModulation:	return tableIndexChain;
		case (int)WavetableInternalChains::TableIndexBipolarChain:  return tableIndexBipolarChain;
		case MidiProcessor:		return midiProcessorChain;
		case EffectChain:		return effectChain;
		default:				jassertfalse; return nullptr;
		}
	};

	virtual const Processor *getChildProcessor(int processorIndex) const override
	{
		jassert(processorIndex < (int)WavetableInternalChains::numInternalChains);

		switch(processorIndex)
		{
		case GainModulation:	return gainChain;
		case PitchModulation:	return pitchChain;
		case (int)WavetableInternalChains::TableIndexModulation:	return tableIndexChain;
		case (int)WavetableInternalChains::TableIndexBipolarChain:  return tableIndexBipolarChain;
		case MidiProcessor:		return midiProcessorChain;
		case EffectChain:		return effectChain;
		default:				jassertfalse; return nullptr;
		}
	};

	void prepareToPlay(double newSampleRate, int samplesPerBlock) override
	{
		if(newSampleRate > -1.0)
		{
			for(int i = 0; i < sounds.size(); i++)
			{
				static_cast<WavetableSound*>(getSound(i))->calculatePitchRatio(newSampleRate);
			}

			
		}

		if (samplesPerBlock > 0 && newSampleRate > 0.0)
			tableIndexKnobValue.prepare(newSampleRate, 80.0);


		ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);
	}

	float getTotalTableModValue(int offset);

	float getAttribute(int parameterIndex) const override 
	{
		if(parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

		switch(parameterIndex)
		{
		case HqMode:		return hqMode ? 1.0f : 0.0f;
		case RefreshMipmap:		return refreshMipmap ? 1.0f : 0.0f;
		case LoadedBankIndex: return (float)currentBankIndex;
		case TableIndexValue: return tableIndexKnobValue.targetValue;
		default:			jassertfalse; return -1.0f;
		}
	};

	void setInternalAttribute(int parameterIndex, float newValue) override
	{
		if(parameterIndex < ModulatorSynth::numModulatorSynthParameters)
		{
			ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
			return;
		}

		switch(parameterIndex)
		{
		case HqMode:
		{
			ScopedLock sl(getMainController()->getLock());

			hqMode = newValue > 0.5f;

			for(int i = 0; i < getNumVoices(); i++)
			{
				static_cast<WavetableSynthVoice*>(getVoice(i))->setHqMode(hqMode);
			}

			break;
		}
		case RefreshMipmap:
		{
			refreshMipmap = newValue > 0.5f;

			for (int i = 0; i < getNumVoices(); i++)
			{
				static_cast<WavetableSynthVoice*>(getVoice(i))->setRefreshMipmap(hqMode);
			}

			break;
		}
		case TableIndexValue:
			tableIndexKnobValue.set(jlimit(0.0f, 1.0f, newValue));

			if(getNumActiveVoices() == 0)
				setDisplayTableValue((1.0f - reversed) * newValue + (1.0f - newValue) * reversed);

			break;
		case LoadedBankIndex:
		{
			loadWavetableFromIndex((int)newValue);
			break;

		}
		default:					jassertfalse;
									break;
		}
	};

	ModulationDisplayValue::QueryFunction::Ptr getModulationQueryFunction(int parameterIndex) const override
	{
		return ModulatorSynth::getModulationQueryFunction(parameterIndex);
	}

	void rebuildWavetableFromAudioFile()
	{
		getMainController()->getKillStateHandler().killVoicesAndCall(this, [](Processor* p)
		{
			auto ws = static_cast<WavetableSynth*>(p);

			ws->loadWavetableInternal();

			return SafeFunctionCall::OK;
		}, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
	}

	
	virtual void bufferWasLoaded() override
	{
		loadWavetableFromIndex(AudioFileIndex);
	}

	
	virtual void bufferWasModified() override
	{
		loadWavetableFromIndex(AudioFileIndex);
	}

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	File getWavetableMonolith() const;

	StringArray getWavetableList() const;

	void loadWavetableFromIndex(int index);

	void setDisplayTableValue(float nv)
	{
		displayTableValue = nv;
	}

	float getDisplayTableValue() const;

	void renderNextBlockWithModulators(AudioSampleBuffer& outputAudio, const HiseEventBuffer& inputMidi) override
	{
		ModulatorSynth::renderNextBlockWithModulators(outputAudio, inputMidi);

		if(getNumActiveVoices() == 0)
		{
			auto dv = getAttribute(SpecialParameters::TableIndexValue);
			auto mv = modChains[(int)ChainIndex::TableIndex].getOneModulationValue(0);
			dv *= mv;
			jlimit(0.0f, 1.0f, dv);
			setDisplayTableValue(dv);
		}
	}

	void setResynthesisOptions(const WavetableHelpers::ResynthesisOptions& options, NotificationType n=dontSendNotification)
	{
		resynthOptions = options;

		if(n != dontSendNotification && currentBankIndex == AudioFileIndex)
			reloadWavetable();
	}

	void setPostProcessors(Array<PostFXProcessor>&& newPostProcessors, NotificationType n=sendNotification)
	{
		ScopedLock sl(postFXLock);
		postFXProcessors.swapWith(newPostProcessors);

		if(n != dontSendNotification)
			renderPostFX(true);
	}

	WavetableHelpers::ResynthesisOptions getResynthesisOptions() const { return resynthOptions; }

	void reloadWavetable()
	{
		loadWavetableFromIndex(currentBankIndex);
	}

	ValueTree getCurrentlyLoadedWavetableTree() const { return currentlyLoadedWavetableTree; }

	void setResynthesisCache(File newCacheFolder, bool clearCache=false)
	{
		cacheFolder = newCacheFolder;

		if(clearCache && cacheFolder.isDirectory())
		{
			auto list = cacheFolder.findChildFiles(File::findFiles, false, "*.tmp");
			for(auto c: list)
			{
				c.deleteFile();
			}
		}
	}

	void referenceShared(ExternalData::DataType type, int index) override
	{
		if(currentBankIndex == AudioFileIndex)
			reloadWavetable();
	}

	AudioSampleBuffer createAudioSampleBufferFromWavetable(int sampleIndex) const
	{
		if(auto s = dynamic_cast<WavetableSound*>(getSound(sampleIndex)))
		{
			auto size = s->getWavetableAmount() * s->getTableSize();

			AudioSampleBuffer b(s->isStereo() ? 2 : 1, size);

			for(int i = 0; i < b.getNumChannels(); i++)
			{
				FloatVectorOperations::copy(b.getWritePointer(i), s->getWaveTableData(i, 0), size);
			}

			return b;
		}

		return {};
	}

	void resetModChains(int voiceIndex)
	{
		modChains[ChainIndex::TableIndex].resetVoice(voiceIndex);
		modChains[ChainIndex::TableIndexBipolar].resetVoice(voiceIndex);
	}

private:

	bool loadFromCache(const String& filename, WavetableHelpers::ExportData& ed)
	{
		if(cacheFolder.isDirectory())
		{
			auto hash = String(filename.hash());

			auto optionHash = resynthOptions.getHash();
			String fn = String(hash) + "_" + String(optionHash);
			auto list = cacheFolder.findChildFiles(File::findFiles, false, "*.tmp");

			for(auto c: list)
			{
				if(c.getFileNameWithoutExtension() == fn)
				{
					lastResynthesisedData = {};
					c.loadFileAsData(lastResynthesisedData);
					return ed.restore(lastResynthesisedData);
				}
			}
		}

		return false;
	}

	bool storeToCache(const String& filename, const WavetableHelpers::ExportData& cd)
	{
		lastResynthesisedData = cd.save();

		if(cacheFolder.isDirectory())
		{
			auto hash = String(filename.hash());
			auto optionHash = resynthOptions.getHash();
			String fn = String(hash) + "_" + String(optionHash);
			auto cf = cacheFolder.getChildFile(fn).withFileExtension(".tmp");
			return cf.replaceWithData(lastResynthesisedData.getData(), lastResynthesisedData.getSize());
		}

		return false;
	}

	File cacheFolder;
	MemoryBlock lastResynthesisedData;

	void renderPostFX(bool forceProcess)
	{
		if(!postFXProcessors.isEmpty() || forceProcess)
		{
			getMainController()->getKillStateHandler().killVoicesAndCall(this, [](Processor* p)
			{
				auto wt = dynamic_cast<WavetableSynth*>(p);

				WavetableHelpers::ExportData copy;

				copy.restore(wt->lastResynthesisedData);

				wt->postProcessCycles(copy);
				wt->rebuildMipMaps(copy);
				return SafeFunctionCall::OK;
			}, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
		}
	}

	CriticalSection postFXLock;

	Array<PostFXProcessor> postFXProcessors;

	void postProcessCycles(WavetableHelpers::ExportData& ed)
	{
		if(!postFXProcessors.isEmpty())
		{
			for(int ci = 0; ci < ed.cycles.size(); ci++)
			{
				auto norm = ed.cycles.size() == 1 ? 0.0 : ((double)ci / (double)(ed.cycles.size()-1));
				auto& b = ed.cycles.getReference(ci);

				ScopedLock sl(postFXLock);

				for(auto& p: postFXProcessors)
				{
					for(int c = 0; c < b.getNumChannels(); c++)
					{
						VariantBuffer::Ptr cycle = new VariantBuffer(b.getWritePointer(c), b.getNumSamples());
						p.apply(cycle, norm, dontSendNotification);
					}
				}
			}
		}

		float maxGain = 0.0f;

		for(auto& c: ed.cycles)
		{
			auto gain = c.getMagnitude(0, c.getNumSamples());

			maxGain = jmax(gain, maxGain);
		}

		for(auto& c: ed.cycles)
			c.applyGain(1.0f / maxGain);
	}

	void rebuildMipMaps(const WavetableHelpers::ExportData& ed)
	{
		WavetableHelpers::ConfigData cd;
		cd.phaseMode = resynthOptions.pm;
		cd.reverseOrder = resynthOptions.reverseOrder;
		cd.useCompression = false;

		auto mipmapSize = resynthOptions.mipMapSize;
		auto loKey = 0;
		auto hiKey = 128;

		for (int i = loKey + mipmapSize / 2; i < hiKey; i += mipmapSize)
		{
			Range<int> nr(i - mipmapSize / 2, i + mipmapSize / 2 - 1);

			auto sd = WavetableHelpers::createMipMapRange(i, nr, ed);
			sd.store(true, cd);
		}

		loadWaveTable(ed.waveTableTree);
	}

	void loadWavetableInternal();

	ValueTree currentlyLoadedWavetableTree;

	Array<AudioSampleBuffer> originalCycles;

	WavetableHelpers::ResynthesisOptions resynthOptions;

	float displayTableValue = 1.0f;

	sfloat tableIndexKnobValue;
	
	float reversed = 0.0f;

	int currentBankIndex = 0;

	static constexpr int AudioFileIndex = 900000;

	bool hqMode = true;
	bool refreshMipmap = false;

	friend class WavetableSynthVoice;

	ModulatorChain* tableIndexChain;
	ModulatorChain* tableIndexBipolarChain;
};


} // namespace hise

