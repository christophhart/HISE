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

namespace nodes {
struct Function;
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

	static ComplexType* getObjectType(Factory& f, const ValueTree& astNode)
	{
		auto ti = getType(f, astNode, AstIds::ObjectType);

		if (ti.isComplexType())
			return ti.getComplexType();

		return nullptr;
	}

	static TypeInfo getType(Factory& f, const ValueTree& astNode, const Identifier& id = Identifier())
	{
		auto idToUse = id.isValid() ? id : AstIds::Type;
		auto typeString = astNode[idToUse].toString();

		if (typeString.isEmpty())
			return TypeInfo();

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

	static void allocateReturnType(const Expression& e, snex::Types::heap<uint8>& data)
	{
		auto ti = e.getTypeInfo();

		if (ti.isComplexType() && !ti.isRef())
		{
			auto numBytes = ti.getRequiredByteSize();
			// pad to 16 byte boundary
			numBytes += (16 - (numBytes % 16));
			data.setSize(numBytes);
		}
		else
			data.setSize(0);
	}
};

#define INLINER(Class, method) { #Class ## "::" ## #method, Class::method }
#define INLINER_NC(Class, C, method) { #Class##"<"## #C ##">::" ## #method, Class<C>::method }
#define INLINER_16CH(Class, method) INLINER_NC(Class, 1, method), INLINER_NC(Class, 2, method), \
									INLINER_NC(Class, 3, method), INLINER_NC(Class, 4, method)

#define SNEX_GETTER(ClassName, C) static snex::Types::ClassName<C>& get(const Args& a) { return a.template getTypedObject<snex::Types::ClassName<C>>(); }

#define CASE(O, TYPE, func) case Ops::O: return func<Types::ID::TYPE, Ops::O>;
#define ALL_CASES(TYPE) CASE(ADD, TYPE, opFunction); \
						CASE(SUB, TYPE, opFunction); \
						CASE(MUL, TYPE, opFunction); \
						CASE(DIV, TYPE, opFunction); \
						CASE(EQ_, TYPE, opFunction); \
						CASE(NEQ, TYPE, opFunction); \
						CASE(GT_, TYPE, opFunction); \
						CASE(GET, TYPE, opFunction); \
						CASE(LT_, TYPE, opFunction); \
						CASE(LET, TYPE, opFunction); \
						CASE(AND, TYPE, opFunction); \
						CASE(OR_, TYPE, opFunction); \
						CASE(NEG, TYPE, opFunction); 

#define INTEGER_CASES(TYPE)	CASE(MOD, TYPE, opFunctionInt); \
							CASE(BO_, TYPE, opFunctionInt); \
							CASE(BA_, TYPE, opFunctionInt); 

struct Statement::OpFunctions
{
	struct Inliners
	{
		struct Args
		{
			Args(Statement& s_) :
				s(s_)
			{};

			Statement& s;

			jit::ComplexType* typePtr = nullptr;
			dyn<uint8> preallocatedReturnData;
			VariableStorage* args = nullptr;
			jit::ComplexType** argTypePtrs = nullptr;
			int numArgTypes = 0;
			int numArgs = 0;

			void throwRuntimeError(const String& error) const
			{
				s.throwRuntimeError(error);
			}

			int getIndexArg(int argsId, int upperLimit) const
			{
				auto index = getTypedArgument<int>(argsId);

				if (!isPositiveAndBelow(index, upperLimit))
					s.throwRuntimeError("out of bounds: " + String(index) + " - maxIndex: " + String(upperLimit - 1));

				return index;
			}

			jit::ComplexType* getArgType(int index) const
			{
				if (!isPositiveAndBelow(index, numArgTypes))
					throwRuntimeError("invalid argument type index " + String(index));

				auto p = argTypePtrs[index];

				if (p == nullptr)
					throwRuntimeError("No type ptr for args #" + String(index));

				return p;
			}

