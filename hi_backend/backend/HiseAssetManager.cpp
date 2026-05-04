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


namespace hise {
using namespace juce;

struct HiseAssetManager::ButtonLaf : public LookAndFeel_V4
{
	void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
	{
		Rectangle<float> b = button.getLocalBounds().toFloat().reduced(1.0f);

		Path p;
		p.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 3.0f, 3.0f,
			!button.isConnectedOnLeft(), !button.isConnectedOnRight(),
			!button.isConnectedOnLeft(), !button.isConnectedOnRight());

		float alpha = 0.8f;
		if (shouldDrawButtonAsDown)
			alpha += 0.1f;
		if (shouldDrawButtonAsHighlighted)
			alpha += 0.1f;

		g.setColour(backgroundColour.withAlpha(alpha));
		g.fillPath(p);
	}

	void drawButtonText(Graphics& g, TextButton& tb, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
	{
		float alpha = 0.8f;
		if (shouldDrawButtonAsHighlighted)
			alpha += 0.1f;
		if (shouldDrawButtonAsDown)
			alpha += 0.1f;

		g.setColour(Colours::white.withAlpha(alpha));

		String x = tb.getButtonText();

		Path p;

		auto b = tb.getLocalBounds().toFloat();

		if (x == "info")
		{
			static const unsigned char pathData[] = { 110,109,80,178,38,68,0,0,158,66,98,144,243,48,68,0,0,158,66,208,16,54,68,192,12,190,66,208,16,54,68,128,43,254,66,98,208,16,54,68,32,37,31,67,144,243,48,68,128,43,47,67,80,178,38,68,128,43,47,67,98,112,116,28,68,128,43,47,67,208,83,23,68,32,37,31,67,
208,83,23,68,128,43,254,66,98,208,83,23,68,192,12,190,66,112,116,28,68,0,0,158,66,80,178,38,68,0,0,158,66,99,109,160,173,24,68,48,159,154,67,98,160,173,24,68,144,91,146,67,0,208,23,68,64,173,140,67,144,17,22,68,160,147,137,67,98,0,83,20,68,16,122,134,
67,0,52,17,68,144,236,132,67,48,177,12,68,144,236,132,67,108,224,68,13,68,224,163,110,67,98,144,242,14,68,160,62,109,67,160,111,18,68,192,235,106,67,144,184,23,68,224,169,103,67,98,192,55,36,68,224,175,96,67,48,136,44,68,64,35,91,67,96,166,48,68,192,
3,87,67,108,0,241,51,68,224,50,93,67,108,0,241,51,68,64,212,253,67,98,208,42,55,68,16,23,1,68,240,72,59,68,224,162,2,68,192,78,64,68,56,141,3,68,108,0,140,63,68,0,64,10,68,108,0,220,13,68,0,64,10,68,108,0,162,14,68,64,53,3,68,98,176,225,18,68,32,104,
2,68,32,61,22,68,224,249,0,68,160,173,24,68,64,212,253,67,108,160,173,24,68,48,159,154,67,99,101,0,0 };

			p.loadPathFromData(pathData, sizeof(pathData));
			auto tr = b.withSizeKeepingCentre(14, 14);
			PathFactory::scalePath(p, tr);
		}
		if (x == "more")
		{
			auto tr = b.withSizeKeepingCentre(10, 8);
			p.addTriangle(tr.getTopLeft(), tr.getTopRight(), { tr.getCentreX(), tr.getBottom() });

		}

		if (!p.isEmpty())
		{
			g.fillPath(p);
			return;
		}

		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(x, b, Justification::centred);
	}
};


struct HiseAssetManager::LogoutButtonLaf : public LookAndFeel_V4
{
	void drawButtonBackground(Graphics& g, Button& button, const Colour&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
	{
		if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
		{
			auto b = button.getLocalBounds().toFloat().reduced(1.0f);
			float alpha = shouldDrawButtonAsDown ? 0.15f : 0.08f;
			g.setColour(Colours::white.withAlpha(alpha));
			g.fillRoundedRectangle(b, 3.0f);
		}
	}

	void drawButtonText(Graphics& g, TextButton& tb, bool, bool)
	{
		g.setFont(GLOBAL_FONT());
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawText(tb.getButtonText(), tb.getLocalBounds().toFloat(), Justification::centredRight);
	}
};

struct HiseAssetManager::LoginTask : public ActionBase
{
	String getMessage() const override { return "Login & fetch data"; }

	bool perform(HiseAssetManager* manager) override
	{
		manager->showStatusMessage("Fetch user data");

		rd = Helpers::post(BaseURL::Repository, "api/v1/user", true, {});

		if (rd.statusCode == 200)
		{
			auto email = rd.returnData["email"].toString();
			auto username = rd.returnData["username"].toString();

			if (username.isEmpty())
				username = rd.returnData["login"].toString();

			auto display = email.isNotEmpty() ? email : username;
			manager->userName = "Logged in as " + display;
			rd = Helpers::post(BaseURL::Repository, "api/v1/user/repos", true, {});
		}

		return true;
	}

	void onCompletion(HiseAssetManager* manager) override
	{
		if (rd.statusCode == 401)
		{
			manager->loginOk = false;
			manager->logoutButton.setVisible(false);
			Helpers::getCredentialFile().deleteFile();
			manager->setState(ErrorState::OK);
			manager->showStatusMessage("The authentication token was not valid. Please retry...");
			manager->productList->setProducts({}, manager);
		}
		else if (rd.statusCode == 200)
		{
			manager->loginOk = true;
			manager->logoutButton.setButtonText(manager->userName);
			manager->logoutButton.setVisible(true);
			manager->resized();
			manager->repaint();

			if (auto l = rd.returnData.getArray())
			{
				manager->productList->setProducts(*l, manager);
			}
		}
		else
		{
			manager->loginOk = false;
			manager->logoutButton.setVisible(false);
			manager->setState(ErrorState::OK);

			if (rd.statusCode == -1)
				manager->showStatusMessage("Connection failed. Check your network and try again.");
			else
				manager->showStatusMessage("Login failed (status " + String(rd.statusCode) + "). Please retry.");

			manager->productList->setProducts({}, manager);
		}
	}

	Helpers::RequestData rd;
};




juce::var HiseAssetManager::Helpers::RequestData::operator[](int index) const
{
	if (auto v = returnData.getArray())
	{
		if (isPositiveAndBelow(index, v->size()))
			return v->getUnchecked(index);
	}

	return var();
}

String HiseAssetManager::Helpers::RequestData::operator[](const String& d) const
{
	return returnData[Identifier(d)].toString();
}

void HiseAssetManager::Helpers::RequestData::forEachListElement(const std::function<void(const var&)>& f)
{
	if (statusCode == 200)
	{
		if (auto v = returnData.getArray())
		{
			for (const auto& c : *v)
				f(c);
		}
	}
}

juce::File HiseAssetManager::Helpers::getCredentialFile()
{
	return ProjectHandler::getAppDataDirectory(nullptr).getChildFile("storeToken.dat");
}

juce::URL HiseAssetManager::Helpers::getSubURL(const String& subURL, BaseURL baseURL)
{
	auto storeAsBase = baseURL == BaseURL::Store;
	URL base(storeAsBase ? "https://store.hise.dev" : "https://git.hise.dev");
	return base.getChildURL(subURL);
}

juce::URL::DownloadTaskOptions HiseAssetManager::Helpers::getDownloadOptions(URL::DownloadTaskListener* l)
{
	URL::DownloadTaskOptions options;
	auto token = getCredentialFile().loadFileAsString();
	options.extraHeaders = "Authorization: Bearer " + token;
	options.listener = l;
	return options;
}

hise::HiseAssetManager::Helpers::RequestData HiseAssetManager::Helpers::post(BaseURL base, const String& subURL, bool getRequest, const StringPairArray& parameters)
{
	auto url = getSubURL(subURL, base).withParameters(parameters);

	URL::InputStreamOptions options(getRequest ? URL::ParameterHandling::inAddress :
		URL::ParameterHandling::inPostData);

	RequestData d;

	String headers;

	if(base == BaseURL::Repository)
	{
		auto token = getCredentialFile().loadFileAsString();

		if (token.isEmpty())
			return { 401, "no token provided" };

		headers = "Authorization: Bearer " + token;
	}

	auto stream = url.createInputStream(options.withStatusCode(&d.statusCode).withExtraHeaders(headers));

	if (stream != nullptr)
	{
		auto data = stream->readEntireStreamAsString();
		auto r = Result::ok();
		auto ok = JSON::parse(data, d.returnData);

		if (!ok.wasOk())
		{
			d.returnData = data;
		}
	}

	return d;
}

struct HiseAssetManager::ProductInfo::ThumbnailTask : public ActionBase
{
	ThumbnailTask(ProductInfo* info_, const String& url_) :
		url(url_),
		info(info_)
	{
	}

