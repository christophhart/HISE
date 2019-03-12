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

	void addLinkResolver(MarkdownParser::LinkResolver* resolver)
	{
		linkResolvers.addSorted(MarkdownParser::LinkResolver::Sorter(), resolver);
	}

	void addImageProvider(MarkdownParser::ImageProvider* provider)
	{
		imageProviders.addSorted(MarkdownParser::ImageProvider::Sorter(), provider);
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
			DBG(message);
		}
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

		MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Cached; }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("CompressedDatabaseResolver"); };
		String getContent(const MarkdownLink& url) override;
		LinkResolver* clone(MarkdownParser* parent) const override
		{
			return new Resolver(root);
		}

		File root;
		SharedResourcePointer<Data> data;
	};

	DatabaseCrawler(MarkdownDatabaseHolder& holder);

	void createContentTree();
	void addImagesFromContent(float maxWidth = 1000.0f);

	void createHtmlFiles(File root, File htmlTemplateDirectoy, Markdown2HtmlConverter::LinkMode m, const String& linkBase);
	void createImageTree();
	void writeImagesToSubDirectory(File htmlDirectory);

	

	

	void createDataFiles(File root, bool createImages);

	void loadDataFiles(File root);

	void setLogger(Logger* l)
	{
		logger = l;
	}

	void setStyleData(MarkdownLayout::StyleData& newStyleData)
	{
		styleData = newStyleData;
	}

	void setProgressCounter(double* p)
	{
		progressCounter = p;
	}

private:

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
	}

	ScopedPointer<Logger> logger;

	void addImagesInternal(ValueTree c, float maxWidth);

	void createHtmlInternal(File currentRoot, ValueTree v);

	void addContentToValueTree(ValueTree& v);

	ValueTree contentTree;
	ValueTree imageTree;

	int numResolved = 0;
	int numUnresolved = 0;

	MarkdownDataBase& db;
};


}