/**
 * @file
 */

#include "Console.h"
#include "app/App.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"
#include "command/Command.h"
#include "core/collection/Set.h"
#include "core/Common.h"
#include "core/Tokenizer.h"
#include "command/CommandHandler.h"
#include "VarUtil.h"
#include <SDL.h>

namespace util {

Console::Console() :
		_mainThread(SDL_ThreadID()) {
}

Console::~Console() {
}

void Console::construct() {
	SDL_LogGetOutputFunction((SDL_LogOutputFunction*)&_logFunction, &_logUserData);
	SDL_LogSetOutputFunction((SDL_LogOutputFunction)logConsole, this);

	command::Command::registerCommand("clear", [&] (const command::CmdArgs& args) { clear(); }).setHelp("Clear the text from the built-in console");
	command::Command::registerCommand("history", [&] (const command::CmdArgs& args) { printHistory(); }).setHelp("Print the command history");
}

bool Console::init() {
	const io::FilesystemPtr& fs = io::filesystem();
	const core::String& content = fs->load("%s", _historyFilename);
	core::string::splitString(content, _history, "\n");
	_historyPos = _history.size();
	Log::debug("Loaded %i history entries", _historyPos);

	return true;
}

void Console::shutdown() {
	core::String content;
	for (const core::String& s : _history) {
		content += s;
		content += '\n';
	}

	const io::FilesystemPtr& fs = io::filesystem();
	if (!fs->write(_historyFilename, content)) {
		Log::warn("Failed to write the history");
	} else {
		Log::debug("Wrote the history");
	}
	clear();

	command::Command::unregisterCommand("clear");
	command::Command::unregisterCommand("history");
	SDL_LogSetOutputFunction((SDL_LogOutputFunction)_logFunction, _logUserData);
}

void Console::printHistory() {
	for (const core::String& s : _history) {
		Log::info("%s", s.c_str());
	}
}

void Console::executeCommandLine() {
	_messages.emplace_back(SDL_LOG_PRIORITY_INFO, _consolePrompt + _commandLine);
	if (_commandLine.empty()) {
		return;
	}
	_history.push_back(_commandLine);
	_historyPos = _history.size();

	command::executeCommands(_commandLine);
	clearCommandLine();
}

void Console::cursorUp() {
	if (_historyPos <= 0) {
		return;
	}

	--_historyPos;
	_commandLine = _history[_historyPos];
}

void Console::cursorDown() {
	++_historyPos;

	const size_t entries = _history.size();
	if (_historyPos >= entries) {
		_historyPos = entries;
		clearCommandLine();
		return;
	}
	_commandLine = _history[_historyPos];
}

void Console::autoComplete() {
	core::DynamicArray<core::String> matches;
	const core::DynamicArray<core::String> allCommands = core::Tokenizer(_commandLine, ";").tokens();
	const core::String& lastCmd = allCommands.empty() ? "" : allCommands.back();
	const core::DynamicArray<core::String> strings = core::Tokenizer(lastCmd, " ").tokens();
	core::String baseSearchString = "";
	bool parameter = _commandLine.last() == ' ' || strings.size() > 1;
	if (parameter) {
		const command::Command* cmd = command::Command::getCommand(strings.front());
		if (cmd != nullptr) {
			if (strings.back() == strings.front()) {
				cmd->complete("", matches);
			} else {
				cmd->complete(strings.back(), matches);
			}
		}
	} else {
		// try to complete the already existing command
		baseSearchString = strings.empty() ? "" : strings.back();
		command::Command::visitSorted([&] (const command::Command& cmd) {
			if (strings.empty() || strings.size() == 1) {
				// match the command name itself
				if (core::string::matches(cmd.name(), baseSearchString + "*")) {
					matches.push_back(cmd.name());
				}
			} else {
				// match parameters for the command
				cmd.complete(strings.back(), matches);
			}
		});
		if (!strings.empty()) {
			// try to complete the last string as it can be used as a parameter to the command
			baseSearchString = strings.back();
		}
		baseSearchString += '*';
		util::visitVarSorted([&] (const core::VarPtr& var) {
			if (core::string::matches(var->name(), baseSearchString)) {
				matches.push_back(var->name());
			}
		}, 0u);
	}

	if (matches.empty()) {
		return;
	}

	core::Set<core::String, 11, core::StringHash> uniqueMatches;
	uniqueMatches.insert(matches.begin(), matches.end());
	matches.clear();
	for (auto i = uniqueMatches.begin(); i != uniqueMatches.end(); ++i) {
		matches.push_back(i->key);
	}
	matches.sort(core::Greater<core::String>());

	if (matches.size() == 1) {
		if (strings.size() <= 1) {
			_commandLine = matches.front() + " ";
		} else {
			const size_t cmdLineSize = _commandLine.size();
			const size_t cmdEraseIndex = cmdLineSize - strings.back().size();
			_commandLine.erase(cmdEraseIndex, strings.back().size());
			_commandLine.insert(cmdEraseIndex, matches.front().c_str());
		}
	} else {
		_messages.emplace_back(SDL_LOG_PRIORITY_INFO, _consolePrompt + _commandLine);
		int pos = 0;
		const core::String first = matches.front();
		for (char c : first) {
			bool allMatch = true;
			for (const core::String& match : matches) {
				if (match[pos] != c) {
					allMatch = false;
					break;
				}
			}
			if (!allMatch) {
				break;
			}
			++pos;
		}
		if (pos > 0) {
			replaceLastParameter(matches.front().substr(0, pos));
		}
		for (const core::String& match : matches) {
			Log::info("%s", match.c_str());
		}
	}
}

void Console::replaceLastParameter(const core::String& param) {
	auto iter = _commandLine.rfind(' ');
	if (iter == core::String::npos) {
		_commandLine = param;
		return;
	}

	_commandLine.erase(iter + 1);
	_commandLine.append(param.c_str());
}

core::String Console::removeAnsiColors(const char* message) {
	core::String out;
	out.reserve(SDL_strlen(message) + 1);
	for (const char *c = message; *c != '\0'; ++c) {
		// https://en.wikipedia.org/wiki/ANSI_escape_code
		if (*c >= 030 && *c < 037 && *(c + 1) == '[') {
			c += 2;
			while (*c != 'm' && *c != '\0') {
				++c;
			}
			continue;
		}
		out += *c;
	}
	return out;
}

void Console::logConsole(void *userdata, int category, int priority, const char *message) {
	if (priority < 0 || priority >= SDL_NUM_LOG_PRIORITIES) {
		return;
	}
	if (priority < SDL_LogGetPriority(category)) {
		return;
	}
	Console* console = (Console*)userdata;
	if (SDL_ThreadID() != console->_mainThread) {
		core_assert(message);
		console->_messageQueue.emplace(category, priority, message);
		return;
	}
	console->addLogLine(category, priority, message);
}

void Console::addLogLine(int category, int priority, const char *message) {
	const core::String& cleaned = removeAnsiColors(message);
	_messages.emplace_back(priority, cleaned);
	if (_useOriginalLogFunction) {
		((SDL_LogOutputFunction)_logFunction)(_logUserData, category, (SDL_LogPriority)priority, message);
	}
}

void Console::update(double /*deltaFrameSeconds*/) {
	core_assert(_mainThread == SDL_ThreadID());
	LogLine msg;
	while (_messageQueue.pop(msg)) {
		core_assert(msg.message);
		addLogLine(msg.category, msg.priority, msg.message);
	}
}

void Console::clear() {
	clearCommandLine();
	_messages.clear();
}

inline void Console::clearCommandLine() {
	_commandLine.clear();
}

}
