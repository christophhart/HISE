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

template <typename... Ps> static constexpr int getNumVoicesOfFirstElement()
{
	using TupleType = std::tuple<Ps...>;
	using FirstElementType = typename std::tuple_element<0, TupleType>::type;
	return FirstElementType::NumVoices;
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




template <typename, typename = void>
struct has_voice_setter : std::false_type {};

template <typename T>
struct has_voice_setter<T, std::void_t<typename T::ObjectType::VoiceSetter>>
	: std::true_type {};



template <typename T, std::size_t I, bool Has = has_voice_setter<T>::value>
struct base_wrapper; 

template <typename T, std::size_t I>
struct base_wrapper<T, I, true>
{
	using WT = typename T::ObjectType;
    using VS = typename WT::VoiceSetter;

	base_wrapper(T& obj, bool forceAll) :
        vs(obj.getObject(), forceAll)
	{}
    
    explicit operator bool() const
    {
        return static_cast<bool>(vs);
    }
    
    VS vs;
};

template <typename T, std::size_t I>
struct base_wrapper<T, I, false> {
	base_wrapper(T&, bool) {} // ignore it

	operator bool() const { return true; }
};

template <typename Tuple, typename Indices> struct sub_tuple_helper_impl;

template <typename... Ts, std::size_t... Is>
struct sub_tuple_helper_impl<std::tuple<Ts...>, std::index_sequence<Is...>>
	: base_wrapper<Ts, Is>...
{
	sub_tuple_helper_impl(std::tuple<Ts...>& t, bool forceAll)
		: base_wrapper<Ts, Is>(std::get<Is>(t), forceAll)... {}

	operator bool() const
	{
		return (static_cast<bool>(static_cast<const base_wrapper<Ts, Is>&>(*this)) && ...);
	}
};

template <typename... Ts>
using sub_tuple = sub_tuple_helper_impl<std::tuple<Ts...>, std::make_index_sequence<sizeof...(Ts)>>;

struct DummyVoiceSetter
{
	template <typename T> DummyVoiceSetter(T& obj, bool) {};
	operator bool() const { return true; }
};

template <typename T, typename = void>
struct get_voice_setter {
	using type = DummyVoiceSetter;
};

template <typename T>
struct get_voice_setter<T, std::void_t<typename T::ObjectType::VoiceSetter>> {
	using type = typename T::ObjectType::VoiceSetter;
};

}



template <class ParameterClass, typename... Processors> struct container_base
{
	using Type = container_base<ParameterClass, Processors...>;
    
	static constexpr bool isModulationSource = false;

    virtual ~container_base() {};
    
	static constexpr int getFixChannelAmount() { return Helpers::getNumChannelsOfFirstElement<Processors...>(); };

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

	struct VoiceSetter : Helpers::sub_tuple<Processors...>
	{
		VoiceSetter(container_base& t, bool forceAll) :
			Helpers::sub_tuple<Processors...>(t.elements, forceAll)
		{}
	};

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

