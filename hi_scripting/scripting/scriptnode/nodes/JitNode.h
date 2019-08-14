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

struct CallbackTypes
{
static constexpr int Channel = 0;
static constexpr int Frame = 1;
static constexpr int Sample = 2;
static constexpr int Inactive = -1;
};

using namespace snex;

#define loop_block(x) for(auto& x)
#define SET_HISE_BLOCK_CALLBACK(s) static constexpr int BlockCallback = CallbackTypes::s;
#define SET_HISE_FRAME_CALLBACK(s) static constexpr int FrameCallback = CallbackTypes::s;

struct jit_base: public RestorableNode
{
	struct Parameter
	{
		String id;
		std::function<void(double)> f;
	};

	virtual ~jit_base() {};

	struct ConsoleDummy
	{
		template <typename T> void print(const T& value)
		{
			DBG(value);
		}
	} Console;

	virtual void createParameters()
	{

	}

	void addParameter(String name, const std::function<void(double)>& f)
	{
		parameters.add({ name, f });
	}

	Array<Parameter> parameters;

	snex::hmath Math;
};

template <class T, int NV> struct hardcoded_jit : public HiseDspBase,
												  public RestorableNode
{
	constexpr static int NumVoices = NV;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_EXTRA_WIDTH(0);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	SET_HISE_POLY_NODE_ID(T::getStaticId());

	GET_SELF_AS_OBJECT(hardcoded_jit);

	void prepare(PrepareSpecs sp)
	{
		obj.prepare(sp);

		obj.forEachVoice([sp](T& o)
		{
			o.prepare(sp.sampleRate, sp.blockSize, sp.numChannels);
		});
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		obj.get().handleEvent(e);
	}

	void reset()
	{
		if (obj.isVoiceRenderingActive())
			obj.get().reset();
		else
			obj.forEachVoice([](T& o) {o.reset(); });
	}

	bool handleModulation(double& value)
	{
		return false;
	}

	void process(ProcessData& d)
	{
		auto& o = obj.get();

		if (T::BlockCallback == CallbackTypes::Channel)
		{
			for (int i = 0; i < d.numChannels; i++)
			{
				snex::block b(d.data[i], d.size);
				o.processChannel(b, i);
			}
		}
		else if (T::BlockCallback == CallbackTypes::Frame)
		{
			float frame[NUM_MAX_CHANNELS];
			float* copyPtr[NUM_MAX_CHANNELS];
			memcpy(copyPtr, d.data, sizeof(float*)*d.numChannels);
			
			ProcessData copy(copyPtr, d.numChannels, d.size);
			copy.allowPointerModification();

			for (int i = 0; i < d.size; i++)
			{
				copy.copyToFrameDynamic(frame);
				o.processFrame(snex::block(frame, d.numChannels));
				copy.copyFromFrameAndAdvanceDynamic(frame);
			}
		}
		else if (T::BlockCallback == CallbackTypes::Sample)
		{
			for (int c = 0; c < d.numChannels; c++)
			{
				for (int i = 0; i < d.size; i++)
				{
					auto v = d.data[c][i];
					d.data[c][i] = o.processSample(v);
				}
			}
		}
	}

	void processSingle(float* data, int numChannels)
	{
		auto& o = obj.get();

		if (T::FrameCallback == CallbackTypes::Frame)
		{
			o.processFrame(snex::block(data, numChannels));
		}
		else if (T::FrameCallback == CallbackTypes::Channel)
		{
			for (int i = 0; i < numChannels; i++)
				o.processChannel(snex::block(data + i, 1), i);
		}
		else if (T::FrameCallback == CallbackTypes::Sample)
		{
			for (int i = 0; i < numChannels; i++)
				data[i] = o.processSample(data[i]);
		}
	}

	void createParameters(Array<ParameterData>& data) override
	{
		obj.getFirst().createParameters();

		for (auto jp : obj.getFirst().parameters)
		{
			ParameterData p(jp.id);
			p.db = jp.f;
			data.add(std::move(p));
		}

		obj.getFirst().parameters.clear();
	}

	String getSnippetText() const override
	{
		return obj.getFirst().getSnippetText();
	}

	PolyData<T, NumVoices> obj;
};


namespace core
{

template <int NV> class jit : public HiseDspBase,
							  public AsyncUpdater
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

	void handleAsyncUpdate()
	{
		if (auto l = ScopedCompileLock(*this, ScopedCompileLock::Compile))
		{
			auto compileEachVoice = [this](CallbackCollection& c)
			{
				snex::jit::Compiler compiler(scope);

				auto newObject = compiler.compileJitObject(lastCode);

				if (compiler.getCompileResult().wasOk())
				{
					c.obj = newObject;
					c.setupCallbacks();
					c.prepare(lastSpecs.sampleRate, lastSpecs.blockSize, lastSpecs.numChannels);
				}
			};

			cData.forEachVoice(compileEachVoice);
		}
		else
		{
			DBG("DEFER");
			triggerAsyncUpdate();
		}
	}

	void updateCode(Identifier id, var newValue)
	{
		auto newCode = newValue.toString();

		if (newCode != lastCode)
		{
			lastCode = newCode;
			handleAsyncUpdate();
		}
	}

