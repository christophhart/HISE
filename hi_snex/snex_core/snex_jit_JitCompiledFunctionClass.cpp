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

snex::jit::FunctionData JitCompiledClassBase::getFunction(const Identifier& id)
{
	auto typePtr = dynamic_cast<StructType*>(classType.get());
	auto sId = typePtr->id.getChildId(id);
	return memberFunctions->getNonOverloadedFunction(sId);
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



}
}
