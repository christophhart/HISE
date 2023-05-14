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
namespace mir {

using namespace juce;


struct SimpleTypeParser
{
	SimpleTypeParser(const String& s, bool includeTemplate_=true);

	MIR_type_t getMirType(bool getPointerIfRef=true) const;
	TypeInfo getTypeInfo() const;
	String getTrailingString() const;
	NamespacedIdentifier parseNamespacedIdentifier();
	NamespacedIdentifier getComplexTypeId();
	FunctionData parseFunctionData();

private:

	bool includeTemplate = true;

	void skipWhiteSpace();
	String parseIdentifier();
	bool matchIf(const char* token);
	String skipTemplate();
	void parse();

	String code;
	String typeId;
	bool isRef = false;
	bool isConst = false;
	bool isStatic = false;
	Types::ID t = Types::ID::Void;
};

struct TypeConverters
{
	static bool forEachChild(const ValueTree& v, const std::function<bool(const ValueTree&)>& f);

	static NamespacedIdentifier String2NamespacedIdentifier(const String& symbol);
	static Types::ID MirType2TypeId(MIR_type_t t);
	static MIR_type_t TypeInfo2MirType(const TypeInfo& t);
	static TypeInfo String2TypeInfo(String t);
	static MIR_type_t String2MirType(String symbol);
	static FunctionData String2FunctionData(String s);
	static String NamespacedIdentifier2MangledMirVar(const NamespacedIdentifier& id);
	static jit::Symbol String2Symbol(const String& symbolCode);
	static MIR_var SymbolToMirVar(const jit::Symbol& s);
	static String FunctionData2MirTextLabel(const FunctionData& f);
	static String MirType2MirTextType(const MIR_type_t& t);
	static String Symbol2MirTextSymbol(const jit::Symbol& s);
	static String TypeInfo2MirTextType(const TypeInfo& t, bool refToPtr=false);
	static String MirTypeAndToken2InstructionText(MIR_type_t type, const String& token);
	static String TemplateString2MangledLabel(const String& templateArgs);

	template <typename T> static constexpr MIR_type_t getMirTypeFromT()
	{
		
		if constexpr (std::is_same<int, T>())
			return MIR_T_I64;
		else if constexpr (std::is_same<void*, T>() || std::is_pointer<T>())
			return MIR_T_P;
		else if constexpr (std::is_same<double, T>())
			return MIR_T_D;
		else if constexpr (std::is_same<float, T>())
			return MIR_T_F;
		else
		{
            return MIR_T_I8;
		}
	}
};


}
}
