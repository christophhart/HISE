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

ModulationSourceNode::ModulationTarget::ModulationTarget(ModulationSourceNode* parent_, ValueTree data_) :
	ConnectionBase(parent_->getScriptProcessor(), data_),
	parent(parent_),
	active(data, PropertyIds::Enabled, parent->getUndoManager(), true)
{
	rangeUpdater.setCallback(data, RangeHelpers::getRangeIds(),
		valuetree::AsyncMode::Coallescated,
		[this](Identifier, var)
	{
		inverted = RangeHelpers::checkInversion(data, &rangeUpdater, parent->getUndoManager());
		parent->rebuildModulationConnections();
		
		connectionRange = RangeHelpers::getDoubleRange(data);
	});

	
	initRemoveUpdater(parent);

	expressionUpdater.setCallback(data, { PropertyIds::Expression },
		valuetree::AsyncMode::Synchronously,
		[this](Identifier, var newValue)
	{
		parent->rebuildModulationConnections();
	});
}



ModulationSourceNode::ModulationTarget::~ModulationTarget()
{
	
}

bool ModulationSourceNode::ModulationTarget::findTarget()
{
	String nodeId = data[PropertyIds::NodeId];
	String parameterId = data[PropertyIds::ParameterId];

	auto list = parent->getRootNetwork()->getListOfNodesWithType<NodeBase>(true);

	for (auto n : list)
	{
		if (n->getId() == nodeId)
		{
			auto enabled = n->getRootNetwork()->isInSignalPath(n);

			if (!enabled && !n->isBeingMoved())
				data.setProperty(PropertyIds::Enabled, false, parent->getUndoManager());

			for (int i = 0; i < n->getNumParameters(); i++)
			{
				if (n->getParameter(i)->getId() == parameterId)
				{
					targetParameter = n->getParameter(i);
					return true;
				}
			}
		}
	}

	return false;
}

juce::ValueTree ModulationSourceNode::getModulationTargetTree()
{
	auto vt = getValueTree().getChildWithName(PropertyIds::ModulationTargets);

	if (!vt.isValid())
	{
		vt = ValueTree(PropertyIds::ModulationTargets);
		getValueTree().addChild(vt, -1, getUndoManager());
	}

	return vt;
}


ModulationSourceNode::ModulationSourceNode(DspNetwork* n, ValueTree d) :
	WrapperNode(n, d),
	SimpleTimer(n->getScriptProcessor()->getMainController_()->getGlobalUIUpdater())
{
	ringBuffer = new SimpleRingBuffer();

	targetListener.setCallback(getModulationTargetTree(),
		valuetree::AsyncMode::Synchronously,
		[this](ValueTree c, bool wasAdded)
	{
		rebuildModulationConnections();

		if (wasAdded)
		{
			ValueTree newTree(c);
			targets.add(new ModulationTarget(this, newTree));
		}
		else
		{
			for (auto t : targets)
			{
				if (t->data == c)
				{
					if (t->targetParameter != nullptr)
						t->targetParameter->data.removeProperty(PropertyIds::ModulationTarget, getUndoManager());

					targets.removeObject(t);
					break;
				}
			}
		}

		checkTargets();
	});
}

var ModulationSourceNode::addModulationTarget(NodeBase::Parameter* n)
{
	for (auto t : targets)
	{
		if (t->targetParameter.get() == n)
			return var(t);
	}

	ValueTree m(PropertyIds::ModulationTarget);

	m.setProperty(PropertyIds::NodeId, n->parent->getId(), nullptr);
	m.setProperty(PropertyIds::ParameterId, n->getId(), nullptr);
	m.setProperty(PropertyIds::OpType, OperatorIds::SetValue.toString(), nullptr);
	m.setProperty(PropertyIds::Enabled, true, nullptr);

	n->getTreeWithValue().setProperty(PropertyIds::ModulationTarget, true, getUndoManager());

	auto range = RangeHelpers::getDoubleRange(n->data);

	RangeHelpers::storeDoubleRange(m, false, range, nullptr);

	m.setProperty(PropertyIds::Expression, "", nullptr);

	getModulationTargetTree().addChild(m, -1, getUndoManager());

	return var(targets.getLast());
}

