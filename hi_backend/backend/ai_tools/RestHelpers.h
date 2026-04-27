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

namespace hise { using namespace juce;

// Forward declaration for ReplayConsoleHandler
class InteractionTestWindow;



/** Helper utilities for REST API request handling.
    
    This struct contains utilities for handling REST API requests that interact
    with HISE's scripting system, including console output capture, error parsing,
    and script compilation. It also provides the central registry of all API routes
    and their handler implementations.
*/
struct RestHelpers
{
    //==========================================================================
    // Route types (nested to avoid polluting hise namespace)

    /** Identifies REST API endpoints. The enum value is the index into getRouteMetadata(). */
    enum class ApiRoute
    {
        ListMethods,            ///< GET  /           - List all available API methods
        Status,                 ///< GET  /api/status - Get project status
        StatusPreprocessors,    ///< GET  /api/status/preprocessors - List preprocessor catalogue with runtime values
        GetScript,              ///< GET  /api/get_script - Read script content
        SetScript,              ///< POST /api/set_script - Update script content
        EvaluateREPL,           ///< POST /api/repl - Evaluate an expression and get the result
        Recompile,              ///< POST /api/recompile - Recompile a processor
        ListComponents,         ///< GET  /api/list_components - List UI components
        GetComponentProperties, ///< GET  /api/get_component_properties - Get component properties
        GetComponentValue,      ///< GET  /api/get_component_value - Get component runtime value
        SetComponentValue,      ///< POST /api/set_component_value - Set component runtime value
        SetComponentProperties, ///< POST /api/set_component_properties - Set component properties
        TestingScreenshot,      ///< GET  /api/testing/screenshot - Capture UI screenshot
        GetSelectedComponents,  ///< GET  /api/get_selected_components - Get selected UI components
        TestingE2e,             ///< POST /api/testing/e2e - Execute end-to-end UI interaction sequence
        DiagnoseScript,         ///< POST /api/diagnose_script - Run diagnostic shadow parse
        GetIncludedFiles,       ///< GET  /api/get_included_files - List included script files
        TestingProfile,         ///< POST /api/testing/profile - Run profiling session or retrieve last result
        ParseCSS,               ///< POST /api/parse_css - Parse CSS and return diagnostics
        Shutdown,               ///< POST /api/shutdown - Gracefully quit HISE
        BuilderTree,            ///< GET  /api/builder/tree - Get module tree hierarchy
        BuilderApply,           ///< POST /api/builder/apply - Apply operations to module tree
        BuilderReset,           ///< POST /api/builder/reset - Reset module tree (File->New)
        UndoPushGroup,          ///< POST /api/undo/push_group - Start a new undo group
        UndoPopGroup,           ///< POST /api/undo/pop_group - End group, execute or discard
        UndoBack,               ///< POST /api/undo/back - Undo last action
        UndoForward,            ///< POST /api/undo/forward - Redo next action
        UndoDiff,               ///< GET  /api/undo/diff - Current diff state
        UndoHistory,            ///< GET  /api/undo/history - Full undo history
        UndoClear,              ///< POST /api/undo/clear - Clear undo history
        WizardInitialise,       ///< GET  /api/wizard/initialise - Get wizard form defaults
        WizardExecute,          ///< POST /api/wizard/execute - Execute a wizard task
        WizardStatus,           ///< GET  /api/wizard/status - Poll async job progress
        UITree,                 ///< GET  /api/ui/tree - Get UI component tree hierarchy
        UIApply,                ///< POST /api/ui/apply - Apply operations to UI component tree
        TestingSequence,        ///< POST /api/testing/sequence - Run timed test sequence (MIDI, attributes, REPL, signals)
        DspList,                ///< GET  /api/dsp/list - List available DspNetwork names
        DspInit,                ///< POST /api/dsp/init - Create/load a DspNetwork
        DspTree,                ///< GET  /api/dsp/tree - Get scriptnode network hierarchy
        DspApply,               ///< POST /api/dsp/apply - Apply operations to scriptnode graph
        DspSave,                ///< POST /api/dsp/save - Save DspNetwork to XML file
        DspScreenshot,          ///< GET  /api/dsp/screenshot - Capture screenshot of current DspNetwork graph
        ProjectList,            ///< GET  /api/project/list - List available HISE projects
        ProjectTree,            ///< GET  /api/project/tree - Project file tree with referenced flags
        ProjectFiles,           ///< GET  /api/project/files - List saveable project files
        ProjectSettingsList,    ///< GET  /api/project/settings/list - All project_info.xml settings
        ProjectSettingsSet,     ///< POST /api/project/settings/set - Update a project setting
        ProjectSave,            ///< POST /api/project/save - Save current state as XML or HIP
        ProjectLoad,            ///< POST /api/project/load - Load an XML or HIP file
        ProjectSwitch,          ///< POST /api/project/switch - Switch active project
        ProjectExportSnippet,   ///< GET  /api/project/export_snippet - Export current project as HISE snippet
        ProjectImportSnippet,   ///< POST /api/project/import_snippet - Import a HISE snippet
        ProjectPreprocessorList,///< GET  /api/project/preprocessor/list - List preprocessor defines
        ProjectPreprocessorSet, ///< POST /api/project/preprocessor/set - Set or clear a preprocessor define
        SnippetBrowser,         ///< POST /api/snippet_browser - Control the snippet browser instance (launch/shutdown/enable/disable)
        numRoutes
    };
    
