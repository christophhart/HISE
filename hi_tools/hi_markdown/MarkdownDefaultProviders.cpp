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

MarkdownParser::DefaultLinkResolver::DefaultLinkResolver(MarkdownParser* parser_) :
	parser(parser_)
{

}

bool MarkdownParser::DefaultLinkResolver::linkWasClicked(const String& url)
{
	if (url.startsWith("CLIPBOARD::"))
	{
		String content = url.fromFirstOccurrenceOf("CLIPBOARD::", false, false);

		SystemClipboard::copyTextToClipboard(content);
		return true;
	}

	if (url.startsWith("http"))
	{
		URL u(url);
		u.launchInDefaultBrowser();
		return true;
	}

	if (url.startsWith("#"))
	{
		if (auto renderer = dynamic_cast<MarkdownRenderer*>(parser))
		{
			for (auto e : parser->elements)
			{
				if (auto headLine = dynamic_cast<Headline*>(e))
				{
					if (url == headLine->anchorURL)
						renderer->scrollToY(headLine->anchorY);
				}
			}

			return true;
		}
		else
			return false;

		
	}

	return false;
}

hise::MarkdownParser::LinkResolver* MarkdownParser::DefaultLinkResolver::clone(MarkdownParser* newParser) const
{
	return new DefaultLinkResolver(newParser);
}


MarkdownParser::FileLinkResolver::FileLinkResolver(const File& root_) :
	root(root_)
{

}


juce::String MarkdownParser::FileLinkResolver::getContent(const String& url)
{
	String urlToUse = url;

	if (urlToUse.startsWith("/"))
		urlToUse = urlToUse.fromFirstOccurrenceOf("/", false, false);

	File match = HtmlGenerator::getLocalFileForSanitizedURL(root, url, File::findFiles, "*.md");

	if (match.existsAsFile())
		return match.loadFileAsString();
	else
		return {};
}


bool MarkdownParser::FileLinkResolver::linkWasClicked(const String& url)
{
	return false;
}

MarkdownParser::FileBasedImageProvider::FileBasedImageProvider(MarkdownParser* parent, const File& root) :
	ImageProvider(parent),
	r(root)
{

}

juce::Image MarkdownParser::FileBasedImageProvider::getImage(const String& imageURL, float width)
{
	File imageFile = r.getChildFile(imageURL);

	if (imageFile.existsAsFile() && ImageFileFormat::findImageFormatForFileExtension(imageFile) != nullptr)
		return resizeImageToFit(ImageCache::getFromFile(imageFile), width);

	return {};
}



juce::String MarkdownParser::FolderTocCreator::getContent(const String& url)
{
	String urlToUse = url.upToFirstOccurrenceOf("#", false, false);

	if (urlToUse.startsWith("/"))
		urlToUse = urlToUse.substring(1);

	File directory = HtmlGenerator::getLocalFileForSanitizedURL(rootFile, url, File::findDirectories);

	if (directory.isDirectory())
	{
		String s;

		auto readme = directory.getChildFile("Readme.md");

		if (readme.existsAsFile())
		{
			s << readme.loadFileAsString();
			s << "  \n";

			return s;
		}
		else
		{
			s << "## Content of " << HtmlGenerator::removeLeadingNumbers(directory.getFileName()) << "  \n";

			Array<File> files;

			directory.findChildFiles(files, File::findFilesAndDirectories, false);

			files.sort();

			for (auto f : files)
			{
				if (f == readme)
					continue;

				auto path = f.getRelativePathFrom(rootFile);

				s << "[" << HtmlGenerator::removeLeadingNumbers(f.getFileNameWithoutExtension()) << "](" << HtmlGenerator::getSanitizedFilename(path) << ")  \n";
			}

			return s;
		}
	}

	return {};
}

MarkdownParser::FolderTocCreator::FolderTocCreator(const File& rootFile_) :
	LinkResolver(),
	rootFile(rootFile_)
{

}

hise::MarkdownParser::LinkResolver* MarkdownParser::FolderTocCreator::clone(MarkdownParser* parent) const
{
	return new FolderTocCreator(rootFile);
}

juce::Image MarkdownParser::URLImageProvider::getImage(const String& urlName, float width)
{
	if (URL::isProbablyAWebsiteURL(urlName))
	{
		URL url(urlName);

		auto path = url.getSubPath();
		auto imageFile = tempDirectory.getChildFile(path);

		if (imageFile.existsAsFile())
			return resizeImageToFit(ImageCache::getFromFile(imageFile), width);

		imageFile.create();

		ScopedPointer<URL::DownloadTask> task = url.downloadToFile(imageFile);

		if (task == nullptr)
		{
			jassertfalse;
			return {};
		}

		int timeout = 5000;

		auto start = Time::getApproximateMillisecondCounter();

		while (!task->isFinished())
		{
			if (Time::getApproximateMillisecondCounter() - start > timeout)
				break;

			Thread::sleep(500);
		}

		if (task->isFinished() && !task->hadError())
		{
			return resizeImageToFit(ImageCache::getFromFile(imageFile), width);
		}

		return {};
	}

	return {};
}

MarkdownParser::URLImageProvider::URLImageProvider(File tempdirectory_, MarkdownParser* parent) :
	ImageProvider(parent),
	tempDirectory(tempdirectory_)
{
	if (!tempDirectory.isDirectory())
		tempDirectory.createDirectory();
}

juce::Image MarkdownParser::GlobalPathProvider::getImage(const String& urlName, float width)
{
	if (urlName.startsWith(path_wildcard))
	{
		float widthToUseMax = jmin(width, 64.0f);

		Path p;

		String nameToCheck = urlName.fromFirstOccurrenceOf(path_wildcard, false, false);

		for (auto f : factories->factories)
		{
			p = f->createPath(nameToCheck);

			if (!p.isEmpty())
				break;

		}

		if (p.isEmpty())
		{
			// Tried to resolve a global path without success
			return {};
		}

		p.scaleToFit(0.0f, 0.0f, widthToUseMax, widthToUseMax, true);

		Image img(Image::ARGB, (int)widthToUseMax, (int)widthToUseMax, true);
		Graphics g(img);
		
		g.setColour(parent->getStyleData().textColour);

		g.fillPath(p);

		return img;
	}

	return {};
}

MarkdownParser::GlobalPathProvider::GlobalPathProvider(MarkdownParser* parent) :
	ImageProvider(parent)
{

}

}