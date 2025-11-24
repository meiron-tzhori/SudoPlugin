#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>  // for std::all_of
#include <sstream>	// for std::istringstream
#include <cctype>
#include "plugin.h"

namespace fs = std::filesystem;

std::vector<PidInfo> findChildProcesses(int parentPid) {
	std::vector<PidInfo> children;
	const fs::path procPath{"/proc"};
	if (!fs::exists(procPath) || !fs::is_directory(procPath)) {
		return children;
	}

	for (const auto& entry : fs::directory_iterator(procPath)) {
		if (!entry.is_directory()) {
			continue;
		}

		const std::string pidStr = entry.path().filename().string();
		const bool isNumeric = std::all_of(pidStr.begin(), pidStr.end(), [](unsigned char c) {
			return std::isdigit(c) != 0;
		});
		if (!isNumeric) {
			continue;
		}

		int pid = 0;
		try {
			pid = std::stoi(pidStr);
		} catch (const std::exception&) {
			continue;
		}

		const fs::path statPath = entry.path() / "stat";
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

	return children;
}

