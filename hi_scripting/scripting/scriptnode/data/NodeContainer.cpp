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


NodeContainer::NodeContainer()
{

}

void NodeContainer::nodeAddedOrRemoved(ValueTree child, bool wasAdded)
{
	auto n = asNode();
	if (auto nodeToProcess = n->getRootNetwork()->getNodeForValueTree(child))
	{
		if (wasAdded)
		{
			if (nodes.contains(nodeToProcess))
				return;

			int insertIndex = getNodeTree().indexOf(child);

			ScopedLock sl(n->getRootNetwork()->getConnectionLock());
			nodes.insert(insertIndex, nodeToProcess);
			updateChannels(n->getValueTree(), PropertyIds::NumChannels);
		}
		else
		{
			ScopedLock sl(n->getRootNetwork()->getConnectionLock());
			nodes.removeAllInstancesOf(nodeToProcess);
			updateChannels(n->getValueTree(), PropertyIds::NumChannels);
		}
	}
}


void NodeContainer::parameterAddedOrRemoved(ValueTree child, bool wasAdded)
{
	auto n = asNode();

	if (wasAdded)
	{
		auto newParameter = new MacroParameter(asNode(), child);
		n->addParameter(newParameter);
	}
	else
	{
		auto index = n->getParameterTree().indexOf(child);

		for (int i = 0; i < n->getNumParameters(); i++)
		{
			if (n->getParameter(i)->data == child)
			{
				n->removeParameter(i);
				break;
			}
		}
	}
}


void NodeContainer::updateChannels(ValueTree v, Identifier id)
{
	if (v == asNode()->getValueTree())
	{
		channelLayoutChanged(nullptr);

		if (originalSampleRate > 0.0)
		{
			PrepareSpecs ps;
			ps.numChannels = asNode()->getNumChannelsToProcess();
			ps.blockSize = originalBlockSize;
			ps.sampleRate = originalSampleRate;
			ps.voiceIndex = lastVoiceIndex;

			asNode()->prepare(ps);
		}
	}
	else if (v.getParent() == getNodeTree())
	{
		if (channelRecursionProtection)
			return;

		auto childNode = asNode()->getRootNetwork()->getNodeForValueTree(v);
		ScopedValueSetter<bool> svs(channelRecursionProtection, true);
		channelLayoutChanged(childNode);

		if (originalSampleRate > 0.0)
		{
			PrepareSpecs ps;
			ps.numChannels = asNode()->getNumChannelsToProcess();
			ps.blockSize = originalBlockSize;
			ps.sampleRate = originalSampleRate;
			ps.voiceIndex = lastVoiceIndex;

			asNode()->prepare(ps);
		}
	}
}

void NodeContainer::assign(const int index, var newValue)
{
	ScopedLock sl(asNode()->getRootNetwork()->getConnectionLock());

	auto un = asNode()->getUndoManager();

	if (auto node = dynamic_cast<NodeBase*>(newValue.getObject()))
	{
		auto tree = node->getValueTree();

		un->beginNewTransaction();
		tree.getParent().removeChild(tree, un);
		getNodeTree().addChild(tree, index, un);
	}
	else
	{
		getNodeTree().removeChild(index, un);
	}
}

juce::String NodeContainer::createTemplateAlias()
{
	String s;

	for (auto n : nodes)
	{
		if (auto c = dynamic_cast<NodeContainer*>(n.get()))
		{
			s << c->createTemplateAlias();
		}
	}

	StringArray children;

	for (auto n : nodes)
		children.add(n->createCppClass(false));

	s << CppGen::Emitter::createTemplateAlias(createCppClassForNodes(false), asNode()->getValueTree()[PropertyIds::FactoryPath].toString().replace(".", "::"), children);

	return s;
}


