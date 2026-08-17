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

- fails with inserting table points OK
- add content restore
- show subtarget in log when click OK
- add a simple replay button OK

- filter console log. error ? only show error list, otherwise: just show log list
- set the green indicator to match the error state (so if a script error occurs, make it red)
- append the initial input list. so log window only shows: input list -> console log or error per run.
- test popupment properly - panels, right click, etc!
- make a comprehensive guide for the MCP server 
- add get screenshot with downscaling & cropping. remove entire old screenshot endpoint
- add autoscroll


- big feature: project-based testing system! 
- record audio output too! make spectrogram with downsampling endpoint
- use profiling toolkit
- add MIDI event logic!
- add REPL for evaluating HiseScript expressions at any given time
- store in projectfolder/Tests
- check with scriptnode test system
- add autodump for screenshots, use projectfolder/Images/screenshots as default...

*/

namespace hise {
using namespace juce;

//==============================================================================
// Debug helper for generating unit test data from recorded sessions
//==============================================================================

//==============================================================================
// InteractionTestWindow::CursorOverlay implementation

InteractionTestWindow::CursorOverlay::CursorOverlay()
{
    setInterceptsMouseClicks(false, false);
}

void InteractionTestWindow::CursorOverlay::setState(State newState)
{
    if (state != newState)
    {
        state = newState;
        repaint();
    }
}

void InteractionTestWindow::CursorOverlay::setPosition(Point<int> pixelPos)
{
    position = pixelPos;
    repaint();
}

void InteractionTestWindow::CursorOverlay::paint(Graphics& g)
{
    painted = true;
    
    auto pixelPos = position.toFloat();
    
    auto cursorBounds = Rectangle<float>(cursorDiameter, cursorDiameter)
        .withCentre(pixelPos);
    
    g.setColour(getColourForState());
    g.fillEllipse(cursorBounds);
    
    g.setColour(Colours::white);
    g.drawEllipse(cursorBounds.reduced(borderWidth / 2.0f), borderWidth);
    
    g.setColour(Colours::white.withAlpha(0.7f));
    g.drawLine(pixelPos.x - 3, pixelPos.y, pixelPos.x + 3, pixelPos.y, 1.0f);
    g.drawLine(pixelPos.x, pixelPos.y - 3, pixelPos.x, pixelPos.y + 3, 1.0f);
}

Colour InteractionTestWindow::CursorOverlay::getColourForState() const
{
    switch (state)
    {
        case State::Idle:     return Colours::grey;
        case State::Hovering: return Colours::yellow;
        case State::Clicking: return Colours::green;
        case State::Dragging: return Colours::orange;
        default:              return Colours::grey;
    }
}

//==============================================================================
// InteractionTestWindow::RecordingListener implementation

void InteractionTestWindow::RecordingListener::setRecording(bool enabled, int64 startTime)
{
    recording = enabled;
    recordingStartTime = startTime;
}

void InteractionTestWindow::RecordingListener::addEvent(const Identifier& type, const MouseEvent& e)
{
    if (!recording) return;
    
    // Get the source component from the event
    auto* sourceComponent = e.eventComponent;
    if (sourceComponent == nullptr) return;
    
    // Navigate up to find InteractionTestWindow
    auto* window = sourceComponent->findParentComponentOfClass<InteractionTestWindow>();
    if (window == nullptr) return;
    
    // Filter out clicks on TopBar and StatusBar (not part of test interface)
    if (sourceComponent->findParentComponentOfClass<TopBar>() != nullptr ||
        sourceComponent->findParentComponentOfClass<StatusBar>() != nullptr)
        return;
    
    auto* topBar = window->getTopBar();
    if (topBar == nullptr) return;
    
    int64 timestamp = Time::getMillisecondCounter() - recordingStartTime;
    
    // Check for popup menu selection on mouseDown
    if (type == InteractionIds::mouseDown)
    {
        auto menuInfo = detectPopupMenuSelection(e);
        
        if (menuInfo.isValid())
        {
            // Store for mouseUp to emit the selectMenuItem event
            pendingMenuSelection = menuInfo;
            return;  // Don't record as normal mouseDown
        }
    }
    
    // Emit selectMenuItem event on mouseUp if we have a pending menu selection
    if (type == InteractionIds::mouseUp && pendingMenuSelection.isValid())
    {
        DynamicObject::Ptr event = new DynamicObject();
        event->setProperty(RestApiIds::type, InteractionIds::selectMenuItem.toString());
        event->setProperty(RestApiIds::itemId, pendingMenuSelection.itemId);
        if (pendingMenuSelection.itemText.isNotEmpty())
            event->setProperty(InteractionIds::menuItemText, pendingMenuSelection.itemText);
        event->setProperty(InteractionIds::timestamp, (int)timestamp);
        
        topBar->addRawEvent(var(event.get()));
        
        pendingMenuSelection.reset();
        return;
    }
    
    // Convert coordinates to ScriptContentComponent space
    auto* scc = window->getContent();
    if (scc == nullptr) return;
    
    Point<int> posInScc = scc->getLocalPoint(sourceComponent, e.getPosition());
    
    // Resolve target component at this moment (captures current UI state)
    InteractionParser::ComponentTargetPath resolvedTarget(InteractionIds::content.toString());
    Array<var> targetBoundsArray;
    
    if (auto* pwsc = window->getProcessor())
    {
        RealExecutor tempExecutor(window, pwsc);
        auto resolveResult = tempExecutor.resolvePosition(posInScc);
        
        if (!resolveResult.target.isEmpty())
        {
            resolvedTarget = resolveResult.target;
            targetBoundsArray.add(resolveResult.componentBounds.getX());
            targetBoundsArray.add(resolveResult.componentBounds.getY());
            targetBoundsArray.add(resolveResult.componentBounds.getWidth());
            targetBoundsArray.add(resolveResult.componentBounds.getHeight());
        }
    }
    
    DynamicObject::Ptr event = new DynamicObject();
    event->setProperty(RestApiIds::type, type.toString());
    event->setProperty(RestApiIds::x, posInScc.x);
    event->setProperty(RestApiIds::y, posInScc.y);
    event->setProperty(InteractionIds::timestamp, (int)timestamp);
    
    // Attach resolved target info
    resolvedTarget.toVar(event.get());
    if (!targetBoundsArray.isEmpty())
        event->setProperty(InteractionIds::targetBounds, targetBoundsArray);
    
    // For subtarget interactions, also store normalized position within parent component
    // This enables fallback when subtarget doesn't exist at replay time (e.g., TableEditor point creation)
    if (resolvedTarget.hasSubtarget())
    {
        if (auto* pwsc = window->getProcessor())
        {
            RealExecutor tempExecutor(window, pwsc);
            InteractionParser::ComponentTargetPath parentOnly(resolvedTarget.componentId);
            auto parentResult = tempExecutor.resolveTarget(parentOnly);
            
            if (parentResult.success() && !parentResult.componentBounds.isEmpty())
            {
                float normX = (posInScc.x - parentResult.componentBounds.getX()) 
                            / (float)parentResult.componentBounds.getWidth();
                float normY = (posInScc.y - parentResult.componentBounds.getY()) 
                            / (float)parentResult.componentBounds.getHeight();
                
                DynamicObject::Ptr normPos = new DynamicObject();
                normPos->setProperty(RestApiIds::x, normX);
                normPos->setProperty(RestApiIds::y, normY);
                event->setProperty(InteractionIds::normalizedPositionInTarget, var(normPos.get()));
            }
        }
    }
    
    if (type == InteractionIds::mouseDown || type == InteractionIds::mouseUp)
    {
        event->setProperty(InteractionIds::rightClick, e.mods.isRightButtonDown());
    }
    
    // Store modifier state
    if (e.mods.isShiftDown() || e.mods.isCtrlDown() || e.mods.isAltDown() || e.mods.isCommandDown())
    {
        DynamicObject::Ptr mods = new DynamicObject();
        mods->setProperty(InteractionIds::shiftDown, e.mods.isShiftDown());
        mods->setProperty(InteractionIds::ctrlDown, e.mods.isCtrlDown());
        mods->setProperty(InteractionIds::altDown, e.mods.isAltDown());
        mods->setProperty(InteractionIds::cmdDown, e.mods.isCommandDown());
        event->setProperty(InteractionIds::modifiers, var(mods.get()));
    }
    
    topBar->addRawEvent(var(event.get()));
}

void InteractionTestWindow::RecordingListener::mouseDown(const MouseEvent& e)
{
    addEvent(InteractionIds::mouseDown, e);
}

void InteractionTestWindow::RecordingListener::mouseUp(const MouseEvent& e)
{
    addEvent(InteractionIds::mouseUp, e);
}

void InteractionTestWindow::RecordingListener::mouseMove(const MouseEvent& e)
{
    addEvent(InteractionIds::mouseMove, e);
}

void InteractionTestWindow::RecordingListener::mouseDrag(const MouseEvent& e)
{
    // Treat drag as mouseMove (the down state is already tracked)
    addEvent(InteractionIds::mouseMove, e);
}

InteractionTestWindow::RecordingListener::PopupMenuSelectionInfo
InteractionTestWindow::RecordingListener::detectPopupMenuSelection(const MouseEvent& e)
{
    PopupMenuSelectionInfo result;
    
    // Get all visible menu items from open popup menus
    auto menuItems = PopupMenu::getVisibleMenuItems(e.eventComponent);
    
    if (menuItems.isEmpty())
        return result;  // No popup menu open
    
    // Get the click position in screen coordinates
    Point<int> screenPos = e.getScreenPosition();
    
    // Check if click is inside any menu item's bounds
    for (const auto& item : menuItems)
    {
        if (item.screenBounds.contains(screenPos))
        {
            result.isMenuClick = true;
            result.itemId = item.itemId;
            result.itemText = item.text;
            return result;
        }
    }
    
    return result;  // Click was not on a menu item (e.g., menu background or outside)
}

//==============================================================================
// InteractionTestWindow::TopBar implementation

Path InteractionTestWindow::TopBar::ButtonPathFactory::createPath(const String& url) const
{
    Path p;
    
    if (url == "record")
    {
        // Simple filled circle for record button
        p.addEllipse(0.0f, 0.0f, 1.0f, 1.0f);
        return p;
    }
    
    LOAD_EPATH_IF_URL("replay", EditorIcons::swapIcon);
    LOAD_EPATH_IF_URL("new", SampleMapIcons::newSampleMap);
    LOAD_EPATH_IF_URL("save", SampleMapIcons::saveSampleMap);
    LOAD_EPATH_IF_URL("load", SampleMapIcons::loadSampleMap);
    
    return p;
}

InteractionTestWindow::TopBar::TopBar()
{
    // Mode toggle button
    modeToggle = new TextButton("Semantic");
    modeToggle->setButtonText("Semantic");
    modeToggle->setTooltip("Toggle recording mode: Semantic (detect clicks/drags) or Raw (exact mouse replay)");
    modeToggle->addListener(this);
    modeToggle->setColour(TextButton::buttonColourId, Colour(0xFF555555));
    modeToggle->setColour(TextButton::textColourOffId, Colours::white);
    addAndMakeVisible(modeToggle);
    
    // Group 1: Record, Replay
    recordButton = new HiseShapeButton("record", this, pathFactory, "record");
    recordButton->setTooltip("Record mouse interactions");
    addAndMakeVisible(recordButton);
    
    replayButton = new HiseShapeButton("replay", this, pathFactory, "replay");
    replayButton->setTooltip("Replay recorded interactions");
    replayButton->setEnabled(false);
    addAndMakeVisible(replayButton);
    
    // Group 2: New, Open, Save
    clearButton = new HiseShapeButton("new", this, pathFactory, "new");
    clearButton->setTooltip("New session");
    addAndMakeVisible(clearButton);
    
    loadButton = new HiseShapeButton("load", this, pathFactory, "load");
    loadButton->setTooltip("Load session from Tests folder");
    addAndMakeVisible(loadButton);
    
    saveButton = new HiseShapeButton("save", this, pathFactory, "save");
    saveButton->setTooltip("Save session to Tests folder");
    saveButton->setEnabled(false);
    addAndMakeVisible(saveButton);
    
    // Session name editor
    sessionNameEditor = new TextEditor("sessionName");
    sessionNameEditor->setText(sessionName, dontSendNotification);
    sessionNameEditor->setFont(GLOBAL_BOLD_FONT());
    GlobalHiseLookAndFeel::setTextEditorColours(*sessionNameEditor);
    addAndMakeVisible(sessionNameEditor);
}

void InteractionTestWindow::TopBar::paint(Graphics& g)
{
    auto b = getLocalBounds();
    
    // Background (same as StatusBar)
    g.setColour(Colour(0xFF333333));
    g.fillRect(b);
    
    // Bottom border line
    g.setColour(Colours::black.withAlpha(0.5f));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());
}

