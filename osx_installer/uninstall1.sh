#!/bin/bash

set -e
set -x

function finish {
	if [[ $OK == "true" ]]
	then
		osascript -e 'display notification \"UrBackup was uninstalled successfully.\" with title \"UrBackup\"'
	else
		osascript -e 'display notification \"Failed to uninstall UrBackup\" with title \"UrBackup\"'
	fi
}

trap finish EXIT

if /bin/launchctl list "org.urbackup.client.backend" &> /dev/null; then
	/bin/launchctl stop "org.urbackup.client.backend" 
    /bin/launchctl unload "/Library/LaunchDaemons/org.urbackup.client.plist"
fi

if /bin/launchctl list "org.urbackup.client.frontend" &> /dev/null; then
	/bin/launchctl stop "org.urbackup.client.frontend" 
    /bin/launchctl unload "/Library/LaunchAgents/org.urbackup.client.plist"
fi

pkgutil --forget org.urbackup.client.service
pkgutil --forget org.urbackup.client

