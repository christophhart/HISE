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

bool RestHelpers::getTrueValue(const var& v)
{
	if (v.isBool())
		return (bool)v;

	if (v.isInt() || v.isInt64())
		return (int)v != 0;

	auto s = v.toString();

	if (s == "true")
		return true;

	if (s == "false" || s.isEmpty())
		return false;

	return s.getIntValue() != 0;
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
	bool forceSync = getTrueValue(obj.getProperty(RestApiIds::forceSynchronousExecution, false));

	// Create ScopedBadBabysitter if forceSynchronousExecution is requested
	// This bypasses all threading checks and executes everything synchronously
	std::unique_ptr<MainController::ScopedBadBabysitter> syncMode;
	if (forceSync)
		syncMode = std::make_unique<MainController::ScopedBadBabysitter>(mc);

	// Start attached profiling session if requested (fire-and-forget).
	// Results are retrieved later via POST /api/testing/profile { "mode": "get" }
#if HISE_INCLUDE_PROFILING_TOOLKIT
	if (getTrueValue(obj.getProperty(RestApiIds::profile, false)))
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

// Convert ParamType to OpenAPI type string
static String paramTypeToOpenApi(RestHelpers::ParamType t)
{
	switch (t)
	{
	case RestHelpers::ParamType::String: return "string";
	case RestHelpers::ParamType::Int:    return "integer";
	case RestHelpers::ParamType::Float:  return "number";
	case RestHelpers::ParamType::Bool:   return "boolean";
	case RestHelpers::ParamType::Array:  return "array";
	case RestHelpers::ParamType::Object: return "object";
	case RestHelpers::ParamType::Enum:   return "string";
	default: return "string";
	}
}

// Recursively convert a RouteParameter to an OpenAPI schema object
static var paramToOpenApiSchema(const RestHelpers::RouteParameter& p)
{
	DynamicObject::Ptr schema = new DynamicObject();

	// Discriminated union → oneOf
	if (!p.variants.isEmpty() && p.discriminator.isNotEmpty())
	{
		schema->setProperty("type", "object");

		// Build properties from the flat property list
		if (!p.properties.isEmpty())
		{
			DynamicObject::Ptr props = new DynamicObject();
			Array<var> requiredArr;

			for (const auto& child : p.properties)
			{
				props->setProperty(child.name, paramToOpenApiSchema(child));

				if (child.required)
					requiredArr.add(child.name.toString());
			}

			schema->setProperty("properties", var(props.get()));

			if (!requiredArr.isEmpty())
				schema->setProperty("required", var(requiredArr));
		}

		// Add discriminator
		DynamicObject::Ptr disc = new DynamicObject();
		disc->setProperty("propertyName", p.discriminator);
		schema->setProperty("discriminator", var(disc.get()));

		// Add variant descriptions as x-variants extension
		Array<var> variantArr;

		for (const auto& v : p.variants)
		{
			DynamicObject::Ptr vObj = new DynamicObject();
			vObj->setProperty("value", v.discriminatorValue);
			vObj->setProperty("description", v.description);
			variantArr.add(var(vObj.get()));
		}

		schema->setProperty("x-variants", var(variantArr));
	}
	// Object with properties
	else if (p.type == RestHelpers::ParamType::Object && !p.properties.isEmpty())
	{
		schema->setProperty("type", "object");

		DynamicObject::Ptr props = new DynamicObject();
		Array<var> requiredArr;

		for (const auto& child : p.properties)
		{
			props->setProperty(child.name, paramToOpenApiSchema(child));

			if (child.required)
				requiredArr.add(child.name.toString());
		}

		schema->setProperty("properties", var(props.get()));

		if (!requiredArr.isEmpty())
			schema->setProperty("required", var(requiredArr));
	}
	// Array with item schema
	else if (p.type == RestHelpers::ParamType::Array && p.itemSchema)
	{
		schema->setProperty("type", "array");
		schema->setProperty("items", paramToOpenApiSchema(*p.itemSchema));
	}
	// Enum
	else if (p.type == RestHelpers::ParamType::Enum && !p.enumValues.isEmpty())
	{
		schema->setProperty("type", "string");

		Array<var> enumArr;

		for (const auto& v : p.enumValues)
			enumArr.add(v);

		schema->setProperty("enum", var(enumArr));
	}
	// Simple type
	else
	{
		schema->setProperty("type", paramTypeToOpenApi(p.type));
	}

	if (p.description.isNotEmpty())
		schema->setProperty("description", p.description);

	if (p.defaultValue.isNotEmpty())
		schema->setProperty("default", p.defaultValue);

	if (p.example.isNotEmpty())
		schema->setProperty("example", p.example);

	return var(schema.get());
}

// Build the response schema for a route, wrapped in the standard envelope
static var buildResponseSchema(const RestHelpers::RouteMetadata& route)
{
	DynamicObject::Ptr envelope = new DynamicObject();
	envelope->setProperty("type", "object");

	DynamicObject::Ptr envProps = new DynamicObject();

	// success (always present)
	DynamicObject::Ptr successProp = new DynamicObject();
	successProp->setProperty("type", "boolean");
	successProp->setProperty("description", "Whether the request completed successfully");
	envProps->setProperty("success", var(successProp.get()));

	// apiVersion (auto-injected by RestServer; semver, bump on envelope/contract changes)
	DynamicObject::Ptr versionProp = new DynamicObject();
	versionProp->setProperty("type", "string");
	versionProp->setProperty("description",
		"REST API contract version (semver). Bumped when the envelope or any "
		"route contract changes - clients should compare against the version "
		"they were built for.");
	versionProp->setProperty("example", String(HISE_REST_API_VERSION));
	envProps->setProperty("apiVersion", var(versionProp.get()));

	// Endpoint-specific response fields (flat, alongside success/logs/errors)
	if (!route.responseFields.isEmpty())
	{
		for (const auto& rf : route.responseFields)
			envProps->setProperty(rf.name, paramToOpenApiSchema(rf));
	}
	else if (route.returns.isNotEmpty())
	{
		// No typed responseFields, but has a returns description:
		// add a generic "result" string field for status messages
		DynamicObject::Ptr resultProp = new DynamicObject();
		resultProp->setProperty("type", "string");
		resultProp->setProperty("description", route.returns);
		envProps->setProperty("result", var(resultProp.get()));
	}

	// logs - captured Console.print() output during request processing
	DynamicObject::Ptr logsProp = new DynamicObject();
	logsProp->setProperty("type", "array");
	logsProp->setProperty("description",
		"Console.print() output captured during request processing");
	DynamicObject::Ptr logItems = new DynamicObject();
	logItems->setProperty("type", "string");
	logsProp->setProperty("items", var(logItems.get()));
	envProps->setProperty("logs", var(logsProp.get()));

	// errors - script errors with message and callstack
	DynamicObject::Ptr errorsProp = new DynamicObject();
	errorsProp->setProperty("type", "array");
	errorsProp->setProperty("description",
		"Script errors captured during request processing");

	DynamicObject::Ptr errorItems = new DynamicObject();
	errorItems->setProperty("type", "object");

	DynamicObject::Ptr errorItemProps = new DynamicObject();

	DynamicObject::Ptr errMsgProp = new DynamicObject();
	errMsgProp->setProperty("type", "string");
	errMsgProp->setProperty("description", "The error message");
	errorItemProps->setProperty("errorMessage", var(errMsgProp.get()));

	DynamicObject::Ptr callstackProp = new DynamicObject();
	callstackProp->setProperty("type", "array");
	callstackProp->setProperty("description",
		"Call stack frames (e.g. \"myFunction() at Scripts/main.js:15:8\")");
	DynamicObject::Ptr callstackItems = new DynamicObject();
	callstackItems->setProperty("type", "string");
	callstackProp->setProperty("items", var(callstackItems.get()));
	errorItemProps->setProperty("callstack", var(callstackProp.get()));

	errorItems->setProperty("properties", var(errorItemProps.get()));
	errorItems->setProperty("required",
		var(Array<var>{ var("errorMessage"), var("callstack") }));

	errorsProp->setProperty("items", var(errorItems.get()));
	envProps->setProperty("errors", var(errorsProp.get()));

	envelope->setProperty("properties", var(envProps.get()));
	envelope->setProperty("required", var(Array<var>{ var("success"), var("apiVersion"), var("logs"), var("errors") }));

	return var(envelope.get());
}

RestServer::Response RestHelpers::handleListMethods(MainController* mc, RestServer::AsyncRequest::Ptr req)
{
	ignoreUnused(mc);

	const auto& metadata = getRouteMetadata();

	// Root object
	DynamicObject::Ptr root = new DynamicObject();
	root->setProperty("openapi", "3.0.3");

	// Info
	DynamicObject::Ptr info = new DynamicObject();
	info->setProperty("title", "HISE REST API");
	info->setProperty("description", "REST API for AI-assisted development in HISE");
	info->setProperty("version", String(HISE_REST_API_VERSION));
	root->setProperty("info", var(info.get()));

	// Servers
	Array<var> servers;
	DynamicObject::Ptr server = new DynamicObject();
	server->setProperty("url", "http://localhost:1900");
	server->setProperty("description", "Local HISE instance");
	servers.add(var(server.get()));
	root->setProperty("servers", var(servers));

	// Collect unique tags from categories
	StringArray categories;

	for (const auto& route : metadata)
		categories.addIfNotAlreadyThere(route.category);

	categories.sort(false);

	Array<var> tags;

	for (const auto& cat : categories)
	{
		DynamicObject::Ptr tag = new DynamicObject();
		tag->setProperty("name", cat);
		tags.add(var(tag.get()));
	}

	root->setProperty("tags", var(tags));

	// Paths
	DynamicObject::Ptr paths = new DynamicObject();

	for (const auto& route : metadata)
	{
		String pathKey = "/" + route.path;
		String methodKey = route.method == RestServer::GET ? "get" : "post";

		DynamicObject::Ptr operation = new DynamicObject();

		// Tags
		operation->setProperty("tags", var(Array<var>{ var(route.category) }));

		// Summary (concise sentence) & description (detailed with behavioral notes)
		operation->setProperty("summary", route.summary.isNotEmpty() ? route.summary : route.description);

		if (route.summary.isNotEmpty())
			operation->setProperty("description", route.description);

		// Operation ID from path
		String opId = route.path.replace("/", "_").replace("api_", "");
		operation->setProperty("operationId", opId);

		// Query parameters
		if (!route.queryParameters.isEmpty())
		{
			Array<var> params;

			for (const auto& qp : route.queryParameters)
			{
				DynamicObject::Ptr param = new DynamicObject();
				param->setProperty("name", qp.name.toString());
				param->setProperty("in", "query");
				param->setProperty("description", qp.description);
				param->setProperty("required", qp.required);
				param->setProperty("schema", paramToOpenApiSchema(qp));

				if (qp.example.isNotEmpty())
					param->setProperty("example", qp.example);

				params.add(var(param.get()));
			}

			operation->setProperty("parameters", var(params));
		}

		// Request body (POST with body params)
		if (!route.bodyParameters.isEmpty())
		{
			DynamicObject::Ptr requestBody = new DynamicObject();
			requestBody->setProperty("required", true);

			DynamicObject::Ptr content = new DynamicObject();
			DynamicObject::Ptr jsonMedia = new DynamicObject();

			// Build body schema from parameters
			DynamicObject::Ptr bodySchema = new DynamicObject();
			bodySchema->setProperty("type", "object");

			DynamicObject::Ptr bodyProps = new DynamicObject();
			Array<var> requiredArr;

			for (const auto& bp : route.bodyParameters)
			{
				bodyProps->setProperty(bp.name, paramToOpenApiSchema(bp));

				if (bp.required)
					requiredArr.add(bp.name.toString());
			}

			bodySchema->setProperty("properties", var(bodyProps.get()));

			if (!requiredArr.isEmpty())
				bodySchema->setProperty("required", var(requiredArr));

			jsonMedia->setProperty("schema", var(bodySchema.get()));

			// Request example
			if (route.requestExample.isNotEmpty())
			{
				var exampleJson;
				JSON::parse(route.requestExample, exampleJson);

				if (!exampleJson.isVoid())
					jsonMedia->setProperty("example", exampleJson);
			}

			content->setProperty("application/json", var(jsonMedia.get()));
			requestBody->setProperty("content", var(content.get()));
			operation->setProperty("requestBody", var(requestBody.get()));
		}

		// Responses
		DynamicObject::Ptr responses = new DynamicObject();

		// 200 OK
		DynamicObject::Ptr ok200 = new DynamicObject();
		ok200->setProperty("description", "Successful response");

		DynamicObject::Ptr okContent = new DynamicObject();
		DynamicObject::Ptr okJsonMedia = new DynamicObject();
		okJsonMedia->setProperty("schema", buildResponseSchema(route));

		// Response example
		if (route.responseExample.isNotEmpty())
		{
			var exampleJson;
			JSON::parse(route.responseExample, exampleJson);

			if (!exampleJson.isVoid())
				okJsonMedia->setProperty("example", exampleJson);
		}

		okContent->setProperty("application/json", var(okJsonMedia.get()));
		ok200->setProperty("content", var(okContent.get()));
		responses->setProperty("200", var(ok200.get()));

		// Error responses. Routes flagged with rejectsInSnippetBrowser() implicitly
		// surface a 409 from the dispatcher; ensure it shows up in the spec even
		// if the route definition didn't list it.
		Array<int> codes = route.errorCodes;
		if (route.rejectInSnippetBrowser && !codes.contains(409))
			codes.add(409);

		for (int code : codes)
		{
			DynamicObject::Ptr errResp = new DynamicObject();

			switch (code)
			{
			case 400: errResp->setProperty("description", "Bad request - invalid or missing parameters"); break;
			case 404: errResp->setProperty("description", "Not found - unknown module or resource"); break;
			case 409: errResp->setProperty("description", "Conflict - operation conflicts with current state"); break;
			case 500: errResp->setProperty("description", "Internal server error"); break;
			case 503: errResp->setProperty("description", "Service unavailable"); break;
			default:  errResp->setProperty("description", "Error"); break;
			}

			responses->setProperty(String(code), var(errResp.get()));
		}

		operation->setProperty("responses", var(responses.get()));

		// Add to paths (handle multiple methods on same path)
		auto existingPath = paths->getProperty(Identifier(pathKey));

		if (auto* existingObj = existingPath.getDynamicObject())
		{
			existingObj->setProperty(Identifier(methodKey), var(operation.get()));
		}
		else
		{
			DynamicObject::Ptr pathItem = new DynamicObject();
			pathItem->setProperty(Identifier(methodKey), var(operation.get()));
			paths->setProperty(Identifier(pathKey), var(pathItem.get()));
		}
	}

	root->setProperty("paths", var(paths.get()));

	req->complete(RestServer::Response::ok(var(root.get())));
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

	// Reflect whether the BP that handled this request is the snippet browser.
	// In snippet mode, callers should expect the project/wizard endpoints to
	// be rejected with 409 (see Active Processor Routing).
	auto bp = dynamic_cast<BackendProcessor*>(mc);
	result->setProperty(RestApiIds::activeIsSnippetBrowser, bp != nullptr && bp->isSnippetBrowser());

	// Server info
	DynamicObject::Ptr server = new DynamicObject();
	server->setProperty(RestApiIds::version, PresetHandler::getVersionString());
	server->setProperty(RestApiIds::commitHash, String(PREVIOUS_HISE_COMMIT));
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

RestServer::Response RestHelpers::handleStatusPreprocessors(MainController* mc,
                                                            RestServer::AsyncRequest::Ptr req)
{
	const bool verbose      = req->getRequest().getTrueValue(RestApiIds::verbose);
	const bool skipDefaults = req->getRequest().getTrueValue(RestApiIds::skipDefaults);

	PreprocessorDataBase db;

	auto preprocessors = db.toJSON(mc, verbose, skipDefaults);

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::preprocessors, preprocessors);
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
	bool forceSync = getTrueValue(obj.getProperty(RestApiIds::forceSynchronousExecution, false));
	
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
		auto useHierarchy = req->getRequest().getTrueValue(RestApiIds::hierarchy);

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
	bool forceSync = getTrueValue(obj.getProperty(RestApiIds::forceSynchronousExecution, false));
	
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
		auto validateRange = getTrueValue(obj.getProperty(RestApiIds::validateRange, false));

		if (validateRange)
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
	
	bool force = getTrueValue(obj.getProperty(RestApiIds::force, false));
	
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
	
	ValueTreeUpdateWatcher::ScopedSuspender sd(content->getUpdateWatcher());

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

RestServer::Response RestHelpers::handleTestingScreenshot(MainController* mc, RestServer::AsyncRequest::Ptr req)
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

RestServer::Response RestHelpers::handleTestingE2e(BackendProcessor* bp, RestServer::AsyncRequest::Ptr req)
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
	bool verbose = getTrueValue(body.getProperty(RestApiIds::verbose, false));
	
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
		// moduleId provided - use it to find the processor
		jp = getScriptProcessor(mc, req);
		
		if (jp == nullptr)
			return req->fail(404, "moduleId is not a valid script processor");
		
		if (filePathStr.isEmpty())
		{
			// moduleId only, no filePath - need to pick a file
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
		// filePath only - resolve the owning processor
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
	
	auto useAsync = getTrueValue(obj.getProperty(RestApiIds::async, false));
	
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
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, Array<var>());
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

RestServer::Response RestHelpers::handleTestingProfile(MainController* mc,
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
			// Check wait param - if false, return immediately with recording status
			bool shouldWait = getTrueValue(obj.getProperty(RestApiIds::wait, true));

			if (!shouldWait)
			{
				DynamicObject::Ptr result = new DynamicObject();
				result->setProperty(RestApiIds::success, true);
				result->setProperty(RestApiIds::recording, true);

				req->complete(RestServer::Response::ok(var(result.get())));
				return req->waitForResponse();
			}

			// Recording in progress - block until it finishes
			return waitForRecording();
		}

		// Not recording - return last result immediately (or "no data")
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
	else // "record" mode (default) - non-blocking, returns immediately
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

// ============================================================================
// Snippet Browser Endpoint Handler
// ============================================================================

RestServer::Response RestHelpers::handleSnippetBrowser(MainController* mc,
                                                        RestServer::AsyncRequest::Ptr req)
{
	// Always operate on the main BackendProcessor regardless of which BP
	// drove the dispatcher. The endpoint manages instance topology and
	// shutdown could destroy the BP we entered with.
	auto bp = dynamic_cast<BackendProcessor*>(mc);
	auto main = bp != nullptr ? bp->getMainInstance() : nullptr;
	auto mainBrw = main != nullptr ? main->currentRootWindow : nullptr;

	auto obj = req->getRequest().getJsonBody();
	auto action = obj[RestApiIds::action].toString();

	// Validate input before checking UI availability, so 400-class errors
	// (missing/invalid action) win over the headless 501 path.
	if (action.isEmpty())
		return req->fail(400, "action is required (launch|shutdown|enable|disable)");

	if (action != "launch" && action != "shutdown" && action != "enable" && action != "disable")
		return req->fail(400, "action must be one of: launch, shutdown, enable, disable");

	if (mainBrw == nullptr)
		return req->fail(501, "snippet browser is not available without a UI root window");

	MessageManager::callAsync([main, mainBrw, action, req]()
	{
		auto findSnippet = [mainBrw]() -> BackendRootWindow*
		{
			for (auto w : mainBrw->allWindowsAndBrowsers)
			{
				if (auto c = w.getComponent())
				{
					if (c != mainBrw && c->getBackendProcessor()->isSnippetBrowser())
						return c;
				}
			}
			return nullptr;
		};

		auto snippetBrw = findSnippet();
		bool willBeDestroyed = false;

		if (action == "launch")
		{
			if (snippetBrw != nullptr)
				snippetBrw->setCurrentlyActiveProcessor();
			else
				BackendCommandTarget::Actions::showExampleBrowser(mainBrw);
		}
		else if (action == "shutdown")
		{
			if (snippetBrw != nullptr)
			{
				snippetBrw->deleteThisSnippetInstance(false);
				willBeDestroyed = true;
			}
		}
		else if (action == "enable")
		{
			if (snippetBrw == nullptr)
			{
				req->complete(RestServer::Response::error(409, "no snippet browser instance to enable"));
				return;
			}
			snippetBrw->setCurrentlyActiveProcessor();
		}
		else if (action == "disable")
		{
			if (snippetBrw == nullptr)
			{
				req->complete(RestServer::Response::error(409, "no snippet browser instance to disable"));
				return;
			}
			mainBrw->setCurrentlyActiveProcessor();
		}

		// Re-evaluate state for the response. deleteThisSnippetInstance(false)
		// schedules the delete for the next message loop iteration, so the
		// snippet pointer may still be alive here. Use willBeDestroyed to
		// report the eventual state.
		bool exists = !willBeDestroyed && (findSnippet() != nullptr);

		String active = "main";
		if (!willBeDestroyed && main->callback != nullptr)
		{
			auto cur = dynamic_cast<BackendProcessor*>(main->callback->getCurrentProcessor());
			if (cur != nullptr && cur->isSnippetBrowser())
				active = "snippet";
		}

		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, true);
		result->setProperty(RestApiIds::exists, exists);
		result->setProperty(RestApiIds::active, active);
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, Array<var>());

		req->complete(RestServer::Response::ok(var(result.get())));
	});

	return req->waitForResponse();
}

