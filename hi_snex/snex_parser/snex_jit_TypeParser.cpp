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

TypeParser::TypeParser(TokenIterator& other_, NamespaceHandler& handler, const TemplateParameter::List& tp) :
	ParserHelpers::TokenIterator(other_),
	namespaceHandler(handler),
	other(other_),
	previouslyParsedArguments(tp),
	nId(NamespacedIdentifier::null())
{

}

void TypeParser::matchType()
{
	if (!matchIfType())
		throwTokenMismatch("Type");
}

bool TypeParser::matchIfType()
{
	auto isStatic = matchIf(JitTokens::static_);
	auto isConst = matchIf(JitTokens::const_);

	if (matchIfTypeInternal())
	{
		auto isRef = matchIf(JitTokens::bitwiseAnd);
		currentTypeInfo = currentTypeInfo.withModifiers(isConst, isRef, isStatic);

		if (auto st = currentTypeInfo.getTypedIfComplexType<StructType>())
			location.test(namespaceHandler.checkVisiblity(st->id));

		other.seek(*this);
		return true;
	}

	return false;
}

juce::Array<snex::jit::TemplateParameter> TypeParser::parseTemplateParameters()
{
	Array<TemplateParameter> parameters;

	match(JitTokens::lessThan);

	while (currentType != JitTokens::greaterThan && !isEOF())
	{
		TypeParser tp(*this, namespaceHandler, {});

		if (tp.matchIfType())
		{


			TemplateParameter p(tp.currentTypeInfo);


			parameters.add(p);
		}
		else
		{
			if (currentType == JitTokens::identifier)
			{
				auto cId = namespaceHandler.getCurrentNamespaceIdentifier().getChildId(Identifier(currentValue.toString()));

				if (namespaceHandler.isTemplateConstantArgument(cId))
				{
					TypeInfo tti(cId);

					TemplateParameter tp(tti);
					parameters.add(tp);

					match(JitTokens::identifier);
					matchIf(JitTokens::comma);
					continue;
				}

			}

			auto e = parseConstExpression(true);

			if (e.getType() != Types::ID::Integer)
				location.throwError("Can't use non-integers as template argument");

			TemplateParameter tp(e.toInt());

			parameters.add(tp);
		}

		matchIf(JitTokens::comma);
	}

	match(JitTokens::greaterThan);

	return parameters;
}

snex::VariableStorage TypeParser::parseConstExpression(bool canBeTemplateParameter)
{
	if (currentType == JitTokens::identifier)
	{
		SymbolParser sp(*this, namespaceHandler);
		auto id = sp.parseExistingSymbol(true);
		return namespaceHandler.getConstantValue(id.id);
	}

	return parseVariableStorageLiteral();
}

bool TypeParser::parseNamespacedIdentifier()
{
	if (currentType == JitTokens::identifier)
	{
		SymbolParser p(*this, namespaceHandler);
		p.parseNamespacedIdentifier<NamespaceResolver::CanExist>();
		nId = p.currentNamespacedIdentifier;
		return true;
	}

	return false;
}

void TypeParser::parseSubType()
{
	while (matchIf(JitTokens::double_colon))
	{
		auto parentType = currentTypeInfo;

		if (!parseNamespacedIdentifier())
			location.throwError("funky");

		SubTypeConstructData sd;
		sd.id = nId;
		sd.handler = &namespaceHandler;

		if (currentType == JitTokens::lessThan)
		{
			sd.l = parseTemplateParameters();
		}

		if (auto subType = parentType.getComplexType()->createSubType(&sd))
		{
			currentTypeInfo = TypeInfo(subType, parentType.isConst(), parentType.isRef());
		}
	}
}

bool TypeParser::matchIfTypeInternal()
{
	if (parseNamespacedIdentifier())
	{
		for (const auto& p : previouslyParsedArguments)
		{
			if (p.argumentId == nId)
			{
				if (p.t == TemplateParameter::TypeTemplateArgument)
				{
					currentTypeInfo = TypeInfo(nId);
					return true;
				}
			}
		}

		// Might be a second constructor
		if (nId.getParent().id == nId.id)
		{
			if (auto constructorType = namespaceHandler.getComplexType(nId.getParent()))
			{
				currentTypeInfo = TypeInfo(constructorType);
				return true;
			}
		}

		if (namespaceHandler.isTemplateTypeArgument(nId))
		{
			currentTypeInfo = TypeInfo(nId);
			return true;
		}

		auto t = namespaceHandler.getAliasType(nId);

		if (namespaceHandler.isTemplateClassId(nId))
		{
			auto tp = parseTemplateParameters();

			Result r = Result::ok();

			auto s = namespaceHandler.createTemplateInstantiation({ nId, {} }, tp, r);
			location.test(r);

			currentTypeInfo = TypeInfo(s);
			parseSubType();
			return true;
		}

		if (auto typePtr = namespaceHandler.getComplexType(nId))
		{
			currentTypeInfo = TypeInfo(typePtr);
			parseSubType();
			return true;
		}

		if (t.isValid())
		{
			currentTypeInfo = t;
			return true;
		}

		auto p = nId;

		while (p.isValid())
		{
			auto id = NamespacedIdentifier(p.getIdentifier());
			p = p.getParent();
			t = namespaceHandler.getAliasType(p);

			if (t.isValid() && t.isComplexType())
			{
				SubTypeConstructData sd;
				sd.id = id;
				sd.handler = &namespaceHandler;

				if (auto st = t.getComplexType()->createSubType(&sd))
				{
					currentTypeInfo = TypeInfo(st, currentTypeInfo.isConst(), currentTypeInfo.isRef());
					return true;
				}
				else
					return false;
			}
		}
	}

	if (matchIfSimpleType())
		return true;
	else if (matchIfComplexType())
	{
		parseSubType();
		return true;
	}

	return false;
}

