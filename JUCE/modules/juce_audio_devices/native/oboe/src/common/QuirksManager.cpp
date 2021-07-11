/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <oboe/AudioStreamBuilder.h>
#include <oboe/Oboe.h>

#include "OboeDebug.h"
#include "QuirksManager.h"

#ifndef __ANDROID_API_R__
#define __ANDROID_API_R__ 30
#endif

using namespace oboe;

int32_t QuirksManager::DeviceQuirks::clipBufferSize(AudioStream &stream,
            int32_t requestedSize) {
    if (!OboeGlobals::areWorkaroundsEnabled()) {
        return requestedSize;
    }
    int bottomMargin = kDefaultBottomMarginInBursts;
    int topMargin = kDefaultTopMarginInBursts;
    if (isMMapUsed(stream)) {
        if (stream.getSharingMode() == SharingMode::Exclusive) {
            bottomMargin = getExclusiveBottomMarginInBursts();
            topMargin = getExclusiveTopMarginInBursts();
        }
    } else {
        bottomMargin = kLegacyBottomMarginInBursts;
    }

    int32_t burst = stream.getFramesPerBurst();
    int32_t minSize = bottomMargin * burst;
    int32_t adjustedSize = requestedSize;
    if (adjustedSize < minSize ) {
        adjustedSize = minSize;
    } else {
        int32_t maxSize = stream.getBufferCapacityInFrames() - (topMargin * burst);
        if (adjustedSize > maxSize ) {
            adjustedSize = maxSize;
        }
    }
    return adjustedSize;
}

bool QuirksManager::DeviceQuirks::isAAudioMMapPossible(const AudioStreamBuilder &builder) const {
    bool isSampleRateCompatible =
            builder.getSampleRate() == oboe::Unspecified
            || builder.getSampleRate() == kCommonNativeRate
            || builder.getSampleRateConversionQuality() != SampleRateConversionQuality::None;
    return builder.getPerformanceMode() == PerformanceMode::LowLatency
            && isSampleRateCompatible
            && builder.getChannelCount() <= kChannelCountStereo;
}

class SamsungDeviceQuirks : public  QuirksManager::DeviceQuirks {
public:
    SamsungDeviceQuirks() {
        std::string arch = getPropertyString("ro.arch");
        isExynos = (arch.rfind("exynos", 0) == 0); // starts with?

        std::string chipname = getPropertyString("ro.hardware.chipname");
        isExynos9810 = (chipname == "exynos9810");
    }

    virtual ~SamsungDeviceQuirks() = default;

    int32_t getExclusiveBottomMarginInBursts() const override {
        // TODO Make this conditional on build version when MMAP timing improves.
        return isExynos ? kBottomMarginExynos : kBottomMarginOther;
    }

    int32_t getExclusiveTopMarginInBursts() const override {
        return kTopMargin;
    }

    // See Oboe issue #824 for more information.
    bool isMonoMMapActuallyStereo() const override {
        return isExynos9810; // TODO We can make this version specific if it gets fixed.
    }

    bool isAAudioMMapPossible(const AudioStreamBuilder &builder) const override {
        return DeviceQuirks::isAAudioMMapPossible(builder)
                // Samsung says they use Legacy for Camcorder
                && builder.getInputPreset() != oboe::InputPreset::Camcorder;
    }

private:
    // Stay farther away from DSP position on Exynos devices.
    static constexpr int32_t kBottomMarginExynos = 2;
    static constexpr int32_t kBottomMarginOther = 1;
    static constexpr int32_t kTopMargin = 1;
    bool isExynos = false;
    bool isExynos9810 = false;
};

QuirksManager::QuirksManager() {
    std::string manufacturer = getPropertyString("ro.product.manufacturer");
    if (manufacturer == "samsung") {
        mDeviceQuirks = std::make_unique<SamsungDeviceQuirks>();
    } else {
        mDeviceQuirks = std::make_unique<DeviceQuirks>();
    }
}

