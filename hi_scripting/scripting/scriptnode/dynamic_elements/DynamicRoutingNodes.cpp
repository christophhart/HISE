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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace scriptnode {
using namespace juce;
using namespace hise;



namespace routing
{

#if USE_BACKEND
struct MatrixEditor : public ScriptnodeExtraComponent<matrix<dynamic_matrix>>
{
	MatrixEditor(matrix<dynamic_matrix>* r, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<matrix<dynamic_matrix>>(r, updater),
		editor(&r->m.getMatrix())
	{
		addAndMakeVisible(editor);
		setSize(600, 200);
		stop();
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new MatrixEditor(static_cast<ObjectType*>(obj), updater);
	}

	void timerCallback() override
	{

	}

	void resized() override
	{
		editor.setBounds(getLocalBounds());
	}

	hise::RouterComponent editor;
};
#else
using MatrixEditor = HostHelpers::NoExtraComponent;
#endif



Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	// create this once to set the property...
	public_mod once;

	registerNode<matrix<dynamic_matrix>, MatrixEditor>();

	registerNode<dynamic_send, cable::dynamic::editor>();
	registerNode<dynamic_receive, cable::dynamic::editor>();
	registerNode<ms_encode>();
	registerNode<ms_decode>();
	registerNode<public_mod>();

	registerNodeRaw<GlobalSendNode>();
	registerPolyNodeRaw<GlobalReceiveNode<1>, GlobalReceiveNode<NUM_POLYPHONIC_VOICES>>();
	registerNodeRaw<GlobalCableNode>();
}

}

namespace cable
{
snex::NamespacedIdentifier dynamic::getReceiveId()
{
	return NamespacedIdentifier("routing").getChildId(dynamic_receive::getStaticId());
}

void dynamic::prepare(PrepareSpecs ps)
{
	sendSpecs = ps;

	checkSourceAndTargetProcessSpecs();

	numChannels = ps.numChannels;

	if (ps.blockSize == 1)
	{
		useFrameDataForDisplay = true;
		frameData.referTo(data_, ps.numChannels);
		buffer.setSize(0);
	}
	else
	{
		useFrameDataForDisplay = false;

		frameData.referTo(data_, ps.numChannels);
		DspHelpers::increaseBuffer(buffer, ps);


		auto ptr = buffer.begin();
		FloatVectorOperations::clear(ptr, ps.blockSize * ps.numChannels);

		for (int i = 0; i < ps.numChannels; i++)
		{
			channels[i].referToRawData(ptr, ps.blockSize);
			ptr += ps.blockSize;
		}
	}
}

void dynamic::restoreConnections(Identifier id, var newValue)
{
	WeakReference<dynamic> safePtr(this);

	auto f = [safePtr, id, newValue]()
	{
		if (safePtr.get() == nullptr)
			return true;

		if (id == PropertyIds::Value && safePtr->parentNode != nullptr)
		{
			auto ids = StringArray::fromTokens(newValue.toString(), ";", "");
			ids.removeDuplicates(false);
			ids.removeEmptyStrings(true);

			auto network = safePtr->parentNode->getRootNetwork();

			auto list = network->getListOfNodesWithPath(getReceiveId(), false);

			for (auto n : list)
			{
				if (auto rn = dynamic_cast<InterpretedNode*>(n.get()))
				{
					auto& ro = rn->getWrappedObject();

					auto source = ro.as<dynamic_receive>().source;

					if (ids.contains(rn->getId()))
					{
						source = safePtr.get();
						source->connect(ro.as<dynamic_receive>());
						return true;
					}
					else
					{
						if (source == safePtr.get())
						{
							source = &(ro.as<dynamic_receive>().null);
							return true;
						}
							
					}
				}
			}
		}

		return false;
	};

	parentNode->getRootNetwork()->addPostInitFunction(f);
}

void dynamic::setConnection(dynamic_receive& receiveTarget, bool addAsConnection)
{
	receiveTarget.source = addAsConnection ? this : &receiveTarget.null;

	if (sendSpecs)
	{
		prepare(sendSpecs);
	}

	if (parentNode != nullptr)
	{
		auto l = parentNode->getRootNetwork()->getListOfNodesWithPath(getReceiveId(), true);

		for (auto n : l)
		{
			if (auto typed = dynamic_cast<InterpretedNode*>(n.get()))
			{
				if (&typed->getWrappedObject().as<dynamic_receive>() == &receiveTarget)
				{
					auto rIds = StringArray::fromTokens(receiveIds.getValue(), ";", "");

					rIds.removeEmptyStrings(true);
					rIds.removeDuplicates(false);
					rIds.sort(false);

					if (addAsConnection)
						rIds.addIfNotAlreadyThere(n->getId());
					else
						rIds.removeString(n->getId());

					receiveIds.storeValue(rIds.joinIntoString(";"), n->getUndoManager());
				}
			}
		}
	}
}



void dynamic::checkSourceAndTargetProcessSpecs()
{
	if (!sendSpecs)
		return;

	if (!receiveSpecs)
		return;
	
	if (postPrepareCheckActive)
		return;

	if (!(sendSpecs == receiveSpecs))
	{
		WeakReference<dynamic> safeThis(this);

		postPrepareCheckActive = true;

		parentNode->getRootNetwork()->addPostInitFunction([safeThis]()
		{
			if (safeThis == nullptr)
				return true;

			auto pn = safeThis->parentNode;
			auto& exp = pn->getRootNetwork()->getExceptionHandler();

			try
			{
				exp.removeError(pn);
				DspHelpers::validate(safeThis->sendSpecs, safeThis->receiveSpecs);
				safeThis->postPrepareCheckActive = false;
				return true;
			}
			catch (Error& e)
			{
				auto pn = safeThis->parentNode;
				exp.addError(pn, e);
				return false;
			}

			return true;
		});
	}
}

dynamic::dynamic() :
	receiveIds(PropertyIds::Connection, "")
{

}

void dynamic::reset()
{
	for (auto& d : frameData)
		d = 0.0f;

	for (auto& v : buffer)
		v = 0.0f;
}

void dynamic::validate(PrepareSpecs receiveSpecs_)
{
	receiveSpecs = receiveSpecs_;

	checkSourceAndTargetProcessSpecs();
}

void dynamic::initialise(NodeBase* n)
{
	parentNode = n;

	receiveIds.initialise(n);
	receiveIds.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::restoreConnections), true);
}

