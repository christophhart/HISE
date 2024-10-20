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
	auto b = NodeComponent::PositionHelpers::getPositionInCanvasForStandardSliders(this, topLeft);
	return getBoundsToDisplay(b);
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
		this->obj.prepare(ps);
		ModulationSourceNode::prepare(ps);
		
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

	data::ui::pimpl::editor_base* e = nullptr;

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

namespace node_templates
{
struct mid_side
{
	SN_NODE_ID("mid_side");

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		TemplateNodeFactory::Builder h(n, v);
		h.setRootType("container.chain");

		h.addNode(0, "routing.ms_decode", "decoder");

		auto multi = h.addNode(0, "container.multi", "ms_splitter");

		h.addNode(0, "routing.ms_encode", "encoder");

		auto mid = h.addNode(multi, "container.chain", "mid_chain");
		auto side = h.addNode(multi, "container.chain", "side_chain");

		h.addNode(mid, "math.mul", "mid_gain");
		h.addNode(side, "math.mul", "side_gain");

		return h.flush();
	}
};

struct dry_wet
{
	SN_NODE_ID("dry_wet");

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		TemplateNodeFactory::Builder h(n, v);
		h.setRootType("container.split");

		auto dryPath = h.addNode(0, "container.chain", "dry_path");
		auto wetPath = h.addNode(0, "container.chain", "wet_path");

		auto xfader = h.addNode(dryPath, "control.xfader", "dry_wet_mixer");

		h.addParameter(0, "DryWet", InvertableParameterRange(0.0, 1.0));

		auto dry_gain = h.addNode(dryPath, "core.gain", "dry_gain");

		auto dummy = h.addNode(wetPath, "math.mul", "dummy");

		h.nodes[dummy].setProperty(PropertyIds::Comment, "Add the wet DSP processing here...", nullptr);
		h.setNodeColour({ dummy }, Colours::white);

		auto wet_gain = h.addNode(wetPath, "core.gain", "wet_gain");

		h.connect(0, PropertyIds::Parameters, 0, xfader, 0);
		h.connect(xfader, PropertyIds::SwitchTargets, 0, dry_gain, 0);
		h.connect(xfader, PropertyIds::SwitchTargets, 1, wet_gain, 0);

		h.setNodeColour({ 0, xfader, dry_gain, wet_gain }, h.getRandomColour());

		h.setFolded({ xfader, dry_gain, wet_gain });

		return h.flush();
	}
};


struct feedback_delay
{
	SN_NODE_ID("feedback_delay");

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		TemplateNodeFactory::Builder b(n, v);
		b.setRootType("container.fix32_block");

		auto out = b.addNode(0, "routing.receive", "fb_out");
		b.addNode(0, "core.fix_delay", "delay");
		auto in = b.addNode(0, "routing.send", "fb_in");

		b.connectSendReceive(in, { out });

		b.setParameterValues({ out }, { "Feedback" }, { 0.4 });
		


		return b.flush();
	}
};

struct bipolar_mod
{
	SN_NODE_ID("bipolar_mod");

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		Array<int> foldNodes;

		TemplateNodeFactory::Builder b(n, v);

		b.setRootType("container.modchain");

		auto modSignal = b.addNode(0, "container.chain", "mod_signal");

		b.addComment(modSignal, "Create a signal between 0...1 here");

		auto dummy = b.addNode(modSignal, "core.ramp", "dummy");

		b.setParameterValues({ dummy }, { "PeriodTime" }, { 1000.0 });

		b.setNodeColour({ dummy }, Colours::white);

		auto sig2mod = b.addNode(0, "math.sig2mod", "sig2mod");

		auto peak = b.addNode(0, "core.peak", "peak");
		auto bipolar = b.addNode(0, "control.bipolar", "bipolar");
		auto pma = b.addNode(0, "control.pma", "pma");

		b.connect(peak, PropertyIds::ModulationTargets, 0, bipolar, 0);
		b.connect(bipolar, PropertyIds::ModulationTargets, 0, pma, 2);

		b.setFolded({ dummy, sig2mod, peak, bipolar });

		b.addComment(pma, "Connect this to the target knob");

		b.addParameter(0, "Value", {});
		b.addParameter(0, "Intensity", {});

		b.connect(0, PropertyIds::Parameters, 0, pma, 0);
		b.connect(0, PropertyIds::Parameters, 1, bipolar, 1);
		
		

		return b.flush();
	}
};

