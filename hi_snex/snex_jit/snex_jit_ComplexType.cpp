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

int ComplexType::numInstances = 0;

Result ComplexType::callConstructor(void* data, InitialiserList::Ptr initList)
{




	FunctionClass::Ptr fc = getFunctionClass();

	if (fc != nullptr)
	{
		auto cf = fc->getSpecialFunction(FunctionClass::Constructor);

		if (cf.function == nullptr)
			return Result::ok();

		InitData defaultValues;
		defaultValues.callConstructor = false;
		defaultValues.dataPointer = data;
		defaultValues.initValues = makeDefaultInitialiserList();



		auto r = initialise(defaultValues);


		if (hasDefaultConstructor())
		{
			// Now change the init values so that it won't fail the type check
			initList = new InitialiserList();
		}


		TypeInfo::List providedArgs;


		/** Set the member pointer here. */
		initList->forEach([data, &providedArgs](InitialiserList::ChildBase* b)
			{
				if (auto mp = dynamic_cast<InitialiserList::MemberPointer*>(b))
				{
					auto offset = mp->st->getMemberOffset(mp->variableId);
					auto memberData = (uint8*)data + offset;

					VariableStorage v;

					if (!mp->getValue(v))
						mp->value = VariableStorage(memberData, mp->st->getMemberTypeInfo(mp->variableId).getRequiredByteSize());

					providedArgs.add(mp->st->getMemberTypeInfo(mp->variableId).withModifiers(false, true));
				}
				else if (auto exp = dynamic_cast<InitialiserList::ExpressionChild*>(b))
				{
					providedArgs.add(exp->expression->getTypeInfo());
				}
				else
				{
					VariableStorage v;
					if (b->getValue(v))
						providedArgs.add(TypeInfo(v.getType()));
				}
				return false;
			});

		if (!cf.matchesArgumentTypes(providedArgs))
		{
			String s;
			s << cf.getSignature({}, false);
			s << ": constructor type mismatch. Expected arguments: (";

			for (auto& pa : providedArgs)
				s << pa.toString() + ", ";

			s = s.upToLastOccurrenceOf(", ", false, false);

			s << ")";

			return Result::fail(s);
		}

		auto args = initList->toFlatConstructorList();

		if (cf.args.size() != args.size())
		{
			// If the constructor has no arguments, pass them to the default initialisation
			if (cf.args.isEmpty())
			{
				InitData id;
				id.dataPointer = data;
				id.callConstructor = false;
				id.initValues = initList;

				auto r = initialise(id);

				if (r.failed())
					return r;
			}
			else
			{
				if (args.isEmpty())
				{

				}

				return Result::fail("constructor argument mismatch");
			}
		}



		Array<Types::ID> types;

		// The constructor might have zero arguments
		if (cf.args.size() > 0)
		{
			for (auto a : args)
				types.add(a.getType());
		}

		for (auto& a : cf.args)
			a.typeInfo = a.typeInfo.toNativePointer();


		if (!cf.matchesNativeArgumentTypes(Types::ID::Void, types))
			return Result::fail("constructor type mismatch");

		cf.object = data;

		cf.callVoidDynamic(args.getRawDataPointer(), args.size());
	}

	return Result::ok();
}

juce::Result ComplexType::callDestructor(DeconstructData& d)
{
	if (hasDestructor())
	{
		FunctionClass::Ptr fc = getFunctionClass();
		auto f = fc->getSpecialFunction(FunctionClass::Destructor);

		if (d.dataPointer != nullptr)
		{
			if (f.function != nullptr)
			{
				f.object = d.dataPointer;
				f.callVoid();
				return Result::ok();
			}

			return Result::fail("no function pointer found");
		}
		else
		{
			auto st = d.inlineData->toSyntaxTreeData();
			auto call = new Operations::FunctionCall(st->location, nullptr, Symbol(f.id, f.returnType), f.templateParameters);
			call->setObjectExpression(st->expression->clone(st->location));

			st->object->addStatement(call);
			st->processUpToCurrentPass(st->object, call);

			return Result::ok();
		}
	}

	return Result::fail("no destructor");

}

bool ComplexType::hasDestructor()
{
	if (FunctionClass::Ptr fc = getFunctionClass())
	{
		return fc->getSpecialFunction(FunctionClass::Destructor).id.isValid();
	}

	return false;
}