// ============================================================================
// Builder Endpoint Handlers
// ============================================================================

RestServer::Response RestHelpers::handleBuilderTree(MainController* mc, 
                                                     RestServer::AsyncRequest::Ptr req)
{
	auto moduleId = req->getRequest()[RestApiIds::moduleId];
	
	Processor* root = mc->getMainSynthChain();
	
	

	auto group = req->getRequest()[RestApiIds::group];

	TreeOptions o;

    o.includeParameters = req->getRequest().getTrueValue(RestApiIds::queryParameters);
    o.verbose = req->getRequest().getTrueValue(RestApiIds::verbose);

	var tree;

	if (group.isNotEmpty())
	{
		if(group != "current")
			return req->fail(501, "only current group is supported");

		auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::BuilderTree);
		auto v = um->getValidationPlan();
		auto treeToUse = v;

		if (moduleId.isNotEmpty())
		{
			treeToUse = ValueTree();

			valuetree::Helpers::forEach(v, [&](const ValueTree& mv)
			{
				if (mv[PropertyIds::ID].toString() == moduleId)
				{
					treeToUse = mv;
					return true;
				}

				return false;
			});

			if (!treeToUse.isValid())
				return req->fail(404, "Module not found: " + moduleId);
		}

		if (!treeToUse.isValid())
			return req->fail(400, group == "current" ? "No current group" : "group not found");

		tree = buildModuleTree({ nullptr, treeToUse }, o);
	}
	else
	{
		if (moduleId.isNotEmpty())
			root = findProcessorByName(mc, moduleId);

		if (root == nullptr)
			return req->fail(404, "Module not found: " + moduleId);

		tree = buildModuleTree({ root, ValueTree() }, o);
	}

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::result, tree);
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}



hise::ControlledObject* BackendProcessor::getOrCreateRestServerBuildUndoManager()
{
	if (buildUndoManager == nullptr)
		buildUndoManager = new RestServerUndoManager::Instance(this);

	return buildUndoManager.get();
}

RestServer::Response RestHelpers::handleBuilderApply(MainController* mc,
                                                      RestServer::AsyncRequest::Ptr req)
{
	static constexpr ApiRoute CurrentEndpoint = ApiRoute::BuilderApply;

	std::vector<RestServerUndoManager::CallStack> errorCallstack;

	req->setUseCustomErrors(true);
	auto obj = req->getRequest().getJsonBody();
	
	// --- Field validation ---
	
	auto ops = obj[RestApiIds::operations];
	if (!ops.isArray())
		return req->fail(400, "operations must be an array");
	
	if (ops.size() == 0)
		return req->fail(400, "operations array must not be empty");
	
	auto p = mc->getMainSynthChain();
	auto noErrors = true;
	
	// Phase 3: Execute via undo manager
	auto um = RestServerUndoManager::Instance::getOrCreate(mc, CurrentEndpoint);
	
	// Phase 1: Validate required fields per operation type
	for (int i = 0; i < ops.size(); i++)
	{
		auto op = ops[i];
		
		auto ok = um->prevalidate(RestServerUndoManager::Domain::Builder, ops[i]);

		if (!ok)
		{
			noErrors = false;

			errorCallstack.push_back(RestServerUndoManager::CallStack(ok.getErrorMessage())
				.withGroup(um->getCurrentGroupId())
				.withPhase(RestServerUndoManager::CallStack::Phase::Prevalidation)
				.withOperation(i, ops[i]["op"].toString())
				.withEndpoint(CurrentEndpoint));

			//debugError(p, "operations[" + String(i) + "]: " + ok.getErrorMessage());
			continue;
		}

	}
	
	if (!noErrors)
	{
		//debugError(p, "Rejected batch: some operations failed validation");
		
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, false);
		result->setProperty(RestApiIds::result, var());
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, RestServerUndoManager::CallStack::toJSONList(errorCallstack));
		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}
	
	using ActionBase = RestServerUndoManager::ActionBase;
	ActionBase::List actions;
	
	for (int i = 0; i < ops.size(); i++)
	{
		auto op = ops[i];

		auto ad = um->createAction(RestServerUndoManager::Domain::Builder, op);
		auto ok = ad->validate();

		if (ok)
			actions.add(ad);
		else
		{
			errorCallstack.push_back(RestServerUndoManager::CallStack(ok.getErrorMessage())
				.withGroup(um->getCurrentGroupId())
				.withPhase(RestServerUndoManager::CallStack::Phase::Validation)
				.withOperation(i, ad->getDescription())
				.withEndpoint(CurrentEndpoint));

			noErrors = false;
			//debugError(p, "operations[" + String(i) + "]: " + ok.getErrorMessage());
		}
	}
	
	if (!noErrors)
	{
		//debugError(p, "Rejected batch: some operations failed validation");
		
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, false);
		result->setProperty(RestApiIds::result, var());
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, RestServerUndoManager::CallStack::toJSONList(errorCallstack));
		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}
	
	um->setValidationErrors(errorCallstack);

	um->performAction(req, actions, [](ActionBase::List l, bool undo)
	{
		if (!l.isEmpty())
		{
			auto mc = l.getFirst()->getMainController();
			
			int rl = 0;

			for (auto a : l)
				rl |= a->getRebuildLevel(RestServerUndoManager::Domain::Builder, undo);

			if(rl & RestServerUndoManager::RebuildLevel::UniqueId)
				PresetHandler::setUniqueIdsForProcessor(mc->getMainSynthChain());

			for (auto a : l)
				debugToConsole(mc->getMainSynthChain(), a->getHistoryMessage(undo));
		}
	});
	
	return req->waitForResponse();
}

// ============================================================================
// Builder Reset Handler
// ============================================================================

RestServer::Response RestHelpers::handleBuilderReset(MainController* mc,
                                                      RestServer::AsyncRequest::Ptr req)
{
    auto um = RestServerUndoManager::Instance::getOrCreate(mc, RestHelpers::ApiRoute::BuilderReset);
    
    
    
    mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), [um, req](Processor* p)
    {
        p->getMainController()->clearPreset(sendNotificationAsync);
        dynamic_cast<BackendProcessor*>(p->getMainController())->createInterface(600, 500);

        DynamicObject::Ptr result = new DynamicObject();
        result->setProperty(RestApiIds::success, true);
        result->setProperty(RestApiIds::result, "Module tree reset");
        result->setProperty(RestApiIds::logs, Array<var>());
        result->setProperty(RestApiIds::errors, Array<var>());

        
        req->complete(RestServer::Response::ok(var(result.get())));
        
        um->clearUndoHistory();
        um->flushUI(p);
        
        return SafeFunctionCall::OK;
    }, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
    
	return req->waitForResponse();
}

// ============================================================================
// Undo Endpoint Handlers
// ============================================================================

RestServer::Response RestHelpers::handleUndoPushGroup(MainController* mc,
                                                       RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	
	auto groupName = obj[RestApiIds::name].toString();
	if (groupName.isEmpty())
		return req->fail(400, "name is required");
	
	auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::UndoPushGroup);
	
	um->pushPlan(groupName);
	
	auto result = RestServerUndoManager::Instance::getResponse({}, um->getDiffJSON(true, true));
	req->complete(result);
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleUndoPopGroup(MainController* mc,
                                                      RestServer::AsyncRequest::Ptr req)
{
	auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::UndoPopGroup);
	
	req->setUseCustomErrors(true);

	if (!um->popPlan(req))
		return req->fail(400, "Not inside a group");
	
	// popPlan reads cancel from the request body internally.
	// When cancel=false, popPlan calls performAction which completes the request.
	// When cancel=true, popPlan just pops without executing, so we complete here.
	auto obj = req->getRequest().getJsonBody();
	bool shouldCancel = getTrueValue(obj.getProperty(RestApiIds::cancel, false));
	
	if (shouldCancel)
	{
		auto result = RestServerUndoManager::Instance::getResponse({}, um->getDiffJSON(true, true));
		req->complete(result);
	}
	
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleUndoBack(MainController* mc,
                                                  RestServer::AsyncRequest::Ptr req)
{
	req->setUseCustomErrors(true);

	auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::UndoBack);
	
	if (!um->undo(req))
	{
		return req->fail(400, "nothing to undo");
	}
	
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleUndoForward(MainController* mc,
                                                     RestServer::AsyncRequest::Ptr req)
{
	req->setUseCustomErrors(true);

	auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::UndoForward);
	
	if (!um->redo(req))
	{
		return req->fail(400, "nothing to redo");
	}
	
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleUndoDiff(MainController* mc,
                                                  RestServer::AsyncRequest::Ptr req)
{
	auto scopeStr = req->getRequest()[RestApiIds::scope];
	auto domainFilter = req->getRequest()[RestApiIds::domain];
	bool shouldFlatten = req->getRequest()[RestApiIds::flatten].getIntValue() != 0;
	
	if (scopeStr.isEmpty()) 
		scopeStr = "group";
	
	if (scopeStr != "group" && scopeStr != "root")
		return req->fail(400, "scope must be 'group' or 'root'");
	
	auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::UndoDiff);
	
	auto group = scopeStr == "group";

	auto domainIndex = RestServerUndoManager::getDomains().indexOf(domainFilter);

	RestServerUndoManager::Domain d = RestServerUndoManager::Domain::Undefined;

	if (domainIndex != -1)
		d = (RestServerUndoManager::Domain)domainIndex;

	auto result = RestServerUndoManager::Instance::getResponse({}, um->getDiffJSON(shouldFlatten, group, d));

	req->complete(result);
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleUndoHistory(MainController* mc,
                                                     RestServer::AsyncRequest::Ptr req)
{
	auto scopeStr = req->getRequest()[RestApiIds::scope];
	auto domainFilter = req->getRequest()[RestApiIds::domain];
	bool shouldFlatten = req->getRequest()[RestApiIds::flatten].getIntValue() != 0;
	
	if (scopeStr.isEmpty()) scopeStr = "group";
	
	if (scopeStr != "group" && scopeStr != "root")
		return req->fail(400, "scope must be 'group' or 'root'");
	
	auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::UndoHistory);
	
	auto group = scopeStr == "group";
	
	auto domainIndex = RestServerUndoManager::getDomains().indexOf(domainFilter);
	
	RestServerUndoManager::Domain d = RestServerUndoManager::Domain::Undefined;
	
	if (domainIndex != -1)
		d = (RestServerUndoManager::Domain)domainIndex;
	
	auto result = RestServerUndoManager::Instance::getResponse({}, um->getHistory(shouldFlatten, group, d));
	
	req->complete(result);
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleUndoClear(MainController* mc,
                                                   RestServer::AsyncRequest::Ptr req)
{
	auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::UndoClear);
	
	um->clearUndoHistory();
	
	auto result = RestServerUndoManager::Instance::getResponse({}, um->getDiffJSON(true, true));
	req->complete(result);
	return req->waitForResponse();
}

