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

#if SNEX_INCLUDE_NMD_ASSEMBLY
#define NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX
#define NMD_ASSEMBLY_IMPLEMENTATION 
#define NMD_ASSEMBLY_PRIVATE
#include "../snex_core/nmd_assembly.h"

#endif

#pragma once

namespace snex {
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;


void FunctionData::setDefaultParameter(const Identifier& s, const Inliner::Func& expressionBuilder)
{
	auto newDefaultParameter = new DefaultParameter();

	for (auto& a : args)
	{
		if (a.id.getIdentifier() == s)
		{
			newDefaultParameter->id = a;
			break;
		}
	}
	
	newDefaultParameter->expressionBuilder = expressionBuilder;
	defaultParameters.add(newDefaultParameter);
}

juce::String FunctionData::getCodeToInsert() const
{
	juce::String s;

	s << id.id.toString() << "(";

	int n = 0;

	for (auto p : args)
	{
		s << p.typeInfo.toString() << " " << p.id.id;

		if (++n != args.size())
			s << ", ";
	}

	s << ")";
		
	return s;
}

juce::String FunctionData::getSignature(const Array<Identifier>& parameterIds, bool useFullParameterIds) const
{
	juce::String s;

	s.preallocateBytes(256);

	s << returnType.toString(false) << " " << id.toString();
	
	if (!templateParameters.isEmpty())
	{
		s << "<";

		for(int i = 0; i < templateParameters.size(); i++)
		{
			auto t = templateParameters[i];

			if (t.type.isValid())
				s << t.type.toString(false);
			else
				s << juce::String(t.constant);

			if (i == (templateParameters.size() - 1))
				s << ">";
			else
				s << ", ";
		}
	}
	
	s << "(";

	int index = 0;

	for (auto arg : args)
	{
		s << arg.typeInfo.toString(false);

		auto pName = parameterIds[index].toString();

		if (pName.isEmpty())
		{
			if (useFullParameterIds)
				pName = arg.id.toString();
			else
				pName = arg.id.getIdentifier().toString();
		}

		if (pName.isNotEmpty())
			s << " " << pName;
		
		if (++index != args.size())
			s << ", ";
	}

	s << ")";

	return s;
}

bool FunctionData::isConstOrHasConstArgs() const
{
	if (!isConst())
		return false;

	for (auto a : args)
	{
		if (a.isReference() && !a.isConst())
			return false;
	}

	return true;
}

bool FunctionData::hasTemplatedArgumentOrReturnType() const
{
	if (returnType.isTemplateType())
		return true;

	for (auto a : args)
	{
		if (a.typeInfo.isTemplateType())
			return true;
	}
		
	return false;
}

bool FunctionData::hasUnresolvedTemplateParameters() const
{
	for (auto t : templateParameters)
	{
		if (t.t == TemplateParameter::IntegerTemplateArgument || t.t == TemplateParameter::TypeTemplateArgument)
			return true;

		if (t.t == TemplateParameter::ConstantInteger && !t.constantDefined)
			return true;

		if (t.t == TemplateParameter::ParameterType::Type && t.type.isInvalid())
			return true;
	}

	return false;
}



snex::jit::FunctionData FunctionData::withParent(const NamespacedIdentifier& newParent) const
{
	auto copy = *this;
	copy.id = newParent.getChildId(id.getIdentifier());
	return copy;
}

snex::jit::TypeInfo FunctionData::getOrResolveReturnType(ComplexType::Ptr p)
{
	if (returnType.isDynamic())
	{
		ReturnTypeInlineData rt(*this);

		if (inliner != nullptr)
		{
			if (auto st = dynamic_cast<StructType*>(p.get()))
			{
				rt.templateParameters = st->getTemplateInstanceParameters();
				inliner->returnTypeFunction(&rt);
			}
		}
	}

	return returnType;
}

String FunctionData::createAssembly() const
{
#if SNEX_INCLUDE_NMD_ASSEMBLY
	if(numBytes != 0)
	{
		auto bytePtr = (uint8*)function;
		int numToDo = (int)numBytes;

		StringArray lines;

		int bytePos = 0;

		while (numToDo > 0)
		{
			auto instructionLength = nmd_x86_ldisasm(bytePtr, numToDo, NMD_X86_MODE_64);

			if (instructionLength == 0)
				break;

			nmd_x86_instruction instruction;
			auto ok = nmd_x86_decode(bytePtr, numBytes, &instruction, NMD_X86_MODE_64, NMD_X86_DECODER_FLAGS_NONE);

			if (ok)
			{
				char buffer[128];
				memset(buffer, 0, 128);

				auto format = NMD_X86_FORMAT_FLAGS_COMMA_SPACES |
							  NMD_X86_FORMAT_FLAGS_POINTER_SIZE |
							  NMD_X86_FORMAT_FLAGS_0X_PREFIX;
				
				nmd_x86_format(&instruction, buffer, NMD_X86_INVALID_RUNTIME_ADDRESS, format);
				
				auto s = String::createStringFromData(buffer, 128);

				if(!s.startsWith("add byte ptr"))
					lines.add(String(bytePos) + "\t" + s);
			}
			else
			{
				jassertfalse;
				break;
			}

			bytePos += (int)instructionLength;
			bytePtr += (int)instructionLength;
			numToDo -= (int)instructionLength;
		}

		return lines.joinIntoString("\n");
	}
#else
	// You have to define SNEX_INCLUDE_NMD_ASSEMBLY
	jassertfalse;
#endif

	return {};
}

bool FunctionData::matchIdArgs(const FunctionData& other) const
{
	auto idMatch = id == other.id;
	auto argMatch = matchesArgumentTypes(other);
	return idMatch && argMatch;
}

bool FunctionData::matchIdArgsAndTemplate(const FunctionData& other) const
{
	auto idArgMatch = matchIdArgs(other);
	auto templateMatch = matchesTemplateArguments(other.templateParameters);

	return idArgMatch && templateMatch;
}

bool argumentMatch(const TypeInfo& functionArgs, const TypeInfo& actualArgs)
{
	if (functionArgs.isInvalid())
		return true;

	return functionArgs == actualArgs;
}

bool FunctionData::matchesArgumentTypes(const Array<TypeInfo>& typeList, bool checkIfEmpty) const
{
	if (!checkIfEmpty && args.isEmpty())
		return true;

	if (args.size() != typeList.size())
		return false;

	for (int i = 0; i < args.size(); i++)
	{
		auto thisArgs = args[i].typeInfo;
		auto otherArgs = typeList[i];

		if (!argumentMatch(args[i].typeInfo, typeList[i]))
			return false;
	}

	return true;
}

bool FunctionData::matchesArgumentTypes(TypeInfo r, const Array<TypeInfo>& argsList, bool checkIfEmpty) const
{
	if (r != returnType && !returnType.isDynamic())
		return false;

	return matchesArgumentTypes(argsList, checkIfEmpty);
}



bool FunctionData::matchesArgumentTypes(const FunctionData& otherFunctionData, bool checkReturnType /*= true*/) const
{
	if (checkReturnType && otherFunctionData.returnType != returnType)
		return false;

	if (args.size() != otherFunctionData.args.size())
		return false;

	for (int i = 0; i < args.size(); i++)
	{
		auto thisType = args[i].typeInfo;
		auto otherType = otherFunctionData.args[i].typeInfo;

		if (!argumentMatch(thisType, otherType))
			return false;
	}

	return true;
}




bool FunctionData::matchesNativeArgumentTypes(Types::ID r, const Array<Types::ID>& nativeArgList) const
{
	Array<TypeInfo> t;

	for (auto n : nativeArgList)
		t.add(TypeInfo(n, n == Types::ID::Pointer ? true : false));

	return matchesArgumentTypes(TypeInfo(r), t);
}



bool FunctionData::matchesArgumentTypesWithDefault(const Array<TypeInfo>& typeList) const
{
	for (int i = 0; i < args.size(); i++)
	{
		if (isPositiveAndBelow(i, typeList.size()))
		{
			auto thisArgs = args[i].typeInfo;
			auto otherArgs = typeList[i];

			if (!argumentMatch(thisArgs, otherArgs))
				return false;
		}
		else
		{
			if (!hasDefaultParameter(args[i]))
				return false;
		}
	}

	return true;
}

bool FunctionData::hasDefaultParameter(const Symbol& arg) const
{
	for (auto d : defaultParameters)
	{
		if (d->id == arg)
			return true;
	}

	return false;
}

bool FunctionData::isValid() const
{
	return id.isValid();
}

juce::Result FunctionData::validateWithArgs(Types::ID r, const Array<Types::ID>& nativeArgList) const
{
	String d = getSignature();

	if (!isResolved())
		return Result::fail(d + " not found");

	if (args.size() != nativeArgList.size())
	{
		d << " - argument amount mismatch: expected " << String(nativeArgList.size());
		return Result::fail(d);
	}
		
	if (r != returnType.getType())
	{
		d << " - return type mismatch: expected " << Types::Helpers::getTypeName(r);
		return Result::fail(d);
	}

	for (int i = 0; i < nativeArgList.size(); i++)
	{
		auto actualType = args[i].typeInfo.getType();

		if (actualType != nativeArgList[i])
		{
			d << " - " << args[i].id.getIdentifier();
			d << " - expected " << Types::Helpers::getTypeName(nativeArgList[i]) << " type";
			return Result::fail(d);
		}
	}

	return Result::ok();
}

juce::Result FunctionData::validateWithArgs(String returnString, const StringArray& argStrings) const
{
	String d = getSignature();

	if (!isResolved())
		return Result::fail(d + " not found");

	if (args.size() != argStrings.size())
	{
		d << " - argument amount mismatch: expected " << String(argStrings.size());
		return Result::fail(d);
	}

	if (returnString.compare(returnType.toString()) != 0)
	{
		d << " - return type mismatch: expected " << returnString;
		return Result::fail(d);
	}

	for (int i = 0; i < argStrings.size(); i++)
	{
		auto actualType = args[i].typeInfo;

		if (argStrings[i].compare(actualType.toString()) != 0)
		{
			d << " - " << args[i].id.getIdentifier();
			d << " - expected " << argStrings[i] << " type";
			return Result::fail(d);
		}
	}

	return Result::ok();
}

snex::jit::Inliner::Func FunctionData::getDefaultExpression(const Symbol& s) const
{
	for (auto d : defaultParameters)
	{
		if (d->id == s)
			return d->expressionBuilder;
	}

	return {};
}

bool FunctionData::matchesTemplateArguments(const TemplateParameter::List& l) const
{
	if (l.size() != templateParameters.size())
		return false;

	for (int i = 0; i < l.size(); i++)
	{
		if (l[i] != templateParameters[i])
			return false;
	}

	return true;
}

int FunctionData::getSpecialFunctionType() const
{
	for (int i = 0; i < FunctionClass::SpecialSymbols::numOperatorOverloads; i++)
	{
		auto ss = FunctionClass::getSpecialSymbol(id.getParent(), (FunctionClass::SpecialSymbols)i);

		if (ss == id.getIdentifier())
			return i;
	}

	return FunctionClass::SpecialSymbols::numOperatorOverloads;
}



struct VariadicCallHelpers
{
#if JUCE_LINUX
#define variadic_call static
#else
#define variadic_call static
#endif

