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

/** A Helper class that encapsulates the regex operations */
class RegexFunctions
{
public:
    
	static Array<StringArray> findSubstringsThatMatchWildcard(const String &regexWildCard, const String &stringToTest);

	/** Searches a string and returns a StringArray with all matches. 
	*	You can specify and index of a capture group (if not, the entire match will be used). */
	static StringArray search(const String& wildcard, const String &stringToTest, int indexInMatch=0);

	/** Returns the first match of the given wildcard in the test string. The first entry will be the whole match, followed by capture groups. */
    static StringArray getFirstMatch(const String &wildcard, const String &stringToTest, const Processor* /*processorForErrorOutput*/=nullptr);
    
	/** Checks if the given string matches the regex wildcard. */
    static bool matchesWildcard(const String &wildcard, const String &stringToTest, const Processor* /*processorForErrorOutput*/=nullptr);

};


/** A small helper class that uses RAII for enabling flush to zero mode. */
class ScopedNoDenormals
{
public:
	ScopedNoDenormals();;

	~ScopedNoDenormals();;

	int oldMXCSR;
};


class SafeChangeBroadcaster;

/** A class for message communication between objects.
*
*	This class has the same functionality as the JUCE ChangeListener class, but it uses a weak reference for the internal list,
*	so deleted listeners will not crash the application.
*/
class SafeChangeListener
{
public:

	virtual ~SafeChangeListener()
	{
		masterReference.clear();
	}

	/** Overwrite this and handle the message. */
	virtual void changeListenerCallback(SafeChangeBroadcaster *b) = 0;

private:

	friend class WeakReference < SafeChangeListener > ;

	WeakReference<SafeChangeListener>::Master masterReference;
};


/** This class is used to coallescate multiple calls to an asynchronous update for a given Listener.
*
*	It is designed to be a replacement for the normal AsyncUpdater which can clog the message thread if too
*	many change notifications are sent.
*
*	In order to use this class, just create an instance and pass this to your subclassed listeners, which then
*	can be used just like the standard AsyncUpdater from JUCE.
*
*	Another nice feature is that you can use the same mechanism to call a function asynchronously by passing
*	a lambda to callFunctionAsynchronously.
*/
class UpdateDispatcher : public AsyncUpdater
{
public:

	UpdateDispatcher() :
		pendingListeners(1024),
		pendingFunctions(1024)
	{};

	~UpdateDispatcher()
	{
		masterReference.clear();
	}

	class Listener
	{
	public:

		Listener(UpdateDispatcher* dispatcher_) :
			dispatcher(dispatcher_),
			pending(false)
		{

		};

		virtual ~Listener()
		{
			masterReference.clear();
		};

		virtual void handleAsyncUpdate() = 0;

		virtual void cancelPendingUpdate()
		{
			if (!pending)
				return;
			
			if(dispatcher != nullptr)
				dispatcher->cancelPendingUpdateForListener(this);
		}

		virtual void triggerAsyncUpdate()
		{
			if (pending)
				return;

			pending = true;

			if (dispatcher != nullptr)
				dispatcher->triggerAsyncUpdateForListener(this);
		}

	private:

		std::atomic<bool> pending;

		friend class WeakReference<Listener>;
		WeakReference<Listener>::Master masterReference;

		friend class UpdateDispatcher;

		WeakReference<UpdateDispatcher> dispatcher;
	};

	using Func = std::function<void(void)>;

	void triggerAsyncUpdateForListener(Listener* l)
	{
		const bool ok = pendingListeners.push(l);

		jassert(ok);
        ignoreUnused(ok);
        
        
		triggerAsyncUpdate();
	}

	void callFunctionAsynchronously(const Func& f)
	{
		pendingFunctions.push(Func(f));

		triggerAsyncUpdate();
	}

	void cancelPendingUpdateForListener(Listener* l)
	{
		cancelledListeners.addIfNotAlreadyThere(l);
	}

private:

	friend class WeakReference<UpdateDispatcher>;
	WeakReference<UpdateDispatcher>::Master masterReference;

