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

namespace hise { using namespace juce;

void SimpleSampleMapDisplay::paint(Graphics& g)
{
	g.setColour(Colours::black.withAlpha(0.5f));
	g.drawRect(mapBounds.expanded(3.0f));
	g.fillRect(mapBounds.expanded(3.0f));
	g.setColour(Colours::white.withAlpha(0.5f));

	g.fillPath(p);
}

void SimpleSampleMapDisplay::mouseDoubleClick(const MouseEvent& e)
{
	if (currentBuffer != nullptr)
		currentBuffer->fromBase64String({});
}

void SimpleSampleMapDisplay::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
{
	if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
	{
		if (currentBuffer != nullptr)
			rebuildMap();
	}
}

void SimpleSampleMapDisplay::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	if (currentBuffer != nullptr)
		currentBuffer->getUpdater().removeEventListener(this);

	if ((currentBuffer = dynamic_cast<MultiChannelAudioBuffer*>(newData)))
		currentBuffer->getUpdater().addEventListener(this);
}

void SimpleSampleMapDisplay::rebuildMap()
{
	auto l = currentBuffer->getXYZItems();

	float x_offset = mapBounds.getX();
	float y_offset = mapBounds.getY();
	float w = mapBounds.getWidth() / 128.0;
	float h = mapBounds.getHeight() / 128.0;

	p.clear();

	for (auto item : l)
	{
		Rectangle<float> a(x_offset + item.keyRange.getStart() * w,
			y_offset + mapBounds.getHeight() - item.veloRange.getEnd() * h,
			w * item.keyRange.getLength(),
			h * item.veloRange.getLength());

		p.addRoundedRectangle(a.reduced(1.0f), w * 0.3f);
	}

	SafeAsyncCall::repaint(this);
}

void SimpleSampleMapDisplay::resized()
{
	mapBounds = getLocalBounds().toFloat().reduced(3.0f);
	rebuildMap();
}

XYZSampleMapProvider::MonolithDataProvider::MonolithDataProvider(XYZSampleMapProvider* p, const ValueTree& sampleMap_) :
	parent(p),
	sampleMap(sampleMap_)
{
	auto pool = p->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
	auto id = sampleMap[SampleIds::ID].toString();

	hmToUse = pool->getMonolith(id);

	if (hmToUse == nullptr)
	{
		MonolithFileReference info(sampleMap_);

		info.addSampleDirectory(p->getMainController()->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Samples));

		auto files = info.getAllFiles();

		hmToUse = pool->loadMonolithicData(sampleMap, files);
	}
}

hise::MultiChannelAudioBuffer::SampleReference::Ptr XYZSampleMapProvider::MonolithDataProvider::loadFile(const String& referenceString)
{
	if (hmToUse != nullptr)
	{
		for (int i = 0; i < hmToUse->getNumSamplesInMonolith(); i++)
		{
			if (referenceString == hmToUse->getFileName(0, i))
			{
				auto lr = new MultiChannelAudioBuffer::SampleReference();
				lr->sampleRate = hmToUse->getMonolithSampleRate(i);

				ScopedPointer<AudioFormatReader> afs = hmToUse->createUserInterfaceReader(i, 0);

				if (afs != nullptr)
				{
					auto s = sampleMap.getChild(i);

					Range<int> sampleRange((int)s[SampleIds::SampleStart], (int)s[SampleIds::SampleEnd]);

					if (sampleRange.isEmpty())
						sampleRange = Range<int>(0, (int)afs->lengthInSamples);

					lr->buffer.setSize(afs->numChannels, sampleRange.getLength());
					afs->read(&lr->buffer, 0, jmin(sampleRange.getLength(), (int)afs->lengthInSamples), sampleRange.getStart(), true, true);
				}


				return lr;
			}
		}
	}

	return new MultiChannelAudioBuffer::SampleReference(false, "not found");
}

XYZSampleMapProvider::FileBasedDataProvider::FileBasedDataProvider(XYZSampleMapProvider* p) :
	parent(p)
{
	afm.registerBasicFormats();
}

hise::MultiChannelAudioBuffer::SampleReference::Ptr XYZSampleMapProvider::FileBasedDataProvider::loadFile(const String& referenceString)
{
	return loadAbsoluteFile(parseFileReference(referenceString), referenceString);
}

