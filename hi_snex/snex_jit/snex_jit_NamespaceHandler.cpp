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
using namespace asmjit;

bool NamespaceHandler::Namespace::contains(const NamespacedIdentifier& symbol) const
{
	if (symbol.getParent() == id)
	{
		for (auto a : aliases)
		{
			if (symbol == a.id)
				return true;
		}
	}

	return false;
}

juce::String NamespaceHandler::Namespace::dump(int level)
{
	juce::String s;

	if (internalSymbol)
		return s;

	auto idName = id.isValid() ? id.toString() : "root";

	s << getIntendLevel(level) << "namespace " << idName << "\n";

	level++;

	for (auto u : usedNamespaces)
	{
		s << getIntendLevel(level) << "using " << u->id.toString() << "\n";
	}

	for (auto a : aliases)
	{
		if (a.internalSymbol)
			continue;

		s << getIntendLevel(level);
		s << a.toString() << "\n";
	}

	for (auto c : childNamespaces)
	{
		s << c->dump(level);
	};

	return s;
}

juce::String NamespaceHandler::Namespace::getIntendLevel(int level)
{
	juce::String s;

	for (int i = 0; i < level; i++)
		s << "  ";

	return s;
}

void NamespaceHandler::Namespace::addSymbol(const NamespacedIdentifier& aliasId, const TypeInfo& type, SymbolType symbolType, Visibility v)
{
	jassert(aliasId.getParent() == id);

	if (contains(aliasId))
		return;

	aliases.add({ aliasId, type, v, symbolType });
	aliases.getReference(aliases.size() - 1).internalSymbol = internalSymbol;
}

juce::String NamespaceHandler::Alias::toString() const
{
	juce::String s;

	switch (visibility)
	{
	case Visibility::Public:    s << "public "; break;
	case Visibility::Private:   s << "private "; break;
	case Visibility::Protected: s << "protected "; break;
	}

	switch (symbolType)
	{
	case Struct:   s << "struct " << id.toString(); break;
	case Function: s << "function " << id.toString() << "\n"; break;
	case Variable: s << type.toString() << " " << id.toString(); break;
	case UsingAlias:	   s << "using " << id.toString() << " = " << type.toString(); break;
	case Constant: s << "static " << type.toString() << " " << id.toString() << " = " << Types::Helpers::getCppValueString(constantValue); break;
	case StaticFunctionClass: s << "Function class " << id.toString(); break;
	case TemplateType: s << "typename " << id.toString() << (!type.isDynamic() ? (" " + type.toString()) : ""); break;
	case TemplateConstant: s << "template int " << id.toString(); break;
	case TemplatedClass: s << "template struct " << id.toString(); break;
	case TemplatedFunction: s << "template function " << id.toString(); break;
	case PreprocessorConstant: s << "#define " << id.toString() << "=" << Types::Helpers::getCppValueString(constantValue); break;
	case Enum:				   s << "enum " << id.toString(); break;
	case EnumValue:			   s << id.toString() << " = " << String(constantValue.toInt()); break;
	default:
		jassertfalse;
	}

	return s;
}

snex::jit::ComplexType::Ptr NamespaceHandler::registerComplexTypeOrReturnExisting(ComplexType::Ptr ptr)
{
	if (ptr == nullptr)
		return nullptr;

	for (auto c : complexTypes)
	{
		if (c->matchesOtherType(*ptr))
			return c;
	}

	if (currentNamespace == nullptr)
		pushNamespace(Identifier());

	ptr->registerExternalAtNamespaceHandler(this);
	
	complexTypes.add(ptr);
	return ptr;
}

snex::jit::ComplexType::Ptr NamespaceHandler::getComplexType(NamespacedIdentifier id)
{
	resolve(id, true);

	for (auto c : complexTypes)
	{
		if (c->matchesId(id))
			return c;
	}

	return nullptr;
}

bool NamespaceHandler::changeSymbolType(NamespacedIdentifier id, SymbolType newType)
{
	resolve(id, false);

	if (auto e = get(id.getParent()))
	{
		for (auto& a : e->aliases)
		{
			if (a.id == id)
			{
				a.symbolType = newType;
				return true;
			}
		}
	}

	return false;
}

