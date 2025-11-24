#include <cstdio>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <sudo_plugin.h>
#include "plugin.h"

namespace fs = std::filesystem;

static sudo_conv_t sudo_conv;
static sudo_printf_t sudo_log;
static std::optional<std::ofstream> info;
static std::optional<std::ofstream> input;
static std::optional<std::ofstream> output;
static unsigned int parentPid;
static bool logged_children = false;

namespace {
	constexpr std::string_view PACKAGE_VERSION = "1.2.3";
	constexpr std::string_view TMP_DIR = "/var/tmp";
	constexpr std::string_view INFO_SUFFIX = ".info";
	constexpr std::string_view OUTPUT_SUFFIX = ".output";
	constexpr std::string_view INPUT_SUFFIX = ".input";
}

std::vector<PidInfo> findChildProcesses(int parentPid);

namespace {
	std::string make_path(std::string_view prefix, unsigned int pid, std::string_view suffix) {
		std::string path{TMP_DIR};
		path += "/";
		path += prefix;
		path += "-";
		path += std::to_string(pid);
		path += suffix;
		return path;
	}

	std::optional<std::ofstream> open_file(const std::string& path) {
		// Check if file already exists (preserve O_EXCL behavior)
		if (fs::exists(path)) {
			return std::nullopt;
		}
		std::ofstream file{path, std::ios::out | std::ios::trunc};
		if (!file.is_open()) {
			return std::nullopt;
		}
		return file;
	}

	// Helper template for type-safe formatting using modern C++ streams
	template<typename... Args>
	void format_write(std::ofstream& file, Args&&... args) {
		if (!file.is_open()) {
			return;
		}
		(file << ... << std::forward<Args>(args));
		file.flush();
	}
}

static int
io_open([[maybe_unused]] unsigned int version, sudo_conv_t conversation,
	sudo_printf_t sudo_plugin_printf, [[maybe_unused]] char * const settings[],
	[[maybe_unused]] char * const user_info[], [[maybe_unused]] char * const command_info[],
	[[maybe_unused]] int argc, [[maybe_unused]] char * const argv[], 
	[[maybe_unused]] char * const user_env[], [[maybe_unused]] char * const args[],
	[[maybe_unused]] const char **errstr)
{
	if (!sudo_conv) {
		sudo_conv = conversation;
	}
	if (!sudo_log) {
		sudo_log = sudo_plugin_printf;
	}

	parentPid = static_cast<unsigned int>(getpid());
	
	// Open info file
	const std::string info_path = make_path("my", parentPid, INFO_SUFFIX);
	info = open_file(info_path);
	if (!info.has_value() || !info->is_open()) {
		return false;
	}
	
	format_write(*info, "Process ID: ", parentPid, "\n");
	
	try {
		const auto cwd = fs::current_path();
		format_write(*info, "Absolute path: ", cwd.string(), "\n");
	} catch (const fs::filesystem_error&) {
		// Ignore if we can't get current directory
	}
	
	format_write(*info, "Process owner: ", static_cast<unsigned int>(getuid()), "\n");

	// Open output file
	const std::string output_path = make_path("my", parentPid, OUTPUT_SUFFIX);
	output = open_file(output_path);
	if (!output.has_value() || !output->is_open()) {
		return false;
	}

	// Open input file
	const std::string input_path = make_path("my", parentPid, INPUT_SUFFIX);
	input = open_file(input_path);
	if (!input.has_value() || !input->is_open()) {
		return false;
	}

	return true;
}


static void
io_close([[maybe_unused]] int exit_status, [[maybe_unused]] int error)
{
	if (info.has_value()) {
		info->close();
		info = std::nullopt;
	}
	if (input.has_value()) {
		input->close();
		input = std::nullopt;
	}
	if (output.has_value()) {
		output->close();
		output = std::nullopt;
	}
}

static int
io_version([[maybe_unused]] int verbose)
{
	if (sudo_log) {
		sudo_log(SUDO_CONV_INFO_MSG, "My I/O plugin version %.*s\n", 
			static_cast<int>(PACKAGE_VERSION.size()), PACKAGE_VERSION.data());
	}
	return true;
}

static int
io_log_input(const char *buf, unsigned int len, [[maybe_unused]] const char **errstr)
{
	if (input.has_value() && input->is_open() && buf && len > 0) {
		input->write(buf, static_cast<std::streamsize>(len));
		input->flush();
	}
	return true;
}

static int
io_log_output(const char *buf, unsigned int len, [[maybe_unused]] const char **errstr)
{
	if (info.has_value() && info->is_open() && !logged_children) {
		logged_children = true;
		format_write(*info, "Child processes:\n");
		const auto children = findChildProcesses(static_cast<int>(parentPid));
		for (const auto& child : children) {
			format_write(*info, "  ", child.pid, " ", child.name, "\n");
		}
	}

	if (output.has_value() && output->is_open() && buf && len > 0) {
		output->write(buf, static_cast<std::streamsize>(len));
		output->flush();
	}
	
	return true;
}

struct io_plugin my_io = {
	SUDO_IO_PLUGIN,
	SUDO_API_VERSION,
	io_open,
	io_close,
	io_version,
	io_log_input,	/* tty input */
	io_log_output,	/* tty output */
	io_log_input,	/* command stdin if not tty */
	io_log_output,	/* command stdout if not tty */
	io_log_output,	/* command stderr if not tty */
	NULL, /* register_hooks */
	NULL, /* deregister_hooks */
	NULL, /* change_winsize */
	NULL, /* log_suspend */
	NULL /* event_alloc() filled in by sudo */
};
