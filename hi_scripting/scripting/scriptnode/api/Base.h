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

class HardcodedNode;

class HiseDspBase
{
public:

	using Function = std::function<void(HiseDspBase&)>;

	virtual ~HiseDspBase() {};

	struct ParameterData
	{
		ParameterData(const String& id_);;
		ParameterData(const String& id_, NormalisableRange<double> r);;
		ParameterData withRange(NormalisableRange<double> r);

		ValueTree createValueTree() const;
		
		void operator()(double newValue) const;
		void setBypass(double newValue) const;
		void callWithRange(double value);
		void addConversion(const Identifier& converterId);
		void setParameterValueNames(const StringArray& valueNames);
		void init();

		String id;
		NormalisableRange<double> range;
		double defaultValue = 0.0;
		std::function<void(double)> db;
		StringArray parameterNames;
	};

	template <class ObjectType> class ExtraComponent : public Component,
		public PooledUIUpdater::SimpleTimer
	{
	protected:

		ExtraComponent(ObjectType* t, PooledUIUpdater* updater) :
			SimpleTimer(updater),
			object(dynamic_cast<HiseDspBase*>(t))
		{};

		ObjectType* getObject() const
		{
			return dynamic_cast<ObjectType*>(object.get());
		}

	private:

		WeakReference<HiseDspBase> object;
	};

	bool isHardcoded() const;

	virtual int getExtraWidth() const { return 0; };

	virtual void initialise(NodeBase* n)
	{
		ignoreUnused(n);
	}

	virtual void prepare(PrepareSpecs specs) {};

	virtual HardcodedNode* getAsHardcodedNode() { return nullptr; }

	virtual void handleHiseEvent(HiseEvent& e)
	{
		ignoreUnused(e);
	};

	virtual Array<HiseDspBase*> createListOfNodesWithSameId()
	{
		return { this };
	}

	virtual Component* createExtraComponent(PooledUIUpdater* updater)
	{
		ignoreUnused(updater);
		return nullptr;
	}

	virtual bool isPolyphonic() const { return false; }

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseDspBase);

	virtual void createParameters(Array<ParameterData>& data) = 0;

};

template <class T> class SingleWrapper : public HiseDspBase
{
public:

	inline void initialise(NodeBase* n) override
	{
		obj.initialise(n);
	}

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return obj.createExtraComponent(updater);
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		obj.handleHiseEvent(e);
	}

	bool isPolyphonic() const override
	{
		return obj.isPolyphonic();
	}

	HardcodedNode* getAsHardcodedNode() override { return obj.getAsHardcodedNode(); }

	Array<HiseDspBase*> createListOfNodesWithSameId() override
	{
		if (std::is_base_of<HiseDspBase, T>())
		{
			// Would be great to use if constexpr here...
			auto ptr = reinterpret_cast<HiseDspBase*>(&obj);
			jassert(ptr != nullptr);
			return { this, ptr };
		}
			
		else
			return { this };
	}

protected:

	T obj;
};

namespace wrap
{

template <class T> class synth : public SingleWrapper<T>
{
public:

	void process(ProcessData& d)
	{
		HiseEventBuffer::Iterator it(*d.eventBuffer);

		float* currentChannels[NUM_MAX_CHANNELS];
		memcpy(currentChannels, d.data, sizeof(float*) * d.numChannels);

		int samplePos = 0;
		int lastPos = 0;
		HiseEvent e;
		int numTodo = d.size;

		while (it.getNextEvent(e, samplePos, true))
		{
			handleEvent(e);

			int numThisTime = samplePos - lastPos;

			if (numThisTime > 0)
			{
				ProcessData copy(currentChannels, d.numChannels, numThisTime);
				copy.eventBuffer = nullptr;
				renderVoices(copy);
				lastPos = samplePos;
				copy.advanceChannelPointers(numThisTime);
				numTodo -= numThisTime;
			}
		}

		if (numTodo > 0)
		{
			ProcessData copy(currentChannels, d.numChannels, numTodo);
			copy.eventBuffer = nullptr;
			renderVoices(copy);
		}
	}

	void handleEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
		{
			auto newVoiceIndex = (uint16)voiceMask.findNextClearBit(0);
			voiceMask.setBit(newVoiceIndex, true);
			activeVoices.insert({ newVoiceIndex, e });

			DBG("Start voice with index " + String(newVoiceIndex));

			ScopedValueSetter<int> svs(currentVoiceIndex, newVoiceIndex);
			this->obj.reset();
		}

		for (const auto& av : activeVoices)
		{
			ScopedValueSetter<int> svs(currentVoiceIndex, av.voiceIndex);

			const bool sameChannel = av.noteOnEvent.getChannel() == e.getChannel();
			const bool sameEvent = av.noteOnEvent.getEventId() == e.getEventId();

			const bool processForThisVoice = e.isAllNotesOff() || (sameChannel && sameEvent);

			if(processForThisVoice)
				this->handleHiseEvent(e);
		}
	}

	void renderVoices(ProcessData& d)
	{
		for (int i = 0; i < activeVoices.size(); i++)
		{
			if(!renderVoice(d, activeVoices[i]))
			{
				jassert(!voiceMask[activeVoices[i].voiceIndex]);
				DBG("Stop voice with index " + String(activeVoices[i].voiceIndex));
				activeVoices.removeElement(i--);
			}
		}	
	}

	bool handleModulation(double&) { return false; }

	bool renderVoice(ProcessData& d, const VoiceData& vd)
	{
		float* chData[NUM_MAX_CHANNELS];
		auto data = ALLOCA_FLOAT_ARRAY(d.size * d.numChannels);
		auto copy = d.copyToRawArray(chData, data, true);
		copy.shouldReset = true;

		ScopedValueSetter<int> svs(currentVoiceIndex, vd.voiceIndex);
		this->obj.process(copy);

		d += copy;

		if (copy.shouldReset)
		{
			voiceMask.clearBit(vd.voiceIndex);
			return false;
		}

		return true;
	}

	void reset()
	{
		this->obj.reset();
		
		activeVoices.clear();
		voiceMask.clear();
	}

	void processSingle(float* d, int numChannels)
	{
		jassertfalse;
	}

	void prepare(PrepareSpecs specs)
	{
		specs.voiceIndex = &currentVoiceIndex;
		this->obj.prepare(specs);
	}
	
	void createParameters(Array<HiseDspBase::ParameterData>& data) override
	{
		this->obj.createParameters(data);
	}

	auto& getObject() { return this->obj.getObject(); }
	const auto& getObject() const { return this->obj.getObject(); }

	hise::UnorderedStack<VoiceData> activeVoices;
	BigInteger voiceMask;
	int currentVoiceIndex = -1;
};
}




}