    /** Parameter type for OpenAPI schema generation. */
    enum class ParamType
    {
        String,   ///< JSON string
        Int,      ///< JSON integer
        Float,    ///< JSON number
        Bool,     ///< JSON boolean
        Array,    ///< JSON array (itemType describes element type)
        Object,   ///< JSON object (free-form)
        Enum      ///< String with fixed set of values (enumValues)
    };

    /** A variant of an object schema, keyed by a discriminator value.
     *  Used for oneOf schemas (e.g., operations array where "op" determines the shape).
     */
    struct SchemaVariant
    {
        String discriminatorValue;  ///< e.g., "add", "remove", "set"
        String description;
    };

    /** Metadata for a REST API route parameter. Uses fluent builder pattern.
     *
     *  Supports nested schemas for complex objects:
     *  - Object type: properties array describes child fields
     *  - Array of objects: itemSchema points to a shared RouteParameter describing the element
     *  - Discriminated unions: discriminator + variants for oneOf schemas
     */
    struct RouteParameter
    {
        Identifier name;
        String description;
        String defaultValue;  ///< Empty = no default
        bool required = true;
        ParamType type = ParamType::String;
        StringArray enumValues;   ///< Valid values for Enum type
        String example;           ///< Example value for OpenAPI spec

        // Nested schema support
        Array<RouteParameter> properties;       ///< Child fields for Object type
        std::shared_ptr<RouteParameter> itemSchema;  ///< Element schema for Array type (replaces itemType)
        String discriminator;                   ///< Field name that selects the variant (e.g., "op", "type")
        Array<SchemaVariant> variants;          ///< Variant descriptions keyed by discriminator value

        /** Constructor with required fields. */
        RouteParameter(const Identifier& name_, const String& desc_)
            : name(name_), description(desc_) {}

        /** Set a default value (implies optional). */
        RouteParameter withDefault(const String& def) const
        {
            auto copy = *this;
            copy.defaultValue = def;
            copy.required = false;
            return copy;
        }

        /** Mark as optional without a default value. */
        RouteParameter asOptional() const
        {
            auto copy = *this;
            copy.required = false;
            return copy;
        }

        /** Set the parameter type for OpenAPI schema. */
        RouteParameter withType(ParamType t) const
        {
            auto copy = *this;
            copy.type = t;
            return copy;
        }

        /** Set allowed enum values (implies Enum type). */
        RouteParameter withEnumValues(const StringArray& values) const
        {
            auto copy = *this;
            copy.type = ParamType::Enum;
            copy.enumValues = values;
            return copy;
        }

        /** Add a child property (implies Object type). */
        RouteParameter withProperty(const RouteParameter& child) const
        {
            auto copy = *this;
            copy.type = ParamType::Object;
            copy.properties.add(child);
            return copy;
        }

        /** Set the array element schema (implies Array type). */
        RouteParameter withArrayItems(const RouteParameter& schema) const
        {
            auto copy = *this;
            copy.type = ParamType::Array;
            copy.itemSchema = std::make_shared<RouteParameter>(schema);
            return copy;
        }

        /** Set the discriminator field for oneOf schemas. */
        RouteParameter withDiscriminator(const String& fieldName) const
        {
            auto copy = *this;
            copy.discriminator = fieldName;
            return copy;
        }

        /** Add a variant for a discriminated union. */
        RouteParameter withVariant(const String& value, const String& desc) const
        {
            auto copy = *this;
            copy.variants.add({ value, desc });
            return copy;
        }

        /** Set an example value for OpenAPI documentation. */
        RouteParameter withExample(const String& ex) const
        {
            auto copy = *this;
            copy.example = ex;
            return copy;
        }
    };
    
    /** Metadata for a registered REST API route. Uses fluent builder pattern. */
    struct RouteMetadata
    {
        ApiRoute id = ApiRoute::numRoutes;
        String path;              ///< e.g., "api/status" (without leading /)
        RestServer::Method method = RestServer::GET;
        String category;          ///< "status", "scripting", "ui"
        String summary;           ///< Short one-sentence summary (OpenAPI summary)
        String description;       ///< Detailed description with behavioral notes (OpenAPI description)
        String returns;           ///< One-liner describing response
        Array<RouteParameter> queryParameters;   ///< For GET requests
        Array<RouteParameter> bodyParameters;    ///< For POST requests (JSON body)
        Array<RouteParameter> responseFields;    ///< Fields inside the "result" object
        Array<int> errorCodes;                   ///< HTTP error status codes (e.g., 400, 404)
        String requestExample;                   ///< JSON string example for request body
        String responseExample;                  ///< JSON string example for response body
        bool rejectInSnippetBrowser = false;     ///< If true, dispatcher returns 409 when the active BP is the snippet browser
        
        /** Default constructor for juce::Array compatibility. */
        RouteMetadata() = default;
        
        /** Constructor with required fields. */
        RouteMetadata(ApiRoute id_, const String& path_)
            : id(id_), path(path_) {}
        
        RouteMetadata withMethod(RestServer::Method m) const
        {
            auto copy = *this;
            copy.method = m;
            return copy;
        }
        
