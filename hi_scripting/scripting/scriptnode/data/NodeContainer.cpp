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


NodeContainer::NodeContainer(DspNetwork* parent, ValueTree data_) :
	NodeBase(parent, data_, 0)
{

}

void NodeContainer::nodeAddedOrRemoved(ValueTree& child, bool wasAdded)
{
	if (auto nodeToProcess = getRootNetwork()->getNodeForValueTree(child))
	{
		if (wasAdded)
		{
			if (nodes.contains(nodeToProcess))
				return;

			int insertIndex = getNodeTree().indexOf(child);

			ScopedLock sl(getRootNetwork()->getConnectionLock());
			nodes.insert(insertIndex, nodeToProcess);
			updateChannels(getValueTree(), PropertyIds::NumChannels);
		}
		else
		{
			ScopedLock sl(getRootNetwork()->getConnectionLock());
			nodes.removeAllInstancesOf(nodeToProcess);
			updateChannels(getValueTree(), PropertyIds::NumChannels);
		}
	}
}


void NodeContainer::parameterAddedOrRemoved(ValueTree& child, bool wasAdded)
{
	if (wasAdded)
	{
		auto newParameter = new MacroParameter(this, child);
		addParameter(newParameter);
	}
	else
	{
		auto index = getParameterTree().indexOf(child);

		for (int i = 0; i < getNumParameters(); i++)
		{
			if (getParameter(i)->data == child)
			{
				removeParameter(i);
				break;
			}
		}
	}
}


void NodeContainer::updateChannels(ValueTree v, Identifier id)
{
	if (v == getValueTree())
	{
		channelLayoutChanged(nullptr);

		if (originalSampleRate > 0.0)
			prepare(originalSampleRate, originalBlockSize);
	}
	else if (v.getParent() == getNodeTree())
	{
		if (channelRecursionProtection)
			return;

		auto childNode = getRootNetwork()->getNodeForValueTree(v);

		ScopedValueSetter<bool> svs(channelRecursionProtection, true);

		channelLayoutChanged(childNode);

		if (originalSampleRate > 0.0)
			prepare(originalSampleRate, originalBlockSize);
	}
}

void NodeContainer::assign(const int index, var newValue)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	if (auto node = dynamic_cast<NodeBase*>(newValue.getObject()))
	{
		auto tree = node->getValueTree();

		getUndoManager()->beginNewTransaction();
		tree.getParent().removeChild(tree, getUndoManager());
		getNodeTree().addChild(tree, index, getUndoManager());
	}
	else
	{
		getNodeTree().removeChild(index, getUndoManager());
	}
}



