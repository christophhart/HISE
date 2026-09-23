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

namespace hise {
using namespace juce;

//==============================================================================
// Main parsing entry point
//==============================================================================

InteractionParser::ParseResult InteractionParser::parseInteractions(
    const var& jsonArray,
    Array<Interaction>& result)
{
    ParseResult parseResult;
    result.clear();
    
    if (!jsonArray.isArray())
        return ParseResult::fail("Input must be a JSON array of interactions");
    
    auto* arr = jsonArray.getArray();
    
    if (arr->isEmpty())
        return ParseResult::fail("Interaction array cannot be empty");
    
    if (arr->size() > MaxInteractions)
        return ParseResult::fail("Too many interactions: " + String(arr->size()) + 
                                 " exceeds maximum of " + String(MaxInteractions));
    
    for (int i = 0; i < arr->size(); i++)
    {
        auto singleResult = parseSingleInteraction((*arr)[i], result, i);
        
        if (singleResult.failed())
            return singleResult;
        
        for (auto& warning : singleResult.warnings)
            parseResult.addWarning(warning);
    }
    
    // Validate total sequence duration
    int totalDuration = 0;
    for (const auto& interaction : result)
    {
        totalDuration += interaction.getDelay() + interaction.getDuration();
    }
    
    if (totalDuration > SessionTimeoutMs)
    {
        parseResult.addWarning("Sequence duration (" + String(totalDuration) + 
            "ms) may exceed session timeout of " + String(SessionTimeoutMs) + "ms");
    }
    
    return parseResult;
}

//==============================================================================
// Single interaction parsing
//==============================================================================

InteractionParser::ParseResult InteractionParser::parseSingleInteraction(
    const var& obj,
    Array<Interaction>& result,
    int index)
{
    if (!obj.isObject())
        return ParseResult::fail(formatError("Must be a JSON object", index));
    
    if (!obj.hasProperty(RestApiIds::type))
        return ParseResult::fail(formatError("Missing required field 'type'", index));
    
    auto typeVar = obj[RestApiIds::type];
    if (!typeVar.isString())
        return ParseResult::fail(formatError("must be a string", index, RestApiIds::type));
    
    String type = typeVar.toString().toLowerCase();
    
    // Handle doubleClick expansion (expands to 2 clicks)
    if (type == InteractionIds::doubleClick.toString().toLowerCase())
        return parseDoubleClick(obj, result, index);
    
    // Parse single interaction
    Interaction interaction;
    auto parseRes = parseMouseInteraction(obj, interaction.mouse, type, index);
    if (parseRes.failed())
        return parseRes;
    
    result.add(interaction);
    return parseRes;
}

//==============================================================================
// DoubleClick expansion
//==============================================================================

InteractionParser::ParseResult InteractionParser::parseDoubleClick(
    const var& obj,
    Array<Interaction>& result,
    int index)
{
    // doubleClick expands to: click, click (with 20ms delay between)
    // The moveTo will be auto-inserted by the normalizer
    
    // Parse target (required)
    if (!obj.hasProperty(InteractionIds::target))
        return ParseResult::fail(formatError("Missing required field 'target'", index));
    
    auto targetVar = obj[InteractionIds::target];
    if (!targetVar.isString())
        return ParseResult::fail(formatError("must be a string", index, InteractionIds::target));
    
    String target = targetVar.toString();
    if (target.isEmpty())
        return ParseResult::fail(formatError("cannot be empty", index, InteractionIds::target));
    
    // Parse position
    auto position = parsePosition(obj);
    
    // Parse modifiers
    ModifierKeys mods = parseModifiers(obj);
    
    // Parse delay (applies to first click)
    int delay = static_cast<int>(obj.getProperty(InteractionIds::delay, 0));
    if (delay < 0)
        return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::delay));
    
    // Helper to create a click interaction
    auto createClick = [&](int clickDelay) {
        Interaction click;
        click.mouse.type = MouseInteraction::Type::Click;
        click.mouse.target = ComponentTargetPath::fromVar(obj);
        click.mouse.position = position;
        click.mouse.modifiers = mods;
        click.mouse.delayMs = clickDelay;
        click.mouse.durationMs = InteractionConstants::DefaultClickDurationMs;
        return click;
    };
    
    result.add(createClick(delay));  // First click with user-specified delay
    result.add(createClick(InteractionConstants::DefaultDoubleClickDurationMs));  // Second click
    
    return ParseResult::ok();
}

