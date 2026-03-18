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

namespace hise { using namespace juce;

//==============================================================================
// BaseScopedConsoleHandler implementation

RestHelpers::BaseScopedConsoleHandler::BaseScopedConsoleHandler(MainController* mc, bool enabled) :
	ControlledObject(mc),
	capturing(false)
{
	if (enabled && !mc->getConsoleHandler().hasCustomLogger())
	{
		capturing = true;
		mc->getConsoleHandler().setCustomCodeHandler(
			BIND_MEMBER_FUNCTION_3(BaseScopedConsoleHandler::onMessage));
	}
}

RestHelpers::BaseScopedConsoleHandler::~BaseScopedConsoleHandler()
{
	if (capturing)
		getMainController()->getConsoleHandler().setCustomCodeHandler({});
}

void RestHelpers::BaseScopedConsoleHandler::onMessage(const String& message, int warning, const Processor* p)
{
	if (warning == 0)
	{
		handleMessage(message);
	}
	else
	{
		auto lines = StringArray::fromLines(message);
		
		auto scriptRoot = getMainController()->getSampleManager().getProjectHandler()
			.getSubDirectory(FileHandlerBase::Scripts);
		auto moduleId = p->getId();
		
		auto error = parseError(lines[0], scriptRoot, moduleId);
		lines.remove(0);
		
		StringArray callstack;
		for (auto& entry : lines)
		{
			auto parsed = parseError(entry, scriptRoot, moduleId);
			if (parsed.location.isNotEmpty())
				callstack.add(parsed.toCallstackString());
		}
		
		handleError(error.message, callstack);
	}
}

String RestHelpers::BaseScopedConsoleHandler::ParsedError::toCallstackString() const
{
	if (functionName.isEmpty())
		return location;
	return functionName + "() at " + location;
}

RestHelpers::BaseScopedConsoleHandler::ParsedError RestHelpers::BaseScopedConsoleHandler::parseError(
	const String& errorString,
	const File& scriptRoot,
	const String& moduleId)
{
	ParsedError result;

	String working = errorString.trim();

	// Strip leading ":\t\t\t" from callstack entries
	if (working.startsWith(":"))
		working = working.fromFirstOccurrenceOf(":", false, false).trim();

	// Check if this is a callstack entry with function name: "funcName() - ..."
	if (working.contains("() - "))
	{
		result.functionName = working.upToFirstOccurrenceOf("()", false, false).trim();
		working = working.fromFirstOccurrenceOf("() - ", false, false);
	}

	// Extract message (everything before the encoded location)
	result.message = working.upToFirstOccurrenceOf("{{", false, false)
		.upToFirstOccurrenceOf("\t", false, false)
		.trim();

	// Extract and decode the Base64 location
	String encoded = working.fromFirstOccurrenceOf("{{", false, false)
		.upToFirstOccurrenceOf("}}", false, false);

	if (encoded.isNotEmpty())
	{
		MemoryOutputStream mos;
		if (Base64::convertFromBase64(mos, encoded))
		{
			String decoded(static_cast<const char*>(mos.getData()), mos.getDataSize());
			StringArray parts = StringArray::fromTokens(decoded, "|", "");

			if (parts.size() >= 5)
			{
				// Format: "processorId|relativePath|charIndex|line|col"
				String path = parts[1];
				int line = parts[3].getIntValue();
				int col = parts[4].getIntValue();

				// Build the path
				String fullPath;

				if (path.isEmpty() || path.contains("()"))
				{
					// Inline callback - use moduleId as filename
					fullPath = moduleId + ".js";
				}
				else
				{
					// External file
					File f = scriptRoot.getChildFile(path);
					if (f.existsAsFile())
						fullPath = f.getRelativePathFrom(scriptRoot.getParentDirectory()).replaceCharacter('\\', '/');
					else
						fullPath = path;
				}

				result.location = fullPath + ":" + String(line) + ":" + String(col);
			}
		}
	}

	return result;
}

//==============================================================================
// ScopedConsoleHandler implementation

RestHelpers::ScopedConsoleHandler::ScopedConsoleHandler(MainController* mc, RestServer::AsyncRequest::Ptr request_) :
	BaseScopedConsoleHandler(mc, true),  // Always enabled for REST
	request(*request_)
{
}

void RestHelpers::ScopedConsoleHandler::handleMessage(const String& message)
{
	request.appendLog(message);
}

void RestHelpers::ScopedConsoleHandler::handleError(const String& message, const StringArray& callstack)
{
	request.appendError(message, callstack);
}

//==============================================================================
// ReplayConsoleHandler implementation

RestHelpers::ReplayConsoleHandler::ReplayConsoleHandler(MainController* mc, InteractionTestWindow* window_, bool enabled) :
	BaseScopedConsoleHandler(mc, enabled),
	window(window_)
{
}

void RestHelpers::ReplayConsoleHandler::handleMessage(const String& message)
{
	logs.add(message);
}

void RestHelpers::ReplayConsoleHandler::handleError(const String& message, const StringArray& callstack)
{
	DynamicObject::Ptr errorObj = new DynamicObject();
	errorObj->setProperty("message", message);
	
	Array<var> callstackArray;
	for (auto& entry : callstack)
		callstackArray.add(entry);
	errorObj->setProperty("callstack", var(callstackArray));
	
	errors.add(var(errorObj.get()));
}

void RestHelpers::ReplayConsoleHandler::finalize(const var& inputInteractions, bool success,
                                                  int interactionsCompleted, int totalElapsedMs)
{
	if (window == nullptr) return;
	
	// Build JSON response similar to REST API format
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, success && errors.isEmpty());
	result->setProperty(RestApiIds::interactionsCompleted, interactionsCompleted);
	result->setProperty(RestApiIds::totalElapsedMs, totalElapsedMs);
	
	Array<var> logsArray;
	for (auto& log : logs)
		logsArray.add(log);
	result->setProperty(RestApiIds::logs, var(logsArray));
	
	if (!errors.isEmpty())
		result->setProperty(RestApiIds::errors, var(errors));
	
	String jsonResponse = JSON::toString(var(result.get()), false);
	
	// Log to StatusBar
	if (auto* tester = window->getInteractionTester())
		tester->logResponse(inputInteractions, jsonResponse, success && errors.isEmpty());
}

//==============================================================================
// Static helper methods

JavascriptProcessor* RestHelpers::getScriptProcessor(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	String moduleId;
	
	if (req->getRequest().method == RestServer::POST)
	{
		// For POST requests, read moduleId from JSON body
		auto body = req->getRequest().getJsonBody();
		moduleId = body[RestApiIds::moduleId].toString();
	}
	else
	{
		// For GET requests, read moduleId from query parameters
		moduleId = req->getRequest()[RestApiIds::moduleId];
	}

	if (moduleId.isEmpty())
		return nullptr;

	return dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));
}

void RestHelpers::waitForPendingCallbacks(ScriptComponent* sc, int timeoutMs)
{
	const int intervalMs = 5;
	int waited = 0;
	
	while (sc->isControlCallbackPending() && waited < timeoutMs)
	{
		Thread::sleep(intervalMs);
		waited += intervalMs;
	}
}

//==============================================================================
// LAF (LookAndFeel) integration helpers

/** Converts LafRegistry::LafInfo::RenderStyle enum to JSON string. */
static String renderStyleToString(ScriptingApi::Content::LafRegistry::LafInfo::RenderStyle style)
{
	using RenderStyle = ScriptingApi::Content::LafRegistry::LafInfo::RenderStyle;
	
	switch (style)
	{
		case RenderStyle::Script:    return "script";
		case RenderStyle::Css:       return "css";
		case RenderStyle::CssInline: return "css_inline";
		case RenderStyle::Mixed:     return "mixed";
		default:                     return "unknown";
	}
}

/** Builds LAF info object for a component, or returns false if no LAF assigned.
    Computes location lazily - by this point watched files are populated. */
static var getLafInfoForComponent(ScriptingApi::Content::LafRegistry* registry, 
                                   ScriptComponent* sc,
                                   const File& scriptRoot,
                                   const String& moduleId)
{
	if (registry == nullptr || sc == nullptr)
		return var(false);
	
	if (auto lafInfo = registry->getLafInfoForComponent(sc->getName()))
	{
		DynamicObject::Ptr laf = new DynamicObject();
		laf->setProperty(RestApiIds::id, lafInfo->variableName);
		laf->setProperty(RestApiIds::renderStyle, renderStyleToString(lafInfo->renderStyle));
		
		// Compute location lazily - watched files are now populated
		String locationStr;
		if (auto jp = dynamic_cast<JavascriptProcessor*>(sc->getScriptProcessor()))
		{
			if (auto codeDoc = jp->getSnippet(lafInfo->location))
			{
				CodeDocument::Position pos(*codeDoc, lafInfo->location.charNumber);
				auto lineNumber = pos.getLineNumber() + 1;  // 1-based
				auto columnNumber = pos.getIndexInLine();
				
				// Build location string directly (same format as error callstacks)
				auto fileName = lafInfo->location.fileName;
				String filePath;
				
				if (fileName.isEmpty() || fileName == "onInit" || fileName.contains("()"))
				{
					// Inline callback - use moduleId.js
					filePath = moduleId + ".js";
				}
				else
				{
					// External file - use relative path from Scripts folder
					filePath = "Scripts/" + fileName;
				}
				
				locationStr = filePath + ":" + String(lineNumber) + ":" + String(columnNumber);
			}
			else
			{
				// Fallback if document not found
				locationStr = lafInfo->location.fileName.isNotEmpty() ? lafInfo->location.fileName : "onInit";
			}
		}
		else
		{
			// Fallback if no JavascriptProcessor
			locationStr = lafInfo->location.fileName.isNotEmpty() ? lafInfo->location.fileName : "onInit";
		}
		
		laf->setProperty(RestApiIds::location, locationStr);
		laf->setProperty(RestApiIds::cssLocation, lafInfo->cssLocation);
		return var(laf.get());
	}
	
	return var(false);
}

/** Adds the externalFiles array to a compile response.
    Lists all watched external .js files for the given processor. */
static void addExternalFilesToResponse(Processor* p, DynamicObject* result)
{
	auto* jp = dynamic_cast<JavascriptProcessor*>(p);
	if (jp == nullptr)
		return;
	
	Array<var> files;
	for (int i = 0; i < jp->getNumWatchedFiles(); i++)
		files.add(jp->getWatchedFile(i).getFileName());
	
	result->setProperty(RestApiIds::externalFiles, var(files));
}

/** Checks for script-based LAF recipients and waits for them to render.
    If timeout occurs, adds lafRenderWarning to the result object.
    Skipped on test port (1901) to avoid test delays. */
static void addLafRenderWarningIfNeeded(MainController* mc, 
                                         ScriptingApi::Content* content, 
                                         DynamicObject* result)
{
	// Skip on test port to avoid test delays
	if (auto bp = dynamic_cast<BackendProcessor*>(mc))
	{
		if (bp->getRestServer().isTestMode())
			return;
	}
	
	auto registry = content->getLafRegistry();
	if (registry == nullptr || !registry->hasScriptBasedRecipients())
		return;
	
	// Poll for render completion - pump message loop to allow UI thread to paint
	constexpr int timeoutMs = 1000;
	constexpr int pollIntervalMs = 50;
	int elapsed = 0;
	
	while (elapsed < timeoutMs && !registry->allRecipientsRendered())
	{
		// Pump the message loop to allow UI thread to process paint events
		if (auto* mm = MessageManager::getInstanceWithoutCreating())
			mm->runDispatchLoopUntil(pollIntervalMs);
		else
			Thread::sleep(pollIntervalMs);
		
		elapsed += pollIntervalMs;
	}
	
	auto unrendered = registry->getUnrenderedComponents();
	
	if (!unrendered.isEmpty())
	{
		DynamicObject::Ptr warning = new DynamicObject();
		warning->setProperty(RestApiIds::errorMessage, 
			"Some LAF components did not render");
		
		Array<var> componentList;
		for (const auto& info : unrendered)
		{
			DynamicObject::Ptr comp = new DynamicObject();
			comp->setProperty(RestApiIds::id, info.id.toString());
			comp->setProperty(RestApiIds::reason, info.isInvisible ? "invisible" : "timeout");
			componentList.add(var(comp.get()));
		}
		warning->setProperty(RestApiIds::unrenderedComponents, componentList);
		warning->setProperty(RestApiIds::timeoutMs, timeoutMs);
		
		result->setProperty(RestApiIds::lafRenderWarning, var(warning.get()));
	}
}