// ============================================================================
// Wizard Endpoint Handlers
// ============================================================================

namespace WizardIds
{
	static const StringArray validWizardIds = {
		"new_project", "recompile", "plugin_export",
		"compile_networks", "audio_export", "install_package_maker"
	};

	static bool isAsyncTask(const String& wizardId)
	{
		if (wizardId == "plugin_export")
			return true;

		return false;
	}

	static bool isValidTaskForWizard(const String& wizardId, const String& task)
	{
		if (wizardId == "new_project")
			return task == "createEmptyProject" || task == "importHxiTask" || task == "extractRhapsody";
		if (wizardId == "recompile")
			return task == "task";
		if (wizardId == "plugin_export")
			return task == "compileTask";
		if (wizardId == "compile_networks")
			return task == "compileTask";
		if (wizardId == "audio_export")
			return task == "onExport";
		if (wizardId == "install_package_maker")
			return task == "writePackageJson";
		return false;
	}

	static String getValidTasksForWizard(const String& wizardId)
	{
		if (wizardId == "new_project")
			return "createEmptyProject, importHxiTask, extractRhapsody";
		if (wizardId == "recompile")
			return "task";
		if (wizardId == "plugin_export")
			return "compileTask";
		if (wizardId == "compile_networks")
			return "compileTask";
		if (wizardId == "audio_export")
			return "onExport";
		if (wizardId == "install_package_maker")
			return "writePackageJson";
		return "";
	}
}

void RestHelpers::WizardExecutor::registerExecutors()
{
	// prove that the API envelope matches
	registerExecutor<DummyTask>("dummy");
	executors.clear();

	registerExecutor<multipage::library::NewProjectCreator>("new_project");

	registerExecutor<multipage::library::CompileProjectDialog>("plugin_export");
	registerExecutor<multipage::library::NetworkCompiler>("compile_networks");
}



RestServer::Response RestHelpers::handleWizardInitialise(MainController* mc,
                                                          RestServer::AsyncRequest::Ptr req)
{
	auto wizardId = req->getRequest()[RestApiIds::id];

	if (wizardId.isEmpty())
		return req->fail(400, "id query parameter is required");

	if (!WizardIds::validWizardIds.contains(wizardId))
		return req->fail(400, "Unknown wizard ID: " + wizardId +
			". Valid IDs: " + WizardIds::validWizardIds.joinIntoString(", "));

    WizardExecutor w(mc, wizardId);
    
    auto ok = w.initialise(req->getRequest());
	req->complete(ok);
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleWizardExecute(MainController* mc,
                                                       RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();

	auto wizardId = obj[RestApiIds::wizardId].toString();
	auto answers = obj[RestApiIds::answers];
	auto tasks = obj[RestApiIds::tasks];

	// Validate required fields
	if (wizardId.isEmpty())
		return req->fail(400, "wizardId is required in request body");

	if (!answers.isObject())
		return req->fail(400, "answers must be a key/value object");

	if (!tasks.isArray() || tasks.size() != 1)
		return req->fail(400, "tasks must be an array with exactly one task function name");

	// Validate wizard ID
	if (!WizardIds::validWizardIds.contains(wizardId))
		return req->fail(400, "Unknown wizard ID: " + wizardId +
			". Valid IDs: " + WizardIds::validWizardIds.joinIntoString(", "));

	// Validate task name for this wizard
	auto taskName = tasks[0].toString();

	if (!WizardIds::isValidTaskForWizard(wizardId, taskName))
		return req->fail(400, "Invalid task '" + taskName + "' for wizard '" + wizardId +
			"'. Valid tasks: " + WizardIds::getValidTasksForWizard(wizardId));

	// TODO (Christoph): Wire up to dialog_library task logic.
	// 1. Create headless wizard instance or call extracted business logic
	// 2. Populate state->globalState with answers
	// 3. Call the task function (e.g., createEmptyProject, compileTask, etc.)
	// 4. For sync tasks: return result directly
	// 5. For async tasks (audio_export): start background job, return {jobId, async: true}

    WizardExecutor w(mc, wizardId);

	auto async = WizardIds::isAsyncTask(wizardId);

	auto bp = dynamic_cast<BackendProcessor*>(mc);
	auto s = dynamic_cast<RestHelpers::WizardExecutor::AsyncRunner*>(bp->getRestWizardRunner());

    auto ok = w.execute(req->getRequest(), async ? s : nullptr);
    req->complete(ok);
    return req->waitForResponse();
}

RestServer::Response RestHelpers::handleWizardStatus(MainController* mc,
                                                      RestServer::AsyncRequest::Ptr req)
{
	auto jobId = req->getRequest()[RestApiIds::jobId];

	if (jobId.isEmpty())
		return req->fail(400, "jobId query parameter is required");

	auto bp = dynamic_cast<BackendProcessor*>(mc);
	auto s = dynamic_cast<RestHelpers::WizardExecutor::AsyncRunner*>(bp->getRestWizardRunner());


	// TODO (Christoph): Look up active async job by jobId.
	// Return {finished, progress, message} from the job's current state.
	// If jobId is unknown, return error.

	if(jobId != s->getActiveJobId())
		return req->fail(404, "No active job with ID: " + jobId);

	req->complete(s->makeResponse());
	return req->waitForResponse();

}

// ============================================================================
// UI Tree Handler
// ============================================================================

static var buildUIComponentTreeFromValueTree(const ValueTree& v)
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(RestApiIds::id, v.getProperty("id"));
	obj->setProperty(RestApiIds::type, v.getProperty("type"));
	obj->setProperty(RestApiIds::visible, v.getProperty("visible", true));
	obj->setProperty(RestApiIds::enabled, v.getProperty("enabled", true));
	obj->setProperty(RestApiIds::saveInPreset, v.getProperty("saveInPreset", false));
	obj->setProperty(RestApiIds::x, v.getProperty("x", 0));
	obj->setProperty(RestApiIds::y, v.getProperty("y", 0));
	obj->setProperty(RestApiIds::width, v.getProperty("width", 128));
	obj->setProperty(RestApiIds::height, v.getProperty("height", 48));

	Array<var> children;
	for (int i = 0; i < v.getNumChildren(); i++)
		children.add(buildUIComponentTreeFromValueTree(v.getChild(i)));

	obj->setProperty(RestApiIds::childComponents, var(children));
	return var(obj.get());
}

static var buildUIComponentTreeFromScriptComponent(ScriptComponent* sc)
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(RestApiIds::id, sc->getName().toString());
	obj->setProperty(RestApiIds::type, sc->getObjectName().toString());
	obj->setProperty(RestApiIds::visible, sc->getScriptObjectProperty(ScriptComponent::visible));
	obj->setProperty(RestApiIds::enabled, sc->getScriptObjectProperty(ScriptComponent::enabled));
	obj->setProperty(RestApiIds::saveInPreset, sc->getScriptObjectProperty(ScriptComponent::saveInPreset));
	obj->setProperty(RestApiIds::x, sc->getScriptObjectProperty(ScriptComponent::x));
	obj->setProperty(RestApiIds::y, sc->getScriptObjectProperty(ScriptComponent::y));
	obj->setProperty(RestApiIds::width, sc->getScriptObjectProperty(ScriptComponent::width));
	obj->setProperty(RestApiIds::height, sc->getScriptObjectProperty(ScriptComponent::height));

	Array<var> children;

	auto v = sc->getPropertyValueTree();
	for (auto c : v)
	{
		if (auto child = sc->getScriptProcessor()->getScriptingContent()->getComponentWithName(c["id"].toString()))
			children.add(buildUIComponentTreeFromScriptComponent(child));
	}

	obj->setProperty(RestApiIds::childComponents, var(children));
	return var(obj.get());
}

RestServer::Response RestHelpers::handleUITree(MainController* mc,
                                                RestServer::AsyncRequest::Ptr req)
{
	if (auto jp = getScriptProcessor(mc, req))
	{
		auto content = dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();
		auto group = req->getRequest()[RestApiIds::group];

		var tree;

		if (group.isNotEmpty())
		{
			if (group != "current")
				return req->fail(501, "only 'current' group is supported");

			auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::UITree);
			auto uiState = um->getCurrentUIValidationState();

			if (uiState == nullptr || !uiState->contentTree.isValid())
				return req->fail(400, group == "current" ? "No current UI validation state" : "group not found");

			// Build tree from UIValidationState's copied ValueTree
			auto& ct = uiState->contentTree;

			DynamicObject::Ptr root = new DynamicObject();
			root->setProperty(RestApiIds::id, "Content");
			root->setProperty(RestApiIds::type, "ScriptPanel");
			root->setProperty(RestApiIds::visible, true);
			root->setProperty(RestApiIds::enabled, true);
			root->setProperty(RestApiIds::saveInPreset, false);
			root->setProperty(RestApiIds::x, 0);
			root->setProperty(RestApiIds::y, 0);
			root->setProperty(RestApiIds::width, ct.getProperty("width", 600));
			root->setProperty(RestApiIds::height, ct.getProperty("height", 500));

			Array<var> children;
			for (int i = 0; i < ct.getNumChildren(); i++)
				children.add(buildUIComponentTreeFromValueTree(ct.getChild(i)));
			root->setProperty(RestApiIds::childComponents, var(children));

			tree = var(root.get());
		}
		else
		{
			// Build root Content node
			DynamicObject::Ptr root = new DynamicObject();
			root->setProperty(RestApiIds::id, "Content");
			root->setProperty(RestApiIds::type, "ScriptPanel");
			root->setProperty(RestApiIds::visible, true);
			root->setProperty(RestApiIds::enabled, true);
			root->setProperty(RestApiIds::saveInPreset, false);
			root->setProperty(RestApiIds::x, 0);
			root->setProperty(RestApiIds::y, 0);
			root->setProperty(RestApiIds::width, content->getContentProperties().getProperty("width", 600));
			root->setProperty(RestApiIds::height, content->getContentProperties().getProperty("height", 500));

			Array<var> children;
			for (int i = 0; i < content->getNumComponents(); i++)
			{
				auto sc = content->getComponent(i);
				if (sc->getParentScriptComponent() == nullptr)
					children.add(buildUIComponentTreeFromScriptComponent(sc));
			}
			root->setProperty(RestApiIds::childComponents, var(children));

			tree = var(root.get());
		}

		// Merge tree fields onto flat response
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, true);
		result->setProperty(RestApiIds::result, tree);
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

// ============================================================================
// UI Apply Handler
// ============================================================================

RestServer::Response RestHelpers::handleUIApply(MainController* mc,
                                                 RestServer::AsyncRequest::Ptr req)
{
	static constexpr ApiRoute CurrentEndpoint = ApiRoute::UIApply;

	std::vector<RestServerUndoManager::CallStack> errorCallstack;

	req->setUseCustomErrors(true);
	auto obj = req->getRequest().getJsonBody();

	// --- Field validation ---

	auto ops = obj[RestApiIds::operations];
	if (!ops.isArray())
		return req->fail(400, "operations must be an array");

	if (ops.size() == 0)
		return req->fail(400, "operations array must not be empty");

	auto noErrors = true;

	auto um = RestServerUndoManager::Instance::getOrCreate(mc, CurrentEndpoint);

	// Phase 1: Validate required fields per operation type
	for (int i = 0; i < ops.size(); i++)
	{
		auto ok = um->prevalidate(RestServerUndoManager::Domain::UI, ops[i]);

		if (!ok)
		{
			noErrors = false;

			errorCallstack.push_back(RestServerUndoManager::CallStack(ok.getErrorMessage())
				.withGroup(um->getCurrentGroupId())
				.withPhase(RestServerUndoManager::CallStack::Phase::Prevalidation)
				.withOperation(i, ops[i][RestApiIds::op].toString())
				.withEndpoint(CurrentEndpoint));
		}
	}

	if (!noErrors)
	{
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, false);
		result->setProperty(RestApiIds::result, var());
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, RestServerUndoManager::CallStack::toJSONList(errorCallstack));
		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}

	// Phase 2: Create actions and validate semantics
	using ActionBase = RestServerUndoManager::ActionBase;
	ActionBase::List actions;

	for (int i = 0; i < ops.size(); i++)
	{
		auto ad = um->createAction(RestServerUndoManager::Domain::UI, ops[i]);
		auto ok = ad->validate();

		if (ok)
			actions.add(ad);
		else
		{
			errorCallstack.push_back(RestServerUndoManager::CallStack(ok.getErrorMessage())
				.withGroup(um->getCurrentGroupId())
				.withPhase(RestServerUndoManager::CallStack::Phase::Validation)
				.withOperation(i, ad->getDescription())
				.withEndpoint(CurrentEndpoint));

			noErrors = false;
		}
	}

	if (!noErrors)
	{
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, false);
		result->setProperty(RestApiIds::result, var());
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, RestServerUndoManager::CallStack::toJSONList(errorCallstack));
		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}

	// Phase 3: Execute via undo manager
	um->setValidationErrors(errorCallstack);

    auto mid = obj[RestApiIds::moduleId].toString();
    auto sp = dynamic_cast<ProcessorWithScriptingContent*>(ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), mid));
    
    if(sp == nullptr)
        return req->fail(404, String("module with ID ") + mid + String(" + not found"));
    
    //ValueTreeUpdateWatcher::ScopedDelayer sd(sp->getScriptingContent()->getUpdateWatcher(), true);
    
	um->performAction(req, actions, [](ActionBase::List l, bool undo)
	{
		if (!l.isEmpty())
		{
			auto mc = l.getFirst()->getMainController();

			for (auto a : l)
				debugToConsole(mc->getMainSynthChain(), a->getHistoryMessage(undo));
		}
	});

	return req->waitForResponse();
}

// ============================================================================
// Builder Helper Methods
// ============================================================================

int RestHelpers::resolveChainIndex(const String& chainName)
{
	auto lower = chainName.toLowerCase();
	
	if (lower == "direct") return raw::IDs::Chains::Direct;
	if (lower == "midi")   return raw::IDs::Chains::Midi;
	if (lower == "gain")   return raw::IDs::Chains::Gain;
	if (lower == "pitch")  return raw::IDs::Chains::Pitch;
	if (lower == "fx")     return raw::IDs::Chains::FX;
	
	return -2; // Invalid
}

