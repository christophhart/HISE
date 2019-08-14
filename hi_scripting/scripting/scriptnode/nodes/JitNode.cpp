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

using namespace snex;

struct JitCodeHelpers
{
	static String createEmptyFunction(snex::jit::FunctionData f, bool allowExecution)
	{
		CppGen::MethodInfo info;

		info.name = f.id.toString();
		info.returnType = Types::Helpers::getTypeName(f.returnType);

		for (auto a : f.args)
		{
			info.arguments.add(Types::Helpers::getTypeName(a));
		}

		if(!allowExecution)
			info.body << "jassertfalse;\n";

		if (f.returnType != Types::ID::Void)
		{
			info.body << "return " << Types::Helpers::getCppValueString(VariableStorage(f.returnType, 0.0)) << ";\n";
		}
		
		String emptyFunction;

		CppGen::Emitter::emitFunctionDefinition(emptyFunction, info);

		return emptyFunction;
	}
};

snex::jit::CallbackCollection& JitNodeBase::getFirstCollection()
{
	if (auto poly = dynamic_cast<core::jit<NUM_POLYPHONIC_VOICES>*>(getInternalJitNode()))
	{
		return poly->cData.getFirst();
	}
	else if (auto mono = dynamic_cast<core::jit<1>*>(getInternalJitNode()))
	{
		return mono->cData.getFirst();
	}
}

juce::String JitNodeBase::convertJitCodeToCppClass(int numVoices, bool addToFactory)
{
	String id = asNode()->createCppClass(false);

	String content;

	auto& cc = getFirstCollection();

	CppGen::Emitter::emitCommentLine(content, 0, "Definitions");

	content << "\n";

	CppGen::Emitter::emitDefinition(content, "SET_HISE_NODE_ID", id, true);
	CppGen::Emitter::emitDefinition(content, "SET_HISE_FRAME_CALLBACK", cc.getBestCallbackName(0), false);
	CppGen::Emitter::emitDefinition(content, "SET_HISE_BLOCK_CALLBACK", cc.getBestCallbackName(1), false);

	content << "\n";

	CppGen::Emitter::emitCommentLine(content, 0, "SNEX code body");

	content << asNode()->getNodeProperty(PropertyIds::Code).toString();

	if (!content.endsWith("\n"))
		content << "\n";

	String missingFunctions;

	snex::jit::FunctionData f;

	if (!cc.resetFunction)
	{
		f.id = "reset";
		f.returnType = Types::ID::Void;
		f.args = {};

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, true);
	}

	if (!cc.eventFunction)
	{
		f.id = "handleEvent";
		f.returnType = Types::ID::Void;
		f.args = {Types::ID::Event};

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, true);
	}

	if (!cc.prepareFunction)
	{
		f.id = "prepare";
		f.returnType = Types::ID::Void;
		f.args = { Types::ID::Double, Types::ID::Integer, Types::ID::Integer };

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, true);
	}

	if (!cc.callbacks[CallbackTypes::Channel])
	{
		f.id = "processChannel";
		f.returnType = Types::ID::Void;
		f.args = { Types::ID::Block, Types::ID::Integer };

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, false);
	}

	if (!cc.callbacks[CallbackTypes::Frame])
	{
		f.id = "processFrame";
		f.returnType = Types::ID::Void;
		f.args = { Types::ID::Block };

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, false);
	}

	if (!cc.callbacks[CallbackTypes::Sample])
	{
		f.id = "processSample";
		f.returnType = Types::ID::Float;
		f.args = { Types::ID::Float };

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, false);
	}

	CppGen::Emitter::emitCommentLine(missingFunctions, 0, "Parameter & Freeze functions");

	if (!cc.parameters.isEmpty())
	{
		CppGen::MethodInfo createParameterFunction;
		createParameterFunction.returnType = "void";
		createParameterFunction.name = "createParameters";
		createParameterFunction.specifiers = " override";

		auto& cBody = createParameterFunction.body;
		auto memberPrefix = "BIND_MEMBER_FUNCTION_1(instance::";

		for (auto p : cc.parameters)
		{
			cBody << "addParameter(\"" << p.name << "\", ";
			cBody << memberPrefix << p.f.id.toString() << "));\n";
		}

		CppGen::Emitter::emitFunctionDefinition(missingFunctions, createParameterFunction);
	}

	CppGen::MethodInfo snippetFunction;
	snippetFunction.name = "getSnippetText";
	snippetFunction.returnType = "String";
	snippetFunction.specifiers = " const override";

	snippetFunction.body << "return \"";
	snippetFunction.body << ValueTreeConverters::convertValueTreeToBase64(asNode()->getValueTree(), true);
	snippetFunction.body << "\";\n";

	CppGen::Emitter::emitFunctionDefinition(missingFunctions, snippetFunction);

	CppGen::Emitter::emitCommentLine(content, 0, "Autogenerated empty functions");

	content << "\n";

	content << missingFunctions;

	auto lines = StringArray::fromLines(content);

	for (auto& l : lines)
	{
		while (l.startsWithChar('\t'))
			l = l.substring(1);
	}

	content = CppGen::Emitter::createJitClass(id, lines.joinIntoString("\n"));

	if (addToFactory)
		content << CppGen::Emitter::createFactoryMacro(numVoices > 1, true);

	content = CppGen::Emitter::wrapIntoNamespace(content, id + "_impl");

	String className = id + "_impl::instance";
	String templateName = "hardcoded_jit";
	String voiceAmount = (numVoices == 1) ? "1" : "NUM_POLYPHONIC_VOICES";
	
	content << CppGen::Emitter::createTemplateAlias(id, templateName, {className, voiceAmount});

	return content;
}

juce::String JitPolyNode::createCppClass(bool isOuterClass)
{
	if (isOuterClass)
		return CppGen::Helpers::createIntendation(convertJitCodeToCppClass(NUM_POLYPHONIC_VOICES, true));
	else
		return getId();
}

juce::String JitNode::createCppClass(bool isOuterClass)
{
	if (isOuterClass)
		return CppGen::Helpers::createIntendation(convertJitCodeToCppClass(1, true));
	else
		return getId();
}

}