RestServer::Response RestHelpers::handleRecompile(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	bool forceSync = (bool)obj.getProperty(RestApiIds::forceSynchronousExecution, false);
	
	// Create ScopedBadBabysitter if forceSynchronousExecution is requested
	// This bypasses all threading checks and executes everything synchronously
	std::unique_ptr<MainController::ScopedBadBabysitter> syncMode;
	if (forceSync)
		syncMode = std::make_unique<MainController::ScopedBadBabysitter>(mc);

	// Start attached profiling session if requested (fire-and-forget).
	// Results are retrieved later via POST /api/profile { "mode": "get" }
#if HISE_INCLUDE_PROFILING_TOOLKIT
	if ((bool)obj.getProperty(RestApiIds::profile, false))
		startProfilingSession(mc, obj, 2000.0);
#endif

	if (auto jp = getScriptProcessor(mc, req))
	{
		mc->refreshExternalFiles(true);

		mc->getKillStateHandler().killVoicesAndCall(dynamic_cast<Processor*>(jp), [req, forceSync, mc](Processor* p)
		{
			JavascriptProcessor::ResultFunction rf = [req, forceSync, mc, p](const JavascriptProcessor::SnippetResult& result)
			{
				DynamicObject::Ptr r = new DynamicObject();
				r->setProperty(RestApiIds::success, result.r.wasOk());
				r->setProperty(RestApiIds::result, result.r.wasOk() ? "Recompiled OK" : "Compilation / Runtime Error");
				r->setProperty(RestApiIds::forceSynchronousExecution, forceSync);
				
				if (forceSync)
					r->setProperty(RestApiIds::warning, "Executed in unsafe synchronous mode - threading checks bypassed");
				
				// Check for LAF render warnings on successful compilation
				if (result.r.wasOk())
				{
					if (auto ps = dynamic_cast<ProcessorWithScriptingContent*>(p))
					{
						if (auto content = ps->getScriptingContent())
							addLafRenderWarningIfNeeded(mc, content, r.get());
					}
				}

				addExternalFilesToResponse(p, r.get());
				req->complete(RestServer::Response::ok(var(r.get())));
			};

			dynamic_cast<JavascriptProcessor*>(p)->compileScript(rf);
			return SafeFunctionCall::OK;
		}, MainController::KillStateHandler::TargetThread::ScriptingThread);
	}
	else
	{
		req->fail(404, "moduleId is not a valid script processor");
	}

	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleEvaluateREPL(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto moduleId = obj[RestApiIds::moduleId].toString();
	auto expression = obj[RestApiIds::expression].toString();

	if (moduleId.isEmpty())
		return req->fail(400, "moduleId is required in request body");

	if (expression.isEmpty())
		return req->fail(400, "expression is required in request body");

	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));

	if (jp == nullptr)
		return req->fail(404, "module not found: " + moduleId);

	auto engine = jp->getScriptEngine();

	if (engine == nullptr)
		return req->fail(500, "no script engine present. Compile at least once before this method");

	auto r = Result::ok();
	auto v = engine->evaluate(expression, &r);

	if (v.isUndefined() || v.isVoid())
		v = "undefined";

	DynamicObject::Ptr res = new DynamicObject();
	res->setProperty(RestApiIds::success, r.wasOk());
	res->setProperty(RestApiIds::moduleId, moduleId);
	res->setProperty(RestApiIds::result, r.wasOk() ? "REPL Evaluation OK" : "Error at REPL Evaluation");
	res->setProperty(RestApiIds::value, v);

	if (!r.wasOk())
		debugError(dynamic_cast<Processor*>(jp), r.getErrorMessage());

	req->complete(RestServer::Response::ok(var(res.get())));
	return req->waitForResponse();
}

/** Internal recursive helper that includes LAF info when registry is provided. */
static DynamicObject::Ptr createRecursivePropertyTreeWithLaf(ScriptComponent* sc, 
                                                              ScriptingApi::Content::LafRegistry* registry,
                                                              const File& scriptRoot,
                                                              const String& moduleId)
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(RestApiIds::id, sc->getName().toString());
	obj->setProperty(RestApiIds::type, sc->getObjectName().toString());
	obj->setProperty(RestApiIds::visible, sc->getScriptObjectProperty(ScriptComponent::visible));
	obj->setProperty(RestApiIds::enabled, sc->getScriptObjectProperty(ScriptComponent::enabled));
	obj->setProperty(RestApiIds::x, sc->getScriptObjectProperty(ScriptComponent::x));
	obj->setProperty(RestApiIds::y, sc->getScriptObjectProperty(ScriptComponent::y));
	obj->setProperty(RestApiIds::width, sc->getScriptObjectProperty(ScriptComponent::width));
	obj->setProperty(RestApiIds::height, sc->getScriptObjectProperty(ScriptComponent::height));
	
	// Add LAF info
	obj->setProperty(RestApiIds::laf, getLafInfoForComponent(registry, sc, scriptRoot, moduleId));

	Array<var> children;

	ScriptComponent::ChildIterator<ScriptComponent> ci(sc);

	auto v = sc->getPropertyValueTree();

	for (auto c : v)
	{
		if (auto child = sc->getScriptProcessor()->getScriptingContent()->getComponentWithName(c[RestApiIds::id].toString()))
		{
			auto cobj = createRecursivePropertyTreeWithLaf(child, registry, scriptRoot, moduleId);
			children.add(var(cobj.get()));
		}
	}

	obj->setProperty(RestApiIds::childComponents, var(children));

	return obj;
}

DynamicObject::Ptr RestHelpers::createRecursivePropertyTree(ScriptComponent* sc)
{
	// Public API without LAF info (for backward compatibility)
	// Pass empty File and String since LAF info won't be used with nullptr registry
	return createRecursivePropertyTreeWithLaf(sc, nullptr, File(), String());
}

//==============================================================================
// Route metadata registry

const Array<RestHelpers::RouteMetadata>& RestHelpers::getRouteMetadata()
{
	static Array<RouteMetadata> metadata = []()
	{
		Array<RouteMetadata> m;
		
		// The order MUST match ApiRoute enum!
		
		// ApiRoute::ListMethods
		m.add(RouteMetadata(ApiRoute::ListMethods, "")
			.withCategory("status")
			.withDescription("List all available API methods with parameters and descriptions")
			.withReturns("Array of method definitions with path, parameters, and documentation"));
		
		// ApiRoute::Status
		m.add(RouteMetadata(ApiRoute::Status, "api/status")
			.withCategory("status")
			.withDescription("Get project status and discover available script processors")
			.withReturns("Server info (version, compileTimeout in seconds), project info, scriptsFolder path, and scriptProcessors with their callbacks"));
		
		// ApiRoute::GetScript
		m.add(RouteMetadata(ApiRoute::GetScript, "api/get_script")
			.withCategory("scripting")
			.withDescription("Read script content from a processor's callbacks")
			.withReturns("Callbacks object with script content for requested callback(s). Includes externalFiles array with name and full path.")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::callback, "Specific callback name (e.g., onInit). If omitted, returns all callbacks.").asOptional()));
		
		// ApiRoute::SetScript
		m.add(RouteMetadata(ApiRoute::SetScript, "api/set_script")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withDescription("Update one or more callbacks and optionally compile. Only specified callbacks are updated; others remain unchanged.")
			.withReturns("Compilation result with updatedCallbacks array, success status, logs, errors, and optional lafRenderWarning listing unrendered LAF components with reason (invisible or timeout)")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withBodyParam(RouteParameter(RestApiIds::callbacks, "Object with callback names as keys and script content as values (e.g., {onInit: \"...\", onNoteOn: \"...\"})"))
			.withBodyParam(RouteParameter(RestApiIds::compile, "Whether to compile after setting").withDefault("true"))
			.withBodyParam(RouteParameter(RestApiIds::forceSynchronousExecution, "Debug tool: Bypass threading model for synchronous execution. WARNING: May cause crashes due to race conditions - use only as last resort after saving.").withDefault("false")));
		
		m.add(RouteMetadata(ApiRoute::EvaluateREPL, "api/repl")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withDescription("Evaluates a script expression with the current script engine and returns the result")
			.withReturns("Evaluation result with success status, logs and error messages")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withBodyParam(RouteParameter(RestApiIds::expression, "The HiseScript expression that is evaluated. Note that any side effects of this evaluation might change the runtime state of HISE.")));

		// ApiRoute::Recompile
		m.add(RouteMetadata(ApiRoute::Recompile, "api/recompile")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withDescription("Recompile a processor (restores preset values, triggering callbacks for saveInPreset components)")
			.withReturns("Compilation result with success status, logs, errors, and optional lafRenderWarning listing unrendered LAF components with reason (invisible or timeout)")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withBodyParam(RouteParameter(RestApiIds::forceSynchronousExecution, "Debug tool: Bypass threading model for synchronous execution. WARNING: May cause crashes due to race conditions - use only as last resort after saving.").withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::profile, "Start a profiling session alongside compilation. Retrieve results later via POST /api/profile with mode=\"get\".").withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::durationMs, "Profiling duration in ms when profile=true (100-5000).").withDefault("2000")));
		
		// ApiRoute::ListComponents
		m.add(RouteMetadata(ApiRoute::ListComponents, "api/list_components")
			.withCategory("ui")
			.withDescription("List all UI components in a script processor")
			.withReturns("Array of components with id, type, and laf info (flat list or hierarchical tree with layout properties)")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::hierarchy, "If true, returns nested tree with layout properties").withDefault("false")));
		
		// ApiRoute::GetComponentProperties
		m.add(RouteMetadata(ApiRoute::GetComponentProperties, "api/get_component_properties")
			.withCategory("ui")
			.withDescription("Get all properties for a specific UI component")
			.withReturns("Component type and array of properties with id, value, isDefault, and options")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::id, "The component's ID (e.g., Button1, Panel1)")));
		
		// ApiRoute::GetComponentValue
		m.add(RouteMetadata(ApiRoute::GetComponentValue, "api/get_component_value")
			.withCategory("ui")
			.withDescription("Get the current runtime value of a UI component")
			.withReturns("Component type, current value, and min/max range")
			.withModuleIdParam()
			.withQueryParam(RouteParameter(RestApiIds::id, "The component's ID (e.g., GainKnob, BypassButton)")));
		
		// ApiRoute::SetComponentValue
		m.add(RouteMetadata(ApiRoute::SetComponentValue, "api/set_component_value")
			.withMethod(RestServer::POST)
			.withCategory("ui")
			.withDescription("Set the runtime value of a UI component (triggers control callback)")
			.withReturns("Success status")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withBodyParam(RouteParameter(RestApiIds::id, "The component's ID"))
			.withBodyParam(RouteParameter(RestApiIds::value, "The value to set"))
			.withBodyParam(RouteParameter(RestApiIds::validateRange, "If true, validates value is within component's min/max range").withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::forceSynchronousExecution, "Debug tool: Bypass threading model for synchronous execution. WARNING: May cause crashes due to race conditions - use only as last resort after saving.").withDefault("false")));
		
		// ApiRoute::SetComponentProperties
		m.add(RouteMetadata(ApiRoute::SetComponentProperties, "api/set_component_properties")
			.withMethod(RestServer::POST)
			.withCategory("ui")
			.withDescription("Set properties on one or more UI components (like Interface Designer)")
			.withReturns("Success with applied changes (recompileRequired=true if parentComponent changed), or error with locked properties if any are script-controlled")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID"))
			.withBodyParam(RouteParameter(RestApiIds::changes, "Array of {id, properties: {...}} objects"))
			.withBodyParam(RouteParameter(RestApiIds::force, "If true, bypasses script-lock check and sets all properties").withDefault("false")));
		
		// ApiRoute::Screenshot
		m.add(RouteMetadata(ApiRoute::Screenshot, "api/screenshot")
			.withCategory("ui")
			.withDescription("Capture screenshot of interface or specific component")
			.withReturns("Base64-encoded PNG image data with dimensions, or file path if outputPath is specified")
			.withQueryParam(RouteParameter(RestApiIds::moduleId, "Script processor ID").withDefault("Interface"))
			.withQueryParam(RouteParameter(RestApiIds::id, "Component ID to capture (omit for full interface)").asOptional())
			.withQueryParam(RouteParameter(RestApiIds::scale, "Scale factor (0.5 or 1.0)").withDefault("1.0"))
			.withQueryParam(RouteParameter(RestApiIds::outputPath, "File path to save PNG (must end with .png). If provided, writes to file instead of returning Base64").asOptional()));
		
		// ApiRoute::GetSelectedComponents
		m.add(RouteMetadata(ApiRoute::GetSelectedComponents, "api/get_selected_components")
			.withCategory("ui")
			.withDescription("Get the currently selected UI components from the Interface Designer")
			.withReturns("Selection count and array of selected components with all properties")
			.withQueryParam(RouteParameter(RestApiIds::moduleId, "The script processor's module ID").withDefault("Interface")));
		
		// ApiRoute::SimulateInteractions
		m.add(RouteMetadata(ApiRoute::SimulateInteractions, "api/simulate_interactions")
			.withMethod(RestServer::POST)
			.withCategory("testing")
			.withDescription("Execute a sequence of UI interactions in a test window. Auto-inserts moveTo events as needed for proper mouse positioning.")
			.withReturns("Execution result with success status, completion count, timing, execution log, captured screenshots, and optionally mouseState when verbose=true")
			.withBodyParam(RouteParameter(RestApiIds::interactions, 
				"Array of interaction objects. Types: 'moveTo' (explicit mouse positioning), 'click' (uses current position), "
				"'doubleClick' (expands to two clicks), 'drag' (uses pixel 'delta'), 'selectMenuItem' (click menu item by text), "
				"'screenshot' (capture interface). Fields: 'target' (component ID), 'delay' (ms before action), "
				"'normalizedPosition' (0-1, default center), 'pixelPosition' (absolute, takes precedence), 'delta' (for drag), "
				"'subtarget' (optional sub-component ID), 'rightClick' (boolean), 'shiftDown'/'ctrlDown'/'altDown'/'cmdDown' (modifiers), "
				"'menuItemText' (for selectMenuItem), 'id'/'scale' (for screenshot)"))
			.withBodyParam(RouteParameter(RestApiIds::verbose, "If true, include auto-insertion details and final mouseState in response").withDefault("false")));
		
		// ApiRoute::DiagnoseScript
		m.add(RouteMetadata(ApiRoute::DiagnoseScript, "api/diagnose_script")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withDescription("Run diagnostic-only shadow parse on a script file. Returns structured diagnostics "
							 "without modifying runtime state. Requires at least one prior successful compile (F5).")
			.withReturns("Array of diagnostics with line, column, severity, source, message, and suggestions")
			.withBodyParam(RouteParameter(RestApiIds::moduleId, 
				"The script processor's module ID. Required if filePath is not provided.").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::filePath,
				"Path to the external .js file (absolute or relative to Scripts folder). "
				"Required if moduleId is not provided. When used alone, HISE resolves the owning processor.").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::async,
				"If true, defer the shadow parse to the scripting thread (slower, blocks audio). "
				"Default is false: runs directly on the HTTP thread with a read lock.").withDefault("false")));
		
		// ApiRoute::GetIncludedFiles
		m.add(RouteMetadata(ApiRoute::GetIncludedFiles, "api/get_included_files")
			.withCategory("scripting")
			.withDescription("List all included (watched) external script files. "
							 "Without moduleId, returns all files across all processors with owning processor names. "
							 "With moduleId, returns files for that processor only.")
			.withReturns("Array of included files as full paths, optionally with owning processor ID")
			.withQueryParam(RouteParameter(RestApiIds::moduleId, 
				"Filter by script processor. If omitted, returns files from all processors with processor names.").asOptional()));
		
		// ApiRoute::StartProfiling
		m.add(RouteMetadata(ApiRoute::StartProfiling, "api/profile")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withDescription("Start a profiling session or retrieve last result. "
							 "mode=\"record\" starts a new session (non-blocking, returns immediately). "
							 "mode=\"get\" returns last result with optional filtering/summary. "
							 "Workflow: record once, then query with different filters.")
			.withReturns("Full tree (threads+flows), filtered results, or recording status")
			.withBodyParam(RouteParameter(RestApiIds::mode,
				"\"record\" = start new session (non-blocking), "
				"\"get\" = return last result (blocks if recording in progress)")
				.withDefault("record"))
			.withBodyParam(RouteParameter(RestApiIds::durationMs,
				"Recording duration in milliseconds (100-5000). Used in record mode.")
				.withDefault("1000"))
			.withBodyParam(RouteParameter(RestApiIds::threadFilter,
				"Array of thread names to include (default: all). In record mode, controls "
				"which threads are recorded. In get mode, filters which threads are queried. "
				"Valid: Audio Thread, Scripting Thread, UI Thread, Loading Thread, etc.")
				.asOptional())
			.withBodyParam(RouteParameter(RestApiIds::eventFilter,
				"Array of event source types to record (default: all). Used in record mode. "
				"Valid: DSP, Script, Lock, Callback, Trace, TimerCallback, Scriptnode, etc.")
				.asOptional())
			.withBodyParam(RouteParameter(RestApiIds::summary,
				"If true, aggregate repeated events with count/median/peak/min/total stats. "
				"Used in get mode.")
				.withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::filter,
				"Wildcard pattern matched against event name (e.g. \"slow*\", \"*.processBlock*\"). "
				"Case-insensitive. Used in get mode.")
				.asOptional())
			.withBodyParam(RouteParameter(RestApiIds::minDuration,
				"Only include events with duration >= this value in ms. Used in get mode.")
				.asOptional())
			.withBodyParam(RouteParameter(RestApiIds::sourceTypeFilter,
				"Wildcard pattern matched against sourceType (e.g. \"Trace\", \"Script\"). "
				"Case-insensitive. Used in get mode.")
				.asOptional())
			.withBodyParam(RouteParameter(RestApiIds::nested,
				"When filtering, include children of matched events. Used in get mode.")
				.withDefault("false"))
			.withBodyParam(RouteParameter(RestApiIds::limit,
				"Max number of results in filtered/summary mode (1-100). Used in get mode.")
				.withDefault("15"))
			.withBodyParam(RouteParameter(RestApiIds::wait,
				"If false, return immediately when recording is in progress instead of "
				"blocking. Returns {recording: true}. Used in get mode.")
				.withDefault("true")));
		
		// ApiRoute::ParseCSS
		m.add(RouteMetadata(ApiRoute::ParseCSS, "api/parse_css")
			.withMethod(RestServer::POST)
			.withCategory("scripting")
			.withDescription("Parse CSS code and return structured diagnostics. "
				"Accepts either inline code or a file path to a .css file. "
				"Optionally resolves properties for a set of selectors using CSS specificity rules")
			.withReturns("Diagnostics array with line/column/severity/message, "
				"list of parsed selectors, "
				"and resolved properties when selectors are provided")
			.withBodyParam(RouteParameter(RestApiIds::code,
				"The CSS code to parse (provide this or filePath)").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::filePath,
				"Path to a .css file. Relative paths resolve against the Scripts/ directory "
				"(provide this or code)").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::selectors,
				"Array of selector strings representing a component's selectors "
				"(e.g. [\"button\", \".my-class\", \"#MyId\"]). "
				"Resolves properties using CSS specificity").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::width,
				"Reference width in pixels for resolving percentage and relative units").asOptional())
			.withBodyParam(RouteParameter(RestApiIds::height,
				"Reference height in pixels for resolving percentage and relative units").asOptional()));
		
		// ApiRoute::Shutdown
		m.add(RouteMetadata(ApiRoute::Shutdown, "api/shutdown")
			.withMethod(RestServer::POST)
			.withCategory("status")
			.withDescription("Gracefully quit the HISE application")
			.withReturns("Success confirmation before shutdown begins"));
		
		// Verify count matches enum
		jassert(m.size() == (int)ApiRoute::numRoutes);
		
		return m;
	}();
	
	return metadata;
}

