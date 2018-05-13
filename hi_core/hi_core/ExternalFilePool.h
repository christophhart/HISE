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

#ifndef EXTERNALFILEPOOL_H_INCLUDED
#define EXTERNALFILEPOOL_H_INCLUDED

namespace hise { using namespace juce;

namespace MetadataIDs
{
const Identifier SampleRate("SampleRate");
const Identifier LoopEnabled("LoopEnabled");
const Identifier LoopStart("LoopStart");
const Identifier LoopEnd("LoopEnd");
}

struct PoolHelpers
{
	

	// Using an empty parameter to get the correct Subdirectory type
	static ProjectHandler::SubDirectories getSubDirectoryType(const AudioSampleBuffer& emptyData);
	static ProjectHandler::SubDirectories getSubDirectoryType(const Image& emptyImage);

	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, AudioSampleBuffer& data, var& additionalData);
	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, Image& data, var& additionalData);

	static Image getEmptyImage(int width, int height);

	struct Reference
	{
		enum Mode
		{
			Invalid,
			AbsolutePath,
			ExpansionPath,
			ProjectPath,
			EmbeddedResource,
			numModes_
		};

		Reference();

		Reference(const MainController* mc, const String& referenceStringOrFile, ProjectHandler::SubDirectories directoryType);
		Reference(MemoryBlock& mb, const String& referenceString, ProjectHandler::SubDirectories directoryType);

		bool operator ==(const Reference& other) const;
		bool operator !=(const Reference& other) const;

		Mode getMode() const { return m; }
		String getReferenceString() const;
		Identifier getId() const;
		File getFile() const;
		bool isRelativeReference() const;
		bool isAbsoluteFile() const;
		bool isEmbeddedReference() const;;
		InputStream* createInputStream();
		int64 getHashCode() const;
		bool isValid() const;
		ProjectHandler::SubDirectories getFileType() const;

	private:

		void parseReferenceString(const MainController* mc, const String& input);

		String reference;
		File f;
		Identifier id;
		Mode m;

		int64 hashCode;

		void* memoryLocation;
		size_t memorySize;

		ProjectHandler::SubDirectories directoryType;
	};

};


class PoolCollection;

using PoolReference = PoolHelpers::Reference;

class PoolBase : public RestorableObject,
				 public ControlledObject,
				 public SafeChangeBroadcaster
{
public:

	ValueTree exportAsValueTree() const override
	{
		ValueTree v(getFileTypeName());

		for (int i = 0; i < getNumLoadedFiles(); i++)
		{
			ValueTree child("PoolData");
			storeItemInValueTree(child, i);
			v.addChild(child, -1, nullptr);
		}

		return v;
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		clearData();

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			ValueTree child = v.getChild(i);
			restoreItemFromValueTree(child);
		}

		notifyTable();
	}

	void notifyTable()
	{
		BACKEND_ONLY(sendChangeMessage());
	}

	virtual int getNumLoadedFiles() const = 0;
	virtual PoolReference getReference(int index) const = 0;
	virtual void clearData() = 0;
	virtual var getAdditionalData(PoolReference r) const = 0;
	virtual StringArray getTextDataForId(int index) const = 0;

protected:

	virtual void storeItemInValueTree(ValueTree& child, int index) const = 0;
	virtual void restoreItemFromValueTree(ValueTree& child) = 0;

	virtual Identifier getFileTypeName() const = 0;

	PoolBase(MainController* mc):
	ControlledObject(mc)
	{

	}

private:

	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PoolBase)
};

    template <class DataType> class PoolEntry: public ReferenceCountedObject
    {
    public:
        
        using Ptr = ReferenceCountedObjectPtr<PoolEntry>;
        
        PoolEntry(PoolReference& r) :
        ref(r),
        data(DataType())
        {};
        
        bool operator ==(const PoolEntry& other) const
        {
            return other.ref == ref;
        }
        
        PoolReference ref;
        DataType data;
        var additionalData;
    };