String RestHelpers::getChainName(int chainIndex)
{
	if (chainIndex == raw::IDs::Chains::Direct) return "direct";
	if (chainIndex == raw::IDs::Chains::Midi)   return "midi";
	if (chainIndex == raw::IDs::Chains::Gain)   return "gain";
	if (chainIndex == raw::IDs::Chains::Pitch)  return "pitch";
	if (chainIndex == raw::IDs::Chains::FX)     return "fx";
	
	return "unknown";
}

bool RestHelpers::validateTypeForChain(const Identifier& typeId, 
                                        int chainIndex, 
                                        DynamicObject::Ptr hints)
{
	ProcessorMetadataRegistry registry;
	auto md = registry.get(typeId);
	jassert(md != nullptr); // Should be checked by caller
	
	StringArray validChains;
	
	// Map metadata type to valid chains
	if (md->type == ProcessorMetadataIds::Modulator)
	{
		validChains.add("gain");
		validChains.add("pitch");
	}
	else if (md->type == ProcessorMetadataIds::Effect)
	{
		validChains.add("fx");
	}
	else if (md->type == ProcessorMetadataIds::MidiProcessor)
	{
		validChains.add("midi");
	}
	else if (md->type == ProcessorMetadataIds::SoundGenerator)
	{
		validChains.add("direct");
	}
	
	hints->setProperty(RestApiIds::validChains, var(validChains));
	
	// Check if requested chain is valid
	String requestedChain = getChainName(chainIndex);
	return validChains.contains(requestedChain);
}

Processor* RestHelpers::findProcessorByName(MainController* mc, const String& name)
{
	return ProcessorHelpers::getFirstProcessorWithName(
		mc->getMainSynthChain(), name);
}

var RestHelpers::buildModuleTree(const ProcessorOrValueTree& root, const TreeOptions& options)
{
	auto md = root.getMetadata();
	auto metadata = md.toJSON();

	auto obj = metadata.getDynamicObject();

	if (!options.verbose)
	{
		obj->removeProperty("description");
		obj->removeProperty("builderPath");
		obj->removeProperty("metadataType");
		obj->removeProperty("interfaces");
	}

	obj->setProperty("bypassed", root.isBypassed());
	obj->setProperty("processorId", root.getId());
	obj->setProperty("colour", root.getColour());

	if (options.includeParameters)
	{
		auto parameters = metadata["parameters"];
		jassert(parameters.size() == md.parameters.size());
		int idx = 0;

		for (const auto& p : md.parameters)
		{
			auto po = parameters[idx].getDynamicObject();

			if (root.isRuntimeData())
			{
				auto value = root.getAttribute(p.parameterIndex);
				auto normValue = p.range.convertTo0to1(value, false);
				String valueAsString = p.vtc.active ? p.vtc(value) : String(value);

				po->setProperty("value", value);
				po->setProperty("valueNormalized", normValue);
				po->setProperty("valueAsString", valueAsString);
			}

			if (!options.verbose)
			{
				po->removeProperty("description");
				po->removeProperty("metadataType");
				po->removeProperty("mode");
				po->removeProperty("type");
				po->removeProperty("unit");
			}

			idx++;
		}
	}
	else
	{
		obj->removeProperty("parameters");
	}

	if (md.type == ProcessorMetadataIds::SoundGenerator)
	{
		auto mp = root.getChild(ModulatorSynth::InternalChains::MidiProcessor);

		Array<var> midiProcessors;
		
		for (int i = 0; i < mp.getNumChildren(); i++)
			midiProcessors.add(buildModuleTree(mp.getChild(i), options));
		
		obj->setProperty("midi", var(midiProcessors));
		
		Array<var> fxProcessors;

		auto fx = root.getChild(ModulatorSynth::InternalChains::EffectChain);

		for (int i = 0; i < fx.getNumChildren(); i++)
			fxProcessors.add(buildModuleTree(fx.getChild(i), options));

		obj->setProperty("fx", var(fxProcessors));

		if (md.hasChildren)
		{
			Array<var> children;

			auto offset = md.modulation.size() + 2;

			for (int i = offset; i < root.getNumChildren(); i++)
			{
				auto cp = root.getChild(i);
				children.add(buildModuleTree(cp, options));
			}

			obj->setProperty("children", var(children));
		}
	}

	auto modData = obj->getProperty("modulation");

	if (auto md = modData.getArray())
	{
		for (auto& modChain : *md)
		{
			auto mo = modChain.getDynamicObject();

			
			
			if (!options.verbose)
			{
				mo->removeProperty("description");
				mo->removeProperty("metadataType");
			}

			Array<var> children;

			if (modChain["disabled"])
			{
				HiseModulationColours hc;

				mo->setProperty("colour", "#" + hc.getColour(HiseModulationColours::ColourId::ExtraMod).toDisplayString(false));

				modChain.getDynamicObject()->setProperty("children", var(children));

				continue;
			}

			auto idx = (int)modChain["chainIndex"];
			auto mc = root.getChild(idx);

			if(mc.getType() == ModulatorChain::getClassType())
			{
				mo->setProperty("colour", mc.getColour());

				for (int i = 0; i < mc.getNumChildren(); i++)
					children.add(buildModuleTree(mc.getChild(i), options));
			}

			modChain.getDynamicObject()->setProperty("children", var(children));
		}
	}

	return metadata;
}

Array<var> RestHelpers::buildChainArray(Processor* parent, int chainIndex)
{
	// TODO: Implement chain array building
	// Get chain from parent, iterate processors, call buildModuleTree on each
	return Array<var>();
}

//==============================================================================
// MidiInjector implementation
//==============================================================================

MidiInjector::MidiInjector(MainController* mc_)
	: mc(mc_)
{
}

MidiInjector::~MidiInjector()
{
	stopTimer();
}

void MidiInjector::queueMessages(const Array<var>& messages)
{
	auto now = Time::getMillisecondCounterHiRes();

	ScopedLock sl(lock);

	// If nothing is playing, reset counters for a fresh sequence
	bool wasIdle = scheduledEvents.isEmpty() && activeNotes.isEmpty();

	if (wasIdle)
	{
		totalEvents = 0;
		playedEvents = 0;
		sequenceStartMs = now;
		sequenceEndMs = now;
	}

	for (const auto& msg : messages)
	{
		auto type = msg.getProperty(RestApiIds::type, "").toString();

		if (type.isEmpty())
			continue;

		ScheduledEvent e;
		e.type = type;
		e.channel = (int)msg.getProperty(RestApiIds::channel, 1);
		e.noteNumber = (int)msg.getProperty(RestApiIds::noteNumber, 0);
		e.velocity = (float)msg.getProperty(RestApiIds::velocity, 1.0);
		e.controller = (int)msg.getProperty(RestApiIds::controller, 0);
		e.value = (int)msg.getProperty(RestApiIds::value, 0);
		e.duration = (int)msg.getProperty(RestApiIds::duration, 500);
		e.expression = msg.getProperty(RestApiIds::expression, "").toString();
		e.replId = msg.getProperty(RestApiIds::id, "").toString();
		e.moduleId = msg.getProperty(RestApiIds::moduleId, "Interface").toString();
		e.attributeValue = (float)msg.getProperty(RestApiIds::value, 0.0);

		// testsignal fields
		e.signal = msg.getProperty(RestApiIds::signal, "").toString();
		e.frequency = (float)msg.getProperty(RestApiIds::frequency, 440.0);
		e.startFrequency = (float)msg.getProperty(RestApiIds::startFrequency, 20.0);
		e.endFrequency = (float)msg.getProperty(RestApiIds::endFrequency, 20000.0);

		// set_attribute uses processorId instead of moduleId
		if (type == "set_attribute")
		{
			e.moduleId = msg.getProperty(RestApiIds::processorId, "Interface").toString();
			e.parameterIndex = (int)msg.getProperty(Identifier("_resolvedIndex"), -1);
			e.attributeValue = (float)msg.getProperty(RestApiIds::value, 0.0);
		}

		e.fireTimeMs = now + (double)(int)msg.getProperty(RestApiIds::timestamp, 0);

		// allNotesOff with delay=0 triggers immediate panic
		if (type == "allNotesOff" && e.fireTimeMs <= now)
		{
			panic();
			// Don't queue anything after allNotesOff in this batch
			break;
		}

		// Insert sorted by fireTimeMs
		int insertIdx = 0;
		while (insertIdx < scheduledEvents.size() &&
		       scheduledEvents[insertIdx].fireTimeMs <= e.fireTimeMs)
			insertIdx++;

		scheduledEvents.insert(insertIdx, e);
		totalEvents++;

		// Update sequence end time (account for note duration)
		double eventEndMs = e.fireTimeMs;

		if (type == "note")
			eventEndMs += e.duration;

		if (eventEndMs > sequenceEndMs)
			sequenceEndMs = eventEndMs;
	}

	scheduleNextCallback();
}

MidiInjector::Status MidiInjector::getStatus() const
{
	ScopedLock sl(lock);

	Status s;
	s.isPlaying = !scheduledEvents.isEmpty() || !activeNotes.isEmpty();
	s.activeNotes = activeNotes.size();
	s.eventsInSequence = totalEvents;
	s.playedEvents = playedEvents;

	if (totalEvents > 0 && sequenceEndMs > sequenceStartMs)
	{
		auto now = Time::getMillisecondCounterHiRes();
		auto elapsed = now - sequenceStartMs;
		auto totalDuration = sequenceEndMs - sequenceStartMs;

		s.durationMs = roundToInt(totalDuration);
		s.progress = s.isPlaying ? jlimit(0.0, 1.0, elapsed / totalDuration) : 1.0;
	}
	else
	{
		s.durationMs = 0;
		s.progress = s.isPlaying ? 0.0 : 1.0;
	}

	return s;
}

void MidiInjector::hiResTimerCallback()
{
	ScopedLock sl(lock);

	auto now = Time::getMillisecondCounterHiRes();

	// Fire all due scheduled events
	while (!scheduledEvents.isEmpty() && scheduledEvents.getFirst().fireTimeMs <= now)
	{
		auto e = scheduledEvents.removeAndReturn(0);
		fireEvent(e);
	}

	// Fire all due note-offs
	fireDueNoteOffs(now);

	// Schedule next or stop
	scheduleNextCallback();
}

void MidiInjector::fireEvent(const ScheduledEvent& e)
{
	if (e.type == "allNotesOff")
	{
		panic();
		return;
	}

	if (e.type == "repl")
	{
		fireReplEvent(e);
		playedEvents++;
		return;
	}

	if (e.type == "set_attribute")
	{
		if (auto* processor = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), e.moduleId))
			processor->setAttribute(e.parameterIndex, e.attributeValue, dispatch::DispatchType::sendNotificationAsync);

		playedEvents++;
		return;
	}

	if (e.type == "testsignal")
	{
		fireTestSignalEvent(e);
		playedEvents++;
		return;
	}

	auto noteOff = RestHelpers::dispatchSingleMidiMessage(
		mc, e.type, e.channel, e.noteNumber, e.velocity, e.controller, e.value);

	playedEvents++;

	if (noteOff.valid)
	{
		// Convert to ActiveNote with absolute note-off time
		ActiveNote an;
		an.channel = noteOff.channel;
		an.noteNumber = noteOff.noteNumber;
		an.noteOffTimeMs = Time::getMillisecondCounterHiRes() + e.duration;

		// Insert sorted by noteOffTimeMs
		int insertIdx = 0;
		while (insertIdx < activeNotes.size() &&
		       activeNotes[insertIdx].noteOffTimeMs <= an.noteOffTimeMs)
			insertIdx++;

		activeNotes.insert(insertIdx, an);
	}
}

void MidiInjector::fireNoteOff(const ActiveNote& n)
{
	mc->getKeyboardState().noteOff(n.channel, n.noteNumber, 1.0f);
}

void MidiInjector::fireDueNoteOffs(double now)
{
	while (!activeNotes.isEmpty() && activeNotes.getFirst().noteOffTimeMs <= now)
	{
		auto n = activeNotes.removeAndReturn(0);
		fireNoteOff(n);
	}
}

void MidiInjector::fireReplEvent(const ScheduledEvent& e)
{
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), e.moduleId));

	DynamicObject::Ptr entry = new DynamicObject();

	if (e.replId.isNotEmpty())
		entry->setProperty(RestApiIds::id, e.replId);

	entry->setProperty(RestApiIds::expression, e.expression);
	entry->setProperty(RestApiIds::moduleId, e.moduleId);
	entry->setProperty(RestApiIds::timestamp, roundToInt(e.fireTimeMs - sequenceStartMs));

	if (jp == nullptr)
	{
		entry->setProperty(RestApiIds::success, false);
		entry->setProperty(RestApiIds::value, "module not found: " + e.moduleId);
	}
	else if (auto engine = jp->getScriptEngine())
	{
		auto r = Result::ok();
		auto v = engine->evaluate(e.expression, &r);

		if (v.isUndefined() || v.isVoid())
			v = "undefined";

		entry->setProperty(RestApiIds::success, r.wasOk());
		entry->setProperty(RestApiIds::value, v);

		if (!r.wasOk())
		{
			auto scriptRoot = mc->getSampleManager().getProjectHandler()
				.getSubDirectory(FileHandlerBase::Scripts);

			auto errorLines = StringArray::fromLines(r.getErrorMessage());
			auto parsed = RestHelpers::BaseScopedConsoleHandler::parseError(
				errorLines[0], scriptRoot, e.moduleId);

			entry->setProperty(RestApiIds::errorMessage, parsed.message);

			if (parsed.location.isNotEmpty())
				entry->setProperty(RestApiIds::location, parsed.location);

			// Parse callstack entries
			Array<var> callstack;

			for (int ci = 1; ci < errorLines.size(); ci++)
			{
				auto csEntry = RestHelpers::BaseScopedConsoleHandler::parseError(
					errorLines[ci], scriptRoot, e.moduleId);

				if (csEntry.location.isNotEmpty())
					callstack.add(csEntry.toCallstackString());
			}

			if (!callstack.isEmpty())
				entry->setProperty(RestApiIds::callstack, var(callstack));
		}
	}
	else
	{
		entry->setProperty(RestApiIds::success, false);
		entry->setProperty(RestApiIds::value, "no script engine present");
	}

	replResults.add(var(entry.get()));
}

Array<var> MidiInjector::takeReplResults()
{
	ScopedLock sl(lock);
	Array<var> results;
	results.swapWith(replResults);
	return results;
}

void MidiInjector::panic()
{
	// Fire note-off for all active notes
	for (const auto& n : activeNotes)
		fireNoteOff(n);

	activeNotes.clear();
	scheduledEvents.clear();
	mc->allNotesOff();
	mc->stopBufferToPlay();

	totalEvents = 0;
	playedEvents = 0;
	sequenceStartMs = 0;
	sequenceEndMs = 0;

	stopTimer();
}

void MidiInjector::scheduleNextCallback()
{
	if (scheduledEvents.isEmpty() && activeNotes.isEmpty())
	{
		stopTimer();
		return;
	}

	auto now = Time::getMillisecondCounterHiRes();
	double nextTime = std::numeric_limits<double>::max();

	if (!scheduledEvents.isEmpty())
		nextTime = jmin(nextTime, scheduledEvents.getFirst().fireTimeMs);

	if (!activeNotes.isEmpty())
		nextTime = jmin(nextTime, activeNotes.getFirst().noteOffTimeMs);

	int deltaMs = jmax(1, roundToInt(nextTime - now));
	startTimer(deltaMs);
}

