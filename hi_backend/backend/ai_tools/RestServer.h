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

//==============================================================================
/** REST API contract version - stamped onto every JSON envelope as `apiVersion`.

    Bump (semver) whenever the response envelope or any route's request/response
    schema changes. Compile-time constant: there is no setter, no init call, no
    runtime field. Consumers read it off any response (or `/api/status`) to
    verify they are talking to a HISE build that matches their expected schema.
*/
#define HISE_REST_API_VERSION "0.5.0"

namespace hise { using namespace juce;

//==============================================================================
/** Identifiers for REST API parameter names and JSON response keys.
    Using Identifiers instead of string literals provides compile-time safety
    and better performance for property lookups.
*/
#define DECLARE_ID(x) const Identifier x(#x);

namespace RestApiIds
{
    // Request/body parameters
    DECLARE_ID(moduleId);
    DECLARE_ID(callback);
    DECLARE_ID(script);
    DECLARE_ID(compile);
    DECLARE_ID(hierarchy);

    // Common response fields
    DECLARE_ID(success);
    DECLARE_ID(result);
    DECLARE_ID(logs);
    DECLARE_ID(errors);
    DECLARE_ID(errorMessage);
    DECLARE_ID(callstack);
    DECLARE_ID(apiVersion);  // Auto-injected envelope version (see HISE_REST_API_VERSION)

    // list_methods response
    DECLARE_ID(methods);
    DECLARE_ID(path);
    DECLARE_ID(method);
    DECLARE_ID(category);
    DECLARE_ID(description);
    DECLARE_ID(returns);
    DECLARE_ID(queryParameters);
    DECLARE_ID(bodyParameters);
    DECLARE_ID(name);
    DECLARE_ID(required);
    DECLARE_ID(defaultValue);
    DECLARE_ID(inverted);

    // status response
    DECLARE_ID(server);
    DECLARE_ID(version);
    DECLARE_ID(commitHash);
    DECLARE_ID(compileTimeout);
    DECLARE_ID(project);
    DECLARE_ID(projectFolder);
    DECLARE_ID(scriptsFolder);
    DECLARE_ID(scriptProcessors);
    DECLARE_ID(isMainInterface);
    DECLARE_ID(externalFiles);
    DECLARE_ID(callbacks);
    DECLARE_ID(id);
    DECLARE_ID(type);
    DECLARE_ID(empty);

    // list_components response
    DECLARE_ID(components);
    DECLARE_ID(childComponents);
    DECLARE_ID(visible);
    DECLARE_ID(enabled);
    DECLARE_ID(x);
    DECLARE_ID(y);
    DECLARE_ID(width);
    DECLARE_ID(height);

    // get_component_properties response
    DECLARE_ID(properties);
    DECLARE_ID(value);
    DECLARE_ID(isDefault);
    DECLARE_ID(options);

    // get/set_component_value
    DECLARE_ID(min);
    DECLARE_ID(max);
    DECLARE_ID(validateRange);

    // Debug parameters
    DECLARE_ID(forceSynchronousExecution);
    DECLARE_ID(warning);

    // set_component_property
    DECLARE_ID(changes);
    DECLARE_ID(force);
    DECLARE_ID(applied);
    DECLARE_ID(locked);
    DECLARE_ID(property);
    DECLARE_ID(recompileRequired);

    // set_script response
    DECLARE_ID(updatedCallbacks);

    // repl
    DECLARE_ID(expression);

    // screenshot
    DECLARE_ID(scale);
    DECLARE_ID(imageData);
    DECLARE_ID(outputPath);
    DECLARE_ID(filePath);
    DECLARE_ID(selectionCount);

    // LAF (LookAndFeel) integration
    DECLARE_ID(laf);                  // LAF info object for a component
    DECLARE_ID(location);             // location of the LAF definition
    DECLARE_ID(renderStyle);          // "script", "css", "css_inline", "mixed"
    DECLARE_ID(cssLocation);          // Full path to external CSS file
    DECLARE_ID(lafRenderWarning);     // Warning object when LAF render timeout
    DECLARE_ID(unrenderedComponents); // Array of unrendered component objects
    DECLARE_ID(timeoutMs);            // Timeout value in milliseconds
    DECLARE_ID(reason);               // Reason for not rendering: "invisible" or "timeout"

