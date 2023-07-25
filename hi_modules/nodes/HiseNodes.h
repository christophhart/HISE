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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;
using namespace snex;
using namespace snex::Types;

#if 0
namespace container
{

template <class P, typename... Ts> using frame1_block = wrap::frame<1, container::chain<P, Ts...>>;
template <class P, typename... Ts> using frame2_block = wrap::frame<2, container::chain<P, Ts...>>;
template <class P, typename... Ts> using frame4_block = wrap::frame<4, container::chain<P, Ts...>>;
template <class P, typename... Ts> using framex_block = wrap::frame_x< container::chain<P, Ts...>>;
template <class P, typename... Ts> using oversample2x = wrap::oversample<2,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample4x = wrap::oversample<4,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample8x = wrap::oversample<8,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample16x = wrap::oversample<16, container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using modchain = wrap::control_rate<chain<P, Ts...>>;

}
#endif

namespace core
{




struct OscDisplay : public ScriptnodeExtraComponent<OscillatorDisplayProvider>
{
	OscDisplay(OscillatorDisplayProvider* n, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent(n, updater)
	{
		p = f.createPath("sine");
		this->setSize(0, 50);
	};

	void paint(Graphics& g) override
	{
		auto h = this->getHeight();
		auto b = this->getLocalBounds().withSizeKeepingCentre(h * 2, h).toFloat();
		p.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), true);
		GlobalHiseLookAndFeel::fillPathHiStyle(g, p, h * 2, h, false);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new OscDisplay(reinterpret_cast<ObjectType*>(obj), updater);
	}

	void timerCallback() override
	{
		if (getObject() == nullptr)
			return;

		auto thisMode = (int)getObject()->currentMode;

		if (currentMode != thisMode)
		{
			currentMode = thisMode;

			auto pId = MarkdownLink::Helpers::getSanitizedFilename(this->getObject()->modes[currentMode]);
			p = f.createPath(pId);
			this->repaint();
		}
	}

	int currentMode = 0;
	WaveformComponent::WaveformFactory f;
	Path p;
};


class hise_mod_base : public data::display_buffer_base<true>
{
public:

	enum class Parameters
	{
		Index, 
		numParameters
	};

	constexpr bool isPolyphonic() const { return true; }

	virtual void initialise(NodeBase* b);
	virtual void prepare(PrepareSpecs ps);
	
	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto numToDo = (double)d.getNumSamples();
		auto& u = uptime.get();
		u = fmod(u + numToDo * uptimeDelta, synthBlockSize);
		auto v = getModulationValue(roundToInt(u));
		modValues.get().setModValueIfChanged(v);
		
		if(&uptime.getFirst() == &u)
			updateBuffer(v, d.getNumSamples());
	}

	bool handleModulation(double& v);;

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& u = uptime.get();
		u = Math.fmod(u + 1.0 * uptimeDelta, synthBlockSize);

		auto v = getModulationValue(roundToInt(u));
		modValues.get().setModValueIfChanged(v);

		if (&uptime.getFirst() == &u)
			updateBuffer(v, 1);
	}

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Index, hise_mod_base);
	}

	virtual void handleHiseEvent(HiseEvent& e);
	
	virtual void setIndex(double index) = 0;

	void reset();

    static void updateBlockSize(hise_mod_base& b, int newBlockSize)
    {
        b.synthBlockSize = newBlockSize;
        b.uptimeDelta = 0.0;
    }
    
protected:

	virtual double getModulationValue(int uptimeValue) = 0;

	virtual bool isConnected() const = 0;

	WeakReference<ModulationSourceNode> parentNode;

	PolyData<ModValue, NUM_POLYPHONIC_VOICES> modValues;
	PolyData<double, NUM_POLYPHONIC_VOICES> uptime;
	
	double uptimeDelta = 0.0;
	double synthBlockSize = 0.0;

	hmath Math;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(hise_mod_base);
};

class extra_mod : public hise_mod_base
{
public:

	SN_NODE_ID("extra_mod");
	SN_GET_SELF_AS_OBJECT(extra_mod);

	extra_mod()
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UncompileableNode);
	}

	enum Index
	{
		Extra1,
		Extra2,
		numIndexes
	};

	static constexpr bool isNormalisedModulation() { return true; }

	void setIndex(double index) override
	{
		auto inp = roundToInt(index);

		if (inp == 0)
			modIndex = JavascriptSynthesiser::ModChains::Extra1;
		else if (inp == 1)
			modIndex = JavascriptSynthesiser::ModChains::Extra2;
	}

	void initialise(NodeBase* b) override
	{
		hise_mod_base::initialise(b);
		parentProcessor = dynamic_cast<JavascriptSynthesiser*>(b->getScriptProcessor());
	}

	void prepare(PrepareSpecs ps) override
	{
		hise_mod_base::prepare(ps);

		if (!isConnected())
		{
			auto& exp = parentNode->getRootNetwork()->getExceptionHandler();
			exp.addCustomError(parentNode, Error::IllegalMod, "the extra_mod node must only be used in a scriptnode synthesiser");
		}

		if (parentProcessor != nullptr && ps.sampleRate > 0.0)
		{
			synthBlockSize = (double)parentProcessor->getLargestBlockSize();
			uptimeDelta = parentProcessor->getSampleRate() / ps.sampleRate;
		}
	}

	double getModulationValue(int startSample) final override 
	{
		return isConnected() ? parentProcessor->getModValueForNode(modIndex, startSample) : 0.0;
	}

	bool isConnected() const final override
	{
		return parentProcessor != nullptr;
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(extra_mod, Index);
			p.setParameterValueNames({ "Extra 1", "Extra 2" });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
	}