	String getMessage() const override { return "Fetch thumbnail..."; }

	bool perform(HiseAssetManager* manager) override
	{
		TemporaryFile tf;

		URL::DownloadTaskOptions options;
		auto task = url.downloadToFile(tf.getFile(), options);

		while (!task->isFinished())
		{
			manager->wait(100);
		}

		if (!task->hadError())
			downloadedImage = ImageFileFormat::loadFrom(tf.getFile());

		return true;
	}

	void onCompletion(HiseAssetManager* manager) override
	{
		manager->showStatusMessage("OK");
		if (downloadedImage.isValid())
		{
			info->image = downloadedImage;
		}
	}

	ProductInfo::Ptr info;
	URL url;
	Image downloadedImage;
};

HiseAssetManager::ProductInfo::ProductInfo(const var& obj, HiseAssetManager* manager) :
	prettyName(obj["product_name"].toString()),
	description(obj["product_short_description"].toString()),
	shopURL(obj["path"].toString())
{
	auto repoPath = URL(obj["repo_link"].toString()).getSubPath();
	auto tokens = StringArray::fromTokens(repoPath, "/", "");

	vendor = tokens[0].trim();
	repoId = tokens[1].trim();

	manager->addJob(new ThumbnailTask(this, obj["thumbnail"].toString()));
}

struct HiseAssetManager::ProductFetchTask : public ActionBase
{
	String getMessage() const override { return "Fetch products from store"; }

	bool perform(HiseAssetManager*) override
	{
		auto rd = Helpers::post(BaseURL::Store, "api/products/", true, {});

		if(rd.statusCode == 200)
			obj = rd.returnData;

		return true;
	}

	var obj;

	void onCompletion(HiseAssetManager* manager) override
	{
		manager->products.clear();

		if (auto ar = obj.getArray())
		{
			for (auto& v : *ar)
			{
				manager->products.add(new ProductInfo(v, manager));
			}
		}

		manager->initialiseLocalAssets();
		manager->tryToLogin();
	}
};



HiseAssetManager::ProductList::TagInfo::TagInfo(const var& obj) :
	name(obj["name"].toString()),
	isSemanticVersion(SemanticVersionChecker(name, name).oldVersionNumberIsValid()),
	date(Time::fromISO8601(obj["commit"]["created"].toString())),
	commitHash(obj["commit"]["sha"].toString()),
	downloadLink(obj["zipball_url"].toString())
{}

bool HiseAssetManager::ProductList::TagInfo::operator<(const TagInfo& other) const
{
	if (isSemanticVersion && other.isSemanticVersion)
	{
		return !SemanticVersionChecker(other.name, name).isUpdate();
	}

	return other.name.compare(name) < 0;
}

bool HiseAssetManager::ProductList::TagInfo::operator>(const TagInfo& other) const
{
	return !(*this < other);
}

HiseAssetManager::ProductList::RowInfo::RowInfo(const HiseAssetInstaller::UninstallInfo& info) :
	installInfo(info)
{

}

HiseAssetManager::ProductList::RowInfo::RowInfo(const var& obj) :
	installInfo(obj),
	repoPath(obj["url"].toString().fromFirstOccurrenceOf("https://git.hise.dev/", false, false))
{

}

HiseAssetManager::ProductList::RowInfo::RowInfo(ProductInfo* info)
{
	setFromProductInfo(info);
	state = State::NotOwned;
}

void HiseAssetManager::ProductList::RowInfo::setFromProductInfo(ProductInfo* info)
{
	if (info != nullptr)
	{
		installInfo.packageName = info->repoId;
		installInfo.vendor = info->vendor;
		description = info->description;
		prettyName = info->prettyName;
		shopURL = URL("https://store.hise.dev").getChildURL(info->shopURL);
	}
}

bool HiseAssetManager::ProductList::RowInfo::matches(const String& searchTerm) const
{
	return searchTerm.isEmpty() || installInfo.packageName.toLowerCase().contains(searchTerm.toLowerCase());
}

bool HiseAssetManager::ProductList::RowInfo::matches(int at) const
{
	auto installed = installInfo.isInstalled();

	auto installTypeMatches = true;

	if (state == State::NotOwned)
	{
		installTypeMatches &= (at & AssetType::Online) != 0;
	}
	else
	{
		if (installed)
			installTypeMatches &= (at & (AssetType::Installed | AssetType::Online)) != 0;
		else
			installTypeMatches &= (at & (AssetType::Uninstalled | AssetType::Online)) != 0;
	}

	auto isLocal = installInfo.isLocalFolder();

	auto sourceMatches = ((at & AssetType::AllSources) == AssetType::AllSources) ||
		(isLocal && ((at & AssetType::Local) != 0)) ||
		(!isLocal && ((at & AssetType::Webstore) != 0));

	return installTypeMatches && sourceMatches;
}

String HiseAssetManager::ProductList::RowInfo::getPackageName() const
{
	return installInfo.packageName;
}

String HiseAssetManager::ProductList::RowInfo::getNameToDisplay() const
{
	if (prettyName.isNotEmpty())
		return prettyName;

	return getPackageName();
}

void HiseAssetManager::ProductList::RowInfo::setFromTagData(const TagInfo& info)
{
	if (!info.isSemanticVersion)
		return;

	installInfo.availableVersion = info.name;

	downloadLink = info.downloadLink;

	updateState();
}

void HiseAssetManager::ProductList::RowInfo::updateState()
{
	if (state == State::NotOwned)
		return;

	if (needsCleanup)
	{
		state = State::NeedsCleanup;
		return;
	}

	if (!installInfo.isInstalled())
	{
		state = State::Uninstalled;
	}
	else if (installInfo.isUpdateAvailable())
	{
		state = State::UpdateAvailable;
	}
	else
	{
		state = State::UpToDate;
	}
}

String HiseAssetManager::ProductList::RowInfo::getActionName() const
{
	switch (state)
	{
	case State::Uninstalled:
		return "Install " + installInfo.availableVersion;
	case State::UpdateAvailable:
		return "Update to " + installInfo.availableVersion;
	case State::UpToDate:
		return "Uninstall " + installInfo.installedVersion;
	case State::NeedsCleanup:
		return "Cleanup";
	case State::NotOwned:
		return "View in HISE store";
	case State::numStates:
		break;
	default:
		break;
	}

	jassertfalse;
	return "Undefined";
}

std::unique_ptr<juce::Drawable> HiseAssetManager::ProductList::RowBase::createSVGfromBase64(const String& b64)
{
	MemoryBlock mb;
	mb.fromBase64Encoding(b64);
	zstd::ZDefaultCompressor comp;
	String text;
	comp.expand(mb, text);

	if (auto xml = XmlDocument::parse(text))
		return juce::Drawable::createFromSVG(*xml);

	return nullptr;
}

bool HiseAssetManager::ProductList::RowBase::paintBase(Graphics& g, Drawable* icon, bool fullOpacity)
{
	g.setColour(Colours::white.withAlpha(fullOpacity ? 0.08f : 0.05f));
	g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 3.0f);

	if (icon != nullptr)
	{
		icon->drawWithin(g, imgBounds.toFloat(), RectanglePlacement::centred, 1.0f);
		return true;
	}