    // Interaction testing
    DECLARE_ID(interactions);         // Array of interaction objects
    DECLARE_ID(interactionsCompleted); // Number of interactions successfully executed
    DECLARE_ID(totalElapsedMs);       // Total execution time in milliseconds
    DECLARE_ID(executionLog);         // Array of executed events with timing
    DECLARE_ID(screenshots);          // Object with screenshot id -> base64 PNG data
    DECLARE_ID(parseWarnings);        // Array of non-fatal parse warnings
    DECLARE_ID(verbose);              // If true, include extra debug info in response
    DECLARE_ID(mouseState);           // Final mouse state object (when verbose=true)
    DECLARE_ID(currentTarget);        // Component ID mouse is currently over
    DECLARE_ID(pixelPosition);        // Absolute pixel position {x, y}

    // SelectMenuItem response fields
    DECLARE_ID(selectedMenuItem);     // Object with selected menu item info
    DECLARE_ID(text);                 // Menu item display text
    DECLARE_ID(itemId);               // Menu item ID

    // diagnose_script response
    DECLARE_ID(diagnostics);          // Array of diagnostic objects
    DECLARE_ID(line);                 // Diagnostic line number
    DECLARE_ID(column);               // Diagnostic column number
    DECLARE_ID(severity);             // "error", "warning", "info", "hint"
    DECLARE_ID(source);               // "syntax", "api-validation", "type-check", "language", "callscope"
    DECLARE_ID(message);              // Diagnostic message text
    DECLARE_ID(suggestions);          // Array of "did you mean?" suggestion strings
    DECLARE_ID(async);                // If true, defer shadow parse to scripting thread (default: false)

    // get_included_files
    DECLARE_ID(files);                // Array of included file entries
    DECLARE_ID(processor);            // Owning processor ID for an included file

    // profile / attachable profiling
    DECLARE_ID(durationMs);           // Profiling duration in milliseconds
    DECLARE_ID(threadFilter);         // Array of thread names to filter
    DECLARE_ID(eventFilter);          // Array of event type names to filter
    DECLARE_ID(threads);              // Array of thread profiling results
    DECLARE_ID(thread);               // Thread name string
    DECLARE_ID(events);               // Array of profiled events in a thread
    DECLARE_ID(duration);             // Event duration in ms
    DECLARE_ID(sourceType);           // Event source type name
    DECLARE_ID(children);             // Nested child events
    DECLARE_ID(start);                // Event start time in ms
    DECLARE_ID(mode);                 // "record" or "get"
    DECLARE_ID(recording);            // Whether a recording is in progress
    DECLARE_ID(profile);              // Enable attached profiling on an endpoint
    DECLARE_ID(trackSource);          // Track source ID (causal link open end, -1 = none)
    DECLARE_ID(trackTarget);          // Track target ID (causal link close end, -1 = none)
    DECLARE_ID(flows);                // Array of cross-thread causal flow connections
    DECLARE_ID(trackId);              // Shared integer ID connecting a flow source to target
    DECLARE_ID(sourceEvent);          // Name of the event that caused the flow
    DECLARE_ID(targetEvent);          // Name of the event that was caused
    DECLARE_ID(sourceThread);         // Thread where the flow originated
    DECLARE_ID(targetThread);         // Thread where the flow terminated

    // Profile query/summary parameters
    DECLARE_ID(summary);              // If true, aggregate repeated events with stats
    DECLARE_ID(filter);               // Wildcard pattern matched against event name
    DECLARE_ID(minDuration);          // Only include events with duration >= this (ms)
    DECLARE_ID(sourceTypeFilter);     // Wildcard pattern matched against sourceType name
    DECLARE_ID(nested);               // When filtering, include children of matched events
    DECLARE_ID(limit);                // Max results returned (default 15)
    DECLARE_ID(wait);                 // If false, return immediately when recording in progress

