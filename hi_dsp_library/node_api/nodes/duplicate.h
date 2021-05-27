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

namespace scriptnode {
using namespace juce; 
using namespace hise;
using namespace snex;
using namespace snex::Types;

namespace duplilogic
{

struct spread
{
	double getValue(int index, int numUsed, double inputValue, double gamma)
	{
		if (numUsed == 1)
			return 0.5;

		auto n = (double)index / (double)(numUsed - 1) - 0.5;
		
		if (gamma != 0.0)
		{
			auto gn = hmath::sin(double_Pi * n);
			gn *= 0.5;
			n = gn * gamma + n * (1.0 - gamma);
		}

		n *= inputValue;

		return n + 0.5;
	}
};

struct triangle
{
	double getValue(int index, int numUsed, double inputValue, double gamma)
	{
		if (numUsed == 1)
			return 1.0;

		auto n = (double)index / (double)(numUsed - 1);

		n = hmath::abs(n - 0.5) * 2.0;

		if (gamma != 0.0)
		{
			auto gn = hmath::sin(n * double_Pi * 0.5);
			gn *= gn;

			n = gamma * gn + (1.0 - gamma) * n;
		}

		return 1.0 - inputValue * n;
	}
};



struct harmonics
{
	double getValue(int index, int numUsed, double inputValue, double gamma)
	{
		return (double)(index + 1) * inputValue;
	}
};

struct random
{
	Random r;

	double getValue(int index, int numUsed, double inputValue, double gamma)
	{
		double n;

		if (numUsed == 1)
			n = 0.5f;
		else
			n = (double)index / (double)(numUsed - 1) ;

		return jlimit(0.0, 1.0, n + (2.0 * r.nextDouble() - 1.0) * inputValue);
	}
};

struct scale
{
	double getValue(int index, int numUsed, double inputValue, double gamma)
	{
		if (numUsed == 1)
			return inputValue;

		auto n = (double)index / (double)(numUsed - 1) * inputValue;

		if (gamma != 1.0)
			n = hmath::pow(n, 1.0 + gamma);

		return n;
	}
};

}

namespace wrap
{

struct duplicate_sender
{
	struct Listener
	{
		virtual ~Listener() {};

		virtual void numVoicesChanged(int newNumVoices) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	duplicate_sender(int initialVoiceAmount) :
		numVoices(initialVoiceAmount)
	{};

	void addNumVoiceListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeNumVoiceListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	int getNumVoices() const { return numVoices; }

	virtual void setVoiceAmount(int newNumVoices)
	{
		numVoices = newNumVoices;
	}

	SimpleReadWriteLock& getVoiceLock() { return voiceMultiplyLock; }

protected:

	void sendMessageToListeners()
	{
		for (auto l : listeners)
		{
			if (l.get() != nullptr)
				l->numVoicesChanged(numVoices);
		}
	}

private:

	hise::SimpleReadWriteLock voiceMultiplyLock;
	int numVoices = 1;
	Array<WeakReference<Listener>> listeners;

