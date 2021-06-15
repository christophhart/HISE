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

#pragma once

namespace hise { using namespace juce;

struct SimpleSampleMapDisplay : public ComplexDataUIBase::EditorBase,
	public ComplexDataUIUpdaterBase::EventListener,
	public Component
{
	void paint(Graphics& g) override
	{
		g.setColour(Colours::black.withAlpha(0.5f));
		g.drawRect(mapBounds.expanded(3.0f));
		g.fillRect(mapBounds.expanded(3.0f));
		g.setColour(Colours::white.withAlpha(0.5f));

		g.fillPath(p);
	}

	void mouseDoubleClick(const MouseEvent& e) override
	{
		if (currentBuffer != nullptr)
			currentBuffer->fromBase64String({});
	}

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override
	{
		if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
		{
			if (currentBuffer != nullptr)
				rebuildMap();
		}
	}

	void setComplexDataUIBase(ComplexDataUIBase* newData) override
	{
		if (currentBuffer != nullptr)
			currentBuffer->getUpdater().removeEventListener(this);

		if (currentBuffer = dynamic_cast<MultiChannelAudioBuffer*>(newData))
			currentBuffer->getUpdater().addEventListener(this);
	}

	void rebuildMap()
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

		repaint();
	}

	void resized() override
	{
		mapBounds = getLocalBounds().toFloat().reduced(3.0f);
		rebuildMap();
	}

	Rectangle<float> mapBounds;
	Path p;

	WeakReference<MultiChannelAudioBuffer> currentBuffer;
};