void NamespaceHandler::pushNamespace(const NamespacedIdentifier& namespaceId)
{
	if (auto e = get(namespaceId))
	{
		currentNamespace = e;
		currentParent = e->parent;
		return;
	}

	if (namespaceId.isExplicit())
	{
		pushNamespace(namespaceId.getIdentifier());
		return;
	}
		
	jassert(!namespaceId.isExplicit());

	auto p = namespaceId.getParent();

	pushNamespace(p);
	pushNamespace(namespaceId.getIdentifier());
}

void NamespaceHandler::pushNamespace(const Identifier& id)
{
	if (currentNamespace == nullptr)
	{
		jassert(!id.isValid());

		currentNamespace = new Namespace();
		currentNamespace->id = NamespacedIdentifier();
		existingNamespace.add(currentNamespace);
		return;
	}

	if (!id.isValid())
	{
		currentNamespace = getRoot();
		return;
	}

	jassert(id.isValid());

	auto newId = currentNamespace->id.getChildId(id);
	currentParent = currentNamespace;

	if (auto existing = get(newId))
	{
		currentNamespace = existing;
	}
	else
	{
		currentNamespace = new Namespace();
		currentNamespace->id = newId;
		currentNamespace->internalSymbol = internalSymbolMode;
		currentNamespace->parent = currentParent;
		existingNamespace.add(currentNamespace);

		currentParent->childNamespaces.add(currentNamespace);
	}

	jassert(currentParent->childNamespaces.contains(currentNamespace));
}

snex::jit::NamespacedIdentifier NamespaceHandler::getRootId() const
{
	return getRoot()->id;
}

bool NamespaceHandler::isNamespace(const NamespacedIdentifier& possibleNamespace) const
{
	return get(possibleNamespace) != nullptr;
}

juce::Result NamespaceHandler::popNamespace()
{
	if (currentNamespace == getRoot())
		return Result::ok();

	if (currentParent == nullptr)
		return Result::fail("Can't pop namespace");

	currentNamespace = currentNamespace->parent;

	return Result::ok();
}

snex::jit::NamespacedIdentifier NamespaceHandler::getCurrentNamespaceIdentifier() const
{
	if (currentNamespace == nullptr)
		return {};

	return currentNamespace->id;
}

juce::String NamespaceHandler::dump()
{
	return getRoot()->dump(0);
}

juce::Result NamespaceHandler::addUsedNamespace(const NamespacedIdentifier& usedNamespace)
{
	if (auto existing = get(usedNamespace))
	{
		currentNamespace->usedNamespaces.add(existing);
		return Result::ok();
	}
	else
		return Result::fail("Can't find namespace " + usedNamespace.toString());
}

juce::Result NamespaceHandler::resolve(NamespacedIdentifier& id, bool allowZeroMatch /*= false*/) const
{
	if (skipResolving)
		return Result::ok();

	if (currentNamespace == nullptr)
		return Result::fail("no namespace available");

	if (currentNamespace->contains(id))
	{
		return Result::ok();
	}

	auto name = id.getIdentifier();

	

	auto isExplicitId = id.getParent().isValid() && !(id.getParent() == currentNamespace->id);

	if (!isExplicitId)
	{
		Namespace::WeakPtr p = currentNamespace->parent;

		while (p != nullptr)
		{
			auto parentId = p->id.getChildId(name);

			if (p->contains(parentId))
			{
				id = parentId;
				return Result::ok();
			}

			p = p->parent;
		}
	}

	auto parent = id.getParent();
	Array<NamespacedIdentifier> matches;

	auto existing = get(parent);

	if (existing == nullptr)
	{
		auto subParent = parent.relocate({}, currentNamespace->id);

		existing = get(subParent);

		ScopedValueSetter<bool> svs(skipResolving, true);

		if (isTemplateTypeArgument(subParent))
		{
			for (const auto& tp : getCurrentTemplateParameters())
			{
				if (tp.argumentId == subParent)
				{
					auto actualParent = tp.type.getTypedComplexType<StructType>()->id;
					
					existing = get(actualParent);
					break;
				}
			}
		}
	}

	if (existing != nullptr)
	{
		auto possibleSymbol = existing->id.getChildId(name);

		if (existing->contains(possibleSymbol))
			matches.add(possibleSymbol);
		else
		{
			for (auto un : existing->usedNamespaces)
			{
				possibleSymbol = un->id.getChildId(name);

				if (un->contains(possibleSymbol))
					matches.add(possibleSymbol);
			}
		}
	}

	if (!allowZeroMatch && matches.size() == 0)
		return Result::fail(name.toString() + " can't be resolved");

	if (matches.size() > 1)
		return Result::fail(name.toString() + " is ambiguous");

	if (matches.size() == 1)
		id = matches.getFirst();

	return Result::ok();
}

