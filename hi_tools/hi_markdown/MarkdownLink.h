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

/** A MarkdownLink is used to resolve links in a Markdown document.

	It is similar to the juce::File object as it takes a String as constructor and then
	offers some convenient functions and safe-checks.
*/
class MarkdownLink
{
public:

	struct Helpers
	{
		static String getPrettyName(const String& urlToPrettify)
		{
			auto n = urlToPrettify.replaceCharacter('-', ' ');
			String pretty;
			auto ptr = n.getCharPointer();

			bool nextIsUppercase = true;

			while (!ptr.isEmpty())
			{
				auto thisChar = *ptr;

				if (nextIsUppercase)
					pretty << CharacterFunctions::toUpperCase(thisChar);
				else
					pretty << thisChar;

				nextIsUppercase = thisChar == ' ';

				ptr++;
			}

			return pretty;
		}

		static String getAnchor(const String& url)
		{
			return url.fromFirstOccurrenceOf("#", true, false);
		}

		static String removeAnchor(const String& url)
		{
			return url.upToFirstOccurrenceOf("#", false, false);
		}

		static String getExtraData(const String& url)
		{
			return url.fromFirstOccurrenceOf(":", false, false);
		}

		static String getPrettyVarString(const var& value)
		{
			String valueString;

			if (value.isObject())
				valueString = "`{}`";
			else if (value.isArray())
				valueString = "`[]`";
			else if (value.isBool())
				valueString = (bool)value ? "`true`" : "`false`";
			else
				valueString = value.toString();

			if (valueString.isEmpty())
				valueString << "`\"\"`";

			return valueString;
		}

		static String removeExtraData(const String& url)
		{
			return url.upToFirstOccurrenceOf(":", false, false);
		}

		static File getLocalFileForSanitizedURL(File root, const String& url, File::TypesOfFileToFind filetype = File::findFiles)
		{
			auto urlToUse = url;
			if (urlToUse.startsWith("/"))
				urlToUse = urlToUse.substring(1);

			auto f = root.getChildFile(urlToUse);

			if (f.isDirectory())
			{
				if (filetype == File::findDirectories)
					return f;
				else
					return f.getChildFile("Readme.md");
			}

			if (!f.existsAsFile())
				f = root.getChildFile(urlToUse).withFileExtension(".md");

			return f;

#if 0
			// You have to sanitize this before calling this method
			jassert(!url.contains("#"));

			auto urlToUse = removeAnchor(url);

			if (urlToUse.startsWithChar('/'))
				urlToUse = urlToUse.substring(1);

			Array<File> files;

			root.findChildFiles(files, filetype, true, extension);

			for (auto f : files)
			{
				auto path = f.getRelativePathFrom(root);

				path = getSanitizedFilename(path);

				if (path == urlToUse)
					return f;
			}

			return {};
#endif
		}

		static File getFolderReadmeFile(File root, const String& url)
		{
			auto f = getLocalFileForSanitizedURL(root, url, File::findDirectories);

			if (f.isDirectory())
			{
				return f.getChildFile("Readme.md");
			}

			return {};
		}

		static String getSanitizedFilename(const String& path)
		{
			if (path.startsWith("http"))
				return path;

			auto p = removeLeadingNumbers(path);

			return StringSanitizer::get(p);
		}

		static String getSanitizedURL(const String& path)
		{
			auto p = getSanitizedFilename(path);

			if (p.startsWith("/"))
				return p;

			return "/" + p;
		}

		static String removeLeadingNumbers(const String& p)
		{
			auto path = p.replaceCharacter('\\', '/').trimCharactersAtStart("01234567890 ");
			path = path.removeCharacters("()[]");
			return path;
		}

		static bool isImageLink(const String& url)
		{
			return url.endsWith(".jpg") || url.endsWith(".JPG") ||
				url.endsWith(".gif") || url.endsWith(".GIF") ||
				url.endsWith(".png") || url.endsWith(".PNG");
		}

