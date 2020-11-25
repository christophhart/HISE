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

void NodeContainer::resetNodes()
{

	for (auto n : nodes)
		n->reset();
}

scriptnode::NodeBase* NodeContainer::asNode()
{

	auto n = dynamic_cast<NodeBase*>(this);
	jassert(n != nullptr);
	return n;
}

const scriptnode::NodeBase* NodeContainer::asNode() const
{

	auto n = dynamic_cast<const NodeBase*>(this);
	jassert(n != nullptr);
	return n;
}

void NodeContainer::prepareContainer(PrepareSpecs& ps)
{
	auto& lock = asNode()->getRootNetwork()->getConnectionLock();
	ScopedTryLock sl(lock);

	// You need to lock this before anyway...
	jassert(sl.isLocked());

	originalSampleRate = ps.sampleRate;
	originalBlockSize = ps.blockSize;
	lastVoiceIndex = ps.voiceIndex;

	ps.sampleRate = getSampleRateForChildNodes();
	ps.blockSize = getBlockSizeForChildNodes();
}

void NodeContainer::prepareNodes(PrepareSpecs ps)
{
	prepareContainer(ps);

	for (auto n : nodes)
	{
		n->prepare(ps);
		n->prepareParameters(ps);
		n->reset();
	}
}

bool NodeContainer::shouldCreatePolyphonicClass() const
{
	if (isPolyphonic())
	{
		for (auto n : getNodeList())
		{
			if (auto nc = dynamic_cast<NodeContainer*>(n.get()))
			{
				if (nc->shouldCreatePolyphonicClass())
					return true;
			}

			if (n->isPolyphonic())
				return true;
		}

		return false;
	}

	return false;
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

			nodeToProcess->setParentNode(asNode());

			int insertIndex = getNodeTree().indexOf(child);

			ScopedLock sl(n->getRootNetwork()->getConnectionLock());
			nodes.insert(insertIndex, nodeToProcess);
			updateChannels(n->getValueTree(), PropertyIds::NumChannels);
		}
		else
		{
			nodeToProcess->setParentNode(nullptr);

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

bool NodeContainer::isPolyphonic() const
{

	if (auto n = dynamic_cast<NodeContainer*>(asNode()->getParentNode()))
	{
		return n->isPolyphonic();
	}
	else
		return asNode()->getRootNetwork()->isPolyphonic();
}

void NodeContainer::assign(const int index, var newValue)
{
	ScopedLock sl(asNode()->getRootNetwork()->getConnectionLock());

	auto un = asNode()->getUndoManager();

	if (auto node = dynamic_cast<NodeBase*>(newValue.getObject()))
	{
		auto tree = node->getValueTree();

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

	if (nodes.size() > 0)
	{
		for (auto n : nodes)
		{
			if (auto c = dynamic_cast<NodeContainer*>(n.get()))
				s << c->createTemplateAlias();
		}

		StringArray children;

		for (auto n : nodes)
			children.add(CppGen::Emitter::addNodeTemplateWrappers(n->createCppClass(false), n));

		s << CppGen::Emitter::createTemplateAlias(createCppClassForNodes(false), asNode()->getValueTree()[PropertyIds::FactoryPath].toString().replace(".", "::"), children);
	}
	else
	{
		s << CppGen::Emitter::createAlias(createCppClassForNodes(false), "core::empty");
	}

	
	return s;
}

juce::String NodeContainer::createJitClasses()
{
	String s;

	for (auto n : nodes)
	{
		if (auto c = dynamic_cast<NodeContainer*>(n.get()))
			s << c->createJitClasses();

#if HISE_INCLUDE_SNEX
		if (auto j = dynamic_cast<JitNodeBase*>(n.get()))
			s << j->convertJitCodeToCppClass(isPolyphonic() ? NUM_POLYPHONIC_VOICES : 1, false);
#endif
	}
	

	return s;
}

scriptnode::NodeBase::List NodeContainer::getChildNodesRecursive()
{
	NodeBase::List l;

	for (auto n : nodes)
	{
		l.add(n);

		if (auto c = dynamic_cast<NodeContainer*>(n.get()))
			l.addArray(c->getChildNodesRecursive());
	}

	return l;
}

void NodeContainer::fillAccessors(Array<CppGen::Accessor>& accessors, Array<int> currentPath)
{
	for (int i = 0; i < nodes.size(); i++)
	{
		Array<int> thisPath = currentPath;
		thisPath.add(i);

		if (auto c = dynamic_cast<NodeContainer*>(nodes[i].get()))
		{
			c->fillAccessors(accessors, thisPath);
		}
		else
		{
			accessors.add({ nodes[i]->getId(), thisPath });
		}
	}
}

struct ClassGenerator
{
	ClassGenerator(NodeContainer& container_) :
		container(container_)
	{
		container.fillAccessors(accessors, {});

		nodeList = container.getChildNodesRecursive();
	}

	NodeBase::List nodeList;
	NodeContainer& container;
	Array<CppGen::Accessor> accessors;

	struct ConnectionData
	{
		String id;
		bool isInverted = false;
		String opType;
		String exprString;
	};

	String createParameterInitialisation()
	{
		String pb;

		CppGen::Emitter::emitCommentLine(pb, 0, "Parameter Initalisation");

		for (auto n : nodeList)
		{
			for (int i = 0; i < n->getNumParameters(); i++)
			{
				auto pName = n->getParameter(i)->getId();
				auto pValue = n->getParameter(i)->getValue();

				pb << "setParameterDefault(\"" << n->getId() << "." << pName << "\", ";
				pb << CppGen::Emitter::createPrettyNumber(pValue, false) << ");\n";
			}
		}

		pb << "\n";

		return pb;
	}

	String createParameterDefinition()
	{
		String s;

		for (const auto& a : accessors)
			s << a.toString(CppGen::Accessor::Format::ParameterDefinition);

		s << "\n";

		return s;
	}

	String createPropertyInitialisation()
	{
		String pb;

		CppGen::Emitter::emitCommentLine(pb, 0, "Setting node properties");

		for (auto n : nodeList)
		{
			for (auto prop : n->getPropertyTree())
			{
				// Don't set the Code property of JIT nodes
				if (prop[PropertyIds::ID].toString() == PropertyIds::Code.toString())
					continue;

				pb << "setNodeProperty(\"" << n->getId() << "." << prop[PropertyIds::ID].toString();
				pb << "\", " << CppGen::Emitter::getVarAsCode(prop[PropertyIds::Value]);
				pb << ", " << ((bool)prop[PropertyIds::Public] ? "true" : "false") << ");\n";
			}
		}

		return pb;
	}

	String createParameter(NodeBase::Parameter* p, bool isInternalParameter)
	{
		String pCode;

		auto macro = dynamic_cast<NodeContainer::MacroParameter*>(p);

		pCode << "ParameterData p(\"";
		
		if (isInternalParameter)
			pCode << p->parent->getId() << ".";

		pCode << macro->getId() << "\", ";
		pCode << CppGen::Emitter::createRangeString(macro->inputRange) << ");\n";

		auto defaultValue = p->getValue();

		if(defaultValue != 0.0)
			pCode << "p.setDefaultValue(" << CppGen::Emitter::createPrettyNumber(p->getValue(), false) << ");\n";

		pCode << "\n";

		

		Array<ConnectionData> connections;

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

			ConnectionData cd;
			cd.id = conIdName;
			cd.isInverted = c[PropertyIds::Inverted];
			auto opType = c[PropertyIds::OpType].toString();

#if HISE_INCLUDE_SNEX
			cd.exprString = snex::JitExpression::convertToValidCpp(c[PropertyIds::Expression].toString());
#endif

			if (!cd.exprString.isEmpty())
				cd.isInverted = false;

			if (opType != OperatorIds::SetValue.toString())
			{
				cd.opType = opType;
			}
			else
			{
				for (auto n : nodeList)
				{
					for (int i = 0; i < n->getNumParameters(); i++)
					{
						auto tp = n->getParameter(i);

						if (tp->matchesConnection(c))
						{
							auto isCombined = tp->getConnectedMacroParameters().size() > 1;

							if (isCombined)
								cd.opType = c[PropertyIds::OpType].toString();
						}
					}
				}
			}

			pCode << "auto " << conIdName << " = ";

			if (cd.opType.isEmpty())
				pCode << "getParameter";
			else
				pCode << "getCombinedParameter";

			pCode << "(\"" << conId << "\", ";

			auto connectionRange = RangeHelpers::getDoubleRange(c);

			if (cd.exprString.isNotEmpty())
				connectionRange = {0.0, 1.0};

			pCode << CppGen::Emitter::createRangeString(connectionRange);

			if (!cd.opType.isEmpty())
				pCode << ", \"" << cd.opType << "\"";

			pCode << ");\n";

			auto converter = c[PropertyIds::Converter].toString();

			if (converter != ConverterIds::Identity.toString() && cd.exprString.isEmpty())
			{
				if (cd.opType.isEmpty())
					pCode << conIdName << ".addConversion(ConverterIds::" << converter << ");\n";
				else
					pCode << conIdName << "->addConversion(ConverterIds::" << converter << ", \"" << cd.opType << "\");\n";
			}

			connections.add(std::move(cd));
		}

		bool useExternalConversion = !RangeHelpers::isIdentity(macro->inputRange);

		pCode << "\n";

		CppGen::MethodInfo l;
		l.name = "[";

		for (auto& c : connections)
		{
			l.name << c.id; 

			if(connections.getLast().id != c.id)
				l.name << ", ";
		}

		if (useExternalConversion)
			l.name << ", outer = p.range";

		l.name << "]";
		l.returnType << "";
		l.arguments = { "double input" };

		String variableName = "input";

		if (useExternalConversion)
		{
			l.body << "input = outer.convertTo0to1(input);\n";
		}
		
		for (auto& c : connections)
		{
			String invPrefix = c.isInverted ? "1.0 - " : "";
			String opTypePrefix = c.opType.isEmpty() ? "" : "->" + c.opType;

			if (c.exprString.isNotEmpty())
			{
				if (c.id.startsWith("bypass_target"))
					jassertfalse;
				else
				{
					l.body << c.id << opTypePrefix << "(" << c.exprString << ");\n";
				}
			}
			else
			{
				if (c.id.startsWith("bypass_target"))
					l.body << c.id << ".setBypass(" << invPrefix << variableName << ");\n";
				else
					l.body << c.id << opTypePrefix << "(" << invPrefix << variableName << ");\n";
			}
		}

		l.addSemicolon = false;
		l.addNewLine = false;

		pCode << "p.setCallback(";
		CppGen::Emitter::emitFunctionDefinition(pCode, l);
		pCode << ");\n";

		if (isInternalParameter)
			pCode << "internalParameterList";
		else
			pCode << "data";
		
		pCode << ".add(std::move(p)); \n";

		return CppGen::Emitter::surroundWithBrackets(pCode);
	}

	String createModulation(NodeBase* n)
	{
		if (auto modSource = dynamic_cast<ModulationSourceNode*>(n))
		{
			if (modSource->getModulationTargetTree().getNumChildren() == 0)
				return {};

			String mCode;
			Array<ConnectionData> modTargets;

			int tIndex = 1;

			for (auto m : modSource->getModulationTargetTree())
			{
				String modTargetId;
				modTargetId << m[PropertyIds::NodeId].toString() << "." << m[PropertyIds::ParameterId].toString();

				auto modIdName = "mod_target" + String(tIndex++);

				ConnectionData cd;

				cd.id = modIdName;
				cd.isInverted = m[PropertyIds::Inverted];
				cd.opType = m[PropertyIds::OpType].toString();

#if HISE_INCLUDE_SNEX
				cd.exprString = snex::JitExpression::convertToValidCpp(m[PropertyIds::Expression].toString());
#endif

				auto modRange = RangeHelpers::getDoubleRange(m);

				if (cd.exprString.isNotEmpty())
				{
					cd.isInverted = false;
					modRange = { 0.0, 1.0 };
				}

				modTargets.add(cd);

				mCode << "auto " << modIdName << " = getParameter(\"" << modTargetId << "\", ";
				mCode << CppGen::Emitter::createRangeString(modRange) << ");\n";
			}

			CppGen::MethodInfo l;
			l.name = "[";

			for (int i = 0; i < modTargets.size(); i++)
			{
				l.name << modTargets[i].id;

				if (i != modTargets.size() - 1)
					l.name << ", ";
			}

			l.name << "]";
			l.returnType << "auto f = ";
			l.arguments = { "double input" };

			bool shouldScale = modSource->shouldScaleModulationValue();

			for (auto modTarget : modTargets)
			{
				auto id = modTarget.id;

				if (modTarget.exprString.isEmpty())
				{
					String invPrefix = modTarget.isInverted ? "1.0 - " : "";

					if (shouldScale)
						l.body << id << "(" << invPrefix << "input);\n";
					else
						l.body << id << ".callUnscaled(" << invPrefix << "input);\n";
				}
				else
				{
					l.body << id << ".callUnscaled(" << modTarget.exprString << ");\n";
				}
			}

			l.addSemicolon = true;

			CppGen::Emitter::emitFunctionDefinition(mCode, l);

			String modAccessor;

			for (const auto& a : accessors)
			{
				if (a.id == modSource->getId())
				{
					modAccessor = a.toString(CppGen::Accessor::Format::GetMethod);
					break;
				}
			}

			mCode << "\nsetInternalModulationParameter(" << modAccessor << ", f);\n";

			return CppGen::Emitter::surroundWithBrackets(mCode);
		}

		return {};
	}
};


int NodeContainer::getCachedIndex(const var &indexExpression) const
{
	return (int)indexExpression;
}

void NodeContainer::clear()
{
	getNodeTree().removeAllChildren(asNode()->getUndoManager());
}

juce::String NodeContainer::createCppClassForNodes(bool isOuterClass)
{
	if (isOuterClass)
	{
		String s;

		String jitClasses = getCppCode(CppGen::CodeLocation::InnerJitClasses);

		if (jitClasses.isNotEmpty())
		{
			CppGen::Emitter::emitCommentLine(s, 0, "Inner SNEX classes");
			s << jitClasses << "\n";
		}

		CppGen::Emitter::emitCommentLine(s, 0, "Template Alias Definition");
		s << getCppCode(CppGen::CodeLocation::TemplateAlias);
		s << "\n";

		String classContent = getCppCode(CppGen::CodeLocation::Definitions);

		classContent << "\n";
		
		CppGen::MethodInfo dataInfo;

		dataInfo.returnType = "String";
		dataInfo.name = "getSnippetText";
		dataInfo.arguments = {};
		dataInfo.specifiers = " const override";
		dataInfo.body << "return \"";
		dataInfo.body << ValueTreeConverters::convertValueTreeToBase64(asNode()->getValueTree(), true);
		dataInfo.body << "\";\n";

		CppGen::Emitter::emitFunctionDefinition(classContent, dataInfo);

		CppGen::MethodInfo parameterMethod;
		parameterMethod.name = "createParameters";


		parameterMethod.arguments = { "Array<ParameterData>& data" };
		parameterMethod.returnType = "void";

		auto& pb = parameterMethod.body;

		CppGen::Emitter::emitCommentLine(pb, 0, "Node Registration");

		ClassGenerator gen(*this);

		Array<CppGen::Accessor> acessors;

		
		pb << gen.createParameterDefinition();

		pb << gen.createParameterInitialisation();
		
		pb << gen.createPropertyInitialisation();

		String modString;

		for (auto n : getChildNodesRecursive())
			modString << gen.createModulation(n);

		if (modString.isNotEmpty())
		{
			pb << "\n";
			CppGen::Emitter::emitCommentLine(pb, 0, "Internal Modulation");
			pb << modString;
		}

		String parameterString;

		for (auto c : getChildNodesRecursive())
		{
			if (auto childContainer = dynamic_cast<NodeContainer*>(c.get()))
			{
				for (int i = 0; i < c->getNumParameters(); i++)
				{
					CppGen::Emitter::emitCommentLine(parameterString, 0, "Internal parameter definition");
					parameterString << gen.createParameter(c->getParameter(i), true);
				}
			}
		}

		auto n = asNode();

		for (int i = 0; i < n->getNumParameters(); i++)
			parameterString << gen.createParameter(n->getParameter(i), false);

		if (parameterString.isNotEmpty())
		{
			pb << "\n";
			CppGen::Emitter::emitCommentLine(pb, 0, "Parameter Callbacks");
			pb << parameterString;
		}

		CppGen::Emitter::emitFunctionDefinition(classContent, parameterMethod);

		s << CppGen::Emitter::createClass(classContent, createCppClassForNodes(false), shouldCreatePolyphonicClass());
		s = CppGen::Helpers::createIntendation(s);

		auto nid = n->getId();

		s << CppGen::Emitter::createFactoryMacro(shouldCreatePolyphonicClass(), false);

		

		auto impl = CppGen::Emitter::wrapIntoNamespace(s, nid + "_impl");

		if (shouldCreatePolyphonicClass())
		{
			String templateClassName = nid + "_impl::instance";
			String def;
			def << nid << ", " << nid << "_poly, " << templateClassName;
			CppGen::Emitter::emitDefinition(impl, "DEFINE_EXTERN_NODE_TEMPLATE", def, false);
			CppGen::Emitter::emitDefinition(impl, "DEFINE_EXTERN_NODE_TEMPIMPL", templateClassName, false);
		}
		else
		{
			impl << "\n" << CppGen::Emitter::createAlias(nid, nid + "_impl::instance");
		}

		return impl;
	}
	else
	{
		return asNode()->getId() + "_";
	}
}


juce::String NodeContainer::getCppCode(CppGen::CodeLocation location)
{
	
	if (location == CppGen::CodeLocation::InnerJitClasses)
	{
		return createJitClasses();
	}
	if (location == CppGen::CodeLocation::TemplateAlias)
	{
		return createTemplateAlias();
	}
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s;

		auto poly = shouldCreatePolyphonicClass();

		CppGen::Emitter::emitDefinition(s, poly ? "SET_HISE_POLY_NODE_ID" : "SET_HISE_NODE_ID", asNode()->getId(), true);
		CppGen::Emitter::emitDefinition(s, "GET_SELF_AS_OBJECT", "instance", false);
		
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
	nodeListener.setCallback(getNodeTree(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::nodeAddedOrRemoved));

	parameterListener.setCallback(asNode()->getParameterTree(),
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(NodeContainer::parameterAddedOrRemoved));

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




juce::Rectangle<int> SerialNode::getPositionInCanvas(Point<int> topLeft) const
{
	using namespace UIValues;

	const int minWidth = jmax(NodeWidth, 100 * getNumParameters() + 50);
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
		bounds = n->getBoundsToDisplay(bounds);
		maxW = jmax<int>(maxW, bounds.getWidth());
		h += bounds.getHeight() + NodeMargin;
		childPos = childPos.translated(0, bounds.getHeight());
	}

	h += PinHeight; // the "hole" for the cable

	return getBoundsToDisplay({ topLeft.getX(), topLeft.getY(), maxW + 2 * NodeMargin, h });
}



juce::String SerialNode::createCppClass(bool isOuterClass)
{
	return createCppClassForNodes(isOuterClass);
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



bool SerialNode::DynamicSerialProcessor::handleModulation(double&)
{

	return false;
}

void SerialNode::DynamicSerialProcessor::handleHiseEvent(HiseEvent& e)
{
	for (auto n : parent->getNodeList())
		n->handleHiseEvent(e);
}

void SerialNode::DynamicSerialProcessor::initialise(NodeBase* p)
{
	parent = dynamic_cast<NodeContainer*>(p);
}

void SerialNode::DynamicSerialProcessor::reset()
{
	for (auto n : parent->getNodeList())
		n->reset();
}

void SerialNode::DynamicSerialProcessor::prepare(PrepareSpecs)
{
	// do nothing here, the container inits the child nodes.
}

void SerialNode::DynamicSerialProcessor::process(ProcessData& d)
{
	jassert(parent != nullptr);

	for (auto n : parent->getNodeList())
		n->process(d);
}

void SerialNode::DynamicSerialProcessor::processSingle(float* frameData, int numChannels)
{
	jassert(parent != nullptr);

	for (auto n : parent->getNodeList())
		n->processSingle(frameData, numChannels);
}


ParallelNode::ParallelNode(DspNetwork* root, ValueTree data) :
	NodeBase(root, data, 0)
{

}

juce::String ParallelNode::getCppCode(CppGen::CodeLocation location)
{
	if (location == CppGen::CodeLocation::Definitions)
	{
		String s = NodeContainer::getCppCode(location);
		CppGen::Emitter::emitDefinition(s, "SET_HISE_NODE_IS_MODULATION_SOURCE", "false", false);

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
		b = n->getBoundsToDisplay(b);
		maxy = jmax(b.getBottom(), maxy);
		startPos = startPos.translated(b.getWidth() + UIValues::NodeMargin, 0);
		maxWidth = startPos.getX();

	}

	maxy += UIValues::PinHeight;
	maxy += UIValues::NodeMargin;

	return { topLeft.getX(), topLeft.getY(), maxWidth, maxy };
}

juce::String ParallelNode::createCppClass(bool isOuterClass)
{
	return createCppClassForNodes(isOuterClass);
}

NodeContainerFactory::NodeContainerFactory(DspNetwork* parent) :
	NodeFactory(parent)
{
	
	registerNodeRaw<ChainNode>({});
	registerNodeRaw<SplitNode>({});
	registerNodeRaw<MultiChannelNode>({});
	registerNodeRaw<ModulationChainNode>({});
	registerNodeRaw<MidiChainNode>({});
	registerNodeRaw<SingleSampleBlock<1>>({});
	registerNodeRaw<SingleSampleBlock<2>>({});
	registerNodeRaw<SingleSampleBlockX>({});
	registerNodeRaw<OversampleNode<2>>({});
	registerNodeRaw<OversampleNode<4>>({});
	registerNodeRaw<OversampleNode<8>>({});
	registerNodeRaw<OversampleNode<16>>({});
	registerNodeRaw<FixedBlockNode<8>>({});
	registerNodeRaw<FixedBlockNode<16>>({});
	registerNodeRaw<FixedBlockNode<32>>({});
	registerNodeRaw<FixedBlockNode<64>>({});
	registerNodeRaw<FixedBlockNode<128>>({});
	registerNodeRaw<FixedBlockNode<256>>({});
}


NodeContainer::MacroParameter::Connection::Connection(NodeBase* parent, MacroParameter* pp, ValueTree d):
	ConnectionBase(parent->getScriptProcessor(), d),
	um(parent->getUndoManager()),
	parentParameter(pp)
{
	initRemoveUpdater(parent);

	auto nodeId = d[PropertyIds::NodeId].toString();

	if (auto targetNode = dynamic_cast<NodeBase*>(parent->getRootNetwork()->get(nodeId).getObject()))
	{
		auto undoManager = um;
		
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

					targetParameter = targetNode->getParameter(i);

					opSyncer.setCallback(data, { PropertyIds::OpType, }, valuetree::AsyncMode::Synchronously,
						BIND_MEMBER_FUNCTION_2(Connection::updateConnectionInTargetParameter));

					break;
				}
			}
		}
	}
	else
	{
		targetParameter = nullptr;
		return;
	}

	auto converterId = d[PropertyIds::Converter].toString();

	if (converterId.isNotEmpty())
		conversion = Identifier(converterId);

	connectionRange = RangeHelpers::getDoubleRange(d);
	inverted = d[PropertyIds::Inverted];
}


