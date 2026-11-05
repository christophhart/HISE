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

// Forward declarations
class BackendProcessor;
struct InteractionTestWindow;

//==============================================================================
/** Screenshot metadata (replaces base64 data in API response). */


//==============================================================================
/** Main coordinator for interaction testing.
 *
 *  Manages a single implicit test session:
 *  - Creates test window on first interaction request
 *  - Re-creates window if it was closed externally
 *  - Orchestrates parsing, normalization, dispatching, and result collection
 *  - Maintains mouse state across API calls
 *
 *  Lifetime is tied to the REST server - created when server starts,
 *  destroyed when server stops.
 */
class InteractionTester
{
public:

    explicit InteractionTester(BackendProcessor* bp);
    ~InteractionTester();

    //==========================================================================
    /** Screenshot metadata (replaces base64 data in API response). */
    struct ScreenshotInfo
    {
        String id;
        String moduleId;
        String componentId;
        float sizeKB = 0.0f;
        float scale = 1.0f;
        int width = 0;
        int height = 0;
        String filePath;
        MemoryBlock pngData;  // Raw PNG data for dump feature (not serialized to JSON)
    };

    //==========================================================================
    /** Tracks virtual mouse position across API calls. */
    struct MouseState
    {
        InteractionParser::ComponentTargetPath currentTarget;  // Component + subtarget mouse is over
        Point<int> pixelPosition{ 0, 0 };     // Absolute pixel position in Interface coords

        void reset()
        {
            currentTarget.clear();
            pixelPosition = { 0, 0 };
        }
    };

    //==========================================================================
    /** Result of executing an interaction sequence. */
    struct TestResult
    {
        bool success = false;
        String errorMessage;
        int interactionsCompleted = 0;
        int totalElapsedMs = 0;
        Array<var> executionLog;
        Array<var> replResults;
        std::map<String, ScreenshotInfo> screenshots;  // id -> metadata + PNG data
        StringArray parseWarnings;

        // Menu item selection result
        InteractionDispatcher::SelectedMenuItemInfo selectedMenuItem;

        // Final mouse state (when verbose=true)
        MouseState finalMouseState;
    };

    //==========================================================================
    /** Execute a sequence of interactions.
     *
     *  Creates the test window if needed. Must be called on the message thread.
     *  Auto-inserts moveTo events as needed for proper mouse positioning.
     *
     *  @param interactionsJson  JSON array of interaction objects
     *  @param verbose           If true, include auto-insertion details in response
     *  @return                  TestResult with success/failure and captured data
     */
    TestResult executeInteractions(const var& interactionsJson, bool verbose = false);

    /** Check if the test window is currently open. */
    bool isWindowOpen() const;

    /** Close the test window if open. */
    void closeWindow();

    /** Get current mouse state (for debugging/verbose output). */
    const MouseState& getMouseState() const { return mouseState; }

    /** Reset mouse state to origin. */
    void resetMouseState();

    /** Log the JSON response for display in the status bar console.
     *  @param inputInteractions  The original input interactions array (for display only)
     *  @param jsonResponse       The full JSON response string
     *  @param success            Whether the overall operation succeeded
     */
    void logResponse(const var& inputInteractions, const String& jsonResponse, bool success);

    /** Get the test window (for internal use). */
    InteractionTestWindow* getTestWindow() { return window.get(); }

    /** Ensure test window is open, creating if needed.
     *  Called from menu command and REST API.
     */
    void ensureWindowOpen();

    /** Get the Tests folder path for saving/loading sessions. */
    File getTestsFolder() const;

private:
    BackendProcessor* backendProcessor;
    std::unique_ptr<InteractionTestWindow> window;
    MouseState mouseState;  // Persists across API calls!

    /** Get the main interface script processor. */
    JavascriptMidiProcessor* getInterfaceProcessor() const;

    /** Normalize sequence by auto-inserting moveTo events.
     *  Modifies interactions array in place.
     *  Uses and updates mouseState.
     */
    void normalizeSequence(Array<InteractionParser::Interaction>& interactions);

    /** Result of createMoveToIfNeeded - either contains a moveTo or indicates none needed. */
    struct MoveToResult
    {
        bool needed = false;
        InteractionParser::MouseInteraction moveTo;
    };

    /** Check if moveTo needed before interaction, create if so. */
    MoveToResult createMoveToIfNeeded(const InteractionParser::MouseInteraction& next);

    /** Update mouse state after interaction. Called during normalization. */
    void updateMouseStateForNormalization(const InteractionParser::MouseInteraction& interaction);

    /** Execute raw recorded events directly (no parsing/normalization).
     *  Used for raw recording mode where events are replayed exactly as captured.
     */
    InteractionDispatcher::ExecutionResult executeRawEvents(
        const var& eventsArray,
        InteractionExecutorBase& executor,
        Array<var>& executionLog);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractionTester)
};


} // namespace hise
