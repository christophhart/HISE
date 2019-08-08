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

template <int NV> class jit : public HiseDspBase
{
public:

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("jit");
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

		SpinLock::ScopedLockType sl(compileLock);

		auto scopePointer = &scope;

		auto lsCopy = lastSpecs;

		auto compileEachVoice = [scopePointer, newCode, lsCopy](CallbackCollection& c)
		{
			snex::jit::Compiler compiler(*scopePointer);

			auto newObject = compiler.compileJitObject(newCode);

			if (compiler.getCompileResult().wasOk())
			{
				c.obj = newObject;
				c.setupCallbacks();
				c.prepare(lsCopy.sampleRate, lsCopy.blockSize, lsCopy.numChannels);
			}
		};

		cData.forEachVoice(compileEachVoice);
	}

	void prepare(PrepareSpecs specs)
	{
		SpinLock::ScopedLockType sl(compileLock);

		cData.prepare(specs);

		lastSpecs = specs;

		voiceIndexPtr = specs.voiceIndex;

		cData.forEachVoice([specs](CallbackCollection& c)
		{
			c.prepare(specs.sampleRate, specs.blockSize, specs.numChannels);
		});
	}

	void createParameters(Array<ParameterData>& data) override
	{
		code.init(nullptr, this);

		auto c = cData.getFirst();

		auto ids = c.obj.getFunctionIds();
		auto names = snex::jit::ParameterHelpers::getParameterNames(c.obj);

		bool isPoly = cData.isPolyphonic();

		for (auto& name : names)
		{
			ParameterData p(name, { 0.0, 1.0 });
			

			using DoubleCallbackPtr = void(*)(double);

			if (!isPoly)
			{
				auto f = snex::jit::ParameterHelpers::getFunction(name, c.obj);

				if (f)
					p.db = reinterpret_cast<DoubleCallbackPtr>(f.function);
				else
					p.db = {};
			}
			else
			{
				Array<DoubleCallbackPtr> voiceFunctions;

				auto tmp = &voiceFunctions;

				cData.forEachVoice([tmp, name](CallbackCollection& cc)
				{
					auto f = snex::jit::ParameterHelpers::getFunction(name, cc.obj);

					if (f)
						tmp->add(reinterpret_cast<DoubleCallbackPtr>(f.function));
				});

				auto ptr = voiceIndexPtr;

				if (ptr == nullptr)
					return;

				p.db = [voiceFunctions, ptr](double newValue)
				{
					jassert(ptr != nullptr);

					if (*ptr != -1)
					{
						auto f = voiceFunctions[*ptr];
						f(newValue);
					}
					else
					{
						for (auto f : voiceFunctions)
						{
							if(f != nullptr)
								f(newValue);
						}
							
					}
				};
			}


			data.add(std::move(p));
		}
	}

	void initialise(NodeBase* n)
	{
		node = n;

		voiceIndexPtr = n->getRootNetwork()->getVoiceIndexPtr();

		code.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(jit::updateCode));
		code.init(n, this);
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		if(cData.isVoiceRenderingActive())
			cData.get().eventFunction.callVoid(e);
	}

	bool handleModulation(double&)
	{
		return false;
	}

	void reset()
	{
		SpinLock::ScopedLockType sl(compileLock);

		if (cData.isVoiceRenderingActive())
			cData.get().resetFunction.callVoid();
		else
			cData.forEachVoice([](CallbackCollection& c) {c.resetFunction.callVoid(); });
	}

	using CallbackCollection = snex::jit::CallbackCollection;

	void process(ProcessData& d)
	{
		SpinLock::ScopedLockType sl(compileLock);

		auto& cc = cData.get();

		auto bestCallback = cc.bestCallback[CallbackCollection::ProcessType::BlockProcessing];

		switch (bestCallback)
		{
		case CallbackCollection::Channel:
		{
			for (int c = 0; c < d.numChannels; c++)
			{
				snex::block b(d.data[c], d.size);
				cc.callbacks[CallbackCollection::Channel].callVoidUnchecked(b, c);
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
				cc.callbacks[CallbackCollection::Frame].callVoidUnchecked(b);

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
					d.data[c][i] = cc.callbacks[CallbackCollection::Sample].callUncheckedWithCopy<float>(value);
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

		auto& cc = cData.get();

		auto bestCallback = cc.bestCallback[CallbackCollection::ProcessType::FrameProcessing];

		switch (bestCallback)
		{
		case CallbackCollection::Frame:
		{
			snex::block b(frameData, numChannels);
			cc.callbacks[bestCallback].callVoidUnchecked(b);
			break;
		}
		case CallbackCollection::Sample:
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto v = static_cast<float>(frameData[i]);
				auto& f = cc.callbacks[bestCallback];
				frameData[i] = f.callUnchecked<float>(v);
			}
			break;
		}
		case CallbackCollection::Channel:
		{
			for (int i = 0; i < numChannels; i++)
			{
				snex::block b(frameData + i, 1);
				cc.callbacks[bestCallback].callVoidUnchecked(b);
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

	PolyData<CallbackCollection, NumVoices> cData;

	NodePropertyT<String> code;
	NodeBase::Ptr node;

	int* voiceIndexPtr = nullptr;
};
}

class JitNodeBase: public snex::jit::DebugHandler
{
public:

	void initUpdater()
	{
		parameterUpdater.setCallback(asNode()->getPropertyTree().getChildWithProperty(PropertyIds::ID, PropertyIds::Code.toString()),
			{ PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
			BIND_MEMBER_FUNCTION_2(JitNodeBase::updateParameters));
	}
	
	NodeBase* asNode() { return dynamic_cast<NodeBase*>(this); }

	void logMessage(const String& message) override
	{
		String s;
		s << dynamic_cast<NodeBase*>(this)->getId() << ": " << message;
		debugToConsole(dynamic_cast<NodeBase*>(this)->getProcessor(), s);
	}

	virtual HiseDspBase* getInternalJitNode() = 0;

	void updateParameters(Identifier id, var newValue)
	{
		auto obj = getInternalJitNode();

		Array<HiseDspBase::ParameterData> l;
		obj->createParameters(l);

		StringArray foundParameters;

		for (int i = 0; i < asNode()->getNumParameters(); i++)
		{
			auto pId = asNode()->getParameter(i)->getId();

			bool found = false;

			for (auto& p : l)
			{
				if (p.id == pId)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				auto v = asNode()->getParameter(i)->data;
				v.getParent().removeChild(v, asNode()->getUndoManager());
				asNode()->removeParameter(i--);
			}

		}

		for (const auto& p : l)
		{
			foundParameters.add(p.id);

			if (auto param = asNode()->getParameter(p.id))
			{
				p.db(param->getValue());
			}
			else
			{
				auto newTree = p.createValueTree();
				asNode()->getParameterTree().addChild(newTree, -1, nullptr);

				auto newP = new NodeBase::Parameter(asNode(), newTree);
				newP->setCallback(p.db);
				asNode()->addParameter(newP);
			}
		}
	}

	virtual ~JitNodeBase() {};

private:

	valuetree::PropertyListener parameterUpdater;

};

class JitNode : public HiseDspNodeBase<core::jit<1>>,
				public JitNodeBase

{
public:

	JitNode(DspNetwork* parent, ValueTree d) :
		HiseDspNodeBase<core::jit<1>>(parent, d)
	{
		dynamic_cast<core::jit<1>*>(getInternalT())->scope.addDebugHandler(this);
		initUpdater();
	}

	HiseDspBase* getInternalJitNode() override
	{
		return wrapper.getInternalT();
	}

	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new JitNode(n, d); };
};

class JitPolyNode : public HiseDspNodeBase<core::jit<NUM_POLYPHONIC_VOICES>>,
	public JitNodeBase

{
public:

	JitPolyNode(DspNetwork* parent, ValueTree d) :
		HiseDspNodeBase<core::jit<NUM_POLYPHONIC_VOICES>>(parent, d)
	{
		dynamic_cast<core::jit<NUM_POLYPHONIC_VOICES>*>(getInternalT())->scope.addDebugHandler(this);
		initUpdater();
	}

	HiseDspBase* getInternalJitNode() override
	{
		return wrapper.getInternalT();
	}

	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new JitPolyNode(n, d); };
};


}
