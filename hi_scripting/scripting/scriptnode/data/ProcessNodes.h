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


template <int B> class FixedBlockNode: public SerialNode
{
public:

	FixedBlockNode(DspNetwork* n, ValueTree d) :
		SerialNode(n, d)
	{
		initListeners();

		bypassListener.setCallback(d, { PropertyIds::Bypassed },
			valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(FixedBlockNode<B>::updateBypassState));
	};

	static constexpr int FixedBlockSize = B;

	SCRIPTNODE_FACTORY(FixedBlockNode<B>, "fix" + String(FixedBlockSize) + "_block");

	void process(ProcessData& data) final override
	{
		if (isBypassed())
		{
			for (auto n : nodes)
				n->process(data);
		}
		else
		{
			int numToDo = data.size;
			data.size = FixedBlockSize;

			while (numToDo > 0)
			{
				for (auto n : nodes)
					n->process(data);

				for (auto& d : data)
					d += FixedBlockSize;

				numToDo -= FixedBlockSize;
			}
		}
	}

	int getBlockSizeForChildNodes() const override
	{ 
		return isBypassed() ? originalBlockSize : FixedBlockSize;
	}

	void updateBypassState(Identifier, var newValue)
	{
		auto b = (bool)newValue;
		prepare(originalBlockSize, originalSampleRate);
	}

	AudioSampleBuffer b;

	valuetree::PropertyListener bypassListener;
};

template <int NumChannels> class SingleSampleBlock : public SerialNode
{
public:

	SingleSampleBlock(DspNetwork* n, ValueTree d) :
		SerialNode(n, d)
	{
		initListeners();
		obj.getObject().initialise(this);
	};

	SCRIPTNODE_FACTORY(SingleSampleBlock<NumChannels>, "frame" + String(NumChannels) + "_block");

	String getCppCode(CppGen::CodeLocation location)
	{
		if (location == CppGen::CodeLocation::PrepareBody)
		{
			String s;
			s << "blockSize = 1;\n";
			s << NodeContainer::getCppCode(location);
			return s;
		}
		if (location == CppGen::CodeLocation::ProcessBody)
		{
			String s;

			s << "static constexpr int NumChannels = " << NumChannels << ";\n\n";
			s << "int numToDo = data.size;\n";
			s << "float frame[NumChannels];\n\n";
			s << "while (--numToDo >= 0)\n{\n";
			s << "data.copyToFrame<NumChannels>(frame);\n";

			for (auto n : nodes)
				s << n->getId() << ".processSingle(frame, NumChannels);\n";

			s << "data.copyFromFrameAndAdvance<NumChannels>(frame);\n";
			s << "}\n";

			return s;
		}
		else
			return SerialNode::getCppCode(location);
	}

	void reset() final override
	{
		obj.reset();
	}

	void process(ProcessData& data) final override
	{
		if (isBypassed())
			obj.getObject().process(data);
		else
			obj.process(data);
	}

	void processSingle(float* frameData, int numChannels) final override
	{
		jassert(numChannels == NumChannels);

		obj.processSingle(frameData, numChannels);
	}

	void updateBypassState(Identifier, var newValue)
	{
		auto b = (bool)newValue;
		
		prepare(originalSampleRate, originalBlockSize);
	}

	int getBlockSizeForChildNodes() const override
	{
		return isBypassed() ? originalBlockSize : 1;
	};

	valuetree::PropertyListener bypassListener;

	wrap::frame<NumChannels, SerialNode::DynamicSerialProcessor> obj;
};

namespace container
{

template <typename... Ts> using frame1_block = wrap::frame<1, container::chain<Ts...>>;
template <typename... Ts> using frame2_block = wrap::frame<2, container::chain<Ts...>>;
template <typename... Ts> using frame3_block = wrap::frame<3, container::chain<Ts...>>;
template <typename... Ts> using frame4_block = wrap::frame<4, container::chain<Ts...>>;
template <typename... Ts> using frame6_block = wrap::frame<6, container::chain<Ts...>>;
template <typename... Ts> using frame8_block = wrap::frame<8, container::chain<Ts...>>;
template <typename... Ts> using frame16_block = wrap::frame<16, container::chain<Ts...>>;

template <typename... Ts> using oversample2x = wrap::oversample<2, container::chain<Ts...>>;
template <typename... Ts> using oversample4x = wrap::oversample<4, container::chain<Ts...>>;
template <typename... Ts> using oversample8x = wrap::oversample<8, container::chain<Ts...>>;
template <typename... Ts> using oversample16x = wrap::oversample<16, container::chain<Ts...>>;

}



/*
using frame1_block = SingleSampleBlock<1>;
using frame2_block = SingleSampleBlock<2>;
using frame3_block = SingleSampleBlock<3>;
using frame4_block = SingleSampleBlock<4>;
using frame6_block = SingleSampleBlock<6>;
using frame8_block = SingleSampleBlock<8>;
using frame16_block = SingleSampleBlock<16>;
using fix32_block = FixedBlockNode<32>;
using fix64_block = FixedBlockNode<64>;
using fix128_block = FixedBlockNode<128>;
using fix256_block = FixedBlockNode<256>;
using fix512_block = FixedBlockNode<512>;
using fix1024_block = FixedBlockNode<1024>;
*/



}
