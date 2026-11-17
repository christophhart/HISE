/* HISE-side, full-document REST bridge for LSP. Included in hi_backend.cpp. */

namespace hise { using namespace juce;

/** Backend-owned LSP document leases, source mapping, and language analysis. */
struct LspRestState : private Timer
{
	struct Document
	{
		File file;
		String uri, sessionId, text;
		var lastDiagnostics;
		int version = -1;
		int epoch = 0;
		int64 lastActivity = 0;
		bool callbacks = false;
		WeakReference<JavascriptProcessor> processor;
		ExternalScriptFile::Ptr included;
	};

	struct ResolvedSource
	{
		String uri;
		String text;
		int index = -1;
	};

	using Diagnostics = std::map<String, std::map<String, Array<var>>>;

	~LspRestState() { clear(); }

	void startLeaseTimer()
	{
		if (!isTimerRunning())
			startTimer(10000);
	}

	void timerCallback() override
	{
		const auto now = Time::currentTimeMillis();
		for (auto it = documents.begin(); it != documents.end();)
		{
			if (now - it->second.lastActivity <= 5 * 60 * 1000)
			{
				++it;
				continue;
			}

			release(it->second);
			it = documents.erase(it);
		}
	}

	static void release(Document& d)
	{
		if (d.callbacks)
		{
			if (auto jp = d.processor.get())
				jp->setMergedCallbackFileOpen(false);
		}
		else if (d.included != nullptr)
			d.included->setLspDocumentOpen(false);
	}

	void clear()
	{
		for (auto& pair : documents)
			release(pair.second);
		documents.clear();
		publishedUris.clear();
	}

	String fileUri(const File& f) const;
	static bool readPosition(const var& body, int& line, int& column);
	static var lspPosition(const String& text, int index);
	static var lspRange(const String& text, int start, int end);
	ResolvedSource resolveLocation(MainController* mc, JavascriptProcessor* jp, const UriLocation& loc);
	var locationObject(MainController* mc, JavascriptProcessor* jp, const UriLocation& loc, int length);
	static bool resolveDocument(MainController* mc, const File& file, Document& result, String& error);
	static Array<JavascriptProcessor*> contexts(MainController* mc, const Document& document);
	void analyze(MainController* mc, Document& document, Diagnostics& results, StringArray& analyzed);
	static var diagnosticsObject(const Diagnostics& results);
	static void mapCompletionEdit(JavascriptProcessor* jp, const Document& document,
		const UriLocation& location, const String& snippet, DynamicObject::Ptr item);
	static bool readQuery(const var& body, const Document& document,
		JavascriptProcessor* jp, UriLocation& location, String& snippet, int& line, int& column);

	File project;
	int nextEpoch = 0;
	std::map<String, Document> documents;
	std::map<String, StringArray> publishedUris;
};

String LspRestState::fileUri(const File& f) const
{
	if (auto it = documents.find(f.getFullPathName()); it != documents.end())
		return it->second.uri; // Preserve the open editor's exact URI spelling.

	auto uri = URL(f).toString(false);
#if JUCE_WINDOWS
	// JUCE escapes the Windows drive colon even though LSP file URIs use D:/.
	if (uri.startsWithIgnoreCase("file:///") && CharacterFunctions::isLetter(uri[8]) &&
		uri.substring(9, 12).equalsIgnoreCase("%3A"))
		uri = uri.replaceSection(9, 3, ":");
#endif
	return uri;
}

bool LspRestState::readPosition(const var& body, int& line, int& column)
{
	auto p = body["position"];
	if (!p.isObject() || !p["line"].isInt() || !p["character"].isInt())
		return false;

	line = (int)p["line"];
	column = (int)p["character"];
	return line >= 0 && column >= 0;
}

var LspRestState::lspPosition(const String& text, int index)
{
	auto position = JavascriptProcessor::MergedCallbackDocument::toLspPosition(text, index);
	if (position.first < 0)
		return {};

	auto p = new DynamicObject();
	p->setProperty("line", position.first);
	p->setProperty("character", position.second);
	return var(p);
}

var LspRestState::lspRange(const String& text, int start, int end)
{
	if (start < 0 || end < start || end > text.length())
		return {};

	auto r = new DynamicObject();
	r->setProperty("start", lspPosition(text, start));
	r->setProperty("end", lspPosition(text, end));
	return var(r);
}

LspRestState::ResolvedSource LspRestState::resolveLocation(
	MainController* mc, JavascriptProcessor* jp, const UriLocation& loc)
{
	ResolvedSource out;
	if (loc.processorId.isNotEmpty() && loc.processorId != jp->getUriProcessorId())
		return out;

	if (loc.getUriType() == UriLocation::UriType::ExternalFile)
	{
		auto root = mc->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Scripts);
		const auto file = loc.getFile(root);
		out.uri = fileUri(file);
		for (int i = 0; i < jp->getNumWatchedFiles(); ++i)
			if (jp->getWatchedFile(i) == file)
			{
				out.text = jp->getWatchedFileDocument(i).getAllContent();
				break;
			}
		if (loc.charIndex >= 0)
			out.index = loc.charIndex;
		else if (loc.line > 0 && loc.col > 0)
			out.index = JavascriptProcessor::MergedCallbackDocument::toCharacterIndex(
				out.text, loc.line - 1, loc.col - 1);
		return out;
	}