juce::String NodeContainer::createCppClassForNodes(bool isOuterClass)
{
	if (isOuterClass)
	{
		String s;
		CppGen::Emitter::emitCommentLine(s, 0, "Template Alias Definition");
		s << getCppCode(CppGen::CodeLocation::TemplateAlias);
		s << "\n";

		String classContent = getCppCode(CppGen::CodeLocation::Definitions);

		classContent << "\n";
		
		CppGen::MethodInfo parameterMethod;
		parameterMethod.name = "createParameters";


		parameterMethod.arguments = { "Array<ParameterData>& data" };
		parameterMethod.returnType = "void";

		auto& pb = parameterMethod.body;

		CppGen::Emitter::emitCommentLine(pb, 0, "Node Registration");

		Array<CppGen::Accessor> acessors;

		fillAccessors(acessors, {});

		for (const auto& a : acessors)
			pb << a.toString(CppGen::Accessor::Format::ParameterDefinition);

		pb << "\n";

		CppGen::Emitter::emitCommentLine(pb, 0, "Parameter Initalisation");

		for (auto n : getChildNodesRecursive())
		{
			for (int i = 0; i < n->getNumParameters(); i++)
			{
				auto pName = n->getParameter(i)->getId();
				auto pValue = n->getParameter(i)->getValue();

				pb << "setParameterDefault(\"" << n->getId() << "." << pName << "\", ";
				pb << CppGen::Emitter::createPrettyNumber(pValue, false) << ");\n";
			}
		}
		
		String modString;

		for (auto n : getChildNodesRecursive())
		{
			if (auto modSource = dynamic_cast<ModulationSourceNode*>(n.get()))
			{
				if (modSource->getModulationTargetTree().getNumChildren() == 0)
					continue;

				String mCode;
				StringArray modTargetIds;

				int tIndex = 1;

				for (auto m : modSource->getModulationTargetTree())
				{
					String modTargetId;
					modTargetId << m[PropertyIds::NodeId].toString() << "." << m[PropertyIds::ParameterId].toString();

					auto modIdName = "mod_target" + String(tIndex++);

					modTargetIds.add(modIdName);

					mCode << "auto " << modIdName << " = getParameter(\"" << modTargetId << "\", ";
					mCode << CppGen::Emitter::createRangeString(RangeHelpers::getDoubleRange(m)) << ");\n";
				}

				CppGen::MethodInfo l;
				l.name = "[";

				for (int i = 0; i < modTargetIds.size(); i++)
				{
					l.name << modTargetIds[i];
					
					if(i != modTargetIds.size() - 1)
						l.name << ", ";
				}
				
				l.name << "]";
				l.returnType << "auto f = ";
				l.arguments = { "double newValue" };

				for (auto c_id : modTargetIds)
					l.body << c_id << "(newValue);\n";

				l.addSemicolon = true;

				CppGen::Emitter::emitFunctionDefinition(mCode, l);

				String modAccessor;

				for (const auto& a : acessors)
				{
					if (a.id == modSource->getId())
					{
						modAccessor = a.toString(CppGen::Accessor::Format::GetMethod);
						break;
					}
				}

				mCode << "\nsetInternalModulationParameter(" << modAccessor << ", f);\n";

				modString << CppGen::Emitter::surroundWithBrackets(mCode);
			}
		}

		if (modString.isNotEmpty())
		{
			pb << "\n";
			CppGen::Emitter::emitCommentLine(pb, 0, "Internal Modulation");
			pb << modString;
		}

		String parameterString;

		auto n = asNode();

		for (int i = 0; i < n->getNumParameters(); i++)
		{
			auto p = n->getParameter(i);

			String pCode;

			auto macro = dynamic_cast<MacroParameter*>(p);

			pCode << "ParameterData p(\"" << macro->getId() << "\", ";
			pCode << CppGen::Emitter::createRangeString(macro->inputRange) << ");\n";
			pCode << "\n";

			StringArray connectionIds;

			int pIndex = 1;

			for (auto c : macro->getConnectionTree())
			{
				
				String conId;
				conId << c[PropertyIds::NodeId].toString() << "." << c[PropertyIds::ParameterId].toString();

				String conIdName;

				if (c[PropertyIds::ParameterId].toString() == PropertyIds::Bypassed.toString())
					conIdName = "bypass_target";
				else
					conIdName = "param_target";
				
				conIdName << String(pIndex++);

				connectionIds.add(conIdName);

				pCode << "auto " << conIdName << " = getParameter(\"" << conId << "\", ";
				pCode << CppGen::Emitter::createRangeString(RangeHelpers::getDoubleRange(c)) << ");\n";
			}

			pCode << "\n";

			CppGen::MethodInfo l;
			l.name = "[";

			for (auto c_id : connectionIds)
				l.name << c_id << ", ";

			l.name << "outer = p.range]";
			l.returnType << "p.db = ";
			l.arguments = { "double newValue" };

			l.body << "auto normalised = outer.convertTo0to1(newValue);\n";

			for (auto c_id : connectionIds)
			{
				if (c_id.startsWith("bypass_target"))
					l.body << c_id << ".setBypass(newValue);\n";
				else
					l.body << c_id << "(normalised);\n";
			}
				

			l.addSemicolon = true;

			CppGen::Emitter::emitFunctionDefinition(pCode, l);

			pCode << "\ndata.add(std::move(p));\n";

			parameterString << CppGen::Emitter::surroundWithBrackets(pCode);
		}

		if (parameterString.isNotEmpty())
		{
			pb << "\n";
			CppGen::Emitter::emitCommentLine(pb, 0, "Parameter Callbacks");
			pb << parameterString;
		}

		CppGen::Emitter::emitFunctionDefinition(classContent, parameterMethod);

		s << CppGen::Emitter::createClass(classContent, createCppClassForNodes(false));
		s = CppGen::Helpers::createIntendation(s);

		auto nid = n->getId();

		auto impl = CppGen::Emitter::wrapIntoNamespace(s, nid + "_impl");

		impl << "\n" << CppGen::Emitter::createAlias(nid, nid + "_impl::instance");

		return impl;
	}
	else
		return asNode()->getId() + "_";
	
}


