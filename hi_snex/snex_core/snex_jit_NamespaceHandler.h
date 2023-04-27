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
namespace jit {
using namespace juce;



class NamespaceHandler: public ReferenceCountedObject
{
public:

	enum class Visibility
	{
		Public = 0,
		Protected,
		Private,
		numVisibilities
	};

	struct SymbolDebugInfo
	{
        SymbolDebugInfo() = default;
        
		static SymbolDebugInfo fromString(const String& s, Visibility v=Visibility::numVisibilities)
		{
			SymbolDebugInfo info;
			info.comment = s;
			info.visibility = v;
			return info;
		}

		Visibility visibility = Visibility::numVisibilities;
		int lineNumber = -1;
		String comment;
	};

	using Ptr = ReferenceCountedObjectPtr<NamespaceHandler>;

	

	enum SymbolType
	{
		Unknown,
		Struct,
		Function,
		Variable,
		UsingAlias,
		Enum,
		EnumValue,
		NamespacePlaceholder,
		PreprocessorConstant,
		Constant,
		StaticFunctionClass,
		TemplatedFunction,
		TemplatedClass,
		TemplateType,
		TemplateConstant,
		numSymbolTypes
	};

private:

	struct Alias
	{
		NamespacedIdentifier id;
		TypeInfo type;
		Visibility visibility = Visibility::Public;
		SymbolType symbolType;
		VariableStorage constantValue;
		bool internalSymbol = false;
		String codeToInsert;
		
		SymbolDebugInfo debugInfo;
		
		bool operator==(const Alias& other) const
		{
			return toString() == other.toString();
		}

		juce::String toString() const;
	};

	struct Namespace : public ReferenceCountedObject
	{
		Namespace() = default;

		using WeakPtr = WeakReference<Namespace>;
		using Ptr = ReferenceCountedObjectPtr<Namespace>;
		using List = ReferenceCountedArray<Namespace>;
		using WeakList = Array<WeakReference<Namespace>>;

		bool contains(const NamespacedIdentifier& symbol) const;

		juce::String dump(int level) const;

		static juce::String getIntendLevel(int level);

		void addSymbol(const NamespacedIdentifier& aliasId, const TypeInfo& type, SymbolType symbolType, Visibility v, const SymbolDebugInfo& description);

		void setPosition(Range<int> namespaceRange)
		{
			lines = namespaceRange;
		}

		bool hasChildNamespace(Namespace::WeakPtr p)
		{
			for (auto e : childNamespaces)
			{
				if (e == p)
					return true;

				if (e->hasChildNamespace(p))
					return true;
			}

			return false;
		}

		Namespace::WeakPtr getNamespaceForLineNumber(int lineNumber)
		{
			for (auto c : childNamespaces)
			{
				if (auto match = c->getNamespaceForLineNumber(lineNumber))
					return match;
			}

			if (lines.contains(lineNumber))
				return this;

			return nullptr;
		}

		NamespacedIdentifier id;
		Array<Alias> aliases;
		ReferenceCountedArray<Namespace> usedNamespaces;
		ReferenceCountedArray<Namespace> childNamespaces;
		WeakPtr parent;
		bool internalSymbol = false;

		SymbolDebugInfo debugInfo;
		Range<int> lines;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Namespace);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Namespace);
	};

	struct SymbolToken : public mcl::TokenCollection::Token
	{
		SymbolToken(NamespaceHandler::Ptr r, Namespace* n_, Alias a_):
			Token(a_.id.id.toString()),
			a(a_),
			n(n_),
			root(r)
		{
			tokenContent = a.id.id.toString();
			markdownDescription = a.debugInfo.comment;

			if (a.codeToInsert.isNotEmpty())
				tokenContent = a.codeToInsert;
		}

		bool matches(const String& input, const String& previousToken, int lineNumber) const override;

		NamespaceHandler::Ptr root;
		Namespace::Ptr n;
		Alias a;

		struct Parser;
	};

