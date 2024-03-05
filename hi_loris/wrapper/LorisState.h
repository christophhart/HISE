/*
 * This file is part of the HISE loris_library codebase (https://github.com/christophhart/loris-tools).
 * Copyright (c) 2023 Christoph Hart
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Properties.h"

#include "Helpers.h"
#include "MultichannelPartialList.h"

namespace loris2hise {
using namespace juce;

/** A state that holds all information. Create it with createLorisState and delete it with deleteLorisState().

	This avoids having a global context, however the error reporting will only work with the
	state that was created last (because the loris error handling is global).
 */
struct LorisState
{
    LorisState();
    
    ~LorisState();
    
    static LorisState* getCurrentInstance(bool forceCreate = false);
    
    static void resetState(void* state);
    
    void reportError(const char* msg);
    
    bool analyse(const juce::File& audioFile, double rootFrequency);
    
    bool setOption(const juce::Identifier& id, const juce::var& data);
    
    double getOption(const juce::Identifier& id) const;
    
    MultichannelPartialList* getExisting(const File& f);

    const char* getLastError() const
    {
        return lastError.getErrorMessage().getCharPointer().getAddress();
    }
    
    String getLastMessage()
    {
        if(!messages.isEmpty())
        {
            auto lastMessage = messages[messages.size()-1];
            messages.remove(messages.size()-1);
            return lastMessage;
        }
        
        return {};
    }
    
	void setThreadController(hise::ThreadController* tc)
	{
		currentOption.threadController = tc;
	}

private:

    friend struct Helpers;
    
    Options currentOption;

    juce::Result lastError;

    juce::OwnedArray<MultichannelPartialList> analysedFiles;

    juce::StringArray messages;
    
	static LorisState* currentInstance;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LorisState);
};

}
