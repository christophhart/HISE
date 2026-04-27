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

// Prevent Windows.h min/max macros from interfering
#define NOMINMAX

#include "httplib.h"
#include <regex>

// Remove Windows GDI Rectangle to avoid conflict with juce::Rectangle
#ifdef _WIN32
#undef Rectangle
#endif

namespace hise { using namespace juce;

//==============================================================================
/** Serialize a var to JSON, normalizing API envelopes on the way out.

    A var is treated as an envelope iff its root is a DynamicObject carrying
    the `success` marker. For envelopes this:

      - stamps `apiVersion` with the compile-time HISE_REST_API_VERSION,
      - guarantees `logs` and `errors` arrays exist so consumers can iterate
        them unconditionally without hasProperty() guards.

    Non-envelope values (OpenAPI doc, raw arrays, primitives) pass through
    untouched. Mutating the underlying DynamicObject is safe because every
    envelope is built fresh by the handler/factory immediately before being
    serialized; no caller observes the var afterwards.
*/
static String varToJsonString(const var& v)
{
    if (auto* obj = v.getDynamicObject())
    {
        if (obj->hasProperty(RestApiIds::success))
        {
            if (!obj->hasProperty(RestApiIds::apiVersion))
                obj->setProperty(RestApiIds::apiVersion, HISE_REST_API_VERSION);

            if (!obj->hasProperty(RestApiIds::logs))
                obj->setProperty(RestApiIds::logs, var(Array<var>{}));

            if (!obj->hasProperty(RestApiIds::errors))
                obj->setProperty(RestApiIds::errors, var(Array<var>{}));
        }
    }
    return JSON::toString(v, false);
}

// Helper to create JSON error response body
static String createErrorJson(const String& message)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty(RestApiIds::success, false);
    obj->setProperty(RestApiIds::result, var());

    Array<var> errors;
    Array<var> logs;

    DynamicObject::Ptr em = new DynamicObject();
    em->setProperty(RestApiIds::errorMessage, message);
    Array<var> callstack;
    em->setProperty(RestApiIds::callstack, var(callstack));
    errors.add(var(em.get()));

    obj->setProperty(RestApiIds::logs, var(logs));
    obj->setProperty(RestApiIds::errors, var(errors));
    return varToJsonString(obj.get());
}

//==============================================================================
// Floating point normalization

String RestServer::formatFloat(double value, int maxDecimalPlaces)
{
    double multiplier = std::pow(10.0, maxDecimalPlaces);
    double rounded = std::round(value * multiplier) / multiplier;
    
    if (std::fmod(rounded, 1.0) == 0.0)
        return String((int64)rounded) + ".0";
    
    String s = String(rounded, maxDecimalPlaces);
    s = s.trimCharactersAtEnd("0");
    
    if (s.endsWithChar('.'))
        s << "0";
    
    return s;
}

void RestServer::normalizeFloatsInVar(var& v, int maxDecimalPlaces)
{
    if (v.isDouble())
    {
        double multiplier = std::pow(10.0, maxDecimalPlaces);
        v = std::round((double)v * multiplier) / multiplier;
    }
    else if (v.isString())
    {
        // Clean up floating point numbers embedded in strings (e.g., log messages)
        // Matches numbers with 5+ decimal places (likely FP noise)
        static const std::regex fpRegex(R"((\d+\.\d{5,}))");
        
        std::string str = v.toString().toStdString();
        std::string result;
        std::sregex_iterator it(str.begin(), str.end(), fpRegex);
        std::sregex_iterator end;
        
        size_t lastPos = 0;
        while (it != end)
        {
            result += str.substr(lastPos, it->position() - lastPos);
            double val = std::stod(it->str());
            result += formatFloat(val, maxDecimalPlaces).toStdString();
            lastPos = it->position() + it->length();
            ++it;
        }
        result += str.substr(lastPos);
        
        v = String(result);
    }
    else if (v.isArray())
    {
        for (int i = 0; i < v.size(); i++)
        {
            var item = v[i];
            normalizeFloatsInVar(item, maxDecimalPlaces);
            v.getArray()->set(i, item);
        }
    }
    else if (auto* obj = v.getDynamicObject())
    {
        for (const auto& prop : obj->getProperties())
        {
            var value = prop.value;
            normalizeFloatsInVar(value, maxDecimalPlaces);
            obj->setProperty(prop.name, value);
        }
    }
}

//==============================================================================
// Platform-specific window repaint

