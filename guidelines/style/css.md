# HISE simple_css agent cheat sheet

This reference describes the current HISE `simple_css` behavior. It styles JUCE
components and can drive a limited JUCE FlexBox layout; it is not browser CSS.

## Generate safely

- Use lowercase ASCII property names, keywords, function names, pseudo names, type names,
  and named colors.
- Emit semicolons after every declaration.
- Use `value !important` as the final separate token.
- Keep selectors simple and non-overlapping.
- Define every `var(--name)` and keep variables acyclic.
- Use `px` for geometry, `deg` for angles, and simple numeric expressions.
- Use comma-form `hsl(H, S%, L%)` and `hsla(H, S%, L%, A)` only.
- Use `rgba()` for browser-style alpha colors.
- Combine transforms in one declaration and shadows with commas in one declaration.
- Use `gap` for flex-child spacing, not child margins.
- Use `display: none` only for default-state flex-managed children.
- Treat transforms as paint-only; never depend on transformed hit testing, clipping, or layout.
- Use one transition tuple and only for pseudo-state paint changes.

## Supported syntax

- Rules, declarations, `/* comments */`, `@import`, and `@font-face`.
- Selectors: type, `.class`, `#id`, compounds, limited one-ancestor descendants, groups.
- Pseudo-classes: `:hover`, `:active`, `:focus`, `:disabled`, `:checked`, `:hidden`,
  `:first-child`, `:last-child`, `:root`, `:empty`.
- Pseudo-elements: `::before`, `::after`, `::before2`, `::after2`, special `::selection`.
- At-rules: `@import url(...)`, `@import "..."`, `@font-face`.

## Properties

### Layout and flex

`display`, `position`, `top`, `right`, `bottom`, `left`, `width`, `height`, `min-width`,
`max-width`, `min-height`, `max-height`, `margin`, `padding`, `box-sizing`, `opacity`.

`flex-direction`, `flex-wrap`, `justify-content`, `align-items`, `align-content`,
`align-self`, `flex-grow`, `flex-shrink`, `flex-basis`, `order`, `gap`.

### Paint

`background`, `background-color`, `background-image`, `background-size`,
`background-position`, `color`, `caret-color`, `object-fit`.

### Borders, shadows, text

`border`, `border-width`, `border-style`, `border-color`, side border width/color longhands,
`border-radius` and its four longhands, `box-shadow`, `text-shadow`.

`font-family`, `font-size`, `font-weight`, `font-style`, `font-stretch`, `letter-spacing`,
`text-align`, `vertical-align`, `text-transform`, `content`.

### Interaction

`cursor`, `transition`, `transform`.

## Values and functions

- Units: bare numbers, `px`, `%`, `em`, `rem` (acts like `em`), `vh` (local height),
  `deg`, `ms`, `s`.
- Math: simple `calc()`, `min()`, `max()`, `clamp()`.
- Colors: named colors, `transparent`, `#RGB`, `#RRGGBB`, HISE `#AARRGGBB`,
  HISE `0xAARRGGBB`, comma-form `rgb()`, `rgba()`, `hsl()`, `hsla()`.
- Images: `url(...)`, `linear-gradient(...)`, HISE Base64 JUCE Path backgrounds.
- Variables: `var(--name)` with no fallback argument.
- Transforms: `none`, `translate`, `translateX`, `translateY`, `scale`, `scaleX`,
  `scaleY`, `rotate`, `rotateZ`, `skew`, `skewX`, `skewY`.
- Timing: `linear`, `ease`, `ease-in`, `ease-out`, `ease-in-out`, `cubic-bezier()`,
  `steps()`.

## Important HISE differences

### Colors

HISE eight-digit hash colors are alpha-first:

```text
#AARRGGBB and 0xAARRGGBB
```

`#80FF0000` is 50% red, not browser CSS `#RRGGBBAA`. Preserve this because
HISEScript and JUCE use the same packed format. Use `rgba(255, 0, 0, 0.5)` when
converting browser CSS. Never emit `#RGBA` or browser trailing-alpha eight-digit hashes.

Unknown colors are rejected. Explicit transparency is `transparent`,
`rgba(...)`, or an alpha-first HISE value.

### Selectors and cascade

Do not assume browser cascade semantics. Descendant matching is limited to a flattened
ancestor condition. Multiple ancestor compounds, complex chains, and ancestor selectors
with pseudo states are unreliable. Avoid `>`, `+`, `~`, attribute selectors, namespaces,
`:not()`, `:is()`, `:where()`, `:has()`, and `:nth-child()`.