	template <typename T> static constexpr bool isDynamic()
	{
		return std::is_same<VariableStorage, T>();
	}

	struct VoidFunctions
	{
		variadic_call void call0(const FunctionData& f)
		{
			f.callVoid();
		}

		variadic_call void call1(const FunctionData& f, const VariableStorage& a1)
		{
			using namespace Types;

			switch (a1.getType())
			{
			case ID::Integer:	f.callVoid((int)a1); break;
			case ID::Double:	f.callVoid((double)a1); break;
			case ID::Float:		f.callVoid((float)a1); break;
			case ID::Pointer:	f.callVoid((void*)a1); break;
			}
		}

		variadic_call void call2(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2)
		{
			using namespace Types;

			switch (a1.getType())
			{
			case ID::Integer:	cv2_tv(f, (int)a1, a2); break;
			case ID::Double:	cv2_tv(f, (double)a1, a2); break;
			case ID::Float:		cv2_tv(f, (float)a1, a2); break;
			case ID::Pointer:	cv2_tv(f, (void*)a1, a2); break;
			}
		}

		template <typename T1> variadic_call void cv2_tv(const FunctionData& f, T1 a1, const VariableStorage& a2)
		{
			using namespace Types;

			switch (a2.getType())
			{
			case ID::Integer:	f.callVoid(a1, (int)a2); break;
			case ID::Double:	f.callVoid(a1, (double)a2); break;
			case ID::Float:		f.callVoid(a1, (float)a2); break;
			case ID::Pointer:	f.callVoid(a1, (void*)a2); break;
			}
		}

