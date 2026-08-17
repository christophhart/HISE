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

class InteractionDispatcherTests : public UnitTest
{
public:
    InteractionDispatcherTests() : UnitTest("Interaction Dispatcher Tests", "AI Tools") {}
    
    void runTest() override
    {
        // MoveTo execution
        testExecuteMoveTo();
        testExecuteMoveToInterpolates();
        testExecuteMoveToWithPixelPosition();
        
        // Click execution
        testExecuteClick();
        testExecuteClickRightClick();
        testExecuteClickWithModifiers();
        
        // Drag execution
        testExecuteDrag();
        testExecuteDragWithModifiers();
        testExecuteDragUpdatesPosition();
        
        // Screenshot
        testExecuteScreenshot();
        testExecuteScreenshotComponent();
        testExecuteScreenshotFailure();

        // REPL
        testExecuteRepl();
        testReplResultOrder();
        testReplFailureContinues();
        
        // SelectMenuItem
        testExecuteSelectMenuItem();
        testExecuteSelectMenuItemNoMenu();
        
        // Timing
        testDelayWorks();
        testTimedMoveAllowsMidpointScreenshot();
        testTimedClickAllowsPressedScreenshot();
        testTimedClickObservationsBracketRelease();
        testTimedDragAllowsMidpointScreenshot();
        testTimedMenuSelectionAllowsScreenshot();
        testTimedInteractionAllowsRepl();
        testTimedInteractionRejectsPointerEvent();
        testDelayedPointerWaitsForTimedCompletion();
        testExplicitZeroDurationRemainsBlocking();
        testTimedInteractionReleasesMouseOnFailure();
        
        // Component resolution
        testResolutionSuccess();
        testResolutionFailsForUnknownComponent();
        testResolutionFailsForHiddenComponent();
        
        // Execution log
        testExecutionLogContainsAllEvents();
    }
    
private:
    
    using Interaction = InteractionParser::Interaction;
    using MouseInteraction = InteractionParser::MouseInteraction;
    using Position = InteractionParser::MouseInteraction::Position;
    using ComponentTargetPath = InteractionParser::ComponentTargetPath;
    
    //==========================================================================
    // Helper methods
    //==========================================================================
    
    void setupDefaultMockComponents(TestExecutor& exec)
    {
        exec.addMockComponent("Button1", {100, 100, 80, 30}, true);
        exec.addMockComponent("Button2", {200, 100, 80, 30}, true);
        exec.addMockComponent("Knob1", {100, 150, 50, 50}, true);
        exec.addMockComponent("Panel1", {200, 150, 200, 200}, true);
    }
    
    Interaction makeMoveTo(const String& target, int delayMs = 0, int durationMs = 100)
    {
        Interaction i;
        i.mouse.type = MouseInteraction::Type::MoveTo;
        i.mouse.target = ComponentTargetPath(target);
        i.mouse.position = Position::center();
        i.mouse.delayMs = delayMs;
        i.mouse.durationMs = durationMs;
        return i;
    }
    
    Interaction makeMoveToPixel(const String& target, int x, int y, int durationMs = 100)
    {
        Interaction i;
        i.mouse.type = MouseInteraction::Type::MoveTo;
        i.mouse.target = ComponentTargetPath(target);
        i.mouse.position = Position::absolute(x, y);
        i.mouse.durationMs = durationMs;
        return i;
    }
    
    Interaction makeClick(int delayMs = 0)
    {
        Interaction i;
        i.mouse.type = MouseInteraction::Type::Click;
        i.mouse.delayMs = delayMs;
        i.mouse.durationMs = InteractionConstants::DefaultClickDurationMs;
        return i;
    }
    
    Interaction makeClickOnTarget(const String& target)
    {
        Interaction i;
        i.mouse.type = MouseInteraction::Type::Click;
        i.mouse.target = ComponentTargetPath(target);
        i.mouse.position = Position::center();
        i.mouse.durationMs = InteractionConstants::DefaultClickDurationMs;
        return i;
    }
    
    Interaction makeDrag(const String& target, int dx, int dy, int durationMs = 100)
    {
        Interaction i;
        i.mouse.type = MouseInteraction::Type::Drag;
        i.mouse.target = ComponentTargetPath(target);
        i.mouse.position = Position::center();
        i.mouse.deltaPixels = {dx, dy};
        i.mouse.durationMs = durationMs;
        return i;
    }
    