File XYZSampleMapProvider::FileBasedDataProvider::parseFileReference(const String& b64) const
{
	PoolReference r(parent->getMainController(), b64, FileHandlerBase::Samples);
	return r.getFile();
}


XYZSampleMapProvider::XYZSampleMapProvider(MainController* mc) :
	XYZProviderBase(mc->getXYZPool()),
	ControlledObject(mc)
{

}

hise::ComplexDataUIBase::EditorBase* XYZSampleMapProvider::createEditor(MultiChannelAudioBuffer* b)
{
	return new Editor(this);
}

juce::Identifier XYZSampleMapProvider::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("SampleMap");
}

juce::Identifier XYZSampleMapProvider::getId() const
{
	return getStaticId();
}

hise::MultiChannelAudioBuffer::DataProvider* XYZSampleMapProvider::getDataProvider()
{
	return sampleMapDataProvider.get();
}

bool XYZSampleMapProvider::parseAdditionalProperties(const ValueTree& s, MultiChannelAudioBuffer::XYZItem& d)
{
	bool shouldRemoveFromPool = false;

	if (d.data != nullptr)
	{
		auto ns = d.data->buffer.getNumSamples();
		auto l = d.data->buffer.getWritePointer(0);
		
		auto r = l;

		if(d.data->buffer.getNumChannels() > 1)
			r = d.data->buffer.getWritePointer(1);

		auto lGain = 1.0f;
		auto rGain = 1.0f;

		if (auto normGain = s[SampleIds::NormalizedPeak])
		{
			lGain = normGain;
			rGain = normGain;
		}
		if (auto vol = (double)s[SampleIds::Volume])
		{
			auto gainFactor = Decibels::decibelsToGain(vol);
			lGain *= gainFactor;
			rGain *= gainFactor;
		}
		if (auto pan = (float)s[SampleIds::Pan])
		{
			lGain *= BalanceCalculator::getGainFactorForBalance(pan, true);
			rGain *= BalanceCalculator::getGainFactorForBalance(pan, false);
		}

		if (lGain != 1.0f || rGain != 1.0f)
		{
			FloatVectorOperations::multiply(l, lGain, ns);
			FloatVectorOperations::multiply(r, rGain, ns);

			shouldRemoveFromPool = true;
		}

		if (auto ct = (float)s[SampleIds::Pitch])
		{
			d.root -= ct / 100.0;
		}

		Range<int> sampleRange = Range<int>(s[SampleIds::SampleStart], s[SampleIds::SampleEnd]);
		Range<int> loopRange = Range<int>(s[SampleIds::LoopStart], s[SampleIds::LoopEnd]);

		loopRange = loopRange.getIntersectionWith(sampleRange);

		Range<int> wholeRange = Range<int>(0, ns);

		if (!sampleRange.isEmpty() && sampleRange != wholeRange)
		{
			AudioSampleBuffer other(2, sampleRange.getLength());

			FloatVectorOperations::copy(other.getWritePointer(0), d.data->buffer.getWritePointer(0, sampleRange.getStart()), other.getNumSamples());
			FloatVectorOperations::copy(other.getWritePointer(1), d.data->buffer.getWritePointer(1, sampleRange.getStart()), other.getNumSamples());

			std::swap(d.data->buffer, other);

			loopRange -= sampleRange.getStart();

			shouldRemoveFromPool = true;
		}

		if (s[SampleIds::LoopEnabled] && !loopRange.isEmpty())
		{
			d.data->loopRange = loopRange;

			if (auto xFade = (int)s[SampleIds::LoopXFade])
			{
				AudioSampleBuffer loopBuffer(2, xFade);

				auto offset = loopRange.getStart() - xFade;

				loopBuffer.copyFrom(0, 0, d.data->buffer, 0, offset, xFade);
				loopBuffer.copyFrom(1, 0, d.data->buffer, 1, offset, xFade);

				loopBuffer.applyGainRamp(0, xFade, 0.0f, 1.0f);

				auto dstOffset = loopRange.getEnd() - xFade;

				d.data->buffer.applyGainRamp(dstOffset, xFade, 1.0f, 0.0f);

				d.data->buffer.addFrom(0, dstOffset, loopBuffer, 0, 0, xFade);
				d.data->buffer.addFrom(1, dstOffset, loopBuffer, 1, 0, xFade);

				shouldRemoveFromPool = true;
			}
		}
	}

	return shouldRemoveFromPool;
}

