# HISE REST API for AI Agent Development

This document describes the REST API available in HISE for enabling AI agents to interact with script processors, enabling automated development workflows like editing scripts, compiling, and fixing errors in a tight feedback loop.

## Overview

- **Base URL**: `http://localhost:1900`
- **Response Format**: JSON with consistent structure
- **Common Fields**: All responses include `success` (boolean), `result` (object/string/null), `logs` (array), and `errors` (array)

### Response Structure

```json
{
  "success": true,
  "result": "...",
  "logs": ["console output line 1", "console output line 2"],
  "errors": [
    {
      "errorMessage": "Description of the error",
      "callstack": [
        "functionName() at Scripts/file.js:10:5",
        "onInit() at Interface.js:3:1"
      ]
    }
  ]
}
```

For HTTP error responses (`400`, `404`, `409`, `500`, etc.), the response body uses the same envelope with `success: false` and an `errors` array. Legacy `error` / `message` fields are no longer used.

### Related Documentation

When writing HISEScript code to pass to the scripting endpoints, refer to **[hisescript.md](../style/hisescript.md)** for language quirks and best practices (e.g., using `local` instead of `var` in inline functions, triggering callbacks with `changed()`).

---

## Quick Start

1. **Discover available API methods** (optional but useful):
   ```bash
   curl "http://localhost:1900/"
   ```

2. **Check if HISE is running** and discover the project structure:
   ```bash
   curl "http://localhost:1900/api/status"
   ```

3. **Get current script** from a processor:
   ```bash
   curl "http://localhost:1900/api/get_script?moduleId=Interface&callback=onInit"
   ```

4. **Modify and compile**:
   ```bash
   curl -X POST "http://localhost:1900/api/set_script" \
     -H "Content-Type: application/json" \
     -d '{"moduleId": "Interface", "callback": "onInit", "script": "Console.print(\"Hello\");", "compile": true}'
   ```

5. **Check for errors** in the response's `errors` array, fix, and repeat.

---

## API Reference

### GET /

List all available API methods with their parameters and descriptions. This is the self-documenting discovery endpoint.

**Parameters**: None

**Example Request**:
```bash
curl "http://localhost:1900/"
```

**Example Response**:
```json
{
  "success": true,
  "methods": [
    {
      "path": "/api/get_script",
      "method": "GET",
      "category": "scripting",
      "description": "Read script content from a processor",
      "returns": "Script content for the specified callback (or all callbacks merged)",
      "queryParameters": [
        { "name": "moduleId", "description": "The script processor's module ID", "required": true },
        { "name": "callback", "description": "Specific callback name (e.g., onInit). If omitted, returns all callbacks merged.", "required": false }
      ]
    },
    {
      "path": "/api/set_script",
      "method": "POST",
      "category": "scripting",
      "description": "Update script content and optionally compile",
      "returns": "Compilation result with success status, logs, and errors",
      "bodyParameters": [
        { "name": "moduleId", "description": "The script processor's module ID", "required": true },
        { "name": "callback", "description": "Specific callback to update. If omitted, script is treated as merged content.", "required": false },
        { "name": "script", "description": "The script content", "required": true },
        { "name": "compile", "description": "Whether to compile after setting", "required": false, "defaultValue": "true" }
      ]
    }
  ],
  "logs": [],
  "errors": []
}
```

**Key Fields**:
| Field | Description |
|-------|-------------|
| `methods[].path` | The endpoint path (e.g., `/api/status`) |
| `methods[].method` | HTTP method (`GET` or `POST`) |
| `methods[].category` | Category: `status`, `scripting`, or `ui` |
| `methods[].description` | One-line description of the endpoint |
| `methods[].returns` | One-line description of what the endpoint returns |
| `methods[].queryParameters` | For GET requests: array of query parameter definitions |
| `methods[].bodyParameters` | For POST requests: array of JSON body parameter definitions |

**Use cases**:
- Discover all available endpoints programmatically
- Generate MCP tool definitions automatically
- Keep agent instructions in sync with the API

---

### GET /api/status

Discover the project structure, available script processors, and their state.

**Parameters**: None

**Example Request**:
```bash
curl "http://localhost:1900/api/status"
```

**Example Response**:
```json
{
  "success": true,
  "server": {
    "version": "1.0.0"
  },
  "project": {
    "name": "My Project",
    "projectFolder": "D:/Projects/MyPlugin",
    "scriptsFolder": "D:/Projects/MyPlugin/Scripts"
  },
  "scriptProcessors": [
    {
      "moduleId": "Interface",
      "isMainInterface": true,
      "externalFiles": ["utils.js", "ui/components.js"],
      "callbacks": [
        { "id": "onInit", "empty": false },
        { "id": "onNoteOn", "empty": true },
        { "id": "onNoteOff", "empty": true },
        { "id": "onController", "empty": true },
        { "id": "onTimer", "empty": true },
        { "id": "onControl", "empty": false }
      ]
    },
    {
      "moduleId": "Script FX1",
      "isMainInterface": false,
      "externalFiles": [],
      "callbacks": [
        { "id": "onInit", "empty": true },
        { "id": "prepareToPlay", "empty": true },
        { "id": "processBlock", "empty": false },
        { "id": "onControl", "empty": true }
      ]
    }
  ],
  "logs": [],
  "errors": []
}
```

**Key Fields**:
| Field | Description |
|-------|-------------|
| `project.scriptsFolder` | Root folder for external `.js` files |
| `scriptProcessors[].moduleId` | Unique identifier for the processor (use in other API calls) |
| `scriptProcessors[].isMainInterface` | `true` if this processor renders the plugin UI |
| `scriptProcessors[].externalFiles` | List of `include()` files (relative to scriptsFolder) |
| `scriptProcessors[].callbacks[].id` | Callback name (e.g., `onInit`, `onNoteOn`) |
| `scriptProcessors[].callbacks[].empty` | `true` if callback has no content |

---

### GET /api/get_script

Read script content from a processor.

**Parameters**:
| Parameter | Required | Description |
|-----------|----------|-------------|
| `moduleId` | Yes | The processor's module ID |
| `callback` | No | Specific callback name. If omitted, returns all callbacks merged. |

**Example: Get single callback**
```bash
curl "http://localhost:1900/api/get_script?moduleId=Interface&callback=onInit"
```

**Response**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "callback": "onInit",
  "script": "Content.makeFrontInterface(600, 500);\nConsole.print(\"Hello\");",
  "logs": [],
  "errors": []
}
```

**Example: Get all callbacks (merged)**
```bash
curl "http://localhost:1900/api/get_script?moduleId=Interface"
```

**Response**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "script": "Content.makeFrontInterface(600, 500);\n\nfunction onNoteOn()\n{\n\t\n}\n\nfunction onNoteOff()\n{\n\t\n}\n...",
  "logs": [],
  "errors": []
}
```

**Important Notes**:
- `onInit` returns raw content (no function wrapper) since it's the top-level code
- Other callbacks return the full function signature: `function onNoteOn() { ... }`
- When getting merged script, all callbacks are concatenated with the standard function wrappers

---

### POST /api/set_script

Update script content in a processor.

