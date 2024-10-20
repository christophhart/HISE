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

namespace data
{
namespace pimpl
{

using namespace snex;

dynamic_base::dynamic_base(data::base& b, ExternalData::DataType t, int index_) :
	dt(t),
	index(index_),
	dataNode(&b)
{

}

dynamic_base::~dynamic_base()
{
	if (forcedUpdateSource != nullptr)
		forcedUpdateSource->removeForcedUpdateListener(this);
}

void dynamic_base::initialise(NodeBase* p)
{
	parentNode = p;

	auto h = parentNode->getRootNetwork()->getExternalDataHolder();

	if ((forcedUpdateSource = dynamic_cast<ExternalDataHolderWithForcedUpdate*>(h)))
	{
		forcedUpdateSource->addForcedUpdateListener(this);
	}

	auto dataTree = parentNode->getValueTree().getOrCreateChildWithName(PropertyIds::ComplexData, parentNode->getUndoManager());
	auto dataName = ExternalData::getDataTypeName(dt);
	auto typeTree = dataTree.getOrCreateChildWithName(Identifier(dataName + "s"), parentNode->getUndoManager());

	if (typeTree.getNumChildren() <= index)
	{
		for (int i = 0; i <= index; i++)
		{
			ValueTree newChild(dataName);
			newChild.setProperty(PropertyIds::Index, -1, nullptr);
			newChild.setProperty(PropertyIds::EmbeddedData, -1, nullptr);
			typeTree.addChild(newChild, -1, parentNode->getUndoManager());
		}
	}

	cTree = typeTree.getChild(index);
	jassert(cTree.isValid());

	dataUpdater.setCallback(cTree, { PropertyIds::Index, PropertyIds::EmbeddedData }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(dynamic_base::updateData));

	getInternalData()->setGlobalUIUpdater(parentNode->getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
	getInternalData()->getUpdater().addEventListener(this);

	setIndex(getIndex(), true);
}

void dynamic_base::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var data)
{
	if (d == ComplexDataUIUpdaterBase::EventType::ContentChange ||
		d == ComplexDataUIUpdaterBase::EventType::ContentRedirected)
	{
		if (currentlyUsedData == getInternalData() && parentNode != nullptr)
		{
			auto s = getInternalData()->toBase64String();
			cTree.setProperty(PropertyIds::EmbeddedData, s, parentNode->getUndoManager());
		}

		updateExternalData();
	}
}

void dynamic_base::forceRebuild(ExternalData::DataType dt_, int index)
{
	auto indexToUse = getIndex();

	if(indexToUse != -1 && dt_ == dt)
		setIndex(indexToUse, false);
}

void dynamic_base::updateExternalData()
{
	if (currentlyUsedData != nullptr)
	{
		auto updater = parentNode != nullptr ? parentNode->getScriptProcessor()->getMainController_()->getGlobalUIUpdater() : nullptr;
		auto um = parentNode != nullptr ? parentNode->getScriptProcessor()->getMainController_()->getControlUndoManager() : nullptr;

		currentlyUsedData->setGlobalUIUpdater(updater);
		currentlyUsedData->setUndoManager(um);

		ExternalData ed(currentlyUsedData, 0);

		{
#if 0
			if (currentlyUsedData->getDataLock().writeAccessIsLocked())
			{
				if (dataNode->externalData.data == ed.data)
				{
					sourceWatcher.setNewSource(currentlyUsedData);
					return;
				}
			}
#endif

			SimpleReadWriteLock::ScopedWriteLock sl(currentlyUsedData->getDataLock());
			setExternalData(*dataNode, ed, index);
		}
		
		sourceWatcher.setNewSource(currentlyUsedData);
	}
}

void dynamic_base::updateData(Identifier id, var newValue)
{
	if (id == PropertyIds::Index)
	{
		setIndex((int)newValue, false);
	}
	if (id == PropertyIds::EmbeddedData)
	{
		auto b64 = newValue.toString();

		if (b64 == "-1")
			b64 = "";

		if (getIndex() == -1)
		{
			auto thisString = getInternalData()->toBase64String();
            
            if(thisString == "-1")
                thisString = "";

			if (thisString.compare(b64) != 0)
				getInternalData()->fromBase64String(b64);
		}
	}
	
	if (forcedUpdateSource != nullptr)
	{
		forcedUpdateSource->sendForceUpdateMessage(this, dt, getIndex());
	}
}

int dynamic_base::getNumDataObjects(ExternalData::DataType t) const
{
	return (int)(t == dt);
}

void dynamic_base::setExternalData(data::base& n, const ExternalData& b, int index)
{
	SimpleRingBuffer::ScopedPropertyCreator sps(b.obj);
	n.setExternalData(b, index);
}

void dynamic_base::setIndex(int index, bool forceUpdate)
{
	ComplexDataUIBase* newData = nullptr;

	if (index != -1 && parentNode != nullptr)
	{
		if (auto h = parentNode->getRootNetwork()->getExternalDataHolder())
			newData = h->getComplexBaseType(dt, index);
	}
		
	if (newData == nullptr)
		newData = getInternalData();
	
	if (currentlyUsedData != newData || forceUpdate)
	{
		if (currentlyUsedData != nullptr)
			currentlyUsedData->getUpdater().removeEventListener(this);

		currentlyUsedData = newData;

		if (currentlyUsedData != nullptr)
			currentlyUsedData->getUpdater().addEventListener(this);

		updateExternalData();
	}
}

}

void dynamic::sliderpack::initialise(NodeBase* p)
{
	dynamicT<hise::SliderPackData>::initialise(p);

	numParameterSyncer.setCallback(getValueTree(), { PropertyIds::NumParameters }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(sliderpack::updateNumParameters));
}

void dynamic::sliderpack::updateNumParameters(Identifier id, var newValue)
{
	if (auto sp = dynamic_cast<SliderPackData*>(currentlyUsedData))
	{
		sp->setNumSliders((int)newValue);
	}
}


void dynamic::audiofile::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var data)
{
	dynamicT<hise::MultiChannelAudioBuffer>::onComplexDataEvent(d, data);

	if (d != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
	{
		sourceHasChanged(nullptr, currentlyUsedData);
	}
}

void dynamic::audiofile::initialise(NodeBase* n)
{
	auto mc = n->getScriptProcessor()->getMainController_();

	auto fileProvider = new PooledAudioFileDataProvider(mc);

	internalData->setProvider(fileProvider);

	internalData->registerXYZProvider("SampleMap", 
		[mc]() { return static_cast<MultiChannelAudioBuffer::XYZProviderBase*>(new hise::XYZSampleMapProvider(mc)); });

	internalData->registerXYZProvider("SFZ",
		[mc]() { return static_cast<MultiChannelAudioBuffer::XYZProviderBase*>(new hise::XYZSFZProvider(mc)); });

	dynamicT<hise::MultiChannelAudioBuffer>::initialise(n);

	allowRangeChange = true;

	rangeSyncer.setCallback(getValueTree(), { PropertyIds::MinValue, PropertyIds::MaxValue }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(audiofile::updateRange));
}

void dynamic::audiofile::sourceHasChanged(ComplexDataUIBase* oldSource, ComplexDataUIBase* newSource)
{
	if (allowRangeChange)
	{
		if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(newSource))
		{
			auto r = af->getCurrentRange();
			getValueTree().setProperty(PropertyIds::MinValue, r.getStart(), newSource->getUndoManager());
			getValueTree().setProperty(PropertyIds::MaxValue, r.getEnd(), newSource->getUndoManager());
		}
	}
}

