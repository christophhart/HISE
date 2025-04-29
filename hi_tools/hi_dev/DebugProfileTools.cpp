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


int ProfileCollection::openTrack(int sourceIndex)
{
	if(!enabled || !isPositiveAndBelow(sourceIndex, trackEvents.size()))
		return 0;

	auto idx = holder->getDebugSession()->openTrackEvent();
	trackEvents[sourceIndex] = idx;
	return idx;
}

void ProfileCollection::closeTrack(int sourceIndex)
{
	if(!enabled || !isPositiveAndBelow(sourceIndex, trackEvents.size()))
		return;

	holder->getDebugSession()->closeTrackEvent(trackEvents[sourceIndex]);
}

void ProfileCollection::setHolder(ApiProviderBase::Holder* h, bool listenToProfileChange)
{
	if(holder != h)
	{
		holder = h;

		if(listenToProfileChange)
		{
			h->getDebugSession()->syncRecordingBroadcaster.addListener(*this, [](ProfileCollection& p, bool on)
			{
				p.enabled = on;
			});
		}
		else
			enabled = true;

		for(auto s: sources)
			s->holder = h;
	}
}

void ProfileCollection::setSourceType(PS::SourceType t)
{
	sourceType = t;

	for(auto s: sources)
		s->sourceType = t;
}

void ProfileCollection::setColour(Colour newColour)
{
	c = newColour;

	for(auto s: sources)
		s->colour = c;
}

void ProfileCollection::setPrefix(const String& newPrefix)
{
	if(prefix != newPrefix)
	{
		for(auto s: sources)
			s->name = s->name.replace(prefix, newPrefix);

		prefix = newPrefix;
	}
}

int ProfileCollection::add(const String& name)
{
	auto ni = new DebugSession::ProfileDataSource();
	ni->name = prefix + name;
	ni->sourceType = sourceType;
	ni->holder = holder.get();

	if(!c.isTransparent())
		ni->colour = c;

	ni->durationThreshold = durationThreshold;
	sources.add(ni);
	trackEvents.push_back(0);
	return sources.size() - 1;
}

void ProfileCollection::clearSource(int sourceIndex)
{
	sources.set(sourceIndex, nullptr);
}

DebugSession::ProfileDataSource::Ptr ProfileCollection::getSource(int sourceIndex) const
{
	if(isPositiveAndBelow(sourceIndex, sources.size()))
		return sources[sourceIndex];

	return nullptr;
}

ProfiledComponent::Profiler::Profiler(ProfiledComponent& parent):
	ScopedProfiler(parent.profileData, parent.holder)
{
	if(*this && parent.profileTrackId)
	{
		parent.holder->getDebugSession()->closeTrackEvent(parent.profileTrackId);
	}
}



ProfiledComponent::ComponentHeatmapGenerator::ComponentHeatmapGenerator(ProfiledComponent* rootNode):
	HeatmapGenerator<ProfiledComponent>(rootNode)
{}

DebugSession::ProfileDataSource* ProfiledComponent::ComponentHeatmapGenerator::getSourceFromDataType(ProfiledComponent* d)
{
	return d->profileData.get();
}

int ProfiledComponent::ComponentHeatmapGenerator::getNumChildren(ProfiledComponent* d) const
{
	auto c = d->asComponent();

	int idx = 0;

	for(int i = 0; i < c->getNumChildComponents(); i++)
	{
		if(auto cpc = dynamic_cast<ProfiledComponent*>(c->getChildComponent(i)))
			idx++;
	}

	return idx;
}

ProfiledComponent* ProfiledComponent::ComponentHeatmapGenerator::getChild(ProfiledComponent* n, int index)
{
	auto c = n->asComponent();

	int idx = 0;

	for(int i = 0; i < c->getNumChildComponents(); i++)
	{
		if(auto cpc = dynamic_cast<ProfiledComponent*>(c->getChildComponent(i)))
		{
			if(idx == index)
				return cpc;

			idx++;
		}
							
	}

	jassertfalse;
	return nullptr;
}