void NamespaceHandler::addSymbol(const NamespacedIdentifier& id, const TypeInfo& t, SymbolType symbolType)
{
	jassert(id.getParent() == currentNamespace->id);

	currentNamespace->internalSymbol = internalSymbolMode;
	currentNamespace->addSymbol(id, t, symbolType, currentVisibility);
}

Result NamespaceHandler::addConstant(const NamespacedIdentifier& id, const VariableStorage& v)
{
	for (auto& a : currentNamespace->aliases)
	{
		if (a.id == id)
		{
			if (a.type == v.getType())
				a.constantValue = v;
			else
				a.constantValue = VariableStorage(a.type.getType(), (double)v);
			
			return Result::ok();
		}
	}

	return Result::fail("fail");
}

juce::Result NamespaceHandler::setTypeInfo(const NamespacedIdentifier& id, SymbolType expectedType, const TypeInfo& t)
{
	if (auto p = get(id.getParent()))
	{
		for (auto& a : p->aliases)
		{
			if (a.id == id)
			{
				if (a.symbolType == expectedType)
				{
					a.type = t;
					return Result::ok();
				}
				else
					return Result::fail("Symbol type mismatch");
			}
		}

		return Result::fail("Can't find symbol");
	}

	return Result::fail("Can't find namespace");
}



snex::jit::NamespaceHandler::Namespace::WeakPtr NamespaceHandler::getNamespaceForLineNumber(int lineNumber) const
{
	if (auto root = getRoot())
	{
		return root->getNamespaceForLineNumber(lineNumber);
	}
}

mcl::TokenCollection::List NamespaceHandler::getTokenList()
{
	using namespace mcl;

	TokenCollection::List l;

	for (auto n : existingNamespace)
	{
		Range<int> tokenScope = n->lines;

		Alias a;
		a.id = n->id;
		a.symbolType = SymbolType::NamespacePlaceholder;
		
		l.add(new SymbolToken(this, n, a));

		for (auto a : n->aliases)
		{
			TokenCollection::TokenPtr p = new SymbolToken(this, n, a);

			bool found = false;

			if(!found)
				l.add(p);
		}
	}

	for (int i = 0; i < l.size(); i++)
	{
		if (l[i]->tokenContent.isEmpty())
		{
			l.remove(i--);
			continue;
		}

		for (int j = i+1; j < l.size(); j++)
		{
			if (*l[j] == *l[i])
			{
				auto s = l[j]->tokenContent;
				l.remove(j--);
			}
		}
	}

	return l;
}


String NamespaceHandler::getDescriptionForItem(const NamespacedIdentifier& n) const
{
	if (auto existing = get(n.getParent()))
	{
		for (auto a : existing->aliases)
		{
			if (a.id == n)
				return a.toString();
		}
	}

	return {};
}

bool NamespaceHandler::isConstantSymbol(SymbolType t)
{
	return t == TemplateConstant || t == PreprocessorConstant || t == Constant || t == EnumValue;
}

bool NamespaceHandler::isTemplateTypeArgument(NamespacedIdentifier classId) const
{
	resolve(classId);

	if (auto p = get(classId.getParent()))
	{
		for (const auto& a : p->aliases)
		{
			if (a.id == classId)
				return a.symbolType == SymbolType::TemplateType;
		}
	}

	return false;
}

bool NamespaceHandler::isTemplateConstantArgument(NamespacedIdentifier classId) const
{
	resolve(classId);

	if (auto p = get(classId.getParent()))
	{
		for (const auto& a : p->aliases)
		{
			if (a.id == classId)
				return a.symbolType == SymbolType::TemplateConstant;
		}
	}

	return false;
}

bool NamespaceHandler::isTemplateFunction(NamespacedIdentifier functionId) const
{
	resolve(functionId, true);

	for (auto& t : templateFunctionIds)
	{
		if (t.id == functionId)
			return true;
	}

	return false;
}