			template <typename T> T& getTypedObject() const
			{
				jassert(args[0].getType() == Types::ID::Pointer);
				return *reinterpret_cast<T*>(args[0].getDataPointer());
			}

			template <typename T> T getTypedArgument(int index) const
			{
				static constexpr Types::ID TID = Types::Helpers::getTypeFromTypeId<T>();

				if (!isPositiveAndBelow(index, numArgs))
					s.throwRuntimeError("invalid arg index " + String(index));

				if (TID != args[index].getType())
					s.throwRuntimeError("invalid type");
				
				return args[index].toType<TID>();
			}

			template <typename T> VariableStorage copyToReturnData(const T& obj) const
			{
				int NumBytesRequired = sizeof(obj);

				jassert(preallocatedReturnData.begin() != nullptr);
				jassert(preallocatedReturnData.size() >= NumBytesRequired);
				memcpy(preallocatedReturnData.begin(), &obj, NumBytesRequired);
				return VariableStorage(preallocatedReturnData.begin());
			}
		};

		typedef VariableStorage(*Prototype)(const Args&);

		struct dyn
		{
			static VariableStorage referTo(const Args& a)
			{
				// referTo changes a view during setup, so the source type is inspected here
				// instead of adding referTo-specific metadata to every function call.
				auto sourceType = a.getArgType(0);
				auto destinationType = dynamic_cast<DynType*>(a.typePtr);
				if (destinationType == nullptr)
					a.throwRuntimeError("invalid dyn referTo destination");

				int totalSize = 0;
				void* sourceData = nullptr;

				if (auto spanType = dynamic_cast<SpanType*>(sourceType))
				{
					totalSize = spanType->getNumElements();
					sourceData = a.getTypedArgument<void*>(1);
				}
				else if (dynamic_cast<DynType*>(sourceType) != nullptr)
				{
					auto sourceView = a.args[1].getType() == Types::ID::Block ?
						a.args[1].toBlock() : *static_cast<block*>(a.getTypedArgument<void*>(1));
					totalSize = sourceView.size();
					sourceData = sourceView.begin();
				}
				else
					a.throwRuntimeError("unsupported dyn referTo source");

				auto size = a.getTypedArgument<int>(2);
				auto offset = a.getTypedArgument<int>(3);

				if (size == -1)
					size = totalSize;

				if (size < 0 || offset < 0 || offset > totalSize || size > totalSize - offset)
					a.throwRuntimeError("illegal dyn referTo size or offset");

				auto destination = static_cast<block*>(a.getTypedArgument<void*>(0));
				if (size == 0)
					destination->referToNothing();
				else
				{
					auto elementSize = destinationType->getElementType().getRequiredByteSizeNonZero();
					auto data = reinterpret_cast<uint8*>(sourceData) + (size_t)offset * elementSize;
					destination->referToRawData(reinterpret_cast<float*>(data), size);
				}

				return a.copyToReturnData(*destination);
			}

			static VariableStorage size(const Args& a)
			{
				jassert(a.numArgs == 1);
				auto bl = a.args[0].toBlock();
				return VariableStorage(bl.size());
			}

		};

		template <int C> struct FrameProcessor
		{
			static VariableStorage next(const Args& a) { return get(a).next(); }
			static VariableStorage subscript(const Args& a) { return &get(a)[a.getIndexArg(1, C)]; }
			static VariableStorage begin(const Args& a) { return get(a).begin(); }
			static VariableStorage size(const Args& a) { return C; }
			

		private:

			SNEX_GETTER(FrameProcessor, C);
		};

