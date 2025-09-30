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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace cable { struct cable_base; }

namespace routing
{

struct base: public mothernode,
			 public polyphonic_base
{
	base(const Identifier& id):
	  polyphonic_base(id, false)
	{
		cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsRoutingNode);
	}

	virtual ~base() {};

	virtual Colour getColour() const = 0;

	

	JUCE_DECLARE_WEAK_REFERENCEABLE(base);
};

struct receive_base: public base
{
	receive_base(const Identifier& id):
	  base(id)
	{};

	~receive_base() override {}

	virtual cable::cable_base** getSourceCablePtr() = 0;
	virtual cable::cable_base* getNullCable() = 0;

	virtual float getFeedbackDisplayValue() const = 0;

	bool isConnected() const
	{
		auto unconst = const_cast<receive_base*>(this);
		return *unconst->getSourceCablePtr() != unconst->getNullCable();
	}
};

}


/** The classes defined in this namespace can be passed into the send / receive nodes
	as template parameters and define the data model of the connection.
*/
namespace cable
{

struct cable_base
{
	virtual ~cable_base() = default;

	virtual void validate(PrepareSpecs ps) {};
	virtual void prepare(PrepareSpecs ps) = 0;
	virtual void initialise(NodeBase* n) {};

	virtual void reset() = 0;

	virtual void connect(routing::receive_base& target)
	{
		*target.getSourceCablePtr() = this;
	}

	virtual void disconnect(routing::receive_base& target)
	{
		*target.getSourceCablePtr() = target.getNullCable();
	}

	Colour colour;
};

/** A cable type for usage in a frame-processing context. */
template <int NV, int C> struct frame: public cable_base
{
	static constexpr int NumVoices = NV;

	using FrameType = span<float, C>;
	using BlockType = ProcessData<C>;

	constexpr bool isPolyphonic() { return NV > 1; }
	constexpr int  getNumChannels() const { return C; };
	static constexpr bool allowFrame() { return true; };
	static constexpr bool allowBlock() { return false; };

	void validate(PrepareSpecs receiveSpecs) override
	{
		jassert(receiveSpecs.numChannels == getNumChannels());
		jassert(receiveSpecs.blockSize == 1);
	}

	void prepare(PrepareSpecs ps) override
	{
		jassert(ps.numChannels <= C);
		frameData.prepare(ps);
	}

	void reset() override
	{
		{
			for(auto& fd: frameData)
				memset(fd.begin(), 0, sizeof(fd));
		}
	}

	void initialise(NodeBase* n) override {};

	void processFrame(FrameType& t)
	{
		int index = 0;

		auto& fd = frameData.get();

		for (auto& d : t)
			fd[index++] = d;
	}

	void setIsNull() {};

	void incCounter(bool, int) noexcept {}

	void process(BlockType& unused)
	{
		jassertfalse;
	};

	PolyData<span<float, C>, NV> frameData;
};


template <int NV, int NumChannels> struct block_base: public cable_base
{
	static constexpr int NumVoices = NV;

	static constexpr bool isPolyphonic() { return NV > 1; }

	template <typename PD> void readIntoBuffer(PD& pd, float feedback)
	{
		auto& d = data.get();

		if(d.buffer.isEmpty())
			return;

		int numTotal = pd.getNumSamples();
		const int readBufferSize = d.channels[0].size();
		int numToDo = numTotal;
		int writePos = 0;
		
		while (numToDo > 0)
		{
			int index = 0;
			int numThisTime = jmin(numToDo, readBufferSize - d.readIndex);

			for (auto& ch : pd)
			{
				jassert(isPositiveAndBelow(d.readIndex + numThisTime, readBufferSize + 1));
				auto src = d.channels[index++].begin() + d.readIndex;
				auto dst = ch.getRawWritePointer() + writePos;

				FloatVectorOperations::addWithMultiply(dst, src, feedback, numThisTime);
			}
			
			incCounter(true, numThisTime);
            numToDo -= numThisTime;
			writePos += numThisTime;
		}
	}

	template <typename PD> void writeToBuffer(PD& pd)
	{
		auto& d = data.get();

		int numTotal = pd.getNumSamples();
		const int writeBufferSize = d.channels[0].size();
		int numToDo = numTotal;
		int readPos = 0;

		while (numToDo > 0)
		{
			int index = 0;
			int numThisTime = jmin(numToDo, writeBufferSize - d.writeIndex);

			for (auto c : pd)
			{
				jassert(isPositiveAndBelow(d.writeIndex + numThisTime, writeBufferSize + 1));
				auto src = c.getRawWritePointer() + readPos;
				auto dst = d.channels[index++].begin() + d.writeIndex;
				
				FloatVectorOperations::copy(dst, src, numThisTime);
			}

			incCounter(false, numThisTime);
			numToDo -= numThisTime;
			readPos += numThisTime;
		}
	}

	void prepare(PrepareSpecs ps) override
	{
		this->data.prepare(ps);

		for(auto& d: data)
			d.prepare(ps);
	}

	void reset() override
	{
		for(auto& d: this->data)
		{
			FloatVectorOperations::clear(d.buffer.begin(), d.buffer.size());
			d.readIndex = 0;
			d.writeIndex = 0;
		}
	}

protected:

	struct Data
	{
		void prepare(PrepareSpecs ps)
		{
			FrameConverters::increaseBuffer(buffer, ps);

			for(int i = 0; i < channels.size(); i++)
			{
				if(isPositiveAndBelow(i, ps.numChannels))
					channels[i].referTo(buffer, ps.blockSize, ps.blockSize * i);
				else
					channels[i].referToNothing();
			}
		}

		span<dyn<float>, NumChannels> channels;

		int writeIndex = 0;
		int readIndex = 0;
		heap<float> buffer;
	};

	PolyData<Data, NV> data;

private:

	void incCounter(bool incReadCounter, int delta)
	{
		auto& d = data.get();

		auto& counter = incReadCounter ? d.readIndex : d.writeIndex;
		counter += delta;

		if (counter == d.channels[0].size())
			counter = 0;
	}
};

template <int NV, int C> struct block: public block_base<NV, C>
{
	using FrameType = span<float, C>;
	using BlockType = ProcessData<C>;

	constexpr int  getNumChannels() const { return C; };

	static constexpr bool allowFrame() { return false; };
	static constexpr bool allowBlock() { return true; };

	void initialise(NodeBase* n) override {};

	void setIsNull() {};

	void validate(PrepareSpecs receiveSpecs) override
	{
		jassert(receiveSpecs.numChannels == getNumChannels());
	}

	void prepare(PrepareSpecs ps) override
	{
		jassert(ps.numChannels <= getNumChannels());
		block_base<NV, C>::prepare(ps);
	};

	void processFrame(FrameType& unused)
	{
		ignoreUnused(unused);
		jassertfalse;
	}

	void process(BlockType& data)
	{
		this->writeToBuffer(data);
	};
};

}