**JSON Body** (all parameters in request body):
| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `moduleId` | Yes | - | The processor's module ID |
| `callback` | No | - | Specific callback to update. If omitted, `script` is treated as merged content. |
| `script` | Yes | - | The script content |
| `compile` | No | `true` | Whether to compile after setting |
| `forceSynchronousExecution` | No | `false` | Debug tool: Bypass threading model. **WARNING: May crash.** See [Advanced Debugging](#advanced-debugging-forcesynchronousexecution). |

**Example: Set single callback**
```bash
curl -X POST "http://localhost:1900/api/set_script" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "callback": "onInit",
    "script": "Content.makeFrontInterface(600, 500);\nConsole.print(123);",
    "compile": true
  }'
```

**Example: Set all callbacks (merged script)**
```bash
curl -X POST "http://localhost:1900/api/set_script" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "script": "Content.makeFrontInterface(600, 500);\n\nfunction onNoteOn()\n{\n\tMessage.makeArtificial();\n}\n\nfunction onNoteOff()\n{\n}\n\nfunction onController()\n{\n}\n\nfunction onTimer()\n{\n}\n\nfunction onControl(number, value)\n{\n}",
    "compile": true
  }'
```

**Example: Set without compiling**
```bash
curl -X POST "http://localhost:1900/api/set_script" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "callback": "onNoteOn",
    "script": "function onNoteOn()\n{\n\tConsole.print(Message.getNoteNumber());\n}",
    "compile": false
  }'
```

**Response (on successful compile)**:
```json
{
  "success": true,
  "result": "Recompiled OK",
  "externalFiles": ["utils.js", "ui.js"],
  "logs": ["123"],
  "errors": []
}
```

**Response (on compile error)**:
```json
{
  "success": false,
  "result": "Compilation / Runtime Error",
  "externalFiles": ["utils.js"],
  "logs": [],
  "errors": [
    {
      "errorMessage": "API call with undefined parameter 0",
      "callstack": [
        "myFunction() at Scripts/utils.js:15:8",
        "onInit() at Interface.js:3:1"
      ]
    }
  ]
}
```

**Note**: `externalFiles` lists all external `.js` files included by this processor (via `include()`). This list may change after compilation if `include()` calls were added or removed. Use this to track which files the LSP should watch for changes.

---

### POST /api/recompile

Recompile a processor without changing its script content.

**JSON Body**:
| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `moduleId` | Yes | - | The processor's module ID |
| `forceSynchronousExecution` | No | `false` | Debug tool: Bypass threading model. **WARNING: May crash.** See [Advanced Debugging](#advanced-debugging-forcesynchronousexecution). |

**Example Request**:
```bash
curl -X POST "http://localhost:1900/api/recompile" \
  -H "Content-Type: application/json" \
  -d '{"moduleId": "Interface"}'
```

**Response**: Same format as `set_script` compile response (includes `externalFiles` array).

**When to use**:
- After editing external `.js` files directly on disk
- To re-run initialization code without changes
- To check current compile state
- The `externalFiles` array in the response reflects the current include list (may have changed if `include()` calls were added/removed)

---

### POST /api/repl

Evaluate a HISEScript expression using the current script engine and return the result. The processor must have been compiled at least once.

**Parameters** (JSON body):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `moduleId` | Yes | — | The script processor's module ID |
| `expression` | Yes | — | The HISEScript expression to evaluate |

**Example Request**:
```bash
curl -X POST http://localhost:1900/api/repl \
  -H "Content-Type: application/json" \
  -d '{"moduleId": "Interface", "expression": "Engine.getSampleRate()"}'
```

**Response**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "result": "REPL Evaluation OK",
  "value": 44100,
  "logs": [],
  "errors": []
}
```

**Error Cases**:

| Status | Condition |
|--------|-----------|
| 400 | Missing `moduleId` or `expression` |
| 404 | Unknown module ID |
| 500 | Script engine not compiled yet |

---

### GET /api/list_components

List all UI components in a script processor.

**Parameters**:
| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `moduleId` | Yes | - | The processor's module ID |
| `hierarchy` | No | `false` | If `true`, returns nested tree with layout properties |

**Example: Flat list (default)**
```bash
curl "http://localhost:1900/api/list_components?moduleId=Interface"
```

**Response**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "components": [
    { "id": "Button1", "type": "ScriptButton" },
    { "id": "Panel1", "type": "ScriptPanel" },
    { "id": "Button2", "type": "ScriptButton" },
    { "id": "Knob1", "type": "ScriptSlider" }
  ],
  "logs": [],
  "errors": []
}
```

**Example: Hierarchical tree**
```bash
curl "http://localhost:1900/api/list_components?moduleId=Interface&hierarchy=true"
```

**Response**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "components": [
    { 
      "id": "Button1", 
      "type": "ScriptButton",
      "visible": true,
      "enabled": false,
      "x": 159.0,
      "y": 88.0,
      "width": 128,
      "height": 28,
      "childComponents": []
    },
    { 
      "id": "Panel1", 
      "type": "ScriptPanel",
      "visible": true,
      "enabled": true,
      "x": 162.0,
      "y": 197.0,
      "width": 360.0,
      "height": 280.0,
      "childComponents": [
        { 
          "id": "Button2", 
          "type": "ScriptButton",
          "visible": true,
          "enabled": true,
          "x": 74.0,
          "y": 45.0,
          "width": 128,
          "height": 28,
          "childComponents": []
        }
      ]
    }
  ],
  "logs": [],
  "errors": []
}
```

**Note**: Component positions (`x`, `y`) are relative to their parent component. Root components are positioned relative to the interface origin.

**Use cases**:
| Format | Best for |
|--------|----------|
| Flat list (default) | Finding component IDs, simple enumeration |
| Hierarchy (`hierarchy=true`) | Debugging visibility issues, understanding layout structure |

---

### GET /api/get_component_properties

Returns all properties for a specific UI component.

**Parameters**:
| Parameter | Required | Description |
|-----------|----------|-------------|
| `moduleId` | Yes | The processor's module ID |
| `id` | Yes | The component's ID (e.g., `"Button1"`, `"Panel1"`) |

**Example Request**:
```bash
curl "http://localhost:1900/api/get_component_properties?moduleId=Interface&id=Button1"
```

**Response**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "id": "Button1",
  "type": "ScriptButton",
  "properties": [
    { "id": "x", "value": 159, "isDefault": true },
    { "id": "y", "value": 88, "isDefault": true },
    { "id": "width", "value": 128, "isDefault": true },
    { "id": "height", "value": 28, "isDefault": true },
    { "id": "visible", "value": true, "isDefault": true },
    { "id": "enabled", "value": false, "isDefault": false },
    { "id": "saveInPreset", "value": true, "isDefault": true },
    { "id": "radioGroup", "value": 0, "isDefault": true },
    { "id": "bgColour", "value": "0x55FFFFFF", "isDefault": true },
    { "id": "itemColour", "value": "0xFD81D427", "isDefault": false },
    { "id": "textColour", "value": "0xFFFFFFFF", "isDefault": true },
    { 
      "id": "macroControl", 
      "value": -1, 
      "isDefault": true,
      "options": ["No MacroControl", "Macro 1", "Macro 2", "..."]
    }
  ],
  "logs": [],
  "errors": []
}
```

**Property fields**:
| Field | Description |
|-------|-------------|
| `id` | Property name (use this with `component.set(id, value)`) |
| `value` | Current value (native type: int, bool, string, or hex color string) |
| `isDefault` | `true` if value hasn't been explicitly changed from default |
| `options` | (optional) Available choices for enum-type properties |

**Use cases**:
- Discover correct property names (e.g., `bgColour` vs `colour`)
- Query component dimensions for layout calculations
- Verify property values after setting them
- Find available options for enum properties

---

### POST /api/set_component_properties

Set properties on one or more UI components, similar to editing in the Interface Designer. This is the recommended way to handle layout and positioning separately from script compilation.

**JSON Body**:
| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `moduleId` | Yes | - | The processor's module ID |
| `changes` | Yes | - | Array of `{id, properties: {...}}` objects |
| `force` | No | `false` | If true, bypasses script-lock check and sets all properties |

**Example: Set position of a single component**
```bash
curl -X POST "http://localhost:1900/api/set_component_properties" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "changes": [
      {
        "id": "Knob1",
        "properties": {
          "x": 100,
          "y": 50,
          "width": 80,
          "height": 80
        }
      }
    ]
  }'
```

**Example: Batch update multiple components**
```bash
curl -X POST "http://localhost:1900/api/set_component_properties" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "changes": [
      {
        "id": "Knob1",
        "properties": { "x": 100, "y": 50 }
      },
      {
        "id": "Knob2", 
        "properties": { "x": 200, "y": 50 }
      },
      {
        "id": "Label1",
        "properties": { "x": 100, "y": 140, "text": "Volume" }
      }
    ]
  }'
```

**Example: Set parent component (create hierarchy)**
```bash
curl -X POST "http://localhost:1900/api/set_component_properties" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "changes": [
      {
        "id": "ChildKnob",
        "properties": {
          "parentComponent": "ParentPanel",
          "x": 10,
          "y": 10
        }
      }
    ]
  }'
```

**Response (success)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "applied": [
    {
      "id": "Knob1",
      "properties": ["x", "y", "width", "height"]
    }
  ],
  "recompileRequired": false,
  "logs": [],
  "errors": []
}
```

**Response (properties locked by script)**:
```json
{
  "success": false,
  "errorMessage": "Properties are locked by script (use force=true to override)",
  "locked": [
    { "id": "Knob1", "property": "x" },
    { "id": "Knob1", "property": "y" }
  ],
  "logs": [],
  "errors": []
}
```

When a property is set in the script (e.g., `Knob1.set("x", 100);`), it becomes "locked" and cannot be changed via this endpoint without using `force: true`.

**Example: Force override script-controlled properties**
```bash
curl -X POST "http://localhost:1900/api/set_component_properties" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "changes": [
      {
        "id": "Knob1",
        "properties": { "x": 100, "y": 50 }
      }
    ],
    "force": true
  }'
```

**Key Fields**:
| Field | Description |
|-------|-------------|
| `applied` | Array of components and properties that were successfully set |
| `locked` | (Error only) Array of components/properties that are controlled by script |
| `recompileRequired` | `true` if `parentComponent` was changed - call `/api/recompile` to apply hierarchy changes |

**Important**: When `recompileRequired` is `true`, you must call `POST /api/recompile` for the `parentComponent` changes to take effect.

#### Recommended Workflow: Separating Script from Layout

For the best development experience, separate your script logic from layout/positioning:

**1. Script compilation step** - Create components without position parameters:

```javascript
// In onInit - create components WITHOUT position arguments
Content.makeFrontInterface(600, 500);

const var MainPanel = Content.addPanel("MainPanel");
const var VolumeKnob = Content.addKnob("VolumeKnob");
const var PanKnob = Content.addKnob("PanKnob");
const var TitleLabel = Content.addLabel("TitleLabel");

// Set up callbacks, paint routines, and logic here
VolumeKnob.setControlCallback(onVolumeChanged);
MainPanel.setPaintRoutine(function(g) {
    g.fillAll(Colours.darkgrey);
});
```

**2. Layout step** - Use `set_component_properties` for all positioning:

```bash
curl -X POST "http://localhost:1900/api/set_component_properties" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "changes": [
      {
        "id": "MainPanel",
        "properties": { "x": 10, "y": 10, "width": 580, "height": 480 }
      },
      {
        "id": "VolumeKnob",
        "properties": { "parentComponent": "MainPanel", "x": 50, "y": 100, "width": 100, "height": 100 }
      },
      {
        "id": "PanKnob",
        "properties": { "parentComponent": "MainPanel", "x": 200, "y": 100, "width": 100, "height": 100 }
      },
      {
        "id": "TitleLabel",
        "properties": { "parentComponent": "MainPanel", "x": 50, "y": 20, "width": 200, "height": 30, "text": "My Synth" }
      }
    ]
  }'