String RestHelpers::getRoutePath(RestHelpers::ApiRoute route)
{
	return getRouteMetadata()[(int)route].path;
}

RestHelpers::ApiRoute RestHelpers::findRoute(const String& subURL)
{
	const auto& metadata = getRouteMetadata();
	for (int i = 0; i < metadata.size(); i++)
	{
		if (metadata[i].path == subURL)
			return (ApiRoute)i;
	}
	return ApiRoute::numRoutes;
}

//==============================================================================
// Route handlers

RestServer::Response RestHelpers::handleListMethods(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	ignoreUnused(mc);
	
	const auto& metadata = getRouteMetadata();
	
	// Sort by category, then alphabetically by path
	Array<int> sortedIndices;
	for (int i = 0; i < metadata.size(); i++)
		sortedIndices.add(i);
	
	// JUCE ElementComparator for sorting indices by category then path
	struct RouteIndexComparator
	{
		const Array<RouteMetadata>& routes;
		
		RouteIndexComparator(const Array<RouteMetadata>& r) : routes(r) {}
		
		int compareElements(int a, int b) const
		{
			const auto& ra = routes[a];
			const auto& rb = routes[b];
			
			int catCompare = ra.category.compare(rb.category);
			if (catCompare != 0)
				return catCompare;
			
			return ra.path.compare(rb.path);
		}
	};
	
	RouteIndexComparator comparator(metadata);
	sortedIndices.sort(comparator);
	
	// Build response
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	
	Array<var> methods;
	for (int idx : sortedIndices)
	{
		const auto& route = metadata[idx];
		
		DynamicObject::Ptr m = new DynamicObject();
		m->setProperty(RestApiIds::path, "/" + route.path);
		m->setProperty(RestApiIds::method, route.method == RestServer::GET ? "GET" : "POST");
		m->setProperty(RestApiIds::category, route.category);
		m->setProperty(RestApiIds::description, route.description);
		m->setProperty(RestApiIds::returns, route.returns);
		
		// Query parameters (for GET)
		if (!route.queryParameters.isEmpty())
		{
			Array<var> params;
			for (const auto& p : route.queryParameters)
			{
				DynamicObject::Ptr param = new DynamicObject();
				param->setProperty(RestApiIds::name, p.name.toString());
				param->setProperty(RestApiIds::description, p.description);
				param->setProperty(RestApiIds::required, p.required);
				if (p.defaultValue.isNotEmpty())
					param->setProperty(RestApiIds::defaultValue, p.defaultValue);
				params.add(param.get());
			}
			m->setProperty(RestApiIds::queryParameters, params);
		}
		
		// Body parameters (for POST)
		if (!route.bodyParameters.isEmpty())
		{
			Array<var> params;
			for (const auto& p : route.bodyParameters)
			{
				DynamicObject::Ptr param = new DynamicObject();
				param->setProperty(RestApiIds::name, p.name.toString());
				param->setProperty(RestApiIds::description, p.description);
				param->setProperty(RestApiIds::required, p.required);
				if (p.defaultValue.isNotEmpty())
					param->setProperty(RestApiIds::defaultValue, p.defaultValue);
				params.add(param.get());
			}
			m->setProperty(RestApiIds::bodyParameters, params);
		}
		
		methods.add(m.get());
	}
	
	result->setProperty(RestApiIds::methods, methods);
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());
	
	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleStatus(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	// Wait for HISE to fully initialise (audio thread running, interface compiled).
	// This makes /api/status a reliable readiness probe after launch.
	if (!mc->isInitialised())
	{
		auto startTime = Time::getMillisecondCounter();

		while (!mc->isInitialised())
		{
			if (Time::getMillisecondCounter() - startTime > 10000)
				return req->fail(503, "HISE is still initialising");

			Thread::sleep(100);
		}
	}

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);

	// Server info
	DynamicObject::Ptr server = new DynamicObject();
	server->setProperty(RestApiIds::version, PresetHandler::getVersionString());
	server->setProperty(RestApiIds::compileTimeout, GET_HISE_SETTING(mc->getMainSynthChain(), HiseSettings::Scripting::CompileTimeout));
	result->setProperty(RestApiIds::server, var(server.get()));

	// Project info
	DynamicObject::Ptr project = new DynamicObject();
	project->setProperty(RestApiIds::name, GET_HISE_SETTING(mc->getMainSynthChain(), HiseSettings::Project::Name));

	auto& projectHandler = mc->getSampleManager().getProjectHandler();
	auto projectFolder = projectHandler.getWorkDirectory().getFullPathName().replace("\\", "/");
	auto scriptsFolder = projectHandler.getSubDirectory(FileHandlerBase::Scripts).getFullPathName().replace("\\", "/");

	project->setProperty(RestApiIds::projectFolder, projectFolder);
	project->setProperty(RestApiIds::scriptsFolder, scriptsFolder);
	result->setProperty(RestApiIds::project, var(project.get()));

	// Script processors
	Array<var> processors;
	Processor::Iterator<JavascriptProcessor> iter(mc->getMainSynthChain());

	while (auto jp = iter.getNextProcessor())
	{
		DynamicObject::Ptr proc = new DynamicObject();
		proc->setProperty(RestApiIds::moduleId, dynamic_cast<Processor*>(jp)->getId());
	
		if (auto jmp = dynamic_cast<JavascriptMidiProcessor*>(jp))
			proc->setProperty(RestApiIds::isMainInterface, jmp->isFront() ? true : false);
		else
			proc->setProperty(RestApiIds::isMainInterface, false);
		
		// External files
		Array<var> externalFilesArr;
		for (int i = 0; i < jp->getNumWatchedFiles(); i++)
		{
			auto fp = jp->getWatchedFile(i);
			auto rp = fp.getRelativePathFrom(scriptsFolder).replaceCharacter('\\', '/');
			externalFilesArr.add(rp);
		}
		proc->setProperty(RestApiIds::externalFiles, externalFilesArr);
	
		Array<var> callbacksArr;
		for (int i = 0; i < jp->getNumSnippets(); i++)
		{
			if (auto snippet = jp->getSnippet(i))
			{
				DynamicObject::Ptr cb = new DynamicObject();
				cb->setProperty(RestApiIds::id, snippet->getCallbackName().toString());
				cb->setProperty(RestApiIds::empty, snippet->isSnippetEmpty());
				callbacksArr.add(var(cb.get()));
			}
		}
		proc->setProperty(RestApiIds::callbacks, callbacksArr);
		processors.add(var(proc.get()));
	}

	result->setProperty(RestApiIds::scriptProcessors, processors);
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleGetScript(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	if (auto jp = getScriptProcessor(mc, req))
	{
		auto callbackParam = req->getRequest()[RestApiIds::callback];
		auto processor = dynamic_cast<Processor*>(jp);
		
		// Build externalFiles array (common to all response types)
		Array<var> externalFilesArr;
		for (int i = 0; i < jp->getNumWatchedFiles(); i++)
		{
			auto fp = jp->getWatchedFile(i);
			DynamicObject::Ptr fileObj = new DynamicObject();
			fileObj->setProperty(RestApiIds::name, fp.getFileName());
			fileObj->setProperty(RestApiIds::path, fp.getFullPathName().replace("\\", "/"));
			externalFilesArr.add(var(fileObj.get()));
		}

		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, true);
		result->setProperty(RestApiIds::moduleId, processor->getId());

		// Always use callbacks object for consistent response structure
		DynamicObject::Ptr callbacksObj = new DynamicObject();

		if (callbackParam.isNotEmpty())
		{
			// Single callback mode - return only the requested callback
			if (auto cb = jp->getSnippet(Identifier(callbackParam)))
			{
				// onInit is raw content, others include function wrapper
				if (callbackParam == "onInit")
					callbacksObj->setProperty(callbackParam, cb->getAllContent());
				else
					callbacksObj->setProperty(callbackParam, cb->getSnippetAsFunction());
			}
			else
				return req->fail(404, "callback " + callbackParam + " not found");
		}
		else
		{
			// All callbacks mode - return all callbacks
			for (int i = 0; i < jp->getNumSnippets(); i++)
			{
				if (auto snippet = jp->getSnippet(i))
				{
					auto cbName = snippet->getCallbackName().toString();
					
					// onInit is raw content, others include function wrapper
					if (cbName == "onInit")
						callbacksObj->setProperty(cbName, snippet->getAllContent());
					else
						callbacksObj->setProperty(cbName, snippet->getSnippetAsFunction());
				}
			}
		}

		result->setProperty(RestApiIds::callbacks, var(callbacksObj.get()));
		result->setProperty(RestApiIds::externalFiles, var(externalFilesArr));
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, Array<var>());

		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}
	
	return req->fail(404, "moduleId is not a valid script processor");
}

