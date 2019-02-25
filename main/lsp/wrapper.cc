#include "wrapper.h"
#include "main/pipeline/pipeline.h"
#include <iostream>
#include <regex>
using namespace std;
namespace sorbet::realmain::lsp {

regex contentLengthRegex("^Content-Length: ([0-9]+)$");

vector<unique_ptr<LSPMessage>> LSPWrapper::getLSPResponsesFor(const LSPMessage &message) {
    gs = lspLoop->processRequest(move(gs), message);

    // Should always run typechecking at least once for each request post-initialization.
    ENFORCE(!initialized || gs->lspTypecheckCount > 0, "Fatal error: LSPLoop did not typecheck GlobalState.");

    if (message.isNotification() && message.method() == "initialized") {
        initialized = true;
    }

    vector<unique_ptr<LSPMessage>> rv;
    string responses = lspOstream.str();
    // Reset error flags and change contents of stream to the empty string.
    lspOstream.clear();
    lspOstream.str(string());

    if (responses.length() == 0) {
        // No response.
        return rv;
    }

    // Parse the results. Should be of the form:
    // Content-Length: length\r\n
    // \r\n
    // [length characters]
    // ...in sequence.

    int pos = 0;
    int len = responses.length();
    while (pos < len) {
        int newlinePos = responses.find("\r\n", pos);
        if (newlinePos == string::npos) {
            Exception::raise("Couldn't find Content-Length in response.");
        }
        string contentLengthLine = responses.substr(pos, newlinePos - pos);
        smatch matches;
        if (!regex_match(contentLengthLine, matches, contentLengthRegex)) {
            Exception::raise(fmt::format("Invalid Content-Length line:\n{}", contentLengthLine));
        }

        int contentLength = stoi(matches[1]);
        pos = newlinePos + 2;
        string emptyLine = responses.substr(pos, 2);
        if (emptyLine != "\r\n") {
            Exception::raise(fmt::format("A carraige return and a newline must separate headers and the body of the "
                                         "LSP message. Instead, got:\n{}",
                                         emptyLine));
        }
        pos += 2;

        if (pos + contentLength > len) {
            Exception::raise(
                fmt::format("Invalid Content-Length: Server specified `{}`, but only `{}` characters provided.",
                            contentLength, len - pos));
        }

        string messageLine = responses.substr(pos, contentLength);
        rv.push_back(make_unique<LSPMessage>(alloc, messageLine));
        pos += contentLength;
    }

    return rv;
}

vector<unique_ptr<LSPMessage>> LSPWrapper::getLSPResponsesFor(const string &message) {
    return getLSPResponsesFor(LSPMessage(alloc, message));
}

LSPWrapper::LSPWrapper(unique_ptr<core::GlobalState> gs, options::Options &&options,
                       const shared_ptr<spdlog::logger> &logger, bool disableFastPath)
    : opts(move(options)) {
    ENFORCE(gs->errorQueue->ignoreFlushes); // LSP needs this
    workers = make_unique<WorkerPool>(0, logger);
    // N.B.: cin will not actually be used the way we are driving LSP.
    // Configure LSPLoop to run on test files (as all test input files are "test" files) and disable configatron.
    lspLoop =
        make_unique<LSPLoop>(std::move(gs), opts, logger, *workers.get(), cin, lspOstream, true, true, disableFastPath);
}
} // namespace sorbet::realmain::lsp