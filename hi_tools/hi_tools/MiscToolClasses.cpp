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

CopyPasteTargetHandler* CopyPasteTarget::HandlerFunction::getHandler(Component* c)
{
	return f(c);
}

CopyPasteTarget::CopyPasteTarget(): isSelected(false)
{}

CopyPasteTarget::~CopyPasteTarget()
{
	masterReference.clear();
}

bool CopyPasteTarget::isSelectedForCopyAndPaste()
{ return isSelected; }

CopyPasteTargetHandler* CopyPasteTarget::getNothing(Component*)
{ return nullptr; }

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

Array<Range<int>> RegexFunctions::findRangesThatMatchWildcard(const String &regexWildCard, const String &stringToTest)
{
    Array<Range<int>> matches;
    
    int pos = 0;
    
    String remainingText = stringToTest;
    StringArray m = getFirstMatch(regexWildCard, remainingText);

    
    
    while (m.size() != 0 && m[0].length() != 0)
    {
        auto thisPos = remainingText.indexOf(m[0]);
        
        matches.add({pos + thisPos, pos + thisPos + m[0].length() });
        
        
        remainingText = remainingText.fromFirstOccurrenceOf(m[0], false, false);
        pos = matches.getLast().getEnd();
        
        m = getFirstMatch(regexWildCard, remainingText);
    }

    return matches;
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

ComplexDataUIBase::SourceListener::~SourceListener()
{}

ComplexDataUIBase::EditorBase::~EditorBase()
{}

void ComplexDataUIBase::EditorBase::setSpecialLookAndFeel(LookAndFeel* l, bool shouldOwn)
{
	laf = l;

	if (shouldOwn)
		ownedLaf = l;

	if (auto asComponent = dynamic_cast<Component*>(this))
		asComponent->setLookAndFeel(l);
}




void ComplexDataUIBase::SourceWatcher::setNewSource(ComplexDataUIBase* newSource)
{
	if (newSource != currentSource)
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->sourceHasChanged(currentSource, newSource);
		}

		currentSource = newSource;
	}
}

