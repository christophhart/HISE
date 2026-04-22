# Processor Metadata Migration Guide

This guide describes how to migrate HISE processors from scattered parameter metadata (parameterNames, getDefaultValue, editor setup code) to the unified `ProcessorMetadata` system. It is designed to be followed mechanically for each processor.

## 1. Base Class Chaining Reference

Each processor's `createMetadata()` starts from a different base depending on its type:

| Processor base class | Chain from | Base parameters provided | Base modulation chains |
|---|---|---|---|
| VoiceStartModulator | `ProcessorMetadata()` | None | None |
| TimeVariantModulator | `ProcessorMetadata()` | None | None |
| EnvelopeModulator | `EnvelopeModulator::createBaseMetadata()` | Monophonic (toggle, default 0.0), Retrigger (toggle, default 1.0) | None |
| MidiProcessor | `ProcessorMetadata()` | None | None |
| EffectProcessor (all subtypes) | `ProcessorMetadata()` | None | None |
| ModulatorSynth | `ModulatorSynth::createBaseMetadata()` | Gain (NormalizedPercentage, default 1.0), Balance (Pan, default 0.0), VoiceLimit (Discrete {1,256,1}, default 64.0), KillFadeTime (Time {0,20000,1}, default 20.0) | GainModulation (ScaleOnly), PitchModulation (Pitch) |

## 2. The Recipe (Standard Processors)

### Step 1: Header (.h)

**a) Clear the SET_PROCESSOR_NAME description.** The third argument becomes `""` since metadata now owns the description:

```cpp
// Before
SET_PROCESSOR_NAME("SimpleEnvelope", "Simple Envelope", "The most simple envelope (only attack and release).")

// After
SET_PROCESSOR_NAME("SimpleEnvelope", "Simple Envelope", "")
```

**b) Strip doxygen comments from parameter enum members.** Metadata owns descriptions now.

Also remove any parameter table from the class doxygen comment (the `ID | Parameter | Description` markdown tables).

**c) Add `const bool metadataInitialised;`** after the constructor declaration:

```cpp
SimpleEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);
~SimpleEnvelope();

const bool metadataInitialised;
```

**d) Add `static ProcessorMetadata createMetadata();`** declaration.

**e) Remove `getDefaultValue()` override declaration** - unless the processor has runtime-dependent defaults (see Section 5).

### Step 2: Source (.cpp)

**a) Write `createMetadata()` at the top of the file** (before the constructor). Use `withStandardMetadata<T>()` to set the id, pretty name, type, and interfaces in one call, then chain from the appropriate base (see Section 1):

```cpp
hise::ProcessorMetadata SimpleEnvelope::createMetadata()
{
    using Par = ProcessorMetadata::ParameterMetadata;
    using Mod = ProcessorMetadata::ModulationMetadata;

    return EnvelopeModulator::createBaseMetadata()
        .withStandardMetadata<SimpleEnvelope>()
        .withDescription("A lightweight two-stage envelope with attack and release, supporting linear or exponential curves.")
        .withParameter(Par(Attack)
            .withId("Attack")
            .withDescription("The attack time in milliseconds")
            .withSliderMode(HiSlider::Time, Range(0.0, 20000.0).withCentreSkew(1000.0))
            .withDefault(5.0f))
        .withParameter(Par(Release)
            .withId("Release")
            .withDescription("The release time in milliseconds")
            .withSliderMode(HiSlider::Time, Range(0.0, 20000.0).withCentreSkew(1000.0))
            .withDefault(10.0f))
        .withParameter(Par(LinearMode)
            .withId("LinearMode")
            .withDescription("Toggles between linear and exponential envelope curves")
            .asToggle()
            .withDefault(1.0f))
        .withModulation(Mod(AttackChain)
            .withId("AttackTimeModulation")
            .withDescription("Modulates the attack time per voice")
            .withConstrainer<VoiceStartModulator>()
            .withMode(scriptnode::modulation::ParameterMode::ScaleOnly));
}
```

For processors that don't chain from a base (VoiceStartModulators, TimeVariantModulators, MidiProcessors, Effects):

```cpp
hise::ProcessorMetadata LfoModulator::createMetadata()
{
    using Par = ProcessorMetadata::ParameterMetadata;
    using Mod = ProcessorMetadata::ModulationMetadata;
    using Range = scriptnode::InvertableParameterRange;

    return ProcessorMetadata()
        .withStandardMetadata<LfoModulator>()
        .withDescription("Generates a periodic modulation signal with multiple waveform types, tempo sync, and an optional step sequencer mode.")
        // ... parameters and modulations
}
```

**`withStandardMetadata<T>()`** is a convenience that chains `.withId(T::getClassType())`, `.withPrettyName(T::getClassName())`, `.withType<T>()`, and `.withInterface<T>()` in one call. Use it instead of writing those four calls manually. If you need to set a different pretty name, call `.withPrettyName()` after `withStandardMetadata` to override it.

**b) Add `metadataInitialised(updateParameterSlots())` to the initializer list.** It must appear before any member that calls `getDefaultValue()`.

If the constructor does NOT call `getDefaultValue()` in the initializer list, place `metadataInitialised(updateParameterSlots())` at the end of the initializer list (it still needs to run before the constructor body).

**c) Delete the `parameterNames.add(...)` block** from the constructor body.

**d) Delete the `updateParameterSlots()` call** from the constructor body (it now runs via the initializer list).

**e) Delete or simplify the `getDefaultValue()` override:**

- **If all defaults are static constants:** delete the entire override. The base class chain handles it:
  - `Processor::getDefaultValue()` reads from cached metadata
  - `ModulatorSynth::getDefaultValue()` checks metadata first, falls back to switch for base params
  - `EnvelopeModulator::getDefaultValue()` handles Monophonic (runtime-dependent) and Retrigger via switch, delegates to metadata for derived-class params

