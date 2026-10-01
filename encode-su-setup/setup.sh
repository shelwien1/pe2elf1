#!/usr/bin/env bash
# Restore the encode.su vBulletin 4.2.6 forum as a local mirror on Ubuntu 24.04:
# Apache 2.4 + PHP 5.6 (ondrej/php PPA) + MySQL 8.0, served at http://encode.su
#
# Usage: sudo ./setup.sh
#
# Idempotent: re-running skips finished downloads/extraction/import and
# re-applies configuration. Environment overrides:
#   WORKDIR=/home/user/dl   where the archives are downloaded
#   WEBROOT=/var/www        the forum is extracted to $WEBROOT/encode.su
#   REIMPORT=1              drop the database and import the dump again
set -euo pipefail

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORKDIR=${WORKDIR:-/home/user/dl}
WEBROOT=${WEBROOT:-/var/www}
FORUM=$WEBROOT/encode.su
FILES_URL=https://nishi.dreamhosters.com/20210323.tar.xz
DUMP_URL=https://nishi.dreamhosters.com/20251008.sql.xz
FILES_ARCHIVE=$WORKDIR/$(basename "$FILES_URL")
DUMP_ARCHIVE=$WORKDIR/$(basename "$DUMP_URL")
SITE_URL=http://encode.su
# "Launchpad PPA for Ondřej Surý" signing key (ppa:ondrej/php)
PPA_KEY_FPR=B8DC7E53946656EFBCE4C1DD71DAEAAB4AD4CAB6
# encode.su itself, the DreamHost MySQL host, and the host name that
# includes/config.php actually uses ($config['MasterServer']['servername'])
LOCAL_HOSTS="encode.su www.encode.su robleto.iad1-mysql-e2-12a.dreamhost.com mysql.ctxmodel.net"

log() { printf '\n==> %s\n' "$*"; }

if [ "$(id -u)" != 0 ]; then
	echo "Run as root (sudo $0)" >&2
	exit 1
fi

# --- 1. Archives ----------------------------------------------------------
remote_size() {
	curl -sSfIL "$1" | tr -d '\r' | awk 'tolower($1) == "content-length:" { n = $2 } END { print n + 0 }'
}

log "Downloading archives to $WORKDIR"
mkdir -p "$WORKDIR"
for url in "$FILES_URL" "$DUMP_URL"; do
	file=$WORKDIR/$(basename "$url")
	want=$(remote_size "$url" 2>/dev/null || echo 0)
	have=$(stat -c %s "$file" 2>/dev/null || echo 0)
	if [ "$have" -gt 0 ] && { [ "$have" = "$want" ] || [ "$want" = 0 ]; }; then
		echo "$(basename "$file"): already downloaded ($have bytes)"
		continue
	fi
	# -C - resumes a partial download
	curl -fL --retry 5 --retry-delay 2 -C - -o "$file" "$url"
done
xz -t "$DUMP_ARCHIVE"

# --- 2. Packages ------------------------------------------------------------
log "Installing Apache 2.4, MySQL 8.0 and PHP 5.6"
export DEBIAN_FRONTEND=noninteractive
if [ ! -f /etc/apt/sources.list.d/ondrej-php.list ]; then
	key=$(mktemp)
	curl -sSf "https://keyserver.ubuntu.com/pks/lookup?op=get&options=mr&search=0x$PPA_KEY_FPR" -o "$key"
	if ! gpg --show-keys --with-colons "$key" 2>/dev/null | grep -q "^fpr:*$PPA_KEY_FPR:"; then
		echo "ondrej/php PPA key fingerprint mismatch" >&2
		exit 1
	fi
	install -d -m 755 /etc/apt/keyrings
	gpg --dearmor < "$key" > /etc/apt/keyrings/ondrej-php.gpg
	rm -f "$key"
	. /etc/os-release
	echo "deb [signed-by=/etc/apt/keyrings/ondrej-php.gpg] https://ppa.launchpadcontent.net/ondrej/php/ubuntu $VERSION_CODENAME main" \
		> /etc/apt/sources.list.d/ondrej-php.list
fi
apt-get update -q
apt-get install -y -q --no-install-recommends \
	apache2 mysql-server mysql-client xz-utils \
	libapache2-mod-php5.6 php5.6-cli php5.6-common php5.6-mysql php5.6-gd \
	php5.6-mbstring php5.6-xml php5.6-curl php5.6-json php5.6-opcache
# make "php" mean PHP 5.6 on the command line too
update-alternatives --set php /usr/bin/php5.6 >/dev/null 2>&1 || true