void InteractionTestWindow::TopBar::resized()
{
    auto b = getLocalBounds();
    
    int buttonSize = 32;
    int toggleWidth = 70;
    int padding = 4;
    int groupMargin = 12;  // Larger margin between button groups
    
    b.removeFromLeft(padding);
    
    // Mode toggle (before record button)
    modeToggle->setBounds(b.removeFromLeft(toggleWidth).reduced(0, 8));
    b.removeFromLeft(padding);
    
    // Group 1: Record, Replay
    recordButton->setBounds(b.removeFromLeft(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize).reduced(4));
    b.removeFromLeft(padding);
    replayButton->setBounds(b.removeFromLeft(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize).reduced(4));
    
    b.removeFromLeft(groupMargin);
    
    // Group 2: New, Open, Save
    clearButton->setBounds(b.removeFromLeft(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize).reduced(4));
    b.removeFromLeft(padding);
    loadButton->setBounds(b.removeFromLeft(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize).reduced(4));
    b.removeFromLeft(padding);
    saveButton->setBounds(b.removeFromLeft(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize).reduced(4));
    b.removeFromLeft(padding);
    
    // Session name editor fills remaining space
    sessionNameEditor->setBounds(b.reduced(4, 6));
}

void InteractionTestWindow::TopBar::buttonClicked(Button* b)
{
    if (b == modeToggle.get())
    {
        // Toggle between Semantic and Raw modes
        if (recordingMode == RecordingMode::Semantic)
        {
            recordingMode = RecordingMode::Raw;
            modeToggle->setButtonText("Raw");
        }
        else
        {
            recordingMode = RecordingMode::Semantic;
            modeToggle->setButtonText("Semantic");
        }
    }
    else if (b == recordButton.get())
    {
        if (!recording && !inCountdown)
        {
            // Start countdown sequence
            startRecordingCountdown();
        }
        else if (recording)
        {
            // Stop recording early (user clicked during recording)
            stopRecording();
        }
        // If in countdown, ignore clicks
    }
    else if (b == replayButton.get())
    {
        // Replay the recorded/loaded interactions
        if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
        {
            if (auto* t = window->getInteractionTester())
            {
                t->executeInteractions(recordedInteractions, false);
            }
        }
    }
    else if (b == clearButton.get())
    {
        // Clear current session
        recordedInteractions = var();
        setHasRecording(false);
        
        // Bump session name if folder exists
        if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
        {
            if (auto* t = window->getInteractionTester())
            {
                setSessionName(generateSessionName(t->getTestsFolder()));
            }
        }
    }
    else if (b == saveButton.get())
    {
        // Save logic (Phase 3)
    }
    else if (b == loadButton.get())
    {
        // Load logic (Phase 3)
    }
}