private:

	WeakReference<JavascriptSynthesiser> parentProcessor;
	int modIndex = -1;
};


class pitch_mod : public hise_mod_base
{
public:

	SN_NODE_ID("pitch_mod");
	SN_GET_SELF_AS_OBJECT(pitch_mod);

	pitch_mod()
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UncompileableNode);
	}

	static constexpr bool isNormalisedModulation() { return false; }

	void setIndex(double ) override
	{
	}

	void initialise(NodeBase* b) override
	{
		hise_mod_base::initialise(b);

		auto p = dynamic_cast<Processor*>(b->getScriptProcessor());

		if ((parentSynth = dynamic_cast<ModulatorSynth*>(p)))
			return;

		parentSynth = dynamic_cast<ModulatorSynth*>(p->getParentProcessor(true));
	}

	static void transformModValues(float* d, int numSamples);

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		hise_mod_base::setExternalData(d, index);

		if (auto rb = dynamic_cast<SimpleRingBuffer*>(d.obj))
		{
			if (auto mp = dynamic_cast<ModPlotter::ModPlotterPropertyObject*>(rb->getPropertyObject().get()))
			{
				mp->transformFunction = transformModValues;
			}
		}
	}

	void prepare(PrepareSpecs ps) override
	{
		hise_mod_base::prepare(ps);

		if (!isConnected())
		{
			parentNode->getRootNetwork()->getExceptionHandler().addCustomError(parentNode, Error::IllegalMod, "the pitch_mod node must only be used in a sound generator with a pitch chain");
		}
		else if (dynamic_cast<ModulatorSynthChain*>(parentSynth.get()))
		{
			parentNode->getRootNetwork()->getExceptionHandler().addCustomError(parentNode, Error::IllegalMod, "the pitch_mod node cannot be used in a container");
		}
		else if (ps.sampleRate > 0.0)
		{
			synthBlockSize = (double)parentSynth->getLargestBlockSize();
			uptimeDelta = parentSynth->getSampleRate() / ps.sampleRate;
			valueRange = { 0, (int)synthBlockSize };
		}
	}

	double getModulationValue(int startSample) final override
	{
		if (isConnected())
		{
			auto ptr = parentSynth->getPitchValuesForVoice();

			if (!valueRange.contains(startSample))
				ptr = nullptr;
			
			if(ptr != nullptr)
				return ptr[startSample];
			else
				return parentSynth->getConstantPitchModValue();
		}
		
		return 0.0;
	}

	bool isConnected() const final override
	{
		return parentSynth != nullptr;
	}

private:

	Range<int> valueRange;
	WeakReference<ModulatorSynth> parentSynth;
	int modIndex = -1;
};

class global_mod : public hise_mod_base
{
public:

	SN_NODE_ID("global_mod");
	SN_GET_SELF_AS_OBJECT(global_mod);

	global_mod()
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UncompileableNode);
	}

	static constexpr bool isNormalisedModulation() { return true; }

	void setIndex(double newIndex) final override
	{
		if (globalContainer != nullptr)
		{
			auto p = globalContainer->getChildProcessor(ModulatorSynth::GainModulation)->getChildProcessor(roundToInt(newIndex));

			if (auto m = dynamic_cast<Modulator*>(p))
				connectedMod = m;
            
            connectedToVoiceStart = dynamic_cast<VoiceStartModulator*>(p) != nullptr;
		}
	}

	void initialise(NodeBase* b) override
	{
		hise_mod_base::initialise(b);

		globalContainer = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(b->getScriptProcessor()->getMainController_()->getMainSynthChain());

	}

	void prepare(PrepareSpecs ps) override
	{
		hise_mod_base::prepare(ps);

		noteNumbers.prepare(ps);

		if (globalContainer == nullptr)
		{
			parentNode->getRootNetwork()->getExceptionHandler().addCustomError(parentNode, Error::IllegalMod, "You need a global modulator container in your signal path");
		}
		else if (ps.sampleRate > 0.0)
		{
			synthBlockSize = (double)globalContainer->getLargestBlockSize();
			uptimeDelta = globalContainer->getSampleRate() / ps.sampleRate;
			valueRange = { 0, (int)synthBlockSize };
		}
	}

	double getModulationValue(int startSample) final override
	{
		if (isConnected())
		{
            if(connectedToVoiceStart)
            {
                return globalContainer->getConstantVoiceValue(connectedMod, noteNumbers.get());
            }
            else if(auto ptr = globalContainer->getModulationValuesForModulator(connectedMod, jmax(0, startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR)))
            {
                return ptr[0];
            }
		}

		return 0.0;
	}

	bool isConnected() const final override
	{
		return connectedMod != nullptr;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		hise_mod_base::handleHiseEvent(e);

		if (e.isNoteOn())
		{
			noteNumbers.get() = e.getNoteNumberIncludingTransposeAmount();
		}
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(global_mod, Index);
			p.setRange({ 0.0, 16.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
	}

private:

	PolyData<int, NUM_POLYPHONIC_VOICES> noteNumbers;

	Range<int> valueRange;
	
	WeakReference<GlobalModulatorContainer> globalContainer;
	WeakReference<Modulator> connectedMod;
    bool connectedToVoiceStart = false;
};


} // namespace core

}
