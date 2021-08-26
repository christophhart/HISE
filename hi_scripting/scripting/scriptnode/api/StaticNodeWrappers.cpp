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

NodeComponent* ComponentHelpers::createDefaultComponent(NodeBase* n)
{
    return  new DefaultParameterNodeComponent(n);
}
 
void ComponentHelpers::addExtraComponentToDefault(NodeComponent* nc, Component* c)
{
    dynamic_cast<DefaultParameterNodeComponent*>(nc)->setExtraComponent(c);
}

WrapperNode::WrapperNode(DspNetwork* parent, ValueTree d) :
	NodeBase(parent, d, 0)
{

}

scriptnode::NodeComponent* WrapperNode::createComponent()
{
	auto nc = ComponentHelpers::createDefaultComponent(this);

	if (auto extra = createExtraComponent())
	{
		ComponentHelpers::addExtraComponentToDefault(nc, extra);
	}

	return nc;
}

juce::Rectangle<int> WrapperNode::getPositionInCanvas(Point<int> topLeft) const
{
	int numParameters = getNumParameters();

	if (numParameters == 7)
		return createRectangleForParameterSliders(4).withPosition(topLeft);
	if (numParameters == 0)
		return createRectangleForParameterSliders(0).withPosition(topLeft);
	if (numParameters % 5 == 0)
		return createRectangleForParameterSliders(5).withPosition(topLeft);
	else if (numParameters % 4 == 0)
		return createRectangleForParameterSliders(4).withPosition(topLeft);
	else if (numParameters % 3 == 0)
		return createRectangleForParameterSliders(3).withPosition(topLeft);
	else if (numParameters % 2 == 0)
		return createRectangleForParameterSliders(2).withPosition(topLeft);
	else if (numParameters == 1)
		return createRectangleForParameterSliders(1).withPosition(topLeft);

	return {};
}

juce::Rectangle<int> WrapperNode::createRectangleForParameterSliders(int numColumns) const
{
	int h = UIValues::HeaderHeight;
	
	if (getEmbeddedNetwork() != nullptr)
		h += 24;

	auto eb = getExtraComponentBounds();

	h += eb.getHeight();
	

	int w = 0;

	if (numColumns == 0)
		w = eb.getWidth() > 0 ? eb.getWidth() : UIValues::NodeWidth * 2;
	else
	{
		int numParameters = getNumParameters();
		int numRows = (int)std::ceil((float)numParameters / (float)numColumns);

		h += numRows * (48 + 18);
		w = jmin(numColumns * 100, numParameters * 100);
	}


	w = jmax(w, eb.getWidth());

	auto b = Rectangle<int>(0, 0, w, h);
	return getBoundsToDisplay(b.expanded(UIValues::NodeMargin));
}



void InterpretedNode::reset()
{
	this->obj.reset();
}

void InterpretedNode::prepare(PrepareSpecs specs)
{
	auto& exceptionHandler = getRootNetwork()->getExceptionHandler();

	exceptionHandler.removeError(this);

	try
	{
		this->obj.prepare(specs);
	}
	catch (Error& s)
	{
		exceptionHandler.addError(this, s);
	}

	NodeBase::prepare(specs);
}

void InterpretedNode::processFrame(NodeBase::FrameType& data)
{
	if (data.size() == 1)
		processMonoFrame(MonoFrameType::as(data.begin()));
	if (data.size() == 2)
		processStereoFrame(StereoFrameType::as(data.begin()));
}

void InterpretedNode::processMonoFrame(MonoFrameType& data)
{
	this->obj.processFrame(data);
}

void InterpretedNode::processStereoFrame(StereoFrameType& data)
{
	this->obj.processFrame(data);
}

void InterpretedNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this);
	this->obj.process(data);
}

void InterpretedNode::setBypassed(bool shouldBeBypassed)
{
	WrapperNode::setBypassed(shouldBeBypassed);
	WrapperType::setParameter<bypass::ParameterId>(&this->obj, (double)shouldBeBypassed);
}

void InterpretedNode::handleHiseEvent(HiseEvent& e)
{
	this->obj.handleHiseEvent(e);
}

