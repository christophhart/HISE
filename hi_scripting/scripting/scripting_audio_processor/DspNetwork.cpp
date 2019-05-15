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

namespace hise
{
namespace scriptnode
{
using namespace juce;


NodeBase::NodeBase(DspNetwork* rootNetwork, ValueTree data_, int numConstants_) :
	ConstScriptingObject(rootNetwork->getScriptProcessor(), numConstants_),
	parent(rootNetwork),
	data(data_)
{
	setDefaultValue(PropertyIds::NumChannels, 2);
}

hise::scriptnode::DspNetwork* NodeBase::getRootNetwork() const
{
	return dynamic_cast<DspNetwork*>(parent.get());
}

void NodeBase::setDefaultValue(const Identifier& id, var newValue)
{
	if (!data.hasProperty(id))
		data.setProperty(id, newValue, nullptr);
}

NodeComponent* NodeBase::createComponent()
{
	return new NodeComponent(this);
}

juce::UndoManager* NodeBase::getUndoManager()
{
	return dynamic_cast< Processor*>(getScriptProcessor())->getMainController()->getControlUndoManager();
}

juce::Rectangle<int> NodeBase::reduceHeightIfFolded(Rectangle<int> originalHeight) const
{
	if (data[PropertyIds::Folded])
		return originalHeight.withHeight(HeaderHeight);
	else
		return originalHeight;
}

NodeContainer::NodeContainer(DspNetwork* parent, ValueTree data_) :
	NodeBase(parent, data_, 0)
{
	data.addListener(this);

	for (auto child : data)
		valueTreeChildAdded(data, child);
}

NodeContainer::~NodeContainer()
{
	data.removeListener(this);
}

void NodeContainer::assign(const int index, var newValue)
{
	ScopedLock sl(getRootNetwork()->getConnectionLock());

	if (auto node = dynamic_cast<NodeBase*>(newValue.getObject()))
	{
		auto tree = node->getValueTree();

		tree.setProperty(PropertyIds::NumChannels, getNumChannelsToProcess(), nullptr);

		getUndoManager()->beginNewTransaction();
		tree.getParent().removeChild(tree, getUndoManager());
		data.addChild(tree, index, getUndoManager());
	}
	else
	{
		data.removeChild(index, getUndoManager());
	}
}

void NodeContainer::valueTreeChildAdded(ValueTree& parentTree, ValueTree& child)
{
	if (parentTree != data)
		return;

	if (auto nodeToAdd = getRootNetwork()->getNodeForValueTree(child))
	{
		if (nodes.contains(nodeToAdd))
			return;

		int insertIndex = parentTree.indexOf(child);

		ScopedLock sl(getRootNetwork()->getConnectionLock());
		nodes.insert(insertIndex, nodeToAdd);
	}
}

void NodeContainer::valueTreeChildRemoved(ValueTree& parentTree, ValueTree& child, int)
{
	if (parentTree != data)
		return;

	if (auto nodeToRemove = getRootNetwork()->getNodeForValueTree(child))
	{
		ScopedLock sl(getRootNetwork()->getConnectionLock());
		nodes.removeAllInstancesOf(nodeToRemove);
	}
}

SerialNode::SerialNode(DspNetwork* root, ValueTree data) :
	NodeContainer(root, data)
{
	
}

hise::scriptnode::NodeComponent* SerialNode::createComponent()
{
	return new SerialNodeComponent(this);
}

void SerialNode::process(ProcessData& data)
{
	for (auto n : nodes)
		n->process(data);
}

juce::Rectangle<int> SerialNode::getPositionInCanvas(Point<int> topLeft) const
{
	const int minWidth = NodeWidth;
	const int topRow = NodeHeight;

	int maxW = minWidth;
	int h = 0;

	h += NodeBase::NodeMargin;
	h += NodeBase::HeaderHeight; // the input
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

hise::scriptnode::NodeComponent* ParallelNode::createComponent()
{
	return new ParallelNodeComponent(this);
}

juce::Rectangle<int> ParallelNode::getPositionInCanvas(Point<int> topLeft) const
{
	Point<int> startPos(NodeBase::NodeMargin, NodeBase::NodeMargin + NodeBase::HeaderHeight + SerialNode::PinHeight);

	int maxy = startPos.getY();
	int maxWidth = NodeWidth + NodeMargin;

	for (auto n : nodes)
	{
		auto b = n->getPositionInCanvas(startPos);
		b = n->reduceHeightIfFolded(b);
		maxy = jmax(b.getBottom(), maxy);
		startPos = startPos.translated(b.getWidth() + NodeBase::NodeMargin, 0);
		maxWidth = startPos.getX();

	}

	maxy += SerialNode::PinHeight;
	maxy += NodeBase::NodeMargin;

	return { topLeft.getX(), topLeft.getY(), maxWidth, maxy };
}

DspNode::DspNode(DspNetwork* root, DspFactory* f_, ValueTree data) :
	NodeBase(root, data, NUM_API_FUNCTION_SLOTS),
	f(f_),
	parameterUpdater(*this),
	moduleName(data[PropertyIds::FactoryPath].toString().fromFirstOccurrenceOf(".", false, false))
{
	initialise();
}


hise::scriptnode::NodeComponent* DspNode::createComponent()
{
	return new DspNodeComponent(this);
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

				Identifier pId(String(buf, len));

				var value = obj->getParameter(i);

				parameterUpdater.registerIdWithCallback(i, pId, value, f);
			}
		}
		else
		{
			reportScriptError("The module " + moduleName + " wasn't found in the Library.");
		}
	}
}