juce::String NodeContainer::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::TemplateAlias)
	{
		return createTemplateAlias();
	}
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s;

		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_ID", asNode()->getId(), true);
		
		return s;
	}
	if (location == CppGen::CodeLocation::PrepareBody)
	{
		String s;

		for (auto n : nodes)
			s << n->getId() << ".prepare(numChannels, sampleRate, blockSize);\n";

		return s;
	}

	return {};
}

void NodeContainer::initListeners()
{
	parameterListener.setCallback(asNode()->getParameterTree(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::parameterAddedOrRemoved));

	nodeListener.setCallback(getNodeTree(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::nodeAddedOrRemoved));

	channelListener.setCallback(asNode()->getValueTree(), { PropertyIds::NumChannels },
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::updateChannels));
}

SerialNode::SerialNode(DspNetwork* root, ValueTree data) :
	NodeBase(root, data, 0)
{
}

NodeComponent* SerialNode::createComponent()
{
	return new SerialNodeComponent(this);
}


ChainNode::ChainNode(DspNetwork* n, ValueTree t) :
	SerialNode(n, t)
{
	initListeners();

	wrapper.getObject().initialise(this);

	setDefaultValue(PropertyIds::BypassRampTimeMs, 20.0);

	bypassListener.setCallback(t, { PropertyIds::Bypassed, PropertyIds::BypassRampTimeMs },
		valuetree::AsyncMode::Asynchronously,
		std::bind(&InternalWrapper::setBypassedFromValueTreeCallback, &wrapper, std::placeholders::_1, std::placeholders::_2));
}

void ChainNode::process(ProcessData& data)
{
	wrapper.process(data);
}


void ChainNode::processSingle(float* frameData, int numChannels)
{
	wrapper.processSingle(frameData, numChannels);
}

void ChainNode::prepare(PrepareSpecs ps)
{
	NodeContainer::prepareNodes(ps);
	wrapper.prepare(ps);
}

String ChainNode::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s = NodeContainer::getCppCode(location);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "false", false);

		return s;
	}
	else if (location == CppGen::CodeLocation::ProcessBody)
	{
		String s;
		s << "bypassHandler.process(data);\n";
		return s;
	}
	else if (location == CppGen::CodeLocation::ProcessSingleBody)
	{
		String s;
		s << "bypassHandler.processSingle(frameData, numChannels);\n";
		return s;
	}
	else if (location == CppGen::CodeLocation::PrepareBody)
	{
		String s = SerialNode::getCppCode(location);
		s << "bypassHandler.prepare(int numChannels, sampleRate, blockSize);\n";
		return s;

	}
	else if (location == CppGen::CodeLocation::PrivateMembers)
	{
		String s;
		s << "BypassHandler bypassHandler;\n";
		return s;
	}
	else
		return SerialNode::getCppCode(location);
}

