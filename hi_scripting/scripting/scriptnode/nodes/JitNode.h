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
using namespace snex;
using namespace jit;

#if 0
struct CompiledNodeBase: public ReferenceCountedObject
{
	CompiledNodeBase(JitObject&& obj_, ComplexType::Ptr typePtr_) :
		obj(obj_),
		typePtr(typePtr_),
		r(Result::ok())
	{
		
	}

	void init(Compiler& c, int numChannels)
	{
		if (typePtr == nullptr)
		{
			r = Result::fail("Can't find class definition");
			return;
		}

		r = c.getCompileResult();

		if (r.failed())
			return;

		data.allocate(typePtr->getRequiredByteSize(), true);

		ComplexType::InitData d;
		d.dataPointer = data.get();
		d.initValues = typePtr->makeDefaultInitialiserList();
		r = typePtr->initialise(d);

		if (r.wasOk())
		{
			FunctionClass::Ptr fc = typePtr->getFunctionClass();
			auto classId = fc->getClassName();
			auto protoTypes = Types::ScriptnodeCallbacks::getAllPrototypes(c, numChannels);

			for (int i = 0; i < Types::ScriptnodeCallbacks::numFunctions; i++)
			{
				auto id = classId.getChildId(protoTypes[i].id.getIdentifier());

				protoTypes.getReference(i).id = id;

				Array<FunctionData> matches;
				fc->addMatchingFunctions(matches, id);

				for (auto m : matches)
				{
					if (m.matchIdArgsAndTemplate(protoTypes[i]))
					{
						callbacks[i] = m;
						jassert(callbacks[i].function != nullptr);
					}
				}

				if (callbacks[i].function == nullptr)
				{
					auto l = Types::ScriptnodeCallbacks::getIds(classId);
					r = Result::fail(l[i].toString() + " can't be resolved");
					break;
				}
			}
		}
		
	}

	void prepare(PrepareSpecs ps)
	{
		jassert(r.wasOk());
		callbacks[Types::ScriptnodeCallbacks::PrepareFunction].callVoidUnchecked(getObject(), &ps);
	}

	void reset()
	{
		jassert(r.wasOk());
		callbacks[Types::ScriptnodeCallbacks::ResetFunction].callVoidUnchecked(getObject());
	}

	void handleEvent(HiseEvent& e)
	{
		jassert(r.wasOk());
		callbacks[Types::ScriptnodeCallbacks::HandleEventFunction].callVoidUnchecked(getObject(), &e);
	}

	void process(ProcessData& data)
	{
		jassert(r.wasOk());
		callbacks[Types::ScriptnodeCallbacks::ProcessFunction].callVoidUnchecked(getObject(), &data);
	}

	void processFrame(float* data)
	{
		jassert(r.wasOk());
		callbacks[Types::ScriptnodeCallbacks::ProcessFrameFunction].callVoidUnchecked(getObject(), &data);
	}

	void* getObject()
	{
		return reinterpret_cast<void*>(data.get());
	}
	
	using Ptr = ReferenceCountedObjectPtr<CompiledNodeBase>;

	FunctionData callbacks[(int)Types::ScriptnodeCallbacks::numFunctions];

	Result r;
	HeapBlock<uint8> data;
	JitObject obj;
	ComplexType::Ptr typePtr;
};
#endif

