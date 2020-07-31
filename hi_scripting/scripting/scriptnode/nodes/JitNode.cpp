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

#if HISE_INCLUDE_SNEX
    
using namespace snex;

struct JitCodeHelpers
{
	static String createEmptyFunction(snex::jit::FunctionData f, bool allowExecution)
	{
		CppGen::MethodInfo info;

		info.name = f.id.toString();
		info.returnType = f.returnType.toString();

		for (auto a : f.args)
		{
			info.arguments.add(a.toString());
		}

		if(!allowExecution)
			info.body << "jassertfalse;\n";

		if (f.returnType != Types::ID::Void)
		{
			info.body << "return " << Types::Helpers::getCppValueString(VariableStorage(f.returnType.getType(), 0.0)) << ";\n";
		}
		
		String emptyFunction;

		CppGen::Emitter::emitFunctionDefinition(emptyFunction, info);

		return emptyFunction;
	}
};


void HiseBufferHandler::registerExternalItems()
{
	if (auto cdh = dynamic_cast<ComplexDataHolder*>(processor.get()))
	{
		for (int i = 0; i < cdh->getNumAudioFiles(); i++)
		{
			auto af = cdh->addOrReturnAudioFile(i);

			auto& buffer = af->getBuffer()->range;

			int numChannels = buffer.getNumChannels();

			if (buffer.getNumSamples() > 0)
			{
				snex::block b(buffer.getWritePointer(0), buffer.getNumSamples());
				registerAudioFile(i, b);
			}
		}
	}
}

namespace core
{

template <int NV>
jit_impl<NV>::jit_impl() :
	code(PropertyIds::Code, "")
{
	
}

template <int NV>
void jit_impl<NV>::handleAsyncUpdate()
{
	if (auto l = SingleWriteLockfreeMutex::ScopedWriteLock(lock))
	{
		auto compileEachVoice = [this](CallbackCollection& c)
		{
			snex::jit::Compiler compiler(scope);

			auto newObject = compiler.compileJitObject(lastCode);

			if (compiler.getCompileResult().wasOk())
			{
				c.obj = newObject;
				c.setupCallbacks();
				c.prepare(lastSpecs.sampleRate, lastSpecs.blockSize, lastSpecs.numChannels);
			}
		};

		cData.forEachVoice(compileEachVoice);
	}
	else
	{
		DBG("DEFER");
		triggerAsyncUpdate();
	}
}

template <int NV>
void jit_impl<NV>::updateCode(Identifier id, var newValue)
{
	auto newCode = newValue.toString();

	if (newCode != lastCode)
	{
		lastCode = newCode;
		handleAsyncUpdate();
	}
}

template <int NV>
void jit_impl<NV>::prepare(PrepareSpecs specs)
{
	cData.prepare(specs);

	lastSpecs = specs;

	voiceIndexPtr = specs.voiceIndex;

	if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
	{
		cData.forEachVoice([specs](CallbackCollection& c)
		{
			c.prepare(specs.sampleRate, specs.blockSize, specs.numChannels);
		});
	}
}

template <int NV>
void jit_impl<NV>::createParameters(Array<ParameterData>& data)
{
	code.init(nullptr, this);

	if (auto l = SingleWriteLockfreeMutex::ScopedWriteLock(lock))
	{
		if (!createParameter<0>(data))
			return;
		if (!createParameter<1>(data))
			return;
		if (!createParameter<2>(data))
			return;
		if (!createParameter<3>(data))
			return;
		if (!createParameter<4>(data))
			return;
		if (!createParameter<5>(data))
			return;
		if (!createParameter<6>(data))
			return;
		if (!createParameter<7>(data))
			return;
		if (!createParameter<8>(data))
			return;
		if (!createParameter<9>(data))
			return;
		if (!createParameter<10>(data))
			return;
		if (!createParameter<11>(data))
			return;
		if (!createParameter<12>(data))
			return;
	}
}

template <int NV>
void jit_impl<NV>::initialise(NodeBase* n)
{
	node = n;

	scope.addOptimization(OptimizationIds::ConstantFolding);
	scope.addOptimization(OptimizationIds::DeadCodeElimination);
	scope.addOptimization(OptimizationIds::Inlining);
	scope.addOptimization(OptimizationIds::BinaryOpOptimisation);

	scope.setBufferHandler(new HiseBufferHandler(dynamic_cast<Processor*>(n->getScriptProcessor())));

	voiceIndexPtr = n->getRootNetwork()->getVoiceIndexPtr();

	code.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(jit_impl::updateCode));
	code.init(n, this);
}

template <int NV>
void jit_impl<NV>::handleHiseEvent(HiseEvent& e)
{
	if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
	{
		if (cData.isMonophonicOrInsideVoiceRendering())
			cData.get().eventFunction.callVoid(e);
	}
}

template <int NV>
bool jit_impl<NV>::handleModulation(double&)
{
	return false;
}