    // Summary result fields
    DECLARE_ID(results);              // Flat array of filtered/aggregated events
    DECLARE_ID(count);                // Number of occurrences in summary
    DECLARE_ID(median);               // Median duration (ms)
    DECLARE_ID(peak);                 // Maximum duration (ms)
    DECLARE_ID(total);                // Sum of durations (ms)

    // parse_css
    DECLARE_ID(code);                 // CSS code string to parse
    DECLARE_ID(selectors);            // Array of selector strings for specificity resolution
    DECLARE_ID(resolved);             // Resolved pixel value for a property

    // Wizard endpoints
    DECLARE_ID(wizardId);             // Wizard identifier string
    DECLARE_ID(answers);              // Key/value form data from wizard
    DECLARE_ID(tasks);                // Array of task function names to execute
    DECLARE_ID(jobId);                // Job identifier for async wizard tasks
    DECLARE_ID(finished);             // Whether an async job has completed
    DECLARE_ID(progress);             // Progress value 0.0-1.0 for async jobs

    // Builder endpoints
    DECLARE_ID(operations);           // Array of builder operation objects
    DECLARE_ID(parent);               // Parent module name
    DECLARE_ID(chain);                // Chain index (-1=direct, 0=midi, 1=gain, 2=pitch, 3=fx)
    DECLARE_ID(target);               // Target module name
    DECLARE_ID(buildIndex);           // Index in creation order
    DECLARE_ID(chains);               // Child chains object (tree response)
    DECLARE_ID(bypassed);             // Module bypass state
    DECLARE_ID(hints);                // Validation hints object
    DECLARE_ID(validChains);          // Array of valid chain names for a type
    const Identifier template_("template");;            // Name template with {n} placeholder
    DECLARE_ID(created);              // Array of created module names
    DECLARE_ID(parameters);
    DECLARE_ID(prettyName);

    // Undo system
    DECLARE_ID(scope);                // "group" or "root"
    DECLARE_ID(domain);               // Domain filter string (e.g., "builder", "ui")
    DECLARE_ID(flatten);              // Flatten groups / compute net effect
    DECLARE_ID(cancel);               // pop_group cancel flag
    DECLARE_ID(group);                // Current group name in response
    DECLARE_ID(groupName);            // Current group name in response
    DECLARE_ID(depth);                // Stack depth in response
    DECLARE_ID(cursor);               // Undo position in history
    DECLARE_ID(history);              // History array
    DECLARE_ID(index);                // History index (maps to cursor)
    DECLARE_ID(symbol);               // Diff entry symbol (+/-/*)
    DECLARE_ID(action);               // Diff entry action type
    DECLARE_ID(diff);                 // diff list 
    DECLARE_ID(op);                   // Operation type (add, remove, clone, set_attributes, etc.)
    DECLARE_ID(attributes);           // Parameter values object {paramName: value}
    DECLARE_ID(effect);               // Effect/network name for HotswappableEffect modules

    // UI component endpoints
    DECLARE_ID(componentType);        // Component type (ScriptButton, ScriptPanel, etc.)
    DECLARE_ID(newId);                // New component ID for rename
    DECLARE_ID(saveInPreset);         // Whether component value is saved in presets
    DECLARE_ID(parentId);             // Parent component ID for add/move operations
    DECLARE_ID(keepPosition);         // Preserve absolute position when reparenting