struct XYZSampleMapProvider : public MultiChannelAudioBuffer::XYZProviderBase,
	public ControlledObject
{
	struct MonolithDataProvider : public MultiChannelAudioBuffer::DataProvider
	{
		MonolithDataProvider(XYZSampleMapProvider* p, const ValueTree& sampleMap_) :
			parent(p),
			sampleMap(sampleMap_)
		{
			auto pool = p->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
			auto id = sampleMap[SampleIds::ID].toString();

			int numMicPositions = jmax(1, sampleMap.getChild(0).getNumChildren());

			hmToUse = pool->getMonolith(id);

			if (hmToUse == nullptr)
			{
				Array<File> files;
				auto sampleRoot = p->getMainController()->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Samples);

				for (int i = 0; i < numMicPositions; i++)
				{
					String extension = "ch";
					extension << String(i + 1);
					files.add(sampleRoot.getChildFile(id).withFileExtension(extension));
				}

				hmToUse = pool->loadMonolithicData(sampleMap, files);

				int numSamples = hmToUse->getNumSamplesInMonolith();
				realRanges.insertMultiple(0, {}, numSamples);

				for (int i = 0; i < numSamples; i++)
				{
					auto offset = hmToUse->getMonolithOffset(i);

					for (const auto& s : sampleMap)
					{
						if ((int)s["MonolithOffset"] == offset)
						{
							auto r = Range<int>(s[SampleIds::SampleStart], s[SampleIds::SampleEnd]);
							realRanges.set(i, r);
							break;
						}
					}
				}
			}
		};

		MultiChannelAudioBuffer::LoadResult::Ptr loadFile(const String& referenceString) override
		{
			if (hmToUse != nullptr)
			{
				for (int i = 0; i < hmToUse->getNumSamplesInMonolith(); i++)
				{
					if (referenceString == hmToUse->getFileName(0, i))
					{
						auto lr = new MultiChannelAudioBuffer::LoadResult();
						lr->sampleRate = hmToUse->getMonolithSampleRate(i);

						auto sampleRange = realRanges[i];

						ScopedPointer<AudioFormatReader> afs = hmToUse->createFallbackReader(i, 0);



						if (afs != nullptr)
						{
							if (sampleRange.isEmpty())
								sampleRange = Range<int>(0, afs->lengthInSamples);

							lr->buffer.setSize(afs->numChannels, sampleRange.getLength());
							afs->read(&lr->buffer, 0, jmin(sampleRange.getLength(), (int)afs->lengthInSamples), sampleRange.getStart(), true, true);
						}


						return lr;
					}
				}
			}

			return new MultiChannelAudioBuffer::LoadResult(false, "not found");
		}


		hise::HlacMonolithInfo::Ptr hmToUse;
		WeakReference<XYZSampleMapProvider> parent;
		ValueTree sampleMap;
		Array<Range<int>> realRanges;
	};

	struct FileBasedDataProvider : public MultiChannelAudioBuffer::DataProvider
	{
		FileBasedDataProvider(XYZSampleMapProvider* p) :
			parent(p)
		{
			afm.registerBasicFormats();
		};

		MultiChannelAudioBuffer::LoadResult::Ptr loadFile(const String& referenceString) override
		{
			PoolReference r(parent->getMainController(), referenceString, FileHandlerBase::Samples);
			return loadAbsoluteFile(r.getFile(), referenceString);
		}

		WeakReference<XYZSampleMapProvider> parent;
	};

	XYZSampleMapProvider(MainController* mc) :
		XYZProviderBase(mc->getXYZPool()),
		ControlledObject(mc)
	{

	}

	struct Editor : public ComplexDataUIBase::EditorBase,
		public Component,
		public ComplexDataUIUpdaterBase::EventListener,
		public ComboBox::Listener
	{
		Editor(XYZSampleMapProvider* p) :
			provider(p)
		{
			if (provider != nullptr)
				cb.addItemList(provider->getMainController()->getActiveFileHandler()->pool->getSampleMapPool().getIdList(), 1);

			addAndMakeVisible(simpleMapDisplay);

			cb.addListener(this);
			cb.setColour(ComboBox::ColourIds::textColourId, Colour(0xFFAAAAAA));
			addAndMakeVisible(cb);
		};

		void comboBoxChanged(ComboBox* c) override
		{
			String sId = provider->getWildcard();
			sId << c->getText();
			currentBuffer->fromBase64String(sId);
		}

		void setComplexDataUIBase(ComplexDataUIBase* newData) override
		{
			simpleMapDisplay.setComplexDataUIBase(newData);

			if (currentBuffer != nullptr)
				currentBuffer->getUpdater().removeEventListener(this);

			if (currentBuffer = dynamic_cast<MultiChannelAudioBuffer*>(newData))
				currentBuffer->getUpdater().addEventListener(this);

			updateComboBoxItem();
		}

		void updateComboBoxItem()
		{
			if (currentBuffer != nullptr)
			{
				auto currentData = currentBuffer->toBase64String();

				if (provider != nullptr)
				{
					currentData = currentData.fromFirstOccurrenceOf(provider->getWildcard(), false, false);

					if (currentData.isNotEmpty())
						cb.setText(currentData, dontSendNotification);
				}
			}
		}

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override
		{
			if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
				updateComboBoxItem();
		}

		void resized() override
		{
			auto b = getLocalBounds();

			auto bottom = b.removeFromBottom(28);

			bottom.removeFromRight(10);
			cb.setBounds(bottom);
			b.removeFromBottom(10);
			b.removeFromTop(10);
			simpleMapDisplay.setBounds(b);
		}

		SimpleSampleMapDisplay simpleMapDisplay;

		ComboBox cb;
		WeakReference<XYZSampleMapProvider> provider;
		WeakReference<MultiChannelAudioBuffer> currentBuffer;

		ScriptnodeComboBoxLookAndFeel claf;
	};

	ComplexDataUIBase::EditorBase* createEditor(MultiChannelAudioBuffer* b) override
	{
		return new Editor(this);
	}

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("SampleMap"); }

	Identifier getId() const override { return getStaticId(); }

	MultiChannelAudioBuffer::DataProvider* getDataProvider() { return sampleMapDataProvider; }

	bool parseAdditionalProperties(const ValueTree& s, MultiChannelAudioBuffer::XYZItem& d)
	{
		bool shouldRemoveFromPool = false;

		if (d.data != nullptr)
		{
			auto ns = d.data->buffer.getNumSamples();
			auto l = d.data->buffer.getWritePointer(0);
			auto r = d.data->buffer.getWritePointer(1);

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

	void parseValueTree(const ValueTree& v, MultiChannelAudioBuffer::XYZItem::List& list)
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

	bool parse(const String& v, MultiChannelAudioBuffer::XYZItem::List& list) override
	{
		auto refString = v.fromFirstOccurrenceOf(getWildcard(), false, false);

		PoolReference r(getMainController(), refString, FileHandlerBase::SampleMaps);

		SampleMapPool::ManagedPtr p;

		if (auto e = getMainController()->getExpansionHandler().getExpansionForWildcardReference(v))
			p = e->pool->getSampleMapPool().loadFromReference(r, PoolHelpers::DontCreateNewEntry);
		else
			p = getMainController()->getActiveFileHandler()->pool->getSampleMapPool().loadFromReference(r, PoolHelpers::DontCreateNewEntry);

		if (p.get() != nullptr)
		{
			parseValueTree(p->data, list);
			return true;
		}

		return false;
	}

	MultiChannelAudioBuffer::DataProvider::Ptr sampleMapDataProvider;

	JUCE_DECLARE_WEAK_REFERENCEABLE(XYZSampleMapProvider);
};

struct XYZSFZProvider : public XYZSampleMapProvider
{
	struct SFZFileLoader : public MultiChannelAudioBuffer::DataProvider
	{
		SFZFileLoader(const File& sfzFile_) :
			sfzFile(sfzFile_)
		{
			afm.registerBasicFormats();
		};

		MultiChannelAudioBuffer::LoadResult::Ptr loadFile(const String& referenceString) override
		{
			auto f = sfzFile.getParentDirectory().getChildFile(referenceString);
			return loadAbsoluteFile(f, referenceString);
		}

		File sfzFile;
	};

	XYZSFZProvider(MainController* mc) :
		XYZSampleMapProvider(mc)
	{

	};

	struct Editor : public Component,
		public ComplexDataUIBase::EditorBase
	{
		Editor(XYZSFZProvider* p) :
			provider(p)
		{
			addAndMakeVisible(dropTarget);
			addAndMakeVisible(display);
		}

		struct DropTarget : public Component,
			public juce::FileDragAndDropTarget
		{
			DropTarget()
			{
				setRepaintsOnMouseActivity(true);
			}

			void paint(Graphics& g) override
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

			bool isInterestedInFileDrag(const StringArray& files) override
			{
				return files.size() == 1 && File(files[0]).hasFileExtension("sfz");
			}

			void fileDragEnter(const StringArray& files, int x, int y) override
			{
				hover = true;
				repaint();
			}

			void fileDragExit(const StringArray& files) override
			{
				hover = false;
				repaint();
			}

			/** Callback to indicate that the user has dropped the files onto this component.

				When the user drops the files, this get called, and you can use the files in whatever
				way is appropriate.

				Note that after this is called, the fileDragExit method may not be called, so you should
				clean up in here if there's anything you need to do when the drag finishes.

				@param files        the set of (absolute) pathnames of the files that the user is dragging
				@param x            the mouse x position, relative to this component
				@param y            the mouse y position, relative to this component
			*/
			void filesDropped(const StringArray& files, int, int)
			{
				findParentComponentOfClass<Editor>()->loadFile(File(files[0]));
			}

			void mouseDown(const MouseEvent& e) override
			{
				FileChooser fc("Load SFZ file", File(), "*.sfz");

				if (fc.browseForFileToOpen())
					findParentComponentOfClass<Editor>()->loadFile(fc.getResult());
			}



			bool hover = false;
		};

		void loadFile(File f)
		{
			if (provider != nullptr)
			{
				String s = provider->getWildcard();
				s << f.getFullPathName();

				if (display.currentBuffer != nullptr)
					display.currentBuffer->fromBase64String(s);
			}
		}

		void setComplexDataUIBase(ComplexDataUIBase* newData) override
		{
			display.setComplexDataUIBase(newData);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			dropTarget.setBounds(b.removeFromBottom(28));
			b.removeFromBottom(10);
			b.removeFromTop(10);
			display.setBounds(b);
		}

		SimpleSampleMapDisplay display;
		DropTarget dropTarget;
		WeakReference<XYZSFZProvider> provider;
	};

	ComplexDataUIBase::EditorBase* createEditor(MultiChannelAudioBuffer* b) override
	{
		return new Editor(this);
	}

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("SFZ"); }

	Identifier getId() const override { return getStaticId(); }

	bool parse(const String& v, MultiChannelAudioBuffer::XYZItem::List& list) override
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

	MultiChannelAudioBuffer::DataProvider* getDataProvider() override { return l; };

	ScopedPointer<SFZFileLoader> l;
	JUCE_DECLARE_WEAK_REFERENCEABLE(XYZSFZProvider);
};


} // namespace hise
