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
/** Executes parsed interactions with timing control.
 *  Must be called on the message thread. Pumps message loop between events.
 */
class InteractionDispatcher : private Timer
{
public:
    //==============================================================================
    /** Listener for interaction execution progress updates.
     *  Implement this interface to receive callbacks during sequence execution.
     */
    struct ProgressListener
    {
        virtual ~ProgressListener() = default;
        
        /** Called when a sequence begins execution.
         *  @param totalInteractions   Number of interactions in the sequence
         *  @param estimatedDurationMs Estimated total duration in milliseconds
         */
        virtual void onSequenceStarted(int totalInteractions, int estimatedDurationMs) = 0;
        
        /** Called when an individual interaction starts.
         *  @param index       Zero-based index of the interaction
         *  @param description Human-readable description (e.g., "Moving to Button1")
         */
        virtual void onInteractionStarted(int index, const String& description) = 0;
        
        /** Called when an individual interaction completes.
         *  @param index  Zero-based index of the completed interaction
         */
        virtual void onInteractionCompleted(int index) = 0;
        
        /** Called when a screenshot is captured. */
        virtual void onScreenshotCaptured() = 0;
        
        /** Called when the sequence completes (success or failure).
         *  @param success True if all interactions completed without error
         *  @param error   Error message if success is false
         */
        virtual void onSequenceCompleted(bool success, const String& error) = 0;
    };

    //==============================================================================
    /** Result of execution with timing info. */
    struct ExecutionResult
    {
        Result result = Result::ok();
        int totalElapsedMs = 0;
        int interactionsCompleted = 0;
        
        static ExecutionResult ok(int elapsed, int completed) 
        { 
            ExecutionResult r;
            r.totalElapsedMs = elapsed;
            r.interactionsCompleted = completed;
            return r;
        }
        
        static ExecutionResult fail(const String& message, int elapsed, int completed)
        {
            ExecutionResult r;
            r.result = Result::fail(message);
            r.totalElapsedMs = elapsed;
            r.interactionsCompleted = completed;
            return r;
        }
    };
    
    //==============================================================================
    /** Information about the selected menu item (from selectMenuItem interaction). */
    struct SelectedMenuItemInfo
    {
        String text;
        int itemId = 0;
        bool wasSelected = false;
    };
    
    //==============================================================================
    /** Execute normalized interactions synchronously. MUST be called on message thread.
     *
     *  Assumes moveTo events have been auto-inserted by normalizeSequence().
     *
     *  @param interactions  Parsed and normalized interactions to execute
     *  @param executor      Implementation that performs the actual operations
     *  @param executedLog   Output: log of executed events for response
     *  @param timeoutMs     Maximum execution time (default 20 seconds)
     *  @returns             ExecutionResult with success/failure and timing
     */
    ExecutionResult execute(const Array<InteractionParser::Interaction>& interactions,
                            InteractionExecutorBase& executor,
                            Array<var>& executedLog,
                            int timeoutMs = 20000);
    
    /** Get info about the last selected menu item.
     *  Only valid after executing a selectMenuItem interaction.
     */
    SelectedMenuItemInfo getLastSelectedMenuItem() const { return lastSelectedMenuItem; }

    Array<var> getReplResults() const;
    
    /** Set a listener to receive progress callbacks during execution. */
    void setProgressListener(ProgressListener* listener) { progressListener = listener; }
    
    //==============================================================================
    static constexpr int MOVE_STEP_INTERVAL_MS = 10;
    static constexpr int DRAG_STEP_INTERVAL_MS = 10;
    static constexpr int UI_SETTLE_DELAY_MS = 50;  // Time to let UI react after interactions
    static constexpr int RESOLVE_RETRY_DELAY_MS = 30;   // Delay between resolve attempts
    static constexpr int RESOLVE_MAX_RETRIES = 10;      // Max attempts (10 * 30ms = 300ms max)
    
private:
    struct TimedInteraction
    {
        InteractionParser::MouseInteraction mouse;
        Point<int> startPos;
        Point<int> endPos;
        ModifierKeys modifiers;
        int64 startTimeMs = 0;
        int64 menuMouseDownTimeMs = 0;
        int interactionIndex = -1;
        int menuItemId = 0;
        String menuItemText;
        bool active = false;
        bool mouseIsDown = false;
        bool menuClickStarted = false;
        bool usedFallback = false;
    };

    int64 startTimeMs = 0;
    int timeoutMs = 20000;
    String lastError;
    SelectedMenuItemInfo lastSelectedMenuItem;
    struct ReplJobState;
    std::shared_ptr<ReplJobState> replJobState;
    ProgressListener* progressListener = nullptr;
    TimedInteraction timedInteraction;
    InteractionExecutorBase* activeExecutor = nullptr;
    Array<var>* activeLog = nullptr;
    int currentInteractionIndex = -1;
    int completedCount = 0;
    