	return false;
}

HiseAssetManager::ProductList::SpecialRow::SpecialRow()
{
	setMouseCursor(MouseCursor::PointingHandCursor);
	setRepaintsOnMouseActivity(true);
}

void HiseAssetManager::ProductList::SpecialRow::mouseUp(const MouseEvent& e)
{
	if (onClick)
		onClick();
}

bool HiseAssetManager::ProductList::SpecialRow::matchesFlag(int filterFlag, const String& searchTerm) const
{
	// Hide when viewing only installed packages
	if (((filterFlag & AssetType::Installed) != 0) &&
		((filterFlag & AssetType::Uninstalled) == 0))
		return false;

	return searchTerm.isEmpty();
}

void HiseAssetManager::ProductList::SpecialRow::setIcon(const String& b64)
{
	icon = createSVGfromBase64(b64);
}

void HiseAssetManager::ProductList::SpecialRow::setText(const String& text)
{
	helpText.append(text, GLOBAL_FONT(), Colours::white.withAlpha(0.6f));
	helpText.setJustification(Justification::centred);
}

void HiseAssetManager::ProductList::SpecialRow::paint(Graphics& g)
{
	paintBase(g, icon.get(), isMouseOver());
	helpText.draw(g, textBounds);
}

void HiseAssetManager::ProductList::SpecialRow::resized()
{
	auto b = getLocalBounds();
	b.removeFromLeft(10);
	b.removeFromLeft(3);
	b.removeFromRight(10);

	b.removeFromRight(5);
	imgBounds = b.removeFromLeft(b.getHeight()).reduced(16);
	b.removeFromLeft(10);
	textBounds = b.toFloat();
}


HiseAssetManager::ProductList::LoginRow::LoginRow(HiseAssetManager* manager_) :
	manager(manager_),
	loginButton("Login"),
	getTokenButton("Get Token"),
	blaf(new ButtonLaf())
{
	auto existingToken = Helpers::getCredentialFile().loadFileAsString();

	helpText.append("Paste your auth token to sync with your HISE Store purchases.", GLOBAL_FONT(), Colours::white.withAlpha(0.6f));
	helpText.setJustification(Justification::centredLeft);

	icon = createSVGfromBase64("1094.nT6K8C1lFTdH.XYOEaBvp791HdIOsrUiWPPbDBwThRP7x0G2EraxVB.vGLtIAAAoDD55A3K.pBPt.HNVhzjwHHoC+EeRjlnmkfghLnqib7a5y3GVxbQXHnW3IKQRQUOvuIy+42rrB6Khq7jgk26ujFACgfg0ZwXmiTUWcEF5nOv+HpJHJy6JrQncfoUcnc060xx5TD3nRNyHVelj0bZMMM0xhQIJ5nJpJKPrrt0KUDNZndr4gkUVtik0hJpLMJN1tP8Y5jBxkOaxX3g.GD.BITQNVg8GIC45F4POop5KTXyVE12HXGonwOUksdqvFIAOM+lnBEgHgCxtpu28091qb+sBnbWW6c7ktZ2a9Vm4wVLkNOWu5qFuqVKti80K+M650U78p48d05uTYeFmytx1Zu68Udtmu1Uu9xW2dNul455Muq69kR1xqd7VA91qW8akJrNpQnbkaqs5u2EJ5W70UMuP66Xb8yx5DUksB6+wIp0v.eyQLn9mkrKJPiupV6dgB8FW4V7BEWqhKAP.ZhxIVgIZR5cNxYhR8c.kAXFfgleTvHV4rTihFFJhThjpa3s8fLaAMVHcFwBIIhvjs4hDyzRoYk9V3i+A28nU3DFLYZLZb4+GYFNXfDB4pSmvIRCHX.BuhmNZN7jEJ6jgOeZjYt83aCxnMMVZP9EYhMMCCN3.pPe8O4BYv2XoECNTttJrNPJvSuemAEPPnOSFfnn2Tt74a9h40KkdMaqdd9h62459duZdkiu9KE1iwYZ+Niw4r+twW7aluaqX7deU4aNtt67dV2lq4qsUq85..fvmoSqCrpvzz9jsmjgANaZBY8xtnozl16Dz75jDSIF4cwh6x+cfiz3iymHYXDwlrD5F1HAdyVmRbe8HV13gNH87vD9jRsKMKHO1t34CHKFKHQSMQRiPyRgKjtjoKQj7Kld5gRsHp3oPkOSb1jgTlDR5P58v2IxQ5Hpr4VYzXlDS3z7HuHxCsCGPYbPmPijSmRw7kcUdcVwywTxxonMwG3TB5ZeRJdwiitExLAhenXluQiDTr.R6SHKM5.g7+.tu88YR4aFC.BIpgSJcJHp.PDAD.PC.PoHSUUUAAVJUlCxHADHDs1.7r.3KKAUnDcHpe4gAUCQBGnnQ50rBfPAs84NQMeSMHN4123YBLIeoWjIkX9LhA4fmhpwdixVIAoONuKIPmyqpzNksarXe.1elGF3LdQTG8xVCRX3sFC9MlbaYilRunhwLRLUPwyaj1AANMNxbiADFkqAKIgdQC.NjQBhnHTLTuJLtXC.6d1QUOXeEMblZgarphEBfv0PhzQs9utzuv0V7hGJGGdjhs3sHX5Wzc2Ty6FhV.NMUmFEUW2R4Zxaq+1bNXJXhOZy8O.AwcWIj8nSX.T.A0JWPk5ldVpFC9lZQ5hL2Dv2xP+bgJXFf4MLNvln6p7I8P2RZE7oqIlEg5EG67JfAjsC");

	addAndMakeVisible(tokenEditor);
	addAndMakeVisible(loginButton);
	addAndMakeVisible(getTokenButton);

	tokenEditor.setPasswordCharacter(0x2022);
	tokenEditor.setTextToShowWhenEmpty("Paste auth token here...", Colours::grey);
	tokenEditor.setText(existingToken, dontSendNotification);
	GlobalHiseLookAndFeel::setTextEditorColours(tokenEditor);
	tokenEditor.setJustification(Justification::centredLeft);

	tokenEditor.onReturnKey = [this]()
	{
		buttonClicked(&loginButton);
	};

	loginButton.setLookAndFeel(blaf);
	getTokenButton.setLookAndFeel(blaf);

	loginButton.setColour(TextButton::ColourIds::buttonColourId, Colour(ProductList::SignalColour));
	getTokenButton.setColour(TextButton::ColourIds::buttonColourId, Colour(0xFF666666));

	loginButton.setConnectedEdges(Button::ConnectedOnRight);
	getTokenButton.setConnectedEdges(Button::ConnectedOnLeft);

	loginButton.addListener(this);
	getTokenButton.addListener(this);
}

bool HiseAssetManager::ProductList::LoginRow::matchesFlag(int filterFlag, const String& searchTerm) const
{
	if (((filterFlag & AssetType::Installed) != 0) &&
		((filterFlag & AssetType::Uninstalled) == 0))
		return false;

	return searchTerm.isEmpty();
}

void HiseAssetManager::ProductList::LoginRow::buttonClicked(Button* b)
{
	if (b == &getTokenButton)
	{
		URL("https://store.hise.dev/account/settings/").launchInDefaultBrowser();
		tokenEditor.grabKeyboardFocus();
		return;
	}

	if (b == &loginButton)
	{
		auto token = tokenEditor.getText().trim();

		if (token.isEmpty())
		{
			manager->showStatusMessage("Please paste a token first.");
			return;
		}

		Helpers::getCredentialFile().replaceWithText(token);
		manager->setState(ErrorState::OK);
		manager->addJob(new LoginTask());
	}
}

void HiseAssetManager::ProductList::LoginRow::paint(Graphics& g)
{
	paintBase(g, icon.get(), false);
	helpText.draw(g, textBounds);
}

void HiseAssetManager::ProductList::LoginRow::resized()
{
	auto b = getLocalBounds();
	b.removeFromLeft(10);
	b.removeFromLeft(3);
	b.removeFromRight(10);
	b.removeFromRight(5);

	imgBounds = b.removeFromLeft(b.getHeight()).reduced(16);
	b.removeFromLeft(10);

	auto top = b.removeFromTop(b.getHeight() / 2);
	textBounds = top.toFloat();

	b.removeFromTop(4);
	b.removeFromBottom(16);

	auto buttonArea = b.removeFromRight(200);
	b.removeFromRight(8);

	tokenEditor.setBounds(b);

	getTokenButton.setBounds(buttonArea.removeFromRight(80));
	loginButton.setBounds(buttonArea);
}

struct HiseAssetManager::ProductList::Row::FetchVersion : public RowAction
{
	FetchVersion(Row* r) : RowAction(r, "Fetch version...") {};

	bool perform(HiseAssetManager*) override
	{
		rd = Helpers::post(BaseURL::Repository, repoPath + "/tags", true, {});
		return true;
	}

	void onCompletion(HiseAssetManager* manager) override
	{
		if (rowComponent.getComponent() != nullptr)
		{
			rd.forEachListElement([&](const var& obj)
				{
					rowComponent->tags.push_back({ obj });
				});

			std::sort(rowComponent->tags.begin(), rowComponent->tags.end());

			rowComponent->updateAfterTag(manager);
		}
	}
};

struct HiseAssetManager::ProductList::Row::FetchInfo : public RowAction
{
	FetchInfo(Row* r) : RowAction(r, "Fetch readme...") {};

