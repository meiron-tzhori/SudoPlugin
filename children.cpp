#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <cstdlib>
#include <algorithm>  // for std::all_of
#include <sstream>	// for std::istringstream


std::vector<int> findChildProcesses(int parentPid) {
	std::vector<int> children;
	DIR* proc = opendir("/proc");
	if (!proc) return children;

	struct dirent* entry;
	while ((entry = readdir(proc)) != nullptr) {
		if (entry->d_type != DT_DIR)
			continue;

		std::string pidStr = entry->d_name;
		if (!std::all_of(pidStr.begin(), pidStr.end(), ::isdigit))
			continue;

		int pid = std::stoi(pidStr);
		std::string statPath = "/proc/" + pidStr + "/stat";

		std::ifstream statFile(statPath);
		if (!statFile)
			continue;

		std::string line;
		std::getline(statFile, line);
		statFile.close();

		// Parse the stat file line (fields separated by spaces)
		// Field 4 is PPID (indexing from 1)
		size_t pos1 = line.find(')');
		if (pos1 == std::string::npos) 
			continue;

		std::string after = line.substr(pos1 + 2); // skip ") "
		std::istringstream iss(after);
		std::string state;
		int ppid;
		iss >> state >> ppid;
		if (ppid == parentPid) {
			children.push_back(pid);
			auto children2 = findChildProcesses(pid);
			for (int pid2 : children2) {
				children.push_back(pid2);
			}
		}
	}

	closedir(proc);
	return children;
}