    // testing/sequence
    DECLARE_ID(messages);             // Array of MIDI message objects
    DECLARE_ID(noteNumber);           // MIDI note number (0-127)
    DECLARE_ID(velocity);             // Note velocity (0.0-1.0)
    DECLARE_ID(channel);              // MIDI channel (1-16)
    DECLARE_ID(controller);           // CC controller number (0-127)
    DECLARE_ID(timestamp);            // Absolute time offset in ms from start of sequence
    DECLARE_ID(isPlaying);            // Whether MIDI sequence is still playing
    DECLARE_ID(activeNotes);          // Number of notes currently on
    DECLARE_ID(eventsInSequence);     // Total logical events in queue
    DECLARE_ID(playedEvents);         // Number of events dispatched so far
    DECLARE_ID(replResults);          // Array of REPL evaluation results from testing/sequence
    DECLARE_ID(processorId);          // Target processor ID for set_attribute
    DECLARE_ID(parameterId);          // Parameter name for set_attribute
    DECLARE_ID(propertyId);           // Property name for DSP node properties
    DECLARE_ID(blocking);             // If true, wait for sequence to complete before responding
    DECLARE_ID(signal);               // Test signal type (sine, saw, sweep, dirac, noise, silence)
    DECLARE_ID(frequency);            // Signal frequency in Hz
    DECLARE_ID(startFrequency);       // Sweep start frequency in Hz
    DECLARE_ID(endFrequency);         // Sweep end frequency in Hz
    DECLARE_ID(recordOutput);         // File path to record audio output to WAV

    // project endpoints
    DECLARE_ID(projects);             // Array of available HISE projects
    DECLARE_ID(active);               // Name of currently active project
    DECLARE_ID(projectName);          // Project name for tree response
    DECLARE_ID(root);                 // Root node of project file tree
    DECLARE_ID(modified);             // ISO8601 timestamp of last modification
    DECLARE_ID(masterChainRenamed);   // True if HIP save renamed the master chain
    DECLARE_ID(newName);              // New master chain name after rename
    DECLARE_ID(snippet);              // HISE snippet string
    DECLARE_ID(settings);             // Flat object of project_info.xml settings
    DECLARE_ID(key);                  // Setting key name
    DECLARE_ID(format);               // Save format ("xml" or "hip")
    DECLARE_ID(file);                 // Relative path to XML or HIP file
    DECLARE_ID(filename);             // Relative path to XML or HIP file
    DECLARE_ID(referenced);           // True if file is actively referenced by HISE runtime
    DECLARE_ID(OS);                   // Target operating system (Windows, macOS, Linux, all)
    DECLARE_ID(preprocessor);         // Preprocessor macro name
    DECLARE_ID(preprocessors);        // Array of preprocessor entries
    DECLARE_ID(skipDefaults);         // If true, omit preprocessors whose runtime value equals the default

    // dsp (scriptnode)
    DECLARE_ID(nodeId);               // Node instance ID within a network
    DECLARE_ID(factoryPath);          // Node factory path (e.g. core.oscillator)
    DECLARE_ID(networks);             // Array of network names
    DECLARE_ID(connections);          // Array of connection objects in tree
    DECLARE_ID(sourceOutput);         // Connection source output name
    DECLARE_ID(parameter);            // Parameter ID for connect/disconnect
    DECLARE_ID(stepSize);             // Parameter step size
    DECLARE_ID(middlePosition);       // Parameter middle position
    DECLARE_ID(skewFactor);           // Parameter skew factor
    DECLARE_ID(matchRange);           // connect op flag: copy target range onto source after wiring

