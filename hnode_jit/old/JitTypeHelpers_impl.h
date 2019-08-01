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

template <typename T> String JITTypeHelpers::getTypeName() { return getTypeName(typeid(T)); }
String JITTypeHelpers::getTypeName(const String &t) { return String(getTypeForLiteral(t).name()); }


String JITTypeHelpers::getTypeName(const TypeInfo& info)
{
	return info.name();
}

template <typename ActualType, typename ExpectedType>
String JITTypeHelpers::getTypeMismatchErrorMessage()
{
	return getTypeMismatchErrorMessage(typeid(ActualType), typeid(ExpectedType));
}


template <typename ExpectedType>
String JITTypeHelpers::getTypeMismatchErrorMessage(const TypeInfo& actualType)
{
	return getTypeMismatchErrorMessage(actualType, typeid(ExpectedType));
}


String JITTypeHelpers::getTypeMismatchErrorMessage(const TypeInfo& actualType, const TypeInfo& expectedType)
{
	String message = "Type mismatch: ";
	message << getTypeName(actualType);
	message << ", Expected: ";
	message << getTypeName(expectedType);

	return message;
}



TypeInfo JITTypeHelpers::getTypeForLiteral(const String &t)
{
	if (t.endsWithChar('f'))			  return typeid(float);
	else if (t.containsChar('.'))		  return typeid(double);
	else if (t == "true" || t == "false") return typeid(BooleanType);
	else return							  typeid(int);
}


hnode::Types::ID JITTypeHelpers::convertToHnodeType(TypeInfo type)
{
	if (matchesType<float>(type))
		return Types::ID::Float;
	if (matchesType<double>(type))
		return Types::ID::Double;
	if (matchesType<int>(type))
		return Types::ID::Integer;

	return Types::ID::Void;
}


hnode::jit::TypeInfo JITTypeHelpers::convertToTypeInfo(Types::ID type)
{
	if (type == Types::ID::Float)
		return typeid(float);
	if (type == Types::ID::Integer)
		return typeid(int);
	if (type == Types::ID::Double)
		return typeid(double);

	jassertfalse;
	return typeid(int);
}


bool JITTypeHelpers::matchesType(Types::ID hnodeTypeID, const TypeInfo& type2)
{
	if (matchesType<float>(type2) && hnodeTypeID == Types::Float)
		return true;

	if (matchesType<double>(type2) && hnodeTypeID == Types::Double)
		return true;

	if (matchesType<int>(type2) && hnodeTypeID == Types::Integer)
		return true;

	return false;
}


template <typename ExpectedType> bool JITTypeHelpers::matchesType(const TypeInfo& actualType) { return actualType == typeid(ExpectedType); }
template <typename ExpectedType> bool JITTypeHelpers::matchesType(const String& t) { return getTypeForLiteral(t) == typeid(ExpectedType); }

TypeInfo JITTypeHelpers::getTypeForToken(const char* token)
{
	if (String(token) == String(JitTokens::double_)) return typeid(double);
	else if (String(token) == String(JitTokens::int_))  return typeid(int);
	else if (String(token) == String(JitTokens::float_))  return typeid(float);
#if INCLUDE_BUFFERS
	else if (String(token) == String(JitTokens::buffer_)) return typeid(Buffer);
#endif
	else if (String(token) == String(JitTokens::bool_)) return typeid(BooleanType);
	else return typeid(void);
}

template <typename R1, typename R2> bool JITTypeHelpers::is() { return typeid(R1) == typeid(R2); }



} // end namespace jit
} // end namespace hnode