void InteractionTestWindow::TopBar::setRecording(bool isRecording)
{
    recording = isRecording;
    
    // Update button appearance
    if (recordButton != nullptr)
    {
        if (recording)
        {
            // Red color when recording
            recordButton->setColours(Colour(0xFFFF4444), Colour(0xFFFF6666), Colour(0xFFFF2222));
        }
        else
        {
            // Default colors
            recordButton->setColours(Colours::white.withAlpha(0.7f), 
                                     Colours::white, 
                                     Colours::white.withAlpha(0.5f));
        }
        recordButton->repaint();
    }
    
    updateButtonStates();
    repaint();
}

void InteractionTestWindow::TopBar::updateButtonStates()
{
    bool busy = recording || inCountdown;
    
    // Disable all buttons during recording or countdown
    modeToggle->setEnabled(!busy);
    replayButton->setEnabled(!busy && hasRecordingData);
    saveButton->setEnabled(!busy && hasRecordingData);
    loadButton->setEnabled(!busy);
    clearButton->setEnabled(!busy);
}

void InteractionTestWindow::TopBar::timerCallback()
{
    if (inCountdown)
    {
        countdownValue--;
        
        if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
        {
            if (auto* statusBar = window->getStatusBar())
            {
                if (countdownValue > 0)
                {
                    statusBar->showCountdown(countdownValue);
                }
                else
                {
                    // Countdown finished, start actual recording
                    inCountdown = false;
                    startRecording();
                }
            }
        }
    }
    else if (recording)
    {
        // Check if recording duration has expired
        auto elapsed = Time::getMillisecondCounter() - recordingStartTime;
        auto remaining = RECORDING_DURATION_SECONDS - (int)(elapsed / 1000);
        
        if (remaining <= 0)
        {
            stopRecording();
        }
        else
        {
            // Update status bar with remaining time
            if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
            {
                if (auto* statusBar = window->getStatusBar())
                {
                    statusBar->showRecordingActive(remaining);
                }
            }
        }
    }
}

void InteractionTestWindow::TopBar::startRecordingCountdown()
{
    inCountdown = true;
    countdownValue = COUNTDOWN_SECONDS;
    rawRecordedEvents.clear();
    
    updateButtonStates();
    
    // Show initial countdown
    if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
    {
        if (auto* statusBar = window->getStatusBar())
        {
            statusBar->showCountdown(countdownValue);
        }
        
        // Hide cursor overlay during countdown and recording
        if (auto* overlay = window->getOverlay())
        {
            overlay->setVisible(false);
        }
    }
    
    // Start timer for countdown (1 second intervals)
    startTimer(1000);
}

void InteractionTestWindow::TopBar::startRecording()
{
    recording = true;
    recordingStartTime = Time::getMillisecondCounter();
    
    // Update button appearance
    if (recordButton != nullptr)
    {
        recordButton->setColours(Colour(0xFFFF4444), Colour(0xFFFF6666), Colour(0xFFFF2222));
        recordButton->repaint();
    }
    
    // Show recording indicator and enable mouse capture
    if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
    {
        if (auto* statusBar = window->getStatusBar())
        {
            statusBar->showRecordingActive(RECORDING_DURATION_SECONDS);
        }
        
        // Enable mouse capture on recording overlay
        window->setRecordingMouseCapture(true);
    }
    
    // Timer continues at 1 second intervals
}

void InteractionTestWindow::TopBar::stopRecording()
{
    stopTimer();
    recording = false;
    inCountdown = false;
    
    // Reset button appearance
    if (recordButton != nullptr)
    {
        recordButton->setColours(Colours::white.withAlpha(0.7f), 
                                 Colours::white, 
                                 Colours::white.withAlpha(0.5f));
        recordButton->repaint();
    }
    
    // Hide countdown/recording indicator, disable mouse capture, show cursor overlay again
    if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
    {
        // Disable mouse capture first
        window->setRecordingMouseCapture(false);
        
        if (auto* statusBar = window->getStatusBar())
        {
            statusBar->hideCountdown();
        }
        
        if (auto* overlay = window->getOverlay())
        {
            overlay->setVisible(true);
        }
    }
    
    // Process the recorded events
    processRecordedEvents();
    
    updateButtonStates();
    repaint();
}

void InteractionTestWindow::TopBar::addRawEvent(const var& event)
{
    if (recording)
    {
        rawRecordedEvents.add(event);
    }
}

void InteractionTestWindow::TopBar::processRecordedEvents()
{
    int eventCount = rawRecordedEvents.size();
    
    if (eventCount == 0)
    {
        // No events recorded
        if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
        {
            if (auto* statusBar = window->getStatusBar())
            {
                statusBar->hideCountdown();
                // Could show a message in log
            }
        }
        return;
    }
    
    Array<var> processedEvents;
    
    if (recordingMode == RecordingMode::Raw)
    {
        // Thin mouse move events to ~100ms intervals
        int64 lastMoveTime = -100;  // Ensure first move is included
        
        for (int i = 0; i < rawRecordedEvents.size(); i++)
        {
            auto& event = rawRecordedEvents.getReference(i);
            Identifier type = Identifier(event.getProperty(RestApiIds::type, "").toString());
            int64 timestamp = (int64)event.getProperty(InteractionIds::timestamp, 0);
            
            if (type == InteractionIds::mouseMove)
            {
                // Only include if 100ms has passed since last move
                if (timestamp - lastMoveTime >= 100)
                {
                    processedEvents.add(event);
                    lastMoveTime = timestamp;
                }
            }
            else
            {
                // Always include mouseDown/mouseUp
                processedEvents.add(event);
            }
        }
        
        // Create wrapper with mode
        DynamicObject::Ptr wrapper = new DynamicObject();
        wrapper->setProperty(InteractionIds::mode, InteractionIds::raw.toString());
        
        Array<var> eventsArray;
        for (auto& e : processedEvents)
            eventsArray.add(e);
        wrapper->setProperty(RestApiIds::interactions, eventsArray);
        
        recordedInteractions = var(wrapper.get());
        setHasRecording(true);
    }
    else
    {
        // Semantic mode - analyze raw events to produce compact interactions
        // Targets are already attached during recording (in addEvent)
        auto events = InteractionAnalyzer::parseRawEvents(rawRecordedEvents);
        
        // Create a dummy executor for the analyze call (targets already pre-resolved)
        // The executor parameter is kept for interface consistency but not used when targets are attached
        auto* window = findParentComponentOfClass<InteractionTestWindow>();
        if (window != nullptr)
        {
            if (auto* pwsc = window->getProcessor())
            {
                RealExecutor tempExecutor(window, pwsc);
                auto analysisResult = InteractionAnalyzer::analyze(events, tempExecutor);
                
                // Store result
                DynamicObject::Ptr wrapper = new DynamicObject();
                wrapper->setProperty(InteractionIds::mode, InteractionIds::semantic.toString());
                wrapper->setProperty(RestApiIds::interactions, analysisResult.interactions);
                
                recordedInteractions = var(wrapper.get());
                setHasRecording(true);
                
                // Use the analyzed interactions for logging
                if (auto* interactionsArray = analysisResult.interactions.getArray())
                {
                    for (const auto& interaction : *interactionsArray)
                        processedEvents.add(interaction);
                }
            }
        }
    }
    
    // Log recording result to StatusBar
    if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
    {
        if (auto* statusBar = window->getStatusBar())
        {
            statusBar->logRecordingResult(recordingMode, processedEvents, eventCount);
        }
    }
}

void InteractionTestWindow::TopBar::setHasRecording(bool hasData)
{
    hasRecordingData = hasData;
    replayButton->setEnabled(hasData && !recording);
    saveButton->setEnabled(hasData && !recording);
}

void InteractionTestWindow::TopBar::setSessionName(const String& name)
{
    sessionName = name;
    if (sessionNameEditor != nullptr)
        sessionNameEditor->setText(name, dontSendNotification);
    repaint();
}

