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

#pragma once



namespace hise {
using namespace juce;


//==============================================================================
/** Window hosting the test interface with cursor overlay.
 *
 *  Creates a DocumentWindow containing:
 *  - FloatingTile with InterfaceContentPanel (mirrors FrontendProcessorEditor structure)
 *  - CursorOverlay on top showing injected mouse events
 *
 *  The FloatingTile hierarchy ensures ScriptContentComponent can find its
 *  expected parent components (required for proper slider/button callbacks).
 *
 *  Window is centered on the primary display and set to always-on-top.
 */
class InteractionTestWindow : public DocumentWindowWithEmbeddedPopupMenu,
                              public ControlledObject
{
public:
    //==========================================================================
    /** Visual cursor overlay showing injected mouse events.
     *
     *  Displays a colored circle at the current mouse position:
     *  - Grey: Idle (between interactions)
     *  - Yellow: Hovering (mouse present, no button pressed)
     *  - Green: Clicking (mouseDown/mouseUp)
     *  - Orange: Dragging (mouseDown while moving)
     */
    class CursorOverlay : public Component
    {
    public:
        enum class State
        {
            Idle,
            Hovering,
            Clicking,
            Dragging
        };
        
        CursorOverlay();
        
        /** Set the current cursor state (affects color). */
        void setState(State newState);
        
        /** Set cursor position in pixel coordinates. */
        void setPosition(Point<int> pixelPos);
        
        /** Get current position in pixels. */
        Point<int> getPosition() const { return position; }
        
        /** Returns true if paint() has been called at least once. */
        bool hasBeenPainted() const { return painted; }
        
        void paint(Graphics& g) override;
        
    private:
        State state = State::Idle;
        Point<int> position{0, 0};
        bool painted = false;
        
        Colour getColourForState() const;
        
        static constexpr float cursorDiameter = 16.0f;
        static constexpr float borderWidth = 2.0f;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CursorOverlay)
    };
    
    //==========================================================================
    /** Mouse listener that captures events during recording without blocking UI. */
    class RecordingListener : public MouseListener
    {
    public:
        RecordingListener() = default;
        
        void mouseDown(const MouseEvent& e) override;
        void mouseUp(const MouseEvent& e) override;
        void mouseMove(const MouseEvent& e) override;
        void mouseDrag(const MouseEvent& e) override;
        
        void setRecording(bool enabled, int64 startTime);
        bool isRecordingActive() const { return recording; }
        
        /** Info about a popup menu item selection. */
        struct PopupMenuSelectionInfo
        {
            bool isMenuClick = false;
            int itemId = -1;
            String itemText;
            
            bool isValid() const { return isMenuClick && itemId >= 0; }
            void reset() { isMenuClick = false; itemId = -1; itemText = ""; }
        };
        
    private:
        bool recording = false;
        int64 recordingStartTime = 0;
        PopupMenuSelectionInfo pendingMenuSelection;
        
        void addEvent(const Identifier& type, const MouseEvent& e);
        
