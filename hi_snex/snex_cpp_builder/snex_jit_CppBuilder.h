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
	static String makeValidCppName(const String& input);

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

	using Header = std::function<String()>;

	using TokenType = const char*;

	enum class OutputType
	{
		NoProcessing,
		Uglify,
		AddTabs,
		WrapInBlock,
		StatementListWithoutSemicolon,
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

	Base& operator<<(juce_wchar specialCharacter);

	Base& operator<<(const jit::FunctionData& f);

	void addComment(const String& comment, CommentType commentType);

	String parseUglified();

	void replaceWildcard(const String& wc, const String& expression);

	static int getRealLineLength(const String& s);

	void addIfNotEmptyLine()
	{
		auto lastLine = lines[lines.size() - 1];

		if (lastLine.isEmpty() || lastLine.startsWith("//"))
			return;

		lines.add("");
	}

	virtual void addEmptyLine()
	{
		lines.add("");
	}

	Base& append(const String& s)
	{
		if (lines.isEmpty())
			lines.add(s);

		lines.getReference(lines.size() - 1) << s;
		return *this;
	}

	Base& addWithSemicolon(const String& line)
	{
		String l;
		l << line << ";";
		lines.add(l);
		return *this;
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

	void setHeader(const Header& h)
	{
		header = h;
	}

    struct ExistingDefinition
    {
        NamespacedIdentifier nid;
        String value;
    };
    
    NamespacedIdentifier pushDefinition(const NamespacedIdentifier& id, const String& value)
    {
        for(const auto& s: existingDefinitions)
        {
            if(s.nid.getParent() == id.getParent() && s.value == value)
            {
                numSavedTemplates++;
                return s.nid;
            }
        }
        
        existingDefinitions.add({id, value});
        return {};
    }
    
protected:

    
    
	StringArray lines;

private:

    mutable int numSavedTemplates = 0;
    
    Array<ExistingDefinition> existingDefinitions;
    
	Header header;

	struct ParseState
	{
		ParseState(const StringArray& l) :
			linesToUse(l)
		{};

		const StringArray& linesToUse;
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

	String parseRawAndAddSemicolon() const;

	String parseLines() const;
	String wrapInBlock() const;

	String parseLine(ParseState& state, int i) const;
	String parseIntendation(ParseState& state, const String& lineContent) const;
	String parseLineWithAlignedComment(ParseState& state, const String& lineContent) const;
	String parseLineWithIntendedLineBreak(ParseState& state, const String& lineContent) const;

	const OutputType t;
	

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
	jit::TemplateParameter::List templateArguments;
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

struct UsingNamespace : public Op
{
	UsingNamespace(Base& b, const NamespacedIdentifier& id):
		Op(b)
	{
		String def;
		def << "using namespace " << id.toString() << ";";
		b << def;
		Op::flush();
	}
};

struct StatementBlock: public Op
{
	StatementBlock(Base& parent, bool addSemicolon=false) :
		Op(parent),
		addSemi(addSemicolon)
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
		parent << (addSemi ? "};" : "}");
		Op::flush();
	}

	const bool addSemi;
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

	Struct(Base& parent, const Identifier& id, const Array<NamespacedIdentifier>& baseClasses, const jit::TemplateParameter::List& tp, bool useIds);



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

template <typename IntegerType, typename OriginalDataType> struct IntegerArray : public Op,
				   public DefinitionBase
{
	IntegerArray(Base& parent, const Identifier& id, const OriginalDataType* originalData, int numOriginalElements) :
		DefinitionBase(parent, id),
		Op(parent)
	{
		static_assert(std::is_unsigned<IntegerType>(), "you need to use unsigned integer types");

		auto ratio = (float)sizeof(OriginalDataType) / (float)sizeof(IntegerType);
		auto leftOver = hmath::fmod((float)numOriginalElements, 1.0f / ratio);

		if (leftOver != 0.0f)
		{
			// If the integer type has more bytes and we have some leftovers, we need
			// zero pad once

			auto numToAdd = (int)(numOriginalElements * ratio);

			HeapBlock<OriginalDataType> paddedData;
			paddedData.calloc(numOriginalElements + (int)leftOver);

			memcpy(paddedData.getData(), originalData, numOriginalElements * sizeof(OriginalDataType));
			
			data.addArray(reinterpret_cast<const IntegerType*>(paddedData.getData()), numToAdd);
		}
		else
		{
			auto numToAdd = (int)(numOriginalElements * ratio);
			data.addArray(reinterpret_cast<const IntegerType*>(originalData), numToAdd);
		}
	}

	~IntegerArray()
	{
		flushIfNot();
	}

	static String getTypeName()
	{
		
		if(sizeof(IntegerType) == 1) return "uint8";
		if (sizeof(IntegerType) == 2) return "uint16";
		if (sizeof(IntegerType) == 4) return "uint32";
		if (sizeof(IntegerType) == 8) return "uint64";

		jassertfalse;

		return "int";
	}

	static constexpr int NumPerLine = 80;

	static void writeToStream(OutputStream& output, IntegerType* dataToWrite, int numElements)
	{
		String line;

		line.preallocateBytes(NumPerLine + 10);

		for (int i = 0; i < numElements; i++)
		{
			if (line.length() > NumPerLine)
			{
				line << "\n";

				output << line;
				line = {};
				line.preallocateBytes(NumPerLine + 10);
			}

			line << "0x" << String::toHexString(dataToWrite[i]).toUpperCase();

			if (i != numElements - 1)
				line << ",";
		}

		if(!line.isEmpty())
			output << line;
	}

	void flush() override
	{
		String def;

		def << "span<" << getTypeName() << ", " << String(data.size()) << "> " << scopedId.getIdentifier() << " = ";

		parent << def;
		parent << "{";

		MemoryOutputStream mos;
		writeToStream(mos, data.begin(), data.size());
		mos.flush();
		parent << mos.toString();

		parent << "};";

		Op::flush();
	}

	Array<IntegerType> data;
};

struct FloatArray : public Op,
					public DefinitionBase
{
	FloatArray(Base& parent, const Identifier& id, const Array<float>& data_) :
		DefinitionBase(parent, id),
		Op(parent)
	{
		data.addArray(data_);
	}

	~FloatArray()
	{
		flushIfNot();
	}

	void flush() override
	{
		String def;
		
		def << "span<float, " << String(data.size()) << "> " << scopedId.getIdentifier() << " = ";
		
		parent << def;
		parent << "{";
		
		int NumPerLine = 6;

		for (int i = 0; i < data.size(); i += NumPerLine)
		{
			String line;

			for (int j = i; j < jmin(i + NumPerLine, data.size()); j++)
			{
				line << Helpers::getCppValueString(data[j]);
				
				if(j != data.size()-1)
					line << ", ";
			}

			parent << line;
		}

		parent << "};";

		Op::flush();
	}

	Array<float> data;
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

struct Include : public Op
{
	Include(Base& b, const File& currentFile, const File& fileToInclude, bool includeContent=false):
		Op(b),
		root(currentFile),
		includeFile(fileToInclude)
	{}

	Include(Base& b, const String& globalHeader_, bool includeContent = false) :
		Op(b),
		root(File()),
		includeFile(File()),
		globalHeader(globalHeader_)
	{}

	~Include()
	{
		flushIfNot();
	}



	void flush() override
	{
		String d;

		if (globalHeader.isNotEmpty())
			d << "#include <" << globalHeader << ">";
		else
			d << "#include " << includeFile.getRelativePathFrom(root).replace("\\", "/").quoted();

		parent << d;

		Op::flush();
	}

	File root;
	File includeFile;
	String globalHeader;
};

struct Macro : public Op
{
	Macro(Base& b, const String& name, const StringArray& args, bool addSemicolon=true):
		Op(b)
	{
		s << name;
		s << "(";
		
		for (auto a : args)
			s << a << ", ";

		s = s.upToLastOccurrenceOf(", ", false, false);

		s << ")";

		if (addSemicolon)
			s << ";";
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

struct EncodedParameterMacro : public Op
{
	EncodedParameterMacro(Base& b, scriptnode::parameter::encoder& pe_) :
		Op(b),
		pe(pe_)
	{}

	~EncodedParameterMacro()
	{
		flushIfNot();
	}

	void flush() override
	{
		String a;

		a << "{\n\t" << IntendMarker;

		auto data = pe.writeItems();

		MemoryInputStream mis(data, true);
		auto numIntsWritten = 0;

		while (!mis.isExhausted())
		{
			auto iData = mis.readShort();

			a << "0x";

			auto hexString = String::toHexString(iData).toUpperCase();

			int numChars = 4;

			if (hexString.length() < numChars)
			{
				for (int i = 0; i < numChars - hexString.length(); i++)
					a << '0';
			}

			a << hexString << ", ";

			if ((
				numIntsWritten + 1) % 8 == 0)
				a << IntendMarker;

			numIntsWritten++;
		}

		a = a.upToLastOccurrenceOf(", ", false, false);
		a << "\n};";

		Macro(parent, "SNEX_METADATA_ENCODED_PARAMETERS", { String(numIntsWritten) }, false);

		parent << a;

		Op::flush();
	}

	scriptnode::parameter::encoder& pe;
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
		for (auto& tp : other.templateArguments)
			templateArguments.addIfNotAlreadyThere(tp);

		args.add(other.toExpression());

		return *this;
	}

	UsingTemplate& operator<<(const Struct& s)
	{
		for (auto& tp : s.templateArguments)
			templateArguments.addIfNotAlreadyThere(tp);

		args.add(s.toExpression());

		return *this;
	}

	int getLength() const { return getUsingExpression().length(); }

	String toString() const;

	String toExpression() const override;

	String getUsingExpression() const;

    void setTemplateId(const NamespacedIdentifier& newTid)
    {
        tId = newTid;
    }
    
	void addTemplateIntegerArgument(const String& id, bool addAsParameter)
	{
		templateArguments.addIfNotAlreadyThere(snex::jit::TemplateParameter(NamespacedIdentifier(id), 0, false));

		if (addAsParameter)
			*this << id;
	}

	void flushIfLong()
	{
		if (getLength() > 60)
			flushIfNot();
	}

private:

    void flush() override;
	
    void appendTemplateParameters(String& s) const
    {
        if (!templateArguments.isEmpty())
        {
            s << "<";

            for (int i = 0; i < templateArguments.size(); i++)
            {
                s << templateArguments[i].argumentId.toString();

                if(isPositiveAndBelow(i, templateArguments.size() - 1))
                    s << ", ";
            }

            s << ">";
        }
    }
    
	NamespacedIdentifier tId;
	StringArray args;

	
};

}




}