	if (!jp->ensureMergedCallbackMapping())
		return out;

	const auto& merged = jp->getMergedCallbackDocument();
	const auto callback = loc.toLocationString(UriLocation::UriFormat::CallbackId);
	for (int i = 0; i < jp->getNumSnippets(); ++i)
	{
		if (jp->getSnippet(i)->getCallbackName().toString() != callback || i >= merged.snippets.size())
			continue;

		int offset = loc.charIndex;
		if (offset < 0 && loc.line > 0 && loc.col > 0)
			offset = JavascriptProcessor::MergedCallbackDocument::toCharacterIndex(
				merged.snippets[i], loc.line - 1, loc.col - 1);
		out.index = merged.toFileIndex(i, offset);
		if (out.index >= 0)
		{
			out.uri = fileUri(jp->getMergedCallbackFile());
			out.text = merged.text;
		}
		break;
	}
	return out;
}

var LspRestState::locationObject(MainController* mc, JavascriptProcessor* jp, const UriLocation& loc, int length)
{
	auto source = resolveLocation(mc, jp, loc);
	if (source.uri.isEmpty() || source.index < 0)
		return {};

	const int end = jmin(source.index + jmax(1, length), source.text.length());
	auto r = lspRange(source.text, source.index, end);
	if (r.isVoid())
		return {};

	auto obj = new DynamicObject();
	obj->setProperty("uri", source.uri);
	obj->setProperty("range", r);
	return var(obj);
}

bool LspRestState::resolveDocument(MainController* mc, const File& file, Document& result, String& error)
{
	JavascriptProcessor* callbackProcessor = nullptr;
	bool watchedInclude = false;
	Processor::Iterator<JavascriptProcessor> iter(mc->getMainSynthChain());
	while (auto jp = iter.getNextProcessor())
	{
		if (jp->getMergedCallbackFile() == file)
		{
			if (callbackProcessor != nullptr)
			{
				error = "More than one processor resolves to this merged callback file";
				return false;
			}
			callbackProcessor = jp;
		}
		for (int i = 0; i < jp->getNumWatchedFiles(); ++i)
			watchedInclude |= jp->getWatchedFile(i) == file;
	}

	if (callbackProcessor != nullptr)
	{
		result.callbacks = true;
		result.processor = callbackProcessor;
	}
	else
	{
		const auto scripts = mc->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Scripts);
		if (!file.isAChildOf(scripts) || !file.hasFileExtension(".js") ||
			(file.isAChildOf(scripts.getChildFile("ScriptProcessors")) && !watchedInclude))
		{
			error = "Not a script file in the active project or a connected callback file";
			return false;
		}
		for (auto current = file; current != scripts; current = current.getParentDirectory())
		{
			if (current.isSymbolicLink())
			{
				error = "Symbolic links cannot escape the active project's script directory";
				return false;
			}
		}
		result.included = mc->getExternalScriptFile(file, true);
	}

	result.file = file;
	return true;
}

Array<JavascriptProcessor*> LspRestState::contexts(MainController* mc, const Document& document)
{
	Array<JavascriptProcessor*> list;
	if (document.callbacks)
	{
		if (auto jp = document.processor.get())
			list.add(jp);
		return list;
	}

	Processor::Iterator<JavascriptProcessor> iter(mc->getMainSynthChain());
	while (auto jp = iter.getNextProcessor())
		for (int i = 0; i < jp->getNumWatchedFiles(); ++i)
			if (jp->getWatchedFile(i) == document.file)
			{
				list.add(jp);
				break;
			}
	return list;
}

