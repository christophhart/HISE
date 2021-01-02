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

		static int handleModulation(void* obj, double* v)
		{
			auto t = static_cast<T*>(obj);
			return t->handleModulation(*v);
		}
	};

	LibraryNode(Compiler& c_, int numChannels_, const Identifier& factoryId_) :
		c(c_),
		numChannels(numChannels_),
		factoryId(NamespacedIdentifier(factoryId_))
	{
		createStructType();
		addParameterCallback();
		addConstructor();
		
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
			auto f = ScriptnodeCallbacks::getPrototype(c, callback, numChannels);
			st->injectInliner(f.id.getIdentifier(), type, func);
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

	void addModulationFunction(const Inliner::Func& highLevelInliner = {})
	{
		auto f = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::HandleModulation, numChannels);

		f.id = st->id.getChildId(f.id.getIdentifier());

		st->addJitCompiledMemberFunction(f);

		if (highLevelInliner)
			st->injectInliner(f.id.getIdentifier(), Inliner::HighLevel, highLevelInliner);

		st->injectMemberFunctionPointer(f, Wrapper::handleModulation);
	}
	
	void addMember(const Identifier& id, const TypeInfo& type, const VariableStorage& defaultValue)
	{
		st->addMember(id, type);
		st->setDefaultValue(id, InitialiserList::makeSingleList(defaultValue));

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

		auto prepaFunction = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::PrepareFunction, numChannels).withParent(id);
		auto eventFunction = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::HandleEventFunction, numChannels).withParent(id);
		auto resetFunction = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::ResetFunction, numChannels).withParent(id);

		st->addJitCompiledMemberFunction(prepaFunction);
		st->addJitCompiledMemberFunction(eventFunction);
		st->addJitCompiledMemberFunction(resetFunction);

		auto addProcessCallbacks = [&](int i)
		{
			auto pi = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::ProcessFunction, i).withParent(id);
			auto fi = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::ProcessFrameFunction, i).withParent(id);

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

		st->injectMemberFunctionPointer(prepaFunction, Wrapper::prepare);
		st->injectMemberFunctionPointer(eventFunction, Wrapper::handleHiseEvent);
		st->injectMemberFunctionPointer(resetFunction, Wrapper::reset);
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
			f.templateParameters.add(TemplateParameter(f.id.getChildId("P"), i, true, jit::TemplateParameter::Single));
			f.returnType = TypeInfo(Types::ID::Void);
			f.addArgs("value", TypeInfo(Types::ID::Double));

			st->addJitCompiledMemberFunction(f);
			f.function = p.dbNew.getFunction();
			st->injectMemberFunctionPointer(f, p.dbNew.getFunction());
		}
	}

	void addConstructor()
	{
		T object;

		FunctionData constructor;
		constructor.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, jit::FunctionClass::Constructor));

		constructor.returnType = TypeInfo(Types::ID::Void);

		st->addJitCompiledMemberFunction(constructor);
		st->injectMemberFunctionPointer(constructor, Wrapper::construct);
	}

	StructType* st;

	Compiler& c;
	int numChannels;
	NamespacedIdentifier factoryId;
};

juce::Result MathNodeLibrary::registerTypes()
{
	LibraryNode<math::add>(c, numChannels, getFactoryId());
	LibraryNode<math::mul>(c, numChannels, getFactoryId());
	LibraryNode<math::clear>(c, numChannels, getFactoryId());

#if 0 // make this when the polydata structure is there (ideally something like b.addPolyDataMember(const TypeInfo& elementType);
	b.injectInliner(ScriptnodeCallbacks::ProcessFrameFunction, Inliner::HighLevel, [](InlineData* b)
	{
		cppgen::Base c;

		c << "for(auto& s: data)";
		c << "    s += this->value.get();";
		
		return SyntaxTreeInlineParser(b, { "data" }, c).flush();
	});
#endif

	return Result::ok();
}

}

juce::Result CoreNodeLibrary::registerTypes()
{
	LibraryNode<core::oscillator>(c, numChannels, getFactoryId());
	LibraryNode<core::fix_delay>(c, numChannels, getFactoryId());
	LibraryNode<core::peak> p(c, numChannels, getFactoryId());
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

	return Result::ok();
}

juce::Result FxNodeLibrary::registerTypes()
{
	LibraryNode<fx::reverb>(c, numChannels, getFactoryId());
	LibraryNode<fx::phase_delay>(c, numChannels, getFactoryId());

	return Result::ok();
}

}
