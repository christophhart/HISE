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

using namespace juce;
USE_ASMJIT_NAMESPACE;

namespace jit
{

ContainerNodeBuilder::ContainerNodeBuilder(Compiler& c, const Identifier& id, int numChannels_) :
	TemplateClassBuilder(c, NamespacedIdentifier("container").getChildId(id)),
	numChannels(numChannels_)
{
	

	addTypeTemplateParameter("ParameterClass");
	addVariadicTypeTemplateParameter("ProcessorTypes");
	addFunction(TemplateClassBuilder::VariadicHelpers::getFunction);

	setInitialiseStructFunction([](const TemplateObject::ConstructData& cd, StructType* st)
	{
		auto pType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 0);

		// Check that every element has DECLARE_NODE defined
		for (int i = 1; i < cd.tp.size(); i++)
		{
			auto type = cd.tp[i].type.getTypedComplexType<StructType>();

			if (!type->hasInternalProperty("IsNode"))
			{
				String s;
				s << type->toString() << " is not declared as node";

				*cd.r = Result::fail(s);
				return;
			}
		}

		// Check that the first element has NumChannels defined
		if (auto firstChainElement = cd.tp[1].type.getTypedComplexType<StructType>())
		{
			if (!firstChainElement->hasInternalProperty("NumChannels"))
			{
				String s;

				s << firstChainElement->toString();
				s << "::NumChannels is not defined";

				*cd.r = Result::fail(s);
				return;
			}

			st->setInternalProperty("NumChannels", firstChainElement->getInternalProperty("NumChannels", 0));
		}

		st->setInternalProperty(WrapIds::IsNode, 1);

		if (!ParameterBuilder::Helpers::isParameterClass(TypeInfo(pType)))
		{
			String s;
			s << "Expected parameter class at Index 0: " << pType->toString();
			*cd.r = Result::fail(s);
		}

		st->addMember("parameters", TypeInfo(pType, false, false));
	});

	addInitFunction(TemplateClassBuilder::VariadicHelpers::initVariadicMembers<1>);
	
	addFunction(Helpers::getParameterFunction);
	addFunction(Helpers::setParameterFunction);
	addFunction(Helpers::constructorFunction);

	addPostFunctionBuilderInitFunction(TemplateClassBuilder::Helpers::redirectProcessCallbacksToFixChannel);
	
	for (int i = 1; i < numChannels + 1; i++)
	{
		auto prototypes = Types::ScriptnodeCallbacks::getAllPrototypes(&c, i);

		for (auto& c : prototypes)
			callbacks.add(c);
	}

	for (auto& cf : callbacks)
		cf.inliner = Inliner::createHighLevelInliner(cf.id, Helpers::defaultForwardInliner);
}

void ContainerNodeBuilder::addHighLevelInliner(const Identifier& functionId, const Inliner::Func& inliner)
{
	jassert(isScriptnodeCallback(functionId));

	for (auto& cf : callbacks)
	{
		if (cf.id.id == functionId)
		{
			cf.inliner = Inliner::createHighLevelInliner(id.getChildId(functionId), inliner);
			break;
		}
	}
}

void ContainerNodeBuilder::addAsmInliner(const Identifier& functionId, const Inliner::Func& inliner)
{
	jassert(isScriptnodeCallback(functionId));

	for (auto& cf : callbacks)
	{
		if (cf.id.id == functionId)
		{
			cf.inliner = Inliner::createAsmInliner(id.getChildId(functionId), inliner);
			break;
		}
	}
}

void ContainerNodeBuilder::deactivateCallback(const Identifier& id)
{
	addHighLevelInliner(id, [](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			d->target = new Operations::Noop(d->location);
			return Result::ok();
		});
}

void ContainerNodeBuilder::flush()
{
	description << "\n#### Template Parameters:\n";
	description << "- **ParameterClass**: a class template (parameter::xxx) that defines the parameter connections\n";
	description << "- **ProcessorTypes**: a dynamic list of nodes that are processed.\n";

	/** TODO

		- fix get<0>() offset
		- make setParameter() && connect work.
		- add integer template parameter to connect
	*/

	jassert(numChannels != -1);

#if 0
	addFunction([](StructType* st)
		{
			FunctionData cData;
			cData.id = st->id.getChildId("connect");
			cData.returnType = TypeInfo(Types::ID::Void);
			cData.addArgs("obj", TypeInfo());
			cData.templateParameters.add(TemplateParameter(cData.id.getChildId("P"), 0, false));

			auto parameterClass = TemplateClassBuilder::Helpers::getStructTypeFromTemplate(st, 0);

			auto il = TemplateClassBuilder::Helpers::getFunctionFromTargetClass(parameterClass, "connect").inliner;

			cData.inliner = Inliner::createAsmInliner(cData.id, [il](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto st = Helpers::getStructTypeFromInlineData(b);
					auto pClass = TemplateClassBuilder::Helpers::getStructTypeFromTemplate(st, 0);

					AsmInlineData copy(*d);

					copy.templateParameters = {};

					return il->process(&copy);
				});

			return cData;
		});

	addFunction([](StructType* st)
		{
			FunctionData pFunc;
			pFunc.id = st->id.getChildId("setParameter");
			pFunc.returnType = TypeInfo(Types::ID::Void);
			pFunc.addArgs("value", TypeInfo(Types::ID::Double));
			pFunc.templateParameters.add(TemplateParameter(pFunc.id.getChildId("P"), 0, false));

			auto parameterClass = TemplateClassBuilder::Helpers::getStructTypeFromTemplate(st, 0);
			auto il = TemplateClassBuilder::Helpers::getFunctionFromTargetClass(parameterClass, "setParameter").inliner;

			pFunc.inliner = Inliner::createHighLevelInliner(pFunc.id, [il](InlineData* b)
				{

					jassertfalse;
					return Result::ok();
				});

			return pFunc;
		});
#endif

	
	for (auto cf : callbacks)
	{
		addFunction([cf](StructType* st)
		{
			return cf;
		});
	}


	TemplateClassBuilder::flush();
}