struct DspNetwork::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(DspNetwork, processBlock);
	API_VOID_METHOD_WRAPPER_2(DspNetwork, prepareToPlay);
	API_METHOD_WRAPPER_2(DspNetwork, create);
	API_METHOD_WRAPPER_1(DspNetwork, get);
	//API_VOID_METHOD_WRAPPER_0(DspNetwork, disconnectAll);
	//API_VOID_METHOD_WRAPPER_3(DspNetwork, injectAfter);
};

DspNetwork::DspNetwork(hise::ProcessorWithScriptingContent* p, ValueTree data_) :
	ConstScriptingObject(p, 2),
	data(data_),
	signalPath(createFromValueTree(data.getChild(0)))
{
	loader = new DspFactory::LibraryLoader(dynamic_cast<Processor*>(p));

	ADD_API_METHOD_1(processBlock);
	ADD_API_METHOD_2(prepareToPlay);
	ADD_API_METHOD_2(create);
	ADD_API_METHOD_1(get);
	//ADD_API_METHOD_0(disconnectAll);
	//ADD_API_METHOD_3(injectAfter);
}

DspNetwork::~DspNetwork()
{
	nodes.clear();
	
}

void DspNetwork::rightClickCallback(const MouseEvent& e, Component* c)
{

#if USE_BACKEND

	auto* d = new DspNetworkGraph(this);

	d->setSize(600, 600);

	auto editor = GET_BACKEND_ROOT_WINDOW(c);

	MouseEvent ee = e.getEventRelativeTo(editor);

	editor->getRootFloatingTile()->showComponentInRootPopup(d, editor, ee.getMouseDownPosition());
#else
	ignoreUnused(e, c);
#endif
}

hise::scriptnode::NodeBase* DspNetwork::getNodeForValueTree(const ValueTree& v)
{
	for (auto n : nodes)
	{
		if (n->getValueTree() == v)
			return n;
	}

	return nullptr;
}

hise::scriptnode::NodeBase::List DspNetwork::getListOfUnconnectedNodes() const
{
	NodeBase::List unconnectedNodes;
	unconnectedNodes.ensureStorageAllocated(nodes.size());

	for (auto n : nodes)
	{
		if (!n->isConnected())
			unconnectedNodes.add(n);
	}

	return unconnectedNodes;
}

juce::StringArray DspNetwork::getListOfAllAvailableModuleIds() const
{
	auto l = dynamic_cast<DspFactory::LibraryLoader*>(loader.get());

	return l->getListOfAllAvailableModules();
}


void DspNetwork::prepareToPlay(double sampleRate, double blockSize)
{
	ScopedLock sl(getConnectionLock());

	signalPath->prepare(sampleRate, blockSize);
}

void DspNetwork::processBlock(var data)
{
	ScopedLock sl(getConnectionLock());

	if (auto ar = data.getArray())
	{
		ProcessData d;
		d.numChannels = ar->size();

		int index = 0;

		for (const auto& v : *ar)
		{
			if (auto bf = v.getBuffer())
			{
				int thisSamples = bf->buffer.getNumSamples();

				if (d.size == 0)
					d.size = thisSamples;
				else if (d.size != thisSamples)
					reportScriptError("Buffer mismatch");

				currentData[index++] = bf->buffer.getWritePointer(0);
			}
		}

		d.data = currentData;

		signalPath->process(d);
	}
}

