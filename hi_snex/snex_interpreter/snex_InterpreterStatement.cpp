/* ===========================================================================
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
namespace interpreter {

Statement::Ptr Statement::Factory::create(const ValueTree& v)
{
	if (createFunctions.find(v.getType()) != createFunctions.end())
	{
		if (auto t = createFunctions.at(v.getType())(*this, v))
		{
			createdNodes.flatList.push_back({ v, t });
			return t;
		}
	}

	return nullptr;
}

struct Statement::Helpers
{
	static void annotateType(UriLocationTree& d, const jit::TypeInfo& type)
	{
		DynamicObject::Ptr ti = new DynamicObject();
		
		if (type.isComplexType())
			ti->setProperty("type", type.getComplexType().get());
		else
			ti->setProperty("type", Types::Helpers::getTypeName(type.getType()));

		ti->setProperty("isConst", type.isConst());
		ti->setProperty("isRef", type.isRef());
		ti->setProperty("isStatic", type.isStatic());
		ti->setProperty("label", type.toString());

		d.data->setProperty("typeInfo", ti.get());
	}

	static UriLocation createUriLocation(Factory& f, const ValueTree& v)
	{
		return f.locationBuilder->createLocation(v);
	}

	static jit::Symbol getSymbol(Factory& f, const ValueTree& astNode, const Identifier& id = Identifier())
	{
		auto idToUse = id.isValid() ? id : AstIds::Symbol;
		auto symbolString = astNode[idToUse].toString();
		TypeParserBase p(symbolString, true, f.typeLibrary);
		jit::Symbol s;
		s.typeInfo = p.getTypeInfo();
		s.id = p.parseNamespacedIdentifier();
		return s;
	}

	static TypeInfo getType(Factory& f, const ValueTree& astNode, const Identifier& id = Identifier())
	{
		auto idToUse = id.isValid() ? id : AstIds::Type;
		auto typeString = astNode[idToUse].toString();
		TypeParserBase p(typeString, true, f.typeLibrary);
		return p.getTypeInfo();
	}

	static FunctionData getFunctionData(Factory& f, const ValueTree& astNode)
	{
		auto signature = astNode[AstIds::Signature].toString();
		TypeParserBase p(signature, true, f.typeLibrary);
		return p.parseFunctionData();
	}

	static Expression::ExprPtr getChildExpression(Factory& f, const ValueTree& astNode, int childIndex)
	{
		auto cn = astNode.getChild(childIndex);

		if (cn.isValid())
		{
			auto s = f.create(cn);

			if (auto e = dynamic_cast<Expression*>(s.get()))
				return e;
		}

		return nullptr;
	}
};

} // namespace interpreter
} // namespace snex