# --- 3. Host names ----------------------------------------------------------
log "Mapping $LOCAL_HOSTS to localhost"
names_re=$(echo "$LOCAL_HOSTS" | sed 's/\./\\./g; s/ /|/g')
hosts_tmp=$(mktemp)
# drop earlier entries for these names, then append ours
grep -v -E "# encode\.su local mirror|(^|[[:space:]])($names_re)([[:space:]]|\$)" /etc/hosts > "$hosts_tmp" || true
{
	cat "$hosts_tmp"
	echo "# encode.su local mirror"
	echo "127.0.0.1 $LOCAL_HOSTS"
	echo "::1 $LOCAL_HOSTS"
} > /etc/hosts # rewritten in place: /etc/hosts is often a bind mount in containers
rm -f "$hosts_tmp"

# --- 4. Forum files ---------------------------------------------------------
if [ ! -f "$FORUM/includes/config.php" ]; then
	log "Extracting $(basename "$FILES_ARCHIVE") to $WEBROOT (2.8 GB, a few minutes)"
	mkdir -p "$WEBROOT"
	tar -xJf "$FILES_ARCHIVE" -C "$WEBROOT" --no-same-owner
fi
chown -R www-data:www-data "$FORUM"

# --- 5. MySQL ---------------------------------------------------------------
log "Configuring and starting MySQL"
install -m 644 "$HERE/conf/mysql-zz-encode-su.cnf" /etc/mysql/mysql.conf.d/zz-encode-su.cnf
install -d -o mysql -g mysql /var/run/mysqld
service mysql restart
for _ in $(seq 60); do
	mysqladmin ping >/dev/null 2>&1 && break
	sleep 1
done
mysqladmin ping

CONFIG_PHP=$FORUM/includes/config.php
DBNAME=$(php5.6 "$HERE/vb_db_sql.php" "$CONFIG_PHP" --dbname)
if [ "${REIMPORT:-0}" = 1 ]; then
	mysql -e "DROP DATABASE IF EXISTS \`$DBNAME\`"
fi
# database + account exactly as includes/config.php expects them
php5.6 "$HERE/vb_db_sql.php" "$CONFIG_PHP" | mysql

tables=$(mysql -N -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '$DBNAME'")
if [ "$tables" = 0 ]; then
	log "Importing $(basename "$DUMP_ARCHIVE") into $DBNAME (about a minute)"
	xzcat "$DUMP_ARCHIVE" | mysql --default-character-set=utf8 "$DBNAME"
fi
mysql -N -e "SELECT CONCAT(COUNT(*), ' tables in $DBNAME') FROM information_schema.tables WHERE table_schema = '$DBNAME'"

# --- 6. Apache + PHP --------------------------------------------------------
log "Configuring Apache and PHP 5.6"
install -m 644 "$HERE/conf/php-99-encode-su.ini" /etc/php/5.6/apache2/conf.d/99-encode-su.ini
install -m 644 "$HERE/conf/php-99-encode-su.ini" /etc/php/5.6/cli/conf.d/99-encode-su.ini
sed "s#/var/www/encode.su#$FORUM#g" "$HERE/conf/apache-encode.su.conf" > /etc/apache2/sites-available/encode.su.conf
echo 'ServerName encode.su' > /etc/apache2/conf-available/servername.conf
a2enmod -q rewrite >/dev/null
a2enconf -q servername >/dev/null
a2dissite -q 000-default >/dev/null 2>&1 || true
a2ensite -q encode.su >/dev/null
apache2ctl configtest
service apache2 restart

# --- 7. Local-mirror adjustments --------------------------------------------
# Production runs on https://encode.su; this mirror is plain HTTP.
log "Pointing the forum at $SITE_URL"
php5.6 "$HERE/vb_set_option.php" "$FORUM" bburl "$SITE_URL"
php5.6 "$HERE/vb_set_option.php" "$FORUM" vbforum_url "$SITE_URL"
mysql "$DBNAME" < "$HERE/local_mirror_fixes.sql"
# CSS is stored as files with absolute (bburl-based) image URLs: regenerate it
php5.6 "$HERE/vb_rebuild_styles.php" "$FORUM"
chown -R www-data:www-data "$FORUM/clientscript/vbulletin_css"

# --- 8. Check ---------------------------------------------------------------
log "Smoke test"
"$HERE/smoke_test.sh"

log "Done: open $SITE_URL (this machine resolves encode.su to 127.0.0.1)"