	void handleAsyncUpdate() override
	{
		WeakReference<Listener> l;

		while (pendingListeners.pop(l))
		{
			if (l != nullptr)
			{
				if (cancelledListeners.contains(l))
				{
					l->pending = false;
					cancelledListeners.removeAllInstancesOf(l);
					continue;
				}
					

				l->handleAsyncUpdate();
				l->pending = false;
			}
		}

		Func f;

		while (pendingFunctions.pop(f))
		{
			f();
		}
	}

	friend class Listener;

	Array<WeakReference<Listener>> cancelledListeners;

	hise::LockfreeQueue<WeakReference<Listener>> pendingListeners;
	hise::LockfreeQueue<Func> pendingFunctions;
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
		{
			ScopedLock sl(arrayLock);
			pendingPropertyChanges.addIfNotAlreadyThere(PropertyChange(v, id));
		}
		
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
			Array<PropertyChange> thisTime;

			{
				ScopedLock sl(parent.arrayLock);
				thisTime.swapWith(parent.pendingPropertyChanges);
			}

			for (auto& pc : thisTime)
			{
				parent.asyncValueTreePropertyChanged(pc.v, pc.id);
			}
		}

		AsyncValueTreePropertyListener& parent;
	};

	CriticalSection arrayLock;

	ValueTree state;
	WeakReference<UpdateDispatcher> dispatcher;
	AsyncHandler asyncHandler;

	Array<PropertyChange> pendingPropertyChanges;
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


/** A collection of little helper functions to clean float arrays.
*
*	Source: http://musicdsp.org/showArchiveComment.php?ArchiveID=191
*/
struct FloatSanitizers
{
	static void sanitizeArray(float* data, int size);;

	static float sanitizeFloatNumber(float& input);;

	struct Test : public UnitTest
	{
		Test():
			UnitTest("Testing float sanitizer")
		{

		};

		void runTest() override;
	};
};


static FloatSanitizers::Test floatSanitizerTest;

/** A drop in replacement for the ChangeBroadcaster class from JUCE but with weak references.
*
*	If you use the normal class and forget to unregister a listener in its destructor, it will crash the application.
*	This class uses a weak reference (but still throws an assertion so you still recognize if something is funky), so it handles this case much more gracefully.
*
*	Also you can add a string to your message for debugging purposes (with the JUCE class you have no way of knowing what caused the message if you call it asynchronously.
*/
class SafeChangeBroadcaster
{
public:

	SafeChangeBroadcaster(const String& name_ = {}) :
		dispatcher(this),
        flagTimer(this),
		name(name_)
	{};

	virtual ~SafeChangeBroadcaster()
	{
		dispatcher.cancelPendingUpdate();
        flagTimer.stopTimer();
	};

	/** Sends a synchronous change message to all the registered listeners.
	*
	*	This will immediately call all the listeners that are registered. For thread-safety reasons, you must only call this method on the main message thread.
	*/
	void sendSynchronousChangeMessage();;

	/** Registers a listener to receive change callbacks from this broadcaster.
	*
	*	Trying to add a listener that's already on the list will have no effect.
	*/
	void addChangeListener(SafeChangeListener *listener);

	/**	Unregisters a listener from the list.
	*
	*	If the listener isn't on the list, this won't have any effect.
	*/
	void removeChangeListener(SafeChangeListener *listener);

	/** Removes all listeners from the list. */
	void removeAllChangeListeners();

	/** Causes an asynchronous change message to be sent to all the registered listeners.
	*
	*	The message will be delivered asynchronously by the main message thread, so this method will return immediately.
	*	To call the listeners synchronously use sendSynchronousChangeMessage().
	*/
	void sendChangeMessage(const String &/*identifier*/ = String());;
    
    /** This will send a message without allocating a message slot.
    *
    *   Use this in the audio thread to prevent malloc calls, but don't overuse this feature.
    */
    void sendAllocationFreeChangeMessage();
    
    void enableAllocationFreeMessages(int timerIntervalMilliseconds);

	bool hasChangeListeners() const noexcept { return !listeners.isEmpty(); }

private:

    class FlagTimer: public Timer
    {
    public:
        
        
        FlagTimer(SafeChangeBroadcaster *parent_):
          parent(parent_),
          send(false)
        {
           
        }
 
        ~FlagTimer()
        {
            stopTimer();
        }
        
        void triggerUpdate()
        {
            send.store(true);
        }
        
        void timerCallback() override
        {
			const bool shouldUpdate = send.load();

            if(shouldUpdate)
            {
                parent->sendSynchronousChangeMessage();
				send.store(false);
            }
        }
        
        SafeChangeBroadcaster *parent;
        std::atomic<bool> send;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlagTimer)
    };
    
	class AsyncBroadcaster : public AsyncUpdater
	{
	public:
		AsyncBroadcaster(SafeChangeBroadcaster *parent_) :
			parent(parent_)
		{}

		void handleAsyncUpdate() override
		{
			parent->sendSynchronousChangeMessage();
		}

		SafeChangeBroadcaster *parent;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsyncBroadcaster)
	};

	const String name;

	AsyncBroadcaster dispatcher;
    FlagTimer flagTimer;

	String currentString;

	Array<WeakReference<SafeChangeListener>, CriticalSection> listeners;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SafeChangeBroadcaster)
};



/** This class is a listener class that can react to tempo changes.
*	@ingroup utility
*
*	In order to use this, subclass this and implement the behaviour in the tempoChanged callback.
*/
class TempoListener
{
public:

	virtual ~TempoListener() { masterReference.clear(); };

	/** The callback function that will be called if the tempo was changed.
	*
	*	This is called synchronously in the audio callback before the processing, so make sure you don't
	*	make something stupid here.
	*
	*	It will be called once per block, so you can't do sample synchronous tempo stuff, but that should be enough.
	*/
	virtual void tempoChanged(double newTempo) = 0;

private:

	friend class WeakReference<TempoListener>;
	WeakReference<TempoListener>::Master masterReference;

	

};


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

	inline float getSaturatedSample(float inputSample)
	{
		return (1.0f + k) * inputSample / (1.0f + k * fabsf(inputSample));
	}

	void setSaturationAmount(float newSaturationAmount)
	{
		saturationAmount = newSaturationAmount;
		if (saturationAmount == 1.0f) saturationAmount = 0.999f;

		k = 2 * saturationAmount / (1.0f - saturationAmount);
	}

private:

	float saturationAmount;
	float k;

};


/** A utility class for linear interpolation between samples.
*	@ingroup utility
*
*/
class Interpolator
{
public:

	/** A simple linear interpolation.
	*
	*	@param lowValue the value of the lower index.
	*	@param highValue the value of the higher index.
	*	@param delta the sub-integer part between the two indexes (must be between 0.0f and 1.0f)
	*	@returns the interpolated value.
	*/
	static float interpolateLinear(const float lowValue, const float highValue, const float delta);

};


/** A class that handles temposyncing.
*	@ingroup utility
*
*	All methods are static and it holds no data, so you have to get the host bpm before
*	you can use this class.
*
*	If the supplied hostTempo is invalid (= 0.0), a default tempo of 120.0 is used.
*/
class TempoSyncer
{
public:

	/** The note values. */
	enum Tempo
	{
		Whole = 0, ///< a whole note
		HalfDuet,
		Half, ///< a half note
		HalfTriplet, ///< a half triplet note
		QuarterDuet,
		Quarter, ///< a quarter note
		QuarterTriplet, ///< a quarter triplet note
		EighthDuet,
		Eighth, ///< a eighth note
		EighthTriplet, ///< a eighth triplet note
		SixteenthDuet,
		Sixteenth, ///< a sixteenth note
		SixteenthTriplet, ///< a sixteenth triplet
		ThirtyTwoDuet,
		ThirtyTwo,
		ThirtyTwoTriplet,
		SixtyForthDuet,
		SixtyForth,
		SixtyForthTriplet,
		numTempos
	};

	/** Returns the sample amount for the specified tempo. */
	static int getTempoInSamples(double hostTempoBpm, double sampleRate, Tempo t)
	{
		if (hostTempoBpm == 0.0) hostTempoBpm = 120.0;

		const float seconds = (60.0f / (float)hostTempoBpm) * getTempoFactor(t);
		return (int)(seconds * (float)sampleRate);
	};