	void prepare(PrepareSpecs specs)
	{
		cData.prepare(specs);

		lastSpecs = specs;

		voiceIndexPtr = specs.voiceIndex;

		if (auto l = ScopedCompileLock(*this, ScopedCompileLock::ReadOnly))
		{
			cData.forEachVoice([specs](CallbackCollection& c)
			{
				c.prepare(specs.sampleRate, specs.blockSize, specs.numChannels);
			});
		}
	}

	template <int Index> bool createParameter(Array<ParameterData>& data)
	{
		auto& c = cData.getFirst();

		auto& cp = c.parameters[Index];

		if (cp.name.isNotEmpty())
		{
			ParameterData p(cp.name, { 0.0, 1.0 });
			p.db = BIND_MEMBER_FUNCTION_1(jit::setParameter<Index>);
			
			data.add(std::move(p));

			return true;
		}

		return false;
	}

	void createParameters(Array<ParameterData>& data) override
	{
		code.init(nullptr, this);

		if (auto l = ScopedCompileLock(*this, ScopedCompileLock::Compile))
		{
			if (!createParameter<0>(data))
				return;
			if (!createParameter<1>(data))
				return;
			if (!createParameter<2>(data))
				return;
			if (!createParameter<3>(data))
				return;
			if (!createParameter<4>(data))
				return;
			if (!createParameter<5>(data))
				return;
			if (!createParameter<6>(data))
				return;
			if (!createParameter<7>(data))
				return;
			if (!createParameter<8>(data))
				return;
			if (!createParameter<9>(data))
				return;
			if (!createParameter<10>(data))
				return;
			if (!createParameter<11>(data))
				return;
			if (!createParameter<12>(data))
				return;
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
		if (auto l = ScopedCompileLock(*this, ScopedCompileLock::ReadOnly))
		{
			if (cData.isVoiceRenderingActive())
				cData.get().eventFunction.callVoid(e);
		}
	}

	bool handleModulation(double&)
	{
		return false;
	}

	void reset()
	{
		if (auto l = ScopedCompileLock(*this, ScopedCompileLock::ReadOnly))
		{
			if (cData.isVoiceRenderingActive())
				cData.get().resetFunction.callVoid();
			else
				cData.forEachVoice([](CallbackCollection& c) {c.resetFunction.callVoid(); });
		}
	}

	using CallbackCollection = snex::jit::CallbackCollection;

	void process(ProcessData& d)
	{
		if (auto l = ScopedCompileLock(*this, ScopedCompileLock::ReadOnly))
		{
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
						d.data[c][i] = cc.callbacks[CallbackCollection::Sample].template callUncheckedWithCopy<float>(value);
					}
				}

				break;
			}
			default:
				break;
			}
		}
	}

	void processSingle(float* frameData, int numChannels)
	{
		if (auto l = ScopedCompileLock(*this, ScopedCompileLock::ReadOnly))
		{
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
					frameData[i] = f.template callUnchecked<float>(v);
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
	}

	template<int Index> void setParameter(double newValue)
	{
		if (auto lock = ScopedCompileLock(*this, ScopedCompileLock::ReadOnly))
		{
			if (cData.isVoiceRenderingActive())
			{
				cData.get().parameters.getReference(Index).f.callVoid(newValue);
			}
			else
			{
				cData.forEachVoice([newValue](CallbackCollection& cc)
				{
					cc.parameters.getReference(Index).f.callVoid(newValue);
				});
			}
		}
	}

	struct ScopedCompileLock
	{
		enum AccessType
		{
			ReadOnly,
			Compile,
			numAccessTypes
		};

		ScopedCompileLock(jit& parent_, AccessType type_):
			parent(parent_),
			type(type_)
		{
			if (type == ReadOnly && !parent.currentlyCompiling)
			{
				++parent.numReaders;
				locked = true;
			}
			else if (parent.numReaders == 0)
			{
				jassert(!parent.currentlyCompiling);

				prevCompileState = parent.currentlyCompiling;
				parent.currentlyCompiling.store(true);
				++parent.numReaders;
				
				locked = true;
			}
			else
				locked = false;
		}

		operator bool() const
		{
			return locked;
		}

		~ScopedCompileLock()
		{
			if (type == Compile)
				parent.currentlyCompiling.store(prevCompileState);

			if (locked)
				--parent.numReaders;
		}

		jit& parent;

		AccessType type;
		bool prevCompileState;
		bool locked;
	};

	std::atomic<int> numReaders = 0;
	std::atomic<bool> currentlyCompiling = false;

	PrepareSpecs lastSpecs;

	snex::jit::GlobalScope scope;

	PolyData<CallbackCollection, NumVoices> cData;

	String lastCode;
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
	
	snex::jit::CallbackCollection& getFirstCollection();

	NodeBase* asNode() { return dynamic_cast<NodeBase*>(this); }

	void logMessage(const String& message) override
	{
		String s;
		s << dynamic_cast<NodeBase*>(this)->getId() << ": " << message;
		debugToConsole(dynamic_cast<NodeBase*>(this)->getProcessor(), s);
	}

	String convertJitCodeToCppClass(int numVoices, bool addToFactory);

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

	String createCppClass(bool isOuterClass) override;

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

	String createCppClass(bool isOuterClass) override;

	HiseDspBase* getInternalJitNode() override
	{
		return wrapper.getInternalT();
	}

	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new JitPolyNode(n, d); };
};


}
