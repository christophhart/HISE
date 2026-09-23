# simple_css approved bug-fix backlog

This is the implementation queue derived from `css_report.md`. It contains approved
fixes only. Deferred behavior is intentionally excluded. The cheat sheet and JSON
dataset describe the post-fix contract assumed after all items below are complete.

Tests belong in `hi_tools/simple_css/simple_css.cpp` under `HI_RUN_UNIT_TESTS`.
The existing `CssTestSuite` registration is currently disabled.

## Policy

- Preserve HISE `#AARRGGBB` and `0xAARRGGBB` colors.
- Preserve lowercase-only CSS names and HISE internal margin behavior.
- Fixes must not expand into deferred selector, cascade, layout, compositing, or
  browser-feature redesigns.
- Invalid input must never corrupt memory, loop forever, or silently acquire unrelated
  semantics.
- Migration warnings must be one-line tooltips.

## Safety and termination

### CSS-FIX-001 - Bound transform names and arguments

- Source: `hi_tools/simple_css/CssParser.cpp`, `TransformParser::parse()`;
  `CssParser.h`, `TransformData`.
- Current: transform names can terminate outside a 20-byte buffer; more than two
  arguments are written before arity is clamped.
- Required: bound every write, reject excessive arguments, and diagnose malformed
  transforms without corrupting later parsing.
- Tests: boundary-length names, three-argument two-value transforms, unterminated
  functions, and successful parsing of a later rule.

### CSS-FIX-002 - Guard parser end-of-input reads

- Source: `hi_tools/simple_css/CssParser.cpp`, parser token, selector, quote, comment,
  parenthesis, and expression scanning.
- Current: several loops dereference after reaching the input end.
- Required: check the end before every dereference and return a bounded parse result.
- Tests: empty, whitespace-only, truncated selector, quote, comment, expression, and
  function inputs.

### CSS-FIX-003 - Reject empty `!important` values

- Source: `CssParser.cpp`, declaration token handling.
- Current: `property: !important;` removes the only token and later indexes an empty list.
- Required: ignore only the invalid declaration and emit a concise diagnostic.
- Tests: empty important values before and after valid declarations.

### CSS-FIX-004 - Handle empty flex child lists

- Source: `FlexboxComponent.cpp`, `getFirstLastComponents()` and position setup.
- Current: first/last access assumes an eligible child exists.
- Required: return null endpoints for empty, hidden-only, and out-of-flow-only lists.
- Tests: empty containers and mixed eligible/ineligible children.

### CSS-FIX-005 - Validate background targets, images, and streams

- Source: `Renderer.cpp`, `FlexboxComponent.cpp`, and `CSSLookAndFeel.cpp` image paths.
- Current: null component targets, null streams, invalid images, and zero dimensions can
  be dereferenced or used in division.
- Required: skip safely, check streams, reject invalid images, and avoid repeated warning
  spam in paint callbacks.
- Tests: null renderer target, missing URL, invalid image, zero-sized image, and valid
  image rendering.

### CSS-FIX-006 - Terminate cyclic variable substitution

- Source: `HelperClasses.cpp`, variable value resolution.
- Current: self and indirect `var()` cycles can loop forever.
- Required: detect cycles, return an unresolved value, and continue unrelated processing.
- Tests: self-cycle, indirect cycle, embedded cycle, long acyclic chain, and unrelated
  variable resolution.

### CSS-FIX-007 - Bound unequal gradient-stop interpolation

- Source: `StyleSheet.cpp`, gradient transition interpolation.
- Current: the larger stop list can index beyond the shorter list.
- Required: sample valid positions from both gradients and preserve solid-to-gradient
  transitions.
- Tests: two-to-three and three-to-two stop transitions at 0, 0.5, and 1.

### CSS-FIX-008 - Initialize and update stylesheet caches correctly

- Source: `StyleSheet.h` and `StyleSheet.cpp`, cache flags, clearing, and updates.
- Current: an uninitialized non-layout flag, incomplete clearing, and by-value cache
  updates produce nondeterministic or stale results.
- Required: initialize flags, clear both cache forms, and mutate cache entries by reference.
- Tests: new single-rule sheets, per-component clear, replacement update, and full clear.

## Parser and shorthand correctness

### CSS-FIX-009 - Correct four-value margin and padding order

- Source: `CssParser.cpp`, positioning shorthand expansion.
- Current: values map as top, bottom, left, right.
- Required: map standard top, right, bottom, left order.
- Migration: release-note warning and visual review of existing four-value declarations.
- Tests: distinct values and asymmetric geometry for margin and padding.

### CSS-FIX-010 - Correct four-value border-radius order

- Source: `CssParser.cpp`, border-radius shorthand expansion.
- Current: bottom corners are reversed.
- Required: map top-left, top-right, bottom-right, bottom-left.
- Migration: release-note warning and visual review.
- Tests: query all four generated longhands with distinct values.

### CSS-FIX-011 - Implement three-value border-radius shorthand