struct new_jit: public SnexSource,
				public HiseDspBase
{
	SET_HISE_NODE_ID("jit");

	SN_GET_SELF_AS_OBJECT(new_jit);

	bool isPolyphonic() const { return true; };

	new_jit():
		classId(PropertyIds::FreezedPath, "core::MyClass"),
		lastResult(Result::ok())
	{}

	String getEmptyText() const override
	{
		String s;
		String nl = "\n";

		s << "namespace core" << nl;
		s << "{" << nl;
		s << "using MyClass = container::chain<parameters::empty, core::skip>;" << nl;
		s << "}" << nl;

		return s;
	}

	void prepare(PrepareSpecs ps) override
	{
		lastSpecs = ps;

		recompile();

		if (compiledNode)
		{
			compiledNode->prepare(ps);
			compiledNode->reset();
		}
	};

	void reset()
	{
		if (compiledNode)
			compiledNode->reset();
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (compiledNode)
			compiledNode->handleEvent(e);
	}

	void initialise(NodeBase* n) override
	{
		SnexSource::initialise(n);
		

		auto rc = [this](Identifier id, var value)
		{
			recompile();
		};

		classId.initialise(n);
		classId.setAdditionalCallback(rc);
	}

	void initCompiler(Compiler& c) override
	{
		c.reset();
		snex::Types::SnexObjectDatabase::registerObjects(c, lastSpecs.numChannels);

		Types::SnexTypeConstructData cd(c);
		cd.nodeParent = parentNode.get();
		cd.numChannels = lastSpecs.numChannels;
		parentNode->getRootNetwork()->createSnexNodeLibrary(cd);
	}

	void recompile() override
	{
		if (lastSpecs.numChannels > 0)
		{
			auto c = expression.getValue();

			snex::jit::Compiler compiler(s);

			auto f = [this](Compiler& c, int)
			{
				initCompiler(c);
			};

			JitCompiledNode::Ptr newNode = new JitCompiledNode(compiler, expression.getValue(), classId.getValue(), lastSpecs.numChannels, f);

 			lastResult = newNode->r;

			ScopedLock sl(parentNode->getRootNetwork()->getConnectionLock());

			if (newNode->r.wasOk())
			{
				newNode->prepare(lastSpecs);
				newNode->reset();

				std::swap(compiledNode, newNode);
				updateParameters();
			}
			else
			{
				compiledNode = nullptr;
			}
		}
	}

	void updateParameters()
	{
		ParameterDataList l;
		createParameters(l);

		StringArray foundParameters;

		for (int i = 0; i < parentNode->getNumParameters(); i++)
		{
			auto pId = parentNode->getParameter(i)->getId();

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
				auto v = parentNode->getParameter(i)->data;
				v.getParent().removeChild(v, parentNode->getUndoManager());
				parentNode->removeParameter(i--);
			}
		}

		for (auto& p : l)
		{
			foundParameters.add(p.id);

			if (auto param = parentNode->getParameter(p.id))
			{
				p.dbNew(param->getValue());
				auto pc = new parameter::dynamic_base(p.dbNew);
				param->setCallbackNew(pc);
			}
			else
			{
				auto newTree = p.createValueTree();
				parentNode->getParameterTree().addChild(newTree, -1, nullptr);

				auto newP = new NodeBase::Parameter(parentNode, newTree);

				auto pc = new parameter::dynamic_base(p.dbNew);
				newP->setCallbackNew(pc);
				parentNode->addParameter(newP);
			}
		}
	}

	void process(ProcessDataDyn& d)
	{
		if (compiledNode)
			compiledNode->process(d);
	}

	template <typename T> void processFrame(T& data)
	{
		if (compiledNode)
			compiledNode->processFrame(data);
	};

	

	void createParameters(ParameterDataList& data)
	{
		if (compiledNode)
		{
			auto l = compiledNode->getParameterList();

			for (int i = 0; i < l.size(); i++)
			{
				parameter::data p(l[i].name);
				p.range = { 0.0, 1.0 };
				p.dbNew.referTo(compiledNode->thisPtr, (parameter::dynamic::Function)l[i].function);

				data.add(p);
			}
		}
	}

	PrepareSpecs lastSpecs;
	JitCompiledNode::Ptr compiledNode;
	Result lastResult;
	NodePropertyT<String> classId;
};

#if OLD_JIT_STUFF


#if HISE_INCLUDE_SNEX
using namespace snex;

template <class T, int NV> struct hardcoded_jit : public HiseDspBase,
												  public RestorableNode
{
	constexpr static int NumVoices = NV;

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_EXTRA_WIDTH(0);
#endif

	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	SET_HISE_POLY_NODE_ID(T::getStaticId());

	SN_GET_SELF_AS_OBJECT(hardcoded_jit);

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

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		// richtig neu denken...
		jassertfalse;
#if 0
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
#endif
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		// richtig neu denken...
		jassertfalse;
#if 0
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
#endif
	}

	void createParameters(ParameterDataList& data) override
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

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(50);
	SET_HISE_NODE_EXTRA_WIDTH(384);
