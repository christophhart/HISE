// Put in the header definitions of every dialog here...

#include "dialog_library.h"
#include "broadcaster.cpp"
#include "snippet_browser.cpp"

namespace hise {
namespace multipage {
namespace library {

CleanDspNetworkFiles::CleanDspNetworkFiles(BackendRootWindow* bpe):
  EncodedDialogBase(bpe)
{
	loadFrom("1049.jNB..D.A........nT6K8CFTVz6G.XnJAhB3via.3zmojz2VMTRjOXG1C1J+abUa3xyKHd.7S4gT9PpjDlgxgapA5A.c.LG.QQynbqqZ88gITrP4phkIWr8ZprCEEqKKJayE6E5Yn3a4pxBAWWrtnTc8l52RkKKUWinxo0EqpJWvzb9FATrrprlbgREiAse9zy5BQI4ZBiA6LjZPfHHRZasHWZTy.toiABz+XGHiAepuAxLyNzfwfwOlommdMHyNzHP+WYWOABx7DskaJ8xnrp30I6Ennr38bxHxzWelQxJEQ6O6a8ZoVBAD3HaLy2sm1+4mhcfTvZMgYLjOU.RIgRRE0kWaCQmOiRNK5qzqIE87a8EZz3rvIZrRxyt8TJhGY8ReS4OZYXZrmY2V91t51OvMX1QemQ+1biXDYnf.3CpxQZR5O9W3KIE5Si5PWmzB5oMFZZNm9Uoc65oVy3bNpFp++GlYGJ1w8rICGZImajM1.l+7SikX910SO687u0y9x8M1yOrDADHimQpsYqyRnwiFO.+2u3FljaEwb8aimSYId59yWoGn2T..a3nho6eecuqjJ27oVyrA7E2XuWVh45dysU1uryQ+32oLB8mEjHNcQP+UN23hgkwCGLZPpeNeekDIYcQ1t5gu21VYJDJEQhDgRwiGbGU93AUEapRwXTYmMRhKvT0ZBiUicUHo1+kJ496Yy4OS4QaxI0k+V.nxfFblQgLTMjTBA.P.f..fQYP5FDlsBLppZXDegEjYyeJWcUzR8NF71YnNrAKlMsPh4NYR.9aYN1RWPIAxIFh5sBdwRHHVvMlLfWeero1E.SVjg53IKU1IyWICMgydirIBhqKjIs2BH6wFAnU61wL26XPHvPmr84VdfpXWHrqoj5Vrzs9PZGwKWDRqzAobqqHPlIY.Fsp36UPo.XOFIXNKMR0ePQdESHIPuoEA59rePDrxUTRCdPvc8jfZyYKqyR3U.NwLC0hEb6tnLsuQFVEstlqKQE94tI9U1JG3lEytkL+LgQfLrByQV+eHKDffbiCH9+k6XUd.WluREJwhTNL6SuTS3exnbPAiDc1IhXRAUCcDVTtMYBZY4hhJ6DhaKcf1Vewb1TzQ7NRbu2KYTFr2j4vd.Dh0QRbZ1aKx7YXssCBmqPwjWm.CIkrQ2Z4cI1k+nePRq.QHvnulgIAH3NelX0fgn0vQ3cxtwvLPQbrFSnEREpkg2Bo1J.8Ay5G1VX3AW9QtL4rhoq9EAguA8vps8fEDnjktAdE6hsJkmZzsf0rGU08v0GV0I1p8bsjV0B+kCCf2VnXS+iYTxZXjHwms3guRE6TbjfLxB7BQb2cnTGe70O.nClFLyHfWDxvTPYL6w+k4ymqt59uNow2fKOPoi...lNB..r5H...");
}

BackendDllManager::FolderSubType CleanDspNetworkFiles::getType(const var::NativeFunctionArgs& args)
{
	auto id = args.arguments[0].toString();

	id = id.replace("setItem", "");
	id = id.replace("clear", "");

	if(id == "Networks")
	{
		return BackendDllManager::FolderSubType::Networks;
	}
	if(id == "SNEX")
	{
		return BackendDllManager::FolderSubType::CodeLibrary;
	}
	if(id == "Faust")
	{
		return BackendDllManager::FolderSubType::FaustCode;
	}
	if(id == "Cpp")
	{
		return BackendDllManager::FolderSubType::ThirdParty;
	}

	return BackendDllManager::FolderSubType::numFolderSubTypes;
}

var CleanDspNetworkFiles::setItems(const var::NativeFunctionArgs& args)
{
	String wildcard = "*";
	bool recursive = false;

	auto ft = getType(args);

	if(ft == BackendDllManager::FolderSubType::CodeLibrary)
	{
		recursive = true;
	}

	auto folder = BackendDllManager::getSubFolder(getMainController(), ft);
	auto files = folder.findChildFiles(File::findFiles, recursive, wildcard);

	Array<var> list;

	Array<File> filesToSkip;

	if(ft == BackendDllManager::FolderSubType::CodeLibrary)
	{
		// skip the faust files
		filesToSkip = getFolder(BackendDllManager::FolderSubType::FaustCode).findChildFiles(File::findFiles, false, "*");

		// skip the XML files...
		auto parameterFiles = folder.findChildFiles(File::findFiles, true, "*.xml");

		filesToSkip.addArray(parameterFiles);
	}
	if(ft == BackendDllManager::FolderSubType::ThirdParty)
	{
		// skip the CPP files created by faust
		auto faustFiles = BackendDllManager::getSubFolder(getMainController(), BackendDllManager::FolderSubType::FaustCode).findChildFiles(File::findFiles, false, "*");

		filesToSkip.add(folder.getChildFile("node_properties.json"));

		for(auto ff: faustFiles)
		{
			auto cppFile = folder.getChildFile(ff.getFileNameWithoutExtension()).withFileExtension(".h");
			filesToSkip.add(cppFile);
		}
	}

	for(auto f: files)
	{
		auto fn = f.getRelativePathFrom(folder);

		if(filesToSkip.contains(f))
			continue;
			
		list.add(fn);
	}


	auto id = args.arguments[0].toString();

	auto listId = id.replace("setItem", "list");

	setElementProperty(listId, mpid::Items, list);

	return var();
}

void CleanDspNetworkFiles::removeNodeProperties(const Array<File>& filesToBeDeleted)
{
	auto jsonFile = getFolder(BackendDllManager::FolderSubType::ThirdParty).getChildFile("node_properties.json");

	if(jsonFile.existsAsFile())
	{
		auto nodeProperties = JSON::parse(jsonFile);

		if(auto obj = nodeProperties.getDynamicObject())
		{
			for(auto& f: filesToBeDeleted)
			{
				auto id = Identifier(f.getFileNameWithoutExtension());

				obj->removeProperty(id);
			}

			jsonFile.replaceWithText(JSON::toString(obj));
		}
	}
}

var CleanDspNetworkFiles::clearFile(const var::NativeFunctionArgs& args)
{
	auto listId = args.arguments[0].toString().replace("clear", "list");

	auto ft = getType(args);

	auto values = dialog->getState().globalState.getDynamicObject()->getProperty(listId);

	if(values.size() != 0)
	{
		auto root = BackendDllManager::getSubFolder(getMainController(), ft);

		auto thirdParty = getFolder(BackendDllManager::FolderSubType::ThirdParty);

		Array<File> filesToDelete;

		String message;
		message << "Press OK to delete the following files:\n";

		for(auto& v: *values.getArray())
		{
			auto p = v.toString();

			auto f = root.getChildFile(p);
			filesToDelete.add(f);
			message << "- `" << f.getFullPathName() << "`\n";

			if(ft == BackendDllManager::FolderSubType::FaustCode)
			{
				auto f1 = getFolder(BackendDllManager::FolderSubType::ThirdParty).getChildFile("src_").getChildFile(f.getFileNameWithoutExtension()).withFileExtension(".cpp");
				auto f2 = getFolder(BackendDllManager::FolderSubType::ThirdParty).getChildFile("src").getChildFile(f.getFileNameWithoutExtension()).withFileExtension(".cpp");
				auto f3 = getFolder(BackendDllManager::FolderSubType::ThirdParty).getChildFile(f.getFileNameWithoutExtension()).withFileExtension(".h");

				message << "- `" << f1.getFullPathName() << "`\n";
				message << "- `" << f2.getFullPathName() << "`\n";
				message << "- `" << f3.getFullPathName() << "`\n";

				filesToDelete.add(f1);
				filesToDelete.add(f2);
				filesToDelete.add(f3);
			}

			if(ft == BackendDllManager::FolderSubType::CodeLibrary)
			{
				auto xmlFile = f.withFileExtension("xml");

				if(xmlFile.existsAsFile())
				{
					message << "- `" << xmlFile.getFullPathName() << "`\n";
					filesToDelete.add(xmlFile);
				}
			}
		}

		if(PresetHandler::showYesNoWindow("Confirm delete", message))
		{
			for(auto ff: filesToDelete)
				ff.deleteFile();

			if(ft == BackendDllManager::FolderSubType::ThirdParty)		
				removeNodeProperties(filesToDelete);
		}

		values.getArray()->clear();
	}

	return var();
}

ExportSetupWizard::ExportSetupWizard(BackendRootWindow* bpe):
	EncodedDialogBase(bpe)
{
	loadFrom("2704.jNB..fmB........nT6K8C1aqT2T.nFYPIQKvBYE2fZk8oTIuY8Ts9srVR7eYYBcitoBwNv2ZY6uP6ASUlOjRN9A8eP+W3gbeDPHAfQ.SuzEIh.YoZOrFbxSt4TOetXZ+XbKwqojPhV.KE0i90DZ+ntUScr1hZRpxR08vKd5uJ7QsR++MqOWc1kzFRPn4TDUVpr+A4dPzPrs3nDDJHRh9PYMe2YU+l2wZDMEMmUDYKO1fnIoIoKRGVo8GUOdjguKzUXVqwa7WMFtnZSFcRdp4YdTdDnD1412U6S8YnKhVpnZCeZOcJmnMrqCcMxXQFd0yptFgJVhvxDPDQKgwG2YsDpwjELSfYh1OtyniV7GwDWh3gIj.yDVtLwGp+HfXhHtLwGILhQBLWfPBK5me8fELXtfohDVtrTQzMO5Y7XMIIQkkJpGAVIIZAXPZWMoJ4JE7htERh7m0HZohdoiQzt65fFVpH9YMcr6qh10AMzjhRyFarPCUNUd9qBIZhhCmMTYOCqZ8uYZCNpbPWoLmKIF0uqncD972EVPYqcS1ZR3VkN7ujaXXd6aDu1mj6LicFUYeGsAcglMRN+8gxPPZB+T47Xasgbqsy98yE72utggUB0w1tG2zHuMHNCayt.IH6JsY+r21Z8e05u1UW5120cy61Yry1XjaqvrNen9qvd10IZWKzzWDlkYZJhLOvNioIInoqdLiFEhI6M5uTQbM7iIrrTdjv2imureoeIfnElPyokhyebVcRHKSVpo+OrTOVGCMgXojPOTVKAQfPgOf.EbPEv5HCDUvoBWlFyCxnCMeXgGwNbvPGVTwiSnJDZjNrPxw7AWl3EvH5P5IIA47fSGRfxr.CtLzINFoiJtTQVkMMyCLaqE3gqj5JhHxs0iju5fOKo8Gq3.yIT.QsOSp9xIQFN4V5Tht+Tpc7B9TlRrAIaJ0MJw9kPINNt4Dtkg1Coouv8AZpVDffL8aKd4+1zs51eXMpbitRIGX5gIKT7fWQFihFDrV4IAjrF8EcyqDn5CJ.Qf1CJhs7h.Tr8ox.JSlLYxGIm8T5GEM+R5w7pSufZRUPTnPUm+mYfPZZmYgUyoXK7cw18P4l1Zwxjk0gusuZi7VE9w91txcbtR12kXapUNlgZqfPnK1rnpkcIYZEGSlLoKN2WGi7WgzN66LtciwBzb6WoPLjbglMULsmb3PHMEVaMXajD8PMqsJMZOLGIGz1iGHsCHL9IjdUYPrC1vi8njy0.poAfM3PHUn1vWcNRWposL.OsqdLqBkPLNqT0u1fBEPP5a4XSUcN48gNUercTtV6qV14yqJ23nju3VNentDn80VaskwCafbq0lNw7es3ulI6kQXtI4l9oxclvQjn8WYMJeqgGQX.QRssnmI1UcT+uZJTCWcOAS+mTo4EcLdck4paOm4PPbVasu9ufulNkabG6rRVd6OITaeFBcXWdb9WLMSDQ9t1xts1Jmsb9F0k1fJPeI8eetm8Fc8lxasC4VaW6jrDjPjDQq8HxN6dIjairDpQ6eYYO8tJ2ak4NGvZyiElWg59uobZp+U6bVQlmMm56Apvd+Z+bbBal94WgBoujrFsmaj07ee8S1WIXbnJ8lPICoTDIh..ff.HDH.Pf3AyRj0r0CBCJPHNFDkTQLF..Hf.Pf..jfAfH..f..lFPXIiQfLMSPueHEpd05DXMIYtYa6fJi2EPx7CyRb3lFOeOqo1p.t63Me420zXmkuGsWWezW4EWGnihKTgd2S8f34Bfjt4EQ.exYTh943S9nS6RINKtasGNrOslldEEARB0iOJoDGbTUyRy3FkEIFvvkvpyRRB6TaPkHvIDHqXDqg5UgGoXBdos8MWrZ9fpxGm3wf5JhC.hOJMssAfE7GpWgD6A21BK3CqdBTpvjMMBg5DM6OAxIKnT0jkT+IGBL6sdGfIjIGrfkINZHCzWCxEiS5dIBS0YWJypdNSTsOz1sKS5v+V8sBZXa0091YzSh6WJptzCArBFVBNqebX.ppO0KUsIYMJ+AqkOagMVwIcpYwjtK17LmVIOvINn8BR2GieJzuomlsDRDC8FzHrXiuNSyHgkO9pQpybd1pENRcmO8ZP6sTxH3D2.f0qwJIcJM7rohvCnuSSlgjiJMK4ffQwHD40HUxnIiipjMBnFxixL9qDLdG7QE5nRoBw6LLkFkZ+HC0wtYLJSTfxAty+CjUH+3wvYu7b2Y8qwbhK67gfk5NcLEHeOSIiY34M8cX9US6CRdkazQZWgJn2PqCoUApQQ7XiZsL8oxUBJH3to11SjfKI+YIBQB+AYAPblmXGVPnSTAQNUYmXS11QxujtRM+4fvFKM.SmCqYjKkOEQbVy6pFUnBAWBfBrM28H.WjsVOIUtCEm9S8J4.bNhhdS3tfhM6fnxoRbGXzIWiLA1iB+Ar7lctbWrCqEGesOdpz9kjaK8lzxRDQF8GgRgLHszqpU1IPcqdpWBt.hf7DrSvtkqMX3Lc0Zj.42Z0+o8fZH0Dc6mWqav67CH4JO5iYpMt6yx3StXJDgUY6+f3jBVsYahwYDHzeYmDx7lGCdv2wOabRAocG+u.H9.nuQf7s.SnORhnTK7zgINl9ai7VlN+k70UoiumY6IclwofK3P4Ex5NgWW4vlQCHCzvrtelgR4.m1VU0AuomukFfiz..WIckWcK7XmH2ZC7sMClIaYji0sh8li5q.ZxE7oHLdtpQgCPiezTslpnmB2reKSBcjLGs1Y450C1gcoIBOeShwIrwFpibZJ+FIc5foYuMfLLoXlRFOqMpVinOIz.IWxPAYDRDniM3YPHSZtBEbfYyYrs5uWmnnBShXnKnJAGVfV8n8PLPlXQnkQm4HcdvT1wpnCFXg246dylQsLgCh5ydEKNn9OgnYBVawW6SIWr+mS8sj+6z4VpwBGUKpoVRnYqG2ay1+tUNXZUHXmJSUzsXZm+jIJX4CF1eWjalP.iV0lH.acRsrXvzdc2XwdzZasqMaIMiPsQjk8lH9PoBUCKsoFubhCibfYwou.sk4N6DwpHSnFfrO6O80R8mGKOIPZPHN.Y82huYN+.tHXFJwzrlSn.RFT58WePuOy0MRl6B5Gzfl.1NnWYjy47jX4g2eoDiCqoIxsrKze7yR+35mxcmVf2sXN9aMVlfwlm0XkMzVXA9F5BFU+TIlxzotxzKeGCv8TFH1Jqis.XvZIxL0L2bZBAYAYxJNkNXxoA7RWoshSsDPkrCG9DDIWdq4iXlNl5BEAbJjEhGDtlHA4Y0nTozk5xwNry7LFcQRGu.poP5gPLRrb.tlL3ZqzPjU0WkOBZfepFj6Auv34i.P6u5Zw4BwhJ01KQgrPLEZ8XONZBoRRVts6EyhVSnDOYnLeZBgyXaiBRanS8XtFAXe+4urBVrQE6POX7YcNknyj4cj4Tm6I8Clj8Q38ts7kO2PSUzoNFUItmRSh28f9j8wDQIuQzsPxe+IdyFIJvBikPxia4AzoOWdTSguVHTQ4YtXlg.1jluZN8IFtZWRdZerk4OiF.b7AEc2eGgBcRCRxhxwKBWRQ4UPW+wzljqxDBVyyle+tyQB1xgQQ.RCviA.u+lsxMe3NN+FFNentgOT5H..foi...qNB...");
}

var ExportSetupWizard::checkHisePath(const var::NativeFunctionArgs& args)
{
	auto exists = readState("HisePath").toString().isNotEmpty();

	writeState("hisePathExists", exists);
	writeState("hisePathExtract", !exists);
	writeState("hisePathDownload", !exists);
	writeState("hiseVersionMatches", true); // TODO: make a proper version check against the source code

	return var();
}

var ExportSetupWizard::checkIDE(const var::NativeFunctionArgs& args)
{
#if JUCE_WINDOWS

	auto MSBuildPath = "C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe";
	writeState("msBuildExists", File(MSBuildPath).existsAsFile());

	if(readState("UseIPP"))
	{
		auto IppPath = "C:/Program Files (x86)/Intel/oneAPI/ipp/latest/include/ipp.h";
		writeState("ippExists", File(IppPath).existsAsFile());
	}
	else
	{
		writeState("ippExists", true);
	}
#elif JUCE_MAC
        {
            juce::ChildProcess xc;
            xc.start("xcodebuild --help");
            auto output = xc.readAllProcessOutput();
            auto xcodeExists = xc.getExitCode() == 0;
            writeState("xcodeExists", xcodeExists);
        }
        {
            juce::ChildProcess xcp;
            xcp.start("gem list xcpretty");
            auto output = xcp.readAllProcessOutput();
            auto xcPrettyExists = output.contains("xcpretty");
            writeState("xcPrettyExists", xcPrettyExists);
        }
#endif

	return var();
}

var ExportSetupWizard::checkSDK(const var::NativeFunctionArgs& args)
{
	auto toolsDir = File(readState("HisePath").toString()).getChildFile("tools");
	auto vst3sdk = toolsDir.getChildFile("SDK/VST3 SDK");

#if JUCE_WINDOWS
	auto projucer = toolsDir.getChildFile("projucer/Projucer.exe");

	auto ok = projucer.startAsProcess("--help");
#elif JUCE_MAC
        
        auto projucer = toolsDir.getChildFile("projucer/Projucer.app/Contents/MacOS/Projucer");
        
        jassert(projucer.existsAsFile());
        
        auto ok = projucer.startAsProcess("--help");
        
#else
		auto ok = true;
#endif

	writeState("projucerWorks", ok);
	writeState("sdkExists", vst3sdk.isDirectory());
	writeState("sdkExtract", !vst3sdk.isDirectory());

	return var();
}

var ExportSetupWizard::prevDownload(const var::NativeFunctionArgs& args)
{
	auto id = args.arguments[0].toString();
	String url;

	url << "https://github.com/christophhart/HISE/archive/refs/tags/";
	url << GlobalSettingManager::getHiseVersion();
	url << ".zip";
		
	writeState("sourceURL", url);
	return var();
}

var ExportSetupWizard::skipIfDesired(const var::NativeFunctionArgs& args)
{
	if(readState("skipEverything"))
		navigate(4, false);

	return var();
}

var ExportSetupWizard::onPost(const var::NativeFunctionArgs& args)
{
	writeState("ExportSetup", true);
		
	auto bp = findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor();

	MessageManager::callAsync([bp]()
	{
		bp->getSettingsObject().loadSettingsFromFile(HiseSettings::SettingFiles::CompilerSettings);
	});
		
	return var();
}



var AboutWindow::initValues(const var::NativeFunctionArgs& args)
{
#define set(X) state->globalState.getDynamicObject()->setProperty(Identifier(#X), X);
#define setXY(X, Y) state->globalState.getDynamicObject()->setProperty(Identifier(#X), Y);
    
    String buildHash(PREVIOUS_HISE_COMMIT);

	state->globalState.getDynamicObject()->setProperty("commitHash", "load current hash...");

	WeakReference<AboutWindow> safeThis(this);

	GitHashManager::checkHash(buildHash, [safeThis](const var& commitObj)
	{
		if(safeThis.get() != nullptr)
		{
			auto nextHash = commitObj["sha"].toString();
			auto shortHash = nextHash.substring(0, 8);

			safeThis->state->globalState.getDynamicObject()->setProperty("commitHash", shortHash);

			if(auto pb = safeThis->dialog->findPageBaseForID("commitHash"))
			{
				MessageManagerLock mm;
				pb->postInit();
			}
			
            String link;
            link << "https://github.com/christophhart/HISE/commit/";
            link << nextHash;
            
            safeThis->commitLink = URL(link);
		}
	});

    

    
    
    String Version = hise::PresetHandler::getVersionString();
    
#if JUCE_DEBUG
    setXY(JUCE_DEBUG, 1);
#else
    setXY(JUCE_DEBUG, 0);
#endif
    
#if HISE_INCLUDE_FAUST
    setXY(HISE_INCLUDE_FAUST, 1);
#else
    setXY(HISE_INCLUDE_FAUST, 0);
#endif
    

    set(Version);
    set(USE_IPP);
    set(HISE_INCLUDE_RLOTTIE);
    set(HISE_INCLUDE_RT_NEURAL);
    set(NUM_POLYPHONIC_VOICES);
    set(NUM_MAX_CHANNELS);
    set(NUM_HARDCODED_FX_MODS);
    set(NUM_HARDCODED_POLY_FX_MODS);
    set(HISE_MAX_DELAY_TIME_SAMPLES);
    set(HISE_USE_SVF_FOR_CURVE_EQ);
    set(USE_MOD2_WAVETABLESIZE);
	set(HISE_USE_WRONG_VOICE_RENDERING_ORDER);
    
    return var();
    
#undef set
#undef setXY
}

var AboutWindow::showCommit(const var::NativeFunctionArgs& args)
{
    commitLink.launchInDefaultBrowser();
    
    return var();
}

ReleaseStartOptionDialog::ReleaseStartOptionDialog(hise::BackendRootWindow* bpe_, ModulatorSampler* sampler_):
	EncodedDialogBase(bpe_, false),
	sampler(sampler_),
	root(bpe_)
{
	setName("Release Start Options");
	loadFrom("1934.sNB..D...............35H...oi...hc.........J09R+f09DEqC.ZeEBOrBzPiZCHtoMRIzRYB5R15NI4lNYHWubBXHMsE5DvqD9C3pS2rGYPtZV.x5q6Cf4.PM.Up7PlY0Q+AAhR4789sqdn6P6QB116LyjxbPd8dzdvg1CHbeP9yIgou+ObfHtny4foK+2MZyG8IMiHJmqALkiydPjb5oMWAkITlDQ6L7scL0pq4Zlznoa7pMCPZCiHrmeNwMv142Eg7e5uYg5xVI.W9eqyebDoCKjoOLkUqayBoPDUlnBFIlXalszMZaNCJUtf4Bk2tz8hkwQuLAFKSf.LTbASEKX9P1eBIvLgELfzL6BEWrHAEUrKWgfJWtX4RDTTwxK5LWCDGJSnHhcC7lE7vuciDXqaTvh4UbfHOkEgVIj9qYDIyD4a2AQxLy.vgLSD8UKV1zuIRlAfCQqY9VNsXmPdepjlaE4Bl77OW6RexkSxXrTtPMxoOLB+70nOgKy0TX1btM9yMxwnPgDXnXYec4nvT6aimvtTG7QWiwGxNgH8tV7ESOyXNLj6nCblLVrAFUvgc4Zfajoc7bigEXn5nCdG+TPsiBAHutoIbgdeKFGogBkOBh2KnP4REfblxVv.WB4nOUo+42pTF8ozs.BWMZO9o2Xj+4Nok6X.KzYZFPl4CYa1pZU8t+D8vZsdNyUVhHyrafgqRcPu8Fia5iju8mDDhixkJYlnewRRZtwVNJ668oNXquu8+ibjrLHH+mTIvd9YakWK04Ho1U+aNcLHv+oc7WeX6pIlyoYuZMyle9ys2Xr1ZVgfK+97iORINT1ajT5ZqJtZr9jvzkZOWFGsWnjyXZjegOOQnEYdGsGv19a6aDodUil0GrJ1izjsm+rcTolYH2IcHq.QMr4xhtTkbBzRfbBT98xqTSnlgwdy2J4r744actHb8TI+kVPfYqpOx4oayrTSHm4D+a96AB8mZZgwb67hpr41a+cosl9fIevQZf7ib2.G8u84IxIWpy1mKedq4959dNeOdFyom21V64qTWiy.nCA.CXfsbzL7MzrQo468gL7strABDp4egM+rqL+uk7EHOUC2e0sONe1+FhY5Cmr3i+Jz1DHUo+1MjaG9Yn6x0p5I3Se3mOXTIyz7+cNRZF5oMM9asvpWErAUz5dQ1ggQlJjk.TlLgh9vXSc4xbmjscI8kS7GBHf.hGp4ZWNo+t8UVTwTE66kU7yy5Sn7t21.htbLYR7ys4mT.Yj5oK74sDt1s+cJW62aieeibxjp2nF12S9t+d6C0IU81HHNJhG85Z5JhGoBga+5ckdkwfPntu3qhuGlpmFw1NaANIpQk4PJkglY..BB.A.w.Q.fwxIYbqARCMLIOMDyo.jAB.nB..Afv.RHyxCCz4kQhnpoZU0acRdhqEM0kJ8v8IiI2QGYWJUtOi1XfNGO1QaLuyflQ4ya9eL4OJzyAbXRu8zIyB9Bue14vwyIXFDEjynULa.oZfRJejxOD1yuqtQ1vZl8cDPXQ0feNxllnOU.p+lQlaTgxzRHHXFaB94f17iFhV5knvYRMy8hDTLrOWqFzbI7o+gv6juA1XRpXqxwnNlbmGsH29SG.k5SF+v5ara+53kMrwA7XA6IsCokfdYlpu45E0KGyTnbBg3CADFa6zYaEfqjx1V6FWfREUQUbGfonMfhyTHqVgwFOXRXQmAVnGerW1cnQoaBI4NDCbe80Kk+.iHbMXLkoweqUlaZtZqao8n.aqF.ZRxPHHFfeO2cj+Ye9pMwOWpS+dkHSSCv5kV+08LE1+8OAXT.oQBlmjDLkfQcOP+hQWmkTbiG2Pr.Fa5lN+zIEu..csy+FgsIjcEGKeoE5x7vR1029tSl6AiDJaNwWJYO6dnk1nGkbkP1gl4WVYUlzBmnPtvrS7ZvWg2Jl6nxRXUiEJHz.aQP3VyeBFISMss1juvrYTE.i7PIYWwhWHLrA1NPvIXjE6qCfXzN.zFI.v.C.7rim8O5vvvRjX3CEyGuaDCOEYGNqtQjNzasyV.ya3NnCFf2i2sl5.3rwzx3Px8xrQc0ChNJrsM6owL+Yb.CYc14tH0.etv+jWplWjEA0VdWThRv8OD56+TqxxxL1x8F.nQQciIf1OoRlBzafXaAQj5LrH3g5O+oCCCMOeoDhbth3xUPGvSAb3hmttMXTC0NRhhYoEzEv6sKHLPMWPFOFeGHsMewyOLuLBnvaIgXBp803PxmyqKYBPjXWi41goTKtoCbiwwOYc1XIkS7TcEEREkwMsPJw+J+YCrbT1Q8phY+pv3zjPVefK3Cx+n.DVvyA+kksec9CKNhFG7fIQTkJ6V2POupG+UZeLt50UAG2qwDH5bFN.iqSlExfN5JnA6P5B4nSXbTi1x47bC7eXLO0a8jZ5n1NHrQ++jf2RUV1YACt3VPYRtc0BTqS7eCROoEmZEO.Wv.b227vsNsqvEmPQ2wSrDHhrVTUQ86UAjYOJNdXu+riF4z.hQw2sxsrT7ff7Qy1tC2jN7zhHvQI61X.MAHEtJLenFcz9JDLwEXrUIV8Qlh9cJXGPoi...lNB..v5H...");
}

var ReleaseStartOptionDialog::initValues(const var::NativeFunctionArgs& args)
{
#if HISE_SAMPLER_ALLOW_RELEASE_START
	auto options = sampler->getSampleMap()->getReleaseStartOptions();

	auto d = options->toJSON();

	const auto& obj = d.getDynamicObject()->getProperties();

	for(int i = 0; i < obj.size(); i++)
	{
		auto id = obj.getName(i);
		auto value = obj.getValueAt(i);

		state->globalState.getDynamicObject()->setProperty(id, value);
	}
#endif

	return var();
}

var ReleaseStartOptionDialog::onPropertyUpdate(const var::NativeFunctionArgs& args)
{
#if HISE_SAMPLER_ALLOW_RELEASE_START
	StreamingHelpers::ReleaseStartOptions::Ptr newData = new StreamingHelpers::ReleaseStartOptions();
	newData->fromJSON(state->globalState);
	sampler->getSampleMap()->setReleaseStartOptions(newData);
	
	Component::callRecursive<SamplerSoundWaveform>(root, [](SamplerSoundWaveform* w)
	{
		w->repaint();
		return false;
	});
#endif

	return var();
}

var ReleaseStartOptionDialog::onCreateScriptCode(const var::NativeFunctionArgs& args)
{
	String code;

	state->globalState.getDynamicObject()->removeProperty("CreateScriptCode");

	code << "Synth.getSampler(" << sampler->getId().quoted() << ").setReleaseStartOptions(";
	code << JSON::toString(state->globalState) << ");";
	SystemClipboard::copyTextToClipboard(code);

	return var();
}

WelcomeScreen::WelcomeScreen(BackendRootWindow* bpe_):
	EncodedDialogBase(bpe_),
	bpe(bpe_)
{
	setWantsBackdrop(true);

	loadFrom("3428.sNB..D...............35H...oi...3z.........J09R+fMSH0kF.JfJZfvBrJQv63ApDFw16VvHHtSqyHsaWf31O5D9gkIyITTbnSlw7T.9mR+oCd2goAve.8Gf+AzYqUZgM9iWr0cH48xe6rIVS0c7ervowGLdh5h7L4yDNJYiN75tHBu5jWMNOniWDk3Ij2M7laCVWD1WomNEziWYs+DjbQTsoIugbdQGUCjVaUJLdUPqTLPIzVIJpJkSltLFSNlvsYyD0V8T.yEaxhO6MhXo33EydgxdePtvrZPhgHcQiXdX4RXBMyBImDuMcrUiyN0YynhhtocKd1Kivnrne5dxQVc7aAZ8gkdit538agzOBihx.E.gfFx7qa44RNDf.BAtVzi2b.H+r6E07gyL.C.O+4YeOLmAADVP0nnNON1Q5xS4AhbMHHt766i2++yLGj2hh+v7FBAtndiOP.jjlfPX7CpQO.PPPD.gfuu6eGT1+Q7EvUvAj4cNDBo3cooF7q.tBZ.tB14L3sAXfFO19y4Qu+YOEGwfuxxrMNSey.J0i4ZoHonIdWa5yUR43xM8Zp1F71+KSJh23Hu+5elSOCT34nAT+Ck2z+14DLy5uOW18WCV.DzAxlab3Fq+xkdYSJSFtLfc3ZChyiYtNhjAKLKIeFDmaj2YvVWXUWTXHrvLTg88j+WGvtpvzv0aKDVUGPjHC1+nLXYUYMgvPk+ru6UwLRQHlxezdIPn1m8Nvji8P.vcOgAXfGuOEyRf5ebDCfPvfOuuv.HHHnCz.PHX.9is4c52y.HnCz.whwakgBc68ayvW1mCeeZT4gVmwtNqiME5bnnVV4v2jwHsijadncWRdvYSBZiSGr2cFgBSKedbpM.45A8NY4gxdMwvfl3PXmLYagnbkMJ7tpNUOQKrBJdmFmfH23x30PCzO9KkYgDUMo6OabIvPsPtM5PC110Uy2JNWUrcJBix5mL5zYmbMLgVKQ0P1U4iWlr4ujDsaeZMtVGxDyTYNdOjH5ZuUQGP.YawdKzUJVktDI4ffcQsaRb3131zvVHWYX2Ktc1d+TQzRXnhyBGUzI2aWhOQ7SdXD8gKSTTl8QDCuKhNdwN16XVmNEq0dyWmOvLoK6ZAkIb0pMhGx0LCnVFdXUrkiSSIypHBM8dcVUqnGpfUA2Npc9WB8MxhH29crzYsTnZtVooUFuV9KWrWIFtX5uVXzqCdTT4WH9tHvMLETHtQoJ1YyvtDw1saEXl0ZNguTWPNXudPfnop8LD0ksE4xngqFcYbW2LZF2tnopJSz3ZhjYqTZV2oHMK6sq6oX7Jo0fwdmTYn4uFXflvd+3nzpYpKn.M7Q66+YVFctPwBol42Vzxn.DzryFh1FgYdb+XujEFkp2nnntDgYetXOM5RvgBwzgCG99doGEkm+rGBLPAWpngVkpjvJqQYiMZMMTwUe8reHwRQxnyeul3kIuR9rnWQ6zv5xr8sObKrPEX2xL0hVHkS7botgmihJFuTX7b4uR8rRt7W8Zeb0k.yI0DMajNnog1NuCWJKoEHUT6TbLUwQ8y3CYWlcB1GNGWph1EVzliVj2M2TqhJaucfX3h+h3DjauXTbQKRLjcJNXw5r2ozsp5lQOrtpTlG6V8ZQGjMKjZaaYlMx0g8cxgmE1qmJeWMAO7aWkQmoDpk0NXltj94xRM2Kah18IxzX4CCOpnZ4EN6EGQq1WbCa2r7q3JZYZrIohKgUL2amkl3aQFlKOvhkRroSn0PuTFshpAY+nxFQ0zBJ5qUhMOJrGcb.QiEjBXprOrX1tTmH3Pd6WuKU8EusiplhShDbME65iMM1sPM4305hs3+fYuenzUU4HgSktZGEFT6pExHbqI+NgnEPubw9H7SkxZ2EMYZhKzcjdT1kPREHIWcRY213fEj60hCYJnPC43UmN+05Php8hIyjrJGeCxMyGlWrPtwQiSjbwTlcwCNJtVMzDRdsT5onYBm0QuKPq7FUQ5R.IMaxKqf+do29RcvY6xaXIwYDl6E1+C4khUAD0vsbp0AIcVGRa3X5vd+vYcutdHkPra7dYXyI6raLwZZDHWc1I+HM+EG6vpNKaQbFnXcbYxCFRTPZgqoxlfmOuRiiIPdaLMQolPHhGNpWhv3ZgjjU9iy95nAruKwkBelrA46idq3TD..K3.L.zT9+ydd+rxSLjuuyd6eoF2A+XAY+s.tPiedD.T9S2K0iQ4S6FOQV.O23JHUC+bAzA744urGBg7FL2DsKQSuXxkAnRrHCjfD.5wezI7wYvaMtCFpatwPS9m7tIWKdtKyacoYY9a9M0aHLzn3Y22Yv03wucu+yQ+s9LiF+O+5fn8QyutciOyRRPBLgRfCfRPBf.43MaN44MlmPPloXzM9C5reAJAB.zNWyOKx658t+4yczf+yNjlu7XMiYFigB6f6dHNRWjq+lZO4Rt74FW4q4N1eHe9BkB1MKwiRfxf39Fx2gR+kaxPbLz3WiB4UtyMVvGBHX2bjnAxf8505nDBMygvPIL.L+83uIzOyvSdBGsCpwi01bt60uoNRY5jE.bqGqRRGZHIgHAw9FqsJP4YYuDxTO9g9f5enOGiBc1Sg5VSyr1FuNj2q4SCPy5Jdbl8t6+LsnPvlrEVjDFiQX.AfeOtzji8eHD74N8QDL.P8ta5+4OZyCBgF+XXXT9Cxl8c8.7YPemuyeb7GBLMydwtu7fM92+ZY13N1a3x3bqln3A4Mkhspx47vpq1HcuvQsrfmPpKWl0wT5k6Awnqwq04UBT7jpaNHZSUjVQzZ8hTof0RzpPZBh6DFHIr2uuot5EtuQWhO+TFci2kXZqGffHgp7Z4Hw4bJjBMCHy.fA.HKZ.TfzDAc8J9fHwoAhAQLLDCn..A..P...f...P...A.vfGjL.Lw3n5MVcktCt95R1O8V3n2UxFv2tLDu6ldR.gvHtrWDyLaLfCY.zLrPrEYEsWmQ6zmJOLLdMWElAmYc1TLnUPkMMWOZlIO.ITIGj+Rcnl6nhiKIX9GEE.NpSPY9qeVYzCX9Oay9DZP0MoEnWpMUml86Br1URZdZsR2thBrYLsnEcgHh5uFA.N0bx.dwN5elkLYJRatq.1HmK51I0GmFv.AQ9lhIu02n5tSzqBE0GcowqjMm362YjkrbX3lnHUWDki29nbsSpkg8eOGSb4rYu7CpPV+oRt5.vwKtKPXlcqmB2bOhPKNIoHzPuiTy+GD3bZiPqf+Aky9lg17fSmHbSVdT1sZ7ZWGEvTY0b5cdjNzhIUjBRj.MUHJSEfWUMdV+cGs4lupLn7JgkOZWGPu8bIvMG3qNZhg95dBRn5bHyFmguEmqhzA1IzGvEQ2617kiPrYzXIWMnDJnMcydKuN+C6wkbmVUgpr77HYUJB8ul7+m9Q+.K8iEpFeQmGMUybZJZJLXxTk2MiMvsXetYRDMliR02i5wmwzvQyhWWnBPEbkwykZnlF7VsOczTD.fad1qIFxAI6bhNO4H8NsQrn9Kgks74Aqjf36YoozwzeJAZORmKfv2bIq86.t4MXSvmjxiB2hfK.sWDAAcEET71Ptjfyrhja.XTc6NE5AXppeqm6yNJnM1QT5HNuUoGhqBlEpUDS7zCpet8UBlw+MZ4aXcy.CQCWCrDS+1p1W0TBF6oHRuCixXJkdnmjnCJXBA3YwYYnfcSy85Gx8oQYz2V2j35RbuBcmtv9lFZSWydv5ZkI1UMxDR+MdgdZocnSDZctjEIo3TnDmFLdU4BunOAAL2sTdzO6ZU8GMtIpJNWb4JInmVO4qAk96ft7neV98mJ6learBg.fxuXEyxNyuZ22a9PFe52uspWPjlcJCHkwuOw06Su+VIPG6cq4AWOd8e2feWfUbZ8QrByKdH8N+7mbXe1mExLmJ71mhgak+Jto5RN3vf0Jp4PcrXeO5tlaxNQhakGl5so4ajxRR6h.MtxOzVQJaqyIU.O4Yx7qegcfmkKZSQU1EKfrhZktHslh7.m9FA.G2Hsku47ZcLT76I59qtPCo3kT4COZwyqp4QIxyezpqCHQISg.scmYT.e6nOJmupeibwc9.CBRfCmrC1Eo54U+zcK5txf4UZ2h6Qpezxrz7UcugzsNAixOs54jPY1UsWGQZODP9Xl8Rqea1zq7z64zS1iMFygSKon5V+XB1tFxZwn8htweVnILtS7iTOFXvhFFu7tC9oe9facHPni2EvNfFR+SGLs2eXKuYCm1hL48tec22i6PJMWdYU5.Pavz2GDVDD.1pA1CkF5dGBb41j9B68LfvyeLSyd26R6Qz0G0EK50G7C4C996A+VdNNqfe7lUMKcRl4iZQlNfRuaJnn8xVbOxZtObfooyUGMaEBZWHp55+0IPm2vC9NR+xwAqWI5HX0GPBxPQHvDQjB+xhvTCJMAT3G0Y1vXOKyVbCigEP.sxJflFIt1xlfY9y1KJ.g0MRMh2fABQf33kbfkOeSiq6PLcHthgYJFhU81eW.f7AAJMzey3b1.jSvNWv0e4yUhDlAEedEGJm9Pv41mneccCV.+lLHJmEAjMzTfu7j8hCwRjGNJ+vOpf4Soi...lNB..v5H...");
}

var WelcomeScreen::populateProjectSelector(const var::NativeFunctionArgs& args)
{
	auto recentProjects = ProjectHandler::getRecentWorkDirectories();

	recentProjects.removeRange(4, 10000);

	if(recentProjects.isEmpty())
	{
		setElementProperty("LoadFile", multipage::mpid::Enabled, false);
	}
	else
	{
		auto& handler = GET_PROJECT_HANDLER(getMainController()->getMainSynthChain());

		String items;

		if(handler.getRootFolder().isDirectory())
		{
			setElementProperty("projectLabel", multipage::mpid::Text, "Load from current project (" + handler.getRootFolder().getFileName() + ")");

			auto hipFiles = handler.getSubDirectory(FileHandlerBase::Presets).findChildFiles(File::findFiles, false, "*.hip");
			auto xmlFiles = handler.getSubDirectory(FileHandlerBase::XMLPresetBackups).findChildFiles(File::findFiles, false, "*.xml");

			struct RecentSorter
			{
				static int compareElements(const File& f1, const File& f2)
				{
					auto a1 = f1.getLastAccessTime();
					auto a2 = f2.getLastAccessTime();

					if(a1 < a2)
						return 1;
					if(a1 > a2)
						return -1;

					auto m1 = f1.getLastModificationTime();
					auto m2 = f2.getLastModificationTime();

					if(m1 < m2)
						return 1;
					if(m1 > m2)
						return -1;

					return 0;
				}
			} recentSorter;

			hipFiles.sort(recentSorter, true);
			xmlFiles.sort(recentSorter, true);

			hipFiles.removeRange(3, 10000);
			xmlFiles.removeRange(3, 10000);

				

			if(!xmlFiles.isEmpty())
			{
				//items << "**Recent XML presets**" << "\n";

				for(auto& i: xmlFiles)
				{
					fileList.add(i);
					items << i.getFileName() << "\n";
				}

				//items << "___\n";
			}

			if(!hipFiles.isEmpty())
			{
				//items << "**Recent HIP presets**" << "\n";

				for(auto& i: hipFiles)
				{
					fileList.add(i);
					items << i.getFileName() << "\n";
				}

				//items << "___\n";
			}
		}

			


		//items << "**Recent Projects**" << "\n";

		recentProjects.remove(0);

		for(auto& i: recentProjects)
		{
			fileList.add(File(i));
			items << File(i).getFileName() + " (Switch Project)" << "\n";
		}

		setElementProperty("LoadFile", multipage::mpid::Items, items.trim());
	}

	return var();
}

var WelcomeScreen::setupExport(const var::NativeFunctionArgs& args)
{
	auto b = bpe;

	closeAndPerform([b]()
	{
		b->clearModalComponent();
		BackendCommandTarget::Actions::setupExportWizard(b);

		auto np = dynamic_cast<multipage::library::ExportSetupWizard*>(b->modalComponent.get());
		jassert(np != nullptr);
		np->navigate(1, false);
	});

	return var();
}

var WelcomeScreen::showDocs(const var::NativeFunctionArgs& args)
{
	BackendCommandTarget::Actions::showDocWindow(bpe);
	return var();
}

var WelcomeScreen::browseSnippets(const var::NativeFunctionArgs& args)
{
	BackendCommandTarget::Actions::showExampleBrowser(bpe);
	return var();
}

var WelcomeScreen::createProject(const var::NativeFunctionArgs& args)
{
	auto b = bpe;

	closeAndPerform([b]()
	{
		b->clearModalComponent();
		BackendCommandTarget::Actions::importProject(b);
	});

	return var();
}

var WelcomeScreen::openProject(const var::NativeFunctionArgs& args)
{
	auto b = bpe;

	closeAndPerform([b]()
	{
		b->clearModalComponent();
		BackendCommandTarget::Actions::loadProject(b);
	});

	return var();
}

var WelcomeScreen::loadPresetFile(const var::NativeFunctionArgs& args)
{
	auto b = bpe;

	DBG(JSON::toString(args.arguments[1]));

	if(args.arguments[1]["eventType"] != "dblclick")
		return var();

	auto fileIndex = (int)args.arguments[1]["row"];

	if(isPositiveAndBelow(fileIndex, fileList.size()))
	{
		auto fileToLoad = fileList[fileIndex];

		closeAndPerform([b, fileToLoad]()
		{
			b->clearModalComponent();

			auto& handler = GET_PROJECT_HANDLER(b->getMainSynthChain());

			if(fileToLoad.isDirectory())
			{
				auto r = handler.setWorkingProject(File(fileToLoad));

				if (r.failed())
				{
					PresetHandler::showMessageWindow("Error loading project", r.getErrorMessage(), PresetHandler::IconType::Error);
				}
				else
				{
					b->getBackendProcessor()->getSettingsObject().refreshProjectData();
					b->getBackendProcessor()->clearPreset(dontSendNotification);
					BackendCommandTarget::Actions::loadFirstXmlAfterProjectSwitch(b);
				}
			}
			else if (fileToLoad.hasFileExtension(".xml"))
			{
				BackendCommandTarget::Actions::openFileFromXml(b, fileToLoad);
			}
			else if (fileToLoad.hasFileExtension(".hip"))
			{
				b->loadNewContainer(fileToLoad);
			}
		});
	}

	
		
	return var();
}

var WelcomeScreen::startupSetter(const var::NativeFunctionArgs& args)
{
	auto& so = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject();
	auto md = so.data.getChildWithName(HiseSettings::SettingFiles::OtherSettings);
	auto cv = md.getChildWithName(HiseSettings::Other::ShowWelcomeScreen);
	cv.setProperty("value", args.arguments[1], nullptr);

	auto sf = so.getFileForSetting(HiseSettings::SettingFiles::OtherSettings);
	auto xml = md.createXml();

	sf.replaceWithText(xml->createDocument(""));

	return var();
}

HiseAudioExporter::HiseAudioExporter(BackendRootWindow* bpe):
	EncodedDialogBase(bpe)
{
	loadFrom("1351.sNB..D...............35H...oi...aT.........J09R+fYaCMhB.lpipl.uzliJMsIeyVRcLyRnboK.QHkJkN5rNRrVYTPlK3YjNfmAVrPnFoBvm.nI.vMiSspk4bNAJSm3sVOtzLO664uZKz9PWCDGRmcaLDW9gqlCqfZjM9zQOksVsPaDCJUvTQyAX1leNcw1l+LoxEMWl.WmtQruJ8BFMVvXflLWzTwhlGj8wbfFLVzPp42kLWrbPlFVmaYPkKWrbgRlJVfa9y4bUMQRFJ4FpMDNjBiX3FpfXqgRv87FHfBpusq0hze8CUtg5a+ApLyL.QxMT5qc12oODUlAHRjpCg7DQNxyeS627bKwubgqqWX9droK.YunM4NRPETjWsSWvwBcOFiyjIGfcwaDSlnwk+TtyBZYjw9zJ8O+VMka9oz6DiKnM5ONYPdTuavG5ZlVNCPzlbgRtA6croBZN6sU5ajVDBa6ct4j9zhr6E0hDoEEiK7xgZE7z2eAHfRaSyq3oN+2EKzu4mzOTVZZL7zJ+AAUdBpMOQxDITZmiAXka9FoBRbcg8lCuHYo4F2xl.3lK6sDeA8QMmd53AjC5VOmQdjmMi+mtM+FMxa++sFfZUd.kLVZQwJsnX19h1WUmCx0t0FSJTX+UuerXsXGiy9xU6sQnTP0x8XMbKj29vfbSl+uWUPyWPMKysmlwysm2FpjZRxF4HvdshSP+sVH2QyeRwsCbiasyuZ4c73e1G+vqu0BJIj5GOJ..iDPGWFgS6a1zYBOMHBGMFZE0RpDCLPlOsg6ChCoBE.oXfAx3Uy+uwlcn83O61d0nPha4QQ+yd+mglbslt82mVlakQ0TlL23TsTSu5ncNqZKTSAbutX3Eeukbvp2dHFWKGpkjh5EpqJS+ru2XeewOM2WLI3iPdzC0O51Wk1qG1wfXUiqls5MuJ5XvMmKoXa095aKiioC8r0PfPfZjcJECM0nIPQPBAw.XfjNpFdfrXCyBjB4bJPAf.HnBFH..Af7PBfPBJ3fdvfLUgQLTBSJUVx3rvOptAl98dNFlQBZnI3BoIIuNHcBFjn5ZZX4EJC2U91+y1aYC2CfPeXgnahFjosc3tk7FrUjERdEc1dlmoE4m3xKNO7uAdg1UKKxqqcg9YEWNOicsnwUPT4bFnADbj1hThD8Ig2Ms0hkSSIEG+4BLY97sMeII1BAf8b+VXhoGnIT2JYIvfCf9UP3oDKGluCrY78Ffi1Yb8MSftFZcx2UaCQ4HfD7rVICw5F76RCAV4lLiyjSCP0qFEi8iLSAAlFD80hMAYjtGfv6avNeHXoSZKiIp91yahCC1c5.glpCfRrt887MdW9v+tnGqz1sTlKDXAXgwp5PUPkDjFe4d8Uv7W4ZtZF.0BLgBdNDw.LfwC2msZA0ql80nrXOUMC5g8n+z.yvPxOgCnPiDFhHA8aibpfhr3E9CnTYUnNc023cWwvq54vjnFpTuI0dTowzgFSbdRqfcXcgzyA.CgAH.39lw+IM1DBEAHilBaHRNj4TLurooVfB67K+8VdqgV78K0YEh2gYuA2E3STW5O6.fQ+XGWfJ0zxta51CB4u3XydgPOkOOAmD+FwLZa.nwmrY2FNpRxUIqtov1acGG6TLF0qy9.21MMqAP3nwJQSBvq.KJ2o9CfZ4s.H2DCeXsFhIewCN1wfRjrLUy36+i7yhCixrop0+aLCYYQQZl16O2j+xFC5BimKx5R5BAwR5vaCm9N.2GtveFzA5.FC7JNKOB.FPVrJCzL4haA9QSQ+nHzW5H..foi...rNB...");
}

void HiseAudioExporter::recordStateChanged(Listener::RecordState isRecording)
{
	switch(isRecording)
	{
	case Idle: break;
	case RecordingMidi:
		recordStart = Time::getMillisecondCounter();
		currentState = RecordState::RecordingMidi;
		break;
	case RecordingAudio: 
		recordStart = Time::getMillisecondCounter();
		currentState = RecordState::RecordingAudio;
		break;
	case Done: currentState = RecordState::WritingToDisk; break;
	default: ;
	}
}

var HiseAudioExporter::onComplete(const var::NativeFunctionArgs& args)
{
	if(readState("OpenInEditor"))
	{
		auto path = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Other::ExternalEditorPath).toString();

		if(path.isNotEmpty() && File(path).existsAsFile())
		{
			auto fileToOpen = readState("Location").toString();
			File(path).startAsProcess(fileToOpen);
		}
	}