String InteractionTestWindow::TopBar::getSessionName() const
{
    if (sessionNameEditor != nullptr)
        return sessionNameEditor->getText();
    return sessionName;
}

String InteractionTestWindow::TopBar::generateSessionName(const File& testsFolder)
{
    int counter = 1;
    String baseName = "new_session_";
    
    while (testsFolder.getChildFile(baseName + String(counter)).exists())
        counter++;
    
    return baseName + String(counter);
}

//==============================================================================
// InteractionTestWindow::StatusBar implementation

Path InteractionTestWindow::StatusBar::ButtonPathFactory::createPath(const String& url) const
{
    Path p;
    LOAD_EPATH_IF_URL("save", SampleMapIcons::saveSampleMap);
    LOAD_EPATH_IF_URL("log", ColumnIcons::scriptWorkspaceIcon);
    return p;
}

InteractionTestWindow::StatusBar::StatusBar()
{
    dumpButton = new HiseShapeButton("dump", this, pathFactory, "save");
    dumpButton->setTooltip("Save screenshots to folder");
    addAndMakeVisible(dumpButton);
    
    logButton = new HiseShapeButton("log", this, pathFactory, "log");
    logButton->setTooltip("Show verbose response log");
    logButton->setToggleModeWithColourChange(true);
    addAndMakeVisible(logButton);
}

InteractionTestWindow::StatusBar::~StatusBar()
{
    stopTimer();
}

void InteractionTestWindow::StatusBar::paint(Graphics& g)
{
    auto b = getLocalBounds();
    
    // Background
    g.setColour(Colour(0xFF333333));
    g.fillRect(b);
    
    if (flashAlpha > 0.0f)
    {
        // Red flash for recording/countdown, white flash for screenshots
        auto flashColour = (state == State::Recording) 
            ? Colour(0xFFFF4444).withAlpha(flashAlpha)
            : Colours::white.withAlpha(flashAlpha);
        g.setColour(flashColour);
        g.fillRect(b);
    }

    // Top border line
    g.setColour(Colours::black.withAlpha(0.5f));
    g.drawHorizontalLine(0, 0.0f, (float)getWidth());
    
    // Layout areas (right to left for fixed-width elements)
    auto dumpArea = b.removeFromRight(28);        // dump button handled by child
    auto screenshotArea = b.removeFromRight(90);  // screenshot counter
    auto stateArea = b.removeFromLeft(90);        // state indicator
    auto logArea = b.removeFromLeft(28);          // log button handled by child
    auto progressArea = b.reduced(4, 4);          // remaining = progress bar with text inside

    ignoreUnused(dumpArea, logArea);
    
    // State indicator (colored dot)
    auto dotArea = stateArea.removeFromLeft(20);
    auto dotBounds = dotArea.withSizeKeepingCentre(10, 10).toFloat();
    
    Colour stateColour = getStateColour();
    if (state == State::Running)
        stateColour = stateColour.withAlpha(pulseAlpha);
    if (state == State::Recording)
        stateColour = stateColour.withAlpha(0.7f + 0.3f * pulseAlpha);  // Pulsing red
    
    g.setColour(stateColour);
    g.fillEllipse(dotBounds);
    
    // State text
    g.setFont(GLOBAL_BOLD_FONT());
    
    // Use red text for countdown/recording, white otherwise
    if (countdownActive)
        g.setColour(Colour(0xFFFF4444));
    else
        g.setColour(Colours::white.withAlpha(0.8f));
    
    g.drawText(getStateText(), stateArea, Justification::centredLeft);
    
    // Progress bar background
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(progressArea.toFloat(), 2.0f);
    
    

    // Progress bar fill (white with 20% opacity)
    if (progress > 0.0)
    {
        auto fillWidth = (int)(progressArea.getWidth() * jlimit(0.0, 1.0, progress));
        if (fillWidth > 0)
        {
            auto fillBounds = progressArea.withWidth(fillWidth);
            g.setColour(Colours::white.withAlpha(0.2f));
            g.fillRoundedRectangle(fillBounds.toFloat(), 2.0f);
        }
    }
    
    // Centered text inside progress bar: "Action (completed/total)"
    g.setFont(GLOBAL_FONT());
    g.setColour(Colours::white.withAlpha(0.8f));
    
    String progressText = currentAction;
    if (totalInteractions > 0)
        progressText += " (" + String(completedInteractions) + "/" + String(totalInteractions) + ")";
    
    g.drawText(progressText, progressArea, Justification::centred, true);
    
    // Screenshot counter
    g.setColour(Colours::white.withAlpha(0.7f));
    String ssText = String(screenshotCount) + (screenshotCount == 1 ? " screenshot" : " screenshots");
    g.drawText(ssText, screenshotArea.reduced(4, 0), Justification::centredRight);
}

void InteractionTestWindow::StatusBar::resized()
{
    auto b = getLocalBounds();
    dumpButton->setBounds(b.removeFromRight(28).reduced(4));
    b.removeFromRight(90);  // screenshot area
    b.removeFromLeft(90);   // state area
    logButton->setBounds(b.removeFromLeft(28).reduced(4));
}

//==============================================================================
// StatusBar ProgressListener implementation

void InteractionTestWindow::StatusBar::onSequenceStarted(int total, int estimatedMs)
{
    state = State::Running;
    totalInteractions = total;
    completedInteractions = 0;
    screenshotCount = 0;
    estimatedDurationMs = estimatedMs;
    sequenceStartTime = Time::currentTimeMillis();
    progress = 0.0;
    currentAction = "Starting...";
    clearScreenshots();
    startTimerHz(30);
    
    // Disable log button during sequence to prevent accidental clicks
    if (logButton != nullptr)
        logButton->setEnabled(false);
    
    repaint();
}

void InteractionTestWindow::StatusBar::onInteractionStarted(int index, const String& description)
{
    ignoreUnused(index);
    currentAction = description;
    repaint();
}

void InteractionTestWindow::StatusBar::onInteractionCompleted(int index)
{
    ignoreUnused(index);
    completedInteractions++;
    repaint();
}

void InteractionTestWindow::StatusBar::onScreenshotCaptured()
{
    flashAlpha = 1.0f;
    screenshotCount++;
    repaint();
}

void InteractionTestWindow::StatusBar::onSequenceCompleted(bool success, const String& error)
{
    state = success ? State::Success : State::Failed;
    currentAction = success ? "Complete" : error;
    progress = 1.0;
    stopTimer();
    
    // Re-enable log button after sequence completes
    if (logButton != nullptr)
        logButton->setEnabled(true);
    
    repaint();
}

//==============================================================================
// StatusBar Timer implementation

void InteractionTestWindow::StatusBar::timerCallback()
{
    // Update progress based on elapsed time
    if (estimatedDurationMs > 0 && state == State::Running)
    {
        auto elapsed = Time::currentTimeMillis() - sequenceStartTime;
        progress = jlimit(0.0, 0.95, (double)elapsed / (double)estimatedDurationMs);
    }
    
    flashAlpha *= 0.9f;

    if (flashAlpha < 0.01f)
        flashAlpha = 0.0f;

    // Pulse animation for running/recording state
    if (state == State::Running || state == State::Recording)
    {
        if (pulseIncreasing)
        {
            pulseAlpha += 0.05f;
            if (pulseAlpha >= 1.0f)
            {
                pulseAlpha = 1.0f;
                pulseIncreasing = false;
            }
        }
        else
        {
            pulseAlpha -= 0.05f;
            if (pulseAlpha <= 0.6f)
            {
                pulseAlpha = 0.6f;
                pulseIncreasing = true;
            }
        }
    }
    
    repaint();
}

//==============================================================================
// StatusBar Button::Listener implementation