void LspRestState::analyze(MainController* mc, Document& document,
	Diagnostics& results, StringArray& analyzed)
{
	results[document.uri];
	for (auto jp : contexts(mc, document))
	{
		if (!jp->isShadowParseReady())
			continue;

		const auto id = jp->getUriProcessorId();
		analyzed.addIfNotAlreadyThere(id);
		for (const auto& previous : publishedUris[id])
			results[previous][id] = {};

		StringArray currentUris;
		for (auto& entry : documents)
		{
			if (contexts(mc, entry.second).contains(jp))
			{
				results[entry.second.uri][id] = {};
				currentUris.addIfNotAlreadyThere(entry.second.uri);
				if (&entry.second != &document)
					entry.second.lastDiagnostics = var(); // Another document changed this processor's scope tree.
			}
		}

		auto sd = jp->createFullShadowParseData();
		if (sd.options.includeTree == nullptr || sd.codeToParse.empty())
		{
			publishedUris[id] = currentUris;
			continue;
		}

		jp->shadowParse(sd, [&](const JavascriptProcessor::DiagnosticList& errors)
		{
			for (const auto& error : errors)
			{
				auto source = resolveLocation(mc, jp, error);
				if (source.uri.isEmpty() || source.index < 0)
					continue;

				auto range = lspRange(source.text, source.index,
					jmin(source.index + 1, source.text.length()));
				if (range.isVoid())
					continue;

				auto diag = new DynamicObject();
				diag->setProperty("range", range);
				diag->setProperty("severity", error.errorType == UriError::Warning ? 2 :
					error.errorType == UriError::Info ? 3 : error.errorType == UriError::Hint ? 4 : 1);
				diag->setProperty("source", "hise");
				diag->setProperty("message", error.errorMessage);
				if (documents.find(URL(source.uri).getLocalFile().getFullPathName()) != documents.end())
				{
					results[source.uri][id].add(var(diag));
					currentUris.addIfNotAlreadyThere(source.uri);
				}
			}
		}, sendNotificationSync);
		publishedUris[id] = currentUris;
	}
}

var LspRestState::diagnosticsObject(const Diagnostics& results)
{
	Array<var> grouped;
	for (const auto& uri : results)
	{
		Array<var> processors;
		for (const auto& processor : uri.second)
		{
			auto entry = new DynamicObject();
			entry->setProperty("moduleId", processor.first);
			entry->setProperty("diagnostics", var(processor.second));
			processors.add(var(entry));
		}
		auto entry = new DynamicObject();
		entry->setProperty("uri", uri.first);
		entry->setProperty("processors", var(processors));
		grouped.add(var(entry));
	}
	return var(grouped);
}

void LspRestState::mapCompletionEdit(JavascriptProcessor* jp, const Document& document,
	const UriLocation& location, const String& snippet, DynamicObject::Ptr item)
{
	auto edit = item->getProperty("textEdit");
	if (!edit.isObject())
		return;

	// ExpressionParser emits CodeDocument (code-point) columns, not LSP UTF-16 columns.
	CodeDocument source;
	source.replaceAllContent(snippet);
	auto range = edit["range"];
	auto start = range["start"];
	auto end = range["end"];
	const int startLine = (int)start["line"], startColumn = (int)start["character"];
	const int endLine = (int)end["line"], endColumn = (int)end["character"];
	if (startLine < 0 || startColumn < 0 || endLine < 0 || endColumn < 0)
	{
		item->removeProperty("textEdit");
		return;
	}

	CodeDocument::Position a(source, startLine, startColumn);
	CodeDocument::Position b(source, endLine, endColumn);
	if (a.getLineNumber() != startLine || a.getIndexInLine() != startColumn ||
		b.getLineNumber() != endLine || b.getIndexInLine() != endColumn)
	{
		item->removeProperty("textEdit");
		return;
	}

	int first = a.getPosition(), last = b.getPosition();
	String fileText = snippet;
	if (document.callbacks)
	{
		const auto& merged = jp->getMergedCallbackDocument();
		const auto callback = location.toLocationString(UriLocation::UriFormat::CallbackId);
		for (int i = 0; i < jp->getNumSnippets(); ++i)
			if (jp->getSnippet(i)->getCallbackName().toString() == callback)
			{
				first = merged.toFileIndex(i, first);
				last = merged.toFileIndex(i, last);
				fileText = merged.text;
				break;
			}
	}

	const auto mapped = lspRange(fileText, first, last);
	if (!mapped.isVoid())
		edit.getDynamicObject()->setProperty("range", mapped);
	else
		item->removeProperty("textEdit");
}

