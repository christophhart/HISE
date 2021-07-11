/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

const int kilobytesPerSecond1x = 176;

struct AudioTrackProducerClass  : public ObjCClass<NSObject>
{
    AudioTrackProducerClass()  : ObjCClass<NSObject> ("JUCEAudioTrackProducer_")
    {
        addIvar<AudioSourceHolder*> ("source");

        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
        addMethod (@selector (initWithAudioSourceHolder:),     initWithAudioSourceHolder,     "@@:^v");
        addMethod (@selector (verifyDataForTrack:intoBuffer:length:atAddress:blockSize:ioFlags:),
                   produceDataForTrack,           "I@:@^cIQI^I");
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE

        addMethod (@selector (cleanupTrackAfterBurn:),         cleanupTrackAfterBurn,         "v@:@");
        addMethod (@selector (cleanupTrackAfterVerification:), cleanupTrackAfterVerification, "c@:@");
        addMethod (@selector (estimateLengthOfTrack:),         estimateLengthOfTrack,         "Q@:@");
        addMethod (@selector (prepareTrack:forBurn:toMedia:),  prepareTrack,                  "c@:@@@");
        addMethod (@selector (prepareTrackForVerification:),   prepareTrackForVerification,   "c@:@");
        addMethod (@selector (produceDataForTrack:intoBuffer:length:atAddress:blockSize:ioFlags:),
                                                               produceDataForTrack,           "I@:@^cIQI^I");
        addMethod (@selector (producePreGapForTrack:intoBuffer:length:atAddress:blockSize:ioFlags:),
                                                               produceDataForTrack,           "I@:@^cIQI^I");

        registerClass();
    }

    struct AudioSourceHolder
    {
        AudioSourceHolder (AudioSource* s, int numFrames)
            : source (s), readPosition (0), lengthInFrames (numFrames)
        {
        }

        ~AudioSourceHolder()
        {
            if (source != nullptr)
                source->releaseResources();
        }

        std::unique_ptr<AudioSource> source;
        int readPosition, lengthInFrames;
    };

private:
    static id initWithAudioSourceHolder (id self, SEL, AudioSourceHolder* source)
    {
        self = sendSuperclassMessage<id> (self, @selector (init));
        object_setInstanceVariable (self, "source", source);
        return self;
    }

    static AudioSourceHolder* getSource (id self)
    {
        return getIvar<AudioSourceHolder*> (self, "source");
    }

    static void dealloc (id self, SEL)
    {
        delete getSource (self);
        sendSuperclassMessage<void> (self, @selector (dealloc));
    }

    static void cleanupTrackAfterBurn (id, SEL, DRTrack*) {}
    static BOOL cleanupTrackAfterVerification (id, SEL, DRTrack*) { return true; }

    static uint64_t estimateLengthOfTrack (id self, SEL, DRTrack*)
    {
        return static_cast<uint64_t> (getSource (self)->lengthInFrames);
    }

    static BOOL prepareTrack (id self, SEL, DRTrack*, DRBurn*, NSDictionary*)
    {
        if (AudioSourceHolder* const source = getSource (self))
        {
            source->source->prepareToPlay (44100 / 75, 44100);
            source->readPosition = 0;
        }

        return true;
    }

    static BOOL prepareTrackForVerification (id self, SEL, DRTrack*)
    {
        if (AudioSourceHolder* const source = getSource (self))
            source->source->prepareToPlay (44100 / 75, 44100);

        return true;
    }

    static uint32_t produceDataForTrack (id self, SEL, DRTrack*, char* buffer,
                                         uint32_t bufferLength, uint64_t /*address*/,
                                         uint32_t /*blockSize*/, uint32_t* /*flags*/)
    {
        if (AudioSourceHolder* const source = getSource (self))
        {
            const int numSamples = jmin ((int) bufferLength / 4,
                                         (source->lengthInFrames * (44100 / 75)) - source->readPosition);

            if (numSamples > 0)
            {
                AudioBuffer<float> tempBuffer (2, numSamples);
                AudioSourceChannelInfo info (tempBuffer);

                source->source->getNextAudioBlock (info);

                typedef AudioData::Pointer <AudioData::Int16,   AudioData::LittleEndian, AudioData::Interleaved,    AudioData::NonConst> CDSampleFormat;
                typedef AudioData::Pointer <AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const> SourceSampleFormat;

                CDSampleFormat left (buffer, 2);
                left.convertSamples (SourceSampleFormat (tempBuffer.getReadPointer (0)), numSamples);
                CDSampleFormat right (buffer + 2, 2);
                right.convertSamples (SourceSampleFormat (tempBuffer.getReadPointer (1)), numSamples);

                source->readPosition += numSamples;
            }

            return static_cast<uint32_t> (numSamples * 4);
        }

        return 0;
    }

