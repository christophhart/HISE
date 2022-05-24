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

/**  TODO SEND STUFF:


- add a dragging icon if not connected
- add error handling (also while dragging)

- remove old stuff

Refactor ideas:

- check whether the node property needs the HiseDspBase at all...
- remove getAsHardcodedNode() once and for all


*/



/** The classes defined in this namespace can be passed into the send / receive nodes
	as template parameters and define the data model of the connection.
*/
namespace cable
{

/** A cable type for usage in a frame-processing context. */
template <int C, int AddToSignal=1> struct frame
{
	using FrameType = span<float, C>;
	using BlockType = ProcessData<C>;

	Colour colour = Colours::transparentBlack;

	constexpr int  getNumChannels() const { return C; };
	constexpr bool addToSignal() const { return AddToSignal != 0; };
	static constexpr bool allowFrame() { return true; };
	static constexpr bool allowBlock() { return false; };

	void validate(PrepareSpecs receiveSpecs)
	{
		jassert(receiveSpecs.numChannels == getNumChannels());
		jassert(receiveSpecs.blockSize == 1);
	}

	void prepare(PrepareSpecs ps)
	{
		jassert(ps.numChannels <= C);
	}

	void reset()
	{
		for (auto& d : frameData)
			d = 0.0f;
	}

	void initialise(NodeBase* n) {};

	void processFrame(FrameType& t)
	{
		int index = 0;

		for (auto& d : t)
			frameData[index++] = d;
	}

	void setIsNull() {};

	void incCounter(bool, int) noexcept {}

	void process(BlockType& unused)
	{
		jassertfalse;
	};

	template <typename T> void connect(T& receiveTarget)
	{
		receiveTarget.source = this;
	}

	span<float, C> frameData;
};


template <int C, int AddToSignal=1> struct block
{
	using FrameType = span<float, C>;
	using BlockType = ProcessData<C>;

	Colour colour = Colours::transparentBlack;

	constexpr int  getNumChannels() const { return C; };
	constexpr bool addToSignal() const { return AddToSignal != 0; };

	static constexpr bool allowFrame() { return false; };
	static constexpr bool allowBlock() { return true; };

	void initialise(NodeBase* n) {};

	void setIsNull() {};

	void validate(PrepareSpecs receiveSpecs)
	{
		jassert(receiveSpecs.numChannels == getNumChannels());
	}

	void prepare(PrepareSpecs ps)
	{
		jassert(ps.numChannels <= getNumChannels());
		snex::Types::FrameConverters::increaseBuffer(buffer, ps);

		int index = 0;

		auto d = ProcessDataHelpers<C>::makeChannelData(buffer);

		for (auto& ch : d)
			channels[index++].referToRawData(ch, ps.blockSize);
	};

	template <typename T> void connect(T& receiveTarget)
	{
		receiveTarget.source = this;
	}

	void reset()
	{
		for (auto& d : channels)
			hmath::vset(d, 0.0f);
	}

	void processFrame(FrameType& unused)
	{
		ignoreUnused(unused);
		jassertfalse;
	}

	void incCounter(bool incReadCounter, int delta)
	{
		auto& counter = incReadCounter ? readIndex : writeIndex;

		counter += delta;

		if (counter == channels[0].size())
			counter = 0;
	}

	void process(BlockType& data)
	{
		int numThisTime = data.getNumSamples();

		int index = 0;
		for (auto c : data)
		{
			auto src = c.getRawWritePointer();
			auto dst = channels[index++].begin() + writeIndex;

			FloatVectorOperations::copy(dst, src, numThisTime);
		}

		incCounter(false, numThisTime);
	};

	heap<float> buffer;
	span<dyn<float>, C> channels;
	int writeIndex = 0;
	int readIndex = 0;
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

struct base
{
	base(const Identifier& id)
	{
		cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsRoutingNode);
	}

	virtual ~base() {};

	virtual Colour getColour() const = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(base);
};

template <typename CableType> struct receive: public base
{
	SN_NODE_ID("receive");

	SN_GET_SELF_AS_OBJECT(receive);
	SN_DESCRIPTION("A signal target for a send node with adjustable feedback");

	enum class Parameters
	{
		Feedback
	};

	receive() :
		base(getStaticId())
	{
		null.setIsNull();
	};

	constexpr bool isPolyphonic() const { return false; }

	template <int P> void setParameter(double value)
	{
		setFeedback(value);
	}

	template <int P> static void setParameterStatic(void* obj, double value)
	{
		auto t = static_cast<receive<CableType>*>(obj);
		t->template setParameter<P>(value);
	}

	SN_EMPTY_RESET;

	bool isConnected() const
	{
		return &null != source;
	}

	void disconnect()
	{
		source = &null;
		
	}

	void handleHiseEvent(HiseEvent& e) {}

	void initialise(NodeBase* n)
	{
		
	}

	void prepare(PrepareSpecs ps)
	{
		currentSpecs = ps;

		null.prepare(ps);

		if (isConnected())
			source->validate(currentSpecs);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (CableType::allowBlock())
		{
			if (auto srcPointer = source->buffer.begin())
			{
				int numToReadThisTime = data.getNumSamples();

				int i = 0;

				

				for (auto& ch : data)
				{
					jassert(isPositiveAndBelow(numToReadThisTime-1, source->channels[i].size()));
					
					auto src = source->channels[i++].begin() + source->readIndex;

					if (source->addToSignal())
						FloatVectorOperations::addWithMultiply(ch.getRawWritePointer(), src, feedback, data.getNumSamples());
					else
						FloatVectorOperations::copy(ch.getRawWritePointer(), src, data.getNumSamples());
				}

				source->incCounter(true, numToReadThisTime);
			}
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if constexpr (CableType::allowFrame())
		{
			jassert(data.size() <= source->frameData.size());

			if (source->addToSignal())
			{
				int index = 0;

				for (auto& d : data)
					d += source->frameData[index++] * feedback;
			}
			else
			{
				int index = 0;

				for (auto& d : data)
					d = source->frameData[index++] * feedback;
			}
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

	Colour getColour() const override 
	{ 
		if(isConnected())
			return source->colour; 

		return Colours::transparentBlack;
	}

	void setFeedback(double value)
	{
		feedback = (float)jlimit(0.0, 1.0, value);
	}

	float feedback = 0.0f;

	PrepareSpecs currentSpecs;

	CableType null;
	CableType* source = &null;
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
template <typename CableType> struct send: public base
{
	SN_NODE_ID("send");

	SN_GET_SELF_AS_OBJECT(CableType);
	SN_DESCRIPTION("Send the signal to one or more targets");

	send() :
		base(getStaticId())
	{};

	constexpr bool isPolyphonic() const { return false; }

	template <typename Target> void connect(Target& t)
	{
		cable.connect(t);
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

	CableType cable;
};

}


namespace routing
{

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
		if (MatrixType::isFixedChannelMatrix())
			FrameConverters::processFix<MatrixType::getNumChannels()>(this, data);
		else
			FrameConverters::forwardToFrame16(this, data);
	}
	
	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (MatrixType::isFixedChannelMatrix())
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

			auto sendIndex = m.getSendChannel(i);

			if (sendIndex != -1)
				data[sendIndex] += chData[i];
		}
	}

	constexpr bool isPolyphonic() const { return false; }

	void createParameters(ParameterDataList&)
	{
		
	}

	MatrixType m;

	JUCE_DECLARE_WEAK_REFERENCEABLE(matrix);
};

}


}
