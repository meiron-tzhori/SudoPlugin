#pragma once

#include <string>
#include <vector>

struct PidInfo {
	int pid;
	std::string name;
};

std::vector<PidInfo> findChildProcesses(int parentPid);