	bool perform(HiseAssetManager*) override
	{
		rd = Helpers::post(BaseURL::Repository, repoPath + "/raw/Readme.md", true, {});
		return true;
	}

	void onCompletion(HiseAssetManager* manager) override
	{
		if (rowComponent.getComponent() != nullptr)
			manager->showHelpPopup(&rowComponent->infoButton, rd.returnData.toString());
	}

	String getMessage() const override { return "Fetch README.md"; }
};

struct HiseAssetManager::ProductList::Row::FetchChangeLog : public RowAction
{
	struct Commit
	{
		Commit(const var& obj) :
			date(Time::fromISO8601(obj["created"].toString())),
			message(obj["commit"]["message"].toString().trim()),
			sha(obj["sha"].toString())
		{}

		bool isInternal() const { return message.contains("[internal]"); }

		Time date;
		String message;
		String sha;
	};

	FetchChangeLog(Row* r) : RowAction(r, "Fetch changelog") {};

	bool perform(HiseAssetManager*) override
	{
		rd = Helpers::post(BaseURL::Repository, repoPath + "/commits", true, {});
		return true;
	}

	void onCompletion(HiseAssetManager* manager) override
	{
		if (rowComponent.getComponent() == nullptr)
			return;

		std::vector<Commit> commits;

		rd.forEachListElement([&](const var& obj)
			{
				commits.push_back({ obj });
			});

		String msg;

		msg << "### Changelog for " << installInfo.packageName << " " << installInfo.availableVersion;
		msg << "\n\n";

		for (const auto& c : commits)
		{
			msg << "- " << c.message << "\n";
		}

		manager->showHelpPopup(&rowComponent->infoButton, msg);
	}
};

struct HiseAssetManager::ProductList::Row::UninstallAction : public RowAction
{
	UninstallAction(Row* r, bool silent_) :
		RowAction(r, "Uninstall " + r->info.getPackageName() + " " + r->info.installInfo.installedVersion),
		silent(silent_)
	{};

	bool perform(HiseAssetManager* manager) override
	{
		try
		{
			HiseAssetInstaller installer(manager->getMainController(), installInfo);
			auto result = installer.uninstall();

			if (result.hasSkippedFiles())
				skippedFiles = result.skippedFiles;

			return result.success;
		}
		catch(HiseAssetInstaller::Error& e)
		{
			e.info = installInfo;
			manager->setError(e);
			return false;
		}
	}

	void onCompletion(HiseAssetManager* manager) override
	{
		if (skippedFiles.isEmpty())
		{
			if (!silent)
				manager->showStatusMessage("Deinstalled " + getInfo().packageName);
		}
		else
		{
			manager->showStatusMessage(String(skippedFiles.size()) + " modified file(s) kept. Click Cleanup when ready.");
		}

		manager->rebuildProductsAsync();
	}

	bool silent = false;
	StringArray skippedFiles;
};

struct HiseAssetManager::ProductList::Row::CleanupAction : public RowAction
{
	CleanupAction(Row* r) :
		RowAction(r, "Cleanup " + r->info.getPackageName())
	{};

	bool perform(HiseAssetManager* manager) override
	{
		try
		{
			HiseAssetInstaller installer(manager->getMainController(), installInfo);
			return installer.cleanup();
		}
		catch (HiseAssetInstaller::Error& e)
		{
			e.info = installInfo;
			manager->setError(e);
			return false;
		}
	}