#endif

	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	SN_GET_SELF_AS_OBJECT(simple_jit);

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

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		SingleWriteLockfreeMutex::ScopedReadLock sl(compileLock);

		if (expr != nullptr && expr->isValid())
		{
			for (auto& s: data)
				s = (float)expr->getValueUnchecked((double)s);
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		SingleWriteLockfreeMutex::ScopedReadLock sl(compileLock);

		if (expr != nullptr && expr->isValid())
		{
			for (auto& ch : data)
			{
				for (auto& s : data.toChannelData(ch))
					s = (float)expr->getValueUnchecked((double)s);
			}
		}
	}

	hise::SingleWriteLockfreeMutex compileLock;

	snex::JitExpression::Ptr expr;

	NodePropertyT<String> code;
	String lastCode;
	Value codeValue;
};


#if NOT_JUST_OSC

template <int NV> class jit_impl : public HiseDspBase,
							  public AsyncUpdater
{
public:

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("jit");
#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(0);
#endif
	SN_GET_SELF_AS_OBJECT();


	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	jit_impl();

	void handleAsyncUpdate();

	void updateCode(Identifier id, var newValue);

	void prepare(PrepareSpecs specs);

	Array<PDataNewCheckCheck> createParameterDataNewFunkFunk() { return {}; }

	void createParameters(ParameterDataList& data) override;
	void initialise(NodeBase* n);
	void handleHiseEvent(HiseEvent& e) final override;
	bool handleModulation(double&);
	void reset();

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
		{
			// �belst neu denken...
			jassertfalse;

#if 0
			auto& cc = cData.get();

			auto bestCallback = cc.bestCallback[CallbackCollection::ProcessType::BlockProcessing];

			switch (bestCallback)
			{
			case CallbackTypes::Channel:
			{
				for (auto ch : d)
				{

				}

				for (int c = 0; c < d.numChannels; c++)
				{
					snex::block b(d.data[c], d.size);
					cc.callbacks[CallbackTypes::Channel].callVoidUnchecked(b, c);
				}
				break;
			}
			case CallbackTypes::Frame:
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
					cc.callbacks[CallbackTypes::Frame].callVoidUnchecked(b);

					copy.copyFromFrameAndAdvanceDynamic(frame);
				}

				break;
			}
#if 0
			case CallbackTypes::Sample:
			{
				for (int c = 0; c < d.numChannels; c++)
				{
					for (int i = 0; i < d.size; i++)
					{
						auto value = d.data[c][i];
						d.data[c][i] = cc.callbacks[CallbackTypes::Sample].template callUncheckedWithCopy<float>(value);
					}
				}

				break;
			}
#endif
			default:
				break;
			}
#endif
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
#if 0
		if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
		{
			auto& cc = cData.get();

			auto bestCallback = cc.bestCallback[CallbackCollection::ProcessType::FrameProcessing];

			switch (bestCallback)
			{
			case CallbackTypes::Frame:
			{
				snex::block b(frameData, numChannels);
				cc.callbacks[bestCallback].callVoidUnchecked(b);
				break;
			}
			case CallbackTypes::Sample:
			{
				for (int i = 0; i < numChannels; i++)
				{
					auto v = static_cast<float>(frameData[i]);
					auto& f = cc.callbacks[bestCallback];
					frameData[i] = f.template callUnchecked<float>(v);
				}
				break;
			}
			case CallbackTypes::Channel:
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
#endif
	}

	template <int Index> bool createParameter(ParameterDataList& data)
	{
#if 0
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
#endif
	}

	template <int Index> static void setParameter(void* obj, double value)
	{
		jassertfalse;
	}

	template<int Index> void setParameter(double newValue)
	{
		if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
		{
			if (cData.isMonophonicOrInsideVoiceRendering())
				cData.get().parameters.getReference(Index).f.callVoid(newValue);
			else
			{
				for(auto& c: cData)
					c.parameters.getReference(Index).f.callVoid(newValue);
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

#endif

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


	HiseDspBase* getInternalJitNode() override
	{
		return &wrapper.getWrappedObject();
	}

	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new JitNode(n, d); };
};

class JitPolyNode : public HiseDspNodeBase<core::jit_poly>,
	public JitNodeBase

{
public:

	JitPolyNode(DspNetwork* parent, ValueTree d);
	String createCppClass(bool isOuterClass) override;

	HiseDspBase* getInternalJitNode() override
	{
		return &wrapper.getWrappedObject();
	}

	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new JitPolyNode(n, d); };
};


#endif

#endif



}