template <int NumBands> struct freq_split
{
	static Identifier getStaticId() { return Identifier("freq_split" + String(NumBands)); }

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		TemplateNodeFactory::Builder b(n, v);

		b.setRootType("container.split");

		InvertableParameterRange fr(20.0, 20000.0);
		fr.setSkewForCentre(1000.0);

		StringArray bandNames;
		Array<double> values;

		for (int i = 0; i < NumBands - 1; i++)
		{

			bandNames.add("Band " + String(i + 1));
			b.addParameter(0, bandNames[bandNames.size() - 1], fr);

			auto normFreq = (double)(i + 1) / (double)(NumBands + 1);

			values.add(fr.convertFrom0to1(normFreq, false));

		}

		b.setParameterValues({ 0 }, bandNames, values);

		Array<Array<int>> filters;

		Array<int> dummies;

		for (int i = 0; i < NumBands; i++)
		{
			Array<int> thisFilter;

			auto bandChain = b.addNode(0, "container.chain", "band" + String(i + 1));

			for (int j = 0; j < NumBands-1; j++)
			{
				String id = "lr";
				id << String(i + 1) << "_" << String(j + 1);
				thisFilter.add(b.addNode(bandChain, "jdsp.jlinkwitzriley", id));
			}

			//b.setComplexDataIndex(thisFilter, ExternalData::DataType::FilterCoefficients, i);

			filters.add(thisFilter);

			b.setNodeColour(filters[i], b.getRandomColour());
			b.setFolded(filters[i]);

			dummies.add(b.addNode(bandChain, "math.mul", "dummy" + String(i + 1)));
		}

		b.setNodeColour(dummies, Colours::white);

		enum class FilterType { LP, HP, AP};
		auto setFilterType = [&](int bandIndex, int filterIndex, FilterType t)
		{
			auto n = filters[bandIndex][filterIndex];
			b.setParameterValues({ n }, { "Type" }, (double)(int)t);
		};

		for (int bi = 0; bi < NumBands; bi++)
		{
			for (int fi = 0; fi < NumBands - 1; fi++)
			{
				if (bi == fi && (bi != NumBands - 1))
					setFilterType(bi, fi, FilterType::LP);
				else if (bi == fi + 1)
					setFilterType(bi, fi, FilterType::HP);
				else
					setFilterType(bi, fi, FilterType::AP);

				b.connect(0, PropertyIds::Parameters, fi, filters[bi][fi], 0);
			}
		}

		return b.flush();
	}
};

template <int NumSwitches> struct softbypass_switch
{
	static Identifier getStaticId() { return Identifier("softbypass_switch" + String(NumSwitches)); }

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		TemplateNodeFactory::Builder b(n, v);

		b.setRootType("container.chain");

		auto xfader = b.addNode(0, "control.xfader", "switcher");

		auto c1 = b.addNode(0, "container.chain", "sb_container");

		Array<int> dummies, sbContainers;

		InvertableParameterRange pr(0.0, (double)NumSwitches-1.0, 1.0);

		b.addParameter(0, "Switch", pr);

		b.connect(0, PropertyIds::Parameters, 0, xfader, 0);

		auto stree = b.nodes[xfader].getOrCreateChildWithName(PropertyIds::SwitchTargets, nullptr);

		auto numToAdd = NumSwitches - stree.getNumChildren();

		for (int i = 0; i < numToAdd; i++)
		{
			ValueTree v(PropertyIds::SwitchTarget);
			stree.addChild(v, -1, nullptr);
		}

		b.setNodeProperty({xfader},
		{
			{"NumParameters", var(NumSwitches)},
			{"Mode", var("Switch")}
		});

		// make it create the switch target trees.
		b.fillValueTree(xfader);

		b.setNodeProperty({ c1 }, { {PropertyIds::IsVertical, false} });

		for (int i = 0; i < NumSwitches; i++)
		{
			sbContainers.add(b.addNode(c1, "container.soft_bypass", "sb" + String(i + 1)));
			dummies.add(b.addNode(sbContainers.getLast(), "math.mul", "dummy"));

			b.connect(xfader, PropertyIds::SwitchTargets, i, sbContainers.getLast(), TemplateNodeFactory::Builder::BypassIndex);
		}

		sbContainers.add(xfader);
		sbContainers.add(c1);
		

		b.setNodeColour(sbContainers, b.getRandomColour());

		b.setNodeColour(dummies, Colours::white);

		return b.flush();
	}
};


}





