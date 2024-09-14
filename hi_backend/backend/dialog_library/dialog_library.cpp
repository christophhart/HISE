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
            xcp.start("gem list");
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

WelcomeScreen::WelcomeScreen(BackendRootWindow* bpe_):
	EncodedDialogBase(bpe_),
	bpe(bpe_)
{
	setWantsBackdrop(true);

	loadFrom("3213.sNB..D...............35H...oi...gw.........J09R+fI4F8JF.JFJHerBrJPN9H3ph2mjqppSC2ouTsfWI2bUS4U0OrTPGF2QMb28zBr+C5+B4+vz.pGv4And.9NTzzniENPcPbhzIBmDrwLutKhPqN4UiiC53EQINB4cCu4xb0Ac8U5oSA83TV6OADtHn1zj2PNuXV0.Y0Vcxh2DzpDCRBsSffnR0boCigTVDtLaln1JmBVtWQV7YuPDKEGuW1JS1qCxElUCHX.RGznEGVtDjPwnPxgfWlN1pwYk5rXSPP2zNEO6kQHDFzOcKkgUG+VfVcPo2nqNd+VD8gtllJ.AOGneL+5Vdtja.P7b.WK5wYL.je18hX9vWJfBfm+7ruGlubvAIfZTTmGG2DYwoz.QtFDDW988w6++4ECxaQweXd+b.WTuwFP.QQROGL9A0nG.HH3.7bfuu6eFT1+P7DvQvAj4c9yAo3cooF7i.NBZ.NB14K3sAT.FO19y4Qu+YOECofuxRrM9Rey3I0i45HnHHIdWa5ySR43xL8ZpVF71+KSJhyjk2e8+xoWAJ7xZ7z+O4L8ucNAyr96ykc+sfD77D.YyMNbi0e4RuroDICVFtNXsAw4wLWCIxP0UUjOChyMx6K3JqKJqoKDUWFnt9bxeqCWVTWX35cEhJpCHRjA6eSFppnpjPXfxe128p3kHHDR4NZuDHT6ydFXxwdH.3tmv.Jvi2mhYDd5eLjBdNnfOuunfmmmm.nAdNn.7Gay6zuWAOOAPCDnMJcvd2YDJLozwwo1viKGzqTEmI607xBZhCgatjsEfxS1jv6p5TcDsvJn3UZLBhbiJiWCMP63uSlERTwjt6rwk3BkB41XFFrsqqluTZrlX6TDBg0OYroyF4ZPBsVhhgrqxGqDYyaEIZ29zZbqNfIdgxb7dHAz0dofNf3wlh8VnqTrFYIRx835hZ2hzvswsogsOrvttVb6r89mfnkv.klEYUzH2aWhNP7RbX.0g6BDDl0ADCuKhMdwM16H1ltDq0dyVmNrDoCyZgjHbUpLhGxULCnVFdWUrkiSKAyp.BM8ZcRUqnGlbUA2Y0M+Jg9FXPja+MT5r1ITL2pzzIi2JekJ1KBFpX5uUXTK6kwANJp76CeGD3lkBJD2jTE6rXWVhX61kBDyZMkv2IqGGrWuGOvT0dFf5v1fTYzfUiNLNq6EMiaGjD0jHZbLPvr0IspqDjVk8108TLdizXrXuQpLz7WCLPSXuebSZ0K0ESfF5n88eLKiMUjPgTy7aKZYT3AJ1YCQaivKNtcrWxBiQ0KDDTWhPrNWLmFcIzLgP5vgCeeujihxye1a.J.AUlngViljnBqQYiMZMMTwUe4rcffchjQm+ZEwKSdQnyhdEsS6pCy125vsfxTmsRoE13OVwV2gjWK+cylXMUuIvc5FdNJpX7NYgik+F0wJ4xe0q0gUWBKmSQzrP1flFZ67NbmnjVfTQsKISZRV8ynCYGlMxUGtrJUQ6BJZywJx6lapQMks2MOLTweQbBxs2qItXEoER+8f8pydkR2npYF8t5pQXbraUqEcVN6iMJjZaaIlEh0g8UJiyB6kCkuql.G9sqxXiTB0x5FKRVQ+bYol6kMQ69DKjnuUI17ltbrI6AlpGErPYcXsrcmNQng71udUp5Jd6rpdRShDbME6piML1sPM43s5ho36bYucjzQU0fflIc0NILl1UJjP3UQ9aBQKfVoh0QXmIk0tJXvzDWn2D4nrKABTfjb0HkcaiCUOtVKNbofBLjiVa572pCIp1qkKQvpZ7MH2LcXVgB4lFMNQxERX1ENzn3V0.SH4kRoWBFILVG8t.sxaPAoKwiTLIOrB9qkd6KMSy1k2fRhSHKWqqeGxqjpBHpga4zpCHzYyhxnwLyd6vUcsrbDkPra7ZYXyI6r6KwZJjGVc1HeHMekrcHUGEHHrcChyfDaVkHNXffBRJbLT1D774MZbLAxaCIII0DBQ3HqVABCqEQQU4ON6qiFv9tDUJzIx1iuO5khSwhwKkgBc68ayvGVmCecZLwg1lwtNaVlBcYJpkUM7L4KR6H4lGZmEBNzjIg.PBB.FPJO9H8fPxm+derxS7CZ2KZ.m7ND5mSev+Bfm2fG4avYvvLvr44S59K662rFPPzn7D9fUnko+MOSvOal82cdqWj+z8wc9BFh2e98jo6Cgj.eem815+G+bwuT6+Lc.cfC.5wczIbfC77wWvaMdC9o+D4ex6lbs34tLu0klk4u42TuevOihcFbMd7a26+LzeqOunwedNy4WGDsOZtwmYDJwhLvANf4SBXvyANvCHGmIyIOmQjqMOe34x9iYJFci+p7j.B.ZmiIKF1e3c81m2nA+m8GLe4w5EyLlS9IetwS9Zd19+3y2mTvtYIdDgmLHtue78mzeYl7CG+L90nOdk6biD7cvA1srDM.30lU9fl4O3mD7Lm6ds.v72i+lO+LCN4HbzNHFOV6apgTlFYA3YO3VOVihNzHJJAw9FqsJP4YYu7wTO9e99bL5yYO8oa0x30g77o+66knY8jqqqIuAYy9td.9Lnuy24ON9a+0llYuX2WZvF+6eLLT5ncYiisaY6LbXZtTQT3f7lJwRU4ZbT0UajtWjUKKvIb5xgYyjRqbOHBYMdqNsRPhWTMyAQDRUjNQzZ8fLofsRzpHJBhqjkGeRc0JbeitDedoL5FuKgzV2tgYpEsOJivIHnCnJuZLPXNGBwLzHyL.X..xBF.EINPPPzrGHYbdjZNhgYT.C.B...SRB.Yg.B..rIA.Aa7N1Uh4XnS5l26Ms3RNd.f9Ttn9MMXWEiWfYmWeT3nwvdPj7CBAhCAc.YnIGuK.iYJsmjkPmC3sNWnGz5C3kkF3ie3kDHJjryC5HH01QdXfdRUDI6xbufFRjOQ3wR1xvTE6cZIB4Q1QVFoNL1w36fY1FyzvYS951S.VR6lxJohRUV9xx.Nricip7pCVDNwHuWzsaTrpDxGfi0ljvNsAS3zfTQe0CcCWcDJyJa+I6mAkqv8ovg6FDYE8QoJAR2tfLLeRvfHQvT9e9wD5wss2AckSf3V9JgiLt1OlPi6fBR8alUXzLQHofyQYv54MvpL+JQTHGNoY4TpQViPeFOnv.lQZlZYGXDiNBTLrkzoFfZ2ihJ.AwSZA74GTPjTg88f5hGTyGYC7CcSFdbFXRTKPH5gnOiSToQrI2jm2W9ghiuIOtoUyYKgYDI7kAgyC99hraFIKhkFcRaUunqAmBszopp1x4ZcFRp9Ybs+JrapXXD4snuURh31GWNx+usRRGIVrXETSX0jbT2RZN4L5R4Mi18bWYqMFkYjXC6xZjYaUnX.UivYJYHck+BX9.93HTOOAckFQ8v9ljQLHOdqHdm0S8CFcbGIaoPnGCUfBHTKlls9CwZPMpEBXvk1Xxqu1jqIsG.Xz.LA+vvxBcGfE35XZCqrD1k3YdExFNK0hHLIbzdlU1zZHOxNqs9eiK0.l9apt4uraCWwjIOiAh9CQSbujjBVn2oYRAN2kWjOgASKhK2+4v9QLvGm2acOBGW75ER8jy.nOB967yhL9SCzNgis2Uk4XuKCTk0Nrh4Ra5hZNr4yJ+sAfE7vwLhkaOc2Xa24gQNOggYDma0nxCrattEQqnSW7lvSffMwgUbUOG3enjuKV5Bq8yXkcusLmbse0yT4uA05OcrY0d905E1ieTsBP7q4QESneOCfPtHa63ct14lwumnFaHE5qha1om9XiLjphDRzcUhMCLinrHuku3gxQ+ZIes2EwhS8fAt0X3LGjffS77MWYrcpVoYd49G49c92LadUcQ28kcKErG9TtmSRSwUQ9FQTLLS9w1bIneJnoCGS6mSSMdDqwlOMizJs8iFnDObYgJ4Ba1+giIyed3QLXr4WDCu4EawZX0GL2gEeeQtrx.DD8GRCS+6YBH1hh585hs6o4NbogeNteLf14Hk+.ZEPvUqA6N8.vz9PHHoA1uhtK.DckiYlqg1n1ax0JCW33WLnFFMu8GHWnjr5+.9EJqFnTDiJOzv4CBMQ26fh7msHXw8bCH.yuh6Vl9OTj7PqBK+b0ttsvT+fP9cmH1Z.BDva..nLCR6Dl3Qppnt8jkWJNnXAFLtCauTyVafwzhIf9X0USnIdRYSQlm4XMwhiUjDFY2sYp.h08QLBFEdV0o6lEEJWZwDWI3TkzenqfrjEgy3+BGyowagXAnfzxhsEIdEDT7GCHg3jKCESnV9sqil.3ZjBgeUTTKofICXqrSN.xxLkgaK9kQQekNB..X5H...qi...");
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
				items << "**Recent XML presets**" << "\n";

				for(auto& i: xmlFiles)
				{
					items << i.getFileName() << "\n";
				}

				items << "___\n";
			}

			if(!hipFiles.isEmpty())
			{
				items << "**Recent HIP presets**" << "\n";

				for(auto& i: hipFiles)
				{
					items << i.getFileName() << "\n";
				}

				items << "___\n";
			}
		}

			


		items << "**Recent Projects**" << "\n";

		for(auto& i: recentProjects)
		{
			items << i << "\n";
		}

		setElementProperty("LoadFile", multipage::mpid::Items, items);
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

var WelcomeScreen::loadPresetFile(const var::NativeFunctionArgs& args)
{
	auto b = bpe;
	auto fileToLoad = args.arguments[1].toString();

	closeAndPerform([b, fileToLoad]()
	{
		b->clearModalComponent();

		auto& handler = GET_PROJECT_HANDLER(b->getMainSynthChain());

		if(File::isAbsolutePath(fileToLoad) && File(fileToLoad).isDirectory())
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
		else if (fileToLoad.endsWith(".xml"))
		{
			File presetToLoad = handler.getSubDirectory(FileHandlerBase::XMLPresetBackups).getChildFile(fileToLoad);

			if(presetToLoad.existsAsFile())
				BackendCommandTarget::Actions::openFileFromXml(b, presetToLoad);
		        
		}
		else if (fileToLoad.endsWith(".hip"))
		{
			File presetToLoad = handler.getSubDirectory(FileHandlerBase::Presets).getChildFile(fileToLoad);

			if(presetToLoad.existsAsFile())
				b->loadNewContainer(presetToLoad);
		}
	});
		
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
	auto chain = bpe->getBackendProcessor()->getMainSynthChain();
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