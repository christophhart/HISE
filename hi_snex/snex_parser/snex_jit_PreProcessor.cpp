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

using String = juce::String;



Preprocessor::TextBlock::TextBlock(String::CharPointerType program_, String::CharPointerType start_) :
	start(start_),
	program(program_),
	originalLocation(start_)
{

}


bool Preprocessor::TextBlock::parseIfToken(Token t) const
{
	if (!isPreprocessorDirective())
		return false;

	return toString().startsWith(t);

}

bool Preprocessor::TextBlock::isPreprocessorDirective() const
{
	return blockType != PreprocessorTokens::code_;
}

snex::jit::ParserHelpers::TokenIterator Preprocessor::TextBlock::createParser()
{
	setProcessedCode(toString());

	ParserHelpers::TokenIterator it(processedCode);
	return it;
}

void Preprocessor::TextBlock::parseBlockStart()
{
	if (*start == '#')
	{
#define MATCH_TOKEN(x) if (parseIfToken(x)) blockType = x;
		MATCH_TOKEN(PreprocessorTokens::define_);
		MATCH_TOKEN(PreprocessorTokens::include_);
		MATCH_TOKEN(PreprocessorTokens::elif_);
		MATCH_TOKEN(PreprocessorTokens::if_);
		MATCH_TOKEN(PreprocessorTokens::else_);
		MATCH_TOKEN(PreprocessorTokens::endif_);
		MATCH_TOKEN(PreprocessorTokens::undef_);
#undef MATCH_TOKEN(x);

		auto tokenLength = String(blockType).length();

		start += tokenLength;
		length -= tokenLength;

		auto s = toString();

		if (s.containsChar('\\'))
		{
			s = s.removeCharacters("\\");
			setProcessedCode(s);
		}
	}
	else
		blockType = PreprocessorTokens::code_;
}

void Preprocessor::TextBlock::flush(String::CharPointerType location)
{
	length = location - start;
	DEBUG_ONLY(debugString = toString().toStdString());

	parseBlockStart();
}

String Preprocessor::TextBlock::toString() const
{
	return { start, length };
}

String Preprocessor::TextBlock::subString(String::CharPointerType location) const
{
	if (*location)
	{
		jassert((location - start) < (int)length);
		auto end = start + length;
		return { location, end };
	}

	return {};
}

String::CharPointerType Preprocessor::TextBlock::getEnd() const
{
	return start + length;
}

void Preprocessor::TextBlock::throwError(const String& error)
{
	ParserHelpers::CodeLocation l(originalLocation, program);
	ParserHelpers::Error e(l);
	e.errorMessage = error;
	throw e;
}

Preprocessor::Preprocessor(const juce::String& code_) :
	code(code_)
{

}

String Preprocessor::process()
{
	auto blocks = parseTextBlocks();

	parseTextBlocks();

	Array<bool> conditions;
	
	for (int i = 0; i < blocks.size(); i++)
	{
		auto& b = blocks.getReference(i);

		if (!conditions.isEmpty() && !conditions.getLast())
		{
			if (!isConditionToken(b))
			{
				// doesn't matter, always false...
				if (b.is(PreprocessorTokens::if_))
					conditions.add(false);

				bool deactivate = true;

				if (b.is(PreprocessorTokens::endif_))
				{
;					conditions.removeLast();
					deactivate = !conditions.isEmpty() && !conditions.getLast();
				}
				
				if(deactivate)
					deactivatedLines.addRange(b.getLineRange() + 1);

				blocks.getReference(i).replaceWithEmptyLines();
				continue;
			}
		}

		try
		{
			auto done = evaluate(b);

			while (!done)
				done = evaluate(b);

			if (b.is(PreprocessorTokens::define_))
				parseDefinition(b);
			if (b.is(PreprocessorTokens::if_))
			{
				auto value = parseCondition(b);
				conditions.add(value);
			}
			if (b.is(PreprocessorTokens::elif_))
			{
				if (conditions.isEmpty())
					b.throwError("Can't use #elif without #if");

				auto value = parseCondition(b);
				conditions.removeLast();
				conditions.add(value);
			}
			if (b.is(PreprocessorTokens::endif_))
			{
				conditions.removeLast();
			}
			if (b.is(PreprocessorTokens::else_))
			{
				if (conditions.isEmpty())
					b.throwError("Can't use #else without #if");

				auto value = !conditions.getLast();
				conditions.removeLast();
				conditions.add(value);
			}
		}
		catch (ParserHelpers::Error& e)
		{
			if (!conditionMode)
			{
				b.processError(e);
				throw e;
			}
		}
	}

	if (!conditionMode && !conditions.isEmpty()) 
	{
		ParserHelpers::CodeLocation l(code.getCharPointer() + code.length(), code.getCharPointer());
		ParserHelpers::Error e(l);
		e.errorMessage = "missing #endif";
		throw e;
	}

	return toString(blocks);
}

