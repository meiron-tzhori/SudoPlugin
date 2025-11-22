# SudoPlugin
A simple linux sudo plugin

Challenge Overview
Goal:
Develop a sudo plugin that collects and prints all possible data about the program executed by sudo.
Task Details
• When a user runs a command with sudo (e.g., sudo ls), your plugin should output all available data about the executed process, such as:
o Process ID (PID)
o Absolute path
o Process owner
o Any other relevant information
• The output can be directed to either:
o The console, or
o An external file

Reference for a sudo plugin sample:
https://github.com/sudo-project/sudo/blob/main/plugins/sample/sample_plugin.c

Example for sudo logging in /var/log/auth.log:
Nov 21 00:46:01 LAPTOP-SQH1HTRE sudo:   meiron : TTY=pts/4 ; PWD=/home/meiron ; USER=root ; COMMAND=/usr/bin/pwd
Nov 21 00:46:01 LAPTOP-SQH1HTRE sudo: pam_unix(sudo:session): session opened for user root(uid=0) by (uid=1000)
Nov 21 00:46:01 LAPTOP-SQH1HTRE sudo: pam_unix(sudo:session): session closed for user root

The log line in /var/log/auth.log is based on struct eventlog:
https://github.com/sudo-project/sudo/blob/main/include/sudo_eventlog.h#L99

Formatted by new_logline()
https://github.com/sudo-project/sudo/blob/main/lib/eventlog/eventlog.c#L76