//==============================================================================
// Test signal generation
//==============================================================================

void MidiInjector::fireTestSignalEvent(const ScheduledEvent& e)
{
	auto sampleRate = mc->getMainSynthChain()->getSampleRate();

	if (sampleRate <= 0.0)
		return;

	auto key = makeSignalCacheKey(e, sampleRate);

	if (!signalCache.contains(key))
	{
		int numSamples = roundToInt(sampleRate * e.duration / 1000.0);

		if (numSamples <= 0)
			numSamples = 1;

		auto buffer = generateSignal(e.signal, sampleRate, numSamples,
		                              e.frequency, e.startFrequency, e.endFrequency);

		signalCache.set(key, std::move(buffer));
	}

	mc->setBufferToPlay(signalCache[key], sampleRate);
}

String MidiInjector::makeSignalCacheKey(const ScheduledEvent& e, double sampleRate)
{
	String key;
	key << e.signal << "|" << String(sampleRate) << "|" << String(e.duration);

	if (e.signal == "sine" || e.signal == "saw")
		key << "|" << String(e.frequency);
	else if (e.signal == "sweep")
		key << "|" << String(e.startFrequency) << "|" << String(e.endFrequency);

	return key;
}

AudioSampleBuffer MidiInjector::generateSignal(const String& signal, double sampleRate,
    int numSamples, float frequency, float startFreq, float endFreq)
{
#if 0
	AudioSampleBuffer buffer(2, numSamples);
	buffer.clear();

	auto* ch0 = buffer.getWritePointer(0);

	if (signal == "sine")
	{
		for (int i = 0; i < numSamples; i++)
		{
			double t = (double)i / sampleRate;
			ch0[i] = (float)std::sin(2.0 * MathConstants<double>::pi * frequency * t);
		}
	}
	else if (signal == "saw")
	{
		for (int i = 0; i < numSamples; i++)
		{
			double t = (double)i / sampleRate;
			double phase = std::fmod(frequency * t, 1.0);
			ch0[i] = (float)(2.0 * phase - 1.0);
		}
	}
	else if (signal == "sweep")
	{
		// Logarithmic sine sweep
		double duration = (double)numSamples / sampleRate;
		double logRatio = std::log(endFreq / startFreq);

		for (int i = 0; i < numSamples; i++)
		{
			double t = (double)i / sampleRate;
			double instantFreq = startFreq * std::exp(logRatio * t / duration);
			double phase = 2.0 * MathConstants<double>::pi * startFreq * duration / logRatio
			             * (std::exp(logRatio * t / duration) - 1.0);
			ch0[i] = (float)std::sin(phase);
		}
	}
	else if (signal == "dirac")
	{
		ch0[0] = 1.0f;
	}
	else if (signal == "noise")
	{
		Random rng;

		for (int i = 0; i < numSamples; i++)
			ch0[i] = rng.nextFloat() * 2.0f - 1.0f;
	}
	// "silence" - buffer is already cleared

	// Copy channel 0 to channel 1
	FloatVectorOperations::copy(buffer.getWritePointer(1), ch0, numSamples);

	return buffer;
#endif
	return {};
}

//==============================================================================
// dispatchSingleMidiMessage
//==============================================================================

RestHelpers::PendingNoteOff RestHelpers::dispatchSingleMidiMessage(
	MainController* mc, const String& type, int channel,
	int noteNumber, float velocity, int controller, int value)
{
	auto& ks = mc->getKeyboardState();

	if (type == "note")
	{
		ks.noteOn(channel, noteNumber, velocity);

		PendingNoteOff result;
		result.channel = channel;
		result.noteNumber = noteNumber;
		result.valid = true;
		return result;
	}
	else if (type == "cc")
	{
		ks.injectMessage(MidiMessage::controllerEvent(channel, controller, value));
	}
	else if (type == "pitchbend")
	{
		ks.injectMessage(MidiMessage::pitchWheel(channel, value));
	}
	else if (type == "allNotesOff")
	{
		mc->allNotesOff();
	}

	return {};
}

//==============================================================================
// handleTestingSequence
//==============================================================================

RestServer::Response RestHelpers::handleTestingSequence(BackendProcessor* bp,
                                                        RestServer::AsyncRequest::Ptr req)
{
	auto* injector = bp->getMidiInjector();

	if (injector == nullptr)
		return req->fail(503, "MIDI injector not available");

	auto body = req->getRequest().getJsonBody();
	auto messagesVar = body.getProperty(RestApiIds::messages, var());

	if (!messagesVar.isArray())
		return req->fail(400, "Missing or invalid 'messages' array");

	auto* messagesArray = messagesVar.getArray();

	// Validate messages before queuing
	for (int i = 0; i < messagesArray->size(); i++)
	{
		auto& msg = messagesArray->getReference(i);
		auto type = msg.getProperty(RestApiIds::type, "").toString();

		if (type.isEmpty())
			return req->fail(400, "Message at index " + String(i) + " missing 'type' field");

		if (type != "note" && type != "cc" && type != "pitchbend" && type != "allNotesOff" && type != "repl" && type != "set_attribute" && type != "testsignal")
			return req->fail(400, "Message at index " + String(i) + " has unknown type: " + type);

		if (type == "testsignal")
		{
			auto signal = msg.getProperty(RestApiIds::signal, "").toString();

			if (signal.isEmpty())
				return req->fail(400, "testsignal at index " + String(i) + " missing 'signal'");

			if (signal != "sine" && signal != "saw" && signal != "sweep" &&
			    signal != "dirac" && signal != "noise" && signal != "silence")
				return req->fail(400, "testsignal at index " + String(i) + " has unknown signal type: " + signal);

			if ((signal == "sine" || signal == "saw") && msg.hasProperty(RestApiIds::frequency))
			{
				float freq = (float)msg.getProperty(RestApiIds::frequency, 440.0);

				if (freq <= 0.0f)
					return req->fail(400, "testsignal at index " + String(i) + " has invalid frequency: " + String(freq));
			}

			if (signal == "sweep")
			{
				float startFreq = (float)msg.getProperty(RestApiIds::startFrequency, 20.0);
				float endFreq = (float)msg.getProperty(RestApiIds::endFrequency, 20000.0);

				if (startFreq <= 0.0f || endFreq <= 0.0f)
					return req->fail(400, "testsignal at index " + String(i) + " has invalid sweep frequency range");
			}
		}
		else if (type == "set_attribute")
		{
			auto processorId = msg.getProperty(RestApiIds::processorId, "Interface").toString();
			auto parameterId = msg.getProperty(RestApiIds::parameterId, "").toString();

			if (parameterId.isEmpty())
				return req->fail(400, "set_attribute at index " + String(i) + " missing 'parameterId'");

			if (!msg.hasProperty(RestApiIds::value))
				return req->fail(400, "set_attribute at index " + String(i) + " missing 'value'");

			auto* processor = ProcessorHelpers::getFirstProcessorWithName(bp->getMainSynthChain(), processorId);

			if (processor == nullptr)
				return req->fail(400, "set_attribute at index " + String(i) + ": processor not found: " + processorId);

			auto metadata = processor->getMetadata();
			int resolvedIndex = -1;

			for (int p = 0; p < metadata.parameters.size(); p++)
			{
				if (metadata.parameters[p].id == Identifier(parameterId))
				{
					resolvedIndex = metadata.parameters[p].parameterIndex;

					// Validate value against range
					auto range = metadata.parameters[p].range;
					float val = (float)msg.getProperty(RestApiIds::value, 0.0);

					if (val < (float)range.rng.start || val > (float)range.rng.end)
						return req->fail(400, "set_attribute at index " + String(i) + ": value " + String(val)
							+ " out of range [" + String(range.rng.start) + ", " + String(range.rng.end) + "] for parameter " + parameterId);

					break;
				}
			}

			if (resolvedIndex == -1)
				return req->fail(400, "set_attribute at index " + String(i) + ": parameter not found: " + parameterId + " on processor " + processorId);

			// Store resolved index back into the message for queueMessages to pick up
			msg.getDynamicObject()->setProperty(Identifier("_resolvedIndex"), resolvedIndex);
		}
		else if (type == "repl")
		{
			auto expr = msg.getProperty(RestApiIds::expression, "").toString();

			if (expr.isEmpty())
				return req->fail(400, "REPL message at index " + String(i) + " missing 'expression'");
		}
		else if (type == "note")
		{
			if (!msg.hasProperty(RestApiIds::noteNumber))
				return req->fail(400, "Note message at index " + String(i) + " missing 'noteNumber'");

			int noteNumber = (int)msg.getProperty(RestApiIds::noteNumber, 0);

			if (noteNumber < 0 || noteNumber > 127)
				return req->fail(400, "Note message at index " + String(i) + " has invalid noteNumber: " + String(noteNumber));
		}
		else if (type == "cc")
		{
			if (!msg.hasProperty(RestApiIds::controller))
				return req->fail(400, "CC message at index " + String(i) + " missing 'controller'");

			if (!msg.hasProperty(RestApiIds::value))
				return req->fail(400, "CC message at index " + String(i) + " missing 'value'");

			int ctrl = (int)msg.getProperty(RestApiIds::controller, 0);
			int val = (int)msg.getProperty(RestApiIds::value, 0);

			if (ctrl < 0 || ctrl > 127)
				return req->fail(400, "CC message at index " + String(i) + " has invalid controller: " + String(ctrl));

			if (val < 0 || val > 127)
				return req->fail(400, "CC message at index " + String(i) + " has invalid value: " + String(val));
		}
		else if (type == "pitchbend")
		{
			if (!msg.hasProperty(RestApiIds::value))
				return req->fail(400, "Pitchbend message at index " + String(i) + " missing 'value'");

			int val = (int)msg.getProperty(RestApiIds::value, 0);

			if (val < 0 || val > 16383)
				return req->fail(400, "Pitchbend message at index " + String(i) + " has invalid value: " + String(val));
		}

		// Validate channel if present (only for MIDI message types)
		if (type != "allNotesOff" && type != "repl" && type != "set_attribute" && type != "testsignal" && msg.hasProperty(RestApiIds::channel))
		{
			int ch = (int)msg.getProperty(RestApiIds::channel, 1);

			if (ch < 1 || ch > 16)
				return req->fail(400, "Message at index " + String(i) + " has invalid channel: " + String(ch));
		}
	}

	// Start audio recording if outputFile specified
	auto recordOutput = body.getProperty(RestApiIds::recordOutput, "").toString();
	bool isRecording = false;

	if (recordOutput.isNotEmpty())
	{
		// Compute sequence duration from messages
		double maxEndMs = 0.0;

		for (int i = 0; i < messagesArray->size(); i++)
		{
			auto& msg = messagesArray->getReference(i);
			double ts = (double)(int)msg.getProperty(RestApiIds::timestamp, 0);
			double dur = (double)(int)msg.getProperty(RestApiIds::duration, 500);
			auto type = msg.getProperty(RestApiIds::type, "").toString();

			double endMs = ts;

			if (type == "note" || type == "testsignal")
				endMs += dur;

			if (endMs > maxEndMs)
				maxEndMs = endMs;
		}

		double recordSeconds = (maxEndMs / 1000.0) + 0.1; // add 100ms margin

		if (recordSeconds < 0.1)
			recordSeconds = 0.1;

		File outputFile(recordOutput);
		bp->getDebugLogger().startRecording(recordSeconds, outputFile);
		isRecording = true;
	}

	// Queue messages
	injector->queueMessages(*messagesArray);

	// If blocking mode, wait for sequence to complete (with 30s timeout)
	// Recording implies blocking - we need to wait for the sequence to finish
	bool blocking = isRecording || getTrueValue(body.getProperty(RestApiIds::blocking, false));

	if (blocking)
	{
		auto startWait = Time::getMillisecondCounterHiRes();
		constexpr double maxWaitMs = 30000.0;

		while (injector->getStatus().isPlaying)
		{
			if (Time::getMillisecondCounterHiRes() - startWait > maxWaitMs)
				return req->fail(500, "Blocking sequence timed out after 30 seconds");

			Thread::sleep(5);
		}

		// If recording, wait a bit more for the DebugLogger to finish writing
		if (isRecording)
			Thread::sleep(200);
	}

	// Build flat response
	auto status = injector->getStatus();

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::isPlaying, status.isPlaying);
	result->setProperty(RestApiIds::durationMs, status.durationMs);
	result->setProperty(RestApiIds::activeNotes, status.activeNotes);
	result->setProperty(RestApiIds::eventsInSequence, status.eventsInSequence);
	result->setProperty(RestApiIds::playedEvents, status.playedEvents);
	result->setProperty(RestApiIds::progress, status.progress);

	if (isRecording)
		result->setProperty(RestApiIds::recordOutput, recordOutput);

	// Include any REPL results accumulated since last call
	auto replResults = injector->takeReplResults();

	if (!replResults.isEmpty())
		result->setProperty(RestApiIds::replResults, var(replResults));

	result->setProperty(RestApiIds::logs, var(Array<var>()));
	result->setProperty(RestApiIds::errors, var(Array<var>()));

	return RestServer::Response::ok(var(result.get()));
}

// ============================================================================
// DSP (scriptnode) endpoints
// ============================================================================

using namespace scriptnode;

static DspNetwork::Holder* getNetworkHolder(MainController* mc, const String& moduleId)
{
	auto p = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId);

	if (p == nullptr)
		return nullptr;

	return dynamic_cast<DspNetwork::Holder*>(p);
}

static DspNetwork* getActiveNetwork(MainController* mc, const String& moduleId)
{
	if (auto holder = getNetworkHolder(mc, moduleId))
		return holder->getActiveOrDebuggedNetwork();

	return nullptr;
}