void dynamic::audiofile::updateRange(Identifier id, var newValue)
{
	if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(currentlyUsedData))
	{
		auto min = (int)getValueTree()[PropertyIds::MinValue];
		auto max = (int)getValueTree()[PropertyIds::MaxValue];

		Range<int> nr(min, max);

		if (!nr.isEmpty())
			af->setRange({ min, max });
	}
}


dynamic::displaybuffer::displaybuffer(data::base& t, int index /*= 0*/) :
	dynamicT<SimpleRingBuffer>(t, index)
{

}

void dynamic::displaybuffer::initialise(NodeBase* n)
{
	dynamicT<hise::SimpleRingBuffer>::initialise(n);

	propertyListener.setCallback(getValueTree(), getCurrentRingBuffer()->getIdentifiers(), valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(displaybuffer::updateProperty));
}

void dynamic::displaybuffer::updateProperty(Identifier id, const var& newValue)
{
	if(!newValue.isVoid())
		getCurrentRingBuffer()->setProperty(id, newValue);
}

namespace ui
{
namespace pimpl
{

	editor_base::editor_base(ObjectType* b, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<ObjectType>(b, updater)
	{
		b->sourceWatcher.addSourceListener(this);
	}

	editor_base::~editor_base()
	{
		if (getObject() != nullptr)
			getObject()->sourceWatcher.removeSourceListener(this);
	}

	void editor_base::showProperties(SimpleRingBuffer* obj, Component* c)
	{
		XmlElement xml("Funky");

		auto pObj = obj->getPropertyObject();

		DynamicObject::Ptr dynObj = new DynamicObject();

		for (auto& nv : pObj->properties)
			dynObj->setProperty(nv.first, nv.second);

		

		auto ed = new JSONEditor(var(dynObj.get()));

		ed->setSize(500, 400);
		ed->setEditable(true);

		ed->setCallback([pObj](const var& o)
		{
			if (auto d = o.getDynamicObject())
			{
				for (auto& nv : d->getProperties())
					pObj->setProperty(nv.name, nv.value);
			}
		});

		c->findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(ed, c, {}, false);
	}

	Colour editor_base::getColourFromNodeComponent(NodeComponent* nc)
	{
		return nc->header.colour;
	}

	void complex_ui_laf::drawTableBackground(Graphics& g, TableEditor& te, Rectangle<float> area, double rulerPosition)
	{
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, area, false);	
	}	