public:

	struct InternalSymbolSetter
	{
		InternalSymbolSetter(NamespaceHandler& h_):
			h(h_),
			s(h.internalSymbolMode, true)
		{}

		NamespaceHandler& h;
		ScopedValueSetter<bool> s;
	};

	struct ScopedVisibilityState
	{
		ScopedVisibilityState(NamespaceHandler& h):
			handler(h)
		{
			state = h.currentVisibility;
		}

		~ScopedVisibilityState()
		{
			handler.setVisiblity(state);
		}

		NamespaceHandler& handler;
		Visibility state;
	};

	struct ScopedTemplateParameterSetter
	{
		ScopedTemplateParameterSetter(NamespaceHandler& h, const TemplateParameter::List& currentList):
			handler(h)
		{
			if (currentList.isEmpty())
				wasEmpty = true;
			else
			{
				wasEmpty = false;

				// You should only use this method with template parameters, not arguments...
				jassert(TemplateParameter::ListOps::readyToResolve(currentList));
				handler.currentTemplateParameters.add(currentList);
			}
		}

		~ScopedTemplateParameterSetter()
		{
			if(!wasEmpty)
				handler.currentTemplateParameters.removeLast();
		}

		bool wasEmpty;
		NamespaceHandler& handler;
	};

	struct ScopedNamespaceSetter
	{
		ScopedNamespaceSetter(NamespaceHandler& h, const NamespacedIdentifier& id):
			handler(h),
			prevNamespace(handler.getCurrentNamespaceIdentifier())
		{
			h.pushNamespace(id);
		}

		ScopedNamespaceSetter(NamespaceHandler& h, const Identifier& id) :
			handler(h),
			prevNamespace(handler.getCurrentNamespaceIdentifier())
		{
			h.pushNamespace(prevNamespace.getChildId(id));
		}

		void clearCurrentNamespace();

		~ScopedNamespaceSetter()
		{
			handler.pushNamespace(prevNamespace);
		}

		NamespaceHandler& handler;
		NamespacedIdentifier prevNamespace;
	};

	NamespaceHandler() = default;
	~NamespaceHandler();

	ComplexType::Ptr registerComplexTypeOrReturnExisting(ComplexType::Ptr ptr);
	ComplexType::Ptr getComplexType(NamespacedIdentifier id);
	ComplexType::Ptr getExistingTemplateInstantiation(NamespacedIdentifier id, const TemplateParameter::List& tp);

	TypeInfo parseTypeFromTextInput(const String& input, int lineNumber);

	bool changeSymbolType(NamespacedIdentifier id, SymbolType newType);

	NamespacedIdentifier getRootId() const;
	bool isNamespace(const NamespacedIdentifier& possibleNamespace) const;
	
	NamespacedIdentifier getCurrentNamespaceIdentifier() const;
	juce::String dump() const;
	Result addUsedNamespace(const NamespacedIdentifier& usedNamespace);
	Result resolve(NamespacedIdentifier& id, bool allowZeroMatch = false) const;
	void addSymbol(const NamespacedIdentifier& id, const TypeInfo& t, SymbolType symbolType, const SymbolDebugInfo& info);
	void setSymbolCode(const NamespacedIdentifier& id, const String& tokenToInsert);

	Result addConstant(const NamespacedIdentifier& id, const VariableStorage& v);
	Result setTypeInfo(const NamespacedIdentifier& id, SymbolType expectedType, const TypeInfo& t);

	Result copySymbolsFromExistingNamespace(const NamespacedIdentifier& existingNamespace);

    Array<ValueTree> createDataLayouts() const
    {
        Array<ValueTree> layouts;
        
        for(auto t: complexTypes)
        {
            auto v = t->createDataLayout();
            
            if(v.isValid())
                layouts.add(v);
        }
        
        return layouts;
    }
    
	Namespace::WeakPtr getNamespaceForLineNumber(int lineNumber) const;

	mcl::TokenCollection::List getTokenList();

	String getDescriptionForItem(const NamespacedIdentifier& n) const;

	static bool isConstantSymbol(SymbolType t);

	bool isTemplateTypeArgument(NamespacedIdentifier& classId) const;

	bool isTemplateConstantArgument(NamespacedIdentifier& classId) const;

	bool isTemplateFunction(TemplateInstance& functionId) const;

	bool isTemplateClass(TemplateInstance& classId) const;

	/** Doesn't check the template parameters, just for the parser to decide whether it should parse template arguments. */
	bool isTemplateClassId(NamespacedIdentifier& classId) const;

	Array<jit::TemplateObject> getTemplateClassTypes() const;

	ComplexType::Ptr createTemplateInstantiation(const TemplateInstance& id, const Array<TemplateParameter>& tp, juce::Result& r);

	void createTemplateFunction(const TemplateInstance& id, const Array<TemplateParameter>& tp, juce::Result& r);

	bool rootHasNamespace(const NamespacedIdentifier& id) const;

	int getDefinitionLine(int lineNumber, const String& token);

	SymbolType getSymbolType(const NamespacedIdentifier& id) const;

	TypeInfo getAliasType(const NamespacedIdentifier& aliasId) const;

	TypeInfo getVariableType(const NamespacedIdentifier& variableId) const;

	VariableStorage getConstantValue(const NamespacedIdentifier& variableId) const;

	StringArray getEnumValues(const NamespacedIdentifier& enumId) const;

	bool isStaticFunctionClass(const NamespacedIdentifier& classId) const;

	bool isClassEnumValue(const NamespacedIdentifier& classId) const;

	Result switchToExistingNamespace(const NamespacedIdentifier& id);

	void addTemplateClass(const TemplateObject& s);

	void addTemplateFunction(const TemplateObject& f);

	TemplateObject getTemplateObject(const TemplateInstance& id, int numProvidedArguments=-1) const;

	Array<TemplateObject> getAllTemplateObjectsWith(const TemplateInstance& id) const;

	Result checkVisiblity(const NamespacedIdentifier& id) const;

	NamespacedIdentifier createNonExistentIdForLocation(const NamespacedIdentifier& customParent, int lineNumber) const;

	void setVisiblity(Visibility newVisibility)
	{
		currentVisibility = newVisibility;
	}

	TemplateParameter::List getCurrentTemplateParameters() const;

	void setNamespacePosition(const NamespacedIdentifier& id, Point<int> s, Point<int> e, const SymbolDebugInfo& ingo);

	Array<Range<int>> createLineRangesFromNamespaces() const;

	ReferenceCountedArray<ComplexType> getComplexTypeList();

	bool removeNamespace(const NamespacedIdentifier& id);

	void setEnableLineNumbers(bool shouldBeEnabled)
	{
		enableLineNumbers = shouldBeEnabled;
	}
	
	bool shouldCalculateNumbers() const { return enableLineNumbers; }

private:

	bool enableLineNumbers = true;

	mutable bool skipResolving = false;

	Array<TemplateParameter::List> currentTemplateParameters;

	bool internalSymbolMode = false;

	void pushNamespace(const Identifier& childId);
	void pushNamespace(const NamespacedIdentifier& id);
	Result popNamespace();

	TypeInfo getTypeInfo(const NamespacedIdentifier& aliasId, const Array<SymbolType>& t) const;

	ReferenceCountedArray<ComplexType> complexTypes;

	Array<TemplateObject> templateClassIds;
	Array<TemplateObject> templateFunctionIds;

	Namespace::WeakPtr getRoot() const;
	Namespace::Ptr get(const NamespacedIdentifier& id) const;
	Namespace::List existingNamespace;
	Namespace::WeakPtr currentNamespace;
	Namespace::WeakPtr currentParent;

	Visibility currentVisibility = Visibility::Public;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NamespaceHandler);
};




}
}
