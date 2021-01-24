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
using MatrixEditor = NoExtraComponent;
#endif



Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	registerNode<matrix<dynamic_matrix>, MatrixEditor>();

	registerNode<dynamic_send, FunkySendComponent>();
	registerNode<dynamic_receive, FunkySendComponent>();
	registerNode<ms_encode>();
	registerNode<ms_decode>();
}

}

snex::NamespacedIdentifier cable::dynamic::getReceiveId()
{
	return NamespacedIdentifier("routing").getChildId(dynamic_receive::getStaticId());
}

void cable::dynamic::prepare(PrepareSpecs ps)
{
	currentSpecs = ps;

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

		for (int i = 0; i < ps.numChannels; i++)
		{
			channels[i].referToRawData(ptr, (size_t)ps.blockSize);
			ptr += ps.blockSize;
		}
	}

	if (parentNode != nullptr)
	{
		auto pn = parentNode->getRootNetwork();

		auto ids = StringArray::fromTokens(receiveIds.getValue(), ";", "");
		ids.removeDuplicates(false);
		ids.removeEmptyStrings(true);

		auto network = parentNode->getRootNetwork();

		auto id = NamespacedIdentifier("routing").getChildId(dynamic_receive::getStaticId());

		auto list = network->getListOfNodesWithPath(id, false);

		for (auto n : list)
		{
			if (auto rn = dynamic_cast<InterpretedNode*>(n.get()))
			{
				auto& ro = rn->getWrappedObject();

				if (ids.contains(rn->getId()))
				{
					validate(ro.as<dynamic_receive>().currentSpecs);
				}
			}
		}
	}
}

void cable::dynamic::restoreConnections(Identifier id, var newValue)
{
	WeakReference<dynamic> safePtr(this);

	auto f = [safePtr, id, newValue]()
	{
		if (safePtr.get() == nullptr)
			return;

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
					}
					else
					{
						if (source == safePtr.get())
							source = &(ro.as<dynamic_receive>().null);
					}

					safePtr.get()->connect(ro.as<dynamic_receive>());
				}
			}
		}
	};

	MessageManager::callAsync(f);
}

void cable::dynamic::setConnection(dynamic_receive& receiveTarget, bool addAsConnection)
{
	receiveTarget.source = addAsConnection ? this : &receiveTarget.null;

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

FunkySendComponent::FunkySendComponent(routing::base* b, PooledUIUpdater* u) :
	ScriptnodeExtraComponent<routing::base>(b, u),
	levelDisplay(0.0f, 0.0f, VuMeter::StereoHorizontal)
{
	addAndMakeVisible(levelDisplay);
	levelDisplay.setInterceptsMouseClicks(false, false);

	levelDisplay.setForceLinear(true);

	levelDisplay.setColour(VuMeter::backgroundColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xff383838)));
	levelDisplay.setColour(VuMeter::ColourId::ledColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xFFAAAAAA)));

	setSize(50, 18);
	start();
}

Error FunkySendComponent::checkConnectionWhileDragging(const SourceDetails& dragSourceDetails)
{
	try
	{
		PrepareSpecs sp, rp;

		auto other = dynamic_cast<FunkySendComponent*>(dragSourceDetails.sourceComponent.get());

		if (auto rn = other->getAsReceiveNode())
		{
			if (auto sn = getAsSendNode())
			{
				sp = sn->cable.currentSpecs;
				rp = rn->currentSpecs;
			}
		}
		if (auto rn = getAsReceiveNode())
		{
			if (auto sn = other->getAsSendNode())
			{
				sp = sn->cable.currentSpecs;
				rp = rn->currentSpecs;
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

void FunkySendComponent::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	dragOver = true;

	currentDragError = checkConnectionWhileDragging(dragSourceDetails);
	
	if (currentDragError.error != Error::OK)
	{
		auto dd = getDragAndDropContainer();

		dd->setCurrentDragImage(createDragImage(ScriptnodeExceptionHandler::getErrorMessage(currentDragError), Colours::red.withAlpha(0.7f)));
	}

	repaint();
}

void FunkySendComponent::itemDragExit(const SourceDetails& dragSourceDetails)
{
	getDragAndDropContainer()->setCurrentDragImage(createDragImage("Drag to connect", Colours::transparentBlack));

	dragOver = false;
	repaint();
}


void FunkySendComponent::paintOverChildren(Graphics& g)
{
	if (dragMode)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(.2f));
		g.fillAll();

		Path p;

		p.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));
		p.scaleToFit(2.0f, 2.0f, (float)getHeight()-4.0f, (float)getHeight()-4.0f, true);

		g.setColour(Colours::white);
		g.fillPath(p);
	}

	if (isMouseOver(true))
	{
		if (auto rn = getAsReceiveNode())
		{
			if (rn->isConnected())
			{
				g.setColour(Colours::red.withAlpha(0.2f));
				g.fillAll();
			}
		}
	}
}

void FunkySendComponent::timerCallback()
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

juce::DragAndDropContainer* FunkySendComponent::getDragAndDropContainer()
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

juce::Image FunkySendComponent::createDragImage(const String& m, Colour bgColour)
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


void FunkySendComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
	auto src = dynamic_cast<FunkySendComponent*>(dragSourceDetails.sourceComponent.get());

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
	repaint();
}

void FunkySendComponent::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		if (auto rn = getAsReceiveNode())
		{
			if (rn->isConnected())
			{
				rn->source->setConnection(*rn, false);

				dynamic_cast<Component*>(getDragAndDropContainer())->repaint();
			}
		}
	}
	else
	{
		auto dd = getDragAndDropContainer();

		auto img = createDragImage("Drag to connect", Colours::transparentBlack);

		Point<int> offset(-15, -13);

		dd->startDragging(var(), this, img, false, &offset);

		auto f = [this](FunkySendComponent* fc)
		{
			if (fc->isValidDragTarget(this))
			{
				fc->dragMode = true;
				fc->repaint();
			}
		};

		auto root = dynamic_cast<Component*>(getDragAndDropContainer());
		callForEach<FunkySendComponent>(root, f);
	}
}

void FunkySendComponent::mouseUp(const MouseEvent& e)
{
	auto root = dynamic_cast<Component*>(getDragAndDropContainer());

	callForEach<FunkySendComponent>(root, [](FunkySendComponent* fc)
	{
		fc->dragMode = false;
		fc->repaint();
	});
}

void FunkySendComponent::paint(Graphics& g)
{
	if (dragOver)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(getLocalBounds().toFloat(), 1.0f);
	}
}

}