void XYZSampleMapProvider::parseValueTree(const ValueTree& v, MultiChannelAudioBuffer::XYZItem::List& list)
{
	auto isMonolith = (int)v["SaveMode"] == SampleMap::SaveMode::Monolith;

	if (isMonolith)
		sampleMapDataProvider = new MonolithDataProvider(this, v);
	else
		sampleMapDataProvider = new FileBasedDataProvider(this);

	for (const auto& s : v)
	{
		auto d = hise::StreamingHelpers::getBasicMappingDataFromSample(s);
		MultiChannelAudioBuffer::XYZItem mi;

		mi.keyRange = { d.lowKey, d.highKey + 1 };
		mi.veloRange = { d.lowVelocity, d.highVelocity + 1 };
		mi.root = d.rootNote;
		mi.rrGroup = (int)s[SampleIds::RRGroup];
		auto fileName = s[SampleIds::FileName].toString();

		// just use the first multimic index
		if (fileName.isEmpty())
			fileName = s.getChild(0)[SampleIds::FileName].toString();

		mi.data = loadFileFromReference(fileName);

		if (parseAdditionalProperties(s, mi))
			removeFromPool(mi.data);

		list.add(std::move(mi));
	}
}

bool XYZSampleMapProvider::parse(const String& v, MultiChannelAudioBuffer::XYZItem::List& list)
{
	auto refString = v.fromFirstOccurrenceOf(getWildcard(), false, false);

	PoolReference r(getMainController(), refString, FileHandlerBase::SampleMaps);

	SampleMapPool::ManagedPtr p;

	if (auto e = getMainController()->getExpansionHandler().getExpansionForWildcardReference(v))
		p = e->pool->getSampleMapPool().loadFromReference(r, PoolHelpers::LoadAndCacheWeak);
	else
		p = getMainController()->getActiveFileHandler()->pool->getSampleMapPool().loadFromReference(r, PoolHelpers::LoadAndCacheWeak);

	if (p.get() != nullptr)
	{
        try
        {
            parseValueTree(p->data, list);
            return true;
        }
        catch(Result& r)
        {
            DBG(r.getErrorMessage());
			ignoreUnused(r);
        }
	}

	return false;
}

XYZSampleMapProvider::Editor::Editor(XYZSampleMapProvider* p) :
	provider(p)
{
	if (provider != nullptr)
		cb.addItemList(provider->getMainController()->getActiveFileHandler()->pool->getSampleMapPool().getIdList(), 1);

	addAndMakeVisible(simpleMapDisplay);

	cb.addListener(this);
	cb.setColour(ComboBox::ColourIds::textColourId, Colour(0xFFAAAAAA));
	addAndMakeVisible(cb);
}

void XYZSampleMapProvider::Editor::comboBoxChanged(ComboBox* c)
{
	String sId = provider->getWildcard();
	sId << c->getText();
	currentBuffer->fromBase64String(sId);
}

void XYZSampleMapProvider::Editor::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	simpleMapDisplay.setComplexDataUIBase(newData);

	if (currentBuffer != nullptr)
		currentBuffer->getUpdater().removeEventListener(this);

	if ((currentBuffer = dynamic_cast<MultiChannelAudioBuffer*>(newData)))
		currentBuffer->getUpdater().addEventListener(this);

	updateComboBoxItem();
}

void XYZSampleMapProvider::Editor::updateComboBoxItem()
{
	if (currentBuffer != nullptr)
	{
		auto currentData = currentBuffer->toBase64String();

		if (provider != nullptr)
		{
			currentData = currentData.fromFirstOccurrenceOf(provider->getWildcard(), false, false);

			if (currentData.isNotEmpty())
			{
				auto t = currentData;

				SafeAsyncCall::call<Component>(*dynamic_cast<Component*>(&cb), [t](Component& c)
				{
					dynamic_cast<ComboBox*>(&c)->setText(t, dontSendNotification);
				});
			}
		}
	}
}

void XYZSampleMapProvider::Editor::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
{
	if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
		updateComboBoxItem();
}