InterpretedModNode::InterpretedModNode(DspNetwork* parent, ValueTree d) :
	ModulationSourceNode(parent, d),
	Base()
{

}

void InterpretedModNode::postInit()
{
	stop();
	Base::postInit();
}

void* InterpretedModNode::getObjectPtr()
{
	return getObjectPtrFromWrapper();
}

scriptnode::ParameterDataList InterpretedModNode::createInternalParameterList()
{
	return createInternalParameterListFromWrapper();
}

void InterpretedModNode::timerCallback()
{
	getParameterHolder()->updateUI();
}

bool InterpretedModNode::isUsingNormalisedRange() const
{
	return this->obj.getWrappedObject().isNormalised;
}

scriptnode::parameter::dynamic_base_holder* InterpretedModNode::getParameterHolder()
{
	return &this->obj.p;
}

void InterpretedModNode::reset()
{
	this->obj.reset();
}

bool InterpretedModNode::isPolyphonic() const
{
	return this->obj.isPolyphonic();
}

void InterpretedModNode::prepare(PrepareSpecs specs)
{
	auto& exceptionHandler = getRootNetwork()->getExceptionHandler();

	exceptionHandler.removeError(this);

	try
	{
		ModulationSourceNode::prepare(specs);
		this->obj.prepare(specs);
	}
	catch (Error& s)
	{
		exceptionHandler.addError(this, s);
	}
}

void InterpretedModNode::processFrame(NodeBase::FrameType& data)
{
	if (data.size() == 1)
		processMonoFrame(MonoFrameType::as(data.begin()));
	if (data.size() == 2)
		processStereoFrame(StereoFrameType::as(data.begin()));
}

void InterpretedModNode::processMonoFrame(MonoFrameType& data)
{
	this->obj.processFrame(data);
}

void InterpretedModNode::processStereoFrame(StereoFrameType& data)
{
	this->obj.processFrame(data);
}

void InterpretedModNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this);
	this->obj.process(data);
}

void InterpretedModNode::handleHiseEvent(HiseEvent& e)
{
	this->obj.handleHiseEvent(e);
}

scriptnode::parameter::dynamic_base_holder* InterpretedCableNode::getParameterHolder()
{
	if (getParameterFunction)
		return getParameterFunction(getObjectPtr());

	return nullptr;
}

void InterpretedCableNode::prepare(PrepareSpecs ps)
{
	auto& exceptionHandler = getRootNetwork()->getExceptionHandler();

	exceptionHandler.removeError(this);

	try
	{
		ModulationSourceNode::prepare(ps);
		this->obj.prepare(ps);
	}
	catch (Error& s)
	{
		exceptionHandler.addError(this, s);
	}
}

void OpaqueNodeDataHolder::setExternalData(const snex::ExternalData& d, int index)
{
	SimpleRingBuffer::ScopedPropertyCreator sps(d.obj);

	base::setExternalData(d, index);
	opaqueNode.setExternalData(d, index);
}

OpaqueNodeDataHolder::OpaqueNodeDataHolder(OpaqueNode& n, NodeBase* pn) :
	opaqueNode(n),
	parentNode(pn)
{
	ExternalData::forEachType(BIND_MEMBER_FUNCTION_1(OpaqueNodeDataHolder::createDataType));

	if (auto fu = dynamic_cast<ExternalDataHolderWithForcedUpdate*>(pn->getRootNetwork()->getExternalDataHolder()))
	{
		fu->addForcedUpdateListener(this);
	}

#if 0
	auto numTables = opaqueNode.numDataObjects[(int)ExternalData::DataType::Table];

	for (int i = 0; i < numTables; i++)
		data.add(new data::dynamic::table(*this, i));

	auto numSliderPacks = opaqueNode.numDataObjects[(int)ExternalData::DataType::SliderPack];

	for (int i = 0; i < numSliderPacks; i++)
		data.add(new data::dynamic::sliderpack(*this, i));

	auto numAudioFiles = opaqueNode.numDataObjects[(int)ExternalData::DataType::AudioFile];

	for (int i = 0; i < numAudioFiles; i++)
		data.add(new data::dynamic::audiofile(*this, i));
#endif

	int index = 0;

	for (auto d : data)
	{
		d->initialise(parentNode);
		
		ExternalData ed(d->currentlyUsedData, index);
		SimpleReadWriteLock::ScopedWriteLock sl(d->currentlyUsedData->getDataLock());
		opaqueNode.setExternalData(ed, index++);
	}
}