static var buildDspNodeTree(const ValueTree& nodeTree, bool verbose, bool includeConnections)
{
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(RestApiIds::nodeId, nodeTree[PropertyIds::ID].toString());
	obj->setProperty(RestApiIds::factoryPath, nodeTree[PropertyIds::FactoryPath].toString());
	obj->setProperty(RestApiIds::bypassed, (bool)nodeTree[PropertyIds::Bypassed]);

	// Parameters
	Array<var> params;
	auto paramTree = nodeTree.getChildWithName(PropertyIds::Parameters);

	for (int i = 0; i < paramTree.getNumChildren(); i++)
	{
		auto p = paramTree.getChild(i);
		DynamicObject::Ptr paramObj = new DynamicObject();
		paramObj->setProperty(RestApiIds::parameterId, p[PropertyIds::ID].toString());
		paramObj->setProperty(RestApiIds::value, p.getProperty(PropertyIds::Value, 0.0));

		if (verbose)
		{
			paramObj->setProperty(RestApiIds::min, p.getProperty(PropertyIds::MinValue, 0.0));
			paramObj->setProperty(RestApiIds::max, p.getProperty(PropertyIds::MaxValue, 1.0));
			paramObj->setProperty(RestApiIds::stepSize, p.getProperty(PropertyIds::StepSize, 0.0));
			paramObj->setProperty(RestApiIds::defaultValue, p.getProperty(PropertyIds::DefaultValue, 0.0));

			auto skew = (double)p.getProperty(PropertyIds::SkewFactor, 1.0);
			if (skew != 1.0)
			{
				auto minVal = (double)p.getProperty(PropertyIds::MinValue, 0.0);
				auto maxVal = (double)p.getProperty(PropertyIds::MaxValue, 1.0);
				auto mid = minVal + (maxVal - minVal) * std::pow(0.5, skew);
				paramObj->setProperty(RestApiIds::middlePosition, mid);
			}
		}

		params.add(var(paramObj.get()));
	}

	obj->setProperty(RestApiIds::parameters, var(params));

	auto isContainer = nodeTree.getChildWithName(PropertyIds::Nodes).isValid();

	auto propList = rest_undo::dsp::Helpers::getInlineNodeProperties(isContainer);

	Array<var> properties;

	for (auto p : propList)
	{
		if (nodeTree.hasProperty(p))
		{
			DynamicObject::Ptr pobj = new DynamicObject();
			pobj->setProperty(RestApiIds::propertyId, p.toString());
			pobj->setProperty(RestApiIds::value, nodeTree[p]);
			properties.add(pobj.get());
		}
	}

	for (auto p : nodeTree.getChildWithName(PropertyIds::Properties))
	{
		if (p[PropertyIds::ID].toString() == "Connection")
			continue;

		DynamicObject::Ptr pobj = new DynamicObject();
		pobj->setProperty(RestApiIds::propertyId, p[PropertyIds::ID]);
		pobj->setProperty(RestApiIds::value, p[PropertyIds::Value]);
		properties.add(pobj.get());
	}

	obj->setProperty(RestApiIds::properties, var(properties));

	if (includeConnections)
	{
		// Connections (modulation targets on parameters)
		Array<var> connections;

		valuetree::Helpers::forEach(nodeTree, [&](const ValueTree& c)
		{
			if (c.getType() == PropertyIds::Property && c[PropertyIds::ID].toString() == PropertyIds::Connection.toString())
			{
				auto receiveIds = StringArray::fromTokens(c[PropertyIds::Value].toString(), ",", "");

				auto parentId = valuetree::Helpers::findParentWithType(c, PropertyIds::Node)[PropertyIds::ID].toString();

				for (auto con : receiveIds)
				{
					DynamicObject::Ptr np = new DynamicObject();
					np->setProperty(RestApiIds::source, parentId);
					np->setProperty(RestApiIds::sourceOutput, "routing");
					np->setProperty(RestApiIds::target, con);
					np->setProperty(RestApiIds::parameter, "");

					connections.add(var(np.get()));
				}
			}

			if (c.getType() == PropertyIds::Connection)
			{
				auto parentId = valuetree::Helpers::findParentWithType(c, PropertyIds::Node)[PropertyIds::ID].toString();

				DynamicObject::Ptr np = new DynamicObject();
				np->setProperty(RestApiIds::source, parentId);

				auto pParent = valuetree::Helpers::findParentWithType(c, PropertyIds::Parameter);
				auto mParent = valuetree::Helpers::findParentWithType(c, PropertyIds::ModulationTargets);
				auto sParent = valuetree::Helpers::findParentWithType(c, PropertyIds::SwitchTarget);

				if (pParent.isValid())
					np->setProperty(RestApiIds::sourceOutput, pParent[PropertyIds::ID]);
				else if (mParent.isValid())
					np->setProperty(RestApiIds::sourceOutput, 0);
				else
					np->setProperty(RestApiIds::sourceOutput, sParent.getParent().indexOf(sParent));

				np->setProperty(RestApiIds::target, c[PropertyIds::NodeId]);
				np->setProperty(RestApiIds::parameter, c[PropertyIds::ParameterId]);

				connections.add(var(np.get()));
			}

			return false;
		});

		obj->setProperty(RestApiIds::connections, var(connections));
	}

	// Children
	Array<var> children;
	auto nodesTree = nodeTree.getChildWithName(PropertyIds::Nodes);

	for (int i = 0; i < nodesTree.getNumChildren(); i++)
		children.add(buildDspNodeTree(nodesTree.getChild(i), verbose, false));

	obj->setProperty(RestApiIds::children, var(children));

	return var(obj.get());
}

RestServer::Response RestHelpers::handleDspList(MainController* mc,
                                                 RestServer::AsyncRequest::Ptr req)
{
	StringArray networkNames;

	// Scan on-disk .xml files
	auto files = BackendDllManager::getNetworkFiles(mc, false);
	for (auto& f : files)
		networkNames.addIfNotAlreadyThere(f.getFileNameWithoutExtension());

	// Also include in-memory networks from any holder
	Processor::Iterator<JavascriptProcessor> iter(mc->getMainSynthChain());

	while (auto jp = iter.getNextProcessor())
	{
		if (auto holder = dynamic_cast<DspNetwork::Holder*>(jp))
		{
			for (auto& id : holder->getIdList())
				networkNames.addIfNotAlreadyThere(id);
		}
	}

	Array<var> nameArray;
	for (auto& n : networkNames)
		nameArray.add(n);

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::networks, var(nameArray));
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleDspInit(MainController* mc,
                                                 RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto mid = obj[RestApiIds::moduleId].toString();
	auto networkName = obj[RestApiIds::name].toString();

	if (mid.isEmpty())
		return req->fail(400, "moduleId is required");

	if (networkName.isEmpty())
		return req->fail(400, "name is required");

	auto mode = obj.getProperty(RestApiIds::mode, "auto").toString();
	if (mode != "create" && mode != "load" && mode != "auto")
		return req->fail(400, "mode must be one of: create, load, auto");

	auto holder = getNetworkHolder(mc, mid);
	if (holder == nullptr)
		return req->fail(404, "Module " + mid + " is not a DspNetwork holder");

	auto networkFolder = BackendDllManager::getSubFolder(mc, BackendDllManager::FolderSubType::Networks);
	auto xmlFile = networkFolder.getChildFile(networkName).withFileExtension("xml");
	auto filePath = xmlFile.getFullPathName();

	if (mode == "create" && xmlFile.existsAsFile())
		return req->fail(409, "Network XML already exists: " + filePath);

	if (mode == "load" && !xmlFile.existsAsFile())
		return req->fail(404, "No network XML found: " + filePath);

	bool existsAlready = xmlFile.existsAsFile();

	auto network = holder->getOrCreate(networkName);
	if (network == nullptr)
		return req->fail(500, "Failed to create network " + networkName);

	auto p = dynamic_cast<Processor*>(holder);

	p->prepareToPlay(p->getSampleRate(), p->getLargestBlockSize());

	auto tree = network->getValueTree();
	auto rootNode = tree.getChild(0); // First child is the root container node

	var treeJson = buildDspNodeTree(rootNode, false, true);

	if (auto brw = dynamic_cast<BackendProcessor*>(mc)->currentRootWindow)
	{
		MessageManager::callAsync([brw, p]()
		{
			brw->gotoIfWorkspace(p);
		});
	}

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::result, treeJson);

	if(mode == "auto")
		result->setProperty(RestApiIds::filePath, filePath);

	Array<var> logs;

	result->setProperty(RestApiIds::source, existsAlready ? "loaded" : "created");

	if (existsAlready)
		logs.add("Loaded network from XML " + xmlFile.getFileName());
	else
		logs.add("Created new network");

	result->setProperty(RestApiIds::logs, logs);
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleDspTree(MainController* mc,
                                                 RestServer::AsyncRequest::Ptr req)
{
	auto mid = req->getRequest()[RestApiIds::moduleId];

	if (mid.isEmpty())
		return req->fail(400, "moduleId query parameter is required");

	bool verbose = req->getRequest().getTrueValue(RestApiIds::verbose);
	auto group = req->getRequest()[RestApiIds::group];

	ValueTree rootNode;

	if (group.isNotEmpty())
	{
		if (group != "current")
			return req->fail(501, "only 'current' group is supported");

		auto um = RestServerUndoManager::Instance::getOrCreate(mc, ApiRoute::DspTree);
		auto dspState = um->getCurrentDspValidationState();

		if (dspState == nullptr || !dspState->networkTree.isValid())
			return req->fail(400, "No current DSP validation state");

		// networkTree is the Network-typed ValueTree; root Node is first child.
		rootNode = dspState->networkTree.getChild(0);
	}
	else
	{
		auto network = getActiveNetwork(mc, mid);
		if (network == nullptr)
			return req->fail(404, "No active DspNetwork for module: " + mid);

		auto tree = network->getValueTree();
		rootNode = tree.getChild(0);
	}

	var treeJson = buildDspNodeTree(rootNode, verbose, true);

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::result, treeJson);
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleDspApply(MainController* mc,
                                                  RestServer::AsyncRequest::Ptr req)
{
	static constexpr ApiRoute CurrentEndpoint = ApiRoute::DspApply;

	std::vector<RestServerUndoManager::CallStack> errorCallstack;

	req->setUseCustomErrors(true);
	auto obj = req->getRequest().getJsonBody();

	auto mid = obj[RestApiIds::moduleId].toString();
	if (mid.isEmpty())
		return req->fail(400, "moduleId is required");

	auto network = getActiveNetwork(mc, mid);
	if (network == nullptr)
		return req->fail(404, "No active DspNetwork for module: " + mid);

	auto ops = obj[RestApiIds::operations];
	if (!ops.isArray())
		return req->fail(400, "operations must be an array");

	if (ops.size() == 0)
		return req->fail(400, "operations array must not be empty");

	// Inject moduleId into each operation so action classes can resolve the network
	for (int i = 0; i < ops.size(); i++)
	{
		if (auto* opObj = ops[i].getDynamicObject())
			opObj->setProperty(RestApiIds::moduleId, mid);
	}

	auto noErrors = true;
	auto um = RestServerUndoManager::Instance::getOrCreate(mc, CurrentEndpoint);

	// Phase 1: Validate required fields per operation type
	for (int i = 0; i < ops.size(); i++)
	{
		auto ok = um->prevalidate(RestServerUndoManager::Domain::DSP, ops[i]);

		if (!ok)
		{
			noErrors = false;

			errorCallstack.push_back(RestServerUndoManager::CallStack(ok.getErrorMessage())
				.withGroup(um->getCurrentGroupId())
				.withPhase(RestServerUndoManager::CallStack::Phase::Prevalidation)
				.withOperation(i, ops[i][RestApiIds::op].toString())
				.withEndpoint(CurrentEndpoint));
		}
	}

	if (!noErrors)
	{
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, false);
		result->setProperty(RestApiIds::result, var());
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, RestServerUndoManager::CallStack::toJSONList(errorCallstack));
		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}

	// Phase 2: Create actions and validate semantics
	using ActionBase = RestServerUndoManager::ActionBase;
	ActionBase::List actions;

	for (int i = 0; i < ops.size(); i++)
	{
		auto ad = um->createAction(RestServerUndoManager::Domain::DSP, ops[i]);
		auto ok = ad->validate();

		if (ok)
			actions.add(ad);
		else
		{
			errorCallstack.push_back(RestServerUndoManager::CallStack(ok.getErrorMessage())
				.withGroup(um->getCurrentGroupId())
				.withPhase(RestServerUndoManager::CallStack::Phase::Validation)
				.withOperation(i, ad->getDescription())
				.withEndpoint(CurrentEndpoint));

			noErrors = false;
		}
	}

	if (!noErrors)
	{
		DynamicObject::Ptr result = new DynamicObject();
		result->setProperty(RestApiIds::success, false);
		result->setProperty(RestApiIds::result, var());
		result->setProperty(RestApiIds::logs, Array<var>());
		result->setProperty(RestApiIds::errors, RestServerUndoManager::CallStack::toJSONList(errorCallstack));
		req->complete(RestServer::Response::ok(var(result.get())));
		return req->waitForResponse();
	}

	// Phase 3: Execute via undo manager
	um->setValidationErrors(errorCallstack);

	um->performAction(req, actions, [](ActionBase::List l, bool undo)
	{
		if (!l.isEmpty())
		{
			auto mc = l.getFirst()->getMainController();

			for (auto a : l)
				debugToConsole(mc->getMainSynthChain(), a->getHistoryMessage(undo));
		}
	});

	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleDspSave(MainController* mc,
                                                 RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto mid = obj[RestApiIds::moduleId].toString();

	if (mid.isEmpty())
		return req->fail(400, "moduleId is required");

	auto holder = getNetworkHolder(mc, mid);
	if (holder == nullptr)
		return req->fail(404, "Module " + mid + " is not a DspNetwork holder");

	auto network = getActiveNetwork(mc, mid);
	if (network == nullptr)
		return req->fail(404, "No active DspNetwork for module: " + mid);

	auto networkName = network->getId();
	auto networkFolder = BackendDllManager::getSubFolder(mc, BackendDllManager::FolderSubType::Networks);
	auto targetFile = networkFolder.getChildFile(networkName).withFileExtension("xml");

	// Embedded networks have no file representation
	if (!targetFile.getParentDirectory().isDirectory())
		return req->fail(400, "Cannot save embedded network to file");

	auto xml = network->getValueTree().createXml();

	if (xml == nullptr)
		return req->fail(500, "Failed to serialize network");

	if (!xml->writeTo(targetFile))
		return req->fail(500, "Failed to write to " + targetFile.getFullPathName());

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::filePath, targetFile.getFullPathName());
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleDspScreenshot(MainController* mc,
                                                       RestServer::AsyncRequest::Ptr req)
{
	// Parse parameters (same shape as /api/testing/screenshot)
	auto moduleId = req->getRequest()[RestApiIds::moduleId];
	if (moduleId.isEmpty())
		return req->fail(400, "moduleId is required");

	auto scaleStr = req->getRequest()[RestApiIds::scale];
	float scale = scaleStr.isNotEmpty() ? scaleStr.getFloatValue() : 1.0f;

	if (scale != 0.5f && scale != 1.0f && scale != 2.0f)
		scale = 1.0f;

	auto outputPath = req->getRequest()[RestApiIds::outputPath];

	if (outputPath.isEmpty())
		return req->fail(400, "outputPath is required");

	if (!outputPath.endsWithIgnoreCase(".png"))
		return req->fail(400, "outputPath must end with .png extension");

	File outputFile;

	if (File::isAbsolutePath(outputPath))
		outputFile = File(outputPath);
	else
	{
		auto imgFolder = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::Images);
		outputFile = imgFolder.getChildFile(outputPath);
	}

	// Resolve active DspNetwork for the module
	auto holder = getNetworkHolder(mc, moduleId);
	if (holder == nullptr)
		return req->fail(404, "Module " + moduleId + " is not a DspNetwork holder");

	auto network = getActiveNetwork(mc, moduleId);
	if (network == nullptr)
		return req->fail(404, "No active DspNetwork for module: " + moduleId);

	auto holderProcessor = dynamic_cast<Processor*>(holder);
	if (holderProcessor == nullptr)
		return req->fail(500, "DspNetwork holder is not a Processor");

	// The DspNetworkGraph only exists inside the BackendRootWindow, so this
	// endpoint requires the HISE backend IDE to be running. Headless contexts
	// (unit tests, CLI) have no root window and cannot capture the graph.
	auto backendProcessor = dynamic_cast<BackendProcessor*>(mc);
	if (backendProcessor == nullptr || backendProcessor->currentRootWindow == nullptr)
		return req->fail(503, "No BackendRootWindow available -- dsp/screenshot requires the HISE IDE to be running");

	Image capturedImage;
	bool captureSuccess = false;
	WaitableEvent captureComplete;

	SafeAsyncCall::callAsyncIfNotOnMessageThread<Processor>(*holderProcessor, [&](Processor& p)
	{
		auto bpe = backendProcessor->currentRootWindow;

		if (bpe != nullptr && bpe->getCurrentWorkspaceProcessor() != &p)
			bpe->gotoIfWorkspace(&p);

		if (bpe != nullptr)
		{
			capturedImage = scriptnode::DspNetwork::createScreenshot(bpe, scale);
			captureSuccess = capturedImage.isValid();
		}

		captureComplete.signal();
	});

	if (!captureComplete.wait(1000))
		return req->fail(500, "screenshot capture timed out");

	if (!captureSuccess)
		return req->fail(500, "failed to capture screenshot");

	outputFile.deleteFile();
	outputFile.create();
	FileOutputStream fos(outputFile);

	if (fos.failedToOpen())
		return req->fail(500, "failed to open output file: " + outputFile.getFullPathName());

	PNGImageFormat pngFormat;
	if (!pngFormat.writeImageToStream(capturedImage, fos))
		return req->fail(500, "failed to write PNG to file");

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::moduleId, moduleId);
	result->setProperty(RestApiIds::filePath, outputFile.getFullPathName());
	result->setProperty(RestApiIds::width, capturedImage.getWidth());
	result->setProperty(RestApiIds::height, capturedImage.getHeight());
	result->setProperty(RestApiIds::scale, scale);
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