TemplateNodeFactory::TemplateNodeFactory(DspNetwork* n) :
	NodeFactory(n)
{
	registerNodeRaw<node_templates::mid_side>();
	registerNodeRaw<node_templates::dry_wet>();
	registerNodeRaw<node_templates::feedback_delay>();
	registerNodeRaw<node_templates::bipolar_mod>();
	registerNodeRaw<node_templates::freq_split<2>>();
	registerNodeRaw<node_templates::freq_split<3>>();
	registerNodeRaw<node_templates::freq_split<4>>();
	registerNodeRaw<node_templates::freq_split<5>>();
	registerNodeRaw<node_templates::softbypass_switch<2>>();
	registerNodeRaw<node_templates::softbypass_switch<3>>();
	registerNodeRaw<node_templates::softbypass_switch<4>>();
	registerNodeRaw<node_templates::softbypass_switch<5>>();
	registerNodeRaw<node_templates::softbypass_switch<6>>();
	registerNodeRaw<node_templates::softbypass_switch<7>>();
	registerNodeRaw<node_templates::softbypass_switch<8>>();

#if USE_BACKEND
	auto fileTemplates = BackendDllManager::getAllNodeTemplates(n->getScriptProcessor()->getMainController_());

	for(auto v: fileTemplates)
	{
		auto name = v[PropertyIds::Name].toString();
		if(name.isEmpty())
			name = v[PropertyIds::ID].toString();

		registerNodeWithLambda(name, [v](DspNetwork* n, ValueTree dummy)
		{
			dummy.setProperty(PropertyIds::ID, "AAARG", nullptr);

			Array<DspNetwork::IdChange> changes;
			auto newTree = n->cloneValueTreeWithNewIds(v, changes, false);
			DuplicateHelpers::removeOutsideConnections({ newTree }, changes );

			for(auto& c: changes)
	            n->changeNodeId(newTree, c.oldId, c.newId, nullptr);

			auto newNode = n->createFromValueTree(n->isPolyphonic(), newTree, true);

			return newNode;
		});
	}
#endif
}

int TemplateNodeFactory::Builder::addNode(int parent, const String& path, const String& id, int index)
{
	ValueTree v(PropertyIds::Node);

	auto newId = network->getNonExistentId(id, existingIds);

	v.setProperty(PropertyIds::ID, newId, nullptr);
	v.setProperty(PropertyIds::FactoryPath, path, nullptr);

	nodes[parent].getOrCreateChildWithName(PropertyIds::Nodes, nullptr).addChild(v, index, nullptr);

	nodes.add(v);

	return nodes.size()-1;
}

void TemplateNodeFactory::Builder::addParameter(int nodeIndex, const String& name, InvertableParameterRange r)
{
	ValueTree p(PropertyIds::Parameter);

	nodes[nodeIndex].setProperty(PropertyIds::ShowParameters, true, nullptr);

	RangeHelpers::storeDoubleRange(p, r, nullptr);
	p.setProperty(PropertyIds::ID, name, nullptr);
	nodes[nodeIndex].getOrCreateChildWithName(PropertyIds::Parameters, nullptr).addChild(p, -1, nullptr);
}


bool TemplateNodeFactory::Builder::connectSendReceive(int sendIndex, Array<int> receiveIndexes)
{
	StringArray sa;

	for (auto r : receiveIndexes)
	{
		sa.add(nodes[r][PropertyIds::ID].toString());
	}

	fillValueTree(sendIndex);
	auto ctree = nodes[sendIndex].getChildWithName(PropertyIds::Properties).getChildWithProperty(PropertyIds::ID, PropertyIds::Connection.toString());

	jassert(ctree.isValid());

	ctree.setProperty(PropertyIds::Value, sa.joinIntoString(";"), nullptr);
	return true;
}

