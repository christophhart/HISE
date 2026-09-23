# simple_css compatibility report

## Purpose and method

This report describes the CSS-like language implemented by `hi_tools/simple_css`. It is intended to serve both as an agent cheat sheet and as an actionable compatibility backlog.

Derived resources:

- [Approved CSS bug-fix backlog](css_bug_backlog.md)
- [Post-fix AI CSS cheat sheet](../style/css.md)
- [MCP CSS dataset](../../tools/mcp_server/data/css.json)

Only executable C++ declarations and behavior were used as evidence. Existing Markdown documentation and source comments were not used as evidence. References use repository-relative C++ file paths and line numbers.

## Standards verification

This section verifies the report's CSS-standard claims against official W3C specifications. The quoted text is included so the assertion can be checked directly. Claims about HISE memory, caches, JUCE behavior, pointer validity, or native-control integration are implementation observations; no CSS specification can prove those repository-specific facts.

### Syntax and parsing

- **Final semicolons:** Confirmed. CSS Syntax 3 says, "Declarations are separated by semicolons." Its declaration-list grammar permits the final declaration to end at the closing block. [CSS Syntax 3, [syntax description](https://www.w3.org/TR/css-syntax-3/#syntax-description), [declaration lists](https://www.w3.org/TR/css-syntax-3/#parse-list-of-declarations)]
- **Error recovery:** Confirmed. CSS Syntax 3 says, "unknown syntax at any point causes the parser to throw away whatever declaration it's currently building, and seek forward until it finds a semicolon (or the end of the block)." [CSS Syntax 3, [error handling](https://www.w3.org/TR/css-syntax-3/#error-handling)]
- **Escapes:** Confirmed. CSS Syntax 3 says, "You can include any code point at all ... by escaping it" and "CSS escape sequences start with a backslash." [CSS Syntax 3, [syntax](https://www.w3.org/TR/css-syntax-3/#syntax-description), [escaping](https://www.w3.org/TR/css-syntax-3/#escaping)]
- **Case handling:** Confirmed. CSS Values and Units 4 says, "Keywords are identifiers and are interpreted ASCII case-insensitively." [CSS Values and Units 4, [pre-defined keywords](https://www.w3.org/TR/css-values-4/#keywords)]
- **String escapes:** Confirmed. CSS Values and Units 4 says, "Double quotes cannot occur inside double quotes, unless escaped." [CSS Values and Units 4, [strings](https://www.w3.org/TR/css-values-4/#strings)]
- **Unsupported components:** Confirmed. CSS Cascade 6 says, "CSS requires that the entire declaration be ignored" when an unsupported component value is present. [CSS Cascade 6, [partial implementations](https://www.w3.org/TR/css-cascade-6/#w3c-partial)]

### Selectors and cascade

- **Selector coverage:** Confirmed. Selectors 4 lists `E F`, `E > F`, `E + F`, `E ~ F`, `E[foo]`, `E:not(...)`, `E:is(...)`, `E:where(...)`, and `E:has(...)`. [Selectors 4, [overview](https://www.w3.org/TR/selectors-4/#overview)]
- **Universal selector:** Confirmed. Selectors 4 defines `*` as "any element." [Selectors 4, [universal selector](https://www.w3.org/TR/selectors-4/#the-universal-selector)]
- **Descendants:** Confirmed. Selectors 4 defines `E F` as "an F element descendant of an E element." [Selectors 4, [descendant combinator](https://www.w3.org/TR/selectors-4/#descendant-combinators)]
- **Structural pseudo-classes:** Confirmed. Selectors 4 defines `:first-child`, `:last-child`, `:root`, and `:empty` using the document tree and child relationships. [Selectors 4, [structural pseudo-classes](https://www.w3.org/TR/selectors-4/#structural-pseudos)]
- **Specificity:** Confirmed. Selectors 4 defines specificity as a tuple and includes pseudo-class and pseudo-element contributions. [Selectors 4, [specificity](https://www.w3.org/TR/selectors-4/#specificity-rules)]
- **Cascade ordering:** Confirmed. Cascade 6 says, "The cascade takes an unordered list of declared values ... sorts them by ... precedence" and says, "The last declaration in document order wins." [CSS Cascade 6, [cascading](https://www.w3.org/TR/css-cascade-6/#cascading), [sorting](https://www.w3.org/TR/css-cascade-6/#cascade-sort)]
- **CSS-wide keywords:** Confirmed. Values and Units 4 says all properties accept CSS-wide keywords; Cascade 5 defines `initial`, `inherit`, `unset`, `revert`, and `revert-layer` separately. [CSS Values and Units 4, [common keywords](https://www.w3.org/TR/css-values-4/#common-keywords)] [CSS Cascade 5, [defaulting keywords](https://www.w3.org/TR/css-cascade-5/#defaulting-keywords)]
- **Custom properties:** Confirmed. Values and Units 4 says custom property names are required to be dashed identifiers. Variables 1 defines `var()` fallback and cycle invalidation at computed-value time. [CSS Values and Units 4, [dashed identifiers](https://www.w3.org/TR/css-values-4/#dashed-idents)] [CSS Variables 1, [defining variables](https://www.w3.org/TR/css-variables-1/#defining-variables), [using variables](https://www.w3.org/TR/css-variables-1/#using-variables)]

### Box model, flex, and backgrounds

- **Four-value margin/padding:** Confirmed. Box Model 4 says, "If four values are given, they apply to the top, right, bottom, and left sides respectively." [CSS Box Model 4, [margin](https://www.w3.org/TR/css-box-4/#propdef-margin), [padding](https://www.w3.org/TR/css-box-4/#propdef-padding)]
- **Four-value border radius:** Confirmed. Backgrounds and Borders 3 defines the order as "top-left, top-right, bottom-right, and bottom-left" and defines slash-separated elliptical radii. [CSS Backgrounds and Borders 3, [border radius](https://www.w3.org/TR/css-backgrounds-3/#border-radius)]
- **Flex margins:** Confirmed. Flexbox 1 says, "Flex item margins ... participate in the flex layout" and auto margins absorb positive free space before alignment. [CSS Flexbox 1, [item margins](https://www.w3.org/TR/css-flexbox-1/#item-margins), [auto margins](https://www.w3.org/TR/css-flexbox-1/#auto-margins)]
- **Flex containers and out-of-flow items:** Confirmed. Flexbox 1 says `display:flex` generates a flex container and absolutely-positioned children do not participate in flex layout. [CSS Flexbox 1, [flex containers](https://www.w3.org/TR/css-flexbox-1/#flex-containers), [absolute children](https://www.w3.org/TR/css-flexbox-1/#abspos-items)]
- **Gap:** Confirmed. Box Alignment 3 defines gaps as gutters between boxes, distinct from margins. [CSS Box Alignment 3, [gaps](https://www.w3.org/TR/css-align-3/#gaps)]
- **Box areas:** Confirmed. Backgrounds and Borders 3 says each box has "a rectangular content area, a band of padding ... a border ... and a margin outside the border." [CSS Backgrounds and Borders 3, [introduction](https://www.w3.org/TR/css-backgrounds-3/#introduction)]
- **Opacity:** Confirmed. Color 4 says opacity applies "to the element as a whole, including its contents" and lists it as "Inherited: no." [CSS Color 4, [opacity](https://www.w3.org/TR/css-color-4/#transparency)]
- **Background layers:** Confirmed. Backgrounds and Borders 3 says the number of layers is determined by the comma-separated values in `background-image`, with corresponding position, size, and repeat lists. [CSS Backgrounds and Borders 3, [layers](https://www.w3.org/TR/css-backgrounds-3/#layering), [background image](https://www.w3.org/TR/css-backgrounds-3/#background-image)]
- **Units:** Confirmed. Values and Units 4 defines `em`, `rem`, viewport units, absolute units, and angle units. The report's local-resolution behavior is implementation-only. [CSS Values and Units 4, [font-relative lengths](https://www.w3.org/TR/css-values-4/#font-relative-lengths), [viewport lengths](https://www.w3.org/TR/css-values-4/#viewport-relative-lengths), [absolute lengths](https://www.w3.org/TR/css-values-4/#absolute-lengths), [angles](https://www.w3.org/TR/css-values-4/#angles)]
- **Math functions:** Confirmed. Values and Units 4 defines `calc()`, `min()`, `max()`, `clamp()`, rounding, trigonometric, exponential, and sign-related functions. The report's exact HISE arity and failure behavior is implementation-only. [CSS Values and Units 4, [math functions](https://www.w3.org/TR/css-values-4/#math)]

### Values, colors, fonts, content, and transforms

- **Color syntax:** Confirmed. Color 4 says `hsl()` specifies colors by "hue, saturation, and lightness" and lists `rgb()`, `hsl()`, `hwb()`, `lab()`, `lch()`, `oklab()`, and `oklch()` as color functions. [CSS Color 4, [color type](https://www.w3.org/TR/css-color-4/#color-type), [HSL](https://www.w3.org/TR/css-color-4/#the-hsl-notation)]
- **Alpha hex and `currentColor`:** Confirmed. Color 4 defines hexadecimal alpha notation and the `currentcolor` keyword. [CSS Color 4, [hex notation](https://www.w3.org/TR/css-color-4/#hex-notation), [currentcolor](https://www.w3.org/TR/css-color-4/#currentcolor-color)]
- **Gradients:** Confirmed. CSS Images 3 defines linear and radial gradients, while CSS Images 4 defines conic and repeating gradient forms. [CSS Images 3, [linear gradients](https://www.w3.org/TR/css-images-3/#linear-gradients), [radial gradients](https://www.w3.org/TR/css-images-3/#radial-gradients)] [CSS Images 4, [gradients](https://www.w3.org/TR/css-images-4/#gradients)]
- **Font fallback:** Confirmed. Fonts 4 says, "This property specifies a prioritized list of font family names or generic family names," and defines the values as a comma-separated list. [CSS Fonts 4, [font-family](https://www.w3.org/TR/css-fonts-4/#font-family-prop)]
- **Generated content:** Confirmed. Generated Content 3 says, "The content property dictates what is rendered inside an element or pseudo-element" and defines strings, images, counters, and `attr()` values. [CSS Generated Content 3, [content](https://www.w3.org/TR/css-content-3/#content-property), [content values](https://www.w3.org/TR/css-content-3/#content-values)]
- **Correction to the previous report:** `content` replacing ordinary element contents is standard CSS behavior, not a HISE-only extension. The HISE behavior remains partial because its accepted grammar and rendering are incomplete. [CSS Generated Content 3, [content](https://www.w3.org/TR/css-content-3/#content-property)]
- **Transforms:** Confirmed. Transforms 1 says transform effects are applied "after elements have been sized and positioned", defines a transform list and `transform-origin`, and says transforms do not affect layout other than overflow. [CSS Transforms 1, [module interactions](https://www.w3.org/TR/css-transforms-1/#module-interactions), [rendering model](https://www.w3.org/TR/css-transforms-1/#transform-rendering), [transform](https://www.w3.org/TR/css-transforms-1/#transform-property), [transform origin](https://www.w3.org/TR/css-transforms-1/#transform-origin-property)]
- **Distinct transform functions:** Confirmed. Transforms 1 defines `translateX`, `translateY`, `scaleX`, `scaleY`, `skewX`, `skewY`, `rotate`, and `matrix` as distinct functions with distinct matrices. [CSS Transforms 1, [transform functions](https://www.w3.org/TR/css-transforms-1/#transform-functions)]
- **Transform geometry:** Confirmed. Transforms 1 says transforms affect client rectangles and overflow while not changing normal flow. Therefore the report's statement that transforms do not affect layout is correct only when it means normal flow; the HISE hit-testing finding is implementation-only. [CSS Transforms 1, [module interactions](https://www.w3.org/TR/css-transforms-1/#module-interactions), [rendering model](https://www.w3.org/TR/css-transforms-1/#transform-rendering)]
- **Transitions:** Confirmed. Transitions 1 says, "Each of the transition properties accepts a comma-separated list," and defines separate property, duration, timing-function, and delay properties. [CSS Transitions 1, [transitions](https://www.w3.org/TR/css-transitions-1/#transitions), [transition property](https://www.w3.org/TR/css-transitions-1/#transition-property-property)]
- **Timing functions:** Confirmed. CSS Easing 2 defines cubic Bezier and step functions, including distinct step position modes. [CSS Easing Functions 2, [timing functions](https://www.w3.org/TR/css-easing-2/#timing-functions), [step functions](https://www.w3.org/TR/css-easing-2/#step-functions)]
- **Cursor:** Confirmed. UI 4 defines a larger predefined cursor grammar and marks `cursor` as inherited. [CSS UI 4, [cursor](https://www.w3.org/TR/css-ui-4/#cursor)]
- **`@import` and `@font-face`:** Confirmed. Values and Units 4 says `@import url("base-theme.css")` and `@import "base-theme.css"` have the same meaning. Fonts 4 defines the `src` descriptor, multiple source items, `format()`, and `local()`. [CSS Values and Units 4, [URLs](https://www.w3.org/TR/css-values-4/#urls)] [CSS Fonts 4, [font resources](https://www.w3.org/TR/css-fonts-4/#font-resources)]

### Claim status rules

- **Confirmed:** The HISE behavior differs from a quoted CSS requirement or definition.
- **Correction:** The report's earlier standards interpretation was too broad or incorrect; the correction is stated inline above.
- **Implementation-only:** The claim concerns C++ behavior and must be checked against the cited repository source, not a CSS specification.

The remaining entries in the compliance backlog are implementation-only unless they appear in the verification groups above. In particular, buffer overwrites, unchecked reads, null dereferences, stale caches, JUCE FlexBox mapping, paint order, component visibility, and native-control affordances cannot be verified by CSS documentation.

Support labels used below:

- **Effective**: the parser stores the declaration and an executable path consumes it.
- **Partial**: an executable path exists, but the accepted syntax or semantics differ materially from CSS.
- **Recognized only**: the keyword database accepts the name, but no useful implementation was found.
- **Warned extension**: executable behavior exists, but the keyword database reports the property or syntax as unsupported.

The keyword database is not a reliable support boundary. Unknown one-token properties are normally stored despite a warning, while unknown multi-token properties are normally discarded. Conversely, some properties used by executable code are absent from the keyword database. [CssParser.cpp:2065-2089, 2241-2267] [LanguageManager.cpp:68-105]

## Compatibility summary

`simple_css` is a JUCE component styling and flex layout system with CSS-like syntax. It is not a standards-conforming CSS parser, cascade, box model, or rendering engine.

The most reliable subset is:

- Type, class, ID, universal, and compound selectors.
- A single, simple ancestor condition.
- Basic source-order cascade and a whitespace-separated `!important` token.
- A fixed set of pseudo states used by JUCE components.
- JUCE flexbox layout, dimensions, padding, basic painting, fonts, shadows, images, and state transitions.
- Custom properties without fallback semantics.
- Basic `@font-face` and `@import url(...)` support through a host data provider.

Standard CSS cannot be copied into this system without review. Important incompatibilities include mandatory trailing semicolons, case-sensitive names, incorrect shorthand mappings, a non-standard cascade, incomplete selector chains, incorrect color and transform behavior, and accepted properties that do nothing.

## Supported stylesheet syntax

### Rules and declarations

Ordinary rules use this shape:

```css
selector, selector {
	property: value;
}
```

- Every declaration, including the final declaration before `}`, requires `;`. [CssParser.cpp:1390-1392, 1638-1656]
- Property names contain only letters, digits, `-`, and `_`. CSS escapes are not implemented. [CssParser.cpp:1341-1349]
- Values are split at whitespace except for quoted strings and parenthesized expressions. [CssParser.cpp:1350-1420]
- Quotes are removed from stored values. Backslash escape handling is not implemented. [CssParser.cpp:1356-1387]
- Block comments using `/* ... */` are skipped. No other comment syntax is accepted. [CssParser.cpp:1267-1289]
- Parsing stops at the first syntax error. Invalid declarations are not skipped in the browser CSS manner. [CssParser.cpp:1583-1670]
- Property names, keywords, selector names, color names, and function prefixes are generally case-sensitive. [CssParser.cpp:1711-1719, 1830-1872]
- `!important` works only as a separate, exact, final value token, as in `red !important`. [CssParser.cpp:2065-2079]

### Selectors

| Syntax | Support | Actual behavior |
|---|---|---|
| `type` | Effective | Matches the component's inferred or assigned type. |
| `.class` | Effective | Matches an assigned component class. |
| `#id` | Effective | Matches the assigned component ID. |
| `*` | Partial | Parses, but universal rules are not normal cascade participants. |
| `type.class#id` | Effective | All selectors in the compound must match the same target component. |
| `A B` | Partial | Tests `B` on the target and searches a flattened set of all ancestor selectors for `A`. |
| `A, B` | Partial | Works when grouped members use compatible pseudo states. Mixed states leak between members. |
| `element(identifier)` | Warned extension | Selects a component through an internal pointer-derived identifier. |
| `::selection` | Non-standard special case | A standalone selector used only for text editor selection colors. |

Selector parsing is implemented in [CssParser.cpp:1499-1580]. Compound and ancestor matching are implemented in [HelperClasses.cpp:345-390, 449-458] and [StyleSheet.cpp:518-539].

Recognized element names are:

`button`, `body`, `div`, `select`, `img`, `input`, `hr`, `label`, `table`, `th`, `tr`, `td`, `p`, `progress`, `scrollbar`, `h1`, `h2`, `h3`, `h4`. [LanguageManager.cpp:64-67]

Other type names parse but produce a warning.

Unsupported selector syntax includes `>`, `+`, `~`, attribute selectors, namespace selectors, escaped identifiers, nesting with `&`, and functional pseudo-classes such as `:not()`, `:is()`, `:where()`, `:has()`, and `:nth-child()`. [CssParser.cpp:1326-1350, 1499-1575]

### Pseudo-classes and pseudo-elements

Parsed pseudo-classes:

- `:first-child`
- `:last-child`
- `:root`
- `:hover`
- `:active`
- `:focus`
- `:disabled`
- `:hidden`
- `:checked`
- `:empty`

Parsed pseudo-elements:

- `::before`
- `::after`
- `::before2` (non-standard)
- `::after2` (non-standard)

The parser mappings are in [CssParser.cpp:1433-1486]. Generic component state generation covers checked, first, last, disabled, hover, active, and focus; other states are supplied only by selected component paths or manually. [Renderer.cpp:710-747]

Pseudo-elements require `content` before an area is created. [StyleSheet.cpp:1555-1591]

`:empty`, `::before2`, and `::after2` are executable but absent from the warning whitelist. [LanguageManager.cpp:64-65] [CssParser.cpp:1443-1480]

### At-rules

| Rule | Support | Accepted behavior |
|---|---|---|
| `@font-face` | Partial | Reads `font-family` and one `src: url(...)`, then asks the collection data provider to load the font. |
| `@import` | Partial | Reads one value and imports only when that value is `url(...)`. |

At-rule execution is in [StyleSheet.cpp:1162-1248]. URL extraction is in [StyleSheet.cpp:2710-2720]. Any `@name` can parse, but only `font-face` and `import` have semantics. [CssParser.cpp:1516-1522] [LanguageManager.cpp:64-66]

`@media`, `@supports`, `@container`, `@layer`, `@keyframes`, `@property`, page rules, and conditional import qualifiers are unsupported.

## Supported property reference

The exact property-name whitelist used for parser warnings and editor metadata is:

```text
::selection
align-items align-content align-self
background background-color background-size background-position background-image
border border-width border-style border-color
border-radius border-top-left-radius border-top-right-radius
border-bottom-left-radius border-bottom-right-radius
bottom box-shadow box-sizing color content caret-color cursor display
flex-wrap flex-direction flex-grow flex-shrink flex-basis
font-family font-size font-weight font-stretch gap height justify-content left
letter-spacing margin margin-top margin-left margin-right margin-bottom
min-width max-width min-height max-height opacity object-fit order overflow
padding padding-top padding-left padding-right padding-bottom
position right text-align text-transform text-shadow transition transform
top vertical-align width
```

This whitelist comes from [LanguageManager.cpp:68-105]. It controls warnings, not runtime support. `::selection` is incorrectly listed as a property even though executable code treats it as a special selector. Effective warned extensions are listed separately below.

### Layout and box model

| Property | Status | Accepted and effective behavior |
|---|---|---|
| `display` | Partial | `none` suppresses CSS painting; flex-managed children can also be hidden. `flex` configures JUCE FlexBox. Other values have no distinct layout. |
| `position` | Partial | `absolute` and `fixed` are removed from flex flow. `fixed` behaves like local absolute positioning. `relative` does not apply offsets. |
| `top`, `right`, `bottom`, `left` | Partial | Used for absolute/fixed bounds. They do not implement normal relative-position semantics. |
| `width`, `height` | Effective | Numeric expressions set flex item or explicit bounds. `auto` has component-specific measurement behavior. |
| `min-width`, `max-width`, `min-height`, `max-height` | Partial | Set JUCE FlexItem constraints. Height percentages are evaluated with the wrong axis in one path. |
| `margin` | Partial | Expands to physical longhands. Four-value order is wrong. Auto margins are incomplete. |
| `margin-top`, `margin-right`, `margin-bottom`, `margin-left` | Partial | Reduce the painted area. They are not installed as normal child FlexItem margins. |
| `padding` | Partial | Expands to physical longhands. Four-value order is wrong. |
| `padding-top`, `padding-right`, `padding-bottom`, `padding-left` | Effective | Reduce content and flex-container layout areas in selected paths. |
| `box-sizing` | Partial | `border-box` changes the background fill path, not width/height sizing. |
| `opacity` | Partial | Multiplies simple_css brushes and images, not the composited component subtree. It is incorrectly inherited. |
| `overflow` | Recognized only | Stored but no clipping, scrolling, or overflow layout behavior was found. |

Dimension and area behavior is in [StyleSheet.cpp:1366-1552, 2611-2707], flex layout use is in [FlexboxComponent.cpp:847-948], and box painting is in [Renderer.cpp:752-827].

### Flexbox

| Property | Status | Accepted values or behavior |
|---|---|---|
| `flex-direction` | Effective | `row`, `row-reverse`, `column`, `column-reverse`. |
| `flex-wrap` | Effective | `nowrap`, `wrap`, `wrap-reverse`. |
| `justify-content` | Partial | `flex-start`, `flex-end`, `center`, `space-between`, `space-around`. |
| `align-items` | Partial | `stretch`, `flex-start`, `flex-end`, `center`. |
| `align-content` | Partial | `stretch`, `flex-start`, `flex-end`, `center`. |
| `align-self` | Partial | `auto`, `flex-start`, `flex-end`, `center`, `stretch`. |
| `flex-grow` | Effective | Numeric expression assigned to JUCE FlexItem. |
| `flex-shrink` | Effective | Numeric expression assigned to JUCE FlexItem. |
| `flex-basis` | Partial | Numeric expression, evaluated without the real container context. |
| `order` | Effective | Integer assigned to JUCE FlexItem. |
| `gap` | Partial | One uniform value emulated with half-margins around children. |

Container mapping is in [StyleSheet.cpp:2579-2592], item mapping in [StyleSheet.cpp:2693-2705], and gap emulation in [FlexboxComponent.cpp:863-922].

Unsupported flex features include `flex`, `flex-flow`, `row-gap`, `column-gap`, `place-content`, `place-items`, `place-self`, baseline alignment, `space-evenly`, intrinsic flex basis keywords, and browser min-content sizing.

### Backgrounds, colors, and images

| Property | Status | Accepted and effective behavior |
|---|---|---|
| `background` | Partial | Extracts color or linear-gradient tokens. It is not the standard layered shorthand. |
| `background-color` | Effective | Solid color, linear gradient, or partial `color-mix()`. |
| `background-image` | Partial | One `url(...)`, a linear gradient rerouted to background color, or a non-standard Base64 JUCE path. |
| `background-size` | Partial | Image placement keyword or one scalar applied to both gradient axes. |
| `background-position` | Partial | One scalar applied to both axes with incorrect axis contexts. |
| `object-fit` | Partial | `fill`, `contain`, `cover`, `none`, `scale-down` for raster images. |
| `color` | Effective | Text and selected control colors. |
| `caret-color` | Partial | Applied to JUCE TextEditor carets. Standard `auto` and `currentColor` semantics are absent. |

Brush and image behavior is in [StyleSheet.cpp:2078-2265] and [Renderer.cpp:796-828, 942-1042]. Image fitting is in [Renderer.cpp:957-997].

There is no image repeat, attachment, origin, clip, blend mode, multiple layers, `object-position`, `image-set()`, radial gradient, conic gradient, or repeating gradient support.

### Borders and shadows

| Property | Status | Accepted and effective behavior |
|---|---|---|
| `border` | Partial | Infers width, style, and color tokens. Only width and color affect rendering. |
| `border-width` | Broken | A normal length is stored as `border-width-width`. |
| `border-style` | Broken/ineffective | A recognized style is stored as `border-style-style`; no renderer reads border style. |
| `border-color` | Effective | Uniform solid or gradient brush. |
| `border-radius` | Partial | One, two, or four values. No three-value or slash syntax. Four-value bottom corners are reversed. |
| `border-top-left-radius`, `border-top-right-radius`, `border-bottom-right-radius`, `border-bottom-left-radius` | Partial | Accepted, but unequal nonzero radii are reduced to one maximum radius plus corner enable flags. |
| Per-side border width/color longhands | Warned extension | `border-top/right/bottom/left-width` and `-color` are consumed by rendering but absent from the property whitelist. Width longhands are vulnerable to duplicate suffixing during parsing. |
| `box-shadow` | Partial | Multiple outer/inset shadows, offsets, blur, spread, color, and transition interpolation. |
| `text-shadow` | Partial | Multiple shadows parse; the text renderer requests only the outer set. |

Border token inference is in [CssParser.cpp:1782-1827, 2230-2248]. Border rendering is in [Renderer.cpp:769-827] and [HelperClasses.cpp:669-780]. Radius conversion is in [CssParser.cpp:2158-2182] and [StyleSheet.cpp:1298-1345]. Shadow parsing is in [CssParser.cpp:683-888].

Recognized border style tokens are `solid`, `dotted`, `outset`, and `dashed`, but all rendered borders are effectively solid.

### Text and fonts

| Property | Status | Accepted and effective behavior |
|---|---|---|
| `font-family` | Partial | One exact family name, `sans-serif`, `monospace`, or a custom loaded font. Quoted multi-word names survive as one token. No fallback list. |
| `font-size` | Partial | General numeric expression. Percentages use geometry rather than inherited font size. |
| `font-weight` | Partial | `400` is normal; `bold`, `bolder`, and `500` through `900` all map to one bold flag. |
| `font-style` | Warned extension | `normal` and `italic` are consumed, but the property is absent from the property whitelist. |
| `font-stretch` | Non-standard | Numeric horizontal scale rather than normal CSS font matching. |
| `letter-spacing` | Partial | Converted to a JUCE extra-kerning factor relative to font size. |
| `text-align` | Partial | `start`, `left`, `end`, `right`, `center` map to JUCE justification. |
| `vertical-align` | Non-standard subset | `top`, `text-top`, `bottom`, `text-bottom`, `middle` alter JUCE text justification. |
| `text-transform` | Partial | `uppercase` and `lowercase` work; `capitalize` is accepted but is a no-op. |
| `content` | Partial | Replaces ordinary rendered text and enables pseudo-element boxes. No counters, `attr()`, quote semantics, or content item lists. |

Font behavior is in [StyleSheet.cpp:1901-1947], text alignment in [StyleSheet.cpp:1788-1820], content and transformation in [StyleSheet.cpp:1872-1898], and rendering in [Renderer.cpp:1056-1082].

The `font` shorthand is not implemented. Multi-token font-category declarations enter an assertion path and are discarded. [CssParser.cpp:2225-2229]

No effective support was found for `line-height`, `white-space`, `word-spacing`, wrapping control, text overflow, text decoration, text indent, bidi/direction, font variants, font features, or variable font axes.

### Interaction and transitions

| Property | Status | Accepted and effective behavior |
|---|---|---|
| `cursor` | Partial | `default`, `pointer`, `wait`, `crosshair`, `text`, `copy`, `grabbing`. Applied only in selected component setup paths. |
| `transition` | Partial | Either one duration for all properties or `property duration [timing] [delay]`. Only one tuple is represented. |
| `transform` | Broken/partial | A stack of 2D affine functions painted around the center. Axis-specific and 3D names do not have CSS semantics. |

Cursor conversion is in [StyleSheet.cpp:1848-1870]. Transition parsing is in [CssParser.cpp:1874-1950, 2127-2156], transition dispatch in [Animator.cpp:171-274], and transform application in [CssParser.cpp:549-592] and [StyleSheet.cpp:2029-2075].

There is no effective `visibility`, `pointer-events`, `user-select`, `z-index`, `filter`, `backdrop-filter`, `animation`, or keyframe support.

### Effective internal or warned properties

The following names have executable or merge behavior but are not reliable standard properties in this implementation:

- `src`: consumed by `@font-face`, but absent from the property whitelist and therefore warned. [StyleSheet.cpp:1203-1213] [LanguageManager.cpp:68-105]
- `font-style`: consumed but warned. [StyleSheet.cpp:1918-1921]
- Per-side border width and color longhands: consumed but warned. [StyleSheet.cpp:301-367]
- `all`: has a non-standard merge-time implementation that only rewrites properties already present in the destination. [StyleSheet.cpp:203-220]
- `x`, `y`, and names starting with `layout`: classified by the parser but no useful standard CSS consumer was found. [CssParser.cpp:1835-1857]
- Any one-token unknown property can be stored, but it has no effect unless host code explicitly queries it. [CssParser.cpp:2244-2267]

## Supported values, units, and functions

### Numeric units

| Syntax | Actual behavior |
|---|---|
| Bare number | Accepted broadly, including places where CSS requires a unit. |
| `px` | Treated as an absolute number. The implementation actually accepts any suffix ending in `x`. |
| `%` | Relative to a caller-selected rectangle width or height. |
| `em` | Multiplied by the supplied default font size. |
| `rem` | Accidentally ends with `em`, so behaves like `em`, not root em. |
| `vh` | Relative to the supplied local rectangle height, not a browser/application viewport. |
| `deg` | Converted to radians for expression users. |
| `auto` | Returns context-specific dimensions or custom centering/measurement behavior. |

Literal evaluation is in [CssParser.cpp:890-914]. Unsupported units commonly degrade to their numeric prefix instead of invalidating the declaration.

Unsupported units include `vw`, `vmin`, `vmax`, dynamic viewport units, `ch`, `ex`, `cap`, `ic`, `lh`, `rlh`, `cm`, `mm`, `Q`, `in`, `pt`, `pc`, `rad`, `grad`, and `turn`.

### Expression functions

| Function | Support | Limitations |
|---|---|---|
| `calc()` | Partial | Exactly two children and one operator are meaningfully evaluated. |
| `min()` | Partial | Evaluates a child list. |
| `max()` | Broken for all-negative input | Starts from the smallest positive float instead of the most-negative float. |
| `clamp()` | Partial | Requires exactly three children. |

Supported `calc()` operators are `+`, `-`, `*`, and `/`. Division by a negative divisor incorrectly returns zero. Parse failures return the default font size, commonly 16, rather than invalidating the declaration. [CssParser.cpp:946-1003, 1038-1158]

Missing numeric functions include `round()`, `mod()`, `rem()`, `abs()`, `sign()`, trigonometric functions, `pow()`, `sqrt()`, and `hypot()`.

### Color functions and values

| Syntax | Support | Limitations |
|---|---|---|
| Named colors | Partial | Large hardcoded, case-sensitive table. Unknown names become transparent black. |
| `transparent` | Effective | Transparent black. |
| `#RGB` | Effective | Expanded to opaque RGB. |
| `#RRGGBB` | Effective | Treated as opaque RGB. |
| `#RGBA` | Broken | Not expanded using CSS short-alpha semantics. |
| `#RRGGBBAA` | Broken | Not converted from CSS trailing-alpha order to JUCE ARGB order. |
| `0xAARRGGBB` | Non-standard extension | Accepted directly. |
| `rgb()` | Partial | Comma-separated integer channels only. Percent and modern space/slash syntax are wrong or unsupported. |
| `rgba()` | Partial | Comma-separated integer channels plus floating alpha. |
| `hsl()` / `hsla()` | Broken | H, S, and L are treated as 0-255 channels divided by 255, not hue and percentages. |
| `linear-gradient()` | Partial | Direction/angle plus color stops. Integer degrees and integer percentage stops. |
| `color-mix()` | Partial | Ignores color space and second weight; interpolates using only the first weight. |

Color parsing is in [CssParser.cpp:38-265], gradient parsing in [CssParser.cpp:358-488], and `color-mix()` in [StyleSheet.cpp:2106-2125].

Missing color/image functions and values include `radial-gradient()`, `conic-gradient()`, repeating gradients, `hwb()`, `lab()`, `lch()`, `oklab()`, `oklch()`, `light-dark()`, relative colors, `currentColor`, `image-set()`, `cross-fade()`, and `element()` as an image source.

### Variables and URL functions

| Function | Support | Limitations |
|---|---|---|
| `var(--name)` | Partial | Whole-value or textual embedded replacement. No fallback argument, state-aware variable cascade, cycle handling, or CSS invalid-at-computed-value behavior. |
| `url(...)` | Partial | Used by background images, `@font-face`, and `@import` through different host paths. |

Variable substitution is in [HelperClasses.cpp:537-570]. Custom declarations are stored in [CssParser.cpp:2003-2011]. URL extraction is in [StyleSheet.cpp:2710-2720].

Missing general value functions include `env()` and `attr()`.

Other standard function families have no consuming property implementation: generated-content `counter()` and `counters()`; font source `local()` and `format()`; shape functions such as `inset()`, `circle()`, `ellipse()`, `polygon()`, and `path()`; filter functions such as `blur()`, `brightness()`, `contrast()`, `drop-shadow()`, `grayscale()`, `hue-rotate()`, `invert()`, `opacity()`, `saturate()`, and `sepia()`; and the piecewise `linear()` easing function.

### Transform functions

Recognized names:

`matrix`, `translate`, `translateX`, `translateY`, `translateZ`, `scale`, `scaleX`, `scaleY`, `scaleZ`, `rotate`, `rotateX`, `rotateY`, `rotateZ`, `skew`, `skewX`, `skewY`, and `none`. [LanguageManager.cpp:122-124]

Actual behavior:

- `matrix()` is recognized but performs no operation.
- Every translate variant uses the same 2D translation operation.
- Every scale variant uses the same 2D scaling operation.
- Every rotate variant uses the same 2D rotation operation.
- Every skew variant uses the same 2D shear operation.
- A one-argument function reuses the first value as the second value. For example, `translateX(10px)` translates both X and Y.
- All transforms paint around the element center. `transform-origin` is unsupported.
- Transforms do not change layout bounds, clipping calculations, or hit testing.

Implementation: [CssParser.cpp:549-592, 600-680] [StyleSheet.cpp:2029-2075] [Renderer.cpp:764-767].

Missing transform functions and features include usable `matrix()`, `matrix3d()`, `translate3d()`, `scale3d()`, `rotate3d()`, `perspective()`, transform origin, transform box, and 3D composition.

### Timing functions

Supported:

- `linear`
- `ease`
- `ease-in`
- `ease-out`
- `ease-in-out`
- `cubic-bezier(x1, y1, x2, y2)`
- `steps(count, mode)` with `jump-start`, `jump-end`, `jump-both`, `jump-none`, `start`, or `end`

Implementation: [CssParser.cpp:1874-1950]. `ease-out` works but is absent from editor metadata. [LanguageManager.cpp:118]

`jump-both` and `jump-none` use the same rounding behavior rather than their distinct CSS definitions. Cubic-bezier X control points are not validated to the CSS range.

## Cascade, inheritance, and state behavior

### Specificity

The score counts IDs, classes, and types on the target compound. [HelperClasses.cpp:240-288]

Differences from CSS:

- Pseudo-classes do not contribute class specificity.
- Pseudo-elements do not contribute type specificity.
- Ancestor compound selectors do not contribute ID/class/type specificity.
- Any rule with an ancestor condition outranks every rule without one before ID/class/type scores are compared. [HelperClasses.cpp:290-315]
- Specificity is derived by comparing the rule to selectors present on the component, rather than solely from selector syntax.

### Inheritance

Automatically copied properties are:

`color`, `cursor`, `font-family`, `font-size`, `font-style`, `font-variant`, `font-weight`, `font`, `letter-spacing`, `opacity`, `text-align`, `text-transform`. [LanguageManager.cpp:60-62]

This list is incomplete relative to CSS and incorrectly includes `opacity`. Parent importance is retained during copying, so an inherited parent `!important` can incorrectly prevent a normal declaration specified directly on the child. [StyleSheet.cpp:227-283]

### CSS-wide keywords

`initial`, `unset`, and `inherit` are all converted to one internal `default` value that behaves as absent. [CssParser.cpp:2025-2058] [HelperClasses.cpp:523-535]

`revert` and `revert-layer` are unsupported. The three accepted keywords do not implement their distinct standard meanings.

### Custom property cascade

- Variables are stored once per stylesheet, not per pseudo state.
- Later pseudo-state variable declarations overwrite the same shared value.
- `!important` is ignored for custom properties.
- Missing variables normally become an empty string.
- `var(--x, fallback)` treats `x, fallback` as the variable name.
- Embedded cyclic substitutions can repeat indefinitely.

Implementation: [CssParser.cpp:2003-2011] [StyleSheet.cpp:196-225, 285-299] [HelperClasses.cpp:537-570].

## Non-standard extensions

These features should be documented as HISE extensions rather than CSS:

- `element(identifier)` selector for an internal component identity. [CssParser.cpp:1548-1555]
- `::before2` and `::after2` extra generated boxes. [CssParser.cpp:1443-1452]
- `:hidden` state bit, which requires manual state injection. [CssParser.cpp:1467-1469] [Renderer.cpp:710-747]
- Standalone `::selection` represented as a special class selector. [CssParser.cpp:1523-1529] [StyleSheet.cpp:1759-1765]
- `0xAARRGGBB` color values. [CssParser.cpp:245-248]
- Base64 JUCE paths in `background-image`, used as element geometry. [StyleSheet.cpp:1311-1322]
- Runtime custom variables such as control name, value, and progress supplied by host components.
- `font-stretch` as a raw JUCE horizontal scale.
- `vertical-align` as a JUCE text justification control.
- `content` replacing text on ordinary elements is standard CSS behavior, but this implementation supports only a narrow subset of the standard content grammar. [StyleSheet.cpp:1872-1879] [CSS Generated Content 3](https://www.w3.org/TR/css-content-3/#content-property)
- `background-size: fill`, shared with the `object-fit` keyword set. [LanguageManager.cpp:120-121]

## Compliance and friction backlog

### Critical correctness and safety issues

1. **Transform argument memory overwrite.** Transform storage has two values, but parsing writes arguments before limiting the count to two. More than two arguments can write out of bounds. [CssParser.h:79-81] [CssParser.cpp:638-675]
2. **Transform name buffer overflow.** A 20-byte name buffer can write its terminator at index 20. [CssParser.cpp:609-628]
3. **Unchecked parser boundary reads.** Whitespace skipping, keyword scanning, and selector whitespace checks can dereference at or beyond the input end. [CssParser.cpp:1259-1265, 1341-1348, 1564-1567]
4. **Empty `!important` value access.** A declaration whose only token is `!important` removes the token and later indexes an empty array. [CssParser.cpp:2071-2079, 2244-2247]
5. **Empty flex list access.** First/last-child calculation reads the first and last entries without checking for an empty in-flow child list. [FlexboxComponent.cpp:816-845, 880]
6. **Null background image target dereference.** URL rendering dereferences the current component while some look-and-feel renderers deliberately have no component target. [Renderer.cpp:800] [CSSLookAndFeel.cpp:303-307]
7. **Null image and stream assumptions.** Zero-sized images can cause division by zero, and asynchronous URL image loading uses a stream without a null check. [Renderer.cpp:969-978] [FlexboxComponent.cpp:1053-1060]
8. **Cyclic embedded variables can loop forever.** Replacement continues while `var(--...)` remains, without cycle detection. [HelperClasses.cpp:549-566]

### Parser and declaration bugs

1. **Final semicolons are mandatory.** Standard CSS permits the last semicolon before `}` to be omitted. [CssParser.cpp:1390-1392, 1638-1656]
2. **No error recovery.** One malformed declaration aborts the remaining stylesheet. [CssParser.cpp:1583-1670]
3. **Four-value margin and padding order is wrong.** The implementation assigns top, bottom, left, right from tokens that CSS defines as top, right, bottom, left. [CssParser.cpp:2116-2123]
4. **Four-value border-radius bottom corners are reversed.** Standard order is top-left, top-right, bottom-right, bottom-left; the implementation assigns token 3 to bottom-left and token 4 to bottom-right. [CssParser.cpp:2174-2180]
5. **Three-value and elliptical border-radius are discarded or unsupported.** [CssParser.cpp:2158-2182]
6. **Border width/style longhands are mangled.** `border-width: 1px` becomes `border-width-width`; style has the same duplicate suffix problem. [CssParser.cpp:1782-1818, 2230-2248]
7. **Unknown property behavior depends on token count.** One-token unknown values survive; multi-token unknown values disappear. [CssParser.cpp:2087-2267]
8. **Multi-token font declarations are discarded.** This affects unquoted family names and the `font` shorthand. [CssParser.cpp:2225-2229]
9. **Case sensitivity differs from CSS.** Common ASCII case-insensitive names and keywords require exact case. [CssParser.cpp:1711-1719, 1830-1872]
10. **`!important` requires preceding whitespace.** `red!important` is not important. [CssParser.cpp:2065-2079]
11. **Quoted strings lack CSS escape semantics.** [CssParser.cpp:1356-1387]
12. **Parenthesis scanning does not honor quoted parentheses.** A parenthesis inside a string can alter nesting. [CssParser.cpp:1393-1413]
13. **Repeated transform and shadow declarations append.** Standard repeated declarations replace earlier declarations. [CssParser.cpp:2003-2036]
14. **Invalid expressions become a default font size.** They are not treated as invalid declarations. [CssParser.cpp:1137-1158]

### Selector and cascade bugs

1. **Descendant combinator after a pseudo-class is lost.** In `button:hover .child`, pseudo parsing consumes the whitespace before the delimiter is recorded. [CssParser.cpp:1564-1574]
2. **Only one ancestor split is represented.** `A B C` becomes approximately ancestor `A` plus target compound `B.C`. [HelperClasses.cpp:345-373]
3. **Ancestor compounds can be assembled from different ancestors.** All ancestor selectors are flattened into one list. [StyleSheet.cpp:528-537] [HelperClasses.cpp:375-390]
4. **Grouped pseudo selectors leak states.** `.a:hover, .b` installs declarations in both hover and default states for the shared rule. [CssParser.cpp:1973-1987, 2013-2059]
5. **Combined pseudo states require exact equality.** `:hover:active` fails if focus or another state is also active. [HelperClasses.cpp:625-660]
6. **State conflict resolution depends on numeric state order.** It is not a complete specificity/source-order cascade per state. [HelperClasses.cpp:639-650] [StyleSheet.cpp:266-274]
7. **Universal rules are not normal rules.** They mainly copy the fixed inherited-property list, so `* { margin: 10px; }` does not reliably apply margin. [StyleSheet.cpp:551-560, 658-659, 279-283]
8. **Universal equality is overbroad.** `Selector::operator==` returns true when either operand is universal. [HelperClasses.cpp:227-237]
9. **Ancestor presence dominates specificity.** A low-specificity descendant selector can outrank an ID selector without an ancestor condition. [HelperClasses.cpp:290-315]
10. **Pseudo-class, pseudo-element, and ancestor specificity is omitted.** [HelperClasses.cpp:240-288]
11. **Standalone `:root` does not parse as a pseudo-class selector.** A base selector is required. [CssParser.cpp:1523-1529]
12. **Legacy `:before` and `:after` aliases do not create pseudo-elements.** Only double-colon forms do. [CssParser.cpp:1438-1481]
13. **`:first-child` and `:last-child` are flex-layout states.** They exclude invisible and out-of-flow children and can follow flex order rather than document child order. [FlexboxComponent.cpp:793-895]
14. **`:empty`, `:root`, and `:hidden` are target-specific or manual.** They are not general DOM-like states. [Renderer.cpp:710-747]
15. **Parent `!important` can incorrectly defeat a specified child value after inheritance copying.** [StyleSheet.cpp:227-283]

### Variables and at-rule gaps

1. **`var()` fallback is unsupported.** [HelperClasses.cpp:537-570]
2. **Variables have no pseudo-state or `!important` cascade.** [CssParser.cpp:2003-2011]
3. **Missing variables become empty text instead of invoking CSS invalid-at-computed-value behavior.** [HelperClasses.cpp:537-570]
4. **`@import "file.css"` is ineffective.** Runtime extraction accepts only `url(...)`. [StyleSheet.cpp:1203-1217, 2710-2720]
5. **Imported parse failures are discarded.** [StyleSheet.cpp:1220-1237]
6. **Isolated collections process fonts but not imports.** [StyleSheet.cpp:1162-1196]
7. **Unknown flat at-rules can parse and then do nothing.** [CssParser.cpp:1516-1522] [StyleSheet.cpp:1200-1238]
8. **`@font-face src` warns despite being consumed.** Common multiple-source and descriptor syntax is unsupported. [LanguageManager.cpp:68-105] [StyleSheet.cpp:1203-1213]
9. **Media queries and all conditional rules are absent.** [LanguageManager.cpp:64-66] [StyleSheet.cpp:1162-1248]

### Box model and flexbox bugs

1. **Child CSS margins do not create normal flex spacing.** They reduce paint geometry rather than populating `FlexItem::margin`. [Renderer.cpp:763] [StyleSheet.cpp:2611-2707]
2. **`box-sizing` does not size boxes.** It changes only background fill geometry. [Renderer.cpp:774-794]
3. **`position: relative` offsets do nothing.** [FlexboxComponent.cpp:924-936]
4. **`position: fixed` is container-relative absolute.** [FlexboxComponent.cpp:924-936]
5. **Absolute/fixed children are forced to the front on resize.** There is no z-index. [FlexboxComponent.cpp:404-408]
6. **`min-height` and `max-height` percentages use width.** [StyleSheet.cpp:1502-1505]
7. **One `auto` calculation swaps width and height.** [StyleSheet.cpp:1373-1383]
8. **`margin:auto` is only a narrow custom centering path.** [StyleSheet.cpp:1373-1440]
9. **Gap is synthetic margin, not CSS gap.** Wrapping and reverse-flow edge behavior can differ. [FlexboxComponent.cpp:863-922]
10. **Flex basis lacks container context.** Percentages and contextual units are unreliable. [StyleSheet.cpp:2704-2705]
11. **Invisible-wrapper auto-height checks max width before applying max height.** [FlexboxComponent.cpp:594-600]
12. **Auto-width is not direction-aware and undercounts margins.** [FlexboxComponent.cpp:562-569]
13. **Column wrapping auto-height temporarily uses a hardcoded 1000-pixel component size.** [FlexboxComponent.cpp:609-631]
14. **`overflow` is advertised but ignored.** [LanguageManager.cpp:93]

### Color, background, border, and image bugs

1. **HSL semantics are incompatible with CSS.** [CssParser.cpp:249-260]
2. **Four- and eight-digit CSS hex alpha are wrong.** [CssParser.cpp:226-243]
3. **Modern RGB/HSL space and slash syntax is absent.** [CssParser.cpp:249-260]
4. **RGB percentages are treated as integer channel values.** [CssParser.cpp:252-259]
5. **Unknown colors silently become transparent black.** [CssParser.cpp:261-264]
6. **Gradient angles and stops truncate fractional values.** [CssParser.cpp:428-471]
7. **Gradient transitions can index missing stops when stop counts differ.** [StyleSheet.cpp:2190-2242]
8. **`color-mix()` ignores color space and the second percentage.** [StyleSheet.cpp:2106-2125]
9. **URL backgrounds suppress normal background fill, box shadows, and border rendering.** [Renderer.cpp:796-828]
10. **Background position uses one value for both axes and swaps percentage contexts.** [StyleSheet.cpp:2159-2177]
11. **Gradient background size applies one scalar to both dimensions.** [StyleSheet.cpp:2149-2157]
12. **`object-fit/background-size: scale-down` can enlarge images.** It uses a maximum scale and clamps upward to at least one. [Renderer.cpp:993-995]
13. **Border styles are visually ignored.** [Renderer.cpp:822-826] [HelperClasses.cpp:729-780]
14. **Unequal corner radius magnitudes collapse to one maximum.** [StyleSheet.cpp:1335-1345]
15. **Pseudo-element URL images query base-element state rather than the current pseudo-element.** [Renderer.cpp:942-968]
16. **Image clipping uses a border-reduced path.** [Renderer.cpp:1007-1026]

### Text and pseudo-element bugs

1. **`text-transform: capitalize` is a no-op.** [StyleSheet.cpp:1884-1898]
2. **`content` replaces text on ordinary elements.** [StyleSheet.cpp:1872-1879]
3. **Explicit pseudo-element `display` can suppress it even when not `none`.** [StyleSheet.cpp:1566-1572]
4. **Pseudo-elements cannot create a visual box without `content`.** [StyleSheet.cpp:1555-1591]
5. **`::after2` uses the `::after` absolute-position flag.** [Renderer.cpp:906-909]
6. **`::before2` does not adjust the local background flow like `::before`.** [Renderer.cpp:849-875]
7. **Generated element paint order does not match normal before/after content stacking.** All are drawn from the background pass. [Renderer.cpp:830-932]
8. **Intrinsic text sizing includes before elements but not after elements.** [StyleSheet.cpp:2735-2762]
9. **TextEditor padding calculations are discarded by `setIndents(0, 0)`.** [StyleSheet.cpp:1740-1752]
10. **Text shadow rendering ignores parsed inset shadows.** [Renderer.cpp:1077]
11. **Text is rendered through one JUCE `drawText()` operation.** Standard line layout, wrapping controls, line height, and overflow behavior are absent. [Renderer.cpp:1073-1081]

### Transform and transition bugs

1. **Axis-specific transforms affect both axes.** [CssParser.cpp:558-581]
2. **3D transform names perform unrelated 2D operations.** [CssParser.cpp:563-581]
3. **`matrix()` is a no-op.** [CssParser.cpp:563-567]
4. **One-argument `translate()` and `skew()` reuse the value for both axes.** [CssParser.cpp:558-581]
5. **Transforms do not update hit testing or layout.** [Renderer.cpp:764-767]
6. **Only one transition tuple is parsed.** Comma-separated transition lists and longhands are absent. [CssParser.cpp:2127-2156]
7. **Transition property matching uses loose prefix matching.** Unrelated names sharing a prefix can match. [HelperClasses.cpp:503-515]
8. **Transitions are triggered by observed pseudo-state changes, not arbitrary property changes.** [Animator.cpp:171-287]
9. **Layout transitions repaint without relayout.** Animated dimensions and flex values do not continuously change component bounds. [Animator.cpp:67-98]
10. **Delay-only and zero-duration delayed transitions have incorrect timing.** [Animator.h:127-133] [Animator.cpp:72-95]
11. **The first animator delta can be excessive because callback time starts at zero.** [Animator.h:159] [Animator.cpp:100-118]
12. **Shadow interpolation returns an empty list when list lengths differ.** [CssParser.h:166-183] [StyleSheet.cpp:1967-1968]
13. **`jump-both` and `jump-none` are implemented identically.** [CssParser.cpp:1896-1915]

### Component integration surprises

1. **`display: none` is not always component visibility.** Outside flex-managed setup it can stop only CSS painting while the JUCE component remains interactive. [Renderer.cpp:752-761] [FlexboxComponent.cpp:436-452]
2. **Pseudo-state `display` does not rebuild component visibility.** Flex setup reads the default state. [FlexboxComponent.cpp:442-452]
3. **Cursors are applied only to selected existing direct flex children and use default state.** [FlexboxComponent.cpp:319-337] [StyleSheet.cpp:1848-1867]
4. **Custom styling can replace native control affordances.** Toggle ticks, combo arrows, popup icons, submenu arrows, scrollbar tracks, and progress fill are not automatically preserved by the CSS rendering paths. [CSSLookAndFeel.cpp:104-120, 312-413, 423-437, 537-575]
5. **Popup and table render paths can have no component target.** This limits state identity and transition safety. [CSSLookAndFeel.cpp:303-307, 582-625]
6. **Transforms leave mouse geometry unchanged.** [Renderer.cpp:764-767]

### Cache and collection bugs

1. **Changing an ancestor class invalidates only that component.** Cached descendant selectors, inheritance, and variables can remain stale. [FlexboxComponent.cpp:253-260, 513-543]
2. **Per-component cache clearing leaves all-state cache entries.** [StyleSheet.cpp:990-1013]
3. **All-state cache update iterates entries by value.** Assignments do not update the cache. [StyleSheet.cpp:1138-1144]
4. **Collection equality compares only the first stylesheet pointer.** Later rules, child collections, variables, fonts, and mode changes are ignored. [StyleSheet.h:55-56] [FlexboxComponent.cpp:1177-1203]
5. **Single-rule fast path can expose an uninitialized non-layout-property flag.** [StyleSheet.cpp:601-607, 2555-2572] [StyleSheet.h:291]
6. **Stylesheet equivalence ignores pseudo-class state flags.** Rules that differ only by state can be replacement matches. [HelperClasses.cpp:424-446] [StyleSheet.cpp:1064-1095]
7. **Isolated child collection selection is insertion-order based, not nearest-ancestor based.** [StyleSheet.cpp:568-578]

## Suggested remediation order

1. Fix memory safety and unchecked boundary access in transform parsing, token scanning, image loading, and empty arrays.
2. Fix silent correctness defects: box shorthand order, border-radius order, border longhand suffixes, axis transforms, negative `max()`, height axis calculations, `scale-down`, and pseudo-element flag errors.
3. Align advertised support with executable support: remove or implement `overflow` and border styles; add `font-style`, `src`, side borders, `:empty`, `::before2`, `::after2`, and `ease-out` to metadata as appropriate.
4. Make unsupported syntax fail locally and predictably without aborting the whole stylesheet.
5. Repair selector chains, grouped pseudo states, specificity, universal rules, inheritance, and CSS-wide keywords before presenting the system as CSS-compatible.
6. Decide whether non-standard features should remain extensions. Give retained extensions explicit HISE documentation and avoid names that imply unsupported standard semantics.
7. Add conformance tests around every issue above before expanding the property surface.

## Fix classification and behavior examples

This section turns the backlog into an implementation decision list. Every item is either
an approved fix or deferred because it needs a broader compatibility or architecture decision.

### Labels

- **Fix:** Approved for implementation. Add a regression test and change behavior to the intended semantics.
- **Defer:** Do not change in the quick-fix batch. Preserve the current behavior and document the HISE-specific contract until a broader decision is made.

### Fix

- Transform argument and name buffer overflows. [Critical correctness and safety issues 1-2]
- Parser end-of-input boundary reads. [Critical correctness and safety issue 3]
- Empty `!important` value access. [Critical correctness and safety issue 4]
- Empty flex child list access. [Critical correctness and safety issue 5]
- Null background targets, null image streams, zero-sized images, and invalid image inputs. [Critical correctness and safety issues 6-7]
- Cyclic embedded variable substitution. [Critical correctness and safety issue 8]
- Unequal gradient stop interpolation bounds. [Color, background, border, and image bug 7]
- Uninitialized and incorrectly updated stylesheet cache state. [Cache and collection bugs 2-3, 5]
- Four-value margin and padding order. [Parser and declaration bug 3]
- Four-value border-radius order. [Parser and declaration bug 4]
- Three-value border-radius shorthand. [Parser and declaration bug 5]
- Duplicated border width and style suffixes. [Parser and declaration bug 6]
- Negative `max()` and negative division evaluation. [Parser and declaration bug 14; expression evaluation]
- Height percentage axis selection and swapped width/height `auto` calculation. [Box model and flexbox bugs 6-7]
- `scale-down` image sizing. [Color, background, border, and image bug 12]
- `::after2` positioning flag selection. [Text and pseudo-element bug 5]
- Omitted final declaration semicolon. [Parser and declaration bug 1]
- Quoted `@import` sources and propagation of imported parse failures. [Variables and at-rule gaps 4-5]
- Quoted parentheses during value scanning. [Parser and declaration bug 12]
- Metadata omissions for effective properties, pseudo-elements, and timing values. [Parser and declaration bug 6; Timing functions]
- Axis-specific `translateX/Y`, `scaleX/Y`, and `skewX/Y`. [Transform and transition bugs 1-2]
- One-argument `translate()` and `skew()` defaults. [Transform and transition bug 4]
- Unsupported 3D transform names and no-op `matrix()`: reject the whole transform declaration with a diagnostic. Keep `rotateZ()` as a valid 2D alias. [Transform and transition bugs 2-3]
- Fractional gradient angles and stop positions. [Color, background, border, and image bug 6]
- Transition property matching. [Transform and transition bug 7]
- `jump-both` and `jump-none` timing behavior. [Transform and transition bug 13]
- First animator delta and delay-only transition timing. [Transform and transition bugs 10-11]
- Standard comma-form `hsl(H, S%, L%)` and `hsla(H, S%, L%, A)` semantics. Existing unit tests encode the current HISE behavior. Modern space/slash syntax remains unsupported.
- Repeated transform declarations replace the previous value. Transform composition remains comma-free within one transform list.
- Repeated shadow declarations replace the previous value. Emit a concise migration warning: `Repeated box-shadow overrides the previous value; use commas to combine shadows.` Use the equivalent `text-shadow` warning for text shadows.
- Unknown color fallback. Ignore the declaration and emit a concise warning instead of producing transparent black.
- Unsupported four-digit alpha colors. Reject them and direct authors to `rgba()` or HISE alpha-first eight-digit syntax.

### Defer

- Inherited `opacity` and subtree compositing. Preserve the current approximation until component-level compositing is designed.
- `background-position` axis handling, keyword syntax, percentage semantics, and URL-image positioning. Defer until gradients and URL images share a coherent positioning model.
- Case-insensitive CSS property and keyword matching. Keep canonical lowercase CSS names as the HISE language policy.
- Stateful `display: none`. Preserve its current paint-pass behavior; it must not change component visibility during painting.
- HISE internal margins. Preserve margins as insets within fixed component bounds; use `gap` for flex-item spacing.
- Eight-digit hash colors as `#AARRGGBB` and `0xAARRGGBB`. Preserve the HISEScript/JUCE interchange contract.
- `element(identifier)`, `:hidden`, `::before2`, and `::after2` as HISE extensions.
- Runtime component variables as host-provided HISE integration.
- Base64 JUCE Path data as a `background-image` extension.
- Selector-chain representation and descendant matching.
- Specificity, grouped pseudo-state cascade, universal rules, inheritance, and CSS-wide keyword redesign.
- True flex margins, `box-sizing`, overflow, clipping, and composited opacity.
- Transform-aware hit testing and layout transitions.
- Full generated-content, background-layer, and transition-list support.
- Broader browser-only feature families beyond the explicitly handled transform names.

### Before and after examples

These examples describe the intended result of the proposed fixes. They are also suitable
as regression-test cases. "Before" describes the current implementation, not standard CSS.

#### Four-value margin shorthand - Fix

Input:

```css
.card {
    margin: 8px 16px 24px 32px;
}
```

Before: HISE maps the values approximately as top 8, bottom 16, left 24, right 32.

After: HISE maps the values as CSS does: top 8, right 16, bottom 24, left 32.

The source CSS does not change. Only the computed geometry changes from the incorrect
implementation to the standard interpretation.

#### Four-value border radius - Fix

Input:

```css
.panel {
    border-radius: 4px 8px 12px 16px;
}
```

Before: the third and fourth values are assigned to the bottom-left and bottom-right
corners in reverse order.

After: values map to top-left 4, top-right 8, bottom-right 12, bottom-left 16.

#### Axis-specific transform - Fix

Input:

```css
.badge {
    transform: translateX(10px);
}
```

Before: HISE applies the value to both axes, moving the badge by approximately (10, 10).

After: HISE moves the badge by (10, 0), matching `translateX()` semantics.

Because existing styles may have compensated for the current behavior, this change needs
a project search and a visual regression check before being enabled.

#### Scale-down image sizing - Fix

Input:

```css
.icon {
    background-size: scale-down;
}
```

Before: a source image smaller than the target can be enlarged.

After: a smaller source image remains at its natural size; a larger source image is scaled
down using contain behavior.

#### Missing final semicolon - Fix

Input:

```css
button {
    color: red
}
```

Before: the declaration is treated as incomplete because the final semicolon is required.

After: the declaration is accepted and produces the same result as `color: red;`.

#### Malformed declaration recovery - Defer

Input:

```css
button {
    color: red;
    border-radius: broken value;
    background: blue;
}
```

Before and after: one malformed declaration can abort the remaining stylesheet or discard the
later valid declaration. General declaration-level recovery is deferred.

#### HISE alpha-first colors - Defer

Input:

```javascript
laf.setStyleSheetProperty("accent", 0x80FF0000, "color");
```

Generated CSS:

```css
button {
    background: var(--accent);
}
```

Before and after: the value remains HISE/JUCE `AARRGGBB`: alpha 0x80, red 0xFF, green
0x00, blue 0x00. It must not be changed to browser CSS `RRGGBBAA` ordering because that
would change existing HISEScript colors.

Use `rgba(255, 0, 0, 0.5)` when importing browser CSS that uses standard CSS color syntax.
Do not reinterpret existing `#AARRGGBB` values as `#RRGGBBAA`.

#### Unsupported four-digit alpha colors - Fix

Input:

```css
.panel {
    color: #f008;
}
```

Before: the form is not decoded using a stable, documented HISE meaning.

After: the declaration is rejected with a diagnostic directing the author to either
`rgba(255, 0, 0, 0.53)` for standard CSS syntax or `#88ff0000` for HISE alpha-first syntax.
This avoids inventing a shorthand whose digit order conflicts with the established HISE
eight-digit format.

#### Cyclic variables - Fix

Input:

```css
body {
    --a: var(--b);
    --b: var(--a);
}
```

Before: substitution can continue indefinitely.

After: cycle detection terminates resolution, marks the value invalid or unresolved, and
allows unrelated declarations and components to continue processing.

## Source files reviewed

- `hi_tools/simple_css/CssParser.h`
- `hi_tools/simple_css/CssParser.cpp`
- `hi_tools/simple_css/CssIds.h`
- `hi_tools/simple_css/HelperClasses.h`
- `hi_tools/simple_css/HelperClasses.cpp`
- `hi_tools/simple_css/StyleSheet.h`
- `hi_tools/simple_css/StyleSheet.cpp`
- `hi_tools/simple_css/Renderer.h`
- `hi_tools/simple_css/Renderer.cpp`
- `hi_tools/simple_css/FlexboxComponent.h`
- `hi_tools/simple_css/FlexboxComponent.cpp`
- `hi_tools/simple_css/CSSLookAndFeel.h`
- `hi_tools/simple_css/CSSLookAndFeel.cpp`
- `hi_tools/simple_css/Animator.h`
- `hi_tools/simple_css/Animator.cpp`
- `hi_tools/simple_css/LanguageManager.h`
- `hi_tools/simple_css/LanguageManager.cpp`
- `hi_tools/simple_css/simple_css.h`
- `hi_tools/simple_css/simple_css.cpp`