        RouteMetadata withCategory(const String& cat) const
        {
            auto copy = *this;
            copy.category = cat;
            return copy;
        }
        
        RouteMetadata withSummary(const String& s) const
        {
            auto copy = *this;
            copy.summary = s;
            return copy;
        }

        RouteMetadata withDescription(const String& desc) const
        {
            auto copy = *this;
            copy.description = desc;
            return copy;
        }
        
        RouteMetadata withReturns(const String& ret) const
        {
            auto copy = *this;
            copy.returns = ret;
            return copy;
        }
        
        RouteMetadata withQueryParam(const RouteParameter& p) const
        {
            auto copy = *this;
            copy.queryParameters.add(p);
            return copy;
        }
        
        RouteMetadata withBodyParam(const RouteParameter& p) const
        {
            auto copy = *this;
            copy.bodyParameters.add(p);
            return copy;
        }
        
        RouteMetadata withRequestExample(const String& ex) const
        {
            auto copy = *this;
            copy.requestExample = ex;
            return copy;
        }

        RouteMetadata withResponseExample(const String& ex) const
        {
            auto copy = *this;
            copy.responseExample = ex;
            return copy;
        }

        RouteMetadata withResponseField(const RouteParameter& p) const
        {
            auto copy = *this;
            copy.responseFields.add(p);
            return copy;
        }

        RouteMetadata withErrorCodes(const Array<int>& codes) const
        {
            auto copy = *this;
            copy.errorCodes = codes;
            return copy;
        }

        /** Convenience: adds standard moduleId query parameter. */
        RouteMetadata withModuleIdParam() const
        {
            return withQueryParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"));
        }

        /** Marks this route as forbidden when the snippet browser is the active BackendProcessor.
            Used for endpoints that would mutate the user's real project (project/* writes,
            wizard/* operations) and must not be accidentally executed against snippet content. */
        RouteMetadata rejectsInSnippetBrowser() const
        {
            auto copy = *this;
            copy.rejectInSnippetBrowser = true;
            return copy;
        }
    };
    
    //==========================================================================
    
    /** Base class for scoped console capture.
     *  
     *  Captures console output during its lifetime by setting a custom logger
     *  on the MainController's CodeHandler. Only captures if enabled AND no
     *  existing custom logger is set.
     *
     *  Subclasses implement handleMessage() and handleError() to process captured output.
     */
    struct BaseScopedConsoleHandler : public ControlledObject
    {
        BaseScopedConsoleHandler(MainController* mc, bool enabled);
        virtual ~BaseScopedConsoleHandler();
        
        bool isCapturing() const { return capturing; }
        
        // Error parsing utilities
        struct ParsedError
        {
            String message;      // "API call with undefined parameter 0"
            String location;     // "Scripts/funky.js:9:16"
            String functionName; // "dudel" (empty if not a callstack entry)

            /** Returns formatted callstack entry: "dudel() at Scripts/funky.js:9:16"
                or just location if no function name. */
            String toCallstackString() const;
        };

        /** Parses an error string and extracts message, location, and optional function name.

            Handles both formats:
            - Error message: "API call with undefined parameter 0 {{SW50ZXJm...}}"
            - Callstack entry: ":\t\t\tdudel() - funky.js (9)\t{{SW50ZXJm...}}"

            @param errorString   The full error/callstack string
            @param scriptRoot    The project's Scripts folder for resolving full paths
            @param moduleId      The script processor's module ID - used as fallback filename

            @returns ParsedError with message, location, and optional functionName
        */
        static ParsedError parseError(const String& errorString,
            const File& scriptRoot,
            const String& moduleId);

    protected:
        virtual void handleMessage(const String& message) = 0;
        virtual void handleError(const String& message, const StringArray& callstack) = 0;
        
        
        
        
        
    private:
        void onMessage(const String& message, int warning, const Processor* p);
        bool capturing = false;
    };
    
    //==========================================================================
    
    /** Scoped handler that captures console output during request processing.
        
        While this object exists, any Console.print() calls or error messages
        will be captured and added to the AsyncRequest's logs/errors arrays,
        which are then merged into the JSON response.
        
        Inherits from IConsoleCapture so it can be attached to AsyncRequest
        with proper lifetime management (destroyed when request completes).
    */
    struct ScopedConsoleHandler : public BaseScopedConsoleHandler,
                                  public RestServer::AsyncRequest::IConsoleCapture
    {
        ScopedConsoleHandler(MainController* mc, RestServer::AsyncRequest::Ptr request_);
        
    protected:
        void handleMessage(const String& message) override;
        void handleError(const String& message, const StringArray& callstack) override;
        
    private:
        // Reference - safe because AsyncRequest owns this ScopedConsoleHandler,
        // so AsyncRequest always outlives this object
        RestServer::AsyncRequest& request;
    };
    
    //==========================================================================
    
    /** Console handler for replay that captures to StatusBar log.
     *  
     *  Used when executeInteractions() is called directly (not through REST API)
     *  to capture console output and display it in the test window's log panel.
     */
    struct ReplayConsoleHandler : public BaseScopedConsoleHandler
    {
        ReplayConsoleHandler(MainController* mc, InteractionTestWindow* window, bool enabled);
        
        /** Build result JSON and log to StatusBar. Call after execution completes. */
        void finalize(const var& inputInteractions, bool success, 
                      int interactionsCompleted, int totalElapsedMs);
        
