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

namespace hise { using namespace juce;

class BackendRootWindow;
class PopupIncludeEditor;

/** Tracks the git state of the project folder.

	- runs git on a background thread (status, diff --name-status, show) and caches the result
	- feeds the line diff of each open code editor into mcl::TextEditor::setDiffLines()

	If git is not installed or the project is not inside a repository, every file is reported as clean.
*/
class GitChangeTracker : public ChangeBroadcaster,
						 private Thread,
						 private Timer,
						 private AsyncUpdater
{
public:

	/** The order defines the priority when the state of a folder is aggregated. */
	enum class Status
	{
		Clean = 0,
		Ignored,
		Untracked,
		Added,
		Modified,
		Conflict,
		numStatus
	};

	enum class BaseMode
	{
		Head,
		SessionStart,
		BranchPoint,
		numBaseModes
	};

	GitChangeTracker(BackendRootWindow* rootWindow);

	~GitChangeTracker();

	/** Returns the git state of a file or the aggregated state of a folder. */
	Status getStatus(const File& f) const;

	/** Returns true if the project folder is inside a git repository. */
	bool isRepository() const;

	/** Changes when a file appears or disappears in the project folder (untracked, added or deleted). */
	int64 getFileSetHash() const;

	/** Returns the changed files that exist on disk (key -> modification time in ms). */
	std::map<String, int64> getChangedFiles() const;

	/** The key used for the status lookup (the full path, lowercase if file names are case insensitive). */
	static String getKey(const File& f);

	/** Returns a short description of the commit the changes are compared against. */
	String getBaseDescription() const;

	/** Rescans the git state on the next timer tick. */
	void requestStatusUpdate();

	/** Adds the "Show changes since" items. Item IDs start at the given offset. */
	void addBaseMenuItems(PopupMenu& m, int offset) const;

	/** Performs a menu item that was added with addBaseMenuItems(). Returns false if the ID is not a base item. */
	bool performBaseMenuItem(int result, int offset);

	static Colour getColour(Status s);

	static String getLetter(Status s);

	/** Returns the file that holds the merged callbacks of the processor: either the connected script
		or the file that "Save as XML" writes to Scripts/ScriptProcessors/{rootId}/{processorId}.js. */
	static File getCallbackFile(ModulatorSynthChain* chain, JavascriptProcessor* jp);

private:

	struct Snapshot : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Snapshot>;

		bool isRepository = false;
		File repoRoot;
		String baseCommit;
		String headCommit;
		StringArray branches;
		HashMap<String, int> fileStatus;
		HashMap<String, int> folderStatus;
		StringArray ignoredFolders;
		std::map<String, int64> changedFiles;
		int64 fileSetHash = 0;
	};

	struct DiffJob
	{
		Component::SafePointer<PopupIncludeEditor> editor;
		File file;
		String content;
		int64 hash = 0;
	};

	struct DiffResult
	{
		Component::SafePointer<PopupIncludeEditor> editor;
		std::map<int, LineDiff::ChangeType> lines;
	};

	struct GitOutput
	{
		bool started = false;
		int exitCode = -1;
		MemoryBlock data;

		String toString() const { return String::fromUTF8((const char*)data.getData(), (int)data.getSize()); }
	};

	struct EditorState
	{
		Component::SafePointer<PopupIncludeEditor> editor;
		int64 lastHash = 0;
	};

	GitOutput runGit(const File& workingDirectory, const StringArray& args) const;

	void run() override;

	void timerCallback() override;

	void handleAsyncUpdate() override;

	Snapshot::Ptr createSnapshot(const File& projectDirectory);

	DiffResult createDiff(const DiffJob& job, const Snapshot& s);

	String getBaseContent(const Snapshot& s, const File& f);

	void updateEditors();

	Snapshot::Ptr getSnapshot() const;

	Component::SafePointer<BackendRootWindow> rootWindow;

	mutable CriticalSection lock;

	Snapshot::Ptr snapshot;

	// guarded by lock
	BaseMode baseMode = BaseMode::Head;
	String baseBranch;
	String sessionStartCommit;
	File requestedDirectory;
	bool statusUpdatePending = true;
	Array<DiffJob> pendingDiffs;
	Array<DiffResult> finishedDiffs;
	bool snapshotChanged = false;

	// background thread only
	std::map<String, String> baseContentCache;

	// message thread only
	Array<EditorState> editorStates;
	String lastBaseCommit;
	int tickCounter = 0;
	bool wasForeground = true;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GitChangeTracker);
};

} // namespace hise
