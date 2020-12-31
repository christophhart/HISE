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

class Base
{
public:

	using TokenType = const char*;

	enum class OutputType
	{
		NoProcessing,
		AddTabs,
		WrapInBlock,
		numOutputTypes
	};

	Base(OutputType type) :
		t(type)
	{}

	virtual ~Base()
	{};

	Base& operator<<(const String& line);

	Base& operator<<(const jit::FunctionData& f);

	void addComment(const String& comment)
	{
		lines.add("// " + comment);
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

	void clear()
	{
		currentNamespace = {};
		lines.clear();
	}

private:

	bool isIntendKeyword(int line) const;
	int getIntendDelta(int line) const;

	bool containsButNot(int line, TokenType toContain, TokenType toNotContain) const;
	bool matchesEnd(int line, TokenType t, TokenType other1 = nullptr, TokenType other2 = nullptr) const;
	bool matchesStart(int line, TokenType t, TokenType other1 = nullptr, TokenType other2 = nullptr) const;

	String parseLines() const;
	String wrapInBlock() const;

	const OutputType t;
	StringArray lines;

	NamespacedIdentifier currentNamespace;
};

struct DefinitionBase
{
	DefinitionBase(Base& b, const Identifier& id) :
		scopedId(b.getCurrentScope().getChildId(id))
	{};

	virtual ~DefinitionBase()
	{

	}

	bool operator==(const DefinitionBase& other) const
	{
		return scopedId == other.scopedId;
	}

	NamespacedIdentifier scopedId;
};


struct Op
{
	Op(Base& parent_) :
		parent(parent_)
	{}

	virtual void flush()
	{
		flushed = true;
	}

	bool isFlushed() const
	{
		return flushed;
	}

	virtual ~Op()
	{
		jassert(flushed);
	}

protected:

	void flushIfNotAlreadyFlushed()
	{
		if (!flushed)
			flush();
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

	void flush() override
	{
		parent << "}";
		Op::flush();
	}

	~StatementBlock()
	{
		flushIfNotAlreadyFlushed();
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
		flushIfNotAlreadyFlushed();
	}
};

struct Struct : public Op
{
	Struct(Base& parent, const Identifier& id, const jit::TemplateParameter::List& tp);

	void flush() override
	{
		parent.popScope();
		parent << "};";
		Op::flush();
	}

	~Struct()
	{
		flushIfNotAlreadyFlushed();
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

	void flush() override;

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

	StackVariable& operator<<(const StackVariable& other)
	{
		expression << other.toString();
		return *this;
	}

	String toString() const;

	jit::TypeInfo type;
	String expression;
};

struct Namespace : public Op
{
	Namespace(Base& parent, const Identifier& id);

	void flush() override
	{
		parent.popScope();
		parent << "}";
		Op::flush();
	}

	~Namespace()
	{
		flushIfNotAlreadyFlushed();
	}
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

	void flush() override
	{
		parent << toString();
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
		if (!other.isFlushed())
		{
			args.add(other.getType());
		}
		else
		{
			if (other.scopedId.getParent() == parent.getCurrentScope())
				args.add(other.scopedId.getIdentifier().toString());
			else
				args.add(other.scopedId.toString());
		}

		return *this;
	}

	String toString() const;

	String getType() const;

private:

	NamespacedIdentifier tId;
	StringArray args;
};

}




}
