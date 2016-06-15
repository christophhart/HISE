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

#ifndef GLOBALSCRIPTCOMPILEBROADCASTER_H_INCLUDED
#define GLOBALSCRIPTCOMPILEBROADCASTER_H_INCLUDED

class ScriptProcessor;

/** A GlobalScriptCompileListener gets informed whenever a script was compiled.
*
*	add it with getMainController()->addGlolablScriptCompileListener(this),
*	overwrite the callback and do whatever you need to do.
*
*/
class GlobalScriptCompileListener
{
public:

	virtual ~GlobalScriptCompileListener() { masterReference.clear(); };

	/** Whenever a script was compiled, a message is sent out to every listener. */
	virtual void scriptWasCompiled(ScriptProcessor *processor) = 0;

private:

	friend class WeakReference < GlobalScriptCompileListener > ;
	WeakReference<GlobalScriptCompileListener>::Master masterReference;
};


/** This class sends a message to all registered listeners. */
class GlobalScriptCompileBroadcaster
{
public:

	GlobalScriptCompileBroadcaster() :
		timeOut(2.0),
		useBackgroundCompiling(false),
		enableGlobalRecompile(true)
	{}

	virtual ~GlobalScriptCompileBroadcaster()
	{};

	/** This sends a synchronous message to all registered listeners.
	*
	*	Listeners which are manually inserted at the beginning (eg. ScriptingContentComponents) are notified first.
	*/
	void sendScriptCompileMessage(ScriptProcessor *processorThatWasCompiled);

	/** Adds a ScriptListener. You can influence the order of the callback by inserting Listeners at the beginning of the list. */
	void addScriptListener(GlobalScriptCompileListener *listener, bool insertAtBeginning = false)
	{
		if (insertAtBeginning)
		{
			listenerListStart.addIfNotAlreadyThere(listener);
		}
		else
		{
			listenerListEnd.addIfNotAlreadyThere(listener);
		}

	};

	void removeScriptListener(GlobalScriptCompileListener *listener)
	{
		listenerListStart.removeAllInstancesOf(listener);
		listenerListEnd.removeAllInstancesOf(listener);
	};

	void setShouldUseBackgroundThreadForCompiling(bool shouldBeEnabled) noexcept{ useBackgroundCompiling = shouldBeEnabled; }
	bool isUsingBackgroundThreadForCompiling() const noexcept{ return useBackgroundCompiling; }

	void setCompileTimeOut(double newTimeOut) noexcept{ timeOut = newTimeOut; }
	double getCompileTimeOut() const noexcept{ return timeOut; }

	void setEnableCompileAllScriptsOnPresetLoad(bool shouldBeEnabled) noexcept{ enableGlobalRecompile = shouldBeEnabled; };
	bool isCompilingAllScriptsOnPresetLoad() const noexcept{ return enableGlobalRecompile; };

	void fillExternalFileList(Array<File> &files, StringArray &processors);

	void setExternalScriptData(ValueTree &collectedExternalScripts);

	String getExternalScriptFromCollection(const String &fileName);

private:
	
	bool useBackgroundCompiling;
	bool enableGlobalRecompile;

	double timeOut;

	ValueTree externalScripts;

	Array<WeakReference<GlobalScriptCompileListener>> listenerListStart;
	Array<WeakReference<GlobalScriptCompileListener>> listenerListEnd;
};


#endif  // GLOBALSCRIPTCOMPILEBROADCASTER_H_INCLUDED
