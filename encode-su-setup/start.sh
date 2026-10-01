#!/usr/bin/env bash
# Bring the mirror back up after a container/machine restart (no systemd here):
# re-adds the /etc/hosts entries if they were reset, starts MySQL and Apache.
set -euo pipefail

LOCAL_HOSTS="encode.su www.encode.su robleto.iad1-mysql-e2-12a.dreamhost.com mysql.ctxmodel.net"

if [ "$(id -u)" != 0 ]; then
	echo "Run as root (sudo $0)" >&2
	exit 1
fi

if ! grep -q '# encode.su local mirror' /etc/hosts; then
	{
		echo "# encode.su local mirror"
		echo "127.0.0.1 $LOCAL_HOSTS"
		echo "::1 $LOCAL_HOSTS"
	} >> /etc/hosts
fi

install -d -o mysql -g mysql /var/run/mysqld
service mysql start
service apache2 start