- **If some defaults are runtime-dependent:** keep only those cases, delegate the rest:

```cpp
float LfoModulator::getDefaultValue(int parameterIndex) const
{
    // Frequency has a dynamic default depending on tempoSync state
    if (parameterIndex == Parameters::Frequency)
        return tempoSync ? (float)TempoSyncer::Eighth : 3.0f;

    // All other parameters use the static default from metadata
    return Processor::getDefaultValue(parameterIndex);
}
```

### Step 3: Editor (.cpp)

**a) Add metadata retrieval** at the top of the setup section:

```cpp
auto md = getProcessor()->getMetadata();
```

**b) Replace manual setup calls** with `md.setup()`:

```cpp
// Before
attackSlider->setup(getProcessor(), SimpleEnvelope::Attack, "Attack Time");
attackSlider->setMode(HiSlider::Time);

// After
md.setup(*attackSlider, getProcessor(), SimpleEnvelope::Attack);
```

The `setup()` method handles:
- `HiSlider`: calls `setup()`, `setMode()`, sets tooltip from description
- `HiComboBox`: calls `setup()`, populates item list
- `HiToggleButton`: calls `setup()`, sets tooltip from description

**c) Keep cosmetic suffixes** that are display-only and not part of the parameter definition:

```cpp
md.setup(*detuneSlider, getProcessor(), WaveSynth::Detune1);
detuneSlider->setTextValueSuffix("ct");  // keep this
```

**d) Replace tempo-sync switching** in `updateGui()`:

```cpp
// Before
if (getProcessor()->getAttribute(LfoModulator::TempoSync) > 0.5f)
    frequencySlider->setMode(HiSlider::Mode::TempoSync);
else
    frequencySlider->setMode(HiSlider::Frequency, NormalisableRange(0.01, 40.0).withCentreSkew(10.0));

// After
auto md = getProcessor()->getMetadata();
md.updateTempoSync(*frequencySlider);
```

### Step 4: Registry (.cpp)

In `ProcessorMetadataRegistry.cpp`, replace the fallback entry:

```cpp
// Before
add(ProcessorMetadata::createFallback<SimpleEnvelope>());

// After
add(SimpleEnvelope::createMetadata());
```

## 3. Variant: Hardcoded and Scripted Processors

### Hardcoded DLL Processors

Hardcoded processors (`HardcodedMasterFX`, `HardcodedPolyphonicFX`, `HardcodedTimeVariantModulator`, `HardcodedEnvelopeModulator`, `HardcodedSynthesiser`) have parameters defined at runtime by the compiled DSP network. They use `DataType::Dynamic` metadata and the `withHardcodedMetadata<T>()` helper on `HardcodedSwappableEffect`:

```cpp
static ProcessorMetadata createMetadata()
{
    return withHardcodedMetadata<HardcodedMasterFX>({})
        .withDescription("Runs a compiled C++ DSP network as a master effect.");
}
```

`withHardcodedMetadata<T>(base)` chains: `.asDynamic()`, `.withId()`, `.withPrettyName()`, `.withType<T>()`, `.withInterface<T>()`, plus all 4 complex data interfaces (Table, SliderPack, AudioFile, DisplayBuffer).

For types that chain from a base (envelopes, synths), pass the base metadata instead of `{}`:

```cpp
return withHardcodedMetadata<HardcodedEnvelopeModulator>(EnvelopeModulator::createBaseMetadata())
    .withDescription("...");

return withHardcodedMetadata<HardcodedSynthesiser>(ModulatorSynth::createBaseMetadata())
    .withDescription("...");
```

### Scripted Processors (Javascript*)

Scripted processors (`JavascriptMasterEffect`, `JavascriptPolyphonicEffect`, `JavascriptTimeVariantModulator`, `JavascriptEnvelopeModulator`, `JavascriptSynthesiser`) use the `withScriptnodeMetadata<T>()` helper on `JavascriptProcessor`, which is identical to the hardcoded helper but also adds `withInterface<HotswappableProcessor>()`.

```cpp
static ProcessorMetadata createMetadata()
{
    return withScriptnodeMetadata<JavascriptMasterEffect>({})
        .withDescription("Processes audio through a scriptnode DSP network as a master effect.");
}
```

**Exceptions:** `JavascriptMidiProcessor` and `JavascriptVoiceStartModulator` don't use the helper because they lack `DisplayBuffer` support and `HotswappableProcessor` interface. They construct metadata manually with only 3 complex data types (Table, SliderPack, AudioFile).

### Hardcoded Script Processors (Arpeggiator, etc.)

Hardcoded script processors define parameters through the scripting Content API in their `onInit()` override. Their migration is different because:
- Parameters are defined via `Content.addKnob()`, `Content.addButton()`, etc. - not C++ enums
- There is no `getDefaultValue()` override
- The `onInit()` code must be preserved (it creates the UI widgets)

**a) Header:** Add `static ProcessorMetadata createMetadata();` declaration. Clear the `SET_PROCESSOR_NAME` description to `""`. No `metadataInitialised` member needed.

**b) Source:** Write a `createMetadata()` that extracts parameter names, types, and defaults from the `onInit()` code. Parameter indices are sequential integers starting from 0.

**c) Registry:** Replace `createFallback<T>()` with `T::createMetadata()`.

**d) `onInit()` stays unchanged.** The Content widget creation code is preserved. The `parameterNames.add()` / `flushContentParameters()` / `updateParameterSlots()` calls also stay.

## 4. Modulation Chain Constrainers

