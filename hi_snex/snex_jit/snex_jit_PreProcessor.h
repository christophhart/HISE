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
using namespace asmjit;

using String = juce::String;


#if JUCE_DEBUG
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x)
#endif
#define HNODE_PREPROCESSOR_KEYWORDS(X) \
    

namespace PreprocessorTokens
{
#define DECLARE_HNODE_JIT_TOKEN(name, str)  static const char* const name = str;

DECLARE_HNODE_JIT_TOKEN(if_, "#if")      
DECLARE_HNODE_JIT_TOKEN(endif_, "#endif") 
DECLARE_HNODE_JIT_TOKEN(elif_, "#elif")		
DECLARE_HNODE_JIT_TOKEN(define_, "#define")  
DECLARE_HNODE_JIT_TOKEN(undef_, "#undef") 
DECLARE_HNODE_JIT_TOKEN(include_, "#include") 
DECLARE_HNODE_JIT_TOKEN(else_, "#else")
DECLARE_HNODE_JIT_TOKEN(eof, "$eof")
DECLARE_HNODE_JIT_TOKEN(literal, "$literal")
DECLARE_HNODE_JIT_TOKEN(identifier, "$identifier")
DECLARE_HNODE_JIT_TOKEN(code_, "code")
}



struct Preprocessor
{
	using Token = const char*;

	Preprocessor(const String& code_);

	juce::String process();
	juce::String getOriginalCode() const { return code; }

	SparseSet<int> getDeactivatedLines()
	{
		conditionMode = true;
		process();
		jassert(conditionMode);
		return deactivatedLines;
	}

private:

	bool conditionMode = false;

	SparseSet<int> deactivatedLines;

	struct Item : public ReferenceCountedObject
	{
		using List = ReferenceCountedArray<Item>;
		using Ptr = ReferenceCountedObjectPtr<Item>;

		virtual ~Item() {};

		NamespacedIdentifier id;
		String body;
	};

	struct Definition : public Item
	{
		bool evaluate(String& b, Result& r)
		{
			jassertfalse;
		}
	};

	struct Macro: public Item
	{
		Macro(const Array<Identifier>& arguments_):
			arguments(arguments_)
		{}

		Array<Identifier> arguments;
	
		String evaluate(StringArray& parameters, Result& r)
		{
			if (parameters.size() != arguments.size())
			{
				r = Result::fail("macro parameter amount mismatch");
				return {};
			}

			String processed = body.trim();

			for (int i = 0; i < arguments.size(); i++)
			{
				ParserHelpers::TokenIterator t(processed);

				String p;

				while (!t.isEOF())
				{
					auto prev = t.location.location;

					if (t.currentType == JitTokens::identifier)
					{
						auto id = Identifier(t.currentValue.toString());

						if (arguments[i] == id)
						{
							t.skip();
							auto now = t.location.location;
							auto old = String(prev, now);

							p << old.replace(id, parameters[i].trim());
							continue;
						}
							
					}
					
					t.skip();
					auto now = t.location.location;
					p << String(prev, now);
				}

				processed = p;
			}

			return processed;
		}
	};

	template <class T> static T* as(Item::Ptr p)
	{
		return dynamic_cast<T*>(p.get());
	}


	struct TextBlock
	{
		TextBlock(String::CharPointerType program_, String::CharPointerType start_);;

		bool isPreprocessorDirective() const;

		void processError(ParserHelpers::CodeLocation::Error& e)
		{
			if (e.program != program)
			{
				int delta = e.location - e.program;

				e.location = originalLocation + delta;
				e.program = program;
			}
		}

		ParserHelpers::TokenIterator createParser();

		bool is(Token t) const
		{
			return blockType == t;
		}

		Range<int> getLineRange() const
		{
			ParserHelpers::CodeLocation s(originalLocation, program);
			ParserHelpers::CodeLocation e(originalLocation + length, program);

			auto sLine = s.getLine();
			auto eLine = jmax(sLine + 1, e.getLine());

			return { sLine, eLine };
		}

		DEBUG_ONLY(std::string debugString);

		void flush(String::CharPointerType location);
		String toString() const;

		void replaceWithEmptyLines()
		{
			auto l = StringArray::fromLines(toString());

			String s; 

			for (int i = 0; i < l.size(); i++)
				s << "\n";

			setProcessedCode(s);
		}

		String subString(String::CharPointerType location) const;

		String::CharPointerType getEnd() const;

		void throwError(const String& e);

		void setProcessedCode(const String& newCode)
		{
			processedCode = newCode;
			start = processedCode.getCharPointer();
			length = processedCode.length();
		}

		bool replace(String::CharPointerType startRep, String::CharPointerType endRep, const String& newText)
		{
			jassert(startRep - start < length);

			String newCode;

			auto ptr = start;
			auto end = getEnd();

			while (ptr != startRep)
				newCode << *ptr++;

			newCode << newText.trim();

			ptr = endRep;

			while (ptr != end)
			{
				newCode << *ptr++;
			}
			
			setProcessedCode(newCode);
			return false;
		}

	private:

		void parseBlockStart();

		bool parseIfToken(Token t) const;

		Token blockType = "";
		String::CharPointerType originalLocation;
		String::CharPointerType start;
		String::CharPointerType program;
		size_t length = 0;

		String processedCode;

	};

	static juce::String toString(const Array<TextBlock>& blocks);

	Array<TextBlock> parseTextBlocks();

	void parseDefinition(TextBlock& b);

	bool parseCondition(TextBlock& b);

	bool hasDefinitions() const { return !entries.isEmpty(); }

	static bool isConditionToken(const TextBlock& b);

	bool evaluate(TextBlock& b);

	Item::List entries;

	String code;
};



}
}