void RestServer::forceRepaintWindow(void* nativeHandle)
{
#ifdef _WIN32
    if (nativeHandle != nullptr)
        RedrawWindow((HWND)nativeHandle, nullptr, nullptr, 
                     RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
#else
    ignoreUnused(nativeHandle);
#endif
}

//==============================================================================
// Request implementation
String RestServer::Request::operator[](const Identifier& name) const
{
    auto nameStr = name.toString();
    auto& names = url.getParameterNames();
    auto& values = url.getParameterValues();
    
    // Search from end to get last occurrence (request params override defaults)
    for (int i = names.size() - 1; i >= 0; --i)
    {
        if (names[i] == nameStr)
            return values[i];
    }
    
    return {};
}

bool RestServer::Request::getTrueValue(const Identifier& name) const
{
    auto v = (*this)[name];
    
    if(v.isEmpty())
        return false;
    
    if(v == "true")
        return true;
    
    if(v == "false")
        return false;
    
    return v.getIntValue();
}

var RestServer::Request::getJsonBody() const
{
    var result;
    auto postData = url.getPostData();
    
    if (postData.isEmpty())
        return var::undefined();
    
    auto parseResult = JSON::parse(postData, result);
    
    if (parseResult.failed())
        return var::undefined();
    
    return result;
}

//==============================================================================
// Response factory methods
RestServer::Response RestServer::Response::ok(const var& json)
{
    return { 200, "application/json", varToJsonString(json) };
}

RestServer::Response RestServer::Response::ok(const String& body, const String& contentType)
{
    return { 200, contentType, body };
}

RestServer::Response RestServer::Response::badRequest(const String& message)
{
    return { 400, "application/json", createErrorJson(message) };
}

RestServer::Response RestServer::Response::notFound(const String& message)
{
    return { 404, "application/json", createErrorJson(message) };
}

RestServer::Response RestServer::Response::internalError(const String& message)
{
    return { 500, "application/json", createErrorJson(message) };
}

RestServer::Response RestServer::Response::error(int statusCode, const String& message)
{
    return { statusCode, "application/json", createErrorJson(message) };
}

//==============================================================================
// AsyncRequest implementation

void RestServer::AsyncRequest::appendLog(const String& message)
{
    ScopedLock sl(logLock);
    logs.add(message);
}

void RestServer::AsyncRequest::appendError(const String& errorMessage, const StringArray& callstack)
{
    ScopedLock sl(logLock);
    
    int64 hash = errorMessage.hashCode64();
    
    // Check for duplicate
    for (const auto& existing : errors)
    {
        if (existing.hash == hash)
            return;
    }
    
    errors.add({ errorMessage, callstack, hash });
}

void RestServer::AsyncRequest::mergeLogsIntoResponse()
{
    ScopedLock sl(logLock);
    
    // Parse existing response body as JSON
    var existingJson;
    auto parseResult = JSON::parse(response.body, existingJson);
    
    DynamicObject::Ptr rootObj;
    
    if (parseResult.wasOk() && existingJson.getDynamicObject() != nullptr)
    {
        // Clone the existing object so we can modify it
        rootObj = existingJson.getDynamicObject()->clone();
    }
    else
    {
        // Wrap non-JSON or non-object response
        rootObj = new DynamicObject();
        
        if (response.body.isNotEmpty())
            rootObj->setProperty(RestApiIds::result, response.body);
    }
    
    Array<var> logsArray;

    if (rootObj->hasProperty(RestApiIds::logs))
    {
        if (auto ar = rootObj->getProperty(RestApiIds::logs).getArray())
            logsArray.addArray(*ar);
    }

    for (const auto& log : logs)
        logsArray.add(log);
    
    rootObj->setProperty(RestApiIds::logs, logsArray);
    
    Array<var> mergedErrors;

    if (rootObj->hasProperty(RestApiIds::errors))
    {
        auto existingErrors = rootObj->getProperty(RestApiIds::errors);

        if (existingErrors.isArray())
        {
            for (const auto& e : *existingErrors.getArray())
                mergedErrors.add(e);
        }
    }

    if (!usesCustomErrors)
    {
		for (const auto& err : errors)
		{
			DynamicObject::Ptr errObj = new DynamicObject();
			errObj->setProperty(RestApiIds::errorMessage, err.errorMessage);

			Array<var> callstackArray;
			for (const auto& frame : err.callstack)
				callstackArray.add(frame);

			errObj->setProperty(RestApiIds::callstack, callstackArray);
			mergedErrors.add(var(errObj.get()));
		}
    }

    rootObj->setProperty(RestApiIds::errors, mergedErrors);

    // Normalize floating point values for clean JSON output (4 decimal places)
    var rootVar(rootObj.get());
    normalizeFloatsInVar(rootVar);

    // Route through varToJsonString so the envelope normalization (apiVersion
    // stamp, default logs/errors arrays) is applied here too.
    response.body = varToJsonString(rootVar);
    response.contentType = "application/json";
}

//==============================================================================
// Internal implementation class
class RestServer::Impl : public Thread
{
public:
    Impl() : Thread("REST API Server") {}

    ~Impl()
    {
        stopServer();
    }

    //==============================================================================
    struct RouteInfo
    {
        Method method = GET;            // HTTP method
        String path;                    // Just the path (e.g., "/api/recompile")
        StringPairArray defaultParams;  // Default parameter values from route URL
        RouteHandler handler;
    };

    //==============================================================================
    void addRoute(Method method, const URL& routeUrl, RouteHandler handler)
    {
        RouteInfo info;
        info.method = method;
        
        // Extract path from URL, prepending "/" since getSubPath() doesn't include it
        String subPath = routeUrl.getSubPath(false);
        info.path = subPath.isEmpty() ? "/" : ("/" + subPath);
        
        info.handler = std::move(handler);
        
        // Extract default parameters from route URL
        auto& names = routeUrl.getParameterNames();
        auto& values = routeUrl.getParameterValues();
        for (int i = 0; i < names.size(); ++i)
            info.defaultParams.set(names[i], values[i]);
        
        routes[{method, info.path}] = std::move(info);
    }

    //==============================================================================
    bool startServer(int port, const String& bindAddress, const String& corsOrigins)
    {
        if (isThreadRunning())
            return false;

        currentPort = port;
        currentBindAddress = bindAddress;
        currentCorsOrigins = corsOrigins.trim();

        server = std::make_unique<httplib::Server>();

        // Register all routes with httplib
        for (auto& pair : routes)
        {
            auto method = pair.first.first;
            auto& routePath = pair.first.second;
            auto& routeInfo = pair.second;

            auto wrappedHandler = [this, routeInfo](const httplib::Request& req, httplib::Response& res)
            {
                handleRequest(routeInfo, req, res);
            };

            std::string pathStr = routePath.toStdString();

            switch (method)
            {
                case GET:    server->Get(pathStr, wrappedHandler); break;
                case POST:   server->Post(pathStr, wrappedHandler); break;
                case PUT:    server->Put(pathStr, wrappedHandler); break;
                case DELETE: server->Delete(pathStr, wrappedHandler); break;
            }
        }

        // Wildcard CORS preflight handler. httplib dispatches OPTIONS through its own
        // handler list, so the actual route handlers above never run for preflight.
        server->Options(R"(.*)", [this](const httplib::Request& req, httplib::Response& res)
        {
            applyCorsHeaders(res, req);
            res.status = 204;
        });

        // Start the server thread
        startThread();

        // Wait briefly to check if server started successfully
        // The thread will set running to true once listen() is active
        for (int i = 0; i < 50; ++i)  // Wait up to 500ms
        {
            if (running.load())
            {
                listeners.call(&RestServer::Listener::serverStarted, currentPort);
                return true;
            }
            Thread::sleep(10);
        }

        // Server failed to start
        stopServer();
        return false;
    }

    //==============================================================================
    void stopServer()
    {
        if (server)
            server->stop();

        if (isThreadRunning())
            stopThread(5000);

        server.reset();
        running.store(false);
    }

    //==============================================================================
    void run() override
    {
        running.store(true);
        
        // This blocks until server->stop() is called
        server->listen(currentBindAddress.toStdString(), currentPort);
        
        running.store(false);
        listeners.call(&RestServer::Listener::serverStopped);
    }

    //==============================================================================
    bool isServerRunning() const
    {
        return running.load();
    }

    int getServerPort() const
    {
        return running.load() ? currentPort : 0;
    }

    //==============================================================================
    ListenerList<RestServer::Listener> listeners;

private:
    //==============================================================================
    void handleRequest(const RouteInfo& routeInfo, const httplib::Request& req, httplib::Response& res)
    {
        // Serialize all request handling — prevents ScopedConsoleHandler contention
        // and ensures consistent engine state across concurrent httplib worker threads.
        const ScopedLock sl(requestSerializationLock);

        // Notify listeners
        String methodStr;
        switch (routeInfo.method)
        {
            case GET:    methodStr = "GET"; break;
            case POST:   methodStr = "POST"; break;
            case PUT:    methodStr = "PUT"; break;
            case DELETE: methodStr = "DELETE"; break;
        }
        
        listeners.call(&RestServer::Listener::requestReceived, methodStr, String(req.path));

        // Build URL with path
        URL url("http://localhost:" + String(currentPort) + String(req.path.c_str()));
        
        // First add default parameters from route definition
        for (int i = 0; i < routeInfo.defaultParams.size(); ++i)
        {
            auto key = routeInfo.defaultParams.getAllKeys()[i];
            auto value = routeInfo.defaultParams.getAllValues()[i];
            url = url.withParameter(key, value);
        }
        
        // Then override with incoming request parameters
        for (auto& param : req.params)
            url = url.withParameter(String(param.first), String(param.second));
        
        // Add POST data if present
        if (!req.body.empty())
            url = url.withPOSTData(String(req.body));
        
        // Build Request object
        Request request;
        request.method = routeInfo.method;
        request.url = url;
        
        // Copy headers
        for (auto& header : req.headers)
            request.headers.set(String(header.first), String(header.second));

        // Call handler with exception safety
        Response response;
        
        try
        {
            response = routeInfo.handler(request);
        }
        catch (const std::exception& e)
        {
            String errorMsg = String("Exception in handler: ") + e.what();
            listeners.call(&RestServer::Listener::serverError, errorMsg);
            response = Response::internalError(errorMsg);
        }
        catch (...)
        {
            String errorMsg = "Unknown exception in handler";
            listeners.call(&RestServer::Listener::serverError, errorMsg);
            response = Response::internalError(errorMsg);
        }

        // Note: envelope normalization (apiVersion stamp, logs/errors arrays)
        // happens inside varToJsonString at construction time, not here.

        // Send response
        res.status = response.statusCode;
        res.set_content(response.body.toStdString(), response.contentType.toStdString());
        applyCorsHeaders(res, req);
    }

    //==============================================================================
    /** Resolve the configured CORS policy and write the matching headers onto `res`.

        Policy interpretation (`currentCorsOrigins`):
        - `"*"`        : always emit `Access-Control-Allow-Origin: *`.
        - empty        : emit no CORS headers (legacy behavior, lets the user opt out).
        - origin list  : comma-separated. Echo the request's `Origin` header iff it
                         appears in the list; otherwise emit no CORS headers.
    */
    void applyCorsHeaders(httplib::Response& res, const httplib::Request& req) const
    {
        if (currentCorsOrigins.isEmpty())
            return;

        String allowOrigin;
        bool echoingSpecificOrigin = false;

        if (currentCorsOrigins == "*")
        {
            allowOrigin = "*";
        }
        else
        {
            auto requestOriginIt = req.headers.find("Origin");
            if (requestOriginIt == req.headers.end())
                return;

            String requestOrigin(requestOriginIt->second);

            StringArray allowed;
            allowed.addTokens(currentCorsOrigins, ",", {});
            allowed.trim();
            allowed.removeEmptyStrings();

            if (! allowed.contains(requestOrigin))
                return;

            allowOrigin = requestOrigin;
            echoingSpecificOrigin = true;
        }

        res.set_header("Access-Control-Allow-Origin", allowOrigin.toStdString());
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Max-Age", "86400");

        if (echoingSpecificOrigin)
            res.set_header("Vary", "Origin");
    }

    //==============================================================================
    std::unique_ptr<httplib::Server> server;
    std::map<std::pair<Method, String>, RouteInfo> routes;
    
    int currentPort = 0;
    String currentBindAddress;
    String currentCorsOrigins;
    std::atomic<bool> running{false};

    CriticalSection requestSerializationLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Impl)
};