juce::Rectangle<int> SerialNode::getPositionInCanvas(Point<int> topLeft) const
{
	using namespace UIValues;

	const int minWidth = NodeWidth;
	const int topRow = NodeHeight;

	int maxW = minWidth;
	int h = 0;

	h += UIValues::NodeMargin;
	h += UIValues::HeaderHeight; // the input

	if (v_data[PropertyIds::ShowParameters])
		h += UIValues::ParameterHeight;

	h += PinHeight; // the "hole" for the cable

	Point<int> childPos(NodeMargin, NodeMargin);

	for (auto n : nodes)
	{
		auto bounds = n->getPositionInCanvas(childPos);

		bounds = n->reduceHeightIfFolded(bounds);

		maxW = jmax<int>(maxW, bounds.getWidth());
		h += bounds.getHeight() + NodeMargin;
		childPos = childPos.translated(0, bounds.getHeight());
	}

	h += PinHeight; // the "hole" for the cable

	return { topLeft.getX(), topLeft.getY(), maxW + 2 * NodeMargin, h };
}



juce::String SerialNode::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::PrepareBody)
	{
		return NodeContainer::getCppCode(location);
	}
	if (location == CppGen::CodeLocation::ProcessBody)
	{
		String s;

		for (auto n : nodes)
			s << n->getId() << ".process(data);\n";

		return s;
	}
	if (location == CppGen::CodeLocation::ProcessSingleBody)
	{
		String s;

		for (auto n : nodes)
			s << n->getId() << ".processSingle(frameData, numChannels);\n";

		return s;
	}
	else
		return NodeContainer::getCppCode(location);

}

NodeComponent* ParallelNode::createComponent()
{
	return new ParallelNodeComponent(this);
}

juce::Rectangle<int> ParallelNode::getPositionInCanvas(Point<int> topLeft) const
{
	using namespace UIValues;

	int y = UIValues::NodeMargin;
	y += UIValues::HeaderHeight;
	y += UIValues::PinHeight;

	if (v_data[PropertyIds::ShowParameters])
		y += UIValues::ParameterHeight;

	Point<int> startPos(UIValues::NodeMargin, y);

	int maxy = startPos.getY();
	int maxWidth = NodeWidth + NodeMargin;

	for (auto n : nodes)
	{
		auto b = n->getPositionInCanvas(startPos);
		b = n->reduceHeightIfFolded(b);
		maxy = jmax(b.getBottom(), maxy);
		startPos = startPos.translated(b.getWidth() + UIValues::NodeMargin, 0);
		maxWidth = startPos.getX();

	}

	maxy += UIValues::PinHeight;



	maxy += UIValues::NodeMargin;

	return { topLeft.getX(), topLeft.getY(), maxWidth, maxy };
}


void SplitNode::prepare(PrepareSpecs ps)
{
	NodeContainer::prepareNodes(ps);

	ps.numChannels *= 2;
	DspHelpers::increaseBuffer(splitBuffer, ps);
}

juce::String SplitNode::getCppCode(CppGen::CodeLocation location)
{
	return NodeContainer::getCppCode(location);

#if 0
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s = NodeContainer::getCppCode(location);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "false", false);
		return s;
	}
	else if (location == CppGen::CodeLocation::TemplateAlias)
	{

	}
	else if (location == CppGen::CodeLocation::PrepareBody)
	{
		String s = NodeContainer::getCppCode(location);

		s << "\nDspHelpers::increaseBuffer(splitBuffer, numChannels * 2, blockSize);\n";
		return s;
	}
	else if (location == CppGen::CodeLocation::ProcessBody)
	{
		String s;
		s << "auto original = data.copyTo(splitBuffer, 0);\n\n";

		bool isFirst = true;

		for (auto n : nodes)
		{
			String code;

			if (isFirst)
			{
				code << n->getId() << ".process(data);\n";
				isFirst = false;
			}
			else
			{
				code << "auto wd = original.copyTo(splitBuffer, 1);\n";
				code << n->getId() << ".process(wd);\n";
				code << "data += wd;\n";
			}

			s << CppGen::Emitter::surroundWithBrackets(code);
		}

		return s;
	}
#endif
}


void SplitNode::process(ProcessData& data)
{
	if (isBypassed())
		return;

	auto original = data.copyTo(splitBuffer, 0);

	bool isFirst = true;

	for (auto n : nodes)
	{
		if (isFirst)
		{
			n->process(data);
			isFirst = false;
		}
		else
		{
			auto wd = original.copyTo(splitBuffer, 1);
			n->process(wd);
			data += wd;
		}
	}
}