bool NamespaceHandler::isTemplateClass(NamespacedIdentifier& classId) const
{
	resolve(classId, true);

	for (auto& t : templateClassIds)
	{
		if (t.id == classId)
			return true;
	}

	return false;
}

snex::jit::ComplexType::Ptr NamespaceHandler::createTemplateInstantiation(const NamespacedIdentifier& id, const Array<TemplateParameter>& tp, juce::Result& r)
{
	NamespacedIdentifier copy(id);

	jassert(isTemplateClass(copy));

	bool createTemplatedComplexType = false;

	for (const auto& p : tp)
	{
		if (p.type.isTemplateType())
		{
			createTemplatedComplexType = true;
			break;
		}
	}

	for (auto c : templateClassIds)
	{
		if (c.id == id)
		{
			TemplateObject::ConstructData d;
			d.handler = this;
			d.tp = tp;
			d.id = c.id;
			d.r = &r;
			
			ComplexType::Ptr ptr;

			if (createTemplatedComplexType)
				ptr = new TemplatedComplexType(c, d);
			else
				ptr = c.makeClassType(d);

			if (!r.wasOk())
				return ptr;

			r = Result::ok();

			ptr = registerComplexTypeOrReturnExisting(ptr);

			if (auto typed = dynamic_cast<ComplexTypeWithTemplateParameters*>(ptr.get()))
			{
				jassert(TemplateParameter::ListOps::isParameter(typed->getTemplateInstanceParameters()));
			}
			else
			{
				jassertfalse;
			}
				

			return ptr;
		}
	}

	r = Result::fail("Can't find template class");

	return nullptr;
}

void NamespaceHandler::createTemplateFunction(const NamespacedIdentifier& id, const Array<TemplateParameter>& tp, juce::Result& r)
{
	for (const auto& f : templateFunctionIds)
	{
		if (f.id == id && TemplateParameter::ListOps::isValidTemplateAmount(f.argList, tp.size()))
		{
			TemplateObject::ConstructData d;
			d.id = id;
			d.r = &r;
			d.handler = this;
			d.tp = tp;

			f.makeFunction(d);
			return;
		}
	}

	r = Result::fail("Can't instantiate function template " + id.toString());
	return;// {};
}

bool NamespaceHandler::rootHasNamespace(const NamespacedIdentifier& id) const
{
	auto type = getSymbolType(id);
	auto ns = get(id);
	
	return type == Unknown || type == Struct && ns != nullptr;
}

NamespaceHandler::SymbolType NamespaceHandler::getSymbolType(const NamespacedIdentifier& id) const
{
	if (auto p = get(id.getParent()))
	{
		for (const auto& a : p->aliases)
		{
			if (a.id == id)
				return a.symbolType;
		}
	}

	return NamespaceHandler::SymbolType::Unknown;
}

TypeInfo NamespaceHandler::getAliasType(const NamespacedIdentifier& aliasId) const
{
	return getTypeInfo(aliasId, { SymbolType::Struct, SymbolType::UsingAlias, SymbolType::TemplateType });
}

TypeInfo NamespaceHandler::getVariableType(const NamespacedIdentifier& variableId) const
{
	return getTypeInfo(variableId, { SymbolType::TemplateConstant, SymbolType::Variable, SymbolType::Constant, SymbolType::Function, SymbolType::EnumValue });
}

snex::VariableStorage NamespaceHandler::getConstantValue(const NamespacedIdentifier& variableId) const
{
	auto p = variableId.getParent();

	if (auto existing = get(p))
	{
		for (auto a : existing->aliases)
		{
			if (a.id == variableId && isConstantSymbol(a.symbolType))
				return a.constantValue;
		}
	}

	return {};
}

bool NamespaceHandler::isStaticFunctionClass(const NamespacedIdentifier& classId) const
{
	auto p = classId.getParent();

	if (auto existing = get(p))
	{
		for (auto& a : existing->aliases)
		{
			if (a.id == classId && a.symbolType == StaticFunctionClass)
				return true;
		}
	}

	return false;
}

