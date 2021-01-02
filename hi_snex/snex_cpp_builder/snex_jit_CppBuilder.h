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

namespace cppgen
{

using namespace juce;

static constexpr juce_wchar Space = ' ';

// This causes a line break with a intendation to the first occurence
static constexpr juce_wchar IntendMarker = '$';

// This causes an alignment of subsequent lines to the biggest character amount
static constexpr juce_wchar AlignMarker = '@';

struct StringHelpers
{
	static String withToken(juce_wchar token, const String& s = {})
	{
		String t;
		t << token;
		t << s;
		return t;
	}

	static void addSuffix(NamespacedIdentifier& id, const String& suffix)
	{
		auto newId = id.id.toString();
		newId << suffix;
		id.id = Identifier(newId);
	}
};

class Base
{
public:

	using TokenType = const char*;

	enum class OutputType
	{
		NoProcessing,
		Uglify,
		AddTabs,
		WrapInBlock,
		numOutputTypes
	};

	Base(OutputType type=OutputType::WrapInBlock) :
		t(type)
	{}

	virtual ~Base()
	{};

	enum class CommentType
	{
		Raw,
		RawWithNewLine,
		AlignOnSameLine,
		FillTo80,
		FillTo80Light
	};

	Base& operator<<(const String& line);

	Base& operator<<(const jit::FunctionData& f);

	void addComment(const String& comment, CommentType commentType);

	String parseUglified() const;

	static int getRealLineLength(const String& s);

	void addIfNotEmptyLine()
	{
		auto lastLine = lines[lines.size() - 1];

		if (lastLine.isEmpty() || lastLine.startsWith("//"))
			return;

		lines.add("");
	}

	void addEmptyLine()
	{
		lines.add("");
	}

	String toString() const;

	NamespacedIdentifier getCurrentScope()
	{
		return currentNamespace;
	}

	void pushScope(const Identifier& id)
	{
		currentNamespace = currentNamespace.getChildId(id);
	}

	void popScope()
	{
		jassert(!currentNamespace.isNull());
		currentNamespace = currentNamespace.getParent();
	}

	virtual void clear()
	{
		currentNamespace = {};
		lines.clear();
	}

private:

	struct ParseState
	{
		int currentLine = 0;
		int intendLevel = 0;
		int currentAlignLength = -1;
		bool lastLineWasEmpty = false;
	};

	bool isIntendKeyword(int line) const;
	int getIntendDelta(int line) const;

	bool containsButNot(int line, TokenType toContain, TokenType toNotContain) const;
	bool matchesEnd(int line, TokenType t, TokenType other1 = nullptr, TokenType other2 = nullptr) const;
	bool matchesStart(int line, TokenType t, TokenType other1 = nullptr, TokenType other2 = nullptr) const;

	String parseLines() const;
	String wrapInBlock() const;

	String parseLine(ParseState& state, int i) const;
	String parseIntendation(ParseState& state, const String& lineContent) const;
	String parseLineWithAlignedComment(ParseState& state, const String& lineContent) const;
	String parseLineWithIntendedLineBreak(ParseState& state, const String& lineContent) const;

	const OutputType t;
	StringArray lines;

	NamespacedIdentifier currentNamespace;
};

struct DefinitionBase
{
	DefinitionBase(Base& b, const Identifier& id) :
		scopedId(b.getCurrentScope().getChildId(id)),
		parent_(b)
	{};

	virtual ~DefinitionBase()
	{

	}

	bool operator==(const DefinitionBase& other) const
	{
		return scopedId == other.scopedId;
	}

	/** Override this and return an expression. */
	virtual String toExpression() const 
	{ 
		auto pScope = parent_.getCurrentScope();

		if (pScope == scopedId.getParent())
			return scopedId.getIdentifier().toString();
		else
		{
			if (pScope.isParentOf(scopedId))
			{
				auto relocated = scopedId.relocate(pScope, {});
				return relocated.toString();
			}
			if (pScope.isParentOf(scopedId.getParent()))
			{
				auto relocated = scopedId.removeSameParent(pScope);
				return relocated.toString();
			}
			

			return scopedId.toString();

		}
			
	}

	Base& parent_;
	NamespacedIdentifier scopedId;
};

struct Op
{
	Op(Base& parent_) :
		parent(parent_)
	{}

	

	bool isFlushed() const
	{
		return flushed;
	}

	virtual ~Op()
	{
		jassert(flushed);
	}

	void flushIfNot()
	{
		if (!isFlushed())
			flush();
	}

protected:

	virtual void flush()
	{
		flushed = true;
	}

