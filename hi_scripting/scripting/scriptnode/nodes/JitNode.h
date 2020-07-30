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


#if HISE_INCLUDE_SNEX
using namespace snex;

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
		if (obj.isMonophonicOrInsideVoiceRendering)
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
		if (NumVoices == 1)
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
		else
		{
			obj.forEachVoice([](T& o)
			{
				o.createParameters();
			});


			int index = 0;

			for (auto jp : obj.getFirst().parameters)
			{
				ParameterData p(jp.id);

				p.db = [index, this](double newValue)
				{
					if (obj.isMonophonicOrInsideVoiceRendering)
					{
						obj.get().parameters[index].f(newValue);
					}
					else
					{
						obj.forEachVoice([index, newValue](T& o)
						{
							o.parameters[index].f(newValue);
						});
					}
				};

				index++;
				data.add(std::move(p));
			}
		}
	}

	String getSnippetText() const override
	{
		return obj.getFirst().getSnippetText();
	}

	PolyData<T, NumVoices> obj;
};

struct HiseBufferHandler : public snex::BufferHandler
{
	HiseBufferHandler(Processor* parentProcessor) :
		processor(parentProcessor)
	{

	}

	void registerExternalItems() override;

	WeakReference<hise::Processor> processor;
};

namespace core
{

class simple_jit : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("simple_jit");
	SET_HISE_NODE_EXTRA_HEIGHT(50);
	SET_HISE_NODE_EXTRA_WIDTH(384);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(simple_jit);

	simple_jit():
		code(PropertyIds::Code, "input")
	{
		
	}

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_MOD;
	HISE_EMPTY_RESET;
	HISE_EMPTY_CREATE_PARAM;
	
	void initialise(NodeBase* n)
	{
		code.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(simple_jit::updateCode));
		code.init(n, this);

		

		codeValue = n->getNodePropertyAsValue(PropertyIds::Code);
	}

	void updateCode(Identifier id, var newValue)
	{
		SingleWriteLockfreeMutex::ScopedWriteLock sl(compileLock);

		expr = new snex::JitExpression(newValue.toString());
	}

	Component* createExtraComponent(PooledUIUpdater* updater) override;

	void processSingle(float* data, int numChannels)
	{
		SingleWriteLockfreeMutex::ScopedReadLock sl(compileLock);

		if (expr != nullptr && expr->isValid())
		{
			for (int i = 0; i < numChannels; i++)
				data[i] = (float)expr->getValueUnchecked((double)data[i]);
		}
	}

	void process(ProcessData& data)
	{
		SingleWriteLockfreeMutex::ScopedReadLock sl(compileLock);

		if (expr != nullptr && expr->isValid())
		{
			for (int c = 0; c < data.numChannels; c++)
			{
				auto ptr = data.data[c];

				for (int i = 0; i < data.size; i++)
				{
					ptr[i] = (float)expr->getValueUnchecked(ptr[i]);
				}
			}
		}
	}

	hise::SingleWriteLockfreeMutex compileLock;

	snex::JitExpression::Ptr expr;

	NodePropertyT<String> code;
	String lastCode;
	Value codeValue;
};

template <int NV> class jit_impl : public HiseDspBase,
							  public AsyncUpdater
{
public:

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("jit");
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	GET_SELF_AS_OBJECT(jit_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	jit_impl();

	void handleAsyncUpdate();

	void updateCode(Identifier id, var newValue);

	void prepare(PrepareSpecs specs);

	void createParameters(Array<ParameterData>& data) override;
	void initialise(NodeBase* n);
	void handleHiseEvent(HiseEvent& e) final override;
	bool handleModulation(double&);
	void reset();
	void process(ProcessData& d);
	void processSingle(float* frameData, int numChannels);

	template <int Index> bool createParameter(Array<ParameterData>& data)
	{
		auto& c = cData.getFirst();
        auto cp = c.parameters[Index];

		if (cp.name.isNotEmpty())
		{
			ParameterData p(cp.name, { 0.0, 1.0 });
			p.db = BIND_MEMBER_FUNCTION_1(jit_impl::setParameter<Index>);
			data.add(std::move(p));
			return true;
		}

		return false;
	}

	template<int Index> void setParameter(double newValue)
	{
		if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
		{
			if (cData.isMonophonicOrInsideVoiceRendering())
				cData.get().parameters.getReference(Index).f.callVoid(newValue);
			else
			{
				cData.forEachVoice([newValue](CallbackCollection& cc)
				{
					cc.parameters.getReference(Index).f.callVoid(newValue);
				});
			}
		}
	}

	

	hise::SingleWriteLockfreeMutex lock;
	PrepareSpecs lastSpecs;
	snex::jit::GlobalScope scope;
	PolyData<CallbackCollection, NumVoices> cData;
	String lastCode;
	NodePropertyT<String> code;
	NodeBase::Ptr node;

	int* voiceIndexPtr = nullptr;
};

DEFINE_EXTERN_NODE_TEMPLATE(jit, jit_poly, jit_impl);


}

class JitNodeBase
{
public:

	virtual ~JitNodeBase() {};

	void initUpdater();
	
	snex::CallbackCollection& getFirstCollection();

	NodeBase* asNode() { return dynamic_cast<NodeBase*>(this); }
	
	String convertJitCodeToCppClass(int numVoices, bool addToFactory);
	virtual HiseDspBase* getInternalJitNode() = 0;
	void updateParameters(Identifier id, var newValue);

private:

	valuetree::PropertyListener parameterUpdater;

};

class JitNode : public HiseDspNodeBase<core::jit>,
				public JitNodeBase

{
public:

	JitNode(DspNetwork* parent, ValueTree d);
	String createCppClass(bool isOuterClass) override;
	HiseDspBase* getInternalJitNode() override;
	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new JitNode(n, d); };
};

class JitPolyNode : public HiseDspNodeBase<core::jit_poly>,
	public JitNodeBase

{
public:

	JitPolyNode(DspNetwork* parent, ValueTree d);
	String createCppClass(bool isOuterClass) override;
	HiseDspBase* getInternalJitNode() override;
	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new JitPolyNode(n, d); };
};
#endif





}
