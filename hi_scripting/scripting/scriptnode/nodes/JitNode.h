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

class JitNode;

namespace core
{

class jit : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("jit");
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	GET_SELF_AS_OBJECT(jit);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	jit() :
		code(PropertyIds::Code, "")
	{

	}

	void updateCode(Identifier id, var newValue)
	{
		auto newCode = newValue.toString();

		snex::jit::Compiler compiler(scope);

		if (compiler.getLastCompiledCode() != newCode)
		{
			SpinLock::ScopedLockType sl(compileLock);

			auto newObject = compiler.compileJitObject(newCode);

			if (compiler.getCompileResult().wasOk())
			{
				cData.obj = newObject;
				cData.setupCallbacks();
				cData.prepare(lastSpecs.sampleRate, lastSpecs.blockSize, lastSpecs.numChannels);
			}
		}
	}

	void prepare(PrepareSpecs specs)
	{
		SpinLock::ScopedLockType sl(compileLock);

		lastSpecs = specs;
		cData.prepare(specs.sampleRate, specs.blockSize, specs.numChannels);
	}

	void createParameters(Array<ParameterData>& data) override
	{
		code.init(nullptr, this);

		auto ids = cData.obj.getFunctionIds();
		auto names = snex::jit::ParameterHelpers::getParameterNames(cData.obj);

		for (auto& name : names)
		{
			ParameterData p(name, { 0.0, 1.0 });
			auto f = snex::jit::ParameterHelpers::getFunction(name, cData.obj);

			if (f)
				p.db = reinterpret_cast<void(*)(double)>(f.function);

			data.add(std::move(p));
		}
	}

	void initialise(NodeBase* n)
	{
		node = n;

		code.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(jit::updateCode));
		code.init(n, this);
	}

	bool handleModulation(double&)
	{
		return false;
	}

	void reset()
	{
		SpinLock::ScopedLockType sl(compileLock);

		if (cData.resetFunction)
			cData.resetFunction.callVoid();
	}

	using CallbackCollection = snex::jit::CallbackCollection;

	void process(ProcessData& d)
	{
		SpinLock::ScopedLockType sl(compileLock);

		auto bestCallback = cData.bestCallback[CallbackCollection::ProcessType::BlockProcessing];

		switch (bestCallback)
		{
		case CallbackCollection::Channel:
		{
			for (int c = 0; c < d.numChannels; c++)
			{
				snex::block b(d.data[c], d.size);
				cData.callbacks[CallbackCollection::Channel].callVoidUnchecked(b, c);
			}
			break;
		}
		case CallbackCollection::Frame:
		{
			float frame[NUM_MAX_CHANNELS];
			float* frameData[NUM_MAX_CHANNELS];
			memcpy(frameData, d.data, sizeof(float*) * d.numChannels);
			ProcessData copy(frameData, d.numChannels, d.size);
			copy.allowPointerModification();

			for (int i = 0; i < d.size; i++)
			{
				copy.copyToFrameDynamic(frame);

				snex::block b(frame, d.numChannels);
				cData.callbacks[CallbackCollection::Frame].callVoidUnchecked(b);

				copy.copyFromFrameAndAdvanceDynamic(frame);
			}

			break;
		}
		case CallbackCollection::Sample:
		{
			for (int c = 0; c < d.numChannels; c++)
			{
				for (int i = 0; i < d.size; i++)
				{
					auto value = d.data[c][i];
					d.data[c][i] = cData.callbacks[CallbackCollection::Sample].callUncheckedWithCopy<float>(value);
				}
			}

			break;
		}
		default:
			break;
		}
	}

	void processSingle(float* frameData, int numChannels)
	{
		SpinLock::ScopedLockType sl(compileLock);

		auto bestCallback = cData.bestCallback[CallbackCollection::ProcessType::FrameProcessing];

		switch (bestCallback)
		{
		case CallbackCollection::Frame:
		{
			snex::block b(frameData, numChannels);
			cData.callbacks[bestCallback].callVoidUnchecked(b);
			break;
		}
		case CallbackCollection::Sample:
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto v = static_cast<float>(frameData[i]);
				auto& f = cData.callbacks[bestCallback];
				frameData[i] = f.callUnchecked<float>(v);
			}
			break;
		}
		case CallbackCollection::Channel:
		{
			for (int i = 0; i < numChannels; i++)
			{
				snex::block b(frameData + i, 1);
				cData.callbacks[bestCallback].callVoidUnchecked(b);
			}
			break;
		}
		default:
			break;
		}
	}

	SpinLock compileLock;

	PrepareSpecs lastSpecs;

	snex::jit::GlobalScope scope;

	CallbackCollection cData;

	NodePropertyT<String> code;
	NodeBase::Ptr node;

};
}

class JitNode : public HiseDspNodeBase<core::jit>,
				public snex::jit::DebugHandler
{
public:

	void logMessage(const String& message) override;

	JitNode(DspNetwork* parent, ValueTree d);

	valuetree::PropertyListener parameterUpdater;

	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new JitNode(n, d); };

	void updateParameters(Identifier id, var newValue);

private:

};


}
