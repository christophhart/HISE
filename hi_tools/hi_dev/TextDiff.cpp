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

#include "dtl/dtl.hpp"

namespace hise { using namespace juce;;


struct DiffHelpers
{
	using ElementType = size_t;
	using SequenceType = std::vector<size_t>;
	using LineInfo = std::pair<ElementType, dtl::eleminfo>;
	using MapType = std::map<ElementType, juce::String>;
	using DiffType = dtl::Diff<ElementType, SequenceType>;
	using Diff3Type = dtl::Diff3<ElementType, SequenceType>;
	using HunkType = std::vector<dtl::uniHunk< LineInfo>>;

	static String getBase64(const SequenceType& t)
	{
		MemoryBlock mb(t.size() * sizeof(size_t));
		size_t* ptr = reinterpret_cast<size_t*>(mb.getData());

		for (const auto& h : t)
			*ptr++ = h;

		MemoryBlock mb2;

		zstd::ZDefaultCompressor comp;
		comp.compress(mb, mb2);
		return mb2.toBase64Encoding();
	}

	static SequenceType getSequenceType(const String& base64)
	{
		MemoryBlock mb2;
		mb2.fromBase64Encoding(base64);

		MemoryBlock mb;

		zstd::ZDefaultCompressor comp;

		comp.expand(mb2, mb);

		SequenceType d;
		auto numElements = mb.getSize() / sizeof(size_t);

		d.resize(numElements, 0);

		size_t* ptr = reinterpret_cast<size_t*>(mb.getData());

		for (int i = 0; i < numElements; i++)
			d[i] = ptr[i];

        return d;
	}

	static LineDiff::Line makeDiffLine(const LineInfo& element, const MapType& map)
	{
		LineDiff::Line d;

		size_t hash = element.first;
		d.oldLine = (int)element.second.beforeIdx;
		d.newLine = (int)element.second.afterIdx;

		switch (element.second.type)
		{
		case dtl::SES_DELETE:
			d.type = LineDiff::ChangeType::Removed;
			d.newLine = -1;
			break;
		case dtl::SES_COMMON:
			d.type = LineDiff::ChangeType::Unchanged;
			break;
		case dtl::SES_ADD:
			d.type = LineDiff::ChangeType::Added;
			d.oldLine = -1;
			break;
		}

		auto it = map.find(hash);

		if (it != map.end())
			d.text = it->second;

		return d;
	}

	struct Parser
	{
		std::array<long long, 4> static parseHeader(const String& header)
		{
			jassert(header.startsWith("@@"));

			auto minus = header.fromFirstOccurrenceOf("-", false, false).upToFirstOccurrenceOf("+", false, false).trim();
			auto plus = header.fromFirstOccurrenceOf("+", false, false).upToLastOccurrenceOf("@@", false, false).trim();

			std::array<long long, 4> lineInfo;

			lineInfo[0] = minus.upToFirstOccurrenceOf(",", false, false).getLargeIntValue();
			lineInfo[1] = minus.fromFirstOccurrenceOf(",", false, false).getLargeIntValue();
			lineInfo[2] = plus.upToFirstOccurrenceOf(",", false, false).getLargeIntValue();
			lineInfo[3] = plus.fromFirstOccurrenceOf(",", false, false).getLargeIntValue();

			return lineInfo;
		}

		Parser(const String& patchContent, MapType* mapToAdd)
		{
			auto lines = StringArray::fromLines(patchContent);

			dtl::Ses<size_t> ret;
			long long x_idx = 1;
			long long y_idx = 1;

			for (const auto& p : lines)
			{
				auto firstChar = p[0];

				size_t e = p.substring(1).hash();

				switch (firstChar)
				{
				case '#':
				{
					prevSequence = getSequenceType(p.substring(1));
					break;
				}
				case '@':
				{
					auto header = parseHeader(p);
					x_idx = header[0];
					y_idx = header[2];
					break;
				}
				case ' ':
					ret.addSequence(e, x_idx, y_idx, dtl::SES_COMMON);
					++x_idx;
					++y_idx;
					break;
				case '+':

					if (mapToAdd != nullptr)
						(*mapToAdd)[e] = p.substring(1);

					ret.addSequence(e, y_idx, 0, dtl::SES_ADD);
					++y_idx;
					break;
				case '-':
					ret.addSequence(e, x_idx, 0, dtl::SES_DELETE);
					++x_idx;
					break;
				case 0:
					break;
				default:
					jassertfalse;
				}
			}

			unihunk = DiffType::composeUnifiedHunksStatic(false, ret.getSequence());
		}