OpaqueNodeDataHolder::~OpaqueNodeDataHolder()
{
	if (parentNode != nullptr)
	{
		if (auto fu = dynamic_cast<ExternalDataHolderWithForcedUpdate*>(parentNode->getRootNetwork()->getExternalDataHolder()))
		{
			fu->removeForcedUpdateListener(this);
		}
	}
}

scriptnode::data::pimpl::dynamic_base* OpaqueNodeDataHolder::create(ExternalData::DataType dt, int i)
{
	switch (dt)
	{
	case ExternalData::DataType::Table: return new data::dynamic::table(*this, i);
	case ExternalData::DataType::SliderPack: return new data::dynamic::sliderpack(*this, i);
	case ExternalData::DataType::AudioFile: return new data::dynamic::audiofile(*this, i);
	case ExternalData::DataType::FilterCoefficients: return new data::dynamic::filter(*this, i);
	case ExternalData::DataType::DisplayBuffer: return new data::dynamic::displaybuffer(*this, i);
    default: jassertfalse; return nullptr;
	}

	jassertfalse;
	return nullptr;
}

void OpaqueNodeDataHolder::createDataType(ExternalData::DataType dt)
{
	auto numObjects = opaqueNode.numDataObjects[(int)dt];

	for (int i = 0; i < numObjects; i++)
		data.add(create(dt, i));
}

int OpaqueNodeDataHolder::getNumDataObjects(ExternalData::DataType t) const
{
	return opaqueNode.numDataObjects[(int)t];
}

hise::Table* OpaqueNodeDataHolder::getTable(int index)
{
	auto dIndex = getAbsoluteIndex(ExternalData::DataType::Table, index);
	auto t = data[dIndex]->getTable(0);
	jassert(t != nullptr);
	return t;
}

hise::SliderPackData* OpaqueNodeDataHolder::getSliderPack(int index)
{
	auto dIndex = getAbsoluteIndex(ExternalData::DataType::SliderPack, index);
	auto t = data[dIndex]->getSliderPack(0);
	jassert(t != nullptr);
	return t;
}

hise::MultiChannelAudioBuffer* OpaqueNodeDataHolder::getAudioFile(int index)
{
	auto dIndex = getAbsoluteIndex(ExternalData::DataType::AudioFile, index);
	auto t = data[dIndex]->getAudioFile(0);
	jassert(t != nullptr);
	return t;
}

hise::FilterDataObject* OpaqueNodeDataHolder::getFilterData(int index)
{
	auto dIndex = getAbsoluteIndex(ExternalData::DataType::FilterCoefficients, index);
	auto t = data[dIndex]->getFilterData(0);
	jassert(t != nullptr);
	return t;
}

hise::SimpleRingBuffer* OpaqueNodeDataHolder::getDisplayBuffer(int index)
{
	auto dIndex = getAbsoluteIndex(ExternalData::DataType::DisplayBuffer, index);
	auto t = data[dIndex]->getDisplayBuffer(0);
	jassert(t != nullptr);
	return t;
}

bool OpaqueNodeDataHolder::removeDataObject(ExternalData::DataType t, int index)
{
	return false;
}



OpaqueNodeDataHolder::Editor::Editor(OpaqueNodeDataHolder* obj, PooledUIUpdater* u, bool addDragger) :
	ScriptnodeExtraComponent<OpaqueNodeDataHolder>(obj, u),
	updater(u)
{
	for (auto d : obj->data)
		addEditor(d);

	if (addDragger)
	{
		addAndMakeVisible(dragger = new ModulationSourceBaseComponent(u));
		
		height += UIValues::NodeMargin;

		dragger->setBounds(0, height, width, 28);
		height += 28;
	}
		

	setSize(width, height);

	stop();
}