	void onCompletion(HiseAssetManager* manager) override
	{
		manager->showStatusMessage("Cleaned up " + getInfo().packageName);
		manager->rebuildProductsAsync();
	}
};

struct HiseAssetManager::ProductList::Row::UpdateAction : public RowAction,
														  public URL::DownloadTaskListener
{
	enum class State
	{
		Idle,
		Downloading,
		Success,
		Fail
	};

	std::atomic<State> currentState { State::Idle };

	UpdateAction(Row* r, TagInfo ti) :
		RowAction(r, "Update " + r->info.getPackageName()),
		tf("hise_store", 0),
		tag(ti)
	{
		if (installInfo.isLocalFolder())
			tag = TagInfo();
	};

	bool perform(HiseAssetManager* manager) override
	{
		currentManager = manager;

		// Uninstall current version first if installed
		if (installInfo.isInstalled())
		{
			manager->showStatusMessage("Uninstalling " + installInfo.packageName + " " + installInfo.installedVersion + "...");

			try
			{
				HiseAssetInstaller installer(manager->getMainController(), installInfo);
				auto result = installer.uninstall();

				if (result.hasSkippedFiles())
				{
					skippedFiles = result.skippedFiles;
					return false;
				}

				if (!result.success)
					return false;
			}
			catch (HiseAssetInstaller::Error& e)
			{
				e.info = installInfo;
				manager->setError(e);
				return false;
			}
		}

		if (!installInfo.isLocalFolder())
		{
			auto link = tag.downloadLink;
			URL u(link);

			manager->showStatusMessage("Downloading...");

			currentTask = u.downloadToFile(tf.getFile(), Helpers::getDownloadOptions(this));

			currentState.store(State::Downloading);

			while (currentState.load() == State::Downloading)
			{
				Thread::getCurrentThread()->wait(500);
			}

			if (currentState == State::Success)
			{
				manager->showStatusMessage("Extracting...");

				ScopedPointer<ZipFile> zf = new ZipFile(tf.getFile());
				HiseAssetInstaller installer(currentManager->getMainController(), zf.get());
				return installer.install(installInfo);
			}

			// Download failed
			return false;
		}
		else
		{
			HiseAssetInstaller installer(currentManager->getMainController(), installInfo);
			return installer.install(installInfo);
		}
	}

	void finished(URL::DownloadTask* task, bool success) override
	{
		currentState = success ? State::Success : State::Fail;
	}

	void progress(URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override
	{
	}

	void onCompletion(HiseAssetManager* manager) override
	{
		if (skippedFiles.isEmpty())
		{
			manager->showStatusMessage("Installed " + installInfo.packageName);
		}
		else
		{
			manager->showStatusMessage(String(skippedFiles.size()) + " modified file(s) kept. Click Cleanup when ready.");
		}

		manager->rebuildProductsAsync();
	}

	TemporaryFile tf;
	TagInfo tag;
	StringArray skippedFiles;
	HiseAssetManager* currentManager = nullptr;
	std::unique_ptr<URL::DownloadTask> currentTask;
};


HiseAssetManager::ProductList::Row::Row(const RowInfo& r, HiseAssetManager* manager) :
	info(r),
	infoButton("info"),
	actionButton("action"),
	moreButton("more"),
    blaf(new ButtonLaf())
{
	downloadIcon = manager->createPath("download");

	infoButton.setConnectedEdges(Button::ConnectedOnRight);
	actionButton.setConnectedEdges(Button::ConnectedOnRight | Button::ConnectedOnLeft);
	moreButton.setConnectedEdges(Button::ConnectedOnLeft);

	infoButton.setColour(TextButton::ColourIds::buttonColourId, Colour(0xFF666666));
	moreButton.setColour(TextButton::ColourIds::buttonColourId, Colour(SignalColour));
	actionButton.setColour(TextButton::ColourIds::buttonColourId, Colour(0xFF666666));

	addAndMakeVisible(infoButton);
	addAndMakeVisible(actionButton);
	addAndMakeVisible(moreButton);

	infoButton.addListener(this);
	actionButton.addListener(this);
	moreButton.addListener(this);

	infoButton.setLookAndFeel(blaf);
	actionButton.setLookAndFeel(blaf);
	moreButton.setLookAndFeel(blaf);

	// Read install state eagerly so the button shows the correct text before
	// FetchVersion completes asynchronously
	{
		HiseAssetInstaller installer(manager->getMainController(), info.installInfo);
		auto logEntry = installer.getInstallInfoFromLog({});
		info.installInfo.installedVersion = logEntry[HiseSettings::Project::Version].toString();
		info.needsCleanup = (bool)logEntry["NeedsCleanup"];
		info.updateState();
		actionButton.setButtonText(info.getActionName());
	}

	manager->addJob(new FetchVersion(this));
}

void HiseAssetManager::ProductList::Row::buttonClicked(Button* b)
{
	auto manager = findParentComponentOfClass<HiseAssetManager>();

	if (b == &infoButton)
	{
		manager->addJob(new FetchChangeLog(this));
	}
	if(b == &actionButton)
	{
		if (info.state == RowInfo::State::NotOwned)
		{
			manager->performAction(SpecialAction::ShowInStore, this, true);
			return;
		}

		if (info.state == RowInfo::State::NeedsCleanup)
		{
			manager->performAction(SpecialAction::Cleanup, this, false);
			return;
		}

		if (info.state == RowInfo::State::UpdateAvailable)
		{
			manager->performAction(SpecialAction::Install, this, false);
			return;
		}

		if(info.installInfo.isInstalled())
		{ 
			manager->performAction(SpecialAction::Uninstall, this, false);
			return;
		}
		else
		{
			manager->performAction(SpecialAction::Install, this, false);
		}
	}

	if (b == &moreButton)
	{
		auto localFolder = info.installInfo.isLocalFolder();

		PopupLookAndFeel laf;
		PopupMenu m;
		m.setLookAndFeel(&laf);
		auto isCleanup = info.state == RowInfo::State::NeedsCleanup;
		m.addItem(1 + SpecialAction::Install, "Install latest version", info.installInfo.isUpdateAvailable() && !isCleanup, false);
		m.addItem(1 + SpecialAction::Uninstall, "Uninstall from project", info.installInfo.isInstalled() && !isCleanup, false);
		m.addItem(1 + SpecialAction::Cleanup, "Cleanup modified files", isCleanup, false);
		m.addItem(1 + SpecialAction::ShowInStore, localFolder ? "Show folder" : "Show in store", true, false);
		m.addItem(1 + SpecialAction::RemoveLocalFolder, "Remove local asset folder", localFolder, false);

		PopupMenu subMenu;
		int idx = 0;
		for(const auto& t: tags)
		{
			subMenu.addItem(1000 + idx++, t.name, !isCleanup && t.name != info.installInfo.installedVersion, false);
		}

		m.addSubMenu("Revert to version", subMenu);
		
		auto result = m.show();

		if(result == 0)
			return;

		if (result >= 1000)
		{
			auto idx = result - 1000;

			if (!isPositiveAndBelow(idx, (int)tags.size()))
				return;

			String msg;
			msg << "Do you want to install " << info.getPackageName() << " " << tags[idx].name;

			if (!PresetHandler::showYesNoWindow("Confirm", msg))
				return;

			manager->addJob(new UpdateAction(this, tags[idx]));
		}
		else
		{
			manager->performAction((SpecialAction)(result-1), this, true);
		}
	}
}

bool HiseAssetManager::ProductList::Row::matchesFlag(int filterFlag, const String& searchTerm) const
{
	return info.matches(filterFlag) && info.matches(searchTerm);
}

void HiseAssetManager::ProductList::Row::updateAfterTag(HiseAssetManager* m)
{
	HiseAssetInstaller installer(m->getMainController(), info.installInfo);

	auto logEntry = installer.getInstallInfoFromLog({});
	info.installInfo.installedVersion = logEntry[HiseSettings::Project::Version].toString();
	info.needsCleanup = (bool)logEntry["NeedsCleanup"];

	if (!tags.empty())
		info.setFromTagData(tags.back());
	else
		info.updateState();

	title.clear();

	title.append(info.getNameToDisplay(), GLOBAL_BOLD_FONT().withHeight(18.0f), Colours::white.withAlpha(0.9f));
	title.append(" by " + info.installInfo.vendor, GLOBAL_BOLD_FONT().withHeight(18.0f), Colours::white.withAlpha(0.6f));

	title.setJustification(Justification::centredLeft);
	description.setJustification(Justification::centredLeft);

	if (info.installInfo.isLocalFolder())
	{
		description.clear();
		description.append(info.installInfo.localFolder.getFullPathName(), GLOBAL_FONT(), Colours::white.withAlpha(0.6f));

		if (localIcon == nullptr)
		{
			auto local_asset = "767.nT6K8ClmEz5E.XLZ5YB7Ppd.6by7JTpv8E0ZuMVdX4.6zQi.FEgPRZCY48ltnlHahrwyGzG.4A.Y.fYp5xFEppIJtm.wX+dnrIUgARjmzlD2MDB0lTQwhjrEfTYXrigwfACD9vx1Fkm64PBPXysaAJhvgfDH.gPdEPnyyAwPzAS.gi5hlKAHrPBnGPH.HQL3gc8dltvrjHcIYSy8rNJHwDuGpqJIrIWStWj62yEVkV1zEmUkLF.IynUJ4HAUYydb5zbuw3yTmlN1zCjoJaSRk2SkLrLztjjs4nfD4Uf7QkU1zVUkKNvxdbUgowjyzFXYaZVprlTfjD10zkoGSVbj4XYUkgUGvJXS3LpltjMoMkY48LMQhEsGv681abUYk4QLFkOZWtW+ruymtnK9dxfcPzSnYjWeVZc1VqUIKud2P+lsVmBBIB7U.A1u48d.Lnxdbga+lox24exsSi7ERR6npk8V5rK4dNaW0cdBMkejrFYI046O6FdORNJaRrzrz6twsbs9cKqt6wJ2SayzX2bemsSoiM..QntnEYo0Xs9rG+4LR42k26yzHqekV1oSd9y57o9yuWkbUd+tgKrS1kdG8cb1bYoa+leZa+H8YYikduMOcZnxXK46WYpsajtDJpo8sxWp69YEJ5iteNaHYka5jZiNWeJKcAXfnFPoigjFZ..B..HnA.kB.kYz.HrnVFEoDTvXDRjB5+mAeHz8ju.dpFczLDT394eBOnGOh02an2XlmV5jUi7BREE3Ra.CM+SH0r7MyPpuvgfypwm8fNWIpTN8wQMJLi5+os52UAEPaBLPjl1aZeS4NqfF7zAQR4T8FFsl3fsxLuT8zRPBkFxxIf.hKF3AR.ajGTOuGN+HZIzThUyRhvKYC9ouEs52Xlu0RbLS8v0.NSRi5FM0CufvtVTVnGHf0rB2Ljpyt6y6eg6h.RNJATCbFrUVEJ765aUVmRzSG1jig+yOruLCkAlLCZiaKBVvw.Wyvb0w3G2VfLbRa7AENG6PmXnMvAX.DzA";
			localIcon = createSVGfromBase64(local_asset);
		}
	}
	else
	{
		description.clear();
		description.append(info.description, GLOBAL_BOLD_FONT(), Colours::white.withAlpha(0.6f));
		localIcon = nullptr;
	}

	actionButton.setButtonText(info.getActionName());
	repaint();
	initialised = true;
}

void HiseAssetManager::ProductList::Row::paint(Graphics& g)
{
	auto iconPainted = paintBase(g, localIcon.get(), false);

	if (!initialised)
	{
		g.setColour(Colours::white.withAlpha(0.1f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Loading data...", getLocalBounds().toFloat(), Justification::centred);
		return;
	}

	if (iconPainted)
	{
		// done...
	}
	else if (productInfo != nullptr && productInfo->image.isValid())
	{
		g.setColour(Colours::white);
		g.drawImageWithin(productInfo->image, imgBounds.getX(), imgBounds.getY(), imgBounds.getWidth(), imgBounds.getHeight(), RectanglePlacement::centred);
	}
	else
	{
		g.setColour(Colours::white);
		g.drawRoundedRectangle(imgBounds.toFloat(), 3.0f, 2.0f);
	}

	if (info.state == RowInfo::State::NeedsCleanup)
	{
		g.setColour(Colour(HISE_WARNING_COLOUR));
		g.fillEllipse(circleArea.toFloat().translated(10.0f, 10.0f));
	}
	else if (info.installInfo.isInstalled() && info.installInfo.isUpdateAvailable())
	{
		g.setColour(Colour(SignalColour));
		g.fillEllipse(circleArea.toFloat().translated(10.0f, 10.0f));
	}

	if (!info.installInfo.isInstalled() && info.state != RowInfo::State::NotOwned)
	{
		auto copy = titleBounds;

		PathFactory::scalePath(downloadIcon, copy.removeFromLeft(16.0f));
		copy.removeFromLeft(3.0f);
		g.setColour(Colours::white.withAlpha(0.7f));
		g.fillPath(downloadIcon);
		title.draw(g, copy);
	}
	else
	{
		title.draw(g, titleBounds);
	}

	description.draw(g, descriptionBounds);
}

void HiseAssetManager::ProductList::Row::resized()
{
	auto b = getLocalBounds();
	circleArea = b.removeFromLeft(10).removeFromTop(10).toFloat();
	b.removeFromLeft(3);

	b.removeFromRight(10);
	auto bb = b.removeFromRight(180).withSizeKeepingCentre(180, 20);

	infoButton.setBounds(bb.removeFromLeft(20));
	moreButton.setBounds(bb.removeFromRight(20));
	actionButton.setBounds(bb);

	b.removeFromRight(5);
	imgBounds = b.removeFromLeft(b.getHeight()).reduced(16);
	b.removeFromLeft(5);

	b = b.reduced(0, 5 + 8);

	titleBounds = b.removeFromTop(b.getHeight() / 2).toFloat();
	descriptionBounds = b.toFloat();
}

hise::HiseAssetManager::ProductList::Row* HiseAssetManager::ProductList::Content::addRow(const RowInfo& r, HiseAssetManager* manager)
{
	auto nr = new Row(r, manager);
	rows.add(nr);
	addAndMakeVisible(rows.getLast());
	return nr;
}



void HiseAssetManager::ProductList::Content::updateFilter()
{
	int h = 0;

	anyVisible = false;

	for (auto rb : rows)
	{
		auto match = rb->matchesFlag(currentAssetFilter, searchTerm.toLowerCase().trim());
		rb->setVisible(match);

		anyVisible |= match;

		if (match)
			h += RowHeight;
	}

	setSize(getWidth(), h);
	resized();
}

void HiseAssetManager::ProductList::Content::rebuild()
{
	updateFilter();
	resized();
}

void HiseAssetManager::ProductList::Content::resized()
{
	if (getLocalBounds().isEmpty())
		return;

	auto b = getLocalBounds();

	for (auto r : rows)
	{
		if (r->isVisible())
			r->setBounds(b.removeFromTop(RowHeight));
	}
}

HiseAssetManager::HiseAssetManager(BackendRootWindow* brw) :
	ControlledObject(brw->getMainController()),
	Thread("HISE Asset Manager operations"),
	resizer(this, &restrainer),
	closeButton("close", nullptr, *this)
{
	getMainController()->getSampleManager().getProjectHandler().addListener(this);

	productList = new ProductList();

	addAndMakeVisible(topBar = new TopBar(productList.get()));
	addAndMakeVisible(productList);

	addJob(new ProductFetchTask());
	projectChanged(getMainController()->getSampleManager().getProjectHandler().getRootFolder());


	addAndMakeVisible(resizer);

	setSize(900, 700);

	restrainer.setSizeLimits(600, 600, 1500, 1500);
	restrainer.setMinimumOnscreenAmounts(0xffffff, 0xffffff, 0xffffff, 0xffffff);

	addAndMakeVisible(closeButton);

	closeButton.onClick = [this]()
	{
		this->stopThread(1000);
		this->destroy();
	};

	logoutLaf = new LogoutButtonLaf();
	addChildComponent(logoutButton);
	logoutButton.setLookAndFeel(logoutLaf);
	logoutButton.setMouseCursor(MouseCursor::PointingHandCursor);
	logoutButton.setTooltip("Click to log out");

	logoutButton.onClick = [this]()
	{
		if (PresetHandler::showYesNoWindow("Confirm Logout", "Do you want to log out from the HISE Store?"))
		{
			Helpers::getCredentialFile().deleteFile();
			loginOk = false;
			userName = "Not logged in";
			logoutButton.setVisible(false);
			rebuildProductsAsync();
			repaint();
		}
	};
}

HiseAssetManager::~HiseAssetManager()
{
	getMainController()->getSampleManager().getProjectHandler().removeListener(this);
}

void HiseAssetManager::projectChanged(const File& newRootDirectory)
{
	currentProjectRoot = newRootDirectory;
	projectName = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name);
	initialiseLocalAssets();
	setProjectFolder(newRootDirectory);
}

void HiseAssetManager::initialiseLocalAssets()
{
	productList->setProducts({}, this);
}

void HiseAssetManager::rebuildProductsAsync()
{
	MessageManager::callAsync([this]()
	{
		productList->setProducts({}, this);
	});
}

void HiseAssetManager::setProjectFolder(const File& f)
{
	projectInfo.clear();
	projectInfo.append("Current HISE project: ", GLOBAL_FONT(), Colours::grey);
	projectInfo.append(f.getFullPathName(), GLOBAL_BOLD_FONT(), Colours::white);
	projectInfo.setJustification(Justification::centredLeft);
	repaint();
}

juce::Path HiseAssetManager::createPath(const String& url) const
{
	Path p;
	LOAD_EPATH_IF_URL("close", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);

	if (url == "download")
	{
		static const unsigned char downloadIcon[] = { 110,109,0,128,38,68,0,0,64,67,98,189,155,55,68,0,0,64,67,0,128,69,68,240,142,119,67,0,128,69,68,0,0,158,67,98,0,128,69,68,112,55,192,67,189,155,55,68,248,255,219,67,0,128,38,68,248,255,219,67,98,191,99,21,68,248,255,219,67,0,128,7,68,112,55,192,67,0,
128,7,68,0,0,158,67,98,0,128,7,68,240,142,119,67,191,99,21,68,0,0,64,67,0,128,38,68,0,0,64,67,99,109,0,128,38,68,160,244,87,67,98,20,178,24,68,160,244,87,67,43,125,13,68,32,100,130,67,43,125,13,68,0,0,158,67,98,43,125,13,68,200,154,185,67,20,178,24,68,
152,4,208,67,0,128,38,68,152,4,208,67,98,103,77,52,68,152,4,208,67,80,130,63,68,200,154,185,67,80,130,63,68,0,0,158,67,98,80,130,63,68,32,100,130,67,103,77,52,68,160,244,87,67,0,128,38,68,160,244,87,67,99,109,127,190,43,68,216,148,160,67,108,35,213,53,
68,216,148,160,67,108,0,128,38,68,112,55,192,67,108,88,42,23,68,216,148,160,67,108,252,64,33,68,216,148,160,67,108,252,64,33,68,16,145,119,67,108,127,190,43,68,16,145,119,67,108,127,190,43,68,216,148,160,67,99,101,0,0 };

		p.loadPathFromData(downloadIcon, sizeof(downloadIcon));
	}

	return p;
}

void HiseAssetManager::tryToLogin()
{
	auto token = Helpers::getCredentialFile().loadFileAsString();

	if (token.isNotEmpty())
		addJob(new LoginTask());
}



void HiseAssetManager::addNewAssetFolder()
{
	FileChooser fc("Add local HISE project as asset", File(), "package_install.json");

	if (fc.browseForFileToOpen())
	{
		auto f = fc.getResult().getParentDirectory();

		if (auto info = HiseAssetInstaller::UninstallInfo(f))
		{
			auto exists = std::any_of(localAssets->assets.begin(), localAssets->assets.end(),
				[&](const HiseAssetInstaller::UninstallInfo& a) { return a.localFolder == f; });

			if (exists)
			{
				showStatusMessage("This folder is already registered.");
				return;
			}

			localAssets->assets.push_back(info);
			productList->setProducts({}, this);
			localAssets->resave();
		}
	}
}

void HiseAssetManager::performAction(SpecialAction a, ProductList::Row* r, bool silent/*=true*/)
{
	String msg;

	if (a == SpecialAction::RemoveLocalFolder)
	{
		silent = false;
		msg << "Remove the folder\n> `" + r->info.installInfo.localFolder.getFullPathName() << "`";
	}
	else if (a == SpecialAction::Cleanup)
	{
		msg << "Delete remaining modified files from the project?\n\n";
		msg << "These files were kept during uninstall because they had local modifications:\n\n";

		HiseAssetInstaller installer(getMainController(), r->info.installInfo);
		auto logEntry = installer.getInstallInfoFromLog({});

		if (auto skippedArray = logEntry["SkippedFiles"].getArray())
		{
			auto rootFolder = getMainController()->getCurrentFileHandler().getRootFolder();

			for (const auto& sf : *skippedArray)
			{
				auto relative = File(sf.toString()).getRelativePathFrom(rootFolder);
				msg << "- `" << relative.replaceCharacter('\\', '/') << "`\n";
			}
		}
	}
	else
	{
		msg << "Do you want to " + r->info.getActionName();
	}

	if (!silent && !PresetHandler::showYesNoWindow("Confirm", msg))
		return;

	switch (a)
	{
	case ShowInStore:

		if (r->info.installInfo.isLocalFolder())
		{
			r->info.installInfo.localFolder.getChildFile("package_install.json").revealToUser();
		}
		else
		{
			r->info.shopURL.launchInDefaultBrowser();
		}

		break;
	case RemoveLocalFolder:
	{
		auto folderToRemove = r->info.installInfo.localFolder;

		for (const auto& a : localAssets->assets)
		{
			if (a.localFolder == folderToRemove)
			{
				auto& v = localAssets->assets;

				v.erase(std::remove(v.begin(), v.end(), a),
					v.end());

				localAssets->resave();
				break;
			}
		}

		rebuildProductsAsync();
		break;
	}
	case Install:
	{
		if (r->info.installInfo.isLocalFolder())
			addJob(new ProductList::Row::UpdateAction(r, {}));
		else if (!r->tags.empty())
			addJob(new ProductList::Row::UpdateAction(r, r->tags.back()));

		break;
	}
	case Uninstall:
		addJob(new ProductList::Row::UninstallAction(r, false));
		break;
	case Cleanup:
		addJob(new ProductList::Row::CleanupAction(r));
		break;
	default:
		break;
	}
}


void HiseAssetManager::addJob(ActionBase::Ptr newJob)
{
	{
		ScopedLock sl(lock);
		pendingTasks.add(newJob);
	}

	startTimer(100);
}

void HiseAssetManager::run()
{
	ScopedValueSetter<bool> svs(running, true);

	setState(ErrorState::Pending);

	ActionBase::List thisTime;

	{
		ScopedLock sl(lock);
		thisTime.swapWith(pendingTasks);
	}

	for (auto t : thisTime)
	{
		auto msg = t->getMessage();

		if (msg.isNotEmpty())
			showStatusMessage(msg);

		t->succeeded = t->perform(this);
		completedTasks.add(t);
	}

	setState(ErrorState::OK);
	triggerAsyncUpdate();
}

void HiseAssetManager::handleAsyncUpdate()
{
	for (auto t : completedTasks)
		t->onCompletion(this);

	completedTasks.clear();

	// Show error dialog if an error was reported during the run
	if (currentError.what != HiseAssetInstaller::Error::OK)
	{
		String errorMsg;
		errorMsg << "There was an issue with the Asset Manager.\n\n";

		switch (currentError.what)
		{
		case HiseAssetInstaller::Error::HashMismatch:
			errorMsg << "> Hash mismatch for `" << currentError.file.getFileName() << "`\n\n";
			break;
		case HiseAssetInstaller::Error::FileWriteError:
			errorMsg << "> Could not write file `" << currentError.file.getFileName() << "`\n\n";
			break;
		default: break;
		}

		errorMsg << "Please check the console for details.";

		PresetHandler::showMessageWindow("Asset Manager Error", errorMsg, PresetHandler::IconType::Error);

		currentError = {};
		setState(ErrorState::OK);
	}

	if (!pendingTasks.isEmpty() && !running)
		this->startThread();
}

void HiseAssetManager::setState(ErrorState newState)
{
	state = newState;
	SafeAsyncCall::repaint(this);
}

void HiseAssetManager::showStatusMessage(const String& newMessage)
{
	currentMessage = newMessage;
	SafeAsyncCall::repaint(this);
}

void HiseAssetManager::showHelpPopup(Component* attachedComponent, const String& markdownText)
{
	auto md = new SimpleMarkdownDisplay("");

	md->margin = 20;
	md->backgroundColour = Colour(0xFF333333);
	md->setOpaque(true);
	md->setSize(700, 500);
	md->setText(markdownText);

	std::unique_ptr<Component> n;
	n.reset(md);

	auto brw = GET_BACKEND_ROOT_WINDOW(attachedComponent);

	auto localArea = brw->getLocalArea(attachedComponent, attachedComponent->getLocalBounds());
	juce::CallOutBox::launchAsynchronously(std::move(n), localArea, brw);
}

void HiseAssetManager::mouseDown(const MouseEvent& e)
{
	dragger.startDraggingComponent(this, e);
}

void HiseAssetManager::mouseDrag(const MouseEvent& e)
{
	dragger.dragComponent(this, e, nullptr);
}

void HiseAssetManager::timerCallback()
{
	if (!running)
		this->startThread();

	stopTimer();
}

void HiseAssetManager::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
	auto b = getLocalBounds();
	auto tb = b.removeFromTop(24);
	GlobalHiseLookAndFeel::drawFake3D(g, tb);
	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white);
	g.drawText("HISE Asset Manager", tb.toFloat(), Justification::centred);

	g.setColour(Colours::white.withAlpha(0.05f));
	g.fillRect(getLocalBounds().removeFromBottom(20));

	if (state != ErrorState::OK && currentMessage.isNotEmpty())
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.5f));
		g.drawText(currentMessage, projectFolderBounds.reduced(5.0f, 0.0f), Justification::left);
	}
	else
	{
		projectInfo.draw(g, projectFolderBounds.reduced(5.0f, 0.0f));
	}

	g.setColour(colours[(int)state]);
	g.drawEllipse(stateIcon, 2.0f);
	g.fillEllipse(stateIcon.reduced(3.0f));

	g.setColour(Colour(0xFF444444));
	g.drawRect(getLocalBounds(), 1);

	if (!loginOk)
	{
		g.setFont(GLOBAL_FONT());
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawText(userName, logoutButton.getBounds().toFloat(), Justification::centredRight);
	}
}

