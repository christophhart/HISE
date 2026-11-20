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
			return t;
	}

	return nullptr;
}

struct Statement::OpFunctions
{
	struct BinaryOp
	{
		typedef VariableStorage(*Function)(const VariableStorage&, const VariableStorage&);

		static Function getFunctionPtr(const TypeInfo& t, juce_wchar opMode)
		{
			if (opMode == '=')
				return nullptr;

			auto opType = t.getType();

			if (opType == Types::ID::Integer)
			{
				switch (opMode)
				{
				case '+': return opFunction<Types::ID::Integer, '+'>;
				case '*': return opFunction<Types::ID::Integer, '*'>;
				case '-': return opFunction<Types::ID::Integer, '-'>;
				case '/': return opFunction<Types::ID::Integer, '/'>;
				}
			}
			if (opType == Types::ID::Double)
			{
				switch (opMode)
				{
				case '+': return opFunction<Types::ID::Double, '+'>;
				case '*': return opFunction<Types::ID::Double, '*'>;
				case '-': return opFunction<Types::ID::Double, '-'>;
				case '/': return opFunction<Types::ID::Double, '/'>;
				}
			}
			if (opType == Types::ID::Float)
			{
				switch (opMode)
				{
				case '+': return opFunction<Types::ID::Float, '+'>;
				case '*': return opFunction<Types::ID::Float, '*'>;
				case '-': return opFunction<Types::ID::Float, '-'>;
				case '/': return opFunction<Types::ID::Float, '/'>;
				}
			}

			jassertfalse;
			return nullptr;
		}

	private:

		template <Types::ID T, char OpType> static VariableStorage opFunction(const VariableStorage& v1, const VariableStorage& v2)
		{
			switch (OpType)
			{
			case '+': return VariableStorage(v1.toType<T>() + v2.toType<T>());
			case '*': return VariableStorage(v1.toType<T>() * v2.toType<T>());
			case '-': return VariableStorage(v1.toType<T>() - v2.toType<T>());
			case '/': return VariableStorage(v1.toType<T>() / v2.toType<T>());
			}
		}
	};

	struct Cast
	{
		typedef VariableStorage(*Function)(const VariableStorage&);

		Function static getFunctionPtr(const TypeInfo& t)
		{
			switch (t.getType())
			{
			case Types::ID::Integer: return castStatic<Types::ID::Integer>;
			case Types::ID::Double:  return castStatic<Types::ID::Double>;
			case Types::ID::Float:   return castStatic<Types::ID::Float>;
			default:				 jassertfalse; return {};
			}
		}

	private:

		template <Types::ID TargetType> static VariableStorage castStatic(const VariableStorage& source)
		{
			return VariableStorage(source.template toType<TargetType>());
		}
	};

	struct Data
	{
		using Address = std::pair<void*, size_t>;
		typedef VariableStorage(*ReadFunction)(Address);
		typedef void(*WriteFunction)(Address, const VariableStorage&);
		using Functions = std::pair<ReadFunction, WriteFunction>;

		static Functions getFunctionPtr(const TypeInfo& t)
		{
			Functions f;

			switch (t.getType())
			{
			case Types::ID::Integer:
				f.first = readStatic<int>;
				f.second = writeStatic<int>;
				break;
			case Types::ID::Float:
				f.first = readStatic<float>;
				f.second = writeStatic<float>;
				break;
			case Types::ID::Double:
				f.first = readStatic<double>;
				f.second = writeStatic<double>;
				break;
			case Types::ID::Pointer:
				f.first = readStatic<void*>;
				f.second = writeStatic<void*>;
				break;
			default:
				jassertfalse;
			}

			return f;
		}

	private:

		template <typename T> static VariableStorage readStatic(Address d)
		{
			auto [data, offset] = d;
			auto ptr = reinterpret_cast<uint8*>(data) + offset;

			if constexpr (std::is_same<void*, T>())
				return VariableStorage(reinterpret_cast<void*>(ptr));
			else
				return VariableStorage(*reinterpret_cast<T*>(ptr));
		}

		template <typename T> static void writeStatic(Address d, const VariableStorage& value)
		{
			auto [data, offset] = d;
			static constexpr auto TID = Types::Helpers::getTypeFromTypeId<T>();

			auto ptr = reinterpret_cast<uint8*>(data) + offset;

			T v = value.template toType<TID>();
			*reinterpret_cast<T*>(ptr) = v;
		}
	};

	

	static String getSigString(const jit::FunctionData& functionData, bool addThisPointer)
	{
		String sig;

		auto getChar = [](const TypeInfo& t)
		{
			switch (t.getType())
			{
			case Types::ID::Void:    return 'v';
			case Types::ID::Integer: return 'i';
			case Types::ID::Float:   return 'f';
			case Types::ID::Double:  return 'd';
			case Types::ID::Pointer: return 'p';
			default:                 return '?';
			}
		};

		sig << getChar(functionData.returnType);

		if (addThisPointer)
			sig << 'p';

		for (const auto& a : functionData.args)
			sig << getChar(a.typeInfo);

		return sig;
	}
};

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

	static UriLocation createUriLocation(UriLocationBuilder::Ptr builder, const ValueTree& v)
	{
		return builder->createLocation((int)v[AstIds::charIndex]);
	}

	static NamespacedIdentifier getNamespacedIdentifier(const ValueTree& astNode, const Identifier& id)
	{
		return NamespacedIdentifier::fromString(astNode[id].toString());
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

	static juce_wchar getOpType(const ValueTree& astNode, const Identifier& id = Identifier())
	{
		auto idToUse = id.isValid() ? id : AstIds::OpType;
		return astNode[idToUse].toString()[0];
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

	static MemoryBlock getInitData(const ValueTree& v)
	{
		MemoryBlock mb;
		mb.fromBase64Encoding(v[AstIds::InitValuesB64].toString());
		return mb;
	}

	static Expression::ExprPtr getChildExpression(Statement* parent, int childIndex)
	{
		if (auto cn = parent->childStatements[childIndex])
			return cn->getTyped<Expression>();

		return nullptr;
	}
};

struct Statement::BlockExecutor
{
	BlockExecutor(Factory& f, Statement& parent_) :
		parent(parent_)
	{
		for (auto c : parent.childStatements)
			locations.push_back(Helpers::createUriLocation(f.locationBuilder, c->astNode));
	}

	ResultCode perform(Scope* s, VariableStorage* returnValue)
	{
		for (int i = instructionPointer; i < parent.childStatements.size(); i++)
		{
			auto c = parent.childStatements.getUnchecked(i);

			try
			{
				if (auto r = c->perform(s, returnValue))
					return r;
			}
			catch (Engine::CallError& ne)
			{
				auto callstackLocation = locations[i];
				ne.e.callstack.push_back({ ne.callstackId, callstackLocation });
				throw ne.e;
			}
		}

		return ResultCode::ok;
	}

	Statement& parent;
	int instructionPointer = 0;
	std::vector<UriLocation> locations;
};


void Statement::throwRuntimeError(const String& errorMessage)
{
	UriError e;
	e.fillLocation(Helpers::createUriLocation(locationBuilder, astNode));
	e.errorType = UriError::ErrorType::Error;
	e.errorMessage = errorMessage;
	throw e;
}

} // namespace interpreter
} // namespace snex
