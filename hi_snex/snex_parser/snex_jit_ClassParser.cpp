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


ClassParser::ClassParser(BaseCompiler* c, const juce::String& code) :
	BlockParser(c, code.getCharPointer(), code.getCharPointer(), code.length())
{

}

ClassParser::ClassParser(BaseCompiler* c, const ParserHelpers::CodeLocation& l, int codeLength) :
	BlockParser(c, l.location, l.program, codeLength)
{

}

void ClassParser::registerTemplateArguments(TemplateParameter::List& templateList, const NamespacedIdentifier& scopeId)
{
	jassert(compiler->namespaceHandler.getCurrentNamespaceIdentifier() == scopeId);

	for (auto& tp : templateList)
	{
		jassert(tp.isTemplateArgument());
		jassert(tp.argumentId.isExplicit() || tp.argumentId.getParent() == scopeId);

		tp.argumentId = scopeId.getChildId(tp.argumentId.getIdentifier());

		jassert(tp.argumentId.getParent() == scopeId);

        
        NamespaceHandler::SymbolDebugInfo di;
        
		if (tp.t == TemplateParameter::TypeTemplateArgument)
		{
            
			compiler->namespaceHandler.addSymbol(tp.argumentId, tp.type, NamespaceHandler::TemplateType, di);
		}
		else
		{
			compiler->namespaceHandler.addSymbol(tp.argumentId, TypeInfo(Types::ID::Integer), NamespaceHandler::TemplateConstant, di);
		}
	}
}

snex::jit::BlockParser::StatementPtr ClassParser::addConstructorToComplexTypeDef(StatementPtr def, const Array<NamespacedIdentifier>& ids, bool matchSemicolon)
{
    // Do not add a constructor in a class definition
    if(matchSemicolon)
        match(JitTokens::semicolon);
    
	return def;
}

BlockParser::StatementPtr ClassParser::parseStatement(bool mustHaveSemicolon)
{
	while (matchIf(JitTokens::semicolon))
		;

	if (auto noop = parseVisibilityStatement())
		return noop;

	while (skipIfConsoleCall())
	{
		if (currentType == JitTokens::closeBrace)
			return new Operations::Noop(location);
	}

	if (matchIf(JitTokens::__internal_property))
	{
		match(JitTokens::openParen);

		if (currentValue.isString())
		{

			Identifier id(currentValue.toString());
			skip();

			match(JitTokens::comma);

			var v;

			if (matchIf(JitTokens::identifier))
			{
				v = currentValue;
			}
			else
			{
				auto value = parseConstExpression(false);
				v = value.toInt();
			}

			match(JitTokens::closeParen);

			return matchSemicolonAndReturn(new Operations::InternalProperty(location, id, v), true);
		}
	}

	if (matchIf(JitTokens::template_))
		templateArguments = parseTemplateParameters(true);
	else
		templateArguments = {};

	if (matchIf(JitTokens::namespace_))
	{
		CommentAttacher ca(*this);

		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, parseIdentifier());

		auto startPos = location.getXYPosition();

		match(JitTokens::openBrace);

		StatementPtr sb = new Operations::StatementBlock(location, compiler->namespaceHandler.getCurrentNamespaceIdentifier());

		while (currentType != JitTokens::eof && currentType != JitTokens::closeBrace)
		{
			CommentAttacher ca(*this);
			auto p = parseStatement(true);
			sb->addStatement(ca.withComment(p));
		}

		auto endPos = location.getXYPosition();
		match(JitTokens::closeBrace);

		compiler->namespaceHandler.setNamespacePosition(compiler->namespaceHandler.getCurrentNamespaceIdentifier(), startPos, endPos, ca.getInfo());



		return sb;
	}

	if (matchIf(JitTokens::using_))
	{
		parseUsingAlias();
		return new Operations::Noop(location);
	}

	if (matchIf(JitTokens::struct_))
	{
		return parseSubclass(NamespaceHandler::Visibility::Public);
	}

	if (matchIf(JitTokens::class_))
	{
		return parseSubclass(NamespaceHandler::Visibility::Private);
	}

	if (matchIf(JitTokens::destructor))
	{
		matchType(templateArguments);
		return parseComplexTypeDefinition(true);
	}

	if (matchIf(JitTokens::operator_))
	{
		matchType(templateArguments);
		return parseTypeCastOverload();
	}

	if (matchIfType(templateArguments))
	{
		if (currentTypeInfo.isComplexType())
			return parseComplexTypeDefinition();
		else
			return parseVariableDefinition();
	}

	location.throwError("Can't parse statement");
	return nullptr;
}


