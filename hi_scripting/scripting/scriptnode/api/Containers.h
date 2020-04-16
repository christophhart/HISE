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

	static auto getIndexSequence()
	{
		return std::index_sequence_for<Processors...>();
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
		init_each(b, getIndexSequence());
	}

	void reset()
	{
		reset_each(getIndexSequence());
	}

	template <int ParameterIndex, class T> void connect(T& object)
	{
		connect<ParameterIndex, 0>(object);
	}

	template <int ParameterIndex, int ConnectionIndex, class T> constexpr void connect(T& object)
	{
		auto offset = reinterpret_cast<int64_t>(&object) - reinterpret_cast<int64_t>(this);

		// If this fires, you are trying to connect a parameter to an object that is not part of this container...
		jassert(isPositiveAndBelow(offset, sizeof(Type)));

		auto& p = getParameter<ParameterIndex>().get<ConnectionIndex>();

		

		p.connect(object.getObject());
	}

	template <int arg> constexpr auto& get() noexcept { return std::get<arg>(processors).getObject(); }
	template <int arg> constexpr const auto& get() const noexcept { return std::get<arg>(processors).getObject(); }
	
	template <size_t arg> constexpr auto& getParameter() noexcept
	{
		return parameters.get<arg>();
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
    bool isPolyphonic() const { return get<0>().isPolyphonic(); }

	ParameterClass parameters;
	std::tuple<Processors...> processors;
	
};


}

}