    protected:
        void handleMessage(const String& message) override;
        void handleError(const String& message, const StringArray& callstack) override;
        
    private:
        InteractionTestWindow* window;
        StringArray logs;
        Array<var> errors;
    };
    
    struct WizardExecutor: public ControlledObject
    {
        struct WizardStateManager : public BaseStateManager
        {
            WizardStateManager(MainController* mc, const RestServer::Request& req_):
              BaseStateManager(mc),
              req(req_),
              result(new DynamicObject())
            {}

            void initAnswers()
            {
				auto jsonBody = req.getJsonBody();
				auto id = Identifier(jsonBody[RestApiIds::wizardId]);
				answers = jsonBody[RestApiIds::answers];
            }

            void write(const Identifier& id, const var& newValue, NotificationType) override
            {
                result.getDynamicObject()->setProperty(id, newValue);
            }

            var read(const Identifier& id) const override
            {
                return answers[id];
            }

            void addToLog(const String& message) override
            {
                logs.add(message);
            }

            const RestServer::Request req;
            var answers;
            var result;

            Array<var> logs;
        };
     
        struct Item
        {
            BaseStateManager::InitFunction init;
            BaseStateManager::Executor     exec;
        };
        
        

        RestServer::Response initialise(const RestServer::Request& req)
        {
            WizardStateManager ws(getMainController(), req);

            if(executors.find(wizardId) != executors.end())
            {
                executors.at(wizardId).init(&ws);
                setResult(ws);
            }
            else
            {
                setError("Can't find wizard with id " + wizardId.toString());
            }
                           
            return makeResponse();
        }
        
        /** class stub for a wizard executor. */
        struct DummyTask
        {
            /** Called before the dialog is shown, return a JSON with all default values. */
            static void initialise(BaseStateManager*)
            {
                throw Result::fail("unimplemented");
            }

            /** Called with the values from the wizard. Perform the operation and return. */
            static void execute(BaseStateManager*) {}
        };

		struct AsyncRunner : public ControlledObject,
                             public Thread,
                             public juce::Logger
		{
            AsyncRunner(MainController* mc) :
                ControlledObject(mc),
                Thread("Async job execution")
            {}

            RestServer::Response add(const RestServer::Request& req)
            {
                if (pending)
                {
                    stopThread(1000);
                }

                currentLog = nullptr;
                errors.clear();
                result = var();
                pending = true;
                progress = 0.0;
                
				auto obj = req.getJsonBody();

				auto wizardId = obj[RestApiIds::wizardId].toString();

                jobId = wizardId + "_" + Time::getCurrentTime().formatted("%H_%M_%S");

                currentResponse = {};
                request = req;

                startThread(5);

                return makeResponse();
            }

			RestServer::Response makeResponse()
			{
                auto o = new DynamicObject();

				o->setProperty(RestApiIds::success, true);
				o->setProperty(RestApiIds::finished, !pending);
				o->setProperty(RestApiIds::jobId, jobId);
				o->setProperty(RestApiIds::progress, progress);
                o->setProperty(RestApiIds::result, result);

                Array<var> thisLogs;

                if(currentLog != nullptr)
                    currentLog->swapWith(thisLogs);

				o->setProperty(RestApiIds::logs, thisLogs);
				o->setProperty(RestApiIds::errors, errors);

				return RestServer::Response::ok(var(o));
			}

            String getActiveJobId() { return jobId; }


			void logMessage(const String& message) override
			{
                if(currentLog != nullptr)
                    currentLog->add(message);
			}

        private:

            var result;
            Array<var>* currentLog;
            Array<var> finishedLogs;
            Array<var> errors;

            String jobId;

			void run() override
			{
				auto obj = request.getJsonBody();

				auto wizardId = obj[RestApiIds::wizardId].toString();
				auto answers = obj[RestApiIds::answers];
				auto tasks = obj[RestApiIds::tasks];

                WizardExecutor e(getMainController(), wizardId);

                currentLog = &e.logs;

                auto prevLogger = Logger::getCurrentLogger();

                Logger::setCurrentLogger(this);

                e.execute(request, nullptr);

                Logger::setCurrentLogger(prevLogger);

                pending = false;
                progress = 1.0;

                finishedLogs.addArray(e.logs);

                currentLog = &finishedLogs;

                result = e.result;
			}

            double progress = 0.0;

            RestServer::Request request;
            bool pending = false;
            RestServer::Response currentResponse;
		};

        template <typename T> void registerExecutor(const Identifier& id)
        {
            executors[id] = { T::initialise, T::execute };
        }
        
		WizardExecutor(MainController* mc, const Identifier& wizardId_) :
			ControlledObject(mc),
            wizardId(wizardId_)
		{
            registerExecutors();
        };

		RestServer::Response execute(const RestServer::Request& req, AsyncRunner* runner)
		{
			if (runner != nullptr)
			{
                return runner->add(req);
			}
			else
			{
				WizardStateManager ws(getMainController(), req);

				ws.initAnswers();

				if (executors.find(wizardId) != executors.end())
				{
					executors.at(wizardId).exec(&ws);
					setResult(ws);
                    logs.addArray(ws.logs);
				}

				else
				{
					setError("Can't find wizard with id " + wizardId.toString());
				}

				return makeResponse();
			}
		}