//==============================================================================
// Mouse interaction parsing
//==============================================================================

InteractionParser::ParseResult InteractionParser::parseMouseInteraction(
    const var& obj,
    MouseInteraction& mouse,
    const String& type,
    int index)
{
    // Parse type
    if (type == InteractionIds::moveTo.toString().toLowerCase())
        mouse.type = MouseInteraction::Type::MoveTo;
    else if (type == InteractionIds::click.toString().toLowerCase())
        mouse.type = MouseInteraction::Type::Click;
    else if (type == InteractionIds::drag.toString().toLowerCase())
        mouse.type = MouseInteraction::Type::Drag;
    else if (type == InteractionIds::screenshot.toString().toLowerCase())
        mouse.type = MouseInteraction::Type::Screenshot;
    else if (type == InteractionIds::selectMenuItem.toString().toLowerCase())
        mouse.type = MouseInteraction::Type::SelectMenuItem;
    else if (type == InteractionIds::repl.toString().toLowerCase())
        mouse.type = MouseInteraction::Type::Repl;
    else
        return ParseResult::fail(formatError(
            "unknown value '" + type + "'. Valid types: moveTo, click, doubleClick, drag, screenshot, selectMenuItem, repl",
            index, RestApiIds::type));

    //==========================================================================
    // REPL handling
    //==========================================================================
    if (mouse.type == MouseInteraction::Type::Repl)
    {
        if (!obj.hasProperty(RestApiIds::id))
            return ParseResult::fail(formatError("required field", index, RestApiIds::id));

        auto idVar = obj[RestApiIds::id];

        if (!idVar.isString())
            return ParseResult::fail(formatError("must be a string", index, RestApiIds::id));

        mouse.replId = idVar.toString();

        if (mouse.replId.isEmpty())
            return ParseResult::fail(formatError("cannot be empty", index, RestApiIds::id));

        if (!obj.hasProperty(RestApiIds::expression))
            return ParseResult::fail(formatError("required field", index, RestApiIds::expression));

        auto expressionVar = obj[RestApiIds::expression];

        if (!expressionVar.isString())
            return ParseResult::fail(formatError("must be a string", index, RestApiIds::expression));

        mouse.replExpression = expressionVar.toString();

        if (mouse.replExpression.isEmpty())
            return ParseResult::fail(formatError("cannot be empty", index, RestApiIds::expression));

        mouse.delayMs = static_cast<int>(obj.getProperty(InteractionIds::delay, 0));

        if (mouse.delayMs < 0)
            return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::delay));

        return ParseResult::ok();
    }
    
    //==========================================================================
    // Screenshot handling
    //==========================================================================
    if (mouse.type == MouseInteraction::Type::Screenshot)
    {
        if (!obj.hasProperty(RestApiIds::id))
            return ParseResult::fail(formatError("required field", index, RestApiIds::id));
        
        auto idVar = obj[RestApiIds::id];
        if (!idVar.isString())
            return ParseResult::fail(formatError("must be a string", index, RestApiIds::id));
        
        mouse.screenshotId = idVar.toString();
        if (mouse.screenshotId.isEmpty())
            return ParseResult::fail(formatError("cannot be empty", index, RestApiIds::id));

        if (obj.hasProperty(RestApiIds::componentId))
        {
            auto componentIdVar = obj[RestApiIds::componentId];

            if (!componentIdVar.isString())
                return ParseResult::fail(formatError("must be a string", index, RestApiIds::componentId));

            mouse.screenshotComponentId = componentIdVar.toString();

            if (mouse.screenshotComponentId.isEmpty())
                return ParseResult::fail(formatError("cannot be empty", index, RestApiIds::componentId));
        }
        
        // Parse scale (optional, defaults to 1.0)
        mouse.screenshotScale = static_cast<float>(obj.getProperty(RestApiIds::scale, 1.0f));
        if (mouse.screenshotScale != 0.5f && mouse.screenshotScale != 1.0f)
            return ParseResult::fail(formatError("must be 0.5 or 1.0", index, RestApiIds::scale));

        mouse.delayMs = static_cast<int>(obj.getProperty(InteractionIds::delay, 0));
        if (mouse.delayMs < 0)
            return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::delay));
        
        // Screenshot has no other fields
        return ParseResult::ok();
    }
    
    //==========================================================================
    // SelectMenuItem handling
    //==========================================================================
    if (mouse.type == MouseInteraction::Type::SelectMenuItem)
    {
        if (!obj.hasProperty(InteractionIds::menuItemText))
            return ParseResult::fail(formatError("Missing required field 'menuItemText' for selectMenuItem", index));
        
        auto textVar = obj[InteractionIds::menuItemText];
        if (!textVar.isString())
            return ParseResult::fail(formatError("must be a string", index, InteractionIds::menuItemText));
        
        mouse.menuItemText = textVar.toString();
        if (mouse.menuItemText.isEmpty())
            return ParseResult::fail(formatError("cannot be empty", index, InteractionIds::menuItemText));
        
        // Parse delay (optional)
        mouse.delayMs = static_cast<int>(obj.getProperty(InteractionIds::delay, 0));
        if (mouse.delayMs < 0)
            return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::delay));
        
        // Parse duration (optional)
        mouse.durationWasExplicit = obj.hasProperty(InteractionIds::duration);

        if (mouse.durationWasExplicit)
        {
            auto durationVar = obj[InteractionIds::duration];
            if (!(durationVar.isInt() || durationVar.isInt64() || durationVar.isDouble()))
                return ParseResult::fail(formatError("must be an integer", index, InteractionIds::duration));

            auto durationValue = static_cast<double>(durationVar);
            if (!std::isfinite(durationValue) || std::floor(durationValue) != durationValue)
                return ParseResult::fail(formatError("must be an integer", index, InteractionIds::duration));
            if (durationValue < 0.0)
                return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::duration));
            if (durationValue > MaxDurationMs)
                return ParseResult::fail(formatError("exceeds maximum of "
                    + String(MaxDurationMs) + "ms", index, InteractionIds::duration));
        }

        mouse.durationMs = static_cast<int>(obj.getProperty(InteractionIds::duration, InteractionConstants::DefaultMenuSelectDurationMs));
        if (mouse.durationMs < 0)
            return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::duration));
        if (mouse.durationMs > MaxDurationMs)
            return ParseResult::fail(formatError("exceeds maximum of " + 
                String(MaxDurationMs) + "ms", index, InteractionIds::duration));
        
        // SelectMenuItem has no target or position
        return ParseResult::ok();
    }
    
    //==========================================================================
    // Target-based interactions (moveTo, click, drag)
    //==========================================================================
    
    // Parse target (required) and optional subtarget
    if (!obj.hasProperty(InteractionIds::target))
        return ParseResult::fail(formatError("Missing required field 'target'", index));
    
    auto targetVar = obj[InteractionIds::target];
    if (!targetVar.isString())
        return ParseResult::fail(formatError("must be a string", index, InteractionIds::target));
    
    mouse.target = ComponentTargetPath::fromVar(obj);
    if (mouse.target.componentId.isEmpty())
        return ParseResult::fail(formatError("cannot be empty", index, InteractionIds::target));
    
    // Parse position (optional, defaults to center)
    mouse.position = parsePosition(obj);
    
    // Parse normalizedPositionInTarget (optional, for subtarget fallback)
    if (obj.hasProperty(InteractionIds::normalizedPositionInTarget))
    {
        auto normPos = obj[InteractionIds::normalizedPositionInTarget];
        if (normPos.isObject() && normPos.hasProperty(RestApiIds::x) && normPos.hasProperty(RestApiIds::y))
        {
            mouse.position.normalizedInParent = Point<float>(
                static_cast<float>(normPos[RestApiIds::x]),
                static_cast<float>(normPos[RestApiIds::y])
            );
        }
    }
    
    // Parse delay (optional)
    mouse.delayMs = static_cast<int>(obj.getProperty(InteractionIds::delay, 0));
    if (mouse.delayMs < 0)
        return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::delay));
    
    //==========================================================================
    // Type-specific parsing
    //==========================================================================
    mouse.durationWasExplicit = obj.hasProperty(InteractionIds::duration);

    if (mouse.durationWasExplicit)
    {
        auto durationVar = obj[InteractionIds::duration];
        if (!(durationVar.isInt() || durationVar.isInt64() || durationVar.isDouble()))
            return ParseResult::fail(formatError("must be an integer", index, InteractionIds::duration));

        auto durationValue = static_cast<double>(durationVar);
        if (!std::isfinite(durationValue) || std::floor(durationValue) != durationValue)
            return ParseResult::fail(formatError("must be an integer", index, InteractionIds::duration));
        if (durationValue < 0.0)
            return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::duration));
        if (durationValue > MaxDurationMs)
            return ParseResult::fail(formatError("exceeds maximum of "
                + String(MaxDurationMs) + "ms", index, InteractionIds::duration));
    }

    switch (mouse.type)
    {
        case MouseInteraction::Type::MoveTo:
        {
            mouse.durationMs = static_cast<int>(obj.getProperty(InteractionIds::duration, InteractionConstants::DefaultMoveDurationMs));
            break;
        }
        
        case MouseInteraction::Type::Click:
        {
            mouse.durationMs = static_cast<int>(obj.getProperty(InteractionIds::duration, InteractionConstants::DefaultClickDurationMs));
            mouse.rightClick = static_cast<bool>(obj.getProperty(InteractionIds::rightClick, false));
            mouse.modifiers = parseModifiers(obj);
            break;
        }
        
        case MouseInteraction::Type::Drag:
        {
            mouse.durationMs = static_cast<int>(obj.getProperty(InteractionIds::duration, InteractionConstants::DefaultDragDurationMs));
            mouse.modifiers = parseModifiers(obj);
            
            // Delta required for drag
            if (!obj.hasProperty(InteractionIds::delta))
                return ParseResult::fail(formatError("missing required field for drag", index, InteractionIds::delta));
            
            auto deltaVar = obj[InteractionIds::delta];
            if (!deltaVar.isObject())
                return ParseResult::fail(formatError("must be an object with 'x' and 'y'", index, InteractionIds::delta));
            
            if (!deltaVar.hasProperty(RestApiIds::x) || !deltaVar.hasProperty(RestApiIds::y))
                return ParseResult::fail(formatError("must have both 'x' and 'y' properties", index, InteractionIds::delta));
            
            mouse.deltaPixels = {
                static_cast<int>(deltaVar[RestApiIds::x]),
                static_cast<int>(deltaVar[RestApiIds::y])
            };
            break;
        }
        
        default:
            break;
    }
    
    // Validate duration
    if (mouse.durationMs < 0)
        return ParseResult::fail(formatError("cannot be negative", index, InteractionIds::duration));
    if (mouse.durationMs > MaxDurationMs)
        return ParseResult::fail(formatError("exceeds maximum of " + 
            String(MaxDurationMs) + "ms", index, InteractionIds::duration));
    
    return ParseResult::ok();
}

//==============================================================================
// Position parsing
//==============================================================================

InteractionParser::MouseInteraction::Position InteractionParser::parsePosition(const var& obj)
{
    // pixelPosition takes precedence over position
    if (obj.hasProperty(RestApiIds::pixelPosition))
    {
        auto px = obj[RestApiIds::pixelPosition];
        if (px.isObject() && px.hasProperty(RestApiIds::x) && px.hasProperty(RestApiIds::y))
        {
            return MouseInteraction::Position::absolute(
                static_cast<int>(px[RestApiIds::x]),
                static_cast<int>(px[RestApiIds::y])
            );
        }
    }
    
    // Normalized position
    if (obj.hasProperty(InteractionIds::normalizedPosition))
    {
        auto pos = obj[InteractionIds::normalizedPosition];
        if (pos.isObject() && pos.hasProperty(RestApiIds::x) && pos.hasProperty(RestApiIds::y))
        {
            return MouseInteraction::Position::normalized(
                static_cast<float>(pos[RestApiIds::x]),
                static_cast<float>(pos[RestApiIds::y])
            );
        }
    }
    
    // Default: center
    return MouseInteraction::Position::center();
}

//==============================================================================
// Modifier parsing
//==============================================================================

ModifierKeys InteractionParser::parseModifiers(const var& obj)
{
    int flags = 0;
    
    if (static_cast<bool>(obj.getProperty(InteractionIds::shiftDown, false)))
        flags |= ModifierKeys::shiftModifier;
    if (static_cast<bool>(obj.getProperty(InteractionIds::ctrlDown, false)))
        flags |= ModifierKeys::ctrlModifier;
    if (static_cast<bool>(obj.getProperty(InteractionIds::altDown, false)))
        flags |= ModifierKeys::altModifier;
    if (static_cast<bool>(obj.getProperty(InteractionIds::cmdDown, false)))
        flags |= ModifierKeys::commandModifier;
    
    return ModifierKeys(flags);
}

