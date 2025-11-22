#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <vector>
#include <sudo_plugin.h>
#include "plugin.h"

static sudo_conv_t sudo_conv;
static sudo_printf_t sudo_log;
static FILE *info, *input, *output;
static unsigned int parentPid;
static bool logged_children = false;

namespace {
	constexpr const char* PACKAGE_VERSION = "1.2.3";
	constexpr size_t PATH_MAX_SIZE = 1024;
}

extern std::vector<struct PidInfo> findChildProcesses(int parentPid);

static int
io_open(unsigned int version, sudo_conv_t conversation,
	sudo_printf_t sudo_plugin_printf, char * const settings[],
	char * const user_info[], char * const command_info[],
	int argc, char * const argv[], char * const user_env[], char * const args[],
	const char **errstr)
{
	int fd;
	char path[PATH_MAX_SIZE];

	if (!sudo_conv)
		sudo_conv = conversation;
	if (!sudo_log)
		sudo_log = sudo_plugin_printf;

	parentPid = static_cast<unsigned int>(getpid());
	
	/* Open info file. */
	std::snprintf(path, sizeof(path), "/var/tmp/my-%u.info", parentPid);
	fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd == -1)
		return false;
	info = fdopen(fd, "w");
	if (!info)
		return false;
	
	std::fprintf(info, "Process ID: %u\n", parentPid);
	if (getcwd(path, sizeof(path)) != nullptr) {
		std::fprintf(info, "Absolute path: %s\n", path);
	}
	std::fprintf(info, "Process owner: %u\n", static_cast<unsigned int>(getuid()));

	/* Open input and output files. */
	std::snprintf(path, sizeof(path), "/var/tmp/my-%u.output", parentPid);
	fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd == -1)
		return false;
	output = fdopen(fd, "w");
	if (!output)
		return false;

	std::snprintf(path, sizeof(path), "/var/tmp/my-%u.input",
		static_cast<unsigned int>(getpid()));
	fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd == -1)
		return false;
	input = fdopen(fd, "w");
	if (!input)
		return false;

	return true;
}


static void
io_close(int exit_status, int error)
{
	fclose(info);
	fclose(input);
	fclose(output);
}

static int
io_version(int verbose)
{
	sudo_log(SUDO_CONV_INFO_MSG, "My I/O plugin version %s\n", PACKAGE_VERSION);
	return true;
}

static int
io_log_input(const char *buf, unsigned int len, const char **errstr)
{
	if (input) {
		std::fwrite(buf, len, 1, input);
	}
	return true;
}

static int
io_log_output(const char *buf, unsigned int len, const char **errstr)
{
	if (info && !logged_children) {
		logged_children = true;
		std::fprintf(info, "Child processes:\n");
		auto children = findChildProcesses(parentPid);
		for (const auto& child : children) {
			std::fprintf(info, "  %d %s\n", child.pid, child.name.c_str());
		}
	}

	if (output) {
		std::fwrite(buf, len, 1, output);
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