template <class DataType> class SharedPoolBase : public PoolBase
{
public:

    

	SharedPoolBase(MainController* mc_) :
		PoolBase(mc_),
		type(PoolHelpers::getSubDirectoryType(DataType()))
	{
		if (type == FileHandlerBase::SubDirectories::AudioFiles)
		{
			afm.registerBasicFormats();
			afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
		}

	};


	~SharedPoolBase() {};

	Identifier getFileTypeName() const override
	{
		return ProjectHandler::getIdentifier(type);
	}

	int getNumLoadedFiles() const override
	{
		return pool.size();
	}

	void clearData() override
	{
		pool.clear();
	}

	bool contains(int64 hashCode) const
	{
		for (auto d : pool)
		{
			if (d->ref.getHashCode() == hashCode)
				return true;
		}

		return false;
	}

	PoolReference getReference(int index) const override
	{
		if (auto entry = pool[index])
			return entry.get()->ref;

		return PoolReference();
	}

	var getAdditionalData(PoolReference r) const override
	{
		for (auto d : pool)
		{
			if (d->ref == r)
				return d->additionalData;
		}

		return {};
	}

	StringArray getTextDataForId(int index) const override
	{
		jassertfalse;

#if 0
		if (auto entry = pool[index])
			return entry.get()->getTextData();
#endif

		return {};
	}

	StringArray getIdList() const
	{
		StringArray sa;

		for (auto d : pool)
			sa.add(d->ref.getReferenceString());

		return sa;
	}

	DataType loadFromReference(PoolReference r)
	{
		for (auto d : pool)
		{
			if (d->ref == r)
				return d->data;
		}

		auto ne = new PoolEntry<DataType>(r);
		auto inputStream = r.createInputStream();

		PoolHelpers::loadData(afm, inputStream, r.getHashCode(), ne->data, ne->additionalData);

		pool.add(ne);

		notifyTable();

		return ne->data;
	}

private:

	ReferenceCountedArray<PoolEntry<DataType>> pool;

	void storeItemInValueTree(ValueTree& child, int index) const
	{
		if (auto entry = pool[index].get())
		{
			child.setProperty("ID", entry->ref.getReferenceString(), nullptr);

			ScopedPointer<InputStream> inputStream = entry->ref.createInputStream();

			MemoryBlock mb = MemoryBlock();
			MemoryOutputStream out(mb, false);
			out.writeFromInputStream(*inputStream, inputStream->getTotalLength());

			child.setProperty("Data", var(mb.getData(), mb.getSize()), nullptr);
		}
	}

	void restoreItemFromValueTree(ValueTree& child)
	{
		String r = child.getProperty("ID", String()).toString();

		var x = child.getProperty("Data", var::undefined());
		auto mb = x.getBinaryData();

		jassert(mb != nullptr);

		PoolReference ref(*mb, r, type);

		loadFromReference(ref);
	}

	ProjectHandler::SubDirectories type;

	AudioFormatManager afm;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SharedPoolBase<DataType>);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedPoolBase)
};

using AudioSampleBufferPool = SharedPoolBase<AudioSampleBuffer>;
using ImagePool = SharedPoolBase<Image>;

class PoolCollection: public ControlledObject
{
public:

	PoolCollection(MainController* mc):
		ControlledObject(mc)
	{
		for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
		{
			switch ((ProjectHandler::SubDirectories)i)
			{
			case ProjectHandler::SubDirectories::AudioFiles:
				dataPools[i] = new AudioSampleBufferPool(mc);
				break;
			case ProjectHandler::SubDirectories::Images:
				dataPools[i] = new ImagePool(mc);
				break;
			default:
				dataPools[i] = nullptr;
			}
		}
	};

	~PoolCollection()
	{
		for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
		{
			if (dataPools[i] != nullptr)
			{
				delete dataPools[i];
				dataPools[i] = nullptr;
			}
		}
	}

	void clear()
	{
		for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
		{
			if (dataPools[i] != nullptr)
			{
				dataPools[i]->clearData();
			}
		}
	}

	template<class DataType> SharedPoolBase<DataType>* getPool()
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());

		jassert(dataPools[type] != nullptr);

		return static_cast<SharedPoolBase<DataType>*>(dataPools[type]);
	}

	template<class DataType> const SharedPoolBase<DataType>* getPool() const
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());

		jassert(dataPools[type] != nullptr);

		return static_cast<SharedPoolBase<DataType>*>(dataPools[type]);
	}


	const AudioSampleBufferPool& getAudioSampleBufferPool() const
	{
		return *getPool<AudioSampleBuffer>();
	}

	AudioSampleBufferPool& getAudioSampleBufferPool()
	{
		return *getPool<AudioSampleBuffer>();
	}

	const ImagePool& getImagePool() const
	{
		return *getPool<Image>();
	}

	ImagePool& getImagePool()
	{
		return *getPool<Image>();
	}

	AudioFormatManager afm;

private:

	PoolBase * dataPools[(int)ProjectHandler::SubDirectories::numSubDirectories];

	JUCE_DECLARE_WEAK_REFERENCEABLE(PoolCollection);
};



} // namespace hise

#endif  // EXTERNALFILEPOOL_H_INCLUDED
