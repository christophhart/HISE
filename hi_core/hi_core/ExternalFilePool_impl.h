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

#if JUCE_WINDOWS
template class SharedPoolBase<AudioSampleBuffer>;
template class SharedPoolBase<Image>;
template class SharedPoolBase<ValueTree>;
template class SharedPoolBase<MidiFileReference>;
template class SharedPoolBase<AdditionalDataReference>;
#endif

template <class DataType>
SharedPoolBase<DataType>::ManagedPtr::ManagedPtr(SharedPoolBase<DataType>* pool_, PoolItem* object, bool refCounted):
    pool(pool_),
    isRefCounted(refCounted),
    weak(refCounted ? nullptr : object),
    strong(refCounted ? object : nullptr)
{
            
}

template <class DataType>
SharedPoolBase<DataType>::ManagedPtr::ManagedPtr():
    pool(nullptr),
    weak(nullptr),
    strong(nullptr),
    isRefCounted(true)
{

}

template <class DataType>
typename SharedPoolBase<DataType>::ManagedPtr& SharedPoolBase<DataType>::ManagedPtr::operator=(const ManagedPtr& other)
{
    if(isRefCounted)
        clear();

    pool = other.pool;
    weak = other.weak;
    strong = other.strong;
    isRefCounted = other.isRefCounted;

    return *this;
}

template <class DataType>
bool SharedPoolBase<DataType>::ManagedPtr::operator==(const ManagedPtr& other) const
{
    return getRef() == other.getRef();
}

template <class DataType>
SharedPoolBase<DataType>::ManagedPtr::operator bool() const
{ return get() != nullptr;}

template <class DataType>
typename SharedPoolBase<DataType>::PoolItem* SharedPoolBase<DataType>::ManagedPtr::get()
{ return isRefCounted ? strong.get() : weak.get(); }

template <class DataType>
const typename SharedPoolBase<DataType>::PoolItem* SharedPoolBase<DataType>::ManagedPtr::get() const
{ return isRefCounted ? strong.get() : weak.get(); }

template <class DataType>
SharedPoolBase<DataType>::ManagedPtr::operator PoolEntry<DataType>*() const noexcept
{ return const_cast<ManagedPtr*>(this)->get(); }

template <class DataType>
typename SharedPoolBase<DataType>::PoolItem* SharedPoolBase<DataType>::ManagedPtr::operator->() noexcept
{ return get(); }

template <class DataType>
const typename SharedPoolBase<DataType>::PoolItem* SharedPoolBase<DataType>::ManagedPtr::operator->() const noexcept
{ return get(); }

template <class DataType>
SharedPoolBase<DataType>::ManagedPtr::~ManagedPtr()
{
    weak = nullptr;

    if (isRefCounted)
        clear();
}

template <class DataType>
StringArray SharedPoolBase<DataType>::ManagedPtr::getTextData() const
{
    StringArray sa;

    if (get() != nullptr)
    {
        sa.add(getRef().getReferenceString());
        auto dataSize = PoolHelpers::getDataSize(getData());

        String s = String((float)dataSize / 1024.0f, 1) + " kB";

        sa.add(s);
        sa.add(String(get()->getReferenceCount()));
    }

    return sa;
}

template <class DataType>
const DataType* SharedPoolBase<DataType>::ManagedPtr::getData() const
{ return *this ? &get()->data : nullptr; }

template <class DataType>
DataType* SharedPoolBase<DataType>::ManagedPtr::getData()
{ return *this ? &get()->data : nullptr; }

template <class DataType>
PoolReference SharedPoolBase<DataType>::ManagedPtr::getRef() const
{ return *this ? get()->ref : PoolReference(); }

template <class DataType>
var SharedPoolBase<DataType>::ManagedPtr::getAdditionalData() const
{ return *this ? get()->additionalData : var(); }

template <class DataType>
void SharedPoolBase<DataType>::ManagedPtr::clear()
{
    if (pool.get() == nullptr)
        return;

    if (!getRef())
        return;

    jassert(isRefCounted);
            
    if(*this)
        pool->releaseIfUnused(*this);
}

template <class DataType>
void SharedPoolBase<DataType>::ManagedPtr::clearStrongReference()
{
    strong = nullptr;
    isRefCounted = false;
}

template <class DataType>
SharedPoolBase<DataType>::~SharedPoolBase()
{
    clearData();
}

template <class DataType>
Identifier SharedPoolBase<DataType>::getFileTypeName() const
{
    return ProjectHandler::getIdentifier(type);
}

template <class DataType>
int SharedPoolBase<DataType>::getNumLoadedFiles() const
{
    return weakPool.size();
}

template <class DataType>
void SharedPoolBase<DataType>::clearData()
{
    ScopedNotificationDelayer snd(*this, EventType::Removed);

    refCountedPool.clear();

    weakPool.clear();
    allFilesLoaded = false;
    sendPoolChangeMessage(Removed);
}

