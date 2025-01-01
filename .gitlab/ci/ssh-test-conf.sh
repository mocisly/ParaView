#!/bin/sh

set -e

# Skip if we're not in a `fedora` job.
if ! echo "$CMAKE_CONFIGURATION" | grep -q -e 'fedora'; then
    exit 0
fi

# Only works when running as root.
if [ "$( id -u )" != "0" ]; then
    exit 0
fi

# Generate keys and append own public key as authorized
mkdir ~/.ssh
ssh-keygen -t rsa -q -f "$HOME/.ssh/id_rsa" -N ""
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

# Generate a SSH server configuration
cat > "$HOME/.ssh/sshd_conf" <<EOF
Port 2222
HostKey ~/.ssh/id_rsa
PidFile ~/.ssh/sshd.pid
AuthorizedKeysFile ~/.ssh/id_rsa.pub
EOF

# Run SSH server as daemon
/usr/sbin/sshd -f $HOME/.ssh/sshd_conf
