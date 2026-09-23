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
/** Identifiers for interaction JSON field names.
    Using Identifiers instead of string literals provides compile-time safety
    and better performance for property lookups.
*/
#define DECLARE_ID(x) const Identifier x(#x);

//==============================================================================
/** Default timing values for interactions */
namespace InteractionConstants
{
	static constexpr int DefaultMoveDurationMs = 700;
	static constexpr int DefaultClickDurationMs = 30;
	static constexpr int DefaultDoubleClickDurationMs = 20;
	static constexpr int DefaultDragDurationMs = 500;
	static constexpr int DefaultMenuSelectDurationMs = 700;
}

namespace InteractionIds
{
    // Event types (semantic)
    DECLARE_ID(click);
    DECLARE_ID(doubleClick);
    DECLARE_ID(drag);
    DECLARE_ID(moveTo);
    DECLARE_ID(screenshot);
    DECLARE_ID(selectMenuItem);
    DECLARE_ID(repl);
    
    // Event types (raw) 
    DECLARE_ID(mouseDown);
    DECLARE_ID(mouseUp);
    DECLARE_ID(mouseMove);
    
    // Position
    DECLARE_ID(normalizedPosition);
    DECLARE_ID(pixelPosition);
    DECLARE_ID(normalizedPositionInTarget);
    DECLARE_ID(delta);
    DECLARE_ID(targetBounds);
    
    // Component targeting
    DECLARE_ID(target);
    DECLARE_ID(subtarget);
    DECLARE_ID(content);
    
    // Timing
    DECLARE_ID(timestamp);
    DECLARE_ID(delay);
    DECLARE_ID(duration);
    
    // Modifiers
    DECLARE_ID(rightClick);
    DECLARE_ID(shiftDown);
    DECLARE_ID(ctrlDown);
    DECLARE_ID(altDown);
    DECLARE_ID(cmdDown);
    DECLARE_ID(modifiers);
    
    // Menu selection
    DECLARE_ID(menuItemText);
    
    // Recording mode
    DECLARE_ID(mode);
    DECLARE_ID(raw);
    DECLARE_ID(semantic);
    
    // Execution log flags
    DECLARE_ID(fallback);
    DECLARE_ID(autoInserted);
}

#undef DECLARE_ID




//==============================================================================
/** Parser and validator for interaction test sequences.
 *
 *  Parses JSON interaction arrays and validates them.
 *  Expands doubleClick to two clicks.
 *  Does NOT perform moveTo auto-insertion (that's done by InteractionTester).
 */
class InteractionParser
{
public:
    //==============================================================================
    /** Represents a UI component target, optionally with a subtarget.
     *  Examples:
     *    - {"Button1", ""}        -> main component
     *    - {"SliderPack1", "11"}  -> slider 11 within SliderPack
     *    - {"FilterGraph", "2"}   -> EQ band 2
     */
    struct ComponentTargetPath
    {
        String componentId;     // Main component ID (e.g., "SliderPack1")
        String subtargetId;     // Child element ID (e.g., "11"), or empty
        
        //==========================================================================
        // Constructors
        ComponentTargetPath() = default;
        ComponentTargetPath(const String& component) : componentId(component) {}
        ComponentTargetPath(const String& component, const String& subtarget)
            : componentId(component), subtargetId(subtarget) {}
        
        //==========================================================================
        // Queries
        bool isEmpty() const { return componentId.isEmpty(); }
        bool isValid() const { return componentId.isNotEmpty(); }
        bool hasSubtarget() const { return subtargetId.isNotEmpty(); }
        
        //==========================================================================
        // Comparison
        bool operator==(const ComponentTargetPath& other) const
        {
            return componentId == other.componentId && subtargetId == other.subtargetId;
        }
        bool operator!=(const ComponentTargetPath& other) const { return !(*this == other); }
        
        //==========================================================================
        // JSON Serialization (backward-compatible separate fields)
        
        /** Read from JSON object with "target" and optional "subtarget" fields. */
        static ComponentTargetPath fromVar(const var& obj)
        {
            ComponentTargetPath path;
            path.componentId = obj.getProperty(InteractionIds::target, "").toString();
            path.subtargetId = obj.getProperty(InteractionIds::subtarget, "").toString();
            return path;
        }
        