bool NamespaceHandler::isClassEnumValue(const NamespacedIdentifier& classId) const
{
	if (auto e = get(classId.getParent()))
	{
		for (auto a : e->aliases)
		{
			if (a.id == classId)
			{
				auto isEnum = a.symbolType == SymbolType::EnumValue;
				auto isClassEnum = a.type.isConst();

				return isEnum && isClassEnum;
			}
		}
	}

	return false;
}

void NamespaceHandler::addTemplateClass(const TemplateObject& s)
{
	jassert(TemplateParameter::ListOps::isArgumentOrEmpty(s.argList));
	jassert(s.makeClassType);
	jassert(!s.makeFunction);
	jassert(!s.functionArgs);

	for (auto& tc : templateClassIds)
	{
		if (s.id.isParentOf(tc.id))
		{
			TemplateParameter::List newList;
			newList.addArray(s.argList);
			newList.addArray(tc.argList);

			tc.argList = newList;
		}
	}

	if (currentNamespace == nullptr)
		pushNamespace(Identifier());

	if (auto p = get(s.id.getParent()))
	{
		Alias a;
		a.id = s.id;
		a.symbolType = TemplatedClass;
		a.visibility = currentVisibility;
		a.internalSymbol = internalSymbolMode;

		p->aliases.add(a);
	}


	templateClassIds.addIfNotAlreadyThere(s);
}

void NamespaceHandler::addTemplateFunction(const TemplateObject& f)
{
	jassert(TemplateParameter::ListOps::isArgument(f.argList));
	jassert(!f.makeClassType);
	jassert(f.makeFunction);
	jassert(f.functionArgs);

	if (currentNamespace == nullptr)
		pushNamespace(Identifier());

	if (auto p = get(f.id.getParent()))
	{
		Alias a;
		a.id = f.id;
		a.internalSymbol = internalSymbolMode;
		a.symbolType = TemplatedFunction;
		p->aliases.add(a);
	}

	templateFunctionIds.addIfNotAlreadyThere(f);
}

TemplateObject NamespaceHandler::getTemplateObject(const NamespacedIdentifier& id, int numArgs) const
{
	for (const auto& c : templateClassIds)
	{
		if (c.id == id && TemplateParameter::ListOps::isValidTemplateAmount(c.argList, numArgs))
			return c;
	}

	for (const auto& f : templateFunctionIds)
	{
		if (f.id == id && TemplateParameter::ListOps::isValidTemplateAmount(f.argList, numArgs))
			return f;
	}

	return {};
}

juce::Array<snex::jit::TemplateObject> NamespaceHandler::getAllTemplateObjectsWith(const NamespacedIdentifier& id) const
{
	Array<TemplateObject> matches;

	for (const auto& c : templateClassIds)
	{
		if (c.id == id)
			matches.add(c);
	}

	for (const auto& c : templateFunctionIds)
	{
		if (c.id == id)
			matches.add(c);
	}

	return matches;
}


juce::Result NamespaceHandler::checkVisiblity(const NamespacedIdentifier& id) const
{
	auto parent = id.getParent();

	if (auto existing = get(parent))
	{
		for (const auto& a : existing->aliases)
		{
			if (a.id == id)
			{
				if (a.visibility == Visibility::Public)
					return Result::ok();

				auto currentId = getCurrentNamespaceIdentifier();

				if (parent.isParentOf(currentId) || parent == currentId)
					return Result::ok();
				else
				{
					return Result::fail(a.toString() + " is not accessible");
				}
			}
		}
	}

	return Result::ok();
}

void NamespaceHandler::setNamespacePosition(const NamespacedIdentifier& id, Point<int> s, Point<int> e)
{
	if (auto ex = get(id))
	{
		ex->setPosition({ s.getX(), e.getX() });
	}
}

juce::Array<juce::Range<int>> NamespaceHandler::createLineRangesFromNamespaces() const
{
	Array<Range<int>> list;

	for (auto n : existingNamespace)
	{
		if (!n->lines.isEmpty())
			list.add(n->lines);
	}

	return list;
}

TypeInfo NamespaceHandler::getTypeInfo(const NamespacedIdentifier& aliasId, const Array<SymbolType>& t) const
{
	auto p = aliasId.getParent();

	if (auto existing = get(p))
	{
		for (auto a : existing->aliases)
		{
			if (a.id == aliasId && t.contains(a.symbolType))
				return a.type;
		}
	}

	return {};
}