		template <int C> struct ProcessData
		{
			static VariableStorage getNumSamples(const Args& a) { return get(a).getNumSamples(); }
			static VariableStorage getNumChannels(const Args& a) { return get(a).getNumChannels(); }
			static VariableStorage toFrameData(const Args& a) { return a.copyToReturnData(get(a).toFrameData()); }
			static VariableStorage subscript(const Args& a) { return get(a)[a.getIndexArg(1, C)]; }

			static VariableStorage toChannelData(const Args& a) 
			{ 
				auto ptr = a.getTypedArgument<void*>(1);
				return get(a).toChannelData(ChannelPtr::fromRawPointer(ptr));
			}

			static VariableStorage toEventData(const Args& a)
			{
				auto d = get(a).toEventData();
				return block(reinterpret_cast<float*>(d.begin()), d.size());
			}

			static VariableStorage begin(const Args& a) { return get(a).begin(); }
			static VariableStorage size(const Args& a) { return C; }

		private:

			SNEX_GETTER(ProcessData, C);
		};


		static Prototype getInliner(Factory& f, const ValueTree& v)
		{
			if (auto objectType = Helpers::getObjectType(f, v))
			{
				auto fData = Helpers::getFunctionData(f, v);
				auto pid = OpFunctions::getInlinerPrototypeId(objectType, fData.id);
				return getInliner(pid);
			}

			return nullptr;
		}

		static Prototype getInliner(StructType* st, jit::FunctionClass::SpecialSymbols ss)
		{
			jit::FunctionClass::Ptr fc = st->getFunctionClass();
			auto id = jit::FunctionClass::getSpecialSymbol(st->id, ss);
			auto pId = OpFunctions::getInlinerPrototypeId(st, st->id.getChildId(id));
			return OpFunctions::Inliners::getInliner(pId);
		}

		static Prototype getInliner(const String& s)
		{
			static const std::map<String, Prototype> inlinerMap{
				INLINER(dyn, size),
				INLINER(dyn, referTo),
				INLINER_16CH(ProcessData, subscript),
				INLINER_16CH(ProcessData, getNumSamples),
				INLINER_16CH(ProcessData, toFrameData),
				INLINER_16CH(ProcessData, getNumChannels),
				INLINER_16CH(ProcessData, begin),
				INLINER_16CH(ProcessData, size),
				INLINER_16CH(ProcessData, toChannelData),
				INLINER_16CH(ProcessData, toEventData),
				INLINER_16CH(FrameProcessor, next),
				INLINER_16CH(FrameProcessor, subscript),
				INLINER_16CH(FrameProcessor, begin),
				INLINER_16CH(FrameProcessor, size)
			};

			if (inlinerMap.find(s) != inlinerMap.end())
				return inlinerMap.at(s);

			return nullptr;
		}
	};

	struct BinaryOp
	{
		using OpType = juce_wchar;

		enum class Ops: OpType
		{
			ADD = '+', // +
			SUB = '-', // -
			MUL = '*', // *
			DIV = '/', // /
			SET = '=', // =
			NEG = '!', // !
			MOD = '%', // %
			EQ_ = 'e', // ==
			NEQ = 'q', // !=
			GT_ = 'G', // >
			GET = 'g', // >=
			LT_ = 'L', // <
			LET = 'l', // <=
			AND = 'A', // &&
			BA_ = 'a', // &
			OR_ = 'O', // ||
			BO_ = 'o', // |
			unknown = 0
		};

		static Ops getFromString(const String& opType)
		{
			static const std::map<String, Ops> map({
			{ "+", Ops::ADD },
			{ "-", Ops::SUB },
			{ "*", Ops::MUL },
			{ "/", Ops::DIV },
			{ "=", Ops::SET },
			{ "!", Ops::NEG },
			{ "%", Ops::MOD },
			{ "==", Ops::EQ_ },
			{ "!=", Ops::NEQ },
			{ ">", Ops::GT_ },
			{ ">=", Ops::GET },
			{ "<", Ops::LT_ },
			{ "<=", Ops::LET },
			{ "&&", Ops::AND },
			{ "&", Ops::BA_ },
			{ "||", Ops::OR_ },
			{ "|", Ops::BO_ }
			});
			
			if (map.find(opType) != map.end())
				return map.at(opType);

			jassertfalse;
			return Ops::unknown;
		}