BlockParser::StatementPtr ClassParser::parseVariableDefinition()
{
	auto s = parseNewSymbol(NamespaceHandler::Variable);

	if (matchIf(JitTokens::openParen))
	{
		if (!compiler->namespaceHandler.changeSymbolType(s.id, NamespaceHandler::Function))
			location.throwError("Can't find function");

		auto st = parseFunction(s);
		return matchIfSemicolonAndReturn(st);
	}
	else
	{
		if (s.typeInfo.isStatic())
		{
			if (!s.typeInfo.isConst())
				location.throwError("Can't define non-const static variables");

			compiler->namespaceHandler.changeSymbolType(s.id, NamespaceHandler::Constant);

			match(JitTokens::assign_);

			auto v = parseConstExpression(false);
			compiler->namespaceHandler.addConstant(s.id, v);

			if (v.getType() == Types::ID::Integer)
			{
				return matchSemicolonAndReturn(new Operations::InternalProperty(location, s.id.getIdentifier(), v.toInt()), true);
			}
			else
			{
				return matchSemicolonAndReturn(new Operations::Noop(location), true);
			}
		}

		if (matchIf(JitTokens::assign_))
		{
			auto target = new Operations::VariableReference(location, s);

			ExprPtr expr;

			expr = new Operations::Immediate(location, parseConstExpression(false));

			return matchSemicolonAndReturn(new Operations::Assignment(location, target, JitTokens::assign_, expr, true), true);
		}

		if (!s.typeInfo.isTemplateType())
		{
			location.throwError("Expected initialiser for non-templated member");
			return nullptr;
		}
		else
		{
			return matchSemicolonAndReturn(new Operations::ComplexTypeDefinition(location, s.id, s.typeInfo), true);
		}
	}
}

BlockParser::StatementPtr ClassParser::parseFunction(const Symbol& s)
{
	using namespace Operations;

	FunctionDefinitionBase* func;

	bool isTemplateFunction = !templateArguments.isEmpty();

	if (isTemplateFunction)
		func = new TemplatedFunction(location, s, templateArguments);
	else
	{
		func = new Function(location, s);

	}

	StatementPtr newStatement = dynamic_cast<Statement*>(func);

	auto& fData = func->data;

	fData.id = func->data.id;
	fData.returnType = currentTypeInfo;
	fData.object = nullptr;
	CommentAttacher ca(*this);
	ca.withComment(fData);

	if (FunctionClass::isConstructor(fData.id))
	{
		fData.returnType = TypeInfo(Types::ID::Void);
	}

	jassert(compiler->namespaceHandler.getCurrentNamespaceIdentifier() == s.id.getParent());

	{
		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, s.id);

		if (isTemplateFunction)
		{
			registerTemplateArguments(as<TemplatedFunction>(newStatement)->templateParameters, s.id);
			templateArguments = as<TemplatedFunction>(newStatement)->templateParameters;
		}


		while (currentType != JitTokens::closeParen && currentType != JitTokens::eof)
		{
			matchType(templateArguments);

			auto s = parseNewSymbol(NamespaceHandler::Variable);
			fData.args.add(s);
			auto argumentId = s.id.id;

			func->parameters.add(argumentId);

			if (matchIf(JitTokens::assign_))
			{
				auto defaultExpression = parseExpression();

				if (defaultExpression->isConstExpr())
				{
					fData.setDefaultParameter(argumentId, defaultExpression->getConstExprValue());
				}
				else
				{
					fData.setDefaultParameter(argumentId, [defaultExpression](InlineData* b)
						{
							auto d = b->toSyntaxTreeData();
							d->args.add(defaultExpression->clone(d->location));
							return Result::ok();
						});
				}
			}

			matchIf(JitTokens::comma);
		}
	}

	if (isTemplateFunction)
	{
		TemplateObject f({ s.id, compiler->namespaceHandler.getCurrentTemplateParameters() });
		f.argList = as<TemplatedFunction>(newStatement)->templateParameters;
		f.makeFunction = std::bind(&TemplatedFunction::createFunction, as<TemplatedFunction>(newStatement), std::placeholders::_1);

		TypeInfo::List callParameters;

		for (auto& a : fData.args)
			callParameters.add(a.typeInfo);

		f.functionArgs = [callParameters]()
		{
			return callParameters;
		};

		compiler->namespaceHandler.addTemplateFunction(f);
	}
	else
	{
		compiler->namespaceHandler.addSymbol(s.id, s.typeInfo, NamespaceHandler::Function, ca.getInfo());
		compiler->namespaceHandler.setSymbolCode(s.id, fData.getCodeToInsert());
	}

	match(JitTokens::closeParen);

	

	bool isConstFunction = matchIf(JitTokens::const_);

	func->data.setConst(isConstFunction);

	func->code = location.location;

	skipIfConsoleCall();

	location.calculateLineIfEnabled(compiler->namespaceHandler.shouldCalculateNumbers());
	auto startPos = location.getXYPosition();

	match(JitTokens::openBrace);
	int numOpenBraces = 1;

	while (currentType != JitTokens::eof && numOpenBraces > 0)
	{
		if (currentType == JitTokens::openBrace) numOpenBraces++;
		if (currentType == JitTokens::closeBrace) numOpenBraces--;
		skip();
	}

	location.calculateLineIfEnabled(compiler->namespaceHandler.shouldCalculateNumbers());
	auto endPos = location.getXYPosition();

	compiler->namespaceHandler.setNamespacePosition(s.id, startPos, endPos, ca.getInfo());

	func->codeLength = static_cast<int>(location.location - func->code);



	return matchIfSemicolonAndReturn(newStatement);
}