void OpaqueNodeDataHolder::Editor::addEditor(data::pimpl::dynamic_base* d)
{
	auto useTwoColumns = getObject()->data.size() % 2 == 0;

	auto dt = ExternalData::getDataTypeForClass(d->getInternalData());

	data::ui::pimpl::editor_base* e;

	if (dt == snex::ExternalData::DataType::Table)
		e = new data::ui::table_editor_without_mod(updater, dynamic_cast<data::dynamic::table*>(d));

	if (dt == snex::ExternalData::DataType::SliderPack)
		e = new data::ui::sliderpack_editor_without_mod(updater, dynamic_cast<data::dynamic::sliderpack*>(d));

	if (dt == snex::ExternalData::DataType::AudioFile)
		e = new data::ui::audiofile_editor_with_mod(updater, dynamic_cast<data::dynamic::audiofile*>(d));

	if (dt == snex::ExternalData::DataType::FilterCoefficients)
		e = new data::ui::filter_editor(updater, dynamic_cast<data::dynamic::filter*>(d));

	if (dt == snex::ExternalData::DataType::DisplayBuffer)
		e = new data::ui::displaybuffer_editor_nomod(updater, dynamic_cast<data::dynamic::displaybuffer*>(d));

	

	addAndMakeVisible(e);
	editors.add(e);

	if (useTwoColumns)
	{
		auto bumpHeight = editors.size() % 2 == 0;

		if (bumpHeight)
		{
			auto thisE = editors.getLast();
			auto prevE = editors[editors.size() - 2];

			prevE->setBounds(0, height, 220, prevE->getHeight());
			thisE->setBounds(220, height, 220, thisE->getHeight());
			height += jmax(thisE->getHeight(), prevE->getHeight());
		}
			
		width = 440;
	}
	else
	{
		height += e->getHeight();
		width = jmax(width, e->getWidth());
	}
}

void OpaqueNodeDataHolder::Editor::resized()
{
	auto useTwoColumns = getObject()->data.size() % 2 == 0;

	if (!useTwoColumns)
	{
		auto b = getLocalBounds();

		for (auto e : editors)
			e->setBounds(b.removeFromTop(e->getHeight()));
	}
}

