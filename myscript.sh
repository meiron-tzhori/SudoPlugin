#!/bin/bash
echo "ls..."
ls
sleepsec=5
echo "sleep $sleepsec ..."
sleep $sleepsec 
echo "Create a directory in /opt (requires root privileges)..."
mkdir -p /opt/my_new_directory