	JUCE_DECLARE_WEAK_REFERENCEABLE(duplicate_sender);
};



template <typename T> struct duplicate_node_reference
{
	using ObjectType = T;

	duplicate_node_reference() = default;

	template <int P> auto get()
	{
		auto& o = firstObj->template get<P>();
        using TType = typename std::remove_reference<decltype(o)>::type;
		duplicate_node_reference<TType> hn;

		hn.firstObj = &o;
		hn.objectDelta = objectDelta;
		
		return hn;
	}

	T* firstObj = nullptr;
	scriptnode::wrap::duplicate_sender* sender;
	size_t objectDelta = 0;
};


template <typename T, int AllowCopySignal, int AllowResizing, int NumDuplicates> struct duplicate_base : public duplicate_sender
{
	constexpr auto& getObject() { return *this; }
	constexpr const auto& getObject() const { return *this; }

	constexpr auto& getWrappedObject() { return *begin(); }
	constexpr const auto& getWrappedObject() const { return *begin(); }

	using ObjectType = duplicate_base;
	using WrappedObjectType = typename T::WrappedObjectType;

	duplicate_base() :
		duplicate_sender(AllowResizing ? 1 : NumDuplicates)
	{};

	template <int P> static void setWrapParameterStatic(void* obj, double v)
	{
		auto value = (int)v;
		auto typed = static_cast<duplicate_base*>(obj);

		if constexpr (P == 0)
			typed->setVoiceAmount(value);
		if constexpr (P == 1)
			typed->setDuplicateSignal(v > 0.0);

	}

	void setDuplicateSignal(bool shouldDuplicateSignal)
	{
		static_assert(options::isDynamic(AllowCopySignal), "AllowCopySignal is not a dynamic property");

		if (shouldDuplicateSignal != copySignal)
		{
			copySignal = shouldDuplicateSignal;
			resetCopyBuffer();
		}
	}

	template <int P> static void setParameterStatic(void* obj, double v)
	{
		auto typed = static_cast<duplicate_base*>(obj);

		for (auto& e : *typed)
			e.setParameterStatic<P>(v);
	}

	PARAMETER_MEMBER_FUNCTION;

	void initialise(NodeBase* n)
	{
		jassert(getNumVoices() == 1);
		parentNode = n;

		if constexpr (prototypes::check::initialise<T>::value)
			begin()->initialise(parentNode);
	}

	void refreshFromFirst()
	{
		SimpleReadWriteLock::ScopedWriteLock sl(getVoiceLock());

		auto& first = *begin();

		auto ptr = begin() + 1;

		while (ptr != end())
		{
			*ptr = std::move(T(first));
			ptr++;
		}
	}

	void resize(int delta)
	{
		{
			SimpleReadWriteLock::ScopedWriteLock sl(getVoiceLock());

			auto start = begin();
			auto ptr = end();

			if (delta > 0)
			{
				for (int i = 0; i < delta; i++)
				{
					*ptr = *begin();

                    if constexpr (scriptnode::prototypes::check::initialise<T>::value)
					{
						if (parentNode != nullptr)
							ptr->initialise(parentNode);
					}

					ptr++;
				}
			}
			if (delta < 0)
			{
				ptr--;

				for (int i = 0; i < -delta; i++)
				{
					*ptr = T();
					ptr--;
				}
			}
		}
	}

	void setVoiceAmount(int numVoices)
	{
		// AllowResizing is not a dynamic property
		jassert(AllowResizing);

		numVoices = jlimit(1, NumDuplicates, numVoices);

		auto delta = numVoices - getNumVoices();

		if (delta != 0)
			resize(delta);

		duplicate_sender::setVoiceAmount(numVoices);

		if (lastSpecs)
			prepare(lastSpecs);

		sendMessageToListeners();
	}

	bool shouldCopySignal() const
	{
		return options::isTrue(copySignal, AllowCopySignal);
	}

	void resetCopyBuffer()
	{
		if (options::isTrueOrDynamic(AllowCopySignal))
		{
			SimpleReadWriteLock::ScopedWriteLock sl(getVoiceLock());

			if (shouldCopySignal())
			{
				FrameConverters::increaseBuffer(signalCopy, lastSpecs);
				FrameConverters::increaseBuffer(workBuffer, lastSpecs);
			}
			else
			{
				signalCopy.setSize(0);
				workBuffer.setSize(0);
			}
		}
	}

	void prepare(PrepareSpecs ps)
	{
		lastSpecs = ps;

		resetCopyBuffer();

		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getVoiceLock()))
		{
			for (auto& obj : *this)
				obj.prepare(ps);
		}
	}