		HunkType getUnihunk() const
		{
			return unihunk;
		}

		SequenceType getPrevSequence() const
		{
			return prevSequence;
		}

	private:

		SequenceType prevSequence;
		HunkType unihunk;
	};

	struct Printer
	{
		Printer(const MapType& map_) :
			map(map_)
		{
		}

		Printer& operator<<(long long hash)
		{
			d << String(hash);
			return *this;
		}

		Printer& operator<<(size_t hash)
		{
			auto h = map.find(hash);

			if (h != map.end())
				d << h->second;

			return *this;
		}

		Printer& operator<<(const char* text)
		{
			d << text;
			return *this;
		}

		void attachSequence(const SequenceType& t)
		{
			d << "\n#";
			d << getBase64(t);
		}

		String getOutput() const { return d; }

		using manipulator = std::ostream& (*)(std::ostream&);

		Printer& operator<<(manipulator manip) {
			d << "\n";
			return *this;
		}

	private:

		String d;
		const MapType& map;
	};
};


LineDiff::HashedDiff::HashedDiff(const String& patch)
{
	DiffHelpers::Parser p(patch, &hashmap);

	ah = p.getPrevSequence();
    auto h = p.getUnihunk();
	bh = DiffHelpers::DiffType::uniPatchStatic(ah, h);
}

LineDiff::HashedDiff::HashedDiff(const StringArray& A, const StringArray& B)
{
	ah = toHash(A);
	bh = toHash(B);
}

LineDiff::HashedDiff::HashedDiff(const StringArray& A, const std::vector<Change>& changes)
{
	ah = toHash(A);

	String patches;

	for(auto& c: changes)
		patches << c.toUnifiedPatchFormat().joinIntoString("\n");

	DiffHelpers::Parser p(patches, &hashmap);

    auto h = p.getUnihunk();
	bh = DiffHelpers::DiffType::uniPatchStatic(ah, h);
}

juce::StringArray LineDiff::HashedDiff::getPatchReport() const
{
	DiffHelpers::DiffType d(ah, bh);
	d.compose();

	DiffHelpers::Printer p(hashmap);

	d.composeUnifiedHunks();
	d.printUnifiedFormat(p);
	p.attachSequence(ah);

	return StringArray::fromLines(p.getOutput());
}

void LineDiff::HashedDiff::merge(StringArray& C)
{
	auto ch = toHash(C);

	DiffHelpers::Diff3Type diff3(bh, ah, ch);
	diff3.compose();

	if (!diff3.merge())
	{

		auto conflict = diff3.getConflictData();

		MergeError e;
		e.mine = DiffHelpers::makeDiffLine(conflict[0], hashmap);
		e.theirs = DiffHelpers::makeDiffLine(conflict[1], hashmap);

		throw e;
	}

	auto merged = diff3.getMergedSequence();

	StringArray M;
	M.ensureStorageAllocated((int)merged.size());

	for (auto& h : merged)
	{
		M.add(hashmap[h]);
	}

	M.swapWith(C);
}


std::vector<LineDiff::Change> LineDiff::HashedDiff::createHunks() const
{
	DiffHelpers::DiffType d(ah, bh);
	d.compose();
	d.composeUnifiedHunks();

	auto hunks = d.getUniHunks();

	std::vector<Change> changes;

	for (const auto& hunk : hunks)
	{
		Change change;

		auto f = [&](const DiffHelpers::LineInfo& element)
		{
			change.lines.push_back(DiffHelpers::makeDiffLine(element, this->hashmap));
		};

		std::for_each(hunk.common[0].begin(), hunk.common[0].end(), f);
		std::for_each(hunk.change.begin(), hunk.change.end(), f);
		std::for_each(hunk.common[1].begin(), hunk.common[1].end(), f);

		changes.push_back(change);
	}

	return changes;
}

std::vector<size_t> LineDiff::HashedDiff::getHashMap(bool getOld) const
{
	return getOld ? ah : bh;
}

String LineDiff::HashedDiff::getContent(bool getOld) const
{
	String s;

	const auto& v = getOld ? ah : bh;
	auto it = v.begin();
	
	while(it != v.end())
	{
		s << hashmap.at(*it);

		if((it + 1) != v.end())
			s << "\n";

		it++;
	}

	return s;
}

std::vector<size_t> LineDiff::HashedDiff::toHash(const StringArray& sa)
{
	std::vector<size_t> hash;
	hash.reserve(sa.size());

	for (const auto& s : sa)
	{
		auto h = s.hash();
		hashmap[h] = s;
		hash.push_back(h);
	}

	return hash;
}

juce::StringArray LineDiff::Change::toUnifiedPatchFormat() const
{
	StringArray sa;

	if (!lines.empty())
	{
		int hunkStartOld = lines[0].oldLine;
		int hunkStartNew = lines[0].newLine;
		int oldCount = lines.back().oldLine - hunkStartOld;
		int newCount = lines.back().newLine - hunkStartNew;

		juce::String hunkHeader = juce::String::formatted(
			"@@ -%d,%d +%d,%d @@", hunkStartOld, oldCount, hunkStartNew, newCount);

		sa.add(hunkHeader);

		for (const auto& line : lines)
		{
			switch (line.type)
			{
			case ChangeType::Unchanged: sa.add(" " + line.text); break;
			case ChangeType::Removed:   sa.add("-" + line.text); break;
			case ChangeType::Added:     sa.add("+" + line.text); break;
			}
		}
	}

	return sa;
}

hise::LineDiff::Change LineDiff::Change::invert() const
{
	Change inv;

	for (auto lineCopy : lines)
	{
		switch (lineCopy.type)
		{
		case ChangeType::Unchanged:
			break;
		case ChangeType::Added:
			lineCopy.type = ChangeType::Removed;
			break;
		case ChangeType::Removed:
			lineCopy.type = ChangeType::Added;
			break;
		};

		inv.lines.push_back(lineCopy);
	}

	auto it = inv.lines.begin();
	auto end = inv.lines.end();

	while (it != end)
	{
		if (it->type == ChangeType::Added)
		{
			auto addStart = it;

			while (it != end && it->type == ChangeType::Added)
				it++;

			if (it != end && it->type == ChangeType::Removed)
			{
				auto remStart = it;

				while (it != end && it->type == ChangeType::Removed)
					it++;

				auto remEnd = it;

				std::rotate(addStart, remStart, remEnd);
			}
		}
		else
		{
			it++;
		}
	}

	int oldIndex = lines[0].newLine;
	int newIndex = lines[0].oldLine;

	for (auto& invLine : inv.lines)
	{
		invLine.oldLine = oldIndex;
		invLine.newLine = newIndex;

		switch (invLine.type)
		{
		case ChangeType::Unchanged:
			oldIndex++;
			newIndex++;
			break;
		case ChangeType::Added:
			invLine.oldLine = -1;
			newIndex++;
			break;
		case ChangeType::Removed:
			invLine.newLine = -1;
			oldIndex++;
			break;
		};
	}

	return inv;
}

bool LineDiff::isTextFile(const File& f)
{
	static const StringArray textExtensions({
		".h",
		".cpp",
		".dsp",
		".js",
		".xml",
		".css",
		".glsl",
		".md",
		".json",
		".txt",
		".hpp",
		".c",
		".hxx",
		".cxx",
		".inl"
		});

	static constexpr int FileSizeLimit = 500 * 1024;

	return textExtensions.contains(f.getFileExtension()) && f.getSize() < FileSizeLimit;
}

juce::File LineDiff::getPatchFile(const File& patchFile)
{
	return patchFile.getSiblingFile(patchFile.getFileName()).withFileExtension(".patch");
}

bool LineDiff::isChangedTextFile(const File& f, int64 prevHash)
{
	if (isTextFile(f))
		return f.loadFileAsString().hashCode64() != prevHash;

	return false;
}

} // namespace hise