```

**Benefits of this approach**:
- **No position reset on recompile**: When the script is recompiled, positions are preserved because they're not set in the script
- **Layout iteration without recompilation**: Adjust positions, sizes, and hierarchy without triggering script recompilation
- **Clean separation**: Script handles logic and behavior; `set_component_properties` handles presentation
- **Batch efficiency**: Update many components in a single API call

---

### GET /api/get_component_value

Get the current runtime value of a UI component.

**Parameters**:
| Parameter | Required | Description |
|-----------|----------|-------------|
| `moduleId` | Yes | The processor's module ID |
| `id` | Yes | The component's ID (e.g., `"GainKnob"`, `"BypassButton"`) |

**Example Request**:
```bash
curl "http://localhost:1900/api/get_component_value?moduleId=Interface&id=GainKnob"
```

**Response**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "id": "GainKnob",
  "type": "ScriptSlider",
  "value": -3.5,
  "min": -12.0,
  "max": 12.0,
  "logs": [],
  "errors": []
}
```

**Key Fields**:
| Field | Description |
|-------|-------------|
| `value` | The current runtime value of the component |
| `min` | The minimum value for the component's range |
| `max` | The maximum value for the component's range |

**Use cases**:
- Verify component state during testing
- Read current knob/slider position
- Check button toggle state
- Get range information for validation

---

### POST /api/set_component_value

Set the runtime value of a UI component programmatically.

**JSON Body**:
| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `moduleId` | Yes | - | The processor's module ID |
| `id` | Yes | - | The component's ID |
| `value` | Yes | - | The value to set |
| `validateRange` | No | `false` | If `true`, validates value is within component's min/max range |
| `forceSynchronousExecution` | No | `false` | Debug tool: Bypass threading model. **WARNING: May crash.** See [Advanced Debugging](#advanced-debugging-forcesynchronousexecution). |

**Example Request**:
```bash
curl -X POST "http://localhost:1900/api/set_component_value" \
  -H "Content-Type: application/json" \
  -d '{"moduleId": "Interface", "id": "GainKnob", "value": 6.0}'
```

**Example with validation**:
```bash
curl -X POST "http://localhost:1900/api/set_component_value" \
  -H "Content-Type: application/json" \
  -d '{"moduleId": "Interface", "id": "GainKnob", "value": 6.0, "validateRange": true}'
```

**Response (success)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "id": "GainKnob",
  "type": "ScriptSlider",
  "logs": [],
  "errors": []
}
```

**Response (validation failure)**:
```json
{
  "success": false,
  "error": "Value 15.0 is out of range [-12.0, 12.0] for component GainKnob"
}
```

**Use cases**:
- Test UI state programmatically
- Simulate user adjusting a control
- Set initial values during automated testing
- Validate values before setting with `validateRange: true`

**Important: Callback Triggering**

Setting a component's value via this endpoint triggers the component's control callback, just as if the user had interacted with the control. Any `Console.print()` output from the callback will appear in the `logs` array of the response.

> **Note**: Callbacks also fire during compilation for `saveInPreset: true` components. See [Component Lifecycle & Callbacks](#component-lifecycle--callbacks) for details on when callbacks are triggered and how `saveInPreset` affects this behavior.

**Best Practice: Using `setControlCallback`**

Instead of relying on the global `onControl(component, value)` callback, assign a dedicated inline function to each component using `setControlCallback`. This is cleaner and allows you to reuse the same callback for multiple related components:

```javascript
// Get a reference to the UI component
const var MyButton = Content.getComponent("MyButton");

// Create an inline function with two parameters
// Important: must be inline because of realtime performance constraints!
inline function onMyButtonChanged(component, value)
{
    Console.print("Button changed to: " + value);
    
    // You can verify which component triggered this
    Console.assertWithMessage(component == MyButton, "points to the same object");
    Console.assertWithMessage(value == MyButton.getValue(), "value == getValue()");
}

// Assign this function to be executed whenever the component value changes
// Note: you can assign the same callback to multiple components and use
// component properties to distinguish them (e.g., for page buttons)
MyButton.setControlCallback(onMyButtonChanged);
```

---

### GET /api/testing/screenshot

Capture a screenshot of the interface or a specific component.

**Parameters**:
| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `moduleId` | No | `Interface` | The processor's module ID |
| `id` | No | - | Component ID to capture (omit for full interface) |
| `scale` | No | `1.0` | Scale factor (`0.5` or `1.0`) |
| `outputPath` | No | - | File path to save PNG (must end with `.png`). If provided, writes to file instead of returning Base64 |

**Example: Full interface screenshot**
```bash
curl "http://localhost:1900/api/testing/screenshot?moduleId=Interface"
```

**Example: Specific component screenshot**
```bash
curl "http://localhost:1900/api/testing/screenshot?moduleId=Interface&id=AmpPanel"
```

**Example: Half-scale screenshot**
```bash
curl "http://localhost:1900/api/testing/screenshot?moduleId=Interface&scale=0.5"
```

**Example: Save to file**
```bash
curl "http://localhost:1900/api/testing/screenshot?moduleId=Interface&outputPath=C:/temp/screenshot.png"
```

**Response (Base64 mode - default)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "id": "AmpPanel",
  "width": 320,
  "height": 310,
  "scale": 1.0,
  "imageData": "iVBORw0KGgoAAAANSUhEUgAA...",
  "logs": [],
  "errors": []
}
```

**Response (file output mode - when outputPath specified)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "id": "AmpPanel",
  "width": 320,
  "height": 310,
  "scale": 1.0,
  "filePath": "C:/temp/screenshot.png",
  "logs": [],
  "errors": []
}
```

**Key Fields**:
| Field | Description |
|-------|-------------|
| `width` | Width of the captured image in pixels |
| `height` | Height of the captured image in pixels |
| `scale` | The scale factor that was applied |
| `imageData` | Base64-encoded PNG image data (only when `outputPath` not specified) |
| `filePath` | Full path to saved PNG file (only when `outputPath` specified) |
| `id` | Component ID (only present if a specific component was captured) |

**Use cases**:
- Verify UI layout matches design mockups
- Automated visual regression testing
- AI-assisted UI development (capture → compare → refine)
- Documentation screenshots

**Notes**:
- When capturing a specific component, the screenshot is cropped to that component's bounds within the full interface render (preserving visual context like parent backgrounds)
- The `scale` parameter is useful for reducing payload size during iterative development (use `0.5` for quick comparisons, `1.0` for final verification)
- Returns error if component ID doesn't exist
- When using `outputPath`:
  - File path must end with `.png` (case-insensitive)
  - Parent directory must exist
  - Existing files are silently overwritten
  - Response contains `filePath` instead of `imageData`

---

### GET /api/get_selected_components

Get the currently selected UI components from HISE's Interface Designer. This enables AI-assisted workflows where the user selects components in the visual editor and asks the AI to perform operations on them.

**Parameters**:
| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `moduleId` | No | `Interface` | The processor's module ID |

**Example Request**:
```bash
curl "http://localhost:1900/api/get_selected_components?moduleId=Interface"
```

**Response (with components selected)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "selectionCount": 4,
  "components": [
    {
      "id": "Button1",
      "type": "ScriptButton",
      "properties": [
        { "id": "text", "value": "Button1", "isDefault": true },
        { "id": "x", "value": 162, "isDefault": true },
        { "id": "y", "value": 113, "isDefault": true },
        { "id": "width", "value": 128, "isDefault": true },
        { "id": "height", "value": 28, "isDefault": true },
        { "id": "visible", "value": true, "isDefault": true },
        { "id": "enabled", "value": true, "isDefault": true },
        { "id": "parentComponent", "value": "", "isDefault": true, "options": ["", "Panel1"] }
      ]
    },
    {
      "id": "Button2",
      "type": "ScriptButton",
      "properties": [...]
    }
  ],
  "logs": [],
  "errors": []
}
```

**Response (no selection)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "selectionCount": 0,
  "components": [],
  "logs": [],
  "errors": []
}
```

**Key Fields**:
| Field | Description |
|-------|-------------|
| `selectionCount` | Number of currently selected components |
| `components` | Array of selected components with full property details |
| `components[].id` | Component ID |
| `components[].type` | Component type (e.g., `ScriptButton`, `ScriptSlider`, `ScriptPanel`) |
| `components[].properties` | All properties with values, default status, and options (same format as `get_component_properties`) |

**Notes**:
- Components are returned in **selection order** (the order the user clicked them)
- All properties are included for each component, enabling complex operations without additional API calls
- An empty selection returns `success: true` with an empty `components` array (not an error)
- Selection is cleared when the script is recompiled

**Use cases**:
- Bulk layout operations: "Align these 8 buttons horizontally"
- Batch property changes: "Make all selected buttons the same size"
- Code generation: "Add control callbacks to all selected components"
- Parent/child relationships: "Put all selected buttons inside this panel"

---

### POST /api/testing/e2e

Execute a sequence of UI interactions (mouse movements, clicks, drags, screenshots) in a dedicated test window. Auto-inserts `moveTo` events as needed for proper mouse positioning. Mouse state persists across API calls for sequential interaction workflows.

**Parameters** (JSON body):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `interactions` | Yes | — | Array of interaction objects (see below) |
| `verbose` | No | `false` | Include auto-insertion details and final mouse state in response |

**Interaction types**: `moveTo`, `click`, `doubleClick`, `drag`, `selectMenuItem`, `screenshot`

Each interaction targets a component by ID and supports optional timing parameters (`delayMs`, `durationMs`).

**Example Request**:
```bash
curl -X POST http://localhost:1900/api/testing/e2e \
  -H "Content-Type: application/json" \
  -d '{
    "interactions": [
      {"type": "click", "target": "Button1"},
      {"type": "screenshot", "id": "after_click"}
    ]
  }'