bool TemplateNodeFactory::Builder::connect(int nodeIndex, const Identifier sourceType, int sourceIndex, int targetNodeIndex, int targetParameterIndex)
{
	fillValueTree(nodeIndex);
	fillValueTree(targetNodeIndex);

	jassert(sourceType == PropertyIds::Parameters ||
			sourceType == PropertyIds::ModulationTargets ||
			sourceType == PropertyIds::SwitchTargets);

	auto sourceTree = nodes[nodeIndex].getChildWithName(sourceType);
	
	if (sourceType != PropertyIds::ModulationTargets)
	{
		sourceTree = sourceTree.getChild(sourceIndex);

		jassert(sourceTree.isValid());
		sourceTree = sourceTree.getOrCreateChildWithName(PropertyIds::Connections, nullptr);
	}
	
	ValueTree c(PropertyIds::Connection);
	c.setProperty(PropertyIds::NodeId, nodes[targetNodeIndex][PropertyIds::ID], nullptr);

	if (targetParameterIndex == BypassIndex) // connect to bypass
	{
		c.setProperty(PropertyIds::ParameterId, PropertyIds::Bypassed.toString(), nullptr);
	}
	else
	{
		auto targetTree = nodes[targetNodeIndex].getChildWithName(PropertyIds::Parameters).getChild(targetParameterIndex);
		jassert(targetTree.isValid());

		c.setProperty(PropertyIds::ParameterId, targetTree[PropertyIds::ID], nullptr);
		targetTree.setProperty(PropertyIds::Automated, true, nullptr);
	}

	
	sourceTree.addChild(c, -1, nullptr);

	return true;
}

void TemplateNodeFactory::Builder::setNodeColour(Array<int> nodeIndexes, Colour c)
{
	for (auto n : nodeIndexes)
	{
		nodes[n].setProperty(PropertyIds::NodeColour, (int64)c.getARGB(), nullptr);
	}
}

void TemplateNodeFactory::Builder::setProperty(Array<int> nodeIndexes, const Identifier& id, const var& value)
{
	for (auto n : nodeIndexes)
		nodes[n].setProperty(id, value, nullptr);
}

void TemplateNodeFactory::Builder::setNodeProperty(Array<int> nodeIndexes, const NamedValueSet& properties)
{
	for (auto n : nodeIndexes)
	{
		fillValueTree(n);

		auto propTree = nodes[n].getOrCreateChildWithName(PropertyIds::Properties, nullptr);

		for (auto p : properties)
		{
			auto existing = propTree.getChildWithProperty(PropertyIds::ID, p.name.toString());

			if (existing.isValid())
				existing.setProperty(PropertyIds::Value, p.value, nullptr);
			else
			{
				ValueTree np(PropertyIds::Property);
				np.setProperty(PropertyIds::ID, p.name.toString(), nullptr);
				np.setProperty(PropertyIds::Value, p.value, nullptr);
				propTree.addChild(np, -1, nullptr);
			}
		}
	}
}

void TemplateNodeFactory::Builder::setParameterValues(Array<int> nodeIndexes, StringArray parameterIds, Array<double> values)
{
	for (auto n : nodeIndexes)
	{
		fillValueTree(n);

		auto ptree = nodes[n].getChildWithName(PropertyIds::Parameters);

		for (int i = 0; i < parameterIds.size(); i++)
		{
			auto p = ptree.getChildWithProperty(PropertyIds::ID, parameterIds[i]);
			p.setProperty(PropertyIds::Value, values[i], nullptr);
		}
	}
}

void TemplateNodeFactory::Builder::setComplexDataIndex(Array<int> nodeIndexes, ExternalData::DataType type, int index)
{
	for (auto n : nodeIndexes)
	{
		fillValueTree(n);

		auto ctree = nodes[n].getChildWithName(PropertyIds::ComplexData);

		auto typeTree = ctree.getChildWithName(ExternalData::getDataTypeName(type, true));

		for (auto c : typeTree)
		{
			c.setProperty(PropertyIds::Index, index, nullptr);
		}
	}
}

void TemplateNodeFactory::Builder::addComment(Array<int> nodeIndexes, const String& comment)
{
	for (auto n : nodeIndexes)
	{
		nodes[n].setProperty(PropertyIds::Comment, comment, nullptr);
	}
}

void TemplateNodeFactory::Builder::setFolded(Array<int> nodeIndexes)
{
	for (auto n : nodeIndexes)
	{
		nodes[n].setProperty(PropertyIds::Folded, true, nullptr);
	}
}

void TemplateNodeFactory::Builder::fillValueTree(int nodeIndex)
{
	// Do not create containers
	if (nodes[nodeIndex][PropertyIds::FactoryPath].toString().startsWith("container"))
		return;

	if (network->getNodeForValueTree(nodes[nodeIndex]))
		return;

	network->createFromValueTree(network->isPolyphonic(), nodes[nodeIndex], false);
	network->deleteIfUnused(nodes[nodeIndex][PropertyIds::ID].toString());
}

}

