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

#define DECLARE_ID(x) static Identifier x(#x);

namespace MarkdownContentIds
{
DECLARE_ID(URL);
DECLARE_ID(LinkType);
DECLARE_ID(Content);
DECLARE_ID(Data);
DECLARE_ID(FilePath);
}

#undef DECLARE_ID

class MarkdownContentProcessor: public MarkdownDatabaseHolder::DatabaseListener
{
public:

	MarkdownContentProcessor(MarkdownDatabaseHolder& holder_):
		holder(holder_)
	{
		holder.addDatabaseListener(this);
	}

	virtual ~MarkdownContentProcessor()
	{
		holder.removeDatabaseListener(this);
	}

	virtual void databaseWasRebuild() {};

	MarkdownDatabaseHolder& getHolder() { return holder; }

	void clearResolvers()
	{
		imageProviders.clear();
		linkResolvers.clear();
	}

	virtual void resolversUpdated() {}

	virtual MarkdownLink resolveLink(const MarkdownLink& hyperLink)
	{
		return hyperLink;
	}

	void addLinkResolver(MarkdownParser::LinkResolver* resolver)
	{
        MarkdownParser::LinkResolver::Sorter s;
		linkResolvers.addSorted(s, resolver);
	}

	void addImageProvider(MarkdownParser::ImageProvider* provider)
	{
        MarkdownParser::ImageProvider::Sorter s;
		imageProviders.addSorted(s, provider);
	}

	template <class T> T* getTypedImageProvider()
	{
		for (auto ip : imageProviders)
		{
			if (auto t = dynamic_cast<T*>(ip))
				return t;
		}

		return nullptr;
	}

	template <class T> T* getLinkResolver()
	{
		for (auto lr : linkResolvers)
		{
			if (auto t = dynamic_cast<T*>(lr))
				return t;
		}

		return nullptr;
	}

protected:

	OwnedArray<MarkdownParser::ImageProvider> imageProviders;
	OwnedArray<MarkdownParser::LinkResolver> linkResolvers;

private:

	MarkdownDatabaseHolder& holder;
	
	JUCE_DECLARE_WEAK_REFERENCEABLE(MarkdownContentProcessor);

};

class DatabaseCrawler: public MarkdownContentProcessor
{
public:

	struct Logger
	{
		virtual ~Logger() {};

		virtual void logMessage(const String& message) 
		{
			ignoreUnused(message);
			DBG(message);
		}

		JUCE_DECLARE_WEAK_REFERENCEABLE(Logger);
	};

	struct Provider : public MarkdownParser::ImageProvider
	{
		struct Data
		{
			void createFromFile(File root);
			ValueTree v;
		};

		Provider(File root_, MarkdownParser* parent);

		MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Cached; }
		Image findImageRecursive(ValueTree& t, const MarkdownLink& url, float width);
		Image getImage(const MarkdownLink& url, float width) override;
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("DatabaseImageProvider"); }
		ImageProvider* clone(MarkdownParser* newParent) const override
		{
			return new Provider(root, newParent);
		}

		SharedResourcePointer<Data> data;
		File root;
	};

	struct Resolver : public MarkdownParser::LinkResolver
	{
		struct Data
		{
			void createFromFile(File root);
			ValueTree v;
		};

		Resolver(File root);

		String findContentRecursive(ValueTree& t, const MarkdownLink& url);

		/** This checks the database and returns the link to get the content.
		
			The HTML converter needs this in order to figure out whether a hyperlink
			points to a file or a folder.
		*/
		MarkdownLink resolveURL(const MarkdownLink& hyperLink) override;

		bool findURLRecursive(ValueTree& t, MarkdownLink& hyperLink);

		MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Cached; }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("CompressedDatabaseResolver"); };
		String getContent(const MarkdownLink& url) override;
		LinkResolver* clone(MarkdownParser* ) const override
		{
			return new Resolver(root);
		}

		File root;
		SharedResourcePointer<Data> data;
	};

	DatabaseCrawler(MarkdownDatabaseHolder& holder);

	void createContentTree();
	void addImagesFromContent(float maxWidth = 1000.0f);

	static void createImagesInHtmlFolder(File htmlRoot, MarkdownDatabaseHolder& holder, DatabaseCrawler::Logger* nonOwnedLogger, double* progress);
	static void createHtmlFilesInHtmlFolder(File htmlRoot, MarkdownDatabaseHolder& holder, DatabaseCrawler::Logger* nonOwnedLogger, double* progress);

	void createImageTree();
	void writeImagesToSubDirectory(File htmlDirectory);

	void createDataFiles(File root, bool createImages);
	void loadDataFiles(File root);

	void writeJSONTocFile(File htmlDirectory);

	void setLogger(Logger* l, bool ownThisLogger);

	void setStyleData(MarkdownLayout::StyleData& newStyleData);

	void setProgressCounter(double* p)
	{
		progressCounter = p;
	}

	int64 getHashFromFileContent(const File& f) const;

	String getContentFromCachedTree(const MarkdownLink& l);

private:

	void createHtmlFilesInternal(File htmlTemplateDirectoy, Markdown2HtmlConverter::LinkMode m, const String& linkBase);

	void addPathResolver();

	double* progressCounter = nullptr;

	int totalLinks = 0;
	int currentLink = 0;

	int totalImages = 0;
	int currentImage = 0;

	MarkdownLayout::StyleData styleData;

	Markdown2HtmlConverter::LinkMode linkMode;
	String linkBaseURL;
	
	File templateDirectory;

	void logMessage(const String& message)
	{
		if (logger != nullptr)
			logger->logMessage(message);
		if (nonOwnedLogger != nullptr)
			nonOwnedLogger->logMessage(message);
	}

	WeakReference<Logger> nonOwnedLogger;
	ScopedPointer<Logger> logger;

	void addImagesInternal(ValueTree c, float maxWidth);

	void createHtmlInternal(ValueTree v);

	void addContentToValueTree(ValueTree& v);

	ValueTree contentTree;
	ValueTree imageTree;

	int numResolved = 0;
	int numUnresolved = 0;

	MarkdownDataBase& db;
};


}