        /** Write to existing JSON object as "target" and "subtarget" fields. */
        void toVar(DynamicObject* obj) const
        {
            if (componentId.isNotEmpty())
                obj->setProperty(InteractionIds::target, componentId);
            if (subtargetId.isNotEmpty())
                obj->setProperty(InteractionIds::subtarget, subtargetId);
        }
        
        //==========================================================================
        // Debug/Display
        String toString() const
        {
            if (subtargetId.isEmpty())
                return componentId;
            return componentId + "." + subtargetId;
        }
        
        void clear()
        {
            componentId = {};
            subtargetId = {};
        }
    };

    //==============================================================================
    /** Mouse interaction data */
    struct MouseInteraction
    {
        //==========================================================================
        /** Position within a component - normalized (0-1) or absolute pixels.
         *  Both modes are relative to the target component's bounds.
         */
        struct Position
        {
            enum class Mode { Normalized, Absolute };
            
            /** Get pixel position in Interface coordinates.
             *  @param componentBounds  Absolute bounds of target component
             */
            Point<int> getPixelPosition(Rectangle<int> componentBounds) const
            {
                if (mode == Mode::Absolute)
                {
                    return {
                        componentBounds.getX() + roundToInt(value.x),
                        componentBounds.getY() + roundToInt(value.y)
                    };
                }
                
                return {
                    componentBounds.getX() + roundToInt(value.x * componentBounds.getWidth()),
                    componentBounds.getY() + roundToInt(value.y * componentBounds.getHeight())
                };
            }
            
            /** Create normalized position (0-1 range relative to component). */
            static Position normalized(float x, float y) 
            { 
                Position p;
                p.mode = Mode::Normalized;
                p.value = {x, y};
                return p;
            }
            
            /** Create absolute pixel position (relative to component origin). */
            static Position absolute(int x, int y) 
            { 
                Position p;
                p.mode = Mode::Absolute;
                p.value = {static_cast<float>(x), static_cast<float>(y)};
                return p;
            }
            
            /** Create normalized position at center (0.5, 0.5). */
            static Position center() { return normalized(0.5f, 0.5f); }
            
            bool isAbsolute() const { return mode == Mode::Absolute; }
            
            bool isCenter() const 
            { 
                return mode == Mode::Normalized && 
                       value.x == 0.5f && value.y == 0.5f; 
            }
            
            bool operator==(const Position& other) const
            {
                return mode == other.mode && value == other.value;
            }
            
            bool operator!=(const Position& other) const { return !(*this == other); }
            
            Mode getMode() const { return mode; }
            Point<float> getValue() const { return value; }
            
            /** Fallback: normalized position within parent component.
             *  Used when subtarget doesn't exist at replay time. */
            Point<float> normalizedInParent = {-1.f, -1.f};
            
            bool hasFallback() const { return normalizedInParent.x >= 0.f; }
            
        private:
            Mode mode = Mode::Normalized;
            Point<float> value{0.5f, 0.5f};
        };
        
        //==========================================================================
        /** Internal primitive types (dispatcher executes these).
         *  Note: doubleClick is expanded to two Click events at parse time.
         */
        enum class Type
        {
            MoveTo,
            Click,
            Drag,
            Screenshot,
            SelectMenuItem,
            Repl
        };
        
        Type type = Type::Click;
        
        /** Target component + optional subtarget (for moveTo, click, drag). */
        ComponentTargetPath target;

        /** Position within target (for moveTo, click, drag start). */
        Position position = Position::center();
        
        /** Drag-specific: pixel delta from current position. */
        Point<int> deltaPixels{0, 0};
        
        /** Delay before this interaction starts (ms). */
        int delayMs = 0;
        
        /** Duration of animated movement (ms). */
        int durationMs = 0;

        /** True when duration was explicitly supplied by the caller. */
        bool durationWasExplicit = false;
        
        /** Modifier keys (shift, ctrl, alt, cmd). */
        ModifierKeys modifiers;
        