template <int NV>
void jit_impl<NV>::reset()
{
	if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
	{
		if (cData.isMonophonicOrInsideVoiceRendering())
			cData.get().resetFunction.callVoid();
		else
			cData.forEachVoice([](CallbackCollection& c) {c.resetFunction.callVoid(); });
	}
}

template <int NV>
void jit_impl<NV>::process(ProcessData& d)
{
	if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
	{
		auto& cc = cData.get();

		auto bestCallback = cc.bestCallback[CallbackCollection::ProcessType::BlockProcessing];

		switch (bestCallback)
		{
		case CallbackTypes::Channel:
		{
			for (int c = 0; c < d.numChannels; c++)
			{
				snex::block b(d.data[c], d.size);
				cc.callbacks[CallbackTypes::Channel].callVoidUnchecked(b, c);
			}
			break;
		}
		case CallbackTypes::Frame:
		{
			float frame[NUM_MAX_CHANNELS];
			float* frameData[NUM_MAX_CHANNELS];
			memcpy(frameData, d.data, sizeof(float*) * d.numChannels);
			ProcessData copy(frameData, d.numChannels, d.size);
			copy.allowPointerModification();

			for (int i = 0; i < d.size; i++)
			{
				copy.copyToFrameDynamic(frame);

				snex::block b(frame, d.numChannels);
				cc.callbacks[CallbackTypes::Frame].callVoidUnchecked(b);

				copy.copyFromFrameAndAdvanceDynamic(frame);
			}

			break;
		}
		case CallbackTypes::Sample:
		{
			for (int c = 0; c < d.numChannels; c++)
			{
				for (int i = 0; i < d.size; i++)
				{
					auto value = d.data[c][i];
					d.data[c][i] = cc.callbacks[CallbackTypes::Sample].template callUncheckedWithCopy<float>(value);
				}
			}

			break;
		}
		default:
			break;
		}
	}
}

template <int NV>
void jit_impl<NV>::processSingle(float* frameData, int numChannels)
{
	if (auto l = SingleWriteLockfreeMutex::ScopedReadLock(lock))
	{
		auto& cc = cData.get();

		auto bestCallback = cc.bestCallback[CallbackCollection::ProcessType::FrameProcessing];

		switch (bestCallback)
		{
		case CallbackTypes::Frame:
		{
			snex::block b(frameData, numChannels);
			cc.callbacks[bestCallback].callVoidUnchecked(b);
			break;
		}
		case CallbackTypes::Sample:
		{
			for (int i = 0; i < numChannels; i++)
			{
				auto v = static_cast<float>(frameData[i]);
				auto& f = cc.callbacks[bestCallback];
				frameData[i] = f.template callUnchecked<float>(v);
			}
			break;
		}
		case CallbackTypes::Channel:
		{
			for (int i = 0; i < numChannels; i++)
			{
				snex::block b(frameData + i, 1);
				cc.callbacks[bestCallback].callVoidUnchecked(b);
			}
			break;
		}
		default:
			break;
		}
	}
}

DEFINE_EXTERN_NODE_TEMPIMPL(jit_impl);

struct SimpleJitComponent : public Component,
							public TextEditor::Listener
{
	SimpleJitComponent(simple_jit* parent_) :
		parent(parent_)
	{
		te.setFont(GLOBAL_MONOSPACE_FONT());
		te.setColour(TextEditor::ColourIds::backgroundColourId, Colour(0xFF333333));
		te.setColour(TextEditor::ColourIds::textColourId, Colours::white);
		te.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.6f));
		te.setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
		te.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
		te.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
		te.addListener(this);
		addAndMakeVisible(te);
		te.setText(parent->codeValue.toString(), dontSendNotification);
		setSize(384, 32);
	}

	void textEditorReturnKeyPressed(TextEditor& te) override
	{
		parent->codeValue.setValue(te.getText());
		repaint();
	}

	void paint(Graphics& g) override
	{
		if (parent->expr != nullptr)
		{
			if (parent->expr->isValid())
			{
				g.fillAll(Colours::green.withSaturation(0.2f).withBrightness(0.4f));
				
			}
			else
			{
				g.fillAll(Colours::red.withSaturation(0.2f).withBrightness(0.2f));
				g.setColour(Colours::white.withAlpha(0.6f));
				g.setFont(GLOBAL_BOLD_FONT());
				g.drawText(parent->expr->getErrorMessage(), getLocalBounds().removeFromBottom(26).toFloat().reduced(2.0f), Justification::left);
			}
		}
		else
		{
			g.fillAll(Colours::white);
		}
	}
	
	void resized() override
	{
		te.setBounds(getLocalBounds().removeFromTop(24));
	}

	simple_jit* parent;
	TextEditor te;
};

juce::Component* simple_jit::createExtraComponent(PooledUIUpdater* updater)
{
	return new SimpleJitComponent(this);
}


}



