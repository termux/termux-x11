#!/bin/bash
set -e -u

ANDROID_SDK_TOOLS_URL="https://dl.google.com/android/repository/commandlinetools-linux-7302050_latest.zip"
ANDROID_SDK_ROOT="${HOME}/android-sdk"
TMP="$(mktemp -d)"

# check if running under root
if [ "$(id -u)" == "0" ]; then
	SUDO=""
else
	SUDO="sudo -Es"
fi

# install dependencies
${SUDO} bash -c \ 
	"apt install curl file unzip || pacman -S curl file unzip --needed || dnf install curl file unzip"

# check java in PATH
if ! "$(command -v java)" !>/dev/null 2>&1; then
	echo "No java on your device is installed! please install openjdk. version 8 is recommended"
	exit 2
fi

# setup android sdk
curl --fail --location "${ANDROID_SDK_TOOLS_URL}" \
	--output "${TMP:-/tmp}/sdk.zip"
unzip -d "${ANDROID_SDK_ROOT}" "${TMP:-/tmp}/sdk.zip" -qq

# accept licenses
yes | "${ANDROID_SDK_ROOT}"/commandline-tools/bin/sdkmanager --licenses \
	--sdk_root="${ANDROID_SDK_ROOT}" 

if [ "${?}" == "0" ]; then
	cat <<- EOL
	Success installing Android SDK tools!

	At this point you can specify the ANDROID_HOME variable to ${ANDROID_SDK_ROOT}
	EOL
else
	echo "An error has occured during the installation"
	exit 2
fi
