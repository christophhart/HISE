/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

struct DspInstance::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(DspInstance, processBlock);
	API_VOID_METHOD_WRAPPER_2(DspInstance, prepareToPlay);
	API_VOID_METHOD_WRAPPER_2(DspInstance, setParameter);
	API_VOID_METHOD_WRAPPER_2(DspInstance, setStringParameter);
	API_METHOD_WRAPPER_1(DspInstance, getParameter);
	API_METHOD_WRAPPER_0(DspInstance, getInfo);
	API_METHOD_WRAPPER_1(DspInstance, getStringParameter);
	API_VOID_METHOD_WRAPPER_1(DspInstance, setBypassed);
	API_METHOD_WRAPPER_0(DspInstance, isBypassed);
    API_METHOD_WRAPPER_0(DspInstance, getNumParameters);
    API_METHOD_WRAPPER_0(DspInstance, getNumConstants);
    API_METHOD_WRAPPER_1(DspInstance, getConstant);
    API_METHOD_WRAPPER_1(DspInstance, getConstantId);
};


DspInstance::DspInstance(const DspFactory *f, const String &moduleName_) :
ConstScriptingObject(nullptr, NUM_API_FUNCTION_SLOTS),
moduleName(moduleName_),
factory(const_cast<DspFactory*>(f)),
object(nullptr),
bypassed(false)
{
	
}


void DspInstance::initialise()
{
    const SpinLock::ScopedLockType sl(getLock());

	if (DynamicDspFactory* dynamicFactory = dynamic_cast<DynamicDspFactory*>(factory.get()))
	{
		if ((int)dynamicFactory->getErrorCode() != (int)LoadingErrorCode::LoadingSuccessful)
		{
			object = nullptr;
			throw String("Library is not correctly loaded. Error code: " + dynamicFactory->getErrorCode().toString());
		}
	}

	if (factory != nullptr)
	{
		object = factory->createDspBaseObject(moduleName);

		if (object != nullptr)
		{
			ADD_API_METHOD_1(processBlock);
			ADD_API_METHOD_2(prepareToPlay);
			ADD_API_METHOD_2(setParameter);
			ADD_API_METHOD_1(getParameter);
			ADD_API_METHOD_2(setStringParameter);
			ADD_API_METHOD_1(getStringParameter);
			ADD_API_METHOD_0(isBypassed);
			ADD_API_METHOD_1(setBypassed);
			ADD_API_METHOD_0(getInfo);
            ADD_API_METHOD_0(getNumParameters);
            ADD_API_METHOD_0(getNumConstants);
            ADD_API_METHOD_1(getConstant);
            ADD_API_METHOD_1(getConstantId);
            

			for (int i = 0; i < object->getNumConstants(); i++)
			{
				char nameBuffer[64];
				int nameLength = 0;

				object->getIdForConstant(i, nameBuffer, nameLength);

				String thisName(nameBuffer, nameLength);

				int intValue;
				if (object->getConstant(i, intValue))
				{
					addConstant(thisName, var(intValue));
					continue;
				}

				float floatValue;
				if (object->getConstant(i, floatValue))
				{
					addConstant(thisName, var(floatValue));
					continue;
				}

				char stringBuffer[512];
				size_t stringBufferLength;

				if (object->getConstant(i, stringBuffer, stringBufferLength))
				{
					String text(stringBuffer, stringBufferLength);
					addConstant(thisName, var(text));
					continue;
				}


				float *externalData;
				int externalDataSize;

				if (object->getConstant(i, &externalData, externalDataSize))
				{
					auto b = new VariantBuffer(externalData, externalDataSize);
					addConstant(thisName, var(b));
					continue;
				}
			}
		}
		else
		{
			throw String("The module " + moduleName + " wasn't found in the Library.");
		}
	}

}

