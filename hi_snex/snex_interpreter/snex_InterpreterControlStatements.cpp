/** ===========================================================================
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
namespace interpreter {
namespace nodes {



struct SyntaxTree : public Statement
{
	SNEX_INTERPRETER_NODE(SyntaxTree, WantsNode::Yes);

	SyntaxTree(Statement::Factory& f, const ValueTree& v) :
	  Statement(v)
	{
		for (auto c : v)
		{
			if (auto ptr = f.create(c))
			{
				children.add(ptr);
				locations.push_back(Helpers::createUriLocation(f, c));
			}
				
		}
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{ 
		for (int i = instructionPointer; i < children.size(); i++)
		{
			auto c = children[i];

			if (auto r = c->perform(s, returnValue))
				return r;
		}

		return ResultCode::ok; 
	};

	int instructionPointer = 0;
	std::vector<UriLocation> locations;
	List children;
};

struct Function : public Statement
{
	SNEX_INTERPRETER_NODE(Function, WantsNode::Yes);

	Function(Statement::Factory& f, const ValueTree& v) :
		Statement(v)
	{
		body = f.create(v.getChildWithName(SyntaxTree::getStatementType()));
		fData = Helpers::getFunctionData(f, v);
		jassert(body != nullptr);
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		return body->perform(s, returnValue);
	};

	void annotateUriLocationTree(UriLocationTree& t) override
	{
		for (auto a : fData.args)
		{
			auto argDef = new UriLocationTree(&t);
			argDef->fillLocation(t);
			argDef->category = "parameter";
			argDef->id = a.id.toString();
			Helpers::annotateType(*argDef, a.typeInfo);
		}
	}

	jit::FunctionData fData;
	Ptr body;
};

struct ReturnStatement : public Statement
{
	SNEX_INTERPRETER_NODE(ReturnStatement, WantsNode::No);

	ReturnStatement(Statement::Factory& f, const ValueTree& v) :
		Statement(v),
		value(Helpers::getChildExpression(f, v, 0))
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		if (returnValue != nullptr && value != nullptr)
			*returnValue = value->getResult(s);
		
		return ResultCode::returnWasHit;
	};

	Expression::ExprPtr value;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex
