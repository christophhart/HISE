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

class InteractionParserTests : public UnitTest
{
public:
    InteractionParserTests() : UnitTest("Interaction Parser Tests", "AI Tools") {}
    
    void runTest() override
    {
        // Basic Parsing
        testParseNotArray();
        testParseEmptyArray();
        testParseMissingType();
        testParseInvalidType();
        testParseMissingTarget();
        testParseInteractionNotObject();
        testParseTypeNotString();
        testParseTargetNotString();
        testParseEmptyTargetString();
        
        // Click Parsing
        testParseValidClick();
        testParseValidClickWithPosition();
        testParseValidClickWithPixelPosition();
        testParseValidClickRightClick();
        testParseValidClickWithModifiers();
        testParseValidClickWithDelay();
        testParseExplicitDuration();
        
        // DoubleClick Expansion
        testParseDoubleClickExpandsToTwoClicks();
        testParseDoubleClickWithDelay();
        testParseDoubleClickPreservesTarget();
        
        // MoveTo Parsing
        testParseValidMoveTo();
        testParseValidMoveToWithPosition();
        testParseValidMoveToWithPixelPosition();
        
        // Drag Parsing
        testParseValidDrag();
        testParseValidDragWithModifiers();
        testParseDragMissingDelta();
        testParseDragDeltaNotObject();
        
        // Screenshot Parsing
        testParseValidScreenshot();
        testParseScreenshotComponent();
        testParseScreenshotDelay();
        testParseScreenshotMissingId();
        testParseScreenshotInvalidScale();
        testParseScreenshotNegativeDelay();

        // REPL Parsing
        testParseValidRepl();
        testParseReplMissingId();
        testParseReplMissingExpression();
        
        // SelectMenuItem Parsing
        testParseValidSelectMenuItem();
        testParseSelectMenuItemMissingText();
        
        // Position Struct
        testPositionNormalized();
        testPositionAbsolute();
        testPositionCenter();
        testPositionGetPixelPosition();
        
        // Delay and Duration
        testParseNegativeDelay();
        testParseNegativeDuration();
        testParseNonNumericDuration();
        testParseDurationExceedsLimit();
        
        // Default Values
        testParseClickDefaults();
        testParseDragDefaults();
        testParseMoveToDefaults();
        
        // Multiple Interactions
        testParseMultipleInteractions();
        testParseTooManyInteractions();
    }
    
private:
    
    //==============================================================================
    // Type aliases
    using Interaction = InteractionParser::Interaction;
    using MouseInteraction = InteractionParser::MouseInteraction;
    using ParseResult = InteractionParser::ParseResult;
    using Position = InteractionParser::MouseInteraction::Position;
    
    //==============================================================================
    // Helper Methods
    
    Array<Interaction> parseOrFail(const String& json)
    {
        var input = JSON::parse(json);
        Array<Interaction> result;
        auto parseResult = InteractionParser::parseInteractions(input, result);
        
        expect(parseResult.wasOk(), 
               "Parse should succeed: " + parseResult.getErrorMessage());
        
        return result;
    }
    
    ParseResult parseExpectingFailure(const String& json)
    {
        var input = JSON::parse(json);
        Array<Interaction> result;
        return InteractionParser::parseInteractions(input, result);
    }
    
    //==============================================================================
    // Basic Parsing Tests
    //==============================================================================
    
    void testParseNotArray()
    {
        beginTest("Syntax: Input must be array");
        
        auto result = parseExpectingFailure(R"({"type": "click", "target": "Btn"})");
        expect(result.failed(), "Should reject non-array input");
        expect(result.getErrorMessage().containsIgnoreCase("array"),
               "Error should mention 'array'");
    }
    
    void testParseEmptyArray()
    {
        beginTest("Syntax: Empty array rejected");
        
        auto result = parseExpectingFailure(R"([])");
        expect(result.failed(), "Should reject empty array");
    }
    
    void testParseMissingType()
    {
        beginTest("Syntax: Missing 'type' field");
        
        auto result = parseExpectingFailure(R"([{"target": "Button1"}])");
        expect(result.failed(), "Should reject missing type");
    }
    
