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

				String name(nameBuffer, nameLength);

				int intValue;
				if (object->getConstant(i, intValue))
				{
					addConstant(name, var(intValue));
					continue;
				}

				float floatValue;
				if (object->getConstant(i, floatValue))
				{
					addConstant(name, var(floatValue));
					continue;
				}

				char stringBuffer[512];
				size_t stringBufferLength;

				if (object->getConstant(i, stringBuffer, stringBufferLength))
				{
					String text(stringBuffer, stringBufferLength);
					addConstant(name, var(text));
					continue;
				}


				float *externalData;
				int externalDataSize;

				if (object->getConstant(i, &externalData, externalDataSize))
				{
					VariantBuffer::Ptr b = new VariantBuffer(externalData, externalDataSize);
					addConstant(name, var(b));
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
	bool skipProcessing = isBypassed() && !switchBypassFlag;

	

	if (object != nullptr && !skipProcessing)
	{
		if (data.isArray())
		{
			Array<var> *a = data.getArray();
			float *sampleData[4]; // this is an arbitrary amount, but it should be OK...
			int numSamples = -1;

			if (a == nullptr)
				throwError("processBlock must be called on array of buffers");

			for (int i = 0; i < jmin<int>(4, a->size()); i++)
			{
				VariantBuffer *b = a->getUnchecked(i).getBuffer();

				if (b != nullptr)
				{
					if (numSamples != -1 && b->size != numSamples)
						throwError("Buffer size mismatch");

					numSamples = b->size;
				}
				else throwError("processBlock must be called on array of buffers");

				sampleData[i] = b->buffer.getWritePointer(0);
			}

			if (switchBypassFlag)
			{
				float* sl = bypassSwitchBuffer.getWritePointer(0);
				float* sr = bypassSwitchBuffer.getWritePointer(1);

				const bool rampUp = !isBypassed();

				FloatVectorOperations::copy(sl, sampleData[0], numSamples);
				FloatVectorOperations::copy(sr, sampleData[1], numSamples);

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

				FloatVectorOperations::copy(sampleData[0], sl, numSamples);
				FloatVectorOperations::copy(sampleData[1], sr, numSamples);

				switchBypassFlag = false;

			}
			else
			{
				object->processBlock(sampleData, a->size(), numSamples);
			}

		}
		else if (data.isBuffer())
		{
			VariantBuffer *b = data.getBuffer();

			if (b != nullptr)
			{
				float *sampleData[1] = { b->buffer.getWritePointer(0) };
				const int numSamples = b->size;

				object->processBlock(sampleData, 1, numSamples);
			}
		}
		else throwError("Data Buffer is not valid");
	}
}

void DspInstance::setParameter(int index, float newValue)
{
	if (object != nullptr && index < object->getNumParameters())
	{
		object->setParameter(index, newValue);
	}
}

var DspInstance::getParameter(int index) const
{
	if (object != nullptr)
	{
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

		for (int i = 0; i < textLength; i++)
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

int DspInstance::getCachedIndex(const var &name) const
{
	return getConstantValue((int)name);
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
	if (object != nullptr)
	{
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
	}
}


void DspInstance::unload()
{
	if (factory != nullptr)
	{
		factory->destroyDspBaseObject(object);
		object = nullptr;
		factory = nullptr;
	}
}
