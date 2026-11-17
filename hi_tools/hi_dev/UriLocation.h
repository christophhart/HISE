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
using namespace juce;

/** A struct containing information about location within a codebase. */
struct UriLocation
{

	/** A opaque handler interface for resolving the location. This is used for internal callbacks that do
	    not correspond to a code file.
	*/
	struct Owner
	{
		virtual ~Owner() {};
		virtual String getUriProcessorId() const = 0;
		virtual std::pair<bool, String> getUriContent(const String& uri) const = 0;
	};

	using WithExpression = std::pair<String, UriLocation>;
	using Callstack = std::vector<WithExpression>;

	enum ResolveState
	{
		PathResolved     = 0x01,
		LineResolved     = 0x02,
		RawCharIndex     = 0x04,
		LocationResolved = 0x06,
		FullyResolved    = 0x07,
		numResolveStates
	};

	enum class UriType
	{
		onInit,
		ExternalFile,
		OtherCallback
	};

	enum class UriFormat
	{
		Callback,   // onInit(), onNoteOn(), fullPath
		CallbackId, // onInit, onNoteOn, fullFilePath
		FilePath,
		RelativePath,
		ConnectionId, // used by PanelWithProcessorConnection::indexList
		LineAndCol,
		Compact, // id(lineNumber)
		Base64,
		GotoString
	};

	virtual ~UriLocation() {};

	operator bool() const { return line != -1 || col != -1 || charIndex != -1 || uri.isNotEmpty(); }

	virtual var toJSON() const;
	static UriLocation fromEncodedString(const String& base64String);

	bool fillLineAndCol(const juce::CodeDocument& doc);
	bool hasResolvedLineAndColNumbers() const;

	File getFile(const File& rootDir) const;
	UriType getUriType() const;
	virtual Range<int> getLineRange() const { return { line, line + 1 }; }
	String toLocationString(UriFormat f) const;
	ResolveState getResolveState() const;

	bool matchesFile(const File& f) const;
	virtual bool matches(const UriLocation& other) const;

	bool sameUriAndProcessorId(const UriLocation& other) const;

	void fillLocation(const UriLocation& other);
	void fillLocation(const CodeDocument::Position& pos);
	void fillLocation(const var& uriObj);

	int line = -1;
	int col = -1;
	int charIndex = -1;
	String uri;
	String processorId;
};


/** An extension to the UriLocation that adds a String message as well as a call stack frame.

	This is used for resolving any kind of diagnostics, errors or runtime events.
*/
struct UriError : public UriLocation
{
	enum ErrorType
	{
		OK         = 0x00,
		Unknown    = 0x01,
		Error      = 0x02,
		Warning    = 0x04,
		Info       = 0x08,
		Hint       = 0x10,
		Breakpoint = 0x20,
		Blink      = 0x40,
		BreakBlink = Breakpoint | Blink
	};

	enum SourceType
	{
		None          = 0x00,
		Compilation   = 0x01, // a non-recoverable error occuring during compilation
		Runtime       = 0x02, // a non-recoverable error occuring during callback execution
		Diagnostics   = 0x04, // recoverable errors / warnings from shadow parsing
		Debug         = 0x08, // breakpoints / blink messages
		Log           = 0x08, // inplace debug values from Console.print, must be Error::Info
		Include		  = 0x10, // include parsing
		All           = 0x1F
	};

	static Colour getColour(ErrorType t);

	static bool sourceAndErrorTypeMatches(SourceType s, ErrorType e);

	static UriError ok();
	static UriError fail(const String& error);
	static UriError fromResult(const Result& r, Owner* owner);

	bool wasOk() const;
	bool failed() const;

	String toConsoleMessage() const;
	String toFullCallStack() const;
	var toJSON() const override;

	UriError withProcessorId(Owner* owner) const;
	UriError withBaseCallstack(const UriLocation::WithExpression& rootStack) const;
	void forInvertedCallstack(const std::function<void(int, const UriLocation::WithExpression&)> & f);

	ErrorType errorType = ErrorType::OK;
	SourceType sourceType = SourceType::None;

	String errorMessage;
	var metadata;
	UriLocation::Callstack callstack;
};

/** An extension to the location that contains information / data about a breakpoint.

	Note that this is not the actual breakpoint event, which is a UriError with ErrorType::Break!
*/
struct UriBreakpoint : public UriLocation
{
	UriBreakpoint():
	  hash(Random::getSystemRandom().nextInt())
	{}

	static UriBreakpoint fromJSON(const var& obj);

	struct Owner : public UriLocation::Owner
	{
		enum class NextBreakpointAction
		{
			Resume = 0,
			StepOver = 1,
			StepInto,
			StepOut,
			numNextActions
		};

		enum class AttachState
		{
			OK = 0,      // could be attached properly
			Duplicate,   // breakpoint at the given statement already exists
			Unresolved,  // couldn't be resolved (yet, wait for onAttachState change)
			Stale,       // the breakpoint is stale until recompilation
		};