juce::String ModulationSourceNode::createCppClass(bool isOuterClass)
{
	auto s = NodeBase::createCppClass(isOuterClass);

	if (getModulationTargetTree().getNumChildren() > 0)
		return CppGen::Emitter::wrapIntoTemplate(s, "wrap::mod");
	else
		return s;
}

scriptnode::NodeBase* ModulationSourceNode::getTargetNode(const ValueTree& m) const
{
	jassert(m.getType() == PropertyIds::ModulationTarget);
	return getRootNetwork()->getNodeWithId(m[PropertyIds::NodeId].toString());
}

parameter::data ModulationSourceNode::getParameterData(const ValueTree& m) const
{
	if (auto targetNode = getTargetNode(m))
	{
#if HISE_INCLUDE_SNEX
		if (auto sn = dynamic_cast<SnexSource::SnexParameter*>(targetNode->getParameter(m[PropertyIds::ParameterId].toString())))
		{
			parameter::data obj;
			obj.info = parameter::pod(sn->data);
			obj.callback.referTo(sn->p.getObjectPtr(), sn->p.getFunction());
			return obj;
		}
#endif

		auto pList = targetNode->createInternalParameterList();

		for (auto& p : pList)
		{
			if (p.info.getId() == m[PropertyIds::ParameterId].toString())
				return p;
		}

		
	}
	
	return parameter::data("");
}

scriptnode::parameter::dynamic_base* ModulationSourceNode::createDynamicParameterData(ValueTree& m)
{
	auto allowRangeConversion = isUsingNormalisedRange();

	auto range = RangeHelpers::getDoubleRange(m);

	auto targetNode = getTargetNode(m);
	auto expression = m[PropertyIds::Expression].toString();

	if (auto p = getParameterData(m))
	{
		ScopedPointer<parameter::dynamic_base> np;

		if (expression.isNotEmpty())
		{
#if HISE_INCLUDE_SNEX
			np = new parameter::dynamic_expression(p.callback, new JitExpression(expression, this));
#else
			// Set the default...
			np = new parameter::dynamic_base(p.callback);
#endif
		}
		else if (allowRangeConversion & !RangeHelpers::isIdentity(range))
			np = new parameter::dynamic_from0to1(p.callback, range);
		else
			np = new parameter::dynamic_base(p.callback);

		double* lValue = &targetNode->getParameter(p.info.getId())->getReferenceToCallback().lastValue;

		np->displayValuePointer = lValue;


		//np->setDataTree(targetNode->getParameter(p.id)->data);

		return np.release();
	}

	return nullptr;
}

void ModulationSourceNode::prepare(PrepareSpecs ps)
{
	NodeBase::prepare(ps);

	rebuildModulationConnections();

	if (ps.sampleRate > 0.0)
		sampleRateFactor = 32768.0 / ps.sampleRate;

	prepareWasCalled = true;

	checkTargets();
}

void ModulationSourceNode::logMessage(int level, const String& s)
{
#if USE_BACKEND
	auto p = dynamic_cast<Processor*>(getScriptProcessor());

	debugToConsole(p, s);
#else
    DBG(s);
    ignoreUnused(s);
#endif
}

void ModulationSourceNode::rebuildModulationConnections()
{
	auto modTree = getModulationTargetTree();

	auto p = getParameterHolder();

	if (p == nullptr)
		return;

	if (modTree.getNumChildren() == 0)
	{
		p->setParameter(nullptr);
		stop();
		return;
	}

	start();

	ScopedPointer<parameter::dynamic_chain> chain = new parameter::dynamic_chain();

	for (auto m : modTree)
	{
		if (auto c = createDynamicParameterData(m))
			chain->addParameter(c);
		else
		{
			p->setParameter(nullptr);
			stop();
			return;
		}
	}

	if (auto s = chain->getFirstIfSingle())

		p->setParameter(s);
	else
		p->setParameter(chain.release());
}

int ModulationSourceNode::fillAnalysisBuffer(AudioSampleBuffer& b)
{
	if (ringBuffer != nullptr)
		return ringBuffer->read(b);

	return 0;
}

void ModulationSourceNode::checkTargets()
{
	// We need to skip this if the node wasn't initialised properly
	if (!prepareWasCalled)
		return;

	for (int i = 0; i < targets.size(); i++)
	{
		if (!targets[i]->findTarget())
			targets.remove(i--);
	}
}

