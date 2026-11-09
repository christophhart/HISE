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

namespace hise { using namespace juce;

namespace GitChangeTrackerHelpers
{
static StringArray args(std::initializer_list<String> list)
{
	StringArray sa;

	for (const auto& s : list)
		sa.add(s);

	return sa;
}

static StringArray splitNullTerminated(const MemoryBlock& mb)
{
	StringArray entries;

	auto data = static_cast<const char*>(mb.getData());
	auto size = (int)mb.getSize();
	int start = 0;

	for (int i = 0; i < size; i++)
	{
		if (data[i] == 0)
		{
			entries.add(String::fromUTF8(data + start, i - start));
			start = i + 1;
		}
	}

	if (start < size)
		entries.add(String::fromUTF8(data + start, size - start));

	return entries;
}
}

GitChangeTracker::GitChangeTracker(BackendRootWindow* rw) :
	Thread("Git Change Tracker"),
	rootWindow(rw)
{
	snapshot = new Snapshot();

	ThreadStarters::startLow(this);
	startTimer(500);
}

GitChangeTracker::~GitChangeTracker()
{
	stopTimer();
	cancelPendingUpdate();

	signalThreadShouldExit();
	notify();
	stopThread(5000);
}

String GitChangeTracker::getKey(const File& f)
{
	auto p = f.getFullPathName();
	return File::areFileNamesCaseSensitive() ? p : p.toLowerCase();
}

static GitChangeTracker::Status getStatusFromSnapshot(const HashMap<String, int>& fileStatus,
													  const HashMap<String, int>& folderStatus,
													  const StringArray& ignoredFolders,
													  const String& key)
{
	using Status = GitChangeTracker::Status;

	if (fileStatus.contains(key))
		return (Status)fileStatus[key];

	if (folderStatus.contains(key))
		return (Status)folderStatus[key];

	for (const auto& ig : ignoredFolders)
	{
		if (key == ig || key.startsWith(ig + File::getSeparatorString()))
			return Status::Ignored;
	}

	return Status::Clean;
}

GitChangeTracker::Status GitChangeTracker::getStatus(const File& f) const
{
	auto s = getSnapshot();

	if (!s->isRepository)
		return Status::Clean;

	return getStatusFromSnapshot(s->fileStatus, s->folderStatus, s->ignoredFolders, getKey(f));
}

File GitChangeTracker::getCallbackFile(ModulatorSynthChain* chain, JavascriptProcessor* jp)
{
	auto p = dynamic_cast<Processor*>(jp);

	if (jp->isConnectedToExternalFile())
	{
		auto reference = jp->getConnectedFileReference();

		if (reference.contains("{GLOBAL_SCRIPT_FOLDER}"))
		{
			auto relativePath = reference.fromFirstOccurrenceOf("{GLOBAL_SCRIPT_FOLDER}", false, false);
			return PresetHandler::getGlobalScriptFolder(p).getChildFile(relativePath);
		}

		return File(GET_PROJECT_HANDLER(p).getFilePath(reference, ProjectHandler::SubDirectories::Scripts));
	}

	auto backupDirectory = XmlBackupFunctions::getScriptDirectoryFor(chain);
	return XmlBackupFunctions::getScriptFileFor(chain, backupDirectory, p->getId());
}

bool GitChangeTracker::isRepository() const
{
	return getSnapshot()->isRepository;
}

int64 GitChangeTracker::getFileSetHash() const
{
	return getSnapshot()->fileSetHash;
}

std::map<String, int64> GitChangeTracker::getChangedFiles() const
{
	return getSnapshot()->changedFiles;
}

String GitChangeTracker::getBaseDescription() const
{
	auto s = getSnapshot();

	if (!s->isRepository)
		return {};

	ScopedLock sl(lock);

	switch (baseMode)
	{
	case BaseMode::Head:		 return "last commit";
	case BaseMode::SessionStart: return "session start (" + s->baseCommit.substring(0, 7) + ")";
	case BaseMode::BranchPoint:	 return "branch point with " + baseBranch;
	default:					 return {};
	}
}

void GitChangeTracker::requestStatusUpdate()
{
	{
		ScopedLock sl(lock);
		statusUpdatePending = true;
	}

	notify();
}

void GitChangeTracker::addBaseMenuItems(PopupMenu& m, int offset) const
{
	auto s = getSnapshot();

	PopupMenu sub;

	ScopedLock sl(lock);

	sub.addItem(offset, "Last commit (HEAD)", true, baseMode == BaseMode::Head);

	String sessionName = "Session start";

	if (sessionStartCommit.isNotEmpty())
		sessionName << " (" << sessionStartCommit.substring(0, 7) << ")";

	sub.addItem(offset + 1, sessionName, true, baseMode == BaseMode::SessionStart);

	if (!s->branches.isEmpty())
	{
		sub.addSeparator();

		for (int i = 0; i < s->branches.size(); i++)
		{
			auto isActive = baseMode == BaseMode::BranchPoint && baseBranch == s->branches[i];
			sub.addItem(offset + 2 + i, "Branch point with " + s->branches[i], true, isActive);
		}
	}

	m.addSubMenu("Show changes since", sub, s->isRepository);
}

bool GitChangeTracker::performBaseMenuItem(int result, int offset)
{
	auto index = result - offset;

	if (index < 0)
		return false;

	auto s = getSnapshot();

	{
		ScopedLock sl(lock);

		if (index == 0)
			baseMode = BaseMode::Head;
		else if (index == 1)
			baseMode = BaseMode::SessionStart;
		else if (isPositiveAndBelow(index - 2, s->branches.size()))
		{
			baseMode = BaseMode::BranchPoint;
			baseBranch = s->branches[index - 2];
		}
		else
			return false;

		statusUpdatePending = true;
	}

	notify();
	return true;
}

Colour GitChangeTracker::getColour(Status s)
{
	switch (s)
	{
	case Status::Ignored:	return Colours::white.withAlpha(0.3f);
	case Status::Untracked:
	case Status::Added:		return Colour(HISE_OK_COLOUR);
	case Status::Modified:	return Colour(HISE_WARNING_COLOUR);
	case Status::Conflict:	return Colour(HISE_ERROR_COLOUR);
	default:				return Colours::transparentBlack;
	}
}

String GitChangeTracker::getLetter(Status s)
{
	switch (s)
	{
	case Status::Untracked: return "U";
	case Status::Added:		return "A";
	case Status::Modified:	return "M";
	case Status::Conflict:	return "C";
	default:				return {};
	}
}

GitChangeTracker::GitOutput GitChangeTracker::runGit(const File& workingDirectory, const StringArray& args) const
{
	GitOutput r;

	// --no-optional-locks: the polling must not hold index.lock while another tool (or agent) runs git
	StringArray command;
	command.add("git");
	command.add("--no-optional-locks");
	command.add("-C");
	command.add(workingDirectory.getFullPathName());
	command.addArray(args);

	ChildProcess cp;

	if (!cp.start(command, ChildProcess::wantStdOut))
		return r;

	r.started = true;

	char buffer[8192];

	for (;;)
	{
		auto numRead = cp.readProcessOutput(buffer, sizeof(buffer));

		if (numRead <= 0)
			break;

		r.data.append(buffer, (size_t)numRead);
	}

	cp.waitForProcessToFinish(10000);
	r.exitCode = (int)cp.getExitCode();

	return r;
}

void GitChangeTracker::run()
{
	while (!threadShouldExit())
	{
		File directory;
		bool doStatus = false;
		Array<DiffJob> jobs;

		{
			ScopedLock sl(lock);
			doStatus = statusUpdatePending;
			statusUpdatePending = false;
			directory = requestedDirectory;
			jobs.swapWith(pendingDiffs);
		}

		if (doStatus && directory.isDirectory())
		{
			auto s = createSnapshot(directory);

			{
				ScopedLock sl(lock);
				snapshot = s;
				snapshotChanged = true;
			}

			triggerAsyncUpdate();
		}

		if (!jobs.isEmpty())
		{
			auto s = getSnapshot();
			Array<DiffResult> results;

			for (const auto& j : jobs)
			{
				if (threadShouldExit())
					return;

				results.add(createDiff(j, *s));
			}

			{
				ScopedLock sl(lock);
				finishedDiffs.addArray(results);
			}

			triggerAsyncUpdate();
		}

		if (!doStatus && jobs.isEmpty())
			wait(-1);
	}
}

GitChangeTracker::Snapshot::Ptr GitChangeTracker::createSnapshot(const File& projectDirectory)
{
	Snapshot::Ptr s = new Snapshot();

	auto top = runGit(projectDirectory, { "rev-parse", "--show-toplevel" });

	if (!top.started || top.exitCode != 0)
		return s;

	s->repoRoot = File(top.toString().trim().replaceCharacter('/', File::getSeparatorChar()));

	if (!s->repoRoot.isDirectory())
		return s;

	s->isRepository = true;

	auto head = runGit(s->repoRoot, { "rev-parse", "HEAD" });

	if (head.exitCode == 0)
		s->headCommit = head.toString().trim();

	BaseMode mode;
	String branch;
	String sessionStart;

	{
		ScopedLock sl(lock);

		if (sessionStartCommit.isEmpty())
			sessionStartCommit = s->headCommit;

		mode = baseMode;
		branch = baseBranch;
		sessionStart = sessionStartCommit;
	}

	s->baseCommit = s->headCommit;

	if (mode == BaseMode::SessionStart && sessionStart.isNotEmpty())
		s->baseCommit = sessionStart;
	else if (mode == BaseMode::BranchPoint && branch.isNotEmpty())
	{
		auto mb = runGit(s->repoRoot, GitChangeTrackerHelpers::args({ "merge-base", "HEAD", branch }));

		if (mb.exitCode == 0)
			s->baseCommit = mb.toString().trim();
	}

	auto refs = runGit(s->repoRoot, { "for-each-ref", "--format=%(refname:short)", "refs/heads" });

	if (refs.exitCode == 0)
	{
		s->branches = StringArray::fromLines(refs.toString());
		s->branches.removeEmptyStrings();
	}

	auto addStatus = [&](String relativePath, Status st)
	{
		auto isFolder = relativePath.endsWithChar('/');
		relativePath = relativePath.trimCharactersAtEnd("/");

		if (relativePath.isEmpty())
			return;

		auto f = s->repoRoot.getChildFile(relativePath.replaceCharacter('/', File::getSeparatorChar()));
		auto key = getKey(f);

		if (st == Status::Ignored)
		{
			if (isFolder)
				s->ignoredFolders.add(key);
			else
				s->fileStatus.set(key, (int)st);

			return;
		}

		auto prev = s->fileStatus.contains(key) ? s->fileStatus[key] : 0;

		if ((int)st > prev)
			s->fileStatus.set(key, (int)st);

		for (auto p = f.getParentDirectory(); p == s->repoRoot || p.isAChildOf(s->repoRoot); p = p.getParentDirectory())
		{
			auto pk = getKey(p);
			auto pv = s->folderStatus.contains(pk) ? s->folderStatus[pk] : 0;

			if ((int)st > pv)
				s->folderStatus.set(pk, (int)st);

			if (p == s->repoRoot)
				break;
		}
	};

	// Changes that were committed after the base commit (+ uncommitted changes of tracked files)
	if (s->baseCommit.isNotEmpty() && s->baseCommit != s->headCommit)
	{
		auto diff = runGit(s->repoRoot, GitChangeTrackerHelpers::args({ "diff", "--name-status", "-z", s->baseCommit }));

		if (diff.exitCode == 0)
		{
			auto entries = GitChangeTrackerHelpers::splitNullTerminated(diff.data);

			for (int i = 0; i < entries.size(); i++)
			{
				auto code = entries[i];

				if (code.isEmpty())
					continue;

				auto c = code[0];

				if (c == 'R' || c == 'C')
				{
					addStatus(entries[i + 2], Status::Added);
					i += 2;
					continue;
				}

				auto path = entries[++i];

				if (c == 'A')
					addStatus(path, Status::Added);
				else if (c == 'U')
					addStatus(path, Status::Conflict);
				else
					addStatus(path, Status::Modified);
			}
		}
	}

	auto status = runGit(s->repoRoot, { "status", "--porcelain=v1", "-z", "-uall", "--ignored=matching" });

	if (status.exitCode == 0)
	{
		auto entries = GitChangeTrackerHelpers::splitNullTerminated(status.data);

		for (int i = 0; i < entries.size(); i++)
		{
			auto e = entries[i];

			if (e.length() < 4)
				continue;

			auto x = e[0];
			auto y = e[1];
			auto path = e.substring(3);

			// renamed / copied entries are followed by the original path
			if (x == 'R' || x == 'C')
				i++;

			Status st;

			if (x == '?')
				st = Status::Untracked;
			else if (x == '!')
				st = Status::Ignored;
			else if (x == 'U' || y == 'U' || (x == 'A' && y == 'A') || (x == 'D' && y == 'D'))
				st = Status::Conflict;
			else if (x == 'A')
				st = Status::Added;
			else
				st = Status::Modified;

			addStatus(path, st);
		}
	}

	StringArray fileSet;

	for (HashMap<String, int>::Iterator it(s->fileStatus); it.next();)
	{
		auto st = (Status)it.getValue();
		File f(it.getKey());

		if (st == Status::Ignored)
			continue;

		if (f.existsAsFile())
			s->changedFiles[it.getKey()] = f.getLastModificationTime().toMilliseconds();

		if (st == Status::Untracked || st == Status::Added || !f.exists())
			fileSet.add(it.getKey());
	}

	fileSet.sort(false);
	s->fileSetHash = fileSet.joinIntoString("|").hashCode64();

	return s;
}

String GitChangeTracker::getBaseContent(const Snapshot& s, const File& f)
{
	if (s.baseCommit.isEmpty())
		return {};

	auto key = s.baseCommit + ":" + f.getRelativePathFrom(s.repoRoot).replaceCharacter('\\', '/');

	auto it = baseContentCache.find(key);

	if (it != baseContentCache.end())
		return it->second;

	auto output = runGit(s.repoRoot, GitChangeTrackerHelpers::args({ "show", key }));

	// a non-zero exit code means that the file does not exist in the base commit
	auto content = output.exitCode == 0 ? output.toString() : String();

	if (baseContentCache.size() > 256)
		baseContentCache.clear();

	baseContentCache[key] = content;
	return content;
}

GitChangeTracker::DiffResult GitChangeTracker::createDiff(const DiffJob& job, const Snapshot& s)
{
	DiffResult r;
	r.editor = job.editor;

	if (!s.isRepository || !job.file.isAChildOf(s.repoRoot))
		return r;

	auto st = getStatusFromSnapshot(s.fileStatus, s.folderStatus, s.ignoredFolders, getKey(job.file));

	if (st == Status::Ignored)
		return r;

	auto baseText = st == Status::Untracked ? String() : getBaseContent(s, job.file);

	// git stores LF, the document might use CRLF
	baseText = baseText.replace("\r\n", "\n");
	auto currentText = job.content.replace("\r\n", "\n");

	if (baseText == currentText)
		return r;

	StringArray a, b;

	if (baseText.isNotEmpty())
		a = StringArray::fromLines(baseText);

	if (currentText.isNotEmpty())
		b = StringArray::fromLines(currentText);

	LineDiff::HashedDiff diff(a, b);
	r.lines = LineDiff::Change::createChangeMap(diff.createHunks());

	return r;
}

GitChangeTracker::Snapshot::Ptr GitChangeTracker::getSnapshot() const
{
	ScopedLock sl(lock);
	return snapshot;
}

void GitChangeTracker::timerCallback()
{
	if (rootWindow == nullptr)
		return;

	tickCounter++;

	auto directory = GET_PROJECT_HANDLER(rootWindow->getMainSynthChain()).getWorkDirectory();
	auto isForeground = Process::isForegroundProcess();
	auto statusInterval = isForeground ? 4 : 20;

	// rescan at once when switching back from an external editor or the file explorer
	auto gotFocus = isForeground && !wasForeground;
	wasForeground = isForeground;

	{
		ScopedLock sl(lock);

		if (gotFocus)
			statusUpdatePending = true;

		if (directory != requestedDirectory)
		{
			requestedDirectory = directory;
			sessionStartCommit = {};
			statusUpdatePending = true;
		}

		if (tickCounter % statusInterval == 0)
			statusUpdatePending = true;
	}

	updateEditors();

	bool shouldNotify;

	{
		ScopedLock sl(lock);
		shouldNotify = statusUpdatePending || !pendingDiffs.isEmpty();
	}

	if (shouldNotify)
		notify();
}

void GitChangeTracker::handleAsyncUpdate()
{
	Array<DiffResult> results;
	bool changed;

	{
		ScopedLock sl(lock);
		results.swapWith(finishedDiffs);
		changed = snapshotChanged;
		snapshotChanged = false;
	}

#if HISE_USE_NEW_CODE_EDITOR
	for (const auto& r : results)
	{
		if (auto pe = r.editor.getComponent())
		{
			if (auto ed = pe->getEditor())
				ed->editor.setDiffLines(r.lines);
		}
	}
#endif

	if (changed)
		sendChangeMessage();
}

void GitChangeTracker::updateEditors()
{
	auto s = getSnapshot();

	for (int i = editorStates.size() - 1; i >= 0; i--)
	{
		if (editorStates[i].editor == nullptr)
			editorStates.remove(i);
	}

	Array<DiffJob> jobs;

	Component::callRecursive<PopupIncludeEditor>(rootWindow.getComponent(), [&](PopupIncludeEditor* pe)
	{
		auto f = pe->getFile();

		if (pe->externalFile == nullptr || !f.existsAsFile())
			return false;

		auto content = pe->externalFile->getFileDocument().getAllContent();
		auto st = s->isRepository ? getStatusFromSnapshot(s->fileStatus, s->folderStatus, s->ignoredFolders, getKey(f)) : Status::Clean;
		auto hash = content.hashCode64() ^ s->baseCommit.hashCode64() ^ ((int64)st << 48);

		EditorState* state = nullptr;

		for (auto& es : editorStates)
		{
			if (es.editor.getComponent() == pe)
				state = &es;
		}

		if (state == nullptr)
		{
			editorStates.add({ pe, 0 });
			state = &editorStates.getReference(editorStates.size() - 1);
		}

		if (state->lastHash != hash)
		{
			state->lastHash = hash;
			jobs.add({ pe, f, content, hash });
		}

		return false;
	});

	if (!jobs.isEmpty())
	{
		ScopedLock sl(lock);
		pendingDiffs.addArray(jobs);
	}
}

} // namespace hise