//==============================================================================
// Project category

namespace
{
	// Parse {projectRoot}/project_info.xml and return the Name attribute, or
	// fall back to the folder basename when the file is missing or malformed.
	static String readProjectDisplayName(const File& projectRoot)
	{
		auto xmlFile = projectRoot.getChildFile("project_info.xml");

		if (xmlFile.existsAsFile())
		{
			if (auto xml = XmlDocument::parse(xmlFile))
			{
				if (auto nameEl = xml->getChildByName("Name"))
				{
					auto n = nameEl->getStringAttribute("value");
					if (n.isNotEmpty())
						return n;
				}
			}
		}

		return projectRoot.getFileName();
	}

	static bool isValidProjectFolder(const File& f)
	{
		return f.isDirectory() && f.getChildFile("project_info.xml").existsAsFile();
	}
}

RestServer::Response RestHelpers::handleProjectList(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req)
{
	Array<File> folders;

	// Recent projects tracked by HISE
	for (const auto& s : ProjectHandler::getRecentWorkDirectories())
	{
		File f(s);
		if (isValidProjectFolder(f))
			folders.addIfNotAlreadyThere(f);
	}

	// Filesystem scan of the configured projects root
	if (auto gsm = dynamic_cast<GlobalSettingManager*>(mc))
	{
		auto rootSetting = gsm->getSettingsObject()
		                      .getSetting(HiseSettings::Compiler::DefaultProjectFolder);
		File projectsRoot(rootSetting.toString());

		if (projectsRoot.isDirectory())
		{
			Array<File> children;
			projectsRoot.findChildFiles(children, File::findDirectories, false);

			for (const auto& c : children)
			{
				if (isValidProjectFolder(c))
					folders.addIfNotAlreadyThere(c);
			}
		}
	}

	Array<var> projects;
	for (const auto& f : folders)
	{
		DynamicObject::Ptr entry = new DynamicObject();
		entry->setProperty(RestApiIds::name, readProjectDisplayName(f));
		entry->setProperty(RestApiIds::path, f.getFullPathName());
		projects.add(var(entry.get()));
	}

	auto activeRoot = mc->getSampleManager().getProjectHandler().getRootFolder();
	String active = activeRoot.isDirectory() ? readProjectDisplayName(activeRoot) : String();

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::projects, var(projects));
	result->setProperty(RestApiIds::active, active);
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectTree(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req)
{
	auto& ph = mc->getSampleManager().getProjectHandler();
	auto projectRoot = ph.getRootFolder();

	if (!projectRoot.isDirectory())
		return req->fail(500, "No active project");

	// Collect referenced file paths per category (absolute paths, set for O(1) lookup).
	std::set<String> refScripts, refSampleMaps, refImages, refDspNetworks, refUserPresets;

	{
		Processor::Iterator<JavascriptProcessor> iter(mc->getMainSynthChain());

		while (auto jp = iter.getNextProcessor())
		{
			for (int i = 0; i < jp->getNumWatchedFiles(); ++i)
				refScripts.insert(jp->getWatchedFile(i).getFullPathName());
		}
	}

	{
		Processor::Iterator<ModulatorSampler> iter(mc->getMainSynthChain());

		while (auto ms = iter.getNextProcessor())
		{
			if (auto smap = ms->getSampleMap())
			{
				auto f = smap->getReference().getFile();
				if (f.existsAsFile())
					refSampleMaps.insert(f.getFullPathName());
			}
		}
	}

	if (auto pool = mc->getCurrentImagePool())
	{
		for (const auto& ref : pool->getListOfAllReferences(false))
			refImages.insert(ref.getFile().getFullPathName());
	}

	{
		Processor::Iterator<Processor> iter(mc->getMainSynthChain());

		while (auto p = iter.getNextProcessor())
		{
			if (auto holder = dynamic_cast<scriptnode::DspNetwork::Holder*>(p))
			{
				if (auto net = holder->getActiveOrDebuggedNetwork())
				{
					auto networksFolder = BackendDllManager::getSubFolder(
						mc, BackendDllManager::FolderSubType::Networks);
					auto f = networksFolder.getChildFile(net->getId())
						.withFileExtension("xml");
					refDspNetworks.insert(f.getFullPathName());
				}
			}
		}
	}

	{
		auto currentPreset = mc->getUserPresetHandler().getCurrentlyLoadedFile();
		if (currentPreset.existsAsFile())
			refUserPresets.insert(currentPreset.getFullPathName());
	}

	enum class RefCategory { None, Scripts, SampleMaps, Images, DspNetworks, UserPresets };

	auto isReferenced = [&](const File& f, RefCategory cat) -> bool
	{
		auto p = f.getFullPathName();
		switch (cat)
		{
			case RefCategory::Scripts:     return refScripts.count(p) > 0;
			case RefCategory::SampleMaps:  return refSampleMaps.count(p) > 0;
			case RefCategory::Images:      return refImages.count(p) > 0;
			case RefCategory::DspNetworks: return refDspNetworks.count(p) > 0;
			case RefCategory::UserPresets: return refUserPresets.count(p) > 0;
			default:                       return false;
		}
	};

	// Per-category child filter. Runs before recursion so excluded folders are
	// never visited (cheaper than building nodes and discarding them).
	auto shouldInclude = [](const File& f, RefCategory cat) -> bool
	{
		// Any "Binaries" subfolder, at any depth, is build output noise.
		if (f.isDirectory() && f.getFileName() == "Binaries")
			return false;

		if (cat == RefCategory::Scripts)
		{
			if (f.isDirectory())
				return f.getFileName() != "ScriptProcessors";

			auto ext = f.getFileExtension().toLowerCase();
			return ext == ".js" || ext == ".glsl" || ext == ".css";
		}

		return true;
	};

	std::function<var(const File&, RefCategory)> buildNode =
		[&](const File& f, RefCategory cat) -> var
	{
		DynamicObject::Ptr node = new DynamicObject();
		node->setProperty(RestApiIds::name, f.getFileName());

		if (f.isDirectory())
		{
			node->setProperty(RestApiIds::type, String("folder"));

			Array<File> kids;
			f.findChildFiles(kids, File::findFilesAndDirectories, false);

			// Folders first, then files; alphabetical within each group.
			std::sort(kids.begin(), kids.end(), [](const File& a, const File& b)
			{
				if (a.isDirectory() != b.isDirectory())
					return a.isDirectory();
				return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
			});

			Array<var> children;
			for (const auto& c : kids)
			{
				if (shouldInclude(c, cat))
					children.add(buildNode(c, cat));
			}

			node->setProperty(RestApiIds::children, children);
		}
		else
		{
			node->setProperty(RestApiIds::type, String("file"));
			node->setProperty(RestApiIds::referenced, isReferenced(f, cat));
		}

		return var(node.get());
	};

	struct FolderSpec { const char* name; RefCategory cat; };

	static const FolderSpec folders[] = {
		{ "Scripts",     RefCategory::Scripts     },
		{ "SampleMaps",  RefCategory::SampleMaps  },
		{ "Images",      RefCategory::Images      },
		{ "DspNetworks", RefCategory::DspNetworks },
		{ "UserPresets", RefCategory::UserPresets },
	};

	DynamicObject::Ptr rootNode = new DynamicObject();
	rootNode->setProperty(RestApiIds::name, projectRoot.getFileName());
	rootNode->setProperty(RestApiIds::type, String("folder"));

	Array<var> rootChildren;
	for (const auto& fs : folders)
	{
		auto folder = projectRoot.getChildFile(fs.name);
		if (folder.isDirectory())
			rootChildren.add(buildNode(folder, fs.cat));
	}
	rootNode->setProperty(RestApiIds::children, rootChildren);

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::projectName, readProjectDisplayName(projectRoot));
	result->setProperty(RestApiIds::root, var(rootNode.get()));
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectFiles(MainController* mc,
                                                     RestServer::AsyncRequest::Ptr req)
{
	auto& ph = mc->getSampleManager().getProjectHandler();
	auto projectRoot = ph.getRootFolder();

	if (!projectRoot.isDirectory())
		return req->fail(500, "No active project");

	Array<var> files;

	auto addEntry = [&](const File& f, const String& typeStr)
	{
		DynamicObject::Ptr e = new DynamicObject();
		e->setProperty(RestApiIds::name, f.getFileName());
		e->setProperty(RestApiIds::type, typeStr);
		e->setProperty(RestApiIds::path,
			f.getRelativePathFrom(projectRoot).replaceCharacter('\\', '/'));
		e->setProperty(RestApiIds::modified, f.getLastModificationTime().toISO8601(true));
		files.add(var(e.get()));
	};

	for (const auto& f : ph.getFileList(FileHandlerBase::XMLPresetBackups, true, false))
	{
		if (f.hasFileExtension("xml"))
			addEntry(f, "xml");
	}

	for (const auto& f : ph.getFileList(FileHandlerBase::Presets, true, false))
	{
		if (f.hasFileExtension("hip"))
			addEntry(f, "hip");
	}

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::files, var(files));
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectSettingsList(MainController* mc,
                                                            RestServer::AsyncRequest::Ptr req)
{
	auto& obj = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject();

	DynamicObject::Ptr settings = new DynamicObject();

	auto addSetting = [&](const Identifier& id)
	{
		DynamicObject::Ptr v = new DynamicObject();
		v->setProperty(RestApiIds::value, obj.getSetting(id));
		auto sa = obj.getOptionsFor(id);

		auto desc = StringArray::fromLines(HiseSettings::SettingDescription::getDescription(id));
		desc.remove(0);
		v->setProperty(RestApiIds::description, desc.joinIntoString("\n"));

		if (!sa.isEmpty())
		{
			Array<var> o;

			if (sa.contains("Yes"))
			{
				o.add(true);
				o.add(false);
			}
			else
			{
				for (auto& s : sa)
					o.add(s);
			}

			v->setProperty(RestApiIds::options, var(o));
		}

		settings->setProperty(id, var(v.get()));
	};

	for (auto id : HiseSettings::Project::getAllIds())
	{
		if (id.toString().startsWith("ExtraDefinitions"))
			continue;

		addSetting(id);
	}
	
	for (auto id : HiseSettings::User::getAllIds())
		addSetting(id);

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::settings, var(settings.get()));
	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectSettingsSet(MainController* mc,
                                                           RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto key = obj[RestApiIds::key].toString();
	auto value = obj[RestApiIds::value];

	if (key.isEmpty())
		return req->fail(400, "key is required");

	if (!obj.hasProperty(RestApiIds::value))
		return req->fail(400, "value is required");

	if (key.startsWith("ExtraDef"))
		return req->fail(400, "use /api/project/preprocessor/set for preprocessor modification");

	auto& settings = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject();
	auto id = Identifier(key);

	const bool isProjectId = HiseSettings::Project::getAllIds().contains(id);
	const bool isUserId    = HiseSettings::User::getAllIds().contains(id);

	if (!isProjectId && !isUserId)
	{
		StringArray keys;
		for (auto& pid : HiseSettings::Project::getAllIds())
			keys.add(pid.toString());

		for (auto& uid : HiseSettings::User::getAllIds())
			keys.add(uid.toString());

		String errorMessage;
		errorMessage << "invalid key " << key;

		auto correct = FuzzySearcher::suggestCorrection(key, keys);
		if (correct.isNotEmpty())
			errorMessage << ". Did you mean: " << correct;

		return req->fail(400, errorMessage);
	}

	auto ok = settings.checkInput(id, value);

	if (!ok.wasOk())
		return req->fail(400, ok.getErrorMessage());

	settings.writeSetting(isProjectId ? HiseSettings::SettingFiles::ProjectSettings
	                                  : HiseSettings::SettingFiles::UserSettings,
	                      id, value);

	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, true);

	Array<var> logs;
	logs.add("Updated " + key + " to " + value.toString());
	result->setProperty(RestApiIds::logs, logs);
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectSave(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto format = obj[RestApiIds::format].toString();
	auto filename = obj[RestApiIds::filename].toString();

	if (format != "xml" && format != "hip")
		return req->fail(400, "format must be 'xml' or 'hip'");

	DynamicObject::Ptr result = new DynamicObject();

	Array<var> logs;

	result->setProperty(RestApiIds::masterChainRenamed, false);

	if (filename.isEmpty())
		filename = mc->getMainSynthChain()->getId();
	else
	{
		if (filename != mc->getMainSynthChain()->getId())
		{
			mc->getMainSynthChain()->setId(filename, sendNotificationAsync);
			logs.add("Renamed master chain to " + filename);
			result->setProperty(RestApiIds::masterChainRenamed, true);
			result->setProperty(RestApiIds::newName, filename);
		}	
	}
	
	

	auto bpe = dynamic_cast<BackendProcessor*>(mc)->currentRootWindow;

	if (format == "xml")
	{
		auto d = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::XMLPresetBackups);
		auto f = d.getChildFile(filename).withFileExtension(format);

		if(!f.isAChildOf(d))
			return req->fail(400, f.getFullPathName() + " not in project folder");

		BackendCommandTarget::Actions::saveFileAsXml(bpe, f);
		result->setProperty(RestApiIds::path, f.getFullPathName());
	}
	if (format == "hip")
	{
		auto d = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::Presets);
		auto f = d.getChildFile(filename).withFileExtension(format);
		
		if (!f.isAChildOf(d))
			return req->fail(400, f.getFullPathName() + " not in project folder");

		PresetHandler::saveProcessorAsPreset(mc->getMainSynthChain());
		result->setProperty(RestApiIds::path, f.getFullPathName());
	}
	
	result->setProperty(RestApiIds::success, true);
	result->setProperty(RestApiIds::logs, logs);
	result->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(result.get())));
	return req->waitForResponse();

}