void SplitNode::processSingle(float* frameData, int numChannels)
{
	if (isBypassed())
		return;

	float original[NUM_MAX_CHANNELS];
	memcpy(original, frameData, sizeof(float)*numChannels);
	bool isFirst = true;

	for (auto n : nodes)
	{
		if (isFirst)
		{
			n->processSingle(frameData, numChannels);
			isFirst = false;
		}
		else
		{
			float wb[NUM_MAX_CHANNELS];
			memcpy(wb, original, sizeof(float)*numChannels);
			n->processSingle(wb, numChannels);
			FloatVectorOperations::add(frameData, wb, numChannels);
		}
	}
}

NodeContainerFactory::NodeContainerFactory(DspNetwork* parent) :
	NodeFactory(parent)
{
	registerNode<ChainNode>({});
	registerNode<SplitNode>({});
	registerNode<MultiChannelNode>({});
	registerNode<ModulationChainNode>({});
	registerNode<FeedbackContainer>({});
	registerNode<EventProcessorNode>({});
	registerNode<OversampleNode<2>>({});
	registerNode<OversampleNode<4>>({});
	registerNode<OversampleNode<8>>({});
	registerNode<OversampleNode<16>>({});
	registerNode<FixedBlockNode<32>>({});
	registerNode<FixedBlockNode<64>>({});
	registerNode<FixedBlockNode<128>>({});
	registerNode<FixedBlockNode<256>>({});
	registerNode<FixedBlockNode<512>>({});
	registerNode<FixedBlockNode<1024>>({});
	registerNode<SingleSampleBlock<1>>({});
	registerNode<SingleSampleBlock<2>>({});
	registerNode<SingleSampleBlock<3>>({});
	registerNode<SingleSampleBlock<4>>({});
	registerNode<SingleSampleBlock<6>>({});
	registerNode<SingleSampleBlock<8>>({});
	registerNode<SingleSampleBlock<16>>({});
	
}


ModulationChainNode::ModulationChainNode(DspNetwork* n, ValueTree t) :
	ModulationSourceNode(n, t),
	NodeContainer()
{
	initListeners();
	obj.initialise(this);
}

void ModulationChainNode::processSingle(float* frameData, int numChannels) noexcept
{
	if (isBypassed())
		return;

	
	obj.processSingle(frameData, 1);

	auto thisValue = *frameData;

	if (thisValue != lastValue)
	{
		lastValue = thisValue;
		sendValueToTargets(thisValue, HISE_EVENT_RASTER);
	}
}

void ModulationChainNode::process(ProcessData& data) noexcept
{
	if (isBypassed())
		return;

	ProcessData copy(data);
	copy.numChannels = 1;

	obj.process(copy);

	double thisValue = 0.0;

	obj.handleModulation(thisValue);

	if (thisValue != lastValue)
	{
		lastValue = thisValue;
		sendValueToTargets(thisValue, data.size);
	}
}

scriptnode::NodeComponent* ModulationChainNode::createComponent()
{
	return new ModChainNodeComponent(this);
}

juce::String ModulationChainNode::getCppCode(CppGen::CodeLocation location)
{
	String s;

	if (location == CppGen::CodeLocation::Definitions)
	{
		String s = NodeContainer::getCppCode(location);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "true", false);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_EXTRA_COMPONENT", "60, ModulationSourcePlotter", false);
		return s;
	}
	if (location == CppGen::CodeLocation::ProcessBody)
	{
		s << "int numToProcess = data.size / HISE_EVENT_RASTER;\n\n";
		s << "auto d = ALLOCA_FLOAT_ARRAY(numToProcess);\n";
		s << "CLEAR_FLOAT_ARRAY(d, numToProcess);\n";
		s << "ProcessData modData(&d, 1, numToProcess);\n\n";

		for (auto n : nodes)
			s << n->getId() << ".process(modData);\n";


		s << "\nmodValue = DspHelpers::findPeak(modData);\n";
	}
	else if (location == CppGen::CodeLocation::ProcessSingleBody)
	{
		s << "if (--singleCounter > 0) return;\n\n";

		s << "singleCounter = HISE_EVENT_RASTER;\n";
		s << "float value = 0.0f;\n\n";

		for (auto n : nodes)
			s << n->getId() << ".processSingle(&value, 1);\n";
	}
	else if (location == CppGen::CodeLocation::PrepareBody)
	{
		s << "sampleRate /= (double)HISE_EVENT_RASTER;\n";
		s << "blockSize /= HISE_EVENT_RASTER;\n";
		s << "numChannels = 1;\n\n";

		s << NodeContainer::getCppCode(location);
	}
	else if (location == CppGen::CodeLocation::HandleModulationBody)
	{
		s << "value = modValue;\n";
		s << "return true;\n";
	}
	else if (location == CppGen::CodeLocation::PrivateMembers)
	{
		s << "int singleCounter = 0;\n";
		s << "double modValue = 0.0;\n";
	}


	return s;
}