namespace routing
{

struct public_mod_target
{
	virtual ~public_mod_target() {};

	bool handleModulation(double& v)
	{
		return modValue.getChangedValue(v);
	}

	ModValue modValue;
};

struct public_mod
{
	SN_NODE_ID("public_mod");
	SN_GET_SELF_AS_OBJECT(public_mod);
	SN_DESCRIPTION("Creates a modulation signal slot in the compiled node / nested network");

	SN_EMPTY_INITIALISE;
	SN_EMPTY_PROCESS;
	SN_EMPTY_PROCESS_FRAME;
	SN_EMPTY_HANDLE_EVENT;
	SN_EMPTY_RESET;

	void prepare(PrepareSpecs ps)
	{
		// This is beyond ugly, but somebody has tucked a mod value to the temposyncer...

		if (ptr == nullptr)
			ptr = ps.voiceIndex->getTempoSyncer()->publicModValue;
	}

	public_mod()
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsPublicMod);
	}

	static constexpr bool isPolyphonic() { return false; }

	template <int P> void setParameter(double v)
	{
		if (ptr != nullptr)
			ptr->setModValueIfChanged(v);
	}
	SN_FORWARD_PARAMETER_TO_MEMBER(public_mod);

	void connect(public_mod_target& obj)
	{
		ptr = &obj.modValue;
	}

	void createParameters(ParameterDataList& data)
	{
		parameter::data d("Value", { 0.0, 1.0 });
		d.callback = parameter::inner<public_mod, 0>(*this);
		data.add(d);
	}

	ModValue* ptr = nullptr;
};