    // snippet browser
    DECLARE_ID(exists);               // Whether a snippet browser instance is alive
    DECLARE_ID(activeIsSnippetBrowser);  // /api/status: is the active BP the snippet browser?

}

#undef DECLARE_ID



//==============================================================================
/**
    A simple REST API server with a JUCE-style interface.
    
    Wraps cpp-httplib to provide a thread-safe HTTP server that can be used
    to control HISE via a REST API.
    
    @note Route handlers are called on the server thread, NOT the message thread.
          Use MessageManager::callAsync() or your own synchronization if you need
          to interact with JUCE components or HISE internals.
    
    Routes are defined using juce::URL objects, which handle query parameters.
    Use getBaseURL() to get a starting URL, then chain getChildURL() and
    withParameter() to define your route.
    
    @code
    RestServer server;
    
    // Simple route
    server.addRoute(RestServer::GET, 
        server.getBaseURL().getChildURL("api/status"),
        [](const RestServer::Request& req) {
            DynamicObject::Ptr json = new DynamicObject();
            json->setProperty("status", "running");
            return RestServer::Response::ok(json.get());
        });
    
    // Route with parameters (accessed via query string: ?moduleId=value)
    server.addRoute(RestServer::GET, 
        server.getBaseURL()
            .getChildURL("api/recompile")
            .withParameter("moduleId", ""),  // empty default = required
        [](const RestServer::Request& req) {
            auto moduleId = req[Identifier("moduleId")];
            if (moduleId.isEmpty())
                return RestServer::Response::badRequest("moduleId is required");
            return RestServer::Response::ok("Recompiling " + moduleId);
        });
    
    server.start(5000);
    @endcode
*/
class RestServer
{
public:
    //==============================================================================
    /** Port used for unit testing. When running on this port, async requests execute synchronously. */
    static constexpr int TestPort = 1901;
    
    //==============================================================================
    enum Method { GET, POST, PUT, DELETE };

    //==============================================================================
    /** Represents an incoming HTTP request. */
    struct Request
    {
        Method method = GET;            //< HTTP method (GET, POST, etc.)
        URL url;                        //< Full URL with path, query params, and POST data
        StringPairArray headers;        //< HTTP headers

        /** Get a query parameter value by Identifier.
            @param name  Parameter name as Identifier
            @returns     Parameter value, or empty string if not present
        */
        String operator[](const Identifier& name) const;

        bool getTrueValue(const Identifier& name) const;
        
        /** Parse POST body as JSON. Returns undefined var on parse failure. */
        var getJsonBody() const;
    };

    //==============================================================================
    /** Represents an HTTP response to send back to the client. */
    struct Response
    {
        int statusCode = 200;
        String contentType = "application/json";
        String body;

        //==============================================================================
        // Factory methods for common responses

        /** 200 OK with JSON body. */
        static Response ok(const var& json);

        /** 200 OK with raw string body. */
        static Response ok(const String& body, const String& contentType);

        /** 400 Bad Request. */
        static Response badRequest(const String& message);

        /** 404 Not Found. */
        static Response notFound(const String& message = "Not found");

        /** 500 Internal Server Error. */
        static Response internalError(const String& message);

        /** Custom error with any status code. */
        static Response error(int statusCode, const String& message);
    };

    //==============================================================================
    /** Callback type for route handlers. */
    using RouteHandler = std::function<Response(const Request&)>;

    //==============================================================================
    /** Formats a double to a clean string, rounded to maxDecimalPlaces.
        Eliminates floating point noise (e.g., 0.300000011920929 -> "0.3").
    */
    static String formatFloat(double value, int maxDecimalPlaces = 4);

    /** Recursively normalizes all floating point values in a var tree.
        - Doubles are rounded to maxDecimalPlaces
        - Strings have embedded numbers cleaned up (5+ decimal places)
        - Arrays and objects are recursed into
    */
    static void normalizeFloatsInVar(var& v, int maxDecimalPlaces = 4);

    /** Forces an immediate repaint of a native window handle (Windows only).
        Used by InteractionTester to ensure synthetic mouse events trigger visual updates.
        On non-Windows platforms, this is a no-op.
    */
    static void forceRepaintWindow(void* nativeHandle);

    //==============================================================================
    /** A reference-counted async request object for coordinating work across threads.
        
        Use this with addAsyncRoute() when you need to dispatch work to another thread
        (e.g., via killVoicesAndCall) and block the HTTP response until complete.
        
        @code
        server.addAsyncRoute(RestServer::GET, "/api/recompile", [this](RestServer::AsyncRequest::Ptr asyncReq)
        {
            ProcessorFunction pf = [asyncReq](Processor* proc)
            {
                // Do work on the target thread...
                asyncReq->complete(RestServer::Response::ok(result));
                return SafeFunctionCall::OK;
            };
            
            getKillStateHandler().killVoicesAndCall(getMainSynthChain(), pf, 
                KillStateHandler::TargetThread::SampleLoadingThread);
            
            return asyncReq->waitForResponse();
        });
        @endcode
    */
    class AsyncRequest : public juce::ReferenceCountedObject
    {
    public:
        using Ptr = ReferenceCountedObjectPtr<AsyncRequest>;