//==============================================================================
// Helper methods
//==============================================================================

String InteractionParser::formatError(const String& message, int index, const Identifier& field)
{
    String error = "Interaction [" + String(index) + "]";
    
    if (field.isValid())
        error += " field '" + field.toString() + "'";
    
    error += ": " + message;
    
    return error;
}

//==============================================================================
// InteractionAnalyzer implementation
//==============================================================================

InteractionAnalyzer::RecordedPosition InteractionAnalyzer::RecordedPosition::fromVar(const var& obj)
{
    RecordedPosition pos;
    
    pos.absolute = Point<int>(
        static_cast<int>(obj.getProperty(RestApiIds::x, 0)),
        static_cast<int>(obj.getProperty(RestApiIds::y, 0))
    );
    
    if (obj.hasProperty(InteractionIds::targetBounds))
    {
        auto bounds = obj[InteractionIds::targetBounds];
        if (bounds.isArray() && bounds.size() >= 4)
        {
            pos.targetBounds = Rectangle<int>(
                static_cast<int>(bounds[0]),
                static_cast<int>(bounds[1]),
                static_cast<int>(bounds[2]),
                static_cast<int>(bounds[3])
            );
        }
    }
    
    if (obj.hasProperty(InteractionIds::normalizedPositionInTarget))
    {
        auto normPos = obj[InteractionIds::normalizedPositionInTarget];
        if (normPos.isObject())
        {
            pos.normalizedInParent = Point<float>(
                static_cast<float>(normPos.getProperty(RestApiIds::x, -1.f)),
                static_cast<float>(normPos.getProperty(RestApiIds::y, -1.f))
            );
        }
    }
    
    return pos;
}

