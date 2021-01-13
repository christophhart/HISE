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

int JitCompiledNode::getNumRequiredDataObjects(ExternalData::DataType t) const
{
	return numRequiredDataTypes[(int)t];
}

void JitCompiledNode::setExternalData(const ExternalData& b, int index)
{
	auto ptr = (void*)&b;
	setExternalDataFunction.callVoid(ptr, index);
}

JitCompiledNode::JitCompiledNode(Compiler& c, const String& code, const String& classId, int numChannels_, const CompilerInitFunction& cf) :
	r(Result::ok()),
	numChannels(numChannels_)
{
	String s;

	memset(numRequiredDataTypes, 0, sizeof(int) * (int)ExternalData::DataType::numDataTypes);

	auto wrapInNamespace = !code.contains("namespace impl");

	NamespacedIdentifier implId;

	if (wrapInNamespace)
	{
		implId = NamespacedIdentifier::fromString("impl::" + classId);

		s << "namespace impl { " << code;
		s << "}\n";
		
	}
	else
	{
		s << code;
		implId = NamespacedIdentifier(classId);
		
	}

	s << implId.toString() << " instance;\n";


	cf(c, numChannels);

	Array<Identifier> fIds;

	// Cheapmaster solution 2000
	if (s.contains("void setExternalData(const ExternalData&"))
	{
		s << "void setExternalData(const ExternalData& d, int index) { instance.setExternalData(d, index); }\n";
	}

	for (auto f : Types::ScriptnodeCallbacks::getAllPrototypes(c, numChannels))
	{
		addCallbackWrapper(s, f);
		fIds.add(f.id.getIdentifier());
	}

	obj = c.compileJitObject(s);

	r = c.getCompileResult();

	if (r.wasOk())
	{
		NamespacedIdentifier impl("impl");

		

		if (instanceType = c.getComplexType(implId))
		{
			
			if (auto libraryNode = dynamic_cast<SnexNodeBase*>(instanceType.get()))
			{
				parameterList = libraryNode->getParameterList();
			}
			if (auto st = dynamic_cast<StructType*>(instanceType.get()))
			{
				WrapBuilder::InnerData inner(instanceType, WrapBuilder::GetObj);

				Array<cppgen::ParameterEncoder::Item> encoderData;

				if (inner.resolve())
				{
					if (auto metadataType = dynamic_cast<jit::StructType*>(c.getComplexType(inner.st->id.getChildId("metadata")).get()))
					{
						scriptnode::ParameterEncoder encoder(metadataType->createByteBlock());

						for (auto& p: encoder)
						{
							encoderData.add(p);
							addParameterMethod(s, p.id, p.index);
						}
					}
					else
					{
						auto pId = st->id.getChildId("Parameters");
						auto pNames = c.getNamespaceHandler().getEnumValues(pId);

						if (!pNames.isEmpty())
						{
							for (int i = 0; i < pNames.size(); i++)
							{
								cppgen::ParameterEncoder::Item item;

								item.index = i;
								item.id = pNames[i];
								addParameterMethod(s, pNames[i], i);
							}
						}
					}
				}
				
				cf(c, numChannels);

				obj = c.compileJitObject(s);
				r = c.getCompileResult();

				auto& nh = c.getNamespaceHandler();

				numRequiredDataTypes[(int)ExternalData::DataType::Table] = nh.getConstantValue(st->id.getChildId("NumTables")).toInt();
				numRequiredDataTypes[(int)ExternalData::DataType::SliderPack] = nh.getConstantValue(st->id.getChildId("NumSliderPacks")).toInt();
				numRequiredDataTypes[(int)ExternalData::DataType::AudioFile] = nh.getConstantValue(st->id.getChildId("NumAudioFiles")).toInt();

				instanceType = c.getComplexType(implId);

				for (auto& n : encoderData)
				{
					auto f = obj[Identifier("set" + n.id)];

					OpaqueSnexParameter osp;
					osp.data = n;
					osp.function = f.function;
					parameterList.add(osp);
				}
			}

			FunctionClass::Ptr fc = instanceType->getFunctionClass();

			thisPtr = obj.getMainObjectPtr();
			ok = true;



			for (int i = 0; i < fIds.size(); i++)
			{
				callbacks[i] = obj[fIds[i]];

				Array<FunctionData> matches;

				fc->addMatchingFunctions(matches, fc->getClassName().getChildId(fIds[i]));

				FunctionData wrappedFunction;

				if (matches.size() == 1)
					wrappedFunction = matches.getFirst();
				else
				{
					for (auto m : matches)
					{
						if (m.matchesArgumentTypes(callbacks[i]))
						{
							wrappedFunction = m;
							break;
						}
					}
				}

				if (!wrappedFunction.matchesArgumentTypes(callbacks[i]))
				{
					r = Result::fail(wrappedFunction.getSignature({}, false) + " doesn't match " + callbacks[i].getSignature({}, false));
					ok = false;
					break;
				}

				if (callbacks[i].function == nullptr)
					ok = false;
			}

			setExternalDataFunction = obj["setExternalData"];

			if (setExternalDataFunction.isResolved())
			{
				ExternalData d;
				setExternalData(d, -1);
			}

		}
		else
		{
			jassertfalse;
		}
	}
}


}
}