void dynamic::connect(routing::receive<cable::dynamic>& receiveTarget)
{
	setConnection(receiveTarget, true);
}

template <typename T> static void callForEach(Component* root, const std::function<void(T*)>& f)
{
	if (auto typed = dynamic_cast<T*>(root))
	{
		f(typed);
	}

	for (int i = 0; i < root->getNumChildComponents(); i++)
	{
		callForEach(root->getChildComponent(i), f);
	}
}

dynamic::editor::editor(routing::base* b, PooledUIUpdater* u) :
	ScriptnodeExtraComponent<routing::base>(b, u),
	levelDisplay(0.0f, 0.0f, VuMeter::StereoHorizontal)
{
	addAndMakeVisible(levelDisplay);
	levelDisplay.setInterceptsMouseClicks(false, false);
	levelDisplay.setForceLinear(true);
	levelDisplay.setColour(VuMeter::backgroundColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xff383838)));
	levelDisplay.setColour(VuMeter::ColourId::ledColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xFFAAAAAA)));

	setSize(100, 18);

	setMouseCursor(ModulationSourceBaseComponent::createMouseCursor());

	start();

	updatePeakMeter();
}

void dynamic::editor::resized()
{


	bool isSend = getAsSendNode() != nullptr;

	auto b = getLocalBounds();

	b.removeFromRight(15);
	b.removeFromLeft(15);

	levelDisplay.setBounds(b.reduced(1));

	float deltaY = JUCE_LIVE_CONSTANT_OFF(-11.5f);
	float deltaXS = JUCE_LIVE_CONSTANT_OFF(41.0f);
	float deltaXE = JUCE_LIVE_CONSTANT_OFF(-41.0f);

	b = getLocalBounds();

	auto iconBounds = isSend ? b.removeFromRight(getHeight()) : b.removeFromLeft(getHeight());

	icon.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));

	PathFactory::scalePath(icon, iconBounds.toFloat().reduced(2.0f));

	getProperties().set("circleOffsetX", isSend ? deltaXS : deltaXE);
	getProperties().set("circleOffsetY", deltaY);
}