juce::Rectangle<int> ModulationChainNode::getPositionInCanvas(Point<int> topLeft) const
{
	using namespace UIValues;

	const int minWidth = NodeWidth;
	const int topRow = NodeHeight;

	int maxW = minWidth;
	int h = 0;

	h += UIValues::NodeMargin;
	h += UIValues::HeaderHeight; // the input

	if (v_data[PropertyIds::ShowParameters])
		h += UIValues::ParameterHeight;

	h += PinHeight; // the "hole" for the cable

	Point<int> childPos(NodeMargin, NodeMargin);

	for (auto n : nodes)
	{
		auto bounds = n->getPositionInCanvas(childPos);

		bounds = n->reduceHeightIfFolded(bounds);

		maxW = jmax<int>(maxW, bounds.getWidth());
		h += bounds.getHeight() + NodeMargin;
		childPos = childPos.translated(0, bounds.getHeight());
	}

	h += PinHeight; // the "hole" for the cable

	return { topLeft.getX(), topLeft.getY(), maxW + 2 * NodeMargin, h };
}

NodeContainer::MacroParameter::Connection::Connection(NodeBase* parent, ValueTree& d)
{
	auto nodeId = d[PropertyIds::NodeId];

	if (auto targetNode = dynamic_cast<NodeBase*>(parent->getRootNetwork()->get(nodeId).getObject()))
	{
		auto parameterId = d[PropertyIds::ParameterId].toString();

		if (parameterId == PropertyIds::Bypassed.toString())
		{
			nodeToBeBypassed = targetNode;
			auto originalRange = RangeHelpers::getDoubleRange(d.getParent().getParent());
			rangeMultiplerForBypass = jlimit(1.0, 9000.0, originalRange.end);
		}
		else
		{
			for (int i = 0; i < targetNode->getNumParameters(); i++)
			{
				if (targetNode->getParameter(i)->getId() == parameterId)
				{
					p = targetNode->getParameter(i);
					opSyncer.setPropertiesToSync(d, p->data, { PropertyIds::OpType }, parent->getUndoManager());
					break;
				}
			}
		}
	}

	auto converterId = d[PropertyIds::Converter].toString();

	if (converterId.isNotEmpty())
		conversion = Identifier(converterId);

	auto opTypeId = d[PropertyIds::OpType].toString();

	if (opTypeId.isNotEmpty())
		opType = Identifier(opTypeId);

	connectionRange = RangeHelpers::getDoubleRange(d);
	inverted = d[PropertyIds::Inverted];
}


scriptnode::DspHelpers::ParameterCallback NodeContainer::MacroParameter::Connection::createCallbackForNormalisedInput()
{
	DspHelpers::ParameterCallback f;

	if (nodeToBeBypassed != nullptr)
	{
		auto n = nodeToBeBypassed;
		auto r = connectionRange.getRange();
		auto m = rangeMultiplerForBypass;

		if (inverted)
		{
			f = [n, r, m](double newValue)
			{
				if (n != nullptr)
					n.get()->setBypassed(r.contains(newValue*m));
			};
		}
		else
		{
			f = [n, r, m](double newValue)
			{
				if (n != nullptr)
					n.get()->setBypassed(!r.contains(newValue*m));
			};
		}

		return f;
	}
	else
	{
		if (opType == OperatorIds::Add)
			f = std::bind(&NodeBase::Parameter::addModulationValue, p.get(), std::placeholders::_1);
		else if (opType == OperatorIds::Multiply)
			f = std::bind(&NodeBase::Parameter::multiplyModulationValue, p.get(), std::placeholders::_1);
		else
			f = std::bind(&NodeBase::Parameter::setValueAndStoreAsync, p.get(), std::placeholders::_1);

		return DspHelpers::wrapIntoConversionLambda(conversion, f, connectionRange, inverted);
	}


}