RestServer::Response RestHelpers::handleSetScript(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	
	// Check for forceSynchronousExecution debug mode
	bool forceSync = (bool)obj.getProperty(RestApiIds::forceSynchronousExecution, false);
	
	std::unique_ptr<MainController::ScopedBadBabysitter> syncMode;
	if (forceSync)
		syncMode = std::make_unique<MainController::ScopedBadBabysitter>(mc);
	
	// Read moduleId from JSON body
	auto moduleId = obj[RestApiIds::moduleId].toString();
	
	// Helper for structured validation errors (consistent with compilation errors)
	auto validationError = [&](int statusCode, const String& message) -> RestServer::Response
	{
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, false);
		result->setProperty(RestApiIds::moduleId, moduleId);
		// Note: logs and errors are merged by AsyncRequest::mergeLogsIntoResponse()
		
		RestServer::Response response;
		response.statusCode = statusCode;
		response.contentType = "application/json";
		response.body = JSON::toString(var(result.get()));
		
		// Add error via appendError so it gets properly merged
		req->appendError(message);
		req->complete(response);
		return req->waitForResponse();
	};
	
	if (moduleId.isEmpty())
		return validationError(400, "moduleId is required in request body");
	
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));
	
	if (jp == nullptr)
		return validationError(404, "module not found: " + moduleId);
	
	// Read callbacks object
	auto callbacksVar = obj[RestApiIds::callbacks];
	if (!callbacksVar.getDynamicObject())
		return validationError(400, "callbacks must be an object");
	
	auto* callbacksObj = callbacksVar.getDynamicObject();
	auto& props = callbacksObj->getProperties();
	
	if (props.isEmpty())
		return validationError(400, "callbacks object cannot be empty");
	
	Array<var> updatedCallbackNames;
	
	// Validate all callback names first (reject unknown callbacks)
	for (auto& nv: props)
	{
		auto cbName = nv.name.toString();
		auto content = nv.value.toString();

		if (auto snippet = jp->getSnippet(Identifier(cbName)))
		{
			snippet->replaceContentAsync(content);
			updatedCallbackNames.add(cbName);
		}
		else
			return validationError(400, "unknown callback: " + cbName);
	}
	
	// Check if compile is requested (default true)
	if (obj.getProperty(RestApiIds::compile, true))
	{
		// Recompile and return result with updatedCallbacks in response
		mc->getKillStateHandler().killVoicesAndCall(dynamic_cast<Processor*>(jp), 
			[req, forceSync, updatedCallbackNames, moduleId, mc](Processor* p)
		{
			JavascriptProcessor::ResultFunction rf = 
				[req, forceSync, updatedCallbackNames, moduleId, mc, p](const JavascriptProcessor::SnippetResult& result)
			{
				DynamicObject::Ptr r = new DynamicObject();
				r->setProperty(RestApiIds::success, result.r.wasOk());
				r->setProperty(RestApiIds::moduleId, moduleId);
				r->setProperty(RestApiIds::updatedCallbacks, var(updatedCallbackNames));
				r->setProperty(RestApiIds::result, result.r.wasOk() ? "Compiled OK" : "Compilation / Runtime Error");
				r->setProperty(RestApiIds::forceSynchronousExecution, forceSync);
				
				if (forceSync)
					r->setProperty(RestApiIds::warning, "Executed in unsafe synchronous mode - threading checks bypassed");
				
				// Check for LAF render warnings on successful compilation
				if (result.r.wasOk())
				{
					if (auto ps = dynamic_cast<ProcessorWithScriptingContent*>(p))
					{
						if (auto content = ps->getScriptingContent())
							addLafRenderWarningIfNeeded(mc, content, r.get());
					}
				}

				addExternalFilesToResponse(p, r.get());
				req->complete(RestServer::Response::ok(var(r.get())));
			};

			dynamic_cast<JavascriptProcessor*>(p)->compileScript(rf);
			return SafeFunctionCall::OK;
		}, MainController::KillStateHandler::TargetThread::ScriptingThread);
		
		return req->waitForResponse();
	}

	// No compile - return success immediately
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::moduleId, moduleId);
	result->setProperty(RestApiIds::updatedCallbacks, var(updatedCallbackNames));
	result->setProperty(RestApiIds::result, "Updated without compilation");
	addExternalFilesToResponse(dynamic_cast<Processor*>(jp), result.get());
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());
	
	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleListComponents(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	if (auto jp = getScriptProcessor(mc, req))
	{
		auto ps = dynamic_cast<ProcessorWithScriptingContent*>(jp);
		auto hierarchyParam = req->getRequest()[RestApiIds::hierarchy];
		auto useHierarchy = (hierarchyParam == "true" || hierarchyParam == "1");

		auto p = dynamic_cast<Processor*>(jp);
		auto moduleId = p->getId();
		auto scriptRoot = mc->getSampleManager().getProjectHandler().getSubDirectory(FileHandlerBase::Scripts);

		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, true);
		result->setProperty(RestApiIds::moduleId, moduleId);

		Array<var> components;

		auto c = ps->getScriptingContent();
		auto registry = c->getLafRegistry();

		if (useHierarchy)
		{
			for (int i = 0; i < c->getNumComponents(); i++)
			{
				auto sc = c->getComponent(i);
				if (sc->getParentScriptComponent() == nullptr)
				{
					auto obj = createRecursivePropertyTreeWithLaf(sc, registry.get(), scriptRoot, moduleId);
					components.add(obj.get());
				}
			}
		}
		else
		{
			// Flat list: id, type, and laf info
			for(int i = 0; i < c->getNumComponents(); i++)
			{
				auto sc = c->getComponent(i);
				DynamicObject::Ptr comp = new DynamicObject();
				comp->setProperty(RestApiIds::id, sc->getName().toString());
				comp->setProperty(RestApiIds::type, sc->getObjectName().toString());
				comp->setProperty(RestApiIds::laf, getLafInfoForComponent(registry.get(), sc, scriptRoot, moduleId));
				components.add(var(comp.get()));
			}
		}

		result->setProperty(RestApiIds::components, components);
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, Array<var>());

		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}
	else
	{
		return req->fail(404, "moduleId is not a valid script processor");
	}
}

RestServer::Response RestHelpers::handleGetComponentProperties(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	if (auto jp = getScriptProcessor(mc, req))
	{
		auto componentId = req->getRequest()[RestApiIds::id];
		auto c = dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();

		if (componentId.isEmpty())
			return req->fail(400, "id parameter is required");

		if (auto sc = c->getComponentWithName(Identifier(componentId)))
		{
			DynamicObject::Ptr result = new DynamicObject();
			result->setProperty(RestApiIds::success, true);
			result->setProperty(RestApiIds::moduleId, dynamic_cast<Processor*>(jp)->getId());
			result->setProperty(RestApiIds::id, componentId);
			result->setProperty(RestApiIds::type, sc->getObjectName().toString());

			Array<var> properties;

			for (int i = 0; i < sc->getNumIds(); i++)
			{
				DynamicObject::Ptr po = new DynamicObject();

				auto propId = sc->getIdFor(i);
				auto v = sc->getScriptObjectProperty(i);
				auto nonDefault = sc->getNonDefaultScriptObjectProperties();

				if (propId.toString().toLowerCase().contains("colour"))
					v = "0x" + ApiHelpers::getColourFromVar(v).toDisplayString(true);

				auto opts = sc->getOptionsFor(propId);

				po->setProperty(RestApiIds::id, propId.toString());
				po->setProperty(RestApiIds::value, v);
				po->setProperty(RestApiIds::isDefault, !nonDefault.hasProperty(propId));

				if (!opts.isEmpty())
				{
					Array<var> ol;
					for (auto o : opts)
						ol.add(var(o));
					po->setProperty(RestApiIds::options, var(ol));
				}

				properties.add(po.get());
			}

			result->setProperty(RestApiIds::properties, var(properties));
			result->setProperty(RestApiIds::logs, Array<var>());
			result->setProperty(RestApiIds::errors, Array<var>());

			req->complete(RestServer::Response::ok(var(result.get())));
			return req->waitForResponse();
		}
		else
			return req->fail(404, "component not found: " + componentId);
	}
	else
	{
		return req->fail(404, "moduleId is not a valid script processor");
	}
}

RestServer::Response RestHelpers::handleGetComponentValue(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	if (auto jp = getScriptProcessor(mc, req))
	{
		auto componentId = req->getRequest()[RestApiIds::id];
		auto c = dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();

		if (componentId.isEmpty())
			return req->fail(400, "id parameter is required");

		if (auto sc = c->getComponentWithName(Identifier(componentId)))
		{
			DynamicObject::Ptr result = new DynamicObject();
			result->setProperty(RestApiIds::success, true);
			result->setProperty(RestApiIds::moduleId, dynamic_cast<Processor*>(jp)->getId());
			result->setProperty(RestApiIds::id, componentId);
			result->setProperty(RestApiIds::type, sc->getObjectName().toString());
			
			result->setProperty(RestApiIds::value, sc->getValue());
			
			// Include min/max range
			result->setProperty(RestApiIds::min, sc->getScriptObjectProperty(ScriptComponent::min));
			result->setProperty(RestApiIds::max, sc->getScriptObjectProperty(ScriptComponent::max));
			
			result->setProperty(RestApiIds::logs, Array<var>());
			result->setProperty(RestApiIds::errors, Array<var>());

			req->complete(RestServer::Response::ok(var(result.get())));
			return req->waitForResponse();
		}
		else
			return req->fail(404, "component not found: " + componentId);
	}
	else
	{
		return req->fail(404, "moduleId is not a valid script processor");
	}
}

RestServer::Response RestHelpers::handleSetComponentValue(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	
	// Check for forceSynchronousExecution debug mode
	bool forceSync = (bool)obj.getProperty(RestApiIds::forceSynchronousExecution, false);
	
	std::unique_ptr<MainController::ScopedBadBabysitter> syncMode;
	if (forceSync)
		syncMode = std::make_unique<MainController::ScopedBadBabysitter>(mc);
	
	auto moduleId = obj[RestApiIds::moduleId].toString();
	if (moduleId.isEmpty())
		return req->fail(400, "moduleId is required in request body");
	
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));
	
	if (jp == nullptr)
		return req->fail(404, "module not found: " + moduleId);
	
	auto componentId = obj[RestApiIds::id].toString();
	if (componentId.isEmpty())
		return req->fail(400, "id is required in request body");
	
	if (!obj.hasProperty(RestApiIds::value))
		return req->fail(400, "value is required in request body");
	
	auto c = dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();
	
	if (auto sc = c->getComponentWithName(Identifier(componentId)))
	{
		auto newValue = obj[RestApiIds::value];
		auto validateRange = obj.getProperty(RestApiIds::validateRange, false);
		
		if ((bool)validateRange)
		{
			double minVal = (double)sc->getScriptObjectProperty(ScriptComponent::min);
			double maxVal = (double)sc->getScriptObjectProperty(ScriptComponent::max);
			double val = (double)newValue;
			
			if (val < minVal || val > maxVal)
				return req->fail(400, "Value " + String(val) + " is out of range [" + 
								 String(minVal) + ", " + String(maxVal) + "] for component " + componentId);
		}
		
		sc->setValue(newValue);
		sc->changed();
		
		// Wait for callback to complete (pumps message loop)
		waitForPendingCallbacks(sc);

		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, true);
		result->setProperty(RestApiIds::moduleId, moduleId);
		result->setProperty(RestApiIds::id, componentId);
		result->setProperty(RestApiIds::type, sc->getObjectName().toString());
		result->setProperty(RestApiIds::forceSynchronousExecution, forceSync);
		
		if (forceSync)
			result->setProperty(RestApiIds::warning, "Executed in unsafe synchronous mode - threading checks bypassed");

		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}
	else
		return req->fail(404, "component not found: " + componentId);
}