    static uint32_t producePreGapForTrack (id, SEL, DRTrack*, char* buffer,
                                           uint32_t bufferLength, uint64_t /*address*/,
                                           uint32_t /*blockSize*/, uint32_t* /*flags*/)
    {
        zeromem (buffer, bufferLength);
        return bufferLength;
    }

    static BOOL verifyDataForTrack (id, SEL, DRTrack*, const char*,
                                    uint32_t /*bufferLength*/, uint64_t /*address*/,
                                    uint32_t /*blockSize*/, uint32_t* /*flags*/)
    {
        return true;
    }
};

struct OpenDiskDevice
{
    OpenDiskDevice (DRDevice* d)
        : device (d),
          tracks ([[NSMutableArray alloc] init]),
          underrunProtection (true)
    {
    }

    ~OpenDiskDevice()
    {
        [tracks release];
    }

    void addSourceTrack (AudioSource* source, int numSamples)
    {
        if (source != nullptr)
        {
            const int numFrames = (numSamples + 587) / 588;

            static AudioTrackProducerClass cls;

            JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
            NSObject* producer = [cls.createInstance()  performSelector: @selector (initWithAudioSourceHolder:)
                                                             withObject: (id) new AudioTrackProducerClass::AudioSourceHolder (source, numFrames)];
            JUCE_END_IGNORE_WARNINGS_GCC_LIKE
            DRTrack* track = [[DRTrack alloc] initWithProducer: producer];

            {
                NSMutableDictionary* p = [[track properties] mutableCopy];
                [p setObject: [DRMSF msfWithFrames: static_cast<UInt32> (numFrames)] forKey: DRTrackLengthKey];
                [p setObject: [NSNumber numberWithUnsignedShort: 2352] forKey: DRBlockSizeKey];
                [p setObject: [NSNumber numberWithInt: 0] forKey: DRDataFormKey];
                [p setObject: [NSNumber numberWithInt: 0] forKey: DRBlockTypeKey];
                [p setObject: [NSNumber numberWithInt: 0] forKey: DRTrackModeKey];
                [p setObject: [NSNumber numberWithInt: 0] forKey: DRSessionFormatKey];
                [track setProperties: p];
                [p release];
            }

            [tracks addObject: track];

            [track release];
            [producer release];
        }
    }

    String burn (AudioCDBurner::BurnProgressListener* listener,
                 bool shouldEject, bool peformFakeBurnForTesting, int burnSpeed)
    {
        DRBurn* burn = [DRBurn burnForDevice: device];

        if (! [device acquireExclusiveAccess])
            return "Couldn't open or write to the CD device";

        [device acquireMediaReservation];

        NSMutableDictionary* d = [[burn properties] mutableCopy];
        [d autorelease];
        [d setObject: [NSNumber numberWithBool: peformFakeBurnForTesting] forKey: DRBurnTestingKey];
        [d setObject: [NSNumber numberWithBool: false] forKey: DRBurnVerifyDiscKey];
        [d setObject: (shouldEject ? DRBurnCompletionActionEject : DRBurnCompletionActionMount) forKey: DRBurnCompletionActionKey];

        if (burnSpeed > 0)
            [d setObject: [NSNumber numberWithFloat: burnSpeed * kilobytesPerSecond1x] forKey: DRBurnRequestedSpeedKey];

        if (! underrunProtection)
            [d setObject: [NSNumber numberWithBool: false] forKey: DRBurnUnderrunProtectionKey];

        [burn setProperties: d];

        [burn writeLayout: tracks];

        for (;;)
        {
            Thread::sleep (300);
            float progress = [[[burn status] objectForKey: DRStatusPercentCompleteKey] floatValue];

            if (listener != nullptr && listener->audioCDBurnProgress (progress))
            {
                [burn abort];
                return "User cancelled the write operation";
            }

            if ([[[burn status] objectForKey: DRStatusStateKey] isEqualTo: DRStatusStateFailed])
                return "Write operation failed";

            if ([[[burn status] objectForKey: DRStatusStateKey] isEqualTo: DRStatusStateDone])
                break;

            NSString* err = (NSString*) [[[burn status] objectForKey: DRErrorStatusKey]
                                                        objectForKey: DRErrorStatusErrorStringKey];
            if ([err length] > 0)
                return nsStringToJuce (err);
        }

        [device releaseMediaReservation];
        [device releaseExclusiveAccess];
        return {};
    }

    DRDevice* device;
    NSMutableArray* tracks;
    bool underrunProtection;
};

//==============================================================================
class AudioCDBurner::Pimpl  : private Timer
{
public:
    Pimpl (AudioCDBurner& b, int deviceIndex)  : owner (b)
    {
        if (DRDevice* dev = [[DRDevice devices] objectAtIndex: static_cast<NSUInteger> (deviceIndex)])
        {
            device.reset (new OpenDiskDevice (dev));
            lastState = getDiskState();
            startTimer (1000);
        }
    }