struct WrapperSlot : public ScriptnodeExtraComponent<InterpretedUnisonoWrapperNode>,
					 public NodeDropTarget,
					 public Value::Listener
{
	WrapperSlot(InterpretedUnisonoWrapperNode* obj, PooledUIUpdater * updater) :
		ScriptnodeExtraComponent<InterpretedUnisonoWrapperNode>(obj, updater)
	{
		wrappedNodeListener.setCallback(getObject()->getValueTree().getOrCreateChildWithName(PropertyIds::Nodes, getObject()->getUndoManager()), valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(WrapperSlot::rebuild));

		duplicateListener.setCallback(getObject()->getParameter(0)->data, {PropertyIds::Value}, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(WrapperSlot::updateVoiceAmount));

		duplicateValue.referTo(getObject()->getNodePropertyAsValue(PropertyIds::SplitSignal));
		duplicateValue.addListener(this);

		isChain = !(bool)duplicateValue.getValue();

		initialised = true;

		refreshNode();

		setRepaintsOnMouseActivity(true);
	};

	Value duplicateValue;

	~WrapperSlot()
	{
		if (getObject() != nullptr)
			getObject()->getNodePropertyAsValue(PropertyIds::SplitSignal).removeListener(this);
	}

	void valueChanged(Value& v)
	{
		isChain = !(bool)v.getValue();
		
		updateVoiceAmount({}, {});
	}

	void refreshNode()
	{
		if (getObject()->wrappedNode != nullptr)
		{
			nc = getObject()->wrappedNode->createComponent();

			addAndMakeVisible(nc);

			Point<int> startPos(UIValues::NodeMargin, UIValues::NodeMargin);

			auto bounds = nc->node->getPositionInCanvas(startPos);
			bounds = nc->node->getBoundsWithoutHelp(bounds);

			nc->setSize(bounds.getWidth(), bounds.getHeight());

			auto w = bounds.getWidth() + 2 * UIValues::NodeMargin;
			auto h = bounds.getHeight() + 2 * UIValues::NodeMargin;

			ncBounds = bounds;

			setSize(w, h);

			getObject()->setCachedSize(w, h);
		}
		else
		{
			nc = nullptr;

			auto w = 256;
			auto h = 20 + 2 * UIValues::NodeMargin;

			setSize(w, h);
			getObject()->setCachedSize(w, h);
		}
	}

	void rebuild(ValueTree v, bool wasAdded)
	{
		if (!initialised)
			return;

		ScopedValueSetter<bool> svs(initialised, false);

		if (auto dn = findParentComponentOfClass<DspNetworkGraph>())
		{
			dn->resizeNodes();
		}
			
	}

	void resized() override
	{
		if (nc != nullptr)
		{
			if (isChain)
			{
				auto y = getHeight() - UIValues::NodeMargin - nc->getHeight();
				nc->setTopLeftPosition(UIValues::NodeMargin, y);
				ncBounds = nc->getBoundsInParent();
			}
			else
			{
				auto x = getWidth() - UIValues::NodeMargin - nc->getWidth();
				auto y = getHeight() - UIValues::NodeMargin - nc->getHeight();
				nc->setTopLeftPosition(x, y);
				ncBounds = nc->getBoundsInParent();
			}
		}
	}

	void mouseDown(const MouseEvent& e) override
	{
		if (auto n = findParentComponentOfClass<DspNetworkGraph>())
		{
			KeyboardPopup* newPopup = new KeyboardPopup(getObject(), 0);

			auto sp = findParentComponentOfClass<ZoomableViewport>();
			auto r = sp->getLocalArea(this, getLocalBounds());

			sp->setCurrentModalWindow(newPopup, r);
		}
	}

	virtual void setDropTarget(Point<int> position)
	{
		insideDrag = getLocalBounds().contains(position);
		repaint();
	}

	void insertDraggedNode(NodeComponent* newNode, bool copyNode)
	{
		auto nodeTree = getObject()->nodeListener.getParentTree();
		nodeTree.removeAllChildren(getObject()->getUndoManager());

		auto newTree = newNode->node->getValueTree();
		newTree.getParent().removeChild(newTree, getObject()->getUndoManager());
		nodeTree.addChild(newTree, 0, getObject()->getUndoManager());
	}

	void clearDropTarget()
	{
		insideDrag = false;
		repaint();
	}

	void mouseExit(const MouseEvent& e) override
	{
		insideDrag = false;
		repaint();
	}

	void removeDraggedNode(NodeComponent* draggedNode) override
	{
		auto oldOne = nc.release();

		removeChildComponent(oldOne);

		nc = new DeactivatedComponent(oldOne->node);

		addAndMakeVisible(nc);

		resized();
		repaint();
	}

	void updateVoiceAmount(Identifier, var)
	{
		if (nc != nullptr)
		{
			numVoices = getObject()->getUnisonoObject()->getNumVoices();
			auto b = ncBounds;

			auto w = (float)(b.getWidth() + 2 * UIValues::NodeMargin);
			auto h = (float)(b.getHeight() + 2 * UIValues::NodeMargin);

			float scale = 0.98f;

			float delta = scale * (float)UIValues::NodeMargin;

			for (int i = 1; i < numVoices; i++)
			{
				if (isChain)
					h += delta;
				else
					w += delta;

				delta *= scale;
			}

			auto b2 = nc->getLocalBounds().reduced(1);
			
			if (isChain)
				b2 = b2.removeFromTop(UIValues::NodeMargin);
			else
				b2 = b2.removeFromLeft(UIValues::NodeMargin);

			snapshot1 = nc->createComponentSnapshot(b2, true);

			{
				Graphics g(snapshot1);
				g.setGradientFill(ColourGradient(Colours::transparentBlack, 0.0f, 0.0f, Colours::black.withAlpha(0.5f), 0.0f, b2.getHeight(), false));
				g.fillAll();
				g.setColour(Colours::black.withAlpha(0.4f));
				g.drawRect(b2.toFloat(), 1.0f);
			}

			{
				PostGraphicsRenderer::DataStack st;
				PostGraphicsRenderer g2(st, snapshot1);
				g2.desaturate();
			}
			
			setSize((int)w, (int)h);

			getObject()->setCachedSize(w, (int)h);

			ScopedValueSetter<bool> svs(initialised, false);

			if (auto dn = findParentComponentOfClass<DspNetworkGraph>())
				dn->resizeNodes();
		}
	}

	void timerCallback() override
	{

	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black.withAlpha(0.4f));

		auto cVar = getObject()->getValueTree()[PropertyIds::NodeColour];
		Colour c = PropertyHelpers::getColourFromVar(cVar);

		float alpha = 0.05f;

		if (isMouseOver(false))
			alpha += 0.03f;

		if (c == Colours::transparentBlack)
		{
			g.setColour(Colours::white.withAlpha(alpha));
		}
		else
		{
			alpha += 0.05f;
			g.setColour(c.withAlpha(alpha));
		}

		if (insideDrag)
		{
			alpha += 0.1f;
			g.setColour(Colour(SIGNAL_COLOUR).withAlpha(alpha));
		}

		auto b = getLocalBounds().toFloat().reduced(2.0f);

		for (int i = 0; i < getHeight(); i += 10)
		{
			auto x = b.removeFromTop(9.0f);
			g.fillRect(x);
			b.removeFromTop(1.0f);
		}

		g.setColour(Colours::white.withAlpha(0.2f));
		g.drawRect(getLocalBounds().toFloat(), 1.0f);

		if (nc == nullptr)
		{
			g.setColour(Colours::white.withAlpha(0.7f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(insideDrag ? "Drop Node here" : "Click to add node", getLocalBounds().toFloat(), Justification::centred);
		}

		auto pos = ncBounds.toFloat();

		alpha = 1.0f;

		float scaleFactor = 0.98f;
		
		float startX, startY, startHeight, startWidth;

		if (isChain)
		{
			startHeight = 10.0 * scaleFactor;
			startY = pos.getY() - 10.0 * scaleFactor;
			startX = pos.getX();
			startWidth = pos.getWidth() * scaleFactor;
			startX += (pos.getWidth() - startWidth) / 2.0f;
		}
		else
		{
			startHeight = pos.getHeight() * scaleFactor;
			startY = pos.getY();
			startX = pos.getX() - 10.0 * scaleFactor;
			startWidth = 10.0 * scaleFactor;
			startY += (pos.getHeight() - startHeight) / 2.0f;
		}

		g.setColour(Colours::black);

		for (int i = 0; i < numVoices-1; i++)
		{
			Rectangle<float> a(startX, startY, startWidth, startHeight);

			g.setColour(Colours::black);

			g.drawImage(snapshot1, a, RectanglePlacement::stretchToFit);

			auto b = (float)i * JUCE_LIVE_CONSTANT_OFF(0.2f);

			g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x751f1f1f)).withMultipliedAlpha(b));
			g.fillRect(a);

			if (isChain)
			{
				startHeight *= scaleFactor;
				startX += (startWidth - startWidth * scaleFactor) / 2.0f;
				startWidth *= scaleFactor;
				startY -= startHeight;
			}
			else
			{
				startHeight *= scaleFactor;
				startY += (startHeight - startHeight * scaleFactor) / 2.0f;
				startWidth *= scaleFactor;
				startX -= startWidth;
			}
		}
	}

	valuetree::ChildListener wrappedNodeListener;
	valuetree::PropertyListener duplicateListener;
	
	Image snapshot1;

	ScopedPointer<NodeComponent> nc;

	Rectangle<int> ncBounds;

	bool isChain = false;

	bool initialised = false;
	int numVoices = 1;
	bool insideDrag = false;
};