void JitNodeBase::initUpdater()
{
	parameterUpdater.setCallback(asNode()->getPropertyTree().getChildWithProperty(PropertyIds::ID, PropertyIds::Code.toString()),
		{ PropertyIds::Value }, valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(JitNodeBase::updateParameters));
}

snex::CallbackCollection& JitNodeBase::getFirstCollection()
{
	if (auto poly = dynamic_cast<core::jit_poly*>(getInternalJitNode()))
	{
		return poly->cData.getFirst();
	}
	else if (auto mono = dynamic_cast<core::jit*>(getInternalJitNode()))
	{
		return mono->cData.getFirst();
	}
    else
    {
        jassertfalse;
        static snex::CallbackCollection c;
        return c;
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

#if 0
	if (!cc.resetFunction)
	{
		f.id = "reset";
		f.returnType = TypeInfo(Types::ID::Void, false);
		f.args = {};

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, true);
	}

	if (!cc.eventFunction)
	{
		f.id = "handleEvent";
		f.returnType = TypeInfo(Types::ID::Void, false);
		f.args.add( Symbol::createRootSymbol("e").withType(Types::ID::Event));

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, true);
	}

	if (!cc.prepareFunction)
	{
		f.id = "prepare";
		f.returnType = TypeInfo(Types::ID::Void, false);
		f.args = { Symbol::createRootSymbol("sampleRate").withType(Types::ID::Double),
			Symbol::createRootSymbol("blockSize").withType(Types::ID::Integer),
			Symbol::createRootSymbol("numChannels").withType(Types::ID::Integer)
		};

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, true);
	}

	if (!cc.callbacks[CallbackTypes::Channel])
	{
		f.id = "processChannel";
		f.returnType = TypeInfo(Types::ID::Void, false);
		f.args = { Symbol::createRootSymbol("channelData").withType(Types::ID::Block),
			Symbol::createRootSymbol("channelIndex").withType(Types::ID::Integer)};

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, false);
	}

#if 0
	if (!cc.callbacks[CallbackTypes::Frame])
	{
		f.id = "processFrame";
		f.returnType = TypeInfo(Types::ID::Void, false);
		f.args = { TypeInfo(Types::ID::Block) };

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, false);
	}

	if (!cc.callbacks[CallbackTypes::Sample])
	{
		f.id = "processSample";
		f.returnType = TypeInfo(Types::ID::Float, false);
		f.args = { TypeInfo(Types::ID::Float) };

		missingFunctions << JitCodeHelpers::createEmptyFunction(f, false);
	}
#endif

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

#endif

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

void JitNodeBase::updateParameters(Identifier id, var newValue)
{
	auto obj = getInternalJitNode();

	Array<HiseDspBase::ParameterData> l;
	obj->createParameters(l);

	StringArray foundParameters;

	for (int i = 0; i < asNode()->getNumParameters(); i++)
	{
		auto pId = asNode()->getParameter(i)->getId();

		bool found = false;

		for (auto& p : l)
		{
			if (p.id == pId)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			auto v = asNode()->getParameter(i)->data;
			v.getParent().removeChild(v, asNode()->getUndoManager());
			asNode()->removeParameter(i--);
		}

	}

	for (const auto& p : l)
	{
		foundParameters.add(p.id);

		if (auto param = asNode()->getParameter(p.id))
		{
			p.db(param->getValue());
		}
		else
		{
			auto newTree = p.createValueTree();
			asNode()->getParameterTree().addChild(newTree, -1, nullptr);

			auto newP = new NodeBase::Parameter(asNode(), newTree);
			newP->setCallback(p.db);
			asNode()->addParameter(newP);
		}
	}
}

JitPolyNode::JitPolyNode(DspNetwork* parent, ValueTree d) :
	HiseDspNodeBase<core::jit_poly>(parent, d)
{
	dynamic_cast<core::jit_poly*>(getInternalT())->scope.addDebugHandler(this);
	initUpdater();
}

juce::String JitPolyNode::createCppClass(bool isOuterClass)
{
	if (isOuterClass)
		return CppGen::Helpers::createIntendation(convertJitCodeToCppClass(NUM_POLYPHONIC_VOICES, true));
	else
		return getId();
}

scriptnode::HiseDspBase* JitPolyNode::getInternalJitNode()
{
	return wrapper.getInternalT();
}

JitNode::JitNode(DspNetwork* parent, ValueTree d) :
	HiseDspNodeBase<core::jit>(parent, d)
{
	dynamic_cast<core::jit*>(getInternalT())->scope.addDebugHandler(this);
	initUpdater();
}

juce::String JitNode::createCppClass(bool isOuterClass)
{
	if (isOuterClass)
		return CppGen::Helpers::createIntendation(convertJitCodeToCppClass(1, true));
	else
		return getId();
}

scriptnode::HiseDspBase* JitNode::getInternalJitNode()
{
	return wrapper.getInternalT();
}
    
#endif

}

