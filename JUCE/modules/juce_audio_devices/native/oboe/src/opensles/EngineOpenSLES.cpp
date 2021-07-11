/*
 * Copyright 2017 The Android Open Source Project
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

#include "common/OboeDebug.h"
#include "EngineOpenSLES.h"
#include "OpenSLESUtilities.h"

using namespace oboe;

EngineOpenSLES &EngineOpenSLES::getInstance() {
    static EngineOpenSLES sInstance;
    return sInstance;
}

SLresult EngineOpenSLES::open() {
    std::lock_guard<std::mutex> lock(mLock);

    SLresult result = SL_RESULT_SUCCESS;
    if (mOpenCount++ == 0) {

        // create engine
        result = slCreateEngine(&mEngineObject, 0, NULL, 0, NULL, NULL);
        if (SL_RESULT_SUCCESS != result) {
            LOGE("EngineOpenSLES - slCreateEngine() result:%s", getSLErrStr(result));
            goto error;
        }

        // realize the engine
        result = (*mEngineObject)->Realize(mEngineObject, SL_BOOLEAN_FALSE);
        if (SL_RESULT_SUCCESS != result) {
            LOGE("EngineOpenSLES - Realize() engine result:%s", getSLErrStr(result));
            goto error;
        }

        // get the engine interface, which is needed in order to create other objects
        result = (*mEngineObject)->GetInterface(mEngineObject, SL_IID_ENGINE, &mEngineInterface);
        if (SL_RESULT_SUCCESS != result) {
            LOGE("EngineOpenSLES - GetInterface() engine result:%s", getSLErrStr(result));
            goto error;
        }
    }

    return result;

error:
    close();
    return result;
}

void EngineOpenSLES::close() {
    std::lock_guard<std::mutex> lock(mLock);
    if (--mOpenCount == 0) {
        if (mEngineObject != nullptr) {
            (*mEngineObject)->Destroy(mEngineObject);
            mEngineObject = nullptr;
            mEngineInterface = nullptr;
        }
    }
}

SLresult EngineOpenSLES::createOutputMix(SLObjectItf *objectItf) {
    return (*mEngineInterface)->CreateOutputMix(mEngineInterface, objectItf, 0, 0, 0);
}

SLresult EngineOpenSLES::createAudioPlayer(SLObjectItf *objectItf,
                                           SLDataSource *audioSource,
                                           SLDataSink *audioSink) {

    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};
    const SLboolean reqs[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    return (*mEngineInterface)->CreateAudioPlayer(mEngineInterface, objectItf, audioSource,
                                                  audioSink,
                                                  sizeof(ids) / sizeof(ids[0]), ids, reqs);
}

SLresult EngineOpenSLES::createAudioRecorder(SLObjectItf *objectItf,
                                             SLDataSource *audioSource,
                                             SLDataSink *audioSink) {

    const SLInterfaceID ids[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };
    const SLboolean reqs[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    return (*mEngineInterface)->CreateAudioRecorder(mEngineInterface, objectItf, audioSource,
                                                    audioSink,
                                                    sizeof(ids) / sizeof(ids[0]), ids, reqs);
}