DspHelpers::ParameterCallback NodeContainer::MacroParameter::Connection::createCallbackForNormalisedInput()
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
		if (opType == OpType::Add)
			f = std::bind(&NodeBase::Parameter::addModulationValue, targetParameter.get(), std::placeholders::_1);
		else if (opType == OpType::Multiply)
			f = std::bind(&NodeBase::Parameter::multiplyModulationValue, targetParameter.get(), std::placeholders::_1);
		else
			f = std::bind(&NodeBase::Parameter::setValueAndStoreAsync, targetParameter.get(), std::placeholders::_1);

		expressionCode = data[PropertyIds::Expression].toString();

		if (expressionCode.isNotEmpty())
		{
#if HISE_INCLUDE_SNEX
			snex::JitExpression::Ptr e = new snex::JitExpression(expressionCode, parentParameter);

			if (e->isValid())
			{
				auto r = connectionRange;

				auto fWithExpression = [e, f](double input)
				{
					input = e->getValue(input);
					f(input);
				};

				return fWithExpression;
			}
			else
				return f;
#else
            jassertfalse;
            return {};
#endif
            
		}
		else
			return DspHelpers::wrapIntoConversionLambda(conversion, f, connectionRange, inverted);
	}


}


void NodeContainer::MacroParameter::Connection::updateConnectionInTargetParameter(Identifier id, var newValue)
{
	if (id == PropertyIds::OpType)
	{
		targetParameter->clearModulationValues();
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

	expressionListener.setCallback(getConnectionTree(),
		{ PropertyIds::Expression },
		valuetree::AsyncMode::Synchronously,
		BIND_MEMBER_FUNCTION_2(MacroParameter::updateRangeForConnection));

	for (auto c : getConnectionTree())
	{
		auto targetId = c[PropertyIds::NodeId].toString();
	}

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
					macroTarget->getValueTree().removeProperty(PropertyIds::DynamicBypass, parent->getUndoManager());
			}
		}

		if (initialised)
		rebuildCallback();
	});

	
	rebuildCallback();
	initialised = true;


}

void NodeContainer::MacroParameter::rebuildCallback()
{
	inputRange = RangeHelpers::getDoubleRange(data);

	connections.clear();
	auto cTree = data.getChildWithName(PropertyIds::Connections);

	for (auto c : cTree)
	{
		ScopedPointer<Connection> newC = new Connection(parent, this, c);

		if (newC->isValid())
			connections.add(newC.release());
		else
		{
			cTree.removeChild(c, nullptr);
			break;
		}
	}

	if (connections.size() > 0)
	{
		Array<DspHelpers::ParameterCallback> connectionCallbacks;

		for (auto c : connections)
			connectionCallbacks.add(c->createCallbackForNormalisedInput());

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

juce::Identifier NodeContainer::MacroParameter::getOpTypeForParameter(Parameter* target) const
{
	for (auto c : connections)
	{
		if (c->matchesTarget(target))
		{
			return c->getOpType();
		}
	}

	return Identifier();
}

bool NodeContainer::MacroParameter::matchesTarget(const Parameter* target) const
{
	for (auto c : connections)
		if (c->matchesTarget(target))
			return true;

	return false;
}


}