void HiseAssetManager::resized()
{
	auto b = getLocalBounds();
	auto header = b.removeFromTop(24);

	closeButton.setBounds(header.removeFromRight(header.getHeight()).reduced(3));


	b = b.reduced(90, 25);

	topBar->setBounds(b.removeFromTop(32));

	b.setWidth(b.getWidth() + productList->vp.getScrollBarThickness());

	b.removeFromTop(20);

	b.removeFromBottom(40);

	productList->setBounds(b);
	resizer.setBounds(getLocalBounds().removeFromBottom(15).removeFromRight(15));

	projectFolderBounds = getLocalBounds().removeFromBottom(20).toFloat();

	stateIcon = projectFolderBounds.removeFromLeft(projectFolderBounds.getHeight()).reduced(4.0f);

	// Keep text clear of the resize handle
	projectFolderBounds.removeFromRight(15.0f);

	auto logoutArea = projectFolderBounds.removeFromRight(GLOBAL_FONT().getStringWidthFloat(userName) + 40.0f);
	logoutButton.setBounds(logoutArea.toNearestInt());

	if (getParentComponent() != nullptr)
	{
		centreWithSize(getWidth(), getHeight());
	}
}

HiseAssetManager::ProductList::ProductList()
{
	addAndMakeVisible(vp);
	vp.setViewedComponent(content = new Content(), false);
	vp.setScrollBarThickness(12);
	vp.setScrollOnDragEnabled(true);

	setSize(600, 300);

	sf.addScrollBarToAnimate(vp.getVerticalScrollBar());
}

