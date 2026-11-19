/* ===========================================================================
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
namespace interpreter {

struct Statement : public ReferenceCountedObject
{
	enum class WantsNode { No, Yes };

	using Ptr = ReferenceCountedObjectPtr<Statement>;
	using List = ReferenceCountedArray<Statement>;
	enum ResultCode { ok = 0, returnWasHit, breakWasHit, continueWasHit };

	struct Collection
	{
		template <typename StatementType=Statement> bool callRecursive(const ValueTree& v, const std::function<bool(StatementType*)>& f) const
		{
			if (auto s = get(v))
			{
				if (auto typed = s->getTyped<StatementType>())
				{
					if (f(typed))
						return true;
				}

				for (auto c : v)
				{
					if (callRecursive(c, f))
						return true;
				}
			}

			return false;
		};

		template <typename StatementType=Statement> bool forEachNodeFlat(const std::function<bool(StatementType*)>& f) const
		{
			for (auto& item : flatList)
			{
				if (auto typed = item.second->getTyped<StatementType>())
				{
					if (f(typed))
						return true;
				}
			}

			return false;
		};

		Ptr get(const ValueTree& v) const
		{
			for (const auto& item : flatList)
			{
				if (item.first == v)
					return item.second;
			}

			return nullptr;
		}

		Ptr root;
		std::vector<std::pair<ValueTree, Ptr>> flatList;
	};

	struct Factory
	{
		using CreateFunction = std::function<Ptr(Factory& f, const ValueTree& v)>;

		Factory();;

		template <typename T> void registerType()
		{
			auto id = T::getStatementType();
			createFunctions[id] = T::create;
		}

		UriLocationBuilder::Ptr locationBuilder;
		Statement::Ptr create(const ValueTree& v);
		std::map<Identifier, CreateFunction> createFunctions;
		
		Collection createdNodes;
		ReferenceCountedArray<jit::ComplexType> typeLibrary;
	};

	struct Helpers;

	Statement(const ValueTree& astNode_) :
	  astNode(astNode_)
	{};

	virtual ~Statement() = default;
	virtual ResultCode perform(Scope* s, VariableStorage* returnValue) { return ResultCode::ok; };

	virtual void annotateUriLocationTree(UriLocationTree& t) {};

	virtual WantsNode shouldCreateNode() const = 0;

	template <typename T> T* getTyped() 
	{ 
		static_assert(std::is_base_of<interpreter::Statement, T>() ||
					  std::is_same<interpreter::Statement, T>(), "not a base of interpreter::Statement");

		return dynamic_cast<T*>(this); 
	}

	ValueTree astNode;
};

struct Expression : public Statement
{
	using ExprPtr = ReferenceCountedObjectPtr<Expression>;
	using ExprList = ReferenceCountedArray<Expression>;

	Expression(const ValueTree& astNode) :
		Statement(astNode)
	{};

	virtual jit::TypeInfo getTypeInfo() const = 0;
	virtual VariableStorage getResult(Scope* s) = 0;

	virtual void assign(Scope* s, const VariableStorage& v) {}
};

} // namespace interpreter
} // namespace snex