InterpretedUnisonoWrapperNode::InterpretedUnisonoWrapperNode(DspNetwork* n, ValueTree d) :
	WrapperNode(n, d),
	InterpretedNodeBase<Base>(),
	duplicateProperty(PropertyIds::SplitSignal, false)
{
	obj.initialise(this);

	duplicateProperty.initialise(this);
	duplicateProperty.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(InterpretedUnisonoWrapperNode::updateDuplicateMode), true);

	auto nodeTree = getValueTree().getOrCreateChildWithName(PropertyIds::Nodes, getUndoManager());
	nodeListener.setCallback(nodeTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(InterpretedUnisonoWrapperNode::updateChildNode));

	for (auto p : createInternalParameterList())
	{
		auto n = p.info.getId();

		auto pTree = getParameterTree().getChildWithProperty(PropertyIds::ID, n);

		if (!pTree.isValid())
		{
			pTree = p.createValueTree();
			getParameterTree().addChild(pTree, -1, getUndoManager());
		}

		auto np = new Parameter(this, pTree);

		addParameter(np);
		np->getDynamicParameterAsHolder()->setParameter(new parameter::dynamic_base(p.callback));
	}
}

ReferenceCountedArray<Parameter> InterpretedUnisonoWrapperNode::getParameterList(const ValueTree& pTree)
{
	ReferenceCountedArray<Parameter> list;

	SimpleReadWriteLock::ScopedReadLock sl(obj.getVoiceLock());

	for (auto& o : obj)
	{
		o.getRootNode()->forEach([&list, &pTree](NodeBase::Ptr nb)
		{
			for (int i = 0; i < nb->getNumParameters(); i++)
			{
				auto p = nb->getParameter(i);

				if (p->data == pTree)
				{
					list.add(p);
					return true;
				}
			}

			return false;
		});
	}

	return list;
}