void HiseAssetManager::ProductList::setProducts(const Array<var>& data, HiseAssetManager* manager)
{
	if (!data.isEmpty())
	{
		storeData.clear();
		storeData.addArray(data);
	}

	content->rows.clear();

	StringArray ownedProducts;

	for (auto& pd : storeData)
	{
		ProductInfo::Ptr info;

		RowInfo ri(pd);

		for (auto pi : manager->products)
		{
			if (pi->repoId == ri.getPackageName())
			{
				info = pi;
				break;
			}
		}

		ri.setFromProductInfo(info.get());
		auto nr = content->addRow(ri, manager);
		nr->productInfo = info;
		ownedProducts.add(ri.getPackageName());
	}

	for (auto pi : manager->products)
	{
		if (ownedProducts.contains(pi->repoId))
			continue;

		auto nr = content->addRow(RowInfo(pi), manager);
		nr->productInfo = pi;
	}

	for (const auto& la : manager->localAssets.get())
	{
		content->addRow(RowInfo(la), manager);
	}

	if (!manager->loginOk)
	{
		auto loginRow = new LoginRow(manager);
		content->rows.add(loginRow);
		content->addAndMakeVisible(loginRow);
	}
	{
		auto localManager = new SpecialRow();

		localManager->onClick = std::bind(&HiseAssetManager::addNewAssetFolder, manager);

		localManager->setText("Add local folders to this list to create assets");
		localManager->setIcon("1130.nT6K8ClaGTvH.XkOHaBvL79+jCCjrG.Kuvje38C2AlNry6f5RjhBmPfhZ2h2lhhhkEEPN3K.wB.u.j9GICUd1f7WBgmgOhxyTTyu3GC99JI8h7M7hgNYHJ9AEKQMV9wB98hN8n9SyJl+imIZCpduiif7KRpw8dqvPImpXtthkBJ76AMUIYl2ULPoe+Q0uzOKFSQQg94Cxz6DBU3YoMOxNZzHJpFloofpS0JaTTXKdBhlLSOZ3AEkVdCEEi5jVDNGcWlBEgNscoSCZVdHvAAfPB0RxJlmrTz6izKEcpxYnPCcUrPx+W4GR+T8ZwULxxO5oWzj3HoDNHftpaMguxZ8L2xs3v498xdcks5L5WKvWWarshwcqthUZpdqX9PuwHNLPECzeBpwjGMzB7MjhpZPAb8hLFuPmwD7P3YCOYtFaspzYqdawYKMs0pxBycwsYbU1lqr38FOcky1s1thauzbUWWakE110aLkJ12DnTOSqc2ZsyK34Ka0cag0Ws8x2ZNOauEjwW1cWyXQuRes8NiE36BLAj9modiUrwyRPTRuyzovClLnpwW7zGvd64YgN4SnXm8voPCjHVn5jgxaotvOMLhFxTBWGHk5UJYHaBPaf3ONwjxDLbtcchxJ.4PZ.SGGVLT5k8gkXzvvl3MqGMQCjLRV1tnvYSDGbzdPC0RmNYZmcOB61.0koRVYHjI2QXr3Pyeg1xi58fAgjlTM7R89pXe+oOJn+aQNyowgsUScMWyRqs36Vqw06WSXsV0ta6bt90h43Ksrtxd4b580kles7qo8kaqz52JalywsWr1tusXt99lsB..BghPoNfphgEFFJIhyscGFINxlzITJIIhHoZg7HKtK6Pnj3.04N4nmHwPKaYg7H5IijzE3HFrDUbfFZ6w+gBQt7QyTgFMKK+rR+I8MZW5TDmXaiWEE7S1bh.gpozEEYZlXyasmP5.85UBTwCOPnPmU5FnBISFkfdeXBuaBNVdz4LBPtBViWY6bFLcLjk7uLB4wWBGQhjACELY5HghGJyacOHmOMuOsPOabLofPhmkrRQxn0qP1MAW1lGTRKK+YifjMhCoPon5..lIpgjxXHFwl..B...M.PgHhUF4AwCRAFmDLiLAjFRDDjT..xC.AapyRVeTYn+JEXTwR5F.MMcUwmBoInyEzLEfaBPLLEb5ijiSjdPSkFTCUpuBgmNHzyvfR41CJ0vJrGF8QLUebRV+5lYh8FFkHLYE6vWUgNqc.iRaXvju+O48+9RxNb9BFQttvOPEz8f+WHf2OvFBsICwv2ADPTjurO6JMuCu+sxHG7LjXnHRnPXivqJHTXvhlgW1r522OJbBRUO3DFXscxxSqD+JiFTeUg7rxFfJEH9X3uwvy7YWiEmjTlY1vzCESCJG07TwZFFcg.Q.fPQiogJwP2g9VkcWzoDEoNzDxvhSALwFhlSPo46omkxwgA3s.u5DQO3sLXoKsCIAhE9lT3VKzJiIer2fWC+cd1rMHlwGvxBXtB5.");
		content->rows.add(localManager);
		content->addAndMakeVisible(localManager);
	}

	content->rebuild();
}

