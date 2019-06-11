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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;




DspNode::DspNode(DspNetwork* root, DspFactory* f_, ValueTree data) :
	NodeBase(root, data, NUM_API_FUNCTION_SLOTS),
	f(f_),
	moduleName(data[PropertyIds::FactoryPath].toString().fromFirstOccurrenceOf(".", false, false))
{
	if (!data.getChildWithName(PropertyIds::Parameters).isValid())
	{
		ValueTree p(PropertyIds::Parameters);
		data.addChild(p, -1, getUndoManager());
	}

	initialise();
}


NodeComponent* DspNode::createComponent()
{
	return new DefaultParameterNodeComponent(this);
}

void DspNode::initialise()
{
	if (DynamicDspFactory* dynamicFactory = dynamic_cast<DynamicDspFactory*>(f.get()))
	{
		if ((int)dynamicFactory->getErrorCode() != (int)LoadingErrorCode::LoadingSuccessful)
		{
			obj = nullptr;
			reportScriptError("Library is not correctly loaded. Error code: " + dynamicFactory->getErrorCode().toString());
		}
	}

	if (f != nullptr)
	{
		obj = f->createDspBaseObject(moduleName);

		if (obj != nullptr)
		{
			for (int i = 0; i < obj->getNumConstants(); i++)
			{
				char nameBuffer[64];
				int nameLength = 0;

				obj->getIdForConstant(i, nameBuffer, nameLength);

				String thisName(nameBuffer, nameLength);

				int intValue;
				if (obj->getConstant(i, intValue))
				{
					addConstant(thisName, var(intValue));
					continue;
				}

				float floatValue;
				if (obj->getConstant(i, floatValue))
				{
					addConstant(thisName, var(floatValue));
					continue;
				}

				char stringBuffer[512];
				size_t stringBufferLength;

				if (obj->getConstant(i, stringBuffer, stringBufferLength))
				{
					String text(stringBuffer, stringBufferLength);
					addConstant(thisName, var(text));
					continue;
				}

				float *externalData;
				int externalDataSize;

				if (obj->getConstant(i, &externalData, externalDataSize))
				{
					VariantBuffer::Ptr b = new VariantBuffer(externalData, externalDataSize);
					addConstant(thisName, var(b));
					continue;
				}
			}

			auto object = obj;
			WeakReference<ConstScriptingObject> weakThis = this;

			auto f = [object](int index, var newValue)
			{
				auto floatValue = (float)newValue;
				FloatSanitizers::sanitizeFloatNumber(floatValue);

				object->setParameter(index, floatValue);
			};

			for (int i = 0; i < obj->getNumParameters(); i++)
			{
				char buf[256];
				int len;
				obj->getIdForConstant(i, buf, len);

				String id(buf, len);

				Identifier pId(id);

				var value = obj->getParameter(i);

				auto pf = [object, i](double newValue)
				{
					object->setParameter(i, (float)newValue);
				};

				ValueTree parameterData = getParameterTree().getChildWithProperty(PropertyIds::ID, id);

				if (parameterData.isValid())
				{
					f(i, parameterData[PropertyIds::Value]);
				}
				else
				{
					auto initialValue = obj->getParameter(i);

					parameterData = ValueTree(PropertyIds::Parameter);
					parameterData.setProperty(PropertyIds::ID, id, nullptr);
					parameterData.setProperty(PropertyIds::Value, obj->getParameter(i), nullptr);
					parameterData.setProperty(PropertyIds::MinValue, 0.0f, nullptr);
					parameterData.setProperty(PropertyIds::MaxValue, initialValue == 0.0f ? 1.0f : initialValue, nullptr);

					getParameterTree().addChild(parameterData, -1, getUndoManager());
				}

				auto newP = new Parameter(this, parameterData);
				newP->setCallback(pf);

				addParameter(newP);
			}
		}
		else
		{
			reportScriptError("The module " + moduleName + " wasn't found in the Library.");
		}
	}
}


}

