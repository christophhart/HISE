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

namespace container
{

namespace Helpers
{
template <typename... Ps> static constexpr int getNumChannelsOfFirstElement()
{
	using TupleType = std::tuple<Ps...>;
	using FirstElementType = typename std::tuple_element<0, TupleType>::type;
	return FirstElementType::NumChannels;
}

template <class ...Types> struct _ChannelCounter;

template <class T> struct _ChannelCounter<T>
{
	constexpr int operator()() { return T::NumChannels; }
};

template <class T, class ...Types> struct _ChannelCounter<T, Types...>
{
	constexpr int operator()() { return T::NumChannels + _ChannelCounter<Types...>()(); }
};

template <class ...Types> static constexpr int getSummedChannels()
{
	return _ChannelCounter<Types...>()();
}

}



template <class ParameterClass, typename... Processors> struct container_base
{
	using Type = container_base<ParameterClass, Processors...>;
    
	static constexpr bool isModulationSource = false;

    virtual ~container_base() {};
    
	void initialise(NodeBase* b)
	{
		call_tuple_iterator1(initialise, b);
	}

	void reset()
	{
		call_tuple_iterator0(reset);
	}

	template <int arg> constexpr auto& get() noexcept { return std::get<arg>(elements).getObject(); }
	template <int arg> constexpr const auto& get() const noexcept { return std::get<arg>(elements).getObject(); }
	
	template <size_t arg> constexpr auto& getParameter() noexcept
	{
		return parameters.template getParameter<arg>();
	}

    void createParameters(Array<parameter::data>& )
    {
		jassertfalse;
    }
    
	template <int P> static void setParameterStatic(void* obj, double v)
	{
		static_cast<Type*>(obj)->setParameter<P>(v);
	}

	template <int P> void setParameter(double v)
	{
		getParameter<P>().call(v);
	}

    bool isPolyphonic() const { return get<0>().isPolyphonic(); }

	ParameterClass parameters;

protected:

	static constexpr auto getIndexSequence()
	{
		return std::index_sequence_for<Processors...>();
	}

	tuple_iterator1(prepare, PrepareSpecs, ps);
	tuple_iterator1(handleHiseEvent, HiseEvent&, e);
	
	std::tuple<Processors...> elements;

private:

	tuple_iterator0(reset);
	tuple_iterator1(initialise, NodeBase*, b);
};


}

}

