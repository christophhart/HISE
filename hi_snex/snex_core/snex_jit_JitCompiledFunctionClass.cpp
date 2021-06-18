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


JitCompiledFunctionClass::JitCompiledFunctionClass(BaseScope* parentScope, const NamespacedIdentifier& classInstanceId)
{
	pimpl = new ClassScope(parentScope, classInstanceId, nullptr);
}


JitCompiledFunctionClass::~JitCompiledFunctionClass()
{
	if(pimpl != nullptr)
		delete pimpl;
}


VariableStorage JitCompiledFunctionClass::getVariable(const Identifier& id)
{
	auto s = pimpl->rootData->getClassName().getChildId(id);

	if (auto r = pimpl->rootData->contains(s))
	{
		return pimpl->rootData->getDataCopy(s);
	}

	jassertfalse;
	return {};
}


snex::jit::ComplexType::Ptr JitCompiledFunctionClass::getMainObjectType()
{
	auto symbols = pimpl->rootData->getAllVariables();

	for (auto s : symbols)
	{
		if (s.id == getMainId())
			return s.typeInfo.getTypedIfComplexType<ComplexType>();
	}

	return nullptr;
}

void* JitCompiledFunctionClass::getMainObjectPtr()
{
	return pimpl->rootData->getDataPointer(getMainId());
}

void* JitCompiledFunctionClass::getVariablePtr(const Identifier& id)
{
	auto s = pimpl->rootData->getClassName().getChildId(id);

	if (pimpl->rootData->contains(s))
		return pimpl->rootData->getDataPointer(s);

	return nullptr;
}

juce::String JitCompiledFunctionClass::dumpTable()
{
	return pimpl->getRootData()->dumpTable();
}

Array<NamespacedIdentifier> JitCompiledFunctionClass::getFunctionIds() const
{
	return pimpl->getRootData()->getFunctionIds();
}

snex::NamespacedIdentifier JitCompiledFunctionClass::getMainId()
{
	return NamespacedIdentifier("instance");
}

FunctionData JitCompiledFunctionClass::getFunction(const NamespacedIdentifier& functionId)
{
	auto s = pimpl->getRootData()->getClassName().getChildId(functionId.getIdentifier());

	if (pimpl->getRootData()->hasFunction(s))
	{
		Array<FunctionData> matches;
		pimpl->getRootData()->addMatchingFunctions(matches, s);

		// We don't allow overloaded functions for JIT compilation anyway...
		return matches.getFirst();
	}

	return {};
}



bool JitObject::isStateless()
{
	return getClassScope()->getRootData()->getAllVariables().isEmpty();
}

snex::jit::FunctionData JitObject::operator[](const NamespacedIdentifier& functionId) const
{
	if (*this)
	{
		return functionClass->getFunction(functionId);
	}
		

	return {};
}


FunctionData JitObject::operator[](const Identifier& functionId) const
{
	return operator[](NamespacedIdentifier(functionId));
}

Array<NamespacedIdentifier> JitObject::getFunctionIds() const
{
	if (*this)
	{
		return functionClass->getFunctionIds();
	}

	return {};
}

JitObject::operator bool() const
{
	return functionClass != nullptr;
}

void JitObject::rebuildDebugInformation()
{
	if(functionClass != nullptr)
		functionClass->pimpl->createDebugInfo(functionClass->debugInformation);
}

hise::DebugableObjectBase* JitObject::getDebugObject(const juce::String& token)
{
#if 0
	for (auto& f : functionClass->debugInformation)
	{
		if (token == f->getCodeToInsert())
		{
			if (f->getType() == Types::ID::Block)
				return functionClass->pimpl->getSubFunctionClass(Symbol::createRootSymbol("Block"));
			if (f->getType() == Types::ID::Event)
				return functionClass->pimpl->getSubFunctionClass(Symbol::createRootSymbol("Message"));
		}
	}
#endif

	return ApiProviderBase::getDebugObject(token);
}

juce::ValueTree JitObject::createValueTree()
{
    return {};
    
	auto c = dynamic_cast<GlobalScope*>(functionClass->pimpl->getParent())->getGlobalFunctionClass(NamespacedIdentifier("Console"));
	auto v = functionClass->pimpl->getRootData()->getApiValueTree();
	v.addChild(FunctionClass::createApiTree(c), -1, nullptr);
	return v;
}