snex::jit::NamespaceHandler::Namespace::WeakPtr NamespaceHandler::getRoot() const
{
	Namespace::WeakPtr r = existingNamespace.getFirst().get();

	if (r == nullptr)
		return nullptr;

	while (r->parent != nullptr)
		r = r->parent;

	return r;
}

snex::jit::NamespaceHandler::Namespace::Ptr NamespaceHandler::get(const NamespacedIdentifier& id) const
{
	for (auto e : existingNamespace)
	{
		if (e->id == id)
			return e;
	}

	return nullptr;
}

struct NamespaceHandler::SymbolToken::Parser: public ParserHelpers::TokenIterator
{
	Parser(NamespaceHandler& nh, Namespace::WeakPtr c, const String& t):
		token(t),
		ParserHelpers::TokenIterator(t),
		current(c),
		handler(nh)
	{}

	bool parseStructType()
	{
		StructType* st = nullptr;

		if (id.isValid())
			st = handler.getVariableType(id).getTypedIfComplexType<StructType>();
		else
			st = dynamic_cast<StructType*>(typePtr.get());

		if (st != nullptr)
		{
			id = st->id;
			typePtr = nullptr;

			if (currentType == JitTokens::identifier)
			{
				auto childId = parseIdentifier();
				id = id.getChildId(childId);
			}

			return true;
		}

		return false;
	}

	bool parseSubscript()
	{
		ArrayTypeBase* atb = nullptr;

		if (id.isValid())
			atb = handler.getVariableType(id).getTypedIfComplexType<ArrayTypeBase>();
		else
			atb = dynamic_cast<ArrayTypeBase*>(typePtr.get());

		if (atb != nullptr)
		{
			id = {};
			typePtr = atb->getElementType().getTypedIfComplexType<ComplexType>();

			while (!isEOF() && currentType != JitTokens::closeBracket)
			{
				skip();
			}

			return true;
		}

		return false;
	}
	
	

	bool parseSubAccess()
	{
		if(matchIf(JitTokens::dot))
		{
			return parseStructType();
		}
		if (matchIf(JitTokens::openBracket))
		{
			return parseSubscript();
		}
		else
		{
			skip();
		}

		return false;
	}

	Namespace::WeakPtr parseNamespaceForToken()
	{
		if (current == nullptr)
			return nullptr;

		try
		{
			

			id = NamespacedIdentifier(parseIdentifier());

			while (matchIf(JitTokens::double_colon))
			{
				if (currentType == JitTokens::identifier)
				{
					id = id.getChildId(parseIdentifier());
				}
				else
				{
					break;
				}
			}

			if (id.isExplicit())
				id = current->id.getChildId(id.getIdentifier());

			ScopedNamespaceSetter sns(handler, current->id);
			handler.resolve(id);

			if (auto aliasType = handler.getAliasType(id).getTypedIfComplexType<ComplexType>())
			{
				typePtr = aliasType;
				id = {};
			}

			while (!isEOF())
			{
				parseSubAccess();
			}

			if (id.isNull())
				parseStructType();

			return handler.get(id);
		}
		catch (ParserHelpers::CodeLocation::Error& e)
		{
			return nullptr;
		}
		
		return nullptr;
	}

	NamespacedIdentifier id;
	ComplexType::Ptr typePtr;
	String token;
	NamespaceHandler& handler;
	Namespace::WeakPtr current;
};

bool NamespaceHandler::SymbolToken::matches(const String& input, const String& previousToken, int lineNumber) const
{
	if (n == nullptr)
		return false;

	if (previousToken.isNotEmpty())
	{
		Namespace* c = n.get();

		try
		{
			Parser p(*root.get(), root->getNamespaceForLineNumber(lineNumber), previousToken);

			if (auto realN = p.parseNamespaceForToken())
			{
				if (n.get() == realN)
				{
					if (a.visibility != Visibility::Public)
						return false;

					if (matchesInput(input, a.id.id.toString()))
						return true;
				}
			}
		}
		catch (ParserHelpers::CodeLocation::Error& e)
		{
			return false;
		}

		
		
		return false;
	}

	if (matchesInput(input, a.id.id.toString()))
	{
		return n.get() == root->getRoot() || n->lines.contains(lineNumber);
		
	}
	
	return false;
}

}
}
