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


#if JUCE_IOS
#else
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
#if JUCE_IOS
#else
	oldMXCSR = _mm_getcsr();
	int newMXCSR = oldMXCSR | 0x8040;
	_mm_setcsr(newMXCSR);
#endif
}

ScopedNoDenormals::~ScopedNoDenormals()
{
#if JUCE_IOS
#else
	_mm_setcsr(oldMXCSR);
#endif
}



SimpleReadWriteLock::ScopedReadLock::ScopedReadLock(SimpleReadWriteLock &lock_, bool busyWait) :
	lock(lock_)
{
	for (int i = 2000; --i >= 0;)
		if (lock.writerThread == nullptr)
			break;

	while (lock.writerThread != nullptr)
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

	return *reinterpret_cast<const float*>(&sanitized);
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

	d0 = sanitizeFloatNumber(d0);
	d1 = sanitizeFloatNumber(d1);
	d2 = sanitizeFloatNumber(d2);
	d3 = sanitizeFloatNumber(d3);
	d4 = sanitizeFloatNumber(d4);
	d5 = sanitizeFloatNumber(d5);

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




}
