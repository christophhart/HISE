/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;


struct CppGen
{
	enum class CodeLocation
	{
		Definitions,
		PrepareBody,
		ProcessBody,
		ProcessSingleBody,
		PrivateMembers,
		HandleModulationBody,
		CreateParameters,
		numCodeLocation
	};

	struct MethodInfo
	{
		String returnType;
		String name;
		String specifiers;
		StringArray arguments;
		String body;
		bool addSemicolon = false;
	};

	struct Emitter
	{
		static void emitDefinition(String& s, const String& definition, const String& value, bool useQuotes = true);

		static void emitConstexprVariable(String& s, const String& variableName, int value);
		static void emitArgumentList(String& s, const StringArray& args);
		static void emitFunctionDefinition(String& s, const MethodInfo& f);
		static void emitCommentLine(String& code, int tabLevel, const String& comment);
		static void emitClassEnd(String& s);
		static String createLine(String& content);
		static String createDefinition(String& name, String& assignment);

		static String createRangeString(NormalisableRange<double> range);

		static String surroundWithBrackets(const String& s);

		static String createClass(const String& content, const String& classId, bool outerClass);

		static String createPrettyNumber(double value, bool createFloat);
	};

	struct Helpers
	{
		static String createIntendation(const String& code);	
	};

};




}