void Preprocessor::parseDefinition(TextBlock& b)
{
	auto p = b.createParser();

	auto id = NamespacedIdentifier(p.parseIdentifier());

	Item::Ptr newItem;

	if (p.matchIf(JitTokens::openParen))
	{
		Array<Identifier> args;

		while (!p.isEOF() && p.currentType != JitTokens::closeParen)
		{
			args.add(p.parseIdentifier());
			p.matchIf(JitTokens::comma);
		}

		

		p.match(JitTokens::closeParen);
		
		newItem = new Macro(args);
	}
	else
	{
		newItem = new Definition();
	}

	newItem->id = id;
	newItem->lineNumber = b.getLineRange().getStart();

	if (p.location.location)
	{
		newItem->body = b.subString(p.location.location);
		entries.add(newItem);
	}
}

bool Preprocessor::evaluate(TextBlock& b)
{
	if (!hasDefinitions())
		return true;

	auto p = b.createParser();

	if (conditionMode && b.is(PreprocessorTokens::code_))
		return true;

	while (!p.isEOF())
	{
		if (!conditionMode && p.currentType == JitTokens::identifier)
		{
			auto macroStart = p.location.location;

			auto id = p.parseIdentifier();

			auto macroEnd = macroStart + id.toString().length();

			for (auto e : entries)
			{
				if (e->id.getIdentifier() == id)
				{
					String after;

					if (auto def = as<Definition>(e))
					{
						after = def->body;
					}
					if (auto m = as<Macro>(e))
					{
						auto parameters = p.parsePreprocessorParameterList();
						macroEnd = p.location.location;

						auto r = Result::ok();
						after = m->evaluate(parameters, r);
						p.location.test(r);
					}

					return b.replace(macroStart, macroEnd, after);
				}
			}
		}
		else
		{
			p.skip();
		}
	}

	return true;
}

bool Preprocessor::parseCondition(TextBlock& b)
{
	auto cond = b.toString().trim();
	auto r = Result::ok();

	// fuck yeah, Javascript to the rescue...
	juce::JavascriptEngine e;
	auto result = e.evaluate(cond, &r);

	return (bool)result;
}

bool Preprocessor::isConditionToken(const TextBlock& b)
{
	return b.is(PreprocessorTokens::else_) || b.is(PreprocessorTokens::elif_);
}