	void complex_ui_laf::drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness)
	{
		UnblurryGraphics ug(g, te, true);

		auto b = area;
		auto b2 = area;

		auto pc = p;

		for (int i = 0; i < 4; i++)
		{
			b.removeFromLeft(area.getWidth() / 4.0f);
			ug.draw1PxVerticalLine(b.getX(), area.getY(), area.getBottom());
			b2.removeFromTop(area.getHeight() / 4.0f);
			ug.draw1PxHorizontalLine(b2.getY(), area.getX(), area.getRight());
		}

		float alpha = 0.8f;

		if (te.isMouseOverOrDragging(true))
			alpha += 0.1f;

		if (te.isMouseButtonDown(true))
			alpha += 0.1f;

		auto c = getNodeColour(&te).withBrightness(1.0f).withAlpha(alpha);
		
		g.setColour(c);

		{
			Path destP;
			float l[2] = { 4.0f * ug.getPixelSize(), 4.0f * ug.getPixelSize() };

			PathStrokeType(2.0f * ug.getPixelSize()).createDashedStroke(destP, pc, l, 2);

			g.fillPath(destP);
		}

		//g.strokePath(p, PathStrokeType(2.0f * ug.getPixelSize()));
		
		

		c = c.withMultipliedAlpha(0.05f);
		g.setColour(c);
		//g.fillPath(p);

		te.setRepaintsOnMouseActivity(true);

		auto i = te.getLastIndex();

		

		if (auto e = dynamic_cast<SampleLookupTable*>(te.getEditedTable()))
		{
			b = area;
			b.removeFromLeft(i * area.getWidth());
			g.setColour(getNodeColour(&te).withBrightness(1.0f).withAlpha(0.9f));
			g.saveState();
			g.excludeClipRegion(b.toNearestInt());
			g.strokePath(p, PathStrokeType(3.0f * ug.getPixelSize()));
			g.restoreState();
		}

		auto a = te.getPointAreaBetweenMouse();

		if (!a.isEmpty())
		{
			auto fp = p;

			fp.lineTo(te.getLocalBounds().toFloat().getBottomRight());
			fp.closeSubPath();

			g.setColour(Colours::white.withAlpha(0.03f));
			auto l = te.getLocalBounds().removeFromLeft(a.getX());
			auto r = te.getLocalBounds();
			r = r.removeFromRight(r.getWidth() - a.getRight());
			g.excludeClipRegion(l);
			g.excludeClipRegion(r);
			g.fillPath(fp);
		}
	}

