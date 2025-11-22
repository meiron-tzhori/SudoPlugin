#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sudo_plugin.h>
#include <vector>

static sudo_conv_t sudo_conv;
static sudo_printf_t sudo_log;
static FILE *info, *input, *output;
static unsigned int parentPid;

#define PACKAGE_VERSION "1.2.3"
#define PATH_MAX        1024
#define ignore_result

extern std::vector<int> findChildProcesses(int parentPid);

static int
io_open(unsigned int version, sudo_conv_t conversation,
	sudo_printf_t sudo_plugin_printf, char * const settings[],
	char * const user_info[], char * const command_info[],
	int argc, char * const argv[], char * const user_env[], char * const args[],
	const char **errstr)
{
	int fd;
	char path[PATH_MAX];

	if (!sudo_conv)
	sudo_conv = conversation;
	if (!sudo_log)
	sudo_log = sudo_plugin_printf;

	parentPid = (unsigned int)getpid();
	
	/* Open info file. */
	snprintf(path, sizeof(path), "/var/tmp/my-%u.info", parentPid);
	fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd == -1)
	return false;
	info = fdopen(fd, "w");
	ignore_result(fprintf(info, "Process ID: %u\n", parentPid));
	ignore_result(getcwd(path, sizeof(path)));
	ignore_result(fprintf(info, "Absolute path: %s\n", path));
	ignore_result(fprintf(info, "Process owner: %u\n", (unsigned int)getuid()));

	/* Open input and output files. */
	snprintf(path, sizeof(path), "/var/tmp/my-%u.output", parentPid);
	fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd == -1)
	return false;
	output = fdopen(fd, "w");

	snprintf(path, sizeof(path), "/var/tmp/my-%u.input",
	(unsigned int)getpid());
	fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd == -1)
	return false;
	input = fdopen(fd, "w");

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
	ignore_result(fwrite(buf, len, 1, input));
	return true;
}

static int
io_log_output(const char *buf, unsigned int len, const char **errstr)
{
	ignore_result(fprintf(info, "Child processes:\n"));
	auto children = findChildProcesses(parentPid);
	for (int pid : children) {
	    ignore_result(fprintf(info, "  %d:\n", pid));
	}


	const char *cp, *ep;
	bool ret = true;

	ignore_result(fwrite(buf, len, 1, output));
	/*
	 * If we find the string "honk!" in the buffer, reject it.
	 * In practice we'd want to be able to detect the word
	 * broken across two buffers.
	 */
	for (cp = buf, ep = buf + len; cp < ep; cp++) {
	if (cp + 5 < ep && memcmp(cp, "honk!", 5) == 0) {
	    ret = false;
	    break;
	}
	}
	return ret;
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