void HiseAssetManager::ProductList::paint(Graphics& g)
{
	if (!content->anyVisible)
	{
		g.setColour(Colours::white.withAlpha(0.4f));
		g.setFont(GLOBAL_FONT());
		g.drawText("No matches :(", getLocalBounds().toFloat(), Justification::centred);
	}
}

void HiseAssetManager::ProductList::resized()
{
	vp.setBounds(getLocalBounds());
	content->setSize(vp.getWidth() - vp.getScrollBarThickness(), content->getHeight());
}

HiseAssetManager::TopBar::TopBar(ProductList* list_) :
	list(list_),
	allButton("My Assets"),
	installedButton("Installed"),
	uninstalledButton("Uninstalled"),
	onlineButton("Browse Store"),
	blaf(new ButtonLaf())
{
	/* TODO:

	- implement project root folder display OK
	- add product info from fetch server OK
	- add sorting & filtering
	- fix text layout
	- fix circle when update available OK
	- set all as default OK
	- add show package file & show install log
	- add show local folder & switch project context option
	- add open in store as context option
	- implement action button functionality with confirmation
	- fix product info not being detected correctly.

	*/
	addAndMakeVisible(allButton);
	addAndMakeVisible(installedButton);
	addAndMakeVisible(uninstalledButton);
	addAndMakeVisible(onlineButton);
	allButton.setLookAndFeel(blaf);
	installedButton.setLookAndFeel(blaf);
	uninstalledButton.setLookAndFeel(blaf);
	onlineButton.setLookAndFeel(blaf);

	allButton.addListener(this);
	installedButton.addListener(this);
	uninstalledButton.addListener(this);
	onlineButton.addListener(this);

	allButton.setClickingTogglesState(true);
	installedButton.setClickingTogglesState(true);
	uninstalledButton.setClickingTogglesState(true);
	onlineButton.setClickingTogglesState(true);

	allButton.setRadioGroupId(12251);
	installedButton.setRadioGroupId(12251);
	uninstalledButton.setRadioGroupId(12251);
	onlineButton.setRadioGroupId(12251);

	allButton.setToggleState(true, dontSendNotification);

	allButton.setColour(TextButton::ColourIds::buttonColourId, Colour(0xFF555555));
	allButton.setColour(TextButton::ColourIds::buttonOnColourId, Colour(ProductList::SignalColour));
	installedButton.setColour(TextButton::ColourIds::buttonColourId, Colour(0xFF555555));
	installedButton.setColour(TextButton::ColourIds::buttonOnColourId, Colour(ProductList::SignalColour));
	uninstalledButton.setColour(TextButton::ColourIds::buttonColourId, Colour(0xFF555555));
	uninstalledButton.setColour(TextButton::ColourIds::buttonOnColourId, Colour(ProductList::SignalColour));
	onlineButton.setColour(TextButton::ColourIds::buttonColourId, Colour(0xFF555555));
	onlineButton.setColour(TextButton::ColourIds::buttonOnColourId, Colour(ProductList::SignalColour));

	allButton.setConnectedEdges(Button::ConnectedOnRight);
	installedButton.setConnectedEdges(Button::ConnectedOnRight | Button::ConnectedOnLeft);
	uninstalledButton.setConnectedEdges(Button::ConnectedOnLeft);

	searchIcon.loadPathFromData(EditorIcons::searchIcon, SIZE_OF_PATH(EditorIcons::searchIcon));
	searchIcon.applyTransform(AffineTransform::rotation(float_Pi));
	addAndMakeVisible(searchBar);
	GlobalHiseLookAndFeel::setTextEditorColours(searchBar);
	searchBar.setTextToShowWhenEmpty("Search assets...", Colours::grey);
	searchBar.setJustification(Justification::centredLeft);
	setSize(600, 36);

	searchBar.onTextChange = [this]()
	{
		updateFilters();
	};
}

void HiseAssetManager::TopBar::updateFilters()
{
	list->content->currentAssetFilter = getAssetType();
	list->content->searchTerm = searchBar.getText().trim().toLowerCase();
	list->content->updateFilter();
	list->repaint();
}

int HiseAssetManager::TopBar::getAssetType() const
{
	int flags = currentDisplayType;
	flags |= AssetType::AllSources;

	return flags;
}

void HiseAssetManager::TopBar::buttonClicked(Button* b)
{
	if (b == &allButton)
		currentDisplayType = AssetType::All;
	if (b == &installedButton)
		currentDisplayType = AssetType::Installed;
	if (b == &uninstalledButton)
		currentDisplayType = AssetType::Uninstalled;
	if (b == &onlineButton)
		currentDisplayType = AssetType::Online;

	updateFilters();
}

void HiseAssetManager::TopBar::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.4f));
	g.fillPath(searchIcon);
}

void HiseAssetManager::TopBar::resized()
{
	auto b = getLocalBounds();
	//projectFolderBounds = b.removeFromTop(32).toFloat().reduced(2.0f);



	onlineButton.setBounds(b.removeFromRight(90).removeFromTop(24));
	b.removeFromRight(10);
	uninstalledButton.setBounds(b.removeFromRight(70).removeFromTop(24));
	installedButton.setBounds(b.removeFromRight(70).removeFromTop(24));
	allButton.setBounds(b.removeFromRight(70).removeFromTop(24));

	auto searchIconArea = b.removeFromLeft(b.getHeight()).toFloat().reduced(3.0f);

	b = b.removeFromLeft(jmin(b.getWidth(), 350));

	searchBar.setBounds(b);



	PathFactory::scalePath(searchIcon, searchIconArea);
}



}