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

namespace hise {
using namespace juce;

struct RestApiEndpoints
{
	using RouteParameter = RestHelpers::RouteParameter;
	using ParamType      = RestHelpers::ParamType;
	using RouteMetadata  = RestHelpers::RouteMetadata;
	using ApiRoute       = RestHelpers::ApiRoute;

	/* GET / */
	static void listMethods(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ListMethods, "")
			.withCategory("status")
			.withSummary("List all available API methods as an OpenAPI 3.0.3 spec")
			.withDescription("Returns a full OpenAPI 3.0.3 JSON specification describing every "
				"registered endpoint, including parameters, request/response schemas, examples, "
				"and error codes. Use this for programmatic discovery and MCP tool generation.")
			.withReturns("OpenAPI 3.0.3 JSON specification"));
	}

	/* GET /api/status */
	static void status(Array<RouteMetadata>& m)
	{
		auto callbackEntry = RouteParameter(Identifier("entry"), "Callback entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::id, "Callback name (e.g. onInit, onNoteOn)"))
			.withProperty(RouteParameter(RestApiIds::empty, "True if callback has no content")
				.withType(ParamType::Bool));

		auto processorEntry = RouteParameter(Identifier("entry"), "Script processor entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::moduleId, "Unique processor identifier"))
			.withProperty(RouteParameter(RestApiIds::isMainInterface, "True if this processor renders the plugin UI")
				.withType(ParamType::Bool))
			.withProperty(RouteParameter(RestApiIds::externalFiles, "Included .js files relative to scriptsFolder")
				.withType(ParamType::Array))
			.withProperty(RouteParameter(RestApiIds::callbacks, "Array of callback entries")
				.withArrayItems(callbackEntry));

		auto serverObj = RouteParameter(RestApiIds::server, "Server info")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::version, "REST API version string"))
			.withProperty(RouteParameter(RestApiIds::commitHash, "Git commit hash embedded at build time (PREVIOUS_HISE_COMMIT)"))
			.withProperty(RouteParameter(RestApiIds::compileTimeout, "Compile timeout in seconds")
				.withType(ParamType::Int));

		auto projectObj = RouteParameter(RestApiIds::project, "Project info")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::name, "Project name"))
			.withProperty(RouteParameter(RestApiIds::projectFolder, "Full path to project folder"))
			.withProperty(RouteParameter(RestApiIds::scriptsFolder, "Full path to Scripts folder"));

		m.add(RouteMetadata(ApiRoute::Status, "api/status")
			.withCategory("status")
			.withSummary("Get project status and discover available script processors")
			.withDescription("Returns server version, project paths, and a list of all script "
				"processors with their callbacks and external file references. Use this as the "
				"first call to discover moduleId values for other endpoints. "
				"The activeIsSnippetBrowser flag tells whether the BackendProcessor that handled "
				"this request is the snippet browser instance; while true, project/* and wizard/* "
				"endpoints return 409.")
			.withReturns("Server info, project info, scriptProcessors array, and the activeIsSnippetBrowser flag")
			.withResponseField(RouteParameter(RestApiIds::activeIsSnippetBrowser,
				"True when the active BackendProcessor is the snippet browser")
				.withType(ParamType::Bool))
			.withResponseField(serverObj)
			.withResponseField(projectObj)
			.withResponseField(RouteParameter(RestApiIds::scriptProcessors, "Array of script processor entries")
				.withArrayItems(processorEntry))
			.withResponseExample(R"({"success": true, "activeIsSnippetBrowser": false, "server": {"version": "1.0.0", "commitHash": "cf2cb9d33a6958c5aa67aa237b60adc5f2e9f7b5", "compileTimeout": 10}, "project": {"name": "My Project", "projectFolder": "D:/Projects/MyPlugin", "scriptsFolder": "D:/Projects/MyPlugin/Scripts"}, "scriptProcessors": [{"moduleId": "Interface", "isMainInterface": true, "externalFiles": ["utils.js"], "callbacks": [{"id": "onInit", "empty": false}]}], "logs": [], "errors": []})"));
	}

	/* GET /api/status/preprocessors */
	static void statusPreprocessors(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::StatusPreprocessors, "api/status/preprocessors")
			.withMethod(RestServer::GET)
			.withCategory("status")
			.withSummary("List HISE preprocessor macros with their current runtime values")
			.withDescription("Returns the catalogue of HISE preprocessor macros tracked by "
				"PreprocessorDataBase. Each entry's 'value' is the runtime value, which honours "
				"hot-reloadable overrides sourced from MainController's extra definitions and can "
				"therefore differ from the compile-time default that is baked into the binary. "
				"Use verbose=false (default) for a compact {name: value} map suited to quick "
				"inspection; verbose=true returns the full per-entry metadata (category, brief, "
				"description, defaults, flags, cross-references) with 'value' overridden to the "
				"runtime value.")
			.withReturns("Object mapping preprocessor macro name to its runtime integer value "
				"(verbose=false) or to the full entry metadata (verbose=true).")
			.withQueryParam(RouteParameter(RestApiIds::verbose,
					"If true, return full per-entry metadata. If false, return a flat {name: value} map.")
				.withType(ParamType::Bool)
				.withDefault("false"))
			.withQueryParam(RouteParameter(RestApiIds::skipDefaults,
					"If true, omit preprocessors whose runtime value equals the entry's default. "
					"Useful for spotting explicitly configured overrides.")
				.withType(ParamType::Bool)
				.withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::preprocessors,
					"Object keyed by preprocessor macro name. Values are integers (verbose=false) "
					"or full Entry objects (verbose=true).")
				.withType(ParamType::Object))
			.withRequestExample("GET /api/status/preprocessors?verbose=false")
			.withResponseExample(R"({"success": true, "preprocessors": {"HISE_USE_EXTENDED_TEMPO_VALUES": 0, "USE_MOD2_WAVETABLESIZE": 1}, "logs": [], "errors": []})"));
	}

	/* GET /api/get_script */
	static void getScript(Array<RouteMetadata>& m)
	{
		auto fileEntry = RouteParameter(Identifier("entry"), "External file entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::name, "File name"))
			.withProperty(RouteParameter(RestApiIds::path, "Full file path"));

		m.add(RouteMetadata(ApiRoute::GetScript, "api/get_script")
			.withCategory("scripting")
			.withSummary("Read script content from a processor's callbacks")
			.withDescription("Returns script content for one or all callbacks. onInit returns raw content "
				"(no function wrapper). Other callbacks return the full function signature. "
				"When no callback is specified, returns a callbacks object keyed by callback name. "
				"Also returns externalFiles with name and full path.")
			.withReturns("Callbacks object with script content. Includes externalFiles array.")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::callback,
				"Specific callback name (e.g. onInit). If omitted, returns all callbacks.").asOptional())
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::callbacks, "Object with callback names as keys and script content as values")
				.withType(ParamType::Object))
			.withResponseField(RouteParameter(RestApiIds::externalFiles, "Array of external file entries")
				.withArrayItems(fileEntry))
			.withErrorCodes({ 404 })
			.withRequestExample(R"(GET /api/get_script?moduleId=Interface&callback=onInit)")
			.withResponseExample("{\"success\": true, \"moduleId\": \"Interface\", \"callbacks\": {\"onInit\": \"Content.makeFrontInterface(600, 500);\"}, \"externalFiles\": [{\"name\": \"utils.js\", \"path\": \"D:/Projects/Scripts/utils.js\"}], \"logs\": [], \"errors\": []}"));
	}

	/* POST /api/set_script */
	static void setScript(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::SetScript, "api/set_script")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withSummary("Update one or more callbacks and optionally compile")
			.withDescription("Update script content for one or more callbacks and optionally trigger "
				"compilation. Only specified callbacks are updated; others remain unchanged. "
				"The response includes compilation status, updated callback names, and any "
				"lafRenderWarning listing unrendered LAF components with reason (invisible or timeout).")
			.withReturns("Compilation result with updatedCallbacks, success, logs, errors, and optional lafRenderWarning")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID")
				.withExample("Interface"))
			.withBodyParam(RouteParameter(RestApiIds::callbacks, "Object with callback names as keys and script content as values")
				.withType(ParamType::Object)
				.withExample("{\"onInit\": \"Content.makeFrontInterface(600, 500);\"}"))
			.withBodyParam(RouteParameter(RestApiIds::compile, "Whether to compile after setting")
				.withType(ParamType::Bool).withDefault("true"))
			.withBodyParam(RouteParameter(RestApiIds::forceSynchronousExecution,
				"Debug tool: Bypass threading model for synchronous execution. "
				"WARNING: May cause crashes due to race conditions - use only as last resort after saving.")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::result, "Compilation status message")
				.asOptional())
			.withResponseField(RouteParameter(RestApiIds::updatedCallbacks, "Array of callback names that were updated")
				.withType(ParamType::Array))
			.withResponseField(RouteParameter(RestApiIds::externalFiles, "Array of included .js file names after compilation")
				.withType(ParamType::Array))
			.withResponseField(RouteParameter(RestApiIds::forceSynchronousExecution, "Whether synchronous mode was used")
				.withType(ParamType::Bool).asOptional())
			.withResponseField(RouteParameter(RestApiIds::warning, "Warning when synchronous mode was used")
				.asOptional())
			.withErrorCodes({ 404 })
			.withRequestExample("{\"moduleId\": \"Interface\", \"callbacks\": {\"onInit\": \"Content.makeFrontInterface(600, 500);\\nConsole.print(123);\"}, \"compile\": true}")
			.withResponseExample(R"({"success": true, "result": "Recompiled OK", "updatedCallbacks": ["onInit"], "externalFiles": ["utils.js"], "logs": ["123"], "errors": []})"));
	}

	/* POST /api/repl */
	static void evaluateRepl(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::EvaluateREPL, "api/repl")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withSummary("Evaluate a HISEScript expression and return the result")
			.withDescription("Evaluates a script expression using the current script engine. "
				"The processor must have been compiled at least once. "
				"Any side effects of the evaluation will change the runtime state of HISE.")
			.withReturns("Evaluation result with value, success status, logs, and error messages")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID")
				.withExample("Interface"))
			.withBodyParam(RouteParameter(RestApiIds::expression, "The HISEScript expression to evaluate")
				.withExample("Engine.getSampleRate()"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::result, "Status message")
				.asOptional())
			.withResponseField(RouteParameter(RestApiIds::value, "The evaluated result value"))
			.withErrorCodes({ 400, 404, 500 })
			.withRequestExample("{\"moduleId\": \"Interface\", \"expression\": \"Engine.getSampleRate()\"}")
			.withResponseExample(R"({"success": true, "moduleId": "Interface", "result": "REPL Evaluation OK", "value": 44100, "logs": [], "errors": []})"));
	}

	/* POST /api/recompile */
	static void recompile(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::Recompile, "api/recompile")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withSummary("Recompile a processor without changing its script")
			.withDescription("Recompile a processor, restoring preset values and triggering callbacks "
				"for saveInPreset components. Use after editing external .js files on disk, "
				"or to re-run initialization code. Optionally starts a profiling session alongside "
				"compilation. The response includes the externalFiles list which may have changed.")
			.withReturns("Compilation result with success, logs, errors, externalFiles, and optional lafRenderWarning")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID")
				.withExample("Interface"))
			.withBodyParam(RouteParameter(RestApiIds::forceSynchronousExecution,
				"Debug tool: Bypass threading model for synchronous execution. "
				"WARNING: May cause crashes due to race conditions - use only as last resort after saving.")
				.withType(ParamType::Bool).withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::profile,
				"Start a profiling session alongside compilation. "
				"Retrieve results later via POST /api/testing/profile with mode=\"get\".")
				.withType(ParamType::Bool).withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::durationMs,
				"Profiling duration in ms when profile=true (100-5000)")
				.withType(ParamType::Int).withDefault("2000"))
			.withResponseField(RouteParameter(RestApiIds::result, "Compilation status message")
				.asOptional())
			.withResponseField(RouteParameter(RestApiIds::externalFiles, "Array of included .js file names after compilation")
				.withType(ParamType::Array))
			.withResponseField(RouteParameter(RestApiIds::forceSynchronousExecution, "Whether synchronous mode was used")
				.withType(ParamType::Bool).asOptional())
			.withResponseField(RouteParameter(RestApiIds::warning, "Warning when synchronous mode was used")
				.asOptional())
			.withErrorCodes({ 404 })
			.withRequestExample(R"({"moduleId": "Interface"})")
			.withResponseExample(R"({"success": true, "result": "Recompiled OK", "externalFiles": ["utils.js"], "logs": [], "errors": []})"));
	}

	/* GET /api/list_components */
	static void listComponents(Array<RouteMetadata>& m)
	{
		auto componentFlat = RouteParameter(Identifier("entry"), "Component entry (flat)")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::id, "Component ID"))
			.withProperty(RouteParameter(RestApiIds::type, "Component type (e.g. ScriptButton, ScriptPanel)"));

		m.add(RouteMetadata(ApiRoute::ListComponents, "api/list_components")
			.withCategory("ui")
			.withSummary("List all UI components in a script processor")
			.withDescription("Returns a flat list or hierarchical tree of all UI components. "
				"Flat list includes id, type, and laf info. Hierarchical tree adds layout "
				"properties (x, y, width, height, visible, enabled) and nested childComponents.")
			.withReturns("Array of components (flat list or nested tree with layout properties)")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::hierarchy, "If true, returns nested tree with layout properties")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::components, "Array of component entries")
				.withArrayItems(componentFlat))
			.withErrorCodes({ 404 })
			.withRequestExample(R"(GET /api/list_components?moduleId=Interface)")
			.withResponseExample(R"({"success": true, "moduleId": "Interface", "components": [{"id": "Button1", "type": "ScriptButton"}, {"id": "Panel1", "type": "ScriptPanel"}], "logs": [], "errors": []})"));
	}

	/* GET /api/get_component_properties */
	static void getComponentProperties(Array<RouteMetadata>& m)
	{
		auto propEntry = RouteParameter(Identifier("entry"), "Property entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::id, "Property name"))
			.withProperty(RouteParameter(RestApiIds::value, "Current value (int, bool, string, or hex color)"))
			.withProperty(RouteParameter(RestApiIds::isDefault, "True if value has not been explicitly changed")
				.withType(ParamType::Bool))
			.withProperty(RouteParameter(RestApiIds::options, "Available choices for enum-type properties")
				.withType(ParamType::Array).asOptional());

		m.add(RouteMetadata(ApiRoute::GetComponentProperties, "api/get_component_properties")
			.withCategory("ui")
			.withSummary("Get all properties for a specific UI component")
			.withDescription("Returns the component type and a full array of properties with current "
				"values, default status, and available options for enum-type properties.")
			.withReturns("Component type and array of properties with id, value, isDefault, and options")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::id, "The component's ID (e.g. Button1, Panel1)")
				.withExample("Button1"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::id, "The component's ID"))
			.withResponseField(RouteParameter(RestApiIds::type, "Component type (e.g. ScriptButton)"))
			.withResponseField(RouteParameter(RestApiIds::properties, "Array of property entries")
				.withArrayItems(propEntry))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"(GET /api/get_component_properties?moduleId=Interface&id=Button1)")
			.withResponseExample(R"({"success": true, "moduleId": "Interface", "id": "Button1", "type": "ScriptButton", "properties": [{"id": "x", "value": 159, "isDefault": true}, {"id": "visible", "value": true, "isDefault": true}], "logs": [], "errors": []})"));
	}

	/* GET /api/get_component_value */
	static void getComponentValue(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::GetComponentValue, "api/get_component_value")
			.withCategory("ui")
			.withSummary("Get the current runtime value of a UI component")
			.withDescription("Returns the current value and min/max range for the specified component.")
			.withReturns("Component type, current value, and min/max range")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::id, "The component's ID (e.g. GainKnob, BypassButton)")
				.withExample("GainKnob"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::id, "The component's ID"))
			.withResponseField(RouteParameter(RestApiIds::type, "Component type (e.g. ScriptSlider)"))
			.withResponseField(RouteParameter(RestApiIds::value, "Current runtime value")
				.withType(ParamType::Float))
			.withResponseField(RouteParameter(RestApiIds::min, "Minimum value for the component's range")
				.withType(ParamType::Float))
			.withResponseField(RouteParameter(RestApiIds::max, "Maximum value for the component's range")
				.withType(ParamType::Float))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"(GET /api/get_component_value?moduleId=Interface&id=GainKnob)")
			.withResponseExample(R"({"success": true, "moduleId": "Interface", "id": "GainKnob", "type": "ScriptSlider", "value": -3.5, "min": -12.0, "max": 12.0, "logs": [], "errors": []})"));
	}

	/* POST /api/set_component_value */
	static void setComponentValue(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::SetComponentValue, "api/set_component_value")
			.withMethod(RestServer::POST)
			.withCategory("ui")
			.withSummary("Set the runtime value of a UI component")
			.withDescription("Sets the component's value and triggers its control callback, just as "
				"if the user had interacted with the control. Console.print() output from the "
				"callback appears in the logs array. Use validateRange to check the value before setting.")
			.withReturns("Success status")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID")
				.withExample("Interface"))
			.withBodyParam(RouteParameter(RestApiIds::id, "The component's ID")
				.withExample("GainKnob"))
			.withBodyParam(RouteParameter(RestApiIds::value, "The value to set")
				.withType(ParamType::Float)
				.withExample("6.0"))
			.withBodyParam(RouteParameter(RestApiIds::validateRange,
				"If true, validates value is within component's min/max range")
				.withType(ParamType::Bool).withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::forceSynchronousExecution,
				"Debug tool: Bypass threading model for synchronous execution. "
				"WARNING: May cause crashes due to race conditions - use only as last resort after saving.")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::id, "The component's ID"))
			.withResponseField(RouteParameter(RestApiIds::type, "Component type"))
			.withResponseField(RouteParameter(RestApiIds::forceSynchronousExecution, "Whether synchronous mode was used")
				.withType(ParamType::Bool).asOptional())
			.withResponseField(RouteParameter(RestApiIds::warning, "Warning when synchronous mode was used")
				.asOptional())
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"({"moduleId": "Interface", "id": "GainKnob", "value": 6.0})")
			.withResponseExample(R"({"success": true, "moduleId": "Interface", "id": "GainKnob", "type": "ScriptSlider", "logs": [], "errors": []})"));
	}

	/* POST /api/set_component_properties */
	static void setComponentProperties(Array<RouteMetadata>& m)
	{
		auto changeEntry = RouteParameter(Identifier("entry"), "Change entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::id, "Component ID"))
			.withProperty(RouteParameter(RestApiIds::properties, "Property key/value pairs to set")
				.withType(ParamType::Object));

		auto appliedEntry = RouteParameter(Identifier("entry"), "Applied change entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::id, "Component ID"))
			.withProperty(RouteParameter(RestApiIds::properties, "Array of property names that were set")
				.withType(ParamType::Array));

		m.add(RouteMetadata(ApiRoute::SetComponentProperties, "api/set_component_properties")
			.withMethod(RestServer::POST)
			.withCategory("ui")
			.withSummary("Set properties on one or more UI components")
			.withDescription("Set properties on UI components, similar to editing in the Interface Designer. "
				"Properties set in script (e.g. via component.set()) are locked and cannot be changed "
				"without force=true. When parentComponent is changed, recompileRequired=true is returned "
				"and you must call POST /api/recompile to apply hierarchy changes.")
			.withReturns("Applied changes with recompileRequired flag, or error with locked properties list")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID")
				.withExample("Interface"))
			.withBodyParam(RouteParameter(RestApiIds::changes, "Array of {id, properties} objects")
				.withArrayItems(changeEntry))
			.withBodyParam(RouteParameter(RestApiIds::force,
				"If true, bypasses script-lock check and sets all properties")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::applied, "Array of applied change entries")
				.withArrayItems(appliedEntry))
			.withResponseField(RouteParameter(RestApiIds::recompileRequired,
				"True if parentComponent was changed - call /api/recompile to apply")
				.withType(ParamType::Bool))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"({"moduleId": "Interface", "changes": [{"id": "Knob1", "properties": {"x": 100, "y": 50}}]})")
			.withResponseExample(R"({"success": true, "moduleId": "Interface", "applied": [{"id": "Knob1", "properties": ["x", "y"]}], "recompileRequired": false, "logs": [], "errors": []})"));
	}

	/* GET /api/testing/screenshot */
	static void testingScreenshot(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::TestingScreenshot, "api/testing/screenshot")
			.withCategory("testing")
			.withSummary("Capture screenshot of interface or specific component")
			.withDescription("Captures a PNG screenshot of the full interface or a specific component. "
				"When capturing a specific component, the screenshot is cropped to that component's "
				"bounds within the full interface render. Returns Base64 by default, or writes to "
				"file when outputPath is specified.")
			.withReturns("Base64-encoded PNG image data with dimensions, or file path if outputPath specified")
			.withQueryParam(RouteParameter(RestApiIds::moduleId, "Script processor ID")
				.withDefault("Interface"))
			.withQueryParam(RouteParameter(RestApiIds::id,
				"Component ID to capture (omit for full interface)").asOptional())
			.withQueryParam(RouteParameter(RestApiIds::scale, "Scale factor (0.5 or 1.0)")
				.withType(ParamType::Float).withDefault("1.0"))
			.withQueryParam(RouteParameter(RestApiIds::outputPath,
				"File path to save PNG (must end with .png). Writes to file instead of returning Base64").asOptional())
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::id, "The component's ID (when specified)")
				.asOptional())
			.withResponseField(RouteParameter(RestApiIds::width, "Width of captured image in pixels")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::height, "Height of captured image in pixels")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::scale, "Scale factor that was applied")
				.withType(ParamType::Float))
			.withResponseField(RouteParameter(RestApiIds::imageData,
				"Base64-encoded PNG data (only when outputPath not specified)").asOptional())
			.withResponseField(RouteParameter(RestApiIds::filePath,
				"Full path to saved PNG file (only when outputPath specified)").asOptional())
			.withErrorCodes({ 400, 404, 500 })
			.withRequestExample(R"(GET /api/testing/screenshot?moduleId=Interface&scale=0.5)")
			.withResponseExample(R"({"success": true, "moduleId": "Interface", "width": 600, "height": 500, "scale": 1.0, "imageData": "iVBORw0KGgo...", "logs": [], "errors": []})"));
	}

	/* GET /api/get_selected_components */
	static void getSelectedComponents(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::GetSelectedComponents, "api/get_selected_components")
			.withCategory("ui")
			.withSummary("Get the currently selected UI components from the Interface Designer")
			.withDescription("Returns all currently selected components with full property details. "
				"Components are returned in selection order (the order the user clicked them). "
				"An empty selection returns success with an empty components array. "
				"Selection is cleared when the script is recompiled.")
			.withReturns("Selection count and array of selected components with all properties")
			.withQueryParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID")
				.withDefault("Interface"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::selectionCount, "Number of currently selected components")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::components, "Array of selected components with properties")
				.withType(ParamType::Array))
			.withErrorCodes({ 404 })
			.withRequestExample(R"(GET /api/get_selected_components?moduleId=Interface)")
			.withResponseExample(R"({"success": true, "moduleId": "Interface", "selectionCount": 1, "components": [{"id": "Button1", "type": "ScriptButton", "properties": [{"id": "x", "value": 162, "isDefault": true}]}], "logs": [], "errors": []})"));
	}

	/* POST /api/testing/e2e */
	static void testingE2e(Array<RouteMetadata>& m)
	{
		auto interactionItem = RouteParameter(Identifier("interaction"), "Interaction object")
			.withType(ParamType::Object)
			.withDiscriminator("type")
			.withVariant("moveTo", "Explicit mouse positioning to a component")
			.withVariant("click", "Click at current or target position")
			.withVariant("doubleClick", "Double-click (expands to two clicks)")
			.withVariant("drag", "Drag using pixel delta from current position")
			.withVariant("selectMenuItem", "Click a menu item by text")
			.withVariant("screenshot", "Capture interface screenshot")
			.withProperty(RouteParameter(RestApiIds::type, "Interaction type")
				.withEnumValues({ "moveTo", "click", "doubleClick", "drag", "selectMenuItem", "screenshot" }))
			.withProperty(RouteParameter(RestApiIds::target, "Component ID to interact with").asOptional())
			.withProperty(RouteParameter(Identifier("delay"), "Delay in ms before action")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(Identifier("normalizedPosition"), "Position within target (0-1, default center)")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(Identifier("pixelPosition"), "Absolute pixel position (takes precedence over normalized)")
				.withType(ParamType::Object).asOptional())
			.withProperty(RouteParameter(Identifier("delta"), "Pixel offset for drag {x, y}")
				.withType(ParamType::Object).asOptional())
			.withProperty(RouteParameter(Identifier("subtarget"), "Optional sub-component ID").asOptional())
			.withProperty(RouteParameter(Identifier("rightClick"), "Use right mouse button")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(Identifier("shiftDown"), "Hold shift modifier")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(Identifier("ctrlDown"), "Hold ctrl modifier")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(Identifier("altDown"), "Hold alt modifier")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(Identifier("cmdDown"), "Hold cmd modifier")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(Identifier("menuItemText"), "Menu item text for selectMenuItem").asOptional())
			.withProperty(RouteParameter(RestApiIds::id, "Screenshot ID for screenshot type").asOptional())
			.withProperty(RouteParameter(RestApiIds::scale, "Scale factor for screenshot type")
				.withType(ParamType::Float).asOptional());

		m.add(RouteMetadata(ApiRoute::TestingE2e, "api/testing/e2e")
			.withMethod(RestServer::POST)
			.withCategory("testing")
			.withSummary("Execute a sequence of UI interactions in a test window")
			.withDescription("Execute mouse movements, clicks, drags, menu selections, and screenshots "
				"in a dedicated test window. Auto-inserts moveTo events as needed for proper mouse "
				"positioning. Mouse state persists across API calls. Blocks until all interactions "
				"complete (30s timeout).")
			.withReturns("Completion count, timing, execution log, captured screenshots, and optional mouseState")
			.withBodyParam(RouteParameter(RestApiIds::interactions, "Array of interaction objects")
				.withArrayItems(interactionItem))
			.withBodyParam(RouteParameter(RestApiIds::verbose,
				"If true, include auto-insertion details and final mouseState in response")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::interactionsCompleted, "Number of interactions executed")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::totalElapsedMs, "Total execution time in milliseconds")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::executionLog, "Array of executed events with timing")
				.withType(ParamType::Array))
			.withResponseField(RouteParameter(RestApiIds::screenshots, "Object with screenshot id -> metadata")
				.withType(ParamType::Object))
			.withResponseField(RouteParameter(RestApiIds::mouseState,
				"Final mouse state object (only when verbose=true)")
				.withType(ParamType::Object).asOptional())
			.withResponseField(RouteParameter(RestApiIds::parseWarnings, "Array of parse warnings")
				.withType(ParamType::Array).asOptional())
			.withResponseField(RouteParameter(RestApiIds::selectedMenuItem, "Selected menu item info")
				.withType(ParamType::Object).asOptional())
			.withErrorCodes({ 400, 500, 503 })
			.withRequestExample(R"({"interactions": [{"type": "click", "target": "Button1"}, {"type": "screenshot", "id": "after_click"}]})")
			.withResponseExample(R"({"success": true, "interactionsCompleted": 2, "totalElapsedMs": 120, "executionLog": [], "screenshots": {"after_click": {"sizeKB": 12.5, "width": 600, "height": 400}}, "logs": [], "errors": []})"));
	}

	/* POST /api/diagnose_script */
	static void diagnoseScript(Array<RouteMetadata>& m)
	{
		auto diagnosticEntry = RouteParameter(Identifier("entry"), "Diagnostic entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::line, "1-based line number")
				.withType(ParamType::Int))
			.withProperty(RouteParameter(RestApiIds::column, "1-based column number")
				.withType(ParamType::Int))
			.withProperty(RouteParameter(RestApiIds::severity, "Diagnostic severity")
				.withEnumValues({ "error", "warning", "info", "hint" }))
			.withProperty(RouteParameter(RestApiIds::source, "Diagnostic category")
				.withEnumValues({ "syntax", "api-validation", "type-check", "language", "callscope" }))
			.withProperty(RouteParameter(RestApiIds::message, "Human-readable diagnostic message"))
			.withProperty(RouteParameter(RestApiIds::suggestions, "Did-you-mean suggestions from fuzzy matching")
				.withType(ParamType::Array).asOptional());

		m.add(RouteMetadata(ApiRoute::DiagnoseScript, "api/diagnose_script")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withSummary("Run diagnostic-only shadow parse on a script file")
			.withDescription("Returns structured diagnostics (API hallucinations, type mismatches, "
				"language rule violations, audio-thread safety warnings) without modifying runtime "
				"state - no recompilation, no execution. Requires at least one prior successful "
				"compile. Always reads the file from disk - save pending edits before calling.")
			.withReturns("Array of diagnostics with line, column, severity, source, message, and suggestions")
			.withBodyParam(RouteParameter(RestApiIds::moduleId,
				"The script processor's module ID. Required if filePath is not provided.").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::filePath,
				"Path to the external .js file (absolute or relative to Scripts folder). "
				"Required if moduleId is not provided. When used alone, HISE resolves the owning processor.").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::async,
				"If true, defer the shadow parse to the scripting thread (slower, blocks audio). "
				"Default is false: runs directly on the HTTP thread with a read lock.")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The resolved script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::filePath, "The resolved file path"))
			.withResponseField(RouteParameter(RestApiIds::diagnostics, "Array of diagnostic entries")
				.withArrayItems(diagnosticEntry))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"({"moduleId": "Interface", "filePath": "Scripts/UI.js"})")
			.withResponseExample("{\"success\": true, \"moduleId\": \"Interface\", \"filePath\": \"D:/Projects/Scripts/UI.js\", \"diagnostics\": [{\"line\": 15, \"column\": 5, \"severity\": \"error\", \"source\": \"api-validation\", \"message\": \"Function / constant not found: Console.prnt (did you mean: print?)\", \"suggestions\": [\"print\"]}], \"logs\": [], \"errors\": []}"));
	}

	/* GET /api/get_included_files */
	static void getIncludedFiles(Array<RouteMetadata>& m)
	{
		auto fileEntry = RouteParameter(Identifier("entry"), "Included file entry (global mode)")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::path, "Full file path"))
			.withProperty(RouteParameter(RestApiIds::processor, "Owning processor ID"));

		m.add(RouteMetadata(ApiRoute::GetIncludedFiles, "api/get_included_files")
			.withCategory("scripting")
			.withSummary("List all included external script files")
			.withDescription("Without moduleId, returns all files across all processors with owning "
				"processor names (objects with path and processor fields). With moduleId, returns "
				"files for that processor only (flat string array of full paths). "
				"Files are registered after a successful compile.")
			.withReturns("Array of included files as full paths, optionally with owning processor ID")
			.withQueryParam(RouteParameter(RestApiIds::moduleId,
				"Filter by script processor. If omitted, returns files from all processors with processor names.").asOptional())
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The filtered processor ID (when moduleId was specified)")
				.asOptional())
			.withResponseField(RouteParameter(RestApiIds::files, "Array of file entries or path strings")
				.withArrayItems(fileEntry))
			.withErrorCodes({ 404 })
			.withRequestExample(R"(GET /api/get_included_files)")
			.withResponseExample(R"({"success": true, "files": [{"path": "D:/Projects/Scripts/utils.js", "processor": "Interface"}], "logs": [], "errors": []})"));
	}

	/* POST /api/testing/profile */
	static void testingProfile(Array<RouteMetadata>& m)
	{
		auto eventEntry = RouteParameter(Identifier("entry"), "Profiled event entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::name, "Event name"))
			.withProperty(RouteParameter(RestApiIds::duration, "Duration in ms")
				.withType(ParamType::Float))
			.withProperty(RouteParameter(RestApiIds::sourceType, "Event source type")
				.withExample("DSP"))
			.withProperty(RouteParameter(RestApiIds::start, "Start time in ms")
				.withType(ParamType::Float))
			.withProperty(RouteParameter(RestApiIds::children, "Nested child events")
				.withType(ParamType::Array).asOptional());

		auto threadEntry = RouteParameter(Identifier("entry"), "Thread profiling result")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::thread, "Thread name"))
			.withProperty(RouteParameter(RestApiIds::events, "Array of profiled events")
				.withArrayItems(eventEntry));

		m.add(RouteMetadata(ApiRoute::TestingProfile, "api/testing/profile")
			.withMethod(RestServer::POST)
			.withCategory("testing")
			.withSummary("Start a profiling session or retrieve last result")
			.withDescription("mode=\"record\" starts a new non-blocking session (returns immediately). "
				"mode=\"get\" returns last result with optional filtering and summary aggregation. "
				"Workflow: record once, then query with different filters. "
				"Returns 409 if a recording is already in progress. "
				"Returns 400 if profiling toolkit is not enabled in the build.")
			.withReturns("Full tree (threads + flows), filtered results, or recording status")
			.withBodyParam(RouteParameter(RestApiIds::mode, "Operation mode")
				.withEnumValues({ "record", "get" }).withDefault("record"))
			.withBodyParam(RouteParameter(RestApiIds::durationMs,
				"Recording duration in milliseconds (100-5000). Used in record mode.")
				.withType(ParamType::Int).withDefault("1000"))
			.withBodyParam(RouteParameter(RestApiIds::threadFilter,
				"Array of thread names to include (default: all). "
				"Valid: Audio Thread, Scripting Thread, UI Thread, Loading Thread, etc.")
				.withType(ParamType::Array).asOptional())
			.withBodyParam(RouteParameter(RestApiIds::eventFilter,
				"Array of event source types to record (default: all). Used in record mode. "
				"Valid: DSP, Script, Lock, Callback, Trace, TimerCallback, Scriptnode, etc.")
				.withType(ParamType::Array).asOptional())
			.withBodyParam(RouteParameter(RestApiIds::summary,
				"If true, aggregate repeated events with count/median/peak/min/total stats. Used in get mode.")
				.withType(ParamType::Bool).withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::filter,
				"Wildcard pattern matched against event name (e.g. \"slow*\"). Case-insensitive. Used in get mode.")
				.asOptional())
			.withBodyParam(RouteParameter(RestApiIds::minDuration,
				"Only include events with duration >= this value in ms. Used in get mode.")
				.withType(ParamType::Float).asOptional())
			.withBodyParam(RouteParameter(RestApiIds::sourceTypeFilter,
				"Wildcard pattern matched against sourceType (e.g. \"Trace\"). Case-insensitive. Used in get mode.")
				.asOptional())
			.withBodyParam(RouteParameter(RestApiIds::nested,
				"When filtering, include children of matched events. Used in get mode.")
				.withType(ParamType::Bool).withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::limit,
				"Max number of results in filtered/summary mode (1-100). Used in get mode.")
				.withType(ParamType::Int).withDefault("15"))
			.withBodyParam(RouteParameter(RestApiIds::wait,
				"If false, return immediately when recording is in progress instead of blocking. Used in get mode.")
				.withType(ParamType::Bool).withDefault("true"))
			.withResponseField(RouteParameter(RestApiIds::recording, "Whether a recording is in progress")
				.withType(ParamType::Bool).asOptional())
			.withResponseField(RouteParameter(RestApiIds::durationMs, "Recording duration in ms (record mode)")
				.withType(ParamType::Int).asOptional())
			.withResponseField(RouteParameter(RestApiIds::message, "Status message (e.g. when no data)")
				.asOptional())
			.withResponseField(RouteParameter(RestApiIds::threads, "Array of thread profiling results")
				.withArrayItems(threadEntry).asOptional())
			.withResponseField(RouteParameter(RestApiIds::flows, "Array of cross-thread causal flow connections")
				.withType(ParamType::Array).asOptional())
			.withResponseField(RouteParameter(RestApiIds::results, "Flat array of filtered/aggregated events")
				.withType(ParamType::Array).asOptional())
			.withErrorCodes({ 400, 409 })
			.withRequestExample(R"({"mode": "record", "durationMs": 2000})")
			.withResponseExample(R"({"success": true, "recording": true, "durationMs": 2000, "logs": [], "errors": []})"));
	}

	/* POST /api/parse_css */
	static void parseCss(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ParseCSS, "api/parse_css")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withSummary("Parse CSS code and return structured diagnostics")
			.withDescription("Accepts either inline CSS code or a file path to a .css file. "
				"Returns diagnostics with line/column/severity/message. Optionally resolves "
				"properties for a set of selectors using CSS specificity rules.")
			.withReturns("Diagnostics array, list of parsed selectors, and resolved properties when selectors provided")
			.withBodyParam(RouteParameter(RestApiIds::code,
				"The CSS code to parse (provide this or filePath)").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::filePath,
				"Path to a .css file. Relative paths resolve against the Scripts/ directory "
				"(provide this or code)").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::selectors,
				"Array of selector strings representing a component's selectors "
				"(e.g. [\"button\", \".my-class\", \"#MyId\"]). Resolves properties using CSS specificity")
				.withType(ParamType::Array).asOptional())
			.withBodyParam(RouteParameter(RestApiIds::width,
				"Reference width in pixels for resolving percentage and relative units")
				.withType(ParamType::Int).asOptional())
			.withBodyParam(RouteParameter(RestApiIds::height,
				"Reference height in pixels for resolving percentage and relative units")
				.withType(ParamType::Int).asOptional())
			.withResponseField(RouteParameter(RestApiIds::diagnostics, "Array of diagnostic entries")
				.withType(ParamType::Array))
			.withResponseField(RouteParameter(RestApiIds::filePath, "The resolved CSS file path (when file was used)")
				.asOptional())
			.withResponseField(RouteParameter(RestApiIds::selectors, "List of parsed selectors")
				.withType(ParamType::Array).asOptional())
			.withResponseField(RouteParameter(RestApiIds::properties, "Resolved properties when selectors provided")
				.withType(ParamType::Object).asOptional())
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"({"code": ".myClass { background: red; padding: 10px; }", "selectors": [".myClass"]})")
			.withResponseExample(R"({"success": true, "diagnostics": [], "selectors": [".myClass"], "properties": {"background": "red", "padding": "10px"}, "logs": [], "errors": []})"));
	}

	/* POST /api/shutdown */
	static void shutdown(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::Shutdown, "api/shutdown")
			.withMethod(RestServer::POST)
			.withCategory("status")
			.withSummary("Gracefully quit the HISE application")
			.withDescription("Schedules an asynchronous shutdown via JUCEApplication::quit(). "
				"The HTTP response is sent before the application exits.")
			.withReturns("Success confirmation before shutdown begins")
			.withResponseExample(R"({"success": true, "result": "Shutdown initiated", "logs": [], "errors": []})"));
	}

	/* GET /api/builder/tree */
	static void builderTree(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::BuilderTree, "api/builder/tree")
			.withMethod(RestServer::GET)
			.withCategory("builder")
			.withSummary("Get the runtime module tree hierarchy")
			.withDescription("Returns the nested JSON module tree with metadata, modulation, and children. "
				"Without group parameter, returns the live runtime tree. "
				"With group=current, returns the active validation tree (400 when no active group). "
				"Other group values return 501.")
			.withReturns("Nested JSON tree with metadata / modulation / children")
			.withQueryParam(RouteParameter(RestApiIds::moduleId,
				"Optional root module ID to return a subtree").asOptional())
			.withQueryParam(RouteParameter(RestApiIds::group,
				"Optional group selector. Only 'current' is supported").asOptional())
			.withQueryParam(RouteParameter(RestApiIds::queryParameters,
				"If false, omit parameter lists from the tree")
				.withType(ParamType::Bool).withDefault("true"))
			.withQueryParam(RouteParameter(RestApiIds::verbose,
				"If true, include verbose metadata in tree nodes")
				.withType(ParamType::Bool).withDefault("false"))
			.withErrorCodes({ 400, 404, 501 })
			.withRequestExample(R"(GET /api/builder/tree)")
			.withResponseExample(R"({"success": true, "result": {"id": "SynthChain", "processorId": "Master Chain", "type": "SoundGenerator", "bypassed": false}, "logs": [], "errors": []})"));
	}

	/* /api/builder/apply */
	static void applyBuilder(Array<RouteMetadata>& m)
	{
		auto opItem = RouteParameter(RestApiIds::op, "Operation object")
			.withType(ParamType::Object)
			.withDiscriminator("op")
			.withVariant("add", "Add a new module (type, parent, chain, name)")
			.withVariant("remove", "Remove a module (target)")
			.withVariant("clone", "Clone a module (source, count, template?)")
			.withVariant("set_attributes", "Set parameter values (target, attributes, mode?)")
			.withVariant("set_id", "Rename a module (target, name)")
			.withVariant("set_bypassed", "Set bypass state (target, bypassed)")
			.withVariant("set_effect", "Set effect/network (target, effect)")
			.withProperty(RouteParameter(RestApiIds::op, "Operation type")
				.withEnumValues({ "add","remove","clone","set_attributes","set_id","set_bypassed","set_effect" }))
			.withProperty(RouteParameter(RestApiIds::type, "Module type ID").asOptional())
			.withProperty(RouteParameter(RestApiIds::parent, "Parent module name").asOptional())
			.withProperty(RouteParameter(RestApiIds::chain, "Chain index (-1=direct, 0=midi, 1=gain, 2=pitch, 3=fx)")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::name, "Module name").asOptional())
			.withProperty(RouteParameter(RestApiIds::target, "Target module name").asOptional())
			.withProperty(RouteParameter(RestApiIds::source, "Source module to clone").asOptional())
			.withProperty(RouteParameter(RestApiIds::count, "Number of clones")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(Identifier("template"), "Name template with {n} placeholder").asOptional())
			.withProperty(RouteParameter(RestApiIds::attributes, "Parameter values {paramName: value}")
				.withType(ParamType::Object).asOptional())
			.withProperty(RouteParameter(RestApiIds::mode, "Value interpretation")
				.withEnumValues({ "value", "normalized", "raw" }).withDefault("value"))
			.withProperty(RouteParameter(RestApiIds::bypassed, "Bypass state")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(RestApiIds::effect, "Effect/network name").asOptional());

		auto diffEntry = RouteParameter(Identifier("entry"), "Diff entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::target, "Module ID affected"))
			.withProperty(RouteParameter(RestApiIds::action, "Net action symbol")
				.withEnumValues({ "+", "-", "*" }))
			.withProperty(RouteParameter(RestApiIds::domain, "Operation domain")
				.withExample("builder"));

		m.add(RouteMetadata(ApiRoute::BuilderApply, "api/builder/apply")
			.withMethod(RestServer::POST)
			.withCategory("builder")
			.withSummary("Apply a set of operations to the module tree")
			.withDescription("Apply one or more operations to the module tree in a single batch. "
				"When called inside an undo group (after push_group), the diff accumulates all operations in the group so far. "
				"The response contains a diff summary with the net effect - if a module was added and then modified, it shows as '+'. "
				"Operations are pre-validated before execution; invalid operations return 400 with detailed error info. "
				"Partial failure rolls back the entire batch.")
			.withReturns("Diff summary showing the net effect of all operations")
			.withBodyParam(RouteParameter(RestApiIds::operations, "Non-empty array of operation objects")
				.withArrayItems(opItem))
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withArrayItems(diffEntry))
			.withErrorCodes({ 400, 404, 409 })
			.withRequestExample(R"({"operations": [{"op": "add", "type": "SineSynth", "parent": "Master Chain", "chain": -1, "name": "MySine"}, {"op": "set_bypassed", "target": "MySine", "bypassed": true}]})")
			.withResponseExample(R"({"success": true, "scope": "group", "groupName": "root", "diff": [{"target": "MySine", "action": "+", "domain": "builder"}], "logs": [], "errors": []})"));
	}

	/* POST /api/builder/reset */
	static void builderReset(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::BuilderReset, "api/builder/reset")
			.withMethod(RestServer::POST)
			.withCategory("builder")
			.withSummary("Reset the module tree to an empty state")
			.withDescription("Equivalent to File -> New. Resets the module tree to its initial "
				"empty state and clears all undo history.")
			.withReturns("Success confirmation")
			.withResponseExample(R"({"success": true, "result": "Module tree reset", "logs": [], "errors": []})"));
	}

	/* POST /api/undo/push_group */
	static void undoPushGroup(Array<RouteMetadata>& m)
	{
		auto diffEntry = RouteParameter(Identifier("entry"), "Diff entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::target, "Affected ID"))
			.withProperty(RouteParameter(RestApiIds::action, "Net action symbol")
				.withEnumValues({ "+", "-", "*" }))
			.withProperty(RouteParameter(RestApiIds::domain, "Operation domain"));

		m.add(RouteMetadata(ApiRoute::UndoPushGroup, "api/undo/push_group")
			.withMethod(RestServer::POST)
			.withCategory("undo")
			.withSummary("Start a new undo group")
			.withDescription("Start a new undo group. Subsequent operations are validated against "
				"group state and recorded. Actions are not executed until pop_group.")
			.withReturns("Current diff state for the new group")
			.withBodyParam(RouteParameter(RestApiIds::name, "Label for the group")
				.withExample("Add synth and configure"))
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withArrayItems(diffEntry))
			.withErrorCodes({ 400 })
			.withRequestExample(R"({"name": "Add synth and configure"})")
			.withResponseExample(R"({"success": true, "scope": "group", "groupName": "Add synth and configure", "diff": [], "logs": [], "errors": []})"));
	}

	/* POST /api/undo/pop_group */
	static void undoPopGroup(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::UndoPopGroup, "api/undo/pop_group")
			.withMethod(RestServer::POST)
			.withCategory("undo")
			.withSummary("End the current undo group")
			.withDescription("End the current undo group. With cancel=false (default), executes all "
				"queued actions as one undoable batch. With cancel=true, discards all queued actions.")
			.withReturns("Updated diff state after group is applied or discarded")
			.withBodyParam(RouteParameter(RestApiIds::cancel,
				"If true, discard the group without executing")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withType(ParamType::Array))
			.withErrorCodes({ 400 })
			.withRequestExample(R"({"cancel": false})")
			.withResponseExample(R"({"success": true, "scope": "group", "groupName": "root", "diff": [{"target": "MySine", "action": "+", "domain": "builder"}], "logs": [], "errors": []})"));
	}

	/* POST /api/undo/back */
	static void undoBack(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::UndoBack, "api/undo/back")
			.withMethod(RestServer::POST)
			.withCategory("undo")
			.withSummary("Undo the last action or group")
			.withDescription("Undo the last action or group. Stops at group boundaries. "
				"Returns 400 when there is nothing to undo.")
			.withReturns("Updated diff state after undo")
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withType(ParamType::Array))
			.withErrorCodes({ 400 })
			.withResponseExample(R"({"success": true, "scope": "group", "groupName": "root", "diff": [], "logs": [], "errors": []})"));
	}

	/* POST /api/undo/forward */
	static void undoForward(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::UndoForward, "api/undo/forward")
			.withMethod(RestServer::POST)
			.withCategory("undo")
			.withSummary("Redo the next action or group")
			.withDescription("Redo the next action or group. Stops at group boundaries. "
				"Returns 400 when there is nothing to redo.")
			.withReturns("Updated diff state after redo")
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withType(ParamType::Array))
			.withErrorCodes({ 400 })
			.withResponseExample(R"({"success": true, "scope": "group", "groupName": "root", "diff": [{"target": "MySine", "action": "+", "domain": "builder"}], "logs": [], "errors": []})"));
	}

	/* GET /api/undo/diff */
	static void undoDiff(Array<RouteMetadata>& m)
	{
		auto diffEntry = RouteParameter(Identifier("entry"), "Diff entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::target, "Affected ID"))
			.withProperty(RouteParameter(RestApiIds::action, "Net action symbol")
				.withEnumValues({ "+", "-", "*" }))
			.withProperty(RouteParameter(RestApiIds::domain, "Operation domain"));

		m.add(RouteMetadata(ApiRoute::UndoDiff, "api/undo/diff")
			.withCategory("undo")
			.withSummary("Get the current diff state showing active changes")
			.withDescription("Returns the current diff state. With scope=group (default), shows "
				"changes in the current group only. With scope=root, shows the full stack. "
				"Use flatten=true to compute the net effect by merging cancelling actions.")
			.withReturns("Diff array with scope, depth, groupName, and action entries")
			.withQueryParam(RouteParameter(RestApiIds::scope,
				"group = current group only, root = full stack")
				.withEnumValues({ "group", "root" }).withDefault("group"))
			.withQueryParam(RouteParameter(RestApiIds::domain,
				"Filter by domain (e.g. builder, ui)").asOptional())
			.withQueryParam(RouteParameter(RestApiIds::flatten,
				"If true, compute net effect merging cancelling actions")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::scope, "Active scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withArrayItems(diffEntry))
			.withErrorCodes({ 400 })
			.withRequestExample(R"(GET /api/undo/diff?scope=group&flatten=true)")
			.withResponseExample(R"({"success": true, "scope": "group", "depth": 0, "groupName": "root", "diff": [{"target": "MySine", "action": "+", "domain": "builder"}], "logs": [], "errors": []})"));
	}

	/* GET /api/undo/history */
	static void undoHistory(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::UndoHistory, "api/undo/history")
			.withCategory("undo")
			.withSummary("Get the full undo history with cursor position")
			.withDescription("Returns the full undo history including redo buffer and cursor position. "
				"With scope=group, shows the current group only. With scope=root, shows the full "
				"stack with nested groups. Use flatten=true to flatten group nesting.")
			.withReturns("History array with cursor position, group nesting, and action entries")
			.withQueryParam(RouteParameter(RestApiIds::scope,
				"group = current group only, root = full stack with nested groups")
				.withEnumValues({ "group", "root" }).withDefault("group"))
			.withQueryParam(RouteParameter(RestApiIds::domain,
				"Filter by domain (e.g. builder, ui)").asOptional())
			.withQueryParam(RouteParameter(RestApiIds::flatten,
				"If true, flatten group nesting to a single list")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::cursor, "Current undo position")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::history, "History entries array")
				.withType(ParamType::Array))
			.withErrorCodes({ 400 })
			.withRequestExample(R"(GET /api/undo/history?scope=root)")
			.withResponseExample(R"({"success": true, "cursor": 2, "history": [], "logs": [], "errors": []})"));
	}

	/* POST /api/undo/clear */
	static void undoClear(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::UndoClear, "api/undo/clear")
			.withMethod(RestServer::POST)
			.withCategory("undo")
			.withSummary("Clear the entire undo history and exit all groups")
			.withDescription("Clears the entire undo history and exits all active groups. "
				"Returns an empty diff state.")
			.withReturns("Empty diff state")
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withType(ParamType::Array))
			.withResponseExample(R"({"success": true, "scope": "group", "groupName": "root", "diff": [], "logs": [], "errors": []})"));
	}

	/* GET /api/wizard/initialise */
	static void wizardInitialise(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::WizardInitialise, "api/wizard/initialise")
			.rejectsInSnippetBrowser()
			.withMethod(RestServer::GET)
			.withCategory("wizard")
			.withSummary("Fetch pre-populated field defaults for a wizard form")
			.withDescription("Returns a flat key/value object of field defaults for the specified "
				"wizard. Returns error if the wizard ID is unknown.")
			.withReturns("Flat key/value object of field defaults, or error if wizard ID unknown")
			.withQueryParam(RouteParameter(RestApiIds::id,
				"Wizard ID string")
				.withEnumValues({ "new_project", "recompile", "plugin_export", "compile_networks", "audio_export", "install_package_maker" }))
			.withErrorCodes({ 400 })
			.withRequestExample(R"(GET /api/wizard/initialise?id=new_project)")
			.withResponseExample(R"({"success": true, "projectName": "MyPlugin", "projectPath": "D:/Projects", "logs": [], "errors": []})"));
	}

	/* POST /api/wizard/execute */
	static void wizardExecute(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::WizardExecute, "api/wizard/execute")
			.rejectsInSnippetBrowser()
			.withMethod(RestServer::POST)
			.withCategory("wizard")
			.withSummary("Execute a wizard task")
			.withDescription("Execute a wizard task. Synchronous tasks return immediately with a "
				"result string. Asynchronous tasks return a jobId for polling via /api/wizard/status.")
			.withReturns("Sync: result string with logs. Async: {jobId, async: true}")
			.withBodyParam(RouteParameter(RestApiIds::wizardId, "Wizard ID string")
				.withExample("new_project"))
			.withBodyParam(RouteParameter(RestApiIds::answers, "Key/value object of all form field answers")
				.withType(ParamType::Object))
			.withBodyParam(RouteParameter(RestApiIds::tasks, "Array with exactly one task function name to execute")
				.withType(ParamType::Array))
			.withResponseField(RouteParameter(RestApiIds::jobId,
				"Job ID for async tasks (only present when async)").asOptional())
			.withErrorCodes({ 400 })
			.withRequestExample(R"({"wizardId": "new_project", "answers": {"projectName": "MyPlugin"}, "tasks": ["createProject"]})")
			.withResponseExample(R"({"success": true, "result": "Project created", "logs": [], "errors": []})"));
	}

	/* GET /api/wizard/status */
	static void wizardStatus(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::WizardStatus, "api/wizard/status")
			.rejectsInSnippetBrowser()
			.withMethod(RestServer::GET)
			.withCategory("wizard")
			.withSummary("Poll progress of a long-running async wizard job")
			.withDescription("Returns the current status of an asynchronous wizard job. "
				"finished=true when the job is done.")
			.withReturns("Job status with finished flag and progress value")
			.withQueryParam(RouteParameter(RestApiIds::jobId, "Job ID returned by wizard/execute"))
			.withResponseField(RouteParameter(RestApiIds::finished, "Whether the job has completed")
				.withType(ParamType::Bool))
			.withResponseField(RouteParameter(RestApiIds::progress, "Progress value 0.0-1.0")
				.withType(ParamType::Float))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"(GET /api/wizard/status?jobId=abc123)")
			.withResponseExample(R"({"success": true, "finished": false, "progress": 0.5, "logs": [], "errors": []})"));
	}

	/* GET /api/ui/tree */
	static void uiTree(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::UITree, "api/ui/tree")
			.withCategory("ui")
			.withSummary("Get UI component tree hierarchy for a script processor")
			.withDescription("Returns a recursive tree with id, type, visible, enabled, saveInPreset, "
				"x, y, width, height, and childComponents for each node. With group=current, "
				"returns the active undo group's validation tree. Returns 501 for unsupported "
				"group values, 400 when no current group.")
			.withReturns("Recursive tree with component properties and nested childComponents")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::group,
				"Optional group selector. 'current' returns the active plan's validation tree").asOptional())
			.withErrorCodes({ 400, 404, 501 })
			.withRequestExample(R"(GET /api/ui/tree?moduleId=Interface)")
			.withResponseExample(R"({"success": true, "result": {"id": "Content", "type": "ScriptPanel", "x": 0, "y": 0, "width": 600, "height": 500, "visible": true, "enabled": true, "saveInPreset": false, "childComponents": []}, "logs": [], "errors": []})"));
	}

	/* POST /api/ui/apply */
	static void uiApply(Array<RouteMetadata>& m)
	{
		auto opItem = RouteParameter(RestApiIds::op, "UI operation object")
			.withType(ParamType::Object)
			.withDiscriminator("op")
			.withVariant("add", "Add a new component (componentType, id?, parentId?, x?, y?, width?, height?)")
			.withVariant("remove", "Remove a component (target)")
			.withVariant("set", "Set properties on a component (target, properties, force?)")
			.withVariant("move", "Move a component to a new parent (target, parent, index?, keepPosition?)")
			.withVariant("rename", "Rename a component (target, newId)")
			.withProperty(RouteParameter(RestApiIds::op, "Operation type")
				.withEnumValues({ "add", "remove", "set", "move", "rename" }))
			.withProperty(RouteParameter(RestApiIds::componentType, "Component type for add op")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::id, "Component ID for add op").asOptional())
			.withProperty(RouteParameter(RestApiIds::parentId, "Parent component ID").asOptional())
			.withProperty(RouteParameter(RestApiIds::target, "Target component ID").asOptional())
			.withProperty(RouteParameter(RestApiIds::x, "X position")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::y, "Y position")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::width, "Width")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::height, "Height")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::properties, "Properties to set for set op")
				.withType(ParamType::Object).asOptional())
			.withProperty(RouteParameter(RestApiIds::force, "Bypass script-lock check for set op")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(RestApiIds::parent, "New parent for move op").asOptional())
			.withProperty(RouteParameter(RestApiIds::index, "Child index for move op")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::keepPosition, "Preserve absolute position when reparenting")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(RestApiIds::newId, "New component ID for rename op").asOptional());

		auto diffEntry = RouteParameter(Identifier("entry"), "Diff entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::target, "Component ID affected"))
			.withProperty(RouteParameter(RestApiIds::action, "Net action symbol")
				.withEnumValues({ "+", "-", "*" }))
			.withProperty(RouteParameter(RestApiIds::domain, "Operation domain")
				.withExample("ui"));

		m.add(RouteMetadata(ApiRoute::UIApply, "api/ui/apply")
			.withMethod(RestServer::POST)
			.withCategory("ui")
			.withSummary("Apply batched UI component operations")
			.withDescription("Apply one or more UI component operations (add, remove, set, move, rename) "
				"in a single batch. Operations are pre-validated before execution; invalid operations "
				"return 400 with detailed error info. All operations are undoable via the undo API. "
				"Partial failure rolls back the entire batch. "
				"Component types: ScriptButton, ScriptSlider, ScriptPanel, ScriptComboBox, ScriptLabel, "
				"ScriptTable, ScriptImage, etc.")
			.withReturns("Diff of changes with domain='ui', action symbols (+/-/*), and target component IDs")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "Script processor ID")
				.withExample("Interface"))
			.withBodyParam(RouteParameter(RestApiIds::operations, "Array of operation objects")
				.withArrayItems(opItem))
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withArrayItems(diffEntry))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"({"moduleId": "Interface", "operations": [{"op": "add", "componentType": "ScriptButton", "id": "MyButton", "x": 10, "y": 10, "width": 100, "height": 30}]})")
			.withResponseExample(R"({"success": true, "scope": "group", "groupName": "root", "diff": [{"target": "MyButton", "action": "+", "domain": "ui"}], "logs": [], "errors": []})"));
	}

	/* POST /api/testing/sequence */
	static void testingSequence(Array<RouteMetadata>& m)
	{
		auto msgItem = RouteParameter(RestApiIds::messages, "MIDI message object")
			.withType(ParamType::Object)
			.withDiscriminator("type")
			.withVariant("note", "MIDI note with auto note-off (noteNumber, velocity?, duration?, channel?)")
			.withVariant("cc", "MIDI CC message (controller, value, channel?)")
			.withVariant("pitchbend", "Pitch bend message (value 0-16383, channel?)")
			.withVariant("allNotesOff", "Panic: clear queue, fire all note-offs, stop test signals")
			.withVariant("repl", "Evaluate HISEScript at timestamp (expression, id?, moduleId?)")
			.withVariant("set_attribute", "Set processor parameter at timestamp (parameterId, value, processorId?)")
			.withVariant("testsignal", "Inject test signal into audio stream (signal, duration?, frequency?)")
			.withProperty(RouteParameter(RestApiIds::type, "Message type")
				.withEnumValues({ "note", "cc", "pitchbend", "allNotesOff", "repl", "set_attribute", "testsignal" }))
			.withProperty(RouteParameter(RestApiIds::timestamp, "Absolute time offset in ms from start of sequence")
				.withType(ParamType::Int).withDefault("0"))
			.withProperty(RouteParameter(RestApiIds::channel, "MIDI channel (1-16)")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::noteNumber, "MIDI note number (0-127)")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::velocity, "Note velocity (0.0-1.0)")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(RestApiIds::duration, "Note duration or signal length in ms")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::controller, "CC controller number (0-127)")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::value, "CC value (0-127), pitch bend (0-16383), or attribute value")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::expression, "HISEScript expression for repl type").asOptional())
			.withProperty(RouteParameter(RestApiIds::id, "Optional tag for repl results").asOptional())
			.withProperty(RouteParameter(RestApiIds::moduleId, "Script processor for repl type")
				.withDefault("Interface"))
			.withProperty(RouteParameter(RestApiIds::processorId, "Target processor for set_attribute")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::parameterId, "Parameter name for set_attribute").asOptional())
			.withProperty(RouteParameter(RestApiIds::signal, "Test signal type")
				.withEnumValues({ "sine", "saw", "sweep", "dirac", "noise", "silence" }).asOptional())
			.withProperty(RouteParameter(RestApiIds::frequency, "Signal frequency in Hz")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(RestApiIds::startFrequency, "Sweep start frequency in Hz")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(RestApiIds::endFrequency, "Sweep end frequency in Hz")
				.withType(ParamType::Float).asOptional());

		auto replResult = RouteParameter(Identifier("entry"), "REPL result entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::id, "Tag from the repl message").asOptional())
			.withProperty(RouteParameter(RestApiIds::expression, "The expression that was evaluated"))
			.withProperty(RouteParameter(RestApiIds::moduleId, "Processor used for evaluation"))
			.withProperty(RouteParameter(RestApiIds::timestamp, "Scheduled timestamp in ms")
				.withType(ParamType::Int))
			.withProperty(RouteParameter(RestApiIds::success, "Whether evaluation succeeded")
				.withType(ParamType::Bool))
			.withProperty(RouteParameter(RestApiIds::value, "Evaluated result value").asOptional());

		m.add(RouteMetadata(ApiRoute::TestingSequence, "api/testing/sequence")
			.withMethod(RestServer::POST)
			.withCategory("testing")
			.withSummary("Inject MIDI messages and test events with precise timing")
			.withDescription("Queue MIDI messages for dispatch via a HighResolutionTimer with sub-ms "
				"precision. Non-blocking by default: returns immediately after queuing. Notes auto-send "
				"note-off after duration. Multiple calls merge into the pending queue. "
				"Send allNotesOff to panic (clears queue). Send empty messages array to poll status. "
				"Use blocking=true or recordOutput to wait for completion.")
			.withReturns("Playback status: isPlaying, durationMs, activeNotes, eventsInSequence, playedEvents, progress")
			.withBodyParam(RouteParameter(RestApiIds::messages, "Array of MIDI message objects")
				.withArrayItems(msgItem))
			.withBodyParam(RouteParameter(RestApiIds::blocking,
				"If true, block until the sequence completes before returning (30s timeout)")
				.withType(ParamType::Bool).withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::recordOutput,
				"File path to record audio output as WAV. Implies blocking mode. "
				"Duration is auto-computed from the sequence.").asOptional())
			.withResponseField(RouteParameter(RestApiIds::isPlaying, "True while sequence is being dispatched or notes are active")
				.withType(ParamType::Bool))
			.withResponseField(RouteParameter(RestApiIds::durationMs, "Total duration of the sequence in ms")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::activeNotes, "Number of notes currently sounding")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::eventsInSequence, "Total logical events in queue")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::playedEvents, "Number of events dispatched so far")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::progress, "Playhead position 0.0-1.0")
				.withType(ParamType::Float))
			.withResponseField(RouteParameter(RestApiIds::replResults, "Array of REPL evaluation results (when available)")
				.withArrayItems(replResult).asOptional())
			.withResponseField(RouteParameter(RestApiIds::recordOutput,
				"File path of recorded WAV (only when recordOutput was specified)").asOptional())
			.withErrorCodes({ 400, 503 })
			.withRequestExample(R"({"messages": [{"type": "note", "noteNumber": 60, "velocity": 0.8, "duration": 1000, "timestamp": 0}, {"type": "note", "noteNumber": 64, "velocity": 0.8, "duration": 1000, "timestamp": 0}]})")
			.withResponseExample(R"({"success": true, "isPlaying": true, "durationMs": 1000, "activeNotes": 2, "eventsInSequence": 2, "playedEvents": 0, "progress": 0.0, "logs": [], "errors": []})"));
	}

	// ========================================================================
	// DSP (scriptnode) endpoints
	// ========================================================================

	static void dspList(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::DspList, "api/dsp/list")
			.withCategory("dsp")
			.withSummary("List available DspNetwork names")
			.withDescription("Scans the project's DspNetworks/ folder for .xml files and "
				"returns their names (without extension). Also includes any in-memory "
				"networks that have been created via dsp/init.")
			.withReturns("Array of network name strings")
			.withResponseField(RouteParameter(RestApiIds::networks, "Array of available network names")
				.withType(ParamType::Array)
				.withArrayItems(RouteParameter(Identifier("name"), "Network name")))
			.withResponseExample(R"({"success": true, "networks": ["MyDSP", "MyEffect", "Reverb"], "logs": [], "errors": []})"));
	}

	static void dspInit(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::DspInit, "api/dsp/init")
			.withMethod(RestServer::POST)
			.withCategory("dsp")
			.withSummary("Create or load a DspNetwork")
			.withDescription("Creates a new DspNetwork or loads an existing one via the module's "
				"DspNetwork::Holder. If the network XML file does not exist, it is created. "
				"If it already exists, it is loaded. Returns the initial tree in the result field "
				"(same shape as GET /api/dsp/tree) plus filePath and embedded flag. "
				"The name is sanitized to a valid C++ identifier.")
			.withReturns("Initial network tree plus filePath and embedded flag")
			.withModuleIdParam()
			.withBodyParam(RouteParameter(RestApiIds::name, "Network name to create or load")
				.withExample("MyDSP"))
			.withBodyParam(RouteParameter(RestApiIds::embedded,
				"Create as embedded network rather than file-based")
				.withType(ParamType::Bool).withDefault("false"))
			.withResponseField(RouteParameter(RestApiIds::filePath, "Absolute path to the network .xml file (empty for embedded networks)"))
			.withResponseField(RouteParameter(RestApiIds::embedded, "Whether the network is embedded")
				.withType(ParamType::Bool))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"({"moduleId": "Script FX1", "name": "MyDSP"})")
			.withResponseExample("{\"success\": true, \"result\": {\"nodeId\": \"MyDSP\", \"factoryPath\": \"container.chain\", \"bypassed\": false, \"parameters\": [], \"connections\": [], \"children\": []}, \"filePath\": \"D:/Projects/MyPlugin/DspNetworks/MyDSP.xml\", \"embedded\": false, \"logs\": [], \"errors\": []}"));
	}

	static void dspTree(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::DspTree, "api/dsp/tree")
			.withCategory("dsp")
			.withSummary("Get scriptnode network hierarchy")
			.withDescription("Returns the nested JSON tree of the active DspNetwork for the given "
				"module. Each node contains its nodeId, factoryPath, bypass state, parameters, "
				"and child nodes. Container nodes also have a connections array listing all "
				"modulation edges within that container (source, sourceOutput, target, parameter). "
				"Use verbose=true to include full parameter range metadata "
				"(min, max, stepSize, middlePosition, defaultValue). "
				"Use group=current inside an undo group (after push_group) to read the accumulated "
				"plan-mode snapshot before the group is committed -- returns 400 if there is no "
				"active DSP validation state, 501 for any group value other than 'current'.")
			.withReturns("Recursive node tree with parameters, connections on containers, and children")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::verbose,
				"Include full parameter range metadata")
				.withType(ParamType::Bool).withDefault("false"))
			.withQueryParam(RouteParameter(RestApiIds::group,
				"Optional group selector. 'current' returns the active plan's validation tree "
				"(accumulated state inside an undo group, before commit)").asOptional())
			.withErrorCodes({ 400, 404, 501 })
			.withRequestExample(R"(GET /api/dsp/tree?moduleId=Script%20FX1)")
			.withResponseExample(R"({"success": true, "result": {"nodeId": "MyDSP", "factoryPath": "container.chain", "bypassed": false, "parameters": [], "connections": [{"source": "PMA1", "sourceOutput": 0, "target": "Osc1", "parameter": "Frequency"}], "children": [{"nodeId": "PMA1", "factoryPath": "control.pma", "bypassed": false, "parameters": [{"parameterId": "Value", "value": 0.0}], "children": []}, {"nodeId": "Osc1", "factoryPath": "core.oscillator", "bypassed": false, "parameters": [{"parameterId": "Frequency", "value": 440}], "children": []}]}, "logs": [], "errors": []})"));
	}

	static void dspApply(Array<RouteMetadata>& m)
	{
		auto opItem = RouteParameter(RestApiIds::op, "DSP operation object")
			.withType(ParamType::Object)
			.withDiscriminator("op")
			.withVariant("add", "Add a node (factoryPath, parent, nodeId?, index?). "
				"If nodeId is provided, it must be unique - returns error if a node with that ID already exists. "
				"If nodeId is omitted, it is derived from the factory path suffix with a unique trailing index "
				"(e.g. container.chain => chain1, core.oscillator => oscillator1)")
			.withVariant("remove", "Remove a node (nodeId)")
			.withVariant("move", "Move a node to a different container (nodeId, parent, index?)")
			.withVariant("connect", "Connect a modulation source to a parameter (source, target, parameter, sourceOutput?). "
				"sourceOutput is a parameter name (string) or output slot index (int) for multi-output mod nodes")
			.withVariant("disconnect", "Disconnect a modulation source (source, target, parameter)")
			.withVariant("set", "Set a parameter value or node property (nodeId, parameterId, value). "
				"When nodeId is the root network node, also supports network-level properties: "
				"AllowCompilation (bool), AllowPolyphonic (bool), CompileChannelAmount (int), "
				"HasTail (bool), SuspendOnSilence (bool), ModulationBlockSize (power-of-2 int or 0)")
			.withVariant("bypass", "Set bypass state (nodeId, bypassed)")
			.withVariant("create_parameter", "Create a dynamic parameter on a container (nodeId, parameterId, min?, max?, defaultValue?, stepSize?, middlePosition?, skewFactor?)")
			.withVariant("clear", "Clear all nodes from the network")
			// All possible properties (union of all variants)
			.withProperty(RouteParameter(RestApiIds::op, "Operation type")
				.withEnumValues({ "add", "remove", "move", "connect", "disconnect", "set", "bypass", "create_parameter", "clear" }))
			.withProperty(RouteParameter(RestApiIds::factoryPath, "Factory path for add op (e.g. core.oscillator, filters.svf)")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::parent, "Parent container node ID for add/move ops")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::nodeId, "Node instance ID")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::parameterId, "Parameter name for set/create_parameter ops")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::index, "Position within parent container")
				.withType(ParamType::Int).asOptional())
			.withProperty(RouteParameter(RestApiIds::source, "Source node ID for connect/disconnect ops")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::target, "Target node ID for connect/disconnect ops")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::parameter, "Target parameter name for connect/disconnect ops")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::sourceOutput,
				"Source output for connect op: parameter name (string) or output slot index (int) for multi-output mod nodes")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::value, "New value for set op")
				.asOptional())
			.withProperty(RouteParameter(RestApiIds::bypassed, "Bypass state for bypass op")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(RestApiIds::min, "Minimum value for create_parameter")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(RestApiIds::max, "Maximum value for create_parameter")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(RestApiIds::defaultValue, "Default value for create_parameter")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(RestApiIds::stepSize, "Step size for create_parameter (0 = continuous)")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(RestApiIds::middlePosition,
				"Value at 50% of range for create_parameter (computes skew internally, mutually exclusive with skewFactor)")
				.withType(ParamType::Float).asOptional())
			.withProperty(RouteParameter(RestApiIds::skewFactor,
				"Raw skew factor for create_parameter (mutually exclusive with middlePosition)")
				.withType(ParamType::Float).asOptional());

		auto diffEntry = RouteParameter(Identifier("entry"), "Diff entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::target, "Node ID affected"))
			.withProperty(RouteParameter(RestApiIds::action, "Net action symbol")
				.withEnumValues({ "+", "-", "*" }))
			.withProperty(RouteParameter(RestApiIds::domain, "Operation domain")
				.withExample("dsp"));

		m.add(RouteMetadata(ApiRoute::DspApply, "api/dsp/apply")
			.withMethod(RestServer::POST)
			.withCategory("dsp")
			.withSummary("Apply batched operations to a scriptnode graph")
			.withDescription("Apply one or more scriptnode operations in a single batch. "
				"Operations are pre-validated before execution; invalid operations return 400 "
				"with detailed error info. All operations are undoable via the undo API. "
				"When called inside an undo group (after push_group), the diff accumulates "
				"all operations in the group so far. The response contains a diff summary "
				"with the net effect. Partial failure rolls back the entire batch.")
			.withReturns("Diff summary showing the net effect of all operations")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "Module ID of the DspNetwork holder")
				.withExample("Script FX1"))
			.withBodyParam(RouteParameter(RestApiIds::operations, "Non-empty array of operation objects")
				.withArrayItems(opItem))
			.withResponseField(RouteParameter(RestApiIds::scope, "Scope level")
				.withEnumValues({ "group", "root" }))
			.withResponseField(RouteParameter(RestApiIds::groupName, "Active group name"))
			.withResponseField(RouteParameter(RestApiIds::diff, "Array of diff entries")
				.withArrayItems(diffEntry))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"({"moduleId": "Script FX1", "operations": [{"op": "add", "factoryPath": "core.oscillator", "parent": "MyDSP", "nodeId": "Osc1", "index": 0}, {"op": "set", "nodeId": "Osc1", "parameterId": "Frequency", "value": 880}]})")
			.withResponseExample(R"({"success": true, "scope": "group", "groupName": "root", "diff": [{"target": "Osc1", "action": "+", "domain": "dsp"}], "logs": [], "errors": []})"));
	}

	static void dspSave(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::DspSave, "api/dsp/save")
			.withMethod(RestServer::POST)
			.withCategory("dsp")
			.withSummary("Save the active DspNetwork to its XML file")
			.withDescription("Saves the current state of the active DspNetwork to its XML file "
				"on disk. Fails if the network is embedded (embedded networks have no file-based "
				"representation).")
			.withReturns("File path of the saved network")
			.withModuleIdParam()
			.withResponseField(RouteParameter(RestApiIds::filePath, "Absolute path to the saved .xml file"))
			.withErrorCodes({ 400, 404 })
			.withRequestExample(R"({"moduleId": "Script FX"})")
			.withResponseExample("{\"success\": true, \"filePath\": \"D:/Projects/MyPlugin/DspNetworks/Networks/MyDSP.xml\", \"logs\": [], \"errors\": []}"));
	}

	/* GET /api/dsp/screenshot */
	static void dspScreenshot(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::DspScreenshot, "api/dsp/screenshot")
			.withCategory("dsp")
			.withSummary("Capture screenshot of currently edited DspNetwork graph to PNG file")
			.withDescription("Renders the DspNetworkGraph of the active (or debugged) DspNetwork "
				"belonging to the given module and writes a PNG file to outputPath. "
				"\n\n"
				"The DspNetworkGraph only exists inside the HISE IDE's BackendRootWindow, so "
				"this endpoint requires HISE to be running with the UI open (returns 503 in "
				"headless contexts). As a side effect of locating the graph, the call switches "
				"the active workspace to the requested module if it is not already shown. "
				"\n\n"
				"outputPath may be either an absolute path or a path relative to the project's "
				"Images folder; it must end with .png. If the file already exists it is "
				"overwritten.")
			.withReturns("File path of the saved PNG plus image dimensions")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::scale, "Scale factor (0.5, 1.0 or 2.0)")
				.withType(ParamType::Float).withDefault("1.0"))
			.withQueryParam(RouteParameter(RestApiIds::outputPath,
				"Absolute path or path relative to the project's Images folder "
				"(must end with .png). Existing files are overwritten."))
			.withResponseField(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withResponseField(RouteParameter(RestApiIds::width, "Width of captured image in pixels")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::height, "Height of captured image in pixels")
				.withType(ParamType::Int))
			.withResponseField(RouteParameter(RestApiIds::scale, "Scale factor that was applied")
				.withType(ParamType::Float))
			.withResponseField(RouteParameter(RestApiIds::filePath,
				"Absolute path to the saved PNG file"))
			.withErrorCodes({ 400, 404, 500, 503 })
			.withRequestExample(R"(GET /api/dsp/screenshot?moduleId=Script FX&scale=1.0&outputPath=graph.png)")
			.withResponseExample(R"({"success": true, "moduleId": "Script FX", "width": 800, "height": 600, "scale": 1.0, "filePath": "D:/Projects/MyPlugin/Images/graph.png", "logs": [], "errors": []})"));
	}

	/* GET /api/project/list */
	static void projectList(Array<RouteMetadata>& m)
	{
		auto projectEntry = RouteParameter(Identifier("entry"), "Project entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::name, "Project name from project_info.xml"))
			.withProperty(RouteParameter(RestApiIds::path, "Absolute path to project folder"));

		m.add(RouteMetadata(ApiRoute::ProjectList, "api/project/list")
			.rejectsInSnippetBrowser()
			.withCategory("project")
			.withSummary("List available HISE projects")
			.withDescription("Returns the merged, deduplicated union of HISE's recent projects "
				"list and a filesystem scan of the configured projects root. Each entry parses "
				"project_info.xml to extract the project Name. The active field holds the name "
				"of the currently loaded project.")
			.withReturns("Array of projects and the active project name")
			.withResponseField(RouteParameter(RestApiIds::projects, "Array of available projects")
				.withArrayItems(projectEntry))
			.withResponseField(RouteParameter(RestApiIds::active, "Name of the currently active project"))
			.withErrorCodes({})
			.withResponseExample(R"({"success": true, "projects": [{"name": "MyPlugin", "path": "/Users/foo/HISE Projects/MyPlugin"}, {"name": "TestSynth", "path": "/Users/foo/HISE Projects/TestSynth"}], "active": "MyPlugin", "logs": [], "errors": []})"));
	}

	/* GET /api/project/tree */
	static void projectTree(Array<RouteMetadata>& m)
	{
		auto treeNode = RouteParameter(Identifier("node"), "File or folder node")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::name, "File or folder name"))
			.withProperty(RouteParameter(RestApiIds::type, "Node type")
				.withEnumValues({ "file", "folder" }))
			.withProperty(RouteParameter(RestApiIds::referenced,
				"True if file is actively referenced by the runtime (file nodes only)")
				.withType(ParamType::Bool).asOptional())
			.withProperty(RouteParameter(RestApiIds::children,
				"Child nodes (folder nodes only)")
				.withType(ParamType::Array).asOptional());

		m.add(RouteMetadata(ApiRoute::ProjectTree, "api/project/tree")
			.rejectsInSnippetBrowser()
			.withCategory("project")
			.withSummary("Get project file tree with runtime reference flags")
			.withDescription("Walks the active project folder and returns its Scripts, SampleMaps, "
				"Images, DspNetworks and UserPresets subfolders. Any Binaries folder is excluded "
				"at any depth (build output). Inside Scripts, the ScriptProcessors subfolder is "
				"excluded and only source files with a .js, .glsl or .css extension are listed. "
				"Each file node carries a referenced flag indicating whether HISE's runtime "
				"actively uses the file (included scripts, loaded samplemaps, images in the "
				"pool, active DSP networks, currently loaded user preset).")
			.withReturns("projectName and the root tree node")
			.withResponseField(RouteParameter(RestApiIds::projectName, "Name of the project"))
			.withResponseField(RouteParameter(RestApiIds::root, "Root folder node")
				.withType(ParamType::Object))
			.withErrorCodes({ 500 })
			.withResponseExample(R"({"success": true, "projectName": "MyPlugin", "root": {"name": "MyPlugin", "type": "folder", "children": [{"name": "Scripts", "type": "folder", "children": [{"name": "Interface.js", "type": "file", "referenced": true}]}]}, "logs": [], "errors": []})"));
	}

	/* GET /api/project/files */
	static void projectFiles(Array<RouteMetadata>& m)
	{
		auto fileEntry = RouteParameter(Identifier("entry"), "Saveable file entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::name, "File name"))
			.withProperty(RouteParameter(RestApiIds::type, "File type")
				.withEnumValues({ "xml", "hip" }))
			.withProperty(RouteParameter(RestApiIds::path, "Path relative to the project root"))
			.withProperty(RouteParameter(RestApiIds::modified, "ISO8601 modification timestamp"));

		m.add(RouteMetadata(ApiRoute::ProjectFiles, "api/project/files")
			.rejectsInSnippetBrowser()
			.withCategory("project")
			.withSummary("List saveable project files (XML and HIP)")
			.withDescription("Enumerates the XmlPresetBackups folder (xml) and the project root "
				"for HIP archives. Returns one entry per file, sorted by modification time "
				"(newest first).")
			.withReturns("Array of saveable file entries")
			.withResponseField(RouteParameter(RestApiIds::files, "Array of saveable file entries")
				.withArrayItems(fileEntry))
			.withErrorCodes({ 500 })
			.withResponseExample(R"({"success": true, "files": [{"name": "MyPlugin.xml", "type": "xml", "path": "XmlPresetBackups/MyPlugin.xml", "modified": "2026-04-09T14:30:00Z"}, {"name": "MyPlugin.hip", "type": "hip", "path": "MyPlugin.hip", "modified": "2026-04-10T09:15:00Z"}], "logs": [], "errors": []})"));
	}

	/* GET /api/project/settings/list */
	static void projectSettingsList(Array<RouteMetadata>& m)
	{
		auto settingEntry = RouteParameter(Identifier("entry"), "Setting entry")
			.withType(ParamType::Object)
			.withProperty(RouteParameter(RestApiIds::value,
				"Current value. Strings pass through; \"Yes\"/\"No\" become true/false booleans."))
			.withProperty(RouteParameter(RestApiIds::description,
				"Markdown-formatted help text describing the setting"))
			.withProperty(RouteParameter(RestApiIds::options,
				"Allowed values when the setting is an enum or boolean flag")
				.withType(ParamType::Array).asOptional());

		m.add(RouteMetadata(ApiRoute::ProjectSettingsList, "api/project/settings/list")
			.rejectsInSnippetBrowser()
			.withCategory("project")
			.withSummary("Get all project settings with metadata")
			.withDescription("Returns every project setting (covering the Project and User "
				"namespaces in HiseSettings) as an object keyed by setting name. Each entry is "
				"an object carrying the current value, a markdown description of what the "
				"setting does, and (when applicable) the allowed values as an options array. "
				"Boolean flags stored in project_info.xml as \"Yes\"/\"No\" are normalised to "
				"JSON true/false.")
			.withReturns("settings object keyed by setting name, with value + description + optional options per entry")
			.withResponseField(RouteParameter(RestApiIds::settings,
				"Object keyed by setting name; each entry holds value, description and optional options")
				.withType(ParamType::Object)
				.withProperty(settingEntry))
			.withErrorCodes({ 500 })
			.withResponseExample(R"({"success": true, "settings": {"Name": {"value": "MyPlugin", "description": "The name of the project..."}, "VST3Support": {"value": true, "description": "If enabled, the exported plugins will use VST3...", "options": [true, false]}, "AAXCategoryFX": {"value": "AAX_ePlugInCategory_Modulation", "description": "AAX effect category...", "options": ["AAX_ePlugInCategory_EQ", "AAX_ePlugInCategory_Modulation"]}}, "logs": [], "errors": []})"));
	}

	/* POST /api/project/settings/set */
	static void projectSettingsSet(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ProjectSettingsSet, "api/project/settings/set")
			.rejectsInSnippetBrowser()
			.withMethod(RestServer::POST)
			.withCategory("project")
			.withSummary("Update a project setting")
			.withDescription("Sets a single project setting in project_info.xml. The key must be "
				"one of the known project settings (see /api/project/settings/list). Writes are "
				"atomic: the file is replaced on success.")
			.withReturns("Status message")
			.withBodyParam(RouteParameter(RestApiIds::key, "Setting key (e.g. Version, PluginCode)")
				.withExample("Version"))
			.withBodyParam(RouteParameter(RestApiIds::value, "New value (sent as string)")
				.withExample("1.1.0"))
			.withErrorCodes({ 400, 501 })
			.withRequestExample(R"({"key": "Version", "value": "1.1.0"})")
			.withResponseExample(R"({"success": true, "logs": ["Updated Version to 1.1.0"], "errors": []})"));
	}

	/* POST /api/project/save */
	static void projectSave(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ProjectSave, "api/project/save")
			.rejectsInSnippetBrowser()
			.withMethod(RestServer::POST)
			.withCategory("project")
			.withSummary("Save the current state as XML or HIP")
			.withDescription("Serializes the main synth chain. Format xml writes a human-readable "
				"XML file to XmlPresetBackups. Format hip writes a binary archive to the Presets "
				"subfolder. An optional filename overrides the default (the current chain id). "
				"When filename differs from the current chain id, HISE renames the master chain "
				"to match the filename before saving (applies to both formats); the response "
				"reports masterChainRenamed=true and the newName in that case.")
			.withReturns("path of the saved file; masterChainRenamed/newName when a rename happened")
			.withBodyParam(RouteParameter(RestApiIds::format, "Save format")
				.withEnumValues({ "xml", "hip" }))
			.withBodyParam(RouteParameter(Identifier("filename"),
				"Override filename without extension")
				.asOptional())
			.withResponseField(RouteParameter(RestApiIds::path,
				"Path of the saved file relative to the project root"))
			.withResponseField(RouteParameter(RestApiIds::masterChainRenamed,
				"True if the master chain was renamed to match the filename")
				.withType(ParamType::Bool).asOptional())
			.withResponseField(RouteParameter(RestApiIds::newName,
				"New master chain name after rename")
				.asOptional())
			.withErrorCodes({ 400, 500 })
			.withRequestExample(R"({"format": "xml", "filename": "MyPlugin_v2"})")
			.withResponseExample(R"({"success": true, "path": "XmlPresetBackups/MyPlugin_v2.xml", "logs": ["Saved as MyPlugin_v2.xml"], "errors": []})"));
	}

	/* POST /api/project/load */
	static void projectLoad(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ProjectLoad, "api/project/load")
			.rejectsInSnippetBrowser()
			.withMethod(RestServer::POST)
			.withCategory("project")
			.withSummary("Load an XML or HIP file into the current project")
			.withDescription("Loads a previously saved XML or HIP file. The path is relative to "
				"the project root. Loading replaces the current chain and triggers the voice "
				"kill coordinator.")
			.withReturns("Status message")
			.withBodyParam(RouteParameter(RestApiIds::file,
				"Relative path to the XML or HIP file to load")
				.withExample("XmlPresetBackups/MyPlugin.xml"))
			.withErrorCodes({ 400, 404, 500, 501 })
			.withRequestExample(R"({"file": "XmlPresetBackups/MyPlugin.xml"})")
			.withResponseExample(R"({"success": true, "logs": ["Loaded MyPlugin.xml"], "errors": []})"));
	}

	/* POST /api/project/switch */
	static void projectSwitch(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ProjectSwitch, "api/project/switch")
			.rejectsInSnippetBrowser()
			.withMethod(RestServer::POST)
			.withCategory("project")
			.withSummary("Switch the active HISE project")
			.withDescription("Switches the active project to the folder at the given absolute "
				"filesystem path. The path must point at a valid project folder (one containing "
				"project_info.xml); otherwise the request is rejected with 400. Use "
				"/api/project/list to enumerate known projects and copy the corresponding path.")
			.withReturns("Status message")
			.withBodyParam(RouteParameter(RestApiIds::project,
				"Absolute filesystem path to the target project folder")
				.withExample("/Users/foo/HISE Projects/TestSynth"))
			.withErrorCodes({ 400 })
			.withRequestExample(R"({"project": "/Users/foo/HISE Projects/TestSynth"})")
			.withResponseExample(R"({"success": true, "logs": ["Switched to TestSynth"], "errors": []})"));
	}

	/* GET /api/project/export_snippet */
	static void projectExportSnippet(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ProjectExportSnippet, "api/project/export_snippet")
			.withCategory("project")
			.withSummary("Export the current project as a HISE snippet string")
			.withDescription("Serializes the main synth chain into a gzip-compressed, "
				"base64-encoded HISE snippet prefixed with \"HiseSnippet \". Snippets embed "
				"script and network resources so they are self-contained and can be shared or "
				"pasted into another HISE session.")
			.withReturns("snippet string starting with HiseSnippet")
			.withResponseField(RouteParameter(RestApiIds::snippet, "HISE snippet string"))
			.withErrorCodes({ 500 })
			.withResponseExample(R"({"success": true, "snippet": "HiseSnippet 1234.abc...", "logs": [], "errors": []})"));
	}

	/* GET /api/project/preprocessor/list */
	static void projectPreprocessorList(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ProjectPreprocessorList, "api/project/preprocessor/list")
			.rejectsInSnippetBrowser()
			.withCategory("project")
			.withSummary("List preprocessor defines grouped by shared scope")
			.withDescription("Returns the active project's ExtraDefinitions preprocessor macros, "
				"cross-referenced across the build targets (Project, Dll) and operating systems "
				"(Windows, macOS, Linux). The response's preprocessors object is keyed by scope:\n"
				"  - \"*.*\": macros with identical value across all targets and OSes.\n"
				"  - \"Project.*\" / \"Dll.*\": macros with identical value across all OSes for "
				"that target (only emitted when the macro is not already lifted to \"*.*\").\n"
				"  - \"Project.Windows\" / \"Project.macOS\" / ... : macros unique to that "
				"(target, OS) slot (only emitted when the macro is not already lifted).\n"
				"Each scope entry is an object keyed by macro name with the raw value. The OS "
				"and target query parameters narrow which per-slot sections are emitted; the "
				"shared scopes (\"*.*\", \"target.*\") are always included when their scope "
				"intersects the filter.")
			.withReturns("preprocessors object keyed by scope; values are {macro: value} maps")
			.withQueryParam(RouteParameter(RestApiIds::OS, "Filter by operating system (default 'all')")
				.withEnumValues({ "Windows", "macOS", "Linux", "all" })
				.withDefault("all"))
			.withQueryParam(RouteParameter(RestApiIds::target, "Filter by build target (default 'all')")
				.withEnumValues({ "Project", "Dll", "all" })
				.withDefault("all"))
			.withResponseField(RouteParameter(RestApiIds::preprocessors,
				"Object keyed by scope ('*.*', 'target.*', 'target.OS'); each value is a "
				"{macro: value} object. Empty scopes are omitted.")
				.withType(ParamType::Object))
			.withErrorCodes({ 400 })
			.withRequestExample(R"(GET /api/project/preprocessor/list?OS=Windows&target=Project)")
			.withResponseExample(R"({"success": true, "preprocessors": {"*.*": {"SHARED_FLAG": 1}, "Project.*": {"HAS_LICENSE_KEY": 1}, "Project.Windows": {"WIN_ONLY_FLAG": 2}}, "logs": [], "errors": []})"));
	}

	/* POST /api/project/preprocessor/set */
	static void projectPreprocessorSet(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ProjectPreprocessorSet, "api/project/preprocessor/set")
			.rejectsInSnippetBrowser()
			.withMethod(RestServer::POST)
			.withCategory("project")
			.withSummary("Upsert or clear a preprocessor define")
			.withDescription("Sets a preprocessor macro for the given OS / target combination. "
				"Passing 'all' for OS or target fans the write out across every matching slot. "
				"Set value to the literal string \"default\" to remove the override and fall "
				"back to HISE's built-in default; any other value must parse as an integer and "
				"is written as MACRO=N.")
			.withReturns("Status envelope (success, logs, errors)")
			.withBodyParam(RouteParameter(RestApiIds::OS, "Target operating system")
				.withEnumValues({ "Windows", "macOS", "Linux", "all" }))
			.withBodyParam(RouteParameter(RestApiIds::target, "Build target")
				.withEnumValues({ "Project", "Dll", "all" }))
			.withBodyParam(RouteParameter(RestApiIds::preprocessor, "Macro name (e.g. ENABLE_FOO)")
				.withExample("ENABLE_FOO"))
			.withBodyParam(RouteParameter(RestApiIds::value,
				"Integer value (as string) or the literal \"default\" to clear the override")
				.withExample("1"))
			.withErrorCodes({ 400, 501 })
			.withRequestExample(R"({"OS": "Windows", "target": "Project", "preprocessor": "ENABLE_FOO", "value": "1"})")
			.withResponseExample(R"({"success": true, "logs": [], "errors": []})"));
	}

	/* POST /api/project/import_snippet */
	static void projectImportSnippet(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::ProjectImportSnippet, "api/project/import_snippet")
			.withMethod(RestServer::POST)
			.withCategory("project")
			.withSummary("Import a HISE snippet string")
			.withDescription("Decodes a HISE snippet string (base64-decode, gunzip) and loads the "
				"resulting ValueTree into the current project, replacing the current chain. "
				"Returns 400 when the snippet prefix is missing or the payload fails to decode.")
			.withReturns("Status message")
			.withBodyParam(RouteParameter(RestApiIds::snippet,
				"HISE snippet string starting with HiseSnippet")
				.withExample("HiseSnippet 1234.abc..."))
			.withErrorCodes({ 400 })
			.withRequestExample(R"({"snippet": "HiseSnippet 1234.abc..."})")
			.withResponseExample(R"({"success": true, "logs": ["Imported snippet"], "errors": []})"));
	}

	/* POST /api/snippet_browser */
	static void snippetBrowser(Array<RouteMetadata>& m)
	{
		m.add(RouteMetadata(ApiRoute::SnippetBrowser, "api/snippet_browser")
			.withMethod(RestServer::POST)
			.withCategory("status")
			.withSummary("Control the snippet browser instance lifecycle")
			.withDescription("Manages a secondary BackendProcessor used for browsing/auditioning HISE snippets. "
				"The snippet BP shares the main audio device and REST server with the main BP, but only one of "
				"the two drives audio at a time. While the snippet browser is active, requests to other endpoints "
				"(e.g. /api/list_components, /api/repl, /api/recompile) operate on the snippet BP. The endpoints "
				"/api/snippet_browser and /api/shutdown always operate on the main BP. "
				"Actions: 'launch' creates the snippet BP and switches to it (or just switches to it if already "
				"present). 'shutdown' destroys the snippet BP and reactivates the main BP. 'enable' switches to "
				"the existing snippet BP. 'disable' switches back to the main BP without destroying the snippet. "
				"Returns 409 when 'enable' or 'disable' is requested but no snippet instance exists. "
				"Returns 501 in headless builds where no UI root window is available.")
			.withReturns("Post-action state: exists (snippet alive?) and active ('main' or 'snippet')")
			.withBodyParam(RouteParameter(RestApiIds::action, "Lifecycle action")
				.withEnumValues({ "launch", "shutdown", "enable", "disable" }))
			.withResponseField(RouteParameter(RestApiIds::exists, "Whether a snippet browser instance is alive after the action")
				.withType(ParamType::Bool))
			.withResponseField(RouteParameter(RestApiIds::active, "Which BackendProcessor is currently driving audio")
				.withEnumValues({ "main", "snippet" }))
			.withErrorCodes({ 400, 409, 501 })
			.withRequestExample(R"({"action": "launch"})")
			.withResponseExample(R"({"success": true, "exists": true, "active": "snippet", "logs": [], "errors": []})"));
	}
};