void ProfiledComponent::setEnableProfiling(bool shouldBeProfiling, ApiProviderBase::Holder* holder_, bool isRoot)
{
	auto isProfiling = profileData != nullptr;
	holder = holder_;

	if(shouldBeProfiling != isProfiling)
	{
		if(shouldBeProfiling)
		{
			profileData = new DebugSession::ProfileDataSource();
			profileData->name = asComponent()->getName() + ".paint()";
			profileData->locationString = asComponent()->getName();
			profileData->sourceType = DebugSession::ProfileDataSource::SourceType::Paint;
			profileData->preferredDomain = DebugSession::ProfileDataSource::TimeDomain::FPS60;
		}
		else
		{
			profileData = nullptr;
		}

		if(auto laf = dynamic_cast<ProfiledLookAndFeel*>(&asComponent()->getLookAndFeel()))
		{
			laf->setEnableProfiling(profileData, holder);
		}

		onProfileEnableChange();
	}

	if(isRoot)
	{
		Component::callRecursive<ProfiledComponent>(asComponent(), [shouldBeProfiling, holder_](ProfiledComponent* c)
		{
			c->setEnableProfiling(shouldBeProfiling, holder_, false);
			return false;
		});
	}
}

void ProfiledLookAndFeel::setEnableProfiling(DebugSession::ProfileDataSource::Ptr ptr,
	ApiProviderBase::Holder* holder_)
{
	p = ptr;
	holder = ptr != nullptr ? holder_ : nullptr;
	onProfileEnableChange();
}

ProfiledProcessor::Profiler::Profiler(ProfiledProcessor& parent, int sourceIndex):
	ScopedProfiler(parent.enabled ? parent.sources[sourceIndex] : nullptr, parent.holder)
{}

void ProfiledProcessor::setEnableProfiling(bool shouldBeEnabled, ApiProviderBase::Holder* holder_, int indexInTree)
{
	holder = holder_;

	if(enabled != shouldBeEnabled)
	{
		enabled = shouldBeEnabled;
		sources[0]->lineRange = { indexInTree, indexInTree + 1};
		onProfileEnableChange();
	}
}

void ProfiledProcessor::forEachProfileSource(const std::function<void(DebugSession::ProfileDataSource::Ptr)>& f)
{
	for(auto& p: sources)
		f(p);
}

DebugSession::ProfileDataSource::Ptr ProfiledProcessor::addProfileDataSource(const String& name, bool isLoop)
{
	auto ni = new DebugSession::ProfileDataSource();
	ni->name = name;
	ni->isLoop = isLoop;
	ni->sourceType = DebugSession::ProfileDataSource::SourceType::DSP;
	ni->preferredDomain = DebugSession::ProfileDataSource::TimeDomain::CpuUsage;
	ni->locationString = name.upToFirstOccurrenceOf(".", false, false);
	sources.add(ni);
	return ni;
}

ProfiledSampleThreadPool::ProfiledSampleThreadPool(DebugSession* s):
	SampleThreadPool(nullptr),
	recordingSession(new DebugSession::ProfileDataSource::RecordingSession(*s, DebugSession::ThreadIdentifier::Type::LoadingThread)),
	h(s),
	cp(nullptr)
{
	s->syncRecordingBroadcaster.addListener(*this, [](ProfiledSampleThreadPool& t, bool start)
	{
		t.enabled = start;
		t.notify();
	}, false);
}

void ProfiledSampleThreadPool::onProfile(Job* j, bool isPush)
{
	SampleThreadPool::onProfile(j, isPush);

	if(enabled)
	{
		auto data = jobData[j];

		if(data == nullptr)
		{
			data = new DebugSession::ProfileDataSource();;
			data->name = j->getName();
			data->preferredDomain = DebugSession::ProfileDataSource::TimeDomain::Absolute;
			data->sourceType = DebugSession::ProfileDataSource::SourceType::BackgroundTask;
			jobData[j] = data;
		}

		cp = DebugSession::ProfileDataSource::Profiler(data);

		if(isPush)
			cp.startProfiling(h);
		else
			cp.stopProfiling(h);

	}
	else if (cp && !isPush)
		cp.stopProfiling(h);
}

void ProfiledSampleThreadPool::checkProfiling()
{
	recordingSession->initIfEmpty(DebugSession::ThreadIdentifier::getCurrent());

	if(enabled)
		recordingSession->checkRecording();
}
}