	/** Returns the time for the specified tempo in milliseconds. */
	static float getTempoInMilliSeconds(double hostTempoBpm, Tempo t)
	{
		if (hostTempoBpm == 0.0) hostTempoBpm = 120.0;

		const float seconds = (60.0f / (float)hostTempoBpm) * getTempoFactor(t);
		return seconds * 1000.0f;
	};

	/** Returns the tempo as frequency (in Hertz). */
	static float getTempoInHertz(double hostTempoBpm, Tempo t)
	{
		if (hostTempoBpm == 0.0) hostTempoBpm = 120.0;

		const float seconds = (60.0f / (float)hostTempoBpm) * getTempoFactor(t);

		return 1.0f / seconds;
	}

	/** Returns the name of the tempo with the index 't'. */
	static const String & getTempoName(int t)
	{
		jassert(t < numTempos);
		return tempoNames[t];
	};

	/** Returns the index of the tempo with the name 't'. */
	static Tempo getTempoIndex(const String &t) { return (Tempo)tempoNames.indexOf(t); };

	/** Fills the internal arrays. Call this on application start. */
	static void initTempoData()
	{
		tempoNames.add("1/1");		tempoFactors[Whole] = 4.0f;
		tempoNames.add("1/2D");	    tempoFactors[HalfDuet] = 2.0f * 1.5f;
		tempoNames.add("1/2");		tempoFactors[Half] = 2.0f;
		tempoNames.add("1/2T");		tempoFactors[HalfTriplet] = 4.0f / 3.0f;
		tempoNames.add("1/4D");	    tempoFactors[QuarterDuet] = 1.0f * 1.5f;
		tempoNames.add("1/4");		tempoFactors[Quarter] = 1.0f;
		tempoNames.add("1/4T");		tempoFactors[QuarterTriplet] = 2.0f / 3.0f;
		tempoNames.add("1/8D");	    tempoFactors[EighthDuet] = 0.5f * 1.5f;
		tempoNames.add("1/8");		tempoFactors[Eighth] = 0.5f;
		tempoNames.add("1/8T");		tempoFactors[EighthTriplet] = 1.0f / 3.0f;
		tempoNames.add("1/16D");	tempoFactors[SixteenthDuet] = 0.25f * 1.5f;
		tempoNames.add("1/16");		tempoFactors[Sixteenth] = 0.25f;
		tempoNames.add("1/16T");	tempoFactors[SixteenthTriplet] = 0.5f / 3.0f;
		tempoNames.add("1/32D");	tempoFactors[ThirtyTwoDuet] = 0.125f * 1.5f;
		tempoNames.add("1/32");		tempoFactors[ThirtyTwo] = 0.125f;
		tempoNames.add("1/32T");	tempoFactors[ThirtyTwoTriplet] = 0.25f / 3.0f;
		tempoNames.add("1/64D");	tempoFactors[SixtyForthDuet] = 0.125f * 0.5f * 1.5f;
		tempoNames.add("1/64");		tempoFactors[SixtyForth] = 0.125f * 0.5f;
		tempoNames.add("1/64T");	tempoFactors[SixtyForthTriplet] = 0.125f * 0.5f / 3.0f;
	}

private:

	static float getTempoFactor(Tempo t) { return tempoFactors[(int)t]; };

	static StringArray tempoNames;
	static float tempoFactors[numTempos];

};

class Processor;

/** Subclass your component from this class and the main window will focus it to allow copy pasting with shortcuts.
*
*   Then, in your mouseDown method, call grabCopyAndPasteFocus().
*	If you call paintOutlineIfSelected from your paint method, it will be automatically highlighted.
*/
class CopyPasteTarget
{
public:

	CopyPasteTarget() : isSelected(false) {};
	virtual ~CopyPasteTarget()
	{
		masterReference.clear();
	};

	virtual String getObjectTypeName() = 0;
	virtual void copyAction() = 0;
	virtual void pasteAction() = 0;