template <class DataType>
void SharedPoolBase<DataType>::refreshPoolAfterUpdate(PoolReference r)
{
    if (r.isValid())
    {
        loadFromReference(r, PoolHelpers::ForceReloadStrong);
    }
    else
    {
        clearData();
        loadAllFilesFromProjectFolder();
    }

        
}

template <class DataType>
bool SharedPoolBase<DataType>::contains(int64 hashCode) const
{
    for (int i = 0; i < getNumLoadedFiles(); i++)
    {
        if (weakPool.getReference(i).getRef().getHashCode() == hashCode)
            return true;
    }

    return false;
}

template <class DataType>
PoolReference SharedPoolBase<DataType>::getReference(int index) const
{
    if (index >= 0 && index < getNumLoadedFiles())
        return weakPool.getReference(index).getRef();

    return PoolReference();
}

template <class DataType>
int SharedPoolBase<DataType>::indexOf(PoolReference ref) const
{
    for (int i = 0; i < getNumLoadedFiles(); i++)
    {
        if (weakPool.getReference(i).getRef() == ref)
            return i;
    }

    return -1;
}

template <class DataType>
var SharedPoolBase<DataType>::getAdditionalData(PoolReference r) const
{
    auto i = indexOf(r);

    if (i >= 0)
        return weakPool.getReference(i).getAdditionalData();

    return {};
}

template <class DataType>
Array<PoolReference> SharedPoolBase<DataType>::getListOfAllReferences(bool includeEmbeddedButUnloadedReferences) const
{
    Array<PoolReference> references;

    for (int i = 0; i < getNumLoadedFiles(); i++)
        references.add(getReference(i));

    if (includeEmbeddedButUnloadedReferences)
    {
        auto additionalRefs = getDataProvider()->getListOfAllEmbeddedReferences();

        for (const auto& r : additionalRefs)
            references.addIfNotAlreadyThere(r);
    }

    return references;
}

template <class DataType>
StringArray SharedPoolBase<DataType>::getTextDataForId(int index) const
{
    if (index >= 0 && index < getNumLoadedFiles())
        return weakPool.getReference(index).getTextData();

    return {};
}

template <class DataType>
void SharedPoolBase<DataType>::loadAllFilesFromDataProvider()
{
    allFilesLoaded = true;
    ScopedNotificationDelayer snd(*this, EventType::Added);

    auto refList = getDataProvider()->getListOfAllEmbeddedReferences();

    for (auto r : refList)
        loadFromReference(r, PoolHelpers::LoadAndCacheStrong);
}

template <class DataType>
void SharedPoolBase<DataType>::loadAllFilesFromProjectFolder()
{
    refCountedPool.clear();
    weakPool.clear();

    ScopedNotificationDelayer snd(*this, EventType::Added);

    auto fileList = parentHandler->getFileList(type, false, true);

    ScopedValueSetter<bool> svs(useSharedCache, false);

    for (auto f : fileList)
    {
        PoolReference ref(getMainController(), f.getFullPathName(), type);

        loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
    }

    allFilesLoaded = true;
}

template <class DataType>
bool SharedPoolBase<DataType>::areAllFilesLoaded() const noexcept
{ return allFilesLoaded; }

template <class DataType>
String SharedPoolBase<DataType>::getStatistics() const
{
    String s;

    s << "Size: " << weakPool.size();

    size_t dataSize = 0;

    for (const auto& d : weakPool)
    {
        if (d)
            dataSize += PoolHelpers::getDataSize(d.getData());
    }

    s << " (" << String(dataSize / 1024.0f / 1024.0f, 2) << " MB)";

    return s;
}

template <class DataType>
StringArray SharedPoolBase<DataType>::getIdList() const
{
    StringArray sa;

    for (const auto& d : weakPool)
        sa.add(d.getRef().getReferenceString());

    return sa;
}

template <class DataType>
void SharedPoolBase<DataType>::releaseIfUnused(ManagedPtr& mptr)
{
    jassert(mptr);
    jassert(mptr.getRef().isValid());

    PoolReference r(mptr.getRef());

    for (int i = 0; i < weakPool.size(); i++)
    {
        if (weakPool.getReference(i) == mptr)
        {
            mptr.clearStrongReference();

            if (weakPool.getReference(i).get() == nullptr)
            {
                weakPool.remove(i--);
                sendPoolChangeMessage(PoolBase::EventType::Removed, sendNotificationAsync, r);
            }
            else
                sendPoolChangeMessage(PoolBase::EventType::Changed, sendNotificationAsync, r);

            return;
        }
    }
}

template <class DataType>
void SharedPoolBase<DataType>::writeItemToOutput(OutputStream& output, PoolReference r)
{
    auto d = getWeakReferenceToItem(r);

    if (d)
    {
        auto ref = d.getRef();

        File original;

        if (!ref.isEmbeddedReference())
            original = ref.getFile();

        getDataProvider()->getCompressor()->write(output, *d.getData(), original);
    }
            
}