void InteractionAnalyzer::RecordedPosition::toInteractionVar(DynamicObject* obj, PositionMode mode) const
{
    if (targetBounds.isEmpty())
    {
        // No component - use absolute coordinates
        DynamicObject::Ptr pos = new DynamicObject();
        pos->setProperty(RestApiIds::x, absolute.x);
        pos->setProperty(RestApiIds::y, absolute.y);
        obj->setProperty(RestApiIds::pixelPosition, var(pos.get()));
    }
    else
    {
        // Component available - use relative coordinates
        Point<int> relPos = getRelativePosition();
        
        if (mode == PositionMode::Absolute)
        {
            DynamicObject::Ptr pos = new DynamicObject();
            pos->setProperty(RestApiIds::x, relPos.x);
            pos->setProperty(RestApiIds::y, relPos.y);
            obj->setProperty(RestApiIds::pixelPosition, var(pos.get()));
        }
        else
        {
            // Normalized mode
            float normX = static_cast<float>(relPos.x) / targetBounds.getWidth();
            float normY = static_cast<float>(relPos.y) / targetBounds.getHeight();
            
            DynamicObject::Ptr pos = new DynamicObject();
            pos->setProperty(RestApiIds::x, normX);
            pos->setProperty(RestApiIds::y, normY);
            obj->setProperty(InteractionIds::normalizedPosition, var(pos.get()));
        }
    }
    
    // Write fallback position if available
    if (hasFallback())
    {
        DynamicObject::Ptr normPos = new DynamicObject();
        normPos->setProperty(RestApiIds::x, normalizedInParent.x);
        normPos->setProperty(RestApiIds::y, normalizedInParent.y);
        obj->setProperty(InteractionIds::normalizedPositionInTarget, var(normPos.get()));
    }
}