Component* InterpretedUnisonoWrapperNode::createWrapperSlot(void* obj, PooledUIUpdater* updater)
{
	auto typed = static_cast<InterpretedUnisonoWrapperNode*>(obj);
	return new WrapperSlot(typed, updater);
}

void InterpretedUnisonoWrapperNode::assign(int index, var newValue)
{
	if (auto n = dynamic_cast<NodeBase*>(newValue.getObject()))
	{
		auto nodeTree = nodeListener.getParentTree();
		nodeTree.removeAllChildren(getUndoManager());
		nodeTree.addChild(n->getValueTree(), -1, getUndoManager());
	}
}

var InterpretedUnisonoWrapperNode::getAssignedValue(int index) const
{
	return var(wrappedNode.get());
}

int InterpretedUnisonoWrapperNode::getCachedIndex(const var &indexExpression) const
{
	return (int)indexExpression;
}

void InterpretedUnisonoWrapperNode::updateChildNode(ValueTree v, bool wasAdded)
{
	if (skipTreeListener)
		return;

	if (wasAdded)
	{
		wrappedNode = getRootNetwork()->createFromValueTree(getRootNetwork()->isPolyphonic(), v);
		wrappedNode->setParentNode(this);
		wrappedNode->setIsUINodeOfDuplicates(true);

		obj.getWrappedObject().clone(wrappedNode);
		obj.refreshFromFirst();

		auto isContainer = dynamic_cast<NodeContainer*>(wrappedNode.get());

		obj.setDuplicateSignal(isContainer);
	}
	else
	{
		if (wrappedNode != nullptr)
		{
			wrappedNode->setParentNode(nullptr);
			wrappedNode->setIsUINodeOfDuplicates(false);
			wrappedNode = nullptr;
		}

		obj.getWrappedObject().clear();
		obj.refreshFromFirst();
	}
}

void InterpretedUnisonoWrapperNode::updateDuplicateMode(Identifier, var value)
{
	auto shouldBeEnabled = (bool)value;
	obj.setDuplicateSignal(shouldBeEnabled);
}

scriptnode::ParameterDataList InterpretedUnisonoWrapperNode::createInternalParameterList()
{
	ParameterDataList l;

	{
		parameter::data p("NumVoices", { 1.0, 16.0, 1.0, 1.0 });
		p.setDefaultValue(1.0);
		p.callback.referTo(&obj, Base::setWrapParameterStatic<0>);
		l.add(p);
	}
	
	return l;
}

}