void DspInstance::processBlock(const var &data)
{
	if (!prepareToPlayWasCalled)
        throw String(moduleName + ": prepareToPlay must be called before processing buffers.");

	checkPriorityInversion();

    const SpinLock::ScopedLockType sl(getLock());
    
	bool skipProcessing = isBypassed() && !switchBypassFlag;

	if (object != nullptr && !skipProcessing)
	{
		if (data.isArray())
		{
			Array<var> *a = data.getArray();

			float *sampleData[NUM_MAX_CHANNELS];
            
#if USE_BACKEND
			const float** sampleDataForListeners = const_cast<const float**>(sampleData);
#endif
            
			int numSamples = -1;

			
			if (a == nullptr)
				throwError("processBlock must be called on array of buffers");

			int numChannels = a->size();

			sampleData[0] = nullptr;
			sampleData[1] = nullptr;

			CHECK_AND_LOG_ASSERTION(processor, DebugLogger::Location::ScriptFXRendering, a->size() == 2, 165);

			for (int i = 0; i < numChannels; i++)
			{
				VariantBuffer *b = a->getUnchecked(i).getBuffer();

				if (b != nullptr)
				{
					if (numSamples != -1 && b->size != numSamples)
						throwError("Buffer size mismatch");

					numSamples = b->size;

					sampleData[i] = b->buffer.getWritePointer(0);
				}
				else throwError("processBlock must be called on array of buffers");
			}

			if (switchBypassFlag)
			{
				if (sampleData[0] == nullptr || sampleData[1] == nullptr)
				{
					throwError("Array wasn't initialized correctly");
				}

				float* leftSamples = bypassSwitchBuffer.getWritePointer(0);
				float* rightSamples = bypassSwitchBuffer.getWritePointer(1);

				const bool rampUp = !isBypassed();

				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRendering, sampleData[0], true, numSamples);
				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRendering, sampleData[1], false, numSamples);

				FloatSanitizers::sanitizeArray(sampleData[0], numSamples);
				FloatSanitizers::sanitizeArray(sampleData[1], numSamples);

				FloatVectorOperations::copy(leftSamples, sampleData[0], numSamples);
				FloatVectorOperations::copy(rightSamples, sampleData[1], numSamples);


#if USE_BACKEND
				for (auto l : listeners)
				{
					if (l.get() != nullptr)
						l->preBlockProcessed(sampleDataForListeners, numChannels, numSamples);
				}
#endif

				object->processBlock(sampleData, a->size(), numSamples);

				if (rampUp)
				{
					bypassSwitchBuffer.applyGainRamp(0, numSamples, 1.0f, 0.0f);

					bypassSwitchBuffer.addFromWithRamp(0, 0, sampleData[0], numSamples, 0.0f, 1.0f);
					bypassSwitchBuffer.addFromWithRamp(1, 0, sampleData[1], numSamples, 0.0f, 1.0f);
				}
				else
				{
					bypassSwitchBuffer.applyGainRamp(0, numSamples, 0.0f, 1.0f);

					bypassSwitchBuffer.addFromWithRamp(0, 0, sampleData[0], numSamples, 1.0f, 0.0f);
					bypassSwitchBuffer.addFromWithRamp(1, 0, sampleData[1], numSamples, 1.0f, 0.0f);
				}

				

				FloatVectorOperations::copy(sampleData[0], leftSamples, numSamples);
				FloatVectorOperations::copy(sampleData[1], rightSamples, numSamples);

				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRenderingPost, sampleData[0], true, numSamples);
				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRenderingPost, sampleData[1], false, numSamples);

				switchBypassFlag = false;

#if USE_BACKEND
				for (auto l : listeners)
				{
					if (l.get() != nullptr)
						l->blockWasProcessed(sampleDataForListeners, numChannels, numSamples);
				}
#endif
			}
			else
			{
				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRendering, sampleData[0], true, numSamples);
				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRendering, sampleData[1], false, numSamples);

				for (int i = 0; i < numChannels; i++)
					FloatSanitizers::sanitizeArray(sampleData[i], numSamples);

#if USE_BACKEND
				for (auto l : listeners)
				{
					if (l.get() != nullptr)
						l->preBlockProcessed(sampleDataForListeners, numChannels, numSamples);
				}
#endif

				object->processBlock(sampleData, numChannels, numSamples);


#if USE_BACKEND
				for (auto l : listeners)
				{
					if (l.get() != nullptr)
						l->blockWasProcessed(sampleDataForListeners, numChannels, numSamples);
				}
#endif

				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRenderingPost, sampleData[0], true, numSamples);
				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRenderingPost, sampleData[1], false, numSamples);

				for (int i = 0; i < numChannels; i++)
					FloatSanitizers::sanitizeArray(sampleData[i], numSamples);
			}

		}
		else if (data.isBuffer())
		{
			VariantBuffer *b = data.getBuffer();

			if (b != nullptr)
			{
				float *sampleData[1] = { b->buffer.getWritePointer(0) };
#if USE_BACKEND
				const float** sampleDataForListeners = const_cast<const float**>(sampleData);
#endif
				const int numSamples = b->size;

				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRendering, sampleData[0], true, numSamples);
				FloatSanitizers::sanitizeArray(sampleData[0], numSamples);


#if USE_BACKEND
				for (auto l : listeners)
				{
					if (l.get() != nullptr)
						l->preBlockProcessed(sampleDataForListeners, 1, numSamples);
				}
#endif

				object->processBlock(sampleData, 1, numSamples);

				CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, debugId, DebugLogger::Location::DspInstanceRenderingPost, sampleData[0], true, numSamples);
				FloatSanitizers::sanitizeArray(sampleData[0], numSamples);

#if USE_BACKEND
				for (auto l : listeners)
				{
					if (l.get() != nullptr)
						l->blockWasProcessed(sampleDataForListeners, 1, numSamples);
				}
#endif
			}
		}
		else throwError("Data Buffer is not valid");
	}
}

void DspInstance::setParameter(int index, float newValue)
{
	if (object != nullptr && index < object->getNumParameters())
	{
		const SpinLock::ScopedLockType sl(getLock());
        
		object->setParameter(index, newValue);

#if USE_BACKEND
		for (auto l : listeners)
		{
			if (l.get() != nullptr)
				l->parameterChanged(index);
		}
#endif
	}
}