InteractionAnalyzer::RawEvent InteractionAnalyzer::RawEvent::fromVar(const var& obj)
{
    RawEvent event;
    
    event.type = Identifier(obj.getProperty(RestApiIds::type, "").toString());
    event.position = RecordedPosition::fromVar(obj);
    event.timestamp = static_cast<int64>(obj.getProperty(InteractionIds::timestamp, 0));
    event.rightClick = static_cast<bool>(obj.getProperty(InteractionIds::rightClick, false));
    
    // Parse modifiers if present
    if (obj.hasProperty(InteractionIds::modifiers))
    {
        auto mods = obj[InteractionIds::modifiers];
        int flags = 0;
        
        if (static_cast<bool>(mods.getProperty(InteractionIds::shiftDown, false)))
            flags |= ModifierKeys::shiftModifier;
        if (static_cast<bool>(mods.getProperty(InteractionIds::ctrlDown, false)))
            flags |= ModifierKeys::ctrlModifier;
        if (static_cast<bool>(mods.getProperty(InteractionIds::altDown, false)))
            flags |= ModifierKeys::altModifier;
        if (static_cast<bool>(mods.getProperty(InteractionIds::cmdDown, false)))
            flags |= ModifierKeys::commandModifier;
        
        event.modifiers = ModifierKeys(flags);
    }
    
    // Parse pre-resolved target info if present
    event.target = InteractionParser::ComponentTargetPath::fromVar(obj);
    
    // Parse menu selection info if present (for "selectMenuItem" type)
    event.menuItemId = static_cast<int>(obj.getProperty(RestApiIds::itemId, -1));
    event.menuItemText = obj.getProperty(InteractionIds::menuItemText, "").toString();
    
    return event;
}

Array<InteractionAnalyzer::RawEvent> InteractionAnalyzer::parseRawEvents(const Array<var>& rawEventVars)
{
    Array<RawEvent> events;
    events.ensureStorageAllocated(rawEventVars.size());
    
    for (const auto& eventVar : rawEventVars)
    {
        events.add(RawEvent::fromVar(eventVar));
    }
    
    return events;
}