	bool flushed = false;
	Base& parent;
};

struct StatementBlock: public Op
{
	StatementBlock(Base& parent) :
		Op(parent)
	{
		parent << "{";
	};

	~StatementBlock()
	{
		flushIfNot();
	}

private:

	void flush() override
	{
		parent << "}";
		Op::flush();
	}
};

struct Function: public Op
{
	Function(Base& parent, const jit::FunctionData& f);;

	void flush() override
	{
		parent.popScope();
		Op::flush();
	}

	~Function()
	{
		flushIfNot();
	}
};

struct Struct : public Op,
				public DefinitionBase
{
	Struct(Base& parent, const Identifier& id, const Array<DefinitionBase*>& baseClasses, const jit::TemplateParameter::List& tp);

	~Struct()
	{
		flushIfNot();
	}

private:

	void flush() override
	{
		parent.popScope();
		parent << "};";
		Op::flush();
	}
};

struct Symbol : public DefinitionBase
{
	Symbol(Base& parent, const Identifier& id):
		DefinitionBase(parent, id)
	{

	}
};

struct StackVariable : public DefinitionBase,
					   public Op
{
	StackVariable(Base& parent, const Identifier& id, const jit::TypeInfo& t);

	~StackVariable()
	{
		Op::flush();
	}

	StackVariable& operator<<(const String& e)
	{
		expression << e;
		return *this;
	}

	StackVariable& operator<<(int i)
	{
		expression << String(i);
		return *this;
	}

	StackVariable& operator<<(StackVariable& other)
	{
		if (other.hasLineBreaks())
		{
			other.flushIfNot();
		}

		expression << other.toExpression();
		return *this;
	}

	String toExpression() const
	{
		if (isFlushed())
		{
			return DefinitionBase::toExpression();
		}
		else
			return expression;
	}

	int getLength() const
	{
		return expression.length();
	}

	void addBreaksBeforeDots(int numDotsPerLine)
	{
		auto dots = StringArray::fromTokens(expression, ".", "\"");

		String newExpression;

		newExpression << IntendMarker;

		for (int i = 0; i < dots.size(); i++)
		{
			newExpression << dots[i] << ".";

			if (((i + 1) % numDotsPerLine) == 0)
				newExpression << IntendMarker;

			
		}

		expression = newExpression.upToLastOccurrenceOf(".", false, false);
	}

	bool hasLineBreaks() const
	{
		return expression.containsChar(IntendMarker);
	}

protected:

	String expression;

private:

	void flush() override;

	jit::TypeInfo type;
};

struct Namespace : public Op
{
	Namespace(Base& parent, const Identifier& id, bool isEmpty);

	~Namespace()
	{
		flushIfNot();
	}

private:

	const bool isEmpty;

	void flush() override
	{
		if (!isEmpty)
		{
			parent.popScope();
			parent << "}";
			parent.addEmptyLine();
		}

		Op::flush();
	}
};

struct Macro : public Op
{
	Macro(Base& b, const String& name, const StringArray& args):
		Op(b)
	{
		s << name;
		s << "(";
		
		for (auto a : args)
			s << a << ", ";

		s = s.upToLastOccurrenceOf(", ", false, false);

		s << ");";
	}

	~Macro()
	{
		flushIfNot();
	}

private:

	void flush() override
	{
		parent << s;
		Op::flush();
	}

	String s;
};

struct UsingTemplate: public DefinitionBase,
				      public Op
{
	UsingTemplate(Base& b, const Identifier& id, const NamespacedIdentifier& templateId):
		DefinitionBase(b, id),
		Op(b),
		tId(templateId)
	{

	}

	~UsingTemplate()
	{
		Op::flush();
	}

	

	UsingTemplate& operator<<(const String& s)
	{
		args.add(s);
		return *this;
	}

	UsingTemplate& operator<<(const NamespacedIdentifier& id)
	{
		args.add(id.toString());
		return *this;
	}

	UsingTemplate& operator<<(int templateConstant)
	{
		args.add(String(templateConstant));
		return *this;
	}

	UsingTemplate& operator<<(const UsingTemplate& other)
	{
		args.add(other.toExpression());

		return *this;
	}

	int getLength() const { return getUsingExpression().length(); }

	String toString() const;

	String toExpression() const override;

	String getUsingExpression() const;

private:

	void flush() override
	{
		auto e = toExpression();

		if (!e.isEmpty())
		{
			parent << toString();
		}

		Op::flush();
	}

	NamespacedIdentifier tId;
	StringArray args;
};

}




}