	return var();
}

var HiseAudioExporter::onExport(const var::NativeFunctionArgs& args)
{
	auto location = File(readState("Location").toString());
	auto useNonRealtimeFlag = (bool)readState("Realtime");
	auto length = readState("Length").toString();
	auto midiInput = (bool)readState("MidiInput");

	getMainController()->getDebugLogger().addListener(this);

	currentState = RecordState::Waiting;

	auto lengthInSeconds = length.getDoubleValue();

	if(length.contains("bar"))
	{
		lengthInSeconds *= TempoSyncer::getTempoInMilliSeconds(getMainController()->getBpm(), TempoSyncer::Quarter) * 4.0 * 0.001;
	}
	
	getMainController()->getDebugLogger().startRecording(lengthInSeconds, location, midiInput, useNonRealtimeFlag);

	state->currentJob->setMessage("Waiting for MIDI note input...");

	while(currentState != RecordState::WritingToDisk)
	{
		if(currentState == RecordState::RecordingMidi ||
		   (!useNonRealtimeFlag && currentState == RecordState::RecordingAudio))
		{
			auto numMillisecondsSinceRecord = Time::getMillisecondCounter() - recordStart;
			auto progress = numMillisecondsSinceRecord * 0.001 / lengthInSeconds;
			state->currentJob->getProgress() = progress;
			state->currentJob->setMessage(currentState == RecordState::RecordingMidi ? "Capture MIDI input" : "Rendering live audio...");
		}
		else if (useNonRealtimeFlag && currentState == RecordState::RecordingAudio)
		{
			state->currentJob->setMessage("Render offline audio...");
		}
		
		Thread::getCurrentThread()->wait(100);
	}
	
	return var();
}

ScriptnodeTemplateExporter::ScriptnodeTemplateExporter(BackendRootWindow* bpe, scriptnode::NodeBase* n):
	EncodedDialogBase(bpe),
	node(n)
{
	loadFrom("897.sNB..D...............35H...oi...UM.........J09R+fIsAcoA.lrVfl.tzniLwvJeGajdMaHDyvtUbuc0f4YTAhcBXdU3KQESTMhwnxL3b9Afb.LG.7Dthf6CiiV7DCJUtTICBWfYKOh6GHpJUYpnjb1xdX30VcIikKGjASUlJVx7k3+BBYtXIyG8CVgoxBBX1vYN1AUpprTQASEKRNG5bztnlfgBGxyZAQOIaUvrWKExf9FBljKjSisP8eAMoZl91eXRUUkHhpYZ+sCC6dqIUkHR.plu4ucruD2AzizgRfauUMcKLNeYenNjHcn4jkZtLV.be+CffoHWzq.tY9y7Hqmy6VPSXwXCv8xgPLocxseanlPMgR6uojynq23lyoShschGi8Pa29LA+2ewYw+Bia7qyU4Lu0W5wRNL42K4nKqXZDUkDw2C41ztKMimbJeo8ho+S8oJ4LnKRcv5iGUiIsSEBefbQDxd57o8+41L0gZ+8KlWhufgwsm07wwtK1fkYv2HNTT+HsG6ghpZzm8zPyg6qCCQc9F8Jl5u98Bhx4cp2YNYI3oC4rIehZlS9ROBB+xP.lzZIP.vHR3HgRQJJpl3pwwHsqQSmI9rQj21EmUSgl1Sdw2LIG3eYIGrl6gO9jXykzFwimbrbpDzgAcxUd8.wYNGCleHcPddh3rSGM2.8ahz6sCBWtd2zMIKZ1q7YR6nrd0M9mcS30BeKoU6fsqrb04uuOgtvjYTYX.njnFRmx3PJ0LkP..DP..jQ.XLipIHJUr.IUJqBpIyB.QjVejCv.hLcr3rADDVtawtHJEc6PFDSFDXlFZwkeYetmqttN7BArpkJdLolMZDl93Peki8rNALClo0C1RVk9eJTFyZbMxCn9nvdmgpB1GcyPQjaOsX8ZDs4dYEB4wLl248O.3r06wnQKTVBEAw17n8AOxnHi+o8+J4KKRPeitpLXq.BY62kwMsAwIAD.FnumFHgx0H1.XHsWTD2pehVnKAxnIgEGHuA3n+dufq+SGFwLxlzf73CurcPUcxa3VbNKWm.ZZ8novmexiRbZ+SPydVz8lBqtAjpu8fnjeTeWdRDxui7Xt1lph1OU0P1TQ9k+nHKZnODXREj+EoUhRDnQos1CtmTna.ho+AcK.z4gnfhQ9bmmHWI9kyJLfeY9C8hety6kNB..X5H...qi...");
}

NewProjectCreator::NewProjectCreator(hise::BackendRootWindow* bpe_):
	EncodedDialogBase(bpe_),
	ImporterBase(bpe_)
{
	setWantsBackdrop(true);

	logData.logFunction = BIND_MEMBER_FUNCTION_1(NewProjectCreator::showStatusMessageBase);
	loadFrom("1816.sNB..D...............35H...oi...ra.........J09R+fUdFUbC.5QDfLvBrPVsMLVGrkiDJT5fzslid35ByE.142bHSC4V6+8W.wLiI0US6On+C1eic4TL.3B.t.r4s7KMiH16wfV83cChxKn5XIRRjl3bNBWNtb5Cji4RCP5CnTPZh5meOVad1lgLz9fbecgXhxon3xObeNbinPxzgM15mVY191rQILghEJWlvR2zsZcbFRnT4REIvsoKFa6QeKWjvxAtPQkKTj3x+w9Vl3hEItzgi6JTTQhInnhs4rCPoREIpLgBJRjSQPKucLTdjHMwjbJZ2.uQAeX3tIRzoaRPq8JfHBpsE0FgzeMinbJ5c9EQYloCfjSQ5qY1952DkoCfDU.Ryw.2H2J+rSCbas+IiFHdly5h+wPVT8PcPjy3cwwBKr0bEFITrrXHIZBTdqblE7xHW8IK8OemFJs7qzynwF8FCHH+wsVvG5uPKywkKWNPEECi3XJnBJpSmb5c66bS1FzYNkEfXbgTN7Eo02+.7wKBkiaM8wXLm+ukmHaSQ8e1NmbTbqQFfDISxopuwgPPq0BPNA81YQd9K4Bw0AAqyc4zk1uH29NtNf35zqGsgH5Y87MZs4+tUA35j4zywExac.UMt.Hq9YPdr13f.bonXHm28LLjDFpdcCL7vCwM46VPOuWO1LO7fiobZkGakxwOmX2ZB8O+FYoSYokWgIQc1pHi1AzCkVrQCFILZo4FcesZkiuszOald2rMegx7LZKjszvFAmlv8cVb8OPV3fP2pzEzgbyiULiZ1VOY2bnPqKufqGVjJPxx8n4B0Wee9wSWtUuNj801SBNWBEVOmlXT67+X4r8CDuIJjKd1wSSYdfl..BwgK3Pz.pLQN8TJGHP1rXofp+wwQ3byRk1HWIEYdKhHlhu+WFzx2nd6B4YZ6qrn1s5XjjjVO3JFTwjcv3FC+AFQJL5X8cNaa9SZfD5EHYgZoGfP5gFTYslm.+Qk+4EBoZYZe8byd3HyQGQki17.RqXNEVQmv8R6Mp924nSa6Gad0QK+kJm+1YGNs8jQlhOGte9vsqlGeZUDbczPALxG5OPZbcRP6+w1zPxxXu50kSnk8XgaHfHDPAJJpQ2oPnYnAPHAD.HPPL..EJnjZcFff3wxAiSQgQY.XfgH...LPf.CH.D..Bk.XAnXv7o02TQsd2F4mt2XnUHayNpWhYFfnvasgfbXg.2f6AMABK2wFSFxW+AvKyA+6V+NViiKKcYA8KUiWsYcqcxxzUHtTRR1I8h0Iy5.6I1B+B6BH6oXa40vzyi6m7D94W1Qh0gyCOAWROdIGETqNptTLNXE68CzyGqfJnNs0jPaPboX.bVEH9fC.LzyC1IhaqvUT4uIMMjOsslD7MrilDRSWjxLtEu1SBIfHMRKHsKaFXvtvaupdIwjH2ZLL+5UjljLB6mWoRkmt2NemH0bLuxRDpni9w7lejm+qSg8knIgOfPpi.sOak3qOTHdlX2i46lqsuvn1Im1bjY5ome90KGpVWkEbcH6XNHFxm3U3DAJSQ2R5RMNYczADDVbvv.zRC0Rrff6fRllwA88MTaJjWrjNrgwjziZCWr+6dYZQMJHqWnZOhYAHn49Y8Ld3G.9Il1TuWnBjx3aJQQ5UdWpA+3CEC8ikN314LGFeK+PqNZDPjnEl2q1TihJ2llEZngnyprbfv64pUkOwGNHjkRE9apjsIj9paH.bKisXDwoFZ9Y9iEkGYe34ooV5CRCQpZzgzXFxskUl1hbNviniGsOtqUas6PAyxvbDCexJfrfGLcMrLdCH1rfGYU3r.z+JipQ6JL27S2Es43gmq9laBgZgjvYXEr0PZliwICcSBQVTVy+X8.saaiSnzh.ppfgr69B9UpC80osG.oz6Zgg4c42sjDIBXQXkAV6a1FzLdBNiMGuAJTa521w1qipjFzS6MlKA1gAbFT8s4WZNzaUJBBly0H6JURga5esi7.JGHfnVzf.qcjmjVGkrdiFj+yZNyL1iEyGtfZksEjzgB3nNZNHQfTmM5J5FeCLyCfIBQZBSoHHe+uLBloVo1XpucHk0IZ+lRIK.k7RwNk2Tdh17J0H.bz.g08fipvbeXED5O2lSF7IgAG3hE1LcHtSDYme+TM9m2pTLvmJrOe75w9PTxQ3NfgVFr3gd6ahJ.9JkuTCEtBasCCbn9IwZpLYCEbh+.+lBgu37uTvXuXIIP4SA9RB8SEyOMkkAvUiwd5wyPD6GFL3qmm4VXbxpPDDLYE2DS57Hdhq2NnFAOvpcSR9Zrbk.J7elHLGzOtjvQRpSh5ifk9CaL9E+.xqUng++cuGci7YB.YAxikQkz2dBZ9EncBGOgxkD135CcmO.7Jfh21Ee4jMlwQOwdcM593mG+DwrCT5H..foi...rNB...");

	dialog->setFinishCallback([this]()
	{
		threadFinished();
		findParentComponentOfClass<ModalBaseWindow>()->clearModalComponent();
	});

}

var NewProjectCreator::onTemplateSelector(const var::NativeFunctionArgs& args)
{
	auto d = dialog.get();
	MessageManager::callAsync([d]() { d->refreshCurrentPage(); });
	return var();
}

var NewProjectCreator::initFolder(const var::NativeFunctionArgs& args)
{
	auto& sd = dynamic_cast<GlobalSettingManager*>(bpe->getBackendProcessor())->getSettingsObject();
	auto s = sd.getSetting(HiseSettings::Compiler::DefaultProjectFolder).toString();
	jassert(s.isNotEmpty());
	state->globalState.getDynamicObject()->setProperty("DefaultProjectFolder", s);
	return var();
}

var NewProjectCreator::onProjectNameUpdate(const var::NativeFunctionArgs& args)
{
	if(auto md = dialog->findPageBaseForID("folderPreview"))
	{
		md->postInit();
	}

	return var();
}

var NewProjectCreator::writeDefaultLocation(const var::NativeFunctionArgs& args)
{
	if(state->globalState["UseDefault"])
	{
		auto& sd = dynamic_cast<GlobalSettingManager*>(bpe->getBackendProcessor())->getSettingsObject();
		auto compSettings = sd.data.getChildWithName(HiseSettings::SettingFiles::CompilerSettings);
		auto ct = compSettings.getChildWithName(HiseSettings::Compiler::DefaultProjectFolder);
		ct.setProperty("value", state->globalState[HiseSettings::Compiler::DefaultProjectFolder], nullptr);
		auto xml = compSettings.createXml();
		auto cf = sd.getFileForSetting(HiseSettings::SettingFiles::CompilerSettings);
		cf.replaceWithText(xml->createDocument(""));
	}
		
	return var();
}

var NewProjectCreator::createEmptyProject(const var::NativeFunctionArgs& args)
{
	didSomething = true;
	GET_PROJECT_HANDLER(getMainController()->getMainSynthChain()).createNewProject(getProjectFolder(), this);

	return var();
}

var NewProjectCreator::importHxiTask(const var::NativeFunctionArgs& args)
{
	didSomething = true;
	auto hxiFile = state->globalState["hxiFile"].toString();

	getProjectFolder().createDirectory();

	extractHxi(File(hxiFile));
	createProjectData();

	return var();
}

var NewProjectCreator::extractRhapsody(const var::NativeFunctionArgs& args)
{
	didSomething = true;
	createProjectData();

	return var();
}

void NewProjectCreator::showStatusMessageBase(const String& message)
{
	if(auto md = dialog->findPageBaseForID("folderPreview"))
	{
		MessageManager::callAsync([md, message]()
		{
			md->getInfoObject().getDynamicObject()->setProperty(multipage::mpid::Text, message);
			md->updateInfoProperty(multipage::mpid::Text);
		});
	}
}

void NewProjectCreator::threadFinished()
{
	if(!didSomething)
		return;

	if (ok.failed())
		PresetHandler::showMessageWindow("Error importing project", ok.getErrorMessage(), PresetHandler::IconType::Error);

	auto newProjectFolder = getProjectFolder();

	if(newProjectFolder.isDirectory())
		getMainController()->getSampleManager().getProjectHandler().setWorkingProject(newProjectFolder, true);
		
	dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject().refreshProjectData();

	if ((int)state->globalState["Template"] == 0)
	{
		bpe->mainEditor->clearPreset();
	}
	else
	{
		auto presetToLoad = newProjectFolder.getChildFile(ProjectHandler::getIdentifier(FileHandlerBase::Presets)).getChildFile("Preset.hip");
		bpe->loadNewContainer(presetToLoad);
	}
}
}	
}	
}