		Owner() = default;
		~Owner() override;

		struct UriBreakpointListener
		{
			enum class ChangeType
			{
				Added,
				Removed,
				Modified,
				AttachStateChange
			};

			virtual ~UriBreakpointListener();
			void setOwner(Owner* o);

			virtual void onUriBreakpointChange(const UriBreakpoint& bp, ChangeType changeType) {};
			virtual void onUriBreakpointClear() {};
			virtual void onUriBreakpointAttachTypeChange(const UriBreakpoint& bp, AttachState newAttachState) {};
			virtual void onUriBreakpointEvent(const UriBreakpoint& knownBreakpoint, const UriError& breakpointEvent) {};

			virtual bool matchesUriLocation(const UriLocation& bp) const = 0;

			virtual bool matchesUriBreakpointEvent(const UriError& e) const;

		protected:

			WeakReference<Owner> owner;

		private:

			JUCE_DECLARE_WEAK_REFERENCEABLE(UriBreakpointListener);
		};

		// Breakpoint methods =========================================================================

		/** Adds a breakpoint to this owner and sends a message to its registered listeners. */
		std::pair<AttachState, UriBreakpoint> addUriBreakpoint(const UriBreakpoint& newBreakpoint, NotificationType n);

		/** Call this to remove a breakpoint from this owner. */
		bool removeUriBreakpoint(const UriBreakpoint& newBreakpoint, NotificationType n);

		bool modifyUriBreakpoint(const UriBreakpoint& bpToModify, NotificationType n);

		/** Removes all breakpoints and sends a onClear message. */
		void clearBreakpoints(NotificationType n);

		/** Call this when a breakpoint occurs. */
		AttachState sendUriBreakpointEvent(const UriError& breakpointEvent);

		// Virtual methods =========================================================================

		/** Override this and react on all breakpoints being cleared. */
		virtual void onBreakpointClear();

		/** Will be called whenever a breakpoint was added / removed. */
		virtual void onUriBreakpointChange(const UriBreakpoint& b, UriBreakpointListener::ChangeType wasAdded) {}

		/** Will be called whenever a breakpoint attach type was changed. */
		virtual void onUriBreakpointAttachTypeChange(const UriBreakpoint& bp, AttachState newAttachState) {};

		virtual void onUriBreakpointEvent(const UriBreakpoint& knownBreakpoint, const UriError& breakpointEvent) {};

		/** Override this method and return t if the breakpoint could be installed. */
		virtual std::pair<AttachState, UriBreakpoint> attachBreakpoint(const UriBreakpoint& bp) = 0;

		/** Override this method and implement moving the instruction pointer to the given location. 
		
			If the instruction marker needs to be moved to another location (eg. clipped to the current
			function, return the actual location of the new pointer. 
		*/
		virtual UriLocation moveInstructionPointer(const UriBreakpoint& newLocation) = 0;

		/** Override this method and implement the logic that handles the breakpoint suspension. */
		virtual void stopOnBreakpoint(const UriError& e) = 0;

		/** Override this method and implement the logic that handles the resuming of the execution. */
		virtual void resumeExecution(NextBreakpointAction n) = 0;

		// Listener methods =========================================================================

		void addUriBreakpointListener(UriBreakpointListener* bp);
		void removeUriBreakpointListener(UriBreakpointListener* bp);

		bool setBreakpointState(const std::pair<AttachState, UriBreakpoint>& newData, NotificationType n);

	protected:

		Array<UriBreakpoint> resetAttachmentAndCreateBreakpointList();

	private:

		void sendBreakpointUpdate(const std::pair<AttachState, UriBreakpoint>& bp, UriBreakpointListener::ChangeType ct, NotificationType n);

		UriError currentBreakpointEvent;
		Array<std::pair<UriBreakpoint, int>> touchedBreakpoints;

		Array<std::pair<AttachState, UriBreakpoint>> currentBreakpoints;
		Array<WeakReference<UriBreakpointListener>> listeners;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Owner);
		JUCE_DECLARE_WEAK_REFERENCEABLE(Owner);
	};

	bool matches(const UriBreakpoint& other) const
	{
		if (other.hash == hash)
			return true;

		UriLocation sliced(other);
		return matches(sliced);
	}

	bool matches(const UriLocation& l) const override
	{
		if (l.getResolveState() & ResolveState::LineResolved &&
			  getResolveState() & ResolveState::LineResolved)
		{
			return sameUriAndProcessorId(l) && l.line == line;
		}

		return UriLocation::matches(l);
	}

	var toJSON() const override;

	String expression = "true";
	bool enabled = true;
	bool breakOnHit = true;
	bool blinkOnHit = false;

private:

	int hash;
};

