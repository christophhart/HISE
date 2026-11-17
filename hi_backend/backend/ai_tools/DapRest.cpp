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

struct BackendProcessor::DapRestState
{
	using Route = RestHelpers::ApiRoute;
	using Change = UriBreakpoint::Owner::UriBreakpointListener::ChangeType;
	using Attach = UriBreakpoint::Owner::AttachState;

	struct Session : public UriBreakpoint::Owner::UriBreakpointListener, 
	                 public UriErrorLogger::Listener
	{
		explicit Session(const String& id) : sessionId(id)
		{
		}

		~Session() override
		{
			unbind();
		}

		void unbind()
		{
			if (auto p = processor.get())
			{
				p->removeUriBreakpointListener(this);
				p->errorProvider.removeListener(this);
			}
			processor = nullptr;
		}

		void bind(JavascriptProcessor* p)
		{
			processor = p;
			moduleId = dynamic_cast<Processor*>(p)->getId();
			p->addUriBreakpointListener(this);
			p->errorProvider.addListener(this, false);
		}

		bool matchesUriLocation(const UriLocation&) const override
		{
			return true;
		}

		bool matchesUriBreakpointEvent(const UriError&) const override
		{
			return true;
		}

		struct Entry
		{
			int id = 0;
			UriBreakpoint breakpoint;
			Attach attach = Attach::Unresolved;
		};

		struct Event
		{
			int64 sequence;
			int stoppedGeneration = 0;
			String reason;
			String type;
			UriError error;
			Entry breakpoint;
			String change;
		};

		int getBreakpointIndex(const UriBreakpoint& bp) const
		{
			for (int i = 0; i < entries.size(); ++i)
				if (entries.getReference(i).breakpoint.matches(bp))
					return i;
			return -1;
		}

		void append(Event e)
		{
			e.sequence = ++sequence;
			journal.add(e);
			
			if (journal.size() > 512)
				journal.remove(0);
		}

		void onUriBreakpointChange(const UriBreakpoint& bp, Change change) override
		{
			const ScopedLock sl(lock);
			auto index = getBreakpointIndex(bp);
			if (change == Change::Removed)
			{
				if (index >= 0)
				{
					Event e;
					e.type = "breakpoint";
					e.change = "removed";
					e.breakpoint = entries.getReference(index);
					append(e);
					entries.remove(index);
				}

				return;
			}
			if (index < 0)
			{
				Entry e;
				e.id = ++nextBreakpointId;
				e.breakpoint = bp;
				entries.add(e);
				index = entries.size() - 1;
			}
			else
				entries.getReference(index).breakpoint = bp;

			Event e;
			e.type = "breakpoint";
			e.change = change == Change::Added ? "new" : "changed";
			e.breakpoint = entries.getReference(index);
			append(e);
		}

		void onUriBreakpointAttachTypeChange(const UriBreakpoint& bp, Attach state) override
		{
			const ScopedLock sl(lock);
			auto i = getBreakpointIndex(bp);
			if (i < 0 || entries.getReference(i).attach == state)
				return;
			entries.getReference(i).attach = state;

			Event e;
			e.type = "breakpoint";
			e.change = "changed";
			e.breakpoint = entries.getReference(i);
			append(e);
		}

		void onUriBreakpointClear() override
		{
			const ScopedLock sl(lock);
			for (auto& b : entries)
			{
				Event e;
				e.type = "breakpoint";
				e.change = "removed";
				e.breakpoint = b;
				append(e);
			}

			entries.clear();
		}

		void onUriBreakpointEvent(const UriBreakpoint&, const UriError& event) override
		{
			if (!event.wasOk())
			{
				if (event.errorType != UriError::Breakpoint)
					return;

				// Registration also delivers an existing hit. The sleeper is already active
				// for both that snapshot and a newly emitted stop event.
				auto p = processor.get();
				if (p == nullptr || !dynamic_cast<Processor*>(p)->getMainController()
					->getJavascriptThreadPool().isCurrentlySleeping())
					return;

				const ScopedLock sl(lock);
				if (stopped)
					return;

				stopped = true;
				stop = event;
				++generation;

				Event e;
				e.type = "stopped";
				e.error = event;
				e.stoppedGeneration = generation;
				e.reason = stepping ? "step" : "breakpoint";
				stepping = false;
				append(e);
				return;
			}

			const ScopedLock sl(lock);
			if (stopped)
			{
				stopped = false;
				Event e;
				e.type = "continued";
				append(e);
			}
		}

		void onError(const UriError& error, int) override
		{
			if (error.sourceType != UriError::SourceType::Debug || error.errorType != UriError::Info)
				return;

			const ScopedLock sl(lock);
			Event e;
			e.type = "output";
			e.error = error;
			append(e);
		}

		void onClear(UriError::SourceType) override
		{
		}

		String sessionId, moduleId;
		WeakReference<JavascriptProcessor> processor;
		CriticalSection lock;
		Array<Entry> entries;
		Array<Event> journal;
		std::map<String, Array<UriBreakpoint>> submitted;
		UriError stop;
		int generation = 0, nextBreakpointId = 0;
		int64 sequence = 0;
		bool stopped = false, stepping = false;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Session);
	};

	void clear()
	{
		sessions.clear();
	}

	static String getUriForLocation(LspRestState& lsp, MainController* mc, JavascriptProcessor* jp,
		const UriLocation& loc, int& line, int& column)
	{
		auto source = lsp.resolveLocation(mc, jp, loc);
		
		if (source.index < 0)
			return {};

		auto pos = JavascriptProcessor::MergedCallbackDocument::toLspPosition(source.text, source.index);
		line = pos.first + 1;
		column = pos.second + 1;
		return source.uri;
	}

	static var getLocation(LspRestState& lsp, MainController* mc, JavascriptProcessor* jp,
		const UriLocation& loc)
	{
		int line = 0, column = 0;
		const auto uri = getUriForLocation(lsp, mc, jp, loc, line, column);
		
		if (uri.isEmpty())
			return {};

		auto o = new DynamicObject();
		o->setProperty("uri", uri);
		o->setProperty("line", line);
		o->setProperty("column", column);
		return var(o);
	}

	static var getBreakpoint(LspRestState& lsp, MainController* mc, JavascriptProcessor* jp,
		const Session::Entry& e)
	{
		auto o = new DynamicObject();
		o->setProperty("breakpointId", e.id);
		o->setProperty("verified", e.attach == Attach::OK || e.attach == Attach::Duplicate);
		
		const char* names[] = { "OK", "Duplicate", "Unresolved", "Stale" };
		o->setProperty("attachState", names[(int)e.attach]);
		o->setProperty("condition", e.breakpoint.expression);
		auto loc = getLocation(lsp, mc, jp, e.breakpoint);
		
		if (loc.isObject())
		{
			o->setProperty("sourceUri", loc["uri"]);
			o->setProperty("line", loc["line"]);
			o->setProperty("column", loc["column"]);
		}
		else
			o->setProperty("message", "Source location cannot be mapped");
		return var(o);
	}

	static var getStack(LspRestState& lsp, MainController* mc, JavascriptProcessor* jp, const UriError& stop)
	{
		Array<var> frames;
		auto add = [&](const UriLocation& loc, const String& label)
		{
				auto position = getLocation(lsp, mc, jp, loc);
			if (!position.isObject())
				return;

			auto f = new DynamicObject();
			f->setProperty("frameId", frames.size() + 1);
			f->setProperty("name", label.isNotEmpty() ? label : loc.uri);
			f->setProperty("uri", position["uri"]);
			f->setProperty("line", position["line"]);
			f->setProperty("column", position["column"]);
			frames.add(var(f));
		};

		add(stop, stop.uri);
		for (const auto& frame : stop.callstack)
			add(frame.second, frame.first);

		return var(frames);
	}

    File project;
	std::map<String, std::unique_ptr<Session>> sessions;
};