void InteractionAnalyzer::attachTargetInfo(Array<RawEvent>& events, InteractionExecutorBase& executor)
{
    for (auto& event : events)
    {
        auto resolveResult = executor.resolvePosition(event.position.absolute);
        
        if (resolveResult.target.isEmpty())
        {
            // No component at position - click on background
            event.target = InteractionParser::ComponentTargetPath(InteractionIds::content.toString());
            event.position.targetBounds = {};
        }
        else
        {
            event.target = resolveResult.target;
            event.position.targetBounds = resolveResult.componentBounds;
            
            // Compute normalized fallback position if this is a subtarget
            if (resolveResult.target.hasSubtarget())
            {
                // Resolve parent-only to get parent bounds
                InteractionParser::ComponentTargetPath parentOnly(resolveResult.target.componentId);
                auto parentResult = executor.resolveTarget(parentOnly);
                
                if (parentResult.success() && !parentResult.componentBounds.isEmpty())
                {
                    float normX = (event.position.absolute.x - parentResult.componentBounds.getX()) 
                                / static_cast<float>(parentResult.componentBounds.getWidth());
                    float normY = (event.position.absolute.y - parentResult.componentBounds.getY()) 
                                / static_cast<float>(parentResult.componentBounds.getHeight());
                    
                    event.position.normalizedInParent = {normX, normY};
                }
            }
        }
    }
}

InteractionAnalyzer::AnalyzeResult InteractionAnalyzer::analyze(
    const Array<RawEvent>& rawEvents,
    const InteractionExecutorBase& executor,
    const Options& options)
{
    ignoreUnused(executor);  // Targets are now pre-attached via attachTargetInfo()
    
    AnalyzeResult result;
    Array<var> interactions;
    GestureState state;
    
    for (const auto& event : rawEvents)
    {
        // Handle selectMenuItem events - pass through directly (already semantic)
        if (event.type == InteractionIds::selectMenuItem)
        {
            DynamicObject::Ptr obj = new DynamicObject();
            obj->setProperty(RestApiIds::type, InteractionIds::selectMenuItem.toString());
            obj->setProperty(RestApiIds::itemId, event.menuItemId);
            if (event.menuItemText.isNotEmpty())
                obj->setProperty(InteractionIds::menuItemText, event.menuItemText);
            interactions.add(var(obj.get()));
            continue;
        }
        
        if (event.type == InteractionIds::mouseDown)
        {
            // Validate that target info was attached (via recording or attachTargetInfo())
            if (event.target.isEmpty())
            {
                result.warnings.add("mouseDown at (" + String(event.position.absolute.x) + ", " + 
                                   String(event.position.absolute.y) + ") has no target info - call attachTargetInfo() first");
            }
            
            state.mouseDown = true;
            state.downPosition = event.position;
            state.downTimestamp = event.timestamp;
            state.downRightClick = event.rightClick;
            state.downModifiers = event.modifiers;
            state.downTarget = event.target;
        }
        else if (event.type == InteractionIds::mouseUp)
        {
            if (!state.mouseDown)
            {
                // mouseUp without mouseDown - skip
                result.warnings.add("mouseUp without matching mouseDown at timestamp " + 
                                   String(event.timestamp));
                continue;
            }
            
            // Calculate movement distance
            int dx = event.position.absolute.x - state.downPosition.absolute.x;
            int dy = event.position.absolute.y - state.downPosition.absolute.y;
            int distance = static_cast<int>(std::sqrt(dx * dx + dy * dy));
            
            if (distance < options.clickMaxMovementPx)
            {
                // It's a click - check for double-click
                bool isDoubleClick = false;
                
                if (state.lastClickTimestamp >= 0)
                {
                    int64 timeSinceLastClick = state.downTimestamp - state.lastClickTimestamp;
                    int lastClickDx = state.downPosition.absolute.x - state.lastClickPosition.x;
                    int lastClickDy = state.downPosition.absolute.y - state.lastClickPosition.y;
                    int lastClickDist = static_cast<int>(std::sqrt(lastClickDx * lastClickDx + lastClickDy * lastClickDy));
                    
                    if (timeSinceLastClick <= options.doubleClickThresholdMs &&
                        lastClickDist < options.clickMaxMovementPx &&
                        state.lastClickTarget == state.downTarget)
                    {
                        isDoubleClick = true;
                    }
                }
                
                if (isDoubleClick)
                {
                    // Remove the previous click and replace with doubleClick
                    if (interactions.size() > 0)
                    {
                        interactions.removeLast();
                    }
                    interactions.add(createDoubleClickInteraction(state, options));
                    
                    // Reset last click tracking (can't triple-click)
                    state.lastClickTimestamp = -1000;
                    state.lastClickPosition = {};
                    state.lastClickTarget.clear();
                }
                else
                {
                    // Single click
                    interactions.add(createClickInteraction(state, options));
                    
                    // Track for potential double-click
                    state.lastClickTimestamp = state.downTimestamp;
                    state.lastClickPosition = state.downPosition.absolute;
                    state.lastClickTarget = state.downTarget;
                }
            }
            else
            {
                // It's a drag
                interactions.add(createDragInteraction(state, event.position.absolute, options));
                
                // Reset last click tracking (drag breaks double-click)
                state.lastClickTimestamp = -1000;
                state.lastClickPosition = {};
                state.lastClickTarget.clear();
            }
            
            state.reset();
        }
        // mouseMove events are ignored for semantic analysis
    }
    
    // Handle case where recording ended with mouse still down
    if (state.mouseDown)
    {
        result.warnings.add("Recording ended with mouse still down");
    }
    
    result.interactions = interactions;
    return result;
}

