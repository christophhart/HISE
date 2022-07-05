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


#if !JUCE_ARM
#include "xmmintrin.h"
#endif




namespace hise {
using namespace juce;



void SafeChangeBroadcaster::sendSynchronousChangeMessage()
{
	if (MessageManager::getInstance()->isThisTheMessageThread() || MessageManager::getInstance()->currentThreadHasLockedMessageManager())
	{
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
	}
	else
	{
		sendChangeMessage();
	}


}

void SafeChangeBroadcaster::addChangeListener(SafeChangeListener *listener)
{
	ScopedLock sl(listeners.getLock());

	listeners.addIfNotAlreadyThere(listener);

	addPooledChangeListener(listener);
}

void SafeChangeBroadcaster::removeChangeListener(SafeChangeListener *listener)
{
	ScopedLock sl(listeners.getLock());

	listeners.removeAllInstancesOf(listener);

	removePooledChangeListener(listener);
}

void SafeChangeBroadcaster::removeAllChangeListeners()
{
	dispatcher.cancelPendingUpdate();

	ScopedLock sl(listeners.getLock());

	listeners.clear();
}

void SafeChangeBroadcaster::sendChangeMessage(const String &/*identifier*/ /*= String()*/)
{
	IF_NOT_HEADLESS(dispatcher.triggerAsyncUpdate());
}

void SafeChangeBroadcaster::sendAllocationFreeChangeMessage()
{
	jassert(isHandlerInitialised());
	sendPooledChangeMessage();
}

void SafeChangeBroadcaster::enablePooledUpdate(PooledUIUpdater* updater)
{
	setHandler(updater);
}


CopyPasteTarget::HandlerFunction* CopyPasteTarget::handlerFunction = nullptr;

void CopyPasteTarget::grabCopyAndPasteFocus()
{
	Component *thisAsComponent = dynamic_cast<Component*>(this);

	if (handlerFunction != nullptr && thisAsComponent)
	{
		CopyPasteTargetHandler *handler = handlerFunction->getHandler(thisAsComponent);

		if (handler != nullptr)
		{
			handler->setCopyPasteTarget(this);
			isSelected = true;
			thisAsComponent->repaint();
		}
	}
}


void CopyPasteTarget::dismissCopyAndPasteFocus()
{
	Component *thisAsComponent = dynamic_cast<Component*>(this);

	if (handlerFunction != nullptr && thisAsComponent)
	{
		CopyPasteTargetHandler *handler = handlerFunction->getHandler(thisAsComponent);

		if (handler != nullptr && isSelected)
		{
			handler->setCopyPasteTarget(nullptr);
			isSelected = false;
			thisAsComponent->repaint();
		}
	}
	else
	{
		// You can only use components as CopyAndPasteTargets!
		jassertfalse;
	}
}


void CopyPasteTarget::paintOutlineIfSelected(Graphics &g)
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

void CopyPasteTarget::deselect()
{
	isSelected = false;
	dynamic_cast<Component*>(this)->repaint();
}


void CopyPasteTarget::setHandlerFunction(HandlerFunction* f)
{
	handlerFunction = f;
}

Array<StringArray> RegexFunctions::findSubstringsThatMatchWildcard(const String &regexWildCard, const String &stringToTest)
{
	Array<StringArray> matches;
	String remainingText = stringToTest;
	StringArray m = getFirstMatch(regexWildCard, remainingText);

	while (m.size() != 0 && m[0].length() != 0)
	{
		remainingText = remainingText.fromFirstOccurrenceOf(m[0], false, false);
		matches.add(m);
		m = getFirstMatch(regexWildCard, remainingText);
	}

	return matches;
}

StringArray RegexFunctions::search(const String& wildcard, const String &stringToTest, int indexInMatch/*=0*/)
{
#if TRAVIS_CI
	return StringArray(); // Travis CI seems to have a problem with libc++...
#else
	try
	{
		StringArray searchResults;

		std::regex includeRegex(wildcard.toStdString());
		std::string xAsStd = stringToTest.toStdString();
		std::sregex_iterator it(xAsStd.begin(), xAsStd.end(), includeRegex);
		std::sregex_iterator it_end;

		while (it != it_end)
		{
			std::smatch result = *it;

			StringArray matches;
			for (auto x : result)
			{
				matches.add(String(x));
			}

			if (indexInMatch < matches.size()) searchResults.add(matches[indexInMatch]);

			++it;
		}

		return searchResults;
	}
	catch (std::regex_error e)
	{
		DBG(e.what());
		return StringArray();
	}
#endif
}

StringArray RegexFunctions::getFirstMatch(const String &wildcard, const String &stringToTest)
{
#if TRAVIS_CI
	return StringArray(); // Travis CI seems to have a problem with libc++...
#else

	try
	{
		std::regex reg(wildcard.toStdString());
		std::string s(stringToTest.toStdString());
		std::smatch match;


		if (std::regex_search(s, match, reg))
		{
			StringArray sa;

			for (auto x : match)
			{
				sa.add(String(x));
			}

			return sa;
		}

		return StringArray();
	}
	catch (std::regex_error e)
	{
		jassertfalse;

		DBG(e.what());
		return StringArray();
	}
#endif
}

bool FuzzySearcher::fitsSearch(const String &searchTerm, const String &stringToMatch, double fuzzyness)
{
	if (stringToMatch.contains(searchTerm))
	{
		return true;
	}

	// Calculate the Levenshtein-distance:
	int levenshteinDistance = getLevenshteinDistance(searchTerm, stringToMatch);

	// Length of the longer string:
	int length = jmax<int>(searchTerm.length(), stringToMatch.length());

	// Calculate the score:
	double score = 1.0 - (double)levenshteinDistance / length;

	// Match?
	return score > fuzzyness;
}

void FuzzySearcher::search(void *outputArray, bool useIndexes, const String &word, const StringArray &wordList, double fuzzyness = 0.4)
{

	StringArray foundWords = StringArray();

	for (int i = 0; i < wordList.size(); i++)
	{
		String thisWord = wordList[i].toLowerCase();
		thisWord = thisWord.removeCharacters("()`[]*_-` ").substring(0, 32);

		String searchWord = word.toLowerCase();
		searchWord = searchWord.removeCharacters("()`[]*_-` ");

		const bool success = fitsSearch(searchWord, thisWord, fuzzyness);

		if (success)
		{
			if (useIndexes)
				static_cast<Array<int>*>(outputArray)->add(i);
			else
				static_cast<Array<String>*>(outputArray)->add(thisWord);
		}
	}

}

StringArray FuzzySearcher::searchForResults(const String &word, const StringArray &wordList, double fuzzyness)
{
	StringArray foundWords;
	search(&foundWords, false, word, wordList, fuzzyness);
	return foundWords;
}

Array<int> FuzzySearcher::searchForIndexes(const String &word, const StringArray &wordList, double fuzzyness)
{
	Array<int> foundIndexes;
	search(&foundIndexes, true, word, wordList, fuzzyness);
	return foundIndexes;
}

#define NUM_MAX_CHARS 128

int FuzzySearcher::getLevenshteinDistance(const String &src, const String &dest)
{
	const int srcLength = src.length();
	const int dstLength = dest.length();

	int d[NUM_MAX_CHARS][NUM_MAX_CHARS];

	int i, j, cost;
	const char *str1 = src.getCharPointer().getAddress();
	const char *str2 = dest.getCharPointer().getAddress();

	for (i = 0; i <= srcLength; i++)
	{
		d[i][0] = i;
	}
	for (j = 0; j <= dstLength; j++)
	{
		d[0][j] = j;
	}
	for (i = 1; i <= srcLength; i++)
	{
		for (j = 1; j <= dstLength; j++)
		{

			if (str1[i - 1] == str2[j - 1])
				cost = 0;
			else
				cost = 1;

			d[i][j] =
				jmin<int>(
					d[i - 1][j] + 1,              // Deletion
					jmin<int>(
						d[i][j - 1] + 1,          // Insertion
						d[i - 1][j - 1] + cost)); // Substitution

			if ((i > 1) && (j > 1) && (str1[i - 1] ==
				str2[j - 2]) && (str1[i - 2] == str2[j - 1]))
			{
				d[i][j] = jmin<int>(d[i][j], d[i - 2][j - 2] + cost);
			}
		}
	}

	int result = d[srcLength][dstLength];



	return result;
}


bool RegexFunctions::matchesWildcard(const String &wildcard, const String &stringToTest)
{
#if TRAVIS_CI
	return false; // Travis CI seems to have a problem with libc++...
#else

	try
	{
		std::regex reg(wildcard.toStdString());

		return std::regex_search(stringToTest.toStdString(), reg);
	}
	catch (std::regex_error e)
	{
		DBG(e.what());

		return false;
	}
#endif
}

ScopedNoDenormals::ScopedNoDenormals()
{
#if !JUCE_ARM
	oldMXCSR = _mm_getcsr();
	int newMXCSR = oldMXCSR | 0x8040;
	_mm_setcsr(newMXCSR);
#endif
}

ScopedNoDenormals::~ScopedNoDenormals()
{
#if !JUCE_ARM
	_mm_setcsr(oldMXCSR);
#endif
}


#if 0
bool SimpleReadWriteLock::ScopedReadLock::anotherThreadHasWriteLock() const
{
	return lock.writerThread != nullptr && lock.writerThread != Thread::getCurrentThreadId();
}



SimpleReadWriteLock::ScopedReadLock::ScopedReadLock(SimpleReadWriteLock &lock_, bool busyWait) :
	lock(lock_)
{
	for (int i = 2000; --i >= 0;)
		if (lock.writerThread == nullptr)
			break;

	while (anotherThreadHasWriteLock())
	{
		if(!busyWait)
			Thread::yield();
	}

	lock.numReadLocks++;
}

SimpleReadWriteLock::ScopedReadLock::~ScopedReadLock()
{
	lock.numReadLocks--;
}

SimpleReadWriteLock::ScopedWriteLock::ScopedWriteLock(SimpleReadWriteLock &lock_, bool busyWait) :
	lock(lock_)
{
	if (lock.writerThread != nullptr)
	{
		if (lock.writerThread == Thread::getCurrentThreadId())
		{
			holdsLock = false;
		}
		else
		{
			// oops, breaking the one-writer rule here...
			jassertfalse;
		}
		
		return;
	}

	for (int i = 100; --i >= 0;)
		if (lock.numReadLocks == 0)
			break;

	while (lock.numReadLocks > 0)
	{
		if(!busyWait)
			Thread::yield();
	}

	lock.writerThread = Thread::getCurrentThreadId();
	holdsLock = true;
}

SimpleReadWriteLock::ScopedWriteLock::~ScopedWriteLock()
{
	if (holdsLock)
		lock.writerThread = nullptr;
}


SimpleReadWriteLock::ScopedTryReadLock::ScopedTryReadLock(SimpleReadWriteLock &lock_) :
	lock(lock_)
{
	if (lock.writerThread == nullptr)
	{
		lock.numReadLocks++;
		is_locked = true;
	}
	else
	{
		is_locked = false;
	}
}

SimpleReadWriteLock::ScopedTryReadLock::~ScopedTryReadLock()
{
	if (is_locked)
		lock.numReadLocks--;
}
#endif

LockfreeAsyncUpdater::~LockfreeAsyncUpdater()
{
	cancelPendingUpdate();

	instanceCount--;
}

void LockfreeAsyncUpdater::triggerAsyncUpdate()
{
	pimpl.triggerAsyncUpdate();
}

void LockfreeAsyncUpdater::cancelPendingUpdate()
{
	pimpl.cancelPendingUpdate();
}

LockfreeAsyncUpdater::LockfreeAsyncUpdater() :
	pimpl(this)
{
	// If you're hitting this assertion, it means
	// that you are creating a lot of these objects
	// which clog the timer thread.
	// Consider using the standard AsyncUpdater instead
	jassert(instanceCount++ < 200);
}



int LockfreeAsyncUpdater::instanceCount = 0;


void FloatSanitizers::sanitizeArray(float* data, int size)
{
	uint32* dataAsInt = reinterpret_cast<uint32*>(data);

	for (int i = 0; i < size; i++)
	{
		const uint32 sample = *dataAsInt;
		const uint32 exponent = sample & 0x7F800000;

		const int aNaN = exponent < 0x7F800000;
		const int aDen = exponent > 0;

		*dataAsInt++ = sample * (aNaN & aDen);
	}
}

float FloatSanitizers::sanitizeFloatNumber(float& input)
{
	uint32* valueAsInt = reinterpret_cast<uint32*>(&input);
	const uint32 exponent = *valueAsInt & 0x7F800000;

	const int aNaN = exponent < 0x7F800000;
	const int aDen = exponent > 0;

	const uint32 sanitized = *valueAsInt * (aNaN & aDen);

	input = *reinterpret_cast<const float*>(&sanitized);

	return input;
}

void FloatSanitizers::Test::runTest()
{
	beginTest("Testing array method");

	float d[6];

	d[0] = INFINITY;
	d[1] = FLT_MIN / 20.0f;
	d[2] = FLT_MIN / -14.0f;
	d[3] = NAN;
	d[4] = 24.0f;
	d[5] = 0.0052f;

	sanitizeArray(d, 6);

	expectEquals<float>(d[0], 0.0f, "Infinity");
	expectEquals<float>(d[1], 0.0f, "Denormal");
	expectEquals<float>(d[2], 0.0f, "Negative Denormal");
	expectEquals<float>(d[3], 0.0f, "NaN");
	expectEquals<float>(d[4], 24.0f, "Normal Number");
	expectEquals<float>(d[5], 0.0052f, "Small Number");

	beginTest("Testing single method");

	float d0 = INFINITY;
	float d1 = FLT_MIN / 20.0f;
	float d2 = FLT_MIN / -14.0f;
	float d3 = NAN;
	float d4 = 24.0f;
	float d5 = 0.0052f;

	sanitizeFloatNumber(d0);
	sanitizeFloatNumber(d1);
	sanitizeFloatNumber(d2);
	sanitizeFloatNumber(d3);
	sanitizeFloatNumber(d4);
	sanitizeFloatNumber(d5);

	expectEquals<float>(d0, 0.0f, "Single Infinity");
	expectEquals<float>(d1, 0.0f, "Single Denormal");
	expectEquals<float>(d2, 0.0f, "Single Negative Denormal");
	expectEquals<float>(d3, 0.0f, "Single NaN");
	expectEquals<float>(d4, 24.0f, "Single Normal Number");
	expectEquals<float>(d5, 0.0052f, "Single Small Number");
}


HiseDeviceSimulator::DeviceType HiseDeviceSimulator::currentDevice = HiseDeviceSimulator::DeviceType::Desktop;

void HiseDeviceSimulator::init(AudioProcessor::WrapperType wrapper)
{
#if HISE_IOS
	const bool isIPad = SystemStats::getDeviceDescription() == "iPad";
	const bool isStandalone = wrapper != AudioProcessor::WrapperType::wrapperType_AudioUnitv3;

	if (isIPad)
		currentDevice = isStandalone ? DeviceType::iPad : DeviceType::iPadAUv3;
	else
		currentDevice = isStandalone ? DeviceType::iPhone : DeviceType::iPhoneAUv3;
#else
	ignoreUnused(wrapper);
	currentDevice = DeviceType::Desktop;
#endif
}

String HiseDeviceSimulator::getDeviceName(int index)
{
	DeviceType thisDevice = (index == -1) ? currentDevice : (DeviceType)index;

	switch (thisDevice)
	{
	case DeviceType::Desktop: return "Desktop";
	case DeviceType::iPad: return "iPad";
	case DeviceType::iPadAUv3: return "iPadAUv3";
	case DeviceType::iPhone: return "iPhone";
	case DeviceType::iPhoneAUv3: return "iPhoneAUv3";
	default:
		return{};
	}
}

bool HiseDeviceSimulator::fileNameContainsDeviceWildcard(const File& f)
{
	String fileName = f.getFileNameWithoutExtension();

	for (int i = 0; i < (int)DeviceType::numDeviceTypes; i++)
	{
		if (fileName.contains(getDeviceName(i)))
			return true;
	}

	return false;
}

Rectangle<int> HiseDeviceSimulator::getDisplayResolution()
{
	switch (currentDevice)
	{
	case HiseDeviceSimulator::DeviceType::Desktop:		return{ 0, 0, 1024, 768 };
	case HiseDeviceSimulator::DeviceType::iPad:			return{ 0, 0, 1024, 768 };
	case HiseDeviceSimulator::DeviceType::iPadAUv3:		return{ 0, 0, 1024, 335 };
	case HiseDeviceSimulator::DeviceType::iPhone:		return{ 0, 0, 568, 320 };
	case HiseDeviceSimulator::DeviceType::iPhoneAUv3:	return{ 0, 0, 568, 172 };
	case HiseDeviceSimulator::DeviceType::numDeviceTypes:
	default:
		return {};
	}
}


void PooledUIUpdater::Broadcaster::sendPooledChangeMessage()
{
	if (pending)
		return;

	if (handler != nullptr)
	{
		pending = true;
		handler.get()->pendingHandlers.push(this);
		
	}
		
	else
		jassertfalse; // you need to register it...
}

void SafeChangeListener::handlePooledMessage(PooledUIUpdater::Broadcaster* b)
{
	changeListenerCallback(dynamic_cast<SafeChangeBroadcaster*>(b));
}

TempoSyncer::TempoString TempoSyncer::tempoNames[numTempos];

float TempoSyncer::tempoFactors[numTempos];


int TempoSyncer::getTempoInSamples(double hostTempoBpm, double sampleRate, float tempoFactor)
{
	if (hostTempoBpm == 0.0) hostTempoBpm = 120.0;

	const float seconds = (60.0f / (float)hostTempoBpm) * tempoFactor;
	return (int)(seconds * (float)sampleRate);
}

int TempoSyncer::getTempoInSamples(double hostTempoBpm, double sampleRate, Tempo t)
{
	return getTempoInSamples(hostTempoBpm, sampleRate, getTempoFactor(t));
}

juce::StringArray TempoSyncer::getTempoNames()
{
	StringArray sa;
	for (int i = 0; i < numTempos; i++)
		sa.add(tempoNames[i]);
	
	return sa;
}

float TempoSyncer::getTempoInMilliSeconds(double hostTempoBpm, Tempo t)
{
	if (hostTempoBpm == 0.0) hostTempoBpm = 120.0;

	const float seconds = (60.0f / (float)hostTempoBpm) * getTempoFactor(t);
	return seconds * 1000.0f;
}

float TempoSyncer::getTempoInHertz(double hostTempoBpm, Tempo t)
{
	if (hostTempoBpm == 0.0) hostTempoBpm = 120.0;

	const float seconds = (60.0f / (float)hostTempoBpm) * getTempoFactor(t);

	return 1.0f / seconds;
}

String TempoSyncer::getTempoName(int t)
{
	jassert(t < numTempos);
	return t < numTempos ? tempoNames[t] : "Invalid";
}

hise::TempoSyncer::Tempo TempoSyncer::getTempoIndexForTime(double currentBpm, double milliSeconds)
{
	float max = 200000.0f;
	int index = -1;

	for (int i = 0; i < numTempos; i++)
	{
		const float thisDelta = fabsf(getTempoInMilliSeconds((float)currentBpm, (Tempo)i) - (float)milliSeconds);

		if (thisDelta < max)
		{
			max = thisDelta;
			index = i;
		}
	}

	if (index >= 0)
		return (Tempo)index;

	// Fallback Dummy
	return Tempo::Quarter;
}

hise::TempoSyncer::Tempo TempoSyncer::getTempoIndex(const String &t)
{
	for (int i = 0; i < numTempos; i++)
	{
		if(strcmp(t.getCharPointer().getAddress(), tempoNames[i]))
			return (Tempo)i;
	}
	
	jassertfalse;
	return Tempo::Quarter;
}

void TempoSyncer::initTempoData()
{
	auto setTempo = [](Tempo t, const char* name, float value)
	{
		strcpy(tempoNames[t], name);
		tempoFactors[t] = value;
	};

	setTempo(Whole, "1/1", 4.0f);
	setTempo(HalfDuet, "1/2D", 2.0f * 1.5f);
	setTempo(Half, "1/2", 2.0f);
	setTempo(HalfTriplet, "1/2T", 4.0f / 3.0f);
	setTempo(QuarterDuet, "1/4D", 1.0f * 1.5f);
	setTempo(Quarter, "1/4", 1.0f);
	setTempo(QuarterTriplet, "1/4T", 2.0f / 3.0f);
	setTempo(EighthDuet, "1/8D", 0.5f * 1.5f);
	setTempo(Eighth, "1/8", 0.5f);
	setTempo(EighthTriplet, "1/8T", 1.0f / 3.0f);
	setTempo(SixteenthDuet, "1/16D", 0.25f * 1.5f);
	setTempo(Sixteenth, "1/16", 0.25f);
	setTempo(SixteenthTriplet, "1/16T", 0.5f / 3.0f);
	setTempo(ThirtyTwoDuet, "1/32D", 0.125f * 1.5f);
	setTempo(ThirtyTwo, "1/32", 0.125f);
	setTempo(ThirtyTwoTriplet, "1/32T", 0.25f / 3.0f);
	setTempo(SixtyForthDuet, "1/64D", 0.125f * 0.5f * 1.5f);
	setTempo(SixtyForth, "1/64", 0.125f * 0.5f);
	setTempo(SixtyForthTriplet, "1/64T", 0.125f / 3.0f);
}

float TempoSyncer::getTempoFactor(Tempo t)
{
	jassert(t < numTempos);
	return t < numTempos ? tempoFactors[(int)t] : tempoFactors[(int)Tempo::Quarter];
}

void ScrollbarFader::Laf::drawScrollbar(Graphics& g, ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
    g.fillAll(bg);

    float alpha = 0.3f;

    if (isMouseOver || isMouseDown)
        alpha += 0.1f;

    if (isMouseDown)
        alpha += 0.1f;

    g.setColour(Colours::white.withAlpha(alpha));

    auto area = Rectangle<int>(x, y, width, height).toFloat();

    if (isScrollbarVertical)
    {
        area.removeFromTop((float)thumbStartPosition);
        area = area.withHeight((float)thumbSize);
    }
    else
    {
        area.removeFromLeft((float)thumbStartPosition);
        area = area.withWidth((float)thumbSize);
    }

    auto cornerSize = jmin(area.getWidth(), area.getHeight());

    area = area.reduced(4.0f);
    cornerSize = jmin(area.getWidth(), area.getHeight());
    
    g.fillRoundedRectangle(area, cornerSize / 2.0f);
}

void ScrollbarFader::timerCallback()
{
    if (!fadeOut)
    {
        fadeOut = true;
        startTimer(30);
    }
    
    {
        if(auto first = scrollbars.getFirst().getComponent())
        {
            auto a = first->getAlpha();
            a = jmax(0.1f, a - 0.05f);

            for(auto sb: scrollbars)
            {
                if(sb != nullptr)
                    sb->setAlpha(a);
            }
            
            if (a <= 0.1f)
            {
                fadeOut = false;
                stopTimer();
            }
        }
    }
}

void ScrollbarFader::startFadeOut()
{
    for(auto sb: scrollbars)
    {
        if(sb != nullptr)
            sb->setAlpha(1.0f);
    }
    
    fadeOut = false;
    startTimer(500);
}

void FFTHelpers::applyWindow(WindowType t, float* data, int s, bool normalise)
{
    using DspWindowType = juce::dsp::WindowingFunction<float>;
    
    switch (t)
    {
    case Rectangle:
        break;
    case BlackmanHarris:
        DspWindowType(s, DspWindowType::blackmanHarris, normalise).multiplyWithWindowingTable(data, s);
        break;
    case Hamming:
        DspWindowType(s, DspWindowType::hamming, normalise).multiplyWithWindowingTable(data, s);
        break;
    case Hann:
        DspWindowType(s, DspWindowType::hann, normalise).multiplyWithWindowingTable(data, s);
        break;
    case Kaiser:
        DspWindowType(s, DspWindowType::kaiser, normalise, 15.0f).multiplyWithWindowingTable(data, s);
        break;
    case Triangle:
        DspWindowType(s, DspWindowType::triangular, normalise).multiplyWithWindowingTable(data, s);
        break;
    case FlatTop:
        DspWindowType(s, DspWindowType::flatTop, normalise).multiplyWithWindowingTable(data, s);
        break;
    default:
        jassertfalse;
        FloatVectorOperations::clear(data, s);
        break;
    }
}

void FFTHelpers::applyWindow(WindowType t, AudioSampleBuffer& b, bool normalise)
{
    auto s = b.getNumSamples() / 2;
    auto data = b.getWritePointer(0);

    applyWindow(t, data, s, normalise);
}

float FFTHelpers::getFreqForLogX(float xPos, float width)
{
	auto lowFreq = 20;
	auto highFreq = 20000.0;

	return lowFreq * pow((highFreq / lowFreq), ((xPos - 2.5f) / (width - 5.0f)));
}

float FFTHelpers::getPixelValueForLogXAxis(float freq, float width)
{
	auto lowFreq = 20;
	auto highFreq = 20000.0;

	return (width - 5) * (log(freq / lowFreq) / log(highFreq / lowFreq)) + 2.5f;
}

juce::PixelARGB Spectrum2D::LookupTable::getColouredPixel(float normalisedInput)
{
	auto lutValue = data[jlimit(0, LookupTableSize - 1, roundToInt(normalisedInput * (float)LookupTableSize))];
	float a = JUCE_LIVE_CONSTANT_OFF(0.3f);
	auto v = jlimit(0.0f, 1.0f, a + (1.0f - a) * normalisedInput);
	auto r = (float)lutValue.getRed() * v;
	auto g = (float)lutValue.getGreen() * v;
	auto b = (float)lutValue.getBlue() * v;
	lutValue.setARGB(255, (uint8)r, (uint8)g, (uint8)b);
	return lutValue;
}

void Spectrum2D::LookupTable::setColourScheme(ColourScheme cs)
{
	ColourGradient grad(Colours::black, 0.0f, 0.0f, Colours::white, 1.0f, 1.0f, false);

	if (cs != colourScheme)
	{
		colourScheme = cs;

		switch (colourScheme)
		{
		case ColourScheme::violetToOrange:
		{
			grad.addColour(0.2f, Colour(0xFF537374).withMultipliedBrightness(0.5f));
			grad.addColour(0.4f, Colour(0xFF57339D).withMultipliedBrightness(0.8f));

			grad.addColour(0.6f, Colour(0xFFB35259).withMultipliedBrightness(0.9f));
			grad.addColour(0.8f, Colour(0xFFFF8C00));
			grad.addColour(0.9f, Colour(0xFFC0A252));
			break;
		}
		case ColourScheme::blackWhite: break;
		case ColourScheme::rainbow:
		{
			grad.addColour(0.2f, Colours::blue);
			grad.addColour(0.4f, Colours::green);
			grad.addColour(0.6f, Colours::yellow);
			grad.addColour(0.8f, Colours::orange);
			grad.addColour(0.9f, Colours::red);
			break;
		}
		case ColourScheme::hiseColours:
		{
			grad.addColour(0.33f, Colour(0xff3a6666));
			grad.addColour(0.66f, Colour(SIGNAL_COLOUR));
		}
        default:
            break;
		}

		grad.createLookupTable(data, LookupTableSize);
	}
}

Spectrum2D::LookupTable::LookupTable()
{
	setColourScheme(ColourScheme::violetToOrange);

	
}

Image Spectrum2D::createSpectrumImage(AudioSampleBuffer& lastBuffer)
{
    auto newImage = Image(Image::ARGB, lastBuffer.getNumSamples(), lastBuffer.getNumChannels() / 2, true);
    auto maxLevel = lastBuffer.getMagnitude(0, lastBuffer.getNumSamples());

    auto s2dHalf = parameters->Spectrum2DSize / 2;
    
    if (maxLevel == 0.0f)
        return newImage;

    for (int y = 0; y < s2dHalf; y++)
    {
        auto skewedProportionY = holder->getYPosition((float)y / (float)s2dHalf);
        auto fftDataIndex = jlimit(0, s2dHalf-1, (int)(skewedProportionY * (int)s2dHalf));

        for (int i = 0; i < lastBuffer.getNumSamples(); i++)
        {
            auto s = lastBuffer.getSample(fftDataIndex, i);

            s *= (1.0f / maxLevel);

            auto alpha = jlimit(0.0f, 1.0f, s);

            alpha = holder->getXPosition(alpha);
            alpha = std::pow(alpha, JUCE_LIVE_CONSTANT_OFF(0.6f));

			auto lutValue = parameters->lut->getColouredPixel(alpha);
            newImage.setPixelAt(i, y, lutValue);//Colour::fromHSV(hue, JUCE_LIVE_CONSTANT(1.0f), 1.0f, alpha));
        }
    }

    return newImage;
}

AudioSampleBuffer Spectrum2D::createSpectrumBuffer()
{
    auto fft = juce::dsp::FFT(parameters->order);

    auto numSamplesToFill = jmax(0, originalSource.getNumSamples() / parameters->Spectrum2DSize * parameters->oversamplingFactor - 1);

    if (numSamplesToFill == 0)
        return {};

    auto paddingSize = JUCE_LIVE_CONSTANT_OFF(0);
    
    AudioSampleBuffer b(parameters->Spectrum2DSize, numSamplesToFill);
    b.clear();

    for (int i = 0; i < numSamplesToFill; i++)
    {
        auto offset = (i * parameters->Spectrum2DSize) / parameters->oversamplingFactor;
        AudioSampleBuffer sb(1, parameters->Spectrum2DSize * 2);
        sb.clear();

        auto numToCopy = jmin(parameters->Spectrum2DSize, originalSource.getNumSamples() - offset);

        FloatVectorOperations::copy(sb.getWritePointer(0), originalSource.getReadPointer(0, offset), numToCopy);

        FFTHelpers::applyWindow(parameters->currentWindowType, sb);

        fft.performRealOnlyForwardTransform(sb.getWritePointer(0), false);

        AudioSampleBuffer out(1, parameters->Spectrum2DSize);

        FFTHelpers::toFreqSpectrum(sb, out);
        FFTHelpers::scaleFrequencyOutput(out, false);

        for (int c = 0; c < b.getNumChannels(); c++)
        {
            b.setSample(c, jmin(numSamplesToFill-1, i+paddingSize), out.getSample(0, c));
        }
    }
    
    return b;
}

void Spectrum2D::Parameters::set(const Identifier& id, int value, NotificationType n)
{
	jassert(getAllIds().contains(id));

	if (id == Identifier("FFTSize"))
	{
		order = jlimit(7, 13, value);
		Spectrum2DSize = roundToInt(std::pow(2.0, (double)order));
	}
    if(id == Identifier("DynamicRange"))
        minDb = value;
	if (id == Identifier("Oversampling"))
		oversamplingFactor = value;
	if (id == Identifier("ColourScheme"))
		lut->setColourScheme((LookupTable::ColourScheme)value);
	if (id == Identifier("WindowType"))
		currentWindowType = (FFTHelpers::WindowType)value;
	if (n != dontSendNotification)
		notifier.sendMessage(n, id, value);
}

int Spectrum2D::Parameters::get(const Identifier& id) const
{
	jassert(getAllIds().contains(id));

	if (id == Identifier("FFTSize"))
		return order;
    if (id == Identifier("DynamicRange"))
        return minDb;
	if (id == Identifier("Oversampling"))
		return oversamplingFactor;
	if (id == Identifier("ColourScheme"))
		return (int)lut->colourScheme;
	if (id == Identifier("WindowType"))
		return currentWindowType;

	return 0;
}

void Spectrum2D::Parameters::saveToJSON(var v) const
{
	if (auto dyn = v.getDynamicObject())
	{
		for (auto id : getAllIds())
			dyn->setProperty(id, get(id));
	}
}

void Spectrum2D::Parameters::loadFromJSON(const var& v)
{
	for (auto id : getAllIds())
	{
		if (v.hasProperty(id))
			set(id, v.getProperty(id, ""), dontSendNotification);
	}

	notifier.sendMessage(sendNotificationAsync, "Allofem", var());
}

juce::Array<juce::Identifier> Spectrum2D::Parameters::getAllIds()
{
	static const Array<Identifier> ids =
	{
		Identifier("FFTSize"),
		Identifier("DynamicRange"),
		Identifier("Oversampling"),
		Identifier("ColourScheme"),
		Identifier("WindowType")
	};
	
	return ids;
}

Spectrum2D::Parameters::Editor::Editor(Parameters::Ptr p) :
	param(p)
{
	claf = new GlobalHiseLookAndFeel();

	setName("Spectrogram Properties");

	addEditor("FFTSize");
	addEditor("Oversampling");
	addEditor("WindowType");
    addEditor("DynamicRange");
	addEditor("ColourScheme");

	setSize(450, RowHeight * editors.size() + 60);
}

void Spectrum2D::Parameters::Editor::addEditor(const Identifier& id)
{
	auto cb = new ComboBox();
	cb->setName(id.toString());

	cb->setLookAndFeel(claf);
	GlobalHiseLookAndFeel::setDefaultColours(*cb);
	
	if (id == Identifier("FFTSize"))
	{
		for (int i = 7; i < 14; i++)
		{
			auto id = i + 1;
			auto pow = std::pow(2, i);
			cb->addItem(String(pow), id);
		}
	}
    if(id == Identifier("DynamicRange"))
    {
        cb->addItem("60dB", 61);
        cb->addItem("80dB", 81);
        cb->addItem("100dB", 101);
        cb->addItem("110dB", 111);
        cb->addItem("120dB", 121);
        cb->addItem("130dB", 131);
    }
	if (id == Identifier("ColourScheme"))
	{
		cb->addItemList(LookupTable::getColourSchemes(), 1);
	}
	if (id == Identifier("Oversampling"))
	{
		cb->addItem("1x", 2);
		cb->addItem("2x", 3);
		cb->addItem("4x", 5);
		cb->addItem("8x", 9);
	}
	if (id == Identifier("WindowType"))
	{
		for (auto w : FFTHelpers::getAvailableWindowTypes())
			cb->addItem(FFTHelpers::getWindowType(w), (int)w + 1);
	}

	addAndMakeVisible(cb);
	editors.add(cb);
	cb->addListener(this);


	cb->setSelectedId(param->get(id)+1, dontSendNotification);

	auto l = new Label();
	l->setEditable(false);
	l->setFont(GLOBAL_FONT());
	l->setText(id.toString(), dontSendNotification);
	l->setColour(Label::ColourIds::textColourId, Colours::white);

	addAndMakeVisible(l);
	labels.add(l);

}

void Spectrum2D::Parameters::Editor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	auto id = Identifier(comboBoxThatHasChanged->getName());
	auto v = comboBoxThatHasChanged->getSelectedId() - 1;

