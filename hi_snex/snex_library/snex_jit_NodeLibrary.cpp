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


namespace snex {
namespace Types {
using namespace juce;
using namespace scriptnode;





template <class Node> struct LibraryNode
{
	using T = Node;

	struct Wrapper
	{
		static void prepare(void* obj, PrepareSpecs specs)
		{
			static_cast<T*>(obj)->prepare(specs);
		}

		static void reset(void* obj)
		{
			static_cast<T*>(obj)->reset();
		};

		template <int NumChannels> static void process(void* obj, ProcessData<NumChannels>& data)
		{
			static_cast<T*>(obj)->process(data);
		}

		template <int NumChannels> static void processFrame(void* obj, span<float, NumChannels>& data)
		{
			static_cast<T*>(obj)->processFrame(data);
		}

		static void handleHiseEvent(void* obj, HiseEvent& e)
		{
			static_cast<T*>(obj)->handleHiseEvent(e);
		}

		static void construct(void* target)
		{
			volatile T* dst = new (target)T();
			jassert(dst != nullptr);
		}

		static void destruct(void* target)
		{
			auto typed = reinterpret_cast<T*>(target);
			typed->~T();
		}

		static int handleModulation(void* obj, double* v)
		{
			auto t = static_cast<T*>(obj);
			return t->handleModulation(*v);
		}

		static void setExternalData(void* obj, ExternalData* d, int index)
		{
			auto t = static_cast<T*>(obj);
			t->setExternalData(*d, index);
		}
	};

	LibraryNode(Compiler& c_, int numChannels_, const Identifier& factoryId_) :
		c(c_),
		numChannels(numChannels_),
		factoryId(NamespacedIdentifier(factoryId_))
	{
		createStructType();
		addParameterCallback();
		addConstructorAndDestructor();
		
	};

	~LibraryNode()
	{
		setSizeFromObject();
		c.registerExternalComplexType(st);
	}

	void injectInliner(ScriptnodeCallbacks::ID callback, Inliner::InlineType type, const Inliner::Func& func)
	{
		if (c.allowInlining())
		{
			auto f = ScriptnodeCallbacks::getPrototype(&c, callback, numChannels);
			st->injectInliner(f.id.getIdentifier(), type, func);
		}
	}

	void injectInlinerForSetParameter(int index, Inliner::InlineType type, const Inliner::Func& func)
	{
		if (c.allowInlining())
		{
			TemplateParameter::List l;
			l.add(TemplateParameter(index));
			auto numInjected = st->injectInliner("setParameter", type, func, l);

			jassert(numInjected == 1);
		}
	}

	void setSizeFromObject()
	{
		if (!hasCustomMembers)
		{
			T object;
			st->setSizeFromObject(object);
		}
		
		st->finaliseAlignment();

		auto actualSize = sizeof(T);

		jassert(st->getRequiredByteSize() == actualSize);
	}

	void addSetExternalFunction()
	{
		auto f = ScriptnodeCallbacks::getPrototype(&c, ScriptnodeCallbacks::SetExternalDataFunction, numChannels);
		f.id = st->id.getChildId(f.id.getIdentifier());
		st->addJitCompiledMemberFunction(f);
		st->injectMemberFunctionPointer(f, (void*)Wrapper::setExternalData);
	}

	void addModulationFunction(const Inliner::Func& highLevelInliner = {})
	{
		auto f = ScriptnodeCallbacks::getPrototype(&c, ScriptnodeCallbacks::HandleModulation, numChannels);

		f.id = st->id.getChildId(f.id.getIdentifier());

		st->addJitCompiledMemberFunction(f);

		if (highLevelInliner)
			st->injectInliner(f.id.getIdentifier(), Inliner::HighLevel, highLevelInliner);

		st->injectMemberFunctionPointer(f, (void*)Wrapper::handleModulation);
	}
	
	void addMember(const Identifier& id, const TypeInfo& type, const VariableStorage& defaultValue)
	{
		st->addMember(id, type);
		st->setDefaultValue(id, InitialiserList::makeSingleList(defaultValue));

		hasCustomMembers = true;
	}

	void addPolyDataMember(const Identifier& id, const TypeInfo& elementType)
	{
		NamespacedIdentifier pd("PolyData");

		TemplateInstance tId(pd, {});

		auto isPolyphonic = c.getGlobalScope().getPolyHandler()->isEnabled();

		int numVoices = isPolyphonic ? NUM_POLYPHONIC_VOICES : 1;

		TemplateParameter::List p;
		p.add(TemplateParameter(elementType));
		p.add(TemplateParameter(numVoices));

		auto r = Result::ok();

		auto pst = c.getNamespaceHandler().createTemplateInstantiation(tId, p, r);

		st->addMember(id, TypeInfo(pst));

		hasCustomMembers = true;
	}

private:

	bool hasCustomMembers = false;

	void createStructType()
	{
		auto id = factoryId.getChildId(T::getStaticId());
		st = new StructType(id);

		st->setInternalProperty(WrapIds::IsNode, 1);
		st->setInternalProperty(WrapIds::GetSelfAsObject, true);
		st->setInternalProperty(WrapIds::NumChannels, numChannels);
		st->setInternalProperty(WrapIds::IsObjectWrapper, 0);

		auto prepaFunction = ScriptnodeCallbacks::getPrototype(&c, ScriptnodeCallbacks::PrepareFunction, numChannels).withParent(id);
		auto eventFunction = ScriptnodeCallbacks::getPrototype(&c, ScriptnodeCallbacks::HandleEventFunction, numChannels).withParent(id);
		auto resetFunction = ScriptnodeCallbacks::getPrototype(&c, ScriptnodeCallbacks::ResetFunction, numChannels).withParent(id);

		st->addJitCompiledMemberFunction(prepaFunction);
		st->addJitCompiledMemberFunction(eventFunction);
		st->addJitCompiledMemberFunction(resetFunction);

		auto addProcessCallbacks = [&](int i)
		{
			auto pi = ScriptnodeCallbacks::getPrototype(&c, ScriptnodeCallbacks::ProcessFunction, i).withParent(id);
			auto fi = ScriptnodeCallbacks::getPrototype(&c, ScriptnodeCallbacks::ProcessFrameFunction, i).withParent(id);

			st->addJitCompiledMemberFunction(pi);
			st->addJitCompiledMemberFunction(fi);

			if (i == 1)
			{
				pi.function = (void*)Wrapper::template process<1>;
				fi.function = (void*)Wrapper::template processFrame<1>;
			}

			if (i == 2)
			{
				pi.function = (void*)Wrapper::template process<2>;
				fi.function = (void*)Wrapper::template processFrame<2>;
			}

			st->injectMemberFunctionPointer(pi, pi.function);
			st->injectMemberFunctionPointer(fi, fi.function);
		};

		addProcessCallbacks(2);
		addProcessCallbacks(1);

		st->injectMemberFunctionPointer(prepaFunction, (void*)Wrapper::prepare);
		st->injectMemberFunctionPointer(eventFunction, (void*)Wrapper::handleHiseEvent);
		st->injectMemberFunctionPointer(resetFunction, (void*)Wrapper::reset);
	}

	void addParameterCallback()
	{
		T object;
		scriptnode::ParameterDataList list;
		object.createParameters(list);

		{
			TemplateObject to({ st->id.getChildId("setParameter"), st->getTemplateInstanceParameters() });
			to.argList.add(TemplateParameter(to.id.id.getChildId("P"), 0, false));
			to.functionArgs = []() {  return TypeInfo::List({ TypeInfo(Types::ID::Double) }); };
			to.makeFunction = [](const TemplateObject::ConstructData& cd) {};

			c.getNamespaceHandler().addTemplateFunction(to);
		}

		for (int i = 0; i < list.size(); i++)
		{
			auto p = list.getReference(i);

			FunctionData f;

			f.id = st->id.getChildId("setParameter");
			f.templateParameters.add(TemplateParameter(i));
			f.returnType = TypeInfo(Types::ID::Void);
			f.addArgs("value", TypeInfo(Types::ID::Double));

			st->addJitCompiledMemberFunction(f);
			f.function = (void*)p.callback.getFunction();
			st->injectMemberFunctionPointer(f, f.function);
		}
	}

	void addConstructorAndDestructor()
	{

		{
			FunctionData constructor;
			constructor.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, jit::FunctionClass::Constructor));

			constructor.returnType = TypeInfo(Types::ID::Void);

			st->addJitCompiledMemberFunction(constructor);
			st->injectMemberFunctionPointer(constructor, (void*)Wrapper::construct);
		}