bool ComplexType::hasConstructor()
{
	if (FunctionClass::Ptr fc = getFunctionClass())
	{
		return fc->getSpecialFunction(FunctionClass::Constructor).id.isValid();
	}

	return false;
}

bool ComplexType::hasDefaultConstructor()
{
	if (!hasConstructor())
		return true;

	if (FunctionClass::Ptr fc = getFunctionClass())
	{
		Array<FunctionData> matches;

		auto fid = fc->getClassName();

		auto id = fid.getChildId(fc->getSpecialSymbol(fid, FunctionClass::Constructor));

		fc->addMatchingFunctions(matches, id);

		for (auto& m : matches)
		{
			if (m.args.isEmpty())
				return true;
		}

		return false;
	}

	return false;
}

void ComplexType::registerExternalAtNamespaceHandler(NamespaceHandler* handler, const juce::String& description)
{
	if (hasAlias())
	{
		if (handler->getSymbolType(getAlias()) == NamespaceHandler::UsingAlias)
			return;

		jassert(getAlias().isExplicit());

		NamespaceHandler::SymbolDebugInfo info;
		info.comment = description;

		handler->addSymbol(getAlias(), TypeInfo(this), NamespaceHandler::UsingAlias, info);
	}
}

bool ComplexType::isValidCastSource(Types::ID nativeSourceType, ComplexType::Ptr complexSourceType) const
{
	if (complexSourceType == this)
		return true;

	return false;
}

bool ComplexType::isValidCastTarget(Types::ID nativeTargetType, ComplexType::Ptr complexTargetType) const
{
	if (complexTargetType == this)
		return true;

	return false;
}





void ComplexType::writeNativeMemberTypeToAsmStack(const ComplexType::InitData& d, int initIndex, int offsetInBytes, int size)
{
	auto& cc = GET_COMPILER_FROM_INIT_DATA(d);
	auto mem = d.asmPtr->memory.cloneAdjustedAndResized(offsetInBytes, size);

	if (auto expr = dynamic_cast<Operations::Expression*>(d.initValues->getExpression(initIndex)))
	{
		auto source = expr->reg;
		source->loadMemoryIntoRegister(cc);
		auto type = source->getType();

		IF_(int) cc.mov(mem, INT_REG_R(source));
		IF_(double) cc.movsd(mem, FP_REG_R(source));
		IF_(float) cc.movss(mem, FP_REG_R(source));
		IF_(void*) cc.mov(mem, INT_REG_R(source));
	}
	else
	{
		VariableStorage v;
		d.initValues->getValue(initIndex, v);

		auto type = v.getType();

		IF_(int)
		{
			cc.mov(mem, v.toInt());
		}
		IF_(float)
		{
			auto t = cc.newFloatConst(ConstPool::kScopeLocal, v.toFloat());
			auto temp = cc.newXmmPs();
			cc.movss(temp, t);
			cc.movss(mem, temp);
		}
		IF_(double)
		{
			auto t = cc.newDoubleConst(ConstPool::kScopeLocal, v.toDouble());
			auto temp = cc.newXmmPd();
			cc.movsd(temp, t);
			cc.movsd(mem, temp);
		}
	}
}

void ComplexType::writeNativeMemberType(void* dataPointer, int byteOffset, const VariableStorage& initValue)
{
	auto dp_raw = getPointerWithOffset(dataPointer, byteOffset);
	auto copy = initValue;

	switch (copy.getType())
	{
	case Types::ID::Integer: *reinterpret_cast<int*>(dp_raw) = (int)initValue; break;
	case Types::ID::Double:  *reinterpret_cast<double*>(dp_raw) = (double)initValue; break;
	case Types::ID::Float:	 *reinterpret_cast<float*>(dp_raw) = (float)initValue; break;
	case Types::ID::Pointer: *((void**)dp_raw) = copy.getDataPointer(); break;
	case Types::ID::Block:
	{
		auto& b = *reinterpret_cast<block*>(dp_raw);
		auto other = initValue.toBlock();

		b.referToRawData(other.begin(), other.size());
		break;
	}
	default:				 jassertfalse;
	}
}

}
}