juce::ValueTree NodeContainer::MacroParameter::getConnectionTree()
{
	auto existing = data.getChildWithName(PropertyIds::Connections);

	if (!existing.isValid())
	{
		existing = ValueTree(PropertyIds::Connections);
		data.addChild(existing, -1, parent->getUndoManager());
	}

	return existing;
}


NodeContainer::MacroParameter::MacroParameter(NodeBase* parentNode, ValueTree data_) :
	Parameter(parentNode, data_)
{
	rangeListener.setCallback(getConnectionTree(),
		RangeHelpers::getRangeIds(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(MacroParameter::updateRangeForConnection));

	connectionListener.setCallback(getConnectionTree(),
		valuetree::AsyncMode::Synchronously,
		[this](ValueTree child, bool wasAdded)
	{
		if (!wasAdded)
		{
			auto macroTargetId = child[PropertyIds::NodeId].toString();
			auto parameterId = child[PropertyIds::ParameterId].toString();

			if (auto macroTarget = parent->getRootNetwork()->getNodeWithId(macroTargetId))
			{
				if (parameterId == PropertyIds::Bypassed.toString())
				{
					macroTarget->getValueTree().removeProperty(PropertyIds::DynamicBypass, parent->getUndoManager());
				}
				else
				{
					if (auto p = macroTarget->getParameter(parameterId))
						p->data.removeProperty(PropertyIds::Connection, parent->getUndoManager());
				}
			}
		}

		rebuildCallback();
	});
}

void NodeContainer::MacroParameter::rebuildCallback()
{
	inputRange = RangeHelpers::getDoubleRange(data);

	Array<Connection> connections;
	auto cTree = data.getChildWithName(PropertyIds::Connections);
	connections.ensureStorageAllocated(cTree.getNumChildren());

	for (auto c : cTree)
	{
		Connection newC({ parent, c });

		if (newC.isValid())
			connections.add(std::move(newC));
	}

	if (connections.size() > 0)
	{

		Array<DspHelpers::ParameterCallback> connectionCallbacks;

		for (auto& c : connections)
			connectionCallbacks.add(c.createCallbackForNormalisedInput());

		if (RangeHelpers::isIdentity(inputRange))
		{
			setCallback([connectionCallbacks](double newValue)
			{
				for (auto& cb : connectionCallbacks)
					cb(newValue);
			});
		}
		else
		{
			auto cp = inputRange;
			setCallback([cp, connectionCallbacks](double newValue)
			{
				auto normedValue = cp.convertTo0to1(newValue);

				for (auto& cb : connectionCallbacks)
					cb(normedValue);
			});
		}
	}
	else
	{
		setCallback({});
	}
}


void NodeContainer::MacroParameter::updateRangeForConnection(ValueTree v, Identifier)
{
	RangeHelpers::checkInversion(v, &rangeListener, parent->getUndoManager());
	rebuildCallback();
}

void MultiChannelNode::channelLayoutChanged(NodeBase* nodeThatCausedLayoutChange)
{
	int numChannelsAvailable = getNumChannelsToProcess();
	int numNodes = nodes.size();

	if (numNodes == 0)
		return;

	// Use the ones with locked channel amounts first
	for (auto n : nodes)
	{
		if (n->hasFixChannelAmount())
		{
			numChannelsAvailable -= n->getNumChannelsToProcess();
			numNodes--;
		}
	}

	if (numNodes > 0)
	{
		int numPerNode = numChannelsAvailable / numNodes;

		for (auto n : nodes)
		{
			if (n->hasFixChannelAmount())
				continue;

			int thisNum = jmax(0, jmin(numChannelsAvailable, numPerNode));

			if (n != nodeThatCausedLayoutChange)
				n->setNumChannels(thisNum);

			numChannelsAvailable -= n->getNumChannelsToProcess();
		}
	}
}

EventProcessorNode::EventProcessorNode(DspNetwork* n, ValueTree t):
	SerialNode(n, t)
{
	obj.initialise(this);
	initListeners();
}

}
