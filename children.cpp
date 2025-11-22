#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <cstdlib>
#include <algorithm>  // for std::all_of
#include <sstream>	// for std::istringstream
#include "plugin.h"

std::vector<PidInfo> findChildProcesses(int parentPid) {
	std::vector<PidInfo> children;
	DIR* proc = opendir("/proc");
	if (!proc) {
		return children;
	}

	struct dirent* entry;
	while ((entry = readdir(proc)) != nullptr) {
		if (entry->d_type != DT_DIR) {
			continue;
		}

		std::string pidStr = entry->d_name;
		if (!std::all_of(pidStr.begin(), pidStr.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
			continue;
		}

		int pid = 0;
		try {
			pid = std::stoi(pidStr);
		} catch (const std::exception&) {
			continue;
		}

		std::string statPath = "/proc/" + pidStr + "/stat";
		std::ifstream statFile(statPath);
		if (!statFile) {
			continue;
		}

		std::string line;
		if (!std::getline(statFile, line)) {
			continue;
		}

		// Parse the stat file line (fields separated by spaces)
		// Field 4 is PPID (indexing from 1)
		std::istringstream iss(line);
		std::string name, state;
		int _pid = 0, ppid = 0;
		if (!(iss >> _pid >> name >> state >> ppid)) {
			continue;
		}

		if (ppid == parentPid) {
			PidInfo pi;
			pi.pid = pid;
			pi.name = name;
			
			children.push_back(pi);
			auto children2 = findChildProcesses(pid);
			children.insert(children.end(), children2.begin(), children2.end());
		}
	}

	closedir(proc);
	return children;
}