RestServer::Response RestHelpers::handleSetComponentProperties(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	
	// Get moduleId
	auto moduleId = obj[RestApiIds::moduleId].toString();
	if (moduleId.isEmpty())
		return req->fail(400, "moduleId is required in request body");
	
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));
	
	if (jp == nullptr)
		return req->fail(404, "module not found: " + moduleId);
	
	auto content = dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();
	
	// Parse changes array
	auto changesVar = obj[RestApiIds::changes];
	if (!changesVar.isArray() || changesVar.size() == 0)
		return req->fail(400, "changes must be a non-empty array");
	
	bool force = (bool)obj.getProperty(RestApiIds::force, false);
	
	// Phase 1: Validation - collect all locked properties if force=false
	Array<var> lockedProperties;
	static const Identifier parentComponentId("parentComponent");
	
	for (int i = 0; i < changesVar.size(); i++)
	{
		auto change = changesVar[i];
		auto componentId = change[RestApiIds::id].toString();
		
		if (componentId.isEmpty())
			return req->fail(400, "changes[" + String(i) + "].id is required");
		
		auto sc = content->getComponentWithName(Identifier(componentId));
		if (sc == nullptr)
			return req->fail(404, "component not found: " + componentId);
		
		auto propsVar = change[RestApiIds::properties];
		if (!propsVar.getDynamicObject())
			return req->fail(400, "changes[" + String(i) + "].properties must be an object");
		
		auto props = propsVar.getDynamicObject()->getProperties();
		
		for (int j = 0; j < props.size(); j++)
		{
			auto propId = props.getName(j);
			
			// Validate property exists
			if (!sc->hasProperty(propId))
				return req->fail(400, "property '" + propId.toString() + 
								 "' does not exist on component '" + componentId + "'");
			
			// Check for script-lock (unless force=true)
			if (!force && sc->isPropertyOverwrittenByScript(propId))
			{
				DynamicObject::Ptr lockedEntry = new DynamicObject();
				lockedEntry->setProperty(RestApiIds::id, componentId);
				lockedEntry->setProperty(RestApiIds::property, propId.toString());
				lockedProperties.add(var(lockedEntry.get()));
			}
			
			// Validate parentComponent target exists
			if (propId == parentComponentId)
			{
				auto targetParent = props.getValueAt(j).toString();
				if (targetParent.isNotEmpty())
				{
					if (content->getComponentWithName(Identifier(targetParent)) == nullptr)
					{
						return req->fail(400, "parentComponent target '" + targetParent + 
										 "' does not exist for component '" + componentId + "'");
					}
				}
			}
		}
	}
	
	// If any properties are locked, fail with details
	if (lockedProperties.size() > 0)
	{
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, false);
		result->setProperty(RestApiIds::errorMessage, "Properties are locked by script (use force=true to override)");
		result->setProperty(RestApiIds::locked, var(lockedProperties));
		
		RestServer::Response response;
		response.statusCode = 400;
		response.contentType = "application/json";
		response.body = JSON::toString(var(result.get()));
		
		req->complete(response);
		return req->waitForResponse();
	}
	
	// Phase 2: Apply all changes
	Array<var> appliedChanges;
	bool recompileRequired = false;
	
	ValueTreeUpdateWatcher::ScopedDelayer sd(content->getUpdateWatcher(), true);

	for (int i = 0; i < changesVar.size(); i++)
	{
		auto change = changesVar[i];
		auto componentId = change[RestApiIds::id].toString();
		auto sc = content->getComponentWithName(Identifier(componentId));
		auto props = change[RestApiIds::properties].getDynamicObject()->getProperties();
		
		// Use ScopedPropertyEnabler to bypass script tracking (like paste action)
		ScriptComponent::ScopedPropertyEnabler spe(sc);
		
		Array<var> appliedProps;
		
		for (int j = 0; j < props.size(); j++)
		{
			auto propId = props.getName(j);
			auto newValue = props.getValueAt(j);
			
			// Track parentComponent changes
			if (propId == parentComponentId)
				recompileRequired = true;
			
			// Handle colour conversion if needed
			if (propId.toString().toLowerCase().contains("colour"))
			{
				auto colour = ApiHelpers::getColourFromVar(newValue);
				newValue = (int64)colour.getARGB();
			}
			
			sc->setScriptObjectPropertyWithChangeMessage(propId, newValue, sendNotificationAsync);
			appliedProps.add(propId.toString());
		}
		
		DynamicObject::Ptr appliedEntry = new DynamicObject();
		appliedEntry->setProperty(RestApiIds::id, componentId);
		appliedEntry->setProperty(RestApiIds::properties, var(appliedProps));
		appliedChanges.add(var(appliedEntry.get()));
	}
	
	// Build success response
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::moduleId, moduleId);
	result->setProperty(RestApiIds::applied, var(appliedChanges));
	result->setProperty(RestApiIds::recompileRequired, recompileRequired);
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());
	
	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleScreenshot(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	// Parse parameters
	auto moduleId = req->getRequest()[RestApiIds::moduleId];
	if (moduleId.isEmpty())
		moduleId = "Interface";
	
	auto componentId = req->getRequest()[RestApiIds::id];
	
	auto scaleStr = req->getRequest()[RestApiIds::scale];
	float scale = scaleStr.isNotEmpty() ? scaleStr.getFloatValue() : 1.0f;
	
	// Validate scale (only 0.5 or 1.0 allowed)
	if (scale != 0.5f && scale != 1.0f)
		scale = 1.0f;
	
	// Parse optional outputPath for file output mode
	auto outputPath = req->getRequest()[RestApiIds::outputPath];
	
	if (outputPath.isNotEmpty())
	{
		// Validate .png extension (case-insensitive)
		if (!outputPath.endsWithIgnoreCase(".png"))
			return req->fail(400, "outputPath must end with .png extension");
		
		// Validate parent directory exists
		File outputFile(outputPath);
		File parentDir = outputFile.getParentDirectory();
		
		if (!parentDir.isDirectory())
			return req->fail(400, "Parent directory does not exist: " + parentDir.getFullPathName());
	}
	
	// Find the script processor
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));
	
	if (jp == nullptr)
		return req->fail(404, "module not found: " + moduleId);
	
	auto content = dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();
	
	// If component ID specified, validate it exists
	Rectangle<int> cropBounds;
	Rectangle<int> fullBounds = { 0, 0, content->getContentWidth(), content->getContentHeight() };
	bool cropToComponent = false;
	
	if (componentId.isNotEmpty())
	{
		auto sc = content->getComponentWithName(Identifier(componentId));

		if (sc == nullptr)
			return req->fail(404, "component not found: " + componentId);
		
		auto x = sc->getGlobalPositionX();
		auto y = sc->getGlobalPositionY();

		cropBounds = { x, y, (int)sc->getScriptObjectProperty("width"), sc->getScriptObjectProperty("height") };
		cropToComponent = true;
	}
	else
	{
		cropBounds = fullBounds;
	}
	
#if 0
	 ============================================
	 TODO: Christoph implements capture logic here
	 ============================================
	 
	 Requirements:
	 - Find the ScriptContentComponent (the actual UI component on screen)
	 - Marshal to message thread for capture
	 - Use Component::createComponentSnapshot(bounds, true, scale)
	 - If componentId specified, crop to that component bounds
	 - 1 second timeout - return error if exceeded
	 - Handle case where UI is mid-compilation gracefully
	
	 Suggested structure:
#endif
	
	Image capturedImage;
	bool captureSuccess = false;
	WaitableEvent captureComplete;
	
	auto sp = dynamic_cast<ProcessorWithScriptingContent*>(jp);

	SafeAsyncCall::callAsyncIfNotOnMessageThread<ProcessorWithScriptingContent>(*sp, [&](ProcessorWithScriptingContent& spp)
	{
		hise::ScriptContentComponent component(&spp);
		component.setBounds(fullBounds);
		capturedImage = component.createComponentSnapshot(cropBounds, true, scale);
		captureSuccess = capturedImage.isValid();
		captureComplete.signal();
	});
	
	if (!captureComplete.wait(1000))
	    return req->fail(500, "screenshot capture timed out");
	
	if (!captureSuccess)
	    return req->fail(500, "failed to capture screenshot");
	
	// ============================================
	// End of Christoph's section
	// ============================================
	
	if (!captureSuccess)
		return req->fail(500, "screenshot capture not yet implemented");
	
	if (!capturedImage.isValid())
		return req->fail(500, "failed to capture screenshot");
	
	// Build response
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::moduleId, moduleId);
	
	if (componentId.isNotEmpty())
		result->setProperty(RestApiIds::id, componentId);
	
	result->setProperty(RestApiIds::width, capturedImage.getWidth());
	result->setProperty(RestApiIds::height, capturedImage.getHeight());
	result->setProperty(RestApiIds::scale, scale);
	
	if (outputPath.isNotEmpty())
	{
		// File output mode: write PNG to file
		File outputFile(outputPath);
		
		// Delete existing file to ensure clean overwrite (FileOutputStream doesn't truncate)
		if (outputFile.existsAsFile())
			outputFile.deleteFile();
		
		FileOutputStream fos(outputFile);
		
		if (fos.failedToOpen())
			return req->fail(500, "failed to open output file: " + outputPath);
		
		PNGImageFormat pngFormat;
		if (!pngFormat.writeImageToStream(capturedImage, fos))
			return req->fail(500, "failed to write PNG to file");
		
		result->setProperty(RestApiIds::filePath, outputFile.getFullPathName());
	}
	else
	{
		// Base64 output mode: encode to base64 PNG
		MemoryBlock mb;
		{
			MemoryOutputStream mos(mb, false);
			PNGImageFormat pngFormat;
			if (!pngFormat.writeImageToStream(capturedImage, mos))
				return req->fail(500, "failed to encode PNG");
		}
		
		auto base64 = Base64::toBase64(mb.getData(), mb.getSize());
		result->setProperty(RestApiIds::imageData, base64);
	}
	
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());
	
	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleGetSelectedComponents(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	// Get moduleId (default to "Interface")
	auto moduleId = req->getRequest()[RestApiIds::moduleId];
	if (moduleId.isEmpty())
		moduleId = "Interface";
	
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));
	
	if (jp == nullptr)
		return req->fail(404, "module not found: " + moduleId);
	
	// Get the selection from the broadcaster
	auto broadcaster = mc->getScriptComponentEditBroadcaster();
	auto selection = broadcaster->getSelection();
	
	// Build response
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::moduleId, moduleId);
	result->setProperty(RestApiIds::selectionCount, selection.size());
	
	Array<var> components;
	
	for (auto sc : selection)
	{
		if (sc == nullptr)
			continue;
		
		DynamicObject::Ptr comp = new DynamicObject();
		comp->setProperty(RestApiIds::id, sc->getName().toString());
		comp->setProperty(RestApiIds::type, sc->getObjectName().toString());
		
		// Include all properties (same format as get_component_properties)
		Array<var> properties;
		
		for (int i = 0; i < sc->getNumIds(); i++)
		{
			DynamicObject::Ptr po = new DynamicObject();
			
			auto propId = sc->getIdFor(i);
			auto v = sc->getScriptObjectProperty(i);
			auto nonDefault = sc->getNonDefaultScriptObjectProperties();
			
			if (propId.toString().toLowerCase().contains("colour"))
				v = "0x" + ApiHelpers::getColourFromVar(v).toDisplayString(true);
			
			auto opts = sc->getOptionsFor(propId);
			
			po->setProperty(RestApiIds::id, propId.toString());
			po->setProperty(RestApiIds::value, v);
			po->setProperty(RestApiIds::isDefault, !nonDefault.hasProperty(propId));
			
			if (!opts.isEmpty())
			{
				Array<var> ol;
				for (auto o : opts)
					ol.add(var(o));
				po->setProperty(RestApiIds::options, var(ol));
			}
			
			properties.add(po.get());
		}
		
		comp->setProperty(RestApiIds::properties, var(properties));
		components.add(var(comp.get()));
	}
	
	result->setProperty(RestApiIds::components, components);
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());
	
	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleSimulateInteractions(BackendProcessor* bp, RestServer::AsyncRequest::Ptr req)
{
	// Capture console output during interaction execution
	ScopedConsoleHandler consoleHandler(bp, req);
	
	// Parse the request body to get interactions array
	auto body = req->getRequest().getJsonBody();
	
	if (body.isUndefined() || body.isVoid())
		return req->fail(400, "Request body is required");
	
	auto interactionsVar = body.getProperty(RestApiIds::interactions, var());
	
	if (!interactionsVar.isArray())
		return req->fail(400, "'interactions' must be an array");
	
	// Parse verbose flag (default false)
	bool verbose = (bool)body.getProperty(RestApiIds::verbose, false);
	
	// Get the interaction tester (only available when REST server is running)
	auto* tester = bp->getInteractionTester();
	
	if (tester == nullptr)
		return req->fail(503, "InteractionTester not available - REST server may have been stopped");
	
	// Execute on message thread
	InteractionTester::TestResult testResult;
	bool executed = false;
	WaitableEvent completed;
	
	SafeAsyncCall::callAsyncIfNotOnMessageThread<BackendProcessor>(*bp, [&, verbose](BackendProcessor& processor)
	{
		auto* t = processor.getInteractionTester();
		if (t != nullptr)
		{
			testResult = t->executeInteractions(interactionsVar, verbose);
			executed = true;
		}
		completed.signal();
	});
	
	// Wait for execution with timeout (30 seconds should be plenty for any interaction sequence)
	if (!completed.wait(30000))
		return req->fail(500, "Interaction execution timed out");
	
	if (!executed)
		return req->fail(500, "Failed to execute interactions");
	
	// Build response
	DynamicObject::Ptr result = new DynamicObject();
	
	// If any script errors were captured during execution, mark as failed
	bool success = testResult.success && !req->hasErrors();
	result->setProperty(RestApiIds::success, success);
	
	if (!success)
	{
		if (testResult.errorMessage.isNotEmpty())
			result->setProperty(RestApiIds::errorMessage, testResult.errorMessage);
		else if (req->hasErrors())
			result->setProperty(RestApiIds::errorMessage, "Script error(s) occurred during execution");
	}
	
	result->setProperty(RestApiIds::interactionsCompleted, testResult.interactionsCompleted);
	result->setProperty(RestApiIds::totalElapsedMs, testResult.totalElapsedMs);
	result->setProperty(RestApiIds::executionLog, testResult.executionLog);
	
	// Convert screenshots to JSON object with metadata (no base64 data)
	DynamicObject::Ptr screenshotsObj = new DynamicObject();
	for (const auto& [id, info] : testResult.screenshots)
	{
		DynamicObject::Ptr ssInfo = new DynamicObject();
		ssInfo->setProperty(RestApiIds::id, info.id);
		ssInfo->setProperty("sizeKB", info.sizeKB);
		ssInfo->setProperty("width", info.width);
		ssInfo->setProperty("height", info.height);
		screenshotsObj->setProperty(Identifier(id), var(ssInfo.get()));
	}
	result->setProperty(RestApiIds::screenshots, var(screenshotsObj.get()));
	
	// Add parse warnings if any
	Array<var> warnings;
	for (auto& w : testResult.parseWarnings)
		warnings.add(w);
	result->setProperty(RestApiIds::parseWarnings, var(warnings));
	
	// Add selected menu item info if a menu item was selected
	if (testResult.selectedMenuItem.wasSelected)
	{
		DynamicObject::Ptr menuItemObj = new DynamicObject();
		menuItemObj->setProperty(RestApiIds::text, testResult.selectedMenuItem.text);
		menuItemObj->setProperty(RestApiIds::itemId, testResult.selectedMenuItem.itemId);
		result->setProperty(RestApiIds::selectedMenuItem, var(menuItemObj.get()));
	}
	
	// Add mouse state when verbose=true
	if (verbose)
	{
		DynamicObject::Ptr mouseStateObj = new DynamicObject();
		mouseStateObj->setProperty(RestApiIds::currentTarget, testResult.finalMouseState.currentTarget.toString());
		
		DynamicObject::Ptr pixelPosObj = new DynamicObject();
		pixelPosObj->setProperty(RestApiIds::x, testResult.finalMouseState.pixelPosition.x);
		pixelPosObj->setProperty(RestApiIds::y, testResult.finalMouseState.pixelPosition.y);
		mouseStateObj->setProperty(RestApiIds::pixelPosition, var(pixelPosObj.get()));
		
		result->setProperty(RestApiIds::mouseState, var(mouseStateObj.get()));
	}
	
	// Complete the request - this merges logs/errors via mergeLogsIntoResponse()
	req->complete(RestServer::Response::ok(var(result.get())));
	auto finalResponse = req->waitForResponse();
	
	// Log the final response (which now includes logs/errors) to the interaction tester's console
	if (auto* t = bp->getInteractionTester())
		t->logResponse(interactionsVar, finalResponse.body, success);
	
	return finalResponse;
}


/** Finds the JavascriptProcessor that includes a given external file.
    Walks all JavascriptProcessor instances in the module tree and checks their watched files. */
static JavascriptProcessor* findProcessorForFile(MainController* mc, const File& targetFile)
{
	Processor::Iterator<JavascriptProcessor> iter(mc->getMainSynthChain());

	while (auto* jp = iter.getNextProcessor())
	{
		for (int i = 0; i < jp->getNumWatchedFiles(); i++)
		{
			if (jp->getWatchedFile(i) == targetFile)
				return jp;
		}
	}

	return nullptr;
}

RestServer::Response RestHelpers::handleDiagnoseScript(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	
	auto filePathStr = obj.getProperty(RestApiIds::filePath, "").toString();
	auto moduleIdStr = obj.getProperty(RestApiIds::moduleId, "").toString();
	
	// Resolve the target file
	File targetFile;
	
	if (filePathStr.isNotEmpty())
	{
		if (File::isAbsolutePath(filePathStr))
			targetFile = File(filePathStr);
		else
			targetFile = mc->getSampleManager().getProjectHandler()
				.getSubDirectory(FileHandlerBase::Scripts)
				.getChildFile(filePathStr);
	}
	
	// Resolve the processor
	JavascriptProcessor* jp = nullptr;
	
	if (moduleIdStr.isNotEmpty())
	{
		// moduleId provided — use it to find the processor
		jp = getScriptProcessor(mc, req);
		
		if (jp == nullptr)
			return req->fail(404, "moduleId is not a valid script processor");
		
		if (filePathStr.isEmpty())
		{
			// moduleId only, no filePath — need to pick a file
			// Use the first external file if available
			if (jp->getNumWatchedFiles() > 0)
			{
				targetFile = jp->getWatchedFile(0);
			}
			else
			{
				return req->fail(400, "filePath is required (this processor has no external files)");
			}
		}
		else if (!targetFile.existsAsFile())
		{
			return req->fail(404, "File not found: " + targetFile.getFullPathName());
		}
	}
	else if (filePathStr.isNotEmpty())
	{
		// filePath only — resolve the owning processor
		if (!targetFile.existsAsFile())
			return req->fail(404, "File not found: " + targetFile.getFullPathName());
		
		jp = findProcessorForFile(mc, targetFile);
		
		if (jp == nullptr)
			return req->fail(404, "No script processor includes this file. "
								  "Has it been compiled at least once (F5)?");
	}
	else
	{
		return req->fail(400, "Either moduleId or filePath must be provided");
	}
	
	// Read file from disk and run shadow parse
	auto code = targetFile.loadFileAsString();
	auto fileName = targetFile.getFullPathName();
	auto resolvedModuleId = dynamic_cast<Processor*>(jp)->getId();
	auto normalizedFilePath = fileName.replace("\\", "/");
	
	auto useAsync = (bool)obj.getProperty(RestApiIds::async, false);
	
	// Shared lambda for building the diagnostics JSON array
	auto buildDiagArray = [](const JavascriptProcessor::DiagnosticList& diagnostics)
	{
		Array<var> diagArray;
		
		for (const auto& d : diagnostics)
		{
			DynamicObject::Ptr dObj = new DynamicObject();
			dObj->setProperty(RestApiIds::line, d.line);
			dObj->setProperty(RestApiIds::column, d.col);
			dObj->setProperty(RestApiIds::severity, ApiClass::DiagnosticResult::getSeverityString(d.severity));
			dObj->setProperty(RestApiIds::source, ApiClass::DiagnosticResult::getClassificationString(d.classification));
			dObj->setProperty(RestApiIds::message, d.message);
			
			if (!d.suggestions.isEmpty())
			{
				Array<var> sugArr;
				for (const auto& s : d.suggestions)
					sugArr.add(s);
				dObj->setProperty(RestApiIds::suggestions, var(sugArr));
			}
			
			diagArray.add(var(dObj.get()));
		}
		
		return diagArray;
	};
	
	auto buildResponse = [resolvedModuleId, normalizedFilePath](const Array<var>& diagArray)
	{
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, true);
		result->setProperty(RestApiIds::moduleId, resolvedModuleId);
		result->setProperty(RestApiIds::filePath, normalizedFilePath);
		result->setProperty(RestApiIds::diagnostics, var(diagArray));
		return RestServer::Response::ok(var(result.get()));
	};
	
	if (useAsync)
	{
		// Async path: defer to scripting thread, callback completes the request on the message thread.
		jp->shadowParseFile(code, fileName, 
			[req, buildDiagArray, buildResponse](const JavascriptProcessor::DiagnosticList& diagnostics)
		{
			req->complete(buildResponse(buildDiagArray(diagnostics)));
		}, sendNotificationAsync);
		
		return req->waitForResponse();
	}
	
	// Sync path (default): run directly on the calling thread.
	Array<var> diagArray;
	
	jp->shadowParseFile(code, fileName, 
		[&diagArray, &buildDiagArray](const JavascriptProcessor::DiagnosticList& diagnostics)
	{
		diagArray = buildDiagArray(diagnostics);
	}, sendNotificationSync);
	
	return buildResponse(diagArray);
}

RestServer::Response RestHelpers::handleGetIncludedFiles(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	auto moduleId = req->getRequest()[RestApiIds::moduleId];
	
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	
	if (moduleId.isNotEmpty())
	{
		// Filtered: return files for this specific processor
		auto* jp = dynamic_cast<JavascriptProcessor*>(
			ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));
		
		if (jp == nullptr)
			return req->fail(404, "module not found: " + moduleId);
		
		result->setProperty(RestApiIds::moduleId, moduleId);
		
		Array<var> fileArray;
		
		for (int i = 0; i < jp->getNumWatchedFiles(); i++)
			fileArray.add(jp->getWatchedFile(i).getFullPathName().replace("\\", "/"));
		
		result->setProperty(RestApiIds::files, var(fileArray));
	}
	else
	{
		// Global: return all files across all processors with owning processor names
		Array<File> files;
		StringArray processors;
		
		mc->fillExternalFileList(files, processors);
		
		Array<var> fileArray;
		
		for (int i = 0; i < files.size(); i++)
		{
			DynamicObject::Ptr entry = new DynamicObject();
			entry->setProperty(RestApiIds::path, files[i].getFullPathName().replace("\\", "/"));
			entry->setProperty(RestApiIds::processor, processors[i]);
			fileArray.add(var(entry.get()));
		}
		
		result->setProperty(RestApiIds::files, var(fileArray));
	}
	
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());
	
	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

//==============================================================================
// Profiling endpoint helpers and handler

#if HISE_INCLUDE_PROFILING_TOOLKIT

bool RestHelpers::startProfilingSession(MainController* mc, const var& bodyJson,
                                         double defaultDurationMs)
{
	auto& dh = mc->getDebugSession();

	if (dh.isRecordingMultithread())
		return false;

	double durationMs = jlimit(100.0, 5000.0,
		(double)bodyJson.getProperty(RestApiIds::durationMs, defaultDurationMs));

	// Build options object matching DebugSession::Options::fromDynamicObject format
	DynamicObject::Ptr optionsObj = new DynamicObject();
	optionsObj->setProperty("recordingTrigger", 0);  // Manual trigger
	optionsObj->setProperty("recordingLength", String((int)durationMs) + " ms");

	// Thread filter
	auto tfProp = bodyJson.getProperty(RestApiIds::threadFilter, var());

	if (tfProp.isArray())
	{
		optionsObj->setProperty("threadFilter", tfProp);
	}
	else
	{
		Array<var> allThreads;

		for (int i = 0; i < (int)DebugSession::ThreadIdentifier::Type::numTypes; i++)
			allThreads.add(DebugSession::ThreadIdentifier::getThreadName(
				(DebugSession::ThreadIdentifier::Type)i));

		optionsObj->setProperty("threadFilter", var(allThreads));
	}

	// Event filter
	auto efProp = bodyJson.getProperty(RestApiIds::eventFilter, var());

	if (efProp.isArray())
	{
		optionsObj->setProperty("eventFilter", efProp);
	}
	else
	{
		Array<var> allEvents;

		for (int i = 0; i < (int)DebugSession::ProfileDataSource::SourceType::numSourceTypes; i++)
			allEvents.add(DebugSession::ProfileDataSource::getSourceTypeName(
				(DebugSession::ProfileDataSource::SourceType)i));

		optionsObj->setProperty("eventFilter", var(allEvents));
	}

	dh.setOptions(var(optionsObj.get()));
	dh.startRecording(durationMs, nullptr);
	return true;
}

