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
USE_ASMJIT_NAMESPACE;

int ComplexType::numInstances = 0;

Result ComplexType::callConstructor(InitData& d)
{
	FunctionClass::Ptr fc = getFunctionClass();

	if (fc != nullptr)
	{
		auto cf = fc->getSpecialFunction(FunctionClass::Constructor);

		if (cf.function == nullptr)
			return Result::ok();

		InitData defaultValues;
		defaultValues.callConstructor = false;
		defaultValues.dataPointer = d.dataPointer;
		defaultValues.initValues = makeDefaultInitialiserList();

		auto r = initialise(defaultValues);


		if (hasDefaultConstructor())
		{
			// Now change the init values so that it won't fail the type check
			d.initValues = new InitialiserList();
		}


		TypeInfo::List providedArgs;


		/** Set the member pointer here. */
		d.initValues->forEach([d, &providedArgs](InitialiserList::ChildBase* b)
		{
			if (auto mp = dynamic_cast<InitialiserList::MemberPointer*>(b))
			{
				auto offset = mp->st->getMemberOffset(mp->variableId);
				auto memberData = (uint8*)d.dataPointer + offset;

				VariableStorage v;

				if (!mp->getValue(v))
					mp->value = VariableStorage(memberData, (int)mp->st->getMemberTypeInfo(mp->variableId).getRequiredByteSize());

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
					providedArgs.add(TypeInfo(v.getType(), v.getType() == Types::ID::Pointer ? true : false));
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

		auto args = d.initValues->toFlatConstructorList();

		if (cf.args.size() != args.size())
		{
			// If the constructor has no arguments, pass them to the default initialisation
			if (cf.args.isEmpty())
			{
				InitData id;
				id.dataPointer = d.dataPointer;
				id.callConstructor = false;
				id.initValues = d.initValues;

				auto r = initialise(id);

				if (r.failed())
					return r;
			}
			else
			{
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

		cf.object = d.dataPointer;

        if(args.size() == 0)
        {
            cf.callVoid();
        }
        else if (args.size() > 1)
        {
			return Result::fail("constructor with more than one argument is not supported in SNEX");
        }
        else
        {
            auto fa = args[0];

            switch (fa.getType())
            {
            case Types::ID::Integer: cf.callVoid((int)args[0]); break;
            case Types::ID::Double: cf.callVoid((double)args[0]); break;
            case Types::ID::Float: cf.callVoid((float)args[0]); break;
            case Types::ID::Pointer: cf.callVoid(args[0].getDataPointer()); break;
            }
        }		
	}

	return Result::ok();
}

juce::Result ComplexType::callDestructor(InitData& d)
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
			auto st = d.functionTree->toSyntaxTreeData();
			auto call = new Operations::FunctionCall(st->location, nullptr, Symbol(f.id, f.returnType), f.templateParameters);
			call->setObjectExpression(st->expression->clone(st->location));

			st->object->addStatement(call);
			st->processUpToCurrentPass(st->object, call);

			return Result::ok();
		}
	}

	return Result::fail("no destructor");

}

snex::jit::FunctionData ComplexType::getDestructor()
{
	if (FunctionClass::Ptr fc = getFunctionClass())
	{
		return fc->getSpecialFunction(FunctionClass::Destructor);
	}

	return {};
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
            if(m.matchesArgumentTypesWithDefault({}))
				return true;
		}
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



snex::jit::FunctionData ComplexType::getNonOverloadedFunction(const Identifier& id)
{
	FunctionClass::Ptr fc = getFunctionClass();
	return fc->getNonOverloadedFunction(NamespacedIdentifier(id));
}

snex::jit::FunctionData ComplexType::getNodeCallback(const Identifier& id, int numChannels, bool checkProcessFunctions)
{
	auto sId = ScriptnodeCallbacks::getCallbackId(NamespacedIdentifier(id));

	if (checkProcessFunctions && 
		(sId == ScriptnodeCallbacks::ProcessFrameFunction ||
		sId == ScriptnodeCallbacks::ProcessFunction))
	{
		FunctionClass::Ptr fc = getFunctionClass();
		Array<FunctionData> matches;
		fc->addMatchingFunctions(matches, fc->getClassName().getChildId(id));

		for (auto f : matches)
		{
			auto t = f.templateParameters[0];

 			if (f.templateParameters.isEmpty())
			{
				auto dArgType = f.args[0].typeInfo;

				if (auto ar = dArgType.getTypedComplexType<SpanType>())
				{
					if (sId == ScriptnodeCallbacks::ProcessFrameFunction && ar->getNumElements() == numChannels)
						return f;
				}

				if (auto st = dArgType.getTypedComplexType<StructType>())
				{
					if (auto st = dArgType.getTypedComplexType<StructType>())
					{
						auto tArg = st->getTemplateInstanceParameters()[0];

						if (sId == ScriptnodeCallbacks::ProcessFunction &&
							st->id == NamespacedIdentifier("ProcessData") &&
							tArg.constantDefined &&
							tArg.constant == numChannels)
							return f;
					}
				}

				continue;
			}

			if (t.isTemplateArgument())
				continue;

			int channelAmount = -1;

			if (t.constantDefined)
				channelAmount = t.constant;
			else
			{
				jassert(t.type.isComplexType());

				if (auto asSpan = t.type.getTypedIfComplexType<SpanType>())
					channelAmount = asSpan->getNumElements();

				if (auto st = t.type.getTypedIfComplexType<StructType>())
				{
					jassert(st->getTemplateInstanceParameters()[0].constantDefined);
					channelAmount = st->getTemplateInstanceParameters()[0].constant;
				}
			}

			if (channelAmount == numChannels)
				return f;
		}

		return {};
	}
	else
		return getNonOverloadedFunction(id);
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
#if SNEX_ASMJIT_BACKEND
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
			auto t = cc.newFloatConst(ConstPoolScope::kLocal, v.toFloat());
			auto temp = cc.newXmmPs();
			cc.movss(temp, t);
			cc.movss(mem, temp);
		}
		IF_(double)
		{
			auto t = cc.newDoubleConst(ConstPoolScope::kLocal, v.toDouble());
			auto temp = cc.newXmmPd();
			cc.movsd(temp, t);
			cc.movsd(mem, temp);
		}
	}
#endif
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

ComplexType::Ptr ComplexType::finaliseAndReturn()
{
	if (!finalised)
		finaliseAlignment();

	return Ptr(this);
}

}
}