RestServer::Response RestHelpers::handleProjectLoad(MainController* mc,
                                                    RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto file = obj[RestApiIds::file].toString();

	if (file.isEmpty())
		return req->fail(400, "file is required");

	auto projectRoot = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getRootFolder();

	auto bpe = dynamic_cast<BackendProcessor*>(mc)->currentRootWindow;

	auto f = projectRoot.getChildFile(file);

	if (!f.existsAsFile())
		return req->fail(404, f.getFullPathName() + " is not a file");

	if (!f.isAChildOf(projectRoot))
		return req->fail(400, f.getFullPathName() + " not in project folder");



	if (f.getFileExtension() == ".xml")
	{
		auto xml = XmlDocument::parse(f);

		if (xml != nullptr)
		{
			XmlBackupFunctions::addContentFromSubdirectory(*xml, f);
			String newId = xml->getStringAttribute("ID");

			auto v = ValueTree::fromXml(*xml);

			if (!(v.getType() == Identifier("Processor")
				&& v.getProperty("Type", var::undefined()).toString() == "SynthChain"))
				return req->fail(400, "XML is not a valid HISE preset (SynthChain)");

			XmlBackupFunctions::restoreAllScripts(v, bpe->getMainSynthChain(), newId);

			bpe->setOnetimeCallbackAfterPresetLoad([req, f]()
			{
				DynamicObject::Ptr r = new DynamicObject();
				r->setProperty(RestApiIds::success, true);

				Array<var> logs;

				logs.add("Loaded " + f.getFullPathName());

				r->setProperty(RestApiIds::logs, logs);
				r->setProperty(RestApiIds::errors, Array<var>());
				req->complete(RestServer::Response::ok(var(r.get())));
			});

			MessageManager::callAsync([bpe, v]()
			{
				bpe->loadNewContainer(v);
			});
		}
		else
		{
			return req->fail(500, "The XML file is not valid. Loading aborted");
		}
	}
	else if (f.getFileExtension() == ".hip")
	{
		bpe->setOnetimeCallbackAfterPresetLoad([req, f]()
		{
			DynamicObject::Ptr r = new DynamicObject();
			r->setProperty(RestApiIds::success, true);

			Array<var> logs;

			logs.add("Loaded " + f.getFullPathName());

			r->setProperty(RestApiIds::logs, logs);
			r->setProperty(RestApiIds::errors, Array<var>());
			req->complete(RestServer::Response::ok(var(r.get())));
		});

		MessageManager::callAsync([bpe, f]()
		{
			bpe->loadNewContainer(f);
		});
	}
	else
	{
		return req->fail(400, "file extension must be .xml or .hip");
	}
		
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectSwitch(MainController* mc,
                                                      RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto project = obj[RestApiIds::project].toString();

	if (project.isEmpty())
		return req->fail(400, "project is required");

	auto valid = isValidProjectFolder(File(project));

	if (!valid)
		return req->fail(400, project + " is not a valid project folder");

	
	

	auto oldProject = GET_HISE_SETTING(mc->getMainSynthChain(), HiseSettings::Project::Name).toString();

	bool done = false;

	auto r = Result::fail("timeout at project switch");

	MessageManager::callAsync([mc, project, &done, &r]()
	{
		auto bpe = dynamic_cast<BackendProcessor*>(mc)->currentRootWindow;
		auto& handler = GET_PROJECT_HANDLER(bpe->getMainSynthChain());
		r = handler.setWorkingProject(File(project));
		bpe->getBackendProcessor()->getSettingsObject().refreshProjectData();
		
		mc->clearExtraDefinitionCache();

		done = true;
	});

	int safeCounter = 0;

	while (!done && ++safeCounter < 3000)
		Thread::sleep(10);
	
	if (r.failed())
	{
		return req->fail(400, r.getErrorMessage());
	}
	else
	{
		
		auto um = RestServerUndoManager::Instance::getOrCreate(mc, RestHelpers::ApiRoute::ProjectSwitch);

		mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), [um, oldProject, req](Processor* p)
		{
			p->getMainController()->clearPreset(sendNotificationAsync);
			dynamic_cast<BackendProcessor*>(p->getMainController())->createInterface(600, 500);

			DynamicObject::Ptr result = new DynamicObject();
			result->setProperty(RestApiIds::success, true);

			auto newProject = GET_HISE_SETTING(p, HiseSettings::Project::Name).toString();

			Array<var> logs;
			logs.add("Switched project from " + oldProject + " to " + newProject);

			result->setProperty(RestApiIds::logs, logs);
			result->setProperty(RestApiIds::errors, Array<var>());

			um->clearUndoHistory();
			um->flushUI(p);

			req->complete(RestServer::Response::ok(var(result.get())));

			return SafeFunctionCall::OK;
		}, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
	}

	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectExportSnippet(MainController* mc,
                                                             RestServer::AsyncRequest::Ptr req)
{
	auto brw = dynamic_cast<BackendProcessor*>(mc)->currentRootWindow;

	auto snippet = BackendCommandTarget::Actions::exportFileAsSnippet(brw, false);

	DynamicObject::Ptr r = new DynamicObject();
	r->setProperty(RestApiIds::success, true);
	r->setProperty(RestApiIds::snippet, snippet);
	r->setProperty(RestApiIds::logs, Array<var>());
	r->setProperty(RestApiIds::errors, Array<var>());

	req->complete(RestServer::Response::ok(var(r.get())));

	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectImportSnippet(MainController* mc,
                                                             RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();
	auto snippet = obj[RestApiIds::snippet].toString();

	if (snippet.isEmpty())
		return req->fail(400, "snippet is required");

	if (!snippet.startsWith("HiseSnippet "))
		return req->fail(400, "snippet must start with 'HiseSnippet '");

	auto data = snippet.fromFirstOccurrenceOf("HiseSnippet ", false, false);

	MemoryBlock mb;
	if (!mb.fromBase64Encoding(data))
		return req->fail(400, "Failed to base64-decode snippet data");

	auto vt = ValueTree::readFromGZIPData(mb.getData(), mb.getSize());

	if (!vt.isValid())
		return req->fail(400, "Failed to decompress snippet ValueTree");

	const bool isExtended = vt.getType() == Identifier("extended_snippet");
	const bool isPreset = (vt.getType() == Identifier("Processor")
	                     && vt.getProperty("Type", var::undefined()).toString() == "SynthChain");

	if (!isExtended && !isPreset)
		return req->fail(400, "Snippet does not contain a valid SynthChain or extended_snippet");

	auto brw = dynamic_cast<BackendProcessor*>(mc)->currentRootWindow;

	brw->setOnetimeCallbackAfterPresetLoad([req]()
	{
		DynamicObject::Ptr r = new DynamicObject();
		r->setProperty(RestApiIds::success, true);
		r->setProperty(RestApiIds::logs, Array<var>());
		r->setProperty(RestApiIds::errors, Array<var>());
		req->complete(RestServer::Response::ok(var(r.get())));
	});

	MessageManager::callAsync([brw, vt]()
	{
		brw->loadNewContainer(vt);
	});

	return req->waitForResponse();
}

//==============================================================================
// Preprocessor endpoints (Phase 1 stubs)

struct PreprocessorHelpers
{
	static const StringArray& getPlatforms()
	{
		static const StringArray p = { "Windows", "macOS", "Linux" };
		return p;
	}

	static const StringArray& getTargets()
	{
		static const StringArray t = { "Project", "Dll" };
		return t;
	}

	static bool isValidPreprocessorOS(const String& s, bool acceptAll)
	{
		return getPlatforms().contains(s) || (acceptAll && s == "all");
	}

	static bool isValidPreprocessorTarget(const String& s, bool acceptAll)
	{
		return getTargets().contains(s) || (acceptAll && s == "all");
	}
};

RestServer::Response RestHelpers::handleProjectPreprocessorList(MainController* mc,
                                                                RestServer::AsyncRequest::Ptr req)
{
	auto os = req->getRequest()[RestApiIds::OS];
	if (os.isEmpty())
		os = "all";

	auto target = req->getRequest()[RestApiIds::target];
	if (target.isEmpty())
		target = "all";

	if (!PreprocessorHelpers::isValidPreprocessorOS(os, true))
		return req->fail(400, "OS must be one of: Windows, macOS, Linux, all");

	if (!PreprocessorHelpers::isValidPreprocessorTarget(target, true))
		return req->fail(400, "target must be one of: Project, Dll, all");

	auto& settings = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject();

	const auto& platforms = PreprocessorHelpers::getPlatforms();
	const auto& targets = PreprocessorHelpers::getTargets();

	// Collect every (target, OS) slot with its full macro map so we can
	// cross-reference below. Run over the full matrix (filters are applied
	// later, at emit time) because sharing detection requires the full data.
	struct Slot { String t; String pl; var obj; };
	Array<Slot> slots;

	for (const auto& t : targets)
		for (const auto& pl : platforms)
			slots.add({ t, pl, settings.getExtraDefinitionsAsObject(pl, t) });

	StringArray macros;
	for (const auto& s : slots)
		if (auto* d = s.obj.getDynamicObject())
			for (const auto& nv : d->getProperties())
				macros.addIfNotAlreadyThere(nv.name.toString());

	auto getValue = [&](const String& t, const String& pl, const String& macro) -> var
	{
		for (const auto& s : slots)
		{
			if (s.t == t && s.pl == pl)
			{
				if (auto* d = s.obj.getDynamicObject())
				{
					Identifier id(macro);
					if (d->hasProperty(id))
						return d->getProperty(id);
				}
				return var::undefined();
			}
		}
		return var::undefined();
	};

	// Returns {true, commonValue} when the macro has the same value in every
	// (t in tsubset, pl in pls) pair and is present in all of them.
	auto sharedAcross = [&](const String& macro,
	                        const StringArray& tsubset,
	                        const StringArray& pls)
	{
		var ref;
		bool seen = false;

		for (const auto& t : tsubset)
		{
			for (const auto& pl : pls)
			{
				auto v = getValue(t, pl, macro);

				if (v.isUndefined())
					return std::make_pair(false, var());

				if (!seen) { ref = v; seen = true; }
				else if (v != ref) return std::make_pair(false, var());
			}
		}

		return std::make_pair(seen, ref);
	};

	// juce::HashMap is non-copyable so nested containers don't work; flatten
	// the (target, OS) leaf map to a single HashMap keyed by "target.OS".
	DynamicObject::Ptr starStar = new DynamicObject();
	HashMap<String, DynamicObject::Ptr> targetStar;
	HashMap<String, DynamicObject::Ptr> leaf;

	for (const auto& t : targets)
	{
		targetStar.set(t, new DynamicObject());

		for (const auto& pl : platforms)
			leaf.set(t + "." + pl, new DynamicObject());
	}

	for (const auto& m : macros)
	{
		auto global = sharedAcross(m, targets, platforms);

		if (global.first)
		{
			starStar->setProperty(Identifier(m), global.second);
			continue;
		}

		for (const auto& t : targets)
		{
			auto shared = sharedAcross(m, { t }, platforms);

			if (shared.first)
			{
				targetStar[t]->setProperty(Identifier(m), shared.second);
			}
			else
			{
				for (const auto& pl : platforms)
				{
					auto v = getValue(t, pl, m);

					if (!v.isUndefined())
						leaf[t + "." + pl]->setProperty(Identifier(m), v);
				}
			}
		}
	}

	// Emit sections filtered by the query parameters. A cross-reference section
	// is emitted when its scope intersects the filter: "*.*" is always
	// relevant; "T.*" is relevant when filter.target includes T; "T.OS" when
	// both filter.target includes T and filter.os includes OS.
	DynamicObject::Ptr fullObj = new DynamicObject();

	auto emit = [&](const String& key, DynamicObject::Ptr obj)
	{
		if (obj != nullptr && obj->getProperties().size() > 0)
			fullObj->setProperty(Identifier(key), var(obj.get()));
	};

	emit("*.*", starStar);

	for (const auto& t : targets)
	{
		if (target != "all" && t != target)
			continue;

		emit(t + ".*", targetStar[t]);

		for (const auto& pl : platforms)
		{
			if (os != "all" && pl != os)
				continue;

			emit(t + "." + pl, leaf[t + "." + pl]);
		}
	}

	DynamicObject::Ptr r = new DynamicObject();
	r->setProperty(RestApiIds::success, true);
	r->setProperty(RestApiIds::logs, Array<var>());
	r->setProperty(RestApiIds::preprocessors, var(fullObj.get()));
	r->setProperty(RestApiIds::errors, Array<var>());
	req->complete(RestServer::Response::ok(var(r.get())));
	return req->waitForResponse();
}

RestServer::Response RestHelpers::handleProjectPreprocessorSet(MainController* mc,
                                                               RestServer::AsyncRequest::Ptr req)
{
	auto obj = req->getRequest().getJsonBody();

	auto os = obj[RestApiIds::OS].toString();
	auto target = obj[RestApiIds::target].toString();
	auto preprocessor = obj[RestApiIds::preprocessor].toString();
	auto value = obj[RestApiIds::value].toString();

	if (!PreprocessorHelpers::isValidPreprocessorOS(os, true))
		return req->fail(400, "OS must be one of: Windows, macOS, Linux, all");

	if (!PreprocessorHelpers::isValidPreprocessorTarget(target, true))
		return req->fail(400, "target must be one of: Project, Dll, all");

	if (preprocessor.isEmpty())
		return req->fail(400, "preprocessor is required");

	if (value.isEmpty())
		return req->fail(400, "value is required");

	if (value != "default" && !value.containsOnly("-0123456789"))
		return req->fail(400, "value must be an integer or the literal string 'default'");

	auto pf = [os, target, preprocessor, value, req](Processor* p)
	{
		const auto& platforms = PreprocessorHelpers::getPlatforms();
		const auto& targets = PreprocessorHelpers::getTargets();

		auto mc = p->getMainController();
		auto& settings = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject();

		Array<var> logs;

		for (auto t : targets)
		{
			if (target != "all" && t != target)
				continue;

			for (auto pl : platforms)
			{
				if (os != "all" && pl != os)
					continue;

				String log;
				log << t << "." << pl << ":";

				auto s = settings.getExtraDefinitionsAsObject(pl, t, false);

				bool didSomething = false;

				if (auto obj = s.getDynamicObject())
				{
					if (value == "default")
					{
						if (obj->hasProperty(preprocessor))
						{
							logs.add(log + " removed " + preprocessor);
							didSomething = true;
							obj->removeProperty(preprocessor);
						}
						else
							logs.add(log + " " + preprocessor + " not set. skip.");
					}
					else
					{
						if (obj->hasProperty(preprocessor))
						{
							auto prevValue = obj->getProperty(preprocessor).toString();

							if (prevValue == value)
							{
								logs.add(log + " " + preprocessor + " already set. skip");
							}
							else
							{
								logs.add(log + " changed " + preprocessor + " from " + prevValue + " to " + value);
								didSomething = true;
								obj->setProperty(preprocessor, value);
							}
						}
						else
						{
							logs.add(log + " set " + preprocessor + " to " + value);
							didSomething = true;
							obj->setProperty(preprocessor, value);
						}
					}
				}

				if(didSomething)
					settings.setExtraDefinitionsFromObject(pl, t, s);
			}
		}

		DynamicObject::Ptr r = new DynamicObject();
		r->setProperty(RestApiIds::success, true);
		r->setProperty(RestApiIds::logs, logs);
		r->setProperty(RestApiIds::errors, Array<var>());
		req->complete(RestServer::Response::ok(var(r.get())));

		return SafeFunctionCall::OK;
	};

	mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), pf, MainController::KillStateHandler::TargetThread::SampleLoadingThread);

	return req->waitForResponse();
}

} // namespace hise