void JitObject::getColourAndLetterForType(int type, Colour& colour, char& letter)
{
	return ApiHelpers::getColourAndLetterForType(type, colour, letter);
}

snex::jit::FunctionData JitCompiledClassBase::getFunction(const Identifier& id)
{
	auto typePtr = dynamic_cast<StructType*>(classType.get());
	auto sId = typePtr->id.getChildId(id);
	return memberFunctions->getNonOverloadedFunction(sId);
}

SnexComplexVarObject::SnexComplexVarObject(SnexStructSriptingWrapper::Ptr p, const var::NativeFunctionArgs& constructorArgs) :
	ptr(p->ptr),
	parent(p.get())
{
	ownedData_.allocate(ptr->getRequiredAlignment() + ptr->getRequiredByteSize(), true);
	dataPtr = ownedData_.get() + ptr->getRequiredAlignment();

	ComplexType::InitData d;
	d.dataPointer = dataPtr;
	d.callConstructor = false;
	d.initValues = ptr->makeDefaultInitialiserList();

	

	ptr->initialise(d);

	if (ptr->hasConstructor())
	{
		FunctionClass::Ptr f = ptr->getFunctionClass();
		auto cf = f->getSpecialFunction(FunctionClass::Constructor);

		VariableStorage cArgs[4];

		for (int i = 0; i < constructorArgs.numArguments; i++)
			cArgs[i] = VariableStorage(cf.args[i].typeInfo.getType(), constructorArgs.arguments[i]);

		cf.object = dataPtr;

		cf.callVoidDynamic(cArgs, constructorArgs.numArguments);
	}

	FunctionClass::Ptr fc = ptr->getFunctionClass();
	Array<snex::NamespacedIdentifier> functionIds;

	fc->getAllFunctionNames(functionIds);

	TypeInfo ti(ptr);

	if (auto st = ti.getTypedIfComplexType<StructType>())
	{
		int i = 0;
		Identifier mId = st->getMemberName(i);

		while (mId.isValid())
		{
			if (st->getMemberVisibility(mId) == NamespaceHandler::Visibility::Public)
			{
				DynamicProperty prop;
				prop.id = mId;
				prop.dataMember = (uint8*)dataPtr + st->getMemberOffset(mId);
				prop.type = st->getMemberTypeInfo(mId).getType();
				dynamicProps.add(prop);
			}

			i++;
			mId = st->getMemberName(i);
		}
	}

	for (auto& fId : functionIds)
	{
		auto fData = fc->getNonOverloadedFunction(fId);
		fData.object = dataPtr;

		if (fData.id.isValid())
			functions.add({ fData });
	}
}

SnexStructSriptingWrapper::SnexStructSriptingWrapper(GlobalScope& s, const String& code, const Identifier& classId_) :
	scope(s),
	r(Result::ok()),
	classId(classId_)
{
	jit::Compiler c(scope);

	snex::Types::SnexObjectDatabase::registerObjects(c, 2);

	obj = c.compileJitObject(code);
	r = c.getCompileResult();

	if (r.wasOk())
	{
		NamespacedIdentifier nId(classId);
		ptr = c.getComplexType(nId);
	}
}

var SnexStructSriptingWrapper::create(const var::NativeFunctionArgs& args)
{
	if (!r.wasOk())
		throw r.getErrorMessage();

	return SnexComplexVarObject::make(this, args);
}

SnexComplexVarObject::DynamicFunction::DynamicFunction(const FunctionData& f_) :
	f(f_)
{
	id = f.id.getIdentifier();
	returnType = f.returnType.getType();

	uint32 requiredComplexArgSize = 0;

	for (int i = 0; i < 4; i++)
	{
		args[i] = VariableStorage();
		
		auto ti = f.args[i].typeInfo;

		cArgs[i].nativeType = ti.getType();

		if (auto p = ti.getTypedIfComplexType<ComplexType>())
		{
			cArgs[i].specialType = getSpecialType(ti);
			cArgs[i].numElements = getNumElements(ti);
		}
	}
}