Universal `*` is not a normal cascade rule. Pseudo-class and pseudo-element specificity
is incomplete. Grouped selectors with different states can leak state values. Combined
pseudo states require exact state equality and can fail if another state bit is active.
Use separate simple rules instead of relying on specificity or subtle source ordering.

Use only double-colon pseudo-elements. Always provide `content` for generated boxes.
`:hidden`, `:root`, `:empty`, `:first-child`, and `:last-child` are component/flex states,
not general DOM states.

### Variables and global values

`var(--name, fallback)` is unsupported. Missing variables do not follow browser invalid-
at-computed-value behavior. Variables are stylesheet-global, not reliably pseudo-state
specific, and custom-property `!important` is not a full CSS cascade.

`initial`, `inherit`, and `unset` collapse to HISE's internal default/absent behavior.
`revert` and `revert-layer` are unsupported.

### Box model and flex

HISE margins are internal paint insets inside fixed component bounds. They do not reserve
external space between flex children. A child with `width: 100px; margin: 10px` still has
fixed 100px bounds and a smaller painted area. Use container `gap` for spacing.

`gap` is emulated with synthetic child margins and can differ at wrapping/reverse-flow
edges. `margin: auto` is only a narrow HISE centering path. `box-sizing` affects paint
geometry, not actual sizing. `overflow` does not clip or scroll. `position: relative`
offsets do nothing; `fixed` is local container-relative absolute positioning. There is no
browser-style `z-index` behavior.

`display: none` hides default-state flex-managed children. In LookAndFeel paint paths,
including `button:hover`, it only suppresses the CSS background paint pass. It does not
hide the JUCE component, remove layout space, or disable interaction. This is deliberate:
visibility must not be changed during painting.

`opacity` remains a HISE approximation and is inherited incorrectly; it does not composite
an entire component subtree. Do not depend on browser opacity layering.

### Backgrounds, borders, and text

`background-position` is deferred and non-standard. Avoid it for portable generated CSS.
URL background rendering can suppress normal background fill, borders, and box shadows.
Use one background image or gradient rather than layered backgrounds.

`border-style` tokens may parse, but rendering remains visually solid. Border width and color
are usable. `border-radius` supports standard one-, two-, three-, and four-value physical
ordering, but not slash-separated elliptical radii. Unequal corner rendering remains limited.

Font family fallback lists are unsupported; use one family name. `font` shorthand is unsafe.
`text-transform: capitalize` remains ineffective. Text uses one JUCE draw operation, so do
not expect browser line layout, wrapping, line-height, overflow, bidi, or decoration.
`content` supports only a narrow subset, despite ordinary-element replacement being standard.

### Transforms and transitions

Transforms are 2D paint transforms around the element center. There is no `transform-origin`.
They do not affect layout, clipping, or hit testing.

`scale(1.2)` means uniform `(1.2, 1.2)`. `translate(10px)` means `(10px, 0px)`.
Axis-specific functions affect only their named axis. `rotateZ()` is retained as a 2D alias.
`matrix()`, `translateZ()`, `scaleZ()`, `rotateX()`, and `rotateY()` are rejected as a whole
transform declaration.

Repeated `transform` declarations replace earlier values. Put composition in one declaration:

```css
transform: translateX(10px) scale(1.2);
```

Only one transition tuple is represented. Transition animation is primarily driven by
pseudo-state changes, not arbitrary property changes. Do not transition dimensions or flex
properties requiring continuous relayout. Repeated shadows replace earlier declarations;
combine shadows with commas. The parser warns when repeated shadow declarations override.

## HISE extensions

- `element(identifier)` selects an internal component identity.
- `:hidden` is a HISE state.
- `::before2` and `::after2` provide extra generated boxes.
- Standalone `::selection` targets text selection rendering.
- `#AARRGGBB`, `0xAARRGGBB`, runtime component variables, and Base64 JUCE Path backgrounds.
- `font-stretch` is a raw JUCE horizontal scale.
- `vertical-align` controls HISE text justification.
- `background-size: fill` is a HISE image/gradient sizing option.

## Do not generate

`@media`, `@keyframes`, conditional rules, radial/conic/repeating gradients, modern color
functions, `currentColor`, `image-set`, `attr`, `env`, counters, filters, shapes, fallback
font lists, CSS-wide `revert`, slash radius syntax, transition lists/longhands, 3D transforms,
`matrix`, `overflow` clipping, browser viewport assumptions, or stateful component visibility.