		static double getSizeFromExtraData(const String& extraData);

#if 0
		static String createHtmlLink(const String& url, const String& rootString)
		{
			if (url.startsWith("http"))
			{
				return url;
			}

			MarkdownLink l({}, url);

			return l.toString(FormattedLinkHtml);

#if 0
			String absoluteFilePath = rootString + url;

			auto urlWithoutAnchor = url.upToFirstOccurrenceOf("#", false, false);
			auto anchor = url.fromFirstOccurrenceOf("#", true, false);

			auto urlWithoutExtension = urlWithoutAnchor.upToLastOccurrenceOf(".", false, false);

			bool isFile = urlWithoutExtension != urlWithoutAnchor;

			String realURL;

			realURL << rootString;

			if (!rootString.endsWith("/") && !urlWithoutExtension.startsWith("/"))
				realURL << "/";

			realURL << getSanitizedFilename(urlWithoutExtension);

			if (isFile)
				realURL << ".html";
			else
				realURL << "/index.html";

			realURL << anchor;


			return realURL;
#endif
		};
#endif



		static bool isReadme(File f)
		{
			return f.getFileNameWithoutExtension().toLowerCase() == "readme";
		}

		static File getFileOrReadmeFromFolder(File root, const String& url)
		{
			auto f = getFolderReadmeFile(root, url);
			if (f.existsAsFile())
				return f;

			f = getLocalFileForSanitizedURL(root, url, File::findFiles);

			if (f.existsAsFile())
				return f;

			return {};
		}

		static String getFileNameFromURL(const String& url)
		{
			return url.fromLastOccurrenceOf("/", false, false).upToFirstOccurrenceOf("#", false, false);
		}

		static String getChildURL(const String &url, const String& childName, bool asAnchor = false)
		{


			jassert(getSanitizedFilename(url) == url);

			return removeAnchor(url) + (asAnchor ? "#" : "/") + getSanitizedFilename(childName);
		}

		static String removeMarkdownHeader(const String& content);

		static String getMarkdownHeader(const String& content);
	};



	enum Format
	{
		Everything = 0,			///< returns the whole link including extra data
		UrlFull,				///< returns the whole link (without the extra data)
		UrlSubPath,					///< /root/something/funky#hello => funky
		UrlWithoutAnchor,				///< /root/something/funky#hello => /root/something/funky
		SubURL,					///< https://youtube.com/funkboymclovin/0.jpg => funkboymclovin/0.jpg
		AnchorWithHashtag,		///< /root/something/funky#hello => #hello
		AnchorWithoutHashtag,	///< /root/something/funky#hello => hello
		FormattedLinkHtml,					///< /root/something/funky#hello => /root/something/funky.html#hello
		FormattedLinkMarkdown,		///< /root/something/funky#hello => [funky](/root/something/funky#hello)
		FormattedLinkMarkdownImage,		///< /root/something/funky#hello => ![funky](/root/something/funky#hello)
		FormattedLinkIcon,		///< /images/icon_MyIcon => MyIcon
		ContentFull,			///< loads the file as string if it exists
		ContentWithoutHeader,	///< loads the file content without the header.
		ContentHeader				///< loads the YAML header from the file
	};

	enum class FileType
	{
		HtmlFile,			///< returns the file as it will be in the HTML directory
		ContentFile,		///< returns the file as markdown file. If the link points to a directory, it will use its readme
		Directory,			///< returns the directory. If the link points to a file, it will return the parent directory.
		ImageFile			///< returns the file from the image directory
	};

	enum Type
	{
		Invalid = 0,
		Rootless,		 ///< a (maybe) valid URL that was created without a root
		MarkdownFileOrFolder, ///< a unresolved link that is supposed to point to a file or folder.
		MarkdownFile,	 ///< a resolved link that points to a markdown file in the directory
		Folder,			 ///< a resolved link that points to a folder in the root directory
		SimpleAnchor,	 ///< just a anchor on the same page
		WebContent,		 ///< points to a web resource
		Icon,			 ///< points to a icon creatable using a PathFactory
		Image,			 ///< a image link
		SVGImage,		 ///< a vector image link
		numTypes
	};

	MarkdownLink();

	bool operator==(const MarkdownLink& other) const;