		typedef VariableStorage(*Function)(const VariableStorage&, const VariableStorage&);

		static Ops getOpType(const ValueTree& astNode, const Identifier& id = Identifier())
		{
			auto idToUse = id.isValid() ? id : AstIds::OpType;
			auto opString = astNode[idToUse].toString();
			return getFromString(opString.getCharPointer().getAddress());
		}

		static Function getFunctionPtr(const TypeInfo& t, Ops opMode)
		{
			if (opMode == Ops::SET)
				return nullptr;

			auto opType = t.getType();
							
			if (opType == Types::ID::Integer) { switch (opMode) { ALL_CASES(Integer); INTEGER_CASES(Integer); }}
			if (opType == Types::ID::Double) { switch (opMode) { ALL_CASES(Double); }}
			if (opType == Types::ID::Float)  { switch (opMode) { ALL_CASES(Float);  }}

			jassertfalse;
			return nullptr;
		}

	private:

		template <Types::ID T, Ops OpType> static VariableStorage opFunctionInt(const VariableStorage& v1, const VariableStorage& v2)
		{
			if constexpr (OpType == Ops::MOD) return VariableStorage((int)v1 %  (int)v2);
			if constexpr (OpType == Ops::BA_) return VariableStorage((int)v1 &  (int)v2);
			if constexpr (OpType == Ops::BO_) return VariableStorage((int)v1 |  (int)v2);

			jassertfalse;
			return VariableStorage(0);
		}

		template <Types::ID T, Ops OpType> static VariableStorage opFunction(const VariableStorage& v1, const VariableStorage& v2)
		{
			if constexpr (OpType == Ops::ADD) return VariableStorage(v1.toType<T>() +  v2.toType<T>());
			if constexpr (OpType == Ops::SUB) return VariableStorage(v1.toType<T>() -  v2.toType<T>());
			if constexpr (OpType == Ops::MUL) return VariableStorage(v1.toType<T>() *  v2.toType<T>());
			if constexpr (OpType == Ops::DIV) return VariableStorage(v1.toType<T>() /  v2.toType<T>());
			if constexpr (OpType == Ops::EQ_) return VariableStorage(v1.toType<T>() == v2.toType<T>());
			if constexpr (OpType == Ops::NEQ) return VariableStorage(v1.toType<T>() != v2.toType<T>());
			if constexpr (OpType == Ops::GT_) return VariableStorage(v1.toType<T>() >  v2.toType<T>());
			if constexpr (OpType == Ops::GET) return VariableStorage(v1.toType<T>() >= v2.toType<T>());
			if constexpr (OpType == Ops::LT_) return VariableStorage(v1.toType<T>() <  v2.toType<T>());
			if constexpr (OpType == Ops::LET) return VariableStorage(v1.toType<T>() <= v2.toType<T>());
			
			if constexpr (OpType == Ops::NEG) return VariableStorage(v1.toInt() != 0);
			if constexpr (OpType == Ops::AND) return VariableStorage(v1.toInt() && v2.toInt());
			if constexpr (OpType == Ops::OR_) return VariableStorage(v1.toInt() || v2.toInt());

			jassertfalse;
			return VariableStorage(T, var(0));
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
		typedef VariableStorage(*ReadFunction)(Scope::LValue);
		typedef void(*WriteFunction)(Scope::LValue, const VariableStorage&);
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

		static VariableStorage readPointerValue(Scope::LValue d)
		{
			auto ptr = reinterpret_cast<uint8*>(d.first) + d.second;
			return VariableStorage(*reinterpret_cast<void**>(ptr));
		}

		template <typename T> static VariableStorage readStatic(Scope::LValue d)
		{
			auto [data, offset] = d;
			auto ptr = reinterpret_cast<uint8*>(data) + offset;

			if constexpr (std::is_same<void*, T>())
				return VariableStorage(reinterpret_cast<void*>(ptr));
			else
				return VariableStorage(*reinterpret_cast<T*>(ptr));
		}

		template <typename T> static void writeStatic(Scope::LValue d, const VariableStorage& value)
		{
			auto [data, offset] = d;
			static constexpr auto TID = Types::Helpers::getTypeFromTypeId<T>();

			auto ptr = reinterpret_cast<uint8*>(data) + offset;

			T v = value.template toType<TID>();
			*reinterpret_cast<T*>(ptr) = v;
		}
	};