ModulationSourceBaseComponent::ModulationSourceBaseComponent(PooledUIUpdater* updater) :
	SimpleTimer(updater)
{

}

void ModulationSourceBaseComponent::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.1f));
	g.drawRect(getLocalBounds(), 1);
	g.setColour(Colours::white.withAlpha(0.1f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Drag to modulation target", getLocalBounds().toFloat(), Justification::centred);
}

juce::Image ModulationSourceBaseComponent::createDragImage()
{
	Image img(Image::ARGB, 100, 60, true);
	Graphics g(img);

	g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.4f));
	g.fillAll();
	g.setColour(Colours::black);
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText(getSourceNodeFromParent()->getId(), 0, 0, 128, 48, Justification::centred);

	return img;
}

void ModulationSourceBaseComponent::mouseDrag(const MouseEvent&)
{
	if (getSourceNodeFromParent() == nullptr)
		return;

	if (auto container = dynamic_cast<DragAndDropContainer*>(findParentComponentOfClass<DspNetworkGraph>()->root.get()))
	{
		// We need to be able to drag it anywhere...
		//while (auto pc = DragAndDropContainer::findParentDragContainerFor(dynamic_cast<Component*>(container)))
//			container = pc;

		var d;

		DynamicObject::Ptr details = new DynamicObject();

		details->setProperty(PropertyIds::ID, sourceNode->getId());
		details->setProperty(PropertyIds::ModulationTarget, true);

		container->startDragging(var(details), this, createDragImage());
	}
}


void ModulationSourceBaseComponent::mouseDown(const MouseEvent& e)
{
	if (getSourceNodeFromParent() == nullptr)
		return;

	if (e.mods.isRightButtonDown())
	{
		auto pe = new MacroPropertyEditor(sourceNode, sourceNode->getValueTree(), PropertyIds::ModulationTargets);

		pe->setName("Edit Modulation Targets");
        
        
        auto g = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();
        auto b = g->getLocalArea(this, getLocalBounds());
        
		g->setCurrentModalWindow(pe, b);

	}
}

scriptnode::ModulationSourceNode* ModulationSourceBaseComponent::getSourceNodeFromParent() const
{
	if (sourceNode == nullptr)
	{
		if (auto pc = findParentComponentOfClass<NodeComponent>())
		{
			sourceNode = dynamic_cast<ModulationSourceNode*>(pc->node.get());
			jassert(sourceNode != nullptr);
		}
	}

	return sourceNode;
}


ModulationSourcePlotter::ModulationSourcePlotter(PooledUIUpdater* updater) :
	ModulationSourceBaseComponent(updater)
{
	setOpaque(true);
	setSize(256, ModulationSourceNode::ModulationBarHeight);

	buffer.setSize(1, SimpleRingBuffer::RingBufferSize);
}

void ModulationSourcePlotter::timerCallback()
{
	skip = !skip;

	if (skip)
		return;

	if (getSourceNodeFromParent() != nullptr)
	{
		jassert(getSamplesPerPixel() > 0);

		auto numNew = sourceNode->fillAnalysisBuffer(buffer);

		pixelCounter += (float)numNew / (float)getSamplesPerPixel();

		

		rebuildPath();

		if (pixelCounter > 4.0f)
		{
			pixelCounter = fmod(pixelCounter, 4.0f);
			repaint();
		}	
	}
}

void ModulationSourcePlotter::rebuildPath()
{
	float offset = 2.0f;

	float rectangleWidth = 1.0f;

	auto width = (float)getWidth() - 2.0f * offset;
	auto maxHeight = (float)getHeight() - 2.0f * offset;

	int samplesPerPixel = getSamplesPerPixel();

	rectangles.clear();

	int sampleIndex = 0;

	for (float i = 0.0f; i < width; i += rectangleWidth)
	{
		float maxValue = jlimit(0.0f, 1.0f, buffer.getMagnitude(0, sampleIndex, samplesPerPixel));
		FloatSanitizers::sanitizeFloatNumber(maxValue);
		float height = maxValue * maxHeight;
		float y = offset + maxHeight - height;

		sampleIndex += samplesPerPixel;

		rectangles.add({ i + offset, y, rectangleWidth, height });
	}
}