        /** Right-click flag (for click only). */
        bool rightClick = false;
        
        /** Screenshot ID (for screenshot only). */
        String screenshotId;

        /** Optional component ID to crop (for screenshot only). */
        String screenshotComponentId;
        
        /** Screenshot scale factor (for screenshot only). */
        float screenshotScale = 1.0f;

        /** Required result ID and expression (for REPL only). */
        String replId;
        String replExpression;
        
        /** Menu item text to match (for selectMenuItem only). */
        String menuItemText;
        
        /** True if this event was auto-inserted by normalization. */
        bool autoInserted = false;
    };
    
    //==============================================================================
    /** Parsed interaction (mouse only - MIDI support deferred). */
    struct Interaction
    {
        MouseInteraction mouse;
        
        int getDelay() const { return mouse.delayMs; }
        int getDuration() const { return mouse.durationMs; }
    };
    
    //==============================================================================
    /** Result of parsing, includes warnings. */
    struct ParseResult
    {
        Result result = Result::ok();
        StringArray warnings;
        
        bool wasOk() const { return result.wasOk(); }
        bool failed() const { return result.failed(); }
        String getErrorMessage() const { return result.getErrorMessage(); }
        
        static ParseResult ok() { return {}; }
        static ParseResult fail(const String& message) 
        { 
            ParseResult r; 
            r.result = Result::fail(message); 
            return r; 
        }
        void addWarning(const String& warning) { warnings.add(warning); }
    };
    
    //==============================================================================
    /** Parse JSON array of interactions.
     *
     *  Expands doubleClick to two clicks.
     *  Does NOT perform moveTo auto-insertion (that's done by InteractionTester).
     *
     *  @param jsonArray    The var containing the JSON array of interactions
     *  @param result       Output array to receive parsed interactions
     *  @return             ParseResult containing success/failure and any warnings
     */
    static ParseResult parseInteractions(const var& jsonArray, 
                                         Array<Interaction>& result);
    
    //==============================================================================
    // Validation limits
    static constexpr int MaxInteractions = 100;
    static constexpr int MaxDurationMs = 10000;      // 10 seconds
    static constexpr int SessionTimeoutMs = 20000;   // 20 seconds
    
private:
    //==============================================================================
    /** Parse a single interaction object (may add multiple to result for doubleClick). */
    static ParseResult parseSingleInteraction(const var& obj, 
                                              Array<Interaction>& result,
                                              int index);
    
    /** Expand doubleClick to two click interactions. */
    static ParseResult parseDoubleClick(const var& obj,
                                        Array<Interaction>& result,
                                        int index);
    
    /** Parse mouse interaction fields. */
    static ParseResult parseMouseInteraction(const var& obj,
                                             MouseInteraction& mouse,
                                             const String& type,
                                             int index);
    
    /** Parse position (normalized or absolute pixel). */
    static MouseInteraction::Position parsePosition(const var& obj);
    
    /** Parse modifier keys from JSON object. */
    static ModifierKeys parseModifiers(const var& obj);
    
    /** Helper to format error messages with context. */
    static String formatError(const String& message, int index, const Identifier& field = {});
};

//==============================================================================
/** Base interface for executing mouse/screenshot actions.
 *  Real implementation injects into JUCE.
 *  Test implementation logs for verification.
 */
struct InteractionExecutorBase
{
    virtual ~InteractionExecutorBase() = default;
    
    //==========================================================================
    /** Result of resolving component ID ↔ position (bidirectional). */
    struct ResolveResult
    {
        Rectangle<int> componentBounds;  // Absolute bounds (of subtarget if specified)
        InteractionParser::ComponentTargetPath target;      // Resolved component + subtarget
        bool visible = false;
        String error;
        
        bool success() const { return error.isEmpty(); }
        
        /** True if clicked on a child element within the component. */
        bool hasSubtarget() const { return target.hasSubtarget(); }
    };
    
    //==========================================================================
    /** Resolve component ID → bounds (for dispatch).
     *  @param target   Component + optional subtarget to resolve
     *  @returns        ResolveResult with componentBounds, visible, error
     */
    virtual ResolveResult resolveTarget(const InteractionParser::ComponentTargetPath& target) = 0;
    