- Source: `CssParser.cpp`, border-radius shorthand expansion.
- Current: three-value syntax is discarded.
- Required: map top-left = 1, top-right/bottom-left = 2, bottom-right = 3.
- Scope: slash-separated elliptical radii remain unsupported.
- Tests: one-, two-, three-, and four-value table-driven cases.

### CSS-FIX-012 - Prevent duplicate border suffixes

- Source: `CssParser.cpp`, `getTokenSuffix()` and border declaration handling.
- Current: names such as `border-width-width` and `border-style-style` are produced.
- Required: do not append an existing `-width`, `-style`, or `-color` suffix.
- Scope: this does not implement visual dotted, dashed, or outset borders.
- Tests: border shorthand, all side width/color longhands, and suffix-name assertions.

### CSS-FIX-013 - Correct negative `max()` and division

- Source: `CssParser.cpp`, expression evaluation.
- Current: negative-only `max()` and negative divisors produce incorrect results.
- Required: initialize max with the lowest value; divide by any nonzero divisor; retain
  safe division-by-zero behavior.
- Tests: negative-only max, mixed max, negative division, and zero division.

### CSS-FIX-014 - Accept an omitted final declaration semicolon

- Source: `CssParser.cpp`, value-string and declaration parsing.
- Current: `property: value}` is rejected.
- Required: accept it as equivalent to `property: value;}`. Separators between declarations
  remain required.
- Tests: one final declaration, multiple declarations, comments, and functional values.

### CSS-FIX-015 - Honor quotes while scanning parentheses

- Source: `CssParser.cpp`, value parenthesis scanning.
- Current: parentheses inside quoted strings alter nesting.
- Required: ignore quoted parentheses while preserving bounded errors for unterminated
  quotes or functions.
- Tests: quoted parentheses in URLs, nested functions, and truncated values.

### CSS-FIX-016 - Support quoted imports and propagate failures

- Source: `StyleSheet.cpp` import processing and `CssParser.cpp` at-rule parsing.
- Current: quoted imports are stored but not loaded; nested parse failures are discarded.
- Required: accept quoted and `url()` sources and return import context with nested errors.
- Scope: isolated child-collection import behavior remains deferred.
- Tests: quoted sources, URL sources, valid imports, and invalid imported sheets.

### CSS-FIX-017 - Complete metadata for executable syntax

- Source: `LanguageManager.cpp`.
- Required metadata: `font-style`, `src`, per-side border width/color names, `:empty`,
  `::before2`, `::after2`, and `ease-out`.
- Scope: do not advertise `overflow` as effective or implement border-style rendering here.
- Tests: representative syntax produces no false unsupported warning; unknown syntax still
  warns.

## Geometry and rendering

### CSS-FIX-018 - Correct height percentage and auto axes

- Source: `StyleSheet.cpp`, bounds and pixel-value calculations.
- Current: height percentages use width; one auto path swaps width and height.
- Required: height uses height context; horizontal auto uses width; vertical auto uses height.
- Scope: retain HISE internal margin:auto behavior.
- Tests: non-square source areas with percentage constraints and auto sizing.

### CSS-FIX-019 - Correct `scale-down`

- Source: `Renderer.cpp`, image placement.
- Current: small images can enlarge and scaling behaves like cover.
- Required: preserve natural size when the source fits; otherwise use contain scaling and
  never enlarge.
- Migration: release-note warning and image visual review.
- Tests: smaller, wider, taller, and mixed-dimension source images.

### CSS-FIX-020 - Use the `::after2` positioning flag

- Source: `Renderer.cpp`, generated background areas.
- Current: `::after2` uses the `::after` absolute-position flag.
- Required: use the independent `afterAbsolute2` flag.
- Migration: release-note warning for users of the HISE extension.
- Tests: opposite relative/absolute combinations for `::after` and `::after2`.

## Transform and declaration replacement

### CSS-FIX-021 - Implement axis-specific 2D transforms

- Source: `CssParser.cpp`, `TransformData::toTransform()`.
- Required: `translateX(x)=(x,0)`, `translateY(y)=(0,y)`, `scaleX(x)=(x,1)`,
  `scaleY(y)=(1,y)`, `skewX(a)` X-only, and `skewY(a)` Y-only.
- Migration: project search and visual review.
- Tests: affine matrix components for positive, negative, zero, and composed transforms.

### CSS-FIX-022 - Correct one-argument transform defaults

- Source: `CssParser.cpp`, transform defaults and interpolation.
- Required: `translate(x)=(x,0)`, `skew(a)=(a,0)`, and `scale(x)=(x,x)`.
- Migration: project search for one-argument translate/skew.
- Tests: one/two argument forms and transition interpolation.

### CSS-FIX-023 - Reject unsupported 3D transforms and `matrix()`

- Source: `CssParser.cpp` transform parsing and `LanguageManager.cpp` metadata.
- Required: reject the complete declaration for `matrix`, `translateZ`, `scaleZ`, `rotateX`,
  and `rotateY`, with a diagnostic. Retain `rotateZ()` as a 2D alias for `rotate()`.