juce::String NodeContainer::createCppClass(bool isOuterClass)
{
	String s;

	
	CppGen::Emitter::emitCommentLine(s, 1, "Node Definitions");
	s << getCppCode(CppGen::CodeLocation::Definitions);

	s << "\n";

	CppGen::Emitter::emitCommentLine(s, 1, "Member Nodes");

	for (auto n : nodes)
		s << n->createCppClass(false);

	s << "\n";

	CppGen::Emitter::emitCommentLine(s, 1, "Interface methods");

	CppGen::MethodInfo initMethod;
	initMethod.name = "initialise";
	initMethod.returnType = "void";
	initMethod.arguments = { "ProcessorWithScriptingContent* sp" };
	initMethod.specifiers = isOuterClass ? " override" : "";
	for (auto n : nodes)
		initMethod.body << n->getId() << ".initialise(sp);\n";
	CppGen::Emitter::emitFunctionDefinition(s, initMethod);

	CppGen::MethodInfo prepareMethod;
	prepareMethod.name = "prepare";
	prepareMethod.arguments = { "int numChannels", "double sampleRate", "int blockSize" };
	prepareMethod.returnType = "void";
	prepareMethod.body << getCppCode(CppGen::CodeLocation::PrepareBody);
	CppGen::Emitter::emitFunctionDefinition(s, prepareMethod);

	CppGen::MethodInfo processMethod;
	processMethod.name = "process";
	processMethod.arguments = { "ProcessData& data" };
	processMethod.returnType = "void";
	processMethod.body = getCppCode(CppGen::CodeLocation::ProcessBody);
	CppGen::Emitter::emitFunctionDefinition(s, processMethod);

	CppGen::MethodInfo processSingleMethod;
	processSingleMethod.name = "processSingle";
	processSingleMethod.arguments = { "float* frameData", "int numChannels" };
	processSingleMethod.returnType = "void";
	processSingleMethod.body = getCppCode(CppGen::CodeLocation::ProcessSingleBody);
	CppGen::Emitter::emitFunctionDefinition(s, processSingleMethod);

	CppGen::MethodInfo handleModMethod;
	handleModMethod.name = "handleModulation";
	handleModMethod.returnType = "bool";
	handleModMethod.arguments = { "ProcessData& data", "double& value" };
	handleModMethod.body = getCppCode(CppGen::CodeLocation::HandleModulationBody);
	CppGen::Emitter::emitFunctionDefinition(s, handleModMethod);

	CppGen::MethodInfo parameterMethod;
	parameterMethod.name = "createParameters";
	parameterMethod.arguments = { "Array<ParameterData>& data" };
	parameterMethod.returnType = "void";

	auto& pb = parameterMethod.body;

	pb << "Array<ParameterData> ip;\n";

	for (auto n : nodes)
	{
		auto id = n->getId();
		auto prefix = (dynamic_cast<NodeContainer*>(n.get()) != nullptr) ? "" : id;
		pb << "ip.addArray(createParametersT(&" << id << ", \"" << prefix << "\"));\n";
	}

	pb << "\n";

	CppGen::Emitter::emitCommentLine(pb, 2, "Parameter Initalisation");
	pb << "Array<ParameterInitValue> iv;\n";

	for (auto n : nodes)
	{
		for (int i = 0; i < n->getNumParameters(); i++)
		{
			auto pName = n->getParameter(i)->getId();
			auto pValue = n->getParameter(i)->getValue();

			pb << "iv.add({ \"" << n->getId() << "." << pName << "\", ";
			pb << CppGen::Emitter::createPrettyNumber(pValue, false) << " });\n";
		}
	}
	pb << "initParameterData(ip, iv);\n\n";

	if (isOuterClass)
	{
		CppGen::Emitter::emitCommentLine(pb, 2, "Parameter Callbacks");
		for (int i = 0; i < getNumParameters(); i++)
		{
			auto p = getParameter(i);

			String pCode;

			auto macro = dynamic_cast<MacroParameter*>(p);

			pCode << "ParameterData p(\"" << macro->getId() << "\");\n";
			pCode << "p.range = " << CppGen::Emitter::createRangeString(macro->inputRange) << ";\n";
			pCode << "auto rangeCopy = p.range;\n";
			pCode << "\n";

			StringArray connectionIds;

			for (auto c : macro->getConnectionTree())
			{
				String conId;
				conId << c[PropertyIds::NodeId].toString() << "." << c[PropertyIds::ParameterId].toString();
				
				auto conIdName = conId.replaceCharacter('.', '_');

				connectionIds.add(conIdName);

				pCode << "auto " << conIdName << " = getParameter(ip, \"" << conId << "\");\n";
			}

			pCode << "\n";

			CppGen::MethodInfo l;
			l.name = "[";

			for (auto c_id : connectionIds)
				l.name << c_id << ", ";

			l.name << "rangeCopy]";
			l.returnType << "p.db = ";
			l.arguments = { "float newValue" };
			
			l.body << "auto normalised = rangeCopy.convertTo0to1(newValue);\n";

			for (auto c_id : connectionIds)
				l.body << c_id << ".db(" << c_id << ".range.convertFrom0to1(normalised));\n";

			l.addSemicolon = true;

			CppGen::Emitter::emitFunctionDefinition(pCode, l);

			pCode << "data.add(std::move(p));\n";

			pb << CppGen::Emitter::surroundWithBrackets(pCode);
		}

	}
	else
	{
		pb << "data.addArray(ip);\n";
	}
#if 0
	else
	{
		CppGen::Emitter::emitCommentLine(parameterMethod.body, 0, "Add parameters to flat list");

		for (auto n : nodes)
			parameterMethod.body << "data.addArray(createParametersT(&" << n->getId() << ", \"" << n->getId() << "\"));\n";

		CppGen::Emitter::emitCommentLine(pb, 2, "Parameter Initalisation");
		pb << "Array<ParameterInitValue> iv;\n";

		for (auto n : nodes)
		{
			for (int i = 0; i < n->getNumParameters(); i++)
			{
				auto pName = n->getParameter(i)->getId();
				auto pValue = n->getParameter(i)->getValue();

				pb << "iv.add({ \"" << n->getId() << "." << pName << "\", ";
				pb << CppGen::Emitter::createPrettyNumber(pValue, false) << " });\n";
			}
		}
		pb << "initParameterData(ip, iv);\n\n";
	}
#endif

	CppGen::Emitter::emitFunctionDefinition(s, parameterMethod);

	CppGen::Emitter::emitCommentLine(s, 1, "Private Members");

	s << getCppCode(CppGen::CodeLocation::PrivateMembers);

	return CppGen::Emitter::createClass(s, getId(), isOuterClass);
}