/** File-local: a flow endpoint collected during tree traversal. */
struct FlowEndpoint
{
	int trackId;
	String eventName;
	String threadName;
};

/** File-local: build matched flows array from collected flow endpoints. */
static Array<var> buildMatchedFlows(
	const std::vector<FlowEndpoint>& flowSources,
	const std::vector<FlowEndpoint>& flowTargets)
{
	Array<var> flowsArray;

	for (size_t s = 0; s < flowSources.size(); s++)
	{
		for (size_t t = 0; t < flowTargets.size(); t++)
		{
			if (flowSources[s].trackId == flowTargets[t].trackId)
			{
				DynamicObject::Ptr flowObj = new DynamicObject();
				flowObj->setProperty(RestApiIds::trackId, flowSources[s].trackId);
				flowObj->setProperty(RestApiIds::sourceEvent, flowSources[s].eventName);
				flowObj->setProperty(RestApiIds::sourceThread, flowSources[s].threadName);
				flowObj->setProperty(RestApiIds::targetEvent, flowTargets[t].eventName);
				flowObj->setProperty(RestApiIds::targetThread, flowTargets[t].threadName);
				flowsArray.add(var(flowObj.get()));
				break;
			}
		}
	}

	return flowsArray;
}

/** File-local helper: convert a single ProfileInfo event to JSON recursively.
    Also collects flow endpoints (trackSource/trackTarget) into the provided vectors. */
static var profileEventToJson(DebugSession::ProfileDataSource::ProfileInfo* event,
                              const String& threadName,
                              std::vector<FlowEndpoint>& flowSources,
                              std::vector<FlowEndpoint>& flowTargets)
{
	DynamicObject::Ptr obj = new DynamicObject();

	String eventName;

	if (event->data.source != nullptr)
	{
		eventName = event->data.source->name;
		obj->setProperty(RestApiIds::name, eventName);
		obj->setProperty(RestApiIds::sourceType,
			DebugSession::ProfileDataSource::getSourceTypeName(event->data.source->sourceType));
	}

	obj->setProperty(RestApiIds::start, event->data.start);
	obj->setProperty(RestApiIds::duration, event->data.delta);

	int ts = event->data.trackSource;
	int tt = event->data.trackTarget;

	if (ts != -1)
	{
		obj->setProperty(RestApiIds::trackSource, ts);
		flowSources.push_back({ ts, eventName, threadName });
	}

	if (tt != -1)
	{
		obj->setProperty(RestApiIds::trackTarget, tt);
		flowTargets.push_back({ tt, eventName, threadName });
	}

	Array<var> childArray;

	for (int i = 0; i < event->children.size(); i++)
	{
		auto pi = dynamic_cast<DebugSession::ProfileDataSource::ProfileInfo*>(event->children[i].get());

		if (pi != nullptr)
			childArray.add(profileEventToJson(pi, threadName, flowSources, flowTargets));
	}

	obj->setProperty(RestApiIds::children, var(childArray));

	return var(obj.get());
}

var RestHelpers::profilingResultToJson(
	DebugSession::ProfileDataSource::ProfileInfoBase* root)
{
	using ProfileInfo = DebugSession::ProfileDataSource::ProfileInfo;
	using CombinedRoot = DebugSession::ProfileDataSource::CombinedRoot;

	DynamicObject::Ptr resultObj = new DynamicObject();
	Array<var> threadsArray;
	std::vector<FlowEndpoint> flowSources;
	std::vector<FlowEndpoint> flowTargets;

	if (root == nullptr)
	{
		resultObj->setProperty(RestApiIds::threads, var(threadsArray));
		resultObj->setProperty(RestApiIds::flows, var(Array<var>()));
		return var(resultObj.get());
	}

	for (int i = 0; i < root->children.size(); i++)
	{
		auto* child = root->children[i].get();

		// CombinedRoot = one per recorded thread
		auto combined = dynamic_cast<CombinedRoot*>(child);

		if (combined != nullptr)
		{
			DynamicObject::Ptr threadObj = new DynamicObject();
			String threadName = DebugSession::ThreadIdentifier::getThreadName(combined->threadType);
			threadObj->setProperty(RestApiIds::thread, threadName);

			Array<var> eventsArray;

			for (int j = 0; j < combined->children.size(); j++)
			{
				auto pi = dynamic_cast<ProfileInfo*>(combined->children[j].get());

				if (pi != nullptr)
					eventsArray.add(profileEventToJson(pi, threadName, flowSources, flowTargets));
			}

			threadObj->setProperty(RestApiIds::events, var(eventsArray));
			threadsArray.add(var(threadObj.get()));
			continue;
		}

		// Direct ProfileInfo at root level (single-thread recording)
		auto pi = dynamic_cast<ProfileInfo*>(child);

		if (pi != nullptr)
		{
			DynamicObject::Ptr threadObj = new DynamicObject();
			String threadName = DebugSession::ThreadIdentifier::getThreadName(pi->data.threadType);
			threadObj->setProperty(RestApiIds::thread, threadName);

			Array<var> eventsArray;
			eventsArray.add(profileEventToJson(pi, threadName, flowSources, flowTargets));
			threadObj->setProperty(RestApiIds::events, var(eventsArray));
			threadsArray.add(var(threadObj.get()));
		}
	}

	resultObj->setProperty(RestApiIds::threads, var(threadsArray));
	resultObj->setProperty(RestApiIds::flows, var(buildMatchedFlows(flowSources, flowTargets)));

	return var(resultObj.get());
}

/** File-local: a collected event from the profiling tree for filtering/aggregation. */
struct CollectedEvent
{
	String name;
	String sourceType;
	String thread;
	double start;
	double duration;
	int trackSource = -1;
	int trackTarget = -1;
	DebugSession::ProfileDataSource::ProfileInfo* profileInfo = nullptr;  // for nested output
};

/** File-local: recursively collect events matching the query options.
    Always recurses the full tree; matched events are added to collected. */
static void collectMatchingEvents(
	DebugSession::ProfileDataSource::ProfileInfo* event,
	const String& threadName,
	const RestHelpers::ProfileQueryOptions& options,
	std::vector<CollectedEvent>& collected,
	std::vector<FlowEndpoint>& flowSources,
	std::vector<FlowEndpoint>& flowTargets)
{
	String eventName;
	String sourceTypeName;

	if (event->data.source != nullptr)
	{
		eventName = event->data.source->name;
		sourceTypeName = DebugSession::ProfileDataSource::getSourceTypeName(
			event->data.source->sourceType);
	}

	// Skip profiler self-cost events
	if (eventName == "Processing profile data")
		return;

	double dur = event->data.delta;
	bool matches = true;

	if (options.filter.isNotEmpty() && !eventName.matchesWildcard(options.filter, false))
		matches = false;

	if (matches && options.sourceTypeFilter.isNotEmpty()
		&& !sourceTypeName.matchesWildcard(options.sourceTypeFilter, false))
		matches = false;

	if (matches && options.minDuration > 0.0 && dur < options.minDuration)
		matches = false;

	if (matches && eventName.isNotEmpty())
	{
		CollectedEvent ce;
		ce.name = eventName;
		ce.sourceType = sourceTypeName;
		ce.thread = threadName;
		ce.start = event->data.start;
		ce.duration = dur;
		ce.trackSource = event->data.trackSource;
		ce.trackTarget = event->data.trackTarget;
		ce.profileInfo = event;
		collected.push_back(ce);

		if (ce.trackSource != -1)
			flowSources.push_back({ ce.trackSource, eventName, threadName });

		if (ce.trackTarget != -1)
			flowTargets.push_back({ ce.trackTarget, eventName, threadName });
	}

	// Always recurse to find deeper matches (even if this node matched)
	for (int i = 0; i < event->children.size(); i++)
	{
		auto pi = dynamic_cast<DebugSession::ProfileDataSource::ProfileInfo*>(
			event->children[i].get());

		if (pi != nullptr)
			collectMatchingEvents(pi, threadName, options, collected, flowSources, flowTargets);
	}
}

var RestHelpers::profilingResultToSummary(
	DebugSession::ProfileDataSource::ProfileInfoBase* root,
	const ProfileQueryOptions& options)
{
	using ProfileInfo = DebugSession::ProfileDataSource::ProfileInfo;
	using CombinedRoot = DebugSession::ProfileDataSource::CombinedRoot;

	DynamicObject::Ptr resultObj = new DynamicObject();

	if (root == nullptr)
	{
		resultObj->setProperty(RestApiIds::results, var(Array<var>()));
		resultObj->setProperty(RestApiIds::flows, var(Array<var>()));
		return var(resultObj.get());
	}

	// Collect all matching events across all threads
	std::vector<CollectedEvent> collected;
	std::vector<FlowEndpoint> flowSources;
	std::vector<FlowEndpoint> flowTargets;

	for (int i = 0; i < root->children.size(); i++)
	{
		auto* child = root->children[i].get();
		auto combined = dynamic_cast<CombinedRoot*>(child);

		if (combined != nullptr)
		{
			String threadName = DebugSession::ThreadIdentifier::getThreadName(combined->threadType);

			// Skip entire thread if threadFilter is active and doesn't include it
			if (options.threadFilter.size() > 0 && !options.threadFilter.contains(threadName))
				continue;

			for (int j = 0; j < combined->children.size(); j++)
			{
				auto pi = dynamic_cast<ProfileInfo*>(combined->children[j].get());

				if (pi != nullptr)
					collectMatchingEvents(pi, threadName, options, collected, flowSources, flowTargets);
			}

			continue;
		}

		auto pi = dynamic_cast<ProfileInfo*>(child);

		if (pi != nullptr)
		{
			String threadName = DebugSession::ThreadIdentifier::getThreadName(pi->data.threadType);

			if (options.threadFilter.size() > 0 && !options.threadFilter.contains(threadName))
				continue;

			collectMatchingEvents(pi, threadName, options, collected, flowSources, flowTargets);
		}
	}

	Array<var> resultsArray;

	if (options.summary)
	{
		// Group by name + sourceType + thread, compute stats
		struct AggKey
		{
			String name;
			String sourceType;
			String thread;

			bool operator==(const AggKey& other) const
			{
				return name == other.name && sourceType == other.sourceType
					   && thread == other.thread;
			}
		};

		struct AggGroup
		{
			AggKey key;
			std::vector<double> durations;
		};

		std::vector<AggGroup> groups;

		for (size_t i = 0; i < collected.size(); i++)
		{
			const auto& ce = collected[i];
			AggKey key = { ce.name, ce.sourceType, ce.thread };

			bool found = false;

			for (size_t g = 0; g < groups.size(); g++)
			{
				if (groups[g].key == key)
				{
					groups[g].durations.push_back(ce.duration);
					found = true;
					break;
				}
			}

			if (!found)
			{
				AggGroup newGroup;
				newGroup.key = key;
				newGroup.durations.push_back(ce.duration);
				groups.push_back(std::move(newGroup));
			}
		}

		// Build result entries with stats
		struct SortEntry
		{
			DynamicObject::Ptr obj;
			double total;
		};

		std::vector<SortEntry> entries;

		for (size_t g = 0; g < groups.size(); g++)
		{
			auto& group = groups[g];
			auto& durs = group.durations;
			int cnt = (int)durs.size();

			double total = 0.0;
			double peak = 0.0;
			double minVal = durs[0];

			for (size_t d = 0; d < durs.size(); d++)
			{
				total += durs[d];

				if (durs[d] > peak)
					peak = durs[d];

				if (durs[d] < minVal)
					minVal = durs[d];
			}

			// Median via nth_element
			size_t mid = durs.size() / 2;
			std::nth_element(durs.begin(), durs.begin() + mid, durs.end());
			double medianVal = durs[mid];

			DynamicObject::Ptr obj = new DynamicObject();
			obj->setProperty(RestApiIds::name, group.key.name);
			obj->setProperty(RestApiIds::sourceType, group.key.sourceType);
			obj->setProperty(RestApiIds::thread, group.key.thread);
			obj->setProperty(RestApiIds::count, cnt);
			obj->setProperty(RestApiIds::median, medianVal);
			obj->setProperty(RestApiIds::peak, peak);
			obj->setProperty(RestApiIds::min, minVal);
			obj->setProperty(RestApiIds::total, total);

			SortEntry se;
			se.obj = obj;
			se.total = total;
			entries.push_back(se);
		}

		// Sort by total descending
		std::sort(entries.begin(), entries.end(),
			[](const SortEntry& a, const SortEntry& b) { return a.total > b.total; });

		int cap = jmin((int)entries.size(), options.limit);

		for (int i = 0; i < cap; i++)
			resultsArray.add(var(entries[i].obj.get()));
	}
	else
	{
		// Non-summary filtered mode: flat list sorted by duration descending
		std::sort(collected.begin(), collected.end(),
			[](const CollectedEvent& a, const CollectedEvent& b)
			{ return a.duration > b.duration; });

		int cap = jmin((int)collected.size(), options.limit);

		for (int i = 0; i < cap; i++)
		{
			const auto& ce = collected[i];

			if (options.nested && ce.profileInfo != nullptr)
			{
				// Build full subtree using profileEventToJson (captures child flows too)
				var eventVar = profileEventToJson(ce.profileInfo, ce.thread,
					flowSources, flowTargets);
				auto eventObj = eventVar.getDynamicObject();

				if (eventObj != nullptr)
					eventObj->setProperty(RestApiIds::thread, ce.thread);

				resultsArray.add(eventVar);
			}
			else
			{
				DynamicObject::Ptr obj = new DynamicObject();
				obj->setProperty(RestApiIds::name, ce.name);
				obj->setProperty(RestApiIds::sourceType, ce.sourceType);
				obj->setProperty(RestApiIds::thread, ce.thread);
				obj->setProperty(RestApiIds::start, ce.start);
				obj->setProperty(RestApiIds::duration, ce.duration);

				if (ce.trackSource != -1)
					obj->setProperty(RestApiIds::trackSource, ce.trackSource);

				if (ce.trackTarget != -1)
					obj->setProperty(RestApiIds::trackTarget, ce.trackTarget);

				resultsArray.add(var(obj.get()));
			}
		}
	}

	resultObj->setProperty(RestApiIds::results, var(resultsArray));
	resultObj->setProperty(RestApiIds::flows, var(buildMatchedFlows(flowSources, flowTargets)));

	return var(resultObj.get());
}