const Array<RestHelpers::RouteMetadata>& RestHelpers::getRouteMetadata()
{
	static Array<RouteMetadata> metadata = []()
		{
			Array<RouteMetadata> m;

			// The order MUST match ApiRoute enum!

			RestApiEndpoints::listMethods(m);
			RestApiEndpoints::status(m);
			RestApiEndpoints::statusPreprocessors(m);
			RestApiEndpoints::getScript(m);
			RestApiEndpoints::setScript(m);
			RestApiEndpoints::evaluateRepl(m);
			RestApiEndpoints::recompile(m);
			RestApiEndpoints::listComponents(m);
			RestApiEndpoints::getComponentProperties(m);
			RestApiEndpoints::getComponentValue(m);
			RestApiEndpoints::setComponentValue(m);
			RestApiEndpoints::setComponentProperties(m);
			RestApiEndpoints::testingScreenshot(m);
			RestApiEndpoints::getSelectedComponents(m);
			RestApiEndpoints::testingE2e(m);
			RestApiEndpoints::diagnoseScript(m);
			RestApiEndpoints::getIncludedFiles(m);
			RestApiEndpoints::testingProfile(m);
			RestApiEndpoints::parseCss(m);
			RestApiEndpoints::shutdown(m);
			RestApiEndpoints::builderTree(m);
			RestApiEndpoints::applyBuilder(m);
			RestApiEndpoints::builderReset(m);
			RestApiEndpoints::undoPushGroup(m);
			RestApiEndpoints::undoPopGroup(m);
			RestApiEndpoints::undoBack(m);
			RestApiEndpoints::undoForward(m);
			RestApiEndpoints::undoDiff(m);
			RestApiEndpoints::undoHistory(m);
			RestApiEndpoints::undoClear(m);
			RestApiEndpoints::wizardInitialise(m);
			RestApiEndpoints::wizardExecute(m);
			RestApiEndpoints::wizardStatus(m);
			RestApiEndpoints::uiTree(m);
			RestApiEndpoints::uiApply(m);
			RestApiEndpoints::testingSequence(m);
			RestApiEndpoints::dspList(m);
			RestApiEndpoints::dspInit(m);
			RestApiEndpoints::dspTree(m);
			RestApiEndpoints::dspApply(m);
			RestApiEndpoints::dspSave(m);
			RestApiEndpoints::dspScreenshot(m);
			RestApiEndpoints::projectList(m);
			RestApiEndpoints::projectTree(m);
			RestApiEndpoints::projectFiles(m);
			RestApiEndpoints::projectSettingsList(m);
			RestApiEndpoints::projectSettingsSet(m);
			RestApiEndpoints::projectSave(m);
			RestApiEndpoints::projectLoad(m);
			RestApiEndpoints::projectSwitch(m);
			RestApiEndpoints::projectExportSnippet(m);
			RestApiEndpoints::projectImportSnippet(m);
			RestApiEndpoints::projectPreprocessorList(m);
			RestApiEndpoints::projectPreprocessorSet(m);
			RestApiEndpoints::snippetBrowser(m);

			// Verify count matches enum
			jassert(m.size() == (int)ApiRoute::numRoutes);

			return m;
		}();

	return metadata;
}

}