Error dynamic::editor::checkConnectionWhileDragging(const SourceDetails& dragSourceDetails)
{
	try
	{
		PrepareSpecs sp, rp;

		auto other = dynamic_cast<editor*>(dragSourceDetails.sourceComponent.get());

		if (auto rn = other->getAsReceiveNode())
		{
			if (auto sn = getAsSendNode())
			{
				sp = sn->cable.sendSpecs;
				rp = sn->cable.receiveSpecs;
			}
		}
		if (auto rn = getAsReceiveNode())
		{
			if (auto sn = other->getAsSendNode())
			{
				sp = sn->cable.sendSpecs;
				rp = sn->cable.receiveSpecs;
			}
		}

		DspHelpers::validate(sp, rp);
	}
	catch (Error& e)
	{
		return e;
	}

	return Error();
}

bool dynamic::editor::isValidDragTarget(editor* other)
{
	if (other == this)
		return false;

	auto srcIsSend = other->getAsSendNode() != nullptr;
	auto thisIsSend = getAsSendNode() != nullptr;

	return srcIsSend != thisIsSend;
}

bool dynamic::editor::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	if (auto src = dynamic_cast<editor*>(dragSourceDetails.sourceComponent.get()))
	{
		return isValidDragTarget(src);
	}

	return false;
}

void dynamic::editor::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	dragOver = true;
	currentDragError = checkConnectionWhileDragging(dragSourceDetails);
	repaint();
}

void dynamic::editor::itemDragExit(const SourceDetails& dragSourceDetails)
{
	dragOver = false;
	repaint();
}


void dynamic::editor::paintOverChildren(Graphics& g)
{
	if (dragMode)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(.1f));
		g.fillRoundedRectangle(getLocalBounds().toFloat(), getHeight() / 2);
	}

	if (isMouseOver(true))
	{
		if (auto rn = getAsReceiveNode())
		{
			if (rn->isConnected())
			{
				g.setColour(Colours::red.withAlpha(0.2f));
				g.fillRoundedRectangle(getLocalBounds().toFloat(), getHeight() / 2);
			}
		}
	}
}

void dynamic::editor::timerCallback()
{
	cable::dynamic* c = nullptr;
	float feedbackValue = 1.0f;

	if (auto sn = getAsSendNode())
		c = &sn->cable;

	if (auto rn = getAsReceiveNode())
	{
		c = rn->source;
		feedbackValue = rn->feedback;
	}

	if (c == nullptr)
	{
		levelDisplay.setPeak(0.0f, 0.0f);
		return;
	}

	int numChannels = c->getNumChannels();


	if (c->useFrameDataForDisplay)
	{
		auto l = c->frameData[0];
		auto r = numChannels == 2 ? c->frameData[1] : l;

		levelDisplay.setPeak(l * feedbackValue, r * feedbackValue);
	}
	else
	{
		int numSamples = c->channels[0].size();

		float l = DspHelpers::findPeak(c->channels[0].begin(), numSamples);
		float r = numChannels == 2 ? DspHelpers::findPeak(c->channels[1].begin(), numSamples) : l;

		levelDisplay.setPeak(l * feedbackValue, r * feedbackValue);
	}
}

juce::DragAndDropContainer* dynamic::editor::getDragAndDropContainer()
{
	auto c = findParentComponentOfClass<NodeComponent>();
	DragAndDropContainer* dd = nullptr;

	while (c != nullptr)
	{
		c = c->findParentComponentOfClass<NodeComponent>();

		if (auto thisDD = dynamic_cast<DragAndDropContainer*>(c))
			dd = thisDD;
	}

	return dd;
}

juce::Image dynamic::editor::createDragImage(const String& m, Colour bgColour)
{
	Path p;

	float margin = 10.0f;

	p.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));
	p.scaleToFit(5.0f, 5.0f, 15.0f, 15.0f, true);

	MarkdownRenderer mp(m, nullptr);

	mp.getStyleData().fontSize = 13.0f;

	mp.parse();
	auto height = (int)mp.getHeightForWidth(200.0f, true);

	Rectangle<float> b(0.0f, 0.0f, 240.0f, (float)height + 2 * margin);

	Image img(Image::ARGB, 240, height + 2 * margin, true);

	Graphics g(img);
	g.setColour(bgColour);
	g.fillRoundedRectangle(b, 3.0f);
	g.setColour(Colours::white);
	g.setFont(GLOBAL_BOLD_FONT());

	g.fillPath(p);

	mp.draw(g, b.reduced(margin));

	return img;
}