#endif // HISE_INCLUDE_PROFILING_TOOLKIT

RestServer::Response RestHelpers::handleStartProfiling(MainController* mc,
                                                        RestServer::AsyncRequest::Ptr req)
{
#if !HISE_INCLUDE_PROFILING_TOOLKIT
	return req->fail(400,
		"Profiling toolkit not enabled (requires HISE_INCLUDE_PROFILING_TOOLKIT=1)");
#else
	auto obj = req->getRequest().getJsonBody();
	auto mode = obj.getProperty(RestApiIds::mode, "record").toString();
	auto& dh = mc->getDebugSession();

	// Parse query options for filtering/summary (used by get mode)
	ProfileQueryOptions queryOpts = ProfileQueryOptions::fromJson(obj);

	// File-local helper: convert a ProfileInfoBase to JSON using either the full
	// tree format or the filtered/summary format based on query options.
	auto buildProfileResponse = [&queryOpts](
		DebugSession::ProfileDataSource::ProfileInfoBase* p) -> var
	{
		if (p == nullptr)
			return var();

		if (queryOpts.hasFilters())
			return profilingResultToSummary(p, queryOpts);

		return profilingResultToJson(p);
	};

	// Helper lambda: wait for an in-progress recording via broadcaster listener,
	// then build JSON response from the result and complete the request.
	auto waitForRecording = [&]() -> RestServer::Response
	{
		auto& rs = dynamic_cast<BackendProcessor*>(mc)->getRestServer();

		// Capture queryOpts by value for the async callback
		auto capturedOpts = queryOpts;

		dh.recordingFlushBroadcaster.addListener(rs,
			[req, capturedOpts](RestServer&,
			      DebugSession::ProfileDataSource::ProfileInfoBase::Ptr p)
		{
			DynamicObject::Ptr result = new DynamicObject();
			result->setProperty(RestApiIds::success, p != nullptr);
			result->setProperty(RestApiIds::recording, false);

			var profileData;

			if (p != nullptr)
			{
				if (capturedOpts.hasFilters())
					profileData = profilingResultToSummary(p.get(), capturedOpts);
				else
					profileData = profilingResultToJson(p.get());
			}

			auto profileObj = profileData.getDynamicObject();

			if (profileObj != nullptr)
			{
				for (auto& prop : profileObj->getProperties())
					result->setProperty(prop.name, prop.value);
			}

			req->complete(RestServer::Response::ok(var(result.get())));
		}, false);  // false = don't fire with initial/last value

		auto response = req->waitForResponse();
		dh.recordingFlushBroadcaster.removeListener(rs);
		return response;
	};

	if (mode == "get")
	{
		if (dh.isRecordingMultithread())
		{
			// Check wait param — if false, return immediately with recording status
			bool shouldWait = (bool)obj.getProperty(RestApiIds::wait, true);

			if (!shouldWait)
			{
				DynamicObject::Ptr result = new DynamicObject();
				result->setProperty(RestApiIds::success, true);
				result->setProperty(RestApiIds::recording, true);

				req->complete(RestServer::Response::ok(var(result.get())));
				return req->waitForResponse();
			}

			// Recording in progress — block until it finishes
			return waitForRecording();
		}

		// Not recording — return last result immediately (or "no data")
		auto lastResult = dh.recordingFlushBroadcaster.getLastValue<0>();

		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, lastResult != nullptr);
		result->setProperty(RestApiIds::recording, false);

		auto profileData = buildProfileResponse(lastResult.get());
		auto profileObj = profileData.getDynamicObject();

		if (profileObj != nullptr)
		{
			for (auto& prop : profileObj->getProperties())
				result->setProperty(prop.name, prop.value);
		}

		if (lastResult == nullptr)
			result->setProperty(RestApiIds::message, "No profiling data available");

		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}
	else // "record" mode (default) — non-blocking, returns immediately
	{
		if (!startProfilingSession(mc, obj))
			return req->fail(409, "A profiling session is already in progress");

		double durationMs = jlimit(100.0, 5000.0,
			(double)obj.getProperty(RestApiIds::durationMs, 1000.0));

		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, true);
		result->setProperty(RestApiIds::recording, true);
		result->setProperty(RestApiIds::durationMs, durationMs);

		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}
#endif
}

RestServer::Response RestHelpers::handleParseCSS(MainController* mc,
                                                   RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto code = obj.getProperty(RestApiIds::code, "").toString();
	auto filePathStr = obj.getProperty(RestApiIds::filePath, "").toString();
	
	String resolvedFilePath;
	
	// Resolve CSS code from either inline code or file path
	if (code.isEmpty() && filePathStr.isEmpty())
		return req->fail(400, "Either code or filePath must be provided");
	
	if (code.isEmpty())
	{
		File targetFile;
		
		if (File::isAbsolutePath(filePathStr))
			targetFile = File(filePathStr);
		else
			targetFile = mc->getSampleManager().getProjectHandler()
				.getSubDirectory(FileHandlerBase::Scripts)
				.getChildFile(filePathStr);
		
		if (!targetFile.existsAsFile())
			return req->fail(404, "File not found: " + targetFile.getFullPathName());
		
		code = targetFile.loadFileAsString();
		resolvedFilePath = targetFile.getFullPathName().replace("\\", "/");
	}
	
	// Parse the CSS
	simple_css::Parser parser(code);
	auto parseResult = parser.parse();
	
	// Helper to parse "Line X, column Y: message" into structured diagnostic data
	auto parseDiagLocation = [](const String& s, int& line, int& col, String& msg)
	{
		if (s.startsWith("Line "))
		{
			line = s.fromFirstOccurrenceOf("Line ", false, false).getIntValue();
			auto afterColumn = s.fromFirstOccurrenceOf("column ", false, false);
			col = afterColumn.getIntValue();
			// The message starts after "column N: "
			msg = afterColumn.fromFirstOccurrenceOf(": ", false, false);
		}
		else
		{
			line = 1;
			col = 1;
			msg = s;
		}
	};
	
	// Build diagnostics array
	Array<var> diagArray;
	bool parseOk = parseResult.wasOk();
	
	if (!parseOk)
	{
		int line, col;
		String msg;
		parseDiagLocation(parseResult.getErrorMessage(), line, col, msg);
		
		DynamicObject::Ptr d = new DynamicObject();
		d->setProperty(RestApiIds::line, line);
		d->setProperty(RestApiIds::column, col);
		d->setProperty(RestApiIds::severity, "error");
		d->setProperty(RestApiIds::source, "css");
		d->setProperty(RestApiIds::message, msg);
		diagArray.add(var(d.get()));
	}
	
	// Add warnings (present even on partial parse failure)
	for (const auto& w : parser.getWarnings())
	{
		int line, col;
		String msg;
		parseDiagLocation(w, line, col, msg);
		
		DynamicObject::Ptr d = new DynamicObject();
		d->setProperty(RestApiIds::line, line);
		d->setProperty(RestApiIds::column, col);
		d->setProperty(RestApiIds::severity, "warning");
		d->setProperty(RestApiIds::source, "css");
		d->setProperty(RestApiIds::message, msg);
		diagArray.add(var(d.get()));
	}
	
	// Build response
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, parseOk);
	result->setProperty(RestApiIds::diagnostics, var(diagArray));
	
	if (resolvedFilePath.isNotEmpty())
		result->setProperty(RestApiIds::filePath, resolvedFilePath);
	
	// List all selectors found in the parsed CSS
	if (parseOk)
	{
		Array<var> selectorArray;
		for (const auto& s : parser.getSelectors())
		{
			auto str = s.toString().trim();
			if (str.isNotEmpty())
				selectorArray.add(str);
		}
		result->setProperty(RestApiIds::selectors, var(selectorArray));
	}
	
	// Optional: resolve properties for given selectors using CSS specificity
	auto selectorInput = obj.getProperty(RestApiIds::selectors, var());
	
	if (parseOk && selectorInput.isArray() && selectorInput.size() > 0)
	{
		auto collection = parser.getCSSValues();
		
		// Set up a dummy animator (required by getForComponent)

		auto bp = dynamic_cast<BackendProcessor*>(mc);
		auto& animator = bp->getCssParseAnimator();
		collection.setAnimator(&animator);
		
		// Build a dummy component with the appropriate CSS selectors
		Component dummy;
		
		Array<var> classArray;
		String typeSelector;
		String idSelector;
		
		for (int i = 0; i < selectorInput.size(); i++)
		{
			auto s = selectorInput[i].toString().trim();
			
			if (s.startsWithChar('#'))
				idSelector = s.substring(1);
			else if (s.startsWithChar('.'))
				classArray.add(s.substring(1));
			else if (s.isNotEmpty())
				typeSelector = s;
		}
		
		if (typeSelector.isNotEmpty())
			dummy.getProperties().set("custom-type", typeSelector);
		
		if (idSelector.isNotEmpty())
			dummy.getProperties().set("id", idSelector);
		
		if (!classArray.isEmpty())
			dummy.getProperties().set("class", var(classArray));
		
		if (auto resolved = collection.getForComponent(&dummy))
		{
			// Check if size was provided for pixel resolution
			auto widthVal = (float)(double)obj.getProperty(RestApiIds::width, 0);
			auto heightVal = (float)(double)obj.getProperty(RestApiIds::height, 0);
			bool hasSize = widthVal > 0.0f || heightVal > 0.0f;
			
			if (hasSize)
			{
				Rectangle<float> area(0.0f, 0.0f, widthVal, heightVal);
				resolved->setFullArea(area);
			}
			
			DynamicObject::Ptr props = new DynamicObject();
			
			resolved->forEachProperty(simple_css::PseudoElementType::None,
				[&](simple_css::PseudoElementType, simple_css::Property& p)
			{
				auto pv = p.getProperty(0);
				if (!pv) return false;
				
				auto raw = pv.getRawValueString();
				
				if (hasSize)
				{
					auto isPixelResolvable = simple_css::Parser::isPixelValueProperty(p.name);
					
					DynamicObject::Ptr propObj = new DynamicObject();
					propObj->setProperty(RestApiIds::value, raw);
					
					if (isPixelResolvable)
					{
						Rectangle<float> area(0.0f, 0.0f, widthVal, heightVal);
						auto val = resolved->getPixelValue(area, { p.name, {} });
						propObj->setProperty(RestApiIds::resolved, val);
					}
					
					props->setProperty(Identifier(p.name), var(propObj.get()));
				}
				else
				{
					props->setProperty(Identifier(p.name), raw);
				}
				
				return false;
			});
			
			result->setProperty(RestApiIds::properties, var(props.get()));
		}
	}
	
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());
	
	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleShutdown(MainController* mc, 
                                                   RestServer::AsyncRequest::Ptr req)
{
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::result, "Shutdown initiated");
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));

	// Schedule quit on the message thread after the HTTP response has been sent.
	// The handler returns the Response to cpp-httplib which flushes it to the
	// client, then on the next message loop iteration the quit fires.
	MessageManager::callAsync([]()
	{
		JUCEApplication::quit();
	});

	return req->waitForResponse();
}

} // namespace hise