void InteractionTestWindow::StatusBar::buttonClicked(Button* b)
{
    if (b == dumpButton.get())
    {
        // If no folder selected yet, prompt for one
        if (!dumpFolder.exists())
        {
            FileChooser chooser("Select folder for screenshots",
                               File::getSpecialLocation(File::userDesktopDirectory));
            
            if (chooser.browseForDirectory())
                dumpFolder = chooser.getResult();
        }
        
        // If we have a folder, save the screenshots
        if (dumpFolder.exists() && !capturedScreenshots.empty())
        {
            for (const auto& screenshot : capturedScreenshots)
            {
                auto file = dumpFolder.getChildFile(screenshot.first + ".png");
                file.replaceWithData(screenshot.second.getData(), screenshot.second.getSize());
            }
        }
    }
    else if (b == logButton.get())
    {
        if (auto* window = findParentComponentOfClass<InteractionTestWindow>())
        {
            window->setLogVisible(logButton->getToggleState());
            window->overlay->setVisible(!logButton->getToggleState());
        }
            
    }
}

//==============================================================================
// StatusBar screenshot storage

void InteractionTestWindow::StatusBar::storeScreenshot(const String& id, const MemoryBlock& pngData)
{
    capturedScreenshots.push_back({id, pngData});
}

void InteractionTestWindow::StatusBar::clearScreenshots()
{
    capturedScreenshots.clear();
}

void InteractionTestWindow::StatusBar::appendToLog(const var& inputInteractions, const String& jsonResponse, bool success)
{
    String text;
    
    // Header with timestamp and status
    text << "\n// === " << Time::getCurrentTime().toString(true, true)
         << (success ? " [SUCCESS]" : " [FAILED]") << " ===\n\n";
    
    // Input section - compact JSON format
    text << "// Input:\n";
    text << JSON::toString(inputInteractions, false) << "\n\n";
    
    // Parse response to extract logs and errors
    var parsed = JSON::parse(jsonResponse);
    auto* obj = parsed.getDynamicObject();
    
    if (obj != nullptr)
    {
        // Console section (always show if not empty)
        auto logsVar = obj->getProperty(RestApiIds::logs);
        if (logsVar.isArray() && logsVar.size() > 0)
        {
            text << "// Console:\n";
            for (int i = 0; i < logsVar.size(); i++)
                text << logsVar[i].toString() << "\n";
            text << "\n";
        }
        
        // Errors section (only show if errors exist)
        auto errorsVar = obj->getProperty(RestApiIds::errors);
        if (errorsVar.isArray() && errorsVar.size() > 0)
        {
            text << "// Errors:\n";
            for (int i = 0; i < errorsVar.size(); i++)
            {
                auto* errObj = errorsVar[i].getDynamicObject();
                if (errObj == nullptr) continue;
                
                text << "[" << (i + 1) << "] " << errObj->getProperty(RestApiIds::errorMessage).toString() << "\n";
                
                // Show location if available
                auto callstack = errObj->getProperty(RestApiIds::callstack);
                if (callstack.isArray() && callstack.size() > 0)
                {
                    text << "    Callstack:\n";
                    for (int j = 0; j < callstack.size(); j++)
                        text << "      " << callstack[j].toString() << "\n";
                }
            }
            text << "\n";
        }
    }
    
    logDocument.insertText(logDocument.getNumCharacters(), text);
}

void InteractionTestWindow::StatusBar::setLogToggleState(bool shouldBeOn)
{
    if (logButton != nullptr)
        logButton->setToggleStateAndUpdateIcon(shouldBeOn, true);
}

void InteractionTestWindow::StatusBar::logRecordingResult(RecordingMode mode, 
                                                          const Array<var>& events, 
                                                          int originalCount)
{
    String text;
    
    // Header with timestamp
    text << "\n// === " << Time::getCurrentTime().toString(true, true)
         << " [RECORDING] ===\n\n";
    
    if (mode == RecordingMode::Raw)
    {
        text << "// Raw mode: " << originalCount << " events captured, "
             << events.size() << " after thinning\n\n";
    }
    else
    {
        text << "// Semantic mode: " << originalCount << " events captured\n\n";
    }
    
    // Output all events as JSON array
    Array<var> eventsArray;
    for (const auto& e : events)
        eventsArray.add(e);
    
    text << JSON::toString(var(eventsArray), true) << "\n";
    
    logDocument.insertText(logDocument.getNumCharacters(), text);
}

void InteractionTestWindow::StatusBar::updateFinalState(bool hasScriptErrors)
{
    if (hasScriptErrors && state == State::Success)
    {
        state = State::Failed;
        currentAction = "Script error(s)";

        SafeAsyncCall::repaint(this);
    }
}

void InteractionTestWindow::StatusBar::showCountdown(int seconds)
{
    countdownActive = true;
    countdownValue = seconds;
    recordingIndicator = false;
    state = State::Recording;
    currentAction = String(seconds);
    flashAlpha = 1.0f;  // Red flash on each countdown tick
    startTimerHz(30);   // Start pulse animation
    repaint();
}

void InteractionTestWindow::StatusBar::showRecordingActive(int remainingSeconds)
{
    countdownActive = true;
    recordingIndicator = true;
    recordingRemaining = remainingSeconds;
    state = State::Recording;
    currentAction = "REC " + String(remainingSeconds) + "s";
    repaint();
}

void InteractionTestWindow::StatusBar::hideCountdown()
{
    countdownActive = false;
    recordingIndicator = false;
    countdownValue = 0;
    recordingRemaining = 0;
    state = State::Idle;
    currentAction = "Ready";
    stopTimer();
    repaint();
}

//==============================================================================
// StatusBar helpers

Colour InteractionTestWindow::StatusBar::getStateColour() const
{
    switch (state)
    {
        case State::Idle:      return Colours::white.withAlpha(0.5f);
        case State::Running:   return Colour(SIGNAL_COLOUR);
        case State::Recording: return Colour(0xFFFF4444);  // Red for recording
        case State::Success:   return Colour(HISE_OK_COLOUR);
        case State::Failed:    return Colour(HISE_ERROR_COLOUR);
        default:               return Colours::white;
    }
}

String InteractionTestWindow::StatusBar::getStateText() const
{
    switch (state)
    {
        case State::Idle:      return "IDLE";
        case State::Running:   return "RUNNING";
        case State::Recording: return recordingIndicator ? "RECORDING" : "COUNT-IN";
        case State::Success:   return "SUCCESS";
        case State::Failed:    return "FAILED";
        default:               return "";
    }
}

//==============================================================================
// InteractionTestWindow::ContentContainer implementation

void InteractionTestWindow::ContentContainer::resized()
{
    auto bounds = getLocalBounds();
    
    // TopBar at top
    if (topBar != nullptr)
        topBar->setBounds(bounds.removeFromTop(TopBar::HEIGHT));
    
    // StatusBar at bottom
    if (statusBar != nullptr)
        statusBar->setBounds(bounds.removeFromBottom(StatusBar::HEIGHT));
    
    // Root tile gets remaining space
    if (rootTile != nullptr)
        rootTile->setBounds(bounds);
}

//==============================================================================
// InteractionTestWindow::RealExecutor implementation

InteractionTestWindow::RealExecutor* InteractionTestWindow::RealExecutor::activeExecutor = nullptr;

InteractionTestWindow::RealExecutor::RealExecutor(InteractionTestWindow* w, ProcessorWithScriptingContent* p)
    : window(w), processor(p)
{
    jassert(window != nullptr);
    jassert(processor != nullptr);
}

InteractionTestWindow::RealExecutor::~RealExecutor()
{
    if (syntheticModeActive)
        endSyntheticInputMode();
}

//==============================================================================

void InteractionTestWindow::RealExecutor::executeSyntheticModeStart(int elapsedMs)
{
    ignoreUnused(elapsedMs);
    beginSyntheticInputMode();
    syntheticModeActive = true;
}