	void complex_ui_laf::drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged)
	{
		float brightness = 1.0f;

		if (!te.isMouseOverOrDragging(true))
		{
			brightness *= 0.9f;
		}

		if (!te.isMouseButtonDown(true))
			brightness *= 0.9f;

		UnblurryGraphics ug(g, te, true);

		auto c = getNodeColour(&te).withBrightness(brightness);

		g.setColour(c);

		auto s = jmin(tablePoint.getWidth(), isEdge ? 15.0f : 10.0f);

		auto dot = tablePoint.withSizeKeepingCentre(s, s);

		g.drawRoundedRectangle(dot, ug.getPixelSize() * 3.0f, ug.getPixelSize());

		if (isHover || isDragged)
		{
			g.setColour(c.withAlpha(0.7f));
			g.fillRoundedRectangle(dot, ug.getPixelSize() * 3.0f);
		}
	}

	void complex_ui_laf::drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition)
	{
		auto a = te.getLocalBounds().toFloat();

		Rectangle<float> ar(rulerPosition * a.getWidth() - 10.0f, a.getY(), 20.0f, a.getHeight());

		g.setColour(Colours::white.withAlpha(0.02f));
		g.fillRect(ar);

		UnblurryGraphics ug(g, te, true);

		g.setColour(Colours::white.withAlpha(0.7f));
		ug.draw1PxVerticalLine(ar.getCentreX(), a.getY(), a.getHeight());

#if 0
		auto c = getNodeColour(&te).withAlpha(0.8f);

		auto x = (float)rulerPosition * area.getWidth() + area.getX();

		g.setColour(c);

		

		if (auto e = dynamic_cast<SampleLookupTable*>(te.getEditedTable()))
		{
			auto yValue = e->getInterpolatedValue(te.getLastIndex(), dontSendNotification);

			Point<float> p(x, (1.0f - yValue) * area.getHeight());

			Rectangle<float> a(p, p);

			g.setColour(Colours::white);
			auto s = 3.0f;
			//g.fillRect(a.withSizeKeepingCentre(s, s));
		}
			


		g.setColour(c.withAlpha(0.4f));
		ug.draw1PxVerticalLine(x, area.getY(), area.getBottom());
#endif
	}

	void complex_ui_laf::drawTableValueLabel(Graphics& g, TableEditor& te, Font f, const String& text, Rectangle<int> textBox)
	{
		auto c = getNodeColour(&te);
		g.setColour(Colours::black.withAlpha(0.4f));
		g.fillRoundedRectangle(textBox.toFloat(), textBox.getHeight() / 2);
		g.setColour(c);
		g.setFont(f);
		g.drawText(text, textBox.toFloat(), Justification::centred);
	}

	void complex_ui_laf::drawFilterBackground(Graphics &g, FilterGraph& fg)
	{
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, fg.getLocalBounds().toFloat(), false);
	}

	void complex_ui_laf::drawFilterGridLines(Graphics &g, FilterGraph& fg, const Path& gridPath)
	{
		auto sf = UnblurryGraphics::getScaleFactorForComponent(&fg);

		g.setColour(Colours::white.withAlpha(0.05f));
		g.strokePath(gridPath, PathStrokeType(1.0f / sf));
	}

	void complex_ui_laf::drawFilterPath(Graphics& g, FilterGraph& fg, const Path& p)
	{
		auto c = getNodeColour(&fg).withAlpha(1.0f);

		auto b = fg.getLocalBounds().expanded(2);

		g.excludeClipRegion(b.removeFromLeft(4));
		g.excludeClipRegion(b.removeFromRight(4));
		g.excludeClipRegion(b.removeFromTop(4));
		g.excludeClipRegion(b.removeFromBottom(4));

		

		g.setColour(c.withAlpha(0.1f));
		g.fillPath(p);
		g.setColour(c);
		g.strokePath(p, PathStrokeType(1.5f));
	}

	void complex_ui_laf::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& s)
	{
		UnblurryGraphics ug(g, s, true);

		if (style == Slider::SliderStyle::LinearBarVertical)
		{
			float leftY;
			float actualHeight;

			const double value = s.getValue();
			const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
			const double proportion = pow(normalizedValue, s.getSkewFactor());

			//const float value = ((float)s.getValue() - min) / (max - min);

			auto h = ug.getPixelSize() * 6.0f;

			actualHeight = (float)proportion * (float)(height - h);

			leftY = (float)height - actualHeight;

			float alpha = 0.4f;

			auto p = s.findParentComponentOfClass<SliderPack>();

			auto sb = s.getBoundsInParent();

			auto over = sb.contains(p->getMouseXYRelative());

			if (over)
			{
				alpha += 0.05f;

				if (p->isMouseButtonDown(true))
					alpha += 0.05f;
			}

			Colour c = getNodeColour(&s).withBrightness(1.0f);
			g.setColour(c.withAlpha(1.0f));

			

			Rectangle<float> ar(0.0f, jmin(s.getHeight() - h, leftY), (float)(width + 1), h);

			g.fillRoundedRectangle(ar.reduced(3.0f, 0.0f), ar.getHeight() / 2.0f);

			Rectangle<float> ar2(0.0f, ar.getBottom(), ar.getWidth(), s.getHeight() - ar.getBottom());

			g.setColour(c.withAlpha(0.05f));
			g.fillRect(ar2.reduced(ar.getHeight(), 0.0f));

		}
	}

	void complex_ui_laf::drawSliderPackBackground(Graphics& g, SliderPack& s)
	{
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, s.getLocalBounds().toFloat(), false);

		UnblurryGraphics ug(g, s, true);

		auto w = (float)s.getWidth() / (float)s.getNumSliders();

		for (float x = -1.0f; x < (float)(s.getWidth()-2); x += w)
			ug.draw1PxVerticalLine(x, 0.0f, (float)s.getHeight());
	}

	void complex_ui_laf::drawSliderPackFlashOverlay(Graphics& g, SliderPack& s, int sliderIndex, Rectangle<int> sliderBounds, float intensity)
	{
		auto c = getNodeColour(&s).withAlpha(intensity);
		g.setColour(c);
		//g.fillRect(sliderBounds);

		g.setColour(Colours::white.withAlpha(intensity * 0.1f));
		g.fillRect(sliderBounds.withY(0).withHeight(s.getHeight()));
	}

	void complex_ui_laf::drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area)
	{
		UnblurryGraphics ug(g, th, true);

		g.setColour(Colours::black.withAlpha(0.3f));
		ug.fillUnblurryRect(area.toFloat());
		g.drawRect(area.toFloat(), 2.0f);
	}

	void complex_ui_laf::drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path)
	{
		
		auto c = getNodeColour(&th).withBrightness(1.0f);

		g.setColour(c.withAlpha(areaIsEnabled ? 0.3f : 0.1f));

		UnblurryGraphics ug(g, th, true);

		if (areaIsEnabled)
		{
			g.fillPath(path);
			g.setColour(c.withAlpha(areaIsEnabled ? 1.0f : 0.1f));
			g.strokePath(path, PathStrokeType(ug.getPixelSize()));
		}
		else
		{
			g.fillPath(path);
			float l[2] = { ug.getPixelSize() * 4.0f, ug.getPixelSize() * 4.0f };
			Path destP;
			g.setColour(c.withAlpha(0.4f));
			PathStrokeType(ug.getPixelSize() * 2.0f).createDashedStroke(destP, path, l, 2);
			g.fillPath(destP);
		}
	}

	void complex_ui_laf::drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area)
	{
		g.setColour(Colours::black.withAlpha(0.3f));

		auto sf = jmax(1.0f, UnblurryGraphics::getScaleFactorForComponent(&th, false));

		auto a = area.withSizeKeepingCentre(area.getWidth() / sf, (area.getHeight() + 5.0f) / sf);

		if (!text.startsWith("Drop"))
		{
			a.setX(th.getRight() - 10.0f - a.getWidth());
			a.setY(area.getY());
		}
		g.fillRoundedRectangle(a, a.getHeight() / 2.0f);

		g.setColour(getNodeColour(&th).withBrightness(1.0f));
		g.setFont(GLOBAL_BOLD_FONT().withHeight(14.0f / sf));
		g.drawText(text, a, Justification::centred);
	}

	void complex_ui_laf::drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
	{
		float br = 0.6f;

		if (shouldDrawButtonAsDown)
			br += 0.2f;
		if (shouldDrawButtonAsDown)
			br += 0.2f;

		auto c = getNodeColour(&b).withBrightness(br);

		g.setColour(c);
		g.setFont(GLOBAL_BOLD_FONT());

		auto ar = b.getLocalBounds().toFloat().reduced(3.0f);

		if (b.getToggleState())
		{
			g.fillRoundedRectangle(ar, ar.getHeight() / 2.0f);
			g.setColour(c.contrasting());
			g.drawText(b.getName(), ar, Justification::centred);
		}
		else
		{
			g.drawRoundedRectangle(ar, ar.getHeight() / 2.0f, 2.0f);
			g.drawText(b.getName(), ar, Justification::centred);
		}
	}

	void complex_ui_laf::drawButtonText(Graphics&, TextButton&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
	{

	}

	juce::Colour complex_ui_laf::getNodeColour(Component* comp)
	{
		if(!nodeColour.isTransparent())
			return nodeColour;

		auto c = Colour(0xFFDADADA);

		if (auto nc = comp->findParentComponentOfClass<NodeComponent>())
		{
			if (nc->header.colour != Colours::transparentBlack)
				c = nc->header.colour;
		}

		return c;
	}

	void complex_ui_laf::drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac, Rectangle<float> area)
	{
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, area, false);
	}

	void complex_ui_laf::drawOscilloscopePath(Graphics& g, RingBufferComponentBase& ac, const Path& p)
	{
		auto c = getNodeColour(dynamic_cast<Component*>(&ac));
		
		auto b = dynamic_cast<Component*>(&ac)->getLocalBounds().expanded(2);

		g.excludeClipRegion(b.removeFromLeft(4));
		g.excludeClipRegion(b.removeFromRight(4));
		g.excludeClipRegion(b.removeFromTop(4));
		g.excludeClipRegion(b.removeFromBottom(4));

		g.setColour(c.withAlpha(0.1f));
		g.fillPath(p);
		g.setColour(c);
		g.strokePath(p, PathStrokeType(1.0f));

	}

	void complex_ui_laf::drawGonioMeterDots(Graphics& g, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index)
	{
		auto c = getNodeColour(dynamic_cast<Component*>(&ac));

		const float alphas[6] = { 1.0f, 0.5f, 0.25f, 0.125f, 0.075f, 0.03f };

		g.setColour(c.withAlpha(alphas[index]));
		g.fillRectList(dots);
	}

	void complex_ui_laf::drawAnalyserGrid(Graphics& g, RingBufferComponentBase& ac, const Path& p)
	{
		auto sf = UnblurryGraphics::getScaleFactorForComponent(dynamic_cast<Component*>(&ac));

		g.setColour(Colours::white.withAlpha(0.05f));
		g.strokePath(p, PathStrokeType(1.0f / sf));
	}

	void complex_ui_laf::drawAhdsrBackground(Graphics& g, AhdsrGraph& graph)
	{
		drawOscilloscopeBackground(g, graph, graph.getLocalBounds().toFloat());
	}

	void complex_ui_laf::drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive)
	{
		if (!isActive)
		{
			drawOscilloscopePath(g, graph, s);
		}
		else
		{
			auto c = getNodeColour(dynamic_cast<Component*>(&graph)).withAlpha(0.05f);
			g.setColour(c);
			g.fillPath(s);
		}
	}

	void complex_ui_laf::drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> p)
	{
		g.setColour(getNodeColour(&graph).withAlpha(1.0f));
		Rectangle<float> a(p, p);
		g.fillEllipse(a.withSizeKeepingCentre(6.0f, 6.0f));
	}

	RingBufferPropertyEditor::Item::Item(dynamic::displaybuffer* b_, Identifier id_, const StringArray& entries, const String& value) :
		f(GLOBAL_FONT()),
		id(id_),
		db(b_)
	{
		addAndMakeVisible(cb);
		cb.addListener(this);
		cb.setLookAndFeel(&laf);
		cb.addItemList(entries, 1);
		cb.setText(value, dontSendNotification);
		cb.setColour(ComboBox::ColourIds::textColourId, Colour(0xFFDADADA));

		auto w = f.getStringWidthFloat(id.toString()) + 10.0f;

		setSize(w + 70, 28);
	}

	RingBufferPropertyEditor::RingBufferPropertyEditor(dynamic::displaybuffer* b, RingBufferComponentBase* e) :
		buffer(b),
		editor(e)
	{
		jassert(buffer != nullptr);
		jassert(editor != nullptr);

		if (auto rb = b->getCurrentRingBuffer())
		{
			for (auto& id : rb->getIdentifiers())
				addItem(id, { "1", "2" });
		}
	}

}

}
}

}