bool setupSpecialType(Types::ProcessDataDyn& data, const var& v)
{
	// Must be set before calling this method...
	jassert(data.getNumChannels() != 0);

	if (v.isBuffer())
	{
		if (data.getNumChannels() == 1)
		{
			auto& vb = *v.getBuffer();
			data.referTo(vb.buffer.getArrayOfWritePointers(), 1, vb.size);
		}
		else
			throw String("Channel mismatch: expected " + String(data.getNumChannels()) + " channels");

		return true;
	}
	else if (v.isArray())
	{
		auto& ar = *v.getArray();

		if (ar.size() == data.getNumChannels())
		{
			int length = 0;

			for (int ch = 0; ch < ar.size(); ch++)
			{
				if (ar[ch].isBuffer())
				{
					auto& cb = *ar[ch].getBuffer();

					if (ch == 0)
						length = cb.size;

					if (cb.size != length)
						throw String("buffer length mismatch at channel " + String(ch));

					auto channels = data.getRawChannelPointers();
					channels[ch] = &cb[0];
				}

				else
					throw String("Expected Buffer at channel index " + String(ch));
			}

			

			if (length == 0)
				throw String("Can't find buffers");

			data.referTo(data.getRawChannelPointers(), data.getNumChannels(), length);

			return true;
		}
		else
		{
			throw  String("channel amount mismatch: expected " + String(data.getNumChannels()) + " channels");
		}
	}

	return false;
}

bool setupSpecialType(block& b, const var& v)
{
	if (v.isBuffer())
	{
		auto& vb = *v.getBuffer();
		
		b.referToRawData(vb.buffer.getWritePointer(0), vb.size);
		return true;
	}
	
	throw String("argument must be Buffer Type");
}


var SnexComplexVarObject::DynamicFunction::call(const var::NativeFunctionArgs& vArgs)
{
	if (getNumArgs() == vArgs.numArguments)
	{
		if (cArgs[0].isSpecial())
		{
			switch (cArgs[0].specialType)
			{
			case ComplexArgument::ProcessData:
			{
				float* data[NUM_MAX_CHANNELS];
				memset(data, 0, sizeof(float*)*NUM_MAX_CHANNELS);
				Types::ProcessDataDyn d(data, 0, cArgs[0].numElements);
				setupSpecialType(d, vArgs.arguments[0]);
				f.callVoid(&d);
				return {};
			}
			case ComplexArgument::BlockType:
			{
				block b;
				setupSpecialType(b, vArgs.arguments[0]);
				f.callVoid(&b);
				return {};
			}
			}
		}
		else
		{
			for (int i = 0; i < vArgs.numArguments; i++)
			{
				args[i] = VariableStorage(cArgs[i].nativeType, vArgs.arguments[i]);
			}

			if (f.returnType.getType() == Types::ID::Void)
				f.callVoidDynamic(args, getNumArgs());
			else
			{
				auto v = f.callDynamic(args, getNumArgs());

				switch (v.getType())
				{
				case Types::ID::Integer: return var(v.toInt());
				case Types::ID::Double: return var(v.toDouble());
				case Types::ID::Float: return var(v.toFloat());
				case Types::ID::Pointer: return var(v.toPtr());
				}
			}
		}
	}

	return {};
}

snex::jit::SnexComplexVarObject::DynamicFunction::ComplexArgument::SpecialType SnexComplexVarObject::DynamicFunction::getSpecialType(const TypeInfo& t)
{
	if (auto dt = t.getTypedIfComplexType<snex::jit::DynType>())
	{
		if (dt->getElementType().getType() == snex::Types::ID::Float)
			return ComplexArgument::BlockType;
	}
	if (auto st = t.getTypedIfComplexType<snex::jit::StructType>())
	{
		if (st->id.getIdentifier().toString() == "ProcessData")
		{
			return ComplexArgument::SpecialType::ProcessData;
		}
	}

	return ComplexArgument::Unknown;
}

int SnexComplexVarObject::DynamicFunction::getNumElements(const TypeInfo& t)
{
	if (auto dt = t.getTypedIfComplexType<snex::jit::SpanType>())
	{
		return dt->getNumElements();
	}
	if (auto st = t.getTypedIfComplexType<snex::jit::StructType>())
	{
		if (st->id.getIdentifier().toString() == "ProcessData")
		{
			return st->getTemplateInstanceParameters()[0].constant;
		}
	}

	return 0;
}

}
}