		variadic_call void call3(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2, const VariableStorage& a3)
		{
			using namespace Types;

			switch (a1.getType())
			{
			case ID::Integer:	cv3_tvv(f, (int)a1, a2, a3); break;
			case ID::Double:	cv3_tvv(f, (double)a1, a2, a3); break;
			case ID::Float:		cv3_tvv(f, (float)a1, a2, a3); break;
			case ID::Pointer:	cv3_tvv(f, (void*)a1, a2, a3); break;
			}
		}

		template <typename T> variadic_call void cv3_tvv(const FunctionData& f, T a1, const VariableStorage& a2, const VariableStorage& a3)
		{
			using namespace Types;

			switch (a2.getType())
			{
			case ID::Integer:	cv3_ttv(f, a1, (int)a2, a3); break;
			case ID::Double:	cv3_ttv(f, a1, (double)a2, a3); break;
			case ID::Float:		cv3_ttv(f, a1, (float)a2, a3); break;
			case ID::Pointer:	cv3_ttv(f, a1, (void*)a2, a3); break;
			}
		}

		template <typename T1, typename T2> variadic_call void cv3_ttv(const FunctionData& f, T1 a1, T2 a2, const VariableStorage& a3)
		{
			using namespace Types;

			switch (a3.getType())
			{
			case ID::Integer:	f.callVoid(a1, a2, a3.toInt()); break;
			case ID::Double:	f.callVoid(a1, a2, a3.toDouble()); break;
			case ID::Float:		f.callVoid(a1, a2, a3.toFloat()); break;
			case ID::Pointer:	f.callVoid(a1, a2, a3.toPtr()); break;
			}
		}


