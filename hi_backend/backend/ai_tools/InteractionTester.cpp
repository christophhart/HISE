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
*   ===========================================================================
*/


/** TODO:

Fixes for semantic mode:

- fails with inserting table points
- add content restore
- show subtarget in log when click




- filter console log. error ? only show error list, otherwise: just show log list
- set the green indicator to match the error state (so if a script error occurs, make it red)
- append the initial input list. so log window only shows: input list -> console log or error per run.
- test popupment properly - panels, right click, etc!
- add a simple replay button
- make a comprehensive guide for the MCP server 
- add get screenshot with downscaling & cropping. remove entire old screenshot endpoint
- add autoscroll
- add subcomponent resolving to allow more complex components to work (eg. Preset browser or slider packs).

## big feature: project-based testing system! 

- record (!) user interaction with event filtering & thinning
- record audio output too! make spectrogram with downsampling endpoint
- use profiling toolkit
- add MIDI event logic!
- record audio
- store in projectfolder/Tests
- check with scriptnode test system
- add autodump for screenshots, use projectfolder/Images/screenshots as default...
- add embedded tool in HISE that replicates my current debug script.

*/

namespace hise {
using namespace juce;


//==============================================================================
// InteractionTester implementation

InteractionTester::InteractionTester(BackendProcessor* bp)
    : backendProcessor(bp)
{
    jassert(backendProcessor != nullptr);
}

InteractionTester::~InteractionTester()
{
    closeWindow();
}

void InteractionTester::resetMouseState()
{
    mouseState.reset();
}

void InteractionTester::logResponse(const var& inputInteractions, const String& jsonResponse, bool success)
{
    if (window == nullptr) return;
    
    auto* sb = window->getStatusBar();
    if (sb == nullptr) return;
    
    // Append formatted log entry
    sb->appendToLog(inputInteractions, jsonResponse, success);
    
    // Update indicator if script errors occurred
    var parsed = JSON::parse(jsonResponse);
    if (auto* obj = parsed.getDynamicObject())
    {
        auto errorsVar = obj->getProperty(RestApiIds::errors);
        bool hasScriptErrors = errorsVar.isArray() && errorsVar.size() > 0;
        sb->updateFinalState(hasScriptErrors);
    }
}

InteractionTester::TestResult InteractionTester::executeInteractions(const var& interactionsJson, bool verbose)
{
    TestResult result;
    
    jassert(MessageManager::getInstance()->isThisTheMessageThread());
    
    // Check for wrapper format with mode indicator
    var actualInteractions = interactionsJson;
    bool isRawMode = false;
    
    if (auto* obj = interactionsJson.getDynamicObject())
    {
        Identifier mode = Identifier(obj->getProperty(InteractionIds::mode).toString());
        if (mode == InteractionIds::raw || mode == InteractionIds::semantic)
        {
            actualInteractions = obj->getProperty(RestApiIds::interactions);
            isRawMode = (mode == InteractionIds::raw);
        }
        // If no mode property, treat as legacy format (semantic)
    }
    
    // Make a local copy to avoid dangling references during replay
    var interactionsCopy = interactionsJson;
    
    // Get interface processor
    auto* processor = getInterfaceProcessor();
    if (processor == nullptr)
    {
        result.errorMessage = "moduleId 'Interface' not found";
        return result;
    }
    
    // Ensure window is open
    ensureWindowOpen();
    
    if (window == nullptr || !window->isVisible())
    {
        result.errorMessage = "Failed to create test window";
        return result;
    }
    
    // Capture console if not already being captured by REST handler
    RestHelpers::ReplayConsoleHandler consoleHandler(backendProcessor, window.get(),
        !backendProcessor->getConsoleHandler().hasCustomLogger());
    
    // Execute with real executor
    auto* pwsc = dynamic_cast<ProcessorWithScriptingContent*>(processor);
    InteractionTestWindow::RealExecutor executor(window.get(), pwsc);
    
    // Sync executor cursor position with our mouse state
    executor.setCursorPosition(mouseState.pixelPosition);
    
    InteractionDispatcher dispatcher;
    
    // Wire up status bar as progress listener
    if (auto* statusBar = window->getStatusBar())
        dispatcher.setProgressListener(statusBar);
    
    InteractionDispatcher::ExecutionResult execResult;
    
    if (isRawMode)
    {
        // Raw mode: execute events directly without parsing/normalization
        execResult = executeRawEvents(actualInteractions, executor, result.executionLog);
    }
    else
    {
        // Semantic mode: parse, normalize, and execute
        Array<InteractionParser::Interaction> interactions;
        auto parseResult = InteractionParser::parseInteractions(actualInteractions, interactions);
        
        if (parseResult.failed())
        {
            result.errorMessage = parseResult.getErrorMessage();
            return result;
        }
        
        result.parseWarnings = parseResult.warnings;
        
        if (interactions.isEmpty())
        {
            result.success = true;
            return result;
        }
        
        // NORMALIZE: Auto-insert moveTo events
        normalizeSequence(interactions);
        
        execResult = dispatcher.execute(interactions, executor, result.executionLog);
    }
    
    // Update our mouse state from executor
    mouseState.pixelPosition = executor.getCurrentCursorPosition();
    
    result.success = execResult.result.wasOk();
    result.errorMessage = execResult.result.getErrorMessage();
    result.interactionsCompleted = execResult.interactionsCompleted;
    result.totalElapsedMs = execResult.totalElapsedMs;
    result.screenshots = executor.getScreenshots();
    result.replResults = dispatcher.getReplResults();
    result.selectedMenuItem = dispatcher.getLastSelectedMenuItem();
    
    if (verbose)
    {
        result.finalMouseState = mouseState;
    }
    
    // Store for replay & enable replay button
    if (window != nullptr && window->getTopBar() != nullptr)
    {
        window->getTopBar()->setRecordedInteractions(interactionsCopy);
        window->getTopBar()->setHasRecording(true);
    }
    
    // Finalize console capture and log to StatusBar
    if (consoleHandler.isCapturing())
    {
        consoleHandler.finalize(interactionsCopy, result.success, 
                                result.interactionsCompleted, result.totalElapsedMs);
    }
    
    return result;
}

bool InteractionTester::isWindowOpen() const
{
    return window != nullptr && window->isVisible();
}

void InteractionTester::closeWindow()
{
    window = nullptr;
}

void InteractionTester::ensureWindowOpen()
{
    if (window != nullptr && !window->isVisible())
    {
        window = nullptr;
    }
    
    if (window == nullptr)
    {
        // Reset mouse state when creating new window
        resetMouseState();
        
        auto* processor = getInterfaceProcessor();
        if (processor != nullptr)
        {
            auto* pwsc = dynamic_cast<ProcessorWithScriptingContent*>(processor);
            if (pwsc != nullptr)
            {
                window = std::make_unique<InteractionTestWindow>(pwsc, this);
            }
        }
    }
}

JavascriptMidiProcessor* InteractionTester::getInterfaceProcessor() const
{
    if (backendProcessor == nullptr)
        return nullptr;
    
    return JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(backendProcessor);
}

File InteractionTester::getTestsFolder() const
{
    if (backendProcessor == nullptr)
        return File();
    
    return GET_PROJECT_HANDLER(backendProcessor->getMainSynthChain()).getWorkDirectory().getChildFile("Tests");
}

//==============================================================================
// Normalization - Auto-insert moveTo events

void InteractionTester::normalizeSequence(Array<InteractionParser::Interaction>& interactions)
{
    if (interactions.isEmpty())
        return;
    
    Array<InteractionParser::Interaction> normalized;
    
    for (auto& interaction : interactions)
    {
        auto& mouse = interaction.mouse;
        
        // Check if moveTo needed before this interaction
        auto moveToResult = createMoveToIfNeeded(mouse);
        
        if (moveToResult.needed)
        {
            InteractionParser::Interaction moveInteraction;
            moveInteraction.mouse = moveToResult.moveTo;
            
            // Transfer delay from original interaction to the moveTo
            moveInteraction.mouse.delayMs = mouse.delayMs;
            mouse.delayMs = 0;
            
            normalized.add(moveInteraction);
            
            // Update state after moveTo
            updateMouseStateForNormalization(moveInteraction.mouse);
        }
        
        // Add original interaction
        normalized.add(interaction);
        
        // Update state after original interaction
        updateMouseStateForNormalization(mouse);
    }
    
    interactions = std::move(normalized);
}

InteractionTester::MoveToResult 
InteractionTester::createMoveToIfNeeded(const InteractionParser::MouseInteraction& next)
{
    using Type = InteractionParser::MouseInteraction::Type;
    
    switch (next.type)
    {
        case Type::Click:
        case Type::Drag:
        {
            // Need moveTo if:
            // 1. Different target, OR
            // 2. Same target but explicit non-center position
            
            bool differentTarget = (mouseState.currentTarget != next.target);
            bool explicitPosition = !next.position.isCenter();
            
            if (differentTarget || explicitPosition)
            {
                MoveToResult result;
                result.needed = true;
                result.moveTo.type = Type::MoveTo;
                result.moveTo.target = next.target;
                result.moveTo.position = next.position;
                result.moveTo.durationMs = InteractionConstants::DefaultMoveDurationMs;
                result.moveTo.delayMs = 0;  // Will be set by caller
                result.moveTo.autoInserted = true;
                return result;
            }
            break;
        }
        
        case Type::MoveTo:
        case Type::SelectMenuItem:
        case Type::Screenshot:
        case Type::Repl:
            // No auto-insertion needed
            break;
    }
    
    return MoveToResult();  // needed = false by default
}

void InteractionTester::updateMouseStateForNormalization(const InteractionParser::MouseInteraction& interaction)
{
    using Type = InteractionParser::MouseInteraction::Type;
    
    switch (interaction.type)
    {
        case Type::MoveTo:
            mouseState.currentTarget = interaction.target;
            // Note: pixelPosition will be updated during actual execution
            // when we have access to resolved component bounds
            break;
            
        case Type::Click:
            // State unchanged (mouse stays where it is)
            break;
            
        case Type::Drag:
            // Position changes by delta - actual update happens during execution
            // Target stays the same
            break;
            
        case Type::SelectMenuItem:
            // Menu closes, reset to unknown state
            mouseState.currentTarget.clear();
            break;
            
        case Type::Screenshot:
        case Type::Repl:
            // No state change
            break;
    }
}

//==============================================================================
// Raw event execution (for recorded raw mode)

InteractionDispatcher::ExecutionResult InteractionTester::executeRawEvents(
    const var& eventsArray,
    InteractionExecutorBase& executor,
    Array<var>& executionLog)
{
    if (!eventsArray.isArray())
    {
        return InteractionDispatcher::ExecutionResult::fail("Raw events must be an array", 0, 0);
    }
    
    int64 startTime = Time::getMillisecondCounter();
    int eventsCompleted = 0;
    
    // Notify progress listener if available
    if (auto* statusBar = window->getStatusBar())
    {
        int totalEvents = eventsArray.size();
        int estimatedMs = 0;
        if (totalEvents > 0)
        {
            // Estimate duration from last event's timestamp
            auto lastEvent = eventsArray[totalEvents - 1];
            estimatedMs = (int)lastEvent.getProperty(InteractionIds::timestamp, 0);
        }
        statusBar->onSequenceStarted(totalEvents, estimatedMs);
    }
    
    for (int i = 0; i < eventsArray.size(); i++)
    {
        auto& event = eventsArray[i];
        String type = event.getProperty(RestApiIds::type, "").toString();
        int x = (int)event.getProperty(RestApiIds::x, 0);
        int y = (int)event.getProperty(RestApiIds::y, 0);
        int timestamp = (int)event.getProperty(InteractionIds::timestamp, 0);
        bool rightClick = (bool)event.getProperty(InteractionIds::rightClick, false);
        
        // Parse modifiers
        ModifierKeys mods;
        var modsVar = event.getProperty(InteractionIds::modifiers, var());
        if (auto* modsObj = modsVar.getDynamicObject())
        {
            int modFlags = 0;
            if ((bool)modsObj->getProperty(InteractionIds::shiftDown))
                modFlags |= ModifierKeys::shiftModifier;
            if ((bool)modsObj->getProperty(InteractionIds::ctrlDown))
                modFlags |= ModifierKeys::ctrlModifier;
            if ((bool)modsObj->getProperty(InteractionIds::altDown))
                modFlags |= ModifierKeys::altModifier;
            if ((bool)modsObj->getProperty(InteractionIds::cmdDown))
                modFlags |= ModifierKeys::commandModifier;
            mods = ModifierKeys(modFlags);
        }
        
        Point<int> pos(x, y);
        
        // Wait until the correct timestamp
        int64 targetTime = startTime + timestamp;
        int64 now = Time::getMillisecondCounter();
        if (targetTime > now)
        {
            int waitMs = (int)(targetTime - now);
            // Pump message loop while waiting
            while (Time::getMillisecondCounter() < targetTime)
            {
                MessageManager::getInstance()->runDispatchLoopUntil(jmin(10, waitMs));
            }
        }
        
        int elapsedMs = (int)(Time::getMillisecondCounter() - startTime);
        
        // Add button modifier flag for mouseDown/mouseUp
        if (type == InteractionIds::mouseDown.toString() || type == InteractionIds::mouseUp.toString())
        {
            mods = mods.withFlags(rightClick ? ModifierKeys::rightButtonModifier 
                                             : ModifierKeys::leftButtonModifier);
        }
        
        // Execute the event
        if (type == InteractionIds::mouseDown.toString())
        {
            // Wiggle to update componentUnderMouse
            executor.executeMouseMove(pos + Point<int>(2, 0), {}, elapsedMs);
            executor.executeMouseMove(pos, {}, elapsedMs);

            executor.executeMouseDown(pos, mods, rightClick, elapsedMs);
        }
        else if (type == InteractionIds::mouseUp.toString())
        {
            executor.executeMouseUp(pos, mods, rightClick, elapsedMs);

            // UI settle time
            MessageManager::getInstance()->runDispatchLoopUntil(50);
        }
        else if (type == InteractionIds::mouseMove.toString())
        {
            executor.executeMouseMove(pos, mods, elapsedMs);
        }
        
        // Add to execution log
        DynamicObject::Ptr logEntry = new DynamicObject();
        logEntry->setProperty(RestApiIds::type, type);
        logEntry->setProperty(RestApiIds::x, x);
        logEntry->setProperty(RestApiIds::y, y);
        logEntry->setProperty(InteractionIds::timestamp, elapsedMs);
        executionLog.add(var(logEntry.get()));
        
        eventsCompleted++;
        
        // Update progress
        if (auto* statusBar = window->getStatusBar())
        {
            statusBar->onInteractionCompleted(i);
        }
    }
    
    int totalElapsedMs = (int)(Time::getMillisecondCounter() - startTime);
    
    // Notify completion
    if (auto* statusBar = window->getStatusBar())
    {
        statusBar->onSequenceCompleted(true, "");
    }
    
    return InteractionDispatcher::ExecutionResult::ok(totalElapsedMs, eventsCompleted);
}

} // namespace hise
