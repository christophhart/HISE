# Profiling Endpoint Guide

`POST /api/testing/profile` — record and query performance data from HISE's profiling toolkit.

## Workflow

Record once, then query the same data with different filters:

```
1. POST { "mode": "record", "durationMs": 500 }     → returns immediately
2. POST { "mode": "get", "wait": true }              → blocks until done, returns full tree
3. POST { "mode": "get", "summary": true, ... }      → re-query same data with filters
```

## Record Mode

Starts a non-blocking recording session. Returns immediately.

```json
{
  "mode": "record",
  "durationMs": 500,
  "threadFilter": ["Audio Thread"],
  "eventFilter": ["DSP", "Script"]
}
```

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `durationMs` | number | 1000 | 100–5000 ms |
| `threadFilter` | string[] | all | Which threads to record. Valid: `Audio Thread`, `Scripting Thread`, `UI Thread`, `Loading Thread` |
| `eventFilter` | string[] | all | Which source types to record. Valid: `DSP`, `Script`, `Lock`, `Callback`, `Trace`, `TimerCallback`, `Scriptnode` |

Response: `{ "success": true, "recording": true, "durationMs": 500 }`

Returns 409 if a recording is already in progress.

## Get Mode

Returns the last recorded data. Blocks until recording finishes (unless `wait: false`).

### Full Tree (no filters)

```json
{ "mode": "get" }
```

Returns `threads` array (hierarchical event tree per thread) and `flows` array (cross-thread causal links).

### Filtered Flat List

```json
{
  "mode": "get",
  "filter": "*.processBlock*",
  "sourceTypeFilter": "DSP",
  "minDuration": 0.1,
  "limit": 10
}
```

Returns `results` array sorted by duration descending. Each entry: `name`, `sourceType`, `thread`, `start`, `duration`.

### Summary (Aggregated Stats)

```json
{
  "mode": "get",
  "summary": true,
  "threadFilter": ["Audio Thread"],
  "limit": 5
}
```

Groups repeated events by `name + sourceType + thread`. Each entry: `name`, `sourceType`, `thread`, `count`, `median`, `peak`, `min`, `total`.

### Nested Mode

```json
{
  "mode": "get",
  "filter": "Master Chain.processBlock*",
  "nested": true,
  "limit": 1
}
```

Returns matched events with their full children subtree intact (same structure as full tree mode, but with a `thread` property added to each root event).

### Non-blocking Poll

```json
{ "mode": "get", "wait": false }
```

If recording is in progress, returns `{ "success": true, "recording": true }` immediately instead of blocking.

## Get Mode Parameters

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `summary` | bool | false | Aggregate repeated events with stats |
| `filter` | string | — | Wildcard against event name (`*`, `?`). Case-insensitive |
| `sourceTypeFilter` | string | — | Wildcard against sourceType name |
| `minDuration` | number | — | Threshold in ms |
| `threadFilter` | string[] | all | Filter threads in output |
| `nested` | bool | false | Include full children subtree of matched events |
| `limit` | number | 15 | Max results (1–100) |
| `wait` | bool | true | If false, don't block when recording in progress |

All filter parameters combine with AND logic. `threadFilter` is applied first at the thread level for efficiency.

## Flows (Cross-Thread Links)

Both full-tree and filtered responses include a `flows` array. Each flow connects a source event on one thread to a target event on another:

```json
{
  "trackId": 1,
  "sourceEvent": "Panel1.repaint()",
  "sourceThread": "Scripting Thread",
  "targetEvent": "Panel1.paintRoutine",
  "targetThread": "UI Thread"
}
```

Events that participate in flows have `trackSource` or `trackTarget` integer fields (omitted when -1).

## Practical Patterns

**Audio thread DSP summary:**
```json
{ "mode": "get", "summary": true, "threadFilter": ["Audio Thread"], "sourceTypeFilter": "DSP" }
```

**Find the most expensive single event:**
```json
{ "mode": "get", "limit": 1 }
```

**Drill into a specific call with full subtree:**
```json
{ "mode": "get", "filter": "Master Chain.processBlock*", "nested": true, "limit": 1 }
```

**Find all trace instrumentation points:**
```json
{ "mode": "get", "filter": "slow*", "sourceTypeFilter": "Trace" }
```

**Record only audio, then compare DSP vs Script cost:**
```json
// Step 1
{ "mode": "record", "durationMs": 1000, "threadFilter": ["Audio Thread"] }
// Step 2 (after recording completes)
{ "mode": "get", "summary": true, "sourceTypeFilter": "DSP" }
// Step 3 (same data, different filter)
{ "mode": "get", "summary": true, "sourceTypeFilter": "Script" }
```