bool QuirksManager::isConversionNeeded(
        const AudioStreamBuilder &builder,
        AudioStreamBuilder &childBuilder) {
    bool conversionNeeded = false;
    const bool isLowLatency = builder.getPerformanceMode() == PerformanceMode::LowLatency;
    const bool isInput = builder.getDirection() == Direction::Input;
    const bool isFloat = builder.getFormat() == AudioFormat::Float;

    // There are multiple bugs involving using callback with a specified callback size.
    // Issue #778: O to Q had a problem with Legacy INPUT streams for FLOAT streams
    // and a specified callback size. It would assert because of a bad buffer size.
    //
    // Issue #973: O to R had a problem with Legacy output streams using callback and a specified callback size.
    // An AudioTrack stream could still be running when the AAudio FixedBlockReader was closed.
    // Internally b/161914201#comment25
    //
    // Issue #983: O to R would glitch if the framesPerCallback was too small.
    //
    // Most of these problems were related to Legacy stream. MMAP was OK. But we don't
    // know if we will get an MMAP stream. So, to be safe, just do the conversion in Oboe.
    if (OboeGlobals::areWorkaroundsEnabled()
            && builder.willUseAAudio()
            && builder.isDataCallbackSpecified()
            && builder.getFramesPerDataCallback() != 0
            && getSdkVersion() <= __ANDROID_API_R__) {
        LOGI("QuirksManager::%s() avoid setFramesPerCallback(n>0)", __func__);
        childBuilder.setFramesPerCallback(oboe::Unspecified);
        conversionNeeded = true;
    }

    // If a SAMPLE RATE is specified for low latency then let the native code choose an optimal rate.
    // TODO There may be a problem if the devices supports low latency
    //      at a higher rate than the default.
    if (builder.getSampleRate() != oboe::Unspecified
            && builder.getSampleRateConversionQuality() != SampleRateConversionQuality::None
            && isLowLatency
            ) {
        childBuilder.setSampleRate(oboe::Unspecified); // native API decides the best sample rate
        conversionNeeded = true;
    }

    // Data Format
    // OpenSL ES and AAudio before P do not support FAST path for FLOAT capture.
    if (isFloat
            && isInput
            && builder.isFormatConversionAllowed()
            && isLowLatency
            && (!builder.willUseAAudio() || (getSdkVersion() < __ANDROID_API_P__))
            ) {
        childBuilder.setFormat(AudioFormat::I16); // needed for FAST track
        conversionNeeded = true;
        LOGI("QuirksManager::%s() forcing internal format to I16 for low latency", __func__);
    }

    // Channel Count conversions
    if (OboeGlobals::areWorkaroundsEnabled()
            && builder.isChannelConversionAllowed()
            && builder.getChannelCount() == kChannelCountStereo
            && isInput
            && isLowLatency
            && (!builder.willUseAAudio() && (getSdkVersion() == __ANDROID_API_O__))
            ) {
        // Workaround for heap size regression in O.
        // b/66967812 AudioRecord does not allow FAST track for stereo capture in O
        childBuilder.setChannelCount(kChannelCountMono);
        conversionNeeded = true;
        LOGI("QuirksManager::%s() using mono internally for low latency on O", __func__);
    } else if (OboeGlobals::areWorkaroundsEnabled()
               && builder.getChannelCount() == kChannelCountMono
               && isInput
               && mDeviceQuirks->isMonoMMapActuallyStereo()
               && builder.willUseAAudio()
               // Note: we might use this workaround on a device that supports
               // MMAP but will use Legacy for this stream.  But this will only happen
               // on devices that have the broken mono.
               && mDeviceQuirks->isAAudioMMapPossible(builder)
               ) {
        // Workaround for mono actually running in stereo mode.
        childBuilder.setChannelCount(kChannelCountStereo); // Use stereo and extract first channel.
        conversionNeeded = true;
        LOGI("QuirksManager::%s() using stereo internally to avoid broken mono", __func__);
    }
    // Note that MMAP does not support mono in 8.1. But that would only matter on Pixel 1
    // phones and they have almost all been updated to 9.0.

    return conversionNeeded;
}