```

**Response**:
```json
{
  "success": true,
  "interactionsCompleted": 2,
  "totalElapsedMs": 120,
  "executionLog": [...],
  "screenshots": {"after_click": {"sizeKB": 12.5, "width": 600, "height": 400}},
  "logs": [],
  "errors": []
}
```

**Notes**:
- This endpoint blocks until all interactions complete (30s timeout)
- Mouse state persists across calls — subsequent calls continue from the last cursor position
- The test window is created when the REST server starts and re-created if closed

---

### POST /api/diagnose_script

Run a diagnostic-only shadow parse on an external `.js` script file. Returns structured diagnostics (API hallucinations, type mismatches, language rule violations, audio-thread safety warnings) **without modifying runtime state** — no recompilation, no execution.

Requires at least one prior successful compile (F5 or `/api/set_script` with `compile: true`) so that API objects are resolved.

**Parameters** (JSON body — at least one required):

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `moduleId` | string | conditional | The script processor's module ID. If `filePath` is omitted, uses the processor's first external file. |
| `filePath` | string | conditional | Path to the `.js` file (absolute, or relative to Scripts folder). If `moduleId` is omitted, HISE resolves the owning processor automatically. |

**Request**:
```json
{
  "moduleId": "Interface",
  "filePath": "Scripts/UI.js"
}
```

**Response (clean file)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "filePath": "D:/Projects/MyPlugin/Scripts/UI.js",
  "diagnostics": [],
  "logs": [],
  "errors": []
}
```

**Response (with diagnostics)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "filePath": "D:/Projects/MyPlugin/Scripts/UI.js",
  "diagnostics": [
    {
      "line": 15,
      "column": 5,
      "severity": "error",
      "source": "api-validation",
      "message": "Function / constant not found: Console.prnt (did you mean: print?)",
      "suggestions": ["print"]
    },
    {
      "line": 23,
      "column": 8,
      "severity": "error",
      "source": "type-check",
      "message": "Message.setNoteNumber() parameter 1 expects int, got String"
    },
    {
      "line": 31,
      "column": 3,
      "severity": "warning",
      "source": "callscope",
      "message": "[CallScope] cable.sendData (unsafe) in audio-thread context"
    }
  ],
  "logs": [],
  "errors": []
}
```

**Diagnostic Fields**:

| Field | Type | Description |
|-------|------|-------------|
| `line` | int | 1-based line number in the file |
| `column` | int | 1-based column number |
| `severity` | string | `"error"`, `"warning"`, `"info"`, or `"hint"` |
| `source` | string | Diagnostic category (see table below) |
| `message` | string | Human-readable diagnostic message |
| `suggestions` | string[] | Optional "did you mean?" suggestions from fuzzy matching |

**Source Categories**:

| Source | What it catches |
|--------|----------------|
| `syntax` | Parse errors, preprocessor errors |
| `api-validation` | Unknown methods/constants, wrong argument count |
| `type-check` | Literal type mismatches (String where int expected, etc.) |
| `language` | HISEScript rule violations (const var in function, nested inline, undefined literal) |
| `callscope` | Unsafe API calls in audio-thread callbacks |

**Error Cases**:

| Status | Condition |
|--------|-----------|
| 400 | Neither `moduleId` nor `filePath` provided |
| 400 | `moduleId` provided but processor has no external files and `filePath` is omitted |
| 404 | Unknown `moduleId` |
| 404 | `filePath` not found on disk |
| 404 | No processor includes the specified file (never compiled) |

**Notes**:
- `success: true` means the shadow parse completed — diagnostics are in the `diagnostics` array, not in `errors`
- The `errors` array is for REST-level errors (captured console output), not diagnostics
- Always reads the file from disk — save pending edits before calling
- The shadow parse takes ~50ms (audio is briefly suspended via `killVoicesAndCall`)
- Does not modify any runtime state — safe to call repeatedly during editing

---

### GET /api/get_included_files

List all currently included (watched) external script files. Useful for LSP auto-suggestion, debugging include issues, and verifying file associations.

**Parameters** (query string — all optional):

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `moduleId` | string | no | Filter by script processor. If omitted, returns files from all processors. |

**Request (global — all processors)**:
```
GET /api/get_included_files
```

**Response (global)**:
```json
{
  "success": true,
  "files": [
    { "path": "D:/Projects/MyPlugin/Scripts/utils.js", "processor": "Interface" },
    { "path": "D:/Projects/MyPlugin/Scripts/ui.js", "processor": "Interface" }
  ],
  "logs": [],
  "errors": []
}
```

**Request (filtered by processor)**:
```
GET /api/get_included_files?moduleId=Interface
```

**Response (filtered)**:
```json
{
  "success": true,
  "moduleId": "Interface",
  "files": [
    "D:/Projects/MyPlugin/Scripts/utils.js",
    "D:/Projects/MyPlugin/Scripts/ui.js"
  ],
  "logs": [],
  "errors": []
}
```

**Notes**:
- Without `moduleId`: returns objects with `path` and `processor` fields (de-duplicated across all processors)
- With `moduleId`: returns flat string array of full paths (since processor is already known)
- Paths use forward slashes on all platforms
- Files are registered after a successful compile — call after F5 or `/api/set_script` with `compile: true`

**Error Cases**:

| Status | Condition |
|--------|-----------|
| 404 | Unknown `moduleId` |

---

### POST /api/testing/profile

Start a profiling session or retrieve the last profiling result. Requires HISE to be built with `HISE_INCLUDE_PROFILING_TOOLKIT`.

**Parameters** (JSON body):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `mode` | No | `"record"` | `"record"` to start a new session, `"get"` to retrieve results |
| `durationMs` | No | `1000` | Recording duration in ms (100–5000), record mode only |
| `threadFilter` | No | all | Array of thread names: `"Audio Thread"`, `"Scripting Thread"`, `"UI Thread"`, `"Loading Thread"` |
| `eventFilter` | No | all | Array of event types: `"DSP"`, `"Script"`, `"Lock"`, `"Callback"`, `"Trace"`, `"TimerCallback"`, `"Scriptnode"` |
| `summary` | No | `false` | Aggregate repeated events with count/median/peak stats (get mode) |
| `filter` | No | — | Wildcard pattern for event names (get mode) |
| `minDuration` | No | `0` | Only include events with duration >= this value in ms (get mode) |
| `nested` | No | `false` | Include children of matched events (get mode) |
| `limit` | No | `15` | Max results 1–100 (get mode) |
| `wait` | No | `true` | If `false`, return immediately when recording is in progress (get mode) |

**Example — Record**:
```bash
curl -X POST http://localhost:1900/api/testing/profile \
  -H "Content-Type: application/json" \
  -d '{"mode": "record", "durationMs": 2000}'
```

**Example — Get with filtering**:
```bash
curl -X POST http://localhost:1900/api/testing/profile \
  -H "Content-Type: application/json" \
  -d '{"mode": "get", "summary": true, "threadFilter": ["Audio Thread"], "minDuration": 0.1}'
```

**Notes**:
- Record mode is non-blocking — returns immediately with `{success: true, recording: true}`
- Returns 409 if a recording session is already in progress
- Returns 400 if profiling toolkit is not enabled in the build

---

### POST /api/parse_css

Parse CSS code and return structured diagnostics. Optionally resolves properties for a set of selectors using CSS specificity rules.

**Parameters** (JSON body):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `code` | Conditional | — | CSS code string to parse (required if `filePath` not given) |
| `filePath` | Conditional | — | Path to a `.css` file (relative to Scripts/ or absolute) |
| `selectors` | No | — | Array of selector strings to resolve properties for |
| `width` | No | — | Reference width in pixels for resolving relative units |
| `height` | No | — | Reference height in pixels for resolving relative units |

**Example Request**:
```bash
curl -X POST http://localhost:1900/api/parse_css \
  -H "Content-Type: application/json" \
  -d '{"code": ".myClass { background: red; padding: 10px; }", "selectors": [".myClass"]}'
```

**Response**:
```json
{
  "success": true,
  "diagnostics": [],
  "selectors": [".myClass"],
  "properties": {"background": "red", "padding": "10px"},
  "logs": [],
  "errors": []
}
```

**Error Cases**:

| Status | Condition |
|--------|-----------|
| 400 | Neither `code` nor `filePath` provided |
| 404 | File not found at `filePath` |

---

### POST /api/shutdown

Gracefully quit the HISE application.

**Parameters**: None

**Example Request**:
```bash
curl -X POST http://localhost:1900/api/shutdown \
  -H "Content-Type: application/json" \
  -d '{}'
