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

class FlexAhdsrEnvelope: public EnvelopeModulator,
						 public ProcessorWithSingleStaticExternalData
{
public:

	SET_PROCESSOR_NAME("FlexAHDSR", "Flex AHDSR Envelope", "A more complex AHDSR envelope modulator with draggable curves & more playback modes");

	using SpecialParameters = flex_ahdsr_base::SpecialParameters;

	enum EditorStates
	{
		AttackTimeChainShown = Processor::numEditorStates,
		AttackLevelChainShown,
		DecayTimeChainShown,
		SustainLevelChainShown,
		ReleaseTimeChainShown,
		numEditorStates
	};

	FlexAhdsrEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);
	~FlexAhdsrEnvelope() override {};

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	Processor *getChildProcessor(int processorIndex) override { return nullptr; }
	const Processor *getChildProcessor(int processorIndex) const override { return nullptr; }
	int getNumChildProcessors() const override { return 0; };
	int getNumInternalChains() const override {return 0;};

	float startVoice(int voiceIndex) override;	
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;;

	void calculateBlock(int startSample, int numSamples);;
	

	ProcessorEditorBody *createEditor(ProcessorEditor* parentEditor) override;

	float getDefaultValue(int parameterIndex) const override;
	void setInternalAttribute (int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	bool isPlaying(int voiceIndex) const override;
    
	ModulatorState *createSubclassedState(int voiceIndex) const override { jassertfalse; return nullptr; }

	class Panel : public PanelWithProcessorConnection
	{
	public:

		enum class FlexPanelIds
		{
			UseOneDimensionDrag = PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds,
			CurvePointTolerance,
			numFlexPanelIds
		};

		SET_PANEL_NAME("FlexAHDSRGraph");

		Panel(FloatingTile* parent);

		void fillIndexList(StringArray& indexList) override
		{
			if(auto b = dynamic_cast<ProcessorWithExternalData*>(getProcessor()))
			{
				auto numObjects = b->getNumDataObjects(ExternalData::DataType::DisplayBuffer);

				for(int i = 0; i < numObjects; i++)
					indexList.add(String("DisplayBuffer #" + (i+1)));
			}
		};

		var toDynamicObject() const override
		{
			auto v = PanelWithProcessorConnection::toDynamicObject();

			auto c = getContent<flex_ahdsr_base::FlexAhdsrGraph>();

			storePropertyInObject(v, (int)FlexPanelIds::CurvePointTolerance, c != nullptr ? var(c->curveTolerance) : var());
			storePropertyInObject(v, (int)FlexPanelIds::UseOneDimensionDrag, c != nullptr ? var(c->useOneDimensionalDrag) : var());

			return v;
		}
		void fromDynamicObject(const var& object) override
		{
			PanelWithProcessorConnection::fromDynamicObject(object);

			if(auto c = getContent<flex_ahdsr_base::FlexAhdsrGraph>())
			{
				c->curveTolerance = (float)getPropertyWithDefault(object, (int)FlexPanelIds::CurvePointTolerance);
				c->useOneDimensionalDrag = (bool)getPropertyWithDefault(object, (int)FlexPanelIds::UseOneDimensionDrag);
			}
		}
		int getNumDefaultableProperties() const override { return (int)FlexPanelIds::numFlexPanelIds; }

		Identifier getDefaultablePropertyId(int index) const override
		{
			if(index < PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds)
				return PanelWithProcessorConnection::getDefaultablePropertyId(index);

			RETURN_DEFAULT_PROPERTY_ID(index, FlexPanelIds::CurvePointTolerance, "CurvePointTolerance");
			RETURN_DEFAULT_PROPERTY_ID(index, FlexPanelIds::UseOneDimensionDrag, "UseOneDimensionDrag");
		}

		var getDefaultProperty(int index) const override
		{
			if (index < (int)SpecialPanelIds::numSpecialPanelIds)
				return PanelWithProcessorConnection::getDefaultProperty(index);

			RETURN_DEFAULT_PROPERTY(index, FlexPanelIds::CurvePointTolerance, 20);
			RETURN_DEFAULT_PROPERTY(index, FlexPanelIds::UseOneDimensionDrag, true);
		}

		Identifier getProcessorTypeId() const override { return FlexAhdsrEnvelope::getClassType(); }
		Component* createContentComponent(int /*index*/) override;
		void fillModuleList(StringArray& moduleList) override;
	};

private:

	static constexpr int getParameterOffset() { return (int)EnvelopeModulator::Parameters::numParameters; }

	friend class FlexAhdsrEnvelopeEditor;

	float parameters[(int)SpecialParameters::numParameters];

	struct AttributeDragHandler: public flex_ahdsr_base::DragHandlerBase
	{
		bool handleAdditionalDrag(int parameterIndex, double value) override
		{
			jassert(parent != nullptr);
			parent->setAttribute(parameterIndex + getParameterOffset(), value, sendNotificationAsync);
			return true;
		};

		FlexAhdsrEnvelope* parent;
	};

	mutable PolyHandler polyHandler;
	scriptnode::envelope::flex_ahdsr<NUM_POLYPHONIC_VOICES, parameter::list<parameter::empty, parameter::empty>, AttributeDragHandler> obj;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlexAhdsrEnvelope);
	JUCE_DECLARE_WEAK_REFERENCEABLE(FlexAhdsrEnvelope);
};


} // namespace hise