		{
			FunctionData destructor;
			destructor.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, jit::FunctionClass::Destructor));
			destructor.returnType = TypeInfo(Types::ID::Void);

			st->addJitCompiledMemberFunction(destructor);
			st->injectMemberFunctionPointer(destructor, (void*)Wrapper::destruct);
		}
		

	}

	StructType* st;

	Compiler& c;
	int numChannels;
	NamespacedIdentifier factoryId;
};

juce::Result MathNodeLibrary::registerTypes()
{
	LibraryNode<math::add<1>> ma(c, numChannels, getFactoryId());
	ma.addPolyDataMember("value", Types::ID::Float);
	

	ma.injectInliner(ScriptnodeCallbacks::ProcessFrameFunction, Inliner::HighLevel, [](InlineData* b)
	{
		cppgen::Base c;

		c << "for(auto& s: data)";
		c << "    s += this->value.get();";

		return SyntaxTreeInlineParser(b, { "data" }, c).flush();
	});

	ma.injectInlinerForSetParameter(0, Inliner::HighLevel, [](InlineData* b)
	{
		cppgen::Base c;

		c << "for(auto& v: this->value)";
		c << "    v = (float)newValue;";

		return SyntaxTreeInlineParser(b, { "newValue" }, c).flush();
	});

	LibraryNode<math::mul<1>>(c, numChannels, getFactoryId());
	LibraryNode<math::clear<1>>(c, numChannels, getFactoryId());

	return Result::ok();
}

}