BlockParser::StatementPtr ClassParser::parseSubclass(NamespaceHandler::Visibility defaultVisibility)
{
	NamespaceHandler::ScopedVisibilityState vs(compiler->namespaceHandler);

	auto startPos = location.getXYPosition();

	SymbolParser sp(*this, compiler->namespaceHandler);

	sp.parseNamespacedIdentifier<NamespaceResolver::MustBeNew>();

	auto classId = sp.currentNamespacedIdentifier;

	skipIfConsoleCall();

	Array<TemplateInstance> baseClasses;

	if (matchIf(JitTokens::colon))
	{
		while (!isEOF() && currentType != JitTokens::openBrace)
		{
			parseVisibility();

			SymbolParser sp(*this, compiler->namespaceHandler);

			sp.parseNamespacedIdentifier<NamespaceResolver::MustExist>();

			TemplateParameter::List tp;

			if (currentType == JitTokens::lessThan)
				tp = parseTemplateParameters(false);

			baseClasses.add({ sp.currentNamespacedIdentifier, tp });
			matchIf(JitTokens::comma);
		}
	}

	skipIfConsoleCall();

	if (templateArguments.isEmpty())
	{
		auto p = new StructType(classId, templateArguments);

		CommentAttacher ca(*this);

		compiler->namespaceHandler.addSymbol(classId, TypeInfo(p), NamespaceHandler::Struct, ca.getInfo());

		compiler->namespaceHandler.registerComplexTypeOrReturnExisting(p);

		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, classId);

		for (const auto& b : baseClasses)
			compiler->namespaceHandler.copySymbolsFromExistingNamespace(b.id);

		compiler->namespaceHandler.setVisiblity(defaultVisibility);

		auto list = parseStatementList();

		list->currentCompiler = compiler;

		skipIfConsoleCall();

		compiler->namespaceHandler.setNamespacePosition(classId, startPos, location.getXYPosition(), ca.getInfo());

		StatementPtr cs = new Operations::ClassStatement(location, p, list, baseClasses);

		Operations::as<Operations::ClassStatement>(cs)->createMembersAndFinalise();

		return matchSemicolonAndReturn(cs, true);
	}
	else
	{
		auto classTemplateArguments = templateArguments;



		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, classId);


		registerTemplateArguments(classTemplateArguments, classId);

		StatementPtr list;

		CommentAttacher ca(*this);

		{
			NamespaceHandler::ScopedVisibilityState vs(compiler->namespaceHandler);
			compiler->namespaceHandler.setVisiblity(defaultVisibility);
			list = ca.withComment(parseStatementList());
		}

		while (currentType == JitTokens::semicolon)
			match(JitTokens::semicolon);

		TemplateInstance cId(classId, compiler->namespaceHandler.getCurrentTemplateParameters());

		auto tcs = new Operations::TemplateDefinition(location, cId, compiler->namespaceHandler, list);

		TemplateObject tc(cId);
		tc.makeClassType = std::bind(&Operations::TemplateDefinition::createTemplate, tcs, std::placeholders::_1);
		tc.argList = classTemplateArguments;

		compiler->namespaceHandler.setNamespacePosition(classId, startPos, location.getXYPosition(), ca.getInfo());

		compiler->namespaceHandler.addTemplateClass(tc);
		return tcs;
	}
}

snex::jit::NamespaceHandler::Visibility ClassParser::parseVisibility()
{
	if (matchIf(JitTokens::public_))
		return NamespaceHandler::Visibility::Public;
	if (matchIf(JitTokens::private_))
		return NamespaceHandler::Visibility::Private;
	if (matchIf(JitTokens::protected_))
		return NamespaceHandler::Visibility::Protected;

	return NamespaceHandler::Visibility::numVisibilities;
}

ClassParser::StatementPtr ClassParser::parseVisibilityStatement()
{
	auto v = parseVisibility();

	if (v != NamespaceHandler::Visibility::numVisibilities)
	{
		compiler->namespaceHandler.setVisiblity(v);

		match(JitTokens::colon);
		return new Operations::Noop(location);
	}

	return nullptr;
}


snex::jit::BlockParser::StatementPtr ClassParser::parseTypeCastOverload()
{
	auto id = compiler->namespaceHandler.getCurrentNamespaceIdentifier().getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::NativeTypeCast));
	match(JitTokens::openParen);
	return parseFunction(Symbol(id, currentTypeInfo));
}

}
}