Modulation chains can declare which modulator types are allowed inside them via constrainer wildcards. This replaces the runtime `FactoryType::Constrainer` system with static metadata that can be queried without instantiating processors.

### Per-chain constrainer (on ModulationMetadata)

Use `.withConstrainer<T>()` on a `ModulationMetadata` entry. The template argument `T` must have a static `getWildcard()` method returning a `ProcessorMetadata::WildcardFilterList` (which is `std::vector<std::pair<Identifier, String>>`). The method extracts the `Modulator` domain wildcard from that list.

```cpp
.withModulation(Mod(AttackChain)
    .withId("AttackTimeModulation")
    .withDescription("Modulates the attack time per voice")
    .withConstrainer<VoiceStartModulator>()
    .withMode(scriptnode::modulation::ParameterMode::ScaleOnly))
```

**When to use:** Envelope internal chains that only accept VoiceStartModulators, or any chain with restricted modulator types.

### Constrainer-after-the-fact (on ProcessorMetadata)

Use `.withModConstrainer<CT>(chainIndex)` to apply a constrainer to an already-declared modulation chain (e.g., one inherited from a base class):

```cpp
return ModulatorSynth::createBaseMetadata()
    .withStandardMetadata<ModulatorSynthChain>()
    .withModConstrainer<NoMidiInputConstrainer>(ModulatorSynth::BasicChains::GainChain);
```

### Disabled chains

Use `.withDisabledChain(chainIndex)` to mark a modulation chain as structurally disabled (not just bypassed - the chain is never processed):

```cpp
.withDisabledChain(ModulatorSynth::BasicChains::PitchChain)
```

Use `.asDisabled()` on a `ModulationMetadata` entry when constructing one from scratch:

```cpp
md.modulation.set(index, 
    ProcessorMetadata::ModulationMetadata(chainIndex)
        .withId("...")
        .asDisabled());
```

### Wildcard syntax

Constrainer wildcards are simple filter expressions:

| Pattern | Meaning | Example |
|---|---|---|
| `*` | Allow everything (default) | No constraint |
| `SubtypeName` | Allow only this subtype | `TimeVariantModulator` - only TVMs allowed |
| `VoiceStartModulator` | Allow only voice-start modulators | Used on envelope internal chains |
| `!TypeName` | Exclude this specific type | `!SlotFX` |
| `!A\|!B\|!C` | Exclude multiple types | `!ModulatorSynthChain\|!GlobalModulatorContainer` |
| `!Name*` | Exclude by glob pattern | `!Global*Modulator` - excludes GlobalVoiceStartModulator, GlobalTimeVariantModulator, etc. |

### Where constrainer wildcards are stored