template <class DataType>
typename SharedPoolBase<DataType>::ManagedPtr SharedPoolBase<DataType>::getWeakReferenceToItem(PoolReference r)
{
    auto index = indexOf(r);

    if (index != -1)
        return ManagedPtr(this, weakPool.getReference(index).get(), false);

    jassertfalse;
    return ManagedPtr();
}

template <class DataType>
typename SharedPoolBase<DataType>::ManagedPtr SharedPoolBase<DataType>::createAsEmbeddedReference(PoolReference r,
    DataType t)
{
    ReferenceCountedObjectPtr<PoolItem> ne = new PoolItem(r);

    ne->data = t;

    PoolHelpers::fillMetadata(ne->data, &ne->additionalData);

    refCountedPool.add(ManagedPtr(this, ne.get(), true));
    weakPool.add(ManagedPtr(this, ne.get(), false));

    return ManagedPtr(this, ne.get(), true);
}

template <class DataType>
typename SharedPoolBase<DataType>::ManagedPtr SharedPoolBase<DataType>::loadFromReference(PoolReference r,
    PoolHelpers::LoadingType loadingType)
{
    if (getDataProvider()->isEmbeddedResource(r))
        r = getDataProvider()->getEmbeddedReference(r);

    if (useSharedCache && sharedCache->contains(r.getHashCode()))
    {
        return ManagedPtr(this, sharedCache->getSharedData(r.getHashCode()), true);
    }

    if (PoolHelpers::shouldSearchInPool(loadingType))
    {
        int index = indexOf(r);

        if (index != -1)
        {
            auto& d = weakPool.getReference(index);

            jassert(d);

            if (PoolHelpers::shouldForceReload(loadingType))
            {
                jassert(!r.isEmbeddedReference());

                if (auto inputStream = r.createInputStream())
                {
                    auto ad = d.getAdditionalData();
                    jassert(ad.isObject());
                        
                    PoolHelpers::loadData(afm, inputStream, r.getHashCode(), *d.getData(), &ad);

                    sendPoolChangeMessage(PoolBase::Reloaded, sendNotificationSync, r);
                    return ManagedPtr(this, d.get(), true);
                }
                else
                {
                    logMessage(getMainController(), r.getReferenceString() + " wasn't found.");

                    jassertfalse; // This shouldn't happen...
                    return ManagedPtr();
                }
            }
            else
            {
                sendPoolChangeMessage(PoolBase::Changed, sendNotificationAsync, r);
                return ManagedPtr(this, d.get(), true);
            }
        }
    }

    if (loadingType == PoolHelpers::DontCreateNewEntry)
    {
        jassertfalse;
        return ManagedPtr();
    }

    ReferenceCountedObjectPtr<PoolItem> ne = new PoolItem(r);
        
    if (r.isEmbeddedReference())
    {
        if (auto mis = getDataProvider()->createInputStream(r.getReferenceString()))
        {
            getDataProvider()->getCompressor()->create(mis, &ne->data);

            ne->additionalData = getDataProvider()->createAdditionalData(r);


            if (loadingType != PoolHelpers::BypassAllCaches)
            {
                if (useSharedCache)
                {
                    sharedCache->store(ne.get());
                }
                else
                {
                    weakPool.add(ManagedPtr(this, ne.get(), false));
                    refCountedPool.add(ManagedPtr(this, ne.get(), true));
                }
            }

            sendPoolChangeMessage(PoolBase::Added);

            return ManagedPtr(this, ne.get(), true);
        }
        else
        {
            if (PoolHelpers::throwIfNotLoaded(loadingType))
            {
                //jassertfalse;

                //PoolHelpers::sendErrorMessage(getMainController(), "The file " + r.getReferenceString() + " is not embedded correctly.");
            }

            return ManagedPtr();
        }
    }
    else
    {
        if (auto inputStream = r.createInputStream())
        {
            PoolHelpers::loadData(afm, inputStream, r.getHashCode(), ne->data, &ne->additionalData);


            if (useSharedCache && loadingType != PoolHelpers::LoadAndCacheStrong)
            {
                sharedCache->store(ne.get());
            }
            else
            {
                weakPool.add(ManagedPtr(this, ne.get(), false));

                if (PoolHelpers::isStrong(loadingType))
                    refCountedPool.add(ManagedPtr(this, ne.get(), true));
            }
                
            sendPoolChangeMessage(PoolBase::Added);

            return ManagedPtr(this, ne.get(), true);
        }
        else
        {
            logMessage(getMainController(), r.getReferenceString() + " wasn't found.");
            return ManagedPtr();
        }
    }
}

template <class DataType>
SharedPoolBase<DataType>::SharedPoolBase(MainController* mc_, FileHandlerBase* handler) :
    PoolBase(mc_, handler)
{
    type = PoolHelpers::getSubDirectoryType(empty);

    if (type == FileHandlerBase::SubDirectories::AudioFiles)
    {
        afm.registerBasicFormats();
        afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
    }

}

} // namespace hise