void XYZSampleMapProvider::Editor::resized()
{
	auto b = getLocalBounds();

	auto bottom = b.removeFromBottom(28);

	bottom.removeFromRight(10);
	cb.setBounds(bottom);
	b.removeFromBottom(10);
	b.removeFromTop(10);
	simpleMapDisplay.setBounds(b);
}

XYZSFZProvider::SFZFileLoader::SFZFileLoader(const File& sfzFile_) :
	sfzFile(sfzFile_)
{
	afm.registerBasicFormats();
}

hise::MultiChannelAudioBuffer::SampleReference::Ptr XYZSFZProvider::SFZFileLoader::loadFile(const String& referenceString)
{
	auto f = sfzFile.getParentDirectory().getChildFile(referenceString);
	return loadAbsoluteFile(f, referenceString);
}

XYZSFZProvider::Editor::DropTarget::DropTarget()
{
	setRepaintsOnMouseActivity(true);
}

void XYZSFZProvider::Editor::DropTarget::paint(Graphics& g)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, getLocalBounds().toFloat(), true);

	float alpha = 0.6f;

	if (hover)
		alpha += 0.2f;

	if (isMouseOver())
		alpha += 0.1f;

	g.setColour(Colours::white.withAlpha(alpha));
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText("Drop SFZ file or right click to open browser", getLocalBounds().toFloat(), Justification::centred);
}

bool XYZSFZProvider::Editor::DropTarget::isInterestedInFileDrag(const StringArray& files)
{
	return files.size() == 1 && File(files[0]).hasFileExtension("sfz");
}

void XYZSFZProvider::Editor::DropTarget::fileDragEnter(const StringArray& files, int x, int y)
{
	hover = true;
	repaint();
}

void XYZSFZProvider::Editor::DropTarget::fileDragExit(const StringArray& files)
{
	hover = false;
	repaint();
}

void XYZSFZProvider::Editor::DropTarget::filesDropped(const StringArray& files, int, int)
{
	findParentComponentOfClass<Editor>()->loadFile(File(files[0]));
}

void XYZSFZProvider::Editor::DropTarget::mouseDown(const MouseEvent& e)
{
	FileChooser fc("Load SFZ file", File(), "*.sfz");

	if (fc.browseForFileToOpen())
		findParentComponentOfClass<Editor>()->loadFile(fc.getResult());
}

XYZSFZProvider::Editor::Editor(XYZSFZProvider* p) :
	provider(p)
{
	addAndMakeVisible(dropTarget);
	addAndMakeVisible(display);
}

void XYZSFZProvider::Editor::loadFile(File f)
{
	if (provider != nullptr)
	{
		String s = provider->getWildcard();
		s << f.getFullPathName();

		if (display.currentBuffer != nullptr)
			display.currentBuffer->fromBase64String(s);
	}
}

void XYZSFZProvider::Editor::setComplexDataUIBase(ComplexDataUIBase* newData)
{
	display.setComplexDataUIBase(newData);
}

void XYZSFZProvider::Editor::resized()
{
	auto b = getLocalBounds();
	dropTarget.setBounds(b.removeFromBottom(28));
	b.removeFromBottom(10);
	b.removeFromTop(10);
	display.setBounds(b);
}

XYZSFZProvider::XYZSFZProvider(MainController* mc) :
	XYZSampleMapProvider(mc)
{

}

hise::ComplexDataUIBase::EditorBase* XYZSFZProvider::createEditor(MultiChannelAudioBuffer* b)
{
	return new Editor(this);
}

juce::Identifier XYZSFZProvider::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("SFZ");
}

juce::Identifier XYZSFZProvider::getId() const
{
	return getStaticId();
}

bool XYZSFZProvider::parse(const String& v, MultiChannelAudioBuffer::XYZItem::List& list)
{
	auto fileName = v.fromFirstOccurrenceOf(getWildcard(), false, false);

	if (File::isAbsolutePath(fileName))
	{
		File sfzFile(fileName);

		l = new SFZFileLoader(sfzFile);

		SfzImporter simp(nullptr, sfzFile);
		auto v = simp.importSfzFile();

		parseValueTree(v, list);
		return true;

	}

	return false;
}

hise::MultiChannelAudioBuffer::DataProvider* XYZSFZProvider::getDataProvider()
{
	return l;
}

} // namespace hise