| Level | Field | Builder method |
|---|---|---|
| Modulation chain (which modulators fit in this chain) | `ModulationMetadata::constrainerWildcard` | `.withConstrainer<T>()` on the Mod entry |
| FX chain (which effects fit in this processor's FX chain) | `ProcessorMetadata::fxConstrainerWildcard` | `.withFXConstrainer<CT>()` |
| Child container (which sound generators can be added) | `ProcessorMetadata::constrainerWildcard` | `.withChildConstrainer<CT>()` |
| Propagated FX (which effects are allowed in children's FX chains) | `ProcessorMetadata::childFXConstrainerWildcard` | `.withChildFXConstrainer<CT>()` |

### How to define a constrainer source

Any class can serve as a constrainer source for `withConstrainer<T>()` by providing a static `getWildcard()` method:

```cpp
// On a Constrainer subclass (traditional pattern)
class NoMidiInputConstrainer : public FactoryType::Constrainer
{
public:
    static ProcessorMetadata::WildcardFilterList getWildcard() 
    { 
        return {
            { ProcessorMetadataIds::Modulator, ProcessorMetadataIds::TimeVariantModulator.toString() },
            { ProcessorMetadataIds::Effect, ProcessorMetadataIds::MasterEffect.toString() }
        };
    }
    // ... allowType() override for runtime compatibility
};

// On a modulator base class (lightweight pattern for simple cases)
class VoiceStartModulator : public Modulator
{
public:
    static ProcessorMetadata::WildcardFilterList getWildcard() 
    { 
        return {
            { ProcessorMetadataIds::Modulator, ProcessorMetadataIds::VoiceStartModulator.toString() }
        };
    }
};
```

Each pair in the list maps a domain (`ProcessorMetadataIds::Modulator`, `::Effect`, `::SoundGenerator`) to a wildcard string. The `withConstrainer<T>()` / `withFXConstrainer<CT>()` / etc. methods extract the relevant domain entry.

### Existing constrainer classes

| Class | Domains | Wildcard | Used by |
|---|---|---|---|
| `NoMidiInputConstrainer` | Modulator -> `TimeVariantModulator`, Effect -> `MasterEffect` | SynthChain gain/FX chains, SendContainer FX chain |
| `SynthGroupFXConstrainer` | Effect -> `VoiceEffect` | SynthGroup child FX chains (propagated) |
| `SynthGroupConstrainer` | SoundGenerator -> `!ModulatorSynthChain\|!GlobalModulatorContainer\|!ModulatorSynthGroup\|!MacroModulationSource` | SynthGroup child container |
| `NoGlobalsConstrainer` | Modulator -> `!Global*Modulator` | GlobalModulatorContainer gain chain |
| `SlotFX::Constrainer` | Effect -> `!PolyFilterEffect\|!PolyshapeFX\|!HarmonicFilter\|!HarmonicMonophonicFilter\|!StereoEffect\|!RouteEffect\|!SlotFX` | SlotFX hosted effect list |
| `VoiceStartModulator` (base class) | Modulator -> `VoiceStartModulator` | Envelope internal chains (AHDSR, TableEnvelope, SimpleEnvelope, FlexAHDSR) |

## 5. Container and Chain Metadata

Processors that contain other processors (sound generators, effects, modulators) have additional metadata fields describing what can be added to them.

### Auto-detected fields

`withType<T>()` automatically sets:
- `hasChildren = true` when `T` inherits from `Chain`
- `hasFX = true` when `T` inherits from `ModulatorSynth`

### FX chain constrainer

Use `.withFXConstrainer<CT>()` to declare which effects are allowed in this processor's own FX chain:

```cpp
return ModulatorSynth::createBaseMetadata()
    .withStandardMetadata<ModulatorSynthChain>()
    .withFXConstrainer<NoMidiInputConstrainer>();
```

Use `.withDisabledFX()` if the processor type never renders its FX chain:

```cpp
return ModulatorSynth::createBaseMetadata()
    .withStandardMetadata<GlobalModulatorContainer>()
    .withDisabledFX();
```

### Child sound generator constrainer

Use `.withChildConstrainer<CT>()` for containers that host child sound generators:

```cpp
return ModulatorSynth::createBaseMetadata()
    .withStandardMetadata<ModulatorSynthGroup>()
    .withChildConstrainer<SynthGroupConstrainer>();
```

### Propagated FX constrainer

Use `.withChildFXConstrainer<CT>()` when a container propagates FX constraints to its children's effect chains at runtime. This is purely informational metadata - the actual propagation still happens via the runtime `Constrainer` system, but the metadata tells consumers (LLM, popup menu, ScriptBuilder) what constraints will be in effect:

```cpp
.withChildFXConstrainer<SynthGroupFXConstrainer>()
```

### Complete container example (ModulatorSynthGroup)

```cpp
static ProcessorMetadata createMetadata()
{
    using Par = ProcessorMetadata::ParameterMetadata;
    using Mod = ProcessorMetadata::ModulationMetadata;

    return ModulatorSynth::createBaseMetadata()
        .withStandardMetadata<ModulatorSynthGroup>()
        .withDescription("A container for synthesisers that share common modulation, with optional FM synthesis and unison detune/spread.")
        .withChildConstrainer<SynthGroupConstrainer>()
        .withChildFXConstrainer<SynthGroupFXConstrainer>()
        .withParameter(Par(EnableFM)
            .withId("EnableFM")
            .withDescription("Enables FM synthesis between two child synths")
            .asToggle()
            .withDefault(0.0f))
        // ... more parameters ...
        .withModulation(Mod(InternalChains::DetuneModulation)
            .withId("Detune Modulation")
            .withDescription("Modulates the unison detune amount")
            .withMode(scriptnode::modulation::ParameterMode::ScaleOnly))
        .withModulation(Mod(InternalChains::SpreadModulation)
            .withId("Spread Modulation")
            .withDescription("Modulates the unison stereo spread")
            .withMode(scriptnode::modulation::ParameterMode::ScaleOnly));
}
```

### Rewriting inherited modulation chains

When a derived class needs to completely replace a base class modulation chain (e.g., GlobalModulatorContainer renames the gain chain to "Global Modulators"), set the entry directly:

```cpp
auto md = ModulatorSynth::createBaseMetadata()
    .withStandardMetadata<GlobalModulatorContainer>()
    .withDisabledChain(ModulatorSynth::BasicChains::PitchChain)
    .withDisabledFX();

// Replace the gain chain entry entirely
md.modulation.set(ModulatorSynth::BasicChains::GainChain,
    ProcessorMetadata::ModulationMetadata(ModulatorSynth::InternalChains::GainModulation)
        .withId("Global Modulators")
        .withDescription("The modulation sources that can be picked up using GlobalModulators")
        .withConstrainer<NoGlobalsConstrainer>());

return md;
```

## 6. Parameter Extraction Checklist

For each processor, gather data from these sources:

| Data | Where to find it |
|---|---|
| Parameter names/order | `parameterNames.add()` in constructor (or `onInit()` for hardcoded) |
| Parameter enum indices | The `SpecialParameters` or `Parameters` enum in the header |
| Default values | `getDefaultValue()` switch statement |
| Slider mode | Editor `.setMode()` calls |
| Range | Editor `.setMode(mode, range)` or `.setRange()` calls |
| Value list (combo boxes) | Editor combo box item population, or `withValueList()` in onInit |
| Toggle parameters | `asToggle()` in editor `.setup()` or `HiToggleButton` usage |
| Tempo sync | Editor tempo-sync switching logic in `updateGui()` |
| Modulation chains | `InternalChains` enum + `new ModulatorChain(...)` calls in constructor |
| Modulation modes | The mode argument to `ModulatorChain` constructor (GainMode, PitchMode, etc.) |
| Chain constrainers | `setConstrainer()` calls in constructor or `setGroup()`, or factory type overrides (e.g., `TimeVariantModulatorFactoryType`) |
| Description | Doxygen comments on enum members, `SET_PROCESSOR_NAME` third argument |
| Type/subtype | The C++ base class (passed to `withType<T>()`) |
| Identity interfaces | Concrete type checks: `ModulatorSampler`, `MidiPlayer`, `WavetableSynth`, `HotswappableProcessor`, `RoutableProcessor` |
| Complex data interfaces | `ProcessorWithStaticExternalData` constructor args or `getNumDataObjects()` overrides |

### Description quality

Descriptions should be concise but informative - explain *what the parameter does* to the signal or behavior, not just restate the parameter name. Aim for one sentence that would help a user who doesn't know what the parameter is.

| Avoid | Prefer |
|---|---|
| "The smoothing time" | "Applies smoothing to reduce hard edges in the oscillator output" |
| "The attack time in milliseconds" | "Controls how long the envelope takes to reach full level after a note-on" |
| "The most simple envelope (only attack and release)." | "A minimal envelope with attack and release stages, using linear or exponential curves." |
| "Smoothing factor for the oscillator" | "Applies low-pass smoothing to the LFO output to reduce discontinuities at waveform edges" |

If the existing doxygen comment or `SET_PROCESSOR_NAME` description is already clear, use it directly. If it merely restates the parameter name (e.g., "the frequency"), rewrite it.

For module descriptions, explain what the module does (not just what it is), mention key features or distinguishing capabilities, and keep it to a single sentence of 15-30 words. For LLM-facing descriptions, note non-obvious domain details (e.g., gain being linear 0-1 not dB).

### ProcessorMetadata builder methods reference

| Method | When to use |
|---|---|
| `.withStandardMetadata<T>()` | Always - sets id, prettyName, type, and interfaces from the class |
| `.withDescription("...")` | Always - brief description of the processor |
| `.withPrettyName("...")` | Only to override the name set by `withStandardMetadata` |
| `.withId(id)` | Only when not using `withStandardMetadata` |
| `.withType<T>()` | Only when not using `withStandardMetadata` |
| `.withInterface<T>()` | Only when not using `withStandardMetadata` (it calls this automatically) |
| `.asDynamic()` | For dynamic/scripted processors whose parameters come from networks/scripts |
| `.withComplexDataInterface(ExternalData::DataType)` | For each data type the processor holds (Table, SliderPack, AudioFile, DisplayBuffer) |
| `.withFXConstrainer<CT>()` | For processors with a constrained FX chain |
| `.withChildConstrainer<CT>()` | For containers with a constrained child list |
| `.withChildFXConstrainer<CT>()` | For containers that propagate FX constraints to children |
| `.withModConstrainer<CT>(index)` | To apply a constrainer to an inherited modulation chain |
| `.withDisabledChain(index)` | To mark a modulation chain as structurally disabled |
| `.withDisabledFX()` | To mark a processor as having no FX chain |

### ParameterMetadata builder methods reference

| Method | When to use |
|---|---|
| `.withId("Name")` | Always - the parameter's string identifier |
| `.withDescription("...")` | Always - brief description |
| `.withDefault(value)` | Always - the default value as float |
| `.withSliderMode(HiSlider::Mode, Range(...))` | For slider parameters with explicit range |
| `.asToggle()` | For on/off parameters (sets range to {0,1,1}) |
| `.withValueList({"item1", "item2", ...})` | For combo box / list parameters (1-indexed by default) |
| `.withValueList(items, startIndex)` | For list parameters with non-default start index |
| `.withTempoSyncMode(toggleParamIndex)` | For parameters that switch between free and tempo-sync modes |
| `.withRange(Range(...))` | For parameters with explicit range but no slider mode |

### ModulationMetadata builder methods reference

| Method | When to use |
|---|---|
| `.withId("Name")` | Always - the chain's string identifier |
| `.withDescription("...")` | Always - what the chain modulates |
| `.withMode(scriptnode::modulation::ParameterMode::X)` | Always - ScaleOnly, ScaleAdd, or Pitch |
| `.withConstrainer<T>()` | When the chain restricts which modulator types are allowed |
| `.withModulatedParameter(paramEnum)` | When the chain modulates a specific parameter (see below) |
| `.asDisabled()` | When the chain is structurally disabled (never processed) |

### Modulation-to-parameter cross-references

Use `.withModulatedParameter(paramEnum)` on a `ModulationMetadata` entry to declare which parameter the modulation chain affects. This stores a `parameterIndex` on the modulation entry and a corresponding `modulationIndex` on the parameter entry, creating a bidirectional cross-link.

**When to add it:** If the processor's `getModulationQueryFunction()` override maps a `parameterIndex` to a specific modulation chain, that same mapping should be declared in metadata via `.withModulatedParameter()`. The heuristic is deterministic - every `case Parameter: return new GetModulationOutput<Chain>()` in `getModulationQueryFunction()` maps to a `.withModulatedParameter(Parameter)` on the corresponding `.withModulation()` entry.

**When NOT to add it:** Some modulation chains modulate the overall processor output rather than a specific parameter (e.g., LFO IntensityChain, ModulatorSynth GainModulation). These have no entry in `getModulationQueryFunction()` and should not get `.withModulatedParameter()`. Custom query functions that exist only for display purposes (e.g., MatrixModulator's Value parameter) are also excluded.

**Ordering requirement:** `.withModulation()` calls that use `.withModulatedParameter()` must appear AFTER the `.withParameter()` calls they reference, because the cross-link resolves the parameter by index at construction time.

**Example:**

```cpp
// getModulationQueryFunction maps Frequency -> FrequencyChain
// so metadata declares the same cross-link:
.withParameter(Par(Frequency)
    .withId("Frequency")
    .withDescription("The modulation frequency")
    .withSliderMode(HiSlider::Frequency, Range(0.01, 40.0, 0.0).withCentreSkew(10.0))
    .withDefault(3.0f)
    .withTempoSyncMode(TempoSync))
// ... more parameters ...
.withModulation(Mod(FrequencyChain)
    .withId("LFO Frequency Mod")
    .withDescription("Modulates the frequency of the LFO")
    .withMode(scriptnode::modulation::ParameterMode::ScaleOnly)
    .withModulatedParameter(Parameters::Frequency))  // <-- cross-link
```

**Already cross-linked processors:**

| Processor | Modulation chain | Modulated parameter |
|---|---|---|
| LfoModulator | FrequencyChain | Frequency |
| AhdsrEnvelope | AttackTimeChain | Attack |
| AhdsrEnvelope | AttackLevelChain | AttackLevel |
| AhdsrEnvelope | DecayTimeChain | Decay |
| AhdsrEnvelope | SustainLevelChain | Sustain |
| AhdsrEnvelope | ReleaseTimeChain | Release |
| SimpleEnvelope | AttackChain | Attack |
| TableEnvelope | AttackChain | Attack |
| TableEnvelope | ReleaseChain | Release |
| GainEffect | GainChain | Gain |
| GainEffect | DelayChain | Delay |
| GainEffect | WidthChain | Width |
| GainEffect | BalanceChain | Balance |
| StereoEffect | BalanceChain | Pan |
| ModulatorSynthGroup | DetuneModulation | UnisonoDetune |
| ModulatorSynthGroup | SpreadModulation | UnisonoSpread |

### Modulation mode mapping

The `ModulatorChain` constructor mode maps to `scriptnode::modulation::ParameterMode`:

| ModulatorChain mode | ParameterMode |
|---|---|
| `ModulatorChain::GainMode` | `ScaleOnly` |
| `ModulatorChain::PitchMode` | `Pitch` |

Chains constructed with a `scaleFunction` that maps 0..1 to -1..1 (like WaveSynth's MixModulation) use `ScaleAdd`.

### withType mapping

| C++ base class | `withType<T>()` argument | Auto-sets |
|---|---|---|
| VoiceStartModulator subclass | `hise::VoiceStartModulator` | type=Modulator, subtype=VoiceStartModulator |
| TimeVariantModulator subclass | `hise::TimeVariantModulator` | type=Modulator, subtype=TimeVariantModulator |
| EnvelopeModulator subclass | `hise::EnvelopeModulator` | type=Modulator, subtype=EnvelopeModulator |
| MasterEffectProcessor subclass | `hise::MasterEffectProcessor` | type=Effect, subtype=MasterEffect |
| MonophonicEffectProcessor subclass | `hise::MonophonicEffectProcessor` | type=Effect, subtype=MonophonicEffect |
| VoiceEffectProcessor subclass | `hise::VoiceEffectProcessor` | type=Effect, subtype=VoiceEffect |
| MidiProcessor subclass | `hise::MidiProcessor` | type=MidiProcessor |
| ModulatorSynth subclass | `hise::ModulatorSynth` | type=SoundGenerator, hasFX=true |
| Chain subclass (any) | (detected via `std::is_base_of`) | hasChildren=true |

Note: type/subtype strings are now `Identifier` constants in the `ProcessorMetadataIds` namespace, not raw strings. This prevents typos and enables efficient comparison.

## 7. Special Cases

### Runtime-dependent defaults

If a parameter's default depends on runtime state (not a compile-time constant), you must keep a `getDefaultValue()` override for that parameter only. Known cases:

- **EnvelopeModulator::Monophonic** - default depends on `getVoiceAmount() == 1` (already handled by `EnvelopeModulator::getDefaultValue()`, not a concern for derived classes)
- **LfoModulator::Frequency** - default depends on `tempoSync` state

The pattern is: handle the dynamic parameter(s) explicitly, delegate everything else to `Processor::getDefaultValue()`:

```cpp
float MyProcessor::getDefaultValue(int parameterIndex) const
{
    if (parameterIndex == DynamicParam)
        return computeRuntimeDefault();

    return Processor::getDefaultValue(parameterIndex);
}
```

Even for dynamic defaults, still declare a static default in `createMetadata()` - this is used by the registry/documentation consumers which don't have a processor instance.

### Tempo-sync parameters

Use `.withTempoSyncMode(toggleParamIndex)` on the target parameter (the one whose range/mode switches), pointing to the toggle parameter that controls the mode:

```cpp
.withParameter(Par(Frequency)
    .withId("Frequency")
    .withSliderMode(HiSlider::Frequency, Range(0.01, 40.0, 0.0).withCentreSkew(10.0))
    .withDefault(3.0f)
    .withTempoSyncMode(TempoSync))  // <-- index of the toggle param
```

### Zero-parameter processors

Processors with no own parameters still benefit from metadata for type/description/registry. For ModulatorSynth subclasses, chaining from `createBaseMetadata()` provides the 4 base parameters. For other types with truly zero parameters, the metadata just has type info:

```cpp
static ProcessorMetadata createMetadata()
{
    return ProcessorMetadata()
        .withStandardMetadata<RouteEffect>()
        .withDescription("Routes audio to other channels.");
}
```

### Processors without editors

Some processors have no dedicated editor file (they use `EmptyProcessorEditorBody` or a generic scripting editor). Skip Step 3 (editor migration) for these. They still get Steps 1, 2, and 4.

### Interfaces (Synth.getXXX() correlation)

There are two methods for declaring interfaces, corresponding to different categories of the `Synth.getXXX()` scripting API:

**`withInterface<T>()`** - for identity-based interfaces (the processor IS this type):

| `withInterface<T>()` argument | Scripting API method | Identifier stored |
|---|---|---|
| `ModulatorSampler` | `Synth.getSampler()` | `ProcessorMetadataIds::Sampler` |
| `MidiPlayer` | `Synth.getMidiPlayer()` | `ProcessorMetadataIds::MidiPlayer` |
| `WavetableSynth` | `Synth.getWavetableController()` | `ProcessorMetadataIds::WavetableController` |
| `HotswappableProcessor` (or subclass) | `Synth.getSlotFX()` | `ProcessorMetadataIds::SlotFX` |
| `RoutableProcessor` (or subclass) | `Synth.getRoutingMatrix()` | `ProcessorMetadataIds::RoutingMatrix` |

Note: `withStandardMetadata<T>()` calls `withInterface<T>()` automatically, so you don't need to repeat it for the processor's own class. You only need explicit `withInterface<>()` calls for additional interfaces not detectable from the processor type itself.

**`withComplexDataInterface(ExternalData::DataType)`** - for data-based interfaces (the processor HOLDS this data type):

| `ExternalData::DataType` | Scripting API method | Identifier stored |
|---|---|---|
| `Table` | `Synth.getTableProcessor()` | `ProcessorMetadataIds::TableProcessor` |
| `SliderPack` | `Synth.getSliderPackProcessor()` | `ProcessorMetadataIds::SliderPackProcessor` |
| `AudioFile` | `Synth.getAudioSampleProcessor()` | `ProcessorMetadataIds::AudioSampleProcessor` |
| `DisplayBuffer` | `Synth.getDisplayBufferSource()` | `ProcessorMetadataIds::DisplayBufferSource` |

Usage: call once per data type the processor holds. The counts come from the `ProcessorWithStaticExternalData` constructor arguments or the `ExternalDataHolder` implementation:

```cpp
// LFO has 1 table, 1 slider pack, 0 audio files, 1 display buffer
.withComplexDataInterface(ExternalData::DataType::Table)
.withComplexDataInterface(ExternalData::DataType::SliderPack)
.withComplexDataInterface(ExternalData::DataType::DisplayBuffer)
```

### Range construction

Use `scriptnode::InvertableParameterRange` (aliased as `Range` in the using declaration):

```cpp
using Range = scriptnode::InvertableParameterRange;

// Simple range: {min, max}
Range(-100.0, 100.0)

// Discrete range: {min, max, interval}
Range(-5.0, 5.0, 1.0)

// Range with skew: {min, max, interval}.withCentreSkew(centre)
Range(0.01, 40.0, 0.0).withCentreSkew(10.0)
```

## 8. Processor Inventory

### Already migrated (full metadata)

| Processor | Base class | Params | Constrainer metadata |
|---|---|---|---|
| ConstantModulator | VoiceStartModulator | 0 | - |
| VelocityModulator | VoiceStartModulator | 3 | - |
| KeyModulator | VoiceStartModulator | 0 | - |
| RandomModulator | VoiceStartModulator | 1 | - |
| GlobalVoiceStartModulator | VoiceStartModulator | 2 | - |
| GlobalStaticTimeVariantModulator | VoiceStartModulator | 2 | - |
| ArrayModulator | VoiceStartModulator | 0 | - |
| EventDataModulator | VoiceStartModulator | 2 | - |
| ControlModulator | TimeVariantModulator | 5 | - |
| PitchwheelModulator | TimeVariantModulator | 3 | - |
| MacroModulator | TimeVariantModulator | 4 | - |
| GlobalTimeVariantModulator | TimeVariantModulator | 2 | - |
| LfoModulator | TimeVariantModulator | 11 | - |
| SimpleEnvelope | EnvelopeModulator | 5 (2 base + 3) | AttackChain: VoiceStartModulator |
| AhdsrEnvelope | EnvelopeModulator | 11 (2 base + 9) | All 5 chains: VoiceStartModulator |
| TableEnvelope | EnvelopeModulator | 4 (2 base + 2) | AttackChain, ReleaseChain: VoiceStartModulator |
| FlexAhdsrEnvelope | EnvelopeModulator | varies | All 5 chains: VoiceStartModulator |
| MPEModulator | EnvelopeModulator | 6 (2 base + 4) | - |
| MatrixModulator | EnvelopeModulator | 4 (2 base + 2) | - |
| ScriptnodeVoiceKiller | EnvelopeModulator | 2 (2 base) | - |
| GlobalEnvelopeModulator | EnvelopeModulator | 4 (2 base + 2) | - |
| EventDataEnvelope | EnvelopeModulator | 5 (2 base + 3) | - |
| WaveSynth | ModulatorSynth | 19 (4 base + 15) | - |
| ModulatorSynthChain | ModulatorSynth + Chain | 4 (4 base) | GainChain: NoMidiInputConstrainer, PitchChain: disabled, FX: NoMidiInputConstrainer |
| GlobalModulatorContainer | ModulatorSynth | 4 (4 base) | GainChain rewritten as "Global Modulators" with NoGlobalsConstrainer, PitchChain: disabled, FX: disabled |
| ModulatorSynthGroup | ModulatorSynth + Chain | 12 (4 base + 8) | Child: SynthGroupConstrainer, ChildFX: SynthGroupFXConstrainer |
| SendContainer | ModulatorSynth | 4 (4 base) | FX: NoMidiInputConstrainer |

### Already migrated (dynamic metadata - hardcoded/scripted processors)

| Processor | Base class | DataType | Notes |
|---|---|---|---|
| HardcodedMasterFX | MasterEffectProcessor | Dynamic | Uses `withHardcodedMetadata` |
| HardcodedPolyphonicFX | VoiceEffectProcessor | Dynamic | Uses `withHardcodedMetadata` |
| HardcodedTimeVariantModulator | TimeVariantModulator | Dynamic | Uses `withHardcodedMetadata` |
| HardcodedEnvelopeModulator | EnvelopeModulator | Dynamic | Uses `withHardcodedMetadata` |
| HardcodedSynthesiser | ModulatorSynth | Dynamic | Uses `withHardcodedMetadata` |
| JavascriptMidiProcessor | MidiProcessor | Dynamic | Manual construction (no DisplayBuffer/HotswappableProcessor) |
| JavascriptVoiceStartModulator | VoiceStartModulator | Dynamic | Manual construction (no DisplayBuffer/HotswappableProcessor) |
| JavascriptTimeVariantModulator | TimeVariantModulator | Dynamic | Uses `withScriptnodeMetadata` |
| JavascriptEnvelopeModulator | EnvelopeModulator | Dynamic | Uses `withScriptnodeMetadata` |
| JavascriptMasterEffect | MasterEffectProcessor | Dynamic | Uses `withScriptnodeMetadata` |
| JavascriptPolyphonicEffect | VoiceEffectProcessor | Dynamic | Uses `withScriptnodeMetadata` |
| JavascriptSynthesiser | ModulatorSynth | Dynamic | Uses `withScriptnodeMetadata` |

### Remaining (still using createFallback)

#### MidiProcessors

| Processor | Params | Has getDefaultValue | Has editor | Notes |
|---|---|---|---|---|
| Transposer | 1 | No | Yes | TransposeAmount |
| MidiPlayer | 7 | No | Yes | |
| ChokeGroupProcessor | 4 | Yes | No | |

#### Effects

| Processor | Base | Params | Has getDefaultValue | InitList calls getDefaultValue | Has editor | Notes |
|---|---|---|---|---|---|---|
| PolyFilterEffect | VoiceEffect | 6 | Yes | Yes | Yes | 4 internal chains, needs metadataInitialised |
| HarmonicFilter | VoiceEffect | 4 | No | No | Yes | |
| HarmonicMonophonicFilter | MonophonicEffect | 4 | No | No | Yes | |
| CurveEq | MasterEffect | 0 | No | No | Yes | Dynamic bands |
| StereoEffect | VoiceEffect | 2 | Yes | Yes | Yes | Needs metadataInitialised |
| SimpleReverbEffect | MasterEffect | 6 | No | No | Yes | |
| GainEffect | MasterEffect | 5 | Yes | No | Yes | 4 internal chains |
| ConvolutionEffect | MasterEffect | 10 | Yes | No | Yes | |
| DelayEffect | MasterEffect | 8 | Yes | No | Yes | Has tempo sync |
| ChorusEffect | MasterEffect | 4 | No | No | Yes | |
| PhaseFX | MasterEffect | 4 | No | No | Yes | 1 internal chain |
| RouteEffect | MasterEffect | 0 | No | No | Yes | Zero params |
| SendEffect | MasterEffect | 4 | Yes | No | Yes | 1 internal chain |
| SaturatorEffect | MasterEffect | 4 | Yes | No | Yes | 1 internal chain (TimeVariantModulator only) |
| SlotFX | MasterEffect | 0 | No | No | Yes | Container with hosted effect constrainer |
| EmptyFX | MasterEffect | 0 | No | No | Yes | Does nothing |
| DynamicsEffect | MasterEffect | 18 | Yes | No | Yes | Gate/Compressor/Limiter |
| AnalyserEffect | MasterEffect | 2 | No | No | Yes | |
| ShapeFX | MasterEffect | 13 | Yes | Yes | Yes | Needs metadataInitialised |
| PolyshapeFX | VoiceEffect | 4 | Yes | No | Yes | 1 internal chain |
| MidiMetronome | MasterEffect | 3 | Yes | No | Yes | |
| NoiseGrainPlayer | VoiceEffect | 4 | Yes | No | Yes | AudioSampleProcessor |

#### SoundGenerators

| Processor | Params | Has getDefaultValue | InitList calls getDefaultValue | Has editor | Notes |
|---|---|---|---|---|---|
| ModulatorSampler | 16 (4 base + 12) | No | No | Yes | Complex sampler |
| SineSynth | 10 (4 base + 6) | Yes | Yes | Yes | Needs metadataInitialised |
| NoiseSynth | 4 (4 base + 0) | No | No | Yes | Base params only |
| WavetableSynth | 8 (4 base + 4) | Yes | No | Yes | Extra internal chains |
| AudioLooper | 10 (4 base + 6) | Yes | No | Yes | |
| MacroModulationSource | 4 (4 base + 0) | No | No | No | Container |
| SilentSynth | 4 (4 base + 0) | No | No | Yes | Produces silence |

#### Hardcoded Script Processors

These use the variant pattern described in Section 3. Parameters come from `onInit()` Content widget creation.

| Processor | Params | Notes |
|---|---|---|
| Arpeggiator | 15 | Manual parameterNames.add() pattern |
| LegatoProcessor | 0 | No Content widgets |
| CCSwapper | 2 | FirstCC, SecondCC |
| ReleaseTriggerScriptProcessor | 3 | TimeAttenuate, Time, TimeTable |
| CCToNoteProcessor | 2 | Bypass, ccSelector |
| ChannelFilterScriptProcessor | 3 | channelNumber, mpeStart, mpeEnd |
| ChannelSetterScriptProcessor | 1 | channelNumber |
| MuteAllScriptProcessor | 2 | ignoreButton, fixStuckNotes |

## 9. Validation

After migrating a processor, verify:

1. **Parameter count matches** - `createMetadata().parameters.size()` equals the old `parameterNames` count
2. **Parameter order preserved** - IDs appear in the same order as the old `parameterNames.add()` calls
3. **Default values match** - each parameter's `.withDefault()` matches the old `getDefaultValue()` return
4. **Ranges match** - compare against editor `.setMode()` / `.setRange()` calls
5. **Modulation chains match** - count and order match the `InternalChains` enum
6. **Chain constrainers match** - compare against `setConstrainer()` calls in constructor, `setGroup()`, and factory type overrides
7. **No leftover parameterNames.add()** - the constructor body should not add parameter names (metadata handles this via `updateParameterSlots()`)
8. **No leftover updateParameterSlots()** in constructor body - it runs via `metadataInitialised` in the initializer list
9. **Editor compiles** - `md.setup()` calls compile and reference the correct enum values
10. **Build succeeds** - full build with no errors
11. **Unit tests pass** - run the ProcessorMetadata tests in CI configuration