var InteractionAnalyzer::createClickInteraction(const GestureState& state,
                                                 const Options& options)
{
    DynamicObject::Ptr obj = new DynamicObject();
    
    obj->setProperty(RestApiIds::type, InteractionIds::click.toString());
    state.downTarget.toVar(obj.get());
    state.downPosition.toInteractionVar(obj.get(), options.positionMode);
    
    if (state.downRightClick)
    {
        obj->setProperty(InteractionIds::rightClick, true);
    }
    
    addModifiersToInteraction(obj.get(), state.downModifiers);
    
    return var(obj.get());
}

var InteractionAnalyzer::createDoubleClickInteraction(const GestureState& state,
                                                       const Options& options)
{
    DynamicObject::Ptr obj = new DynamicObject();
    
    obj->setProperty(RestApiIds::type, InteractionIds::doubleClick.toString());
    state.downTarget.toVar(obj.get());
    state.downPosition.toInteractionVar(obj.get(), options.positionMode);
    
    addModifiersToInteraction(obj.get(), state.downModifiers);
    
    return var(obj.get());
}

var InteractionAnalyzer::createDragInteraction(const GestureState& state,
                                                Point<int> endPosition,
                                                const Options& options)
{
    DynamicObject::Ptr obj = new DynamicObject();
    
    obj->setProperty(RestApiIds::type, InteractionIds::drag.toString());
    state.downTarget.toVar(obj.get());
    state.downPosition.toInteractionVar(obj.get(), options.positionMode);
    
    // Delta
    DynamicObject::Ptr delta = new DynamicObject();
    delta->setProperty(RestApiIds::x, endPosition.x - state.downPosition.absolute.x);
    delta->setProperty(RestApiIds::y, endPosition.y - state.downPosition.absolute.y);
    obj->setProperty(InteractionIds::delta, var(delta.get()));
    
    addModifiersToInteraction(obj.get(), state.downModifiers);
    
    return var(obj.get());
}

void InteractionAnalyzer::addModifiersToInteraction(DynamicObject* obj,
                                                     ModifierKeys mods)
{
    // Only add modifiers that are active (non-default = non-noisy)
    if (mods.isShiftDown())
        obj->setProperty(InteractionIds::shiftDown, true);
    if (mods.isCtrlDown())
        obj->setProperty(InteractionIds::ctrlDown, true);
    if (mods.isAltDown())
        obj->setProperty(InteractionIds::altDown, true);
    if (mods.isCommandDown())
        obj->setProperty(InteractionIds::cmdDown, true);
}

} // namespace hise