        /** Represents an error with optional callstack information. */
        struct ErrorEntry
        {
            String errorMessage;
            StringArray callstack;
            int64 hash = 0;  // For internal deduplication (not serialized)
        };

        /** Creates an AsyncRequest wrapping the given HTTP request. */
        AsyncRequest(const Request& r) : request(r) {}

        /** Returns the original HTTP request. */
        const Request& getRequest() const { return request; }

        /** Append a log message (thread-safe).
            @param message The message to add to the log.
        */
        void appendLog(const String& message);

        /** Append an error with optional callstack (thread-safe).
            @param errorMessage The error message.
            @param callstack    Optional callstack frames (e.g., "functionName() - Line 7, column 16").
        */
        void appendError(const String& errorMessage, const StringArray& callstack = {});

        /** Call this from the target thread when work is complete.
            
            This will merge any collected logs and errors into the response body
            before making it available to the waiting HTTP thread.
            
            @param r The response to send back to the HTTP client.
        */
        void complete(const Response& r)
        {
            response = r;
            mergeLogsIntoResponse();
            completed.store(true);
        }

        /** Completes the request with an error and returns the response.
            
            Use this for preflight checks before dispatching to another thread,
            or for errors that occur during async execution.
            
            @param statusCode HTTP status code (e.g., 400, 404, 500)
            @param message    Error message to include in the response.
            @returns          The error response (for immediate return from handler).
            
            @code
            // Preflight check - no need to kill voices
            if (module == nullptr)
                return asyncReq->fail(404, "Module not found");
            
            // Otherwise dispatch to another thread...
            @endcode
        */
        Response fail(int statusCode, const String& message)
        {
            complete(Response::error(statusCode, message));
            return waitForResponse();
        }

        /** Blocks until complete() is called or timeout expires.
            @param timeoutMs Maximum time to wait in milliseconds (default 30 seconds).
            @returns The response set via complete(), or a 504 timeout error.
        */
        Response waitForResponse(int timeoutMs = 30000)
        {
            auto startTime = Time::getMillisecondCounter();

            while (!completed.load())
            {
                if (Time::getMillisecondCounter() - startTime > (uint32)timeoutMs)
                    return Response::error(504, "Operation timed out");

                Thread::sleep(50);
            }

            return response;
        }

        /** Returns true if running in test mode (synchronous execution). */
        bool isTestMode() const { return testMode; }

        /** Sets test mode for synchronous execution. Called internally by addAsyncRoute. */
        void setTestMode(bool shouldBeSync) { testMode = shouldBeSync; }

        /** Opaque interface for console capture (implemented by HISE-specific code).
            Allows the RestServer to hold a console handler without knowing HISE internals.
        */
        struct IConsoleCapture {
            virtual ~IConsoleCapture() = default;
        };
        using ConsoleCapturePtr = std::unique_ptr<IConsoleCapture>;
        
        /** Attach a console capture handler to this request.
            The handler lives until the request is destroyed.
        */
        void setConsoleCapture(ConsoleCapturePtr capture) { 
            consoleCapture = std::move(capture); 
        }

        /** Returns true if any errors have been captured. */
        bool hasErrors() const { return !errors.isEmpty(); }

        /** Enable this if you have defined a custom error field that you don't want to be overwritten with
            the console error output. The RestUndoManager uses this for operations that define a custom callstack.
        */
        void setUseCustomErrors(bool shouldUseCustomErrors)
        {
            usesCustomErrors = shouldUseCustomErrors;
        }

    private:
        ConsoleCapturePtr consoleCapture;
        /** Merges collected logs and errors into the response body as JSON. */
        void mergeLogsIntoResponse();

        bool usesCustomErrors = false;