	static String getInlinerPrototypeId(jit::ComplexType* typePtr, const NamespacedIdentifier& functionId)
	{
		int first = -1;

		if (auto st = dynamic_cast<ComplexTypeWithTemplateParameters*>(typePtr))
		{
			for (auto t : st->getTemplateInstanceParameters())
			{
				if (t.constantDefined)
				{
					first = t.constant;
					break;
				}
			}
		}

		String sig;

		if (first != -1)
		{
			sig << functionId.getParent().toString();
			sig << "<" << String(first) << ">::";
			sig << functionId.id;
		}
		else
			sig << functionId.toString();

		if (sig.contains("operator[]"))
			sig = sig.replace("operator[]", "subscript");

		return sig;
	}

	static String getSigString(const jit::FunctionData& functionData, bool addThisPointer)
	{
		String sig;

		auto getChar = [](const TypeInfo& t)
		{
			if(t.isRef())
				return 'p';

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

#undef SNEX_GETTER
#undef INLINER
#undef INLINER_NC
#undef INLINER_16CH

#undef CASE
#undef ALL_CASES
#undef INTEGER_CASES



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
			catch (Scope::ScopeError& e)
			{
				auto ue = UriError::fail(e.toString());
				ue.fillLocation(locations[i]);
				throw ue;
			}
		}

		return ResultCode::ok;
	}

	Statement& parent;
	int instructionPointer = 0;
	std::vector<UriLocation> locations;
};


void Statement::resolveDataPointersRecursive(Scope* s)
{
	if (auto def = getTyped<DefinitionBase>())
	{
		if (def->statementScope.get() != nullptr)
			s = def->statementScope.get();
	}

	resolveDataPointers(s);

	for (auto cs : childStatements)
		cs->resolveDataPointersRecursive(s);
}

#define JASSERT_ON_FAIL 1

void Statement::throwRuntimeError(const String& errorMessage)
{
	UriError e;
	e.fillLocation(Helpers::createUriLocation(locationBuilder, astNode));
	e.errorType = UriError::ErrorType::Error;
	e.errorMessage = errorMessage;

#if JASSERT_ON_FAIL
	jassertfalse;
#endif

	throw e;
}

Result DefinitionBase::allocateChildStatements(Statement* thisAsStatement, Scope* scopeToUse)
{
	if (auto def = thisAsStatement->getTyped<DefinitionBase>())
	{
		if (auto c = def->createScope(scopeToUse))
		{
			def->statementScope = c;
			scopeToUse = c;
		}

		for (auto c : thisAsStatement->childStatements)
		{
			auto ok = allocateChildStatements(c, scopeToUse);

			if (ok.failed())
				c->throwRuntimeError(ok.getErrorMessage());
		}

		auto ok = def->allocateInScope(scopeToUse);

		if(!ok.wasOk())
			thisAsStatement->throwRuntimeError(ok.getErrorMessage());

		if (def->statementScope != nullptr)
			def->statementScope->finalise();

		return ok;
	}
	else
	{
		for (auto c : thisAsStatement->childStatements)
		{
			auto ok = allocateChildStatements(c, scopeToUse);

			if (ok.failed())
				c->throwRuntimeError(ok.getErrorMessage());
		}
			
		return Result::ok();
	}

	return Result::ok();
}	

} // namespace interpreter
} // namespace snex
