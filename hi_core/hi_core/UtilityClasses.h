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

#ifndef UTILITYCLASSES_H_INCLUDED
#define UTILITYCLASSES_H_INCLUDED

namespace hise { using namespace juce;

class Processor;
class MainController;





/** A base class for objects that need to call dispatched messages. */
struct Dispatchable
{
	enum class Status
	{
		OK = 0,
		notExecuted,
		needsToRunAgain,
		cancelled
	};

	using Function = std::function<Status(Dispatchable* obj)>;

	virtual ~Dispatchable() {};

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(Dispatchable);
};

/** This class is used to coallescate multiple calls to an asynchronous update for a given Listener.
*	@ingroup event_handling
*
*	It is designed to be a replacement for the normal AsyncUpdater which can clog the message thread if too
*	many change notifications are sent.
*
*	In order to use this class, just create an instance and pass this to your subclassed listeners, which then
*	can be used just like the standard AsyncUpdater from JUCE.
*/
class UpdateDispatcher : private Timer
{
public:

	UpdateDispatcher(MainController* mc_);;

	~UpdateDispatcher()
	{
		pendingListeners.clear();

		stopTimer();
	}

	void suspendUpdates(bool shouldSuspendUpdates)
	{
		if (shouldSuspendUpdates)
			stopTimer();
		else
			startTimer(30);
	}

	/** This class contains the sender logic of the UpdateDispatcher scheme.
	*
	*	In order to use it, subclass your object from this, register the parent UpdateDispatcher in 
	*	the constructor, and then use triggerAsyncUpdate() just like you would do with a normal AsyncUpdater
	*
	*/
	class Listener
	{
	public:

		Listener(UpdateDispatcher* dispatcher_);;

		virtual ~Listener()
		{
			masterReference.clear();
		};

		virtual void handleAsyncUpdate() = 0;

		void cancelPendingUpdate()
		{
			if (!pending)
				return;

			cancelled.store(true);
		}

		void triggerAsyncUpdate();

	private:

		std::atomic<bool> cancelled;
		std::atomic<bool> pending;

		friend class WeakReference<Listener>;
		WeakReference<Listener>::Master masterReference;

		friend class UpdateDispatcher;

		WeakReference<UpdateDispatcher> dispatcher;
	};

private:

	void triggerAsyncUpdateForListener(Listener* l);

	MultithreadedLockfreeQueue<WeakReference<Listener>, MultithreadedQueueHelpers::Configuration::AllocationsAllowedAndTokenlessUsageAllowed> pendingListeners;

	void timerCallback() override;

	friend class Listener;

	MainController* mc;

	JUCE_DECLARE_WEAK_REFERENCEABLE(UpdateDispatcher);
};

/** This class can be used to listen to ValueTree property changes asynchronously.
*
*	It uses the UpdateDispatcher class to coallescate multiple updates without clogging the message thread
*/
class AsyncValueTreePropertyListener : public ValueTree::Listener
{
public:

	AsyncValueTreePropertyListener(ValueTree state_, UpdateDispatcher* dispatcher_) :
		state(state_),
		dispatcher(dispatcher_),
		asyncHandler(*this)
	{
		pendingPropertyChanges.ensureStorageAllocated(1024);
		state.addListener(this);
	}

	void valueTreePropertyChanged(ValueTree& v, const Identifier& id) final override
	{
		pendingPropertyChanges.addIfNotAlreadyThere(PropertyChange(v, id));
		asyncHandler.triggerAsyncUpdate();
	};

	virtual void asyncValueTreePropertyChanged(ValueTree& v, const Identifier& id) = 0;

	void valueTreeChildAdded(ValueTree&, ValueTree&) override {}
	void valueTreeChildRemoved(ValueTree&, ValueTree&, int) override {}
	void valueTreeChildOrderChanged(ValueTree&, int, int) override {}
	void valueTreeParentChanged(ValueTree&) override {}

private:

	struct PropertyChange
	{
		PropertyChange(ValueTree v_, Identifier id_) : v(v_), id(id_) {};
		PropertyChange() {};

		bool operator==(const PropertyChange& other) const
		{
			return v == other.v && id == other.id;
		}

		ValueTree v;
		Identifier id;
	};

	struct AsyncHandler : public UpdateDispatcher::Listener
	{
		AsyncHandler(AsyncValueTreePropertyListener& parent_) :
			Listener(parent_.dispatcher),
			parent(parent_)
		{};

