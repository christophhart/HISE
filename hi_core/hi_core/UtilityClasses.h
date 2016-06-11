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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef UTILITYCLASSES_H_INCLUDED
#define UTILITYCLASSES_H_INCLUDED

#include <regex>

#if JUCE_IOS
#else
#include "xmmintrin.h"
#endif

class Processor;

/** A Helper class that encapsulates the regex operations */
class RegexFunctions
{
public:
    
    static StringArray getMatches(const String &wildcard, const String &stringToTest, const Processor* /*processorForErrorOutput*/=nullptr)
    {
        try
        {
            std::regex reg(wildcard.toStdString());
            
            std::smatch match;
            
            if (std::regex_search(stringToTest.toStdString(), match, reg))
            {
                StringArray sa;
                for (auto x:match)
                {
                    sa.insert(-1, String(x));
                }
                
                return sa;
            }
            
            return StringArray();
        }
        catch (std::regex_error e)
        {
            jassertfalse;
            return StringArray();
        }
        
        
    }
    
    static bool matchesWildcard(const String &wildcard, const String &stringToTest, const Processor* /*processorForErrorOutput*/=nullptr)
    {
        try
        {
            std::regex reg(wildcard.toStdString());
            
            return std::regex_search(stringToTest.toStdString(), reg);
        }
        catch (std::regex_error e)
        {
            //debugError(sampler, e.what());
            
            return false;
        }
    }
};


/** A small helper class that uses RAII for enabling flush to zero mode. */
class ScopedNoDenormals
{
public:
	ScopedNoDenormals()
    {
#if JUCE_IOS
#else
		oldMXCSR = _mm_getcsr();
	    int newMXCSR = oldMXCSR | 0x8040;
		_mm_setcsr(newMXCSR);
#endif
	};

	~ScopedNoDenormals()
	{
#if JUCE_IOS
#else
		_mm_setcsr(oldMXCSR);
        #endif
	};

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
    ScopedGlitchDetector(const Identifier &id):
      identifier(id),
      startTime(Time::getMillisecondCounterHiRes())
    {
        if(lastPositiveId == id)
        {
            // Resets the identifier if a GlitchDetector is recreated...
            lastPositiveId = Identifier::null;
        }
    };
    
    ~ScopedGlitchDetector()
    {
        const double stopTime = Time::getMillisecondCounterHiRes();
        const double interval = (stopTime - startTime);
        
        if (lastPositiveId.isNull() && interval > maxMilliSeconds)
        {
            lastPositiveId = identifier;
            
            if(Logger::getCurrentLogger() != nullptr)
            {
                Logger::getCurrentLogger()->writeToLog("Time out in function: " + identifier.toString());
            }
        }
    }
    
    // =================================================================================================================================
    
    /** Change the global time out value. */
    static void setMaxMilliSeconds(double newMaxMilliSeconds) noexcept
    {
        maxMilliSeconds = newMaxMilliSeconds;
    };
    
    /** Same as setMaxMilliSeconds, but accepts the sample rate and the buffer size as parameters to save you some calculation. */
    static void setMaxTimeOutFromBufferSize(double sampleRate, double bufferSize) noexcept
    {
        maxMilliSeconds = (bufferSize / sampleRate) * 1000.0;
    };
    
private:
    
    // =================================================================================================================================
    
    const Identifier identifier;
    const double startTime;
    static double maxMilliSeconds;
    static Identifier lastPositiveId;
    
    // =================================================================================================================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedGlitchDetector)
};

#define USE_GLITCH_DETECTION 1

#if USE_GLITCH_DETECTION
#define ADD_GLITCH_DETECTOR(x) static Identifier glitchId(x); ScopedGlitchDetector sgd(glitchId)
#else
#define ADD_GLITCH_DETECTOR(x)
#endif

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

	SafeChangeBroadcaster() :
		dispatcher(this)
	{};

	virtual ~SafeChangeBroadcaster()
	{
		dispatcher.cancelPendingUpdate();
	};

	/** Sends a synchronous change message to all the registered listeners.
	*
	*	This will immediately call all the listeners that are registered. For thread-safety reasons, you must only call this method on the main message thread.
	*/
	void sendSynchronousChangeMessage(const String &identifier = String::empty)
	{
		currentString = identifier;

		// Ooops, only call this in the message thread.
		// Use sendChangeMessage() if you need to send a message from elsewhere.
		jassert(MessageManager::getInstance()->isThisTheMessageThread());

		ScopedLock sl(listeners.getLock());

		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i].get() != nullptr)
			{
				listeners[i]->changeListenerCallback(this);
			}
			else
			{
				// Ooops, you called an deleted listener. 
				// Normally, it would crash now, but since this is really lame, this class only throws an assert!
				jassertfalse;

				listeners.remove(i--);
			}
		}
	};

	/** Registers a listener to receive change callbacks from this broadcaster.
	*
	*	Trying to add a listener that's already on the list will have no effect.
	*/
	void addChangeListener(SafeChangeListener *listener)
	{
		ScopedLock sl(listeners.getLock());

		listeners.addIfNotAlreadyThere(listener);
	}

	/**	Unregisters a listener from the list.
	*
	*	If the listener isn't on the list, this won't have any effect.
	*/
	void removeChangeListener(SafeChangeListener *listener)
	{
		ScopedLock sl(listeners.getLock());

		listeners.removeAllInstancesOf(listener);
	}

	/** Removes all listeners from the list. */
	void removeAllChangeListeners()
	{
		dispatcher.cancelPendingUpdate();

		ScopedLock sl(listeners.getLock());

		listeners.clear();
	}

	/** Causes an asynchronous change message to be sent to all the registered listeners.
	*
	*	The message will be delivered asynchronously by the main message thread, so this method will return immediately.
	*	To call the listeners synchronously use sendSynchronousChangeMessage().
	*/
	void sendChangeMessage(const String &identifier = String::empty)
	{
		currentString = identifier;

		dispatcher.triggerAsyncUpdate();
	};