bool ContainerNodeBuilder::isScriptnodeCallback(const Identifier& id) const
{
	for (auto f : callbacks)
	{
		if (f.id.id == id)
			return true;
	}

	return false;
}

snex::jit::FunctionData ContainerNodeBuilder::Helpers::constructorFunction(StructType* st)
{
	if (st->hasConstructor())
	{
		FunctionData f;
		f.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, FunctionClass::Constructor));
		f.returnType = TypeInfo(Types::ID::Void);
		f.inliner = Inliner::createHighLevelInliner(f.id, defaultForwardInliner);

		return f;
	}
	else
	{
		return {};
	}
}

juce::Result ContainerNodeBuilder::Helpers::defaultForwardInliner(InlineData* b)
{
	auto d = b->toSyntaxTreeData();
	auto st = TemplateClassBuilder::Helpers::getStructTypeFromInlineData(b);
	auto id = getFunctionIdFromInlineData(b);

	constexpr int ParameterOffset = 1;

	d->target = TemplateClassBuilder::VariadicHelpers::callEachMember(d, st, id, ParameterOffset);
	return Result::ok();
}

juce::Identifier ContainerNodeBuilder::Helpers::getFunctionIdFromInlineData(InlineData* b)
{
	auto d = b->toSyntaxTreeData();
	return Operations::as<Operations::FunctionCall>(d->expression)->function.id.id;
}

snex::jit::FunctionData ContainerNodeBuilder::Helpers::getParameterFunction(StructType* st)
{
	FunctionData gf;
	gf.id = st->id.getChildId("getParameter");
	gf.returnType = TypeInfo(Types::ID::Dynamic);
	gf.templateParameters.add(TemplateParameter(gf.id.getChildId("Index"), 0, false));

	gf.inliner = Inliner::createHighLevelInliner(gf.id, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();

			int index;
			StructType* parameterType;

			ParameterBuilder::Helpers::forwardToListElements(st, d->templateParameters, &parameterType, index);

			int offset = ParameterBuilder::Helpers::getParameterListOffset(st, index);

			jassert(ParameterBuilder::Helpers::isParameterClass(TypeInfo(parameterType, false, true)));



			d->target = new Operations::MemoryReference(d->location, d->object, TypeInfo(parameterType, false, true), offset);

			return Result::ok();
		});

	gf.inliner->returnTypeFunction = [st](InlineData* b)
	{
		auto rt = dynamic_cast<ReturnTypeInlineData*>(b);

		int index;
		StructType* parameterType;

		ParameterBuilder::Helpers::forwardToListElements(st, rt->templateParameters, &parameterType, index);

		jassert(ParameterBuilder::Helpers::isParameterClass(TypeInfo(parameterType, false, true)));

		rt->f.returnType = TypeInfo(parameterType, false, true);
		return Result::ok();
	};

	return gf;
}

snex::jit::FunctionData ContainerNodeBuilder::Helpers::setParameterFunction(StructType* st)
{
	FunctionData sf;
	sf.id = st->id.getChildId("setParameter");
	sf.returnType = TypeInfo(Types::ID::Void, false, false);
	sf.addArgs("value", TypeInfo(Types::ID::Double));
	sf.templateParameters.add(TemplateParameter(sf.id.getChildId("P"), 0, false));

	sf.inliner = Inliner::createHighLevelInliner(sf.id, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();

			int index;
			StructType* parameterType;

			ParameterBuilder::Helpers::forwardToListElements(st, d->templateParameters, &parameterType, index);

			int offset = ParameterBuilder::Helpers::getParameterListOffset(st, index);

			jassert(ParameterBuilder::Helpers::isParameterClass(TypeInfo(parameterType, false, true)));

			auto newCall = TemplateClassBuilder::Helpers::createFunctionCall(parameterType, d, "call", d->args);

			auto obj = new Operations::MemoryReference(d->location, d->object, TypeInfo(parameterType, false, true), offset);

			dynamic_cast<Operations::FunctionCall*>(newCall.get())->setObjectExpression(obj);

			d->target = newCall;

			return Result::ok();
		});

	return sf;
}


}

namespace Types {
Result ContainerLibraryBuilder::registerTypes()
{
	ContainerNodeBuilder chain(c, "chain", numChannels);
	chain.setDescription("Processes all nodes serially");

	chain.flush();

	ContainerNodeBuilder split(c, "split", numChannels);
	split.setDescription("Copies the signal, processes all nodes parallel and sums up the processed signal at the end");
	split.flush();

	return Result::ok();
}

}

}