void ComplexDataUIBase::SourceWatcher::addSourceListener(SourceListener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void ComplexDataUIBase::SourceWatcher::removeSourceListener(SourceListener* l)
{
	listeners.removeAllInstancesOf(l);
}

ComplexDataUIBase::~ComplexDataUIBase()
{}

void ComplexDataUIBase::setGlobalUIUpdater(PooledUIUpdater* updater)
{
	internalUpdater.setUpdater(updater);
}

void ComplexDataUIBase::sendDisplayIndexMessage(float n)
{
	internalUpdater.sendDisplayChangeMessage(n, sendNotificationAsync);
}

ComplexDataUIUpdaterBase& ComplexDataUIBase::getUpdater()
{ return internalUpdater; }

const ComplexDataUIUpdaterBase& ComplexDataUIBase::getUpdater() const
{ return internalUpdater; }

void ComplexDataUIBase::setUndoManager(UndoManager* managerToUse)
{
	undoManager = managerToUse;
}

UndoManager* ComplexDataUIBase::getUndoManager(bool useUndoManager)
{ return useUndoManager ? undoManager : nullptr; }

hise::SimpleReadWriteLock& ComplexDataUIBase::getDataLock() const
{ return dataLock; }

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

    if(srcLength >= NUM_MAX_CHARS || dstLength >= NUM_MAX_CHARS)
        return INT_MAX;
    
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

	LockfreeAsyncUpdater::TimerPimpl::TimerPimpl(LockfreeAsyncUpdater* p_):
		parent(*p_)
	{
		dirty = false;
		startTimer(30);
	}

	LockfreeAsyncUpdater::TimerPimpl::~TimerPimpl()
	{
		dirty = false;
		stopTimer();
	}

	void LockfreeAsyncUpdater::TimerPimpl::timerCallback()
	{
		bool v = true;
		if (dirty.compare_exchange_strong(v, false))
		{
			parent.handleAsyncUpdate();
		}
	}

	void LockfreeAsyncUpdater::TimerPimpl::triggerAsyncUpdate()
	{
		dirty.store(true);
	}

	void LockfreeAsyncUpdater::TimerPimpl::cancelPendingUpdate()
	{
		dirty.store(false);
	}

	void LockfreeAsyncUpdater::suspend(bool shouldBeSuspended)
	{
		pimpl.suspendTimer(shouldBeSuspended);
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


bool FloatSanitizers::isSilence(const float value)
{
	static const float Silence = std::pow(10.0f, (float)HISE_SILENCE_THRESHOLD_DB * -0.05f);
	static const float MinusSilence = -1.0f * Silence;
	return value < Silence && value > MinusSilence;
}

bool FloatSanitizers::isNotSilence(const float value)
{
	return !isSilence(value);
}

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

double FloatSanitizers::sanitizeDoubleNumber(double& input)
{
	uint64_t* valueAsInt = reinterpret_cast<uint64_t*>(&input);
	const uint64_t exponent = *valueAsInt & 0x7FF0000000000000;

	const int aNaN = exponent < 0x7FF0000000000000;
	const int aDen = exponent > 0;

	const uint64_t sanitized = *valueAsInt * (aNaN & aDen);

	input = *reinterpret_cast<const double*>(&sanitized);

	return input;
}

FloatSanitizers::Test::Test():
	UnitTest("Testing float sanitizer")
{

}

void FloatSanitizers::Test::runTest()
{
	testSingleSanitizer<float>();
	testSingleSanitizer<double>();
	testArray();
}

void FloatSanitizers::Test::testArray()
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

ScrollbarFader::ScrollbarFader() = default;

ScrollbarFader::~ScrollbarFader()
{
	for(auto sb: scrollbars)
	{
		if(sb != nullptr)
		{
			sb->removeListener(this);
			sb->setLookAndFeel(nullptr);
		}
	}
}

void ScrollbarFader::Laf::drawStretchableLayoutResizerBar(Graphics& g, int w, int h, bool cond, bool isMouseOver,
	bool isMouseDragging)
{
	float alpha = 0.0f;
            
	if(isMouseOver)
		alpha += 0.3f;
            
	if(isMouseDragging)
		alpha += 0.3f;
            
	g.setColour(Colour(SIGNAL_COLOUR).withAlpha(alpha));
            
	Rectangle<float> area(0.0f, 0.0f, (float)w, (float)h);
            
	area = area.reduced(1.0f);
	g.fillRoundedRectangle(area, jmin(area.getWidth() / 2.0f, area.getHeight() / 2.0f));
}

void ScrollbarFader::scrollBarMoved(ScrollBar* sb, double)
{
	if(sb->getRangeLimit() != sb->getCurrentRange())
	{
		sb->setAlpha(1.0f);
		startFadeOut();
	}
}

void ScrollbarFader::addScrollBarToAnimate(ScrollBar& b)
{
	b.addListener(this);
	b.setLookAndFeel(&slaf);
	scrollbars.add({&b});
}

#if !HISE_NO_GUI_TOOLS

GlContextHolder::GlContextHolder(juce::Component& topLevelComponent): parent(topLevelComponent)
{
	context.setRenderer(this);
	context.setContinuousRepainting(true);
	context.setComponentPaintingEnabled(true);
	context.attachTo(parent);
}

void GlContextHolder::detach()
{
	jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

	const int n = clients.size();
	for (int i = 0; i < n; ++i)
		if (juce::Component* comp = clients.getReference(i).c)
			comp->removeComponentListener(this);

	context.detach();
	context.setRenderer(nullptr);
}

void GlContextHolder::registerOpenGlRenderer(juce::Component* child)
{
	jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

	if (dynamic_cast<juce::OpenGLRenderer*> (child) != nullptr)
	{
		if (findClientIndexForComponent(child) < 0)
		{
			clients.add(Client(child, (parent.isParentOf(child) ? Client::State::running : Client::State::suspended)));
			child->addComponentListener(this);
		}
	}
	else
	jassertfalse;
}

void GlContextHolder::unregisterOpenGlRenderer(juce::Component* child)
{
	jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

	const int index = findClientIndexForComponent(child);

	if (index >= 0)
	{
		Client& client = clients.getReference(index);
		{
			juce::ScopedLock stateChangeLock(stateChangeCriticalSection);
			client.nextState = Client::State::suspended;
		}

		child->removeComponentListener(this);
		context.executeOnGLThread([this](juce::OpenGLContext&)
		{
			checkComponents(false, false);
		}, true);
		client.c = nullptr;

		clients.remove(index);
	}
}

void GlContextHolder::setBackgroundColour(const juce::Colour c)
{
	backgroundColour = c;
}

void GlContextHolder::checkComponents(bool isClosing, bool isDrawing)
{
	juce::Array<juce::Component*> initClients, runningClients;

	{
		juce::ScopedLock arrayLock(clients.getLock());
		juce::ScopedLock stateLock(stateChangeCriticalSection);

		const int n = clients.size();

		for (int i = 0; i < n; ++i)
		{
			Client& client = clients.getReference(i);
			if (client.c != nullptr)
			{
				Client::State nextState = (isClosing ? Client::State::suspended : client.nextState);

				if (client.currentState == Client::State::running   && nextState == Client::State::running)   runningClients.add(client.c);
				else if (client.currentState == Client::State::suspended && nextState == Client::State::running)   initClients.add(client.c);
				else if (client.currentState == Client::State::running   && nextState == Client::State::suspended)
				{
					dynamic_cast<juce::OpenGLRenderer*> (client.c)->openGLContextClosing();
				}

				client.currentState = nextState;
			}
		}
	}

	for (int i = 0; i < initClients.size(); ++i)
		dynamic_cast<juce::OpenGLRenderer*> (initClients.getReference(i))->newOpenGLContextCreated();

	if (runningClients.size() > 0 && isDrawing)
	{
		const float displayScale = static_cast<float> (context.getRenderingScale());
		const juce::Rectangle<int> parentBounds = (parent.getLocalBounds().toFloat() * displayScale).getSmallestIntegerContainer();

		for (int i = 0; i < runningClients.size(); ++i)
		{
			juce::Component* comp = runningClients.getReference(i);
				
			juce::Rectangle<int> r = (parent.getLocalArea(comp, comp->getLocalBounds()).toFloat() * displayScale).getSmallestIntegerContainer();
			juce::gl::glViewport((GLint)r.getX(),
			                     (GLint)parentBounds.getHeight() - (GLint)r.getBottom(),
			                     (GLsizei)r.getWidth(), (GLsizei)r.getHeight());
			juce::OpenGLHelpers::clear(backgroundColour);

			dynamic_cast<juce::OpenGLRenderer*> (comp)->renderOpenGL();
		}
	}
}

void GlContextHolder::componentParentHierarchyChanged(juce::Component& component)
{
	if (Client* client = findClientForComponent(&component))
	{
		juce::ScopedLock stateChangeLock(stateChangeCriticalSection);

		client->nextState = (parent.isParentOf(&component) && component.isVisible() ? Client::State::running : Client::State::suspended);
	}
}

void GlContextHolder::componentVisibilityChanged(juce::Component& component)
{
	if (Client* client = findClientForComponent(&component))
	{
		juce::ScopedLock stateChangeLock(stateChangeCriticalSection);

		client->nextState = (parent.isParentOf(&component) && component.isVisible() ? Client::State::running : Client::State::suspended);
	}
}

void GlContextHolder::componentBeingDeleted(juce::Component& component)
{
	const int index = findClientIndexForComponent(&component);

	if (index >= 0)
	{
		Client& client = clients.getReference(index);

		// You didn't call unregister before deleting this component
		jassert(client.nextState == Client::State::suspended);
		client.nextState = Client::State::suspended;

		component.removeComponentListener(this);
		context.executeOnGLThread([this](juce::OpenGLContext&)
		{
			checkComponents(false, false);
		}, true);

		client.c = nullptr;

		clients.remove(index);
	}
}

void GlContextHolder::newOpenGLContextCreated()
{
	checkComponents(false, false);
}

void GlContextHolder::renderOpenGL()
{
	juce::OpenGLHelpers::clear(backgroundColour);
	checkComponents(false, true);
}

void GlContextHolder::openGLContextClosing()
{
	checkComponents(true, false);
}

GlContextHolder::Client::Client(juce::Component* comp, State nextStateToUse): c(comp), currentState(State::suspended), nextState(nextStateToUse)
{}

int GlContextHolder::findClientIndexForComponent(juce::Component* comp)
{
	const int n = clients.size();
	for (int i = 0; i < n; ++i)
		if (comp == clients.getReference(i).c)
			return i;

	return -1;
}

GlContextHolder::Client* GlContextHolder::findClientForComponent(juce::Component* comp)
{
	const int index = findClientIndexForComponent(comp);
	if (index >= 0)
		return &clients.getReference(index);

	return nullptr;
}



KeyPress TopLevelWindowWithKeyMappings::getKeyPressFromString(Component* c, const String& s)
{
	if (s.isEmpty())
		return {};

	if (s.startsWith("$"))
	{
		auto id = Identifier(s.removeCharacters("$"));
		return getFirstKeyPress(c, id);
	}
	else
		return KeyPress::createFromDescription(s);
}

void TopLevelWindowWithKeyMappings::addShortcut(Component* c, const String& category, const Identifier& id,
	const String& description, const KeyPress& k)
{
	if (auto t = getFromComponent(c))
	{
		if (t->shortcutIds.contains(id))
			return;

		auto info = ApplicationCommandInfo(t->shortcutIds.size() + 1);
		t->shortcutIds.add(id);

		info.categoryName = category;
		info.shortName << description << " ($" << id.toString() << ")";
		info.defaultKeypresses.add(k);
		t->m.registerCommand(info);
		t->keyMap.resetToDefaultMapping(info.commandID);
	}
}

KeyPress TopLevelWindowWithKeyMappings::getFirstKeyPress(Component* c, const Identifier& id)
{
	if (auto t = getFromComponent(c))
	{
		if (auto idx = t->shortcutIds.indexOf(id) + 1)
			return t->keyMap.getKeyPressesAssignedToCommand(idx).getFirst();
	}

	return KeyPress();
}

bool TopLevelWindowWithKeyMappings::matches(Component* c, const KeyPress& k, const Identifier& id)
{
	if (auto t = getFromComponent(c))
	{
		if (auto idx = t->shortcutIds.indexOf(id) + 1)
		{
			return t->keyMap.getKeyPressesAssignedToCommand(idx).contains(k);
		}
	}

	return false;
}

KeyPressMappingSet& TopLevelWindowWithKeyMappings::getKeyPressMappingSet()
{ return keyMap; }

TopLevelWindowWithKeyMappings::TopLevelWindowWithKeyMappings():
	keyMap(m)
{}

TopLevelWindowWithKeyMappings::~TopLevelWindowWithKeyMappings()
{
	jassert(loaded);
	// If you hit this assertion, you need to store the data in your
	// sub class constructor
	jassert(saved);
}

void TopLevelWindowWithKeyMappings::initialiseAllKeyPresses()
{
	initialised = true;
}

void TopLevelWindowWithKeyMappings::saveKeyPressMap()
{
	auto f = getKeyPressSettingFile();
	auto xml = keyMap.createXml(true);
	f.replaceWithText(xml->createDocument(""));
	saved = true;
}

void TopLevelWindowWithKeyMappings::loadKeyPressMap()
{
	initialiseAllKeyPresses();

	auto f = getKeyPressSettingFile();

	if (auto xml = XmlDocument::parse(f))
		keyMap.restoreFromXml(*xml);

	loaded = true;
}

TopLevelWindowWithKeyMappings* TopLevelWindowWithKeyMappings::getFromComponent(Component* c)
{
	if (auto same = dynamic_cast<TopLevelWindowWithKeyMappings*>(c))
		return same;

	return c->findParentComponentOfClass<TopLevelWindowWithKeyMappings>();
}

TopLevelWindowWithOptionalOpenGL::~TopLevelWindowWithOptionalOpenGL()
{
	// Must call detachOpenGL() in derived destructor!
		
}

Component* TopLevelWindowWithOptionalOpenGL::findRoot(Component* c)
{
	return dynamic_cast<Component*>(c->findParentComponentOfClass<TopLevelWindowWithOptionalOpenGL>());
}

TopLevelWindowWithOptionalOpenGL::ScopedRegisterState::ScopedRegisterState(TopLevelWindowWithOptionalOpenGL& t_,
	Component* c_):
	t(t_),
	c(c_)
{
	if (t.contextHolder != nullptr)
		t.contextHolder->registerOpenGlRenderer(c);
}

TopLevelWindowWithOptionalOpenGL::ScopedRegisterState::~ScopedRegisterState()
{
	if (t.contextHolder != nullptr)
		t.contextHolder->unregisterOpenGlRenderer(c);
}

bool TopLevelWindowWithOptionalOpenGL::isOpenGLEnabled() const
{ return contextHolder != nullptr; }

void TopLevelWindowWithOptionalOpenGL::detachOpenGl()
{
	if (contextHolder != nullptr)
		contextHolder->detach();
}

void TopLevelWindowWithOptionalOpenGL::setEnableOpenGL(Component* c)
{
	contextHolder = new GlContextHolder(*c);
}

void TopLevelWindowWithOptionalOpenGL::addChildComponentWithOpenGLRenderer(Component* c)
{
	if (contextHolder != nullptr)
	{
		contextHolder->registerOpenGlRenderer(c);
	}
}

void TopLevelWindowWithOptionalOpenGL::removeChildComponentWithOpenGLRenderer(Component* c)
{
	if (contextHolder != nullptr)
		contextHolder->unregisterOpenGlRenderer(c);
}

#endif

SuspendableTimer::Manager::~Manager()
{}

SuspendableTimer::SuspendableTimer():
	internalTimer(*this)
{}

SuspendableTimer::~SuspendableTimer()
{ internalTimer.stopTimer(); }

void SuspendableTimer::startTimer(int milliseconds)
{
	lastTimerInterval = milliseconds;

#if !HISE_HEADLESS
	if (!suspended)
		internalTimer.startTimer(milliseconds);
#endif
}

void SuspendableTimer::stopTimer()
{
	lastTimerInterval = -1;

	if (!suspended)
	{
		internalTimer.stopTimer();
	}
	else
	{
		// Must be stopped by suspendTimer
		jassert(!internalTimer.isTimerRunning());
	}
}

void SuspendableTimer::suspendTimer(bool shouldBeSuspended)
{
	if (shouldBeSuspended != suspended)
	{
		suspended = shouldBeSuspended;

#if !HISE_HEADLESS
		if (suspended)
			internalTimer.stopTimer();
		else if (lastTimerInterval != -1)
			internalTimer.startTimer(lastTimerInterval);
#endif
	}
}

bool SuspendableTimer::isSuspended()
{
	return suspended;
}

SuspendableTimer::Internal::Internal(SuspendableTimer& parent_):
	parent(parent_)
{}

void SuspendableTimer::Internal::timerCallback()
{ parent.timerCallback(); }

PooledUIUpdater::PooledUIUpdater():
	pendingHandlers(8192)
{
	suspendTimer(false);
	startTimer(30);
}

PooledUIUpdater::Listener::~Listener()
{}

PooledUIUpdater::SimpleTimer::SimpleTimer(PooledUIUpdater* h, bool shouldStart):
	updater(h)
{
	if(updater != nullptr && shouldStart)
		start();
}

PooledUIUpdater::SimpleTimer::~SimpleTimer()
{
	stop();
}

void PooledUIUpdater::SimpleTimer::start()
{
	startOrStop(true);
}

void PooledUIUpdater::SimpleTimer::stop()
{
	startOrStop(false);
}

bool PooledUIUpdater::SimpleTimer::isTimerRunning() const
{ return isRunning; }

void PooledUIUpdater::SimpleTimer::startOrStop(bool shouldStart)
{
	if(updater == nullptr)
		return;

	WeakReference<SimpleTimer> safeThis(this);

	auto f = [safeThis, shouldStart]()
	{
		if (safeThis.get() != nullptr)
		{
			safeThis->isRunning = shouldStart;

			if(shouldStart)
				safeThis.get()->updater->simpleTimers.addIfNotAlreadyThere(safeThis);
			else
				safeThis.get()->updater->simpleTimers.removeAllInstancesOf(safeThis);
		}
	};

	if (MessageManager::getInstance()->currentThreadHasLockedMessageManager())
		f();
	else
		MessageManager::callAsync(f);
}

PooledUIUpdater::Broadcaster::Broadcaster()
{}

PooledUIUpdater::Broadcaster::~Broadcaster()
{}

void PooledUIUpdater::Broadcaster::setHandler(PooledUIUpdater* handler_)
{
	handler = handler_;
}

void PooledUIUpdater::Broadcaster::addPooledChangeListener(Listener* l)
{ pooledListeners.addIfNotAlreadyThere(l); }

void PooledUIUpdater::Broadcaster::removePooledChangeListener(Listener* l)
{ pooledListeners.removeAllInstancesOf(l); }

bool PooledUIUpdater::Broadcaster::isHandlerInitialised() const
{ return handler != nullptr; }

void PooledUIUpdater::timerCallback()
{
	PerfettoHelpers::setCurrentThreadName("UI Timer Thread");

	TRACE_DISPATCH("UI Timer callback");

	{
		ScopedLock sl(simpleTimers.getLock());

		int x = 0;

		for (int i = 0; i < simpleTimers.size(); i++)
		{
			auto st = simpleTimers[i];

			x++;
			if (st.get() != nullptr)
				st->timerCallback();
			else
				simpleTimers.remove(i--);
		}
	}

	WeakReference<Broadcaster> b;

	while (pendingHandlers.pop(b))
	{
		if (b.get() != nullptr)
		{
			b->pending = false;

			for (auto l : b->pooledListeners)
			{
				if (l != nullptr)
					l->handlePooledMessage(b);
			}
		}
	}
}

ComplexDataUIUpdaterBase::EventListener::~EventListener()
{}

ComplexDataUIUpdaterBase::~ComplexDataUIUpdaterBase()
{
	ScopedLock sl(updateLock);
	listeners.clear();
}

void ComplexDataUIUpdaterBase::addEventListener(EventListener* l)
{
	ScopedLock sl(updateLock);

	jassert(isPositiveAndBelow(listeners.size(), NumListenerSlots));

	for(int i = 0; i < listeners.size(); i++)
	{
		if(listeners[i] == nullptr)
			listeners.removeElement(i--);
	}

	listeners.insert(l);
	
	updateUpdater();
}

void ComplexDataUIUpdaterBase::removeEventListener(EventListener* l)
{
	ScopedLock sl(updateLock);

	listeners.remove(l);
	updateUpdater();
}

void ComplexDataUIUpdaterBase::setUpdater(PooledUIUpdater* updater)
{
	if (globalUpdater == nullptr)
	{
		ScopedLock sl(updateLock);
		globalUpdater = updater;
		updateUpdater();
	}
}

void ComplexDataUIUpdaterBase::sendDisplayChangeMessage(float newIndexValue, NotificationType notify,
	bool forceUpdate) const
{
	sendMessageToListeners(EventType::DisplayIndex, var(newIndexValue), notify, forceUpdate);
}

void ComplexDataUIUpdaterBase::sendContentChangeMessage(NotificationType notify, int indexThatChanged)
{
	sendMessageToListeners(EventType::ContentChange, var(indexThatChanged), notify, true);
}

void ComplexDataUIUpdaterBase::sendContentRedirectMessage()
{
	sendMessageToListeners(EventType::ContentRedirected, {}, sendNotificationSync, true);
}

PooledUIUpdater* ComplexDataUIUpdaterBase::getGlobalUIUpdater()
{
	return globalUpdater;
}

float ComplexDataUIUpdaterBase::getLastDisplayValue() const
{
	return lastDisplayValue;
}

void ComplexDataUIUpdaterBase::updateUpdater()
{
	if (globalUpdater != nullptr && currentUpdater == nullptr && listeners.size() > 0)
		currentUpdater = new Updater(*this);

	if (listeners.size() == 0 || globalUpdater == nullptr)
		currentUpdater = nullptr;
}

void ComplexDataUIUpdaterBase::Updater::timerCallback()
{
	if (parent.lastChange != EventType::Idle)
	{
		parent.sendMessageToListeners(parent.lastChange, parent.lastValue, sendNotificationSync, true);
	}
}

ComplexDataUIUpdaterBase::Updater::Updater(ComplexDataUIUpdaterBase& parent_):
	SimpleTimer(parent_.globalUpdater),
	parent(parent_)
{
	start();
}

void ComplexDataUIUpdaterBase::sendMessageToListeners(EventType t, var v, NotificationType n, bool forceUpdate) const
{
	if (n == dontSendNotification)
		return;

	if (t == EventType::DisplayIndex)
		lastDisplayValue = (float)v;

	if (n == sendNotificationSync)
	{
		bool isMoreImportantChange = t >= lastChange;
		bool valueHasChanged = lastValue != v;

		if (forceUpdate || (isMoreImportantChange && valueHasChanged))
		{
			ScopedLock sl(updateLock);

			lastChange = jmax(t, lastChange);

			for (auto l : listeners)
			{
				if (l.get() != nullptr)
				{
					l->onComplexDataEvent(t, v);

					if (lastChange != EventType::DisplayIndex)
						l->onComplexDataEvent(ComplexDataUIUpdaterBase::EventType::DisplayIndex, lastDisplayValue);
				}
			}
		}

		lastChange = EventType::Idle;
	}
	else
	{
		if (t >= lastChange)
		{
			lastChange = jmax(lastChange, t);
			lastValue = v;
		}
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


double TempoSyncer::getTempoInSamples(double hostTempoBpm, double sampleRate, float tempoFactor)
{
	if (hostTempoBpm == 0.0) hostTempoBpm = 120.0;

	const auto seconds = (60.0 / hostTempoBpm) * (double)tempoFactor;
	return seconds * sampleRate;
}

double TempoSyncer::getTempoInSamples(double hostTempoBpm, double sampleRate, Tempo t)
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
		if(strcmp(t.getCharPointer().getAddress(), tempoNames[i]) == 0)
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

#if HISE_USE_EXTENDED_TEMPO_VALUES
	setTempo(EightBar, "8/1", 4.0f * 8.0f);
	setTempo(SixBar, "6/1", 4.0f * 6.0f);
	setTempo(FourBar, "4/1", 4.0f * 4.0f);
	setTempo(ThreeBar, "3/1", 4.0f * 3.0f);
	setTempo(TwoBars, "2/1", 4.0f * 2.0f);
#endif
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

void MasterClock::setNextGridIsFirst()
{
	waitForFirstGrid = true;
}

void MasterClock::setSyncMode(SyncModes newSyncMode)
{
	currentSyncMode = newSyncMode;
}

bool MasterClock::changeState(int timestamp, bool internalClock, bool startPlayback)
{
	if (currentSyncMode == SyncModes::Inactive)
		return false;

	if (internalClock)
		internalClockIsRunning = startPlayback;

	// Already stopped / not running, just return
	if (!startPlayback && currentState == State::Idle)
		return false;

	// Nothing to do
	if (internalClock && startPlayback && currentState == State::InternalClockPlay)
		return false;

	// Nothing to do
	if (!internalClock && startPlayback && currentState == State::ExternalClockPlay)
		return false;

	// Ignore any internal clock events when the external is running and should be preferred
	if(!shouldPreferInternal() && (currentState == State::ExternalClockPlay && internalClock))
		return false;

	// Ignore any external clock events when the external is running and should be preferred
	if (shouldPreferInternal() && (currentState == State::InternalClockPlay && !internalClock))
		return false;
		
	// Ignore the stop command from the external clock
	if (currentSyncMode == SyncModes::SyncInternal && !startPlayback && !internalClock)
		return false;

	nextTimestamp = timestamp;

	if (startPlayback)
		nextState = internalClock ? State::InternalClockPlay : State::ExternalClockPlay;
	else
		nextState = State::Idle;

	// Restart the internal clock when the external is stopped
	if (!internalClock && !startPlayback && internalClockIsRunning)
	{
		if(stopInternalOnExternalStop)
			nextState = State::Idle;
		else
			nextState = State::InternalClockPlay;
	}

	return true;
}

MasterClock::GridInfo MasterClock::processAndCheckGrid(int numSamples,
	const AudioPlayHead::CurrentPositionInfo& externalInfo)
{
	// check whether we want to process the external bpm
	auto shouldUseExternalBpm = !linkBpmToSync || !shouldPreferInternal();

	if (bpm != externalInfo.bpm && shouldUseExternalBpm)
		setBpm(externalInfo.bpm);

	GridInfo gi;

	if (currentSyncMode == SyncModes::Inactive)
		return gi;

	if (currentSyncMode == SyncModes::SyncInternal && externalInfo.isPlaying)
	{
        const auto quarterInSamples = (double)TempoSyncer::getTempoInSamples(externalInfo.bpm, sampleRate, 1.0f);
        
        // do not use timeInSamples, it's not accurate if changin
        // the host tempo
        uptime = externalInfo.ppqPosition * quarterInSamples;
        
		samplesToNextGrid = gridDelta - (uptime % gridDelta);
	}

	if (currentState != nextState)
	{
		currentState = nextState;
		uptime = numSamples - nextTimestamp;
		currentGridIndex = 0;

		if (currentState != State::Idle && gridEnabled)
		{

			gi.change = true;
			gi.timestamp = nextTimestamp;
			gi.gridIndex = currentGridIndex;
			gi.firstGridInPlayback = true;

			samplesToNextGrid = gridDelta - nextTimestamp;
		}

		nextTimestamp = 0;
	}
	else
	{
		if (currentState == State::Idle)
			uptime = 0;
		else
		{
			jassert(nextTimestamp == 0);
			uptime += numSamples;

			samplesToNextGrid -= numSamples;

			if (samplesToNextGrid < 0 && gridEnabled)
			{
				currentGridIndex++;

				gi.change = true;
				gi.firstGridInPlayback = waitForFirstGrid;
				waitForFirstGrid = false;
				gi.gridIndex = currentGridIndex;
				gi.timestamp = numSamples + samplesToNextGrid;

				samplesToNextGrid += gridDelta;
			}
		}
	}

	return gi;
}

bool MasterClock::isPlaying() const
{
	return currentState == State::ExternalClockPlay || currentState == State::InternalClockPlay;
}

MasterClock::SyncModes MasterClock::getSyncMode() const
{
	return currentSyncMode;
}

MasterClock::GridInfo MasterClock::updateFromExternalPlayHead(const AudioPlayHead::CurrentPositionInfo& info,
	int numSamples)
{
	GridInfo gi;

	if (currentSyncMode == SyncModes::Inactive)
		return gi;

	auto isPlayingExternally = currentState == State::ExternalClockPlay;
	auto shouldPlayExternally = (currentSyncMode == SyncModes::ExternalOnly || currentSyncMode == SyncModes::PreferExternal) &&
		info.isPlaying;
		
	if (isPlayingExternally != shouldPlayExternally)
	{
		changeState(0, false, shouldPlayExternally);

		if (currentSyncMode == SyncModes::PreferExternal &&
			currentState == State::InternalClockPlay &&
			nextState == State::ExternalClockPlay)
		{
			gi.change = true;
			gi.gridIndex = 0;
			gi.firstGridInPlayback = true;
		}

		currentState = nextState;

		if (currentState == State::ExternalClockPlay && gridEnabled)
		{
			auto multiplier = (double)TempoSyncer::getTempoFactor(clockGrid);

			auto gridPos = std::fmod(info.ppqPosition, multiplier);

			if (std::abs(gridPos) <= 0.2)
			{
				gi.change = true;
				gi.gridIndex = roundToInt(info.ppqPosition / multiplier);
				gi.firstGridInPlayback = true;
				gi.timestamp = 0;
				waitForFirstGrid = false;
			}
			else
			{
				waitForFirstGrid = true;
			}
		}
	}
		
	Range<int64> estimatedRange(uptime, uptime + currentBlockSize * 3);
        
    const auto quarterInSamples = (double)TempoSyncer::getTempoInSamples(info.bpm, sampleRate, 1.0f);
    
    // do not use timeInSamples, it's not accurate if changin
    // the host tempo
    uptime = info.ppqPosition * quarterInSamples;

	if(!estimatedRange.contains(uptime))
	{
		if(info.isPlaying)
		{
			gi.resync = true;
		}
	}
        
	if (info.isPlaying && gridEnabled)
	{
		
		auto numSamplesInPPQ = (double)numSamples / quarterInSamples;
		auto ppqBefore = info.ppqPosition;
		auto ppqAfter = ppqBefore + numSamplesInPPQ;
		auto multiplier = (double)TempoSyncer::getTempoFactor(clockGrid);

		auto i1 = (int)(ppqBefore / multiplier);
		auto i2 = (int)(ppqAfter / multiplier);

		if (i1 != i2)
		{
			auto gridPosPPQ = (double)i2 * multiplier;
			auto deltaPPQ = gridPosPPQ - ppqBefore;

			gi.change = true;
			gi.gridIndex = i2;
			gi.timestamp = TempoSyncer::getTempoInSamples(bpm, sampleRate, (float)deltaPPQ);

			if (waitForFirstGrid)
			{
				gi.firstGridInPlayback = true;
				waitForFirstGrid = false;
			}
		}
	}

	return gi;
}

AudioPlayHead::CurrentPositionInfo MasterClock::createInternalPlayHead()
{
	AudioPlayHead::CurrentPositionInfo info;
		
	int ms = 1000.0 * uptime / sampleRate;
	auto quarterMs = TempoSyncer::getTempoInMilliSeconds(bpm, TempoSyncer::Quarter);
	float quarterPos = ms / quarterMs;

	info.bpm = bpm;
	info.isPlaying = currentState != State::Idle;

	info.timeInSamples = uptime;
	info.ppqPosition = quarterPos;

	return info;
}

void MasterClock::checkInternalClockForExternalStop(AudioPlayHead::CurrentPositionInfo& infoToUse,
	const AudioPlayHead::CurrentPositionInfo& externalInfo)
{
	if (externalClockWasPlayingLastTime && !externalInfo.isPlaying)
	{
		nextState = State::Idle;
		infoToUse.isPlaying = false;
	}
		
	externalClockWasPlayingLastTime = externalInfo.isPlaying;
}

void MasterClock::prepareToPlay(double newSampleRate, int blockSize)
{
	sampleRate = newSampleRate;
	currentBlockSize = blockSize;
	updateGridDelta();
}

void MasterClock::setBpm(double newBPM)
{
	bpm = newBPM;
	updateGridDelta();
}

void MasterClock::setLinkBpmToSyncMode(bool should)
{
	linkBpmToSync = should;
}

TempoSyncer::Tempo MasterClock::getCurrentClockGrid() const
{ return clockGrid; }

bool MasterClock::allowExternalSync() const
{
	return currentSyncMode != SyncModes::InternalOnly;
}

void MasterClock::setStopInternalClockOnExternalStop(bool shouldStop)
{
	stopInternalOnExternalStop = shouldStop;
}

bool MasterClock::shouldCreateInternalInfo(const AudioPlayHead::CurrentPositionInfo& externalInfo) const
{
	if (currentSyncMode == SyncModes::Inactive)
		return false;

	if (currentSyncMode == SyncModes::ExternalOnly)
		return false;

	if (currentSyncMode == SyncModes::InternalOnly)
		return true;

	if (currentSyncMode == SyncModes::PreferExternal && (externalInfo.isPlaying || currentState == State::ExternalClockPlay))
		return false;

	if (currentSyncMode == SyncModes::SyncInternal)
		return true;

	return true;
}

void MasterClock::setClockGrid(bool enableGrid, TempoSyncer::Tempo t)
{
	gridEnabled = enableGrid;
	clockGrid = t;
	updateGridDelta();
}

bool MasterClock::isGridEnabled() const
{ return gridEnabled; }

double MasterClock::getPPQPos(int timestampFromNow) const
{
	if (currentSyncMode == SyncModes::Inactive)
		return 0.0;

	auto quarterSamples = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, 1.0f);
	auto uptimeToUse = uptime - timestampFromNow;
	return uptimeToUse / quarterSamples;
}

void MasterClock::reset()
{
	gridEnabled = false;
	clockGrid = TempoSyncer::numTempos;
	currentSyncMode = SyncModes::Inactive;
        
	uptime = 0;
	samplesToNextGrid = 0;
        
	currentGridIndex = 0;

	internalClockIsRunning = false;

	externalClockWasPlayingLastTime = false;

	// they don't need to be resetted...
	//sampleRate = 44100.0;
	//bpm = 120.0;

	nextTimestamp = 0;
	currentState = State::Idle;
	nextState = State::Idle;

	waitForFirstGrid = false;
        
	updateGridDelta();
}

void MasterClock::updateGridDelta()
{
	if (gridEnabled)
	{
		gridDelta = TempoSyncer::getTempoInSamples(bpm, sampleRate, clockGrid);
	}
}

bool MasterClock::shouldPreferInternal() const
{
	return currentSyncMode == SyncModes::PreferInternal || currentSyncMode == SyncModes::InternalOnly || currentSyncMode == SyncModes::SyncInternal;
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

	if(area.getWidth() > 10.0)
		area = area.reduced(4.0f);
	else
		area = area.reduced(2.0f);

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

Array<FFTHelpers::WindowType> FFTHelpers::getAvailableWindowTypes()
{
	return { Rectangle, Triangle, Hamming, Hann, BlackmanHarris, Kaiser, FlatTop };
}

String FFTHelpers::getWindowType(WindowType w)
{
	switch (w)
	{
	case Rectangle: return "Rectangle";
	case Hamming: return "Hamming";
	case Hann: return "Hann";
	case BlackmanHarris: return "Blackman Harris";
	case Triangle: return "Triangle";
	case FlatTop: return "FlatTop";
	case Kaiser: return "Kaiser";
	default: return {};
	}
}

void FFTHelpers::toComplexArray(const AudioSampleBuffer& phaseBuffer, const AudioSampleBuffer& magBuffer,
	AudioSampleBuffer& out)
{
	auto phase = phaseBuffer.getReadPointer(0);
	auto mag = magBuffer.getReadPointer(0);

	auto output = out.getWritePointer(0);
		
	jassert(phaseBuffer.getNumSamples() == magBuffer.getNumSamples());
	jassert(phaseBuffer.getNumSamples() * 2 == out.getNumSamples());

	int size = phaseBuffer.getNumSamples();

	for (int i = 0; i < size; i++)
	{
		auto re = mag[i] * std::cos(phase[i]);
		auto im = mag[i] * std::sin(phase[i]);

		output[i * 2] = re;
		output[i * 2 + 1] = im;
	}
}

void FFTHelpers::toPhaseSpectrum(const AudioSampleBuffer& inp, AudioSampleBuffer& out)
{
	auto input = inp.getReadPointer(0);
	auto output = out.getWritePointer(0);

	jassert(inp.getNumSamples() == out.getNumSamples() * 2);

	auto numOriginalSamples = out.getNumSamples();

	for (int i = 0; i < numOriginalSamples; i++)
	{
		auto re = input[i * 2];
		auto im = input[i * 2 + 1];
		output[i] = std::atan2(im, re);
	}
}

void FFTHelpers::toFreqSpectrum(const AudioSampleBuffer& inp, AudioSampleBuffer& out)
{
	auto input = inp.getReadPointer(0);
	auto output = out.getWritePointer(0);
        
	jassert(inp.getNumSamples() == out.getNumSamples() * 2);

	auto numOriginalSamples = out.getNumSamples();

	for (int i = 0; i < numOriginalSamples; i++)
	{
		auto re = input[i * 2];
		auto im = input[i * 2 + 1];
		output[i] = sqrt(re * re + im * im);
	}
}

void FFTHelpers::scaleFrequencyOutput(AudioSampleBuffer& b, bool convertToDb, bool invert)
{
	auto data = b.getWritePointer(0);
	auto numOriginalSamples = b.getNumSamples();

	if (numOriginalSamples == 0)
		return;

	auto factor = 2.f / (float)numOriginalSamples;

	if (invert)
	{
		factor = 1.0f / factor;
		factor *= 0.5f;

		if (convertToDb)
		{
			for (int i = 0; i < numOriginalSamples; i++)
				data[i] = Decibels::decibelsToGain(data[i]);
		}
	}

	FloatVectorOperations::multiply(data, factor, numOriginalSamples);

	if (!invert && convertToDb)
	{
		for (int i = 0; i < numOriginalSamples; i++)
			data[i] = Decibels::gainToDecibels(data[i]);
	}
}

StringArray Spectrum2D::LookupTable::getColourSchemes()
{ return { "BlackWhite", "Rainbow", "VioletOrange", "HiseColours" }; }

void Spectrum2D::Parameters::setFromBuffer(const AudioSampleBuffer& originalSource)
{
	auto numSamplesToCheck = (double)originalSource.getNumSamples();
	numSamplesToCheck = std::pow(numSamplesToCheck, JUCE_LIVE_CONSTANT_OFF(0.54));

	auto bestOrder = 11;

	set("FFTSize", bestOrder, dontSendNotification);

	notifier.sendMessage(sendNotificationSync, "All", -1);
}

Spectrum2D::Holder::~Holder()
{}

float Spectrum2D::Holder::getXPosition(float input) const
{
	auto db = (float)getParameters()->minDb;
	auto l = Decibels::gainToDecibels(input, -1.0f * db);
	l = (l + db) / db;
	return l * l;
}

float Spectrum2D::Holder::getYPosition(float input) const
{
	return 1.0f - std::exp(std::log(input) * 0.2f);
}

Spectrum2D::Spectrum2D(Holder* h, const AudioSampleBuffer& s):
	holder(h),
	originalSource(s),
	parameters(new Parameters())
{
	//parameters->setFromBuffer(s);
}

void Spectrum2D::draw(Graphics& g, const Image& img, Rectangle<int> area, Graphics::ResamplingQuality quality)
{
	g.saveState();
	g.setImageResamplingQuality(quality);

	auto t = AffineTransform::translation(-img.getWidth() / 2, -img.getHeight() / 2);

	t = t.followedBy(AffineTransform::rotation(float_Pi * 1.5f));
	t = t.followedBy(AffineTransform::scale((float)area.getWidth() / (float)img.getHeight(), (float)area.getHeight() / (float)img.getWidth()));
	t = t.followedBy(AffineTransform::translation(area.getWidth() / 2 + area.getX(), area.getHeight() / 2 + area.getY()));

	g.drawImageTransformed(img, t);
		
	//g.drawImageWithin(img, area.getX(), area.getY(), area.getWidth(), area.getHeight(), RectanglePlacement::stretchToFit);
	g.restoreState();
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

juce::PixelARGB Spectrum2D::LookupTable::getColouredPixel(float normalisedInput, bool useAlphaValue)
{
	auto lutValue = data[jlimit(0, LookupTableSize - 1, roundToInt(normalisedInput * (float)LookupTableSize))];
	float a = JUCE_LIVE_CONSTANT_OFF(0.3f);
	auto v = jlimit(0.0f, 1.0f, a + (1.0f - a) * normalisedInput);
	auto r = (float)lutValue.getRed() * v;
	auto g = (float)lutValue.getGreen() * v;
	auto b = (float)lutValue.getBlue() * v;
	
	return PixelARGB(useAlphaValue ? jmax(r, g, b) : 255, (uint8)r, (uint8)g, (uint8)b);
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
			break;
		}
		case ColourScheme::preColours:
		{
			grad.addColour(0.33f, Colour(0xff666666));
			grad.addColour(0.66f, Colour(0xff9d629a));
			break;
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
	TRACE_EVENT("scripting", "create spectrum image");

    auto newImage = Image(Image::ARGB, lastBuffer.getNumSamples(), lastBuffer.getNumChannels(), true);

	Image::BitmapData bd(newImage, Image::BitmapData::writeOnly);
	
	for(int y = 0; y < lastBuffer.getNumChannels(); y++)
	{
		auto src = lastBuffer.getReadPointer(y);
		
		for(int x = 0; x < lastBuffer.getNumSamples(); x++)
		{
			auto lutValue = parameters->lut->getColouredPixel(src[x], useAlphaChannel);
			auto pp = (PixelARGB*)(bd.getPixelPointer(x, y));
			pp->set(lutValue);
		}
	}

	testImage(newImage, false, "after creation");


    return newImage;
}



AudioSampleBuffer Spectrum2D::createSpectrumBuffer(bool useFallback)
{
	TRACE_EVENT("scripting", "create spectrum buffer");
    auto fft = juce::dsp::FFT(parameters->order, useFallback);

    auto numSamplesToFill = jmax(0, originalSource.getNumSamples() / parameters->Spectrum2DSize * parameters->oversamplingFactor - 1);

    if (numSamplesToFill == 0)
        return {};

    AudioSampleBuffer b(numSamplesToFill, parameters->Spectrum2DSize / 2);
    b.clear();

	AudioSampleBuffer out(1, parameters->Spectrum2DSize);
	AudioSampleBuffer sb(1, parameters->Spectrum2DSize * 2);

	AudioSampleBuffer window(1, parameters->Spectrum2DSize * 2);

	FloatVectorOperations::fill(window.getWritePointer(0), 1.0f, parameters->Spectrum2DSize);

	FFTHelpers::applyWindow(parameters->currentWindowType, window);

	

	auto size = b.getNumSamples();

	HeapBlock<float> positions;
	positions.calloc(size);

	for(int i = 0; i < size; i++)
	{
		auto normIndex = (float)i / (float)size;
		auto skewedProportionY = holder->getYPosition(normIndex);
		positions[i] = skewedProportionY;
	}


	auto db = (float)parameters->minDb;

	auto minGain = Decibels::decibelsToGain(-1.0f * db, -140.0f);

	NormalisableRange<float> gainRange(minGain, 1.0f);

    for (int i = 0; i < numSamplesToFill; i++)
    {
        auto offset = (i * parameters->Spectrum2DSize) / parameters->oversamplingFactor;

		auto numToCopy = jmin(parameters->Spectrum2DSize, originalSource.getNumSamples() - offset);

        {
			TRACE_EVENT("scripting", "pre FFT");
	        sb.clear();

	        FloatVectorOperations::copy(sb.getWritePointer(0), originalSource.getReadPointer(0, offset), numToCopy);
			FloatVectorOperations::multiply(sb.getWritePointer(0), window.getReadPointer(0), numToCopy);
        }

        {
			TRACE_EVENT("scripting", "perform FFT");
	        fft.performRealOnlyForwardTransform(sb.getWritePointer(0), false);
        }

        {
			TRACE_EVENT("scripting", "post FFT");
	        FFTHelpers::toFreqSpectrum(sb, out);
			FFTHelpers::scaleFrequencyOutput(out, false);
        }



		FloatVectorOperations::copy(b.getWritePointer(i), out.getReadPointer(0), b.getNumSamples());

		auto src = out.getReadPointer(0);
		auto dst = b.getWritePointer(i);

		{
	        TRACE_EVENT("scripting", "scale freq output");

			auto bitmask = (b.getNumSamples()-1);

			for(int i = 0; i < size; i++)
			{
				auto idx = positions[i] * (float)(size-1);

				auto i1 = (int)(idx) & bitmask;
				auto i0 = jmax(i1-1, 0);
				auto i2 = (i1+1) & bitmask;
				auto i3 = (i1+2) & bitmask;
				auto alpha = idx - (float)i1;

				auto dstIndex = size - (i+1);

				float value;

				if(parameters->quality == Graphics::highResamplingQuality)
					value = Interpolator::interpolateCubic(src[i0], src[i1], src[i2], src[i3], alpha);
				else
					value = Interpolator::interpolateLinear<float>(src[i1], src[i2], alpha);

				value = jmax<float>(value, minGain) - minGain;
				
				value = std::pow(value, parameters->getGamma());
				dst[dstIndex] = value;
			}
        }
    }

	auto thisGain = parameters->getGainFactor();

	if(thisGain == 0.0f)
	{
		thisGain = b.getMagnitude(0, b.getNumSamples());

		if(thisGain != 0.0f)
			thisGain = 1.0f / thisGain;
	}

	b.applyGain(thisGain);

    return b;
}

void Spectrum2D::Parameters::set(const Identifier& id, var value, NotificationType n)
{
	jassert(getAllIds().contains(id));

	if (id == Identifier("FFTSize"))
	{
		order = jlimit(7, 13, (int)value);
		Spectrum2DSize = roundToInt(std::pow(2.0, (double)order));
	}
    if(id == Identifier("DynamicRange"))
        minDb = value;
	if (id == Identifier("Oversampling"))
		oversamplingFactor = value;
	if (id == Identifier("Gamma"))
		gammaPercent = jlimit(0, 150, (int)value);
	if (id == Identifier("ColourScheme"))
		lut->setColourScheme((LookupTable::ColourScheme)(int)value);
	if (id == Identifier("WindowType"))
		currentWindowType = (FFTHelpers::WindowType)(int)value;
	if(id == Identifier("ResamplingQuality"))
	{
		StringArray q("Low", "Mid", "High");
		if(q.contains(value.toString()))
			quality = (Graphics::ResamplingQuality)(int)q.indexOf(value.toString());
	}
	if (id == Identifier("GainFactor"))
	{
		gainFactorDb = (int)value;
	}
	if (n != dontSendNotification)
		notifier.sendMessage(n, id, value);
}

var Spectrum2D::Parameters::get(const Identifier& id) const
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
	if (id == Identifier("GainFactor"))
		return gainFactorDb;
	if (id == Identifier("Gamma"))
		return gammaPercent;
	if (id == Identifier("ResamplingQuality"))
	{
		StringArray q("Low", "Mid", "High");
		return q[(int)quality];
	}
	if (id == Identifier("WindowType"))
		return (int)currentWindowType;

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
		Identifier("GainFactor"),
		Identifier("ResamplingQuality"),
		Identifier("Gamma"),
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
	addEditor("Gamma");
	addEditor("ResamplingQuality");
	addEditor("GainFactor");

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
	if (id == Identifier("Gamma"))
	{
		cb->addItem("12%", 13);
		cb->addItem("25%", 26);
		cb->addItem("33%", 34);
        cb->addItem("50%", 51);
		cb->addItem("66%", 67);
        cb->addItem("75%", 76);
        cb->addItem("100%", 101);
        cb->addItem("125%", 126);
        cb->addItem("150%", 151);
		
	}
	if(id == Identifier("ResamplingQuality"))
	{
		cb->addItem("Low", 1);
		cb->addItem("Mid", 2);
		cb->addItem("High", 3);
	}
	if (id == Identifier("GainFactor"))
	{
		cb->addItem("Auto", 1001);
		cb->addItem("0dB",  1);
		cb->addItem("+6dB", 7);
		cb->addItem("+12dB", 13);
		cb->addItem("+18dB", 14);
	}

	cb->setSelectedId((int)param->get(id)+1, dontSendNotification);

	addAndMakeVisible(cb);
	editors.add(cb);
	cb->addListener(this);

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
	auto range = (int)param->get("DynamicRange");

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
		auto p = param->lut->getColouredPixel(ninput, true);

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

SemanticVersionChecker::SemanticVersionChecker(const std::array<int, 3>& oldVersion_, const std::array<int, 3>& newVersion_)
{
    newVersion.majorVersion = newVersion_[0];
	newVersion.minorVersion = newVersion_[1];
	newVersion.patchVersion = newVersion_[2];
	newVersion.validVersion = true;

	oldVersion.majorVersion = oldVersion_[0];
	oldVersion.minorVersion = oldVersion_[1];
	oldVersion.patchVersion = oldVersion_[2];
	oldVersion.validVersion = true;
}

SemanticVersionChecker::SemanticVersionChecker(const String& oldVersion_, const String& newVersion_)
{
	parseVersion(oldVersion, oldVersion_);
	parseVersion(newVersion, newVersion_);
}

bool SemanticVersionChecker::isMajorVersionUpdate() const
{ return newVersion.majorVersion > oldVersion.majorVersion; }

bool SemanticVersionChecker::isMinorVersionUpdate() const
{ return newVersion.minorVersion > oldVersion.minorVersion; }

bool SemanticVersionChecker::isPatchVersionUpdate() const
{ return newVersion.patchVersion > oldVersion.patchVersion; }

bool SemanticVersionChecker::oldVersionNumberIsValid() const
{ return oldVersion.validVersion; }

bool SemanticVersionChecker::newVersionNumberIsValid() const
{ return newVersion.validVersion; }

void SemanticVersionChecker::parseVersion(VersionInfo& info, const String& v)
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
}

bool SemanticVersionChecker::isUpdate() const
{
    if (newVersion.majorVersion > oldVersion.majorVersion)
        return true;
    else if (newVersion.majorVersion < oldVersion.majorVersion)
        return false;
    else
    {
        if (newVersion.minorVersion > oldVersion.minorVersion)
            return true;
        else if (newVersion.minorVersion < oldVersion.minorVersion)
            return false;
        else
        {
            if (newVersion.patchVersion > oldVersion.patchVersion)
                return true;
            else
                return false;
        }
    }
}

double MasterClock::getBpmToUse(double hostBpm, double internalBpm) const
{
	if (hostBpm <= 0.0 && internalBpm <= 0.0)
		return 120.0;
	if (hostBpm <= 0.0)
		return internalBpm;
	if (internalBpm <= 0.0)
		return hostBpm;
	
	if (linkBpmToSync)
		return shouldPreferInternal() ? internalBpm : hostBpm;

	return internalBpm;
}

}