    private:
        
        Identifier wizardId;

        void registerExecutors();

        RestServer::Response makeResponse() const
        {
            auto o = new DynamicObject();
            o->setProperty(RestApiIds::success, success);
            o->setProperty(RestApiIds::result, result);  // Empty defaults placeholder
            o->setProperty(RestApiIds::logs, logs);
            o->setProperty(RestApiIds::errors, errors);
            
            return RestServer::Response::ok(var(o));
        }
        
        void addLog(const String& message)
        {
            logs.add(var(message));
        }
        
        void setResult(const WizardStateManager& ws)
        {
            success = true;
            result = ws.result;
        }
        
        void setError(const String& error)
        {
            success = false;
            errors.add(error);
        }
        
        bool success = true;
        var result;
        Array<var> logs;
        Array<var> errors;
        
        std::map<Identifier, Item> executors;
    };
    
	

    /** Gets a JavascriptProcessor from the request's moduleId parameter.
        
        @param mc   The MainController
        @param req  The async request containing the moduleId parameter
        @returns    The JavascriptProcessor, or nullptr if not found
    */
    static JavascriptProcessor* getScriptProcessor(MainController* mc, 
                                                   RestServer::AsyncRequest::Ptr req);
    
    /** Waits for any pending control callbacks to complete by pumping the message loop.
        
        @param sc        The component to wait for
        @param timeoutMs Maximum time to wait (default 200ms)
    */
    static void waitForPendingCallbacks(ScriptComponent* sc, int timeoutMs = 200);
    
    /** Builds a hierarchical property tree for a UI component.
        
        Creates a DynamicObject with the component's properties and recursively
        includes all child components in a "childComponents" array.
        
        Used by the /api/list_components?hierarchy=true endpoint.
        
        @param sc   The script component to build the tree for
        @returns    DynamicObject with id, type, position, size, and childComponents
    */
    static DynamicObject::Ptr createRecursivePropertyTree(ScriptComponent* sc);

    /** Robustly converts a var to bool, handling JSON booleans, integers, and
        string representations ("true"/"false"/"0"/"1").
        Use this for all boolean API parameters from POST JSON bodies.
    */
    static bool getTrueValue(const var& v);

    //==========================================================================
    // Route metadata registry
    
    /** Returns metadata for all registered API routes.
        The array index corresponds to the ApiRoute enum value.
    */
    static const Array<RouteMetadata>& getRouteMetadata();
    
    /** Get the path string for a route. */
    static String getRoutePath(ApiRoute route);
    
    /** Find route by subURL path. Returns ApiRoute::numRoutes if not found. */
    static ApiRoute findRoute(const String& subURL);
    
    //==========================================================================
    // Route handlers (static methods called from BackendProcessor::onAsyncRequest)
    