    int getElapsedMs() const;
    void waitForDuration(int ms, InteractionExecutorBase& executor);
    void waitWithWiggle(int ms, InteractionExecutorBase& exec);
    void wiggleToUpdateComponent(Point<int> pos, InteractionExecutorBase& exec);
    void interpolateMovement(Point<int> startPos, Point<int> endPos, int durationMs,
                             ModifierKeys mods, InteractionExecutorBase& exec);
    String checkAbortConditions() const;
    void waitForPendingReplResults();
    void waitForTimedInteraction(InteractionExecutorBase& executor);
    void timerCallback() override;
    void updateTimedInteraction();
    void completeTimedInteraction();
    void abortTimedInteraction();
    void startTimedInteraction(const InteractionParser::MouseInteraction& mouse,
                               Point<int> startPos, Point<int> endPos,
                               ModifierKeys modifiers, bool mouseIsDown = false);
    bool shouldRunTimed(const InteractionParser::MouseInteraction& mouse) const;
    bool isAllowedDuringTimedInteraction(InteractionParser::MouseInteraction::Type type) const;
    String getTimedInteractionConflict(const InteractionParser::MouseInteraction& mouse) const;
    
    // Execute individual interaction types
    void executeMoveTo(const InteractionParser::MouseInteraction& mouse, InteractionExecutorBase& exec, Array<var>& log);
    void executeClick(const InteractionParser::MouseInteraction& mouse, InteractionExecutorBase& exec, Array<var>& log);
    void executeDrag(const InteractionParser::MouseInteraction& mouse, InteractionExecutorBase& exec, Array<var>& log);
    void executeScreenshot(const InteractionParser::MouseInteraction& mouse, InteractionExecutorBase& exec, Array<var>& log);
    void executeSelectMenuItem(const InteractionParser::MouseInteraction& mouse, InteractionExecutorBase& exec, Array<var>& log);
    void executeRepl(const InteractionParser::MouseInteraction& mouse, InteractionExecutorBase& exec, Array<var>& log);
    
    // Create log entry for response
    var createLogEntry(const String& type, Point<int> pixelPos, int elapsedMs) const;
};

//==============================================================================
/** Test executor that logs events for verification without actual UI injection. */
class TestExecutor : public InteractionExecutorBase
{
public:
    //==========================================================================
    /** Log entry for recorded mouse/screenshot events. */
    struct LogEntry
    {
        Identifier type;       // InteractionIds::mouseDown, mouseUp, mouseMove, screenshot
        Point<int> pixelPos;
        ModifierKeys mods;
        bool rightClick = false;
        String screenshotId;
        String screenshotComponentId;
        float screenshotScale = 1.0f;
        String replId;
        String replExpression;
        int elapsedMs = 0;
    };
    
    //==========================================================================
    /** Mock component for testing resolution. */
    struct MockComponent
    {
        Rectangle<int> absoluteBounds;
        bool visible = true;
        std::map<String, Rectangle<int>> subtargets;  // subtargetId → bounds
    };
    
    //==========================================================================
    Array<LogEntry> log;
    std::map<String, MockComponent> mockComponents;
    Point<int> cursorPosition{0, 0};
    Array<PopupMenu::VisibleMenuItem> mockMenuItems;
    bool replShouldFail = false;
    String screenshotError;
    
    //==========================================================================
    /** Reset all state for a new test. */
    void reset();
    
    /** Add a mock component for resolution testing. */
    void addMockComponent(const String& id, Rectangle<int> bounds, bool visible = true);
    
    /** Clear all mock components. */
    void clearMockComponents();
    
    /** Add a subtarget (child component) to a mock component. */
    void addMockSubtarget(const String& parentId, const String& subtargetId, Rectangle<int> bounds);
    
    /** Add a mock menu item for testing selectMenuItem. */
    void addMockMenuItem(const String& text, int itemId, Rectangle<int> bounds);
    
    /** Clear all mock menu items. */
    void clearMockMenuItems();
    
    //==========================================================================
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
    
    void executeSyntheticModeStart(int elapsedMs) override { ignoreUnused(elapsedMs); }
    void executeSyntheticModeEnd(int elapsedMs) override { ignoreUnused(elapsedMs); }
    
    Point<int> getCurrentCursorPosition() const override { return cursorPosition; }
    void setCursorPosition(Point<int> pos) override { cursorPosition = pos; }
    
    Array<PopupMenu::VisibleMenuItem> getVisibleMenuItems() const override { return mockMenuItems; }
    
    int waitUntilReady() override { return 0; }  // TestExecutor is always immediately ready
};

} // namespace hise
