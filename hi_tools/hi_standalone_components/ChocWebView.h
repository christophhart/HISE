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

namespace choc { namespace ui { class WebView; } }

namespace hise {
using namespace juce;

struct WebViewData : public ReferenceCountedObject
{
	/** This specifies the mode which will be used to resolve external resources. */
	enum class ServerType
	{
		Uninitialised,
		FileBased,	    ///< Uses a root directory to load the content from files using their relative path
		Embedded,       ///< Uses a cached data object to load the content from embedded data
		numServerTypes
	};

	using Ptr = ReferenceCountedObjectPtr<WebViewData>;
	using CallbackType = std::function<var(const var&)>;

	WebViewData(File projectRoot);

	~WebViewData();

	/** Setup an error logger that will be notified whenever a resource isn't found. */
	void setErrorLogger(const std::function<void(const String& m)>& errorFunction)
	{
		errorLogger = errorFunction;
	}

	/** Adds a custom JS script to the web view. */
	void addScript(const String& script)
	{
		scripts.add(script);
	}

	/** Binds a C++ function to the webview that will be executed when the JS function is being called.

		Be aware that this uses the JS Promise object for asynchronous execution, so you need to use
		the following syntax in your JS code:

		```
		callbackName(someArgs).then ((result) => { doSomethingWith(result); });
		```

		The CallbackType is a callable C++ object that can be called with this signature:

		```
		var func(const var& args)
		```

		(It automatically converts between the choc::Value type and the juce::var class).
	*/
	void addCallback(const String& callbackName, const CallbackType& function);

	/** Adds a dynamic resources with the given path and mime type. */
	void addResource(const String& path, const String& mimeType, const String& content);

	/** Adds a juce::Image as PNG image with the given path. */
	void addPNGImage(const String& path, const Image& img);

	/** Exports the resources as valuetree that can be imported later. Be aware that this takes the current state
		into account and the WebView has to be initialised at least once so that the resources are cached (also, you
		must not disable caching before calling this method, obviously...). */
	ValueTree exportAsValueTree() const;

	/** Restores the resources from a previously exported ValueTree (and sets the mode to ServerType::Embedded. */
	void restoreFromValueTree(const ValueTree& v);

	/** Sets the root directory from where all files should be cached (and sets the mode to ServerType::FileBased). */
	void setRootDirectory(const File& newRootDirectory);

	/** If set to true, a new webview will be initialised will all function calls that have been called so far. This allow
		you to initialise a state correctly when the webview is created after the data has been initialised.
	*/
	void setUsePersistentCalls(bool usePersistentCalls);

	void setUseScaleFactorForZoom(bool shouldApplyScaleFactorAsZoom)
	{
		applyScaleFactorAsZoom = shouldApplyScaleFactorAsZoom;
	}

	/** Clears all caches and persistent calls and (optionally) the file resource information. */
	void reset(bool resetFileStructure=false);

	/** Sets the relative file name from the root directory that will be used as initial content (usually that's index.html). */
	void setIndexFile(const String& newName)
	{
		rootFile = newName.toStdString();
	}

	/** Enables caching of the files. Set this to false if you're working on the content and want to refresh using the browser refresh button. */
	void setEnableCache(bool shouldCacheFiles);

	/** Calls a JS function for every webview.

		Be aware that the function needs to be visible in the global scope.
	*/
	void call(const String& functionName, const var& args);

	/** Evaluates the javascript code in the web view. You need to pass in an unique identifier so that the code will also be evaluated
		at initialisation when you create a new web view.
	*/
	void evaluate(const String& identifier, const String& jsCode);

	File getRootDirectory() const { return rootDirectory; }

	/** Returns true if the webview root directory is a child of the project root folder
	    and enableCache is set to true. */
	bool shouldBeEmbedded() const;

	/** Converts a embedded webview to a file based one. This only works if:
	
		1. The mode is embedded
		2. You've supplied a valid project root folder.
		3. You've called restoreFromValueTree with a ValueTree that contains the file path information
	*/
	bool explode();

	bool shouldApplyScaleFactorAsZoom() const
	{
		return applyScaleFactorAsZoom;
	}

    bool isDebugModeEnabled() const
    {
        return debugModeEnabled;
    }
    
    void setEnableDebugMode(bool shouldBeEnabled)
    {
        debugModeEnabled = shouldBeEnabled;
    }
    
private:

	File projectRootDirectory;

	bool applyScaleFactorAsZoom = true;

	bool usePersistentCalls = true;

    bool debugModeEnabled = false;
    
	StringPairArray initScripts;

	using OpaqueResourceType = std::tuple<std::vector<uint8>, std::string>;

	struct Pimpl;

	struct CallbackItem;
	
	struct ExternalResource
	{
		ExternalResource(const String& path_, const String& mimeType, const String& data);

		ExternalResource(const String& path_);;

		~ExternalResource() = default;

		const std::string path;

		OpaqueResourceType resource;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExternalResource);
	};

	OpaqueResourceType fetch(const std::string& path);

	void registerWebView(Component* c);

	void deregisterWebView(Component* c);

	Array<Component::SafePointer<Component>> registeredViews;

	bool enableCache = true;

	ServerType serverType = ServerType::Uninitialised;

	File rootDirectory;
	std::string rootFile = "/";

	friend class WebViewWrapper;

	std::function<void(const String&)> errorLogger;

	StringArray scripts;

	Pimpl* pimpl = nullptr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewData);
};

struct SorryDavid: public Component
{
    void paint(Graphics& g) override
    {
        g.fillAll(Colours::grey);
        g.setColour(Colours::black);
        g.drawText("Not there yet...", getLocalBounds().toFloat(), Justification::centred);
    }
    void doNothing(float){}
    void doNothing(void*){};
	void doNothing(){};
};

class WebViewWrapper : public Component
{
public:

#if JUCE_WINDOWS
	using NativeComponentType = juce::HWNDComponent;
#define setWindowHandle setHWND
#define resizeToFitCrossPlatform resizeToFit
#elif JUCE_MAC
	using NativeComponentType = juce::NSViewComponent;
#define setWindowHandle setView
#define resizeToFitCrossPlatform resizeToFitView
#elif JUCE_LINUX
    // Unfortunately I'm too stupid to figure out how to
    // use the Linux native handle wrapper -
    // that would be juce::XEmbedComponent
	using NativeComponentType = SorryDavid;
#define setWindowHandle doNothing
#define resizeToFitCrossPlatform doNothing
#endif

	WebViewWrapper(WebViewData::Ptr data);

	~WebViewWrapper();

	void resized();

    void navigateToURL(const URL& urlToOpen);
    
	void call(const String& jsCode);

	void refresh();

	void refreshBounds(float scaleFactor);

	WebViewData::Ptr getData() const { return data; }

private:

	float lastScaleFactor = 1.0f;

	WebViewData::Ptr data;
#if !JUCE_LINUX
	ScopedPointer<choc::ui::WebView> webView;
#endif
	ScopedPointer<NativeComponentType> content;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewWrapper);
};


}
