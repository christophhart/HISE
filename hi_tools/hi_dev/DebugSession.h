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

#define DECLARE_ID(x) static const Identifier x(#x);

namespace ProfileIds
{
	DECLARE_ID(profile);
	DECLARE_ID(dataList);
	DECLARE_ID(sourceList);
	DECLARE_ID(dataItem);
	DECLARE_ID(children);
	DECLARE_ID(root);
	DECLARE_ID(name);
	DECLARE_ID(lineStart);
	DECLARE_ID(lineEnd);
	DECLARE_ID(domain);
	DECLARE_ID(loopCounter);
	DECLARE_ID(runIndex);
	DECLARE_ID(highPriority);
	DECLARE_ID(dlocPos);
	DECLARE_ID(dlocName);
	DECLARE_ID(length);
	DECLARE_ID(sourceIndex);
	DECLARE_ID(start);
	DECLARE_ID(multithread);
	DECLARE_ID(thread);
	DECLARE_ID(trackSource);
	DECLARE_ID(trackTarget);
	DECLARE_ID(colour);
	DECLARE_ID(sourceType);
	DECLARE_ID(value);
	DECLARE_ID(label);
}

#undef DECLARE_ID

#define DBG_P(x) // DBG(x);


 
class DebugSession: public ApiProviderBase,
					public ApiProviderBase::Holder,
					public PooledUIUpdater::SimpleTimer
{
public:

	static constexpr int MaxLoopCounter = 64;
	static constexpr int MaxRunsToKeep = 512;
	static constexpr int QueueSize = 65536;
	static constexpr int StackSize = 128;

	enum class RecordingState
	{
		Idle,
		Armed,
		WaitingForRecording,
		Recording,
		WaitingForStop,
		numRecordingStates
	};

	enum class TriggerType
	{
		Manual,
		Compilation,
		MidiInput,
		MouseClick,
		numTriggerTypes
	};

	struct SerialisedData: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<SerialisedData>;
		using List = ReferenceCountedArray<SerialisedData>;

		SerialisedData(const Identifier& type_):
		  type(type_)
		{};

		Identifier getType() const { return type; }

		void setProperty(const Identifier& id, const var& newValue, void* unused=nullptr);
		static Ptr create(const Identifier& id) { return new SerialisedData(id); }
		var operator[](const Identifier& id) const { return values[id]; }
		void addChild(SerialisedData::Ptr d, int unused1=-1, void* unused2=nullptr);
		static SerialisedData::Ptr fromStream(InputStream& input);
		bool writeToStream(OutputStream& output);
		std::string toString(int intend) const;

		SerialisedData** begin() const { return const_cast<SerialisedData*>(this)->children.begin(); }
		SerialisedData** end() const { return const_cast<SerialisedData*>(this)->children.end(); }

		int getNumChildren() const;
		Ptr getChildWithName(const Identifier& id) const;

		int numTotalItems = 1;
		Identifier type;
		NamedValueSet values;
		List children;
	};

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
			WorkerThread,
			numTypes
		};

		static String getThreadName(Type t)
		{
			switch(t)
			{
			case Type::Undefined: return "Unknown";
			case Type::AudioThread: return "Audio Thread";
			case Type::UIThread: return "UI Thread";
			case Type::ScriptingThread: return "Scripting Thread";
			case Type::LoadingThread: return "Loading Thread";
			case Type::WorkerThread: return "Worker Thread";
			case Type::ServerThread: return "Server Thread";
			default: ;
			}

			jassertfalse;
			return "Unknown";
		}

		static ThreadIdentifier fromThread(Thread* t, Type type)
		{
			return { t->getThreadId(), type };
		}

		static ThreadIdentifier getCurrent()
		{
			return { Thread::getCurrentThreadId(), {} };
		}

		bool operator!=(const ThreadIdentifier& other) const
		{
			return !(*this == other);
		}

		bool operator<(const ThreadIdentifier& other) const
		{
			return threadId < other.threadId;
		}

		bool operator==(const ThreadIdentifier& other) const
		{
			return other.threadId == threadId;
		}

		bool hasType() const { return type != Type::Undefined; }

		ThreadIdentifier withType(Type t) const
		{
			auto copy = *this;
			copy.type = t;
			return copy;
		}

		void* threadId = nullptr;
		Type type = Type::Undefined;
	};

	struct ItemBase: public DebugInformationBase
	{
		WeakReference<ApiProviderBase::Holder> p;

		String getIconPath() const override { return "graph"; }
		Component* createPopupComponent(const MouseEvent& e, Component* componentToNotify);
		Component* createComponent(bool forceJSON);
	};

	struct DataItem: public ItemBase
	{
		using Ptr = ReferenceCountedObjectPtr<DataItem>;
		using List = ReferenceCountedArray<DataItem>;

		int getType() const override { return 3; }
		String getCategory() const override { return "DataItem"; }
		String getTextForName() const override;;
		String getTextForValue() const override { return data.toString(); };
		String getTextForDataType() const override { return getVarType(data); }
		String getCodeToInsert() const override { return getTextForName(); }
		var getVariantCopy() const override { return data; }

		SerialisedData::Ptr toSerialisedData() const;

		static Ptr fromSerialisedData(SerialisedData::Ptr v_);

		int idx;
		var data;
		Identifier id;
		int lineNumber;
		String label;
		WeakReference<ApiProviderBase::Holder> p;
	};

	struct ProfileDataSource: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<ProfileDataSource>;
		using List = ReferenceCountedArray<ProfileDataSource>;

		enum class TimeDomain: uint8
		{
			Undefined = 0,
			Absolute,
			Relative,
			CpuUsage,
			FPS60
		};

		enum class SourceType: uint8
		{
			Undefined = 0,
			Lock,
			Script,
			ScriptCallback,
			Broadcaster,
			TimerCallback,
			Server,
			Paint,
			DSP,
			Scriptnode,
			Trace,
			BackgroundTask,
			RecordingSession,
			numSourceTypes
		};

		struct ViewComponents
		{
			static constexpr int TopBarHeight = 40;
			static constexpr int ItemHeight = 25;
			
			static constexpr int MenuBarHeight = 28;
			static constexpr int MultiViewerMenuBarHeight = 40;
			static constexpr int ResizerWidth = 5;
			
			static constexpr int DefaultWidth = 1000;
			static constexpr int LabelMargin = 25;
			static constexpr int BreadcrumbHeight = 35;
			static constexpr int ItemMargin = 1;

			static constexpr int RowHeight = 20; // ItemPopup

			struct BaseWithManagerConnection;
			struct ViewItem;
			struct Viewer;
			struct MultiViewer;
			struct StatisticsComponent;
			struct ItemPopup;
			struct Manager;
			struct SingleThreadPopup;

			struct Helpers
			{
				static String getDuration(float w, TimeDomain domain, double contextValue);

				static void drawLabel(Graphics& g, const std::pair<Rectangle<float>, String>& l);
			};
		};

		static String getSourceTypeName(SourceType t)
		{
			switch(t)
			{
			case SourceType::Undefined : return "Undefined";
			case SourceType::Lock: return "Lock";
			case SourceType::Script: return "Script";
			case SourceType::Scriptnode: return "Scriptnode";
			case SourceType::ScriptCallback: return "Callback";
			case SourceType::TimerCallback: return "TimerCallback";
			case SourceType::Broadcaster: return "Broadcaster";
			case SourceType::Paint: return "Paint";
			case SourceType::DSP: return "DSP";
			case SourceType::Trace: return "Trace";
			case SourceType::Server: return "Server";
			case SourceType::BackgroundTask: return "Background Task";
			case SourceType::RecordingSession: return "Threads";
            case SourceType::numSourceTypes:
                    break;
            }

			return {};
		}

		struct Data
		{
			double start = 0.0;
			double delta = 0.0;

			uint64_t runIndex = 0;
			int64_t loopIndex = -1;
			int trackSource = -1;
			int trackTarget = -1;

			DataItem::List attachedData;

			Ptr source;
			Ptr parent;
			ThreadIdentifier::Type threadType;
			WeakReference<ApiProviderBase::Holder> holder;
		};

		struct Profiler
		{
			Profiler(Ptr statementData):
			  data(statementData)
			{};

			virtual ~Profiler() {};

			operator bool() const { return data != nullptr; }

			void startProfiling(ApiProviderBase::Holder* holder);
			void stopProfiling(ApiProviderBase::Holder* holder = nullptr);

			Ptr data;
			DebugSession* session;
		};

		/** Generates a heatmap index and writes it into the profile data source line ranges.
		 *
		 *  This is templated to be used with any kind of class that has a parent / child relation ship.
		 *
		 */
		template <typename ObjectType>struct HeatmapGenerator
		{
			HeatmapGenerator(ObjectType* root_):
			  root(root_)
			{}

			void generateHeatmapIndexes()
			{
				buildRecursive(root);

				for(const auto& item: ranges)
				{
					if(auto s = getSourceFromDataType(item.first))
						s->lineRange = item.second;
				}
			}

		protected:

			virtual ~HeatmapGenerator() {};

			/** Override this method and return the profile data source that represents this object. */
			virtual ProfileDataSource* getSourceFromDataType(ObjectType* d) = 0;

			/** Override this method and return the number of child objects that have a data source themselves. */
			virtual int getNumChildren(ObjectType* d) const = 0;

			/** Override this method and return the child object with the given index. */
			virtual ObjectType* getChild(ObjectType* d, int index) = 0;

		private:

			void buildRecursive(ObjectType* v)
			{
				auto thisStart = currentIndex++;

				for(int i = 0; i < getNumChildren(v); i++)
				{
					auto d = getChild(v, i);
					buildRecursive(d);
				}
				
				auto thisEnd = currentIndex;

				ranges.push_back({v, { thisStart, thisEnd }});
			}

			ObjectType* root;

			int currentIndex = 0;
			std::vector<std::pair<ObjectType*, Range<int>>> ranges;
		};

		struct ScopedProfiler: public Profiler
		{
			ScopedProfiler(ProfileDataSource::Ptr statementData, ApiProviderBase::Holder* holder_);
			~ScopedProfiler() override;
		};

		/** Subclass a thread from this and then periodically call checkRecording(). */
		struct RecordingSession
		{
			RecordingSession(DebugSession& session_, ThreadIdentifier::Type type);
			~RecordingSession();

			RecordingState getCurrentRecordingState() const { return nextState; }

			void setRecordingState(RecordingState state, ApiProviderBase::Holder* h);

			void checkRecording();

			bool isDataSource(DebugInformationBase::Ptr firstChildOfCombinedRun) const;

			ThreadIdentifier getThread() const { return recordedThread; }

			void initIfEmpty(ThreadIdentifier t);

			Ptr getDataSource() { return threadData; }

		private:

			const ThreadIdentifier::Type type;
			std::atomic<RecordingState> nextState = RecordingState::Idle;

			bool enabled = true;

			struct Recorder: public DebugSession::ProfileDataSource::Profiler
			{
				Recorder(RecordingSession& parent_);
				RecordingSession& parent;
			};

			DebugSession& session;
			DebugSession::ProfileDataSource::Ptr threadData;
			WeakReference<ApiProviderBase::Holder> holder;
			ThreadIdentifier recordedThread;
			Recorder currentRecorder;
			double recordingStart = 0.0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(RecordingSession);
		};

		struct ProfileInfoBase: public DebugInformationBase
		{
			using Ptr = ReferenceCountedObjectPtr<ProfileInfoBase>;

			ProfileInfoBase(DebugSession& d_):
			  d(d_)
			{};

			virtual ~ProfileInfoBase() {}

			virtual void gotoLocation(Component* clickedComponent) {};

			virtual SerialisedData::Ptr toSerialisedData(ProfileDataSource::List& existingSources, DataItem::List& existingData) const;

			String toBase64() const;

			static ProfileInfoBase::Ptr fromBase64(const String& b64, DebugSession& h);

			static ProfileInfoBase::Ptr combineRuns(DebugSession& session, Ptr rootInfo, Ptr infoItem);

			static ProfileInfoBase::Ptr trimToCurrentRange(DebugSession& session, Range<double> currentRange, ProfileInfoBase::Ptr rootInfo);

			virtual void fromSerialisedData(SerialisedData::Ptr v_, const ProfileDataSource::List& existingSources, const DataItem::List& existingData, ProfileDataSource::Ptr parent, ApiProviderBase::Holder* h);

			virtual ProfileDataSource::Ptr getSource() const { return nullptr; }

			virtual Identifier getProfileId() const = 0;

			String getIconPath() const override { return "profile"; }

			Component* createPopupComponent(const MouseEvent& e, Component* componentToNotify) override;
			Component* createComponent();

			void addChild(ProfileInfoBase::Ptr p);

			ProfileInfoBase* getRoot();

			virtual bool canBeOptimized() const { return false; }

			void optimize();

			int getNumChildElements() const override { return children.size(); }
			DebugInformationBase::Ptr getChildElement(int index) override { return children[index]; }

			DebugSession& d;
			WeakReference<ProfileInfoBase> parent;
			ReferenceCountedArray<ProfileInfoBase> children;

			JUCE_DECLARE_WEAK_REFERENCEABLE(ProfileInfoBase);
		};

		struct ProfileInfo: public ProfileInfoBase
		{
			ProfileInfo(DebugSession& d_):
			  ProfileInfoBase(d_)
			{};

			String getTextForValue() const override;
			String getTextForName() const override { return String(data.delta) + " ms"; }
			
			ProfileDataSource::Ptr getSource() const override { return data.source; }

			void gotoLocation(Component* clickedComponent) override;

			bool canBeOptimized() const override;

			SerialisedData::Ptr toSerialisedData(ProfileDataSource::List& existingSources, DataItem::List& existingData) const override;

			void fromSerialisedData(SerialisedData::Ptr v_, const ProfileDataSource::List& existingSources, const DataItem::List& existingList, ProfileDataSource::Ptr parent, ApiProviderBase::Holder* h);

			Identifier getProfileId() const override { return ProfileIds::profile; }
			
			Data data;
			
			void generateHeatMap(std::map<int, double>& heatmap);
		};

		struct CombinedRoot: public ProfileInfoBase
		{
			CombinedRoot(DebugSession& d_, ThreadIdentifier::Type t);;

			ThreadIdentifier::Type threadType;
			ProfileDataSource::Ptr sourceForRuns;

			SerialisedData::Ptr toSerialisedData(ProfileDataSource::List& existingSources, DataItem::List& existingData) const override;
			void fromSerialisedData(SerialisedData::Ptr v_, const ProfileDataSource::List& existingSources, const DataItem::List& existingData, ProfileDataSource::Ptr parent, ApiProviderBase::Holder* h) override;

			Identifier getProfileId() const override { return ProfileIds::root; }
		};

		struct MultiThreadSession: public ProfileInfoBase
		{
			using Ptr = ReferenceCountedObjectPtr<MultiThreadSession>;

			MultiThreadSession(DebugSession& d):
			  ProfileInfoBase(d)
			{};

			void fromSerialisedData(SerialisedData::Ptr v_, const ProfileDataSource::List& existingSources, const DataItem::List& existingData, ProfileDataSource::Ptr parent, ApiProviderBase::Holder* h) override;

			Identifier getProfileId() const override { return ProfileIds::multithread; }
		};

		ProfileDataSource();;
		~ProfileDataSource() override {};

		static ProfileDataSource::Ptr fromSerialisedData(SerialisedData::Ptr d_);

		void writeIntoSerialisedData(SerialisedData::Ptr d_) const;

		WeakReference<ApiProviderBase::Holder> holder;
		std::function<void(ApiProviderBase::Holder*, DebugInformationBase::Ptr info)> onMessageFlush;
		String name;
		String locationString;
		uint64_t currentRunIndex = 0;
		uint64_t loopCounter = 0;
		Range<int> lineRange;
		TimeDomain preferredDomain = TimeDomain::Undefined;
		bool isLoop = false;
		ThreadIdentifier::Type threadRoot = ThreadIdentifier::Type::Undefined;
		
		Colour colour = Colours::transparentBlack;
		SourceType sourceType = SourceType::Undefined;

		double durationThreshold = 0.0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProfileDataSource);
	};

	using GotoLocationCallback = std::function<void(Component*, ProfileDataSource::SourceType, String)>;

	DebugSession(PooledUIUpdater* updater);;
	~DebugSession();

	ApiProviderBase* getProviderBase() override;

	int getNumDebugObjects() const override { return sessions.size() + flushedSessions.size(); }
	void getColourAndLetterForType(int type, Colour& colour, char& letter) override;
	DebugInformationBase::Ptr getDebugInformation(int index) override;

	bool addDataItem(DataItem::Ptr newItem);
	void clearData(ApiProviderBase::Holder* p);

	DebugInformationBase* startSession(ApiProviderBase::Holder* p, const String& sessionId);

	void popSession();
	bool isActive() const;

	void startScopedProfile(ProfileDataSource::Ptr newParent);
	void pushProfileInfo(ProfileDataSource::Ptr dataToPop);

	DebugInformationBase::Ptr getLastProfileRoot(ThreadIdentifier::Type) const;

	void startRecording(double milliSeconds, ApiProviderBase::Holder* h);
	void stopRecording();
	void initUIThread();
	Component* createPopupViewer(const DebugInformationBase::Ptr ptr);
	

	LambdaBroadcaster<bool> syncRecordingBroadcaster;
	LambdaBroadcaster<ProfileDataSource::ProfileInfoBase::Ptr> recordingFlushBroadcaster;

	void addAudioThread(ThreadIdentifier t);
	void checkAudioThreadRecorders();

	void checkMouseClickProfiler(bool isDown);

	DebugSession* getDebugSession() override { return this; }
	Component* createMultiViewer();

	bool isRecordingMultithread() const;
	TriggerType getTriggerType() const { return currentOptions.trigger; }

	void setTriggerType(TriggerType newTriggerType)
	{
		currentOptions.trigger = newTriggerType;
		optionBroadcaster.sendMessage(sendNotificationSync, &currentOptions);
	}

	int openTrackEvent();
	void closeTrackEvent(int id);

	void setGotoCallback(const GotoLocationCallback& f);

	std::function<double()> getBufferDuration;

	struct Options
	{
		Options()
		{
			for(int i = 0; i < (int)ProfileDataSource::SourceType::numSourceTypes; i++)
			{
				auto t = (ProfileDataSource::SourceType)i;
				eventFilter.push_back(t);
			}

			for(int i = 0; i < (int)ThreadIdentifier::Type::numTypes; i++)
			{
				auto t = (ThreadIdentifier::Type)i;
				threadFilter.push_back(t);
			}
		}

		void writeToObject(DynamicObject::Ptr obj)
		{
			Array<var> filters;

			for(auto tv: threadFilter)
				filters.add(DebugSession::ThreadIdentifier::getThreadName(tv));

			Array<var> eventFilters;

			for(auto& ev: eventFilter)
				eventFilters.add(DebugSession::ProfileDataSource::getSourceTypeName(ev));

			obj->setProperty("threadFilter", var(filters));
			obj->setProperty("eventFilter", var(eventFilters));
			obj->setProperty("recordingLength", String((int)millisecondsToRecord) + " ms");
			obj->setProperty("recordingTrigger", (int)trigger);
		}

		static Options fromDynamicObject(DynamicObject::Ptr obj)
		{
			auto ev = obj->getProperty("eventFilter");
			auto tv = obj->getProperty("threadFilter");

			Options newOptions;

			newOptions.eventFilter.clear();
			newOptions.threadFilter.clear();
			newOptions.trigger = (TriggerType)(int)obj->getProperty("recordingTrigger");
			newOptions.millisecondsToRecord = obj->getProperty("recordingLength").toString().getDoubleValue();

			if(ev.isArray())
			{
				for(auto& v: *ev.getArray())
				{
					auto n = v.toString();
					auto found = false;

					for(int i = 0; i < (int)ProfileDataSource::SourceType::numSourceTypes; i++)
					{
						auto t = (ProfileDataSource::SourceType)i;

						if(ProfileDataSource::getSourceTypeName(t) == n)
						{
							newOptions.eventFilter.push_back(t);
							found = true;
						}
					}

					jassert(found);
				}
			}

			newOptions.eventFilter.push_back(ProfileDataSource::SourceType::Undefined);
			newOptions.eventFilter.push_back(ProfileDataSource::SourceType::RecordingSession);

			if(tv.isArray())
			{
				for(auto& v: *tv.getArray())
				{
					auto n = v.toString();
					auto found = false;

					for(int i = 0; i < (int)ThreadIdentifier::Type::numTypes; i++)
					{
						auto t = (ThreadIdentifier::Type)i;

						if(ThreadIdentifier::getThreadName(t) == n)
						{
							newOptions.threadFilter.push_back(t);
							found = true;
						}
					}

					jassert(found);
				}
			}

			return newOptions;
		}

		TriggerType trigger = TriggerType::Manual;
		double millisecondsToRecord = 1000.0;

		std::vector<ThreadIdentifier::Type> threadFilter;
		std::vector<ProfileDataSource::SourceType> eventFilter;
	};

	const Options& getOptions() const { return currentOptions; }

	void setOptions(const var& jsonObject)
	{
		currentOptions = Options::fromDynamicObject(jsonObject.getDynamicObject());
		optionBroadcaster.sendMessage(sendNotificationAsync, &currentOptions);
	}

	bool isMidiTriggerEnabled() const { return currentOptions.trigger == TriggerType::MidiInput; }

	LambdaBroadcaster<Options*> optionBroadcaster;