bool LspRestState::readQuery(const var& body, const Document& document,
	JavascriptProcessor* jp, UriLocation& location, String& snippet, int& line, int& column)
{
	if (!readPosition(body, line, column))
		return false;

	location.processorId = jp->getUriProcessorId();
	if (document.callbacks)
	{
		if (!jp->ensureMergedCallbackMapping())
			return false;

		const auto& merged = jp->getMergedCallbackDocument();
		const int index = JavascriptProcessor::MergedCallbackDocument::toCharacterIndex(
			merged.text, line, column);
		const int snippetIndex = merged.getSnippetIndex(index);
		if (snippetIndex < 0)
			return false;

		snippet = merged.snippets[snippetIndex];
		location.uri = jp->getSnippet(snippetIndex)->getCallbackName().toString() + "()";
		location.charIndex = merged.toSnippetIndex(snippetIndex, index);
	}
	else
	{
		snippet = document.included->getFileDocument().getAllContent();
		location.uri = document.file.getFullPathName();
		location.charIndex = JavascriptProcessor::MergedCallbackDocument::toCharacterIndex(
			snippet, line, column);
	}

	if (location.charIndex < 0)
		return false;

	CodeDocument source;
	source.replaceAllContent(snippet);
	CodeDocument::Position local(source, location.charIndex);
	location.line = local.getLineNumber() + 1;
	location.col = local.getIndexInLine() + 1;
	line = local.getLineNumber();
	column = local.getIndexInLine();
	return true;
}

LspRestState& BackendProcessor::getLspRestState()
{
	if (lspRestState == nullptr)
		lspRestState = std::make_unique<LspRestState>();

	return *lspRestState;
}