void ModulationSourcePlotter::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF333333));

	g.setColour(Colour(0xFF999999));
	g.fillRectList(rectangles);

	ModulationSourceBaseComponent::paint(g);
}

int ModulationSourcePlotter::getSamplesPerPixel() const
{
	float offset = 2.0f;

	float rectangleWidth = 1.0f;

	auto width = (float)getWidth() - 2.0f * offset;

	int samplesPerPixel = SimpleRingBuffer::RingBufferSize / jmax((int)(width / rectangleWidth), 1);
	return samplesPerPixel;
}

SimpleRingBuffer::SimpleRingBuffer()
{
	clear();
}

void SimpleRingBuffer::clear()
{
	memset(buffer, 0, sizeof(float) * RingBufferSize);
}

int SimpleRingBuffer::read(AudioSampleBuffer& b)
{
	while (isBeingWritten) // busy wait, but OK
	{
		return 0;
	}

	jassert(b.getNumSamples() == RingBufferSize && b.getNumChannels() == 1);

	auto dst = b.getWritePointer(0);
	int thisWriteIndex = writeIndex;
	int numBeforeIndex = thisWriteIndex;
	int offsetBeforeIndex = RingBufferSize - numBeforeIndex;

	FloatVectorOperations::copy(dst + offsetBeforeIndex, buffer, numBeforeIndex);
	FloatVectorOperations::copy(dst, buffer + thisWriteIndex, offsetBeforeIndex);

	int numToReturn = numAvailable;
	numAvailable = 0;
	return numToReturn;
}

void SimpleRingBuffer::write(double value, int numSamples)
{
	ScopedValueSetter<bool> svs(isBeingWritten, true);

	if (numSamples == 1)
	{
		buffer[writeIndex++] = (float)value;

		if (writeIndex >= RingBufferSize)
			writeIndex = 0;

		numAvailable++;
		return;
	}
	else
	{
		int numBeforeWrap = jmin(numSamples, RingBufferSize - writeIndex);
		int numAfterWrap = numSamples - numBeforeWrap;

		if (numBeforeWrap > 0)
			FloatVectorOperations::fill(buffer + writeIndex, (float)value, numBeforeWrap);

		writeIndex += numBeforeWrap;

		if (numAfterWrap > 0)
		{
			FloatVectorOperations::fill(buffer, (float)value, numAfterWrap);
			writeIndex = numAfterWrap;
		}

		numAvailable += numSamples;
	}
}

juce::Component* WrapperNode::createExtraComponent()
{
	if (extraComponentFunction)
	{
		auto obj = static_cast<uint8*>(getObjectPtr());

		obj += uiOffset;

		PooledUIUpdater* updater = getScriptProcessor()->getMainController_()->getGlobalUIUpdater();
		return extraComponentFunction((void*)obj, updater);
	}

	return nullptr;
}

juce::Rectangle<int> WrapperNode::getExtraComponentBounds() const
{
	if (cachedExtraHeight == -1)
	{
		ScopedPointer<Component> c = const_cast<WrapperNode*>(this)->createExtraComponent();

		if (c != nullptr)
		{
			cachedExtraWidth = c->getWidth();
			cachedExtraHeight = c->getHeight();
		}
		else
		{
			cachedExtraWidth = 0;
			cachedExtraHeight = 0;
		}

	}

	return { 0, 0, cachedExtraWidth, cachedExtraHeight };
}

void WrapperNode::initParameterData(ParameterDataList& pData)
{
	auto d = getValueTree();

	d.getOrCreateChildWithName(PropertyIds::Parameters, getUndoManager());

	for (auto p : pData)
	{
		auto existingChild = getParameterTree().getChildWithProperty(PropertyIds::ID, p.info.getId());

		if (!existingChild.isValid())
		{
			existingChild = p.createValueTree();
			getParameterTree().addChild(existingChild, -1, getUndoManager());
		}

		auto newP = new Parameter(this, existingChild);

		auto ndb = new parameter::dynamic_base(p.callback);

		newP->setCallbackNew(ndb);
		newP->valueNames = p.parameterNames;

		addParameter(newP);
	}
}

}