		variadic_call void call4(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2, const VariableStorage& a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (a1.getType())
			{
			case ID::Integer:	cv4_tvvv(f, (int)a1, a2, a3, a4); break;
			case ID::Double:	cv4_tvvv(f, (double)a1, a2, a3, a4); break;
			case ID::Float:		cv4_tvvv(f, (float)a1, a2, a3, a4); break;
			case ID::Pointer:	cv4_tvvv(f, (void*)a1, a2, a3, a4); break;
			}
		}

		template <typename T> variadic_call void cv4_tvvv(const FunctionData& f, T a1, const VariableStorage& a2, const VariableStorage& a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (a2.getType())
			{
			case ID::Integer:	cv4_ttvv(f, a1, (int)a2, a3, a4); break;
			case ID::Double:	cv4_ttvv(f, a1, (double)a2, a3, a4); break;
			case ID::Float:		cv4_ttvv(f, a1, (float)a2, a3, a4); break;
			case ID::Pointer:	cv4_ttvv(f, a1, (void*)a2, a3, a4); break;
			}
		}

		template <typename T1, typename T2> variadic_call void cv4_ttvv(const FunctionData& f, T1 a1, T2 a2, const VariableStorage& a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (a3.getType())
			{
			case ID::Integer:	cv4_tttv(f, a1, a2, (int)a3, a4); break;
			case ID::Double:	cv4_tttv(f, a1, a2, (double)a3, a4); break;
			case ID::Float:		cv4_tttv(f, a1, a2, (float)a3, a4); break;
			case ID::Pointer:	cv4_tttv(f, a1, a2, (void*)a3, a4); break;
			}
		}

		template <typename T1, typename T2, typename T3> variadic_call void cv4_tttv(const FunctionData& f, T1 a1, T2 a2, T3 a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (a4.getType())
			{
			case ID::Integer:	f.callVoid(a1, a2, a3, (int)a4); break;
			case ID::Double:	f.callVoid(a1, a2, a3, (double)a4); break;
			case ID::Float:		f.callVoid(a1, a2, a3, (float)a4); break;
			case ID::Pointer:	f.callVoid(a1, a2, a3, (void*)a4); break;
			}
		}
	};

	struct ReturnFunctions
	{
		variadic_call VariableStorage call0(const FunctionData& f)
		{
			using namespace Types;

			switch (f.returnType.getType())
			{
			case ID::Integer:	return { f.call<int>() };
			case ID::Double:	return { f.call<double>() };
			case ID::Float:		return { f.call<float>() };
			case ID::Pointer:	return { f.call<void*>(), 0 };
			}

			return {};
		}

		variadic_call VariableStorage call1(const FunctionData& f, const VariableStorage& a1)
		{
			using namespace Types;

			switch (f.returnType.getType())
			{
			case ID::Integer:	return { c1_v<int>(f, a1) };
			case ID::Double:	return { c1_v<double>(f, a1) };
			case ID::Float:		return { c1_v<float>(f, a1) };
			case ID::Pointer:	return { c1_v<void*>(f, a1), 0 };
			}

			return {};
		}

