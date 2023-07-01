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
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;


snex::jit::ComplexType::Ptr Operations::TemplateDefinition::createTemplate(const TemplateObject::ConstructData& d)
{
	auto instanceParameters = TemplateParameter::ListOps::merge(getTemplateArguments(), d.tp, *d.r);

	for (auto es : *this)
	{
		if (auto ecs = as<ClassStatement>(es))
		{
			auto tp = ecs->getStructType()->getTemplateInstanceParameters();

			if (TemplateParameter::ListOps::match(instanceParameters, tp))
			{
				return ecs->getStructType();
			}
		}
	}

	for (auto c : clones)
	{
		auto p = as<TemplateDefinition>(c)->createTemplate(d);
	}

	if (d.r->failed())
		throwError(d.r->getErrorMessage());

	TemplateParameter::List ipToUse;
	ipToUse.addArray(templateClassId.tp);
	ipToUse.addArray(instanceParameters);

	ComplexType::Ptr p = new StructType(templateClassId.id, ipToUse);

	p = handler.registerComplexTypeOrReturnExisting(p);

	Ptr cb = new SyntaxTree(location, as<ScopeStatementBase>(statements)->getPath());

	statements->cloneChildren(cb);

	auto c = new ClassStatement(location, p, cb, {});

	addStatement(c);

	c->forEachRecursive([ipToUse, c, d](Ptr p)
	{
		if (auto tf = as<TemplatedFunction>(p))
		{
			auto cId = dynamic_cast<StructType*>(c->classType.get())->getTemplateInstanceId();

			TemplateObject fo({ tf->data.id, ipToUse });
			fo.makeFunction = std::bind(&TemplatedFunction::createFunction, tf, std::placeholders::_1);
			fo.argList = tf->templateParameters;

			auto args = tf->data.args;

			fo.functionArgs = [args]()
			{
				TypeInfo::List l;

				for (auto a : args)
					l.add(a.typeInfo);

				return l;
			};

			d.handler->addTemplateFunction(fo);
		}

		return false;
	}, IterationType::AllChildStatements);

	c->createMembersAndFinalise();

	if (currentCompiler != nullptr)
	{
		TemplateParameterResolver resolver(instanceParameters);
		resolver.process(c);

		c->currentCompiler = currentCompiler;
		c->processAllPassesUpTo(currentPass, currentScope);
	}

	return d.handler->registerComplexTypeOrReturnExisting(p);
}

void Operations::TemplateDefinition::process(BaseCompiler* compiler, BaseScope* scope)
{
	auto cp = compiler->getCurrentPass();

	Statement::processBaseWithoutChildren(compiler, scope);

	if (cp == BaseCompiler::ComplexTypeParsing || cp == BaseCompiler::FunctionParsing)
	{
		for (auto c : *this)
		{
			auto tip = collectParametersFromParentClass(c, {});
			TemplateParameterResolver resolver(tip);
			auto r = resolver.process(c);

			if (!r.wasOk())
				throwError(r.getErrorMessage());
		}
	}

	Statement::processAllChildren(compiler, scope);
}