/** An extension to the location that builds up a node tree of locations.

	Unlike the other extensions, this object is supposed to live on the heap
	and is ref-counted. 

	Creating a location tree node automatically registers it to its parent 
	(if provided) and the toJSON() function creates a recursive data structure covering
	the entire tree.

	It also has the ability to hold a end location which will then define a range within
	the code that is covered by this tree node. Any child node must then stay within the 
	given range or checkSanity() will fail.
*/
struct UriLocationTree : public UriLocation,
						 public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<UriLocationTree>;
	using List = ReferenceCountedArray<UriLocationTree>;
	using WeakPtr = WeakReference<UriLocationTree>;

	UriLocationTree(WeakPtr parent_);

	void setParent(WeakPtr newParent);

	void cleanup(const std::function<bool(const UriLocationTree& item)>& f);
	bool forEach(const std::function<bool(const UriLocationTree& item)>& f) const;
	bool forEachParent(const std::function<bool(const UriLocationTree&)>& f) const;
	bool forEachChildWithCategory(const Identifier& childCategory, const std::function<bool(const UriLocationTree&)> f) const;
	bool forEachLeafChild(const std::function<bool(const UriLocationTree& t)>& f) const;


	bool containsLocation(const UriLocation& other) const;

	void setToStringFunction(const std::function<String(const UriLocationTree&)>& f);

	String getAsString() const;

	var toJSON() const override;
	WeakPtr getParent() const { return parent; }
	const UriLocationTree* getRoot() const;
	int getNumChildren() const;

	/** Returns the first subtree that matches the given location. 
	*   
	*   If position is true then it requires the subtree to contain the actual charIndex / line
	*   otherwise it just checks processorId and uri
	*/
	Ptr getSubTree(const UriLocation& l, bool checkPosition) const;

	Ptr getSubTreeWithId(const Identifier& id, bool lookRecursive) const;

	/** Returns the most inner tree that contains this location. */
	Ptr getInnerTree(const UriLocation& l) const;

	WeakPtr getThisOrParentWithCategory(const Array<Identifier>& ids) const
	{
		if(ids.contains(category))
			return const_cast<UriLocationTree*>(this);

		if(parent != nullptr)
			return parent->getThisOrParentWithCategory(ids);

		return nullptr;
	}

	Colour getIconColour() const;

	UriLocationTree::Ptr getChild(int index) const;
	UriError checkSanity() const;

	bool hasRange() const { return (bool)closeLocation; }
	bool matchesCategory(const Identifier& c) const { return category == c; }
	bool matchesId(const Identifier& otherId) const { return id == otherId; }

	Range<int> getLineRange() const override;

	DynamicObject::Ptr data;
	Identifier category;
	Identifier id;
	UriLocation closeLocation;

	UriLocationTree** begin() const { return const_cast<UriLocationTree**>(children.begin()); }
	UriLocationTree** end() const { return begin() + children.size(); }

	UriLocation getTokenLocation() const
	{
		if (tokenLocation)
			return tokenLocation;

		return *this;
	}

	void setTokenLocation(const UriLocation& tk)
	{
		tokenLocation = tk;
	}

private:

	UriLocation tokenLocation;

	UriError createError(const UriLocation& errorLocation, const String& errorMessage) const;

	std::function<String(const UriLocationTree&)> toStringFunction;
	List children;
	WeakPtr parent;

	JUCE_DECLARE_WEAK_REFERENCEABLE(UriLocationTree);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UriLocationTree);

};

/** A simple base manager class that provides an interface for communicating
	error events across the application. 
	
*/
struct UriErrorLogger: public UriLocation::Owner,
					   public AsyncUpdater
{
	using SourceType = UriError::SourceType;

	struct Listener
	{
		virtual ~Listener() = default;

		virtual void onError(const UriError& e, int numOccurrences) = 0;
		virtual void onClear(SourceType s) = 0;
		virtual void onLocationTree(UriLocationTree::Ptr newTree) {};

		virtual int getSourceTypeMask() const { return SourceType::All; }

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	UriErrorLogger();

	virtual ~UriErrorLogger();;

	virtual void clearUriErrors(SourceType s, NotificationType n = sendNotificationAsync);
	virtual void logUriError(SourceType s, const UriError& e, NotificationType n = sendNotificationAsync);
	virtual void logLocationTree(UriLocationTree::Ptr p, NotificationType n = sendNotificationAsync);

	UriLocationTree::Ptr getCurrentScopeTree() const { return lastTree; }

	void addListener(Listener* l, bool initWithErrors);
	void removeListener(Listener* l);

	int getNumErrors(SourceType s) const;

	std::vector<std::pair<UriError, int>> getErrorList(SourceType s) const;

protected:

	Array<WeakReference<Listener>> listeners;

	UriLocationTree::Ptr lastTree;

private:

	ReadWriteLock lock;
	std::map<SourceType, std::vector<std::pair<UriError, int>>> data;
};


}