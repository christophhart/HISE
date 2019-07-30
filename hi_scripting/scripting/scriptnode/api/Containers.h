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

template <int T> struct IsZero
{
	static constexpr bool value = T == 0;
};


template <typename... Processors> struct container_base
{
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

	template <int arg> auto& get() noexcept { return std::get<arg>(processors); }
	template <int arg> const auto& get() const noexcept { return std::get<arg>(processors); }

	auto& getObject() { return *this; };
	const auto& getObject() const { return *this; };

	std::tuple<Processors...> processors;
	std::index_sequence_for<Processors...> indexes;
};


#if 0

namespace impl { 

template <int arg>
struct AccessHelper
{
	template <typename ProcessorType>
	static auto& get(ProcessorType& a) noexcept { return AccessHelper<arg - 1>::get(a.processors); }

	template <typename ProcessorType>
	static const auto& get(const ProcessorType& a) noexcept { return AccessHelper<arg - 1>::get(a.processors); }
};

template <>
struct AccessHelper<0>
{
	template <typename ProcessorType>
	static auto& get(ProcessorType& a) noexcept { return a.getProcessor(); }

	template <typename ProcessorType>
	static const auto& get(const ProcessorType& a) noexcept { return a.getProcessor(); }
};

}

#endif


}

}
