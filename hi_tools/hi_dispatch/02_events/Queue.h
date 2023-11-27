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

namespace hise {
namespace dispatch {	
using namespace juce;





class Logger;




class Queue final : public Queueable
{
public:

	enum class FlushType
	{
		Flush,
		KeepData,
		numFlushTypes
	};

	static constexpr size_t MaxQueueSize = 1024 * 1024 * 4; // 4MB should be enough TODO: add dynamic upper limit with warning

	HashedCharPtr getDispatchId() const override { return HashedCharPtr("queue"); }

	struct ScopedExplicitSuspender
	{
		ScopedExplicitSuspender(Queue& q):
		  queue(q)
		{
			prevState = queue.getState();
			queue.setQueueState(State::Paused);
		}

		~ScopedExplicitSuspender()
		{
			queue.setQueueState(prevState);
		}

		State prevState;
		Queue& queue;
	};

	struct QueuedEvent
	{
		Queueable* source = nullptr;
		uint16 numBytes = 0;
		EventType eventType = EventType::Nothing;

		explicit operator bool() const { return source != nullptr || eventType == EventType::Nothing; }

		size_t getTotalByteSize() const;;
		static uint8* getValuePointer(uint8* ptr);
		size_t write(uint8* ptr, const void* dataValues) const;
		static QueuedEvent fromData(uint8* ptr);
	};

	/** An iterator object that walks through the data of the queue. */
	struct Iterator
	{
		explicit Iterator(Queue& queueToIterate);;
		bool next(QueuedEvent& e);

		/** Returns the position of the next element. */
		uint8* getNextPosition() const noexcept { return ptr;}

		/** Returns the position of the current element. If the iteration hasn't start, it's nullptr. */
		uint8* getPositionOfCurrentQueuable() const noexcept { return lastPos; }

		/** Rewinds the iterator and sets the last position to nullptr. */
		void rewind();

		/** Seeks the iterator to the position. */
		bool seekTo(size_t position);

	private:
		Queue& parent;
		uint8* ptr = nullptr;
		uint8* lastPos = nullptr;
		uint8* startPos = nullptr;
	};

	/** Packed data structure supplied to the flush function. */
	struct FlushArgument
	{
		template <typename T> T& getTypedObject() const
		{
			static_assert(std::is_base_of<Queueable, T>(), "not a base of Queueable");
			auto t = dynamic_cast<T*>(source);

			// should never be nullptr!
			jassert(t != nullptr);
			return *t;
		}

		Queueable* source = nullptr;
		uint8* data = nullptr;
		EventType eventType = EventType::Nothing;
		uint16 numBytes = 0;
	};

	using DataType = uint8;

	// The function that will be used to flush the queue. Arguments:
	// - a pointer to a Queueable
	// - a event type (used for prioritizing (TODO))
	// - the data to store
	// - the number of bytes that the data has
	using FlushFunction = std::function<bool(const FlushArgument&)>;

	/** Creates a queue and allocates the given amount of bytes. */
	Queue(RootObject& root, size_t initAllocatedSize);

	~Queue() override;

	/** Checks that the allocated storage is enough for the next message.
	 *  If not, it prints a warning (TODO)
	 *  and allocates to the next power of two until the MaxQueueSize is reached
	 */
	bool ensureAllocated(size_t numBytesRequired);;

	/** Checks if the queue is empty. */
	bool isEmpty() const noexcept { return numUsed == 0; }

	/** Checks the amount of allocated bytes. */
	size_t getNumAllocated() const noexcept { return numAllocated; }

	size_t size() const noexcept { return numElements; }

	/** pushes a event to the queue. */
	bool push(Queueable* s, EventType t, const void* values, size_t numValues);

	/** flushes the queue with the given function. If the function returns FALSE, it will abort the iteration and clean the remaining queue. */
	bool flush(const FlushFunction& f, FlushType flushType);

	/** Attaches a logger to the queue. non-owned, lifetime of logger > queue. */
	void setLogger(Logger* l);

	/** Called by the destructor of the queuable and ensures that its removed from this queue. */
	void invalidateQueuable(Queueable* q, DanglingBehaviour behaviour);

	/** Removes the first matching value in the queue and closes the gap. Returns true if something was removed.
	 *
	 *	The offset parameter can be used to seek to the current position (if it is known).
	 */
	bool removeFirstMatchInQueue(Queueable* q, size_t offset=0);

	/** Removes all matching objects. */
	bool removeAllMatches(Queueable* q);

	void setOverrideDanglingBehaviour(DanglingBehaviour forcedBehaviour) noexcept { queueBehaviour = forcedBehaviour; }

	/** Clears the queue (just moves the pointer to the start, O(1) operation. */
	void clear() { numUsed = 0; numElements = 0; }

	void addPushCheck(const std::function<bool(Queueable*)>& pc) { pushCheckFunction = pc; }

	void setQueueState(State newState);

	void resumeAfterPause()
	{
#if ENABLE_DISPATCH_QUEUE_RESUME
		if(resumeData != nullptr)
		{
			if(flush(resumeData->f, resumeData->flushType))
				resumeData = nullptr;
		}
#endif
	}

	State getState() const { return explicitState.load(); }

private:

	bool flushPending = false;

	std::atomic<State> explicitState = { State::Running };

	std::function<bool(Queueable*)> pushCheckFunction;

#if ENABLE_DISPATCH_QUEUE_RESUME

	struct ResumeData
	{
		size_t offset = 0;
		FlushType flushType = FlushType::Flush;
		FlushFunction f;
	};
	
	ScopedPointer<ResumeData> resumeData;

#endif

	FlushArgument createFlushArgument(const QueuedEvent& e, uint8* eventPos) const noexcept;

	void clearPositionInternal(uint8* start, uint8* end);

	static uint64 alignedToPointerSize(uint64 N);
	static bool isAlignedToPointerSize(uint8* ptr);

	HeapBlock<uint8> data;
	size_t numUsed = 0;		// the amount of bytes used
	size_t numAllocated = 0;	// the amount of bytes allocated
	size_t numElements = 0;	// the amount of events in the queue
	DanglingBehaviour queueBehaviour = DanglingBehaviour::Undefined; // overrides the incoming behaviour request if not undefined

	Logger* attachedLogger = nullptr;
	bool logRecursion = false;
};





} // dispatch
} // hise