    void testParseInvalidType()
    {
        beginTest("Syntax: Invalid type value");
        
        auto result = parseExpectingFailure(R"([{"type": "invalid", "target": "Button1"}])");
        expect(result.failed(), "Should reject invalid type");
    }
    
    void testParseMissingTarget()
    {
        beginTest("Syntax: Missing 'target' for click");
        
        auto result = parseExpectingFailure(R"([{"type": "click"}])");
        expect(result.failed(), "Should reject missing target");
    }
    
    void testParseInteractionNotObject()
    {
        beginTest("Syntax: Interaction must be object");
        
        auto result = parseExpectingFailure(R"(["click"])");
        expect(result.failed(), "Should reject non-object");
    }
    
    void testParseTypeNotString()
    {
        beginTest("Syntax: Type must be string");
        
        auto result = parseExpectingFailure(R"([{"type": 123, "target": "Button1"}])");
        expect(result.failed(), "Should reject non-string type");
    }
    
    void testParseTargetNotString()
    {
        beginTest("Syntax: Target must be string");
        
        auto result = parseExpectingFailure(R"([{"type": "click", "target": 123}])");
        expect(result.failed(), "Should reject non-string target");
    }
    
    void testParseEmptyTargetString()
    {
        beginTest("Syntax: Empty target rejected");
        
        auto result = parseExpectingFailure(R"([{"type": "click", "target": ""}])");
        expect(result.failed(), "Should reject empty target");
    }
    
    //==============================================================================
    // Click Parsing Tests
    //==============================================================================
    
    void testParseValidClick()
    {
        beginTest("Click: Valid minimal click");
        
        auto interactions = parseOrFail(R"([{"type": "click", "target": "Button1"}])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.type == MouseInteraction::Type::Click, "Should be Click");
        expect(interactions[0].mouse.target.componentId == "Button1", "Target should match");
        expect(interactions[0].mouse.position.isCenter(), "Position should be center");
    }
    
    void testParseValidClickWithPosition()
    {
        beginTest("Click: With normalized position");
        
        auto interactions = parseOrFail(R"([
            {"type": "click", "target": "Panel1", "normalizedPosition": {"x": 0.2, "y": 0.8}}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(!interactions[0].mouse.position.isAbsolute(), "Should be normalized");
        
        auto pos = interactions[0].mouse.position.getValue();
        expectWithinAbsoluteError(pos.x, 0.2f, 0.001f, "X should be 0.2");
        expectWithinAbsoluteError(pos.y, 0.8f, 0.001f, "Y should be 0.8");
    }
    
    void testParseValidClickWithPixelPosition()
    {
        beginTest("Click: With absolute pixel position");
        
        auto interactions = parseOrFail(R"([
            {"type": "click", "target": "Panel1", "pixelPosition": {"x": 50, "y": 75}}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.position.isAbsolute(), "Should be absolute");
        
        // Test getPixelPosition with mock bounds
        Rectangle<int> bounds{100, 100, 200, 200};
        auto pixel = interactions[0].mouse.position.getPixelPosition(bounds);
        expect(pixel.x == 150, "Pixel X should be 100 + 50 = 150");
        expect(pixel.y == 175, "Pixel Y should be 100 + 75 = 175");
    }
    
    void testParseValidClickRightClick()
    {
        beginTest("Click: Right click");
        
        auto interactions = parseOrFail(R"([
            {"type": "click", "target": "Knob1", "rightClick": true}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.rightClick, "Should be right click");
    }
    
    void testParseValidClickWithModifiers()
    {
        beginTest("Click: With modifiers");
        
        auto interactions = parseOrFail(R"([
            {"type": "click", "target": "Button1", 
             "shiftDown": true, "ctrlDown": true, "altDown": false}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.modifiers.isShiftDown(), "Shift should be down");
        expect(interactions[0].mouse.modifiers.isCtrlDown(), "Ctrl should be down");
        expect(!interactions[0].mouse.modifiers.isAltDown(), "Alt should not be down");
    }
    
    void testParseValidClickWithDelay()
    {
        beginTest("Click: With delay");
        
        auto interactions = parseOrFail(R"([
            {"type": "click", "target": "Button1", "delay": 500}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.delayMs == 500, "Delay should be 500ms");
    }

    void testParseExplicitDuration()
    {
        beginTest("Click: Explicit duration is tracked");

        auto explicitDuration = parseOrFail(R"([
            {"type": "moveTo", "target": "Button1", "duration": 100},
            {"type": "click", "target": "Button1", "duration": 100},
            {"type": "drag", "target": "Button1", "delta": {"x": 10, "y": 0}, "duration": 100},
            {"type": "selectMenuItem", "menuItemText": "Option 1", "duration": 100}
        ])");
        auto defaultDuration = parseOrFail(R"([{"type": "click", "target": "Button1"}])");

        for (const auto& interaction : explicitDuration)
            expect(interaction.mouse.durationWasExplicit, "Supplied duration should be explicit");
        expect(!defaultDuration[0].mouse.durationWasExplicit, "Default duration should not be explicit");
    }
    
    //==============================================================================
    // DoubleClick Expansion Tests
    //==============================================================================
    
    void testParseDoubleClickExpandsToTwoClicks()
    {
        beginTest("DoubleClick: Expands to two clicks");
        
        auto interactions = parseOrFail(R"([
            {"type": "doubleClick", "target": "Button1"}
        ])");
        
        expect(interactions.size() == 2, "Should expand to 2 clicks");
        expect(interactions[0].mouse.type == MouseInteraction::Type::Click, "First should be Click");
        expect(interactions[1].mouse.type == MouseInteraction::Type::Click, "Second should be Click");
        expect(interactions[1].mouse.delayMs == InteractionConstants::DefaultDoubleClickDurationMs, 
               "Second click should have 20ms delay");
    }
    