Array<Preprocessor::TextBlock> Preprocessor::parseTextBlocks()
{
	Array<TextBlock> blocks;

	auto end = code.getCharPointer() + code.length();
	auto start = code.getCharPointer();
	auto currentLine = start;
	auto defaultNewLine = NewLine::getDefault();
	uint8 firstNewLineChar = '\n';
	auto lineNumber = 0;

	while (start != end)
	{
		while (start != end)
		{
			if (CharacterFunctions::isWhitespace(*start))
			{
				if (*start == firstNewLineChar)
					lineNumber++;

				start++;
			}
			else
				break;
		}

		auto blockStart = lineNumber;

		TextBlock currentBlock(code.getCharPointer(), start);
		bool isPreprocessor = *start == '#';
		auto breakCharacter = isPreprocessor ? '\n' : '#';

		while (start < end)
		{
			if (*start == '/')
			{
				auto n = *(start + 1);
				if (n == '*')
				{
					auto endOfComment = CharacterFunctions::find(start, CharPointer_ASCII("*/"));
					
					if (endOfComment.isEmpty())
						currentBlock.throwError("Unterminated comment");

					while (start != endOfComment)
					{
						if (*start == firstNewLineChar)
							lineNumber++;

						start++;
					}

					start++;
				}
				else if (n == '/')
				{
					while (start != end)
					{
						if (*start == firstNewLineChar)
						{
							lineNumber++;
							break;
						}

						start++;
					}
				}
			}
            
			if (start >= end)
				break;

			start++;

			if (*start == firstNewLineChar)
				lineNumber++;

			if (isPreprocessor && *start == '\\')
			{
				start++;

				while (start != end && CharacterFunctions::isWhitespace(*start))
					start++;
			}

			if (*start == breakCharacter)
			{
				if (isPreprocessor) // go to next line
					start++;

				break;
			}
		}

		while (start != end)
		{
			if (CharacterFunctions::isWhitespace(*start))
			{
				if (*start == firstNewLineChar)
					lineNumber++;

				start++;
			}

			else
				break;
		}

		currentBlock.setLineRange(blockStart, lineNumber);

		currentBlock.flush(start);
		blocks.add(currentBlock);
	}

	

	return blocks;
}

juce::String Preprocessor::toString(Array<TextBlock>& blocks)
{
	String s;

	for (auto& b : blocks)
	{
		if (!b.isPreprocessorDirective())
			s << b.toString();
		else
		{
			b.replaceWithEmptyLines();

			auto l = b.toString().substring(1);

			s << l;
		}
	}

	return s;
}

juce::Array<Preprocessor::AutocompleteData> Preprocessor::getAutocompleteData() const
{
	Array<AutocompleteData> l;

	for (auto i : entries)
	{
		if (auto ad = i->getAutocompleteData())
		{
			l.add(ad);
		}
	}

	return l;
}

void Preprocessor::addDefinitionsFromScope(const ExternalPreprocessorDefinition::List& l)
{
	for (const auto& e : l)
	{
		if (e.t == ExternalPreprocessorDefinition::Type::Macro)
		{
			auto name = e.name.upToFirstOccurrenceOf("(", false, false);
			auto args = e.name.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false);

			Array<Identifier> ids;

			for (auto s : StringArray::fromTokens(args, ",", ""))
				ids.add(s.trim());

			Item::Ptr newItem = new Macro(ids);
			newItem->body = e.value;
			newItem->id = NamespacedIdentifier(name);
			newItem->lineNumber = -1;
			newItem->description = e.description;

			entries.add(newItem);
		}
		else if (e.t == ExternalPreprocessorDefinition::Type::Empty)
		{
			Item::Ptr newItem = new Item();
			newItem->body == "";
			newItem->id = NamespacedIdentifier(e.name);
			newItem->lineNumber = -1;
			newItem->description = "Resolves to nothing (most likely for C++ compatibility)";
			entries.add(newItem);
		}
		else
		{
			Item::Ptr newItem = new Definition();
			newItem->body << e.value;
			newItem->id = NamespacedIdentifier(e.name);
			newItem->lineNumber = -1;
			newItem->description = e.description;
			entries.add(newItem);
		}
	}
}

juce::SparseSet<int> Preprocessor::getDeactivatedLines()
{
	try
	{
		conditionMode = true;
		process();
		jassert(conditionMode);
	}
	catch (ParserHelpers::Error& e)
	{
		DBG(e.toString());
	}

	return deactivatedLines;
}

String Preprocessor::Macro::evaluate(StringArray& parameters, Result& r)
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

}
}