void InteractionTestWindow::RealExecutor::executeSyntheticModeEnd(int elapsedMs)
{
    ignoreUnused(elapsedMs);
    
    // Force a final repaint to ensure last UI change is visible
    if (auto* peer = window->getPeer())
    {
        RestServer::forceRepaintWindow(peer->getNativeHandle());
        peer->performAnyPendingRepaintsNow();
    }
    
    MessageManager::getInstance()->runDispatchLoopUntil(200);
    
    if (syntheticModeActive)
    {
        endSyntheticInputMode();
        syntheticModeActive = false;
    }
}

//==============================================================================

void InteractionTestWindow::RealExecutor::beginSyntheticInputMode()
{
    activeExecutor = this;
    syntheticModifiers = ModifierKeys();
    
    if (window != nullptr)
        window->grabKeyboardFocus();
    
    PopupMenu::setSyntheticInputMode(true);
    
    auto& desktop = Desktop::getInstance();
    for (int i = 0; i < desktop.getNumMouseSources(); ++i)
    {
        if (auto* source = desktop.getMouseSource(i))
            source->setSyntheticPositionMode(true);
    }
    
    ComponentPeer::getNativeRealtimeModifiers = &RealExecutor::getSyntheticModifiersCallback;
}

void InteractionTestWindow::RealExecutor::endSyntheticInputMode()
{
    PopupMenu::setSyntheticInputMode(false);
    
    auto& desktop = Desktop::getInstance();
    for (int i = 0; i < desktop.getNumMouseSources(); ++i)
    {
        if (auto* source = desktop.getMouseSource(i))
            source->setSyntheticPositionMode(false);
    }
    
    ComponentPeer::getNativeRealtimeModifiers = nullptr;
    activeExecutor = nullptr;
}

ModifierKeys InteractionTestWindow::RealExecutor::getSyntheticModifiersCallback()
{
    if (activeExecutor != nullptr)
        return activeExecutor->syntheticModifiers;
    
    return ModifierKeys::currentModifiers;
}

InteractionExecutorBase::ResolveResult 
InteractionTestWindow::RealExecutor::resolveTarget(const InteractionParser::ComponentTargetPath& target)
{
    InteractionExecutorBase::ResolveResult result;
    
    if (processor == nullptr)
    {
        result.error = "No processor available";
        return result;
    }
    
    auto* content = processor->getScriptingContent();
    if (content == nullptr)
    {
        result.error = "No scripting content available";
        return result;
    }
    
    auto* sc = content->getComponentWithName(Identifier(target.componentId));
    if (sc == nullptr)
    {
        result.error = "Component '" + target.componentId + "' not found";
        return result;
    }
    
    result.target = target;
    result.visible = sc->isShowing(true);
    if (!result.visible)
    {
        result.error = "Component '" + target.componentId + "' is not visible";
        return result;
    }
    
    auto localBounds = sc->getPosition();
    int globalX = sc->getGlobalPositionX();
    int globalY = sc->getGlobalPositionY();
    
    if (target.hasSubtarget())
    {
        Component* c = window->getContent()->getComponentForScriptComponentWithId(target.componentId);
        result.componentBounds = DocumentWindowWithEmbeddedPopupMenu::resolveToGlobalBounds(window->getContent(), c, target.subtargetId);
    }
    else
    {
		result.componentBounds = Rectangle<int>(
			globalX, globalY,
			localBounds.getWidth(), localBounds.getHeight()
		);
    }

    if (result.componentBounds.isEmpty())
    {
        result.error = "Component '" + target.toString() + "' has zero size";
        return result;
    }
    
    return result;
}

InteractionExecutorBase::ResolveResult 
InteractionTestWindow::RealExecutor::resolvePosition(Point<int> absolutePos) const
{
    InteractionExecutorBase::ResolveResult result;
    
	auto scc = window->getContent();

	if (auto c = scc->getScriptComponentComponent(absolutePos))
	{
        auto localPos = c->getLocalPoint(scc, absolutePos);
        auto subTarget = DocumentWindowWithEmbeddedPopupMenu::findSubTargetId(scc, c, localPos);

        auto componentBounds = scc->getLocalArea(c, c->getLocalBounds());
        result.componentBounds = subTarget.first.isNotEmpty() ? subTarget.second : componentBounds;
        result.target = InteractionParser::ComponentTargetPath(c->getName(), subTarget.first);
        result.visible = true;
	}

    return result;
}

void InteractionTestWindow::RealExecutor::executeMouseDown(Point<int> pixelPos, ModifierKeys mods,
                                                           bool rightClick, int elapsedMs)
{
    ignoreUnused(elapsedMs);
    
    mouseCurrentlyDown = true;
    cursorPosition = pixelPos;
    
    syntheticModifiers = mods;
    
    injectMouseEvent(pixelPos.toFloat(), mods);
    
    window->getOverlay()->setPosition(pixelPos);
    window->getOverlay()->setState(CursorOverlay::State::Clicking);
}

void InteractionTestWindow::RealExecutor::executeMouseUp(Point<int> pixelPos, ModifierKeys mods,
                                                          bool rightClick, int elapsedMs)
{
    ignoreUnused(elapsedMs);
    
    mouseCurrentlyDown = false;
    cursorPosition = pixelPos;

    ignoreUnused(rightClick);
    auto releasedMods = mods.withoutMouseButtons();
    syntheticModifiers = releasedMods;
    injectMouseEvent(pixelPos.toFloat(), releasedMods);
    
    window->getOverlay()->setPosition(pixelPos);
    window->getOverlay()->setState(CursorOverlay::State::Idle);

    // Script LAF state is updated while painting, so flush the released state
    // before a following screenshot or REPL interaction can observe it.
    if (auto* content = window->getContent())
        content->repaint();

    if (auto* peer = window->getPeer())
    {
        RestServer::forceRepaintWindow(peer->getNativeHandle());
        peer->performAnyPendingRepaintsNow();
    }
}

void InteractionTestWindow::RealExecutor::executeMouseMove(Point<int> pixelPos, ModifierKeys mods,
                                                           int elapsedMs)
{
    ignoreUnused(elapsedMs);
    
    cursorPosition = pixelPos;
    
    if (mouseCurrentlyDown && !mods.isAnyMouseButtonDown())
        mods = mods.withFlags(ModifierKeys::leftButtonModifier);
    
    syntheticModifiers = mods;
    
    injectMouseEvent(pixelPos.toFloat(), mods);
    
    window->getOverlay()->setPosition(pixelPos);
    
    if (mouseCurrentlyDown)
        window->getOverlay()->setState(CursorOverlay::State::Dragging);
    else
        window->getOverlay()->setState(CursorOverlay::State::Hovering);
}