```

**Response**:
```json
{
  "success": true,
  "result": "Shutdown initiated",
  "logs": [],
  "errors": []
}
```

**Notes**:
- The HTTP response is sent before the application exits
- The shutdown is scheduled asynchronously via `JUCEApplication::quit()`

---

## Builder And Undo API

These endpoints provide transactional module tree editing with grouped execution, undo/redo, and diff/history inspection.

### GET /api/builder/tree

Return the runtime module tree, or the active validation tree when `group=current`.

**Query Parameters**:

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `moduleId` | No | root | Optional subtree root. In runtime mode, this resolves from live processors. In group mode, this resolves inside the plan tree. |
| `group` | No | - | Optional group selector. Only `current` is supported. |
| `queryParameters` | No | `true` | Include parameter lists in each node (`1` / `0`). |
| `verbose` | No | `false` | Include extended metadata fields in each node (`1` / `0`). |

**Behavior**:

- No `group` parameter -> runtime tree.
- `group=current` -> current validation tree.
- `group=current` with no active group -> `400`.
- `group` with any value other than `current` -> `501`.
- `moduleId` not found in selected tree -> `404`.

**Example (runtime tree)**:

```bash
curl "http://localhost:1900/api/builder/tree"
```

**Example (current group tree)**:

```bash
curl "http://localhost:1900/api/builder/tree?group=current"
```

### POST /api/builder/apply

Apply one or more module-tree operations in a single request.

Returns success / validation / runtime status with `result`, `logs`, and `errors`.

**JSON Body**:

| Field | Required | Description |
|-------|----------|-------------|
| `operations` | Yes | Non-empty array of operation objects. |

Supported operations:

- `add` -> `type`, `parent`, `chain`, `name`
- `remove` -> `target`
- `clone` -> `source`, `count`, optional `template`
- `set_attributes` -> `target`, `attributes`, optional `mode` (`value`/`normalized`/`raw`)
- `set_id` -> `target`, `name`
- `set_bypassed` -> `target`, `bypassed`
- `set_effect` -> `target`, `effect`
- `set_complex_data` -> reserved/deferred

**Example**:

```bash
curl -X POST "http://localhost:1900/api/builder/apply" \
  -H "Content-Type: application/json" \
  -d '{
    "operations": [
      { "op": "add", "type": "SineSynth", "parent": "Master Chain", "chain": -1, "name": "MySine" },
      { "op": "set_bypassed", "target": "MySine", "bypassed": true }
    ]
  }'
```

**Response**:

The `result` field contains a diff summary showing the net effect of all operations. This is the same format as `GET /api/undo/diff?flatten=true`.

```json
{
  "success": true,
  "result": {
    "scope": "group",
    "groupName": "root",
    "diff": [
      { "target": "MySine", "action": "+", "domain": "builder" }
    ]
  },
  "logs": [],
  "errors": []
}
```

When called inside an undo group (after `push_group`), the diff accumulates all operations in the group so far. The `scope` is `"group"` and `groupName` reflects the group name passed to `push_group`.

**Result fields**:

| Field | Description |
|-------|-------------|
| `scope` | `"group"` when inside a group, `"root"` at root level (currently always returns `"group"`) |
| `groupName` | Name of the current group (or `"root"` at root level) |
| `diff` | Array of diff entries representing the net effect |

**Diff entry fields**:

| Field | Description |
|-------|-------------|
| `target` | Module ID affected |
| `action` | `"+"` (added), `"-"` (removed), `"*"` (modified). Structural actions take priority - if a module was added and then modified, it shows as `"+"`. |
| `domain` | Operation domain (e.g., `"builder"`) |

### POST /api/builder/reset

Reset the module tree to an empty state (equivalent to File -> New). Clears all undo history.

**Parameters**: None

**Example Request**:
```bash
curl -X POST http://localhost:1900/api/builder/reset \
  -H "Content-Type: application/json" \
  -d '{}'
```

**Response**:
```json
{
  "success": true,
  "result": "Module tree reset",
  "logs": [],
  "errors": []
}
```

### POST /api/undo/push_group

Start a new group. Subsequent builder operations are validated against group state and queued until pop.

**Body**: `{ "name": "My Group" }`

### POST /api/undo/pop_group

End current group.

- `cancel=false` (default): execute queued actions as one undoable unit.
- `cancel=true`: discard queued actions.

**Body**:

```json
{ "cancel": false }
```

### POST /api/undo/back

Undo one action/group boundary.

- If there is nothing to undo: `400` with `errors[0].errorMessage = "nothing to undo"`.

### POST /api/undo/forward

Redo one action/group boundary.

- If there is nothing to redo: `400` with `errors[0].errorMessage = "nothing to redo"`.

### GET /api/undo/diff

Return active diff list.

**Query Parameters**:

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `scope` | No | `group` | `group` or `root` |
| `domain` | No | all | Domain filter (eg `builder`) |
| `flatten` | No | `false` | Merge cancelling actions into net effect |

### GET /api/undo/history

Return history list and cursor position.

**Query Parameters**: same as `/api/undo/diff` (`scope`, `domain`, `flatten`).

### POST /api/undo/clear

Clear all undo history and active groups.

---

## Wizard API

These endpoints drive the wizard system for project creation, plugin export, and other multi-step tasks.

### GET /api/wizard/initialise

Fetch pre-populated field defaults for a wizard form.

**Parameters** (query string):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `id` | Yes | — | Wizard ID: `new_project`, `recompile`, `plugin_export`, `compile_networks`, `audio_export`, `install_package_maker` |

**Example Request**:
```bash
curl "http://localhost:1900/api/wizard/initialise?id=new_project"
```

**Response**: Flat key/value object of field defaults for the wizard form.

**Error Cases**:

| Status | Condition |
|--------|-----------|
| 400 | Missing or unknown wizard ID |

### POST /api/wizard/execute

Execute a wizard task. Synchronous tasks return immediately; asynchronous tasks return a job ID for polling.

**Parameters** (JSON body):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `wizardId` | Yes | — | Wizard ID string |
| `answers` | Yes | — | Key/value object of all form field answers |
| `tasks` | Yes | — | Array with exactly one task function name to execute |

**Example Request**:
```bash
curl -X POST http://localhost:1900/api/wizard/execute \
  -H "Content-Type: application/json" \
  -d '{"wizardId": "new_project", "answers": {"projectName": "MyPlugin"}, "tasks": ["createProject"]}'
```

**Response (sync)**: Result string with logs.

**Response (async)**: `{"jobId": "...", "async": true}` — poll with `/api/wizard/status`.

**Error Cases**:

| Status | Condition |
|--------|-----------|
| 400 | Missing/invalid `wizardId`, `answers`, or `tasks` |
| 400 | Unknown wizard ID or invalid task for wizard |

### GET /api/wizard/status

Poll progress of a long-running asynchronous wizard job.

**Parameters** (query string):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `jobId` | Yes | — | Job ID returned by `/api/wizard/execute` |

**Response**: `{"finished": true/false, "progress": 0.0-1.0}`

**Error Cases**:

| Status | Condition |
|--------|-----------|
| 400 | Missing `jobId` |
| 404 | No active job with that ID |

---

## UI Component Tree API

These endpoints provide batched UI component editing with undo support, similar to the Builder API for the module tree.

### GET /api/ui/tree

Get the UI component tree hierarchy for a script processor's interface.

**Parameters** (query string):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `moduleId` | Yes | — | Script processor ID (e.g., `"Interface"`) |
| `group` | No | — | `"current"` returns the active undo group's validation tree |

**Example Request**:
```bash
curl "http://localhost:1900/api/ui/tree?moduleId=Interface"
```

**Response**:
```json
{
  "success": true,
  "result": {
    "id": "Content",
    "type": "ScriptPanel",
    "x": 0, "y": 0, "width": 600, "height": 500,
    "visible": true, "enabled": true, "saveInPreset": false,
    "childComponents": [
      {
        "id": "Button1",
        "type": "ScriptButton",
        "x": 10, "y": 10, "width": 128, "height": 28,
        "visible": true, "enabled": true, "saveInPreset": true,
        "childComponents": []
      }
    ]
  },
  "logs": [],
  "errors": []
}
```

### POST /api/ui/apply

Apply batched UI component operations (add, remove, set, move, rename) with undo support.

**Parameters** (JSON body):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `moduleId` | Yes | — | Script processor ID |
| `operations` | Yes | — | Array of operation objects (see below) |

**Operation types**:
- `add` — `{op: "add", componentType, id?, parentId?, x?, y?, width?, height?}`
- `remove` — `{op: "remove", target}`
- `set` — `{op: "set", target, properties, force?}`
- `move` — `{op: "move", target, parent, index?, keepPosition?}`
- `rename` — `{op: "rename", target, newId}`

**Example Request**:
```bash
curl -X POST http://localhost:1900/api/ui/apply \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "operations": [
      {"op": "add", "componentType": "ScriptButton", "id": "MyButton", "x": 10, "y": 10, "width": 100, "height": 30}
    ]
  }'