    Interaction makeScreenshot(const String& id, int delayMs = 0)
    {
        Interaction i;
        i.mouse.type = MouseInteraction::Type::Screenshot;
        i.mouse.screenshotId = id;
        i.mouse.delayMs = delayMs;
        return i;
    }

    Interaction makeRepl(const String& id, const String& expression)
    {
        Interaction i;
        i.mouse.type = MouseInteraction::Type::Repl;
        i.mouse.replId = id;
        i.mouse.replExpression = expression;
        return i;
    }
    
    Interaction makeSelectMenuItem(const String& text, int durationMs = 100)
    {
        Interaction i;
        i.mouse.type = MouseInteraction::Type::SelectMenuItem;
        i.mouse.menuItemText = text;
        i.mouse.durationMs = durationMs;
        return i;
    }
    
    //==========================================================================
    // MoveTo Tests
    //==========================================================================
    
    void testExecuteMoveTo()
    {
        beginTest("MoveTo: Basic execution");
        
        TestExecutor exec;
        setupDefaultMockComponents(exec);
        exec.cursorPosition = {0, 0};
        
        Array<Interaction> interactions;
        interactions.add(makeMoveTo("Button1"));
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);
        
        expect(result.result.wasOk(), "Execution should succeed");
        
        // Button1 is at (100, 100) with size 80x30
        // Center is (140, 115)
        expect(exec.cursorPosition.x == 140, "Cursor X should be at Button1 center: " + String(exec.cursorPosition.x));
        expect(exec.cursorPosition.y == 115, "Cursor Y should be at Button1 center: " + String(exec.cursorPosition.y));
    }
    
    void testExecuteMoveToInterpolates()
    {
        beginTest("MoveTo: Interpolates movement");
        
        TestExecutor exec;
        setupDefaultMockComponents(exec);
        exec.cursorPosition = {0, 0};
        
        Array<Interaction> interactions;
        interactions.add(makeMoveTo("Button1", 0, 100));  // 100ms duration
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        dispatcher.execute(interactions, exec, log);
        
        // Should have multiple mouseMove events
        int moveCount = 0;
        for (const auto& entry : exec.log)
        {
            if (entry.type == InteractionIds::mouseMove)
                moveCount++;
        }
        
        expect(moveCount > 1, "Should have multiple moves for interpolation, got: " + String(moveCount));
    }
    
    void testExecuteMoveToWithPixelPosition()
    {
        beginTest("MoveTo: With pixel position");
        
        TestExecutor exec;
        setupDefaultMockComponents(exec);
        exec.cursorPosition = {0, 0};
        
        Array<Interaction> interactions;
        interactions.add(makeMoveToPixel("Panel1", 50, 75));  // Panel at (200, 150)
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        dispatcher.execute(interactions, exec, log);
        
        // Panel1 at (200, 150) + pixel (50, 75) = (250, 225)
        expect(exec.cursorPosition.x == 250, "Cursor X should be 200 + 50 = 250: " + String(exec.cursorPosition.x));
        expect(exec.cursorPosition.y == 225, "Cursor Y should be 150 + 75 = 225: " + String(exec.cursorPosition.y));
    }
    
    //==========================================================================
    // Click Tests
    //==========================================================================
    
    void testExecuteClick()
    {
        beginTest("Click: Basic execution");
        
        TestExecutor exec;
        setupDefaultMockComponents(exec);
        exec.cursorPosition = {140, 115};  // At Button1 center
        
        Array<Interaction> interactions;
        interactions.add(makeClick());
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);
        
        expect(result.result.wasOk(), "Execution should succeed");
        
        // Should have mouseDown and mouseUp at cursor position
        bool hasDown = false, hasUp = false;
        for (const auto& entry : exec.log)
        {
            if (entry.type == InteractionIds::mouseDown)
            {
                hasDown = true;
                expect(entry.pixelPos.x == 140, "MouseDown X should be 140");
                expect(entry.pixelPos.y == 115, "MouseDown Y should be 115");
                expect(entry.mods.isLeftButtonDown(), "MouseDown should include the left button modifier");
            }
            if (entry.type == InteractionIds::mouseUp)
            {
                hasUp = true;
                expect(entry.pixelPos.x == 140, "MouseUp X should be 140");
                expect(entry.pixelPos.y == 115, "MouseUp Y should be 115");
                expect(!entry.mods.isAnyMouseButtonDown(), "MouseUp should not include a mouse button modifier");
            }
        }
        
        expect(hasDown, "Should have mouseDown");
        expect(hasUp, "Should have mouseUp");
        expect(exec.log.getLast().type == InteractionIds::mouseUp,
               "Post-click settling should not require another mouse move");
    }
    
    void testExecuteClickRightClick()
    {
        beginTest("Click: Right click");
        
        TestExecutor exec;
        exec.cursorPosition = {100, 100};
        
        Array<Interaction> interactions;
        Interaction click = makeClick();
        click.mouse.rightClick = true;
        interactions.add(click);
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        dispatcher.execute(interactions, exec, log);
        
        bool hasRightDown = false;
        for (const auto& entry : exec.log)
        {
            if (entry.type == InteractionIds::mouseDown && entry.rightClick)
                hasRightDown = true;
        }
        
        expect(hasRightDown, "Should have right mouseDown");
    }
    
    void testExecuteClickWithModifiers()
    {
        beginTest("Click: With modifiers");
        
        TestExecutor exec;
        exec.cursorPosition = {100, 100};
        
        Array<Interaction> interactions;
        Interaction click = makeClick();
        click.mouse.modifiers = ModifierKeys(ModifierKeys::shiftModifier | ModifierKeys::ctrlModifier);
        interactions.add(click);
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        dispatcher.execute(interactions, exec, log);
        
        bool hasDownModifiers = false;
        bool hasUpModifiers = false;
        for (const auto& entry : exec.log)
        {
            if (entry.type == InteractionIds::mouseDown)
            {
                hasDownModifiers = entry.mods.isShiftDown() && entry.mods.isCtrlDown()
                    && entry.mods.isLeftButtonDown();
            }
            else if (entry.type == InteractionIds::mouseUp)
            {
                hasUpModifiers = entry.mods.isShiftDown() && entry.mods.isCtrlDown()
                    && !entry.mods.isAnyMouseButtonDown();
            }
        }
        
        expect(hasDownModifiers, "MouseDown should retain keyboard and button modifiers");
        expect(hasUpModifiers, "MouseUp should retain keyboard modifiers without a mouse button");
    }
    
    //==========================================================================
    // Drag Tests
    //==========================================================================
    
    void testExecuteDrag()
    {
        beginTest("Drag: Basic execution");
        
        TestExecutor exec;
        setupDefaultMockComponents(exec);
        exec.cursorPosition = {125, 175};  // At Knob1 center
        
        Array<Interaction> interactions;
        interactions.add(makeDrag("Knob1", 0, -50));  // Drag up 50px
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);
        
        expect(result.result.wasOk(), "Execution should succeed");
        
        // Final position should be start + delta
        expect(exec.cursorPosition.y == 125, "Cursor Y should be 175 - 50 = 125: " + String(exec.cursorPosition.y));
    }
    
    void testExecuteDragWithModifiers()
    {
        beginTest("Drag: With shift modifier");
        
        TestExecutor exec;
        setupDefaultMockComponents(exec);
        exec.cursorPosition = {125, 175};
        
        Array<Interaction> interactions;
        Interaction drag = makeDrag("Knob1", 0, -50);
        drag.mouse.modifiers = ModifierKeys(ModifierKeys::shiftModifier);
        interactions.add(drag);
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        dispatcher.execute(interactions, exec, log);
        
        bool hasShift = false;
        for (const auto& entry : exec.log)
        {
            if (entry.type == InteractionIds::mouseDown && entry.mods.isShiftDown())
                hasShift = true;
        }
        
        expect(hasShift, "Should have shift modifier during drag");
    }
    
    void testExecuteDragUpdatesPosition()
    {
        beginTest("Drag: Updates cursor position");
        
        TestExecutor exec;
        setupDefaultMockComponents(exec);
        exec.cursorPosition = {100, 200};
        
        Array<Interaction> interactions;
        interactions.add(makeDrag("Panel1", 30, -40));
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        dispatcher.execute(interactions, exec, log);
        
        expect(exec.cursorPosition.x == 130, "Cursor X should be 100 + 30 = 130");
        expect(exec.cursorPosition.y == 160, "Cursor Y should be 200 - 40 = 160");
    }
    
    //==========================================================================
    // Screenshot Tests
    //==========================================================================
    
    void testExecuteScreenshot()
    {
        beginTest("Screenshot: Basic execution");
        
        TestExecutor exec;
        
        Array<Interaction> interactions;
        interactions.add(makeScreenshot("test_capture"));
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);
        
        expect(result.result.wasOk(), "Execution should succeed");
        
        bool hasScreenshot = false;
        for (const auto& entry : exec.log)
        {
            if (entry.type == InteractionIds::screenshot && entry.screenshotId == "test_capture")
                hasScreenshot = true;
        }
        
        expect(hasScreenshot, "Should have screenshot event");
    }

    void testExecuteScreenshotComponent()
    {
        beginTest("Screenshot: Component crop is forwarded");

        TestExecutor exec;
        Array<Interaction> interactions;
        auto screenshot = makeScreenshot("test_capture");
        screenshot.mouse.screenshotComponentId = "Button1";
        interactions.add(screenshot);

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Execution should succeed");
        expect(exec.log.size() == 1, "Should have one screenshot event");
        expect(exec.log[0].screenshotComponentId == "Button1",
               "Component ID should be forwarded");
    }

    void testExecuteScreenshotFailure()
    {
        beginTest("Screenshot: Capture failure fails E2E sequence");

        TestExecutor exec;
        exec.screenshotError = "Screenshot component not found: MissingButton";

        Array<Interaction> interactions;
        interactions.add(makeScreenshot("missing_component"));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.failed(), "Execution should fail");
        expect(result.result.getErrorMessage().contains("MissingButton"),
               "Failure should identify the missing component");
        expectEquals(result.interactionsCompleted, 0,
                     "Failed screenshot should not count as completed");
    }

    void testExecuteRepl()
    {
        beginTest("REPL: Result and execution log");

        TestExecutor exec;
        Array<Interaction> interactions;
        interactions.add(makeRepl("buttonValue", "Button1.getValue()"));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Execution should succeed");
        expectEquals(result.interactionsCompleted, 1, "REPL should count as completed");
        expect(dispatcher.getReplResults().size() == 1, "Should collect one REPL result");
        expect(dispatcher.getReplResults()[0][RestApiIds::id].toString() == "buttonValue",
               "Result ID should match");
        expect(exec.log[0].replExpression == "Button1.getValue()", "Expression should be forwarded");
    }

    void testReplFailureContinues()
    {
        beginTest("REPL: Failure does not stop sequence");

        TestExecutor exec;
        exec.replShouldFail = true;

        Array<Interaction> interactions;
        interactions.add(makeRepl("failure", "doesNotExist()"));
        interactions.add(makeScreenshot("after_failure"));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "REPL failure should not fail E2E execution");
        expectEquals(result.interactionsCompleted, 2, "Later interactions should execute");
        expect(!(bool)dispatcher.getReplResults()[0][RestApiIds::success],
               "REPL result should report failure");
        expect(exec.log.getLast().screenshotId == "after_failure",
               "Screenshot after failed REPL should execute");
    }

    void testReplResultOrder()
    {
        beginTest("REPL: Results preserve interaction order");

        TestExecutor exec;
        Array<Interaction> interactions;
        interactions.add(makeRepl("first", "1"));
        interactions.add(makeRepl("second", "2"));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);
        auto replResults = dispatcher.getReplResults();

        expect(result.result.wasOk(), "Execution should succeed");
        expectEquals(replResults.size(), 2, "Should collect both REPL results");
        expect(replResults[0][RestApiIds::id].toString() == "first", "First result should stay first");
        expect(replResults[1][RestApiIds::id].toString() == "second", "Second result should stay second");
    }
    
    //==========================================================================
    // SelectMenuItem Tests
    //==========================================================================
    
    void testExecuteSelectMenuItem()
    {
        beginTest("SelectMenuItem: Basic execution");
        
        TestExecutor exec;
        exec.cursorPosition = {100, 100};
        exec.addMockMenuItem("Option 1", 1, {50, 200, 150, 25});
        exec.addMockMenuItem("Option 2", 2, {50, 225, 150, 25});
        
        Array<Interaction> interactions;
        interactions.add(makeSelectMenuItem("Option 2"));
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);
        
        expect(result.result.wasOk(), "Execution should succeed");
        
        auto selected = dispatcher.getLastSelectedMenuItem();
        expect(selected.wasSelected, "Should have selected item");
        expect(selected.text == "Option 2", "Selected text should match");
        expect(selected.itemId == 2, "Selected ID should be 2");
    }
    
    void testExecuteSelectMenuItemNoMenu()
    {
        beginTest("SelectMenuItem: Fails when no menu open");
        
        TestExecutor exec;
        // No mock menu items added
        
        Array<Interaction> interactions;
        interactions.add(makeSelectMenuItem("Option 1"));
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);
        
        expect(result.result.failed(), "Should fail when no menu open");
        expect(result.result.getErrorMessage().containsIgnoreCase("menu"), 
               "Error should mention menu");
    }
    
    //==========================================================================
    // Timing Tests
    //==========================================================================
    
    void testDelayWorks()
    {
        beginTest("Timing: Delay is applied");
        
        TestExecutor exec;
        exec.cursorPosition = {100, 100};
        
        Array<Interaction> interactions;
        interactions.add(makeClick(50));  // 50ms delay
        
        auto startTime = Time::getMillisecondCounterHiRes();
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        dispatcher.execute(interactions, exec, log);
        
        auto elapsed = Time::getMillisecondCounterHiRes() - startTime;
        
        // Should have waited at least 50ms (with some tolerance)
        expect(elapsed >= 40, "Should wait for delay, elapsed: " + String(elapsed));
    }

    void testTimedMoveAllowsMidpointScreenshot()
    {
        beginTest("Timing: Explicit move allows a midpoint screenshot");

        TestExecutor exec;
        setupDefaultMockComponents(exec);
        auto move = makeMoveTo("Button1", 0, 120);
        move.mouse.durationWasExplicit = true;

        Array<Interaction> interactions;
        interactions.add(move);
        interactions.add(makeScreenshot("mid_move", 40));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Timed move sequence should succeed");
        expectEquals(result.interactionsCompleted, 2, "Both interactions should complete");

        int screenshotIndex = -1;
        for (int i = 0; i < exec.log.size(); ++i)
            if (exec.log[i].type == InteractionIds::screenshot)
                screenshotIndex = i;

        expect(screenshotIndex > 0, "Screenshot should occur after movement starts");
        if (screenshotIndex > 0)
        {
            int previousMoveIndex = screenshotIndex - 1;
            while (previousMoveIndex >= 0
                   && exec.log[previousMoveIndex].type != InteractionIds::mouseMove)
                previousMoveIndex--;

            expect(previousMoveIndex >= 0, "A mouse move should precede the screenshot");
            if (previousMoveIndex >= 0)
                expect(exec.log[previousMoveIndex].pixelPos.x > 0
                    && exec.log[previousMoveIndex].pixelPos.x < 140,
                    "Cursor should be between start and target at capture time");
        }
        expect(exec.cursorPosition == Point<int>(140, 115), "Timed move should finish at the target");
    }

    void testTimedClickAllowsPressedScreenshot()
    {
        beginTest("Timing: Explicit click allows a pressed screenshot");

        TestExecutor exec;
        exec.cursorPosition = {140, 115};
        auto click = makeClick();
        click.mouse.durationMs = 120;
        click.mouse.durationWasExplicit = true;

        Array<Interaction> interactions;
        interactions.add(click);
        interactions.add(makeScreenshot("pressed", 30));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Timed click sequence should succeed");
        expectEquals(result.interactionsCompleted, 2, "Both interactions should complete");

        int screenshotIndex = -1;
        int mouseUpIndex = -1;
        for (int i = 0; i < exec.log.size(); ++i)
        {
            if (exec.log[i].type == InteractionIds::screenshot)
                screenshotIndex = i;
            if (exec.log[i].type == InteractionIds::mouseUp)
                mouseUpIndex = i;
        }

        expect(screenshotIndex >= 0 && mouseUpIndex > screenshotIndex,
               "Screenshot should be captured before mouseUp");
    }

    void testTimedClickObservationsBracketRelease()
    {
        beginTest("Timing: Click observations run on the correct side of mouseUp");

        TestExecutor exec;
        exec.cursorPosition = {140, 115};
        auto click = makeClick();
        click.mouse.durationMs = 120;
        click.mouse.durationWasExplicit = true;

        auto pressedRepl = makeRepl("pressed", "currentlyClicked");
        auto releasedRepl = makeRepl("released", "currentlyClicked");

        Array<Interaction> interactions;
        interactions.add(click);
        interactions.add(makeScreenshot("pressed", 30));
        interactions.add(pressedRepl);
        interactions.add(makeScreenshot("released", 120));
        interactions.add(releasedRepl);

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Timed click observation sequence should succeed");

        int pressedScreenshotIndex = -1;
        int pressedReplIndex = -1;
        int mouseUpIndex = -1;
        int releasedScreenshotIndex = -1;
        int releasedReplIndex = -1;

        for (int i = 0; i < exec.log.size(); ++i)
        {
            const auto& entry = exec.log.getReference(i);

            if (entry.type == InteractionIds::mouseUp)
            {
                mouseUpIndex = i;
                expect(!entry.mods.isAnyMouseButtonDown(),
                       "Timed mouseUp should not include a mouse button modifier");
            }
            else if (entry.type == InteractionIds::screenshot && entry.screenshotId == "pressed")
                pressedScreenshotIndex = i;
            else if (entry.type == InteractionIds::screenshot && entry.screenshotId == "released")
                releasedScreenshotIndex = i;
            else if (entry.type == InteractionIds::repl && entry.replId == "pressed")
                pressedReplIndex = i;
            else if (entry.type == InteractionIds::repl && entry.replId == "released")
                releasedReplIndex = i;
        }

        expect(pressedScreenshotIndex >= 0 && pressedScreenshotIndex < mouseUpIndex,
               "Pressed screenshot should run before mouseUp");
        expect(pressedReplIndex >= 0 && pressedReplIndex < mouseUpIndex,
               "Pressed REPL should run before mouseUp");
        expect(releasedScreenshotIndex > mouseUpIndex,
               "Released screenshot should run after mouseUp");
        expect(releasedReplIndex > mouseUpIndex,
               "Released REPL should run after mouseUp");
    }

    void testTimedDragAllowsMidpointScreenshot()
    {
        beginTest("Timing: Explicit drag allows a midpoint screenshot");

        TestExecutor exec;
        exec.cursorPosition = {100, 100};
        auto drag = makeDrag("Panel1", 100, 0, 120);
        drag.mouse.durationWasExplicit = true;

        Array<Interaction> interactions;
        interactions.add(drag);
        interactions.add(makeScreenshot("mid_drag", 40));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Timed drag sequence should succeed");
        expectEquals(result.interactionsCompleted, 2, "Both interactions should complete");

        int screenshotIndex = -1;
        for (int i = 0; i < exec.log.size(); ++i)
            if (exec.log[i].type == InteractionIds::screenshot)
                screenshotIndex = i;

        expect(screenshotIndex > 0, "Screenshot should occur after drag movement starts");
        if (screenshotIndex > 0)
        {
            int previousMoveIndex = screenshotIndex - 1;
            while (previousMoveIndex >= 0
                   && exec.log[previousMoveIndex].type != InteractionIds::mouseMove)
                previousMoveIndex--;

            expect(previousMoveIndex >= 0, "A drag move should precede the screenshot");
            if (previousMoveIndex >= 0)
                expect(exec.log[previousMoveIndex].pixelPos.x > 100
                    && exec.log[previousMoveIndex].pixelPos.x < 200,
                    "Cursor should be between drag endpoints at capture time");
        }
        expect(exec.cursorPosition == Point<int>(200, 100), "Timed drag should finish at its endpoint");
    }

    void testTimedMenuSelectionAllowsScreenshot()
    {
        beginTest("Timing: Explicit menu selection allows a screenshot");

        TestExecutor exec;
        exec.addMockMenuItem("Option 1", 1, {100, 100, 100, 20});
        auto selection = makeSelectMenuItem("Option 1", 100);
        selection.mouse.durationWasExplicit = true;

        Array<Interaction> interactions;
        interactions.add(selection);
        interactions.add(makeScreenshot("menu_move", 30));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Timed menu selection should succeed");
        expectEquals(result.interactionsCompleted, 2, "Both interactions should complete");

        int screenshotIndex = -1;
        int mouseDownIndex = -1;
        for (int i = 0; i < exec.log.size(); ++i)
        {
            if (exec.log[i].type == InteractionIds::screenshot)
                screenshotIndex = i;
            if (exec.log[i].type == InteractionIds::mouseDown)
                mouseDownIndex = i;
        }

        expect(screenshotIndex >= 0 && mouseDownIndex > screenshotIndex,
               "Screenshot should occur before the menu item click");
        expect(dispatcher.getLastSelectedMenuItem().wasSelected,
               "Menu item should be selected when the timed interaction completes");

        int mouseUpIndex = -1;
        for (int i = mouseDownIndex + 1; i < exec.log.size(); ++i)
            if (exec.log[i].type == InteractionIds::mouseUp)
                mouseUpIndex = i;

        expect(mouseDownIndex >= 0 && mouseUpIndex > mouseDownIndex,
               "Timed menu selection should emit mouseDown before mouseUp");
        if (mouseDownIndex >= 0 && mouseUpIndex > mouseDownIndex)
            expect(exec.log[mouseUpIndex].elapsedMs - exec.log[mouseDownIndex].elapsedMs >= 15,
                   "Timed menu click should retain its 20ms hold");
    }

    void testTimedInteractionAllowsRepl()
    {
        beginTest("Timing: REPL is allowed during a timed interaction");

        TestExecutor exec;
        auto click = makeClick();
        click.mouse.durationMs = 80;
        click.mouse.durationWasExplicit = true;
        auto repl = makeRepl("pressedValue", "Button1.getValue()");
        repl.mouse.delayMs = 20;

        Array<Interaction> interactions;
        interactions.add(click);
        interactions.add(repl);

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "REPL should not conflict with a timed interaction");
        expectEquals(dispatcher.getReplResults().size(), 1, "REPL result should be collected");
    }

    void testTimedInteractionRejectsPointerEvent()
    {
        beginTest("Timing: Pointer event is rejected during a timed interaction");

        TestExecutor exec;
        setupDefaultMockComponents(exec);
        auto move = makeMoveTo("Button1", 0, 150);
        move.mouse.durationWasExplicit = true;
        auto conflictingMove = makeMoveTo("Button2", 20, 50);

        Array<Interaction> interactions;
        interactions.add(move);
        interactions.add(conflictingMove);

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.failed(), "Conflicting pointer event should fail");
        expect(result.result.getErrorMessage().contains("Only screenshot and repl"),
               "Error should identify the allowed interactions");
        expectEquals(result.interactionsCompleted, 0,
                     "Aborted timed interaction should not count as completed");
    }

    void testDelayedPointerWaitsForTimedCompletion()
    {
        beginTest("Timing: Pointer conflict is checked after its delay");

        TestExecutor exec;
        setupDefaultMockComponents(exec);
        auto firstMove = makeMoveTo("Button1", 0, 80);
        firstMove.mouse.durationWasExplicit = true;
        auto delayedMove = makeMoveTo("Button2", 100, 50);

        Array<Interaction> interactions;
        interactions.add(firstMove);
        interactions.add(delayedMove);

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Pointer interaction should run after the timed interaction completes");
        expect(exec.cursorPosition == Point<int>(240, 115), "Second move should reach Button2");
    }

    void testExplicitZeroDurationRemainsBlocking()
    {
        beginTest("Timing: Explicit zero duration remains blocking");

        TestExecutor exec;
        auto click = makeClick();
        click.mouse.durationMs = 0;
        click.mouse.durationWasExplicit = true;

        Array<Interaction> interactions;
        interactions.add(click);
        interactions.add(makeScreenshot("after_zero_click"));

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.wasOk(), "Zero-duration click should execute synchronously");

        int mouseUpIndex = -1;
        int screenshotIndex = -1;
        for (int i = 0; i < exec.log.size(); ++i)
        {
            if (exec.log[i].type == InteractionIds::mouseUp)
                mouseUpIndex = i;
            if (exec.log[i].type == InteractionIds::screenshot)
                screenshotIndex = i;
        }

        expect(mouseUpIndex >= 0 && screenshotIndex > mouseUpIndex,
               "MouseUp should occur before the following screenshot");
    }

    void testTimedInteractionReleasesMouseOnFailure()
    {
        beginTest("Timing: Failure releases a held mouse button");

        TestExecutor exec;
        auto click = makeClick();
        click.mouse.durationMs = 150;
        click.mouse.durationWasExplicit = true;
        auto conflictingClick = makeClick(20);

        Array<Interaction> interactions;
        interactions.add(click);
        interactions.add(conflictingClick);

        InteractionDispatcher dispatcher;
        Array<var> log;
        auto result = dispatcher.execute(interactions, exec, log);

        expect(result.result.failed(), "Sequence should fail on the conflicting click");
        expect(exec.log.size() >= 2, "Cleanup should emit mouseUp");
        expect(exec.log.getLast().type == InteractionIds::mouseUp,
               "Last executor event should release the mouse button");
        expect(!exec.log.getLast().mods.isAnyMouseButtonDown(),
               "Cleanup mouseUp should not include a mouse button modifier");
    }
    
    //==========================================================================
    // Component Resolution Tests
    //==========================================================================
    
    void testResolutionSuccess()
    {
        beginTest("Resolution: Finds component");
        
        TestExecutor exec;
        exec.addMockComponent("TestBtn", {50, 50, 100, 50}, true);
        
        auto result = exec.resolveTarget(ComponentTargetPath("TestBtn"));
        
        expect(result.success(), "Resolution should succeed");
        expect(result.componentBounds == Rectangle<int>(50, 50, 100, 50), "Bounds should match");
    }
    
    void testResolutionFailsForUnknownComponent()
    {
        beginTest("Resolution: Fails for unknown component");
        
        TestExecutor exec;
        
        auto result = exec.resolveTarget(ComponentTargetPath("NonExistent"));
        
        expect(!result.success(), "Resolution should fail");
        expect(result.error.containsIgnoreCase("not found"), "Error should mention not found");
    }
    
    void testResolutionFailsForHiddenComponent()
    {
        beginTest("Resolution: Fails for hidden component");
        
        TestExecutor exec;
        exec.addMockComponent("HiddenBtn", {50, 50, 100, 50}, false);  // visible = false
        
        auto result = exec.resolveTarget(ComponentTargetPath("HiddenBtn"));
        
        expect(!result.success(), "Resolution should fail");
        expect(result.error.containsIgnoreCase("not visible"), "Error should mention not visible");
    }
    
    //==========================================================================
    // Execution Log Tests
    //==========================================================================
    
    void testExecutionLogContainsAllEvents()
    {
        beginTest("Log: Contains all events");
        
        TestExecutor exec;
        setupDefaultMockComponents(exec);
        exec.cursorPosition = {140, 115};
        
        Array<Interaction> interactions;
        interactions.add(makeClick());
        interactions.add(makeScreenshot("snap"));
        
        InteractionDispatcher dispatcher;
        Array<var> log;
        dispatcher.execute(interactions, exec, log);
        
        // Should have: mouseDown, mouseUp, screenshot
        expect(log.size() >= 3, "Log should have at least 3 entries: " + String(log.size()));
        
        bool hasDown = false, hasUp = false, hasScreenshot = false;
        for (const auto& entry : log)
        {
            if (entry.isObject())
            {
                String type = entry.getProperty(RestApiIds::type, "").toString();
                if (type == InteractionIds::mouseDown.toString()) hasDown = true;
                if (type == InteractionIds::mouseUp.toString()) hasUp = true;
                if (type == InteractionIds::screenshot.toString()) hasScreenshot = true;
            }
        }
        
        expect(hasDown, "Log should contain mouseDown");
        expect(hasUp, "Log should contain mouseUp");
        expect(hasScreenshot, "Log should contain screenshot");
    }
};

static InteractionDispatcherTests interactionDispatcherTests;

} // namespace hise