	param->set(id, v, sendNotificationSync);
	repaint();
}

void Spectrum2D::Parameters::Editor::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
	auto b = getLocalBounds().removeFromBottom(60);

	auto specArea = b.reduced(12);
	auto textArea = specArea.removeFromTop(13);
	auto range = param->get("DynamicRange");

	auto parts = (float)specArea.getWidth() / (float)(range / 10);
	auto tparts = (float)textArea.getWidth() / (float)(range / 10);

	auto meterArea = specArea.removeFromTop(8).toFloat();

	g.setColour(Colours::white.withAlpha(0.8f));

	g.setFont(GLOBAL_FONT().withHeight(12.0f));

	for (int i = 0; i < range; i += 10)
	{
		auto mm = meterArea.removeFromLeft(parts);
		g.drawVerticalLine(mm.getX(), meterArea.getY(), meterArea.getBottom());

		auto tm = textArea.removeFromLeft(tparts).toFloat();

		String s;
		s << "-" << String(range - i) << "dB";
		g.drawText(s, tm, Justification::centredLeft);
	}

	for (int i = 0; i < specArea.getWidth(); i+= 2)
	{
		auto ninput = (float)i / (float)specArea.getWidth();
		auto p = param->lut->getColouredPixel(ninput);

		Colour c(p.getNativeARGB());
		g.setColour(c);
		g.fillRect(specArea.getX() + i, specArea.getY(), 3, specArea.getHeight());
	}
}

void Spectrum2D::Parameters::Editor::resized()
{
	auto b = getLocalBounds();
	
	b.removeFromLeft(12);

	for (int i = 0; i < editors.size(); i++)
	{
		auto r = b.removeFromTop(RowHeight);
		labels[i]->setBounds(r.removeFromLeft(128));
		editors[i]->setBounds(r);
	}	
}



}