    /** Handler for GET / - List all available API methods. */
    static RestServer::Response handleListMethods(MainController* mc, 
                                                  RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/status - Get project status. */
    static RestServer::Response handleStatus(MainController* mc,
                                             RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/status/preprocessors - List preprocessor catalogue with runtime values.
     *  verbose=false (default): flat { name: int } map.
     *  verbose=true: full Entry metadata per preprocessor, with 'value' overridden to the runtime value.
     */
    static RestServer::Response handleStatusPreprocessors(MainController* mc,
                                                          RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/get_script - Read script content. */
    static RestServer::Response handleGetScript(MainController* mc, 
                                                RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/set_script - Update script content. */
    static RestServer::Response handleSetScript(MainController* mc, 
                                                RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/recompile - Recompile a script processor. */
    static RestServer::Response handleRecompile(MainController* mc, 
                                                RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/repl - Evaluate a REPL expression. */
	static RestServer::Response handleEvaluateREPL(MainController* mc,
		                                           RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/list_components - List UI components. */
    static RestServer::Response handleListComponents(MainController* mc, 
                                                     RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/get_component_properties - Get component properties. */
    static RestServer::Response handleGetComponentProperties(MainController* mc, 
                                                             RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/get_component_value - Get component runtime value. */
    static RestServer::Response handleGetComponentValue(MainController* mc, 
                                                        RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/set_component_value - Set component runtime value. */
    static RestServer::Response handleSetComponentValue(MainController* mc, 
                                                        RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/set_component_properties - Set component properties. */
    static RestServer::Response handleSetComponentProperties(MainController* mc, 
                                                             RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/testing/screenshot - Capture UI screenshot. */
    static RestServer::Response handleTestingScreenshot(MainController* mc,
                                                        RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/get_selected_components - Get selected UI components. */
    static RestServer::Response handleGetSelectedComponents(MainController* mc, 
                                                            RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/testing/e2e - Execute end-to-end UI interaction sequence.
     *  Note: Takes BackendProcessor* to access InteractionTester.
     */
    static RestServer::Response handleTestingE2e(BackendProcessor* bp,
                                                  RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/diagnose_script - Run diagnostic shadow parse.
     *  Accepts moduleId and/or filePath. Reads file from disk, runs shadow parse,
     *  returns structured diagnostics without modifying runtime state.
     */
    static RestServer::Response handleDiagnoseScript(MainController* mc, 
                                                     RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/get_included_files - List included script files.
     *  Without moduleId: returns all included files across all processors (with owning processor).
     *  With moduleId: returns files for that specific processor only.
     */
    static RestServer::Response handleGetIncludedFiles(MainController* mc, 
                                                       RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/testing/profile - Start profiling or retrieve last result.
     *  mode="record": starts a new session (non-blocking, returns immediately).
     *  mode="get": returns last result, with optional filter/summary parameters.
     *  When recording is in progress, "get" blocks until done (unless wait=false).
     */
    static RestServer::Response handleTestingProfile(MainController* mc,
                                                      RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/parse_css - Parse CSS code and return diagnostics.
     *  HISE-agnostic: does not require a script processor.
     *  Accepts either inline code or a file path to a .css file.
     *  Optionally resolves properties for a set of selectors using CSS specificity.
     */
    static RestServer::Response handleParseCSS(MainController* mc, 
                                               RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/shutdown - Gracefully quit HISE.
     *  Sends the HTTP 200 response before scheduling the quit on the message thread.
     */
    static RestServer::Response handleShutdown(MainController* mc, 
                                                RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/builder/tree - Get module tree hierarchy */
    static RestServer::Response handleBuilderTree(MainController* mc, 
                                                   RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/builder/apply - Apply operations to module tree */
    static RestServer::Response handleBuilderApply(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/builder/reset - Reset module tree (File->New) */
    static RestServer::Response handleBuilderReset(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/undo/push_group - Start a new undo group */
    static RestServer::Response handleUndoPushGroup(MainController* mc,
                                                     RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/undo/pop_group - End group, execute or discard */
    static RestServer::Response handleUndoPopGroup(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/undo/back - Undo last action */
    static RestServer::Response handleUndoBack(MainController* mc,
                                                RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/undo/forward - Redo next action */
    static RestServer::Response handleUndoForward(MainController* mc,
                                                   RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/undo/diff - Current diff state */
    static RestServer::Response handleUndoDiff(MainController* mc,
                                                RestServer::AsyncRequest::Ptr req);
    
    /** Handler for GET /api/undo/history - Full undo history */
    static RestServer::Response handleUndoHistory(MainController* mc,
                                                   RestServer::AsyncRequest::Ptr req);
    
    /** Handler for POST /api/undo/clear - Clear undo history */
    static RestServer::Response handleUndoClear(MainController* mc,
                                                 RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/wizard/initialise - Get wizard form defaults */
    static RestServer::Response handleWizardInitialise(MainController* mc,
                                                        RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/wizard/execute - Execute a wizard task */
    static RestServer::Response handleWizardExecute(MainController* mc,
                                                     RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/wizard/status - Poll async job progress */
    static RestServer::Response handleWizardStatus(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/ui/tree - Get UI component tree hierarchy */
    static RestServer::Response handleUITree(MainController* mc,
                                              RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/ui/apply - Apply operations to UI component tree */
    static RestServer::Response handleUIApply(MainController* mc,
                                               RestServer::AsyncRequest::Ptr req);

    //==========================================================================
    // MIDI injection

    /** Info about a pending note-off event. */
    struct PendingNoteOff
    {
        int channel = 0;
        int noteNumber = 0;
        double fireTimeMs = 0.0;  // absolute time (hi-res)
        bool valid = false;       // true if this represents a real pending note-off
    };

    /** Inject one MIDI message into keyboard state.
     *  No timing logic — caller handles scheduling.
     *  Designed to be callable from InteractionDispatcher in the future.
     *
     *  @param mc          MainController to inject into
     *  @param type        Message type: "note", "cc", "pitchbend", "allNotesOff"
     *  @param channel     MIDI channel (1-16)
     *  @param noteNumber  Note number (0-127), used for "note" type
     *  @param velocity    Velocity (0.0-1.0), used for "note" type
     *  @param controller  CC number (0-127), used for "cc" type
     *  @param value       CC value (0-127) or pitchbend value (0-16383)
     *  @returns           PendingNoteOff with valid=true if this was a note-on
     */
    static PendingNoteOff dispatchSingleMidiMessage(
        MainController* mc, const String& type, int channel,
        int noteNumber, float velocity, int controller, int value);

    /** Handler for POST /api/testing/sequence - Run timed test sequence */
    static RestServer::Response handleTestingSequence(BackendProcessor* bp,
                                                       RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/dsp/list - List available DspNetwork names */
    static RestServer::Response handleDspList(MainController* mc,
                                              RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/dsp/init - Create/load a DspNetwork */
    static RestServer::Response handleDspInit(MainController* mc,
                                              RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/dsp/tree - Get scriptnode network hierarchy */
    static RestServer::Response handleDspTree(MainController* mc,
                                              RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/dsp/apply - Apply operations to scriptnode graph */
    static RestServer::Response handleDspApply(MainController* mc,
                                               RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/dsp/save - Save DspNetwork to XML file */
    static RestServer::Response handleDspSave(MainController* mc,
                                              RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/dsp/screenshot - Capture screenshot of current DspNetwork graph */
    static RestServer::Response handleDspScreenshot(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/project/list - List available HISE projects */
    static RestServer::Response handleProjectList(MainController* mc,
                                                  RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/project/tree - Project file tree with runtime reference flags */
    static RestServer::Response handleProjectTree(MainController* mc,
                                                  RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/project/files - List saveable project files */
    static RestServer::Response handleProjectFiles(MainController* mc,
                                                   RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/project/settings/list - All project_info.xml settings */
    static RestServer::Response handleProjectSettingsList(MainController* mc,
                                                          RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/project/settings/set - Update a project setting */
    static RestServer::Response handleProjectSettingsSet(MainController* mc,
                                                         RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/project/save - Save current state as XML or HIP */
    static RestServer::Response handleProjectSave(MainController* mc,
                                                  RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/project/load - Load an XML or HIP file */
    static RestServer::Response handleProjectLoad(MainController* mc,
                                                  RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/project/switch - Switch active project */
    static RestServer::Response handleProjectSwitch(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/project/export_snippet - Export current project as HISE snippet */
    static RestServer::Response handleProjectExportSnippet(MainController* mc,
                                                           RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/project/import_snippet - Import a HISE snippet */
    static RestServer::Response handleProjectImportSnippet(MainController* mc,
                                                           RestServer::AsyncRequest::Ptr req);

    /** Handler for GET /api/project/preprocessor/list - List preprocessor defines */
    static RestServer::Response handleProjectPreprocessorList(MainController* mc,
                                                              RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/project/preprocessor/set - Upsert or clear a preprocessor define */
    static RestServer::Response handleProjectPreprocessorSet(MainController* mc,
                                                             RestServer::AsyncRequest::Ptr req);

    /** Handler for POST /api/snippet_browser - Control snippet browser instance lifecycle.
        Operates on the main BackendProcessor regardless of which BP is currently active. */
    static RestServer::Response handleSnippetBrowser(MainController* mc,
                                                     RestServer::AsyncRequest::Ptr req);

#if HISE_INCLUDE_PROFILING_TOOLKIT
    /** Query options for filtering and summarizing profiling results. */
    struct ProfileQueryOptions
    {
        bool summary = false;
        bool nested = false;
        String filter;              // wildcard pattern (e.g., "slow*"), empty = no filter
        String sourceTypeFilter;    // wildcard pattern (e.g., "Trace"), empty = no filter
        double minDuration = 0.0;
        int limit = 15;
        StringArray threadFilter;   // thread names to include (empty = all threads)

        bool hasFilters() const
        {
            return summary || filter.isNotEmpty() || sourceTypeFilter.isNotEmpty()
                   || minDuration > 0.0 || threadFilter.size() > 0;
        }

        static ProfileQueryOptions fromJson(const var& json)
        {
            ProfileQueryOptions opts;
            opts.summary = getTrueValue(json.getProperty(RestApiIds::summary, false));
            opts.nested = getTrueValue(json.getProperty(RestApiIds::nested, false));
            opts.filter = json.getProperty(RestApiIds::filter, "").toString();
            opts.sourceTypeFilter = json.getProperty(RestApiIds::sourceTypeFilter, "").toString();
            opts.minDuration = (double)json.getProperty(RestApiIds::minDuration, 0.0);
            opts.limit = jlimit(1, 100, (int)json.getProperty(RestApiIds::limit, 15));

            auto tf = json.getProperty(RestApiIds::threadFilter, var());

            if (tf.isArray())
            {
                for (int i = 0; i < tf.size(); i++)
                    opts.threadFilter.add(tf[i].toString());
            }

            return opts;
        }
    };

    /** Starts a fire-and-forget profiling session from body JSON params.
     *  Reads durationMs, threadFilter, eventFilter from bodyJson.
     *  Returns true if recording started, false if already recording.
     */
    static bool startProfilingSession(MainController* mc, const var& bodyJson, 
                                       double defaultDurationMs = 1000.0);

    /** Walks a ProfileInfoBase tree and returns a JSON-ready DynamicObject 
        with "threads" array and "flows" array (cross-thread causal connections). */
    static var profilingResultToJson(
        DebugSession::ProfileDataSource::ProfileInfoBase* root);

    /** Walks a ProfileInfoBase tree with filtering/aggregation and returns a 
        JSON-ready DynamicObject with "results" array and "flows" array. */
    static var profilingResultToSummary(
        DebugSession::ProfileDataSource::ProfileInfoBase* root,
        const ProfileQueryOptions& options);
#endif

private:
    // Builder helpers
    
    /** Resolve chain name string to raw::IDs::Chains constant.
     *  Returns -2 if invalid. Case-insensitive.
     */
    static int resolveChainIndex(const String& chainName);
    
    /** Get chain name from index constant. */
    static String getChainName(int chainIndex);
    
    /** Validate module type for chain using ProcessorMetadata.
     *  Returns true if valid, sets errorHints on failure.
     */
    static bool validateTypeForChain(const Identifier& typeId, 
                                      int chainIndex, 
                                      DynamicObject::Ptr errorHints);
    
    /** Find processor by name in tree. */
    static Processor* findProcessorByName(MainController* mc, const String& name);
    
    struct TreeOptions
    {
        bool includeParameters = false;
        bool verbose = false;
    };

    struct ProcessorOrValueTree
    {
        ProcessorMetadata getMetadata() const
        {
            if (p != nullptr)
                return p->getMetadata();
            else
            {
                ProcessorMetadataRegistry rd;

                if (auto pmd = rd.get(v.getType()))
                    return *pmd;
            }

            return {};
        }

        bool isBypassed() const
        {
            if (p != nullptr)
                return p->isBypassed();

            return v[scriptnode::PropertyIds::Bypassed];
        }

        String getId() const
        {
            if (p != nullptr)
                return p->getId();

            return v[PropertyIds::ID].toString();
        }

        String getColour() const
        {
            if (p != nullptr)
                return "#" + p->getColour().toDisplayString(false);

            return v[PropertyIds::ModColour].toString();
        }

        Identifier getType() const
        {
            if (p != nullptr)
                return p->getType();

            return v.getType();
        }

        bool isRuntimeData() const { return p != nullptr; }

        int getNumChildren() const
        {
            if (p != nullptr)
                return p->getNumChildProcessors();

            return v.getNumChildren();
        }

        ProcessorOrValueTree getChild(int i) const
        {
            Processor* c = (p != nullptr && p->getNumChildProcessors() > i) ? p->getChildProcessor(i) : nullptr;
            ValueTree cv = v.isValid() ? v.getChild(i) : ValueTree();
            return { c, cv };
        }

        float getAttribute(int parameterIndex) const
        {
            if (p != nullptr)
                return p->getAttribute(parameterIndex);

            return 0.0f;
        }

        Processor* p = nullptr;
        ValueTree v;
    };

    /** Build JSON tree representation of module hierarchy. */
    static var buildModuleTree(const ProcessorOrValueTree& root, const TreeOptions& options);
    
    /** Build JSON array for a specific chain. */
    static Array<var> buildChainArray(Processor* parent, int chainIndex);
};

//==============================================================================
/** Asynchronous MIDI message dispatcher using a high-resolution timer.
 *
 *  Owned by BackendProcessor. Queues MIDI messages from HTTP requests and
 *  dispatches them with precise timing. Notes automatically schedule note-off
 *  events to prevent stuck notes.
 *
 *  Thread-safe: queueMessages() and getStatus() are called from the HTTP thread,
 *  hiResTimerCallback() runs on the timer's dedicated thread. All MidiKeyboardState
 *  methods are internally synchronized.
 */
class MidiInjector : public HighResolutionTimer
{
public:

	MidiInjector(MainController* mc);
	~MidiInjector() override;

	//==========================================================================
	/** Parsed MIDI event ready for scheduling. */
	struct ScheduledEvent
	{
		String type;        // "note", "cc", "pitchbend", "allNotesOff", "repl", "set_attribute"
		int channel = 1;
		int noteNumber = 0;
		float velocity = 1.0f;
		int controller = 0;
		int value = 0;
		int duration = 500;  // note-off delay (ms), only for "note"
		double fireTimeMs;   // absolute hi-res time to dispatch
		String expression;   // REPL expression, only for "repl"
		String replId;       // Optional identifier for "repl" results
		String moduleId;     // Target processor for "repl" / "set_attribute" (default: "Interface")
		int parameterIndex = -1;    // Resolved parameter index, only for "set_attribute"
		float attributeValue = 0.0f; // Parameter value, only for "set_attribute"
		String signal;              // Signal type, only for "testsignal"
		float frequency = 440.0f;   // Signal frequency, only for "testsignal"
		float startFrequency = 20.0f;  // Sweep start, only for "testsignal"
		float endFrequency = 20000.0f; // Sweep end, only for "testsignal"
	};

	/** Currently sounding note awaiting automatic note-off. */
	struct ActiveNote
	{
		int channel;
		int noteNumber;
		double noteOffTimeMs;  // absolute hi-res time
	};

	/** Snapshot of the injector state for API response. */
	struct Status
	{
		bool isPlaying = false;
		int durationMs = 0;
		int activeNotes = 0;
		int eventsInSequence = 0;
		int playedEvents = 0;
		double progress = 0.0;
	};

	//==========================================================================

	/** Parse and queue MIDI messages from JSON array. Thread-safe.
	 *  Delays are relative to "now"; converted to absolute timestamps internally.
	 *  allNotesOff with delay=0 triggers immediate panic and clears the queue.
	 */
	void queueMessages(const Array<var>& messages);

	/** Get current playback status. Thread-safe. */
	Status getStatus() const;

	/** Retrieve and clear accumulated REPL results. Thread-safe. */
	Array<var> takeReplResults();

	//==========================================================================
	// HighResolutionTimer
	void hiResTimerCallback() override;

private:

	MainController* mc;
	mutable CriticalSection lock;

	Array<ScheduledEvent> scheduledEvents;  // sorted by fireTimeMs
	Array<ActiveNote> activeNotes;          // sorted by noteOffTimeMs
	Array<var> replResults;                 // REPL evaluation results

	int totalEvents = 0;
	int playedEvents = 0;
	double sequenceStartMs = 0;
	double sequenceEndMs = 0;

	void fireEvent(const ScheduledEvent& e);
	void fireReplEvent(const ScheduledEvent& e);
	void fireTestSignalEvent(const ScheduledEvent& e);
	void fireNoteOff(const ActiveNote& n);
	void fireDueNoteOffs(double now);
	void panic();
	void scheduleNextCallback();

	// Test signal cache
	HashMap<String, AudioSampleBuffer> signalCache;

	static String makeSignalCacheKey(const ScheduledEvent& e, double sampleRate);
	static AudioSampleBuffer generateSignal(const String& signal, double sampleRate,
	    int numSamples, float frequency, float startFreq, float endFreq);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiInjector)
};

} // namespace hise