- Tests: each rejected function alone and mixed with valid functions; `rotateZ` equivalence.

### CSS-FIX-024 - Repeated transform declarations replace

- Source: `CssParser.cpp`, property accumulation.
- Current: repeated declarations append.
- Required: the later declaration replaces the earlier one; functions within one whitespace-
  separated declaration still compose.
- Migration: release-note warning and project search.
- Tests: repeated declarations, one declaration with multiple functions, important priority,
  and independent pseudo states.

## Colors, gradients, and shadows

### CSS-FIX-025 - Preserve fractional gradient angles and stops

- Source: `CssParser.cpp`, `ColourGradientParser`.
- Current: degrees and percentage stops use integer conversion.
- Required: parse floating-point degrees and percentages; clamp safely where needed.
- Scope: unsupported stop units remain rejected rather than treated as percentages.
- Tests: fractional angles, fractional stops, and boundary values.

### CSS-FIX-026 - Implement standard comma-form HSL and HSLA

- Source: `CssParser.cpp`, `ColourParser`.
- Current: H, S, and L are treated as 0-255 channels.
- Required: accept `hsl(H, S%, L%)` and `hsla(H, S%, L%, A)` with hue in degrees,
  percentage saturation/lightness, and numeric alpha.
- Scope: modern space/slash syntax remains unsupported.
- Migration: project audit and release-note warning; replace old unit tests.
- Tests: primary colors, hue wrapping, endpoints, and alpha.

### CSS-FIX-027 - Repeated shadow declarations replace with warnings

- Source: `CssParser.cpp`, shadow accumulation and parser warnings.
- Required: later `box-shadow` or `text-shadow` replaces earlier declarations; comma-separated
  shadows within one declaration still compose.
- Warnings: `Repeated box-shadow overrides the previous value; use commas to combine shadows.`
  and the equivalent `text-shadow` message.
- Tests: repeated declarations, comma lists, important priority, and pseudo states.

### CSS-FIX-028 - Ignore unknown colors with a warning

- Source: `CssParser.cpp`, color validity and declaration handling.
- Current: unknown names can become transparent black.
- Required: distinguish invalid colors from valid transparent black; ignore only the invalid
  declaration and warn. Preserve a prior valid cascaded value.
- Scope: unresolved variable-backed colors are not rejected during initial parsing.
- Tests: misspellings, fallback to a prior color, `transparent`, valid RGB/HSL, and variables.

### CSS-FIX-029 - Reject four-digit hash alpha colors

- Source: `CssParser.cpp`, color parsing.
- Required: reject `#RGBA`, warn with `rgba()` and HISE `#AARRGGBB` alternatives, and never
  reinterpret eight-digit HISE hashes as browser trailing-alpha hashes.
- Tests: `#RGB`, `#RRGGBB`, rejected `#RGBA`, HISE `#AARRGGBB`, and `0xAARRGGBB`.

## Transitions and timing

### CSS-FIX-030 - Use exact transition matching plus explicit shorthand families

- Source: `HelperClasses.cpp`, `PropertyKey::looseMatch()`, and transition assignment.
- Current: arbitrary prefix matching attaches transitions to unrelated properties.
- Required: `all` matches all; otherwise use exact names and explicit approved shorthand
  relationships such as `background` to supported background longhands and `border` to
  supported border longhands.
- Migration: release-note warning for accidental prefix matches.
- Tests: `border` does not match `border-radius`; `background` retains intended support;
  `all` still matches.

### CSS-FIX-031 - Distinguish `jump-both` and `jump-none`

- Source: `CssParser.cpp`, `parseTimingFunction()`.
- Current: both use the same rounding function.
- Required: implement endpoint behavior independently; preserve start/end aliases; validate
  invalid step counts such as `steps(1, jump-none)`.
- Tests: sample endpoints and interior boundaries for every step mode.

### CSS-FIX-032 - Correct animator baseline and delay timing

- Source: `Animator.h` and `Animator.cpp`.
- Current: the first delta is measured from zero; delay is normalized by duration and fails
  for zero-duration delayed transitions.
- Required: initialize the timestamp, track delay in seconds, consume callback overshoot,
  complete zero-duration transitions after delay, and support negative delay.
- Scope: layout relayout during transitions remains deferred.
- Tests: first callback, normal duration, positive delay, zero duration, boundary crossing,
  negative delay, and restart/reverse behavior.

## Explicitly excluded

These remain deferred and must not be added to this backlog without a new decision:

- General malformed-declaration recovery.
- Inherited opacity and subtree compositing.
- Background-position redesign.
- Case-insensitive CSS names.
- Stateful display visibility changes.
- HISE internal margins and true flex margins.
- Selector chains, specificity, universal rules, inheritance, and CSS-wide keywords.
- Box sizing, overflow, clipping, transform hit testing, and layout transitions.
- Full generated content, background layers, transition lists, and 3D transforms.