		template <typename R> variadic_call R c1_v(const FunctionData& f, const VariableStorage& a1)
		{
			using namespace Types;

			switch (a1.getType())
			{
			case ID::Integer:	return f.call<R>((int)a1);
			case ID::Double:	return f.call<R>((double)a1);
			case ID::Float:		return f.call<R>((float)a1);
			case ID::Pointer:	return f.call<R>((void*)a1);
			}

			return R();
		}

		variadic_call VariableStorage call2(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2)
		{
			using namespace Types;

			switch (f.returnType.getType())
			{
			case ID::Integer:	return { c2_vv<int>(f, a1, a2) };
			case ID::Double:	return { c2_vv<double>(f, a1, a2) };
			case ID::Float:		return { c2_vv<float>(f, a1, a2) };
			case ID::Pointer:	return { c2_vv<void*>(f, a1, a2), 0 };
			}

			return {};
		}

		template <typename R> variadic_call R c2_vv(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2)
		{
			using namespace Types;

			switch (a1.getType())
			{
			case ID::Integer:	return c2_tv<R>(f, (int)a1, a2);
			case ID::Double:	return c2_tv<R>(f, (double)a1, a2);
			case ID::Float:		return c2_tv<R>(f, (float)a1, a2);
			case ID::Pointer:	return c2_tv<R>(f, (void*)a1, a2);
			}

			return R();
		}

		template <typename R, typename T1> variadic_call R c2_tv(const FunctionData& f, T1 a1, const VariableStorage& a2)
		{
			using namespace Types;

			switch (a2.getType())
			{
			case ID::Integer:	return f.call<R>(a1, (int)a2);
			case ID::Double:	return f.call<R>(a1, (double)a2);
			case ID::Float:		return f.call<R>(a1, (float)a2);
			case ID::Pointer:	return f.call<R>(a1, (void*)a2);
			}

			return R();
		}

		variadic_call VariableStorage call3(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2, const VariableStorage& a3)
		{
			using namespace Types;

			switch (f.returnType.getType())
			{
			case ID::Integer:	return { c3_vvv<int>(f, a1, a2, a3) };
			case ID::Double:	return { c3_vvv<double>(f, a1, a2, a3) };
			case ID::Float:		return { c3_vvv<float>(f, a1, a2, a3) };
			case ID::Pointer:	return { c3_vvv<void*>(f, a1, a2, a3), 0 };
			}

			return {};
		}

		template <typename R> variadic_call R c3_vvv(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2, const VariableStorage& a3)
		{
			using namespace Types;

			switch (a1.getType())
			{
			case ID::Integer:	return c3_tvv<R>(f, (int)a1, a2, a3);
			case ID::Double:	return c3_tvv<R>(f, (double)a1, a2, a3);
			case ID::Float:		return c3_tvv<R>(f, (float)a1, a2, a3);
			case ID::Pointer:	return c3_tvv<R>(f, (void*)a1, a2, a3);
			}

			return R();
		}

		template <typename R, typename T1> variadic_call R c3_tvv(const FunctionData& f, T1 a1, const VariableStorage& a2, const VariableStorage& a3)
		{
			using namespace Types;

			switch (a2.getType())
			{
			case ID::Integer:	return c3_ttv<R>(f, a1, (int)a2, a3);
			case ID::Double:	return c3_ttv<R>(f, a1, (double)a2, a3);
			case ID::Float:		return c3_ttv<R>(f, a1, (float)a2, a3);
			case ID::Pointer:	return c3_ttv<R>(f, a1, (void*)a2, a3);
			}

			return R();
		}

		template <typename R, typename T1, typename T2> variadic_call R c3_ttv(const FunctionData& f, T1 a1, T2 a2, const VariableStorage& a3)
		{
			using namespace Types;

			switch (a3.getType())
			{
			case ID::Integer:	return f.call<R>(a1, a2, (int)a3);
			case ID::Double:	return f.call<R>(a1, a2, (double)a3);
			case ID::Float:		return f.call<R>(a1, a2, (float)a3);
			case ID::Pointer:	return f.call<R>(a1, a2, (void*)a3);
			}

			return R();
		}

		variadic_call VariableStorage call4(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2, const VariableStorage& a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (f.returnType.getType())
			{
			case ID::Integer:	return { c4_vvvv<int>(f, a1, a2, a3, a4) };
			case ID::Double:	return { c4_vvvv<double>(f, a1, a2, a3, a4) };
			case ID::Float:		return { c4_vvvv<float>(f, a1, a2, a3, a4) };
			case ID::Pointer:	return { c4_vvvv<void*>(f, a1, a2, a3, a4), 0 };
			}

			return {};
		}