private:

	Options currentOptions;

	GotoLocationCallback gotoLocation;

	ThreadIdentifier getCurrentThreadIdentifier() const;

	struct Session: public ItemBase
	{
		int getNumChildElements() const override { return valueGroup.size(); }
		Ptr getChildElement(int index) override { return valueGroup[index]; }
		virtual String getTextForName() const { return id; };
		String getCategory() const override { return "Session"; }
		int getType() const override { return 1; }
		String getTextForDataType() const override { return "Session"; }
		String getCodeToInsert() const override { return getTextForName(); }
		var getVariantCopy() const override;

		struct ValueGroup: public ItemBase
		{
			int getNumChildElements() const override;
			String getTextForName() const override;;
			String getCodeToInsert() const override;
			Ptr getChildElement(int index) override;
			var getVariantCopy() const override;
			int getType() const override { return 2; }
			String getCategory() const override { return "Group"; }
			String getTextForDataType() const override { return "Group"; }

			String label;
			std::pair<Identifier, int> location;
			ReferenceCountedArray<DataItem> dataItems;
		};

		ReferenceCountedArray<DebugInformationBase> profileInfo;
		ReferenceCountedArray<ValueGroup> valueGroup;
		String id;
	};

	Session* getCurrentSession();

	void initAudioThreadAndMessageThreadRecorders();

	void timerCallback() override;

	struct Event
	{
		enum EventType: uint8
		{
			EmptyEvent,
			PtrEvent,
			TrackEvent,
			DataItemEvent
		};

		struct Queue
		{
			Queue(ThreadIdentifier thread_):
			  queue(QueueSize),
			  thread(thread_)
			{
				stack.reserve(StackSize);
			};

			int getNumEventsInQueue() const { return queue.size(); }

			void push(Event& e)
			{
				queue.push(e);
			}

			ThreadIdentifier thread;
			LockfreeQueue<Event> queue;
			std::vector<ReferenceCountedObjectPtr<ProfileDataSource::ProfileInfo>> stack;

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Queue);
		};

		Event(ProfileDataSource::Ptr data_, double startTime, bool push):
		  time(static_cast<double>(Time::getMillisecondCounterHiRes()) * (push ? 1.0 : -1.0) ),
		  ptr(data_),
		  typeId(EventType::PtrEvent),
		  value(0)
		{}

		Event(EventType eventType, double startTime, int eventValue, bool push) noexcept:
		  time(static_cast<double>(Time::getMillisecondCounterHiRes()) * (push ? 1.0 : -1.0)),
		  ptr(nullptr),
		  typeId(eventType),
		  value(eventValue)
		{}

		Event() noexcept:
		  typeId(EventType::EmptyEvent),
		  ptr(nullptr),
		  value(0),
		  time(0.0f)
		{};

		operator bool() const
		{
			if(typeId == EventType::EmptyEvent)
				return false;

			if(isPtrEvent())
				return ptr != nullptr;

			return true;
		}

		bool isValueEvent() const { return *this && typeId != EventType::PtrEvent; }
		bool isPtrEvent() const noexcept { return typeId == EventType::PtrEvent; }
		bool isPush() const { return time > 0.0; }
		double getTime() const { return (double)std::abs(time); }
		int getValue() const { jassert(isValueEvent()); return static_cast<int>(value); }
		EventType getEventType() const { jassert(isValueEvent()); return typeId; }
		ProfileDataSource::Ptr getData() const { return ptr; }

	private:

		ProfileDataSource::Ptr ptr;
		EventType typeId;
		int16 value;
		double time;
	};
	static constexpr int x = sizeof(Event);
	int trackCounter = 0;

	void processEventQueue();
	bool isEventQueueEmpty();

	CriticalSection recordingLock;

	ThreadIdentifier uiThread;
	ThreadIdentifier timerThread;

	std::vector<uint32> openTracks;

	OwnedArray<Event::Queue> multithreadedEventQueue;
	ReferenceCountedObjectPtr<ProfileDataSource::MultiThreadSession> currentMultithreadSession;
	ReferenceCountedArray<ProfileDataSource::CombinedRoot> combinedRuns;
	std::vector<std::pair<ThreadIdentifier, ReferenceCountedObjectPtr<ProfileDataSource::ProfileInfo>>> flushedRoots;

	ReferenceCountedArray<Session> sessions;
	ReferenceCountedArray<Session> flushedSessions;
	Array<WeakReference<ProfileDataSource::RecordingSession>> availableRecordingSessions;
	ScopedPointer<ProfileDataSource::RecordingSession> messageThreadRecorder;
	OwnedArray<ProfileDataSource::RecordingSession> audioThreadRecorders;
	UnorderedStack<ThreadIdentifier, 16> registeredAudioThreads;
	DataItem::List recordedDataItems;

	std::atomic<RecordingState> nextState { RecordingState::Idle };
	WeakReference<ApiProviderBase::Holder> recordHolder;
	double recordingStart= 0.0;
	double recordingDelta = 0.0;
};


} // namespace hise