RestServer::Response RestHelpers::handleLsp(BackendProcessor* bp, RestServer::AsyncRequest::Ptr req, ApiRoute route)
{
	using Route = ApiRoute;

	// Update CodeDocuments, callback snippets, and processor trees on the actual message thread.
	// The REST server serializes HTTP requests, so the queued operations keep request order.
	if (!MessageManager::getInstance()->isThisTheMessageThread())
	{
		if (!MessageManager::callAsync([bp, req, route]()
		{
			try
			{
				auto response = RestHelpers::handleLsp(bp, req, route);
				if (response.statusCode < 400)
					req->complete(response);
			}
			catch (const std::exception& e)
			{
				req->complete(RestServer::Response::error(500, String(e.what())));
			}
			catch (...)
			{
				req->complete(RestServer::Response::error(500, "Unexpected LSP error"));
			}
		}))
			return req->fail(500, "Could not schedule LSP request on the message thread");

		return req->waitForResponse();
	}

	MainController* mc = bp;
	const auto body = req->getRequest().getJsonBody();
	const auto uri = body["uri"].toString();
	const auto session = body["sessionId"].toString();
	if (!uri.startsWithIgnoreCase("file://") || session.isEmpty())
		return req->fail(400, "uri must be a local file URI and sessionId must be provided");

	const URL url(uri);
	// JUCE treats the Windows drive letter in file:///D:/... as the URL domain.
	// Check the hostless URI form directly rather than using URL::getDomain().
	bool hostless = uri.startsWithIgnoreCase("file:///");
#if JUCE_WINDOWS
	hostless = hostless && CharacterFunctions::isLetter(uri[8]) &&
		((uri[9] == ':' && uri[10] == '/') ||
		 (uri.substring(9, 12).equalsIgnoreCase("%3A") && uri[12] == '/'));
#else
	hostless = hostless && uri[8] != '/';
#endif
	if (!url.isLocalFile() || !hostless || uri.containsChar('?') || uri.containsChar('#'))
		return req->fail(400, "uri must refer to a local file without a host, query, or fragment");

	for (const auto& segment : StringArray::fromTokens(url.getSubPath(), "/", ""))
		if (URL::removeEscapeChars(segment) == "..")
			return req->fail(400, "Path traversal is not permitted in LSP document URIs");

	const File file = url.getLocalFile();
	if (!File::isAbsolutePath(file.getFullPathName()) || !file.hasFileExtension(".js"))
		return req->fail(400, "uri must identify an absolute JavaScript file");

	auto& registry = bp->getLspRestState();
	registry.startLeaseTimer();
	const auto project = mc->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Scripts);
	if (registry.project != project)
	{
		registry.clear();
		registry.project = project;
	}

	const auto key = file.getFullPathName();
	auto found = registry.documents.find(key);
	const bool isOpen = route == Route::LspDocumentOpen;
	const bool isClose = route == Route::LspDocumentClose;
	const bool isSave = route == Route::LspDocumentSave;
	const bool isChange = route == Route::LspDocumentChange;
	const bool isQuery = !isOpen && !isClose && !isSave && !isChange;

	if (found == registry.documents.end() && !isOpen)
		return req->fail(404, "Document has not been opened by LSP");

	if (isOpen && found == registry.documents.end())
	{
		if (!body["version"].isInt() || (int)body["version"] < 0
			|| !body.hasProperty("text") || !body["text"].isString())
			return req->fail(400, "Open requires a non-negative version and full text");

		LspRestState::Document next;
		String error;
		if (!registry.resolveDocument(mc, file, next, error))
			return req->fail(404, error);
		if (next.callbacks)
		{
			JavascriptProcessor::MergedCallbackDocument candidate;
			auto valid = candidate.parse(*next.processor.get(), body["text"].toString());
			if (valid.failed())
				return req->fail(400, valid.getErrorMessage());
		}
		next.uri = uri;
		next.sessionId = session;
		next.epoch = ++registry.nextEpoch;
		next.lastActivity = Time::currentTimeMillis();
		found = registry.documents.emplace(key, std::move(next)).first;
	}

	auto& document = found->second;
	if (document.sessionId != session)
		return req->fail(409, "Document is already owned by another LSP session");
	document.lastActivity = Time::currentTimeMillis();

	if (!isOpen && (!body["epoch"].isInt() || (int)body["epoch"] != document.epoch))
		return req->fail(409, "Stale document epoch");

	if (isClose)
	{
		registry.release(document);
		registry.documents.erase(found);
		auto response = new DynamicObject();
		response->setProperty("success", true);
		return RestServer::Response::ok(var(response));
	}

	if (!body["version"].isInt() || (int)body["version"] < 0)
		return req->fail(400, "version must be a non-negative integer");

	const int version = (int)body["version"];
	if (isQuery && version != document.version)
		return req->fail(409, "Query version does not match the analyzed document");

	if (isOpen || isChange || isSave)
	{
		if ((isOpen || isChange) && !body.hasProperty("text"))
			return req->fail(400, "Full document text is required");
		if (version < document.version || (isChange && version == document.version))
			return req->fail(409, "Stale document version");

		const bool hasText = body.hasProperty("text");
		const auto text = hasText ? body["text"].toString() : document.text;
		if (version == document.version && text != document.text && !isSave)
			return req->fail(409, "The same document version cannot have different text");
		if (isSave && version > document.version && !hasText)
			return req->fail(409, "A newer saved version requires full text");
		if (isSave && version == document.version && text == document.text
			&& !document.lastDiagnostics.isVoid())
		{
			auto unchanged = new DynamicObject();
			unchanged->setProperty("success", true);
			unchanged->setProperty("uri", document.uri);
			unchanged->setProperty("version", version);
			unchanged->setProperty("epoch", document.epoch);
			unchanged->setProperty("diagnosticsByUri", document.lastDiagnostics);
			return RestServer::Response::ok(var(unchanged));
		}

		for (auto jp : registry.contexts(mc, document))
			if (!jp->isShadowParseReady())
				return req->fail(422, "Processor has no initialized shadow-parse context");

		if (version != document.version || (isSave && text != document.text))
		{
			if (document.callbacks)
			{
				auto jp = document.processor.get();
				if (jp == nullptr)
					return req->fail(404, "Script processor no longer exists");
				auto applied = jp->applyMergedCallbackDocument(text);
				if (applied.failed())
					return req->fail(400, applied.getErrorMessage());
				jp->setMergedCallbackFileOpen(true);
			}
			else
			{
				document.included->setLspDocumentOpen(true);
				document.included->getFileDocument().replaceAllContent(text);
			}
			document.text = text;
		}

		LspRestState::Diagnostics diagnostics;
		StringArray analyzed;
		registry.analyze(mc, document, diagnostics, analyzed);
		if (analyzed.isEmpty() && !registry.contexts(mc, document).isEmpty())
			return req->fail(422, "Processor has no initialized shadow-parse context");
		document.version = version;
		document.lastDiagnostics = registry.diagnosticsObject(diagnostics);

		auto response = new DynamicObject();
		response->setProperty("success", true);
		response->setProperty("uri", document.uri);
		response->setProperty("version", version);
		response->setProperty("epoch", document.epoch);
		response->setProperty("documentKind", document.callbacks ? "callbacks" : "included");
		Array<var> processors;
		for (auto jp : registry.contexts(mc, document))
			processors.add(jp->getUriProcessorId());
		response->setProperty(isChange ? "analyzedProcessors" : "processors", var(processors));
		if (document.callbacks && document.processor != nullptr)
			response->setProperty("moduleId", document.processor->getUriProcessorId());
		response->setProperty("diagnosticsByUri", document.lastDiagnostics);
		return RestServer::Response::ok(var(response));
	}

	LspRestState::Diagnostics currentDiagnostics;
	StringArray analyzed;
	registry.analyze(mc, document, currentDiagnostics, analyzed);

	int originalLine = 0, originalColumn = 0;
	if (!registry.readPosition(body, originalLine, originalColumn))
		return req->fail(400, "position requires non-negative integer line and character");

	Array<JavascriptProcessor*> processors = registry.contexts(mc, document);
	const auto moduleId = body[RestApiIds::moduleId].toString();
	if (moduleId.isNotEmpty())
		processors.removeIf([&](JavascriptProcessor* jp) { return jp->getUriProcessorId() != moduleId; });
	if (processors.isEmpty())
		return req->fail(422, "No script processor analyzes this document");

	Array<var> values;
	for (auto jp : processors)
	{
		if (!jp->isShadowParseReady())
			return req->fail(422, "Script processor has no shadow-parse context");

		UriLocation location;
		String snippet;
		int line = originalLine, column = originalColumn;
		if (!registry.readQuery(body, document, jp, location, snippet, line, column))
			return req->fail(400, "Position is outside the document or callback");

		mcl::TokenCollection::ExpressionParser expression(snippet, line, column);
		const auto autocomplete = expression.createTokenData();
		location.charIndex = autocomplete.charIndex;
		if (autocomplete.isCommentLine)
			continue;

		if (route == Route::LspCompletion)
		{
			mcl::TokenCollection::List tokens;
			auto provider = std::make_unique<HiseJavascriptEngine::TokenProvider>(jp);
			provider->addTokens(tokens);
			auto scopeTokens = HiseJavascriptEngine::SimpleExpressionData::parse(
				jp->getScriptEngine(), { autocomplete.tokenBefore, location });
			scopeTokens = scopeTokens.evaluate(jp->getScriptEngine(),
				HiseJavascriptEngine::SimpleExpressionData::EvaluationType::TypeInfo);
			if (scopeTokens.getType() != HiseJavascriptEngine::SimpleExpressionData::Type::NeedsEvaluation &&
				scopeTokens.getType() != HiseJavascriptEngine::SimpleExpressionData::Type::Function)
				scopeTokens.addAutocompleteTokens(tokens);
			Array<var> items;
			for (auto token : tokens)
			{
				if (!token->matches(autocomplete.input, autocomplete.tokenBefore, autocomplete.lineNumber))
					continue;
				auto item = token->getLspDiagnosticObject(autocomplete.tokenBefore);
				expression.addLspLocation(item, autocomplete);
				registry.mapCompletionEdit(jp, document, location, snippet, item);
				item->setProperty("data", jp->getUriProcessorId());
				const int kind = (int)item->getProperty("kind");
				if (kind < 1 || kind > 25)
					item->setProperty("kind", 1); // Non-standard token kinds become LSP Text.
				items.add(var(item.get()));
			}
			auto list = new DynamicObject();
			list->setProperty("isIncomplete", false);
			list->setProperty("items", var(items));
			values.add(var(list));
			continue;
		}

		String symbol = (autocomplete.tokenBefore + autocomplete.input).trimEnd();
		int start = symbol.length();
		while (start > 0)
		{
			const auto c = symbol[start - 1];
			if (!CharacterFunctions::isLetterOrDigit(c) && c != '_' && c != '.')
				break;
			--start;
		}
		symbol = symbol.substring(start);
		if (symbol.isEmpty())
			continue;

		auto resolved = HiseJavascriptEngine::SimpleExpressionData::parse(
			jp->getScriptEngine(), { symbol, location });
		auto node = resolved.getResolvedNode();
		if (node == nullptr)
			continue;

		if (route == Route::LspHover)
		{
			auto hover = new DynamicObject();
			auto contents = new DynamicObject();
			contents->setProperty("kind", "markdown");
			String markdown = "`" + node->id.toString() + "` (" + jp->getUriProcessorId() + ")";
			if (node->data != nullptr)
				markdown << JavascriptLocationTreeHelpers::getTypeAnnotation(*node);
			if (node->hasRange())
			{
				StringArray parameters;
				for (auto child : *node)
					if (child->category == Identifier("parameter"))
					{
						String parameter = child->id.toString();
						if (child->data != nullptr)
							parameter << JavascriptLocationTreeHelpers::getTypeAnnotation(*child);
						parameters.add(parameter);
					}
				markdown << "(" << parameters.joinIntoString(", ") << ")";
			}
			if (node->data != nullptr)
			{
				auto comment = node->data->getProperty("comment").toString();
				if (comment.isNotEmpty())
					markdown << "  \n" << comment;
			}
			contents->setProperty("value", markdown);
			hover->setProperty("contents", var(contents));
			values.add(var(hover));
		}
		else if (route == Route::LspDefinition)
		{
			const auto target = registry.locationObject(mc, jp, node->getTokenLocation(), node->id.toString().length());
			if (!target.isVoid())
				values.add(target);
		}
		else if (route == Route::LspReferences)
		{
			const bool declaration = body["context"]["includeDeclaration"];
			if (declaration)
			{
				auto target = registry.locationObject(mc, jp, node->getTokenLocation(), node->id.toString().length());
				if (!target.isVoid())
					values.add(target);
			}
			if (node->data == nullptr)
				continue;

			for (const auto& occurrence : JavascriptLocationTreeHelpers::getOccurrences(*node))
			{
				auto target = registry.locationObject(mc, jp, occurrence.second, node->id.toString().length());
				if (!target.isVoid())
					values.add(target);
			}
		}
	}

	auto response = new DynamicObject();
	response->setProperty("success", true);
	response->setProperty("uri", document.uri);
	response->setProperty("version", version);
	if (route == Route::LspCompletion)
	{
		Array<var> all;
		for (const auto& list : values)
		{
			auto itemsValue = list["items"];
			if (auto items = itemsValue.getArray())
				all.addArray(*items);
		}
		response->setProperty("isIncomplete", false);
		response->setProperty("items", var(all));
	}
	else if (route == Route::LspHover)
	{
		if (values.isEmpty())
			response->setProperty("hover", var());
		else if (values.size() == 1)
			response->setProperty("hover", values[0]);
		else
		{
			StringArray descriptions;
			for (const auto& hover : values)
				descriptions.add(hover["contents"]["value"].toString());
			auto markdown = new DynamicObject();
			markdown->setProperty("kind", "markdown");
			markdown->setProperty("value", descriptions.joinIntoString("\n\n---\n\n"));
			auto hover = new DynamicObject();
			hover->setProperty("contents", var(markdown));
			response->setProperty("hover", var(hover));
		}
	}
	else
	{
		Array<var> unique;
		StringArray seen;
		for (const auto& value : values)
		{
			const auto key = JSON::toString(value, false);
			if (!seen.contains(key))
			{
				seen.add(key);
				unique.add(value);
			}
		}
		if (route == Route::LspDefinition)
			response->setProperty("locations", unique.isEmpty() ? var() :
				unique.size() == 1 ? unique[0] : var(unique));
		else
			response->setProperty("references", var(unique));
	}

	return RestServer::Response::ok(var(response));
}

} // namespace hise