		void handleAsyncUpdate() override
		{
			while (!parent.pendingPropertyChanges.isEmpty())
			{
				auto pc = parent.pendingPropertyChanges.removeAndReturn(0);
				parent.asyncValueTreePropertyChanged(pc.v, pc.id);
			}
		}

		AsyncValueTreePropertyListener& parent;
	};

	ValueTree state;
	WeakReference<UpdateDispatcher> dispatcher;
	AsyncHandler asyncHandler;

	Array<PropertyChange, CriticalSection> pendingPropertyChanges;
};

template <int Offset, int Length> class StackTrace
{
public:

	StackTrace():
		id(0)
	{
		for (int i = 0; i < Length; i++)
			stackTrace[i] = {};
	}

	bool operator ==(const StackTrace& other) const noexcept
	{
		return id == other.id;
	}

	StackTrace(StackTrace&& other) noexcept
	{
		id = other.id;

		for (int i = 0; i < Length; i++)
			stackTrace[i].swap(other.stackTrace[i]);
	}

	StackTrace& operator=(StackTrace&& other) noexcept
	{
		id = other.id;

		

		for (int i = 0; i < Length; i++)
			stackTrace[i].swap(other.stackTrace[i]);

		return *this;
	}

	StackTrace(uint16 id_, bool createStackTrace=true):
		id(id_)
	{
		if (createStackTrace)
		{
			auto full = StringArray::fromLines(SystemStats::getStackBacktrace());

			for (int i = Offset; i < Offset + Length; i++)
			{
				stackTrace[i - Offset] = full.strings[i].toStdString();
			}
		}
		else
		{
			for (int i = 0; i < Length; i++)
				stackTrace[i] = {};
		}
		
	}

	uint16 id;
	std::string stackTrace[Length];

	JUCE_DECLARE_NON_COPYABLE(StackTrace);
};


/** A base class for all objects that can be saved as value tree.
*/
class RestorableObject
{
public:

	virtual ~RestorableObject() {};

	/** Overwrite this method and return a representation of the object as ValueTree. */
	virtual ValueTree exportAsValueTree() const = 0;

	/** Overwrite this method and restore the properties of this object using the referenced ValueTree. */
	virtual void restoreFromValueTree(const ValueTree &previouslyExportedState) = 0;
};

class MainController;

/** A base class for all objects that need access to a MainController.
*
*	If you want to have access to the main controller object, derive the class from this object and pass 
*	a pointer to the MainController instance in the constructor.
*/
class ControlledObject
{
public:

	/** Creates a new ControlledObject. The MainController must be supplied. */
	ControlledObject(MainController *m, bool notifyOnShutdown=false);

	virtual ~ControlledObject();

	/** Overwrite this and make sure that it stops accessing the main controller. */
	virtual void mainControllerIsDeleted() {};

	/** Provides read-only access to the main controller. */
	const MainController *getMainController() const noexcept
	{
		jassert(controller != nullptr);
		return controller;
	};

	/** Traverses the component hierarchy and returns the main controller from one of its parents. */
	static MainController* getMainControllerFromParent(Component* c)
	{
		if (auto co = c->findParentComponentOfClass<ControlledObject>())
		{
			return co->getMainController();
		}

		jassertfalse;
		return nullptr;
	}

	/** Provides write access to the main controller. Use this if you want to make changes. */
	MainController *getMainController() noexcept
	{
		jassert(controller != nullptr);
		return controller;
	}

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(ControlledObject);

	bool registerShutdown = false;

	MainController* const controller;

	friend class MainController;
	friend class ProcessorFactory;
};

/** A small helper class that detects a timeout.
*
*   Use this to catch down drop outs by adding it to time critical functions in the audio thread and set a global time out value 
*   using either setMaxMilliSeconds() or, more convenient, setMaxTimeOutFromBufferSize().
*
*   Then in your function create a object of this class on the stack and when it is destroyed after the function terminates it
*   measures the lifespan of the object and logs to the current system Logger if it exceeds the timespan.
*
*   The Identifier you pass in should be unique. It will be used to deactivate logging until you create a GlitchDetector with the same
*   ID again. This makes sure you only get one log per glitch (if you have multiple GlitchDetectors in your stack trace, 
*   they all would fire if a glitch occurred.
*
*   You might want to use the macro ADD_GLITCH_DETECTOR(name) (where name is a simple C string literal which will be parsed to a 
*   static Identifier, because it can be excluded for deployment builds using
*   
*       #define USE_GLITCH_DETECTION 0
*
*   This macro can be only used once per function scope, but this should be OK...
*/
class ScopedGlitchDetector
{
public:
    