juce::Result CoreNodeLibrary::registerTypes()
{
#if 0
	LibraryNode<core::empty> e(c, numChannels, getFactoryId());

	

	e.injectInliner(ScriptnodeCallbacks::ProcessFrameFunction, Inliner::HighLevel, [](InlineData* b)
	{
		cppgen::Base c;
		return SyntaxTreeInlineParser(b, { "data" }, c).flush();
	});

	e.injectInliner(ScriptnodeCallbacks::ProcessFunction, Inliner::HighLevel, [](InlineData* b)
	{
		cppgen::Base c;
		return SyntaxTreeInlineParser(b, { "data" }, c).flush();
	});

	e.injectInliner(ScriptnodeCallbacks::ResetFunction, Inliner::HighLevel, [](InlineData* b)
	{
		cppgen::Base c;
		return SyntaxTreeInlineParser(b, { }, c).flush();
	});

	e.injectInliner(ScriptnodeCallbacks::HandleEventFunction, Inliner::HighLevel, [](InlineData* b)
	{
		cppgen::Base c;
		return SyntaxTreeInlineParser(b, { "e" }, c).flush();
	});

	LibraryNode<core::table> tb(c, numChannels, getFactoryId());
	tb.addSetExternalFunction();

	p.addMember("max", Types::ID::Double, 0.0);

	p.addModulationFunction([](InlineData* b)
	{
		cppgen::Base c;

		c << "value = max;";
		c << "return true;";

		return SyntaxTreeInlineParser(b, { "value" }, c).flush();
	});

	p.injectInliner(ScriptnodeCallbacks::ProcessFrameFunction, Inliner::HighLevel, [](InlineData* b)
	{
		cppgen::Base c;

		c << "this->max = 0.0;";
		c << "for (const auto& s : data)";
		c << "    this->max = Math.max(this->max, Math.abs((double)s));";

		return SyntaxTreeInlineParser(b, { "data" }, c).flush();
	});

#endif

	return Result::ok();
}

juce::Result FxNodeLibrary::registerTypes()
{
	

	return Result::ok();
}

}
