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

namespace hise {
using namespace juce;

#if HISE_INCLUDE_RLOTTIE
class RLottieFloatingTile : public FloatingTileContent,
							public Component
{
public:

	SET_PANEL_NAME("RLottieDevPanel");

	RLottieFloatingTile(FloatingTile* parent) :
		FloatingTileContent(parent),
		devComponent(parent->getMainController()->getRLottieManager())
	{
		addAndMakeVisible(devComponent);
	}

	void resized() override
	{
		devComponent.setBounds(getLocalBounds());
	}

	RLottieDevComponent devComponent;
};
#endif


class DocUpdater : public DialogWindowWithBackgroundThread,
				   public MarkdownContentProcessor,
				   public DatabaseCrawler::Logger,
				   public URL::DownloadTask::Listener,
				   public ComboBox::Listener
{
public:

	enum CacheURLType
	{
		Hash,
		Content,
		Images,
		numCacheURLTypes
	};

	enum DownloadResult
	{
		NotExecuted =       0b0000,
		FileErrorContent =  0b1110,
		FileErrorImage =    0b1101,
		CantResolveServer = 0b1000,
		UserCancelled =    0b11000,
		ImagesUpdated =     0b0101,
		ContentUpdated =    0b0110,
		EverythingUpdated = 0b0111,
		NothingUpdated =    0b0100
	};

	struct Helpers
	{
		static int withError(int result)
		{
			return result |= CantResolveServer;
		}

		static bool wasOk(int r)
		{
			return (r & 0b1000) == 0;
		}

		static bool somethingDownloaded(DownloadResult r)
		{
			return wasOk(r) && ((r & 0xb0100) != 0);
		}

		static int getIndexFromFileName(const String& fileName)
		{
			if (fileName == "content.dat")
				return 0b0110;
			else
				return 0b0101;
		}
	};

	DocUpdater(MarkdownDatabaseHolder& holder_, bool fastMode_, bool allowEdit);
	~DocUpdater();

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

	void logMessage(const String& message) override
	{
		showStatusMessage(message);
	}

	static void runSilently(MarkdownDatabaseHolder& holder)
	{
		new DocUpdater(holder, true, false);
	}

	void run() override;

	void threadFinished() override;

	void updateFromServer();

	void createSnippetDatabase();

	void databaseWasRebuild() override
	{

	}
	
	URL getCacheUrl(CacheURLType type) const;

	URL getBaseURL() const;

	void createLocalHtmlFiles();

	void downloadAndTestFile(const String& targetFileName);

	void progress(URL::DownloadTask* /*task*/, int64 bytesDownloaded, int64 totalLength) override
	{
		setProgress((double)bytesDownloaded / (double)totalLength);
	}



	void finished(URL::DownloadTask* , bool ) override
	{

	}

	ScopedPointer<MarkdownHelpButton> helpButton1;
	ScopedPointer<MarkdownHelpButton> helpButton2;

	bool fastMode = false;
	bool editingShouldBeEnabled = false;
	MarkdownDatabaseHolder& holder;
	ScopedPointer<juce::FilenameComponent> markdownRepository;
	ScopedPointer<juce::FilenameComponent> htmlDirectory;
	ScopedPointer<DatabaseCrawler> crawler;

	int result = NotExecuted;
	ScopedPointer<URL::DownloadTask> currentDownload;

};



struct HiseMarkdownPreview : public MarkdownPreview
{
	HiseMarkdownPreview(MarkdownDatabaseHolder& holder) :
		MarkdownPreview(holder)
	{
	};

	void showDoc() override
	{
#if USE_BACKEND || HISE_USE_ONLINE_DOC_UPDATER
		auto doc = new DocUpdater(getHolder(), false, editingEnabled);
		doc->setModalBaseWindowComponent(this);
#endif
	}

	void enableEditing(bool shouldBeEnabled) override; 
	void editCurrentPage(const MarkdownLink& link, bool showExactContent = false) override;

};

}