        Request request;
        std::atomic<bool> completed{false};
        Response response;
        bool testMode = false;

        CriticalSection logLock;
        StringArray logs;
        Array<ErrorEntry> errors;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsyncRequest)
    };

    //==============================================================================
    /** Callback type for async route handlers. */
    using AsyncRouteHandler = std::function<Response(AsyncRequest::Ptr)>;

    //==============================================================================
    /** Listener interface for server events.
        
        @note Listeners are called on the server thread, not the message thread.
              Use MessageManager::callAsync() if you need to update UI.
        
        @note State is automatically synchronised on registration: addListener()
              calls serverStarted() if the server is already running, and
              removeListener() calls serverStopped() before removing.
    */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when the server starts successfully. */
        virtual void serverStarted(int port) {}

        /** Called when the server stops. */
        virtual void serverStopped() {}

        /** Called when a request is received (for logging/debugging). */
        virtual void requestReceived(const String& method, const String& path) {}

        /** Called when an error occurs (e.g., exception in handler). */
        virtual void serverError(const String& message) {}
    };

    //==============================================================================
    RestServer();

    /** Destructor. Calls stop() automatically (RAII). */
    ~RestServer();

    //==============================================================================
    /** Returns a base URL for building route definitions.
        
        Use this with getChildURL() and withParameter() to define routes.
        
        @code
        server.addRoute(RestServer::GET, 
            server.getBaseURL()
                .getChildURL("api/recompile")
                .withParameter("moduleId", ""),
            handler);
        @endcode
        
        @returns URL with "http://localhost:PORT" where PORT is the server port,
                 or 0 if not yet started.
    */
    URL getBaseURL() const;

    /** Register a route handler.
        
        @param method   The HTTP method (GET, POST, PUT, DELETE)
        @param routeUrl URL defining the path and default parameter values.
                        Use getBaseURL().getChildURL().withParameter() to build.
        @param handler  Function called when route matches. Called on server thread!
        
        @note Call this before start(). Adding routes while running is not supported.
    */
    void addRoute(Method method, const URL& routeUrl, RouteHandler handler);

    /** Register an async route handler for operations that dispatch to other threads.
        
        This is a convenience method that wraps the handler with AsyncRequest creation.
        Use this when you need to dispatch work via killVoicesAndCall() or similar
        and block until the operation completes.
        
        @param method   The HTTP method (GET, POST, PUT, DELETE)
        @param routeUrl URL defining the path and default parameter values.
                        Use getBaseURL().getChildURL().withParameter() to build.
        @param handler  Function that receives an AsyncRequest::Ptr. Must call
                        asyncReq->complete() and return asyncReq->waitForResponse().
        
        @note Call this before start(). Adding routes while running is not supported.
        
        @see AsyncRequest
    */
    void addAsyncRoute(Method method, const URL& routeUrl, AsyncRouteHandler handler);

    //==============================================================================
    /** Start listening for connections.

        @param port                 Port number to listen on
        @param bindAddress          Address to bind to. Default "127.0.0.1" for localhost only.
        @param corsAllowedOrigins   CORS policy for the `Access-Control-Allow-Origin` header.
                                    `"*"` (default) allows any origin, `""` disables CORS entirely,
                                    or a comma-separated origin list to whitelist specific origins.
        @returns                    true if server started successfully
    */
    bool start(int port, const String& bindAddress = "127.0.0.1", const String& corsAllowedOrigins = "*");

    /** Stop the server. 
        
        Idempotent (safe to call multiple times). 
        Blocks until server thread has terminated.
    */
    void stop();

    /** Returns true if the server is currently running. */
    bool isRunning() const;

    /** Returns the port the server is listening on, or 0 if not running. */
    int getPort() const;

    /** Returns true if server is running on the test port (synchronous async execution). */
    bool isTestMode() const { return getPort() == TestPort; }

    //==============================================================================
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RestServer)
    JUCE_DECLARE_WEAK_REFERENCEABLE(RestServer)
};

} // namespace hise