private:

	class AsyncBroadcaster : public AsyncUpdater
	{
	public:
		AsyncBroadcaster(SafeChangeBroadcaster *parent_) :
			parent(parent_)
		{}

		void handleAsyncUpdate() override
		{
			parent->sendSynchronousChangeMessage(parent->currentString);
		}

		SafeChangeBroadcaster *parent;

	};

	AsyncBroadcaster dispatcher;

	String currentString;

	Array<WeakReference<SafeChangeListener>, CriticalSection> listeners;
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

	/** Converts a balance value to the gain factor for the supplied channel using an equal power formula. */
	static float getGainFactorForBalance(float balanceValue, bool calculateLeftChannel)
	{
		const float balance = balanceValue / 100.0f;

		float panValue = (float_Pi * (balance + 1.0f)) * 0.25f;

		return 1.4142f * (calculateLeftChannel ? cosf(panValue) : sinf(panValue));
	};

	/** Processes a stereo buffer with an array of balance values (from 0...1) - typically the output of a modulation chain. 
	*
	*	This is slightly faster than calling getGainFactorForBalance because it uses some vectorization...
	*	The float array that is passed in is used as working buffer, so don't rely on it not being changed...
	*	
	*/
	static void processBuffer(AudioSampleBuffer &stereoBuffer, float *panValues, int startSample, int numSamples)
	{
		FloatVectorOperations::multiply(panValues + startSample, float_Pi * 0.5f, numSamples);

		stereoBuffer.applyGain(1.4142f); // +3dB for equal power...

		float *l = stereoBuffer.getWritePointer(0, startSample);
		float *r = stereoBuffer.getWritePointer(1, startSample);

		while (--numSamples >= 0)
		{
			*l++ *= cosf(*panValues) * 1.4142f;
			*r++ *= sinf(*panValues);

			panValues++;
		}
	}

	/** Returns a string version of the pan value. */
	static String getBalanceAsString(int balanceValue)
	{
		if (balanceValue == 0) return "C";

		else return String(balanceValue) + (balanceValue > 0 ? "R" : "L");
	}
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
		Half, ///< a half note
		Quarter, ///< a quarter note
		Eighth, ///< a eighth note
		Sixteenth, ///< a sixteenth note
		HalfTriplet, ///< a half triplet note
		QuarterTriplet, ///< a quarter triplet note
		EighthTriplet, ///< a eighth triplet note
		SixteenthTriplet, ///< a sixteenth triplet
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
		tempoNames.add("1/2");		tempoFactors[Half] = 2.0f;
		tempoNames.add("1/4");		tempoFactors[Quarter] = 1.0f;
		tempoNames.add("1/8");		tempoFactors[Eighth] = 0.5f;
		tempoNames.add("1/16");		tempoFactors[Sixteenth] = 0.25f;
		tempoNames.add("1/2T");		tempoFactors[HalfTriplet] = 4.0f / 3.0f;
		tempoNames.add("1/4T");		tempoFactors[QuarterTriplet] = 2.0f / 3.0f;
		tempoNames.add("1/8T");		tempoFactors[EighthTriplet] = 1.0f / 3.0f;
		tempoNames.add("1/16T");	tempoFactors[SixteenthTriplet] = 0.5f / 3.0f;
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

	bool isSelectedForCopyAndPaste() { return isSelected; };

	void paintOutlineIfSelected(Graphics &g)
	{
		if (isSelected)
		{
			Component *thisAsComponent = dynamic_cast<Component*>(this);

			if (thisAsComponent != nullptr)
			{
				Rectangle<float> bounds = Rectangle<float>(thisAsComponent->getLocalBounds().getX(),
														   thisAsComponent->getLocalBounds().getY(),
														   thisAsComponent->getLocalBounds().getWidth(),
														   thisAsComponent->getLocalBounds().getHeight());



				Colour outlineColour = Colour(0xffb9d2d6);
				Colour transparentColour = outlineColour.withAlpha(0.0f);

				const float gradientWidth = 4.0f;

				g.setColour(outlineColour);

				/*
				g.setGradientFill(ColourGradient(outlineColour, bounds.getX(), bounds.getY(),
												 transparentColour, gradientWidth, bounds.getY(), false));

				g.fillRect(bounds.getX(), bounds.getY(), gradientWidth, bounds.getHeight());

				g.setGradientFill(ColourGradient(outlineColour, bounds.getX(), bounds.getY(),
												 transparentColour, bounds.getX(), gradientWidth, false));

				g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), gradientWidth);

				g.setGradientFill(ColourGradient(outlineColour, bounds.getWidth(), bounds.getY(),
					transparentColour, bounds.getWidth() - gradientWidth, bounds.getY(), false));

				g.fillRect(bounds.getWidth() - gradientWidth, bounds.getY(), gradientWidth, bounds.getHeight());

				g.setGradientFill(ColourGradient(outlineColour, bounds.getX(), bounds.getHeight(),
					transparentColour, bounds.getX(), bounds.getHeight() - gradientWidth, false));

				g.fillRect(bounds.getX(), bounds.getHeight() - gradientWidth, bounds.getWidth(), gradientWidth);
				*/

				g.drawRect(bounds, 2.0f);

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

#endif  // UTILITYCLASSES_H_INCLUDED