void Operations::TemplatedFunction::createFunction(const TemplateObject::ConstructData& d)
{
	Result r = Result::ok();
	auto instanceParameters = TemplateParameter::ListOps::merge(templateParameters, d.tp, r);
	location.test(r);

	//DBG("Creating template function " + d.id.toString() + TemplateParameter::ListOps::toString(templateParameters));

	if (currentCompiler != nullptr)
	{
		auto currentParameters = currentCompiler->namespaceHandler.getCurrentTemplateParameters();
		location.test(TemplateParameter::ListOps::expandIfVariadicParameters(instanceParameters, currentParameters));
		instanceParameters = TemplateParameter::ListOps::merge(templateParameters, instanceParameters, *d.r);
		//DBG("Resolved template parameters: " + TemplateParameter::ListOps::toString(instanceParameters));

		if (instanceParameters.size() < templateParameters.size())
		{
			// Shouldn't happen, the parseCall() method should have resolved the template parameters 
			// to another function already...
			jassertfalse;
		}
	}

	TemplateParameterResolver resolve(collectParametersFromParentClass(this, instanceParameters));

	for (auto e : *this)
	{
		if (auto ef = as<Function>(e))
		{
			auto fParameters = ef->data.templateParameters;

			if (TemplateParameter::ListOps::match(fParameters, instanceParameters))
				return;// ef->data;
		}
	}

	for (auto c : clones)
		as<TemplatedFunction>(c)->createFunction(d);

	FunctionData fData = data;

	resolve.resolveIds(fData);

	fData.templateParameters = instanceParameters;

	auto newF = new Function(location, {});
	newF->code = code;
	newF->codeLength = codeLength;
	newF->data = fData;
	newF->parameters = parameters;

	addStatement(newF);

	auto isInClass = findParentStatementOfType<ClassStatement>(this) != nullptr;

	auto ok = resolve.process(newF);

	if (isInClass)
	{

		location.test(ok);
	}


	if (currentCompiler != nullptr)
	{
		NamespaceHandler::ScopedTemplateParameterSetter stps(currentCompiler->namespaceHandler, instanceParameters);
		newF->currentCompiler = currentCompiler;
		newF->processAllPassesUpTo(currentPass, currentScope);
	}

	//dumpSyntaxTree(findParentStatementOfType<TemplateDefinition>(this));

	return;// fData;
}

snex::jit::Operations::Function* Operations::TemplatedFunction::getFunctionWithTemplateAmount(const NamespacedIdentifier& id, int numTemplateParameters)
{
	for (auto f_ : *this)
	{
		auto f = as<Function>(f_);

		if (id == f->data.id && f->data.templateParameters.size() == numTemplateParameters)
			return f;
	}

	// Now we'll have to look at the parent syntax tree
	SyntaxTreeWalker w(this);

	while (auto tf = w.getNextStatementOfType<TemplatedFunction>())
	{
		if (tf == this)
			continue;

		if (tf->data.id == id)
		{
			auto list = tf->collectFunctionInstances();

			for (auto f_ : list)
			{
				auto f = as<Function>(f_);

				if (id == f->data.id && f->data.templateParameters.size() == numTemplateParameters)
					return f;
			}
		}
	}

	return nullptr;
}

snex::jit::Operations::Statement::List Operations::TemplatedFunction::collectFunctionInstances()
{
	Statement::List orderedFunctions;
	auto id = data.id;

	for (auto f_ : *this)
	{
		auto f = as<Function>(f_);

		int numProvided = f->data.templateParameters.size();
		DBG("Scanning " + f->data.getSignature({}) + "for recursive function calls");

		f->statements->forEachRecursive([id, numProvided, this, &orderedFunctions](Ptr p)
		{
			if (auto fc = as<FunctionCall>(p))
			{
				if (fc->function.id == id)
				{
					DBG("Found recursive function call " + fc->function.getSignature({}));
					auto numThisF = fc->function.templateParameters.size();

					if (auto rf = getFunctionWithTemplateAmount(id, numThisF))
						orderedFunctions.addIfNotAlreadyThere(rf);
				}
			}

			return false;
		}, IterationType::AllChildStatements);

		orderedFunctions.addIfNotAlreadyThere(f);
	}

	return orderedFunctions;
}

void Operations::TemplatedFunction::process(BaseCompiler* compiler, BaseScope* scope)
{
	Statement::processBaseWithoutChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::FunctionCompilation)
	{
		if (TemplateParameter::ListOps::isVariadicList(templateParameters))
		{
			auto list = collectFunctionInstances();

			struct Sorter
			{
				static int compareElements(Ptr first, Ptr second)
				{
					auto f1 = as<Function>(first);
					auto f2 = as<Function>(second);
					auto s1 = f1->data.templateParameters.size();
					auto s2 = f2->data.templateParameters.size();

					if (s1 > s2)      return 1;
					else if (s1 < s2) return -1;
					else              return 0;
				}
			};

			Sorter s;
			list.sort(s);

			for (auto l : list)
				l->process(compiler, scope);

			return;
		}
	}

	Statement::processAllChildren(compiler, scope);
}

}
}