        /** Detect if this mouseDown is on a popup menu item.
         *  Returns info about the menu item if found.
         */
        PopupMenuSelectionInfo detectPopupMenuSelection(const MouseEvent& e);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingListener)
    };
    
    //==========================================================================
    /** Recording mode for captured mouse interactions. */
    enum class RecordingMode
    {
        Semantic,   // Analyze raw events to detect clicks/drags, store as compact format
        Raw         // Store thinned raw events for exact replay
    };
    
    //==========================================================================
    /** Top bar with action buttons for recording, replay, save, and load.
     *
     *  Displays:
     *  - Mode toggle button (Semantic/Raw)
     *  - Record button (toggle, red when recording)
     *  - Replay button (disabled until recording exists)
     *  - Save button (disabled until recording exists)
     *  - Load button (always enabled)
     *  - Session name label
     */
    class TopBar : public Component,
                   public Button::Listener,
                   public Timer
    {
    public:
        TopBar();
        
        // Component
        void paint(Graphics& g) override;
        void resized() override;
        
        // Button::Listener
        void buttonClicked(Button* b) override;
        
        // Timer (for countdown and recording duration)
        void timerCallback() override;
        
        // Recording mode
        RecordingMode getRecordingMode() const { return recordingMode; }
        
        // Recording state
        void setRecording(bool isRecording);
        bool isRecording() const { return recording; }
        
        // Start the countdown sequence (3-2-1 then record)
        void startRecordingCountdown();
        
        // Called when recording actually starts (after countdown)
        void startRecording();
        
        // Called when recording duration expires
        void stopRecording();
        
        // Add a raw mouse event during recording
        void addRawEvent(const var& event);
        
        // Get the raw recorded events
        const Array<var>& getRawRecordedEvents() const { return rawRecordedEvents; }
        
        // Enable/disable based on session state
        void setHasRecording(bool hasRecording);
        
        // Recorded interactions for replay
        void setRecordedInteractions(const var& interactions) { recordedInteractions = interactions; }
        const var& getRecordedInteractions() const { return recordedInteractions; }
        
        // Session name
        void setSessionName(const String& name);
        String getSessionName() const;
        
        /** Generate a unique session name (new_session_1, new_session_2, etc.) */
        String generateSessionName(const File& testsFolder);
        
        static constexpr int HEIGHT = 40;
        
        static constexpr int COUNTDOWN_SECONDS = 3;
        static constexpr int RECORDING_DURATION_SECONDS = 5;
        
    private:
        struct ButtonPathFactory : public PathFactory
        {
            Path createPath(const String& id) const override;
        };
        
        ButtonPathFactory pathFactory;
        GlobalHiseLookAndFeel laf;
        
        ScopedPointer<TextButton> modeToggle;
        ScopedPointer<HiseShapeButton> recordButton;
        ScopedPointer<HiseShapeButton> replayButton;
        ScopedPointer<HiseShapeButton> clearButton;
        ScopedPointer<HiseShapeButton> loadButton;
        ScopedPointer<HiseShapeButton> saveButton;
        ScopedPointer<TextEditor> sessionNameEditor;
        
        String sessionName = "new_session_1";
        bool recording = false;
        bool hasRecordingData = false;
        RecordingMode recordingMode = RecordingMode::Semantic;
        
        // Countdown state
        bool inCountdown = false;
        int countdownValue = 0;
        
        // Recording timing
        int64 recordingStartTime = 0;
        
        // Raw events captured during recording
        Array<var> rawRecordedEvents;
        
        var recordedInteractions;  // Stores the recorded/loaded session data
        
        // Helper to update button enabled states
        void updateButtonStates();
        
        // Process raw events after recording stops
        void processRecordedEvents();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
    };
    
    //==========================================================================
    /** Status bar showing execution progress at bottom of test window.
     *
     *  Displays:
     *  - State indicator (colored dot + text: IDLE/RUNNING/RECORDING/SUCCESS/FAILED)
     *  - Current action description
     *  - Time-based progress bar
     *  - Step counter (completed/total)
     *  - Screenshot counter with flash effect
     *  - Log toggle button
     */
    class StatusBar : public Component,
                      public InteractionDispatcher::ProgressListener,
                      public Timer,
                      public Button::Listener
    {
    public:
        enum class State { Idle, Running, Recording, Success, Failed };
        
        StatusBar();
        ~StatusBar() override;
        
        // Component
        void paint(Graphics& g) override;
        void resized() override;
        
        // ProgressListener
        void onSequenceStarted(int totalInteractions, int estimatedDurationMs) override;
        void onInteractionStarted(int index, const String& description) override;
        void onInteractionCompleted(int index) override;
        void onScreenshotCaptured() override;
        void onSequenceCompleted(bool success, const String& error) override;
        
        // Timer (for animations)
        void timerCallback() override;
        
        // Button::Listener
        void buttonClicked(Button* b) override;
        
        /** Store a screenshot for later dumping.
         *  @param id      Screenshot identifier (used as filename)
         *  @param pngData Raw PNG data
         */
        void storeScreenshot(const String& id, const MemoryBlock& pngData);
        
        /** Clear all stored screenshots. */
        void clearScreenshots();
        
        /** Append formatted log entry showing input, console output, and errors. */
        void appendToLog(const var& inputInteractions, const String& jsonResponse, bool success);
        
        /** Update the final state after script errors are known.
         *  Call this after sequence completes if script errors were detected.
         */
        void updateFinalState(bool hasScriptErrors);
        
        /** Get the log document for the editor. */
        CodeDocument& getLogDocument() { return logDocument; }
        
        /** Get the tokeniser for syntax highlighting. */
        CodeTokeniser* getLogTokeniser() { return &logTokeniser; }
        
        /** Set the log button toggle state (used when hiding log programmatically). */
        void setLogToggleState(bool state);
        
        /** Log the result of a recording session.
         *  @param mode          Recording mode (Semantic or Raw)
         *  @param events        The processed events (thinned for raw mode)
         *  @param originalCount Number of events before processing
         */
        void logRecordingResult(RecordingMode mode, const Array<var>& events, int originalCount);
        
        /** Show countdown overlay (for recording countdown). */
        void showCountdown(int seconds);
        
        /** Hide countdown overlay and show recording indicator. */
        void showRecordingActive(int remainingSeconds);
        
        /** Hide countdown/recording overlay. */
        void hideCountdown();
        
        /** Check if countdown is currently showing. */
        bool isShowingCountdown() const { return countdownActive; }
        
        static constexpr int HEIGHT = 28;
        
    private:

        float flashAlpha = 0.0f;

        State state = State::Idle;
        String currentAction = "Ready";
        int totalInteractions = 0;
        int completedInteractions = 0;
        int screenshotCount = 0;
        int estimatedDurationMs = 0;
        int64 sequenceStartTime = 0;
        
        // Animation state
        float pulseAlpha = 1.0f;
        bool pulseIncreasing = false;
        
        // Progress (0.0 - 1.0)
        double progress = 0.0;
        
        // Components
        ScopedPointer<HiseShapeButton> dumpButton;
        ScopedPointer<HiseShapeButton> logButton;
        
        // Dump state
        File dumpFolder;
        std::vector<std::pair<String, MemoryBlock>> capturedScreenshots;  // id -> PNG data
        
        // Log console
        CodeDocument logDocument;
        JavascriptTokeniser logTokeniser;
        
        // Countdown/Recording overlay state
        bool countdownActive = false;
        int countdownValue = 0;
        bool recordingIndicator = false;
        int recordingRemaining = 0;
        
        // LAF
        GlobalHiseLookAndFeel laf;
        
        // Path factory for button icons
        struct ButtonPathFactory : public PathFactory
        {
            Path createPath(const String& id) const override;
        };
        
        ButtonPathFactory pathFactory;
        
        // Helpers
        Colour getStateColour() const;
        String getStateText() const;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
    };
    
    //==========================================================================
    /** Executor that injects real mouse events via ComponentPeer.
     *
     *  Implements the InteractionDispatcher::Executor interface to:
     *  - Convert normalized coordinates to pixel positions
     *  - Inject mouse events via ComponentPeer::handleMouseEvent()
     *  - Capture screenshots with overlay temporarily hidden
     *  - Update cursor overlay state for visual feedback
     *
     *  Uses InputSourceType::mouse with index 0 (primary mouse).
     */
    class RealExecutor : public InteractionExecutorBase
    {
    public:
        RealExecutor(InteractionTestWindow* window, ProcessorWithScriptingContent* processor);
        
        //======================================================================
        // InteractionExecutorBase interface implementation
        
        InteractionExecutorBase::ResolveResult resolveTarget(const InteractionParser::ComponentTargetPath& target) override;
        
        InteractionExecutorBase::ResolveResult resolvePosition(Point<int> absolutePos) const override;
        
        void executeMouseDown(Point<int> pixelPos, ModifierKeys mods,
                              bool rightClick, int elapsedMs) override;
        
        void executeMouseUp(Point<int> pixelPos, ModifierKeys mods,
                            bool rightClick, int elapsedMs) override;
        
        void executeMouseMove(Point<int> pixelPos, ModifierKeys mods,
                              int elapsedMs) override;
        
        Result executeScreenshot(const String& id, const String& componentId,
                                 float scale, int elapsedMs) override;

        void executeRepl(const String& id, const String& expression, int elapsedMs,
                         const ReplCompletion& completion) override;
        
        void executeSyntheticModeStart(int elapsedMs) override;
        void executeSyntheticModeEnd(int elapsedMs) override;
        
        Point<int> getCurrentCursorPosition() const override;
        void setCursorPosition(Point<int> pos) override;
        
        Array<PopupMenu::VisibleMenuItem> getVisibleMenuItems() const override;
        
        int waitUntilReady() override;
        
        //======================================================================
        /** Get all captured screenshots. */
        const std::map<String, InteractionTester::ScreenshotInfo>& getScreenshots() const { return screenshots; }
        
        /** Get MainController for console logging. */
        MainController* getMainController() const override;
        
        //======================================================================
        /** Destructor ensures synthetic mode is cleaned up if still active. */
        ~RealExecutor();
        
    private:
        
        InteractionTestWindow* window;
        ProcessorWithScriptingContent* processor;
        std::map<String, InteractionTester::ScreenshotInfo> screenshots;
        bool mouseCurrentlyDown = false;
        Point<int> cursorPosition{0, 0};
        
        // Synthetic input mode state
        bool syntheticModeActive = false;
        ModifierKeys syntheticModifiers;
        
        void beginSyntheticInputMode();
        void endSyntheticInputMode();
        static ModifierKeys getSyntheticModifiersCallback();
        static RealExecutor* activeExecutor;
        
        /** Inject a mouse event into the window's ComponentPeer. */
        void injectMouseEvent(Point<float> pixelPos, ModifierKeys mods);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealExecutor)
    };
    
    //==========================================================================
    /** Create window for the given script processor's interface. */
    InteractionTestWindow(ProcessorWithScriptingContent* processor, InteractionTester* tester);
    ~InteractionTestWindow() override;
    
    void closeButtonPressed() override;
    void childrenChanged() override;
    void resized() override;
    
    /** Get the ScriptContentComponent displaying the interface.
     *  Navigates through FloatingTile -> InterfaceContentPanel -> ScriptContentComponent.
     */
    ScriptContentComponent* getContent();
    
    /** Get the cursor overlay component. */
    CursorOverlay* getOverlay() { return overlay.get(); }
    
    /** Get the status bar component. */
    StatusBar* getStatusBar() { return statusBar.get(); }
    
    /** Enable/disable recording mode for mouse capture. */
    void setRecordingMouseCapture(bool enabled);
    
    /** Show/hide the log editor overlay. */
    void setLogVisible(bool visible);
    
    /** Check if log is currently visible. */
    bool isLogVisible() const { return logEditor != nullptr; }
    
    /** Get the content size in pixels. */
    Rectangle<int> getContentBounds() const;
    
    /** Get the top bar component. */
    TopBar* getTopBar() { return topBar.get(); }
    
    /** Get the interaction tester that owns this window. */
    InteractionTester* getInteractionTester() { return tester; }
    
    /** Get the processor for creating temporary RealExecutor during recording.
     *  Returns nullptr if processor is not available.
     */
    ProcessorWithScriptingContent* getProcessor() const;
    
private:

    /** Container component that holds TopBar, FloatingTile, and StatusBar. */
    class ContentContainer : public Component
    {
    public:
        ContentContainer() = default;
        void resized() override;
        
        TopBar* topBar = nullptr;
        FloatingTile* rootTile = nullptr;
        StatusBar* statusBar = nullptr;
    };
    
    InteractionTester* tester;
    std::unique_ptr<ContentContainer> container;
    std::unique_ptr<TopBar> topBar;
    std::unique_ptr<FloatingTile> rootTile;
    std::unique_ptr<CursorOverlay> overlay;
    std::unique_ptr<RecordingListener> recordingListener;
    std::unique_ptr<StatusBar> statusBar;
    std::unique_ptr<CodeEditorComponent> logEditor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractionTestWindow)
};




} // namespace hise