BackendProcessor::DapRestState& BackendProcessor::getDapRestState()
{
	if (!dapRestState)
		dapRestState = std::make_unique<DapRestState>();
	return *dapRestState;
}

RestServer::Response RestHelpers::handleDap(BackendProcessor* bp,
	RestServer::AsyncRequest::Ptr req, ApiRoute route)
{
	using State = BackendProcessor::DapRestState;
	auto mc = static_cast<MainController*>(bp);
	auto& registry = bp->getDapRestState();
	auto& lsp = bp->getLspRestState();
	const auto project = mc->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Scripts);
	if (registry.project != project)
	{
		registry.clear();
		registry.project = project;
	}
	const bool get = req->getRequest().method == RestServer::Method::Get;
	const auto body = get ? var() : req->getRequest().getJsonBody();
	const String id = get ? req->getRequest()[Identifier("sessionId")] : body["sessionId"].toString();
	if (id.isEmpty())
		return req->fail(400, "sessionId is required");

	auto result = new DynamicObject();
	result->setProperty("success", true);
	result->setProperty("sessionId", id);
	result->setProperty("threadId", 1);

	auto resolveSource = [&](const String& source, JavascriptProcessor* required,
		LspRestState::Document& doc, String& error) -> JavascriptProcessor*
	{
		if (!source.startsWithIgnoreCase("file:///"))
		{
			error = "Expected an absolute local file URI";
			return nullptr;
		}

		const URL url(source);
		bool hostless = source.startsWithIgnoreCase("file:///");
#if JUCE_WINDOWS
		hostless = hostless && CharacterFunctions::isLetter(source[8]) &&
			((source[9] == ':' && source[10] == '/') ||
			 (source.substring(9, 12).equalsIgnoreCase("%3A") && source[12] == '/'));
#else
		hostless = hostless && source[8] != '/';
#endif
		if (!url.isLocalFile() || !hostless || source.containsChar('?') || source.containsChar('#'))
		{
			error = "Invalid source URI";
			return nullptr;
		}

		for (const auto& segment : StringArray::fromTokens(url.getSubPath(), "/", ""))
		{
			if (URL::removeEscapeChars(segment) == "..")
			{
				error = "Path traversal is not permitted";
				return nullptr;
			}
		}

		const auto file = url.getLocalFile();
		if (!File::isAbsolutePath(file.getFullPathName()) || !file.hasFileExtension(".js"))
		{
			error = "Expected an absolute JavaScript file";
			return nullptr;
		}

		if (!LspRestState::resolveDocument(mc, file, doc, error))
			return nullptr;

		if (doc.callbacks)
		{
			auto p = doc.processor.get();
			if (required && p != required)
			{
				error = "Source belongs to another processor";
				return nullptr;
			}

			return p;
		}

		for (auto p : LspRestState::contexts(mc, doc))
		{
			if (!required || required == p)
				return p;
		}
		error = "Processor does not watch this source";
		return nullptr;
	};

	if (route == ApiRoute::DapSessionOpen)
	{
		if (registry.sessions.count(id))
			return req->fail(409, "Session already exists");
		JavascriptProcessor* p = nullptr;
		const auto module = body["moduleId"].toString();
		const auto source = body["sourceUri"].toString();
		if (module.isNotEmpty())
		{
			p = dynamic_cast<JavascriptProcessor*>(ProcessorHelpers::getFirstProcessorWithName(
				mc->getMainSynthChain(), module));
			if (!p)
				return req->fail(404, "Unknown script processor");
		}
		if (source.isNotEmpty())
		{
			LspRestState::Document doc;
			String error;
			auto selected = resolveSource(source, p, doc, error);
			if (!selected)
				return req->fail(400, error);
			p = selected;
		}
		if (p)
		{
			for (const auto& existing : registry.sessions)
				if (existing.second->processor.get() == p)
					return req->fail(409, "Processor already has a debugger controller");
		}

		auto session = std::make_unique<State::Session>(id);
		if (p)
			session->bind(p);
		result->setProperty("moduleId", p ? dynamic_cast<Processor*>(p)->getId() : String());
		registry.sessions.emplace(id, std::move(session));
		return RestServer::Response::ok(var(result));
	}

	auto it = registry.sessions.find(id);
	if (it == registry.sessions.end())
		return req->fail(409, "Stale DAP session");
	auto& s = *it->second;
	if (route == ApiRoute::DapSessionClose)
	{
		registry.sessions.erase(it);
		return RestServer::Response::ok(var(result));
	}
	auto p = s.processor.get();
	if (!p && route != ApiRoute::DapBreakpointsSet && route != ApiRoute::DapState &&
		route != ApiRoute::DapEvents && route != ApiRoute::DapBreakpointsList)
		return req->fail(409, "Session is not bound to a processor");
	result->setProperty("moduleId", s.moduleId);

	if (route == ApiRoute::DapBreakpointsSet)
	{
		const auto source = body["sourceUri"].toString();
		if (source.isEmpty() || !body["breakpoints"].isArray())
			return req->fail(400, "sourceUri and breakpoints array are required");
		LspRestState::Document doc;
		String error;
		auto selected = resolveSource(source, p, doc, error);
		if (!selected)
			return req->fail(p ? 409 : 400, error);
		if (!p)
		{
			for (const auto& other : registry.sessions)
				if (other.second.get() != &s && other.second->processor.get() == selected)
					return req->fail(409, "Processor already has a debugger controller");
			s.bind(selected);
			p = selected;
		}
		if (doc.callbacks && !p->ensureMergedCallbackMapping())
			return req->fail(409, "Merged callback source cannot be mapped");
		const auto& merged = p->getMergedCallbackDocument();
		String text;
		if (doc.callbacks)
			text = merged.text;
		else
		{
			for (int i = 0; i < p->getNumWatchedFiles(); ++i)
				if (p->getWatchedFile(i) == doc.file)
					text = p->getWatchedFileDocument(i).getAllContent();
		}
		Array<UriBreakpoint> requested;
		for (const auto& item : *body["breakpoints"].getArray())
		{
			if (!item.isObject() || !item["line"].isInt() || (int)item["line"] < 1 ||
				(item.hasProperty("column") && (!item["column"].isInt() || (int)item["column"] < 1)) ||
				(item.hasProperty("condition") && !item["condition"].isString()) ||
				item.hasProperty("hitCondition") || item.hasProperty("logMessage"))
				return req->fail(400, "Invalid or unsupported breakpoint fields");
		}
		Array<var> responses;
		for (const auto& item : *body["breakpoints"].getArray())
		{
			const int index = JavascriptProcessor::MergedCallbackDocument::toCharacterIndex(
				text, (int)item["line"] - 1, item.hasProperty("column") ? (int)item["column"] - 1 : 0);
			UriBreakpoint bp;
			bp.processorId = p->getUriProcessorId();
			bp.expression = item.hasProperty("condition") ? item["condition"].toString() : "true";
			if (index >= 0)
			{
				if (doc.callbacks)
				{
					const int snippet = merged.getSnippetIndex(index);
					if (snippet >= 0 && snippet < p->getNumSnippets())
					{
						bp.uri = p->getSnippet(snippet)->getCallbackName().toString() + "()";
						bp.charIndex = merged.toSnippetIndex(snippet, index);
					}
				}
				else
				{
					bp.uri = doc.file.getRelativePathFrom(project);
					bp.charIndex = index;
				}
			}
			auto response = new DynamicObject();
			response->setProperty("line", item["line"]);
			if (item.hasProperty("column"))
				response->setProperty("column", item["column"]);
			if (bp.charIndex < 0)
			{
				response->setProperty("verified", false);
				response->setProperty("attachState", "Unresolved");
				response->setProperty("message", "Location is outside a mapped source");
			}
			else
			{
				auto attached = p->addUriBreakpoint(bp, sendNotificationSync);
				// Attachment can relocate a line breakpoint to a statement. Keep the
				// resolved identity for later omission/removal, not the line start.
				requested.add(attached.second);
				if (attached.first == State::Attach::Duplicate && attached.second.expression != bp.expression)
				{
					auto modified = attached.second;
					modified.expression = bp.expression;
					p->modifyUriBreakpoint(modified, sendNotificationSync);
				}
				const ScopedLock sl(s.lock);
				const auto i = s.getBreakpointIndex(attached.second);
				if (i >= 0)
				{
					const auto data = State::getBreakpoint(lsp, mc, p, s.entries.getReference(i));
					for (const auto& key : { "breakpointId", "verified", "attachState", "line", "column", "message" })
						if (data.hasProperty(key)) response->setProperty(key, data[key]);
				}
			}
			responses.add(var(response));
		}
		for (const auto& old : s.submitted[source])
		{
			bool found = false;
			for (const auto& current : requested)
				found |= old.matches(current);

			if (!found)
				p->removeUriBreakpoint(old, sendNotificationSync);
		}
		s.submitted[source] = requested;
		result->setProperty("moduleId", s.moduleId);
		result->setProperty("sourceUri", source);
		result->setProperty("breakpoints", var(responses));
	}
	else if (route == ApiRoute::DapBreakpointsList)
	{
		Array<var> list;
		Array<State::Session::Entry> snapshot;
		{
			const ScopedLock sl(s.lock);
			snapshot = s.entries;
		}
		if (p)
		{
			for (const auto& entry : snapshot)
				list.add(State::getBreakpoint(lsp, mc, p, entry));
		}
		result->setProperty("breakpoints", var(list));
	}
	else if (route == ApiRoute::DapEvents)
	{
		const auto cursor = req->getRequest()[Identifier("afterSequence")];
		const int64 after = cursor.isEmpty() ? 0 : cursor.getLargeIntValue();
		if (after < 0 || (cursor.isNotEmpty() && !cursor.containsOnly("0123456789")))
			return req->fail(400, "Invalid afterSequence");

		Array<var> events;
		Array<State::Session::Event> snapshot;
		int64 latest = 0;
		{
			const ScopedLock sl(s.lock);
			if (s.journal.size() && after < s.journal.getFirst().sequence - 1)
				return req->fail(409, "DAP event journal overflow; resnapshot state and breakpoints");
			snapshot = s.journal;
			latest = s.sequence;
		}

		for (const auto& e : snapshot)
		{
			if (e.sequence <= after)
				continue;

			auto obj = new DynamicObject();
			obj->setProperty("sequence", e.sequence);
			obj->setProperty("type", e.type);
			if (e.type == "breakpoint" && p)
			{
				obj->setProperty("reason", e.change);
				obj->setProperty("breakpoint", State::getBreakpoint(lsp, mc, p, e.breakpoint));
			}
			else if (e.type == "output")
			{
				obj->setProperty("category", "console");
				obj->setProperty("output", e.error.errorMessage);
				if (p)
					obj->setProperty("location", State::getLocation(lsp, mc, p, e.error));
			}
			else if (e.type == "stopped" && p)
			{
				obj->setProperty("reason", e.reason);
				obj->setProperty("threadId", 1);
				obj->setProperty("stoppedGeneration", e.stoppedGeneration);
				obj->setProperty("allThreadsStopped", true);
				obj->setProperty("location", State::getLocation(lsp, mc, p, e.error));
				obj->setProperty("callstack", State::getStack(lsp, mc, p, e.error));
			}
			events.add(var(obj));
		}

		result->setProperty("events", var(events));
		result->setProperty("nextSequence", latest);
	}
	else if (route == ApiRoute::DapState)
	{
		UriError stop;
		bool stopped;
		{
			const ScopedLock sl(s.lock);
			stopped = s.stopped;
			stop = s.stop;
			result->setProperty("latestSequence", s.sequence);
			result->setProperty("stoppedGeneration", s.generation);
		}

		result->setProperty("state", !p ? "unbound" : stopped ? "stopped" : "running");
		if (p && stopped)
		{
			result->setProperty("location", State::getLocation(lsp, mc, p, stop));
			result->setProperty("callstack", State::getStack(lsp, mc, p, stop));
		}
	}
	else if (route == ApiRoute::DapContinue || route == ApiRoute::DapStep ||
			 route == ApiRoute::DapStackTrace || route == ApiRoute::DapScopes ||
			 route == ApiRoute::DapVariables || route == ApiRoute::DapEvaluate)
	{
		if (!body["stoppedGeneration"].isInt() || !p)
			return req->fail(409, "Missing or stale stopped generation");

		{
			const ScopedLock sl(s.lock);
			if (!s.stopped || (int)body["stoppedGeneration"] != s.generation)
				return req->fail(409, "Stale stopped generation");
		}

		if (route == ApiRoute::DapStackTrace)
			result->setProperty("frames", State::getStack(lsp, mc, p, s.stop));
		else if (route == ApiRoute::DapContinue || route == ApiRoute::DapStep)
		{
			if (!body["threadId"].isInt() || (int)body["threadId"] != 1)
				return req->fail(400, "Invalid threadId");

			auto action = UriBreakpoint::Owner::NextBreakpointAction::Resume;
			if (route == ApiRoute::DapStep)
			{
				const auto kind = body["kind"].toString();
				if (kind == "next")
					action = UriBreakpoint::Owner::NextBreakpointAction::StepOver;
				else if (kind == "stepIn")
					action = UriBreakpoint::Owner::NextBreakpointAction::StepInto;
				else if (kind == "stepOut")
					action = UriBreakpoint::Owner::NextBreakpointAction::StepOut;
				else
					return req->fail(400, "Unsupported step kind");
			}
			{
				const ScopedLock sl(s.lock);
				s.stopped = false;
				s.stepping = action != UriBreakpoint::Owner::NextBreakpointAction::Resume;
			}
			p->resumeExecution(action);
			result->setProperty("accepted", true);
			result->setProperty("allThreadsContinued", true);
		}
		else if (route == ApiRoute::DapEvaluate)
		{
			const auto expression = body["expression"].toString();
			const auto context = body["context"].toString();
			if (expression.isEmpty() || (context != "repl" && context != "hover"))
				return req->fail(400, "Expected expression and repl or hover context");

			struct Evaluation
			{
				WaitableEvent done;
				std::atomic<bool> cancelled { false };
				var value;
				Result status = Result::ok();
			};

			auto evaluation = std::make_shared<Evaluation>();
			const int generation = (int)body["stoppedGeneration"];
			mc->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::ReplEvaluation, p,
				[evaluation, expression](JavascriptProcessor* jp)
				{
					if (evaluation->cancelled)
					{
						evaluation->done.signal();
						return UriError::ok();
					}

					evaluation->value = jp->getScriptEngine()->evaluate(expression, &evaluation->status);
					evaluation->done.signal();
					return UriError::ok();
				});
			if (!evaluation->done.wait(2000))
			{
				evaluation->cancelled = true;
				return req->fail(409, "Paused evaluation timed out");
			}
			{
				const ScopedLock sl(s.lock);
				if (!s.stopped || s.generation != generation)
					return req->fail(409, "Execution resumed during evaluation");
			}
			result->setProperty("result", evaluation->status.wasOk() ?
				evaluation->value.toString() : evaluation->status.getErrorMessage());
			result->setProperty("variablesReference", 0);
			if (evaluation->status.failed())
				result->setProperty("error", evaluation->status.getErrorMessage());
		}
		else
			return req->fail(501, "Paused-scope inspection is not implemented");
	}

	return RestServer::Response::ok(var(result));
}

} // namespace hise