	/** Creates a markdown link from a String and the root directory. You can pass any string that is appropriate and the
		link will parse it and create a meaningful URL from it. */
	MarkdownLink(const File& rootDirectory, const String& url);

	/** Creates a markdown link without a root. In this case, there will be no processing and the link must be formatted correctly. */
	static MarkdownLink createWithoutRoot(const String& validURL, Type forcedType=Type::numTypes);

	/** Resolves the link to a file in the given root directory. */
	File toFile(FileType type, File rootToUse = {}) const noexcept;

	/** returns the markdown file associated with this link. If the type is a folder, it will return the Readme.md file. */
	File getMarkdownFile(const File& rootDirectory) const noexcept;

	/** Returns a copy of the link with the given anchor. */
	MarkdownLink withAnchor(const String& newAnchor) const noexcept;

	/** This will return the image file. */
	File getImageFile(const File& rootDirectory) const noexcept;

	File getRoot() const noexcept { return root; }

	/** Creates a string representation. */
	String toString(Format format, const File& rootDirectory = {}) const noexcept;

	/** Returns the type. */
	Type getType() const noexcept;

	String getTypeString() const noexcept;

	String getNameFromHeader() const;

	/** Returns a child URL for the given subpath. If it's a folder it will return a child url.
		if it's a file, it will return an anchor.

		Attention: This will remove the root directory to prevent file searching and exponential slowdown during construction. If you
		want to get a child URL with the root, use getChildUrlWithRoot instead.
	*/
	MarkdownLink getChildUrl(const String& childName, bool asAnchor = false) const;

	MarkdownLink getChildUrlWithRoot(const String& childName, bool asAnchor = false) const;

	/** If the link is either a file or a folder readme, this will look up in the directory to resolve it. */
	bool resolveFileOrFolder(const File& rootDirectory = File());

	/** Returns the parent URL. If it has an anchor, it removes the anchor, otherwise it will point to the parent directory. */
	MarkdownLink getParentUrl() const;

	/** Checks if the link is a child URL of the other link. This only works if otherlink is a folder. */
	bool isChildOf(const MarkdownLink& parent) const;

	/** Returns true if the links are on the same page. */
	bool isSamePage(const MarkdownLink& otherLink) const;

	File getDirectory(const File& rootDirectory) const noexcept;

	/** Returns a pretty version of the filename without leading numbers. */
	String getPrettyFileName() const noexcept;

	bool hasAnchor() const noexcept { return toString(AnchorWithoutHashtag).isNotEmpty(); }

	bool isValid() const noexcept { return type != Invalid; }

	bool isInvalid() const noexcept { return type == Invalid; }

	bool isImageType() const noexcept;

	MarkdownLink withRoot(const File& rootDirectory, bool reparseLink) const;

	MarkdownLink withPostData(const String& postData) const;

	MarkdownLink withExtraData(String extraData) const;

	String getPostData() const noexcept { return postData; }

	String getExtraData() const noexcept;

	MarkdownHeader getHeaderFromFile(const File& rootDirectory) const;

	void setType(Type t)
	{
		type = t;
	}

	explicit operator bool() const 
	{ 
		switch (type)
		{
		case hise::MarkdownLink::Icon:
		case hise::MarkdownLink::WebContent:
		case hise::MarkdownLink::SimpleAnchor:	return true;
		case hise::MarkdownLink::Invalid:		return false;
		case hise::MarkdownLink::MarkdownFile:
		case hise::MarkdownLink::Folder:
		case hise::MarkdownLink::Image:
		case hise::MarkdownLink::SVGImage:		return fileExists({});
		default:
			break;
		}

		return false;
	}

	bool fileExists(const File& rootDirectory) const noexcept;

	String getHtmlStringForBaseURL(const String& baseURL)
	{
		String u;

		u << baseURL;
		u << toString(FormattedLinkHtml);
		return u;
	}

private:

	String createHtmlLink() const noexcept;
	

	File root;
	Type type;
	String originalURL;
	String sanitizedURL;
	String anchor;
	String extraString;
	String postData;
	File file;
};

}