```

**Response**: Diff of changes with `domain="ui"`, action symbols (`+`/`-`/`*`), and target component IDs.

**Notes**:
- Operations are pre-validated before execution; invalid operations return 400 with detailed error
- All operations are undoable via the undo API
- Component types: `ScriptButton`, `ScriptSlider`, `ScriptPanel`, `ScriptComboBox`, `ScriptLabel`, `ScriptTable`, `ScriptImage`, etc.

---

### POST /api/testing/sequence

Inject MIDI messages and test events into HISE with precise timing. Non-blocking by default: queues messages and returns immediately. A background `HighResolutionTimer` dispatches events at the correct timestamps with sub-ms precision. Notes automatically send note-off after their `duration` to prevent stuck notes.

Multiple calls merge into the pending queue. Send `allNotesOff` as a panic button to clear everything.

**Parameters** (JSON body):

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `messages` | Yes | — | Array of event objects (see below) |
| `blocking` | No | `false` | If `true`, wait for the entire sequence to complete before returning. Useful for test scenarios where you want all REPL results in one response. 30s timeout. |
| `recordOutput` | No | — | File path to record audio output as WAV. Recording duration is auto-computed from the sequence. Implies `blocking: true`. The response includes `recordOutput` with the file path on success. |

**Message types**:

| Field | note | cc | pitchbend | allNotesOff | repl | set_attribute | testsignal |
|-------|------|----|-----------|-------------|------|---------------|------------|
| `type` | required | required | required | required | required | required | required |
| `timestamp` | optional (0) | optional (0) | optional (0) | optional (0) | optional (0) | optional (0) | optional (0) |
| `channel` | optional (1, range 1-16) | optional (1) | optional (1) | — | — | — | — |
| `noteNumber` | required (0-127) | — | — | — | — | — | — |
| `velocity` | optional (0.0-1.0, default 1.0) | — | — | — | — | — | — |
| `duration` | optional (default 500ms) | — | — | — | — | — | optional (default 500ms) |
| `controller` | — | required (0-127) | — | — | — | — | — |
| `value` | — | required (0-127) | required (0-16383) | — | — | required | — |
| `expression` | — | — | — | — | required | — | — |
| `id` | — | — | — | — | optional | — | — |
| `moduleId` | — | — | — | — | optional ("Interface") | — | — |
| `processorId` | — | — | — | — | — | optional ("Interface") | — |
| `parameterId` | — | — | — | — | — | required | — |
| `signal` | — | — | — | — | — | — | required |
| `frequency` | — | — | — | — | — | — | optional (440) |
| `startFrequency` | — | — | — | — | — | — | optional (20) |
| `endFrequency` | — | — | — | — | — | — | optional (20000) |

- **`timestamp`**: Absolute time offset in ms from the start of the sequence. Use `0` for immediate dispatch. Multiple messages with the same timestamp fire simultaneously (chords).
- **`duration`**: Time in ms before automatic note-off (for `note`) or signal length (for `testsignal`).
- **`allNotesOff`**: Fires note-off for all active notes, calls `MainController::allNotesOff()`, stops any playing test signal, and clears the entire remaining queue.
- **`repl`**: Evaluates a HISEScript expression at the given timestamp with precise timing. Results are collected in the `replResults` array on the next response (or in the same response when `blocking: true`). An optional `id` field lets you tag results for identification.
- **`testsignal`**: Injects a predefined test signal into the audio stream via `MainController::setBufferToPlay()`. The signal is processed through the entire synth chain including master FX. Available signals: `sine`, `saw`, `sweep` (log sine sweep), `dirac` (impulse), `noise` (white), `silence`. Generated signals are cached for reuse across calls.
- **`set_attribute`**: Sets a processor parameter at the given timestamp. The `processorId` identifies the target processor (any module in the tree), and `parameterId` is the parameter name (resolved via `Processor::getMetadata()`). Value is validated against the parameter's range at request time.

**Example — Play a chord**:
```bash
curl -X POST http://localhost:1900/api/testing/sequence \
  -H "Content-Type: application/json" \
  -d '{
    "messages": [
      {"type": "note", "noteNumber": 60, "velocity": 0.8, "duration": 1000, "timestamp": 0},
      {"type": "note", "noteNumber": 64, "velocity": 0.8, "duration": 1000, "timestamp": 0},
      {"type": "note", "noteNumber": 67, "velocity": 0.8, "duration": 1000, "timestamp": 0}
    ]
  }'
```

**Example — Melody with polyphony**:
```bash
curl -X POST http://localhost:1900/api/testing/sequence \
  -H "Content-Type: application/json" \
  -d '{
    "messages": [
      {"type": "note", "noteNumber": 60, "duration": 2000, "timestamp": 0},
      {"type": "note", "noteNumber": 67, "duration": 400, "timestamp": 1000},
      {"type": "cc", "controller": 74, "value": 100, "timestamp": 1500},
      {"type": "note", "noteNumber": 72, "duration": 500, "timestamp": 2000}
    ]
  }'
```

**Example — DSP test: inject signal, check state, record output**:
```bash
curl -X POST http://localhost:1900/api/testing/sequence \
  -H "Content-Type: application/json" \
  -d '{
    "blocking": true,
    "recordOutput": "D:/Tests/sweep_output.wav",
    "messages": [
      {"type": "testsignal", "signal": "sweep", "duration": 1000, "timestamp": 0},
      {"type": "repl", "id": "level", "expression": "Synth.getNumPressedKeys()", "timestamp": 500}
    ]
  }'
```

**Example — Automate a parameter during playback**:
```bash
curl -X POST http://localhost:1900/api/testing/sequence \
  -H "Content-Type: application/json" \
  -d '{
    "messages": [
      {"type": "note", "noteNumber": 60, "duration": 2000, "timestamp": 0},
      {"type": "set_attribute", "processorId": "Simple Gain", "parameterId": "Gain", "value": -12.0, "timestamp": 500},
      {"type": "set_attribute", "processorId": "Simple Gain", "parameterId": "Gain", "value": 0.0, "timestamp": 1500}
    ]
  }'
```

**Example — Poll status / Panic**:
```bash
# Poll (empty messages)
curl -X POST http://localhost:1900/api/testing/sequence \
  -H "Content-Type: application/json" \
  -d '{"messages": []}'

# Panic (stop everything)
curl -X POST http://localhost:1900/api/testing/sequence \
  -H "Content-Type: application/json" \
  -d '{"messages": [{"type": "allNotesOff"}]}'
```

**Response** (standard envelope with payload inside `result`):
```json
{
  "success": true,
  "result": {
    "isPlaying": true,
    "durationMs": 2500,
    "activeNotes": 2,
    "eventsInSequence": 4,
    "playedEvents": 1,
    "progress": 0.4
  },
  "logs": [],
  "errors": []
}
```

**Response with REPL results** (when `blocking: true`):
```json
{
  "success": true,
  "result": {
    "isPlaying": false,
    "durationMs": 1000,
    "activeNotes": 0,
    "eventsInSequence": 2,
    "playedEvents": 2,
    "progress": 1.0,
    "replResults": [
      {
        "id": "voicecheck",
        "expression": "Synth.getNumPressedKeys()",
        "moduleId": "Interface",
        "timestamp": 500,
        "success": true,
        "value": 1
      }
    ]
  },
  "logs": [],
  "errors": []
}
```

**Result Fields**:
| Field | Description |
|-------|-------------|
| `isPlaying` | `true` while the sequence is being dispatched or notes are active |
| `durationMs` | Total duration of the sequence (last event timestamp + its duration) |
| `activeNotes` | Number of notes currently sounding (note-on sent, note-off not yet fired) |
| `eventsInSequence` | Total logical events in queue (note on+off = 1 event) |
| `playedEvents` | Number of events dispatched so far |
| `progress` | 0.0–1.0 playhead position against sequence duration |
| `replResults` | Array of REPL evaluation results (only present when results are available) |
| `recordOutput` | File path of recorded WAV (only present when `recordOutput` was specified) |

**Notes**:
- By default the endpoint returns immediately (non-blocking). Use `blocking: true` or `recordOutput` to wait for completion
- Send an empty `messages` array to poll the current playback status without queuing anything
- Multiple calls merge into the pending queue; timestamps in new calls are relative to "now"
- `allNotesOff` acts as a panic button: clears the entire queue, fires all pending note-offs, stops any playing test signal
- The `activeNotes` count in the immediate response may be 0 if the timer hasn't fired yet; poll again to see the updated state
- REPL errors include sanitized `errorMessage`, `location`, and `callstack` fields (no base64)

---

## Component Lifecycle & Callbacks

### When Control Callbacks Fire

Control callbacks (assigned via `setControlCallback` or the global `onControl`) are triggered when:

1. **During compilation/recompilation** - For components with `saveInPreset: true` (the default), values are restored from the preset state, which triggers callbacks
2. **When the user interacts** with the control in the UI
3. **When calling `set_component_value`** via the REST API

### The `saveInPreset` Property

The `saveInPreset` property determines whether a component's value is saved/restored with user presets:

| Value | Behavior |
|-------|----------|
| `true` (default) | Value is persisted in presets. Callback fires during compilation when the value is restored. |
| `false` | Value is NOT persisted. Callback does NOT fire during compilation. |

**Important**: Set `saveInPreset` based on your plugin's preset requirements, NOT to control callback timing. Components that should be part of user presets MUST have `saveInPreset: true`.

Examples of components that typically have `saveInPreset: false`:
- UI state indicators (e.g., current page, animation state)
- Temporary display elements
- Controls that derive their value from other sources

### Implications for Runtime Errors

If a callback contains a runtime error (e.g., accessing an undefined variable), the error will appear:

| Scenario | When error appears |
|----------|-------------------|
| `saveInPreset: true` component | During `set_script`/`recompile` (when value is restored) |
| `saveInPreset: false` component | During `set_component_value` (when explicitly triggered) |
| Any component | During `set_component_value` (when explicitly triggered) |

This means runtime errors in callbacks for `saveInPreset: true` components may appear in the compilation response, even though the script syntax itself is valid. The `success` field will reflect the compilation status, while the `errors` array will contain any runtime errors that occurred during callback execution.

---

## Development Workflows

### Recommended: API-Only Workflow

Use the API exclusively for script modifications. This approach ensures the script content in HISE is always in sync with what you're working on.

**Advantages**:
- Atomic operations (get, modify, set in one flow)
- No file sync issues
- Immediate feedback on compile errors
- Works for inline callbacks that aren't stored in external files

**Workflow Pseudocode**:
```
1. GET /api/status
   -> Store scriptProcessors list
   -> Identify target processor (usually isMainInterface: true)

