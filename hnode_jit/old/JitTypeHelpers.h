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

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;


struct JITTypeHelpers
{
	/** Returns a pretty name for the given Type. */
	template <typename T> static String getTypeName();

	/** Returns a pretty name for the given String literal. */
	static String getTypeName(const String &t);

	static String getTypeName(const TypeInfo& info);

	/** Creates a error message if the types don't match. */
	template <typename ActualType, typename ExpectedType> static String getTypeMismatchErrorMessage();

	template <typename ExpectedType> static String getTypeMismatchErrorMessage(const TypeInfo& actualType);

	static String getTypeMismatchErrorMessage(const TypeInfo& type1, const TypeInfo& type2);

	/** Returns the type ID for the given String literal. Currently supported: double, float & int. */
	static TypeInfo getTypeForLiteral(const String &t);;

	/** Checks if the given type ID matches the expected type. */
	template <typename ExpectedType> static bool matchesType(const TypeInfo& actualType);

	/** Checks if the given string matches the expected type. */
	template <typename ExpectedType> static bool matchesType(const String& t);;

	static bool matchesType(Types::ID hnodeTypeID, const TypeInfo& type2);

	/** Checks if the given token matches the type. */
	template <typename ExpectedType> static bool matchesToken(const char* token)
	{
		return matchesType<ExpectedType>(getTypeForToken(token));
	}

	template <typename ExpectedType> static bool matchesToken(const Identifier& tokenName)
	{
		return matchesType<ExpectedType>(getTypeForToken(tokenName.toString().getCharPointer()));
	}

	static uint32 getAsmJitType(Types::ID type)
	{
		if(type == Types::ID::Float) return asmjit::TypeIdOf<float>::kTypeId;
		if (type == Types::ID::Double) return asmjit::TypeIdOf<double>::kTypeId;
		if (type == Types::ID::Integer) return asmjit::TypeIdOf<int>::kTypeId;
	}

	static Types::ID convertToHnodeType(TypeInfo type);
	static TypeInfo convertToTypeInfo(Types::ID type);

	static TypeInfo getTypeForToken(const char* token);

	/** Compares two types. */
	template <typename R1, typename R2> static bool is();;
};

#define TYPE_MATCH(typeclass, typeinfo) JITTypeHelpers::matchesType<typeclass>(typeinfo)


} // end namespace jit
} // end namespace hnode