var DspNetwork::create(String path, String id)
{
	var existing = get(id);

	if (existing.isObject())
		return existing;
	
	ValueTree newNodeData(PropertyIds::Node);

	if (id.isEmpty())
	{
		int trailingIndex = 1;

		String nameToUse = path.fromFirstOccurrenceOf(".", false, false);

		String newId = nameToUse + String(trailingIndex++);

		var existingNode = get(newId);

		while (existingNode.isObject())
		{
			newId = nameToUse + String(trailingIndex++);
			existingNode = get(newId);
		}

		id = newId;
	}

	newNodeData.setProperty(PropertyIds::ID, id, nullptr);

	if      (path == "chain")
		newNodeData.setProperty(PropertyIds::Type, PropertyIds::SerialNode.toString(), nullptr);
	else if (path == "split")
		newNodeData.setProperty(PropertyIds::Type, PropertyIds::SplitNode.toString(), nullptr);
	else if (path == "multi")
		newNodeData.setProperty(PropertyIds::Type, PropertyIds::MultiChannelNode.toString(), nullptr);
	else if (path.startsWith("hise"))
	{
		auto type = path.fromFirstOccurrenceOf(".", false, false);

		newNodeData.setProperty(PropertyIds::Type, PropertyIds::HiseFXNode.toString(), nullptr);
		newNodeData.setProperty(PropertyIds::FactoryPath, type, nullptr);
	}
	else
	{
		newNodeData.setProperty(PropertyIds::Type, PropertyIds::DspNode.toString(), nullptr);
		newNodeData.setProperty(PropertyIds::FactoryPath, path, nullptr);
	}

	return createFromValueTree(newNodeData);
}

var DspNetwork::get(String id)
{
	if (id.isEmpty())
		return {};

	if (id == getId())
		return var(signalPath);

	for (auto n : nodes)
	{
		if (n->getId() == id)
			return var(n);
	}
	
	return {};
}

hise::scriptnode::NodeBase* DspNetwork::createFromValueTree(ValueTree d)
{
	auto type = Identifier(d[PropertyIds::Type].toString());

	NodeBase::Ptr newNode;

	if (type == PropertyIds::DspNode)
	{
		auto path = d[PropertyIds::FactoryPath].toString();
		auto factoryId = path.upToFirstOccurrenceOf(".", false, false);
		auto loader_ = dynamic_cast<hise::DspFactory::LibraryLoader*>(loader.get());
		auto factory = dynamic_cast<DspFactory*>(loader_->load(factoryId, "").getObject());

		auto moduleName = path.fromFirstOccurrenceOf(".", false, false);
		newNode = new DspNode(this, factory, d);
	}
	else if (type == PropertyIds::HiseFXNode)
	{
		//newNode = new HiseFXNode(this, d);
	}
	else if (type == PropertyIds::SerialNode)
		newNode = new SerialNode(this, d);
	else if (type == PropertyIds::MultiChannelNode)
		newNode = new MultiChannelNode(this, d);
	else if (type == PropertyIds::SplitNode)
		newNode = new SplitNode(this, d);

	nodes.add(newNode);
	return newNode.get();
}


hise::scriptnode::DspNetwork* DspNetwork::Holder::getOrCreate(const String& id)
{
	auto asScriptProcessor = dynamic_cast<ProcessorWithScriptingContent*>(this);

	for (auto n : networks)
	{
		if (n->getId() == id)
			return n;
	}

	ValueTree v(scriptnode::PropertyIds::Network);
	v.setProperty(scriptnode::PropertyIds::ID, id, nullptr);

	ValueTree s(scriptnode::PropertyIds::Node);
	s.setProperty(scriptnode::PropertyIds::Type, scriptnode::PropertyIds::SerialNode.toString(), nullptr);
	s.setProperty(scriptnode::PropertyIds::ID, id, nullptr);

	v.addChild(s, -1, nullptr);

	auto newNetwork = new DspNetwork(asScriptProcessor, v);

	networks.add(newNetwork);

	return newNetwork;
}


void DspNetwork::Holder::saveNetworks(ValueTree& d) const
{
	if (networks.size() > 0)
	{
		ValueTree v("Networks");

		for (auto n : networks)
		{
			v.addChild(n->getValueTree().createCopy(), -1, nullptr);
		}

		d.addChild(v, -1, nullptr);
	}
}

void DspNetwork::Holder::restoreNetworks(const ValueTree& d)
{
	auto v = d.getChildWithName("Networks");

	if (v.isValid())
	{
		networks.clear();

		for (auto c : v)
		{
			auto newNetwork = new DspNetwork(dynamic_cast<ProcessorWithScriptingContent*>(this),
				c.createCopy());

			networks.add(newNetwork);
		}
	}
}

NodeBase::AsyncPropertyListener::AsyncPropertyListener(NodeBase& parent_) :
	parent(parent_)
{
	setHandler(parent.getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
	addChangeListener(this);
}

}
}