2. GET /api/get_script?moduleId={target}&callback={callback}
   -> Get current script content

3. Analyze/modify the script content

4. POST /api/set_script?moduleId={target}
   body: { callback, script, compile: true }
   
5. Check response:
   IF success == true AND errors is empty:
      -> Done, or continue with next change
   ELSE:
      -> Parse errors[].errorMessage and errors[].callstack
      -> Fix the issue in script
      -> Go to step 4

REPEAT steps 2-5 for each change needed
```

**Example: Fix a bug in onInit**
```bash
# 1. Get current script
SCRIPT=$(curl -s "http://localhost:1900/api/get_script?moduleId=Interface&callback=onInit" | jq -r '.script')

# 2. Modify script (example: agent modifies $SCRIPT here)
MODIFIED_SCRIPT="..."

# 3. Set and compile (all params in JSON body)
curl -X POST "http://localhost:1900/api/set_script" \
  -H "Content-Type: application/json" \
  -d "{\"moduleId\": \"Interface\", \"callback\": \"onInit\", \"script\": \"$MODIFIED_SCRIPT\", \"compile\": true}"

# 4. Check response for errors, fix if needed, repeat
```

---

### Alternative: Direct File Editing

Edit `.js` files directly in the `scriptsFolder`, then trigger recompilation via the API.

**When to use**:
- Large refactoring across multiple files
- Working with external include files (listed in `externalFiles`)
- Using external tools (linters, formatters) on the files
- Version control workflows (git)

**Important**: External files are stored in the `scriptsFolder` path from `/api/status`. Inline callbacks (the code inside the processor itself) cannot be edited this way.

**Workflow Pseudocode**:
```
1. GET /api/status
   -> Get scriptsFolder path
   -> Get list of externalFiles for target processor

2. Read/modify files directly:
   {scriptsFolder}/{externalFile}

3. GET /api/recompile?moduleId={target}
   -> Triggers recompilation with updated file contents

4. Check response for errors:
   IF errors is not empty:
      -> Parse errors[].callstack for file:line:col
      -> Fix the file
      -> Go to step 3

REPEAT until no errors
```

**Example: Edit an external file**
```bash
# 1. Get project info
STATUS=$(curl -s "http://localhost:1900/api/status")
SCRIPTS_FOLDER=$(echo $STATUS | jq -r '.project.scriptsFolder')
# -> "D:/Projects/MyPlugin/Scripts"

# 2. Edit external file directly
# File: D:/Projects/MyPlugin/Scripts/utils.js
# (make changes with your preferred method)

# 3. Recompile
curl "http://localhost:1900/api/recompile?moduleId=Interface"

# 4. Check for errors in response
```

---

### Visual Development Loop with Screenshots

Use the screenshot endpoint to create a visual feedback loop for UI development. This enables an iterative workflow: modify script → capture screenshot → verify visually → refine.

**Workflow:**

```
1. POST /api/set_script
   -> Create/modify UI components with paint routines

2. GET /api/testing/screenshot?outputPath=/path/to/screenshot.png
   -> Capture the result to a file

3. Inspect the image file
   -> Verify the UI looks correct

4. If changes needed, go back to step 1
```

**Example: Create and verify a colored panel**

```bash
# 1. Create a panel with a blue fill
curl -X POST "http://localhost:1900/api/set_script" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "callback": "onInit",
    "script": "Content.makeFrontInterface(400, 300);\nconst var MyPanel = Content.addPanel(\"MyPanel\", 10, 10);\nMyPanel.set(\"width\", 100);\nMyPanel.set(\"height\", 100);\nMyPanel.setPaintRoutine(function(g) {\n    g.fillAll(Colours.blue);\n});",
    "compile": true
  }'

# 2. Capture screenshot of the panel
curl "http://localhost:1900/api/testing/screenshot?moduleId=Interface&id=MyPanel&outputPath=/path/to/screenshot.png"

# 3. Inspect the image - should show a 100x100 blue square

# 4. Change to green and verify
curl -X POST "http://localhost:1900/api/set_script" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "callback": "onInit",
    "script": "Content.makeFrontInterface(400, 300);\nconst var MyPanel = Content.addPanel(\"MyPanel\", 10, 10);\nMyPanel.set(\"width\", 100);\nMyPanel.set(\"height\", 100);\nMyPanel.setPaintRoutine(function(g) {\n    g.fillAll(Colours.green);\n});",
    "compile": true
  }'

# 5. Capture again - should now show green
curl "http://localhost:1900/api/testing/screenshot?moduleId=Interface&id=MyPanel&outputPath=/path/to/screenshot.png"
```

**Tips:**

- Use `outputPath` to save screenshots to a known location for easy file access
- Use the `id` parameter to capture specific components instead of the full interface
- Use `scale=0.5` for faster iteration when pixel-perfect accuracy isn't needed
- The same output path can be reused - files are automatically overwritten

---

### Working with Interface Designer Selection

Use `/api/get_selected_components` to enable AI-assisted workflows where the user selects components visually and the AI performs operations on them. This is particularly useful for layout tasks, batch property changes, and code generation.

**Workflow:**

```
1. User selects components in HISE's Interface Designer (shift-click to multi-select)

2. GET /api/get_selected_components
   -> Get all selected components with their properties

3. Analyze selection and perform requested operation:
   - Layout: Calculate new positions, use set_component_properties
   - Code: Generate callbacks, use set_script
   - Hierarchy: Set parentComponent, use set_component_properties

4. Optionally verify with GET /api/testing/screenshot
```

**Example: Align selected buttons horizontally**

User prompt: *"I've selected 4 buttons. Please align them horizontally with even spacing."*

```bash
# 1. Get the selected components
curl -s "http://localhost:1900/api/get_selected_components?moduleId=Interface"
```

Response shows 4 buttons with positions:
- Button1: x=162, y=113
- Button2: x=170, y=170  
- Button3: x=180, y=220
- Button4: x=220, y=270

```bash
# 2. Calculate new positions (same y, evenly spaced x)
# Starting at x=50, spacing of 150px

# 3. Apply the new layout
curl -X POST "http://localhost:1900/api/set_component_properties" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "changes": [
      { "id": "Button1", "properties": { "x": 50, "y": 100 } },
      { "id": "Button2", "properties": { "x": 200, "y": 100 } },
      { "id": "Button3", "properties": { "x": 350, "y": 100 } },
      { "id": "Button4", "properties": { "x": 500, "y": 100 } }
    ]
  }'
```

**Example: Create a panel and parent selected buttons to it**

User prompt: *"Create a background panel and put all selected buttons inside it."*

```bash
# 1. Get selected components to know their bounds
curl -s "http://localhost:1900/api/get_selected_components?moduleId=Interface"

# 2. Get current script
curl -s "http://localhost:1900/api/get_script?moduleId=Interface"

# 3. Modify script to add a panel and set parent relationships
curl -X POST "http://localhost:1900/api/set_script" \
  -H "Content-Type: application/json" \
  -d '{
    "moduleId": "Interface",
    "script": "Content.makeFrontInterface(600, 600);\n\nconst var bgPanel = Content.addPanel(\"BackgroundPanel\", 0, 0);\nbgPanel.set(\"width\", 600);\nbgPanel.set(\"height\", 600);\nbgPanel.set(\"bgColour\", 0xFF333333);\n\nconst var Button1 = Content.addButton(\"Button1\", 50, 50);\nButton1.set(\"parentComponent\", \"BackgroundPanel\");\n\nconst var Button2 = Content.addButton(\"Button2\", 50, 100);\nButton2.set(\"parentComponent\", \"BackgroundPanel\");\n\nconst var Button3 = Content.addButton(\"Button3\", 50, 150);\nButton3.set(\"parentComponent\", \"BackgroundPanel\");\n\nconst var Button4 = Content.addButton(\"Button4\", 50, 200);\nButton4.set(\"parentComponent\", \"BackgroundPanel\");\n\nfunction onNoteOn() {}\nfunction onNoteOff() {}\nfunction onController() {}\nfunction onTimer() {}\n\nfunction onControl(number, value)\n{\n    if (number == Button1) Console.print(\"Button 1 clicked (index 0)\");\n    else if (number == Button2) Console.print(\"Button 2 clicked (index 1)\");\n    else if (number == Button3) Console.print(\"Button 3 clicked (index 2)\");\n    else if (number == Button4) Console.print(\"Button 4 clicked (index 3)\");\n}",
    "compile": true
  }'