    /** Resolve position → component ID (for analysis).
     *  @param absolutePos   Position in Interface/ScriptContentComponent coords
     *  @returns             ResolveResult with componentId, subtargetId,
     *                       componentBounds, visible, error
     */
    virtual ResolveResult resolvePosition(Point<int> absolutePos) const = 0;
    
    //==========================================================================
    // Primitive mouse operations (pixel coordinates)
    virtual void executeMouseDown(Point<int> pixelPos, ModifierKeys mods, 
                                  bool rightClick, int elapsedMs) = 0;
    virtual void executeMouseUp(Point<int> pixelPos, ModifierKeys mods,
                                bool rightClick, int elapsedMs) = 0;
    virtual void executeMouseMove(Point<int> pixelPos, ModifierKeys mods,
                                  int elapsedMs) = 0;
    
    // Screenshot capture
    virtual Result executeScreenshot(const String& id, const String& componentId,
                                     float scale, int elapsedMs) = 0;

    using ReplCompletion = std::function<void(var)>;
    virtual void executeRepl(const String& id, const String& expression, int elapsedMs,
                             const ReplCompletion& completion) = 0;
    
    // Synthetic input mode control
    virtual void executeSyntheticModeStart(int elapsedMs) = 0;
    virtual void executeSyntheticModeEnd(int elapsedMs) = 0;
    
    //==========================================================================
    // Cursor and menu state
    
    /** Get current cursor position in Interface coordinates. */
    virtual Point<int> getCurrentCursorPosition() const = 0;
    
    /** Set cursor position (for state tracking). */
    virtual void setCursorPosition(Point<int> pos) = 0;
    
    /** Get all currently visible menu items from open popup menus. */
    virtual Array<PopupMenu::VisibleMenuItem> getVisibleMenuItems() const = 0;
    
    /** Wait until the executor is ready to process events.
     *  @returns The number of milliseconds waited, or -1 if timed out.
     */
    virtual int waitUntilReady() = 0;
    
    /** Get MainController for console logging (optional).
     *  @returns MainController pointer, or nullptr for test executors.
     */
    virtual MainController* getMainController() const { return nullptr; }
};

//==============================================================================
/** Analyzes raw mouse events and produces semantic interactions.
 *
 *  This is the INVERSE of InteractionParser:
 *  - Parser:   JSON (semantic) → Interaction structs
 *  - Analyzer: Raw events → JSON (semantic)
 *
 *  Used by the recording feature to convert captured mouse events
 *  into the same format that the parser accepts.
 */
class InteractionAnalyzer
{
public:
    //==========================================================================
    /** Position output mode for generated JSON. */
    enum class PositionMode
    {
        Absolute,      // {"pixelPosition": {"x": 50, "y": 75}}
        Normalized     // {"position": {"x": 0.3, "y": 0.7}}
    };
    
    //==========================================================================
    /** Configuration options for analysis. */
    struct Options
    {
        PositionMode positionMode;
        int doubleClickThresholdMs;
        int clickMaxMovementPx;
        
        Options() 
            : positionMode(PositionMode::Absolute)
            , doubleClickThresholdMs(400)
            , clickMaxMovementPx(5)
        {}
    };
    
    //==========================================================================
    /** Position data captured during recording, including fallback for subtarget creation.
     *  Groups absolute position, target bounds, and parent-relative fallback together.
     */
    struct RecordedPosition
    {
        Point<int> absolute;                              // Position in ScriptContentComponent coords
        Rectangle<int> targetBounds;                      // Bounds of resolved target (subtarget if present)
        Point<float> normalizedInParent = {-1.f, -1.f};   // Normalized position in parent (for fallback)
        
        bool hasFallback() const { return normalizedInParent.x >= 0.f; }
        bool hasTargetBounds() const { return !targetBounds.isEmpty(); }
        
        /** Compute pixel position relative to target bounds. */
        Point<int> getRelativePosition() const
        {
            return {
                absolute.x - targetBounds.getX(),
                absolute.y - targetBounds.getY()
            };
        }
        
