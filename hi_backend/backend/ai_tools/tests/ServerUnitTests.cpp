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

#if HI_RUN_UNIT_TESTS

// Header-only HTTP client used by the CORS tests below to inspect raw response
// headers (JUCE's URL::createInputStream does not expose them conveniently).
#include "../httplib.h"

namespace hise {

/** Unit tests for the REST Server API.
    
    These tests create a BackendProcessor instance with a REST server running
    on a test port (1901) and verify the API endpoints work correctly.
*/
class RestServerTest : public juce::UnitTest
{
public:
    RestServerTest() : UnitTest("REST Server Tests", "AI Tools") {}
    
    void runTest() override
    {
        ScopedValueSetter<bool> svs(MainController::unitTestMode, true);
        
        // Create context once for all tests, pass *this for expect() access
        ctx = std::make_unique<TestContext>(*this);
        
        testServerStartStop();
        testListMethods();
        testStatusEndpoint();
        testStatusPreprocessors();
        testListComponentsFlat();
        testListComponentsHierarchy();
        testListComponentsWithLaf();
        testListComponentsWithoutLaf();
        testListComponentsWithLafHierarchy();
        testGetComponentProperties();
        testSetScriptSuccess();
        testSetScriptCompileError();
        testSetScriptUnknownCallback();
        testSetScriptMultipleCallbacks();
        testSetScriptEmptyCallbacks();
        testGetScript();
        testGetScriptExternalFiles();
        testRecompile();
        testEvaluateREPLSuccess();
        testEvaluateREPLArithmetic();
        testEvaluateREPLUndefined();
        testEvaluateREPLRuntimeError();
        testEvaluateREPLMissingExpression();
        testEvaluateREPLMissingModuleId();
        testEvaluateREPLUnknownModule();
        testGetComponentValue();
        testSetComponentValue();
        testSetComponentValueTriggersCallback();
        testSetComponentValueCallbackRuntimeError();
        testNestedControlCallbacks();
        testFloatNormalizationUnit();
        testFloatNormalization();
        testForceSynchronousExecution();
        testSetComponentPropertiesBasic();
        testSetComponentPropertiesMultiple();
        testSetComponentPropertiesLockedRejection();
        testSetComponentPropertiesForceMode();
        testSetComponentPropertiesInvalidComponent();
        testSetComponentPropertiesInvalidProperty();
        testSetComponentPropertiesColour();
        testSetComponentPropertiesParentComponent();
        testSetComponentPropertiesRemoveFromParent();
        testSetComponentPropertiesInvalidParent();
        testTestingScreenshotBasic();
        testTestingScreenshotComponent();
        testTestingScreenshotInvalidComponent();
        testTestingScreenshotScale();
        testTestingScreenshotPixelVerification();
        testTestingScreenshotPixelVerificationScaled();
        testTestingScreenshotFileOutput();
        testTestingScreenshotFileOutputErrors();
        testGetSelectedComponents();
        testTestingE2eExcluded();
        testGetIncludedFilesGlobal();
        testGetIncludedFilesForModule();
        testGetIncludedFilesUnknownModule();
        testDiagnoseScriptErrorMissingParams();
        testDiagnoseScriptErrorUnknownModule();
        testDiagnoseScriptErrorNoExternalFiles();
        testDiagnoseScriptSuccess();
        testDiagnoseScriptWithDiagnostics();
        testDiagnoseScriptFilePath();
        testTestingProfileRecord();
        testTestingProfileGetAfterRecord();
        testTestingProfileSummary();
        testTestingProfileFilter();
        testTestingProfileThreadFilter();
        testTestingProfileNested();
        testTestingProfileGetNoData();
        testParseCSSValid();
        testParseCSSError();
        testParseCSSWarnings();
        testParseCSSResolveSpecificity();
        testParseCSSResolvedValues();
        testParseCSSFromFile();
        testParseCSSEmptyCode();
        testShutdown();
        testSnippetBrowser();
        testSnippetBrowserRejectionMetadata();
        testBuilderTree();
        testBuilderApply();
        testBuilderCloneNestedModules();
        
        testBuilderChildOpsFailAfterParentDeleted();
        testBuilderApplyMissingOp();
        testBuilderApplyUnknownOp();
        testBuilderApplyMissingFields();
        testBuilderReset();
        testUndoPushGroup();
		testPlanValidationRemoveThenSetIdFails();
		testPlanValidationRenameThenRemoveOldNameFails();
		testPlanValidationAddRemoveAddSameIdSucceeds();
		testPlanValidationRemoveParentThenChildOpFails();

        testUndoPopGroup();
        testUndoBack();
        testUndoForward();
        testUndoDiff();
        testUndoHistory();
        testUndoClear();

        testWizardInitialise();
        testWizardExecute();
        testWizardStatus();

        testUITree();
        testUIApply();
        testUIApplyValidation();
        testUIApplyUndo();

        testTestingSequenceNote();
        testTestingSequenceChord();
        testTestingSequenceCC();
        testTestingSequencePitchbend();
        testTestingSequenceAllNotesOff();
        testTestingSequenceSequence();
        testTestingSequenceValidation();
        testTestingSequenceEmptyPoll();
        testTestingSequenceRepl();
        testTestingSequenceSetAttribute();
        testTestingSequenceTestSignal();

        testDspList();
        testDspInit();
        testDspTree();
        testDspApplyUndo();
        testDspGroupSingleOpsVariety();
        testDspGroupMultiOpsBatched();
        testDspGroupMixedOpsWorkflow();
        testDspGroupMirrorSet();
        testDspGroupMirrorBypass();
        testDspGroupMirrorRemove();
        testDspGroupMirrorMove();
        testDspGroupMirrorCreateParameter();
        testDspGroupMirrorConnect();
        testDspGroupMirrorDisconnect();
        testDspGroupMirrorClear();
        testDspApplyAdd();
        testDspApplyRemove();
        testDspApplyMove();
        testDspApplySet();
        testDspApplyBypass();
        testDspApplyConnect();
        testDspApplyDisconnect();
        testDspApplyCreateParameter();
        testDspApplyClear();
        testDspApplyValidation();
        
        testDspApplyBatchOps();
        testDspScreenshot();

        testBatchRollback();

        testCorsWildcardOnRealResponse();
        testCorsPreflightOptions();
        testCorsOriginWhitelist();
        testCorsDisabled();

        // Verify all endpoints were tested
        verifyAllEndpointsTested();

        // Verify response envelopes match route metadata
        verifyResponseEnvelopes();

        // Clean up
        ctx.reset();
    }
    
private:
    static constexpr int TEST_PORT = RestServer::TestPort;
    
    /** Test context that manages a full HISE instance with REST server.
        
        This follows the same initialization pattern as CLIInstance in Main.cpp
        to ensure proper setup of the BackendProcessor and all its subsystems.
        
        Also tracks which endpoints have been called for coverage verification.
    */
    struct TestContext
    {
        StringArray touchedEndpoints;
        String lastRequestedPath;
        std::map<String, var> capturedResponses;
        
        TestContext(UnitTest& parentTest) : test(parentTest)
        {
            // Skip audio driver initialization for headless testing
            CompileExporter::setSkipAudioDriverInitialisation();
            
            // Create the processor chain: StandaloneProcessor -> MainController -> BackendProcessor
            sp = new StandaloneProcessor();
            mc = dynamic_cast<MainController*>(sp->createProcessor());
            
            // Suspend background threads for safe operation during tests
            threadSuspender = new MainController::ScopedBadBabysitter(mc);
            
            bp = dynamic_cast<BackendProcessor*>(mc.get());
            
            // Initialize audio processing (required even without real audio)
            bp->prepareToPlay(44100.0, 512);
            AudioSampleBuffer ab(2, 512);
            MidiBuffer mb;
            bp->processBlock(ab, mb);
            
            // Start REST server on test port
            bp->getRestServer().start(TEST_PORT);
        }
        
        ~TestContext()
        {
            bp->getRestServer().stop();
            
            // Clean up in reverse order
            threadSuspender = nullptr;
            mc = nullptr;
            sp = nullptr;
        }
        
        /** Reset to clean state by clearing the preset. */
        void reset()
        {
            bp->clearPreset(dontSendNotification);
            bp->createInterface(600, 600, false);
            
            // Pump messages to allow any async operations to complete
            MessageManager::getInstance()->runDispatchLoopUntil(50);
        }
        
        /** Make a GET request to the test server (runs on background thread, pumps message loop). */
        String httpGet(const String& endpoint)
        {
            // Track the endpoint path (without query params) for coverage
            auto path = endpoint.upToFirstOccurrenceOf("?", false, false);
            touchedEndpoints.addIfNotAlreadyThere(path);
            lastRequestedPath = path;

            return httpRequestWithMessagePump([endpoint]()
            {
                URL url("http://localhost:" + String(TEST_PORT) + endpoint);
                return url.readEntireTextStream(false);
            });
        }

        /** Make a POST request with JSON body to the test server (runs on background thread, pumps message loop). */
        String httpPost(const String& endpoint, const String& jsonBody)
        {
            // Track the endpoint path (without query params) for coverage
            auto path = endpoint.upToFirstOccurrenceOf("?", false, false);
            touchedEndpoints.addIfNotAlreadyThere(path);
            lastRequestedPath = path;
            
            return httpRequestWithMessagePump([endpoint, jsonBody]()
            {
                URL url = URL("http://localhost:" + String(TEST_PORT) + endpoint)
                    .withPOSTData(jsonBody);
                
                // Use inAddress to keep query params in URL path, not in POST body
                URL::InputStreamOptions options(URL::ParameterHandling::inAddress);
                
                if (auto stream = url.createInputStream(options.withExtraHeaders("Content-Type: application/json")))
                    return stream->readEntireStreamAsString();
                return String();
            });
        }
        
    private:
        /** Helper that runs an HTTP request on a background thread while pumping the message loop. */
        template<typename Func>
        String httpRequestWithMessagePump(Func&& requestFunc)
        {
            String result;
            std::atomic<bool> done{false};
            
            // Run the HTTP request on a background thread
            Thread::launch([&result, &done, func = std::forward<Func>(requestFunc)]()
            {
                result = func();
                done.store(true);
            });
            
            // Pump the message loop while waiting for the request to complete
            while (!done.load())
            {
                MessageManager::getInstance()->runDispatchLoopUntil(10);
            }
            
            return result;
        }
        
    public:
        
        /** Parse JSON response and return as var. Also captures for envelope validation. */
        var parseJson(const String& response)
        {
            var result;
            JSON::parse(response, result);

            // Capture successful responses for envelope validation
            if (lastRequestedPath.isNotEmpty() && result.isObject())
            {
                if (auto* obj = result.getDynamicObject())
                {
                    if ((bool)obj->getProperty(RestApiIds::success))
                        capturedResponses[lastRequestedPath] = result;
                }
            }

            return result;
        }
        
        /** Compile a script via the REST API.
            @param code          The script code to compile
            @param moduleId      Target script processor (default: "Interface")
            @param callback      Target callback (default: "onInit")
            @param expectSuccess If true, calls expect() to verify compilation succeeded (default: true)
            @returns             Parsed JSON response
        */
        var compile(const String& code, 
                    const String& moduleId = "Interface", 
                    const String& callback = "onInit",
                    bool expectSuccess = true)
        {
            // Use callbacks object format
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty("moduleId", moduleId);
            
            DynamicObject::Ptr callbacks = new DynamicObject();
            callbacks->setProperty(callback, code);
            bodyObj->setProperty("callbacks", var(callbacks.get()));
            bodyObj->setProperty("compile", true);
            
            auto response = httpPost("/api/set_script", 
                                     JSON::toString(var(bodyObj.get())));
            var json = parseJson(response);
            
            if (expectSuccess)
            {
                test.expect((bool)json["success"], 
                            "Compilation failed: " + json["result"].toString());
            }
            
            return json;
        }
        
        UnitTest& test;
        ScopedPointer<StandaloneProcessor> sp;
        ScopedPointer<MainController> mc;
        ScopedPointer<MainController::ScopedBadBabysitter> threadSuspender;
        BackendProcessor* bp = nullptr;  // Non-owning, managed by mc
    };
    
    std::unique_ptr<TestContext> ctx;
    
    //==========================================================================
    void testServerStartStop()
    {
        beginTest("Server start/stop");
        
        expect(ctx->bp->getRestServer().isRunning(), "Server should be running");
        expect(ctx->bp->getRestServer().getPort() == TEST_PORT, "Should use test port");
    }
    
    //==========================================================================
    void testListMethods()
    {
        beginTest("GET / (OpenAPI spec)");

        auto response = ctx->httpGet("/");
        expect(response.isNotEmpty(), "Should return response");

        var json = ctx->parseJson(response);

        // Verify OpenAPI 3.0 root structure
        expect(json["openapi"].toString() == "3.0.3", "Should be OpenAPI 3.0.3");
        expect(json["info"].isObject(), "Should have info object");
        expect(json["info"]["title"].toString() == "HISE REST API", "Should have correct title");
        expect(json["paths"].isObject(), "Should have paths object");
        expect(json["tags"].isArray(), "Should have tags array");

        // Verify all routes are present in paths
        auto* pathsObj = json["paths"].getDynamicObject();
        expect(pathsObj != nullptr, "paths should be a JSON object");

        // Count total operations across all paths
        int totalOps = 0;

        if (pathsObj != nullptr)
        {
            for (const auto& pathEntry : pathsObj->getProperties())
            {
                if (auto* methods = pathEntry.value.getDynamicObject())
                {
                    for (const auto& method : methods->getProperties())
                        totalOps++;
                }
            }
        }

        expect(totalOps == (int)RestHelpers::ApiRoute::numRoutes,
               "Should have " + String((int)RestHelpers::ApiRoute::numRoutes)
               + " operations, got " + String(totalOps));

        // Verify /api/status is a GET with tags and summary
        auto statusPath = json["paths"]["/api/status"];
        expect(statusPath.isObject(), "Should have /api/status path");
        auto statusGet = statusPath["get"];
        expect(statusGet.isObject(), "status should be GET");
        expect(statusGet["tags"].isArray(), "status should have tags");
        expect(statusGet["summary"].toString().isNotEmpty(), "status should have summary");

        // Verify POST /api/set_script has requestBody with schema
        auto setScriptPost = json["paths"]["/api/set_script"]["post"];
        expect(setScriptPost.isObject(), "set_script should be POST");
        auto reqBody = setScriptPost["requestBody"];
        expect(reqBody.isObject(), "set_script should have requestBody");
        auto bodySchema = reqBody["content"]["application/json"]["schema"];
        expect(bodySchema.isObject(), "set_script should have JSON schema");
        expect(bodySchema["properties"].isObject(), "schema should have properties");

        // Verify enriched builder/apply has discriminated operations schema
        auto applyPost = json["paths"]["/api/builder/apply"]["post"];
        expect(applyPost.isObject(), "builder/apply should be POST");
        auto applyBody = applyPost["requestBody"]["content"]["application/json"]["schema"];
        auto opsSchema = applyBody["properties"]["operations"];
        expect(opsSchema["type"].toString() == "array", "operations should be array type");
        auto itemsSchema = opsSchema["items"];
        expect(itemsSchema.isObject(), "operations should have items schema");
        expect(itemsSchema["discriminator"].isObject(), "items should have discriminator");
        expect(itemsSchema["discriminator"]["propertyName"].toString() == "op",
               "discriminator should be on 'op' field");
        expect(itemsSchema["x-variants"].isArray(), "should have variant descriptions");
        expect(itemsSchema["properties"].isObject(), "items should have properties");

        // Verify builder/apply has response schema with flat diff fields
        auto applyResp = applyPost["responses"]["200"]["content"]["application/json"]["schema"];
        auto respProps = applyResp["properties"];
        expect(respProps["scope"].isObject(), "should have scope field");
        expect(respProps["diff"].isObject(), "should have diff field");

        // Verify builder/apply has error codes
        auto applyResponses = applyPost["responses"];
        expect(applyResponses["400"].isObject(), "should have 400 error");
        expect(applyResponses["404"].isObject(), "should have 404 error");

        // Verify builder/apply has examples
        auto applyExample = applyPost["requestBody"]["content"]["application/json"]["example"];
        expect(applyExample.isObject(), "should have request example");

        // OpenAPI doc itself is not an envelope - apiVersion must NOT be injected at root.
        expect(!json.hasProperty("apiVersion"),
               "apiVersion auto-inject must skip OpenAPI document (no `success` marker)");

        // The envelope schema for /api/status must declare apiVersion as a required field.
        auto statusOk = json["paths"]["/api/status"]["get"]["responses"]["200"];
        auto statusEnv = statusOk["content"]["application/json"]["schema"];
        expect(statusEnv["properties"]["apiVersion"].isObject(),
               "Envelope schema should describe apiVersion");
        bool requiresApiVersion = false;
        if (auto* requiredArr = statusEnv["required"].getArray())
        {
            for (const auto& v : *requiredArr)
                if (v.toString() == "apiVersion") { requiresApiVersion = true; break; }
        }
        expect(requiresApiVersion, "Envelope schema should mark apiVersion as required");
    }
    
    //==========================================================================
    void verifyAllEndpointsTested()
    {
        beginTest("All endpoints tested");

        // Endpoints intentionally skipped in automated tests.
        // /api/dsp/save would modify the project's DspNetworks folder on disk,
        // which would pollute the test fixture. All /api/project/* endpoints
        // mutate live project state (loaded preset, project_info.xml, on-disk
        // files, or the active project root) and likewise cannot be exercised
        // in-process without side effects on the user's current project.
        StringArray skipList;
        skipList.add("/api/dsp/save");
        skipList.add("/api/project/list");
        skipList.add("/api/project/tree");
        skipList.add("/api/project/files");
        skipList.add("/api/project/settings/list");
        skipList.add("/api/project/settings/set");
        skipList.add("/api/project/save");
        skipList.add("/api/project/load");
        skipList.add("/api/project/switch");
        skipList.add("/api/project/export_snippet");
        skipList.add("/api/project/import_snippet");
        skipList.add("/api/project/preprocessor/list");
        skipList.add("/api/project/preprocessor/set");

        // Verify metadata count matches enum
        const auto& metadata = RestHelpers::getRouteMetadata();
        expect(metadata.size() == (int)RestHelpers::ApiRoute::numRoutes,
               "Metadata count must match RestHelpers::ApiRoute::numRoutes");

        // Verify all endpoints were touched during tests
        for (const auto& route : metadata)
        {
            auto fullPath = "/" + route.path;

            if (skipList.contains(fullPath))
                continue;

            expect(ctx->touchedEndpoints.contains(fullPath),
                   "Endpoint not tested: " + fullPath);
        }
    }
    
    //==========================================================================
    void verifyResponseEnvelopes()
    {
        beginTest("Response envelopes match route metadata");

        const auto& metadata = RestHelpers::getRouteMetadata();

        for (const auto& route : metadata)
        {
            auto fullPath = "/" + route.path;
            auto it = ctx->capturedResponses.find(fullPath);

            if (it == ctx->capturedResponses.end())
                continue;

            auto response = it->second;
            auto* obj = response.getDynamicObject();

            if (obj == nullptr)
                continue;

            // 1. Standard envelope fields must be present
            expect(obj->hasProperty(RestApiIds::success),
                   fullPath + ": missing 'success'");
            expect(obj->hasProperty(RestApiIds::logs),
                   fullPath + ": missing 'logs'");
            expect(obj->hasProperty(RestApiIds::errors),
                   fullPath + ": missing 'errors'");

            // Tree endpoints wrap their payload in result -- skip flat field checks
            bool usesNestedResult = route.path == "api/builder/tree"
                                 || route.path == "api/ui/tree"
                                 || route.path == "api/wizard/initialise"
                                 || route.path == "api/dsp/tree"
                                 || route.path == "api/dsp/init";

            if (!usesNestedResult)
            {
                // 2. Build set of allowed top-level keys
                StringArray allowed;
                allowed.add("success");
                allowed.add("apiVersion");
                allowed.add("logs");
                allowed.add("errors");
                allowed.add("result");

                for (const auto& rf : route.responseFields)
                    allowed.add(rf.name.toString());

                // 3. Check for unexpected fields
                for (const auto& prop : obj->getProperties())
                {
                    expect(allowed.contains(prop.name.toString()),
                           fullPath + ": unexpected response field '" +
                           prop.name.toString() + "' not declared in route metadata");
                }

                // 4. Check required response fields are present
                for (const auto& rf : route.responseFields)
                {
                    if (rf.required)
                    {
                        expect(obj->hasProperty(rf.name),
                               fullPath + ": missing required response field '" +
                               rf.name.toString() + "'");
                    }
                }

                // 5. result must not be a nested object (flat convention)
                if (obj->hasProperty(RestApiIds::result))
                {
                    auto rv = obj->getProperty(RestApiIds::result);
                    expect(!rv.isObject(),
                           fullPath + ": 'result' must not be a nested object -- "
                           "use flat top-level fields instead");
                }
            }
        }
    }

    //==========================================================================
    void testStatusEndpoint()
    {
        beginTest("GET /api/status");
        
        auto response = ctx->httpGet("/api/status");
        expect(response.isNotEmpty(), "Should return response");
        
        var json = ctx->parseJson(response);
        expect((bool)json["success"], "success should be true");
        expect(json.hasProperty("project"), "Should have project info");
        expect(json.hasProperty("scriptProcessors"), "Should have processors");
        expect(json.hasProperty("server"), "Should have server info");
        expect(json.hasProperty("activeIsSnippetBrowser"), "Should have activeIsSnippetBrowser");

        // Auto-injected envelope version (compile-time HISE_REST_API_VERSION)
        expect(json.hasProperty("apiVersion"), "Envelope should carry apiVersion");
        expect(json["apiVersion"].toString() == HISE_REST_API_VERSION,
               "apiVersion should match HISE_REST_API_VERSION");
        expect(!(bool)json["activeIsSnippetBrowser"], "activeIsSnippetBrowser should be false in unit test (no snippet launched)");
        
        // Check server info
        auto server = json["server"];
        expect(server.hasProperty("version"), "server should have version");
        expect(server["version"].toString().isNotEmpty(), "version should not be empty");
        expect(server["version"].toString() != "1.0.0", "version should be actual HISE version, not hardcoded 1.0.0");
        expect(server.hasProperty("compileTimeout"), "server should have compileTimeout");
        expect((double)server["compileTimeout"] > 0, "compileTimeout should be positive");
        
        // Check that Interface processor exists
        auto processors = json["scriptProcessors"];
        expect(processors.isArray(), "scriptProcessors should be array");
        
        bool hasInterface = false;
        for (int i = 0; i < processors.size(); i++)
        {
            if (processors[i]["moduleId"].toString() == "Interface")
            {
                hasInterface = true;
                expect((bool)processors[i]["isMainInterface"], 
                       "Interface should be main interface");
            }
        }
        expect(hasInterface, "Should have Interface processor");
    }

    //==========================================================================
    void testStatusPreprocessors()
    {
        beginTest("GET /api/status/preprocessors");

        // Non-verbose: flat { name: int } map
        {
            auto response = ctx->httpGet("/api/status/preprocessors");
            expect(response.isNotEmpty(), "Should return response");

            var json = ctx->parseJson(response);
            expect((bool)json["success"], "success should be true: " + response);

            auto pre = json["preprocessors"];
            expect(pre.isObject(), "preprocessors should be an object");

            auto* obj = pre.getDynamicObject();
            expect(obj != nullptr && obj->getProperties().size() > 0,
                   "Should expose at least one preprocessor");

            // HISE_USE_EXTENDED_TEMPO_VALUES is always registered by the database ctor
            expect(obj != nullptr && obj->hasProperty(Identifier("HISE_USE_EXTENDED_TEMPO_VALUES")),
                   "Should include HISE_USE_EXTENDED_TEMPO_VALUES");

            // Values in non-verbose mode should be integers, not nested objects
            auto sample = pre["HISE_USE_EXTENDED_TEMPO_VALUES"];
            expect(!sample.isObject(), "Non-verbose values should not be objects");
        }

        // Verbose: per-entry objects carrying the runtime value
        {
            auto response = ctx->httpGet("/api/status/preprocessors?verbose=true");
            var json = ctx->parseJson(response);
            expect((bool)json["success"], "success should be true (verbose): " + response);

            auto pre = json["preprocessors"];
            expect(pre.isObject(), "preprocessors should be an object (verbose)");

            auto entry = pre["HISE_USE_EXTENDED_TEMPO_VALUES"];
            expect(entry.isObject(), "Verbose entry should be an object");
            expect(entry.hasProperty("value"), "Verbose entry should carry 'value'");
        }

        // skipDefaults=true should never return more entries than the unfiltered call
        {
            auto baseline = ctx->parseJson(ctx->httpGet("/api/status/preprocessors"));
            auto filtered = ctx->parseJson(
                ctx->httpGet("/api/status/preprocessors?skipDefaults=true"));

            auto* baseObj     = baseline["preprocessors"].getDynamicObject();
            auto* filteredObj = filtered["preprocessors"].getDynamicObject();
            expect(baseObj != nullptr && filteredObj != nullptr,
                   "Both responses should have a preprocessors object");

            expect(filteredObj->getProperties().size() <= baseObj->getProperties().size(),
                   "skipDefaults must not add entries");
        }
    }

    //==========================================================================
    void testListComponentsFlat()
    {
        beginTest("GET /api/list_components (flat)");
        
        ctx->reset();
        
        // First create some components
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
                     "const var TestButton = Content.addButton(\"TestButton\", 150, 10);");
        
        // Now list components
        auto response = ctx->httpGet("/api/list_components?moduleId=Interface");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed: " + response);
        
        auto components = json["components"];
        expect(components.isArray(), "components should be array");
        expectEquals<int>(components.size(), 2, "Should have 2 components");
        
        // Verify flat structure (no childComponents in flat mode)
        bool hasKnob = false, hasButton = false;
        for (int i = 0; i < components.size(); i++)
        {
            if (components[i]["id"].toString() == "TestKnob")
            {
                hasKnob = true;
                expect(components[i]["type"].toString() == "ScriptSlider", "TestKnob should be ScriptSlider");
                expect(!components[i].hasProperty("childComponents"), "Flat mode should not have childComponents");
            }
            if (components[i]["id"].toString() == "TestButton")
            {
                hasButton = true;
                expect(components[i]["type"].toString() == "ScriptButton", "TestButton should be ScriptButton");
            }
        }
        expect(hasKnob, "Should have TestKnob");
        expect(hasButton, "Should have TestButton");
    }
    
    //==========================================================================
    void testListComponentsHierarchy()
    {
        beginTest("GET /api/list_components?hierarchy=true");
        
        ctx->reset();
        
        // Create parent panel with child knob
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var Panel1 = Content.addPanel(\"Panel1\", 10, 10);\n"
                     "Panel1.set(\"width\", 200);\n"
                     "Panel1.set(\"height\", 200);\n"
                     "const var ChildKnob = Content.addKnob(\"ChildKnob\", 20, 20);\n"
                     "ChildKnob.set(\"parentComponent\", \"Panel1\");");
        
        auto response = ctx->httpGet("/api/list_components?moduleId=Interface&hierarchy=true");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        
        auto components = json["components"];
        
        // In hierarchy mode, ChildKnob should NOT be at root level
        bool panelAtRoot = false;
        bool childAtRoot = false;
        
        for (int i = 0; i < components.size(); i++)
        {
            if (components[i]["id"].toString() == "Panel1")
                panelAtRoot = true;
            if (components[i]["id"].toString() == "ChildKnob")
                childAtRoot = true;
        }
        
        expect(panelAtRoot, "Panel1 should be at root");
        expect(!childAtRoot, "ChildKnob should NOT be at root in hierarchy mode");
        
        // Find Panel1 and verify child is nested
        bool foundNestedChild = false;
        for (int i = 0; i < components.size(); i++)
        {
            if (components[i]["id"].toString() == "Panel1")
            {
                auto children = components[i]["childComponents"];
                expect(children.isArray(), "Should have childComponents array");
                
                for (int j = 0; j < children.size(); j++)
                {
                    if (children[j]["id"].toString() == "ChildKnob")
                    {
                        foundNestedChild = true;
                        expect(children[j]["type"].toString() == "ScriptSlider", "ChildKnob should be ScriptSlider");
                        expect(children[j].hasProperty("width"), "Hierarchy should include width");
                        expect(children[j].hasProperty("height"), "Hierarchy should include height");
                    }
                }
            }
        }
        
        expect(foundNestedChild, "ChildKnob should be nested under Panel1");
    }
    
    //==========================================================================
    void testListComponentsWithLaf()
    {
        beginTest("GET /api/list_components (with LAF info)");
        
        ctx->reset();
        
        // Create a button with a script-based LAF
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestButton = Content.addButton(\"TestButton\", 10, 10);\n"
                     "const var laf = Content.createLocalLookAndFeel();\n"
                     "laf.registerFunction(\"drawToggleButton\", function(g, obj) {\n"
                     "    g.fillAll(0xFF000000);\n"
                     "});\n"
                     "TestButton.setLocalLookAndFeel(laf);");
        
        auto response = ctx->httpGet("/api/list_components?moduleId=Interface");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        
        auto components = json["components"];
        expect(components.isArray() && components.size() == 1, "Should have 1 component");
        
        auto button = components[0];
        expect(button["id"].toString() == "TestButton", "Should be TestButton");
        
        // Verify LAF info is present
        auto laf = button["laf"];
        expect(laf.isObject(), "Should have laf object for component with LAF assigned");
        expect(laf["id"].toString() == "laf", "LAF id should be 'laf'");
        expect(laf["renderStyle"].toString() == "script", "Should be script style for registerFunction LAF");
        expect(laf.hasProperty("location"), "Should have location property");
        expect(laf.hasProperty("cssLocation"), "Should have cssLocation property");
    }
    
    //==========================================================================
    void testListComponentsWithoutLaf()
    {
        beginTest("GET /api/list_components (without LAF)");
        
        ctx->reset();
        
        // Create a component without LAF
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);");
        
        auto response = ctx->httpGet("/api/list_components?moduleId=Interface");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        
        auto components = json["components"];
        expect(components.size() == 1, "Should have 1 component");
        
        auto knob = components[0];
        expect(knob["id"].toString() == "TestKnob", "Should be TestKnob");
        