struct NoCheck
{
	SN_EMPTY_INITIALISE;
	SN_EMPTY_PREPARE;
};

template <int NV, typename CheckClass=NoCheck> struct event_data_reader: 
	public mothernode,
	public polyphonic_base
{
	SN_NODE_ID("event_data_reader");
	SN_GET_SELF_AS_OBJECT(event_data_reader);
	SN_DESCRIPTION("reads data that was written to the event ID storage of the global routing manager");

	static constexpr bool isPolyphonic() { return NV > 1; };

	event_data_reader():
	  polyphonic_base(getStaticId(), true)
	{};

	enum Parameters
	{
		SlotIndex,
		Static
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(SlotIndex, event_data_reader);
		DEF_PARAMETER(Static, event_data_reader);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	SN_EMPTY_RESET;
	SN_EMPTY_PROCESS_FRAME;
	SN_EMPTY_PROCESS;

	void initialise(NodeBase* b)
	{
		checkClass.initialise(b);
	}

	void prepare(PrepareSpecs ps)
	{
		checkClass.prepare(ps);

		eventStorage = ps.voiceIndex->getTempoSyncer()->additionalEventStorage;
		currentEventId.prepare(ps);

		if(eventStorage == nullptr)
			Error::throwError(Error::NoGlobalManager);
	}

	constexpr bool isProcessingHiseEvent() const { return true; }

	bool handleModulation(double& value)
	{
		if(isStatic)
			return staticValue.getChangedValue(value);
		else
			return eventStorage != nullptr ? eventStorage->changed(currentEventId.get(), dataSlot, value) : false;
	}
	
	static constexpr bool isNormalisedModulation() { return true; }

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(event_data_reader_base, SlotIndex);
			p.setRange({0.0, (double)AdditionalEventStorage::NumDataSlots, 1.0});
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(event_data_reader_base, Static);
			p.setParameterValueNames({ "Off", "On"});
			data.add(std::move(p));
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if(e.isNoteOn())
		{
			currentEventId.get() = e.getEventId();

			if(isStatic && eventStorage != nullptr)
			{
				auto v = eventStorage->getValue(currentEventId.get(), dataSlot);

				if(v.first)
					staticValue.setModValue(v.second);
			}
		}
	}

	void setSlotIndex(double newValue)
	{
		dataSlot = jlimit<uint8>(0, hise::AdditionalEventStorage::NumDataSlots, (uint8)newValue);
	}

	void setStatic(double newValue)
	{
		isStatic = newValue > 0.5;
	}

	hise::AdditionalEventStorage* eventStorage = nullptr;
	bool isStatic = false;
	PolyData<uint16, NV> currentEventId;
	ModValue staticValue;
	uint8 dataSlot = 0;

	CheckClass checkClass;
};



template <int NV, typename CheckClass=NoCheck> struct event_data_writer: 
	public mothernode,
	public polyphonic_base
{
	SN_NODE_ID("event_data_writer");
	SN_GET_SELF_AS_OBJECT(event_data_writer);
	SN_DESCRIPTION("writes data to the event ID storage of the global routing manager");

	event_data_writer():
	  polyphonic_base(getStaticId(), true)
	{}

	static constexpr bool isPolyphonic() { return NV > 1; }

	enum Parameters
	{
		SlotIndex,
		Value
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(SlotIndex, event_data_writer);
		DEF_PARAMETER(Value, event_data_writer);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	SN_EMPTY_RESET;
	SN_EMPTY_PROCESS_FRAME;
	SN_EMPTY_PROCESS;

	void initialise(NodeBase* b)
	{
		checkClass.initialise(b);
	}

	constexpr bool isProcessingHiseEvent() const { return true; }

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(event_data_writer, SlotIndex);
			p.setRange({0.0, (double)AdditionalEventStorage::NumDataSlots, 1.0});
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(event_data_writer, Value);
			data.add(std::move(p));
		}
	}

	

	void prepare(PrepareSpecs ps)
	{
		checkClass.prepare(ps);

		eventStorage = ps.voiceIndex->getTempoSyncer()->additionalEventStorage;
		currentEventId.prepare(ps);

		if(eventStorage == nullptr)
			Error::throwError(Error::NoGlobalManager);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if(e.isNoteOn() && eventStorage != nullptr)
		{
			auto& s = currentEventId.get();
			s.first = e.getEventId();
			eventStorage->setValue(s.first, dataSlot, s.second, dontSendNotification);
		}
	}

	void setSlotIndex(double newValue)
	{
		dataSlot = jlimit<uint8>(0, hise::AdditionalEventStorage::NumDataSlots, (uint8)newValue);
	}

	void setValue(double newValue)
	{
		if(eventStorage == nullptr)
			return;

		for(auto& s: currentEventId)
		{
			s.second = newValue;
			eventStorage->setValue(s.first, dataSlot, newValue, dontSendNotification);
		}
	}

	hise::AdditionalEventStorage* eventStorage = nullptr;
	PolyData<std::pair<uint16, double>, NV> currentEventId;
	uint8 dataSlot = 0;

	CheckClass checkClass;
};