Result InteractionTestWindow::RealExecutor::executeScreenshot(const String& id,
                                                               const String& componentId,
                                                               float scale, int elapsedMs)
{
    ignoreUnused(elapsedMs);
    
    auto* contentComponent = window->getContent();
    auto* overlayComponent = window->getOverlay();
    
    if (contentComponent == nullptr)
        return Result::fail("Screenshot content is not available");
    
    // Hide overlay for screenshot
    bool wasVisible = overlayComponent != nullptr && overlayComponent->isVisible();
    if (overlayComponent != nullptr)
        overlayComponent->setVisible(false);
    
    // Capture the full window (includes embedded popup menus)
    auto windowImage = window->createComponentSnapshot(window->getLocalBounds(), true, scale);
    
    if (overlayComponent != nullptr)
        overlayComponent->setVisible(wasVisible);
    
    if (!windowImage.isValid())
        return Result::fail("Failed to capture screenshot");
    
    // Calculate crop region to exclude status bar (content area only)
    // Note: Native title bar is already excluded from window->getLocalBounds()
    auto contentBoundsInWindow = window->getLocalArea(contentComponent, contentComponent->getLocalBounds());
    auto cropBoundsInWindow = contentBoundsInWindow;

    if (componentId.isNotEmpty())
    {
        auto* scriptingContent = processor->getScriptingContent();
        auto* component = scriptingContent != nullptr
            ? scriptingContent->getComponentWithName(Identifier(componentId))
            : nullptr;

        if (component == nullptr)
            return Result::fail("Screenshot component not found: " + componentId);

        cropBoundsInWindow = Rectangle<int>(
            component->getGlobalPositionX(),
            component->getGlobalPositionY(),
            component->getPosition().getWidth(),
            component->getPosition().getHeight())
            .translated(contentBoundsInWindow.getX(), contentBoundsInWindow.getY());
    }
    
    // Scale the crop region
    auto scaledCropBounds = Rectangle<int>(
        roundToInt(cropBoundsInWindow.getX() * scale),
        roundToInt(cropBoundsInWindow.getY() * scale),
        roundToInt(cropBoundsInWindow.getWidth() * scale),
        roundToInt(cropBoundsInWindow.getHeight() * scale)
    );
    
    // Crop to just the content area (excludes status bar)
    auto image = windowImage.getClippedImage(scaledCropBounds);
    
    if (!image.isValid())
        return Result::fail("Failed to crop screenshot to component: " + componentId);
    
    MemoryBlock mb;
    {
        MemoryOutputStream mos(mb, false);
        PNGImageFormat pngFormat;
        if (!pngFormat.writeImageToStream(image, mos))
            return Result::fail("Failed to encode screenshot PNG");
    }

    auto* mc = getMainController();
    auto projectRoot = mc != nullptr
        ? GET_PROJECT_HANDLER(mc->getMainSynthChain()).getRootFolder()
        : File();

    if (!projectRoot.isDirectory())
        return Result::fail("Cannot save screenshot without an active project root");

    auto safeId = File::createLegalFileName(id);

    if (safeId.isEmpty())
        safeId = "screenshot";

    auto outputFile = projectRoot.getChildFile(safeId + ".png").getNonexistentSibling(false);

    if (!outputFile.replaceWithData(mb.getData(), mb.getSize()))
        return Result::fail("Failed to write screenshot PNG: " + outputFile.getFullPathName());
    
    // Store PNG data in StatusBar for dump feature
    if (auto* sb = window->getStatusBar())
        sb->storeScreenshot(id, mb);
    
    // Store screenshot info (metadata + raw PNG data)
    InteractionTester::ScreenshotInfo info;
    info.id = id;

    if (auto* p = dynamic_cast<Processor*>(processor))
        info.moduleId = p->getId();
    else
        info.moduleId = "Interface";

    info.componentId = componentId;
    info.sizeKB = static_cast<float>(mb.getSize()) / 1024.0f;
    info.scale = scale;
    info.width = image.getWidth();
    info.height = image.getHeight();
    info.filePath = outputFile.getFullPathName();
    info.pngData = std::move(mb);
    screenshots[id] = std::move(info);
    return Result::ok();
}

void InteractionTestWindow::RealExecutor::executeRepl(const String& id,
                                                       const String& expression,
                                                       int elapsedMs,
                                                       const ReplCompletion& completion)
{
    auto* mc = getMainController();
    auto* jp = mc != nullptr
        ? JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(mc)
        : nullptr;

    if (jp == nullptr)
    {
        DynamicObject::Ptr entry = new DynamicObject();
        entry->setProperty(RestApiIds::id, id);
        entry->setProperty(RestApiIds::expression, expression);
        entry->setProperty(RestApiIds::moduleId, "Interface");
        entry->setProperty(RestApiIds::timestamp, elapsedMs);
        entry->setProperty(RestApiIds::success, false);
        entry->setProperty(RestApiIds::value, "interface script processor not found");
        completion(var(entry.get()));
        return;
    }

    mc->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::ReplEvaluation, jp,
        [id, expression, elapsedMs, completion](JavascriptProcessor* processor)
    {
        auto* p = dynamic_cast<Processor*>(processor);
        auto* controller = p->getMainController();

        DynamicObject::Ptr entry = new DynamicObject();
        entry->setProperty(RestApiIds::id, id);
        entry->setProperty(RestApiIds::expression, expression);
        entry->setProperty(RestApiIds::moduleId, p->getId());
        entry->setProperty(RestApiIds::timestamp, elapsedMs);

        auto* engine = processor->getScriptEngine();

        if (engine == nullptr)
        {
            entry->setProperty(RestApiIds::success, false);
            entry->setProperty(RestApiIds::value, "no script engine present");
            completion(var(entry.get()));
            return Result::ok();
        }

        auto evaluationResult = Result::ok();
        auto value = engine->evaluate(expression, &evaluationResult);

        if (value.isUndefined() || value.isVoid())
            value = "undefined";

        entry->setProperty(RestApiIds::success, evaluationResult.wasOk());
        entry->setProperty(RestApiIds::value, value);

        if (evaluationResult.failed())
        {
            auto scriptRoot = controller->getSampleManager().getProjectHandler()
                .getSubDirectory(FileHandlerBase::Scripts);
            auto errorLines = StringArray::fromLines(evaluationResult.getErrorMessage());

            if (!errorLines.isEmpty())
            {
                auto parsed = RestHelpers::BaseScopedConsoleHandler::parseError(
                    errorLines[0], scriptRoot, p->getId());
                entry->setProperty(RestApiIds::errorMessage, parsed.message);

                if (parsed.location.isNotEmpty())
                    entry->setProperty(RestApiIds::location, parsed.location);
            }

            Array<var> callstack;

            for (int i = 1; i < errorLines.size(); i++)
            {
                auto parsed = RestHelpers::BaseScopedConsoleHandler::parseError(
                    errorLines[i], scriptRoot, p->getId());

                if (parsed.location.isNotEmpty())
                    callstack.add(parsed.toCallstackString());
            }

            if (!callstack.isEmpty())
                entry->setProperty(RestApiIds::callstack, var(callstack));
        }

        completion(var(entry.get()));
        return Result::ok();
    });
}

void InteractionTestWindow::RealExecutor::injectMouseEvent(Point<float> pixelPos, ModifierKeys mods)
{
    auto* contentComponent = window->getContent();
    if (contentComponent == nullptr)
        return;
    
    if (auto* peer = window->getPeer())
    {
        auto contentTopLeft = contentComponent->getScreenPosition() - window->getScreenPosition();
        auto windowPos = pixelPos + contentTopLeft.toFloat();
        
        peer->handleMouseEvent(
            MouseInputSource::InputSourceType::mouse,
            windowPos,
            mods,
            MouseInputSource::invalidPressure,
            MouseInputSource::invalidOrientation,
            Time::currentTimeMillis(),
            {},
            0
        );

        //RestServer::forceRepaintWindow(peer->getNativeHandle());
        //peer->performAnyPendingRepaintsNow();
    }
}

Point<int> InteractionTestWindow::RealExecutor::getCurrentCursorPosition() const
{
    return cursorPosition;
}

void InteractionTestWindow::RealExecutor::setCursorPosition(Point<int> pos)
{
    cursorPosition = pos;
}

Array<PopupMenu::VisibleMenuItem> InteractionTestWindow::RealExecutor::getVisibleMenuItems() const
{
    auto screenItems = PopupMenu::getVisibleMenuItems(window);
    
    Array<PopupMenu::VisibleMenuItem> localItems;
    
    if (window != nullptr)
    {
        for (auto& item : screenItems)
        {
            auto localBounds = window->getLocalArea(nullptr, item.screenBounds);
            localBounds.translate(0, -TopBar::HEIGHT);  // Offset for TopBar
            
            PopupMenu::VisibleMenuItem localItem;
            localItem.text = item.text;
            localItem.itemId = item.itemId;
            localItem.screenBounds = localBounds;
            localItems.add(localItem);
        }
    }
    
    return localItems;
}