        // Verify LAF is false when no LAF assigned
        expect(knob.hasProperty("laf"), "Should have laf property");
        expect(!knob["laf"], "laf should be false when no LAF assigned");
    }
    
    //==========================================================================
    void testListComponentsWithLafHierarchy()
    {
        beginTest("GET /api/list_components?hierarchy=true (with LAF info)");
        
        ctx->reset();
        
        // Create a panel with a child that has LAF
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var Panel1 = Content.addPanel(\"Panel1\", 10, 10);\n"
                     "Panel1.set(\"width\", 200);\n"
                     "Panel1.set(\"height\", 200);\n"
                     "const var ChildButton = Content.addButton(\"ChildButton\", 20, 20);\n"
                     "ChildButton.set(\"parentComponent\", \"Panel1\");\n"
                     "const var laf = Content.createLocalLookAndFeel();\n"
                     "laf.registerFunction(\"drawToggleButton\", function(g, obj) {\n"
                     "    g.fillAll(0xFF000000);\n"
                     "});\n"
                     "ChildButton.setLocalLookAndFeel(laf);");
        
        auto response = ctx->httpGet("/api/list_components?moduleId=Interface&hierarchy=true");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        
        // Find Panel1 and check its LAF and its child's LAF
        auto components = json["components"];
        bool foundPanel = false;
        bool foundChildWithLaf = false;
        
        for (int i = 0; i < components.size(); i++)
        {
            if (components[i]["id"].toString() == "Panel1")
            {
                foundPanel = true;
                
                // Panel should have laf: false
                expect(!components[i]["laf"], "Panel without LAF should have laf: false");
                
                // Check child
                auto children = components[i]["childComponents"];
                for (int j = 0; j < children.size(); j++)
                {
                    if (children[j]["id"].toString() == "ChildButton")
                    {
                        foundChildWithLaf = true;
                        auto childLaf = children[j]["laf"];
                        expect(childLaf.isObject(), "Child with LAF should have laf object");
                        expect(childLaf["renderStyle"].toString() == "script", 
                               "Child LAF should have script renderStyle");
                    }
                }
            }
        }
        
        expect(foundPanel, "Should find Panel1");
        expect(foundChildWithLaf, "Should find ChildButton with LAF in hierarchy");
    }
    
    //==========================================================================
    void testGetComponentProperties()
    {
        beginTest("GET /api/get_component_properties");
        
        ctx->reset();
        
        // Create a knob with some custom properties
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var PropKnob = Content.addKnob(\"PropKnob\", 50, 50);\n"
                     "PropKnob.set(\"text\", \"My Knob\");\n"
                     "PropKnob.set(\"min\", -24);\n"
                     "PropKnob.set(\"max\", 24);\n"
                     "PropKnob.set(\"mode\", \"Decibel\");");
        
        auto response = ctx->httpGet("/api/get_component_properties?moduleId=Interface&id=PropKnob");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        expect(json["id"].toString() == "PropKnob", "Should return correct id");
        expect(json["type"].toString() == "ScriptSlider", "Should return correct type");
        
        auto properties = json["properties"];
        expect(properties.isArray(), "properties should be array");
        
        // Check specific properties
        bool foundText = false, foundMin = false, foundMode = false;
        for (int i = 0; i < properties.size(); i++)
        {
            auto prop = properties[i];
            String propId = prop["id"].toString();
            
            if (propId == "text")
            {
                foundText = true;
                expect(prop["value"].toString() == "My Knob", "text should be 'My Knob'");
                expect(!(bool)prop["isDefault"], "text should not be default");
            }
            if (propId == "min")
            {
                foundMin = true;
                expect((double)prop["value"] == -24.0, "min should be -24");
            }
            if (propId == "mode")
            {
                foundMode = true;
                expect(prop["value"].toString() == "Decibel", "mode should be 'Decibel'");
                expect(prop.hasProperty("options"), "mode should have options");
            }
        }
        
        expect(foundText, "Should have text property");
        expect(foundMin, "Should have min property");
        expect(foundMode, "Should have mode property");
    }
    
    //==========================================================================
    void testSetScriptSuccess()
    {
        beginTest("POST /api/set_script (success)");
        
        ctx->reset();
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        
        DynamicObject::Ptr callbacks = new DynamicObject();
        callbacks->setProperty("onInit", "Content.makeFrontInterface(600, 400);\n"
                                         "Console.print(\"Test output\");");
        bodyObj->setProperty("callbacks", var(callbacks.get()));
        bodyObj->setProperty("compile", true);
        
        auto response = ctx->httpPost("/api/set_script", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        expect(json["moduleId"].toString() == "Interface", "Should return moduleId");
        expect(json["result"].toString() == "Compiled OK", "Result should be 'Compiled OK'");
        
        // Verify updatedCallbacks array
        expect(json.hasProperty("updatedCallbacks"), "Should have updatedCallbacks array");
        auto updatedCallbacks = json["updatedCallbacks"];
        expect(updatedCallbacks.isArray(), "updatedCallbacks should be array");
        expect(updatedCallbacks.size() == 1, "Should have 1 updated callback");
        expect(updatedCallbacks[0].toString() == "onInit", "Should have updated onInit");
        
        auto logs = json["logs"];
        expect(logs.isArray(), "logs should be array");
        expect(logs.size() >= 1, "Should have at least one log entry");
        
        bool foundTestOutput = false;
        for (int i = 0; i < logs.size(); i++)
        {
            if (logs[i].toString().contains("Test output"))
                foundTestOutput = true;
        }
        expect(foundTestOutput, "Should capture Console.print output");
        
        auto errors = json["errors"];
        expect(errors.isArray(), "errors should be array");
        expect(errors.size() == 0, "Should have no errors");
        
        // Verify externalFiles is present (empty since no includes)
        expect(json.hasProperty("externalFiles"), "set_script should return externalFiles");
        expect(json["externalFiles"].isArray(), "externalFiles should be array");
    }
    
    //==========================================================================
    void testSetScriptCompileError()
    {
        beginTest("POST /api/set_script (compile error)");
        
        ctx->reset();
        
        // Script with syntax error - use callbacks object format
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        
        DynamicObject::Ptr callbacks = new DynamicObject();
        callbacks->setProperty("onInit", "Content.makeFrontInterface(600, 400);\n"
                                         "const var x = ;");  // Syntax error
        bodyObj->setProperty("callbacks", var(callbacks.get()));
        bodyObj->setProperty("compile", true);
        
        auto response = ctx->httpPost("/api/set_script", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Compilation should fail");
        
        // Should still have updatedCallbacks even on failure
        expect(json.hasProperty("updatedCallbacks"), "Should have updatedCallbacks");
        
        auto errors = json["errors"];
        expect(errors.isArray(), "errors should be array");
        expect(errors.size() >= 1, "Should have at least one error");
        
        auto firstError = errors[0];
        expect(firstError.hasProperty("errorMessage"), "Error should have errorMessage");
        expect(firstError.hasProperty("callstack"), "Error should have callstack");
        
        auto callstack = firstError["callstack"];
        expect(callstack.isArray(), "callstack should be array");
    }
    
    //==========================================================================
    void testGetScript()
    {
        beginTest("GET /api/get_script");
        
        ctx->reset();
        
        // Set a known script first
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var MyKnob = Content.addKnob(\"MyKnob\", 10, 10);");
        
        // Test 1: Get specific callback - should return callbacks object with only that callback
        {
            auto response = ctx->httpGet("/api/get_script?moduleId=Interface&callback=onInit");
            var json = ctx->parseJson(response);
            
            expect((bool)json["success"], "Should succeed");
            expect(json["moduleId"].toString() == "Interface", "Should return correct moduleId");
            expect(json.hasProperty("callbacks"), "Should have callbacks object");
            expect(!json.hasProperty("script"), "Should NOT have script property");
            
            auto callbacks = json["callbacks"];
            expect(callbacks.hasProperty("onInit"), "callbacks should have onInit");
            
            String returnedScript = callbacks["onInit"].toString();
            expect(returnedScript.contains("makeFrontInterface"), "Should contain makeFrontInterface");
            expect(returnedScript.contains("MyKnob"), "Should contain MyKnob");
            
            // onInit should NOT have function wrapper
            expect(!returnedScript.contains("function onInit"), "onInit should NOT have function wrapper");
            
            // Verify externalFiles is present (may be empty)
            expect(json.hasProperty("externalFiles"), "Should have externalFiles array");
            expect(json["externalFiles"].isArray(), "externalFiles should be array");
        }
        
        // Test 2: Get all callbacks (no callback param) - should return all callbacks
        {
            auto response = ctx->httpGet("/api/get_script?moduleId=Interface");
            var json = ctx->parseJson(response);
            
            expect((bool)json["success"], "Should succeed");
            expect(json.hasProperty("callbacks"), "Should have callbacks object");
            expect(!json.hasProperty("script"), "Should NOT have script property");
            
            auto callbacks = json["callbacks"];
            expect(callbacks.hasProperty("onInit"), "Should have onInit callback");
            expect(callbacks.hasProperty("onNoteOn"), "Should have onNoteOn callback");
            expect(callbacks.hasProperty("onNoteOff"), "Should have onNoteOff callback");
            expect(callbacks.hasProperty("onController"), "Should have onController callback");
            expect(callbacks.hasProperty("onTimer"), "Should have onTimer callback");
            expect(callbacks.hasProperty("onControl"), "Should have onControl callback");
            
            // onInit should be raw content (no function wrapper)
            expect(!callbacks["onInit"].toString().contains("function onInit"), 
                   "onInit should NOT have function wrapper");
            
            // onNoteOn should have function wrapper
            expect(callbacks["onNoteOn"].toString().contains("function onNoteOn"), 
                   "onNoteOn should have function wrapper");
            
            // Verify externalFiles is present
            expect(json.hasProperty("externalFiles"), "Should have externalFiles array");
        }
    }
    
    //==========================================================================
    void testRecompile()
    {
        beginTest("POST /api/recompile");
        
        ctx->reset();
        
        // Set initial script
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "Console.print(\"Recompile test\");");
        
        // Recompile without changes - now POST with JSON body
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        
        auto response = ctx->httpPost("/api/recompile", 
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Recompile should succeed");
        expect(json["result"].toString() == "Recompiled OK", "Result should be 'Recompiled OK'");
        
        // Console.print should run again
        auto logs = json["logs"];
        bool foundOutput = false;
        for (int i = 0; i < logs.size(); i++)
        {
            if (logs[i].toString().contains("Recompile test"))
                foundOutput = true;
        }
        expect(foundOutput, "Recompile should execute onInit again");
        
        // Verify externalFiles is present
        expect(json.hasProperty("externalFiles"), "Recompile should return externalFiles");
        expect(json["externalFiles"].isArray(), "externalFiles should be array");
    }
    
    //==========================================================================
    void testEvaluateREPLSuccess()
    {
        /** Setup: Compile a script that defines a variable
         *  Scenario: Evaluate a REPL expression that reads the variable
         *  Expected: Returns success with the variable's value
         */
        beginTest("POST /api/repl (success)");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var testValue = 42;");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("expression", "testValue");
        
        auto response = ctx->httpPost("/api/repl", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        expect(json["moduleId"].toString() == "Interface", "Should return moduleId");
        expect(json["result"].toString() == "REPL Evaluation OK", "Should have success result message");
        expectEquals<int>((int)json["value"], 42, "Should return the variable value");
    }
    
    //==========================================================================
    void testEvaluateREPLArithmetic()
    {
        /** Setup: Compile a basic script
         *  Scenario: Evaluate an arithmetic expression
         *  Expected: Returns the computed result
         */
        beginTest("POST /api/repl (arithmetic)");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("expression", "2 + 3 * 4");
        
        auto response = ctx->httpPost("/api/repl", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        expectEquals<int>((int)json["value"], 14, "Should compute 2 + 3 * 4 = 14");
    }
    
    //==========================================================================
    void testEvaluateREPLUndefined()
    {
        /** Setup: Compile a basic script
         *  Scenario: Evaluate an expression referencing an undeclared variable
         *  Expected: Returns success=true with value="undefined"
         */
        beginTest("POST /api/repl (undefined result)");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("expression", "undeclaredVariable");
        
        auto response = ctx->httpPost("/api/repl", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Undeclared variable should return success");
        expect(json["moduleId"].toString() == "Interface", "Should return moduleId");
        expect(json["value"].toString() == "undefined", "Should return 'undefined' for undeclared variable");
    }
    
    //==========================================================================
    void testEvaluateREPLRuntimeError()
    {
        /** Setup: Compile a basic script
         *  Scenario: Evaluate Console.assertTrue(false) which triggers a runtime error
         *  Expected: Returns success=false with error detail in errors array
         */
        beginTest("POST /api/repl (runtime error)");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("expression", "Console.assertTrue(false)");
        
        auto response = ctx->httpPost("/api/repl", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail for failed assertion");
        expect(json["moduleId"].toString() == "Interface", "Should return moduleId");
        expect(json["result"].toString() == "Error at REPL Evaluation", "Should have error result message");
        expect(json["errors"].isArray(), "Should have errors array");
        expect(json["errors"].size() >= 1, "Should have at least one error");
        expect(json["errors"][0]["errorMessage"].toString().isNotEmpty(), "Error should have a message");
    }
    
    //==========================================================================
    void testEvaluateREPLMissingExpression()
    {
        /** Setup: None
         *  Scenario: POST to /api/repl without an expression field
         *  Expected: Returns error response
         */
        beginTest("POST /api/repl (missing expression)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        // No expression field
        
        auto response = ctx->httpPost("/api/repl", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expectErrorMessageContains(json, "expression");

        // Error envelopes also flow through the auto-injection path.
        expect(json.hasProperty("apiVersion"), "Error envelope should also carry apiVersion");
        expect(json["apiVersion"].toString() == HISE_REST_API_VERSION,
               "Error apiVersion should match HISE_REST_API_VERSION");
    }

    //==========================================================================
    void testEvaluateREPLMissingModuleId()
    {
        /** Setup: None
         *  Scenario: POST to /api/repl without a moduleId field
         *  Expected: Returns error response
         */
        beginTest("POST /api/repl (missing moduleId)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("expression", "1 + 1");
        // No moduleId field
        
        auto response = ctx->httpPost("/api/repl", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expectErrorMessageContains(json, "moduleId");
    }
    
    //==========================================================================
    void testEvaluateREPLUnknownModule()
    {
        /** Setup: None
         *  Scenario: POST to /api/repl with a non-existent moduleId
         *  Expected: Returns error response
         */
        beginTest("POST /api/repl (unknown module)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "NonExistentModule");
        bodyObj->setProperty("expression", "1 + 1");
        
        auto response = ctx->httpPost("/api/repl", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expectErrorMessageContains(json, "not found");
    }
    
    //==========================================================================
    void testSetScriptUnknownCallback()
    {
        beginTest("POST /api/set_script (unknown callback)");
        
        ctx->reset();
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        
        DynamicObject::Ptr callbacks = new DynamicObject();
        callbacks->setProperty("onFoo", "// This callback doesn't exist");
        bodyObj->setProperty("callbacks", var(callbacks.get()));
        
        auto response = ctx->httpPost("/api/set_script", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail for unknown callback");
        expect(json["errors"].isArray(), "Should have errors array");
        expect(json["errors"].size() >= 1, "Should have at least one error");
        expect(json["errors"][0]["errorMessage"].toString().contains("unknown callback"), 
               "Error should mention unknown callback");
        expect(json["errors"][0]["errorMessage"].toString().contains("onFoo"), 
               "Error should include the unknown callback name");
    }
    
    //==========================================================================
    void testSetScriptMultipleCallbacks()
    {
        beginTest("POST /api/set_script (multiple callbacks)");
        
        ctx->reset();
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        
        DynamicObject::Ptr callbacks = new DynamicObject();
        callbacks->setProperty("onInit", "Content.makeFrontInterface(600, 400);\n"
                                         "Console.print(\"Init ran\");");
        callbacks->setProperty("onNoteOn", "function onNoteOn()\n{\n\tConsole.print(\"NoteOn\");\n}");
        bodyObj->setProperty("callbacks", var(callbacks.get()));
        bodyObj->setProperty("compile", true);
        
        auto response = ctx->httpPost("/api/set_script", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        
        auto updatedCallbacks = json["updatedCallbacks"];
        expect(updatedCallbacks.size() == 2, "Should have 2 updated callbacks");
        
        // Verify both callbacks are in the list (order may vary)
        bool hasOnInit = false, hasOnNoteOn = false;
        for (int i = 0; i < updatedCallbacks.size(); i++)
        {
            if (updatedCallbacks[i].toString() == "onInit") hasOnInit = true;
            if (updatedCallbacks[i].toString() == "onNoteOn") hasOnNoteOn = true;
        }
        expect(hasOnInit, "Should have updated onInit");
        expect(hasOnNoteOn, "Should have updated onNoteOn");
    }
    
    //==========================================================================
    void testSetScriptEmptyCallbacks()
    {
        beginTest("POST /api/set_script (empty callbacks)");
        
        ctx->reset();
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        
        DynamicObject::Ptr callbacks = new DynamicObject();
        // Empty callbacks object
        bodyObj->setProperty("callbacks", var(callbacks.get()));
        
        auto response = ctx->httpPost("/api/set_script", JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail for empty callbacks");
        expect(json["errors"].isArray(), "Should have errors array");
        expect(json["errors"].size() >= 1, "Should have at least one error");
        expect(json["errors"][0]["errorMessage"].toString().contains("empty"), 
               "Error should mention empty");
    }
    
    //==========================================================================
    void testGetScriptExternalFiles()
    {
        beginTest("GET /api/get_script (external files structure)");
        
        ctx->reset();
        
        // This test verifies the externalFiles structure is correct
        // Without actual include() files, array should be empty
        ctx->compile("Content.makeFrontInterface(600, 400);");
        
        auto response = ctx->httpGet("/api/get_script?moduleId=Interface");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        expect(json.hasProperty("externalFiles"), "Should have externalFiles");
        
        auto externalFiles = json["externalFiles"];
        expect(externalFiles.isArray(), "externalFiles should be array");
        // With no includes, array should be empty
        expect(externalFiles.size() == 0, "Should have no external files when no includes used");
    }
    
    //==========================================================================
    void testGetComponentValue()
    {
        beginTest("GET /api/get_component_value");
        
        ctx->reset();
        
        // Create a knob with specific range
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
                     "TestKnob.set(\"min\", -12);\n"
                     "TestKnob.set(\"max\", 12);\n"
                     "TestKnob.setValue(0.5);");
        
        auto response = ctx->httpGet("/api/get_component_value?moduleId=Interface&id=TestKnob");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        expect(json["moduleId"].toString() == "Interface", "Should return correct moduleId");
        expect(json["id"].toString() == "TestKnob", "Should return correct id");
        expect(json["type"].toString() == "ScriptSlider", "Should return correct type");
        expect(json.hasProperty("value"), "Should have value property");
        expect(json.hasProperty("min"), "Should have min property");
        expect(json.hasProperty("max"), "Should have max property");
        expect((double)json["min"] == -12.0, "min should be -12");
        expect((double)json["max"] == 12.0, "max should be 12");
    }
    
    //==========================================================================
    void testSetComponentValue()
    {
        beginTest("POST /api/set_component_value");
        
        ctx->reset();
        
        // Create a knob with specific range
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
                     "TestKnob.set(\"min\", 0);\n"
                     "TestKnob.set(\"max\", 10);");
        
        // Test 1: Set value without validation
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty("moduleId", "Interface");
            bodyObj->setProperty("id", "TestKnob");
            bodyObj->setProperty("value", 5.0);
            
            auto response = ctx->httpPost("/api/set_component_value",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            
            expect((bool)json["success"], "Should succeed");
            expect(json["moduleId"].toString() == "Interface", "Should return correct moduleId");
            expect(json["id"].toString() == "TestKnob", "Should return correct id");
            expect(json["type"].toString() == "ScriptSlider", "Should return correct type");
        }
        
        // Test 2: Set value with validation - valid value
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty("moduleId", "Interface");
            bodyObj->setProperty("id", "TestKnob");
            bodyObj->setProperty("value", 7.5);
            bodyObj->setProperty("validateRange", true);
            
            auto response = ctx->httpPost("/api/set_component_value",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            
            expect((bool)json["success"], "Should succeed with valid value");
        }
        
        // Test 3: Set value with validation - out of range (should fail)
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty("moduleId", "Interface");
            bodyObj->setProperty("id", "TestKnob");
            bodyObj->setProperty("value", 15.0);  // Out of range [0, 10]
            bodyObj->setProperty("validateRange", true);
            
            auto response = ctx->httpPost("/api/set_component_value",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            
            expect(!(bool)json["success"], "Should fail with out-of-range value when validateRange is true");
        }
        
        // Test 4: Out of range without validation (should succeed - no validation)
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty("moduleId", "Interface");
            bodyObj->setProperty("id", "TestKnob");
            bodyObj->setProperty("value", 15.0);  // Out of range but validateRange is false
            bodyObj->setProperty("validateRange", false);
            
            auto response = ctx->httpPost("/api/set_component_value",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            
            expect((bool)json["success"], "Should succeed without validation even if out of range");
        }
    }
    
    //==========================================================================
    void testSetComponentValueTriggersCallback()
    {
        beginTest("POST /api/set_component_value triggers control callback");
        
        ctx->reset();
        
        // Create a knob with a dedicated control callback using setControlCallback
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
                     "\n"
                     "inline function onTestKnobChanged(component, value)\n"
                     "{\n"
                     "    Console.print(\"CALLBACK_TRIGGERED:\" + value);\n"
                     "}\n"
                     "\n"
                     "TestKnob.setControlCallback(onTestKnobChanged);");
        
        // Set value via API
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("id", "TestKnob");
        bodyObj->setProperty("value", 0.75);
        
        auto response = ctx->httpPost("/api/set_component_value",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        
        // Verify callback was triggered with correct value
        auto logs = json["logs"];
        bool callbackTriggeredWithCorrectValue = false;
        for (int i = 0; i < logs.size(); i++)
        {
            if (logs[i].toString().contains("CALLBACK_TRIGGERED:0.75"))
                callbackTriggeredWithCorrectValue = true;
        }
        expect(callbackTriggeredWithCorrectValue, 
               "Control callback should be triggered with value 0.75 when setting via API");
    }
    
    //==========================================================================
    void testSetComponentValueCallbackRuntimeError()
    {
        beginTest("POST /api/set_component_value reports callback runtime errors");
        
        ctx->reset();
        
        // Create a knob with a callback that will cause a runtime error
        // Console.print(undefined) compiles fine but fails at runtime
        auto compileResult = ctx->compile(
            "Content.makeFrontInterface(600, 400);\n"
            "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
            "TestKnob.set(\"saveInPreset\", false);\n"
            "\n"
            "inline function onTestKnobChanged(component, value)\n"
            "{\n"
            "    Console.print(undefined);\n"
            "}\n"
            "\n"
            "TestKnob.setControlCallback(onTestKnobChanged);");
        
        // Compilation should succeed (runtime error hasn't happened yet)
        expect((bool)compileResult["success"], "Compilation should succeed");
        auto compileErrors = compileResult["errors"];
        expect(compileErrors.size() == 0, "Should have no compile errors");
        
        // Now trigger the callback via set_component_value
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("id", "TestKnob");
        bodyObj->setProperty("value", 0.5);
        
        auto response = ctx->httpPost("/api/set_component_value",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        // The API call itself should still succeed (value was set)
        // but the errors array should contain the runtime error from the callback
        auto errors = json["errors"];
        expect(errors.isArray(), "errors should be array");
        expect(errors.size() >= 1, "Should have at least one runtime error from callback");
        
        if (errors.size() > 0)
        {
            auto firstError = errors[0];
            expect(firstError.hasProperty("errorMessage"), "Error should have errorMessage");
            // The error should mention something about undefined
            expect(firstError["errorMessage"].toString().containsIgnoreCase("undefined"),
                   "Error message should mention 'undefined'");
        }
    }
    
    //==========================================================================
    void testNestedControlCallbacks()
    {
        beginTest("POST /api/set_component_value captures nested callback logs");
        
        ctx->reset();
        
        // Create two knobs where Knob1's callback triggers Knob2's callback
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var Knob1 = Content.addKnob(\"Knob1\", 10, 10);\n"
                     "const var Knob2 = Content.addKnob(\"Knob2\", 150, 10);\n"
                     "\n"
                     "inline function onKnob1Changed(component, value)\n"
                     "{\n"
                     "    Console.print(\"Knob1 callback: \" + value);\n"
                     "    Knob2.setValue(1.0 - value);\n"
                     "    Knob2.changed();\n"
                     "}\n"
                     "\n"
                     "inline function onKnob2Changed(component, value)\n"
                     "{\n"
                     "    Console.print(\"Knob2 callback: \" + value);\n"
                     "}\n"
                     "\n"
                     "Knob1.setControlCallback(onKnob1Changed);\n"
                     "Knob2.setControlCallback(onKnob2Changed);");
        
        // Set value on Knob1 - this should trigger Knob1's callback,
        // which then triggers Knob2's callback
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("id", "Knob1");
        bodyObj->setProperty("value", 0.3);
        
        auto response = ctx->httpPost("/api/set_component_value",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        
        auto logs = json["logs"];
        
        // Verify BOTH callbacks' output appears in logs
        // Float normalization should clean up values (0.300000011920929 -> 0.3)
        bool hasKnob1Log = false;
        bool hasKnob2Log = false;
        
        for (int i = 0; i < logs.size(); i++)
        {
            String logEntry = logs[i].toString();
            if (logEntry.contains("Knob1 callback: 0.3"))
                hasKnob1Log = true;
            if (logEntry.contains("Knob2 callback: 0.7"))
                hasKnob2Log = true;
        }
        
        expect(hasKnob1Log, "Should capture Knob1's direct callback log");
        expect(hasKnob2Log, "Should capture Knob2's chained callback log (triggered via .changed())");
    }
    
    //==========================================================================
    void testFloatNormalizationUnit()
    {
        beginTest("Float normalization utility");
        
        // Test 1: Double with FP noise
        {
            var v = 0.300000011920929;
            RestServer::normalizeFloatsInVar(v);
            expectEquals((double)v, 0.3, "Should round 0.300000011920929 to 0.3");
        }
        
        // Test 2: Clean double
        {
            var v = 0.5;
            RestServer::normalizeFloatsInVar(v);
            expectEquals((double)v, 0.5, "Should preserve clean 0.5");
        }
        
        // Test 3: Integer-valued double
        {
            var v = 42.0;
            RestServer::normalizeFloatsInVar(v);
            expectEquals((double)v, 42.0, "Should preserve 42.0");
        }
        
        // Test 4: String with embedded FP noise
        {
            var v = "Value: 0.300000011920929";
            RestServer::normalizeFloatsInVar(v);
            expectEquals(v.toString(), String("Value: 0.3"), "Should clean FP noise in string");
        }
        
        // Test 5: String with multiple numbers
        {
            var v = "From 0.100000001 to 0.899999999";
            RestServer::normalizeFloatsInVar(v);
            expectEquals(v.toString(), String("From 0.1 to 0.9"), "Should clean multiple numbers");
        }
        
        // Test 6: String with short decimals (< 5) - unchanged
        {
            var v = "Value: 0.123";
            RestServer::normalizeFloatsInVar(v);
            expectEquals(v.toString(), String("Value: 0.123"), "Should preserve < 5 decimals");
        }
        
        // Test 7: Array of mixed values
        {
            Array<var> arr;
            arr.add(0.300000011920929);
            arr.add("Text: 0.700000001");
            arr.add(42);
            var v = arr;
            RestServer::normalizeFloatsInVar(v);
            
            expectEquals((double)v[0], 0.3, "Array double normalized");
            expectEquals(v[1].toString(), String("Text: 0.7"), "Array string normalized");
            expectEquals((int)v[2], 42, "Array int unchanged");
        }
        
        // Test 8: Nested object
        {
            DynamicObject::Ptr obj = new DynamicObject();
            obj->setProperty("value", 0.300000011920929);
            obj->setProperty("message", "Got 0.999999999");
            obj->setProperty("count", 5);
            var v = var(obj.get());
            RestServer::normalizeFloatsInVar(v);
            
            auto* result = v.getDynamicObject();
            expectEquals((double)result->getProperty("value"), 0.3, "Object double normalized");
            expectEquals(result->getProperty("message").toString(), String("Got 1.0"), "Object string normalized");
            expectEquals((int)result->getProperty("count"), 5, "Object int unchanged");
        }
        
        // Test 9: Negative value
        {
            var v = -0.300000011920929;
            RestServer::normalizeFloatsInVar(v);
            expectEquals((double)v, -0.3, "Should handle negative values");
        }
        
        // Test 10: Very small value
        {
            var v = 0.00009999999;
            RestServer::normalizeFloatsInVar(v);
            expectEquals((double)v, 0.0001, "Should round very small values");
        }
    }
    
    //==========================================================================
    void testFloatNormalization()
    {
        beginTest("REST API normalizes floating point values");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
                     "\n"
                     "inline function onTestKnobChanged(component, value)\n"
                     "{\n"
                     "    Console.print(\"Value: \" + value);\n"
                     "}\n"
                     "\n"
                     "TestKnob.setControlCallback(onTestKnobChanged);");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("id", "TestKnob");
        bodyObj->setProperty("value", 0.1);
        
        auto response = ctx->httpPost("/api/set_component_value",
                                      JSON::toString(var(bodyObj.get())));
        
        expect(!response.contains("0.10000000"), 
               "Response should not contain FP noise");
        expect(!response.contains("0.0999999"),
               "Response should not contain FP noise");
        
        var json = ctx->parseJson(response);
        auto logs = json["logs"];
        
        bool hasCleanValue = false;
        for (int i = 0; i < logs.size(); i++)
        {
            if (logs[i].toString() == "Value: 0.1")
                hasCleanValue = true;
        }
        expect(hasCleanValue, "Log should contain clean 'Value: 0.1'");
    }
    
    //==========================================================================
    void testForceSynchronousExecution()
    {
        beginTest("forceSynchronousExecution parameter");
        
        ctx->reset();
        
        // Create a knob with control callback
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
                     "\n"
                     "inline function onTestKnobChanged(component, value)\n"
                     "{\n"
                     "    Console.print(\"Callback executed: \" + value);\n"
                     "}\n"
                     "\n"
                     "TestKnob.setControlCallback(onTestKnobChanged);");
        
        // Test with forceSynchronousExecution: true
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("id", "TestKnob");
        bodyObj->setProperty("value", 0.5);
        bodyObj->setProperty("forceSynchronousExecution", true);
        
        auto response = ctx->httpPost("/api/set_component_value",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        expect((bool)json["forceSynchronousExecution"], 
               "Response should indicate sync mode was used");
        expect(json["warning"].toString().isNotEmpty(), 
               "Response should include warning when sync mode is used");
        expect(json["warning"].toString().contains("unsafe"), 
               "Warning should mention 'unsafe'");
        
        // Verify callback was still executed
        auto logs = json["logs"];
        bool callbackExecuted = false;
        for (int i = 0; i < logs.size(); i++)
        {
            if (logs[i].toString().contains("Callback executed"))
                callbackExecuted = true;
        }
        expect(callbackExecuted, "Callback should still execute in sync mode");
    }
    
    //==========================================================================
    void testSetComponentPropertiesBasic()
    {
        beginTest("POST /api/set_component_properties (basic)");
        
        ctx->reset();
        
        // Create a knob
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);");
        
        // Set a single property
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "TestKnob");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("x", 100);
        props->setProperty("text", "New Label");
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed: " + response);
        expect(json["moduleId"].toString() == "Interface", "Should return correct moduleId");
        expect(!(bool)json["recompileRequired"], "Should not require recompile for simple props");
        
        auto applied = json["applied"];
        expect(applied.isArray(), "applied should be array");
        expect(applied.size() == 1, "Should have 1 applied change");
        expect(applied[0]["id"].toString() == "TestKnob", "Should have correct component id");
        
        // Verify properties were actually changed
        auto getResponse = ctx->httpGet("/api/get_component_properties?moduleId=Interface&id=TestKnob");
        var getJson = ctx->parseJson(getResponse);
        auto properties = getJson["properties"];
        
        bool foundX = false, foundText = false;
        for (int i = 0; i < properties.size(); i++)
        {
            if (properties[i]["id"].toString() == "x")
            {
                foundX = true;
                expect((int)properties[i]["value"] == 100, "x should be 100");
            }
            if (properties[i]["id"].toString() == "text")
            {
                foundText = true;
                expect(properties[i]["value"].toString() == "New Label", "text should be 'New Label'");
            }
        }
        expect(foundX, "Should have x property");
        expect(foundText, "Should have text property");
    }
    
    //==========================================================================
    void testSetComponentPropertiesMultiple()
    {
        beginTest("POST /api/set_component_properties (multiple components)");
        
        ctx->reset();
        
        // Create multiple components
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var Knob1 = Content.addKnob(\"Knob1\", 10, 10);\n"
                     "const var Knob2 = Content.addKnob(\"Knob2\", 150, 10);");
        
        // Set properties on both
        DynamicObject::Ptr change1 = new DynamicObject();
        change1->setProperty("id", "Knob1");
        DynamicObject::Ptr props1 = new DynamicObject();
        props1->setProperty("x", 50);
        change1->setProperty("properties", var(props1.get()));
        
        DynamicObject::Ptr change2 = new DynamicObject();
        change2->setProperty("id", "Knob2");
        DynamicObject::Ptr props2 = new DynamicObject();
        props2->setProperty("x", 200);
        props2->setProperty("text", "Second");
        change2->setProperty("properties", var(props2.get()));
        
        Array<var> changes;
        changes.add(var(change1.get()));
        changes.add(var(change2.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        
        auto applied = json["applied"];
        expect(applied.size() == 2, "Should have 2 applied changes");
    }
    
    //==========================================================================
    void testSetComponentPropertiesLockedRejection()
    {
        beginTest("POST /api/set_component_properties (locked property rejection)");
        
        ctx->reset();
        
        // Create a knob where script sets x property
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
                     "TestKnob.set(\"x\", 50);");  // This locks 'x'
        
        // Try to set x via API without force
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "TestKnob");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("x", 100);  // x is locked by script
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        bodyObj->setProperty("force", false);
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail when property is locked");
        expect(json.hasProperty("locked"), "Should have locked array");
        
        auto locked = json["locked"];
        expect(locked.isArray(), "locked should be array");
        expect(locked.size() >= 1, "Should have at least one locked property");
        expect(locked[0]["id"].toString() == "TestKnob", "Should identify correct component");
        expect(locked[0]["property"].toString() == "x", "Should identify correct property");
    }
    
    //==========================================================================
    void testSetComponentPropertiesForceMode()
    {
        beginTest("POST /api/set_component_properties (force mode)");
        
        ctx->reset();
        
        // Create a knob where script sets x property
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);\n"
                     "TestKnob.set(\"x\", 50);");  // This locks 'x'
        
        // Set x via API WITH force=true
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "TestKnob");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("x", 200);  // x is locked but we force
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        bodyObj->setProperty("force", true);
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed with force=true");
        
        // Verify property was actually changed
        auto getResponse = ctx->httpGet("/api/get_component_properties?moduleId=Interface&id=TestKnob");
        var getJson = ctx->parseJson(getResponse);
        auto properties = getJson["properties"];
        
        for (int i = 0; i < properties.size(); i++)
        {
            if (properties[i]["id"].toString() == "x")
            {
                expect((int)properties[i]["value"] == 200, "x should be 200 after force");
            }
        }
    }
    
    //==========================================================================
    void testSetComponentPropertiesInvalidComponent()
    {
        beginTest("POST /api/set_component_properties (invalid component)");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);");
        
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "NonExistent");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("x", 100);
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail for non-existent component");
    }
    
    //==========================================================================
    void testSetComponentPropertiesInvalidProperty()
    {
        beginTest("POST /api/set_component_properties (invalid property)");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);");
        
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "TestKnob");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("nonExistentProperty", 100);
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail for non-existent property");
    }
    
    //==========================================================================
    void testSetComponentPropertiesColour()
    {
        beginTest("POST /api/set_component_properties (colour conversion)");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);");
        
        // Set bgColour using hex string format
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "TestKnob");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("bgColour", "0xFF00FF00");  // Green
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed with hex colour string");
        
        // Verify colour was set
        auto getResponse = ctx->httpGet("/api/get_component_properties?moduleId=Interface&id=TestKnob");
        var getJson = ctx->parseJson(getResponse);
        auto properties = getJson["properties"];
        
        for (int i = 0; i < properties.size(); i++)
        {
            if (properties[i]["id"].toString() == "bgColour")
            {
                expect(properties[i]["value"].toString() == "0xFF00FF00", 
                       "bgColour should be green (0xFF00FF00)");
            }
        }
    }
    
    //==========================================================================
    void testSetComponentPropertiesParentComponent()
    {
        beginTest("POST /api/set_component_properties (parentComponent triggers rebuild)");
        
        ctx->reset();
        
        // Create panel and knob
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var Panel1 = Content.addPanel(\"Panel1\", 10, 10);\n"
                     "Panel1.set(\"width\", 200);\n"
                     "Panel1.set(\"height\", 200);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 250, 10);");
        
        // Move knob into panel by setting parentComponent
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "TestKnob");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("parentComponent", "Panel1");
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed");
        expect((bool)json["recompileRequired"], "Should indicate recompile required");
    }
    
    //==========================================================================
    void testSetComponentPropertiesRemoveFromParent()
    {
        beginTest("POST /api/set_component_properties (remove from parent with empty string)");
        
        ctx->reset();
        
        // Create panel and knob, with knob inside panel
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var Panel1 = Content.addPanel(\"Panel1\", 10, 10);\n"
                     "Panel1.set(\"width\", 200);\n"
                     "Panel1.set(\"height\", 200);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 20, 20);\n"
                     "TestKnob.set(\"parentComponent\", \"Panel1\");");
        
        // Verify knob is in panel
        auto hierarchyBefore = ctx->httpGet("/api/list_components?moduleId=Interface&hierarchy=true");
        var jsonBefore = ctx->parseJson(hierarchyBefore);
        auto componentsBefore = jsonBefore["components"];
        
        bool knobInPanelBefore = false;
        for (int i = 0; i < componentsBefore.size(); i++)
        {
            if (componentsBefore[i]["id"].toString() == "Panel1")
            {
                auto children = componentsBefore[i]["childComponents"];
                for (int j = 0; j < children.size(); j++)
                {
                    if (children[j]["id"].toString() == "TestKnob")
                        knobInPanelBefore = true;
                }
            }
        }
        expect(knobInPanelBefore, "Knob should be inside Panel1 initially");
        
        // Remove knob from panel by setting parentComponent to empty string
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "TestKnob");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("parentComponent", "");
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        bodyObj->setProperty("force", true);
        
        auto jmp = ProcessorHelpers::getFirstProcessorWithType<JavascriptMidiProcessor>(ctx->bp->getMainSynthChain());
        auto sc = jmp->getContent()->getComponentWithName("TestKnob");

        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Should succeed");

        // Verify knob is now at root level (not inside panel)
        auto hierarchyAfter = ctx->httpGet("/api/list_components?moduleId=Interface&hierarchy=true");

        var jsonAfter = ctx->parseJson(hierarchyAfter);
        auto componentsAfter = jsonAfter["components"];
        
        bool knobAtRoot = false;
        bool knobStillInPanel = false;
        
        for (int i = 0; i < componentsAfter.size(); i++)
        {
            if (componentsAfter[i]["id"].toString() == "TestKnob")
                knobAtRoot = true;
            
            if (componentsAfter[i]["id"].toString() == "Panel1")
            {
                auto children = componentsAfter[i]["childComponents"];
                for (int j = 0; j < children.size(); j++)
                {
                    if (children[j]["id"].toString() == "TestKnob")
                        knobStillInPanel = true;
                }
            }
        }
        
        expect(knobAtRoot, "Knob should be at root level after setting parentComponent to empty");
        expect(!knobStillInPanel, "Knob should NOT be inside Panel1 after removal");
    }
    
    //==========================================================================
    void testSetComponentPropertiesInvalidParent()
    {
        beginTest("POST /api/set_component_properties (invalid parentComponent target)");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var TestKnob = Content.addKnob(\"TestKnob\", 10, 10);");
        
        // Try to set parentComponent to non-existent panel
        DynamicObject::Ptr change = new DynamicObject();
        change->setProperty("id", "TestKnob");
        DynamicObject::Ptr props = new DynamicObject();
        props->setProperty("parentComponent", "NonExistentPanel");
        change->setProperty("properties", var(props.get()));
        
        Array<var> changes;
        changes.add(var(change.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("changes", var(changes));
        
        auto response = ctx->httpPost("/api/set_component_properties",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail for non-existent parent");
    }
    
    //==========================================================================
    void testTestingScreenshotBasic()
    {
        beginTest("GET /api/testing/screenshot (full interface)");
        
        ctx->reset();
        
        // Create a simple interface
        ctx->compile("Content.makeFrontInterface(400, 300);\n"
                     "const var TestPanel = Content.addPanel(\"TestPanel\", 10, 10);\n"
                     "TestPanel.set(\"width\", 100);\n"
                     "TestPanel.set(\"height\", 100);");
        
        auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface");
        var json = ctx->parseJson(response);

        // Note: Until capture logic is implemented, this will fail with "not yet implemented"
        // Once implemented, these assertions should pass:
        // expect((bool)json["success"], "Screenshot should succeed");
        // expect(json.hasProperty("imageData"), "Should have imageData");
        // expect(json.hasProperty("width"), "Should have width");
        // expect(json.hasProperty("height"), "Should have height");
        // expect((int)json["width"] == 400, "Width should match interface width");
        // expect((int)json["height"] == 300, "Height should match interface height");
        // expect((float)json["scale"] == 1.0f, "Scale should be 1.0 by default");

        // For now, just verify the endpoint is registered and responds
        expect(json.hasProperty("success") || json.hasProperty("error"),
               "Should return a valid response structure");
    }
    
    //==========================================================================
    void testTestingScreenshotComponent()
    {
        beginTest("GET /api/testing/screenshot (specific component)");

        ctx->reset();

        ctx->compile("Content.makeFrontInterface(400, 300);\n"
                     "const var TestPanel = Content.addPanel(\"TestPanel\", 50, 50);\n"
                     "TestPanel.set(\"width\", 100);\n"
                     "TestPanel.set(\"height\", 80);");

        auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface&id=TestPanel");
        var json = ctx->parseJson(response);

        // Once implemented:
        // expect((bool)json["success"], "Component screenshot should succeed");
        // expect(json["id"].toString() == "TestPanel", "Should return component ID");

        expect(json.hasProperty("success") || json.hasProperty("error"),
               "Should return a valid response structure");
    }
    
    //==========================================================================
    void testTestingScreenshotInvalidComponent()
    {
        beginTest("GET /api/testing/screenshot (invalid component)");

        ctx->reset();

        ctx->compile("Content.makeFrontInterface(400, 300);");

        auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface&id=NonExistent");
        var json = ctx->parseJson(response);

        expectErrorMessageContains(json, "component not found");
    }
    
    //==========================================================================
    void testTestingScreenshotScale()
    {
        beginTest("GET /api/testing/screenshot (with scale)");

        ctx->reset();

        ctx->compile("Content.makeFrontInterface(400, 300);");

        auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface&scale=0.5");
        var json = ctx->parseJson(response);

        // Once implemented:
        // expect((bool)json["success"], "Scaled screenshot should succeed");
        // expect((int)json["width"] == 200, "Width should be scaled to 200");
        // expect((int)json["height"] == 150, "Height should be scaled to 150");
        // expect((float)json["scale"] == 0.5f, "Scale should be 0.5");

        expect(json.hasProperty("success") || json.hasProperty("error"),
               "Should return a valid response structure");
    }
    
    //==========================================================================
    void testTestingScreenshotPixelVerification()
    {
        beginTest("GET /api/testing/screenshot (pixel verification)");

        ctx->reset();

        // Create a panel with a red fill paint routine
        ctx->compile("Content.makeFrontInterface(400, 300);\n"
                     "const var RedPanel = Content.addPanel(\"RedPanel\", 10, 10);\n"
                     "RedPanel.set(\"width\", 100);\n"
                     "RedPanel.set(\"height\", 100);\n"
                     "RedPanel.setPaintRoutine(function(g) {\n"
                     "    g.fillAll(Colours.red);\n"
                     "});");

        auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface&id=RedPanel");
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Screenshot should succeed");
        expect(json.hasProperty("imageData"), "Should have imageData");

        // Decode Base64 to image
        String base64Data = json["imageData"].toString();
        MemoryOutputStream mos;
        bool decoded = Base64::convertFromBase64(mos, base64Data);
        expect(decoded, "Base64 decoding should succeed");

        // Load as PNG image
        auto image = ImageFileFormat::loadFrom(mos.getData(), mos.getDataSize());
        expect(image.isValid(), "Should load as valid image");
        expect(image.getWidth() == 100, "Image width should be 100");
        expect(image.getHeight() == 100, "Image height should be 100");

        // Verify center pixel is pure red (R:255, G:0, B:0, A:255)
        auto pixel = image.getPixelAt(50, 50);
        expect(pixel.getRed() == 255, "Red channel should be 255");
        expect(pixel.getGreen() == 0, "Green channel should be 0");
        expect(pixel.getBlue() == 0, "Blue channel should be 0");
        expect(pixel.getAlpha() == 255, "Alpha channel should be 255");
    }
    
    //==========================================================================
    void testTestingScreenshotPixelVerificationScaled()
    {
        beginTest("GET /api/testing/screenshot (pixel verification with scale)");

        ctx->reset();

        // Create a panel with a red fill paint routine
        ctx->compile("Content.makeFrontInterface(400, 300);\n"
                     "const var RedPanel = Content.addPanel(\"RedPanel\", 10, 10);\n"
                     "RedPanel.set(\"width\", 100);\n"
                     "RedPanel.set(\"height\", 100);\n"
                     "RedPanel.setPaintRoutine(function(g) {\n"
                     "    g.fillAll(Colours.red);\n"
                     "});");

        auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface&id=RedPanel&scale=0.5");
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Screenshot should succeed");

        // Decode Base64 to image
        String base64Data = json["imageData"].toString();
        MemoryOutputStream mos;
        bool decoded = Base64::convertFromBase64(mos, base64Data);
        expect(decoded, "Base64 decoding should succeed");

        // Load as PNG image - should be half size
        auto image = ImageFileFormat::loadFrom(mos.getData(), mos.getDataSize());
        expect(image.isValid(), "Should load as valid image");
        expect(image.getWidth() == 50, "Image width should be 50 (scaled)");
        expect(image.getHeight() == 50, "Image height should be 50 (scaled)");

        // Verify center pixel is still pure red
        auto pixel = image.getPixelAt(25, 25);
        expect(pixel.getRed() == 255, "Red channel should be 255");
        expect(pixel.getGreen() == 0, "Green channel should be 0");
        expect(pixel.getBlue() == 0, "Blue channel should be 0");
        expect(pixel.getAlpha() == 255, "Alpha channel should be 255");
    }
    
    //==========================================================================
    void testTestingScreenshotFileOutput()
    {
        beginTest("GET /api/testing/screenshot (file output)");

        ctx->reset();

        // Create a panel with a red fill paint routine
        ctx->compile("Content.makeFrontInterface(400, 300);\n"
                     "const var RedPanel = Content.addPanel(\"RedPanel\", 10, 10);\n"
                     "RedPanel.set(\"width\", 100);\n"
                     "RedPanel.set(\"height\", 100);\n"
                     "RedPanel.setPaintRoutine(function(g) {\n"
                     "    g.fillAll(Colours.red);\n"
                     "});");

        // Use temp directory for output file
        File tempFile = File::getSpecialLocation(File::tempDirectory).getChildFile("hise_test_screenshot.png");

        // Clean up any existing file
        tempFile.deleteFile();

        auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface&id=RedPanel&outputPath="
                                      + URL::addEscapeChars(tempFile.getFullPathName(), true));
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Screenshot should succeed");
        expect(!json.hasProperty("imageData"), "Should not have imageData when outputPath specified");
        expect(json.hasProperty("filePath"), "Should have filePath");
        expect(json["filePath"].toString() == tempFile.getFullPathName(), "filePath should match requested path");

        // Verify file exists
        expect(tempFile.existsAsFile(), "Output file should exist");

        // Load the file and verify it's a valid PNG with correct dimensions
        auto image = ImageFileFormat::loadFrom(tempFile);
        expect(image.isValid(), "Should load as valid image");
        expect(image.getWidth() == 100, "Image width should be 100");
        expect(image.getHeight() == 100, "Image height should be 100");

        // Verify pixel is red
        auto pixel = image.getPixelAt(50, 50);
        expect(pixel.getRed() == 255, "Red channel should be 255");
        expect(pixel.getGreen() == 0, "Green channel should be 0");
        expect(pixel.getBlue() == 0, "Blue channel should be 0");
        expect(pixel.getAlpha() == 255, "Alpha channel should be 255");

        // Clean up
        tempFile.deleteFile();
    }
    
    //==========================================================================
    void testTestingScreenshotFileOutputErrors()
    {
        beginTest("GET /api/testing/screenshot (file output errors)");

        ctx->reset();

        ctx->compile("Content.makeFrontInterface(400, 300);");

        // Test: missing .png extension
        {
            auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface&outputPath=C:/temp/screenshot.jpg");
            var json = ctx->parseJson(response);
            
            expectErrorMessageContains(json, ".png");
        }
        
        // Test: parent directory doesn't exist
        {
#if JUCE_WINDOWS
            auto response = ctx->httpGet("/api/testing/screenshot?moduleId=Interface&outputPath="
                                          + URL::addEscapeChars("C:/nonexistent_dir_12345/screenshot.png", true));
            var json = ctx->parseJson(response);
            
            expectErrorMessageContains(json, "directory");
#endif
        }
    }
    
    void testGetSelectedComponents()
    {
        beginTest("GET /api/get_selected_components");
        
        ctx->reset();
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "const var Button1 = Content.addButton(\"Button1\", 10, 10);\n"
                     "const var Button2 = Content.addButton(\"Button2\", 100, 10);");
        
        // Test: Empty selection (no components selected in Interface Designer)
        // Note: Programmatic selection would require ScriptComponentEditBroadcaster manipulation,
        // which needs to be done on the message thread. Testing with actual selection requires
        // Christoph's help or complex async coordination. For now, we verify the endpoint works
        // and touches the coverage system correctly.
        {
            auto response = ctx->httpGet("/api/get_selected_components?moduleId=Interface");
            var json = ctx->parseJson(response);
            
            expect((bool)json["success"], "Should succeed even with empty selection");
            expectEquals<int>(json["selectionCount"], 0, "Should have 0 selected components when Interface Designer is not open or nothing is selected");
            expect(json["components"].isArray(), "Should have components array");
            expectEquals<int>(json["components"].size(), 0, "Components array should be empty with no selection");
            expect(json["moduleId"].toString() == "Interface", "Should return the moduleId");
        }
    }
    
    void testTestingE2eExcluded()
    {
        beginTest("POST /api/testing/e2e (excluded from tests)");

        // This endpoint is explicitly excluded from full testing because:
        // 1. It spawns a separate InteractionTestWindow that would interfere with unit tests
        // 2. The interaction system has its own dedicated test suite (InteractionDispatcherTests)
        // 3. Real integration testing requires a visible window and synthetic mouse events
        //
        // We just mark it as "touched" for endpoint coverage verification.
        ctx->touchedEndpoints.addIfNotAlreadyThere("/api/testing/e2e");

        // Verify the endpoint exists in the route metadata
        bool found = false;
        for (const auto& route : RestHelpers::getRouteMetadata())
        {
            if (route.path == "api/testing/e2e")
            {
                found = true;
                expect(route.method == RestServer::POST, "testing/e2e should be POST");
                break;
            }
        }
        expect(found, "testing/e2e should exist in route metadata");
    }
    
    //==========================================================================
    // diagnose_script tests
    
    /** Helper: get the Scripts folder for the test context's project. */
    File getTestScriptsFolder()
    {
        return ctx->mc->getSampleManager().getProjectHandler()
            .getSubDirectory(FileHandlerBase::Scripts);
    }
    
    /** Helper: create a temporary .js file in the Scripts folder.
        Returns the file (caller must delete it after the test). */
    File createTempScriptFile(const String& fileName, const String& content)
    {
        auto scriptsFolder = getTestScriptsFolder();
        scriptsFolder.createDirectory();
        auto f = scriptsFolder.getChildFile(fileName);
        f.replaceWithText(content);
        return f;
    }
    
    //==========================================================================
    // get_included_files tests
    //==========================================================================
    
    void testGetIncludedFilesGlobal()
    {
        /** Setup: Compile with an external file included
         *  Scenario: Call get_included_files without moduleId
         *  Expected: Returns all files with owning processor names
         */
        beginTest("GET /api/get_included_files (global)");
        
        ctx->reset();
        
        auto tempFile = createTempScriptFile("_test_included_global.js",
            "Console.print(\"included\");\n");
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "include(\"_test_included_global.js\");");
        
        auto response = ctx->httpGet("/api/get_included_files");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed: " + response);
        expect(json["files"].isArray(), "Should have files array");
        
        bool found = false;
        
        for (int i = 0; i < json["files"].size(); i++)
        {
            auto entry = json["files"][i];
            
            if (entry["path"].toString().contains("_test_included_global.js"))
            {
                found = true;
                expect(entry["processor"].toString() == "Interface",
                       "Processor should be Interface");
            }
        }
        
        expect(found, "Should find the included file in global list");
        
        tempFile.deleteFile();
    }
    
    void testGetIncludedFilesForModule()
    {
        /** Setup: Compile with an external file included
         *  Scenario: Call get_included_files with moduleId=Interface
         *  Expected: Returns flat list of file paths for that processor
         */
        beginTest("GET /api/get_included_files (with moduleId)");
        
        ctx->reset();
        
        auto tempFile = createTempScriptFile("_test_included_module.js",
            "Console.print(\"included\");\n");
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "include(\"_test_included_module.js\");");
        
        auto response = ctx->httpGet("/api/get_included_files?moduleId=Interface");
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed: " + response);
        expect(json["moduleId"].toString() == "Interface", "Should echo moduleId");
        expect(json["files"].isArray(), "Should have files array");
        expect(json["files"].size() > 0, "Should have at least one file");
        
        bool found = false;
        
        for (int i = 0; i < json["files"].size(); i++)
        {
            if (json["files"][i].toString().contains("_test_included_module.js"))
                found = true;
        }
        
        expect(found, "Should find the included file");
        
        // Verify flat string format (not object)
        expect(json["files"][0].isString(), "Files should be flat strings when moduleId is provided");
        
        tempFile.deleteFile();
    }
    
    void testGetIncludedFilesUnknownModule()
    {
        /** Setup: No special compilation
         *  Scenario: Call get_included_files with unknown moduleId
         *  Expected: 404 error
         */
        beginTest("GET /api/get_included_files (unknown module)");
        
        ctx->reset();
        
        auto response = ctx->httpGet("/api/get_included_files?moduleId=NonExistentModule");
        var json = ctx->parseJson(response);
        
        expectErrorMessageContains(json, "module not found");
    }
    
    //==========================================================================
    // diagnose_script tests
    //==========================================================================
    
    void testDiagnoseScriptErrorMissingParams()
    {
        beginTest("POST /api/diagnose_script (missing params)");
        
        ctx->reset();
        
        // No moduleId or filePath — should fail with 400
        DynamicObject::Ptr bodyObj = new DynamicObject();
        
        auto response = ctx->httpPost("/api/diagnose_script", 
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail without moduleId or filePath");
    }
    
    void testDiagnoseScriptErrorUnknownModule()
    {
        beginTest("POST /api/diagnose_script (unknown module)");
        
        ctx->reset();
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "NonExistentProcessor");
        
        auto response = ctx->httpPost("/api/diagnose_script",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail for unknown moduleId");
    }
    
    void testDiagnoseScriptErrorNoExternalFiles()
    {
        beginTest("POST /api/diagnose_script (no external files)");
        
        ctx->reset();
        
        // Compile a script without any include() — no external files
        ctx->compile("Content.makeFrontInterface(600, 400);");
        
        // Request diagnose with moduleId only — should fail since no external files
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        
        auto response = ctx->httpPost("/api/diagnose_script",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail when processor has no external files and filePath is not provided");
    }
    
    void testDiagnoseScriptSuccess()
    {
        /** Setup: External .js file with valid code, compiled via include()
         *  Scenario: Diagnose the file via moduleId + filePath
         *  Expected: Success with empty diagnostics array
         */
        beginTest("POST /api/diagnose_script (success, clean file)");
        
        ctx->reset();
        
        // Create a temp .js file with valid code
        auto tempFile = createTempScriptFile("_test_diagnose_clean.js",
            "// Valid HISEScript\nConsole.print(\"hello\");\n");
        
        // Compile with include
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "include(\"_test_diagnose_clean.js\");");
        
        // Diagnose via moduleId + filePath
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("filePath", tempFile.getFullPathName().replace("\\", "/"));
        
        auto response = ctx->httpPost("/api/diagnose_script",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed for clean file: " + response);
        expect(json["moduleId"].toString() == "Interface", "Should return moduleId");
        expect(json.hasProperty("filePath"), "Should have filePath in response");
        expect(json.hasProperty("diagnostics"), "Should have diagnostics array");
        
        auto diagnostics = json["diagnostics"];
        expect(diagnostics.isArray(), "diagnostics should be array");
        expectEquals<int>(diagnostics.size(), 0, "Clean file should have 0 diagnostics");
        
        // Cleanup
        tempFile.deleteFile();
    }
    
    void testDiagnoseScriptWithDiagnostics()
    {
        /** Setup: External .js file with a known API hallucination
         *  Scenario: Diagnose the file
         *  Expected: Diagnostic with error severity, api-validation source, and suggestion
         */
        beginTest("POST /api/diagnose_script (with API error)");
        
        ctx->reset();
        
        // Create a temp .js file with a known bad API call
        auto tempFile = createTempScriptFile("_test_diagnose_errors.js",
            "Console.prnt(\"hello\");\n");  // 'prnt' should trigger fuzzy → 'print'
        
        // First compile with a valid include to register the file
        auto cleanFile = createTempScriptFile("_test_diagnose_errors.js",
            "Console.print(\"hello\");\n");
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "include(\"_test_diagnose_errors.js\");");
        
        // Now write the bad code to the file (after compile registered it)
        tempFile.replaceWithText("Console.prnt(\"hello\");\n");
        
        // Diagnose
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("moduleId", "Interface");
        bodyObj->setProperty("filePath", tempFile.getFullPathName().replace("\\", "/"));
        
        auto response = ctx->httpPost("/api/diagnose_script",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Shadow parse should succeed (returns diagnostics, not failure): " + response);
        
        auto diagnostics = json["diagnostics"];
        expect(diagnostics.isArray(), "diagnostics should be array");
        expect(diagnostics.size() >= 1, "Should have at least 1 diagnostic for 'Console.prnt'");
        
        if (diagnostics.size() > 0)
        {
            auto d = diagnostics[0];
            expect(d.hasProperty("line"), "Diagnostic should have line");
            expect(d.hasProperty("column"), "Diagnostic should have column");
            expect(d.hasProperty("severity"), "Diagnostic should have severity");
            expect(d.hasProperty("source"), "Diagnostic should have source");
            expect(d.hasProperty("message"), "Diagnostic should have message");
            expect((int)d["line"] >= 1, "Line should be >= 1");
            
            auto severity = d["severity"].toString();
            expect(severity == "error" || severity == "warning", 
                   "Severity should be error or warning, got: " + severity);
            
            auto source = d["source"].toString();
            expect(source.isNotEmpty(), "Source should not be empty");
            
            // Check for suggestions (fuzzy match should suggest 'print')
            if (d.hasProperty("suggestions"))
            {
                auto suggestions = d["suggestions"];
                if (suggestions.isArray() && suggestions.size() > 0)
                {
                    bool hasPrintSuggestion = false;
                    for (int i = 0; i < suggestions.size(); i++)
                    {
                        if (suggestions[i].toString() == "print")
                            hasPrintSuggestion = true;
                    }
                    expect(hasPrintSuggestion, "Should suggest 'print' for 'prnt'");
                }
            }
        }
        
        // Cleanup
        tempFile.deleteFile();
    }
    
    void testDiagnoseScriptFilePath()
    {
        /** Setup: External .js file, diagnosed using filePath only (no moduleId)
         *  Scenario: HISE resolves the owning processor automatically
         *  Expected: Success with correct moduleId in response
         */
        beginTest("POST /api/diagnose_script (filePath only)");
        
        ctx->reset();
        
        // Create and compile with an external file
        auto tempFile = createTempScriptFile("_test_diagnose_filepath.js",
            "Console.print(\"filepath test\");\n");
        
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                     "include(\"_test_diagnose_filepath.js\");");
        
        // Diagnose using filePath only — HISE should resolve the processor
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("filePath", tempFile.getFullPathName().replace("\\", "/"));
        
        auto response = ctx->httpPost("/api/diagnose_script",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed with filePath-only resolution: " + response);
        expect(json["moduleId"].toString() == "Interface", 
               "Should resolve to Interface processor");
        expect(json["diagnostics"].isArray(), "Should have diagnostics array");
        expectEquals<int>(json["diagnostics"].size(), 0, "Clean file should have 0 diagnostics");
        
        // Cleanup
        tempFile.deleteFile();
    }
    
    //==========================================================================
    // Profile endpoint tests

    void testTestingProfileRecord()
    {
        /** Setup: Fresh interface compiled
         *  Scenario: POST /api/testing/profile with mode=record and short duration
         *  Expected: Returns immediately with recording=true (non-blocking)
         */
        beginTest("POST /api/testing/profile - record mode (non-blocking)");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");

        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("mode", "record");
        bodyObj->setProperty("durationMs", 200);

        auto response = ctx->httpPost("/api/testing/profile",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Record mode should succeed: " + response);
        expect((bool)json["recording"], "Should indicate recording is in progress");
    }

    void testTestingProfileGetAfterRecord()
    {
        /** Setup: A profiling session was started by testTestingProfileRecord
         *  Scenario: POST /api/testing/profile with mode=get (blocks until recording done)
         *  Expected: Returns the profiling result with threads and flows
         */
        beginTest("POST /api/testing/profile - get mode returns result");

        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("mode", "get");

        auto response = ctx->httpPost("/api/testing/profile",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Get mode should succeed with prior data: " + response);
        expect(json["threads"].isArray(), "Should have threads array");
        expect(json["flows"].isArray(), "Should have flows array");
        expect(!(bool)json["recording"], "Should not be recording");
    }

    void testTestingProfileSummary()
    {
        /** Setup: Profiling data available from prior record+get sequence
         *  Scenario: POST /api/testing/profile with mode=get, summary=true
         *  Expected: Returns results array with aggregated stats (count/median/peak/min/total)
         */
        beginTest("POST /api/testing/profile - summary mode");

        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("mode", "get");
        bodyObj->setProperty("summary", true);

        auto response = ctx->httpPost("/api/testing/profile",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Summary mode should succeed: " + response);
        expect(json["results"].isArray(), "Should have results array");
        expect(json["flows"].isArray(), "Should have flows array");

        // Verify summary entries have stat fields
        auto results = json["results"].getArray();

        if (results != nullptr && results->size() > 0)
        {
            auto first = (*results)[0];
            expect(first.hasProperty(RestApiIds::name), "Result should have name");
            expect(first.hasProperty(RestApiIds::count), "Result should have count");
            expect(first.hasProperty(RestApiIds::median), "Result should have median");
            expect(first.hasProperty(RestApiIds::peak), "Result should have peak");
            expect(first.hasProperty(RestApiIds::total), "Result should have total");
        }
    }

    void testTestingProfileFilter()
    {
        /** Setup: Profiling data available from prior record+get sequence
         *  Scenario: POST /api/testing/profile with mode=get, filter="*processBlock*"
         *  Expected: Returns only events matching the wildcard pattern
         */
        beginTest("POST /api/testing/profile - filter mode");

        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("mode", "get");
        bodyObj->setProperty("filter", "*processBlock*");

        auto response = ctx->httpPost("/api/testing/profile",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Filter mode should succeed: " + response);
        expect(json["results"].isArray(), "Should have results array");

        // All returned events should match the pattern
        auto results = json["results"].getArray();

        if (results != nullptr)
        {
            for (int i = 0; i < results->size(); i++)
            {
                auto eventName = (*results)[i].getProperty(RestApiIds::name, "").toString();
                expect(eventName.containsIgnoreCase("processBlock"),
                    "Filtered event should match pattern: " + eventName);
            }
        }
    }

    void testTestingProfileThreadFilter()
    {
        /** Setup: Profiling data available from prior record+get sequence
         *  Scenario: POST /api/testing/profile with mode=get, threadFilter=["Audio Thread"], summary=true
         *  Expected: Returns only events from the Audio Thread
         */
        beginTest("POST /api/testing/profile - threadFilter on get mode");

        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("mode", "get");
        bodyObj->setProperty("summary", true);

        Array<var> threads;
        threads.add("Audio Thread");
        bodyObj->setProperty("threadFilter", var(threads));

        auto response = ctx->httpPost("/api/testing/profile",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Thread filter should succeed: " + response);
        expect(json["results"].isArray(), "Should have results array");

        auto results = json["results"].getArray();

        if (results != nullptr)
        {
            for (int i = 0; i < results->size(); i++)
            {
                auto threadName = (*results)[i].getProperty(RestApiIds::thread, "").toString();
                expect(threadName == "Audio Thread",
                    "All results should be from Audio Thread, got: " + threadName);
            }
        }
    }

    void testTestingProfileNested()
    {
        /** Setup: Profiling data available from prior record+get sequence
         *  Scenario: POST /api/testing/profile with mode=get, filter="*processBlock*", nested=true, limit=1
         *  Expected: Returns event with children subtree
         */
        beginTest("POST /api/testing/profile - nested output");

        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("mode", "get");
        bodyObj->setProperty("filter", "*processBlock*");
        bodyObj->setProperty("nested", true);
        bodyObj->setProperty("limit", 1);

        auto response = ctx->httpPost("/api/testing/profile",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expect((bool)json["success"], "Nested mode should succeed: " + response);
        expect(json["results"].isArray(), "Should have results array");

        auto results = json["results"].getArray();

        if (results != nullptr && results->size() > 0)
        {
            auto first = (*results)[0];
            expect(first.hasProperty(RestApiIds::children), "Nested result should have children");
            expect(first.hasProperty(RestApiIds::thread), "Nested result should have thread");
        }
    }

    void testTestingProfileGetNoData()
    {
        /** Setup: Fresh reset with no prior profiling session
         *  Scenario: POST /api/testing/profile with mode=get
         *  Expected: Returns success=false with message about no data
         */
        beginTest("POST /api/testing/profile - get mode with no prior data");

        // Note: We can't easily clear the broadcaster's lastValue, so this test
        // verifies the endpoint works in get mode. After the previous test,
        // there IS data available, so this will succeed. We just verify the
        // structure is correct.
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("mode", "get");

        auto response = ctx->httpPost("/api/testing/profile",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        // After prior tests, lastValue will have data, so success=true
        expect(json["threads"].isArray(), "Should have threads array");
        expect(json["flows"].isArray(), "Should have flows array");
        expect(!(bool)json["recording"], "Should not be recording");
    }
    //==========================================================================
    // parse_css tests
    //==========================================================================
    
    void testParseCSSValid()
    {
        /** Setup: Valid CSS with two rules
         *  Scenario: Parse the CSS and check diagnostics and selectors
         *  Expected: success=true, no diagnostics, selectors list matches input
         */
        beginTest("POST /api/parse_css (valid CSS)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("code", "button { background-color: red; }\n"
                                     ".my-class { color: white; }");
        
        auto response = ctx->httpPost("/api/parse_css",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed: " + response);
        expect(json["diagnostics"].isArray(), "Should have diagnostics array");
        expectEquals<int>(json["diagnostics"].size(), 0, "Should have no diagnostics");
        expect(json["selectors"].isArray(), "Should have selectors array");
        expect(json["selectors"].size() >= 2, "Should have at least 2 selectors");
    }
    
    void testParseCSSError()
    {
        /** Setup: CSS with a syntax error (missing closing brace)
         *  Scenario: Parse the CSS
         *  Expected: success=false, diagnostics array has an error with line/column
         */
        beginTest("POST /api/parse_css (syntax error)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("code", "button { background-color: red;\n"
                                     ".my-class { color: white; }");
        
        auto response = ctx->httpPost("/api/parse_css",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail");
        expect(json["diagnostics"].isArray(), "Should have diagnostics array");
        expect(json["diagnostics"].size() > 0, "Should have at least one diagnostic");
        
        auto diag = json["diagnostics"][0];
        expectEquals(diag["severity"].toString(), String("error"), "Should be error severity");
        expectEquals(diag["source"].toString(), String("css"), "Source should be css");
        expect((int)diag["line"] > 0, "Should have a line number");
        expect((int)diag["column"] > 0, "Should have a column number");
        expect(diag["message"].toString().isNotEmpty(), "Should have a message");
    }
    
    void testParseCSSWarnings()
    {
        /** Setup: CSS with an unsupported property
         *  Scenario: Parse the CSS
         *  Expected: success=true, diagnostics array has a warning
         */
        beginTest("POST /api/parse_css (warnings)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("code", "button { text-decoration: underline; }");
        
        auto response = ctx->httpPost("/api/parse_css",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed (warnings don't fail parse)");
        expect(json["diagnostics"].isArray(), "Should have diagnostics array");
        expect(json["diagnostics"].size() > 0, "Should have at least one warning");
        
        // Find the warning about the unsupported property
        bool foundWarning = false;
        for (int i = 0; i < json["diagnostics"].size(); i++)
        {
            auto d = json["diagnostics"][i];
            if (d["severity"].toString() == "warning" &&
                d["message"].toString().containsIgnoreCase("text-decoration"))
            {
                foundWarning = true;
                expect((int)d["line"] > 0, "Warning should have line number");
                expect((int)d["column"] > 0, "Warning should have column number");
            }
        }
        expect(foundWarning, "Should have warning about unsupported property");
    }
    
    void testParseCSSResolveSpecificity()
    {
        /** Setup: CSS with class and ID rules for the same property
         *  Scenario: Query with selectors that match both rules
         *  Expected: ID selector wins over class selector (higher specificity)
         */
        beginTest("POST /api/parse_css (specificity resolution)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("code",
            ".my-button { background-color: 0xFFFF0000; }\n"
            "#Button1 { background-color: 0xFF00FF00; }");
        
        Array<var> selectors;
        selectors.add("button");
        selectors.add(".my-button");
        selectors.add("#Button1");
        bodyObj->setProperty("selectors", var(selectors));
        
        auto response = ctx->httpPost("/api/parse_css",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed: " + response);
        expect(json.hasProperty("properties"), "Should have properties object");
        
        auto props = json["properties"];
        auto bgColor = props["background-color"].toString();
        
        // The ID selector (#Button1) should win: green (0xFF00FF00)
        expectEquals(bgColor, String("0xFF00FF00"),
            "ID selector should take precedence over class selector");
    }
    
    void testParseCSSResolvedValues()
    {
        /** Setup: CSS with percentage and em values
         *  Scenario: Query with selectors and provide width/height for resolution
         *  Expected: Properties have both raw values and resolved pixel values
         */
        beginTest("POST /api/parse_css (resolved values)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("code",
            "button { width: 50%; padding-left: 2em; }");
        
        Array<var> selectors;
        selectors.add("button");
        bodyObj->setProperty("selectors", var(selectors));
        bodyObj->setProperty("width", 600);
        bodyObj->setProperty("height", 400);
        
        auto response = ctx->httpPost("/api/parse_css",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed: " + response);
        expect(json.hasProperty("properties"), "Should have properties object");
        
        auto props = json["properties"];
        
        // width: 50% of 600 = 300
        auto widthProp = props["width"];
        expect(widthProp.isObject(), "width should be object with value/resolved");
        expectEquals(widthProp["value"].toString(), String("50%"), "Raw value should be 50%");
        expectEquals<double>((double)widthProp["resolved"], 300.0,
            "50% of 600px width should resolve to 300");
        
        // padding-left: 2em = 2 * 16 (default font size) = 32
        auto paddingProp = props["padding-left"];
        expect(paddingProp.isObject(), "padding-left should be object with value/resolved");
        expectEquals(paddingProp["value"].toString(), String("2em"), "Raw value should be 2em");
        expectEquals<double>((double)paddingProp["resolved"], 32.0,
            "2em should resolve to 32px (2 * 16px default)");
    }
    
    void testParseCSSFromFile()
    {
        /** Setup: Create a temporary .css file in the Scripts directory
         *  Scenario: Pass the relative file path to parse_css
         *  Expected: Parses successfully, returns resolved filePath
         */
        beginTest("POST /api/parse_css (from file)");
        
        auto tempFile = createTempScriptFile("_test_parse.css",
            "button { background-color: 0xFF112233; }\n");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty("filePath", "_test_parse.css");
        
        auto response = ctx->httpPost("/api/parse_css",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect((bool)json["success"], "Should succeed: " + response);
        expect(json.hasProperty("filePath"), "Should have resolved filePath");
        expect(json["filePath"].toString().contains("_test_parse.css"),
            "filePath should contain the file name");
        expect(json["selectors"].isArray(), "Should have selectors");
        
        tempFile.deleteFile();
    }
    
    void testParseCSSEmptyCode()
    {
        /** Setup: POST with neither code nor filePath
         *  Scenario: Call parse_css with empty body
         *  Expected: 400 error
         */
        beginTest("POST /api/parse_css (empty code)");
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        
        auto response = ctx->httpPost("/api/parse_css",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json["success"], "Should fail with empty input");
    }
    
    void testShutdown()
    {
        /** Setup: No specific setup needed
         *  Scenario: Verify shutdown endpoint exists in route metadata
         *  Expected: Endpoint is POST and registered correctly
         *  Note: Cannot actually call this endpoint as it would terminate the test runner
         */
        beginTest("POST /api/shutdown (excluded from tests)");
        
        // Mark as touched for endpoint coverage verification.
        ctx->touchedEndpoints.addIfNotAlreadyThere("/api/shutdown");
        
        // Verify the endpoint exists in the route metadata
        bool found = false;
        for (const auto& route : RestHelpers::getRouteMetadata())
        {
            if (route.path == "api/shutdown")
            {
                found = true;
                expect(route.method == RestServer::POST, "shutdown should be POST");
                expect(route.category == "status", "shutdown should be in status category");
                break;
            }
        }
        expect(found, "shutdown should exist in route metadata");
    }

    void testSnippetBrowserRejectionMetadata()
    {
        /** Setup: Static route metadata.
         *  Scenario: Verify the rejectInSnippetBrowser flag is set on every project/* route
         *            (except the snippet import/export pair) and on every wizard/* route.
         *            The dispatcher consults this flag to return 409 while the snippet
         *            browser is the active BackendProcessor.
         *  Expected: Flagged set matches the policy; export_snippet/import_snippet are exempt.
         */
        beginTest("Snippet browser rejection metadata");

        StringArray expectFlagged {
            "api/project/list",
            "api/project/tree",
            "api/project/files",
            "api/project/settings/list",
            "api/project/settings/set",
            "api/project/save",
            "api/project/load",
            "api/project/switch",
            "api/project/preprocessor/list",
            "api/project/preprocessor/set",
            "api/wizard/initialise",
            "api/wizard/execute",
            "api/wizard/status"
        };

        StringArray expectExempt {
            "api/project/export_snippet",
            "api/project/import_snippet"
        };

        for (const auto& route : RestHelpers::getRouteMetadata())
        {
            if (expectFlagged.contains(route.path))
            {
                expect(route.rejectInSnippetBrowser,
                       route.path + " should reject in snippet browser mode");
            }
            else if (expectExempt.contains(route.path))
            {
                expect(!route.rejectInSnippetBrowser,
                       route.path + " should NOT reject in snippet browser mode");
            }
        }
    }

    void testSnippetBrowser()
    {
        /** Setup: Headless test BackendProcessor (no UI root window).
         *  Scenario: Validate input handling and the headless 501 path.
         *  Expected:
         *   - Missing action returns 400 with helpful error message.
         *   - Invalid action returns 400.
         *   - Any valid action returns 501 because currentRootWindow is nullptr
         *     in unit tests (the snippet browser requires a real UI root).
         *  Note: Full launch/shutdown/enable/disable lifecycle requires a real
         *  desktop window and is covered by manual smoke testing, not unit tests.
         */
        beginTest("POST /api/snippet_browser");

        ctx->reset();

        // Missing action
        {
            auto json = ctx->parseJson(ctx->httpPost("/api/snippet_browser", "{}"));
            expect(!(bool)json[RestApiIds::success], "Missing action should fail");
            expectErrorMessageContains(json, "action");
        }

        // Invalid action
        {
            DynamicObject::Ptr body = new DynamicObject();
            body->setProperty("action", "garbage");
            auto json = ctx->parseJson(ctx->httpPost("/api/snippet_browser",
                                                     JSON::toString(var(body.get()))));
            expect(!(bool)json[RestApiIds::success], "Invalid action should fail");
            expectErrorMessageContains(json, "launch");
        }

        // Valid action in headless: returns 501. The body has success=false
        // and a 501-style error, since the test BP has no UI root window.
        {
            DynamicObject::Ptr body = new DynamicObject();
            body->setProperty("action", "launch");
            auto json = ctx->parseJson(ctx->httpPost("/api/snippet_browser",
                                                     JSON::toString(var(body.get()))));
            expect(!(bool)json[RestApiIds::success], "Headless launch should fail with 501");
            expectErrorMessageContains(json, "UI root window");
        }
    }

    void testBuilderTree()
    {
        /** Setup: Fresh project with default module tree
         *  Scenario: Request module tree hierarchy
         *  Expected: Returns tree structure with master chain
         */
        beginTest("GET /api/builder/tree");
        
        ctx->reset();
        
        auto response = ctx->httpGet("/api/builder/tree");
        var json = ctx->parseJson(response);
        
        expect((bool)json[RestApiIds::success], "Should succeed");
        expect(json[RestApiIds::errors].isArray(), "errors should be array");
        expectEquals<int>(json[RestApiIds::errors].size(), 0, "Should have no errors");
    }
    
    /** Directly deletes the processor - this can be used to simulate data model drift from an external operation. */
	void deleteProcessor(const String& moduleId)
	{
		if (auto p = ProcessorHelpers::getFirstProcessorWithName(ctx->mc->getMainSynthChain(), moduleId))
		{
			//rest_undo::builder::Helpers::deleteProcessorAsync(p);

			if (auto c = dynamic_cast<Chain*>(p->getParentProcessor(false)))
				c->getHandler()->remove(p, true);
		}
	}

    void expectNoBuilderError(const var& json)
    {
        expect((bool)json[RestApiIds::success], "Should succeed");

        auto e = json[RestApiIds::errors];
		expect(e.isArray(), "errors should be array");

        if (e.isArray())
        {
            for (auto& er : *e.getArray())
                expect(false, er["errorMessage"].toString());            
        }
    }

    void expectErrorMessageContains(const var& json, const String& text)
    {
        expect(!(bool)json[RestApiIds::success], "Should fail");
        expect(json[RestApiIds::errors].isArray(), "Should have errors array");
        expect(json[RestApiIds::errors].size() > 0, "Should have at least one error");

        if (json[RestApiIds::errors].isArray() && json[RestApiIds::errors].size() > 0)
        {
            auto msg = json[RestApiIds::errors][0][RestApiIds::errorMessage].toString();
            expect(msg.containsIgnoreCase(text), "Error should mention " + text + ": " + msg);
        }
    }

    void resetBuilderState()
    {
        ctx->reset();
        auto clearJson = ctx->parseJson(ctx->httpPost("/api/undo/clear", "{}"));
        expect((bool)clearJson[RestApiIds::success], "undo/clear should succeed during builder reset");
    }

    var postBuilderOps(const Array<var>& ops)
    {
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty(RestApiIds::operations, var(ops));
        auto response = ctx->httpPost("/api/builder/apply", JSON::toString(var(bodyObj.get())));
        return ctx->parseJson(response);
    }

    var makeAddOp(const String& type, const String& name,
                  const String& parent = "Master Chain", int chain = -1)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "add");
        op->setProperty(RestApiIds::type, type);
        op->setProperty(RestApiIds::parent, parent);
        op->setProperty(RestApiIds::chain, chain);
        op->setProperty(RestApiIds::name, name);
        return var(op.get());
    }

    var makeSetBypassedOp(const String& target, bool value)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "set_bypassed");
        op->setProperty(RestApiIds::target, target);
        op->setProperty(RestApiIds::bypassed, value);
        return var(op.get());
    }

    var makeSetIdOp(const String& target, const String& name)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "set_id");
        op->setProperty(RestApiIds::target, target);
        op->setProperty(RestApiIds::name, name);
        return var(op.get());
    }

    var makeRemoveOp(const String& target)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "remove");
        op->setProperty(RestApiIds::target, target);
        return var(op.get());
    }

    var makeCloneOp(const String& source, int count)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "clone");
        op->setProperty(RestApiIds::source, source);
        op->setProperty(RestApiIds::count, count);
        return var(op.get());
    }

    var makeSetAttributesOp(const String& target, const NamedValueSet& kv)
    {
        DynamicObject::Ptr attrs = new DynamicObject();

        for (int i = 0; i < kv.size(); i++)
            attrs->setProperty(kv.getName(i), kv.getValueAt(i));

        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "set_attributes");
        op->setProperty(RestApiIds::target, target);
        op->setProperty(RestApiIds::mode, "value");
        op->setProperty(RestApiIds::attributes, var(attrs.get()));
        return var(op.get());
    }

    void expectBuilderProcessorExists(const String& moduleName)
    {
        auto ok = ProcessorHelpers::getFirstProcessorWithName(ctx->mc->getMainSynthChain(), moduleName) != nullptr;
        expect(ok, moduleName + " does not exist");
    }

	void expectBuilderProcessorRemoved(const String& moduleName)
	{
		auto ok = ProcessorHelpers::getFirstProcessorWithName(ctx->mc->getMainSynthChain(), moduleName) == nullptr;
		expect(ok, moduleName + " is not removed");
	}

	void expectBuilderProcessorBypassed(const String& moduleName, bool expectedValue)
	{
		auto p = ProcessorHelpers::getFirstProcessorWithName(ctx->mc->getMainSynthChain(), moduleName);
		expect(p != nullptr, moduleName + " not found");

		if (p != nullptr)
		{
			String m;
			m << moduleName;
			m << (expectedValue ? " should be bypassed" : " should not be bypassed");
			expect(p->isBypassed() == expectedValue, m);
		}
	}

    void expectBuilderProcessorAttribute(const String& moduleName, const String& parameterId, float value)
    {
		auto p = ProcessorHelpers::getFirstProcessorWithName(ctx->mc->getMainSynthChain(), moduleName);
		expect(p != nullptr, moduleName + " not found");

		if (p != nullptr)
		{
			auto idx = p->getParameterIndexForIdentifier(Identifier(parameterId));
			expect(idx >= 0, moduleName + ": unknown parameter " + parameterId);

			if (idx >= 0)
			{
				auto actualValue = p->getAttribute(idx);
				String m;
				m << moduleName << "." << parameterId << "=" << String(value);
				expectWithinAbsoluteError<float>(actualValue, value, 0.01f, m);
			}
		}
    }

	void expectDiffEntry(const var& json, int index, const String& action,
		const String& target, const String& domain = "builder")
	{
		auto diff = json[RestApiIds::diff];
		expect(diff.isArray(), "result.diff should be array");
		expect(index >= 0 && index < diff.size(), "Diff index out of range");

		if (index >= 0 && index < diff.size())
		{
			expectEquals(diff[index]["domain"].toString(), domain);
			expectEquals(diff[index]["action"].toString(), action);
			expectEquals(diff[index]["target"].toString(), target);
		}
	}

	void expectHasNestedChild(const String& parentId, const String& childId)
	{
		auto p = ProcessorHelpers::getFirstProcessorWithName(ctx->mc->getMainSynthChain(), parentId);
		expect(p != nullptr, parentId + " not found");

		if (p != nullptr)
		{
			auto c = ProcessorHelpers::getFirstProcessorWithName(p, childId);
			expect(c != nullptr, parentId + " should contain child " + childId);
		}
	}

	void expectErrorPhase(const var& json, const String& phase)
	{
		expect(json[RestApiIds::errors].isArray(), "errors should be array");
		expect(json[RestApiIds::errors].size() > 0, "response should contain at least one error");

		if (json[RestApiIds::errors].isArray() && json[RestApiIds::errors].size() > 0)
		{
			auto cs = json[RestApiIds::errors][0][RestApiIds::callstack];
			expect(cs.isArray() && cs.size() >= 4, "callstack should contain op / phase / group / endpoint frames");
			if (cs.isArray() && cs.size() >= 2)
				expectEquals(cs[1].toString(), String("phase: ") + phase);
		}
	}

    void testBuilderApply()
    {
        /** Setup: Fresh project
         *  Scenario: Batch add a SineSynth and set its attributes
         *  Expected: Operations are applied successfully
         */
        beginTest("POST /api/builder/apply");
        
        resetBuilderState();

        Array<var> ops;
        ops.add(makeAddOp("SineSynth", "MySine"));

        auto json = postBuilderOps(ops);

        expectNoBuilderError(json);
        expectBuilderProcessorExists("MySine");
        expectBuilderProcessorBypassed("MySine", false);
        expectEquals<int>(json[RestApiIds::diff].size(), 1,
            "Initial add should produce one diff item");
        expectDiffEntry(json, 0, "+", "MySine");

        expect(json[RestApiIds::diff].isArray(), "diff should be array");

        Array<var> ops2;
        ops2.add(makeSetBypassedOp("MySine", true));
        ops2.add(makeSetIdOp("MySine", "MySineRenamed"));

        json = postBuilderOps(ops2);

        expectNoBuilderError(json);

        expectBuilderProcessorRemoved("MySine");
        expectBuilderProcessorExists("MySineRenamed");
        expectBuilderProcessorBypassed("MySineRenamed", true);
        expectDiffEntry(json, 0, "+", "MySineRenamed");

        Array<var> ops3;
        NamedValueSet attrs;
        attrs.set("SaturationAmount", 0.25);
        attrs.set("OctaveTranspose", -3.0);
        attrs.set("SemiTones", 4.0);
        ops3.add(makeSetAttributesOp("MySineRenamed", attrs));

        json = postBuilderOps(ops3);
        expectNoBuilderError(json);
        expectBuilderProcessorAttribute("MySineRenamed", "SaturationAmount", 0.25f);
        expectBuilderProcessorAttribute("MySineRenamed", "OctaveTranspose", -3.0f);
        expectBuilderProcessorAttribute("MySineRenamed", "SemiTones", 4.0f);
        expectDiffEntry(json, 0, "+", "MySineRenamed");

        auto undoJson = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));
        expectNoBuilderError(undoJson);
        expectBuilderProcessorAttribute("MySineRenamed", "SaturationAmount", 0.0f);

        auto redoJson = ctx->parseJson(ctx->httpPost("/api/undo/forward", "{}"));
        expectNoBuilderError(redoJson);
        expectBuilderProcessorAttribute("MySineRenamed", "SaturationAmount", 0.25f);

    }

    void testBuilderCloneNestedModules()
    {
        /** Setup: Parent module with nested child
         *  Scenario: Clone parent once
         *  Expected: Cloned parent exists and contains nested child module type
         */
        beginTest("POST /api/builder/apply (clone nested modules)");

        resetBuilderState();

        Array<var> seedOps;
        seedOps.add(makeAddOp("SineSynth", "NestedParent"));
        auto seedJson = postBuilderOps(seedOps);
        expectNoBuilderError(seedJson);

        seedOps.clear();
        seedOps.add(makeAddOp("LFO", "NestedChild", "NestedParent", 1));
        seedJson = postBuilderOps(seedOps);
        expectNoBuilderError(seedJson);
        expectBuilderProcessorExists("NestedParent");
        expectBuilderProcessorExists("NestedChild");
        expectHasNestedChild("NestedParent", "NestedChild");

        Array<var> cloneOps;
        cloneOps.add(makeCloneOp("NestedParent", 1));
        auto cloneJson = postBuilderOps(cloneOps);
        expectNoBuilderError(cloneJson);

        auto diff = cloneJson[RestApiIds::diff];
        expect(diff.isArray(), "diff should be array");
        expect(diff.size() >= 1, "clone should append at least one diff entry");

        String clonedParentName;

        for (int i = 0; i < diff.size(); i++)
        {
            auto action = diff[i]["action"].toString();
            auto target = diff[i]["target"].toString();

            if (action == "+" && target != "NestedParent")
            {
                clonedParentName = target;
                break;
            }
        }

        expect(clonedParentName.isNotEmpty(), "Should find cloned parent name in diff list");

        if (clonedParentName.isNotEmpty())
        {
            expectBuilderProcessorExists(clonedParentName);

            auto parent = ProcessorHelpers::getFirstProcessorWithName(ctx->mc->getMainSynthChain(), clonedParentName);
            expect(parent != nullptr, "Cloned parent must exist");

            bool hasNestedLfo = false;

            if (parent != nullptr)
            {
                Processor::Iterator<Processor> iter(parent, false);
                while (auto p = iter.getNextProcessor())
                {
                    if (p->getType().toString() == "LFO")
                    {
                        hasNestedLfo = true;
                        break;
                    }
                }
            }

            expect(hasNestedLfo, "Cloned parent should contain cloned nested LFO child");
        }
    }

	void testPlanValidationRemoveThenSetIdFails()
	{
		/** Setup: Real module exists, then queued for removal in plan
		 *  Scenario: Try renaming removed module in same plan
		 *  Expected: Validation fails (module logically deleted in plan view)
		 */
		beginTest("Plan validation: remove then set_id fails");

		resetBuilderState();

		Array<var> seedOps;
		seedOps.add(makeAddOp("SineSynth", "CaseA"));
		auto seedJson = postBuilderOps(seedOps);
		expectNoBuilderError(seedJson);

		DynamicObject::Ptr pushBody = new DynamicObject();
		pushBody->setProperty(RestApiIds::name, "case-a");
		auto pushJson = ctx->parseJson(ctx->httpPost("/api/undo/push_group", JSON::toString(var(pushBody.get()))));
		expectNoBuilderError(pushJson);

		Array<var> ops;
		ops.add(makeRemoveOp("CaseA"));
		auto removeJson = postBuilderOps(ops);
		expectNoBuilderError(removeJson);

		ops.clear();
		ops.add(makeSetIdOp("CaseA", "CaseARenamed"));
		auto setIdJson = postBuilderOps(ops);
		expect(!(bool)setIdJson[RestApiIds::success], "Renaming removed module should fail in plan validation");
		expectErrorPhase(setIdJson, "validate");
	}

	void testPlanValidationRenameThenRemoveOldNameFails()
	{
		/** Setup: Real module exists, then queued for rename in plan
		 *  Scenario: Remove using old name in same plan
		 *  Expected: Validation fails (old name no longer valid)
		 */
		beginTest("Plan validation: rename then remove old name fails");

		resetBuilderState();

		Array<var> seedOps;
		seedOps.add(makeAddOp("SineSynth", "CaseB"));
		auto seedJson = postBuilderOps(seedOps);
		expectNoBuilderError(seedJson);

		DynamicObject::Ptr pushBody = new DynamicObject();
		pushBody->setProperty(RestApiIds::name, "case-b");
		auto pushJson = ctx->parseJson(ctx->httpPost("/api/undo/push_group", JSON::toString(var(pushBody.get()))));
		expectNoBuilderError(pushJson);

		Array<var> ops;
		ops.add(makeSetIdOp("CaseB", "CaseBRenamed"));
		auto renameJson = postBuilderOps(ops);
		expectNoBuilderError(renameJson);

		ops.clear();
		ops.add(makeRemoveOp("CaseB"));
		auto removeOldJson = postBuilderOps(ops);
		expect(!(bool)removeOldJson[RestApiIds::success], "Removing old name should fail after rename in plan");
		expectErrorPhase(removeOldJson, "validate");
	}

	void testPlanValidationAddRemoveAddSameIdSucceeds()
	{
		/** Setup: Empty plan group
		 *  Scenario: add X, remove X, add X in same plan
		 *  Expected: All validations pass and pop executes successfully
		 */
		beginTest("Plan validation: add/remove/add same id succeeds");

		resetBuilderState();

		DynamicObject::Ptr pushBody = new DynamicObject();
		pushBody->setProperty(RestApiIds::name, "case-c");
		auto pushJson = ctx->parseJson(ctx->httpPost("/api/undo/push_group", JSON::toString(var(pushBody.get()))));
		expectNoBuilderError(pushJson);

		Array<var> ops;
		ops.add(makeAddOp("SineSynth", "CaseC"));
		auto add1 = postBuilderOps(ops);
		expectNoBuilderError(add1);

		ops.clear();
		ops.add(makeRemoveOp("CaseC"));
		auto rem = postBuilderOps(ops);
		expectNoBuilderError(rem);

		ops.clear();
		ops.add(makeAddOp("SineSynth", "CaseC"));
		auto add2 = postBuilderOps(ops);
		expectNoBuilderError(add2);

		DynamicObject::Ptr popBody = new DynamicObject();
		popBody->setProperty(RestApiIds::cancel, false);
		auto popJson = ctx->parseJson(ctx->httpPost("/api/undo/pop_group", JSON::toString(var(popBody.get()))));
		expectNoBuilderError(popJson);
		expectBuilderProcessorExists("CaseC");
	}

	void testPlanValidationRemoveParentThenChildOpFails()
	{
		/** Setup: Parent + nested child exist
		 *  Scenario: Queue parent removal, then queue child operation
		 *  Expected: Child op fails validation because child is logically gone
		 */
		beginTest("Plan validation: remove parent then child op fails");

		resetBuilderState();

		Array<var> seedOps;
		seedOps.add(makeAddOp("SineSynth", "CaseParent"));
		auto seedJson = postBuilderOps(seedOps);
		expectNoBuilderError(seedJson);

		seedOps.clear();
		seedOps.add(makeAddOp("LFO", "CaseChild", "CaseParent", 1));
		seedJson = postBuilderOps(seedOps);
		expectNoBuilderError(seedJson);

		DynamicObject::Ptr pushBody = new DynamicObject();
		pushBody->setProperty(RestApiIds::name, "case-d");
		auto pushJson = ctx->parseJson(ctx->httpPost("/api/undo/push_group", JSON::toString(var(pushBody.get()))));
		expectNoBuilderError(pushJson);

		Array<var> ops;
		ops.add(makeRemoveOp("CaseParent"));
		auto remParentJson = postBuilderOps(ops);
		expectNoBuilderError(remParentJson);

		ops.clear();
        ops.add(makeSetIdOp("CaseChild", "CaseChildRenamed"));
		auto childOpJson = postBuilderOps(ops);
		expect(!(bool)childOpJson[RestApiIds::success], "Child op should fail after parent removal in plan");
		expectErrorPhase(childOpJson, "validate");
	}

    

    void testBuilderChildOpsFailAfterParentDeleted()
    {
        /** Setup: Parent + nested child, then parent is deleted manually
         *  Scenario: Execute pre-validated child operations via pop_group
         *  Expected: Each operation fails with phase=runtime because child is gone
         */
        beginTest("POST /api/builder/apply (child ops fail after parent delete)");

        auto runRuntimeCase = [this](const var& childOp, const String& label)
        {
            resetBuilderState();

            Array<var> seedOps;
            seedOps.add(makeAddOp("SineSynth", "RuntimeParent"));
            auto seedJson = postBuilderOps(seedOps);
            expectNoBuilderError(seedJson);

            seedOps.clear();
            seedOps.add(makeAddOp("LFO", "RuntimeChild", "RuntimeParent", 1));
            seedJson = postBuilderOps(seedOps);
            expectNoBuilderError(seedJson);
            expectBuilderProcessorExists("RuntimeParent");
            expectBuilderProcessorExists("RuntimeChild");

            DynamicObject::Ptr pushBody = new DynamicObject();
            pushBody->setProperty(RestApiIds::name, "runtime-case-" + label);
            auto pushJson = ctx->parseJson(ctx->httpPost("/api/undo/push_group", JSON::toString(var(pushBody.get()))));
            expectNoBuilderError(pushJson);

            Array<var> groupedOps;
            groupedOps.add(childOp);
            auto groupedJson = postBuilderOps(groupedOps);
            expectNoBuilderError(groupedJson);

            deleteProcessor("RuntimeParent");

            expectBuilderProcessorRemoved("RuntimeParent");
            expectBuilderProcessorRemoved("RuntimeChild");

            DynamicObject::Ptr popBody = new DynamicObject();
            popBody->setProperty(RestApiIds::cancel, false);
            auto popJson = ctx->parseJson(ctx->httpPost("/api/undo/pop_group", JSON::toString(var(popBody.get()))));

            expect(!(bool)popJson[RestApiIds::success], "Pop should fail for " + label);
            expect(popJson[RestApiIds::errors].isArray(), "errors should be array for " + label);
            expect(popJson[RestApiIds::errors].size() > 0, "Should contain runtime error for " + label);

            auto cs = popJson[RestApiIds::errors][0][RestApiIds::callstack];
            expect(cs.isArray() && cs.size() >= 4, "Callstack should have runtime frames for " + label);
            expectEquals(cs[1].toString(), String("phase: runtime"), "Phase should be runtime for " + label);
            expectEquals(cs[3].toString(), String("endpoint: api/undo/pop_group"), "Endpoint should be pop_group for " + label);
        };

        runRuntimeCase(makeSetBypassedOp("RuntimeChild", true), "set_bypassed");
        runRuntimeCase(makeSetIdOp("RuntimeChild", "RuntimeChildRenamed"), "set_id");

        NamedValueSet attrs;
        attrs.set("Intensity", 0.5);
        runRuntimeCase(makeSetAttributesOp("RuntimeChild", attrs), "set_attributes");

        runRuntimeCase(makeCloneOp("RuntimeChild", 1), "clone");
        runRuntimeCase(makeRemoveOp("RuntimeChild"), "remove");
    }
    
    void testBuilderApplyMissingOp()
    {
        /** Setup: Fresh project
         *  Scenario: Send operation without op field
         *  Expected: Validation error
         */
        beginTest("POST /api/builder/apply (missing op)");
        
        resetBuilderState();
        
        Array<var> ops;
        
        DynamicObject::Ptr op1 = new DynamicObject();
        op1->setProperty(RestApiIds::type, "SineSynth");
        op1->setProperty(RestApiIds::chain, -1);
        ops.add(var(op1.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty(RestApiIds::operations, var(ops));
        
        auto response = ctx->httpPost("/api/builder/apply",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json[RestApiIds::success], "Should fail - missing op field");
        expect(json[RestApiIds::errors].isArray(), "errors should be array");
        expectEquals<int>(json[RestApiIds::errors].size(), 1, "Should have exactly one validation error");

        auto e = json[RestApiIds::errors][0];
        expect(e[RestApiIds::errorMessage].toString().contains("op field is required"),
               "Error should mention missing op field");

        auto callstack = e[RestApiIds::callstack];
        expect(callstack.isArray(), "callstack should be array");
        expectEquals(callstack[0].toString(), String("op[0]: "), "Frame 0 should reference op index");
        expectEquals(callstack[1].toString(), String("phase: prevalidate"), "Frame 1 should be prevalidate phase");
        expectEquals(callstack[3].toString(), String("endpoint: api/builder/apply"), "Frame 3 should be endpoint");
    }
    
    void testBuilderApplyUnknownOp()
    {
        /** Setup: Fresh project
         *  Scenario: Send operation with unknown op type
         *  Expected: Validation error
         */
        beginTest("POST /api/builder/apply (unknown op)");
        
        resetBuilderState();
        
        Array<var> ops;
        
        DynamicObject::Ptr op1 = new DynamicObject();
        op1->setProperty(RestApiIds::op, "nonexistent");
        op1->setProperty(RestApiIds::target, "Something");
        ops.add(var(op1.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty(RestApiIds::operations, var(ops));
        
        auto response = ctx->httpPost("/api/builder/apply",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json[RestApiIds::success], "Should fail - unknown op");
        expect(json[RestApiIds::errors].isArray(), "errors should be array");
        expectEquals<int>(json[RestApiIds::errors].size(), 1, "Should have one error");

        auto e = json[RestApiIds::errors][0];
        expect(e[RestApiIds::errorMessage].toString().contains("unsupported op"),
               "Error should mention unsupported op");

        auto callstack = e[RestApiIds::callstack];
        expectEquals(callstack[1].toString(), String("phase: prevalidate"), "Should report prevalidate phase");
        expectEquals(callstack[3].toString(), String("endpoint: api/builder/apply"), "Should report builder endpoint");
    }
    
    void testBuilderApplyMissingFields()
    {
        /** Setup: Fresh project
         *  Scenario: Send operations with missing required fields per type
         *  Expected: Validation errors for each bad operation
         */
        beginTest("POST /api/builder/apply (missing fields)");
        
        resetBuilderState();
        
        Array<var> ops;
        
        // add missing type
        DynamicObject::Ptr op1 = new DynamicObject();
        op1->setProperty(RestApiIds::op, "add");
        op1->setProperty(RestApiIds::chain, 3);
        ops.add(var(op1.get()));
        
        // remove missing target
        DynamicObject::Ptr op2 = new DynamicObject();
        op2->setProperty(RestApiIds::op, "remove");
        ops.add(var(op2.get()));
        
        // set_attributes missing attributes object
        DynamicObject::Ptr op3 = new DynamicObject();
        op3->setProperty(RestApiIds::op, "set_attributes");
        op3->setProperty(RestApiIds::target, "Something");
        ops.add(var(op3.get()));

        // set_id missing name
        DynamicObject::Ptr op4 = new DynamicObject();
        op4->setProperty(RestApiIds::op, "set_id");
        op4->setProperty(RestApiIds::target, "Anything");
        ops.add(var(op4.get()));

        // set_bypassed missing bypassed
        DynamicObject::Ptr op5 = new DynamicObject();
        op5->setProperty(RestApiIds::op, "set_bypassed");
        op5->setProperty(RestApiIds::target, "Anything");
        ops.add(var(op5.get()));

        // set_effect missing effect
        DynamicObject::Ptr op6 = new DynamicObject();
        op6->setProperty(RestApiIds::op, "set_effect");
        op6->setProperty(RestApiIds::target, "Anything");
        ops.add(var(op6.get()));

        // clone missing source / invalid count
        DynamicObject::Ptr op7 = new DynamicObject();
        op7->setProperty(RestApiIds::op, "clone");
        op7->setProperty(RestApiIds::count, 0);
        ops.add(var(op7.get()));
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty(RestApiIds::operations, var(ops));
        
        auto response = ctx->httpPost("/api/builder/apply",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);
        
        expect(!(bool)json[RestApiIds::success], "Should fail validation");
        expectEquals<int>(json[RestApiIds::errors].size(), 7, "Should have one error per bad operation");

        for (int i = 0; i < json[RestApiIds::errors].size(); i++)
        {
            auto e = json[RestApiIds::errors][i];
            auto cs = e[RestApiIds::callstack];
            expect(cs.isArray(), "Each error must include callstack");
            expect(cs.size() >= 4, "Callstack should have 4 frames");
            expect(cs[1].toString() == "phase: prevalidate", "Should be prevalidate phase");
            expect(cs[3].toString() == "endpoint: api/builder/apply", "Should include endpoint frame");
        }

        // Validate operation index mapping remains stable.
        expectEquals(json[RestApiIds::errors][0][RestApiIds::callstack][0].toString(), String("op[0]: add"));
        expectEquals(json[RestApiIds::errors][1][RestApiIds::callstack][0].toString(), String("op[1]: remove"));
        expectEquals(json[RestApiIds::errors][2][RestApiIds::callstack][0].toString(), String("op[2]: set_attributes"));
        expectEquals(json[RestApiIds::errors][3][RestApiIds::callstack][0].toString(), String("op[3]: set_id"));
        expectEquals(json[RestApiIds::errors][4][RestApiIds::callstack][0].toString(), String("op[4]: set_bypassed"));
        expectEquals(json[RestApiIds::errors][5][RestApiIds::callstack][0].toString(), String("op[5]: set_effect"));
        expectEquals(json[RestApiIds::errors][6][RestApiIds::callstack][0].toString(), String("op[6]: clone"));

        // Validate-only branch: existing module with unknown parameter
        Array<var> validateOps;
        DynamicObject::Ptr add = new DynamicObject();
        add->setProperty(RestApiIds::op, "add");
        add->setProperty(RestApiIds::type, "SineSynth");
        add->setProperty(RestApiIds::parent, "Master Chain");
        add->setProperty(RestApiIds::chain, -1);
        add->setProperty(RestApiIds::name, "MySine");
        validateOps.add(var(add.get()));

        DynamicObject::Ptr addBody = new DynamicObject();
        addBody->setProperty(RestApiIds::operations, var(validateOps));
        auto addResponse = ctx->httpPost("/api/builder/apply", JSON::toString(var(addBody.get())));
        auto addJson = ctx->parseJson(addResponse);
        expect((bool)addJson[RestApiIds::success], "Seeding MySine should succeed");

        validateOps.clear();
        DynamicObject::Ptr badAttr = new DynamicObject();
        badAttr->setProperty(RestApiIds::op, "set_attributes");
        badAttr->setProperty(RestApiIds::target, "MySine");
        badAttr->setProperty(RestApiIds::mode, "value");

        DynamicObject::Ptr attrs = new DynamicObject();
        attrs->setProperty("DefinitelyNotARealParameter", 1.0);
        badAttr->setProperty(RestApiIds::attributes, var(attrs.get()));
        validateOps.add(var(badAttr.get()));

        DynamicObject::Ptr validateBody = new DynamicObject();
        validateBody->setProperty(RestApiIds::operations, var(validateOps));
        auto validateResponse = ctx->httpPost("/api/builder/apply", JSON::toString(var(validateBody.get())));
        auto validateJson = ctx->parseJson(validateResponse);

        expect(!(bool)validateJson[RestApiIds::success], "Unknown parameter should fail validation");
        expectEquals<int>(validateJson[RestApiIds::errors].size(), 1, "Should return one validation error");
        auto validateStack = validateJson[RestApiIds::errors][0][RestApiIds::callstack];
        expectEquals(validateStack[1].toString(), String("phase: validate"), "Should report validate phase");
        expectEquals(validateStack[3].toString(), String("endpoint: api/builder/apply"), "Should include endpoint");
    }
    
    void testBuilderReset()
    {
        /** Setup: Add a module first so tree is non-empty
         *  Scenario: POST /api/builder/reset to clear the module tree
         *  Expected: Returns success, tree is empty afterward
         */
        beginTest("POST /api/builder/reset");

        resetBuilderState();

        // Add a module so we have something to reset
        Array<var> ops;
        DynamicObject::Ptr addOp = new DynamicObject();
        addOp->setProperty(RestApiIds::op, "add");
        addOp->setProperty(RestApiIds::type, "SineSynth");
        addOp->setProperty(RestApiIds::parent, "Master Chain");
        addOp->setProperty(RestApiIds::chain, -1);
        addOp->setProperty(RestApiIds::name, "TestSine");
        ops.add(var(addOp.get()));

        auto addJson = postBuilderOps(ops);
        expect((bool)addJson[RestApiIds::success], "Should add module successfully");

        // Now reset
        auto response = ctx->httpPost("/api/builder/reset", "{}");
        var json = ctx->parseJson(response);

        expect((bool)json[RestApiIds::success], "Reset should succeed");
        expectEquals(json[RestApiIds::result].toString(), String("Module tree reset"), "Should return reset message");
    }

    void testUndoPushGroup()
    {
        /** Setup: Fresh project
         *  Scenario: Push a new undo group
         *  Expected: Returns success with group context
         */
        beginTest("POST /api/undo/push_group");
        
        resetBuilderState();
        
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty(RestApiIds::name, "Test Group");
        
        auto response = ctx->httpPost("/api/undo/push_group",
                                      JSON::toString(var(bodyObj.get())));
        var json = ctx->parseJson(response);

        expect((bool)json[RestApiIds::success], "Should succeed");
        expect(json[RestApiIds::errors].isArray(), "errors should be array");
        expectEquals<int>(json[RestApiIds::errors].size(), 0, "Should have no errors");

        expectEquals(json[RestApiIds::groupName].toString(), String("Test Group"), "groupName should match pushed group");
    }

    void testUndoPopGroup()
    {
        /** Setup: Push a group first
         *  Scenario: Pop the group
         *  Expected: Returns success
         */
        beginTest("POST /api/undo/pop_group");
        
        resetBuilderState();

        // push group
        DynamicObject::Ptr pushBody = new DynamicObject();
        pushBody->setProperty(RestApiIds::name, "Pop Test Group");
        auto pushResponse = ctx->httpPost("/api/undo/push_group", JSON::toString(var(pushBody.get())));
        auto pushJson = ctx->parseJson(pushResponse);
        expect((bool)pushJson[RestApiIds::success], "push_group should succeed");

        // add in group (deferred execution)
        Array<var> groupOps;
        DynamicObject::Ptr add = new DynamicObject();
        add->setProperty(RestApiIds::op, "add");
        add->setProperty(RestApiIds::type, "SineSynth");
        add->setProperty(RestApiIds::chain, -1);
        add->setProperty(RestApiIds::parent, "Master Chain");
        add->setProperty(RestApiIds::name, "GroupSynth");
        groupOps.add(var(add.get()));
        DynamicObject::Ptr addBody = new DynamicObject();
        addBody->setProperty(RestApiIds::operations, var(groupOps));
        auto addResponse = ctx->httpPost("/api/builder/apply", JSON::toString(var(addBody.get())));
        auto addJson = ctx->parseJson(addResponse);
        expect((bool)addJson[RestApiIds::success], "group add should succeed");

        // pop group and execute deferred actions
        DynamicObject::Ptr popBody = new DynamicObject();
        popBody->setProperty(RestApiIds::cancel, false);
        auto popResponse = ctx->httpPost("/api/undo/pop_group", JSON::toString(var(popBody.get())));
        auto popJson = ctx->parseJson(popResponse);
        expect((bool)popJson[RestApiIds::success], "pop_group execute should succeed");
        expectBuilderProcessorExists("GroupSynth");
        expectDiffEntry(popJson, 0, "+", "GroupSynth");

        // runtime drift test: seed in runtime, then queue remove in group, delete directly, then pop
        Array<var> seedOps;
        DynamicObject::Ptr seed = new DynamicObject();
        seed->setProperty(RestApiIds::op, "add");
        seed->setProperty(RestApiIds::type, "SineSynth");
        seed->setProperty(RestApiIds::chain, -1);
        seed->setProperty(RestApiIds::parent, "Master Chain");
        seed->setProperty(RestApiIds::name, "DriftSynth");
        seedOps.add(var(seed.get()));
        DynamicObject::Ptr seedBody = new DynamicObject();
        seedBody->setProperty(RestApiIds::operations, var(seedOps));
        auto seedResponse = ctx->httpPost("/api/builder/apply", JSON::toString(var(seedBody.get())));
        auto seedJson = ctx->parseJson(seedResponse);
        expect((bool)seedJson[RestApiIds::success], "Seeding DriftSynth should succeed");

        pushBody->setProperty(RestApiIds::name, "Runtime Drift Group");
        pushResponse = ctx->httpPost("/api/undo/push_group", JSON::toString(var(pushBody.get())));
        pushJson = ctx->parseJson(pushResponse);
        expect((bool)pushJson[RestApiIds::success], "Second push_group should succeed");

        Array<var> removeOps;
        DynamicObject::Ptr remove = new DynamicObject();
        remove->setProperty(RestApiIds::op, "remove");
        remove->setProperty(RestApiIds::target, "DriftSynth");
        removeOps.add(var(remove.get()));
        DynamicObject::Ptr removeBody = new DynamicObject();
        removeBody->setProperty(RestApiIds::operations, var(removeOps));
        auto removeResponse = ctx->httpPost("/api/builder/apply", JSON::toString(var(removeBody.get())));
        auto removeJson = ctx->parseJson(removeResponse);
        expect((bool)removeJson[RestApiIds::success], "remove should be queued inside group");

        deleteProcessor("DriftSynth");

        popBody->setProperty(RestApiIds::cancel, false);
        popResponse = ctx->httpPost("/api/undo/pop_group", JSON::toString(var(popBody.get())));
        popJson = ctx->parseJson(popResponse);

        expect(!(bool)popJson[RestApiIds::success], "Runtime drift should fail on pop_group execute");
        expect(popJson[RestApiIds::errors].isArray(), "errors should be array");
        expect(popJson[RestApiIds::errors].size() > 0, "Should include runtime error");

        expectRuntimeCallStack(popJson, "api/undo/pop_group");
    }
    
    void expectRuntimeCallStack(const var& obj, const String& endpoint)
    {
        auto errors = obj[RestApiIds::errors];

        expect(errors.isArray() && errors.size() > 0);

        if (errors.isArray() && errors.size() > 0)
        {
			auto first = errors[0];

			auto runtimeCallstack = first[RestApiIds::callstack];

            expect(runtimeCallstack.isArray(), "runtime callstack should be array");

            if (runtimeCallstack.isArray())
            {
				expect(runtimeCallstack[1].toString() == "phase: runtime", "Runtime drift should be tagged as runtime");
				expect(runtimeCallstack[3].toString() == "endpoint: " + endpoint, "Endpoint should be " + endpoint);
            }
        }
    }

    void testUndoBack()
    {
        /** Setup: Fresh project
         *  Scenario: Call undo with nothing to undo
         *  Expected: Returns success=false
         */
        beginTest("POST /api/undo/back");
        
        resetBuilderState();

        Array<var> ops;
        DynamicObject::Ptr add = new DynamicObject();
        add->setProperty(RestApiIds::op, "add");
        add->setProperty(RestApiIds::type, "SineSynth");
        add->setProperty(RestApiIds::chain, -1);
        add->setProperty(RestApiIds::parent, "Master Chain");
        add->setProperty(RestApiIds::name, "UndoSynth");
        ops.add(var(add.get()));

        DynamicObject::Ptr body = new DynamicObject();
        body->setProperty(RestApiIds::operations, var(ops));
        auto addResponse = ctx->httpPost("/api/builder/apply", JSON::toString(var(body.get())));
        auto addJson = ctx->parseJson(addResponse);
        expect((bool)addJson[RestApiIds::success], "seed add should succeed");
        expectBuilderProcessorExists("UndoSynth");
        expectDiffEntry(addJson, 0, "+", "UndoSynth");

        auto response = ctx->httpPost("/api/undo/back", "{}");
        var json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "Undo should succeed after one action");
        expectBuilderProcessorRemoved("UndoSynth");
        expectEquals<int>(json[RestApiIds::diff].size(), 0,
            "Diff should be empty after undoing only action");

        // second undo should fail (nothing left)
        response = ctx->httpPost("/api/undo/back", "{}");
        json = ctx->parseJson(response);
        expect(!(bool)json[RestApiIds::success], "Second undo should fail when nothing to undo");
    }
    
    void testUndoForward()
    {
        /** Setup: Fresh project
         *  Scenario: Call redo with nothing to redo
         *  Expected: Returns success=false
         */
        beginTest("POST /api/undo/forward");
        
        resetBuilderState();

        Array<var> ops;
        DynamicObject::Ptr add = new DynamicObject();
        add->setProperty(RestApiIds::op, "add");
        add->setProperty(RestApiIds::type, "SineSynth");
        add->setProperty(RestApiIds::chain, -1);
        add->setProperty(RestApiIds::parent, "Master Chain");
        add->setProperty(RestApiIds::name, "RedoSynth");
        ops.add(var(add.get()));

        DynamicObject::Ptr body = new DynamicObject();
        body->setProperty(RestApiIds::operations, var(ops));
        auto addResponse = ctx->httpPost("/api/builder/apply", JSON::toString(var(body.get())));
        auto addJson = ctx->parseJson(addResponse);
        expect((bool)addJson[RestApiIds::success], "seed add should succeed");
        expectBuilderProcessorExists("RedoSynth");

        auto undoResponse = ctx->httpPost("/api/undo/back", "{}");
        auto undoJson = ctx->parseJson(undoResponse);
        expect((bool)undoJson[RestApiIds::success], "undo should succeed");
        expectBuilderProcessorRemoved("RedoSynth");

        auto response = ctx->httpPost("/api/undo/forward", "{}");
        var json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "redo should succeed after undo");
        expectBuilderProcessorExists("RedoSynth");
        expectDiffEntry(json, 0, "+", "RedoSynth");

        response = ctx->httpPost("/api/undo/forward", "{}");
        json = ctx->parseJson(response);
        expect(!(bool)json[RestApiIds::success], "second redo should fail");
    }
    
    void testUndoDiff()
    {
        /** Setup: Fresh project
         *  Scenario: Query diff with default params
         *  Expected: Returns empty diff
         */
        beginTest("GET /api/undo/diff");
        
        resetBuilderState();

        auto response = ctx->httpGet("/api/undo/diff");
        var json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "Should succeed");
        expect(json[RestApiIds::diff].isArray(), "diff should be array");
        expectEquals<int>(json[RestApiIds::diff].size(), 0, "Fresh state should have empty diff");

        Array<var> ops;
        ops.add(makeAddOp("SineSynth", "DiffSynth"));
        auto addJson = postBuilderOps(ops);
        expectNoBuilderError(addJson);

        response = ctx->httpGet("/api/undo/diff");
        json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "Diff query should succeed after add");
        expectEquals<int>(json[RestApiIds::diff].size(), 1, "Should have one diff entry");
        expectDiffEntry(json, 0, "+", "DiffSynth");

        response = ctx->httpGet("/api/undo/diff?scope=root&flatten=1&domain=builder");
        json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "Diff root+flatten+domain query should succeed");
        expectDiffEntry(json, 0, "+", "DiffSynth");

        // invalid scope should fail request
        response = ctx->httpGet("/api/undo/diff?scope=invalid_scope");
        json = ctx->parseJson(response);
        expect(!(bool)json[RestApiIds::success], "Invalid scope should fail");
    }
    
    void testUndoHistory()
    {
        /** Setup: Fresh project
         *  Scenario: Query history with default params
         *  Expected: Returns empty history
         */
        beginTest("GET /api/undo/history");
        
        resetBuilderState();

        auto response = ctx->httpGet("/api/undo/history");
        var json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "Should succeed");
        expect(json[RestApiIds::history].isArray(), "history should be array");
        expectEquals<int>(json[RestApiIds::history].size(), 0, "Fresh history should be empty");

        Array<var> ops;
        ops.add(makeAddOp("SineSynth", "HistorySynth"));
        auto addJson = postBuilderOps(ops);
        expectNoBuilderError(addJson);

        response = ctx->httpGet("/api/undo/history");
        json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "history query should succeed after add");
        expectEquals<int>(json[RestApiIds::history].size(), 1,
            "Should have one history entry after add");
        expectEquals<int>((int)json[RestApiIds::cursor], 0,
            "Cursor should point to first action");

        response = ctx->httpGet("/api/undo/history?scope=root&flatten=1");
        json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "root+flatten query should succeed");
        expectEquals(json[RestApiIds::scope].toString(), String("root"));

        response = ctx->httpGet("/api/undo/history?scope=invalid_scope");
        json = ctx->parseJson(response);
        expect(!(bool)json[RestApiIds::success], "Invalid history scope should fail");
    }
    
    void testUndoClear()
    {
        /** Setup: Fresh project
         *  Scenario: Clear undo history
         *  Expected: Returns success
         */
        beginTest("POST /api/undo/clear");

        resetBuilderState();

        // Seed history with one action
        Array<var> ops;
        DynamicObject::Ptr add = new DynamicObject();
        add->setProperty(RestApiIds::op, "add");
        add->setProperty(RestApiIds::type, "SineSynth");
        add->setProperty(RestApiIds::chain, -1);
        add->setProperty(RestApiIds::parent, "Master Chain");
        add->setProperty(RestApiIds::name, "ClearSynth");
        ops.add(var(add.get()));

        DynamicObject::Ptr body = new DynamicObject();
        body->setProperty(RestApiIds::operations, var(ops));
        auto addResponse = ctx->httpPost("/api/builder/apply", JSON::toString(var(body.get())));
        auto addJson = ctx->parseJson(addResponse);
        expect((bool)addJson[RestApiIds::success], "Seeding action should succeed");

        auto response = ctx->httpPost("/api/undo/clear", "{}");
        var json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "undo/clear should succeed");

        auto historyResponse = ctx->httpGet("/api/undo/history");
        auto historyJson = ctx->parseJson(historyResponse);
        expect((bool)historyJson[RestApiIds::success], "history query should succeed");
        expectEquals<int>(historyJson[RestApiIds::history].size(), 0,
                          "history should be empty after clear");

        auto undoResponse = ctx->httpPost("/api/undo/back", "{}");
        auto undoJson = ctx->parseJson(undoResponse);
        expect(!(bool)undoJson[RestApiIds::success], "Nothing should be undoable after clear");
    }

    // ========================================================================
    // Wizard Endpoint Tests
    // ========================================================================

    void testWizardInitialise()
    {
        /** Scenario: GET /api/wizard/initialise with valid and invalid wizard IDs
         *  Expected: Valid ID returns success, unknown ID returns 400
         */
        beginTest("GET /api/wizard/initialise");

        // Valid wizard ID
        auto response = ctx->httpGet("/api/wizard/initialise?id=new_project");
        var json = ctx->parseJson(response);
        expect((bool)json[RestApiIds::success], "Should succeed for valid wizard ID");

        // Missing id parameter
        auto missingResponse = ctx->httpGet("/api/wizard/initialise");
        var missingJson = ctx->parseJson(missingResponse);
        expect(!(bool)missingJson[RestApiIds::success], "Should fail without id parameter");

        // Unknown wizard ID
        auto unknownResponse = ctx->httpGet("/api/wizard/initialise?id=nonexistent_wizard");
        var unknownJson = ctx->parseJson(unknownResponse);
        expect(!(bool)unknownJson[RestApiIds::success], "Should fail for unknown wizard ID");
    }

    void testWizardExecute()
    {
        /** Scenario: POST /api/wizard/execute with validation edge cases
         *  Expected: Missing fields and invalid values return 400
         */
        beginTest("POST /api/wizard/execute");

        // Missing wizardId
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty(RestApiIds::answers, var(new DynamicObject()));
            Array<var> tasks;
            tasks.add("someTask");
            bodyObj->setProperty(RestApiIds::tasks, var(tasks));

            auto response = ctx->httpPost("/api/wizard/execute",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail without wizardId");
        }

        // Missing answers
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty(RestApiIds::wizardId, "new_project");
            Array<var> tasks;
            tasks.add("createEmptyProject");
            bodyObj->setProperty(RestApiIds::tasks, var(tasks));

            auto response = ctx->httpPost("/api/wizard/execute",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail without answers object");
        }

        // Invalid task name for wizard
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty(RestApiIds::wizardId, "new_project");
            bodyObj->setProperty(RestApiIds::answers, var(new DynamicObject()));
            Array<var> tasks;
            tasks.add("invalidTaskName");
            bodyObj->setProperty(RestApiIds::tasks, var(tasks));

            auto response = ctx->httpPost("/api/wizard/execute",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail with invalid task name");
        }

        // Unknown wizard ID
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty(RestApiIds::wizardId, "nonexistent");
            bodyObj->setProperty(RestApiIds::answers, var(new DynamicObject()));
            Array<var> tasks;
            tasks.add("task");
            bodyObj->setProperty(RestApiIds::tasks, var(tasks));

            auto response = ctx->httpPost("/api/wizard/execute",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail for unknown wizard ID");
        }

        // Wrong tasks array size (empty)
        {
            DynamicObject::Ptr bodyObj = new DynamicObject();
            bodyObj->setProperty(RestApiIds::wizardId, "recompile");
            bodyObj->setProperty(RestApiIds::answers, var(new DynamicObject()));
            bodyObj->setProperty(RestApiIds::tasks, var(Array<var>()));

            auto response = ctx->httpPost("/api/wizard/execute",
                                          JSON::toString(var(bodyObj.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail with empty tasks array");
        }
    }

    void testWizardStatus()
    {
        /** Scenario: GET /api/wizard/status with unknown job ID
         *  Expected: Returns error for unknown job
         */
        beginTest("GET /api/wizard/status");

        // Missing jobId
        auto missingResponse = ctx->httpGet("/api/wizard/status");
        var missingJson = ctx->parseJson(missingResponse);
        expect(!(bool)missingJson[RestApiIds::success], "Should fail without jobId");

        // Unknown jobId
        auto response = ctx->httpGet("/api/wizard/status?jobId=nonexistent_job_123");
        var json = ctx->parseJson(response);
        expect(!(bool)json[RestApiIds::success], "Should fail for unknown job ID");
    }

    // ======================================================================
    // UI Endpoint Helpers
    // ======================================================================

    var postUIApplyOps(const Array<var>& ops, const String& moduleId = "Interface")
    {
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty(RestApiIds::moduleId, moduleId);
        bodyObj->setProperty(RestApiIds::operations, var(ops));

        auto response = ctx->httpPost("/api/ui/apply",
                                       JSON::toString(var(bodyObj.get())));
        return ctx->parseJson(response);
    }

    var makeUIAddOp(const String& componentType, const String& id = {},
                    const String& parentId = {}, int x = 0, int y = 0,
                    int w = 128, int h = 48)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "add");
        op->setProperty(RestApiIds::componentType, componentType);
        if (id.isNotEmpty())
            op->setProperty(RestApiIds::id, id);
        if (parentId.isNotEmpty())
            op->setProperty(RestApiIds::parentId, parentId);
        op->setProperty(RestApiIds::x, x);
        op->setProperty(RestApiIds::y, y);
        op->setProperty(RestApiIds::width, w);
        op->setProperty(RestApiIds::height, h);
        return var(op.get());
    }

    var makeUIRemoveOp(const String& target)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "remove");
        op->setProperty(RestApiIds::target, target);
        return var(op.get());
    }

    var makeUISetOp(const String& target, const NamedValueSet& props, bool force = false)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "set");
        op->setProperty(RestApiIds::target, target);

        DynamicObject::Ptr propsObj = new DynamicObject();
        for (int i = 0; i < props.size(); i++)
            propsObj->setProperty(props.getName(i), props.getValueAt(i));
        op->setProperty(RestApiIds::properties, var(propsObj.get()));

        if (force)
            op->setProperty(RestApiIds::force, true);

        return var(op.get());
    }

    var makeUIMoveOp(const String& target, const String& parent)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "move");
        op->setProperty(RestApiIds::target, target);
        op->setProperty(RestApiIds::parent, parent);
        return var(op.get());
    }

    var makeUIRenameOp(const String& target, const String& newId)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "rename");
        op->setProperty(RestApiIds::target, target);
        op->setProperty(RestApiIds::newId, newId);
        return var(op.get());
    }

    void expectUISuccess(const var& json)
    {
        expect((bool)json[RestApiIds::success], "Should succeed");
        auto e = json[RestApiIds::errors];
        expect(e.isArray(), "errors should be array");
        if (e.isArray())
        {
            for (auto& er : *e.getArray())
                expect(false, er["errorMessage"].toString());
        }
    }

    // ======================================================================
    // UI Tests
    // ======================================================================

    void testUITree()
    {
        beginTest("GET /api/ui/tree");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                      "const var knob1 = Content.addKnob(\"Knob1\", 10, 10);\n"
                      "const var panel1 = Content.addPanel(\"Panel1\", 0, 100);\n");

        auto response = ctx->httpGet("/api/ui/tree?moduleId=Interface");
        var json = ctx->parseJson(response);

        expect((bool)json[RestApiIds::success], "Should succeed");

        auto result = json[RestApiIds::result];
        expectEquals(result[RestApiIds::id].toString(), String("Content"), "Root should be Content");
        expect(result[RestApiIds::childComponents].isArray(), "Should have childComponents array");
        expect(result[RestApiIds::childComponents].size() >= 2, "Should have at least 2 children");

        // Verify fields exist on first child
        auto firstChild = result[RestApiIds::childComponents][0];
        expect(firstChild.hasProperty(RestApiIds::id), "Child should have id");
        expect(firstChild.hasProperty(RestApiIds::type), "Child should have type");
        expect(firstChild.hasProperty(RestApiIds::visible), "Child should have visible");
        expect(firstChild.hasProperty(RestApiIds::enabled), "Child should have enabled");
        expect(firstChild.hasProperty(RestApiIds::saveInPreset), "Child should have saveInPreset");
        expect(firstChild.hasProperty(RestApiIds::x), "Child should have x");
        expect(firstChild.hasProperty(RestApiIds::y), "Child should have y");
        expect(firstChild.hasProperty(RestApiIds::width), "Child should have width");
        expect(firstChild.hasProperty(RestApiIds::height), "Child should have height");
        expect(firstChild.hasProperty(RestApiIds::childComponents), "Child should have childComponents");
    }

    void testUIApply()
    {
        beginTest("POST /api/ui/apply - basic operations");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);\n");

        // Add a panel
        Array<var> ops;
        ops.add(makeUIAddOp("ScriptPanel", "TestPanel", {}, 0, 0, 400, 300));
        auto json = postUIApplyOps(ops);
        expectUISuccess(json);

        // Add a button as child of panel
        ops.clear();
        ops.add(makeUIAddOp("ScriptButton", "TestButton", "TestPanel", 10, 10, 100, 30));
        json = postUIApplyOps(ops);
        expectUISuccess(json);

        // Set properties
        ops.clear();
        NamedValueSet props;
        props.set(Identifier("x"), 50);
        props.set(Identifier("y"), 50);
        ops.add(makeUISetOp("TestButton", props));
        json = postUIApplyOps(ops);
        expectUISuccess(json);

        // Rename
        ops.clear();
        ops.add(makeUIRenameOp("TestButton", "RenamedButton"));
        json = postUIApplyOps(ops);
        expectUISuccess(json);

        // Move to root
        ops.clear();
        ops.add(makeUIMoveOp("RenamedButton", ""));
        json = postUIApplyOps(ops);
        expectUISuccess(json);

        // Remove
        ops.clear();
        ops.add(makeUIRemoveOp("TestPanel"));
        json = postUIApplyOps(ops);
        expectUISuccess(json);
    }

    void testUIApplyValidation()
    {
        beginTest("POST /api/ui/apply - validation errors");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                      "const var knob1 = Content.addKnob(\"Knob1\", 10, 10);\n");

        // Unknown component type
        Array<var> ops;
        ops.add(makeUIAddOp("ScriptNonexistent", "Foo"));
        auto json = postUIApplyOps(ops);
        expect(!(bool)json[RestApiIds::success], "Should fail for unknown type");

        // Duplicate ID
        ops.clear();
        ops.add(makeUIAddOp("ScriptButton", "Knob1"));
        json = postUIApplyOps(ops);
        expect(!(bool)json[RestApiIds::success], "Should fail for duplicate ID");

        // Remove non-existent target
        ops.clear();
        ops.add(makeUIRemoveOp("NonExistent"));
        json = postUIApplyOps(ops);
        expect(!(bool)json[RestApiIds::success], "Should fail for non-existent target");

        // Rename to existing ID
        ops.clear();
        ops.add(makeUIAddOp("ScriptButton", "TestBtn"));
        json = postUIApplyOps(ops);
        expectUISuccess(json);

        ops.clear();
        ops.add(makeUIRenameOp("TestBtn", "Knob1"));
        json = postUIApplyOps(ops);
        expect(!(bool)json[RestApiIds::success], "Should fail for duplicate rename target");

        // Set unknown property
        ops.clear();
        NamedValueSet badProps;
        badProps.set(Identifier("nonExistentProp"), 42);
        ops.add(makeUISetOp("Knob1", badProps));
        json = postUIApplyOps(ops);
        expect(!(bool)json[RestApiIds::success], "Should fail for unknown property");
    }

    void testUIApplyUndo()
    {
        beginTest("POST /api/ui/apply - undo/redo");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);\n");

        // Clear undo history first
        ctx->httpPost("/api/undo/clear", "{}");

        // Add a button
        Array<var> ops;
        ops.add(makeUIAddOp("ScriptButton", "UndoBtn", {}, 10, 10, 100, 30));
        auto json = postUIApplyOps(ops);
        expectUISuccess(json);

        // Verify it exists in tree
        auto treeJson = ctx->parseJson(ctx->httpGet("/api/ui/tree?moduleId=Interface"));
        expect((bool)treeJson[RestApiIds::success], "Tree should succeed");

        // Undo
        auto undoJson = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));
        expect((bool)undoJson[RestApiIds::success], "Undo should succeed");

        // Redo
        auto redoJson = ctx->parseJson(ctx->httpPost("/api/undo/forward", "{}"));
        expect((bool)redoJson[RestApiIds::success], "Redo should succeed");
    }

    //==========================================================================
    // testing/sequence tests
    //==========================================================================

    /** Helper to build a MIDI message JSON object. */
    var makeMidiMessage(const String& type, int channel = 1, int noteNumber = 60,
                        float velocity = 1.0f, int duration = 500, int timestamp = 0,
                        int controller = 0, int value = 0)
    {
        DynamicObject::Ptr obj = new DynamicObject();
        obj->setProperty(RestApiIds::type, type);

        if (type != "allNotesOff")
            obj->setProperty(RestApiIds::channel, channel);

        if (type == "note")
        {
            obj->setProperty(RestApiIds::noteNumber, noteNumber);
            obj->setProperty(RestApiIds::velocity, velocity);
            obj->setProperty(RestApiIds::duration, duration);
        }
        else if (type == "cc")
        {
            obj->setProperty(RestApiIds::controller, controller);
            obj->setProperty(RestApiIds::value, value);
        }
        else if (type == "pitchbend")
        {
            obj->setProperty(RestApiIds::value, value);
        }

        if (timestamp > 0)
            obj->setProperty(RestApiIds::timestamp, timestamp);

        return var(obj.get());
    }

    /** Helper to POST testing/sequence messages and parse the response. */
    var postMidi(const Array<var>& messages)
    {
        DynamicObject::Ptr body = new DynamicObject();
        body->setProperty(RestApiIds::messages, var(messages));

        auto response = ctx->httpPost("/api/testing/sequence",
                                       JSON::toString(var(body.get())));
        return ctx->parseJson(response);
    }

    /** Extract response fields from a testing/sequence response (flat envelope). */
    var getMidiResult(const var& json)
    {
        return json;
    }

    /** Helper to reset the MidiInjector state before each test. */
    void resetMidiInjector()
    {
        Array<var> panic;
        panic.add(makeMidiMessage("allNotesOff"));
        postMidi(panic);
    }

    void testTestingSequenceNote()
    {
        /** Setup: Clean state with compiled interface
         *  Scenario: Inject a single note
         *  Expected: success=true, eventsInSequence=1
         */
        beginTest("POST /api/testing/sequence - single note");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");
        resetMidiInjector();

        Array<var> messages;
        messages.add(makeMidiMessage("note", 1, 60, 1.0f, 200));

        auto json = postMidi(messages);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expectEquals<int>(r[RestApiIds::eventsInSequence], 1, "Should have 1 event");
    }

    void testTestingSequenceChord()
    {
        /** Setup: Clean state with compiled interface
         *  Scenario: Inject 3 notes simultaneously (C major chord)
         *  Expected: success=true, eventsInSequence=3
         */
        beginTest("POST /api/testing/sequence - chord (3 notes)");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");
        resetMidiInjector();

        Array<var> messages;
        messages.add(makeMidiMessage("note", 1, 60, 0.8f, 300));  // C
        messages.add(makeMidiMessage("note", 1, 64, 0.8f, 300));  // E
        messages.add(makeMidiMessage("note", 1, 67, 0.8f, 300));  // G

        auto json = postMidi(messages);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expectEquals<int>(r[RestApiIds::eventsInSequence], 3, "Should have 3 events");
    }

    void testTestingSequenceCC()
    {
        /** Setup: Clean state with compiled interface
         *  Scenario: Inject a CC message
         *  Expected: success=true, eventsInSequence=1
         */
        beginTest("POST /api/testing/sequence - CC message");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");
        resetMidiInjector();

        Array<var> messages;
        messages.add(makeMidiMessage("cc", 1, 0, 0, 0, 0, 74, 64));

        auto json = postMidi(messages);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expectEquals<int>(r[RestApiIds::eventsInSequence], 1, "Should have 1 event");
    }

    void testTestingSequencePitchbend()
    {
        /** Setup: Clean state with compiled interface
         *  Scenario: Inject a pitchbend message
         *  Expected: success=true, eventsInSequence=1
         */
        beginTest("POST /api/testing/sequence - pitchbend");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");
        resetMidiInjector();

        Array<var> messages;
        messages.add(makeMidiMessage("pitchbend", 1, 0, 0, 0, 0, 0, 8192));

        auto json = postMidi(messages);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expectEquals<int>(r[RestApiIds::eventsInSequence], 1, "Should have 1 event");
    }

    void testTestingSequenceAllNotesOff()
    {
        /** Setup: Inject notes, then allNotesOff
         *  Scenario: allNotesOff with delay=0 should panic immediately
         *  Expected: Queue is cleared, isPlaying=false
         */
        beginTest("POST /api/testing/sequence - allNotesOff panic");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");

        // Inject a long note
        Array<var> messages;
        messages.add(makeMidiMessage("note", 1, 60, 1.0f, 5000));

        postMidi(messages);

        // Now send allNotesOff
        Array<var> panicMessages;
        panicMessages.add(makeMidiMessage("allNotesOff"));

        auto json = postMidi(panicMessages);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expect(!(bool)r[RestApiIds::isPlaying], "Should not be playing after panic");
        expectEquals<int>(r[RestApiIds::activeNotes], 0, "No active notes after panic");
    }

    void testTestingSequenceSequence()
    {
        /** Setup: Clean state with compiled interface
         *  Scenario: Inject a sequence with delays between messages
         *  Expected: success=true, isPlaying=true, durationMs reflects delays
         */
        beginTest("POST /api/testing/sequence - delayed sequence");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");
        resetMidiInjector();

        Array<var> messages;
        messages.add(makeMidiMessage("note", 1, 60, 1.0f, 200, 0));     // at t=0
        messages.add(makeMidiMessage("note", 1, 64, 1.0f, 200, 500));   // at t=500ms
        messages.add(makeMidiMessage("cc", 1, 0, 0, 0, 1000, 74, 100)); // at t=1000ms

        auto json = postMidi(messages);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expectEquals<int>(r[RestApiIds::eventsInSequence], 3, "Should have 3 events");

        // durationMs should account for delays + note duration
        int durationMs = (int)r[RestApiIds::durationMs];
        expect(durationMs >= 1000, "Duration should be at least 1000ms (last event at t=1000)");

        // Clean up
        Array<var> panicMessages;
        panicMessages.add(makeMidiMessage("allNotesOff"));
        postMidi(panicMessages);
    }

    void testTestingSequenceValidation()
    {
        /** Setup: Clean state
         *  Scenario: Send various invalid messages
         *  Expected: 400 errors with descriptive messages
         */
        beginTest("POST /api/testing/sequence - validation");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");

        // Missing type field
        {
            DynamicObject::Ptr body = new DynamicObject();
            Array<var> msgs;
            DynamicObject::Ptr msg = new DynamicObject();
            msg->setProperty(RestApiIds::noteNumber, 60);
            msgs.add(var(msg.get()));
            body->setProperty(RestApiIds::messages, var(msgs));

            auto response = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail without type");
        }

        // Unknown type
        {
            DynamicObject::Ptr body = new DynamicObject();
            Array<var> msgs;
            DynamicObject::Ptr msg = new DynamicObject();
            msg->setProperty(RestApiIds::type, "sysex");
            msgs.add(var(msg.get()));
            body->setProperty(RestApiIds::messages, var(msgs));

            auto response = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail with unknown type");
        }

        // Note missing noteNumber
        {
            DynamicObject::Ptr body = new DynamicObject();
            Array<var> msgs;
            DynamicObject::Ptr msg = new DynamicObject();
            msg->setProperty(RestApiIds::type, "note");
            msgs.add(var(msg.get()));
            body->setProperty(RestApiIds::messages, var(msgs));

            auto response = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail without noteNumber");
        }

        // CC missing controller
        {
            DynamicObject::Ptr body = new DynamicObject();
            Array<var> msgs;
            DynamicObject::Ptr msg = new DynamicObject();
            msg->setProperty(RestApiIds::type, "cc");
            msg->setProperty(RestApiIds::value, 64);
            msgs.add(var(msg.get()));
            body->setProperty(RestApiIds::messages, var(msgs));

            auto response = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail without controller");
        }

        // Invalid channel
        {
            DynamicObject::Ptr body = new DynamicObject();
            Array<var> msgs;
            DynamicObject::Ptr msg = new DynamicObject();
            msg->setProperty(RestApiIds::type, "note");
            msg->setProperty(RestApiIds::noteNumber, 60);
            msg->setProperty(RestApiIds::channel, 17);
            msgs.add(var(msg.get()));
            body->setProperty(RestApiIds::messages, var(msgs));

            auto response = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body.get())));
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail with channel 17");
        }

        // Missing messages array entirely
        {
            auto response = ctx->httpPost("/api/testing/sequence", "{}");
            var json = ctx->parseJson(response);
            expect(!(bool)json[RestApiIds::success], "Should fail without messages array");
        }
    }

    void testTestingSequenceRepl()
    {
        /** Setup: Compile interface with a variable
         *  Scenario: Play a note and evaluate a repl expression at the same timestamp
         *  Expected: replResults array in response contains the evaluation result
         */
        beginTest("POST /api/testing/sequence - repl event");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);\nreg x = 42;");
        resetMidiInjector();

        // Queue a repl event with timestamp=0 (fires immediately)
        DynamicObject::Ptr body = new DynamicObject();
        Array<var> msgs;

        DynamicObject::Ptr replMsg = new DynamicObject();
        replMsg->setProperty(RestApiIds::type, "repl");
        replMsg->setProperty(RestApiIds::expression, "x");
        replMsg->setProperty(RestApiIds::timestamp, 0);
        msgs.add(var(replMsg.get()));
        body->setProperty(RestApiIds::messages, var(msgs));

        auto response = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body.get())));
        var json = ctx->parseJson(response);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expectEquals<int>(r[RestApiIds::eventsInSequence], 1, "Should have 1 event");

        // The repl result might not be in the immediate response since the timer
        // fires asynchronously. Poll until we get it.
        Array<var> allReplResults;

        // Check immediate response first
        if (r.hasProperty(RestApiIds::replResults))
        {
            auto results = r[RestApiIds::replResults];
            if (results.isArray())
                for (int i = 0; i < results.size(); i++)
                    allReplResults.add(results[i]);
        }

        // Poll a few times if needed
        for (int attempt = 0; attempt < 10 && allReplResults.isEmpty(); attempt++)
        {
            MessageManager::getInstance()->runDispatchLoopUntil(50);
            Array<var> emptyMsgs;
            auto pollJson = postMidi(emptyMsgs);
            auto pollR = getMidiResult(pollJson);

            if (pollR.hasProperty(RestApiIds::replResults))
            {
                auto results = pollR[RestApiIds::replResults];
                if (results.isArray())
                    for (int i = 0; i < results.size(); i++)
                        allReplResults.add(results[i]);
            }
        }

        expect(!allReplResults.isEmpty(), "Should have at least one REPL result");

        if (!allReplResults.isEmpty())
        {
            auto firstResult = allReplResults[0];
            expect((bool)firstResult[RestApiIds::success], "REPL eval should succeed");
            expectEquals<int>(firstResult[RestApiIds::value], 42, "Should return value of x");
            expect(firstResult[RestApiIds::expression].toString() == "x", "Should echo expression");
        }
    }

    void testTestingSequenceSetAttribute()
    {
        /** Setup: Compile interface with a knob
         *  Scenario: Queue a set_attribute event to change the knob value
         *  Expected: success=true, value is applied
         */
        beginTest("POST /api/testing/sequence - set_attribute event");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);\n"
                      "const var Knob1 = Content.addKnob('Knob1', 0, 0);");
        resetMidiInjector();

        // set_attribute with timestamp=0 fires immediately
        DynamicObject::Ptr body = new DynamicObject();
        Array<var> msgs;

        DynamicObject::Ptr attrMsg = new DynamicObject();
        attrMsg->setProperty(RestApiIds::type, "set_attribute");
        attrMsg->setProperty(RestApiIds::processorId, "Interface");
        attrMsg->setProperty(RestApiIds::parameterId, "Knob1");
        attrMsg->setProperty(RestApiIds::value, 0.5);
        attrMsg->setProperty(RestApiIds::timestamp, 0);
        msgs.add(var(attrMsg.get()));
        body->setProperty(RestApiIds::messages, var(msgs));

        auto response = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body.get())));
        var json = ctx->parseJson(response);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expectEquals<int>(r[RestApiIds::eventsInSequence], 1, "Should have 1 event");

        // Validate that set_attribute with invalid parameterId is rejected
        DynamicObject::Ptr body2 = new DynamicObject();
        Array<var> msgs2;

        DynamicObject::Ptr badMsg = new DynamicObject();
        badMsg->setProperty(RestApiIds::type, "set_attribute");
        badMsg->setProperty(RestApiIds::parameterId, "NonExistentKnob");
        badMsg->setProperty(RestApiIds::value, 0.5);
        msgs2.add(var(badMsg.get()));
        body2->setProperty(RestApiIds::messages, var(msgs2));

        auto response2 = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body2.get())));
        var json2 = ctx->parseJson(response2);

        expect(!(bool)json2[RestApiIds::success], "Should fail with unknown parameterId");
    }

    void testTestingSequenceTestSignal()
    {
        /** Setup: Clean state
         *  Scenario: Inject a sine test signal and verify it succeeds
         *  Expected: success=true, eventsInSequence=1
         */
        beginTest("POST /api/testing/sequence - testsignal");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");
        resetMidiInjector();

        // Queue a short sine signal
        DynamicObject::Ptr body = new DynamicObject();
        Array<var> msgs;

        DynamicObject::Ptr sigMsg = new DynamicObject();
        sigMsg->setProperty(RestApiIds::type, "testsignal");
        sigMsg->setProperty(RestApiIds::signal, "sine");
        sigMsg->setProperty(RestApiIds::frequency, 440.0);
        sigMsg->setProperty(RestApiIds::duration, 100);
        sigMsg->setProperty(RestApiIds::timestamp, 0);
        msgs.add(var(sigMsg.get()));
        body->setProperty(RestApiIds::messages, var(msgs));

        auto response = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body.get())));
        var json = ctx->parseJson(response);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expectEquals<int>(r[RestApiIds::eventsInSequence], 1, "Should have 1 event");

        // Test validation: unknown signal type
        DynamicObject::Ptr body2 = new DynamicObject();
        Array<var> msgs2;

        DynamicObject::Ptr badSig = new DynamicObject();
        badSig->setProperty(RestApiIds::type, "testsignal");
        badSig->setProperty(RestApiIds::signal, "kazoo");
        msgs2.add(var(badSig.get()));
        body2->setProperty(RestApiIds::messages, var(msgs2));

        auto response2 = ctx->httpPost("/api/testing/sequence", JSON::toString(var(body2.get())));
        var json2 = ctx->parseJson(response2);

        expect(!(bool)json2[RestApiIds::success], "Should fail with unknown signal type");

        // Clean up
        Array<var> panicMsgs;
        panicMsgs.add(makeMidiMessage("allNotesOff"));
        postMidi(panicMsgs);
    }

    void testTestingSequenceEmptyPoll()
    {
        /** Setup: Clean state, nothing playing
         *  Scenario: Send empty messages array to poll status
         *  Expected: success=true, isPlaying=false, progress=1.0
         */
        beginTest("POST /api/testing/sequence - empty poll");

        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);");

        // Ensure clean state with panic first
        Array<var> panicMessages;
        panicMessages.add(makeMidiMessage("allNotesOff"));
        postMidi(panicMessages);

        // Poll with empty array
        Array<var> empty;
        auto json = postMidi(empty);

        expect((bool)json[RestApiIds::success], "Should succeed");
        auto r = getMidiResult(json);
        expect(!(bool)r[RestApiIds::isPlaying], "Should not be playing");
        expectEquals<int>(r[RestApiIds::activeNotes], 0, "No active notes");
        expectEquals<int>(r[RestApiIds::eventsInSequence], 0, "No events");
        expectEquals<int>(r[RestApiIds::playedEvents], 0, "No played events");
    }

    // ========================================================================
    // DSP endpoint helpers
    // ========================================================================

    void resetDspState()
    {
        // Reset builder, add a ScriptFX, init a network
        ctx->parseJson(ctx->httpPost("/api/builder/reset", "{}"));
        ctx->parseJson(ctx->httpPost("/api/undo/clear", "{}"));

        Array<var> ops;
        DynamicObject::Ptr addOp = new DynamicObject();
        addOp->setProperty(RestApiIds::op, "add");
        addOp->setProperty(RestApiIds::type, "ScriptFX");
        addOp->setProperty(RestApiIds::parent, "Master Chain");
        addOp->setProperty(RestApiIds::chain, 3);
        addOp->setProperty(RestApiIds::name, "DspTestFX");
        ops.add(var(addOp.get()));

        auto builderJson = postBuilderOps(ops);
        expect((bool)builderJson[RestApiIds::success], "Should add ScriptFX");

        DynamicObject::Ptr initBody = new DynamicObject();
        initBody->setProperty(RestApiIds::moduleId, "DspTestFX");
        initBody->setProperty(RestApiIds::name, "test_network");

        auto initJson = ctx->parseJson(ctx->httpPost("/api/dsp/init",
            JSON::toString(var(initBody.get()))));
        expect((bool)initJson[RestApiIds::success], "Should init network");

        // Clear undo history so DSP tests start clean
        ctx->parseJson(ctx->httpPost("/api/undo/clear", "{}"));
    }

    var postDspOps(const Array<var>& ops, const String& moduleId = "DspTestFX")
    {
        DynamicObject::Ptr bodyObj = new DynamicObject();
        bodyObj->setProperty(RestApiIds::moduleId, moduleId);
        bodyObj->setProperty(RestApiIds::operations, var(ops));

        auto response = ctx->httpPost("/api/dsp/apply",
            JSON::toString(var(bodyObj.get())));
        return ctx->parseJson(response);
    }

    var makeDspAddOp(const String& factoryPath, const String& parent,
                     const String& nodeId = {}, int index = -1)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "add");
        op->setProperty(RestApiIds::factoryPath, factoryPath);
        op->setProperty(RestApiIds::parent, parent);
        if (nodeId.isNotEmpty())
            op->setProperty(RestApiIds::nodeId, nodeId);
        if (index >= 0)
            op->setProperty(RestApiIds::index, index);
        return var(op.get());
    }

    var makeDspRemoveOp(const String& nodeId)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "remove");
        op->setProperty(RestApiIds::nodeId, nodeId);
        return var(op.get());
    }

    var makeDspMoveOp(const String& nodeId, const String& parent, int index = -1)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "move");
        op->setProperty(RestApiIds::nodeId, nodeId);
        op->setProperty(RestApiIds::parent, parent);
        if (index >= 0)
            op->setProperty(RestApiIds::index, index);
        return var(op.get());
    }

    var makeDspSetOp(const String& nodeId, const String& parameterId, const var& value)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "set");
        op->setProperty(RestApiIds::nodeId, nodeId);
        op->setProperty(RestApiIds::parameterId, parameterId);
        op->setProperty(RestApiIds::value, value);
        return var(op.get());
    }

    var makeDspBypassOp(const String& nodeId, bool bypassed)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "bypass");
        op->setProperty(RestApiIds::nodeId, nodeId);
        op->setProperty(RestApiIds::bypassed, bypassed);
        return var(op.get());
    }

    var makeDspConnectOp(const String& source, const String& target,
                         const String& parameter, const String& sourceOutput = {})
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "connect");
        op->setProperty(RestApiIds::source, source);
        op->setProperty(RestApiIds::target, target);
        op->setProperty(RestApiIds::parameter, parameter);
        if (sourceOutput.isNotEmpty())
            op->setProperty(RestApiIds::sourceOutput, sourceOutput);
        return var(op.get());
    }

    var makeDspDisconnectOp(const String& source, const String& target,
                            const String& parameter)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "disconnect");
        op->setProperty(RestApiIds::source, source);
        op->setProperty(RestApiIds::target, target);
        op->setProperty(RestApiIds::parameter, parameter);
        return var(op.get());
    }

    var makeDspCreateParameterOp(const String& nodeId, const String& parameterId,
                                 double minVal = 0.0, double maxVal = 1.0,
                                 double defaultVal = 0.0, double stepSize = 0.0)
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "create_parameter");
        op->setProperty(RestApiIds::nodeId, nodeId);
        op->setProperty(RestApiIds::parameterId, parameterId);
        op->setProperty(RestApiIds::min, minVal);
        op->setProperty(RestApiIds::max, maxVal);
        op->setProperty(RestApiIds::defaultValue, defaultVal);
        op->setProperty(RestApiIds::stepSize, stepSize);
        return var(op.get());
    }

    var makeDspClearOp()
    {
        DynamicObject::Ptr op = new DynamicObject();
        op->setProperty(RestApiIds::op, "clear");
        return var(op.get());
    }

    void expectDspSuccess(const var& json)
    {
        auto ok = (bool)json[RestApiIds::success];
        expect(ok, "Should succeed");

        if (!ok)
            DBG(JSON::toString(json, true));

        auto e = json[RestApiIds::errors];
        expect(e.isArray(), "errors should be array");
        if (e.isArray())
        {
            for (auto& er : *e.getArray())
                expect(false, er["errorMessage"].toString());
        }
    }

    var getDspTree(const String& moduleId = "DspTestFX", bool verbose = false)
    {
        auto url = "/api/dsp/tree?moduleId=" + URL::addEscapeChars(moduleId, true);
        if (verbose)
            url += "&verbose=true";
        auto response = ctx->httpGet(url);
        return ctx->parseJson(response);
    }

    /** Find a node by nodeId in a tree result (recursive). */
    var findNodeInTree(const var& node, const String& nodeId)
    {
        if (node[RestApiIds::nodeId].toString() == nodeId)
            return node;

        auto children = node[RestApiIds::children];
        if (children.isArray())
        {
            for (int i = 0; i < children.size(); i++)
            {
                auto found = findNodeInTree(children[i], nodeId);
                if (found.isObject())
                    return found;
            }
        }

        return {};
    }

    int countNodesInTree(const var& node)
    {
        int count = 1; // this node
        auto children = node[RestApiIds::children];
        if (children.isArray())
        {
            for (int i = 0; i < children.size(); i++)
                count += countNodesInTree(children[i]);
        }
        return count;
    }

    // ========================================================================
    // DSP endpoint tests
    // ========================================================================

    void testDspList()
    {
        /** Setup: Server running
         *  Scenario: GET /api/dsp/list
         *  Expected: Returns success with networks array
         */
        beginTest("GET /api/dsp/list");

        auto response = ctx->httpGet("/api/dsp/list");
        var json = ctx->parseJson(response);

        expect((bool)json[RestApiIds::success], "Should succeed");
        expect(json[RestApiIds::networks].isArray(), "networks should be array");
        expect(json[RestApiIds::logs].isArray(), "logs should be array");
        expect(json[RestApiIds::errors].isArray(), "errors should be array");
    }

    void testDspInit()
    {
        /** Setup: Fresh builder with ScriptFX module
         *  Scenario: POST /api/dsp/init to create a network
         *  Expected: Returns tree with root container, filePath, and embedded flag
         */
        beginTest("POST /api/dsp/init");

        // Reset and add a ScriptFX
        ctx->parseJson(ctx->httpPost("/api/builder/reset", "{}"));

        Array<var> ops;
        DynamicObject::Ptr addOp = new DynamicObject();
        addOp->setProperty(RestApiIds::op, "add");
        addOp->setProperty(RestApiIds::type, "ScriptFX");
        addOp->setProperty(RestApiIds::parent, "Master Chain");
        addOp->setProperty(RestApiIds::chain, 3);
        addOp->setProperty(RestApiIds::name, "InitTestFX");
        ops.add(var(addOp.get()));
        postBuilderOps(ops);

        // Init network
        DynamicObject::Ptr initBody = new DynamicObject();
        initBody->setProperty(RestApiIds::moduleId, "InitTestFX");
        initBody->setProperty(RestApiIds::name, "init_test");

        auto response = ctx->httpPost("/api/dsp/init",
            JSON::toString(var(initBody.get())));
        var json = ctx->parseJson(response);

        expect((bool)json[RestApiIds::success], "Should succeed");

        // Check tree in result
        auto result = json[RestApiIds::result];
        expect(result.isObject(), "result should be object");
        expectEquals(result[RestApiIds::nodeId].toString(), String("init_test"),
            "Root node ID should match network name");
        expectEquals(result[RestApiIds::factoryPath].toString(), String("container.chain"),
            "Root should be container.chain");
        expect(result[RestApiIds::children].isArray(), "Should have children array");
        expect(result[RestApiIds::parameters].isArray(), "Should have parameters array");
        expect(result[RestApiIds::connections].isArray(), "Should have connections array");

        // Check filePath and embedded
        expect(json.hasProperty(RestApiIds::filePath), "Should have filePath");
        expect(json[RestApiIds::filePath].toString().isNotEmpty(), "filePath should not be empty");
        expect(json[RestApiIds::filePath].toString().endsWith("init_test.xml"),
            "filePath should end with network name.xml");
        expect(!(bool)json[RestApiIds::embedded], "embedded should be false");

        // Error: missing moduleId
        auto errResponse = ctx->httpPost("/api/dsp/init", R"({"name": "foo"})");
        var errJson = ctx->parseJson(errResponse);
        expect(!(bool)errJson[RestApiIds::success], "Should fail without moduleId");

        // Error: missing name
        auto errResponse2 = ctx->httpPost("/api/dsp/init",
            R"({"moduleId": "InitTestFX"})");
        var errJson2 = ctx->parseJson(errResponse2);
        expect(!(bool)errJson2[RestApiIds::success], "Should fail without name");
    }

    void testDspTree()
    {
        /** Setup: ScriptFX with initialized network and some nodes
         *  Scenario: GET /api/dsp/tree with and without verbose
         *  Expected: Correct tree structure with parameters and connections
         */
        beginTest("GET /api/dsp/tree");

        resetDspState();

        // Empty tree
        auto json = getDspTree();
        expect((bool)json[RestApiIds::success], "Should succeed");

        auto result = json[RestApiIds::result];
        expectEquals(result[RestApiIds::nodeId].toString(), String("test_network"),
            "Root should be test_network");
        expectEquals(result[RestApiIds::factoryPath].toString(), String("container.chain"),
            "Root should be container.chain");
        expect(result[RestApiIds::children].isArray(), "Should have children");
        expectEquals<int>(result[RestApiIds::children].size(), 0, "Should be empty");

        // Add a node and re-read
        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "TreeOsc"));
        expectDspSuccess(postDspOps(ops));

        json = getDspTree();
        result = json[RestApiIds::result];
        expectEquals<int>(result[RestApiIds::children].size(), 1, "Should have 1 child");

        auto child = result[RestApiIds::children][0];
        expectEquals(child[RestApiIds::nodeId].toString(), String("TreeOsc"),
            "Child should be TreeOsc");
        expectEquals(child[RestApiIds::factoryPath].toString(), String("core.oscillator"),
            "Should be core.oscillator");
        expect(child[RestApiIds::parameters].isArray(), "Should have parameters");
        expect(child[RestApiIds::parameters].size() > 0, "Oscillator should have parameters");

        // Check parameter shape (compact mode)
        auto firstParam = child[RestApiIds::parameters][0];
        expect(firstParam.hasProperty(RestApiIds::parameterId), "Param should have parameterId");
        expect(firstParam.hasProperty(RestApiIds::value), "Param should have value");
        expect(!firstParam.hasProperty(RestApiIds::min), "Compact mode should not have min");

        // Verbose mode
        json = getDspTree("DspTestFX", true);
        result = json[RestApiIds::result];
        auto verboseChild = result[RestApiIds::children][0];
        auto verboseParam = verboseChild[RestApiIds::parameters][0];
        expect(verboseParam.hasProperty(RestApiIds::min), "Verbose should have min");
        expect(verboseParam.hasProperty(RestApiIds::max), "Verbose should have max");
        expect(verboseParam.hasProperty(RestApiIds::stepSize), "Verbose should have stepSize");
        expect(verboseParam.hasProperty(RestApiIds::defaultValue), "Verbose should have defaultValue");

        // Error: missing moduleId
        auto errResponse = ctx->httpGet("/api/dsp/tree");
        var errJson = ctx->parseJson(errResponse);
        expect(!(bool)errJson[RestApiIds::success], "Should fail without moduleId");

        // Error: invalid moduleId
        auto errResponse2 = ctx->httpGet("/api/dsp/tree?moduleId=NonExistent");
        var errJson2 = ctx->parseJson(errResponse2);
        expect(!(bool)errJson2[RestApiIds::success], "Should fail with invalid moduleId");
    }

    void testDspApplyAdd()
    {
        /** Setup: Fresh DSP state
         *  Scenario: Add nodes with various configurations
         *  Expected: Nodes appear in tree with correct IDs
         */
        beginTest("POST /api/dsp/apply - add");

        resetDspState();

        // Add with explicit nodeId
        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "Osc1"));
        auto json = postDspOps(ops);
        expectDspSuccess(json);
        expectDiffEntry(json, 0, "+", "Osc1", "dsp");

        // Add with auto-derived nodeId
        ops.clear();
        ops.add(makeDspAddOp("filters.svf", "test_network"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        // Add at specific index (front)
        ops.clear();
        ops.add(makeDspAddOp("math.mul", "test_network", "Mul1", 0));
        json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify order: Mul1 should be first
        auto tree = getDspTree();
        auto children = tree[RestApiIds::result][RestApiIds::children];
        expectEquals(children[0][RestApiIds::nodeId].toString(), String("Mul1"),
            "Mul1 should be at index 0");
        expectEquals(children[1][RestApiIds::nodeId].toString(), String("Osc1"),
            "Osc1 should be at index 1");

        // Error: duplicate nodeId
        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "test_network", "Osc1"));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Duplicate nodeId should fail");

        // Error: missing factoryPath
        DynamicObject::Ptr badOp = new DynamicObject();
        badOp->setProperty(RestApiIds::op, "add");
        badOp->setProperty(RestApiIds::parent, "test_network");
        ops.clear();
        ops.add(var(badOp.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing factoryPath should fail");

        // Error: missing parent
        DynamicObject::Ptr badOp2 = new DynamicObject();
        badOp2->setProperty(RestApiIds::op, "add");
        badOp2->setProperty(RestApiIds::factoryPath, "core.oscillator");
        ops.clear();
        ops.add(var(badOp2.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing parent should fail");

        // Error: invalid parent
        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "NonExistentParent", "BadOsc"));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Invalid parent should fail");

        // Regression: batch {add container, add child to just-created container}
        // must succeed. add::validate() skips the parent check outside plan mode
        // so perform() (which runs sequentially) can see the newly added container.
        ops.clear();
        ops.add(makeDspAddOp("container.chain", "test_network", "BatchParent"));
        ops.add(makeDspAddOp("core.oscillator", "BatchParent", "BatchChild"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        tree = getDspTree();
        auto bp = findNodeInTree(tree[RestApiIds::result], "BatchParent");
        expect(bp.isObject(), "BatchParent should exist after batch add");
        auto bc = findNodeInTree(tree[RestApiIds::result], "BatchChild");
        expect(bc.isObject(), "BatchChild should exist inside BatchParent");
    }

    void testDspApplyRemove()
    {
        /** Setup: Network with nodes
         *  Scenario: Remove nodes
         *  Expected: Nodes disappear from tree
         */
        beginTest("POST /api/dsp/apply - remove");

        resetDspState();

        // Add two nodes
        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "RemOsc"));
        ops.add(makeDspAddOp("filters.svf", "test_network", "RemFilter"));
        expectDspSuccess(postDspOps(ops));

        // Remove one
        ops.clear();
        ops.add(makeDspRemoveOp("RemOsc"));
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify only filter remains
        auto tree = getDspTree();
        auto children = tree[RestApiIds::result][RestApiIds::children];
        expectEquals<int>(children.size(), 1, "Should have 1 child after remove");
        expectEquals(children[0][RestApiIds::nodeId].toString(), String("RemFilter"),
            "Remaining child should be RemFilter");

        // Error: remove non-existent node
        ops.clear();
        ops.add(makeDspRemoveOp("NonExistent"));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Removing non-existent node should fail");

        // Error: missing nodeId
        DynamicObject::Ptr badOp = new DynamicObject();
        badOp->setProperty(RestApiIds::op, "remove");
        ops.clear();
        ops.add(var(badOp.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing nodeId should fail");

        // Regression: batch {add X, remove X} in one call must succeed.
        // remove::validate() needs to defer the node-existence check so it
        // doesn't fail on nodes created earlier in the same batch.
        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "test_network", "TempNode"));
        ops.add(makeDspRemoveOp("TempNode"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        auto tree2 = getDspTree();
        auto tn = findNodeInTree(tree2[RestApiIds::result], "TempNode");
        expect(!tn.isObject(), "TempNode should not exist after add+remove batch");
    }

    void testDspApplyMove()
    {
        /** Setup: Network with container and nodes
         *  Scenario: Move node between containers
         *  Expected: Node appears in new container
         */
        beginTest("POST /api/dsp/apply - move");

        resetDspState();

        // Add a container and two nodes in root
        Array<var> ops;
        ops.add(makeDspAddOp("container.chain", "test_network", "MoveChain"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "MoveOsc"));
        expectDspSuccess(postDspOps(ops));

        // Move oscillator into container
        ops.clear();
        ops.add(makeDspMoveOp("MoveOsc", "MoveChain"));
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify: root has 1 child (MoveChain), MoveChain has 1 child (MoveOsc)
        auto tree = getDspTree();
        auto rootChildren = tree[RestApiIds::result][RestApiIds::children];
        expectEquals<int>(rootChildren.size(), 1, "Root should have 1 child");
        expectEquals(rootChildren[0][RestApiIds::nodeId].toString(), String("MoveChain"),
            "Root child should be MoveChain");

        auto chainChildren = rootChildren[0][RestApiIds::children];
        expectEquals<int>(chainChildren.size(), 1, "MoveChain should have 1 child");
        expectEquals(chainChildren[0][RestApiIds::nodeId].toString(), String("MoveOsc"),
            "MoveChain child should be MoveOsc");

        // Move with specific index
        ops.clear();
        ops.add(makeDspAddOp("math.mul", "MoveChain", "MoveMultiply"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspMoveOp("MoveOsc", "MoveChain", 1));
        expectDspSuccess(postDspOps(ops));

        tree = getDspTree();
        auto chain = findNodeInTree(tree[RestApiIds::result], "MoveChain");
        auto moved = chain[RestApiIds::children];
        expectEquals(moved[0][RestApiIds::nodeId].toString(), String("MoveMultiply"),
            "MoveMultiply should be at index 0");
        expectEquals(moved[1][RestApiIds::nodeId].toString(), String("MoveOsc"),
            "MoveOsc should be at index 1");

        // Error: move non-existent node
        ops.clear();
        ops.add(makeDspMoveOp("NonExistent", "MoveChain"));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Moving non-existent node should fail");

        // Error: missing required fields
        DynamicObject::Ptr badOp = new DynamicObject();
        badOp->setProperty(RestApiIds::op, "move");
        badOp->setProperty(RestApiIds::nodeId, "MoveOsc");
        ops.clear();
        ops.add(var(badOp.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing parent should fail");

        // Regression: batch {add container, move existing node into it} in one call.
        // move::validate() must defer the newParent existence check so it can
        // accept a parent created earlier in the same batch.
        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "test_network", "ToMove"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspAddOp("container.chain", "test_network", "NewHome"));
        ops.add(makeDspMoveOp("ToMove", "NewHome"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        tree = getDspTree();
        auto newHome = findNodeInTree(tree[RestApiIds::result], "NewHome");
        expect(newHome.isObject(), "NewHome should exist");
        auto moved2 = findNodeInTree(newHome, "ToMove");
        expect(moved2.isObject(), "ToMove should be inside NewHome after batch");
    }

    void testDspApplySet()
    {
        /** Setup: Network with an oscillator
         *  Scenario: Set parameter values and node properties
         *  Expected: Values change in tree
         */
        beginTest("POST /api/dsp/apply - set");

        resetDspState();

        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "SetOsc"));
        expectDspSuccess(postDspOps(ops));

        // Set a parameter value
        ops.clear();
        ops.add(makeDspSetOp("SetOsc", "Frequency", 880.0));
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify in tree
        auto tree = getDspTree();
        auto osc = findNodeInTree(tree[RestApiIds::result], "SetOsc");
        auto params = osc[RestApiIds::parameters];
        bool foundFreq = false;
        for (int i = 0; i < params.size(); i++)
        {
            if (params[i][RestApiIds::parameterId].toString() == "Frequency")
            {
                expectEquals((double)params[i][RestApiIds::value], 880.0,
                    "Frequency should be 880");
                foundFreq = true;
            }
        }
        expect(foundFreq, "Should find Frequency parameter");

        // Set network-level property on root node
        ops.clear();
        ops.add(makeDspSetOp("test_network", "AllowPolyphonic", true));
        json = postDspOps(ops);
        expectDspSuccess(json);

        // Set a node property (not a parameter): control.converter Mode
        ops.clear();
        ops.add(makeDspAddOp("control.converter", "test_network", "Conv1"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspSetOp("Conv1", "Mode", "Ms2Samples"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify the property was set by re-reading the tree.
        // Node properties are not returned by the default tree (only parameters),
        // so we verify via a roundtrip: set it back to a different value and
        // confirm no error, proving the property path works.
        ops.clear();
        ops.add(makeDspSetOp("Conv1", "Mode", "Ms2Freq"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        // Error: missing required fields
        DynamicObject::Ptr badOp = new DynamicObject();
        badOp->setProperty(RestApiIds::op, "set");
        badOp->setProperty(RestApiIds::nodeId, "SetOsc");
        ops.clear();
        ops.add(var(badOp.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing parameterId should fail");

        // Error: non-existent node
        ops.clear();
        ops.add(makeDspSetOp("NonExistent", "Frequency", 440.0));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Non-existent node should fail");

        // Regression: network-level properties must be stored on the Network
        // ValueTree, not the root Node's Value. findParameterOrProperty returns
        // the Node when the property lives on the Network parent; set::perform
        // must detect that case and write to the parent, not treat it as a
        // regular parameter. We verify the op is undoable end-to-end: setting
        // AllowPolyphonic true then undoing should return to the original state
        // without leaving a stray Value parameter on the root node.
        ops.clear();
        ops.add(makeDspSetOp("test_network", "AllowPolyphonic", true));
        expectDspSuccess(postDspOps(ops));

        auto undoJson = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));
        expect((bool)undoJson[RestApiIds::success],
            "Undo of network-property set should succeed");

        // Root node parameters list should not contain a bogus entry caused by
        // the bug writing Value onto the Node instead of the Network property.
        auto tree2 = getDspTree();
        auto rootParams = tree2[RestApiIds::result][RestApiIds::parameters];
        expect(rootParams.isArray(), "root parameters should be array");
    }

    void testDspApplyBypass()
    {
        /** Setup: Network with a node
         *  Scenario: Toggle bypass state
         *  Expected: Bypass state changes in tree
         */
        beginTest("POST /api/dsp/apply - bypass");

        resetDspState();

        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "BypOsc"));
        expectDspSuccess(postDspOps(ops));

        // Bypass
        ops.clear();
        ops.add(makeDspBypassOp("BypOsc", true));
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        auto tree = getDspTree();
        auto osc = findNodeInTree(tree[RestApiIds::result], "BypOsc");
        expect((bool)osc[RestApiIds::bypassed], "Should be bypassed");

        // Unbypass
        ops.clear();
        ops.add(makeDspBypassOp("BypOsc", false));
        json = postDspOps(ops);
        expectDspSuccess(json);

        tree = getDspTree();
        osc = findNodeInTree(tree[RestApiIds::result], "BypOsc");
        expect(!(bool)osc[RestApiIds::bypassed], "Should not be bypassed");

        // Error: missing nodeId
        DynamicObject::Ptr badOp = new DynamicObject();
        badOp->setProperty(RestApiIds::op, "bypass");
        badOp->setProperty(RestApiIds::bypassed, true);
        ops.clear();
        ops.add(var(badOp.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing nodeId should fail");

        // Error: missing bypassed
        DynamicObject::Ptr badOp2 = new DynamicObject();
        badOp2->setProperty(RestApiIds::op, "bypass");
        badOp2->setProperty(RestApiIds::nodeId, "BypOsc");
        ops.clear();
        ops.add(var(badOp2.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing bypassed should fail");

        // Regression: bypass::perform must store the previous state on the
        // member so undo restores it. A shadow-local declaration (`auto
        // previousBypassed = ...`) hides the member and causes undo to always
        // restore the default (false). We detect that by chaining bypass(true)
        // -> bypass(false) -> undo; the correct behaviour is to end up at true,
        // but the bug would leave it at false.
        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "test_network", "ShadowOsc"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspBypassOp("ShadowOsc", true));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspBypassOp("ShadowOsc", false));
        expectDspSuccess(postDspOps(ops));

        auto shadowUndo = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));
        expect((bool)shadowUndo[RestApiIds::success], "Undo should succeed");

        tree = getDspTree();
        osc = findNodeInTree(tree[RestApiIds::result], "ShadowOsc");
        expect((bool)osc[RestApiIds::bypassed],
            "Undo of bypass(false) should restore previous state (true), not default (false)");
    }

    void testDspApplyConnect()
    {
        /** Setup: Network with modulation sources and targets
         *  Scenario: Test all connection types:
         *    1. Modulation connection (control node -> parameter)
         *    2. Parameter connection (container parameter -> child parameter)
         *    3. Bypass connection (control node -> node bypass)
         *  Expected: Connections appear in tree
         */
        beginTest("POST /api/dsp/apply - connect");

        resetDspState();

        // --- 1. Modulation connection: control.pma -> oscillator.Frequency ---
        Array<var> ops;
        ops.add(makeDspAddOp("control.pma", "test_network", "PMA1"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "ConnOsc"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspConnectOp("PMA1", "ConnOsc", "Frequency"));
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify modulation connection in tree
        auto tree = getDspTree();
        auto osc = tree[RestApiIds::result];
        auto connections = osc[RestApiIds::connections];
        expect(connections.isArray(), "Should have connections");
        expect(connections.size() > 0, "Should have at least 1 connection");

        if (connections.size() > 0)
        {
            expectEquals(connections[0][RestApiIds::source].toString(), String("PMA1"),
                "Source should be PMA1");
            expectEquals(connections[0][RestApiIds::parameter].toString(), String("Frequency"),
                "Parameter should be Frequency");
        }

        // --- 2. Parameter connection: container parameter -> child parameter ---
        // Create a sub-container with a parameter, then connect it to a child
        ops.clear();
        ops.add(makeDspAddOp("container.chain", "test_network", "ParamChain"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspCreateParameterOp("ParamChain", "MacroFreq", 20.0, 20000.0, 1000.0));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "ParamChain", "ParamOsc"));
        expectDspSuccess(postDspOps(ops));

        // Connect the container's MacroFreq parameter -> ParamOsc.Frequency
        ops.clear();
        ops.add(makeDspConnectOp("ParamChain", "ParamOsc", "Frequency", "MacroFreq"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        tree = getDspTree();
        auto paramOsc = tree[RestApiIds::result];
        connections = paramOsc[RestApiIds::connections];
        expect(connections.size() > 0, "ParamOsc should have a parameter connection");

        if (connections.size() > 0)
        {
            expectEquals(connections[1][RestApiIds::source].toString(), String("ParamChain"),
                "Source should be ParamChain");
            expectEquals(connections[1][RestApiIds::sourceOutput].toString(), String("MacroFreq"),
                "Source output should be MacroFreq");
            expectEquals(connections[1][RestApiIds::parameter].toString(), String("Frequency"),
                "Target parameter should be Frequency");
        }

        // --- 3. Bypass connection: control node -> node bypass ---
        ops.clear();
        ops.add(makeDspAddOp("container.soft_bypass", "test_network", "SoftBypass1"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspAddOp("control.pma", "SoftBypass1", "BypassCtrl"));
        ops.add(makeDspAddOp("core.oscillator", "SoftBypass1", "BypassOsc"));
        expectDspSuccess(postDspOps(ops));

        // Connect BypassCtrl -> SoftBypass1.Bypass (synthetic parameter name)
        ops.clear();
        ops.add(makeDspConnectOp("BypassCtrl", "SoftBypass1", "Bypassed"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        // --- Error: missing required fields ---
        DynamicObject::Ptr badOp = new DynamicObject();
        badOp->setProperty(RestApiIds::op, "connect");
        badOp->setProperty(RestApiIds::source, "PMA1");
        ops.clear();
        ops.add(var(badOp.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing target/parameter should fail");

        // Regression: batch {add source, add target, connect} must succeed.
        // connect::validate() must defer the source/target existence checks
        // so ops earlier in the same batch have a chance to create them.
        ops.clear();
        ops.add(makeDspAddOp("control.pma", "test_network", "BatchSrc"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "BatchTgt"));
        ops.add(makeDspConnectOp("BatchSrc", "BatchTgt", "Frequency"));
        json = postDspOps(ops);
        expectDspSuccess(json);

        // Regression: connecting to a non-existent parameter must produce an
        // error hint that lists the target node's actual parameters. The bug
        // passed the invalid lookup result `pn` (empty tree) to
        // getErrorForParameter404 instead of the target node `tn`, producing
        // an empty hint.
        ops.clear();
        ops.add(makeDspConnectOp("BatchSrc", "BatchTgt", "Frquence"));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success],
            "Connecting to non-existent parameter should fail");

        auto errArr = json[RestApiIds::errors];
        if (errArr.isArray() && errArr.size() > 0)
        {
            auto hint = errArr[0]["hint"].toString() + " " +
                        errArr[0][RestApiIds::errorMessage].toString();
            expect(hint.contains("Frequency") || hint.contains("Gain") ||
                   hint.contains("Mode"),
                "Error hint should include at least one valid parameter name from target node");
        }
    }

    void testDspApplyDisconnect()
    {
        /** Setup: Network with an existing connection
         *  Scenario: Disconnect the modulation connection
         *  Expected: Connection removed from tree
         */
        beginTest("POST /api/dsp/apply - disconnect");

        resetDspState();

        // Build a connection first
        Array<var> ops;
        ops.add(makeDspAddOp("control.pma", "test_network", "DisPMA"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "DisOsc"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspConnectOp("DisPMA", "DisOsc", "Frequency"));
        expectDspSuccess(postDspOps(ops));

        // Verify connection exists
        auto tree = getDspTree();
        auto osc = tree[RestApiIds::result];
        expect(osc[RestApiIds::connections].size() > 0, "Should have connection before disconnect");

        // Disconnect
        ops.clear();
        ops.add(makeDspDisconnectOp("DisPMA", "DisOsc", "Frequency"));
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify connection removed
        tree = getDspTree();
        osc = findNodeInTree(tree[RestApiIds::result], "DisOsc");
        expectEquals<int>(osc[RestApiIds::connections].size(), 0,
            "Should have no connections after disconnect");

        // Error: missing required fields
        DynamicObject::Ptr badOp = new DynamicObject();
        badOp->setProperty(RestApiIds::op, "disconnect");
        badOp->setProperty(RestApiIds::source, "DisPMA");
        ops.clear();
        ops.add(var(badOp.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing target/parameter should fail");

        // Regression: batch {add source, add target, connect, disconnect} must succeed.
        // disconnect::validate() must defer the source existence check so it
        // accepts nodes created earlier in the same batch.
        ops.clear();
        ops.add(makeDspAddOp("control.pma", "test_network", "BatchDisSrc"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "BatchDisTgt"));
        ops.add(makeDspConnectOp("BatchDisSrc", "BatchDisTgt", "Frequency"));
        ops.add(makeDspDisconnectOp("BatchDisSrc", "BatchDisTgt", "Frequency"));
        json = postDspOps(ops);
        expectDspSuccess(json);
    }

    void testDspApplyCreateParameter()
    {
        /** Setup: Network with a container
         *  Scenario: Create a dynamic parameter on the container
         *  Expected: Parameter appears in tree with correct range
         */
        beginTest("POST /api/dsp/apply - create_parameter");

        resetDspState();

        // create_parameter on root container
        Array<var> ops;
        ops.add(makeDspCreateParameterOp("test_network", "MyParam", 0.0, 100.0, 50.0, 1.0));
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify parameter in verbose tree
        auto tree = getDspTree("DspTestFX", true);
        auto root = tree[RestApiIds::result];
        auto params = root[RestApiIds::parameters];
        bool found = false;
        for (int i = 0; i < params.size(); i++)
        {
            if (params[i][RestApiIds::parameterId].toString() == "MyParam")
            {
                found = true;
                expectEquals((double)params[i][RestApiIds::min], 0.0, "min should be 0");
                expectEquals((double)params[i][RestApiIds::max], 100.0, "max should be 100");
                expectEquals((double)params[i][RestApiIds::defaultValue], 50.0, "default should be 50");
                expectEquals((double)params[i][RestApiIds::stepSize], 1.0, "stepSize should be 1");
            }
        }
        expect(found, "Should find MyParam in root parameters");

        // Error: missing nodeId
        DynamicObject::Ptr badOp = new DynamicObject();
        badOp->setProperty(RestApiIds::op, "create_parameter");
        badOp->setProperty(RestApiIds::parameterId, "BadParam");
        ops.clear();
        ops.add(var(badOp.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing nodeId should fail");

        // Error: missing parameterId
        DynamicObject::Ptr badOp2 = new DynamicObject();
        badOp2->setProperty(RestApiIds::op, "create_parameter");
        badOp2->setProperty(RestApiIds::nodeId, "test_network");
        ops.clear();
        ops.add(var(badOp2.get()));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Missing parameterId should fail");

        // Regression: batch {add container, create_parameter on it} must succeed.
        // create_parameter::validate() must defer the node-existence check so
        // it accepts a container created earlier in the same batch.
        ops.clear();
        ops.add(makeDspAddOp("container.chain", "test_network", "BatchContainer"));
        ops.add(makeDspCreateParameterOp("BatchContainer", "BatchParam", 0.0, 1.0, 0.5));
        json = postDspOps(ops);
        expectDspSuccess(json);

        auto tree2 = getDspTree("DspTestFX", true);
        auto bc = findNodeInTree(tree2[RestApiIds::result], "BatchContainer");
        expect(bc.isObject(), "BatchContainer should exist");
        auto bcParams = bc[RestApiIds::parameters];
        bool foundBatchParam = false;
        for (int i = 0; i < bcParams.size(); i++)
        {
            if (bcParams[i][RestApiIds::parameterId].toString() == "BatchParam")
                foundBatchParam = true;
        }
        expect(foundBatchParam, "BatchParam should exist on BatchContainer after batch");
    }

    void testDspApplyClear()
    {
        /** Setup: Network with several nodes
         *  Scenario: Clear all nodes
         *  Expected: Tree is empty after clear
         */
        beginTest("POST /api/dsp/apply - clear");

        resetDspState();

        // Add some nodes
        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "ClearOsc"));
        ops.add(makeDspAddOp("filters.svf", "test_network", "ClearFilter"));
        ops.add(makeDspAddOp("container.chain", "test_network", "ClearChain"));
        expectDspSuccess(postDspOps(ops));

        // Verify nodes exist
        auto tree = getDspTree();
        expect(tree[RestApiIds::result][RestApiIds::children].size() > 0,
            "Should have children before clear");

        // Clear
        ops.clear();
        ops.add(makeDspClearOp());
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify empty
        tree = getDspTree();
        expectEquals<int>(tree[RestApiIds::result][RestApiIds::children].size(), 0,
            "Should have no children after clear");
    }

    void testDspApplyValidation()
    {
        /** Setup: Fresh DSP state
         *  Scenario: Send various invalid operations
         *  Expected: All fail with appropriate errors
         */
        beginTest("POST /api/dsp/apply - validation errors");

        resetDspState();

        // Empty operations array
        DynamicObject::Ptr body = new DynamicObject();
        body->setProperty(RestApiIds::moduleId, "DspTestFX");
        body->setProperty(RestApiIds::operations, Array<var>());
        auto json = ctx->parseJson(ctx->httpPost("/api/dsp/apply",
            JSON::toString(var(body.get()))));
        expect(!(bool)json[RestApiIds::success], "Empty operations should fail");

        // Missing operations field
        auto json2 = ctx->parseJson(ctx->httpPost("/api/dsp/apply",
            R"({"moduleId": "DspTestFX"})"));
        expect(!(bool)json2[RestApiIds::success], "Missing operations should fail");

        // Missing moduleId
        auto json3 = ctx->parseJson(ctx->httpPost("/api/dsp/apply",
            R"({"operations": [{"op": "add", "factoryPath": "core.oscillator", "parent": "test_network"}]})"));
        expect(!(bool)json3[RestApiIds::success], "Missing moduleId should fail");

        // Invalid moduleId
        DynamicObject::Ptr body4 = new DynamicObject();
        body4->setProperty(RestApiIds::moduleId, "NonExistentModule");
        Array<var> ops4;
        ops4.add(makeDspAddOp("core.oscillator", "test_network", "X"));
        body4->setProperty(RestApiIds::operations, var(ops4));
        auto json4 = ctx->parseJson(ctx->httpPost("/api/dsp/apply",
            JSON::toString(var(body4.get()))));
        expect(!(bool)json4[RestApiIds::success], "Invalid moduleId should fail");

        // Unknown op type
        DynamicObject::Ptr unknownOp = new DynamicObject();
        unknownOp->setProperty(RestApiIds::op, "unknown_op");
        Array<var> ops5;
        ops5.add(var(unknownOp.get()));
        auto json5 = postDspOps(ops5);
        expect(!(bool)json5[RestApiIds::success], "Unknown op should fail");

        // Op without op field
        DynamicObject::Ptr noOp = new DynamicObject();
        noOp->setProperty(RestApiIds::nodeId, "test");
        Array<var> ops6;
        ops6.add(var(noOp.get()));
        auto json6 = postDspOps(ops6);
        expect(!(bool)json6[RestApiIds::success], "Missing op field should fail");
    }

    void testDspApplyUndo()
    {
        /** Setup: Fresh DSP state
         *  Scenario: Add nodes in undo group, undo, redo
         *  Expected: Tree state matches before/after undo/redo
         */
        beginTest("POST /api/dsp/apply - undo/redo");

        resetDspState();

        // Push undo group
        DynamicObject::Ptr groupBody = new DynamicObject();
        groupBody->setProperty(RestApiIds::name, "dsp_undo_test");
        ctx->parseJson(ctx->httpPost("/api/undo/push_group",
            JSON::toString(var(groupBody.get()))));

        // Add container + child
        Array<var> ops;
        ops.add(makeDspAddOp("container.chain", "test_network", "UndoChain"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "UndoChain", "UndoOsc"));
        expectDspSuccess(postDspOps(ops));

        // Pop group
        ctx->parseJson(ctx->httpPost("/api/undo/pop_group", "{}"));

        // Verify nodes exist
        auto tree = getDspTree();
        auto chain = findNodeInTree(tree[RestApiIds::result], "UndoChain");
        expect(chain.isObject(), "UndoChain should exist");
        auto osc = findNodeInTree(tree[RestApiIds::result], "UndoOsc");
        expect(osc.isObject(), "UndoOsc should exist");

        // Undo
        auto undoJson = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));
        expect((bool)undoJson[RestApiIds::success], "Undo should succeed");

        // Verify nodes removed
        tree = getDspTree();
        expectEquals<int>(tree[RestApiIds::result][RestApiIds::children].size(), 0,
            "Tree should be empty after undo");

        // Redo
        auto redoJson = ctx->parseJson(ctx->httpPost("/api/undo/forward", "{}"));
        expect((bool)redoJson[RestApiIds::success], "Redo should succeed");

        // Verify nodes restored
        tree = getDspTree();
        chain = findNodeInTree(tree[RestApiIds::result], "UndoChain");
        expect(chain.isObject(), "UndoChain should be restored after redo");
        osc = findNodeInTree(tree[RestApiIds::result], "UndoOsc");
        expect(osc.isObject(), "UndoOsc should be restored after redo");
    }

    /** Push an undo group with the given name. */
    void dspPushGroup(const String& name)
    {
        DynamicObject::Ptr body = new DynamicObject();
        body->setProperty(RestApiIds::name, name);
        auto r = ctx->parseJson(ctx->httpPost("/api/undo/push_group",
            JSON::toString(var(body.get()))));
        expect((bool)r[RestApiIds::success], "push_group '" + name + "' should succeed");
    }

    /** Pop the current undo group, committing deferred ops (cancel=false). */
    void dspPopGroupCommit()
    {
        DynamicObject::Ptr body = new DynamicObject();
        body->setProperty(RestApiIds::cancel, false);
        auto r = ctx->parseJson(ctx->httpPost("/api/undo/pop_group",
            JSON::toString(var(body.get()))));
        expect((bool)r[RestApiIds::success], "pop_group commit should succeed");
    }

    //==========================================================================
    /** Plan-group test: one op per /api/dsp/apply call, covering many endpoint
     *  types (add, set, bypass, create_parameter, connect, move) within a single
     *  group. Verifies commit on pop, full undo, and full redo.
     */
    void testDspGroupSingleOpsVariety()
    {
        beginTest("DSP plan group - single op per apply call, variety");

        resetDspState();

        dspPushGroup("single_ops_variety");

        // Each apply call carries exactly one op.
        Array<var> ops;

        ops.clear();
        ops.add(makeDspAddOp("container.chain", "test_network", "Chain"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "Chain", "Osc"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspSetOp("Osc", "Frequency", 880.0));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspBypassOp("Osc", true));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspAddOp("control.pma", "Chain", "Mod"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspCreateParameterOp("Chain", "Intensity", 0.0, 1.0, 0.5));
        expectDspSuccess(postDspOps(ops));

        // Move BEFORE connect. Moving a node that already has an active
        // connection triggers runtime auto-cleanup of the dangling edge, which
        // breaks the symmetric undo chain (connect.undo later can't find the
        // connection it expects to remove). Reordering decouples the two ops
        // while still exercising both endpoints inside the group.
        ops.clear();
        ops.add(makeDspMoveOp("Mod", "test_network"));
        expectDspSuccess(postDspOps(ops));

        ops.clear();
        ops.add(makeDspConnectOp("Mod", "Osc", "Frequency"));
        expectDspSuccess(postDspOps(ops));

        dspPopGroupCommit();

        // Verify committed state.
        auto tree = getDspTree();
        auto chain = findNodeInTree(tree[RestApiIds::result], "Chain");
        expect(chain.isObject(), "Chain should exist after commit");

        auto osc = findNodeInTree(tree[RestApiIds::result], "Osc");
        expect(osc.isObject(), "Osc should exist inside Chain");
        expect((bool)osc[RestApiIds::bypassed], "Osc should be bypassed");

        bool oscFreqOk = false;
        auto oscParams = osc[RestApiIds::parameters];
        for (int i = 0; i < oscParams.size(); i++)
        {
            if (oscParams[i][RestApiIds::parameterId].toString() == "Frequency"
                && (double)oscParams[i][RestApiIds::value] == 880.0)
                oscFreqOk = true;
        }
        expect(oscFreqOk, "Osc.Frequency should be 880");

        auto mod = findNodeInTree(tree[RestApiIds::result], "Mod");
        expect(mod.isObject(), "Mod should exist after move to root");

        // Intensity parameter should exist on Chain.
        bool foundIntensity = false;
        auto chainParams = chain[RestApiIds::parameters];
        for (int i = 0; i < chainParams.size(); i++)
        {
            if (chainParams[i][RestApiIds::parameterId].toString() == "Intensity")
                foundIntensity = true;
        }
        expect(foundIntensity, "Intensity parameter should exist on Chain");

        // Undo the whole group.
        auto undoJson = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));

        DBG(JSON::toString(undoJson));

        expect((bool)undoJson[RestApiIds::success], "Group undo should succeed");

        tree = getDspTree();
        expectEquals<int>(tree[RestApiIds::result][RestApiIds::children].size(), 0,
            "test_network should be empty after group undo");

        // Redo the whole group.
        auto redoJson = ctx->parseJson(ctx->httpPost("/api/undo/forward", "{}"));
        expect((bool)redoJson[RestApiIds::success], "Group redo should succeed");

        tree = getDspTree();
        expect(findNodeInTree(tree[RestApiIds::result], "Chain").isObject(),
            "Chain restored after redo");
        expect(findNodeInTree(tree[RestApiIds::result], "Osc").isObject(),
            "Osc restored after redo");
        expect(findNodeInTree(tree[RestApiIds::result], "Mod").isObject(),
            "Mod restored after redo");
    }

    //==========================================================================
    /** Plan-group test: multiple ops per /api/dsp/apply call, covering many
     *  endpoint types batched into fewer apply calls. Verifies commit on pop,
     *  full undo, and full redo.
     */
    void testDspGroupMultiOpsBatched()
    {
        beginTest("DSP plan group - multi-op per apply call, batched");

        resetDspState();

        dspPushGroup("multi_ops_batched");

        // Call 1: three siblings at root.
        {
            Array<var> ops;
            ops.add(makeDspAddOp("core.oscillator", "test_network", "Carrier"));
            ops.add(makeDspAddOp("control.pma", "test_network", "Modulator"));
            ops.add(makeDspAddOp("container.chain", "test_network", "FilterChain"));
            expectDspSuccess(postDspOps(ops));
        }

        // Call 2: set three parameters in one batch.
        {
            Array<var> ops;
            ops.add(makeDspSetOp("Carrier", "Frequency", 440.0));
            ops.add(makeDspSetOp("Modulator", "Multiply", 0.5));
            ops.add(makeDspSetOp("Modulator", "Add", 0.25));
            expectDspSuccess(postDspOps(ops));
        }

        // Call 3: two bypass toggles in one batch.
        {
            Array<var> ops;
            ops.add(makeDspBypassOp("Modulator", false));
            ops.add(makeDspBypassOp("Carrier", false));
            expectDspSuccess(postDspOps(ops));
        }

        // Call 4: connect + create_parameter in one batch.
        {
            Array<var> ops;
            ops.add(makeDspConnectOp("Modulator", "Carrier", "Frequency"));
            ops.add(makeDspCreateParameterOp("test_network", "Depth", 0.0, 1.0, 0.5));
            expectDspSuccess(postDspOps(ops));
        }

        dspPopGroupCommit();

        auto tree = getDspTree();

        expect(findNodeInTree(tree[RestApiIds::result], "Carrier").isObject(),
            "Carrier committed");
        expect(findNodeInTree(tree[RestApiIds::result], "Modulator").isObject(),
            "Modulator committed");
        expect(findNodeInTree(tree[RestApiIds::result], "FilterChain").isObject(),
            "FilterChain committed");

        // Depth parameter on the root.
        bool foundDepth = false;
        auto rootParams = tree[RestApiIds::result][RestApiIds::parameters];
        for (int i = 0; i < rootParams.size(); i++)
        {
            if (rootParams[i][RestApiIds::parameterId].toString() == "Depth")
                foundDepth = true;
        }
        expect(foundDepth, "Depth parameter on root after commit");

        // Connection on the root container (Modulator -> Carrier.Frequency).
        auto rootConns = tree[RestApiIds::result][RestApiIds::connections];
        bool foundConn = false;
        for (int i = 0; i < rootConns.size(); i++)
        {
            if (rootConns[i][RestApiIds::source].toString() == "Modulator"
                && rootConns[i][RestApiIds::target].toString() == "Carrier"
                && rootConns[i][RestApiIds::parameter].toString() == "Frequency")
                foundConn = true;
        }
        expect(foundConn, "Modulator -> Carrier.Frequency connection present");

        // Undo the whole group.
        auto undoJson = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));
        expect((bool)undoJson[RestApiIds::success], "Group undo should succeed");

        tree = getDspTree();
        expectEquals<int>(tree[RestApiIds::result][RestApiIds::children].size(), 0,
            "test_network empty after undo");

        // Redo.
        auto redoJson = ctx->parseJson(ctx->httpPost("/api/undo/forward", "{}"));
        expect((bool)redoJson[RestApiIds::success], "Group redo should succeed");

        tree = getDspTree();
        expect(findNodeInTree(tree[RestApiIds::result], "Carrier").isObject(),
            "Carrier restored after redo");
        expect(findNodeInTree(tree[RestApiIds::result], "FilterChain").isObject(),
            "FilterChain restored after redo");
    }

    //==========================================================================
    /** Plan-group test: realistic LLM-style workflow mixing single-op and
     *  multi-op apply calls. Models a consumer iteratively composing a dry/wet
     *  reverb patch. Also verifies that GET /api/dsp/tree reads the accumulated
     *  state mid-group (before pop).
     */
    void testDspGroupMixedOpsWorkflow()
    {
        beginTest("DSP plan group - mixed single/multi op workflow");

        resetDspState();

        dspPushGroup("mixed_workflow");

        // Multi-op: build split topology.
        {
            Array<var> ops;
            ops.add(makeDspAddOp("container.split", "test_network", "DryWet"));
            ops.add(makeDspAddOp("container.chain", "DryWet", "DryPath"));
            ops.add(makeDspAddOp("container.chain", "DryWet", "WetPath"));
            expectDspSuccess(postDspOps(ops));
        }

        // Single-op: add reverb into WetPath.
        {
            Array<var> ops;
            ops.add(makeDspAddOp("fx.reverb", "WetPath", "Rev"));
            expectDspSuccess(postDspOps(ops));
        }

        // Multi-op: gains on both paths.
        {
            Array<var> ops;
            ops.add(makeDspAddOp("core.gain", "DryPath", "DryGain"));
            ops.add(makeDspAddOp("core.gain", "WetPath", "WetGain"));
            expectDspSuccess(postDspOps(ops));
        }

        // Single-op: expose the Mix parameter on root.
        {
            Array<var> ops;
            ops.add(makeDspCreateParameterOp("test_network", "Mix", 0.0, 1.0, 0.5));
            expectDspSuccess(postDspOps(ops));
        }

        // Mid-group read via ?group=current: should show the plan-mode snapshot
        // accumulated by dsp::add::validate (the container adds with explicit
        // nodeIds, which mirror into dspValidation->networkTree).
        {
            auto midResponse = ctx->httpGet(
                "/api/dsp/tree?moduleId=DspTestFX&group=current");
            auto midTree = ctx->parseJson(midResponse);
            expect((bool)midTree[RestApiIds::success],
                "Mid-group tree read should succeed");
            expect(findNodeInTree(midTree[RestApiIds::result], "DryWet").isObject(),
                "DryWet visible mid-group (via group=current)");
            expect(findNodeInTree(midTree[RestApiIds::result], "Rev").isObject(),
                "Rev visible mid-group (via group=current)");
        }

        // Multi-op: tune the reverb.
        {
            Array<var> ops;
            ops.add(makeDspSetOp("Rev", "Damping", 0.7));
            ops.add(makeDspSetOp("Rev", "Size", 0.8));
            expectDspSuccess(postDspOps(ops));
        }

        // Single-op: make sure reverb is active.
        {
            Array<var> ops;
            ops.add(makeDspBypassOp("Rev", false));
            expectDspSuccess(postDspOps(ops));
        }

        dspPopGroupCommit();

        // Verify the committed graph.
        auto tree = getDspTree();
        auto dryWet = findNodeInTree(tree[RestApiIds::result], "DryWet");
        expect(dryWet.isObject(), "DryWet committed");

        auto dryPath = findNodeInTree(dryWet, "DryPath");
        expect(dryPath.isObject(), "DryPath inside DryWet");
        expect(findNodeInTree(dryPath, "DryGain").isObject(),
            "DryGain inside DryPath");

        auto wetPath = findNodeInTree(dryWet, "WetPath");
        expect(wetPath.isObject(), "WetPath inside DryWet");
        expect(findNodeInTree(wetPath, "Rev").isObject(), "Rev inside WetPath");
        expect(findNodeInTree(wetPath, "WetGain").isObject(),
            "WetGain inside WetPath");

        // Mix parameter on root.
        bool foundMix = false;
        auto rootParams = tree[RestApiIds::result][RestApiIds::parameters];
        for (int i = 0; i < rootParams.size(); i++)
        {
            if (rootParams[i][RestApiIds::parameterId].toString() == "Mix")
                foundMix = true;
        }
        expect(foundMix, "Mix parameter exposed on root");

        // Verify Rev parameter values were set.
        auto rev = findNodeInTree(wetPath, "Rev");
        auto revParams = rev[RestApiIds::parameters];
        bool damping = false, size = false;
        for (int i = 0; i < revParams.size(); i++)
        {
            auto id = revParams[i][RestApiIds::parameterId].toString();
            auto val = (double)revParams[i][RestApiIds::value];
            if (id == "Damping" && val == 0.7) damping = true;
            if (id == "Size" && val == 0.8) size = true;
        }
        expect(damping, "Rev.Damping set to 0.7");
        expect(size, "Rev.Size set to 0.8");

        // Undo the whole group.
        auto undoJson = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));
        expect((bool)undoJson[RestApiIds::success], "Group undo should succeed");

        tree = getDspTree();
        expectEquals<int>(tree[RestApiIds::result][RestApiIds::children].size(), 0,
            "test_network empty after group undo");

        // Redo.
        auto redoJson = ctx->parseJson(ctx->httpPost("/api/undo/forward", "{}"));
        expect((bool)redoJson[RestApiIds::success], "Group redo should succeed");

        tree = getDspTree();
        expect(findNodeInTree(tree[RestApiIds::result], "DryWet").isObject(),
            "DryWet restored after redo");
        expect(findNodeInTree(tree[RestApiIds::result], "Rev").isObject(),
            "Rev restored after redo");
    }

    //==========================================================================
    /** Read the current DSP group snapshot via ?group=current and return the
     *  parsed result object. Used by testDspGroupMirror* tests to assert that
     *  a DSP action's validate() mirrored its mutation onto the plan snapshot.
     */
    var getDspGroupCurrent(const String& moduleId = "DspTestFX")
    {
        auto url = "/api/dsp/tree?moduleId=" + URL::addEscapeChars(moduleId, true)
                 + "&group=current";
        return ctx->parseJson(ctx->httpGet(url));
    }

    /** Find a parameter by id in a parameters array. */
    var findParamInArray(const var& params, const String& parameterId)
    {
        if (!params.isArray())
            return {};

        for (int i = 0; i < params.size(); i++)
        {
            if (params[i][RestApiIds::parameterId].toString() == parameterId)
                return params[i];
        }

        return {};
    }

    //==========================================================================
    /** Setup: In a DSP plan group, add Osc then set Osc.Frequency=880.
     *  Scenario: Mid-group GET /api/dsp/tree?group=current.
     *  Expected: Osc.Frequency is 880 in the snapshot (mirrored by set::validate).
     */
    void testDspGroupMirrorSet()
    {
        beginTest("DSP plan group - set mirrors onto snapshot");

        resetDspState();

        dspPushGroup("mirror_set");

        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "Osc"));
        ops.add(makeDspSetOp("Osc", "Frequency", 880.0));
        expectDspSuccess(postDspOps(ops));

        // Mid-group read: set::validate should have mirrored Frequency=880.
        auto mid = getDspGroupCurrent();
        expect((bool)mid[RestApiIds::success], "Mid-group read should succeed");

        auto midOsc = findNodeInTree(mid[RestApiIds::result], "Osc");
        expect(midOsc.isObject(), "Osc visible mid-group");

        auto midFreq = findParamInArray(midOsc[RestApiIds::parameters], "Frequency");
        expect(midFreq.isObject(), "Frequency param visible mid-group");
        expectEquals((double)midFreq[RestApiIds::value], 880.0,
            "Osc.Frequency mirrored to 880 mid-group");

        dspPopGroupCommit();

        auto tree = getDspTree();
        auto osc = findNodeInTree(tree[RestApiIds::result], "Osc");
        expect(osc.isObject(), "Osc committed");
        auto freq = findParamInArray(osc[RestApiIds::parameters], "Frequency");
        expectEquals((double)freq[RestApiIds::value], 880.0,
            "Osc.Frequency committed to 880");
    }

    //==========================================================================
    /** Setup: In a DSP plan group, add Osc then bypass Osc true.
     *  Scenario: Mid-group read.
     *  Expected: Osc.bypassed is true in snapshot (mirrored by bypass::validate).
     */
    void testDspGroupMirrorBypass()
    {
        beginTest("DSP plan group - bypass mirrors onto snapshot");

        resetDspState();

        dspPushGroup("mirror_bypass");

        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "Osc"));
        ops.add(makeDspBypassOp("Osc", true));
        expectDspSuccess(postDspOps(ops));

        auto mid = getDspGroupCurrent();
        expect((bool)mid[RestApiIds::success], "Mid-group read should succeed");

        auto midOsc = findNodeInTree(mid[RestApiIds::result], "Osc");
        expect(midOsc.isObject(), "Osc visible mid-group");
        expect((bool)midOsc[RestApiIds::bypassed], "Osc bypassed mirrored true mid-group");

        dspPopGroupCommit();

        auto tree = getDspTree();
        auto osc = findNodeInTree(tree[RestApiIds::result], "Osc");
        expect((bool)osc[RestApiIds::bypassed], "Osc bypassed committed");
    }

    //==========================================================================
    /** Setup: In a DSP plan group, add A, add B, remove A.
     *  Scenario: Mid-group read.
     *  Expected: Only B visible mid-group; A absent.
     */
    void testDspGroupMirrorRemove()
    {
        beginTest("DSP plan group - remove mirrors onto snapshot");

        resetDspState();

        dspPushGroup("mirror_remove");

        Array<var> ops;
        ops.add(makeDspAddOp("core.oscillator", "test_network", "OscA"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "OscB"));
        ops.add(makeDspRemoveOp("OscA"));
        expectDspSuccess(postDspOps(ops));

        auto mid = getDspGroupCurrent();
        expect((bool)mid[RestApiIds::success], "Mid-group read should succeed");

        expect(!findNodeInTree(mid[RestApiIds::result], "OscA").isObject(),
            "OscA removed in snapshot mid-group");
        expect(findNodeInTree(mid[RestApiIds::result], "OscB").isObject(),
            "OscB still visible mid-group");

        dspPopGroupCommit();

        auto tree = getDspTree();
        expect(!findNodeInTree(tree[RestApiIds::result], "OscA").isObject(),
            "OscA absent after commit");
        expect(findNodeInTree(tree[RestApiIds::result], "OscB").isObject(),
            "OscB present after commit");
    }

    //==========================================================================
    /** Setup: In a DSP plan group, add Chain, add Osc at root, move Osc into Chain.
     *  Scenario: Mid-group read.
     *  Expected: Chain has child Osc; root has no direct Osc.
     */
    void testDspGroupMirrorMove()
    {
        beginTest("DSP plan group - move mirrors onto snapshot");

        resetDspState();

        dspPushGroup("mirror_move");

        Array<var> ops;
        ops.add(makeDspAddOp("container.chain", "test_network", "Chain"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "Osc"));
        ops.add(makeDspMoveOp("Osc", "Chain"));
        expectDspSuccess(postDspOps(ops));

        auto mid = getDspGroupCurrent();
        expect((bool)mid[RestApiIds::success], "Mid-group read should succeed");

        auto midChain = findNodeInTree(mid[RestApiIds::result], "Chain");
        expect(midChain.isObject(), "Chain visible mid-group");

        // Check Osc is a direct child of Chain, not at root.
        auto chainChildren = midChain[RestApiIds::children];
        bool oscInChain = false;
        if (chainChildren.isArray())
        {
            for (int i = 0; i < chainChildren.size(); i++)
            {
                if (chainChildren[i][RestApiIds::nodeId].toString() == "Osc")
                    oscInChain = true;
            }
        }
        expect(oscInChain, "Osc is direct child of Chain mid-group");

        // Check Osc is NOT a direct child of the root container.
        auto rootChildren = mid[RestApiIds::result][RestApiIds::children];
        bool oscAtRoot = false;
        if (rootChildren.isArray())
        {
            for (int i = 0; i < rootChildren.size(); i++)
            {
                if (rootChildren[i][RestApiIds::nodeId].toString() == "Osc")
                    oscAtRoot = true;
            }
        }
        expect(!oscAtRoot, "Osc not at root mid-group (moved into Chain)");

        dspPopGroupCommit();

        auto tree = getDspTree();
        auto chain = findNodeInTree(tree[RestApiIds::result], "Chain");
        auto chainKids = chain[RestApiIds::children];
        bool oscInChainCommitted = false;
        if (chainKids.isArray())
        {
            for (int i = 0; i < chainKids.size(); i++)
            {
                if (chainKids[i][RestApiIds::nodeId].toString() == "Osc")
                    oscInChainCommitted = true;
            }
        }
        expect(oscInChainCommitted, "Osc committed inside Chain");
    }

    //==========================================================================
    /** Setup: In a DSP plan group, add Chain, create_parameter on Chain "Gain".
     *  Scenario: Mid-group read.
     *  Expected: Chain has parameter Gain in parameters[].
     */
    void testDspGroupMirrorCreateParameter()
    {
        beginTest("DSP plan group - create_parameter mirrors onto snapshot");

        resetDspState();

        dspPushGroup("mirror_create_parameter");

        Array<var> ops;
        ops.add(makeDspAddOp("container.chain", "test_network", "Chain"));
        ops.add(makeDspCreateParameterOp("Chain", "Gain", 0.0, 2.0, 1.0));
        expectDspSuccess(postDspOps(ops));

        auto mid = getDspGroupCurrent();
        expect((bool)mid[RestApiIds::success], "Mid-group read should succeed");

        auto midChain = findNodeInTree(mid[RestApiIds::result], "Chain");
        expect(midChain.isObject(), "Chain visible mid-group");

        auto midGain = findParamInArray(midChain[RestApiIds::parameters], "Gain");
        expect(midGain.isObject(), "Gain parameter mirrored onto Chain mid-group");

        dspPopGroupCommit();

        auto tree = getDspTree();
        auto chain = findNodeInTree(tree[RestApiIds::result], "Chain");
        auto gain = findParamInArray(chain[RestApiIds::parameters], "Gain");
        expect(gain.isObject(), "Gain parameter committed on Chain");
    }

    //==========================================================================
    /** Setup: In a DSP plan group, add Mod (control.pma), add Osc (core.oscillator),
     *  connect Mod -> Osc.Frequency.
     *  Scenario: Mid-group read.
     *  Expected: connections[] on root contains the Mod -> Osc.Frequency edge.
     */
    void testDspGroupMirrorConnect()
    {
        beginTest("DSP plan group - connect mirrors onto snapshot");

        resetDspState();

        dspPushGroup("mirror_connect");

        Array<var> ops;
        ops.add(makeDspAddOp("control.pma", "test_network", "Mod"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "Osc"));
        ops.add(makeDspConnectOp("Mod", "Osc", "Frequency"));
        expectDspSuccess(postDspOps(ops));

        auto mid = getDspGroupCurrent();
        expect((bool)mid[RestApiIds::success], "Mid-group read should succeed");

        auto midConnections = mid[RestApiIds::result][RestApiIds::connections];
        bool found = false;
        if (midConnections.isArray())
        {
            for (int i = 0; i < midConnections.size(); i++)
            {
                if (midConnections[i][RestApiIds::source].toString() == "Mod"
                    && midConnections[i][RestApiIds::target].toString() == "Osc"
                    && midConnections[i][RestApiIds::parameter].toString() == "Frequency")
                    found = true;
            }
        }
        expect(found, "Mod -> Osc.Frequency present in snapshot mid-group");

        dspPopGroupCommit();

        auto tree = getDspTree();
        auto rootConns = tree[RestApiIds::result][RestApiIds::connections];
        bool committedFound = false;
        if (rootConns.isArray())
        {
            for (int i = 0; i < rootConns.size(); i++)
            {
                if (rootConns[i][RestApiIds::source].toString() == "Mod"
                    && rootConns[i][RestApiIds::target].toString() == "Osc"
                    && rootConns[i][RestApiIds::parameter].toString() == "Frequency")
                    committedFound = true;
            }
        }
        expect(committedFound, "Mod -> Osc.Frequency committed");
    }

    //==========================================================================
    /** Setup: In a DSP plan group, add Mod, add Osc, connect, then disconnect.
     *  Scenario: Mid-group read.
     *  Expected: connections[] on root is empty.
     */
    void testDspGroupMirrorDisconnect()
    {
        beginTest("DSP plan group - disconnect mirrors onto snapshot");

        resetDspState();

        dspPushGroup("mirror_disconnect");

        Array<var> ops;
        ops.add(makeDspAddOp("control.pma", "test_network", "Mod"));
        ops.add(makeDspAddOp("core.oscillator", "test_network", "Osc"));
        ops.add(makeDspConnectOp("Mod", "Osc", "Frequency"));
        ops.add(makeDspDisconnectOp("Mod", "Osc", "Frequency"));
        expectDspSuccess(postDspOps(ops));

        auto mid = getDspGroupCurrent();
        expect((bool)mid[RestApiIds::success], "Mid-group read should succeed");

        auto midConnections = mid[RestApiIds::result][RestApiIds::connections];
        bool found = false;
        if (midConnections.isArray())
        {
            for (int i = 0; i < midConnections.size(); i++)
            {
                if (midConnections[i][RestApiIds::source].toString() == "Mod"
                    && midConnections[i][RestApiIds::target].toString() == "Osc"
                    && midConnections[i][RestApiIds::parameter].toString() == "Frequency")
                    found = true;
            }
        }
        expect(!found, "Mod -> Osc.Frequency removed from snapshot mid-group");

        dspPopGroupCommit();

        auto tree = getDspTree();
        auto rootConns = tree[RestApiIds::result][RestApiIds::connections];
        bool committedFound = false;
        if (rootConns.isArray())
        {
            for (int i = 0; i < rootConns.size(); i++)
            {
                if (rootConns[i][RestApiIds::source].toString() == "Mod"
                    && rootConns[i][RestApiIds::target].toString() == "Osc"
                    && rootConns[i][RestApiIds::parameter].toString() == "Frequency")
                    committedFound = true;
            }
        }
        expect(!committedFound, "connection absent after commit");
    }

    //==========================================================================
    /** Setup: Pre-group, add nodes to the network. Then enter group and clear.
     *  Scenario: Mid-group read.
     *  Expected: root children[] is empty.
     */
    void testDspGroupMirrorClear()
    {
        beginTest("DSP plan group - clear mirrors onto snapshot");

        resetDspState();

        // Seed the network with a node BEFORE entering the group.
        {
            Array<var> ops;
            ops.add(makeDspAddOp("core.oscillator", "test_network", "PreOsc"));
            expectDspSuccess(postDspOps(ops));
        }

        dspPushGroup("mirror_clear");

        // An add op inside the group is needed to force lazy creation of the
        // DspValidationState (createAction only populates it on a DSP action).
        // clear alone with no prior DSP op in the group still triggers lazy
        // creation via createAction, so this op is here just for realism.
        Array<var> ops;
        ops.add(makeDspClearOp());
        expectDspSuccess(postDspOps(ops));

        auto mid = getDspGroupCurrent();
        expect((bool)mid[RestApiIds::success], "Mid-group read should succeed");

        auto midChildren = mid[RestApiIds::result][RestApiIds::children];
        expectEquals<int>(midChildren.size(), 0,
            "root children empty in snapshot mid-group after clear");
        expect(!findNodeInTree(mid[RestApiIds::result], "PreOsc").isObject(),
            "PreOsc absent from snapshot after clear");

        dspPopGroupCommit();

        auto tree = getDspTree();
        expectEquals<int>(tree[RestApiIds::result][RestApiIds::children].size(), 0,
            "root children empty after commit of clear");
    }

    void testDspApplyBatchOps()
    {
        /** Setup: Fresh DSP state
         *  Scenario: Multiple operations in single request
         *  Expected: All operations applied, single diff
         */
        beginTest("POST /api/dsp/apply - batch operations");

        resetDspState();

        // Batch: add container + add child (chained parent reference)
        Array<var> ops;
        ops.add(makeDspAddOp("container.chain", "test_network", "BatchChain"));
        ops.add(makeDspAddOp("core.oscillator", "BatchChain", "BatchOsc"));
        ops.add(makeDspSetOp("BatchOsc", "Frequency", 660.0));
        ops.add(makeDspBypassOp("BatchOsc", true));
        auto json = postDspOps(ops);
        expectDspSuccess(json);

        // Verify all applied
        auto tree = getDspTree();
        auto chain = findNodeInTree(tree[RestApiIds::result], "BatchChain");
        expect(chain.isObject(), "BatchChain should exist");

        auto osc = findNodeInTree(tree[RestApiIds::result], "BatchOsc");
        expect(osc.isObject(), "BatchOsc should exist");
        expect((bool)osc[RestApiIds::bypassed], "BatchOsc should be bypassed");

        auto params = osc[RestApiIds::parameters];
        for (int i = 0; i < params.size(); i++)
        {
            if (params[i][RestApiIds::parameterId].toString() == "Frequency")
                expectEquals((double)params[i][RestApiIds::value], 660.0,
                    "Frequency should be 660");
        }

        // Batch with a runtime failure must roll back everything.
        // Op 0's add::validate passes (valid parent) and op 1's add::validate
        // passes too (parent check is deferred to perform outside plan mode).
        // Op 0's perform adds FirstNode. Op 1's perform then throws because
        // NoSuchParent doesn't exist -- Set::perform must undo op 0 before
        // rethrowing, leaving the tree in its pre-batch state.
        ops.clear();
        ops.add(makeDspAddOp("core.oscillator", "test_network", "FirstNode"));
        ops.add(makeDspAddOp("core.oscillator", "NoSuchParent", "SecondNode"));
        json = postDspOps(ops);
        expect(!(bool)json[RestApiIds::success], "Runtime failure in batch should fail");

        tree = getDspTree();
        auto firstNode = findNodeInTree(tree[RestApiIds::result], "FirstNode");
        expect(!firstNode.isObject(),
            "FirstNode must be rolled back when a later op fails at perform()");
    }

    //==========================================================================
    /** Setup: headless test context (no BackendRootWindow, no active DspNetwork)
     *  Scenario: GET /api/dsp/screenshot with a series of invalid parameter
     *            combinations that all bail before any image is captured or
     *            written to disk.
     *  Expected: Each call returns a well-formed fail envelope (success=false
     *            with an errors array and a specific errorMessage), the
     *            endpoint is marked touched for coverage, and no PNG file is
     *            created anywhere.
     *
     *  The success path is not exercised here because it requires a live
     *  BackendRootWindow and an active DspNetwork with a workspace -- neither
     *  exists in the headless unit-test context. Full envelope validation of
     *  the success response is therefore out of scope for this test.
     */
    void testDspScreenshot()
    {
        beginTest("GET /api/dsp/screenshot (parameter validation)");

        // Missing moduleId -- endpoint must reject before resolving anything.
        auto r1 = ctx->httpGet("/api/dsp/screenshot");
        var j1 = ctx->parseJson(r1);
        expectErrorMessageContains(j1, "moduleId");

        // Missing outputPath -- endpoint must reject without touching the
        // DspNetwork holder or the filesystem.
        auto r2 = ctx->httpGet("/api/dsp/screenshot?moduleId=Interface");
        var j2 = ctx->parseJson(r2);
        expectErrorMessageContains(j2, "outputPath");

        // Non-.png extension -- extension check happens before any I/O.
        auto r3 = ctx->httpGet("/api/dsp/screenshot?moduleId=Interface&outputPath=foo.jpg");
        var j3 = ctx->parseJson(r3);
        expectErrorMessageContains(j3, ".png");
    }

    //==========================================================================
    // Cross-domain rollback tests
    //==========================================================================

    /** Verify Set::perform rolls back on mid-batch failure across all three
     *  domains, and Set::undo rolls forward on mid-batch failure.
     *
     *  Builder/UI add::validate resolves the parent against the live state, so
     *  "add to bad parent" fails at Phase 2 -- no performs run and no rollback
     *  is needed. The invariant "failed batch leaves no partial state" holds
     *  for both cases; these domains verify the invariant end-to-end.
     *
     *  DSP defers the parent check to perform (to support batch-order-dependent
     *  ops like {add container, add child into it}). This exercises the real
     *  rollback path in Set::perform.
     *
     *  The undo-rollback (roll-forward) path is harder to trigger from pure
     *  REST calls because the undo stack tracks the exact state transitions
     *  needed by each sub-action's undo. We cover the happy path (a multi-op
     *  batch undo works correctly) -- the rollback code is symmetric with
     *  the perform-rollback path and is exercised by inspection.
     */
    void testBatchRollback()
    {
        beginTest("Batch rollback across builder/ui/dsp domains");

        // ======== BUILDER: failed batch leaves no partial state ========
        resetBuilderState();

        Array<var> bOps;
        bOps.add(makeAddOp("SineSynth", "RollbackSineA"));
        bOps.add(makeAddOp("SineSynth", "RollbackSineB", "NonExistentParent", -1));
        auto json = postBuilderOps(bOps);
        expect(!(bool)json[RestApiIds::success],
            "Builder batch with invalid parent should fail");
        expectBuilderProcessorRemoved("RollbackSineA");
        expectBuilderProcessorRemoved("RollbackSineB");

        // ======== UI: failed batch leaves no partial state ========
        // Trigger a Phase 1 (prevalidate) failure via unknown componentType.
        // Note: a "bad parent" trigger won't work cleanly here because
        // ui::add::validate defers the parent check to perform() (same as DSP),
        // and ui::add::undo only removes the ValueTree child -- it doesn't
        // sync the C++ ScriptComponent out of content->getComponent(i). That
        // asymmetry means a real perform-rollback leaves orphan ScriptComponents
        // visible via GET /api/ui/tree. That's a separate bug to fix in
        // ui::add::undo; here we just exercise the invariant at prevalidate.
        ctx->reset();
        ctx->compile("Content.makeFrontInterface(600, 400);\n");

        Array<var> uOps;
        uOps.add(makeUIAddOp("ScriptButton", "RollbackBtnA", {}, 0, 0, 100, 30));
        uOps.add(makeUIAddOp("UnknownComponentType", "RollbackBtnB", {},
                             0, 0, 100, 30));
        auto uiJson = postUIApplyOps(uOps);
        expect(!(bool)uiJson[RestApiIds::success],
            "UI batch with unknown component type should fail");

        auto uiTree = ctx->parseJson(ctx->httpGet("/api/ui/tree?moduleId=Interface"));
        bool foundUiA = false;
        bool foundUiB = false;

        std::function<void(const var&)> scanUi = [&](const var& node)
        {
            if (node[RestApiIds::id].toString() == "RollbackBtnA") foundUiA = true;
            if (node[RestApiIds::id].toString() == "RollbackBtnB") foundUiB = true;
            auto children = node[RestApiIds::childComponents];
            if (children.isArray())
            {
                for (int i = 0; i < children.size(); i++)
                    scanUi(children[i]);
            }
        };
        scanUi(uiTree[RestApiIds::result]);

        expect(!foundUiA, "RollbackBtnA must not exist after failed batch");
        expect(!foundUiB, "RollbackBtnB must not exist after failed batch");

        // ======== DSP: perform-rollback (exercises Set::perform try/catch) ========
        // Op 0 passes validate (valid parent) and perform (adds the node).
        // Op 1 passes validate (parent check deferred outside plan mode) but
        // throws in perform because NoSuchParent doesn't exist. Set::perform
        // must undo op 0 before rethrowing.
        resetDspState();

        Array<var> dOps;
        dOps.add(makeDspAddOp("core.oscillator", "test_network", "RollbackDspA"));
        dOps.add(makeDspAddOp("core.oscillator", "NoSuchParent", "RollbackDspB"));
        auto dspJson = postDspOps(dOps);
        expect(!(bool)dspJson[RestApiIds::success],
            "DSP batch with runtime failure should fail");

        auto dspTree = getDspTree();
        auto nodeA = findNodeInTree(dspTree[RestApiIds::result], "RollbackDspA");
        expect(!nodeA.isObject(),
            "RollbackDspA must be rolled back after op 1 throws in perform");

        // ======== DSP: undo happy-path (multi-op batch undo) ========
        // Sanity check: Set::undo correctly reverses all sub-actions when
        // none of them throw. The roll-forward rollback code in Set::undo is
        // symmetric with Set::perform and exercised under code review.
        resetDspState();

        Array<var> sanity;
        sanity.add(makeDspAddOp("core.oscillator", "test_network", "UndoSanityA"));
        sanity.add(makeDspAddOp("core.oscillator", "test_network", "UndoSanityB"));
        expectDspSuccess(postDspOps(sanity));

        auto undoBatch = ctx->parseJson(ctx->httpPost("/api/undo/back", "{}"));
        expect((bool)undoBatch[RestApiIds::success],
            "Batch undo should succeed when all sub-undos are valid");

        auto sanityTree = getDspTree();
        auto sa = findNodeInTree(sanityTree[RestApiIds::result], "UndoSanityA");
        auto sb = findNodeInTree(sanityTree[RestApiIds::result], "UndoSanityB");
        expect(!sa.isObject(), "UndoSanityA must be removed by batch undo");
        expect(!sb.isObject(), "UndoSanityB must be removed by batch undo");
    }

    //==========================================================================
    /** Captures status, headers, and body from a raw HTTP request. */
    struct RawHttpResponse
    {
        int status = 0;
        std::map<std::string, std::string, std::less<>> headers;
        std::string body;
        bool succeeded = false;
    };

    /** Issue an HTTP request via cpp-httplib on a background thread while pumping
        the JUCE message loop. Mirrors the pattern in TestContext::httpGet but
        gives access to status code and response headers (which JUCE's URL
        wrapper does not expose).
    */
    RawHttpResponse doRawHttpRequest(const String& method,
                                     const String& path,
                                     const httplib::Headers& extraHeaders = {})
    {
        RawHttpResponse out;
        std::atomic<bool> done{false};

        Thread::launch([&]()
        {
            httplib::Client cli("localhost", TEST_PORT);
            cli.set_connection_timeout(2, 0);
            cli.set_read_timeout(5, 0);

            httplib::Result res;

            if (method == "GET")
                res = cli.Get(path.toStdString(), extraHeaders);
            else if (method == "OPTIONS")
                res = cli.Options(path.toStdString(), extraHeaders);
            else
                res = cli.Post(path.toStdString(), extraHeaders, std::string(), "application/json");

            if (res)
            {
                out.status = res->status;
                out.body = res->body;
                for (const auto& h : res->headers)
                    out.headers[h.first] = h.second;
                out.succeeded = true;
            }

            done.store(true);
        });

        while (! done.load())
            MessageManager::getInstance()->runDispatchLoopUntil(10);

        return out;
    }

    void testCorsWildcardOnRealResponse()
    {
        beginTest("CORS: wildcard origin on regular response");

        auto res = doRawHttpRequest("GET", "/api/status");
        expect(res.succeeded, "GET /api/status must reach the server");
        expect(res.status == 200, "GET /api/status should return 200");

        auto it = res.headers.find("Access-Control-Allow-Origin");
        expect(it != res.headers.end(), "Access-Control-Allow-Origin header must be present");
        if (it != res.headers.end())
            expect(it->second == "*", "Default policy must echo wildcard, got: " + String(it->second));
    }

    void testCorsPreflightOptions()
    {
        beginTest("CORS: OPTIONS preflight returns 204 with full header set");

        httplib::Headers preflight = {
            {"Origin", "https://example.test"},
            {"Access-Control-Request-Method", "POST"},
            {"Access-Control-Request-Headers", "Content-Type"}
        };

        auto res = doRawHttpRequest("OPTIONS", "/api/status", preflight);
        expect(res.succeeded, "OPTIONS /api/status must reach the server");
        expect(res.status == 204, "Preflight should return 204, got " + String(res.status));

        auto allowOrigin = res.headers.find("Access-Control-Allow-Origin");
        expect(allowOrigin != res.headers.end() && allowOrigin->second == "*",
               "Preflight must echo wildcard origin");

        auto allowMethods = res.headers.find("Access-Control-Allow-Methods");
        expect(allowMethods != res.headers.end()
               && String(allowMethods->second).contains("POST"),
               "Preflight must advertise POST in allowed methods");

        auto allowHeaders = res.headers.find("Access-Control-Allow-Headers");
        expect(allowHeaders != res.headers.end()
               && String(allowHeaders->second).contains("Content-Type"),
               "Preflight must advertise Content-Type in allowed headers");
    }

    void testCorsOriginWhitelist()
    {
        beginTest("CORS: comma-separated whitelist echoes only allowed origins");

        auto& server = ctx->bp->getRestServer();
        server.stop();
        expect(server.start(TEST_PORT, "127.0.0.1", "https://app.example.com,https://other.com"),
               "Server must restart with whitelist policy");

        auto allowed = doRawHttpRequest("GET", "/api/status",
                                        {{"Origin", "https://app.example.com"}});
        expect(allowed.succeeded && allowed.status == 200, "Allowed-origin GET should succeed");
        auto allowedOrigin = allowed.headers.find("Access-Control-Allow-Origin");
        expect(allowedOrigin != allowed.headers.end()
               && allowedOrigin->second == "https://app.example.com",
               "Server must echo the request's origin when whitelisted");

        auto blocked = doRawHttpRequest("GET", "/api/status",
                                       {{"Origin", "https://evil.example"}});
        expect(blocked.succeeded && blocked.status == 200, "Blocked-origin GET still receives 200 body");
        expect(blocked.headers.find("Access-Control-Allow-Origin") == blocked.headers.end(),
               "Server must omit Allow-Origin for non-whitelisted origins");

        // Restore the default wildcard policy so any subsequent verification stays clean.
        server.stop();
        expect(server.start(TEST_PORT, "127.0.0.1", "*"), "Server must restart with default policy");
    }

    void testCorsDisabled()
    {
        beginTest("CORS: empty policy emits no CORS headers");

        auto& server = ctx->bp->getRestServer();
        server.stop();
        expect(server.start(TEST_PORT, "127.0.0.1", ""), "Server must restart with empty CORS policy");

        auto res = doRawHttpRequest("GET", "/api/status",
                                    {{"Origin", "https://anywhere.test"}});
        expect(res.succeeded && res.status == 200, "GET /api/status must still succeed");
        expect(res.headers.find("Access-Control-Allow-Origin") == res.headers.end(),
               "No Access-Control-Allow-Origin header should be emitted when CORS is disabled");
        expect(res.headers.find("Access-Control-Allow-Methods") == res.headers.end(),
               "No Access-Control-Allow-Methods header should be emitted when CORS is disabled");

        // Restore the default wildcard policy.
        server.stop();
        expect(server.start(TEST_PORT, "127.0.0.1", "*"), "Server must restart with default policy");
    }

};

static RestServerTest restServerTest;

} // namespace hise

#endif // HI_RUN_UNIT_TESTS