# 4. Verify the hierarchy
curl -s "http://localhost:1900/api/list_components?moduleId=Interface&hierarchy=true"
```

**Tips:**

- Selection order matters: Components are returned in the order they were selected, which can be useful for operations like "number these from left to right"
- Use `set_component_properties` for layout changes (doesn't require recompilation)
- Use `set_script` when adding callbacks or creating new components
- The selection is cleared on recompile, so capture it before making script changes
- Combine with `/api/testing/screenshot` to verify visual results

---

## Error Handling

### Error Response Structure

When compilation fails or a runtime error occurs, the `errors` array contains detailed information:

```json
{
  "errors": [
    {
      "errorMessage": "API call with undefined parameter 0",
      "callstack": [
        "helperFunction() at Scripts/utils.js:42:12",
        "initUI() at Scripts/ui.js:15:5",
        "onInit() at Interface.js:8:1"
      ]
    }
  ]
}
```

### Callstack Format

Callstack supports two formats, depending on endpoint domain:

1. **Script/runtime stack frames**:

```
functionName() at path:line:column
```

- `functionName` - The function where the error occurred (or was called from)
- `path` - Relative path from project root, or `ModuleId.js` for inline callbacks
- `line` - Line number (1-based)
- `column` - Column number (1-based)

2. **Builder/undo operation context frames** (most specific first):

```
op[<index>]: <operation summary>
phase: prevalidate|validate|runtime
group: <group name>
endpoint: <api path>
```

Example:

```json
{
  "errorMessage": "Can't find module with ID LFO1",
  "callstack": [
    "op[0]: set_id target=LFO1 name=LFO2",
    "phase: validate",
    "group: My Group",
    "endpoint: api/builder/apply"
  ]
}
```

### Error-Fixing Loop

```
WHILE errors is not empty:
    error = errors[0]
    location = parse callstack[0]  # Top of stack = error location
    
    IF location.path ends with ".js" AND exists in scriptsFolder:
        # External file - edit directly or via include
        Edit file at {scriptsFolder}/{location.path}
        GET /api/recompile?moduleId={moduleId}
    ELSE:
        # Inline callback
        callback = extract callback name from location.path (e.g., "onInit")
        GET /api/get_script?moduleId={moduleId}&callback={callback}
        Fix the error in returned script
        POST /api/set_script with fixed script
    
    Check new response for errors
```

---

## Debugging Techniques

### Querying Component Properties (Recommended)

Use `/api/get_component_properties` to discover property names and values without modifying scripts:

```bash
curl "http://localhost:1900/api/get_component_properties?moduleId=Interface&id=Button1"
```

This returns all properties with their correct names, current values, and available options - eliminating guesswork about property names like `bgColour` vs `colour`.

### Debugging Visibility Issues

Use `/api/list_components?hierarchy=true` to understand the component tree and check visibility:

```bash
curl "http://localhost:1900/api/list_components?moduleId=Interface&hierarchy=true"
```

This shows each component's `visible` and `enabled` state along with positions. If a component isn't visible, check:
1. Its own `visible` property
2. Parent component's `visible` property (traverse up the tree)
3. Position - is it within the parent's bounds?

### Querying Runtime Values

For dynamic values that can't be queried via the API, use `Console.print()`. The output appears in the `logs` array of the response:

```javascript
// In onInit - query dynamic values
Console.print(myArray.length);
Console.print(calculatedValue);
```

Response:
```json
{
  "success": true,
  "logs": ["5", "42"],
  "errors": []
}
```

### Asserting Conditions

Use `Console.assertWithMessage(condition, errorMessage)` to validate assumptions. If the condition is false, compilation fails with your custom message:

```javascript
// Validate expected state
Console.assertWithMessage(buttons.length == 5, "Expected 5 buttons but got " + buttons.length);
Console.assertWithMessage(panel.get("width") > 0, "Panel width must be positive");

// Verify calculations
var totalWidth = buttonCount * (buttonWidth + gap);
Console.assertWithMessage(totalWidth <= 600, "Buttons exceed interface width: " + totalWidth);
```

This is useful for:
- Catching configuration errors early
- Validating calculations before they cause layout issues
- Providing clear error messages instead of debugging unexpected behavior

---

## Best Practices

### 1. Always Start with /api/status

Before any operation, call `/api/status` to:
- Verify HISE is running and the API is available
- Discover all available script processors and their IDs
- Get the `scriptsFolder` path for direct file access
- Check which callbacks have content (`empty: false`)

### 2. Target the Right Processor

- Use `isMainInterface: true` for UI-related changes
- Check `externalFiles` to understand dependencies
- Different processor types have different callbacks (e.g., Script FX has `processBlock`)

### 3. Check Callback State Before Reading

Use the `empty` field from `/api/status` to know which callbacks have content:
- `empty: true` - Callback exists but has no code
- `empty: false` - Callback has code worth reading

### 4. Compile Frequently

Compile after each logical change rather than batching many changes:
- Easier to identify which change caused an error
- Faster feedback loop
- Prevents cascading errors

### 5. Use Appropriate Workflow

| Scenario | Recommended Approach |
|----------|---------------------|
| Fixing a bug in a callback | API-only workflow |
| Adding a new feature to onInit | API-only workflow |
| Refactoring shared utility functions | Direct file editing |
| Working with multiple include files | Direct file editing |
| Quick iteration on UI code | API-only workflow |

### 6. Handle the onInit Special Case

Remember that `onInit` content is returned raw (no function wrapper), while other callbacks are wrapped:
- `onInit`: `Console.print("hello");`
- `onNoteOn`: `function onNoteOn() { Console.print("hello"); }`

When setting `onInit`, provide raw code. When setting other callbacks, include the function wrapper.

### 7. Prefer Action Over Confirmation

When working iteratively:
- Try changes directly rather than asking for confirmation
- The error feedback loop is robust - mistakes are recoverable
- Use `Console.print()` to discover unknown values (check `logs` in response)
- Use `Console.assertWithMessage()` to validate assumptions

The compile → check errors → fix cycle is fast enough that trial-and-error is often more efficient than extensive upfront planning.

---

## Advanced Debugging: `forceSynchronousExecution`

### Overview

The `forceSynchronousExecution` parameter is a **debug tool of last resort** available on three POST endpoints:

- `POST /api/set_script`
- `POST /api/recompile`
- `POST /api/set_component_value`

When set to `true`, this parameter bypasses HISE's normal threading model and executes everything synchronously on the current thread. This can help diagnose timing-related issues where callbacks or async operations aren't behaving as expected.

### ⚠️ WARNING: Use With Extreme Caution

**This parameter bypasses HISE's threading safety checks and may cause crashes due to race conditions.**

- **Never use automatically** - Always prompt the user first
- **Save work first** - The user should save their project before attempting
- **Last resort only** - Only use when normal execution produces unexpected results

### When to Consider Using It

You might suspect a threading issue if:

1. **Missing callback logs** - `set_component_value` returns empty `logs` array even though the callback has `Console.print()` calls
2. **Inconsistent behavior** - The same request sometimes works and sometimes doesn't
3. **Different results** - Manual UI interaction produces different results than API calls

### Workflow for Diagnosing Threading Issues

```
1. Execute request normally (without forceSynchronousExecution)
   -> Observe unexpected behavior (e.g., missing logs)

2. PROMPT THE USER:
   "I suspect a threading issue. Using forceSynchronousExecution may help 
   diagnose this, but it bypasses safety checks and could cause HISE to crash.
   Please save your work before proceeding. Do you want me to try this?"

3. If user confirms, execute with forceSynchronousExecution: true

4. Compare results:
   - If sync mode produces DIFFERENT results → Threading issue confirmed
   - If sync mode produces SAME results → Issue is not threading-related

5. Report findings to the user for guidance on how to proceed
```

### Example Usage

**Normal request (async execution):**
```bash
curl -X POST "http://localhost:1900/api/set_component_value" \
  -H "Content-Type: application/json" \
  -d '{"moduleId": "Interface", "id": "Knob1", "value": 0.5}'
```

**Debug request (forced synchronous execution):**
```bash
curl -X POST "http://localhost:1900/api/set_component_value" \
  -H "Content-Type: application/json" \
  -d '{"moduleId": "Interface", "id": "Knob1", "value": 0.5, "forceSynchronousExecution": true}'
```

### Response When Using `forceSynchronousExecution`

When `forceSynchronousExecution: true` is used, the response includes additional fields:

```json
{
  "success": true,
  "moduleId": "Interface",
  "id": "Knob1",
  "type": "ScriptSlider",
  "forceSynchronousExecution": true,
  "warning": "Executed in unsafe synchronous mode - threading checks bypassed",
  "logs": ["Callback executed: 0.5"],
  "errors": []
}
```

| Field | Description |
|-------|-------------|
| `forceSynchronousExecution` | Confirms sync mode was used (`true`) or not (`false`) |
| `warning` | Present only when sync mode was used - reminder that safety was bypassed |

### What This Tool Does NOT Do

- **Does not fix threading issues** - It's a diagnostic tool only
- **Does not make code thread-safe** - If sync mode works but async doesn't, there's an underlying issue
- **Does not replace proper async handling** - The user/developer needs to address the root cause

### Reporting Threading Issues

If you discover that `forceSynchronousExecution: true` produces different results than normal execution, report this to the user with:

1. The specific endpoint and parameters that exhibited the issue
2. The difference in behavior (e.g., "logs were empty without sync mode but contained expected output with sync mode")
3. A recommendation to investigate the callback code for potential threading issues

---

## Troubleshooting

### API Not Responding

- Ensure HISE is running
- Check that the REST server started (look for console message on HISE startup)
- Verify port 1900 is not blocked by firewall

### Module Not Found (404)

- Call `/api/status` to get the exact `moduleId` values
- Module IDs are case-sensitive
- The processor must exist in the current project

### Script Changes Not Taking Effect

- Ensure `compile: true` in `set_script` request (or call `recompile` after)
- Check the response for errors that may have prevented compilation
- For external files, make sure you're editing the right path from `scriptsFolder`

### Errors Keep Returning

- Check the full callstack, not just the error message
- The actual bug may be in a different file/function than where it manifests
- Look at `externalFiles` to understand the include chain