    void testParseDoubleClickWithDelay()
    {
        beginTest("DoubleClick: Delay goes to first click");
        
        auto interactions = parseOrFail(R"([
            {"type": "doubleClick", "target": "Button1", "delay": 300}
        ])");
        
        expect(interactions.size() == 2, "Should expand to 2 clicks");
        expect(interactions[0].mouse.delayMs == 300, "First click should have 300ms delay");
        expect(interactions[1].mouse.delayMs == InteractionConstants::DefaultDoubleClickDurationMs, 
               "Second click should have 20ms delay");
    }
    
    void testParseDoubleClickPreservesTarget()
    {
        beginTest("DoubleClick: Both clicks have same target");
        
        auto interactions = parseOrFail(R"([
            {"type": "doubleClick", "target": "Panel1", "normalizedPosition": {"x": 0.3, "y": 0.7}}
        ])");
        
        expect(interactions.size() == 2, "Should expand to 2 clicks");
        expect(interactions[0].mouse.target.componentId == "Panel1", "First target should match");
        expect(interactions[1].mouse.target.componentId == "Panel1", "Second target should match");
        expect(interactions[0].mouse.position == interactions[1].mouse.position, 
               "Both should have same position");
    }
    
    //==============================================================================
    // MoveTo Parsing Tests
    //==============================================================================
    
    void testParseValidMoveTo()
    {
        beginTest("MoveTo: Valid minimal");
        
        auto interactions = parseOrFail(R"([{"type": "moveTo", "target": "Button1"}])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.type == MouseInteraction::Type::MoveTo, "Should be MoveTo");
        expect(interactions[0].mouse.durationMs == InteractionConstants::DefaultMoveDurationMs, 
               "Duration should be default 700ms");
    }
    
    void testParseValidMoveToWithPosition()
    {
        beginTest("MoveTo: With position");
        
        auto interactions = parseOrFail(R"([
            {"type": "moveTo", "target": "Panel1", "normalizedPosition": {"x": 0.1, "y": 0.9}}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        auto pos = interactions[0].mouse.position.getValue();
        expectWithinAbsoluteError(pos.x, 0.1f, 0.001f, "X should be 0.1");
        expectWithinAbsoluteError(pos.y, 0.9f, 0.001f, "Y should be 0.9");
    }
    
    void testParseValidMoveToWithPixelPosition()
    {
        beginTest("MoveTo: With pixel position");
        
        auto interactions = parseOrFail(R"([
            {"type": "moveTo", "target": "Panel1", "pixelPosition": {"x": 100, "y": 50}}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.position.isAbsolute(), "Should be absolute");
    }
    
    //==============================================================================
    // Drag Parsing Tests
    //==============================================================================
    
    void testParseValidDrag()
    {
        beginTest("Drag: Valid with delta");
        
        auto interactions = parseOrFail(R"([
            {"type": "drag", "target": "Knob1", "delta": {"x": 0, "y": -50}}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.type == MouseInteraction::Type::Drag, "Should be Drag");
        expect(interactions[0].mouse.deltaPixels.x == 0, "Delta X should be 0");
        expect(interactions[0].mouse.deltaPixels.y == -50, "Delta Y should be -50");
    }
    
    void testParseValidDragWithModifiers()
    {
        beginTest("Drag: With shift modifier");
        
        auto interactions = parseOrFail(R"([
            {"type": "drag", "target": "Knob1", "delta": {"x": 0, "y": -50}, "shiftDown": true}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.modifiers.isShiftDown(), "Shift should be down");
    }
    
    void testParseDragMissingDelta()
    {
        beginTest("Drag: Missing delta rejected");
        
        auto result = parseExpectingFailure(R"([
            {"type": "drag", "target": "Knob1"}
        ])");
        
        expect(result.failed(), "Should reject drag without delta");
        expect(result.getErrorMessage().containsIgnoreCase("delta"), 
               "Error should mention 'delta'");
    }
    
    void testParseDragDeltaNotObject()
    {
        beginTest("Drag: Delta must be object");
        
        auto result = parseExpectingFailure(R"([
            {"type": "drag", "target": "Knob1", "delta": [0, -50]}
        ])");
        
        expect(result.failed(), "Should reject delta as array");
    }
    
    //==============================================================================
    // Screenshot Parsing Tests
    //==============================================================================
    
    void testParseValidScreenshot()
    {
        beginTest("Screenshot: Valid");
        
        auto interactions = parseOrFail(R"([{"type": "screenshot", "id": "capture1"}])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.type == MouseInteraction::Type::Screenshot, "Should be Screenshot");
        expect(interactions[0].mouse.screenshotId == "capture1", "ID should match");
        expect(interactions[0].mouse.screenshotComponentId.isEmpty(), "Component crop should default to empty");
        expect(interactions[0].mouse.screenshotScale == 1.0f, "Default scale should be 1.0");
    }

    void testParseScreenshotComponent()
    {
        beginTest("Screenshot: Component crop and scale");

        auto interactions = parseOrFail(R"([{"type": "screenshot", "id": "capture1", "componentId": "Button1", "scale": 0.5}])");

        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.screenshotComponentId == "Button1", "Component ID should match");
        expect(interactions[0].mouse.screenshotScale == 0.5f, "Scale should match");
    }

    void testParseScreenshotDelay()
    {
        beginTest("Screenshot: Delay is parsed");

        auto interactions = parseOrFail(R"([{"type": "screenshot", "id": "capture1", "delay": 75}])");

        expectEquals(interactions[0].mouse.delayMs, 75, "Delay should match");
    }
    
    void testParseScreenshotMissingId()
    {
        beginTest("Screenshot: Missing id rejected");
        
        auto result = parseExpectingFailure(R"([{"type": "screenshot"}])");
        expect(result.failed(), "Should reject screenshot without id");
    }
    
    void testParseScreenshotInvalidScale()
    {
        beginTest("Screenshot: Invalid scale rejected");
        
        auto result = parseExpectingFailure(R"([
            {"type": "screenshot", "id": "test", "scale": 0.75}
        ])");
        
        expect(result.failed(), "Should reject scale other than 0.5 or 1.0");
    }

    void testParseScreenshotNegativeDelay()
    {
        beginTest("Screenshot: Negative delay rejected");

        auto result = parseExpectingFailure(R"([{"type": "screenshot", "id": "test", "delay": -1}])");
        expect(result.failed(), "Should reject a negative screenshot delay");
    }

    void testParseValidRepl()
    {
        beginTest("REPL: Valid with required fields and delay");

        auto interactions = parseOrFail("[{\"type\": \"repl\", \"id\": \"buttonValue\", "
            "\"expression\": \"Button1.getValue()\", \"delay\": 25}]");

        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.type == MouseInteraction::Type::Repl, "Should be REPL");
        expect(interactions[0].mouse.replId == "buttonValue", "ID should match");
        expect(interactions[0].mouse.replExpression == "Button1.getValue()", "Expression should match");
        expectEquals(interactions[0].mouse.delayMs, 25, "Delay should match");
    }

    void testParseReplMissingId()
    {
        beginTest("REPL: Missing id rejected");

        auto result = parseExpectingFailure(R"([{"type": "repl", "expression": "1 + 1"}])");
        expect(result.failed(), "Should reject REPL without id");
    }

    void testParseReplMissingExpression()
    {
        beginTest("REPL: Missing expression rejected");

        auto result = parseExpectingFailure(R"([{"type": "repl", "id": "result"}])");
        expect(result.failed(), "Should reject REPL without expression");
    }
    
    //==============================================================================
    // SelectMenuItem Parsing Tests
    //==============================================================================
    
    void testParseValidSelectMenuItem()
    {
        beginTest("SelectMenuItem: Valid");
        
        auto interactions = parseOrFail(R"([
            {"type": "selectMenuItem", "menuItemText": "Learn MIDI CC"}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.type == MouseInteraction::Type::SelectMenuItem, "Should be SelectMenuItem");
        expect(interactions[0].mouse.menuItemText == "Learn MIDI CC", "Text should match");
    }
    
    void testParseSelectMenuItemMissingText()
    {
        beginTest("SelectMenuItem: Missing text rejected");
        
        auto result = parseExpectingFailure(R"([{"type": "selectMenuItem"}])");
        expect(result.failed(), "Should reject selectMenuItem without text");
    }
    
    //==============================================================================
    // Position Struct Tests
    //==============================================================================
    
    void testPositionNormalized()
    {
        beginTest("Position: Normalized creation");
        
        auto pos = Position::normalized(0.3f, 0.7f);
        expect(!pos.isAbsolute(), "Should be normalized");
        expect(!pos.isCenter(), "Should not be center");
        
        auto val = pos.getValue();
        expectWithinAbsoluteError(val.x, 0.3f, 0.001f, "X should be 0.3");
        expectWithinAbsoluteError(val.y, 0.7f, 0.001f, "Y should be 0.7");
    }
    
    void testPositionAbsolute()
    {
        beginTest("Position: Absolute creation");
        
        auto pos = Position::absolute(50, 75);
        expect(pos.isAbsolute(), "Should be absolute");
        expect(!pos.isCenter(), "Should not be center");
    }
    
    void testPositionCenter()
    {
        beginTest("Position: Center creation");
        
        auto pos = Position::center();
        expect(!pos.isAbsolute(), "Should be normalized");
        expect(pos.isCenter(), "Should be center");
        
        auto val = pos.getValue();
        expectWithinAbsoluteError(val.x, 0.5f, 0.001f, "X should be 0.5");
        expectWithinAbsoluteError(val.y, 0.5f, 0.001f, "Y should be 0.5");
    }
    
    void testPositionGetPixelPosition()
    {
        beginTest("Position: getPixelPosition calculation");
        
        Rectangle<int> bounds{100, 200, 80, 60};  // x=100, y=200, w=80, h=60
        
        // Normalized position
        {
            auto pos = Position::normalized(0.5f, 0.5f);  // Center
            auto pixel = pos.getPixelPosition(bounds);
            expect(pixel.x == 140, "Normalized center X: 100 + 40 = 140");
            expect(pixel.y == 230, "Normalized center Y: 200 + 30 = 230");
        }
        
        // Absolute position
        {
            auto pos = Position::absolute(10, 20);
            auto pixel = pos.getPixelPosition(bounds);
            expect(pixel.x == 110, "Absolute X: 100 + 10 = 110");
            expect(pixel.y == 220, "Absolute Y: 200 + 20 = 220");
        }
    }
    
    //==============================================================================
    // Delay and Duration Tests
    //==============================================================================
    
    void testParseNegativeDelay()
    {
        beginTest("Range: Negative delay rejected");
        
        auto result = parseExpectingFailure(R"([
            {"type": "click", "target": "Button1", "delay": -100}
        ])");
        
        expect(result.failed(), "Should reject negative delay");
    }
    
    void testParseNegativeDuration()
    {
        beginTest("Range: Negative duration rejected");
        
        auto result = parseExpectingFailure(R"([
            {"type": "drag", "target": "Knob1", "delta": {"x": 0, "y": -50}, "duration": -50}
        ])");
        
        expect(result.failed(), "Should reject negative duration");
    }

    void testParseNonNumericDuration()
    {
        beginTest("Range: Non-numeric duration rejected");

        auto result = parseExpectingFailure(R"([
            {"type": "click", "target": "Button1", "duration": "100"}
        ])");

        expect(result.failed(), "Should reject a non-numeric duration");
    }
    
    void testParseDurationExceedsLimit()
    {
        beginTest("Range: Duration > 10s rejected");
        
        auto result = parseExpectingFailure(R"([
            {"type": "drag", "target": "Knob1", "delta": {"x": 0, "y": -50}, "duration": 15000}
        ])");
        
        expect(result.failed(), "Should reject duration > 10 seconds");
    }
    
    //==============================================================================
    // Default Values Tests
    //==============================================================================
    
    void testParseClickDefaults()
    {
        beginTest("Defaults: Click uses correct defaults");
        
        auto interactions = parseOrFail(R"([{"type": "click", "target": "Button1"}])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.durationMs == InteractionConstants::DefaultClickDurationMs,
               "Duration should be 30ms");
        expect(interactions[0].mouse.delayMs == 0, "Delay should be 0");
        expect(!interactions[0].mouse.rightClick, "Should not be right click");
        expect(interactions[0].mouse.position.isCenter(), "Position should be center");
    }
    
    void testParseDragDefaults()
    {
        beginTest("Defaults: Drag uses correct defaults");
        
        auto interactions = parseOrFail(R"([
            {"type": "drag", "target": "Knob1", "delta": {"x": 0, "y": -50}}
        ])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.durationMs == InteractionConstants::DefaultDragDurationMs, 
               "Duration should be 500ms");
    }
    
    void testParseMoveToDefaults()
    {
        beginTest("Defaults: MoveTo uses correct defaults");
        
        auto interactions = parseOrFail(R"([{"type": "moveTo", "target": "Button1"}])");
        
        expect(interactions.size() == 1, "Should have 1 interaction");
        expect(interactions[0].mouse.durationMs == InteractionConstants::DefaultMoveDurationMs, 
               "Duration should be 700ms");
    }
    
    //==============================================================================
    // Multiple Interactions Tests
    //==============================================================================
    
    void testParseMultipleInteractions()
    {
        beginTest("Sequence: Multiple interactions");
        
        auto interactions = parseOrFail(R"([
            {"type": "click", "target": "Button1"},
            {"type": "click", "target": "Button2", "delay": 200},
            {"type": "drag", "target": "Knob1", "delta": {"x": 0, "y": -50}}
        ])");
        
        expect(interactions.size() == 3, "Should have 3 interactions");
        expect(interactions[0].mouse.type == MouseInteraction::Type::Click, "First should be Click");
        expect(interactions[1].mouse.type == MouseInteraction::Type::Click, "Second should be Click");
        expect(interactions[2].mouse.type == MouseInteraction::Type::Drag, "Third should be Drag");
    }
    
    void testParseTooManyInteractions()
    {
        beginTest("Limit: Too many interactions rejected");
        
        // Build JSON array with 101 interactions (limit is 100)
        String json = "[";
        for (int i = 0; i < 101; i++)
        {
            if (i > 0) json += ",";
            json += R"({"type": "click", "target": "Button1"})";
        }
        json += "]";
        
        auto result = parseExpectingFailure(json);
        expect(result.failed(), "Should reject > 100 interactions");
    }
};

static InteractionParserTests interactionParserTests;

} // namespace hise
