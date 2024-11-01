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
		static String getPrettyName(const String& urlToPrettify);
		static String getAnchor(const String& url);
		static String removeAnchor(const String& url);
		static String getExtraData(const String& url);
		static String getPrettyVarString(const var& value);
		static String removeExtraData(const String& url);
		static File getLocalFileForSanitizedURL(File root, const String& url, File::TypesOfFileToFind filetype = File::findFiles);
		static File getFolderReadmeFile(File root, const String& url);
		static String getSanitizedFilename(const String& path);
		static String getSanitizedURL(const String& path);
		static String removeLeadingNumbers(const String& p);
		static bool isImageLink(const String& url);
		static double getSizeFromExtraData(const String& extraData);
		static bool isReadme(File f);
		static File getFileOrReadmeFromFolder(File root, const String& url);
		static String getFileNameFromURL(const String& url);
		static String getChildURL(const String &url, const String& childName, bool asAnchor = false);
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

	void setType(Type t);

	explicit operator bool() const;

	bool fileExists(const File& rootDirectory) const noexcept;

	String getHtmlStringForBaseURL(const String& baseURL) const;

	String getEditLinkOnGitHub(bool rawLink) const;
	
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