//==============================================================================
// RestServer public interface implementation

RestServer::RestServer() : pimpl(std::make_unique<Impl>()) {}

RestServer::~RestServer()
{
    stop();
}

URL RestServer::getBaseURL() const
{
    int port = pimpl->getServerPort();  // Returns 0 if not running
    return URL("http://localhost:" + String(port));
}

void RestServer::addRoute(Method method, const URL& routeUrl, RouteHandler handler)
{
    pimpl->addRoute(method, routeUrl, std::move(handler));
}

void RestServer::addAsyncRoute(Method method, const URL& routeUrl, AsyncRouteHandler handler)
{
    addRoute(method, routeUrl, [this, handler](const Request& req) -> Response
    {
        AsyncRequest::Ptr asyncReq = new AsyncRequest(req);
        asyncReq->setTestMode(isTestMode());
        return handler(asyncReq);
    });
}

bool RestServer::start(int port, const String& bindAddress, const String& corsAllowedOrigins)
{
    return pimpl->startServer(port, bindAddress, corsAllowedOrigins);
}

void RestServer::stop()
{
    pimpl->stopServer();
}

bool RestServer::isRunning() const
{
    return pimpl->isServerRunning();
}

int RestServer::getPort() const
{
    return pimpl->getServerPort();
}

void RestServer::addListener(Listener* listener)
{
    pimpl->listeners.add(listener);

	if (isRunning())
		listener->serverStarted(getPort());
}

void RestServer::removeListener(Listener* listener)
{
	if (isRunning())
		listener->serverStopped();

    pimpl->listeners.remove(listener);
}

} // namespace hise
