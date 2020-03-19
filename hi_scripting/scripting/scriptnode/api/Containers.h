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

struct ContainerParameter
{
	using FuncPointer = void(*)(void*, double);

	Identifier name;

	

	void* obj;
	FuncPointer f = nullptr;

	//std::function<void(double)> f;
};

template <class T> struct Accessor
{
	template <class T> Accessor(T& obj_) :
		obj(obj_)
	{}

	template <class Obj, int I> static auto& get_(Obj& o, std::index_sequence<I> path)
	{
		return o.get<I>();
	}

	template <class Obj, int I, int... Is> static auto& get_(Obj& o, std::index_sequence<I, Is...> path)
	{
		return get_(o.get<I>(), std::index_sequence<Is...>());
	}

	template <size_t... Path> auto& get() const
	{
		return get_(obj, std::index_sequence<Path...>());
	}

	T& obj;
};

template <class ParameterClass, typename... Processors> struct container_base
{
	using Type = container_base<ParameterClass, Processors...>;

	template <std::size_t ...Ns>
	void init_each(NodeBase* b, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (std::get<Ns>(processors).initialise(b), void(), int{})...
		};
	}

	template <std::size_t ...Ns>
	void prepare_each(PrepareSpecs ps, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (std::get<Ns>(processors).prepare(ps), void(), int{})...
		};
	}

	template <std::size_t ...Ns>
	void reset_each(std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (std::get<Ns>(processors).reset(), void(), int{})...
		};
	}

	template <std::size_t ...Ns>
	void handle_event_each(HiseEvent& e, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (std::get<Ns>(processors).handleHiseEvent(e), void(), int{})...
		};
	}

	void initialise(NodeBase* b)
	{
		init_each(b, indexes);
	}

	void reset()
	{
		reset_each(indexes);
	}

	template <size_t arg> constexpr auto& get() noexcept { return std::get<arg>(processors); }
	
	template <size_t arg, size_t... args> constexpr auto& getFunky() noexcept
	{
		Accessor<Type> a(*this);
		return a.get<arg, args...>();
	}

	template <size_t... args> constexpr auto& getParameter() noexcept
	{
		Accessor<ParameterClass> a(parameters);
		return a.get<args...>();
	}

    void createParameters(Array<HiseDspBase::ParameterData>& d)
    {
        
    }
    
	template <int P> static void setParameter(void* obj, double v)
	{
		static_cast<Type*>(obj)->setParameter<P>(v);
	}

	template <int P> void setParameter(double v)
	{
		parameters.call<P>(v);
	}
	

    int getExtraWidth() const { return 0; }
    int getExtraHeight() const { return 0; }
    
    HardcodedNode* getAsHardcodedNode() { return nullptr; };
    Component* createExtraComponent(PooledUIUpdater* updater) { return nullptr; }
    bool isPolyphonic() const { return std::get<0>(processors).getObject().isPolyphonic(); }

	std::tuple<Processors...> processors;
	std::index_sequence_for<Processors...> indexes;

	ParameterClass parameters;
};


}

}