bool TypeParser::matchIfSimpleType()
{
	Types::ID t;

	if (matchIf(JitTokens::float_))		  t = Types::ID::Float;
	else if (matchIf(JitTokens::int_))	  t = Types::ID::Integer;
	else if (matchIf(JitTokens::bool_))   t = Types::ID::Integer;
	else if (matchIf(JitTokens::double_)) t = Types::ID::Double;
	else if (matchIf(JitTokens::void_))	  t = Types::ID::Void;
	else if (matchIf(JitTokens::auto_))	  t = Types::ID::Dynamic;
	else
	{
		currentTypeInfo = {};
		return false;
	}

	currentTypeInfo = TypeInfo(t);
	return true;
}

bool TypeParser::matchIfComplexType()
{
	if (nId.isValid())
	{

	}

	return false;
}

snex::jit::ComplexType::Ptr TypeParser::parseComplexType(const juce::String& token)
{
	jassertfalse;
	return nullptr;
}

snex::Types::ID TypeParser::matchTypeId()
{
	if (matchIf(JitTokens::bool_))   return Types::ID::Integer;
	if (matchIf(JitTokens::float_))  return Types::ID::Float;
	if (matchIf(JitTokens::int_))	 return Types::ID::Integer;
	if (matchIf(JitTokens::double_)) return Types::ID::Double;
	if (matchIf(JitTokens::void_))	 return Types::ID::Void;
	if (matchIf(JitTokens::auto_))   return Types::ID::Dynamic;

	throwTokenMismatch("Type");

	RETURN_DEBUG_ONLY(Types::ID::Void);
}

ExpressionTypeParser::ExpressionTypeParser(NamespaceHandler& n, const String& statement, int lineNumber_) :
	TokenIterator(statement),
	nh(n),
	lineNumber(lineNumber_)
{

}

snex::jit::TypeInfo ExpressionTypeParser::parseType()
{
	try
	{
		auto id = parseIdentifier();
		currentId = NamespacedIdentifier(id);

		if (auto ns = nh.getNamespaceForLineNumber(lineNumber))
		{
			currentId = ns->id.getChildId(id);

			nh.switchToExistingNamespace(ns->id);
			nh.resolve(currentId);
		}

		auto ct = nh.getVariableType(currentId);
		return parseDot(ct);
	}
	catch (ParserHelpers::Error& )
	{
		return TypeInfo();
	}
}

snex::jit::TypeInfo ExpressionTypeParser::parseDot(TypeInfo parent)
{
	if (parent == TypeInfo())
		return {};

	if (matchIf(JitTokens::dot))
	{
		if (auto st = parent.getTypedIfComplexType<StructType>())
		{
			currentId = NamespacedIdentifier(parseIdentifier());

			if (st->hasMember(currentId.id))
				return parseDot(st->getMemberTypeInfo(currentId.id));
			else
			{
				FunctionClass::Ptr fc = st->getFunctionClass();

				auto fData = fc->getNonOverloadedFunctionRaw(st->id.getChildId(currentId.id));

				return fData.returnType;
			}

		}

		location.throwError("illegal dot operator");
	}

	return parseSubscript(parent);
}

snex::jit::TypeInfo ExpressionTypeParser::parseSubscript(TypeInfo parent)
{
	if (matchIf(JitTokens::openBracket))
	{
		if (auto at = parent.getTypedIfComplexType<jit::ArrayTypeBase>())
		{
			while (!isEOF() && currentType != JitTokens::closeBracket)
				skip();

			match(JitTokens::closeBracket);

			auto t = at->getElementType();
			return parseDot(t);
		}

		location.throwError("Not an array");
	}

	return parseCall(parent);
}

snex::jit::TypeInfo ExpressionTypeParser::parseCall(TypeInfo parent)
{
	if (matchIf(JitTokens::openParen))
	{
		while (!isEOF() && currentType != JitTokens::closeParen)
			skip();

		match(JitTokens::closeParen);
		FunctionClass::Ptr fc = parent.getComplexType()->getFunctionClass();
		auto fData = fc->getNonOverloadedFunction(NamespacedIdentifier(currentId));
		return parseDot(fData.returnType);
	}

	return parent;
}

}
}