        /** Parse from raw event JSON. */
        static RecordedPosition fromVar(const var& obj);
        
        /** Write position fields to interaction JSON (pixelPosition + normalizedPositionInTarget). */
        void toInteractionVar(DynamicObject* obj, PositionMode mode) const;
    };
    
    //==========================================================================
    /** A single raw mouse event from recording. */
    struct RawEvent
    {
        Identifier type;       // InteractionIds::mouseDown, mouseUp, mouseMove, selectMenuItem
        RecordedPosition position;  // Position data with fallback info
        int64 timestamp = 0;   // Milliseconds since recording start
        bool rightClick = false;
        ModifierKeys modifiers;
        
        // Pre-resolved target info (attached during recording or via attachTargetInfo())
        InteractionParser::ComponentTargetPath target;       // Component + subtarget, or empty (unresolved)
        
        // Menu selection info (for "selectMenuItem" type events)
        int menuItemId = -1;
        String menuItemText;
        
        /** Create RawEvent from a JSON object (from RecordingListener). */
        static RawEvent fromVar(const var& obj);
    };
    
    //==========================================================================
    /** Result of analysis. */
    struct AnalyzeResult
    {
        var interactions;      // JSON array matching parser input format
        StringArray warnings;
        
        bool hasWarnings() const { return !warnings.isEmpty(); }
    };
    
    //==========================================================================
    /** Analyze raw events and produce semantic interactions JSON.
     *
     *  Detects:
     *  - click: mouseDown + mouseUp with < 5px movement
     *  - doubleClick: two clicks within 400ms at same position
     *  - drag: mouseDown + movement >= 5px + mouseUp
     *
     *  @param rawEvents   Array of raw events (from RecordingListener)
     *  @param executor    Executor for component lookup (resolvePosition)
     *  @param options     Analysis configuration
     *  @return            JSON array of interactions + warnings
     */
    static AnalyzeResult analyze(const Array<RawEvent>& rawEvents,
                                 const InteractionExecutorBase& executor,
                                 const Options& options = {});
    
    /** Convert var array (from recording) to RawEvent array. */
    static Array<RawEvent> parseRawEvents(const Array<var>& rawEventVars);
    
    /** Attach target info to raw events using executor for position resolution.
     *
     *  Iterates through events and calls executor.resolvePosition() to populate
     *  the target, subtarget, and targetBounds fields.
     *
     *  For real recording: Not needed - targets are resolved during capture.
     *  For unit tests: Call this before analyze() to simulate resolved targets.
     *
     *  @param events   Array of events to modify in place
     *  @param executor Executor providing resolvePosition()
     */
    static void attachTargetInfo(Array<RawEvent>& events, InteractionExecutorBase& executor);
    
private:
    //==========================================================================
    /** Internal state for tracking mouse gestures during analysis. */
    struct GestureState
    {
        bool mouseDown = false;
        RecordedPosition downPosition;       // Position data with fallback info
        int64 downTimestamp = 0;
        bool downRightClick = false;
        ModifierKeys downModifiers;
        InteractionParser::ComponentTargetPath downTarget;      // Component where mouse went down
        
        // For double-click detection
        int64 lastClickTimestamp = -1000;    // Far in the past
        Point<int> lastClickPosition;
        InteractionParser::ComponentTargetPath lastClickTarget; // For double-click same-target detection
        
        void reset()
        {
            mouseDown = false;
            downPosition = {};
            downTimestamp = 0;
            downRightClick = false;
            downModifiers = {};
            downTarget.clear();
        }
    };
    
    /** Create a JSON interaction object for a click. */
    static var createClickInteraction(const GestureState& state,
                                      const Options& options);
    
    /** Create a JSON interaction object for a double-click. */
    static var createDoubleClickInteraction(const GestureState& state,
                                            const Options& options);
    
    /** Create a JSON interaction object for a drag. */
    static var createDragInteraction(const GestureState& state,
                                     Point<int> endPosition,
                                     const Options& options);
    
    /** Add modifiers to interaction JSON (only if non-default). */
    static void addModifiersToInteraction(DynamicObject* obj,
                                          ModifierKeys mods);
};

} // namespace hise
