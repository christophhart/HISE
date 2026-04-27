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



class MatrixModulator: public EnvelopeModulator,
					   public ExternalDataHolder,
					   public ModulatorSynthChain::Handler::Listener
{
public:

	SET_PROCESSOR_NAME("MatrixModulator", "Matrix Modulator", "");

	enum SpecialParameters
	{
		Value = EnvelopeModulator::numParameters, ///< the base value of the modulation output
		SmoothingTime, ///< the smoothing time of the value parameter
		numTotalParameters
	};

	MatrixModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);
	~MatrixModulator();

	static ProcessorMetadata createMetadata();

	void setInternalAttribute(int parameterInde, float newValue) override;
	float getAttribute(int parameterIndex) const;
	ModulationDisplayValue::QueryFunction::Ptr getModulationQueryFunction(int parameterIndex) const override;

	void setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler) noexcept override;

	void restoreFromValueTree(const ValueTree &v) override;
	ValueTree exportAsValueTree() const override;

	int getNumInternalChains() const override { return getNumChildProcessors(); };
	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int ) override { return nullptr; };
	const Processor *getChildProcessor(int ) const override  { return nullptr; };

	float startVoice(int voiceIndex) override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;
	bool isPlaying(int voiceIndex) const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;

	void processorChanged(EventType, Processor* p) override;
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	ModulatorState *createSubclassedState(int voiceIndex) const override { return nullptr; };

	Table* getTable(int) override { return nullptr; }
	SliderPackData* getSliderPack(int) override { return nullptr; }
	MultiChannelAudioBuffer* getAudioFile(int) override { return nullptr; }
	FilterDataObject* getFilterData(int) override { return nullptr; }
	int getNumDataObjects(ExternalData::DataType t) const override;
	SimpleRingBuffer* getDisplayBuffer(int index) override;
	bool removeDataObject(ExternalData::DataType , int) override { jassertfalse; return false; }

	void onModulationDrop(int parameterIndex, int modulationSourceIndex) override;

	float getInactiveModValue() const override;

	String getModulationTargetId(int parameterIndex) const override;

	void setRangeData(const MatrixIds::Helpers::Properties::RangeData& rd)
	{
		rangeData = rd;

		auto ri = getRangeData(true);
		auto ro = getRangeData(false);

		rangeData.writeToValueTrees(ri, ro);

		if(getMode() == Modulation::PitchMode)
		{
			setIsBipolar(true);
			setIntensity(1.0);
		}

		if(getMode() == Modulation::PanMode)
		{
			setIsBipolar(true);
			setIntensity(1.0);
		}
	}

	MatrixIds::Helpers::IntensityTextConverter::ConstructData getIntensityTextConstructData()
	{
		MatrixIds::Helpers::IntensityTextConverter::ConstructData cd;
		cd.p = this;
		cd.inputRange = rangeData.inputRange.rng;
		cd.parameterIndex = SpecialParameters::Value;
		cd.prettifierMode = rangeData.converter;
		return cd;
	}

	ValueTree getRangeData(bool inputRange) const { return valueRangeData.getChildWithName(inputRange ? MatrixIds::InputRange : MatrixIds::OutputRange); }

	String getMatrixTargetId() const
	{
		if(customTargetId.isEmpty())
			return getId();

		return customTargetId;
	}

	void setCustomTargetId(const String& newId)
	{
		customTargetId = newId;
	}

	int displaySourceIndex = -1;

private:

	String customTargetId;

	static Array<Identifier> getRangeIds(bool isInput);

	friend class MatrixModulatorBody;
	using ModType = scriptnode::core::matrix_mod<NUM_POLYPHONIC_VOICES>;

	bool isConnected() const { return container != nullptr; }

	struct Item
	{
		Item(const ValueTree& v, UndoManager* um);
		~Item() { mod.disconnect(); }

		void onUpdate(const Identifier& id, const var& newValue);
		void handleScaleDrag(bool isDown, float delta);
		void handleDisplayValue(ModulationDisplayValue& mv, scriptnode::InvertableParameterRange outputRange, double rangeFactor);
		void prepare(double sampleRate, int blockSize, scriptnode::PolyHandler& ph);
		bool isConnected() const noexcept { return sourceIndex != -1; }
		SimpleRingBuffer* getRingBuffer() const;

		ValueTree data;
		valuetree::PropertyListener watcher;
		ModType mod;
		bool isScale = false;
		bool isBipolar = false;
		float intensity = 1.0f;
		float auxIntensity = 0.0f;
		bool inverted = false;
		float startIntensity = 1.0f;
		int sourceIndex = -1;
		UndoManager* um;
		SimpleRingBuffer::Ptr displayBuffer;
	};

	ModulationDisplayValue getDisplayValue(double nv, NormalisableRange<double> nr) const;

	void setValueRange(bool isInputRange, scriptnode::InvertableParameterRange nr);
	void setValueInternal(float valueWithinInputRange);

	bool onScaleDrag(bool isDown, float delta);
	void rebuildModList();
	void onMatrixChange(const ValueTree& v, bool wasAdded);
	void onModeChange(const ValueTree& v, const Identifier& id);
	static double getModeValue(const var& v);
	void init();

	void onRangeUpdate(const Identifier& id, const var& newValue, bool isInputRange);

	ValueTree valueRangeData;

	bool rangeRecursiveChecker = false;
	MatrixIds::Helpers::Properties::RangeData rangeData;

	bool hasScaleEnvelopes = false;
	uint8 active[NUM_POLYPHONIC_VOICES];

	mutable SimpleReadWriteLock matrixLock;
	mutable scriptnode::PolyHandler polyHandler;

	WeakReference<GlobalModulatorContainer> container;

	ValueTree globalMatrixData;
	valuetree::ChildListener connectionWatcher;
	valuetree::RecursivePropertyListener modeWatcher;

	valuetree::PropertyListener inputRangeWatcher;
	valuetree::PropertyListener outputRangeWatcher;

	OwnedArray<Item> items;
	std::vector<ModType*> scaleMods;
	std::vector<ModType*> addMods;
	std::vector<ModType*> allMods;

	float inputValue = 0.0f;
	sfloat baseValue;
	float smoothingTime = 50.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MatrixModulator);
	JUCE_DECLARE_WEAK_REFERENCEABLE(MatrixModulator);
};


} // namespace hise