		template <typename R> variadic_call R c4_vvvv(const FunctionData& f, const VariableStorage& a1, const VariableStorage& a2, const VariableStorage& a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (a1.getType())
			{
			case ID::Integer:	return c4_tvvv<R>(f, (int)a1, a2, a3, a4);
			case ID::Double:	return c4_tvvv<R>(f, (double)a1, a2, a3, a4);
			case ID::Float:		return c4_tvvv<R>(f, (float)a1, a2, a3, a4);
			case ID::Pointer:	return c4_tvvv<R>(f, (void*)a1, a2, a3, a4);
			}

			return R();
		}

		template <typename R, typename T1> variadic_call R c4_tvvv(const FunctionData& f, T1 a1, const VariableStorage& a2, const VariableStorage& a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (a2.getType())
			{
			case ID::Integer:	return c4_ttvv<R>(f, a1, (int)a2, a3, a4);
			case ID::Double:	return c4_ttvv<R>(f, a1, (double)a2, a3, a4);
			case ID::Float:		return c4_ttvv<R>(f, a1, (float)a2, a3, a4);
			case ID::Pointer:	return c4_ttvv<R>(f, a1, (void*)a2, a3, a4);
			}

			return R();
		}

		template <typename R, typename T1> variadic_call R c4_ttvv(const FunctionData& f, T1 a1, const VariableStorage& a2, const VariableStorage& a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (a3.getType())
			{
			case ID::Integer:	return c4_ttvv<R>(f, a1, a2, (int)a3, a4);
			case ID::Double:	return c4_ttvv<R>(f, a1, a2, (double)a3, a4);
			case ID::Float:		return c4_ttvv<R>(f, a1, a2, (float)a3, a4);
			case ID::Pointer:	return c4_ttvv<R>(f, a1, a2, (void*)a3, a4);
			}

			return R();
		}

		template <typename R, typename T1> variadic_call R c4_tttv(const FunctionData& f, T1 a1, const VariableStorage& a2, const VariableStorage& a3, const VariableStorage& a4)
		{
			using namespace Types;

			switch (a4.getType())
			{
			case ID::Integer:	return f.call<R>(a1, a2, a3, (int)a4);
			case ID::Double:	return f.call<R>(a1, a2, a3, (double)a4);
			case ID::Float:		return f.call<R>(a1, a2, a3, (float)a4);
			case ID::Pointer:	return f.call<R>(a1, a2, a3, (void*)a4);
			}

			return R();
		}
	};
};

void FunctionData::callVoidDynamic(VariableStorage* args, int numArgs) const
{
	switch (numArgs)
	{
	case 0: callVoid(); break;
	case 1: VariadicCallHelpers::VoidFunctions::call1(*this, args[0]); break;
	case 2: VariadicCallHelpers::VoidFunctions::call2(*this, args[0], args[1]); break;
	case 3: VariadicCallHelpers::VoidFunctions::call3(*this, args[0], args[1], args[2]); break;
	case 4: VariadicCallHelpers::VoidFunctions::call4(*this, args[0], args[1], args[2], args[3]); break;
	}
}

snex::VariableStorage FunctionData::callDynamic(VariableStorage* args, int numArgs) const
{
	switch (numArgs)
	{
	case 0: return VariadicCallHelpers::ReturnFunctions::call0(*this);
	case 1: return VariadicCallHelpers::ReturnFunctions::call1(*this, args[0]);
	case 2: return VariadicCallHelpers::ReturnFunctions::call2(*this, args[0], args[1]);
	case 3: return VariadicCallHelpers::ReturnFunctions::call3(*this, args[0], args[1], args[2]);
	case 4: return VariadicCallHelpers::ReturnFunctions::call4(*this, args[0], args[1], args[2], args[3]);
	default:
		jassertfalse;
		return {};
	}
}

ExternalTypeParser::ExternalTypeParser(String::CharPointerType location, String::CharPointerType wholeProgram) :
	parseResult(Result::ok()),
	l(nullptr)
{
	NamespaceHandler nh;
	ParserHelpers::TokenIterator it(location, wholeProgram, wholeProgram - location);
	TypeParser tp(it, nh, {});

	try
	{
		tp.matchType();
		type = tp.currentTypeInfo;
		parseResult = Result::ok();
		l = tp.location.location;
	}
	catch (ParserHelpers::Error& s)
	{
		parseResult = Result::fail(s.toString());
		type = {};
		l = nullptr;
	}
}



} // end namespace jit
} // end namespace snex