	void grabCopyAndPasteFocus();

	void dismissCopyAndPasteFocus();

	bool isSelectedForCopyAndPaste() { return isSelected; };

	void paintOutlineIfSelected(Graphics &g)
	{
		if (isSelected)
		{
			Component *thisAsComponent = dynamic_cast<Component*>(this);

			if (thisAsComponent != nullptr)
			{
				Rectangle<float> bounds = Rectangle<float>((float)thisAsComponent->getLocalBounds().getX(),
														   (float)thisAsComponent->getLocalBounds().getY(),
														   (float)thisAsComponent->getLocalBounds().getWidth(),
														   (float)thisAsComponent->getLocalBounds().getHeight());



				Colour outlineColour = Colour(SIGNAL_COLOUR).withAlpha(0.3f);
				
				g.setColour(outlineColour);

				g.drawRect(bounds, 1.0f);

			}
			else jassertfalse;
		}
	}

	void deselect()
	{
		isSelected = false;
		dynamic_cast<Component*>(this)->repaint();
	}

private:

	WeakReference<CopyPasteTarget>::Master masterReference;
	friend class WeakReference < CopyPasteTarget > ;

	WeakReference<Processor> processor;

	bool isSelected;

};



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

class AutoSaver : public Timer
{
public:

	

	AutoSaver(MainController *mc_) :
		mc(mc_),
		currentAutoSaveIndex(0)
	{
		enableAutoSaving();
	};

	void enableAutoSaving()
	{
		startTimer(1000 * 60 * 5); // autosave all 5 minutes
	}

	void disableAutoSaving()
	{
		stopTimer();
	}

	bool isAutoSaving() const
	{
		return isTimerRunning();
	}

	void timerCallback() override;

	void toggleAutoSaving()
	{
		if (isAutoSaving()) disableAutoSaving();
		else enableAutoSaving();
	}

private:

	File getAutoSaveFile();

	Array<File> fileList;

	int currentAutoSaveIndex;

	MainController *mc;
};

/** This class is used to simulate different devices.
*
*	In the backend application you can choose the current device. In compiled apps
*	it will be automatically set to the actual model.
*
*	It will use different UX paradigms depending on the model as well.
*
*	Due to simplicity, it uses a static variable which may cause some glitches when used with plugins, so 
*	it's recommended to use this only in standalone mode.
*/
class HiseDeviceSimulator
{
public:
	enum class DeviceType
	{
		Desktop = 0,
		iPad,
		iPadRetina,
		iPadPro,
		iPadAUv3,
		iPhone5,
		iPhone6,
		iPodTouch6,
		numDeviceTypes
	};

    static void init(AudioProcessor::WrapperType wrapper);

	static void setDeviceType(DeviceType newDeviceTye)
	{
		currentDevice = newDeviceTye;
	}

	static DeviceType getDeviceType() { return currentDevice; }
	
	static String getDeviceName(int index=-1);

	static bool fileNameContainsDeviceWildcard(const File& f);

	static bool isMobileDevice() { return currentDevice > DeviceType::Desktop; }
	static bool isRetina() { return currentDevice > DeviceType::iPad; }

    static bool isAUv3() { return currentDevice == DeviceType::iPadAUv3; };
    
    static bool isStandalone()
    {
#if HISE_IOS
        return !isAUv3();
#else
      
#if IS_STANDALONE_FRONTEND || USE_BACKEND
        return true;
#else
        return false;
#endif
        
#endif
    }
    
	static Rectangle<int> getDisplayResolution();

	private:

	static DeviceType currentDevice;
};

class SemanticVersionChecker
{
public:

	SemanticVersionChecker(const String& oldVersion_, const String& newVersion_)
	{
		parseVersion(oldVersion, oldVersion_);
		parseVersion(newVersion, newVersion_);
	};

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

using ProcessorFunction = std::function<bool(Processor*)>;

struct SafeFunctionCall
{
	SafeFunctionCall(Processor* p_, const ProcessorFunction& f_);

	SafeFunctionCall();;

	bool call();

	ProcessorFunction f;
	WeakReference<Processor> p;
};


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
