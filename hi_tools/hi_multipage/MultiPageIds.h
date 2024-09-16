/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which must be separately licensed for closed source applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace hise
{
using namespace juce;

#define MULTIPAGE_MAJOR_VERSION 1
#define MULTIPAGE_MINOR_VERSION 0
#define MULTIPAGE_PATCH_VERSION 0


#define HISE_MULTIPAGE_ID(x) static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER(x); };

#define MULTIPAGE_ADD_ASSET_TO_LIST(x) list.add(Asset::fromMemory(std::move(MemoryBlock(x, sizeof(x))), x ## _Type, String(x ## _Filename), #x));

#define DEFAULT_PROPERTIES(x) HISE_MULTIPAGE_ID(#x); DefaultProperties getDefaultProperties() const override { return getStaticDefaultProperties(); } \
    static DefaultProperties getStaticDefaultProperties()

namespace multipage {




#define DECLARE_ID(x) static const Identifier x(#x);

enum class MessageType: uint32
{
    Clear              = 0x00000000,
	ValueChangeMessage = 0x00000001,
    Navigation         = 0x00000002,
    ActionEvent        = 0x00000004,
    NetworkEvent       = 0x00000008,
    Hlac               = 0x0000000F,
    ProgressMessage    = 0x00000010,
    FileOperation      = 0x00000020,
    Javascript         = 0x00000040,
    AllMessages        = 0xFFFFFFFF
};

namespace mpid
{
    // ALWAYS COPY IT HERE TOO!
    struct Helpers
    {
        enum RequiredUpdate
		{
			PostInit,
			ResizeParent,
            UpdateVisibility,
			UpdateCSS,
			FullRebuild
		};

        static RequiredUpdate getUpdateType(const Identifier& id);

	    static var getIdList();
    };

    DECLARE_ID(ActionType);
    DECLARE_ID(AllowDemo);
    DECLARE_ID(Assets);
    DECLARE_ID(Args);
    DECLARE_ID(Autofocus);
    DECLARE_ID(ButtonType);
    DECLARE_ID(BinaryName);
    DECLARE_ID(CallOnTyping);
	DECLARE_ID(CheckSubmit);
    DECLARE_ID(Cleanup);
    DECLARE_ID(Class);
    DECLARE_ID(CloseMessage);
    DECLARE_ID(ContentType);
    DECLARE_ID(ConfirmClose);
    DECLARE_ID(Code);
    DECLARE_ID(Columns);
    DECLARE_ID(Children);
    DECLARE_ID(Company);
    DECLARE_ID(Custom);
    DECLARE_ID(Data);
    DECLARE_ID(Directory);
    DECLARE_ID(DecodeFlac);
    DECLARE_ID(EmptyText);
    DECLARE_ID(Enabled);
    DECLARE_ID(EventTrigger);
    DECLARE_ID(ExtraHeaders);
    DECLARE_ID(FailIndex);
    DECLARE_ID(Filename);
    DECLARE_ID(FilterFunction);
    DECLARE_ID(Foldable);
    DECLARE_ID(Folded);
    DECLARE_ID(Function);
    DECLARE_ID(GlobalState);
    DECLARE_ID(Header);
    DECLARE_ID(Help);
    DECLARE_ID(Height);
    DECLARE_ID(Icon);
    DECLARE_ID(Image);
    DECLARE_ID(Inverted);
    DECLARE_ID(ID);
    DECLARE_ID(InitValue);
    DECLARE_ID(Items);
    DECLARE_ID(LayoutData);
    DECLARE_ID(Multiline);
    DECLARE_ID(NumTodo);
    DECLARE_ID(NoLabel);
    DECLARE_ID(Overwrite);
    DECLARE_ID(OperatingSystem);
    DECLARE_ID(Parameters);
    DECLARE_ID(ParseArray);
    DECLARE_ID(ParseJSON);
    DECLARE_ID(Product);
    DECLARE_ID(ProjectName);
    DECLARE_ID(Properties);
    DECLARE_ID(Required);
    DECLARE_ID(RelativePath);
    DECLARE_ID(SaveFile);
    DECLARE_ID(SelectOnClick);
    DECLARE_ID(SimulateFileAction);
    DECLARE_ID(SerialNumber);
    DECLARE_ID(SkipIfNoSource);
    DECLARE_ID(SkipFirstFolder);
    DECLARE_ID(SkipIfTrue);
    DECLARE_ID(Source);
    DECLARE_ID(SpecialLocation);
	DECLARE_ID(StyleData);
    DECLARE_ID(Style);
    DECLARE_ID(StyleSheet);
    DECLARE_ID(Subtitle);
    DECLARE_ID(SupportFullDynamics);
    DECLARE_ID(Syntax);
    DECLARE_ID(Target);
    DECLARE_ID(Tooltip);
    DECLARE_ID(Text);
    DECLARE_ID(Trigger);
    
    DECLARE_ID(Type);
    DECLARE_ID(UseChildState);
    DECLARE_ID(UseGlobalAppData);
    DECLARE_ID(UseInitValue);
    DECLARE_ID(UseLabel);
    DECLARE_ID(UsePost);
    DECLARE_ID(UseProject);
    DECLARE_ID(UseTotalProgress);
    DECLARE_ID(UserEmail);
    DECLARE_ID(UseViewport);
    DECLARE_ID(Value);
    DECLARE_ID(ValueMode);
    DECLARE_ID(Version);
    DECLARE_ID(Visibility);
    DECLARE_ID(WaitTime);
    DECLARE_ID(Width);
    DECLARE_ID(Wildcard);
}

#undef DECLARE_ID

class DefaultProperties
{
public:
    DefaultProperties(std::initializer_list<std::pair<Identifier, var>> ilist)
    : ilist_(ilist) {}

    static DefaultProperties getForSetting(const var& infoObject, const Identifier& id, const String& help)
    {
        return DefaultProperties({
            { mpid::ID, id.toString() },
            { mpid::Text, id.toString() },
            { mpid::Value, infoObject[id] },
            { mpid::Help, help }
            });
    }
    
    const std::pair<Identifier, var>* begin() const noexcept
    {
        return ilist_.data();
    }

    const std::pair<Identifier, var>* end() const noexcept
    {
        return ilist_.data() + size();
    }

    std::size_t size() const noexcept
    {
        return ilist_.size();
    }

private:
    
    const std::vector<std::pair<Identifier, var>> ilist_;
};

}

}
