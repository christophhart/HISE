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
	void paint(Graphics& g) override;
	void mouseDoubleClick(const MouseEvent& e) override;
	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;
	void setComplexDataUIBase(ComplexDataUIBase* newData) override;
	void rebuildMap();
	void resized() override;

	Rectangle<float> mapBounds;
	Path p;

	WeakReference<MultiChannelAudioBuffer> currentBuffer;
};

struct XYZSampleMapProvider : public MultiChannelAudioBuffer::XYZProviderBase,
	public ControlledObject
{
	struct MonolithDataProvider : public MultiChannelAudioBuffer::DataProvider
	{
		MonolithDataProvider(XYZSampleMapProvider* p, const ValueTree& sampleMap_);;
		MultiChannelAudioBuffer::SampleReference::Ptr loadFile(const String& referenceString) override;

		File parseFileReference(const String& b64) const override { return {}; }

		hise::HlacMonolithInfo::Ptr hmToUse;
		WeakReference<XYZSampleMapProvider> parent;
		ValueTree sampleMap;
	};

	struct FileBasedDataProvider : public MultiChannelAudioBuffer::DataProvider
	{
		FileBasedDataProvider(XYZSampleMapProvider* p);;
		MultiChannelAudioBuffer::SampleReference::Ptr loadFile(const String& referenceString) override;

		File parseFileReference(const String& b64) const override;

		WeakReference<XYZSampleMapProvider> parent;
	};

	XYZSampleMapProvider(MainController* mc);

	struct Editor : public ComplexDataUIBase::EditorBase,
		public Component,
		public ComplexDataUIUpdaterBase::EventListener,
		public ComboBox::Listener
	{
		Editor(XYZSampleMapProvider* p);;

		void comboBoxChanged(ComboBox* c) override;
		void setComplexDataUIBase(ComplexDataUIBase* newData) override;
		void updateComboBoxItem();
		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;
		void resized() override;

		SimpleSampleMapDisplay simpleMapDisplay;

		ComboBox cb;
		WeakReference<XYZSampleMapProvider> provider;
		WeakReference<MultiChannelAudioBuffer> currentBuffer;

		ScriptnodeComboBoxLookAndFeel claf;
	};

	ComplexDataUIBase::EditorBase* createEditor(MultiChannelAudioBuffer* b) override;
	static Identifier getStaticId();
	Identifier getId() const override;
	MultiChannelAudioBuffer::DataProvider* getDataProvider();
	bool parseAdditionalProperties(const ValueTree& s, MultiChannelAudioBuffer::XYZItem& d);
	void parseValueTree(const ValueTree& v, MultiChannelAudioBuffer::XYZItem::List& list);
	bool parse(const String& v, MultiChannelAudioBuffer::XYZItem::List& list) override;
	MultiChannelAudioBuffer::DataProvider::Ptr sampleMapDataProvider;

	JUCE_DECLARE_WEAK_REFERENCEABLE(XYZSampleMapProvider);
};

struct XYZSFZProvider : public XYZSampleMapProvider
{
	struct SFZFileLoader : public MultiChannelAudioBuffer::DataProvider
	{
		SFZFileLoader(const File& sfzFile_);;
		MultiChannelAudioBuffer::SampleReference::Ptr loadFile(const String& referenceString) override;

		File parseFileReference(const String& f) const override
		{
			if(File::isAbsolutePath(f))
				return File(f);

			return File();
		}

		File sfzFile;
	};

	XYZSFZProvider(MainController* mc);;

	struct Editor : public Component,
		public ComplexDataUIBase::EditorBase
	{
		Editor(XYZSFZProvider* p);

		struct DropTarget : public Component,
			public juce::FileDragAndDropTarget
		{
			DropTarget();

			void paint(Graphics& g) override;

			bool isInterestedInFileDrag(const StringArray& files) override;
			void fileDragEnter(const StringArray& files, int x, int y) override;
			void fileDragExit(const StringArray& files) override;
			void filesDropped(const StringArray& files, int, int);
			void mouseDown(const MouseEvent& e) override;

			bool hover = false;
		};

		void loadFile(File f);

		void setComplexDataUIBase(ComplexDataUIBase* newData) override;

		void resized() override;

		SimpleSampleMapDisplay display;
		DropTarget dropTarget;
		WeakReference<XYZSFZProvider> provider;
	};

	ComplexDataUIBase::EditorBase* createEditor(MultiChannelAudioBuffer* b) override;

	static Identifier getStaticId();

	Identifier getId() const override;

	bool parse(const String& v, MultiChannelAudioBuffer::XYZItem::List& list) override;

	MultiChannelAudioBuffer::DataProvider* getDataProvider() override;;

	ScopedPointer<SFZFileLoader> l;
	JUCE_DECLARE_WEAK_REFERENCEABLE(XYZSFZProvider);
};


} // namespace hise