int InteractionTestWindow::RealExecutor::waitUntilReady()
{
    constexpr int MAX_WAIT_MS = 5000;
    constexpr int POLL_INTERVAL_MS = 10;
    constexpr int BUFFER_MS = 100;
    
    // Hide log editor before sequence starts to prevent accidental clicks
    if (window->isLogVisible())
        window->setLogVisible(false);
    
    auto* overlay = window->getOverlay();
    if (overlay == nullptr)
        return -1;
    
    if (overlay->hasBeenPainted())
        return 0;
    
    overlay->repaint();
    
    auto startTime = Time::getMillisecondCounterHiRes();
    
    while (!overlay->hasBeenPainted())
    {
        auto elapsed = Time::getMillisecondCounterHiRes() - startTime;
        if (elapsed > MAX_WAIT_MS)
            return -1;
        
        MessageManager::getInstance()->runDispatchLoopUntil(POLL_INTERVAL_MS);
    }
    
    MessageManager::getInstance()->runDispatchLoopUntil(BUFFER_MS);
    
    return static_cast<int>(Time::getMillisecondCounterHiRes() - startTime);
}

MainController* InteractionTestWindow::RealExecutor::getMainController() const
{
    return processor != nullptr ? processor->getMainController_() : nullptr;
}

//==============================================================================
// InteractionTestWindow implementation

InteractionTestWindow::InteractionTestWindow(ProcessorWithScriptingContent* processor, InteractionTester* t)
    : DocumentWindowWithEmbeddedPopupMenu("Interaction Test", 
                     Colours::darkgrey, 
                     DocumentWindow::closeButton,
                     true),
      ControlledObject(processor->getMainController_()),
      tester(t)
{
    setUsingNativeTitleBar(true);
    
	rootTile = std::make_unique<FloatingTile>(processor->getMainController_(), nullptr);
    rootTile->setNewContent("InterfacePanel");
    
    int contentWidth = 600;
    int contentHeight = 400;
    
    if (auto* scc = getContent())
    {
        contentWidth = scc->getContentWidth();
        contentHeight = scc->getContentHeight();
    }
    
    // Add TopBar and StatusBar height to content height
    contentHeight += TopBar::HEIGHT;
    contentHeight += StatusBar::HEIGHT;
    
    overlay = std::make_unique<CursorOverlay>();
    recordingListener = std::make_unique<RecordingListener>();
    topBar = std::make_unique<TopBar>();
    statusBar = std::make_unique<StatusBar>();
    
    container = std::make_unique<ContentContainer>();
    container->rootTile = rootTile.get();
    container->topBar = topBar.get();
    container->statusBar = statusBar.get();
    
    container->addAndMakeVisible(topBar.get());
    container->addAndMakeVisible(rootTile.get());
    container->addAndMakeVisible(statusBar.get());
    
    container->setSize(contentWidth, contentHeight);
    
    setContentNonOwned(container.get(), true);
    
    // Add overlay as direct child of window (same level as popup menus)
    Component::addAndMakeVisible(overlay.get());
    overlay->setAlwaysOnTop(true);
    
    if (auto* display = Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        auto displayArea = display->userArea;
        setCentrePosition(displayArea.getCentre().translated(1000, 0));
    }
    
    setVisible(true);
    toFront(true);

	auto useOpenGL = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Other::UseOpenGL).toString() == "1";

	if (useOpenGL)
		setEnableOpenGL(this);
}

InteractionTestWindow::~InteractionTestWindow()
{
    detachOpenGl();

    // Remove the listener before destroying
    if (recordingListener != nullptr)
    {
        if (auto* scc = getContent())
            scc->removeMouseListener(recordingListener.get());
    }
    
    logEditor = nullptr;  // Destroy before StatusBar (which owns CodeDocument)
    recordingListener = nullptr;
    overlay = nullptr;
    topBar = nullptr;
    statusBar = nullptr;
    rootTile = nullptr;
    container = nullptr;
}

void InteractionTestWindow::closeButtonPressed()
{
    setVisible(false);
}

void InteractionTestWindow::childrenChanged()
{
    // Ensure overlay stays on top when children change (e.g., popup menu added)
    // Only reorder if overlay exists and isn't already the frontmost child
    if (overlay != nullptr && getChildren().getLast() != overlay.get())
        overlay->toFront(false);
}

void InteractionTestWindow::resized()
{
    // Call base class to handle content component
    DocumentWindowWithEmbeddedPopupMenu::resized();
    
    // Position overlay to cover the content area (excluding TopBar)
    if (overlay != nullptr && container != nullptr)
    {
        auto overlayBounds = container->getBounds();
        overlayBounds.removeFromTop(TopBar::HEIGHT);
        overlay->setBounds(overlayBounds);
    }
    
    // Position log editor over content area (excluding TopBar and StatusBar)
    if (logEditor != nullptr && container != nullptr)
    {
        auto logBounds = container->getBounds();
        logBounds.removeFromTop(TopBar::HEIGHT);
        logBounds.removeFromBottom(StatusBar::HEIGHT);
        logEditor->setBounds(logBounds);
        logEditor->toFront(false);
    }
}

void InteractionTestWindow::setLogVisible(bool visible)
{
    // Sync button toggle state
    if (auto* sb = getStatusBar())
    {
        sb->setLogToggleState(visible);
        overlay->setVisible(!visible);
    }
        
    
    if (visible && logEditor == nullptr)
    {
        auto* sb = getStatusBar();
        if (sb == nullptr) return;
        
        logEditor = std::make_unique<CodeEditorComponent>(sb->getLogDocument(), sb->getLogTokeniser());
        
        // Style to match JSONEditor / HISE dark theme
        logEditor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
        logEditor->setColour(CodeEditorComponent::defaultTextColourId, Colour(0xFFCCCCCC));
        logEditor->setColour(CodeEditorComponent::lineNumberTextId, Colour(0xFFCCCCCC));
        logEditor->setColour(CodeEditorComponent::lineNumberBackgroundId, Colour(0xff363636));
        logEditor->setColour(CodeEditorComponent::highlightColourId, Colour(0xff666666));
        logEditor->setColour(CaretComponent::caretColourId, Colour(0xFFDDDDDD));
        logEditor->setColour(ScrollBar::thumbColourId, Colour(0x3dffffff));
        logEditor->setReadOnly(true);
        logEditor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(14.0f));
        
        Component::addAndMakeVisible(logEditor.get());
        resized();
        logEditor->moveCaretToEnd(false);
    }
    else if (!visible && logEditor != nullptr)
    {
        logEditor = nullptr;
        resized();
    }
}

ScriptContentComponent* InteractionTestWindow::getContent()
{
    if (rootTile == nullptr)
        return nullptr;
    
    if (auto* panel = dynamic_cast<InterfaceContentPanel*>(rootTile->getCurrentFloatingPanel()))
        return panel->getScriptContentComponent();
    
    return nullptr;
}

Rectangle<int> InteractionTestWindow::getContentBounds() const
{
    auto* self = const_cast<InteractionTestWindow*>(this);
    if (auto* scc = self->getContent())
        return { 0, 0, scc->getContentWidth(), scc->getContentHeight() };
    
    return {};
}

ProcessorWithScriptingContent* InteractionTestWindow::getProcessor() const
{
    // Get processor through ScriptContentComponent -> ScriptingApi::Content -> Processor
    auto* self = const_cast<InteractionTestWindow*>(this);
    if (auto* scc = self->getContent())
    {
        auto pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(scc->getScriptProcessor());
        return const_cast<ProcessorWithScriptingContent*>(pwsc);
    }
    
    return nullptr;
}

void InteractionTestWindow::setRecordingMouseCapture(bool enabled)
{
    if (recordingListener == nullptr)
        return;
    
    if (enabled)
    {
        int64 startTime = Time::getMillisecondCounter();
        recordingListener->setRecording(true, startTime);
        addMouseListener(recordingListener.get(), true);  // true = wantsEventsForAllNestedChildComponents
    }
    else
    {
        removeMouseListener(recordingListener.get());
        recordingListener->setRecording(false, 0);
    }
}


} // namespace hise
