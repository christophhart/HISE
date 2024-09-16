/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
namespace Types {
using namespace juce;


struct LibraryBuilderBase
{
	LibraryBuilderBase(Compiler& c_, int numChannels_):
		c(c_),
		numChannels(numChannels_)
	{

	}

	/** Override this and return a namespace. */
	virtual Identifier getFactoryId() const = 0;

	NamespacedIdentifier getLibraryNamespace() const { return NamespacedIdentifier(getFactoryId()); }

	virtual ~LibraryBuilderBase() {};

	/** override this method and register all types. */
	virtual Result registerTypes() = 0;

protected:

	Compiler& c;
	const int numChannels;
};


struct ScriptnodeCallbacks
{
	enum ID
	{
		PrepareFunction,
		ResetFunction,
		HandleEventFunction,
		ProcessFunction,
		ProcessFrameFunction,
		OptionalOffset, // Use this as array length to exlude the optional callbacks below
		HandleModulation,
		SetExternalDataFunction,
		GetPlotValue,
		numTotalFunctions
	};

	static ID getCallbackId(const NamespacedIdentifier& p);

	static Array<NamespacedIdentifier> getIds(const NamespacedIdentifier& p);

	static Array<FunctionData> getAllPrototypes(Compiler* c, int numChannels);

	static jit::FunctionData getPrototype(Compiler* c, ID id, int numChannels);
};


struct SnexObjectDatabase
{
	static void registerObjects(Compiler& c, int numChannels);
	
};


struct SnexTypeConstructData
{
	SnexTypeConstructData(snex::jit::Compiler& c_) :
		c(c_)
	{}

	snex::jit::Compiler& c;
	void* nodeParent = nullptr;
	int numChannels = 0;
	bool polyphonic = false;
	NamespacedIdentifier id;
};

struct DefaultFunctionClass
{
	using CreateFunction = std::function<FunctionData*(const SnexTypeConstructData& )>;

	DefaultFunctionClass() {};

	template <typename ObjectType> DefaultFunctionClass(ObjectType* t)
	{
        jassertfalse;
#if 0
		resetFunction = Types::SnexNodeBase::Wrappers::createResetFunction<ObjectType>;
		prepareFunction = Types::SnexNodeBase::Wrappers::createPrepareFunction<ObjectType>;
		processFunction = Types::SnexNodeBase::Wrappers::createProcessFunction<ObjectType>;
		handleEventFunction = Types::SnexNodeBase::Wrappers::createHandleEventFunction<ObjectType>;
        processFrameFunction = Types::SnexNodeBase::Wrappers::createProcessFrameFunction<ObjectType>;
#endif
	}

	CreateFunction resetFunction;
	CreateFunction prepareFunction;
	CreateFunction handleEventFunction;
	CreateFunction processFunction;
	CreateFunction processFrameFunction;
};

#if 0
struct SnexNodeBase : public snex::ComplexType
{
	SnexNodeBase(const SnexTypeConstructData& cd_) :
		cd(cd_)
	{};

	virtual ~SnexNodeBase() {};

	template <typename OT> struct DefaultWrappers
	{
		static void reset(void* obj)
		{
			static_cast<OT*>(obj)->reset();
		}

		static void handleHiseEvent(void* obj, HiseEvent* e)
		{
			static_cast<OT*>(obj)->handleHiseEvent(*e);
		}

		static void prepare(void* obj, PrepareSpecs* ps)
		{
			static_cast<OT*>(obj)->prepare(*ps);
		}
	};

	struct Wrappers
	{
		template <typename OT, typename T> static void process(void* obj, T& d)
		{
			static_cast<OT*>(obj)->process(d);
		}

		template <typename OT, typename T> static void processFrame(void* obj, T& d)
		{
			static_cast<OT*>(obj)->processFrame(d);
		}

