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

struct ProfileCollection
{
	using PS = DebugSession::ProfileDataSource;
	using ID = int;

	PS::ScopedProfiler profile(int sourceIndex) const
	{
		return { enabled ? sources[sourceIndex] : nullptr, holder };
	}

	int openTrack(int sourceIndex);
	void closeTrack(int sourceIndex);

	void setHolder(ApiProviderBase::Holder* h, bool listenToProfileChange);
	void setSourceType(PS::SourceType t);
	void setColour(Colour newColour);
	void setPrefix(const String& newPrefix);
	void setDurationThreshold(double minimumDurationMilliseconds)
	{
		durationThreshold = minimumDurationMilliseconds;

		for(auto s: sources)
			s->durationThreshold = durationThreshold;
	}

	int add(const String& name);
	void clearSource(int sourceIndex);
	PS::Ptr getSource(int sourceIndex) const;

private:

	double durationThreshold = 0.0;
	bool enabled = false;
	std::vector<uint32> trackEvents;

	WeakReference<ApiProviderBase::Holder> holder;
	String prefix;
	PS::SourceType sourceType = PS::SourceType::Undefined;
	Colour c;
	PS::List sources;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ProfileCollection);
};

/** A interface class for all components that should record their paint performance.
 *
 *  In order to use it, subclass from this class and add the macro paintAndProfileChildren(g); to your class
 *	which will intercept the paint routine and child component paint routines and add a profiler scope.
 */
struct ProfiledComponent
{
	/** Add this in your paint method... */
	struct Profiler: public DebugSession::ProfileDataSource::ScopedProfiler
	{
		Profiler(ProfiledComponent& parent);;
	};

	struct ComponentHeatmapGenerator: public DebugSession::ProfileDataSource::HeatmapGenerator<ProfiledComponent>
	{
		ComponentHeatmapGenerator(ProfiledComponent* rootNode);;

		DebugSession::ProfileDataSource* getSourceFromDataType(ProfiledComponent* d) override;
		int getNumChildren(ProfiledComponent* d) const override;
		ProfiledComponent* getChild(ProfiledComponent* n, int index) override;
	};
	
	virtual ~ProfiledComponent() = default;

	int getHeatmapIndex() const { return profileData != nullptr ? profileData->lineRange.getStart() : -1; }
	bool isProfiling() const { return profileData != nullptr; }
	virtual void onProfileEnableChange() {}
	void setEnableProfiling(bool shouldBeProfiling, ApiProviderBase::Holder* holder_, bool isRoot=true);

	Component* asComponent() { jassert(dynamic_cast<Component*>(this) != nullptr); return dynamic_cast<Component*>(this); }
	const Component* asComponent() const { return dynamic_cast<const Component*>(this); }

	/** You can call this method if you want to track where the paint call is coming from. */
	void repaintWithProfileTrack(uint32 p)
	{
		setRepaintTrackId(p);
		asComponent()->repaint();
	}

	void setRepaintTrackId(uint32 p)
	{
		profileTrackId = p;
	}

	/** Call this from every mouse callback that is supposed to trigger a recording session. */
	void checkTriggerRecord(bool isDown)
	{
		if(getDebugSession)
			getDebugSession()->checkMouseClickProfiler(isDown);
	}

	/** Call this to set the debug session to all components that are in the same window as the given one. */
	static void setDebugSessionToAllComponents(Component* any, DebugSession& s)
	{
		// yeah that's a good idea...
		jassert(any->getParentComponent() != nullptr);

		WeakReference<ApiProviderBase::Holder> session(&s);
		auto tc = any->getTopLevelComponent();
		auto f = [session]()
		{
			jassert(session != nullptr);
			return dynamic_cast<DebugSession*>(session.get());
		};

		Component::callRecursive<ProfiledComponent>(tc, [f](ProfiledComponent* pc)
		{
			pc->getDebugSession = f;
			return false;
		});
	}

protected:

	ApiProviderBase::Holder* getProfileHolderIfProfiling() { return isProfiling() ? holder.get() : nullptr; }

private:

	std::function<DebugSession*()> getDebugSession;
	uint32 profileTrackId = 0;

	DebugSession::ProfileDataSource::Ptr profileData;
	WeakReference<ApiProviderBase::Holder> holder;
};

struct ProfiledLookAndFeel
{
	struct Profiler: public DebugSession::ProfileDataSource::ScopedProfiler
	{
		Profiler(ProfiledLookAndFeel& parent):
		  ScopedProfiler(parent.p, parent.holder)
		{}
	};

	virtual ~ProfiledLookAndFeel() = default;

	/** Override this method and react on the profile data change. */
	virtual void onProfileEnableChange() {}
	void setEnableProfiling(DebugSession::ProfileDataSource::Ptr ptr, ApiProviderBase::Holder* holder_);
	bool isProfiling() const { return p != nullptr; }
	
	WeakReference<ApiProviderBase::Holder> holder;
	DebugSession::ProfileDataSource::Ptr p;
};

struct ProfiledProcessor
{
	struct Profiler: public DebugSession::ProfileDataSource::ScopedProfiler
	{
		Profiler(ProfiledProcessor& parent, int sourceIndex);
	};

	virtual ~ProfiledProcessor() = default;
	bool isProfiling() const { return enabled; }
	void setEnableProfiling(bool shouldBeEnabled, ApiProviderBase::Holder* holder_, int indexInTree);
	int getHeatmapIndex() const { return isProfiling() ? sources[0]->lineRange.getStart() : -1; }
	virtual void onProfileEnableChange() {};

protected:

	void forEachProfileSource(const std::function<void(DebugSession::ProfileDataSource::Ptr)>& f);

	/** Call this from the constructor of the processor to add different profile sources. */
	DebugSession::ProfileDataSource::Ptr addProfileDataSource(const String& name, bool isLoop = false);

private:

	bool enabled = false;
	ReferenceCountedArray<DebugSession::ProfileDataSource> sources;
	WeakReference<ApiProviderBase::Holder> holder;
};

struct ProfiledSampleThreadPool: public SampleThreadPool
{
	ProfiledSampleThreadPool(DebugSession* s);;

	void onProfile(Job* j, bool isPush) override;
	void checkProfiling() override;

private:

	DebugSession::ProfileDataSource::Profiler cp;

	bool enabled = false;
	std::map<Job*, DebugSession::ProfileDataSource::Ptr> jobData;
	ApiProviderBase::Holder* h;
	ScopedPointer<DebugSession::ProfileDataSource::RecordingSession> recordingSession;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ProfiledSampleThreadPool);
};

#define paintAndProfileChildren(g) void paintComponentAndChildren(Graphics& g) override { Profiler p(*this); Component::paintComponentAndChildren(g); }
#define ProfiledRecordingSession DebugSession::ProfileDataSource::RecordingSession
#define PROFILE_ONLY(x) x;


}