var DspInstance::getParameter(int index) const
{
	if (object != nullptr)
	{
        //const SpinLock::ScopedLockType sl(getLock());
        
		return object->getParameter(index);
	}

	return var::undefined();
}



var DspInstance::getNumParameters() const
{
    if (object != nullptr)
    {
        return object->getNumParameters();
    }

	return 0;
}


var DspInstance::getNumConstants() const
{
    if (object != nullptr)
    {
        return object->getNumConstants();
    }

	return 0;
}


var DspInstance::getConstantId(int index) const
{
    if (object != nullptr)
    {
        return getConstantName(index).toString();
    }

	return "";
}

var DspInstance::getConstant(int index) const
{
    if (object != nullptr)
    {
        return getConstantValue(index);
    }

	return 0;
}

void DspInstance::setStringParameter(int index, String value)
{
	if (object != nullptr)
	{
        SpinLock::ScopedLockType sl(getLock());
        
		object->setStringParameter(index, value.getCharPointer(), value.length());
	}
}

String DspInstance::getStringParameter(int index)
{
	if (object != nullptr)
	{
		size_t textLength = 0;
		const char* newText = object->getStringParameter(index, textLength);

		std::string s;

		s.reserve(textLength);

		for (size_t i = 0; i < textLength; i++)
			s.push_back(*newText++);

		return String(s);
	}
	else
	{
		return String();
	}
}

void DspInstance::setBypassed(bool shouldBeBypassed)
{
    const SpinLock::ScopedLockType sl(getLock());
    
	bypassed.store(shouldBeBypassed);
	switchBypassFlag = true;
}

bool DspInstance::isBypassed() const
{
	return bypassed.load();
}

void DspInstance::assign(const int index, var newValue)
{
	setParameter(index, newValue);
}

var DspInstance::getAssignedValue(int index) const
{
	return getParameter(index);
}

int DspInstance::getCachedIndex(const var &constantName) const
{
	return getConstantValue((int)constantName);
}

var DspInstance::getInfo() const
{
	if (object != nullptr)
	{
		String info;

		info << "Name: " + moduleName << "\n";

		info << "Parameters: " << String(object->getNumParameters()) << "\n";

		for (int i = 0; i < object->getNumParameters(); i++)
		{
			const String line = String("Parameter #" + String(i) + String(": current value: ") + String(object->getParameter(i)) + String("\n"));

			info << line;
		}

		info << "\n";

		info << "Constants: " << String(object->getNumConstants()) << "\n";

		for (int i = 0; i < object->getNumConstants(); i++)
		{
			info << "Constant #" << String(i) << ": " << getConstantName(i).toString() << +" = " << getConstantValue(i).toString() << "\n";
		}

		return var(info);
	}

	return var("No module loaded");
}

void DspInstance::setProcessor(Processor* p)
{
	//jassert(p != nullptr);
	processor = p;
        
	if(p != nullptr)
	{
		logger = &processor->getMainController()->getDebugLogger();
	}
        
		
}

void DspInstance::setId(const String& newName)
{
	if (Identifier::isValidIdentifier(newName))
	{
		debugId = Identifier(newName);
	}
}

void DspInstance::checkPriorityInversion()
{
	if (logger != nullptr && logger->isLogging())
	{
		logger->checkPriorityInversion(getLock(), DebugLogger::Location::DspInstanceRendering, processor.get(), debugId);
	}
}

void DspInstance::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void DspInstance::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void DspInstance::operator>>(const var &data)
{
	processBlock(data);
}

void DspInstance::operator<<(const var &data)
{
	processBlock(data);
}

DspInstance::~DspInstance()
{
	if (object != nullptr)
	{
		for (int i = 0; i < object->getNumConstants(); i++)
		{
			if (getConstantValue(i).isBuffer())
				getConstantValue(i).getBuffer()->referToData(nullptr, 0);
		}
	}

	unload();
}

void DspInstance::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	if (object != nullptr && samplesPerBlock > 0 && sampleRate > 0.0)
	{
        const SpinLock::ScopedLockType sl(getLock());
        
		object->prepareToPlay(sampleRate, samplesPerBlock);

		bypassSwitchBuffer.setSize(2, samplesPerBlock);

		for (int i = 0; i < object->getNumConstants(); i++)
		{
			if (getConstantValue(i).isBuffer())
			{
				float* data;
				int size;

				object->getConstant(i, &data, size);

				getConstantValue(i).getBuffer()->referToData(data, size);
			}
		}

		prepareToPlayWasCalled = true;
	}
}


void DspInstance::unload()
{
	if (factory != nullptr)
	{
        const SpinLock::ScopedLockType sl(getLock());
        
		factory->destroyDspBaseObject(object);
		object = nullptr;
		factory = nullptr;
	}
}

} // namespace hise