		template <typename OT, int MaxChannels> static Array<void*> createProcessWrappers()
		{
			Array<void*> f;

			f.add(process<OT, Types::ProcessData<jmin(1, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(2, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(3, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(4, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(5, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(6, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(7, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(8, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(9, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(10, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(11, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(12, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(13, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(14, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(15, MaxChannels)>>);
			f.add(process<OT, Types::ProcessData<jmin(16, MaxChannels)>>);

			return f;
		}

		template <typename OT, int MaxChannels> static Array<void*> createProcessFrameWrappers()
		{
			Array<void*> f;

			f.add((void*)processFrame<OT, Types::span<float, jmin(1, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(2, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(3, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(4, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(5, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(6, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(7, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(8, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(9, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(10, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(11, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(12, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(13, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(14, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(15, MaxChannels)>>);
			f.add((void*)processFrame<OT, Types::span<float, jmin(16, MaxChannels)>>);

			return f;
		}

		template <typename OT> static FunctionData* createResetFunction(const SnexTypeConstructData& cd)
		{
			FunctionData* r = new FunctionData();
			r->id = cd.id.getChildId("reset");
			r->returnType = TypeInfo(Types::ID::Void);
			r->function = &DefaultWrappers<OT>::reset;
			return r;
		}

		template <typename OT> static FunctionData* createPrepareFunction(const SnexTypeConstructData& cd)
		{
			FunctionData* ps = new FunctionData();
			ps->id = cd.id.getChildId("prepare");
			ps->returnType = TypeInfo(Types::ID::Void);
			ps->args.add({ ps->id.getChildId("specs"), TypeInfo(cd.c.getComplexType(NamespacedIdentifier("PrepareSpecs"))) });
			ps->function = &DefaultWrappers<OT>::prepare;
			return ps;
		}

		template <typename OT> static FunctionData* createHandleEventFunction(const SnexTypeConstructData& cd)
		{
			FunctionData* he = new FunctionData();
			he->id = cd.id.getChildId("handleHiseEvent");

			Symbol s(he->id.getChildId("e"), TypeInfo(cd.c.getComplexType(NamespacedIdentifier("HiseEvent")), false, true));
			he->args.add(s);

			he->returnType = TypeInfo(Types::ID::Void);
			he->function = &DefaultWrappers<OT>::handleHiseEvent;

			return he;
		}

		template <typename OT> static FunctionData* createProcessFrameFunction(const SnexTypeConstructData& cd)
		{
			FunctionData* pf = new FunctionData();
			pf->id = cd.id.getChildId("processFrame");
			pf->returnType = TypeInfo(Types::ID::Void);

			Symbol s(pf->id.getChildId("data"), createFrameType(cd));

			pf->args.add(s);
			
			auto list = createProcessFrameWrappers<OT, 16>();
			
			pf->function = list[cd.numChannels - 1];

			return pf;
		}

		template <typename OT> static FunctionData* createProcessFunction(const SnexTypeConstructData& cd)
		{
			jassertfalse;

			return nullptr;
#if 0
			FunctionData* p = new FunctionData();
			p->id = cd.id.getChildId("process");
			p->returnType = TypeInfo(Types::ID::Void);

			Array<TemplateParameter> tp;
			tp.add(TemplateParameter(cd.numChannels, TemplateParameter::Single));

			auto r = Result::ok();



			ComplexType::Ptr pType(cd.c.getNamespaceHandler().createTemplateInstantiation(NamespacedIdentifier("ProcessData"), tp, r));

			jassert(r.wasOk());

			
			p->args.add(Symbol(p->id.getChildId("data"), TypeInfo(pType, false, true)));

			auto list = createProcessWrappers<OT, 16>();

			p->function = list[cd.numChannels - 1];
			
			return p;
#endif
		}

		static TypeInfo createFrameType(const SnexTypeConstructData& cd);
	};

	void addParameterCallbacks(snex::jit::FunctionClass* fc)
	{
		auto l = getParameterList();

		for (int i = 0; i < l.size(); i++)
		{
			snex::jit::FunctionData* p = new FunctionData();
			p->id = fc->getClassName().getChildId("setParameter");
			p->templateParameters.add(TemplateParameter(i));
			p->args.add(Symbol(NamespacedIdentifier("value"), TypeInfo(Types::ID::Double)));
			p->returnType = TypeInfo(Types::ID::Void);
			p->function = l[i].function;
			fc->addFunction(p);
		}
	}

	virtual ParameterDataList getParameterList() = 0;

	snex::jit::FunctionClass* getFunctionClass() override
	{
		FunctionClass* fc = new FunctionClass(cd.id);

		fc->addFunction(functionCreator.prepareFunction(cd));
		fc->addFunction(functionCreator.resetFunction(cd));
		fc->addFunction(functionCreator.handleEventFunction(cd));
		fc->addFunction(functionCreator.processFunction(cd));
		fc->addFunction(functionCreator.processFrameFunction(cd));

		addParameterCallbacks(fc);

		return fc;
	};


	static Inliner::Ptr createInliner(const NamespacedIdentifier& id, const Array<void*>& functions);

	size_t getRequiredAlignment() const override { return 0; }

	virtual void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const {};

	InitialiserList::Ptr makeDefaultInitialiserList() const override { return new InitialiserList(); }

	virtual bool forEach(const TypeFunction& t, Ptr typePtr, void* dataPointer) { return false; };

	String toStringInternal() const override
	{
		return cd.id.toString();
	}

	SnexTypeConstructData cd;

	DefaultFunctionClass functionCreator;
};
#endif


}
}