    ~Pimpl() override
    {
        stopTimer();
    }

    DiskState getDiskState() const
    {
        if ([device->device isValid])
        {
            NSDictionary* status = [device->device status];
            NSString* state = [status objectForKey: DRDeviceMediaStateKey];

            if ([state isEqualTo: DRDeviceMediaStateNone])
            {
                if ([[status objectForKey: DRDeviceIsTrayOpenKey] boolValue])
                    return trayOpen;

                return noDisc;
            }

            if ([state isEqualTo: DRDeviceMediaStateMediaPresent])
            {
                if ([[[status objectForKey: DRDeviceMediaInfoKey] objectForKey: DRDeviceMediaBlocksFreeKey] intValue] > 0)
                    return writableDiskPresent;

                return readOnlyDiskPresent;
            }
        }

        return unknown;
    }

    bool openTray()    { return [device->device isValid] && [device->device ejectMedia]; }

    Array<int> getAvailableWriteSpeeds() const
    {
        Array<int> results;

        if ([device->device isValid])
            for (id kbPerSec in [[[device->device status] objectForKey: DRDeviceMediaInfoKey] objectForKey: DRDeviceBurnSpeedsKey])
                results.add ([kbPerSec intValue] / kilobytesPerSecond1x);

        return results;
    }

    bool setBufferUnderrunProtection (const bool shouldBeEnabled)
    {
        if ([device->device isValid])
        {
            device->underrunProtection = shouldBeEnabled;
            return shouldBeEnabled && [[[device->device status] objectForKey: DRDeviceCanUnderrunProtectCDKey] boolValue];
        }

        return false;
    }

    int getNumAvailableAudioBlocks() const
    {
        return [[[[device->device status] objectForKey: DRDeviceMediaInfoKey]
                                          objectForKey: DRDeviceMediaBlocksFreeKey] intValue];
    }

    std::unique_ptr<OpenDiskDevice> device;

private:
    void timerCallback() override
    {
        const DiskState state = getDiskState();

        if (state != lastState)
        {
            lastState = state;
            owner.sendChangeMessage();
        }
    }

    DiskState lastState;
    AudioCDBurner& owner;
};

//==============================================================================
AudioCDBurner::AudioCDBurner (const int deviceIndex)
{
    pimpl.reset (new Pimpl (*this, deviceIndex));
}

AudioCDBurner::~AudioCDBurner()
{
}

AudioCDBurner* AudioCDBurner::openDevice (const int deviceIndex)
{
    std::unique_ptr<AudioCDBurner> b (new AudioCDBurner (deviceIndex));

    if (b->pimpl->device == nil)
        b = nullptr;

    return b.release();
}

StringArray AudioCDBurner::findAvailableDevices()
{
    StringArray s;

    for (NSDictionary* dic in [DRDevice devices])
        if (NSString* name = [dic valueForKey: DRDeviceProductNameKey])
            s.add (nsStringToJuce (name));

    return s;
}

AudioCDBurner::DiskState AudioCDBurner::getDiskState() const
{
    return pimpl->getDiskState();
}

bool AudioCDBurner::isDiskPresent() const
{
    return getDiskState() == writableDiskPresent;
}

bool AudioCDBurner::openTray()
{
    return pimpl->openTray();
}

AudioCDBurner::DiskState AudioCDBurner::waitUntilStateChange (int timeOutMilliseconds)
{
    const int64 timeout = Time::currentTimeMillis() + timeOutMilliseconds;
    DiskState oldState = getDiskState();
    DiskState newState = oldState;

    while (newState == oldState && Time::currentTimeMillis() < timeout)
    {
        newState = getDiskState();
        Thread::sleep (100);
    }

    return newState;
}

Array<int> AudioCDBurner::getAvailableWriteSpeeds() const
{
    return pimpl->getAvailableWriteSpeeds();
}

bool AudioCDBurner::setBufferUnderrunProtection (const bool shouldBeEnabled)
{
    return pimpl->setBufferUnderrunProtection (shouldBeEnabled);
}

int AudioCDBurner::getNumAvailableAudioBlocks() const
{
    return pimpl->getNumAvailableAudioBlocks();
}

bool AudioCDBurner::addAudioTrack (AudioSource* source, int numSamps)
{
    if ([pimpl->device->device isValid])
    {
        pimpl->device->addSourceTrack (source, numSamps);
        return true;
    }

    return false;
}

String AudioCDBurner::burn (AudioCDBurner::BurnProgressListener* listener,
                            bool ejectDiscAfterwards,
                            bool performFakeBurnForTesting,
                            int writeSpeed)
{
    if ([pimpl->device->device isValid])
        return pimpl->device->burn (listener, ejectDiscAfterwards, performFakeBurnForTesting, writeSpeed);

    return "Couldn't open or write to the CD device";
}

}
