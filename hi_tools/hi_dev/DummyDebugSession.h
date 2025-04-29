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

namespace dummy {

class DebugSession
{
public:

	struct ThreadIdentifier
	{
		enum class Type
		{
			Undefined,
			LoadingThread,
			AudioThread,
			ScriptingThread,
			ServerThread,
			UIThread,
			WorkerThread
		};

		static ThreadIdentifier getCurrent() { return {}; }
	};

	DebugSession(PooledUIUpdater*) {}

	struct ProfileDataSource: public ReferenceCountedObject
	{
		enum class SourceType: uint8
		{
			Undefined = 0,
			Lock,
			Script,
			ScriptCallback,
			Broadcaster,
			Server,
			Paint,
			DSP,
			Scriptnode,
			Trace,
			BackgroundTask,
			RecordingSession,
			numSourceTypes
		};

		using Ptr = ReferenceCountedObjectPtr<ProfileDataSource>;
		using List = ReferenceCountedArray<ProfileDataSource>;

		struct Profiler
		{
			Profiler(Ptr p_, void* unused=nullptr): p(p_) {}

			void startProfiling(void* unused=nullptr) {}
			void stopProfiling(void* unused=nullptr) {}

			operator bool() const { return false; }

			Ptr p;
		};

		using ScopedProfiler = Profiler;
	};

	void initUIThread() {};
};

class DummyProfiledComponent
{
public:

	virtual ~DummyProfiledComponent() = default;

	void repaintWithProfileTrack(uint32)
	{
		auto c = dynamic_cast<Component*>(this);
		jassert(c != nullptr);
		c->repaint();
	}

	void setEnableProfiling(bool, void*, bool) {};
};

class DummyProfiledProcessor
{
public:

	struct Profiler
	{
		Profiler(DummyProfiledProcessor&, int) {};
	};

	virtual ~DummyProfiledProcessor() {};
	virtual void onProfileEnableChange() {}
	virtual double getBufferDuration() const { return 0.0; }
	void setEnableProfiling(bool) {};
};



class DummyProfiledLookAndFeel
{
public:
};

class DummyProfileRecordingSession
{
public:

	DummyProfileRecordingSession(DebugSession&, DebugSession::ThreadIdentifier::Type) {};

	void initIfEmpty(DebugSession::ThreadIdentifier) {}
	void checkRecording() {};
	DebugSession::ProfileDataSource::Ptr getDataSource() const { return nullptr; }
};


struct DummyProfileCollection
{
	using ID = int;
	using PS = DebugSession::ProfileDataSource;

	PS::ScopedProfiler profile(int sourceIndex) const
	{
		return { nullptr, nullptr };
	}

	int openTrack(int) { return 0; }
	void closeTrack(int) {}
	void setHolder(void*, bool) {};
	void setSourceType(PS::SourceType) {}
	void setColour(Colour) {}
	void setPrefix(const String&) {}
	void setDurationThreshold(double) {}

	int add(const String& name) { return 0; }
	void clearSource(int sourceIndex) {}
	PS::Ptr getSource(int sourceIndex) const { return nullptr; }

	JUCE_DECLARE_WEAK_REFERENCEABLE(DummyProfileCollection);
};

} // dummy


class DebugSession: public dummy::DebugSession
{
public:

	DebugSession(PooledUIUpdater*):
	  dummy::DebugSession(nullptr)
	{}
};

#define ProfileCollection dummy::DummyProfileCollection
#define ProfiledComponent dummy::DummyProfiledComponent
#define paintAndProfileChildren(g) 
#define ProfiledProcessor dummy::DummyProfiledProcessor
#define ProfiledSampleThreadPool SampleThreadPool
#define ProfiledLookAndFeel dummy::DummyProfiledLookAndFeel
#define ProfiledRecordingSession dummy::DummyProfileRecordingSession
#define PROFILE_ONLY(x)

}