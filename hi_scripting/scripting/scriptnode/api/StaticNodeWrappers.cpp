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
	FrameDataPeakChecker fd(this, data.begin(), data.size());
	this->obj.processFrame(data);
}

void InterpretedNode::processStereoFrame(StereoFrameType& data)
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());
	this->obj.processFrame(data);
}

void InterpretedNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this, data.getNumSamples());
	ProcessDataPeakChecker fd(this, data);
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

bool InterpretedModNode::isUsingNormalisedRange() const
{
	return this->obj.getWrappedObject().isNormalised;
}

scriptnode::parameter::dynamic_base_holder* InterpretedModNode::getParameterHolder()
{
	return &this->obj.obj.p;
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
	FrameDataPeakChecker fd(this, data.begin(), data.size());

	this->obj.processFrame(data);
}

void InterpretedModNode::processStereoFrame(StereoFrameType& data)
{
	FrameDataPeakChecker fd(this, data.begin(), data.size());
	this->obj.processFrame(data);
}

void InterpretedModNode::process(ProcessDataDyn& data) noexcept
{
	NodeProfiler np(this, data.getNumSamples());
	ProcessDataPeakChecker pd(this, data);
	this->obj.process(data);
}

void InterpretedModNode::setBypassed(bool shouldBeBypassed)
{
    WrapperNode::setBypassed(shouldBeBypassed);
    WrapperType::setParameter<bypass::ParameterId>(&this->obj, (double)shouldBeBypassed);
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


scriptnode::parameter::dynamic_base_holder* control::dynamic_dupli_pack::getParameterFunction(void* obj)
{
	auto typed = static_cast<dynamic_dupli_pack*>(obj);
	return &typed->getParameter();
}

}