juce::String NodeContainer::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s;
		
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_ID", getId(), true);

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
	parameterListener.setCallback(getParameterTree(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::parameterAddedOrRemoved));

	nodeListener.setCallback(getNodeTree(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::nodeAddedOrRemoved));

	channelListener.setCallback(data, { PropertyIds::NumChannels },
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::updateChannels));
}

SerialNode::SerialNode(DspNetwork* root, ValueTree data) :
	NodeContainer(root, data)
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
}

void ChainNode::process(ProcessData& data)
{
	if (isBypassed())
		return;

	for (auto n : nodes)
		n->process(data);
}


void ChainNode::processSingle(float* frameData, int numChannels)
{
	if (isBypassed())
		return;

	for (auto n : nodes)
		n->processSingle(frameData, numChannels);
}

String ChainNode::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s = NodeContainer::getCppCode(location);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "false", false);
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

	if (data[PropertyIds::ShowParameters])
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

	return {};
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

	if (data[PropertyIds::ShowParameters])
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


void SplitNode::prepare(double sampleRate, int blockSize)
{
	NodeContainer::prepare(sampleRate, blockSize);

	DspHelpers::increaseBuffer(splitBuffer, getNumChannelsToProcess() * 2, blockSize);
}

juce::String SplitNode::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s = NodeContainer::getCppCode(location);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "false", false);
		return s;
	}
	else if(location == CppGen::CodeLocation::PrepareBody)
	{
		String s = NodeContainer::getCppCode(location);

		s << "\nDspHelpers::increaseBuffer(splitBuffer, numChannels * 2, blockSize);\n";
		return s;
	}
	else if(location == CppGen::CodeLocation::ProcessBody)
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
	else if(location == CppGen::CodeLocation::PrivateMembers)
	{
		return "AudioSampleBuffer splitBuffer;\n";
	}
	
	return {};
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
	registerNode<container::chain>({});
	registerNode<container::split>({});
	registerNode<container::multi>({});
	registerNode<container::mod>({});
	registerNode<container::oversample2x>({});
	registerNode<container::oversample4x>({});
	registerNode<container::oversample8x>({});
	registerNode<container::oversample16x>({});
	registerNode<container::fix32_block>({});
	registerNode<container::fix64_block>({});
	registerNode<container::fix128_block>({});
	registerNode<container::fix256_block>({});
	registerNode<container::fix512_block>({});
	registerNode<container::fix1024_block>({});
	registerNode<container::frame1_block>({});
	registerNode<container::frame2_block>({});
	registerNode<container::frame3_block>({});
	registerNode<container::frame4_block>({});
	registerNode<container::frame6_block>({});
	registerNode<container::frame8_block>({});
	registerNode<container::frame16_block>({});
}


ModulationChainNode::ModulationChainNode(DspNetwork* n, ValueTree t) :
	SerialNode(n, t)
{
	initListeners();
}

void ModulationChainNode::processSingle(float* frameData, int numChannels) noexcept
{
	if (isBypassed())
		return;

	if (--singleCounter > 0) return;
	
	singleCounter = HISE_EVENT_RASTER;
	float value = 0.0f;

	for (auto n : nodes)
		n->processSingle(&value, 1);
}

void ModulationChainNode::process(ProcessData& data) noexcept
{
	if (isBypassed())
		return;

	int numToProcess = data.size / HISE_EVENT_RASTER;

	auto d = ALLOCA_FLOAT_ARRAY(numToProcess);
	CLEAR_FLOAT_ARRAY(d, numToProcess);
	ProcessData modData = { &d, 1, numToProcess };

	for (auto n : nodes)
		n->process(modData);
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
		[this](ValueTree& child, bool wasAdded)
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
	auto inputRange = RangeHelpers::getDoubleRange(data);

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
			setCallback([inputRange, connectionCallbacks](double newValue)
			{
				auto normedValue = inputRange.convertTo0to1(newValue);

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


void NodeContainer::MacroParameter::updateRangeForConnection(ValueTree& v, Identifier)
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

}