	void reset()
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getVoiceLock()))
		{
			for (auto& obj : *this)
				obj.reset();
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& frameData)
	{
		static_assert(FrameDataType::hasCompileTimeSize(), "Can't use this with dynamic data");

		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getVoiceLock()))
		{
			if (shouldCopySignal() && getNumVoices() > 1)
			{
				FrameDataType work;
				FrameDataType original = frameData;

				data[0].processFrame(frameData);

				for (int i = 1; i < getNumVoices(); i++)
				{
					work = original;
					data[1].processFrame(work);
					frameData += work;
				}
			}
			else
			{
				for (auto& obj : *this)
					obj.processFrame(frameData);
			}
		}
	}

	template <int P> void processSplitFix(ProcessData<P>& d)
	{
		constexpr int NumChannels = P;

		snex::Types::ProcessDataHelpers<NumChannels>::copyTo(d, signalCopy);

		auto wcd = snex::Types::ProcessDataHelpers<NumChannels>::makeChannelData(workBuffer);

		ProcessData<NumChannels> wd(wcd.begin(), d.getNumSamples());
		wd.copyNonAudioDataFrom(d);

		data[0].process(d);

		auto dPtr = d.getRawDataPointers();
		const auto wPtr = wd.getRawDataPointers();

		for (int i = 1; i < getNumVoices(); i++)
		{
			signalCopy.copyTo(workBuffer);
			data[i].process(wd);

			for (int i = 0; i < NumChannels; i++)
				FloatVectorOperations::add(dPtr[i], wPtr[i], d.getNumSamples());
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getVoiceLock()))
		{
			if (shouldCopySignal() && getNumVoices() > 1)
			{
				if constexpr (ProcessDataType::hasCompileTimeSize())
				{
					processSplitFix(d);
				}
				else
				{
					switch (d.getNumChannels())
					{
					case 1: processSplitFix(d.template as<ProcessData<1>>()); break;
					case 2: processSplitFix(d.template as<ProcessData<2>>()); break;
					case 3: processSplitFix(d.template as<ProcessData<3>>()); break;
					case 4: processSplitFix(d.template as<ProcessData<4>>()); break;
					case 6: processSplitFix(d.template as<ProcessData<6>>()); break;
					case 8: processSplitFix(d.template as<ProcessData<8>>()); break;
					case 16: processSplitFix(d.template as<ProcessData<16>>()); break;
					}
				}
			}
			else
			{
				for (auto& obj : *this)
					obj.process(d);
			}
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getVoiceLock()))
		{
			for (auto& obj : *this)
				obj.handleHiseEvent(e);
		}
	}

	template <int P> auto get()
	{
		auto d = begin();
		auto& o = d->template get<P>();
		using TType = typename std::remove_reference<decltype(o)>::type;

		duplicate_node_reference<TType> hn;

		hn.objectDelta = sizeof(T);
		hn.sender = static_cast<duplicate_sender*>(this);
		hn.firstObj = &o;

		return hn;
	}

	T* begin() const
	{
		return const_cast<T*>(data);
	}

	T* end() const
	{
		return begin() + (AllowResizing ? getNumVoices() : NumDuplicates);
	}

	T data[NumDuplicates];
	PrepareSpecs lastSpecs;
	NodeBase* parentNode = nullptr;

	heap<float> signalCopy, workBuffer;
	bool copySignal;
};

}

namespace parameter
{
using SenderType = scriptnode::wrap::duplicate_sender;

template <class ParameterClass> struct dupli
{
	PARAMETER_SPECS(ParameterType::Dupli, 1);

	ParameterClass p;

	void call(int index, double v)
	{
		jassert(sender != nullptr);

		jassert(isPositiveAndBelow(index, sender->getNumVoices()));

		auto thisPtr = (uint8*)firstObj + index * objectDelta;

		p.setObjPtr(thisPtr);
		p.call(v);
	}

	int getNumVoices() const
	{
		jassert(sender != nullptr);
		return sender->getNumVoices();
	}

	void setParentNumVoiceListener(SenderType::Listener* l)
	{
		if (sender != nullptr)
			sender->addNumVoiceListener(l);
		else
			parentListener = l;
	}

	template <int Unused> auto& getParameter()
	{
		return *this;
	}

	template <int P, typename DupliRefType> void connect(DupliRefType& t)
	{
		sender = t.sender;
		objectDelta = t.objectDelta;
		firstObj = t.firstObj;

		if (parentListener != nullptr)
			sender->addNumVoiceListener(parentListener);
	}

	SenderType* sender = nullptr;
	void* firstObj = nullptr;
	size_t objectDelta;
	SenderType::Listener* parentListener = nullptr;
};

template <class... DupliParameters> struct duplichain : public advanced_tuple<DupliParameters...>
{
	using Type = advanced_tuple<DupliParameters...>;

	PARAMETER_SPECS(ParameterType::DupliChain, 1);

	template <int P> auto& getParameter()
	{
		return *this;
	}

	void* getObjectPtr() { return this; }

	tuple_iterator2(call, int, index, double, v);

	void call(int index, double v)
	{
		call_tuple_iterator2(call, index, v);
	}

	tuple_iterator1(setParentNumVoiceListener, SenderType::Listener*, l);

	void setParentNumVoiceListener(SenderType::Listener* l)
	{
		call_tuple_iterator1(setParentNumVoiceListener, l);
	}

	template <int Index, class Target> void connect(Target& t)
	{
		this->template get<Index>().template connect<0>(t);
	}

	int getNumVoices() const
	{
		return this->template get<0>().getNumVoices();
	}
};
}




} 