    /** Creates a new GlitchDetector with a given identifier. 
    *
    *   It uses the id to only log the last call in a stack trace, so you only get the GlitchDetector where the glitch actually occured.
    */
    ScopedGlitchDetector(Processor* const processor, int location_);;
    
	~ScopedGlitchDetector();
    
    // =================================================================================================================================
    

	static double getAllowedPercentageForLocation(int locationId);

private:
    
	

    // =================================================================================================================================
    
	int location = 0;

	static double locationTimeSum[30];
	static int locationIndex[30];

    const double startTime;
    
    static int lastPositiveId;

	WeakReference<Processor> p;

    // =================================================================================================================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedGlitchDetector)
};


#if USE_GLITCH_DETECTION // && !JUCE_DEBUG
#define ADD_GLITCH_DETECTOR(processor, location) ScopedGlitchDetector sgd(processor, (int)location)
#else
#define ADD_GLITCH_DETECTOR(processor, loc)
#endif




/** Calculates the balance.
*	@ingroup utility
*
*/
class BalanceCalculator
{
public:

	/** Converts a balance value to the gain factor for the supplied channel using an equal power formula. Input is -100.0 ... 100.0 */
	static float getGainFactorForBalance(float balanceValue, bool calculateLeftChannel);;

	/** Processes a stereo buffer with an array of balance values (from 0...1) - typically the output of a modulation chain. 
	*
	*	This is slightly faster than calling getGainFactorForBalance because it uses some vectorization...
	*	The float array that is passed in is used as working buffer, so don't rely on it not being changed...
	*	
	*/
	static void processBuffer(AudioSampleBuffer &stereoBuffer, float *panValues, int startSample, int numSamples);

	/** Returns a string version of the pan value. */
	static String getBalanceAsString(int balanceValue);
};



class Saturator
{
public:

	Saturator()
	{
		setSaturationAmount(0.0f);
	};

	inline float getSaturatedSample(float inputSample) const
	{
		return (1.0f + k) * inputSample / (1.0f + k * fabsf(inputSample));
	}

	void setSaturationAmount(float newSaturationAmount)
	{
		saturationAmount = jmin(newSaturationAmount, 0.999f);
		
		k = 2 * saturationAmount / (1.0f - saturationAmount);
	}

private:

	float saturationAmount;
	float k;

};




class Processor;




/** A keyboard state which adds the possibility of colouring the keys. */
class CustomKeyboardState : public MidiKeyboardState,
	public SafeChangeBroadcaster
{
public:

	/** Creates a new keyboard state. */
	CustomKeyboardState() :
		MidiKeyboardState(),
		lowestKey(40)
	{
		for (int i = 0; i < 127; i++)
		{
			setColourForSingleKey(i, Colours::transparentBlack);
		}
	}

	/** Returns the colour for the given note number. */
	Colour getColourForSingleKey(int noteNumber) const
	{
		return noteColours[noteNumber];
	};

	/** Checks if a colour was specified for the given note number. */
	bool isColourDefinedForKey(int noteNumber) const
	{
		return noteColours[noteNumber] != Colours::transparentBlack;
	};

	/** Changes the colour for the given note number. */
	void setColourForSingleKey(int noteNumber, Colour colour)
	{
		if (noteNumber >= 0 && noteNumber < 127)
		{
			noteColours[noteNumber] = colour;
		}

		sendChangeMessage();
	}
	void setLowestKeyToDisplay(int lowestKeyToDisplay)
	{
		lowestKey = lowestKeyToDisplay;
	}


	int getLowestKeyToDisplay() const { return lowestKey; }

private:

	Colour noteColours[127];
	int lowestKey;

};



class MainController;

class AutoSaver : private Timer
{
public:

	

	AutoSaver(MainController *mc_) :
		mc(mc_),
		currentAutoSaveIndex(0)
	{
		
	};

	

	void updateAutosaving()
	{
		if (isAutoSaving()) enableAutoSaving();
		else disableAutoSaving();
	}

private:

	int getIntervalInMinutes() const;

	void enableAutoSaving()
	{
		IF_NOT_HEADLESS(startTimer(1000 * 60 * getIntervalInMinutes())); // autosave all 5 minutes
	}

	void disableAutoSaving()
	{
		stopTimer();
	}

	bool isAutoSaving() const;

	void timerCallback() override;

	File getAutoSaveFile();

	Array<File> fileList;

	int currentAutoSaveIndex;

	MainController *mc;
};



class SemanticVersionChecker
{
public:

	SemanticVersionChecker(const String& oldVersion_, const String& newVersion_)
	{
		parseVersion(oldVersion, oldVersion_);
		parseVersion(newVersion, newVersion_);
	};

	bool isUpdate() const;

	bool isMajorVersionUpdate() const { return newVersion.majorVersion > oldVersion.majorVersion; };
	bool isMinorVersionUpdate() const { return newVersion.minorVersion > oldVersion.minorVersion; };
	bool isPatchVersionUpdate() const { return newVersion.patchVersion > oldVersion.patchVersion; };
	bool oldVersionNumberIsValid() const { return oldVersion.validVersion; }
	bool newVersionNumberIsValid() const { return newVersion.validVersion; }

private:

	struct VersionInfo
	{
		bool validVersion = false;
		int majorVersion = 0;
		int minorVersion = 0;
		int patchVersion = 0;
	};

	static void parseVersion(VersionInfo& info, const String& v)
	{
		const String sanitized = v.replace("v", "", true);
		StringArray a = StringArray::fromTokens(sanitized, ".", "");

		if (a.size() != 3)
		{
			info.validVersion = false;
			return;
		}
		else
		{
			info.majorVersion = a[0].getIntValue();
			info.minorVersion = a[1].getIntValue();
			info.patchVersion = a[2].getIntValue();
			info.validVersion = true;
		}
	};

	VersionInfo oldVersion;
	VersionInfo newVersion;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SemanticVersionChecker);
};

class DelayedFunctionCaller: public Timer
{
public:

	DelayedFunctionCaller(std::function<void(void)> func, int delayInMilliseconds) :
		f(func)
	{
		startTimer(delayInMilliseconds);
	}


	void timerCallback() override
	{
		stopTimer();

		if(f)
			f();

		delete this;
	}

private:

	std::function<void(void)> f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayedFunctionCaller);
};


/** A wrapper around a lambda that will get executed with the given processor
	as argument.
	
	This is used by the suspension system to execute functions asynchronously.
	
	Since the time between creating this and the actual execution can be long
	and all kinds of things might have happened in the mean time, it will check
	if the processor still exists and automatically cancels the function if the
	processor got deleted.

	You will never have to create one of these objects manually, the only thing you
	need to know is the Function prototype, since all lambdas you pass in have
	to meet its structure:

	\code
	auto f = [](hise::Processor* p)
	{
	    bool success = p->doSomething();

		if(success)
		    return SafeFunctionCall::OK;
		else
			return SafeFunctionCall::cancelled;
	};
	\endcode
*/
struct SafeFunctionCall
{
	/** The return status for a lambda used with this class. */
	enum Status
	{
		OK = 0, ///< The default return type
		cancelled, ///< Indicates if there is a abnormal event that prevents the successfull execution. For example for a sample preload task, switching the sample map in the mean time can cause this since the sample doesn't need to be loaded anymore
		processorWasDeleted, ///< If a Processor was deleted between the creation and execution of this method, this will be the return type. Normally you don't have to use it at all, since this will be taken care of automatically
		nullPointerCall, ///< this is used when you try to call a SafeFunction call with a null pointer (the effective result is the same as processorDeleted, but it might give you a clue for debugging).
		numStatuses
	};

	/** The prototype for all lambdas that can be passed in the constructor. */
	using Function = std::function<Status(Processor*)>;

	SafeFunctionCall(Processor* p_, const Function& f_) noexcept;

	SafeFunctionCall() noexcept;;

	Status call() const;

	bool isValid() const noexcept { return (bool)f; };

	Result callWithResult() const
	{
		auto r = call();

		if (r == OK)
			return Result::ok();

		return Result::fail(String(r));
	}

	Function f;
	WeakReference<Processor> p;
};

/** A function prototype for lambdas passed into the suspended task system. */
using ProcessorFunction = SafeFunctionCall::Function;

#if USE_VDSP_FFT

#define MAX_VDSP_FFT_SIZE 13

class VDspFFT
{
public:
    
    VDspFFT(int maxN=16);
    
    ~VDspFFT();
    
    void complexFFTInplace(float* data, int size, bool unpack=true);
    
    void complexFFTInverseInplace(float* data, int size);
    
    void multiplyComplex(float* output, float* in1, int in1Offset, float* in2, int in2Offset, int numSamples, bool addToOutput);
    
private:
    
    class Pimpl;
    
    ScopedPointer<Pimpl> pimpl;
};

#endif

} // namespace hise

#endif  // UTILITYCLASSES_H_INCLUDED