scriptnode::cable::dynamic::dynamic_send* dynamic::editor::getAsSendNode()
{
	if (auto c = dynamic_cast<dynamic_send*>(getObject()))
	{
		return c;
	}

	return nullptr;
}

scriptnode::cable::dynamic::dynamic_receive* dynamic::editor::getAsReceiveNode()
{
	if (auto c = dynamic_cast<dynamic_receive*>(getObject()))
	{
		return c;
	}

	return nullptr;
}

void dynamic::editor::itemDropped(const SourceDetails& dragSourceDetails)
{
	auto src = dynamic_cast<editor*>(dragSourceDetails.sourceComponent.get());

	jassert(src != nullptr);



	if (auto thisAsCable = getAsSendNode())
	{
		if (auto srcAsReceive = src->getAsReceiveNode())
			thisAsCable->connect(*srcAsReceive);
	}
	if (auto thisAsReceive = getAsReceiveNode())
	{
		if (auto srcAsSend = src->getAsSendNode())
			srcAsSend->connect(*thisAsReceive);
	}

	dynamic_cast<Component*>(getDragAndDropContainer())->repaint();
	dragOver = false;

	src->updatePeakMeter();
	updatePeakMeter();
}

void dynamic::editor::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		if (auto rn = getAsReceiveNode())
		{
			if (rn->isConnected())
			{
				rn->source->setConnection(*rn, false);
				findParentComponentOfClass<DspNetworkGraph>()->repaint();
			}
		}
	}
	else
	{
		auto dd = getDragAndDropContainer();
		dd->startDragging(var(), this, ScaledImage(ModulationSourceBaseComponent::createDragImageStatic(false)));

		findParentComponentOfClass<DspNetworkGraph>()->repaint();

		auto f = [this](editor* fc)
		{
			if (fc->isValidDragTarget(this))
			{
				fc->dragMode = true;
				fc->repaint();
			}
		};

		auto root = dynamic_cast<Component*>(getDragAndDropContainer());
		callForEach<editor>(root, f);
	}
}

void dynamic::editor::mouseUp(const MouseEvent& e)
{
	auto root = dynamic_cast<Component*>(getDragAndDropContainer());

	callForEach<editor>(root, [](editor* fc)
		{
			fc->dragMode = false;
			fc->repaint();
		});

	findParentComponentOfClass<DspNetworkGraph>()->repaint();
}

void dynamic::editor::mouseDrag(const MouseEvent& event)
{
	findParentComponentOfClass<DspNetworkGraph>()->repaint();
}



void dynamic::editor::mouseDoubleClick(const MouseEvent& event)
{
	if (auto rn = getAsReceiveNode())
	{
		if (rn->isConnected())
		{
			rn->source->setConnection(*rn, false);
			findParentComponentOfClass<DspNetworkGraph>()->repaint();
		}
	}

	updatePeakMeter();
}

bool dynamic::editor::isConnected()
{
	if (auto rn = getAsReceiveNode())
		return rn->isConnected();

	if (auto sn = getAsSendNode())
		return sn->cable.receiveIds.getValue().isNotEmpty();

	return false;
}

void dynamic::editor::updatePeakMeter()
{
	levelDisplay.setVisible(isConnected());
	repaint();
}

void dynamic::editor::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), getHeight() / 2, 1.0f);

	g.fillPath(icon);

	if (!levelDisplay.isVisible())
	{
		String s = "Drag to ";

		if (getAsSendNode() != nullptr)
			s << "receive";
		else
			s << "send";

		g.setFont(GLOBAL_BOLD_FONT().withHeight(12.0f));
		g.drawText(s, levelDisplay.getBoundsInParent().toFloat(), Justification::centred);
	}

	if (dragOver)
	{
		auto c = !currentDragError.isOk() ? Colours::red : Colour(SIGNAL_COLOUR);
		g.setColour(c);
		g.drawRect(getLocalBounds().toFloat(), 1.0f);
	}
}

}



}