struct send_base: public base
{
	send_base(const Identifier& id):
	  base(id)
	{};

	void connect(receive_base& b)
	{
		getCable()->connect(b);
	}

	virtual cable::cable_base* getCable() = 0;

	~send_base() override {}
};

template <int NV, typename CableType> struct receive: public receive_base
{
	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID("receive");

	SN_GET_SELF_AS_OBJECT(receive);
	SN_DESCRIPTION("A signal target for a send node with adjustable feedback");

	enum class Parameters
	{
		Feedback
	};

	receive() :
		receive_base(getStaticId())
	{
		static_assert(std::is_base_of<cable::cable_base, CableType>(), "not a a base of cable::cable_base");
		static_assert(NumVoices == CableType::NumVoices, "voice amount doesn't match");
		null.setIsNull();
	};

	~receive() override
	{}

	template <int P> void setParameter(double value)
	{
		setFeedback(value);
	}

	template <int P> static void setParameterStatic(void* obj, double value)
	{
		auto t = static_cast<receive<NV, CableType>*>(obj);
		t->template setParameter<P>(value);
	}

	void reset()
	{
		source->reset();
	}

	cable::cable_base** getSourceCablePtr() override
	{
		return &source;
	}
	cable::cable_base* getNullCable() override
	{
		return &null;
	}

	void handleHiseEvent(HiseEvent& e) {}

	void initialise(NodeBase* n)
	{
		
	}

	float getFeedbackDisplayValue() const override
	{
		return feedback.getFirst();
	}

	Colour getColour() const override 
	{ 
		if(isConnected())
		{
			return source->colour;
		}

		return Colours::transparentBlack;
	}

	void prepare(PrepareSpecs ps)
	{
		currentSpecs = ps;

		null.prepare(ps);

		feedback.prepare(ps);

		if (isConnected())
			source->validate(currentSpecs);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if constexpr (CableType::allowBlock())
		{
			getTypedSource().readIntoBuffer(data, feedback.get());
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& fd)
	{
		if constexpr (CableType::allowFrame())
		{
			int index = 0;

			auto& sourceData = getTypedSource().frameData.get();

			jassert(fd.size() <= sourceData.size());

			auto fb = feedback.get();

			for (auto& d : fd)
				d += sourceData[index++] * fb;
		}
		else
		{
			// should never be called...
			jassertfalse;
		}
	}

	void createParameters(ParameterDataList& data)
	{
		DEFINE_PARAMETERDATA(receive, Feedback);
		data.add(p);
	}

	CableType& getTypedSource()
	{
		jassert(dynamic_cast<CableType*>(source) != nullptr);
		return *static_cast<CableType*>(source);
	}

	void setFeedback(double value)
	{
		for(auto& f: feedback)
			f = (float)jlimit(0.0, 1.0, value);
	}

	PolyData<float, NumVoices> feedback;

	PrepareSpecs currentSpecs;

	CableType null;
	cable::cable_base* source = &null;
};

/** A node that sends the signal to one or more receive nodes. 

	This can be used to break up the tree structure of the signal chain and implement
	feedback paths.

	If you use it in scriptnode, the CableType will be a polymorphic class that 
	allows usage in all contexts.

	If you export the node or want to use it in the JIT realm, you will need to 
	pass in one of the cable types from the `cable` namespace, then connect the nodes
	like parameters or modulation targets
*/
template <int NV, typename CableType> struct send: public send_base
{
	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID("send");

	SN_GET_SELF_AS_OBJECT(send);
	SN_DESCRIPTION("Send the signal to one or more targets");

	send() :
		send_base(getStaticId())
	{
		static_assert(std::is_base_of<cable::cable_base, CableType>(), "not a a base of cable::cable_base");
		static_assert(NumVoices == CableType::NumVoices, "voice amount doesn't match");
	};

	~send()
	{
		
	}

	void handleHiseEvent(HiseEvent& e) {};

	void createParameters(ParameterDataList&) {};

	void initialise(NodeBase* n)
	{
		cable.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		cable.prepare(ps);
	}

	void reset()
	{
		cable.reset();
	}

	Colour getColour() const override { return cable.colour; }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		cable.process(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		cable.processFrame(data);
	}

	cable::cable_base* getCable() override { return &cable; };

	CableType cable;
};

struct ms_encode: public HiseDspBase
{
	SN_NODE_ID("ms_encode");
	SN_GET_SELF_AS_OBJECT(ms_encode);
	SN_DESCRIPTION("A MS encoder (`L-R -> M-S`)");

	SN_EMPTY_RESET;
	SN_EMPTY_PREPARE;
	SN_EMPTY_CREATE_PARAM;
	SN_EMPTY_INITIALISE;
	SN_EMPTY_HANDLE_EVENT;
	SN_EMPTY_SET_PARAMETER;
	
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		FrameConverters::forwardToFrameStereo(this, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (data.size() == 2)
		{
			auto& l = data[0];
			auto& r = data[1];

			auto m = (l + r) * 0.5f;
			auto s = (l - r) * 0.5f;

			l = m;
			r = s;
		}
	}
};

struct ms_decode: public HiseDspBase
{
	SN_NODE_ID("ms_decode");
	SN_GET_SELF_AS_OBJECT(ms_decode);
	SN_DESCRIPTION("A MS decoder (`M-S -> L-R`)");

	SN_EMPTY_RESET;
	SN_EMPTY_PREPARE;
	SN_EMPTY_CREATE_PARAM;
	SN_EMPTY_INITIALISE;
	SN_EMPTY_HANDLE_EVENT;
	SN_EMPTY_SET_PARAMETER;

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		FrameConverters::forwardToFrameStereo(this, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (data.size() == 2)
		{
			auto& m = data[0];
			auto& s = data[1];

			auto l = (m + s);
			auto r = (m - s);

			m = l;
			s = r;
		}
	}
};

struct selector_base: public mothernode
{
	~selector_base() override {};

	virtual int getChannelIndex() const = 0;

	bool clearOtherChannels = true;
    int numChannels = 1;
    bool selectOutput = false;
    int numProcessingChannels = 2;

	JUCE_DECLARE_WEAK_REFERENCEABLE(selector_base);
};

template <int NV> struct selector: public selector_base,
								   public polyphonic_base
{
    enum class Parameters
    {
        ChannelIndex,
        NumChannels,
        SelectOutput,
        ClearOtherChannels
    };
    
    DEFINE_PARAMETERS
    {
        DEF_PARAMETER(ChannelIndex, selector);
        DEF_PARAMETER(NumChannels, selector);
        DEF_PARAMETER(SelectOutput, selector);
        DEF_PARAMETER(ClearOtherChannels, selector);
    };
	SN_PARAMETER_MEMBER_FUNCTION;
    
    constexpr bool isPolyphonic() { return NV > 1; }

	selector():
	  polyphonic_base(getStaticId(), false)
	{}

    SN_NODE_ID("selector");
    SN_GET_SELF_AS_OBJECT(selector);
    SN_DESCRIPTION("A dynamic router of the first channel (pair)");

    SN_EMPTY_RESET;
    SN_EMPTY_INITIALISE;
    SN_EMPTY_HANDLE_EVENT;
    
    void prepare(PrepareSpecs ps)
    {
        this->numProcessingChannels = ps.numChannels;
		channelIndex.prepare(ps);
    }
    
    static void copy(block dst, const block& src)
    {
        hmath::vmov(dst, src);
    }
    
    static void copy(float& dst, const float& src)
    {
        dst = src;
    }

	int getChannelIndex() const override { return channelIndex.getFirst(); }

    template <typename T> void op(T& data, int size)
    {
		auto c = channelIndex.get();

        int numToProcess = jmin(this->numChannels, size - c);

        if(c != 0)
        {
            if(this->selectOutput)
            {
                for(int i = 0; i < numToProcess; i++)
                    copy(data[c + i], data[i]);
            }
			else
            {
                for(int i = 0; i < numToProcess; i++)
                    copy(data[i], data[c + i]);
            }
        }
        
        if(this->clearOtherChannels)
        {
			if (this->selectOutput)
			{
				for (int i = 0; i < size; i++)
				{
					if (i >= c && i < (c + this->numChannels))
						continue;

					data[i] = 0.0f;
				}
			}
			else
			{
				for (int i = this->numChannels; i < size; i++)
					data[i] = 0.0f;
			}
        }
    }
    
    template <typename ProcessDataType> void process(ProcessDataType& data)
    {
        op(data, data.getNumChannels());
    }

    template <typename FrameDataType> void processFrame(FrameDataType& data)
    {
        op(data, data.size());
    }
    
    void setChannelIndex(double v)
    {
		for(auto& s: channelIndex)
			s = jlimit(0, 16, roundToInt(v));
    }
    
    void setNumChannels(double v)
    {
        this->numChannels = jlimit(0, 16, roundToInt(v));
    };
    
    void setClearOtherChannels(double v)
    {
        this->clearOtherChannels = v > 0.5;
    }
    
    void setSelectOutput(double v)
    {
        this->selectOutput = v > 0.5;
    }
    
    void createParameters(ParameterDataList& data)
    {
        {
            DEFINE_PARAMETERDATA(receive, ChannelIndex);
            p.setRange({0.0, 16.0, 1.0});
            data.add(p);
        }
        
        {
            DEFINE_PARAMETERDATA(receive, NumChannels);
            p.setRange({1.0, 16.0, 1.0});
            p.setDefaultValue(1.0);
            data.add(p);
        }
        
        {
            DEFINE_PARAMETERDATA(receive, SelectOutput);
            p.setRange({1.0, 16.0, 1.0});
            p.setParameterValueNames({"Disabled", "Enabled"});
            p.setDefaultValue(0.0);
            data.add(p);
        }
        
        {
            DEFINE_PARAMETERDATA(receive, ClearOtherChannels);
            p.setParameterValueNames({"Disabled", "Enabled"});
            p.setDefaultValue(1.0);
            data.add(p);
        }
        
    }

	PolyData<int, NV> channelIndex;

    JUCE_DECLARE_WEAK_REFERENCEABLE(selector);
};

struct matrix_helpers
{
	enum class SpecialType
	{
		NoSpecialType,
		AddRightToLeft,
		AddLeftToRight,
		SwapChannels,
		LeftOnly,
		RightOnly,
		numSpecialTypes
	};

	template <SpecialType Type> static void applySpecialType(ProcessData<2>& d)
	{
		static_assert(Type != SpecialType::NoSpecialType);

		auto ptrs = d.getRawDataPointers();

		switch (Type)
		{
		case SpecialType::LeftOnly:
			FloatVectorOperations::clear(ptrs[1], d.getNumSamples());
			break;
		case SpecialType::RightOnly:
			FloatVectorOperations::clear(ptrs[0], d.getNumSamples());
			break;
		case SpecialType::AddRightToLeft:
			FloatVectorOperations::add(ptrs[0], ptrs[1], d.getNumSamples());
			FloatVectorOperations::clear(ptrs[1], d.getNumSamples());
			break;
		case SpecialType::AddLeftToRight:
			FloatVectorOperations::add(ptrs[1], ptrs[0], d.getNumSamples());
			FloatVectorOperations::clear(ptrs[0], d.getNumSamples());
			break;
		case SpecialType::SwapChannels:
		{
			auto bf = (float*)alloca(sizeof(float) * d.getNumSamples());
			FloatVectorOperations::copy(bf, ptrs[0], d.getNumSamples());
			FloatVectorOperations::copy(ptrs[0], ptrs[1], d.getNumSamples());
			FloatVectorOperations::copy(ptrs[1], bf, d.getNumSamples());
			break;
		}
		}
	}

	template <typename MatrixType> static constexpr SpecialType getSpecialType()
	{
		if constexpr (MatrixType::getNumChannels() != 2 || MatrixType::hasSendChannels())
			return SpecialType::NoSpecialType;

		constexpr int l = MatrixType::getChannel(0);
		constexpr int r = MatrixType::getChannel(1);

		if constexpr (l == 0 && r == 0)
			return SpecialType::AddRightToLeft;
		else if constexpr (l == 1 && r == 1)
			return SpecialType::AddLeftToRight;
		else if constexpr (l == 0 && r == -1)
			return SpecialType::LeftOnly;
		else if constexpr (l == -1 && r == 1)
			return SpecialType::RightOnly;
		else if constexpr (l == 1 && r == 0)
			return SpecialType::SwapChannels;

		return SpecialType::NoSpecialType;
	}
};

template <typename IndexType, typename ParameterClass> struct global_cable:
public mothernode,
public control::pimpl::no_processing,
public control::pimpl::parameter_node_base<ParameterClass>,
public runtime_target::indexable_target<IndexType, runtime_target::RuntimeTarget::GlobalCable, double>
{
	using DataCallback = std::function<void(const var& f)>;

    SN_GET_SELF_AS_OBJECT(global_cable);
    SN_NODE_ID("global_cable");
    SN_ADD_SET_VALUE(global_cable);
    
    global_cable():
    control::pimpl::parameter_node_base<ParameterClass>(getStaticId())
    {
        
    };
    
    ~global_cable() = default;
    
    static constexpr bool isPolyphonic() { return false; }
    
    void onValue(double c) override
    {
        if(recursion)
            return;
        
        if(this->getParameter().isConnected())
            this->getParameter().call(c);
    }

	void onData(const void* data, size_t numBytes) override
    {
		if(recursion || !dataCallback)
			return;

		MemoryInputStream mis(data, numBytes, false);
		auto x = var::readFromStream(mis);
		dataCallback(x);
    }

	bool sendData(const void* data, size_t numBytes)
    {
		if(numBytes != 0)
		{
			ScopedValueSetter<bool> rec(recursion, true);
			return this->sendDataToSource(const_cast<void*>(data), numBytes);
		}

		return false;
    }

	void setDataCallback(const DataCallback& f)
    {
	    dataCallback = f;
    }

    void setValue(double newValue)
    {
        ScopedValueSetter<bool> rec(recursion, true);
        this->sendValueToSource(newValue);
    }

	DataCallback dataCallback;
    bool recursion = false;
};

template <class MatrixType> struct matrix
{
	SN_NODE_ID("matrix");

	SN_GET_SELF_AS_OBJECT(matrix);
	SN_DESCRIPTION("A dynamic routing matrix for any arbitrary channel routing");

	SN_EMPTY_RESET;
	SN_EMPTY_SET_PARAMETER;

	void prepare(PrepareSpecs specs)
	{
		m.prepare(specs);
	}

	void initialise(NodeBase* node)
	{
		m.initialise(node);
	}
	
	void handleHiseEvent(HiseEvent& e) {};

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		m.handleDisplayValues(true, data.template as<ProcessDataDyn>());

		if constexpr (MatrixType::isFixedChannelMatrix())
		{
			constexpr auto SType = matrix_helpers::getSpecialType<MatrixType>();

			if constexpr (SType != matrix_helpers::SpecialType::NoSpecialType)
				matrix_helpers::applySpecialType<SType>(data);
			else
				FrameConverters::processFix<MatrixType::getNumChannels()>(this, data);
		}
		else
			FrameConverters::forwardToFrame16(this, data);

		m.handleDisplayValues(false, data.template as<ProcessDataDyn>());
	}
	
	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if constexpr (MatrixType::isFixedChannelMatrix())
		{
			auto& fd = span<float, MatrixType::getNumChannels()>::as(data.begin());
			processMatrixFrame(fd);
		}
		else
		{
			processMatrixFrame(data);
		}
	}

	template <typename FrameDataType> void processMatrixFrame(FrameDataType& data)
	{
		constexpr int MaxChannelAmount = MatrixType::getNumChannels();

		float copyData[MaxChannelAmount + 1];
		copyData[0] = 0.0f;
		float* chData = copyData + 1;

		for (int i = 0; i < data.size(); i++)
		{
			chData[i] = data[i];
			data[i] = 0.0f;
		}

		for (int i = 0; i < data.size(); i++)
		{
			auto index = m.getChannel(i);

			if (index != -1)
				data[index] += chData[i];

			if constexpr (MatrixType::hasSendChannels())
			{
				auto sendIndex = m.getSendChannel(i);

				if (sendIndex != -1)
					data[sendIndex] += chData[i];
			}
		}
	}

	constexpr bool isPolyphonic() const { return false; }

	void createParameters(ParameterDataList&)
	{
		
	}

	MatrixType m;

	JUCE_DECLARE_WEAK_REFERENCEABLE(matrix);
};



template <int N, typename SubType, bool HasSendChannels> struct static_matrix
{
	virtual ~static_matrix() {};
	static constexpr bool isFixedChannelMatrix() { return true; }
	static constexpr bool hasSendChannels() { return HasSendChannels; }

	static constexpr bool createDisplayValues() { return false; }
	void handleDisplayValues(bool, ProcessDataDyn& ) {}

	static constexpr int getChannel(int index) { return SubType::channels[index]; }

	static constexpr int getSendChannel(int index)
	{ 
		if constexpr(hasSendChannels())
			return SubType::sendChannels[index]; 
		else
			return -1;
	}

	SN_EMPTY_INITIALISE;
	SN_EMPTY_PREPARE;

	static constexpr int getNumChannels() { return N; }
};

#ifndef SN_GLOBAL_CABLE
#define SN_GLOBAL_CABLE(hash) routing::global_cable<runtime_target::indexers::fix_hash<hash>, parameter::empty>
#endif

/** A interface class that can be used to send values back to HISE through a global cable.
 
    In order to use this class
    - create a type definition from this template class with the hash codes of the
      global cable IDs using the SN_GLOBAL_CABLE() macro.
    - subclass your node from this the type definition
    - call setGlobalCableValue(0.33) from anywhere in your node

    Note: it's recommended to use the function in HISE that will create the code snippet
          (Tools -> Create C++ code for global cables) which will autogenerate
          the relevant code bits from all available global cables
*/
template <typename... Ts> struct global_cable_cpp_manager: private advanced_tuple<Ts...>
{
	global_cable_cpp_manager()
	{
		ObjectWithJSONConverter::registerStreamCreatorStatic<DynamicObject>();
		ObjectWithJSONConverter::registerStreamCreatorStatic<VariantBuffer>();
	}

	virtual ~global_cable_cpp_manager()
	{
		this->connectToRuntimeTarget(false, {});
	}

	virtual void connectToRuntimeTarget(bool addConnection, const runtime_target::connection& c)
	{
		reset_each(addConnection, c, this->getIndexSequence());

		if(addConnection)
		{
			sendPending_each(this->getIndexSequence());

			for(auto& c: this->pendingData)
				c = {};
		}
	}

	template <auto CableIndex> void sendDataToGlobalCable(const var& dataToSend)
	{
        static constexpr int Idx = static_cast<int>(CableIndex);
        auto& c = this->template get<Idx>();

		this->pendingData[Idx] = {};

		MemoryOutputStream mos;
		dataToSend.writeToStream(mos);
		mos.flush();

		if(!c.sendData(mos.getData(), mos.getDataSize()))
		{
			this->pendingData[Idx] = mos.getMemoryBlock();
		}
	}

	template <auto CableIndex> void registerDataCallback(const std::function<void(const var&)>& f)
	{
		static constexpr int Idx = static_cast<int>(CableIndex);
        auto& c = this->template get<Idx>();
        c.setDataCallback(f);
	}

	template <auto CableIndex> void setGlobalCableValue(double value)
	{
        static constexpr int Idx = static_cast<int>(CableIndex);
        auto& c = this->template get<Idx>();
        c.setValue(value);
	}

private:

	template <std::size_t ...Ns> void reset_each(bool addConnection, const runtime_target::connection& c, std::index_sequence<Ns...>)
	{
		using swallow = int[]; (void)swallow { 1, ( std::get<Ns>(this->elements).connectToRuntimeTarget(addConnection, c) , void(), int{})... };
	};

	template <std::size_t ...Ns> void sendPending_each(std::index_sequence<Ns...>)
	{
		using swallow = int[]; (void)swallow { 1, ( std::get<Ns>(this->elements).sendData(this->pendingData[Ns].getData(), this->pendingData[Ns].getSize()) , void(), int{})... };
	};

	std::array<MemoryBlock, sizeof...(Ts)> pendingData;
	bool dataEnableCalled = false;
};



}


}
