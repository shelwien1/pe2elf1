#!/bin/sh
# php85undo.sh - undo php85fix.sh: put back the forum files and the database values it changed.
#
# Run it from the forum's root directory, like php85fix.sh:
#
#     cd encode.su && sh ../tmp5/php85undo.sh
#
# By default, the database fields that php85fix.sh changed get their old values back;
# posts, users and everything else written to the database since stay as they are.
#
# Options:
#   --full-db  load the complete database dump that php85fix.sh made instead. Everything
#              written to the database since is lost; the current database is dumped to
#              the backup directory first.
#   --force    also overwrite forum files and database values that were changed after
#              php85fix.sh ran, restore into a different forum directory than the one that
#              was backed up, and finish the job after php85fix.sh failed or was interrupted.
#
# Afterwards the backup directory is renamed to php85-backup.undone-<date>, so php85fix.sh
# can be run again. PHP=/path/to/php selects the PHP command line binary (default: php).

set -u
umask 077

KIT=$(cd "$(dirname "$0")" && pwd -P) || exit 1
FORUM=$(pwd -P)
BACKUP=$KIT/php85-backup
PHP=${PHP:-php}
CNF=$KIT/.php85-mysql.cnf

say() { printf '%s\n' "$*"; }
die() { printf 'php85undo: %s\n' "$*" >&2; exit 1; }

trap 'rm -f "$CNF"' EXIT
trap 'exit 1' HUP INT TERM

FULL=0
FORCE=0
for arg in "$@"; do
	case $arg in
		--full-db) FULL=1 ;;
		--force) FORCE=1 ;;
		*) die "unknown option '$arg' (options: --full-db, --force)" ;;
	esac
done
forceflag=
[ "$FORCE" = 1 ] && forceflag=--force

# ---- checks: nothing is changed until all of them pass --------------------------------------

[ -f global.php ] && [ -f includes/config.php ] ||
	die "run this from the forum's root directory (the one with global.php and includes/)"
[ -f "$BACKUP/state" ] && [ -f "$BACKUP/files.tar.gz" ] ||
	die "no backup in $BACKUP: php85fix.sh has not been run, or it has been undone already"
for f in vb_php8_db_fix.php php8_barewords.php; do
	[ -f "$KIT/$f" ] || die "$KIT/$f is missing"
done
for tool in tar gzip cksum cmp diff mysql mysqldump; do
	command -v "$tool" >/dev/null 2>&1 || die "'$tool' is not installed"
done
"$PHP" -r 'exit(version_compare(PHP_VERSION, "5.6.0", ">=") && function_exists("mysqli_init") ? 0 : 1);' >/dev/null 2>&1 ||
	die "'$PHP' is not a PHP 5.6+ command line binary with the mysqli extension; set PHP=/path/to/php"

saved_forum=$(sed -n 's/^forum=//p' "$BACKUP/state")
if [ "$saved_forum" != "$FORUM" ] && [ "$FORCE" = 0 ]; then
	die "the backup was made for $saved_forum, not for $FORUM (--force restores it here anyway)"
fi
step=$(sed -n 's/^step=//p' "$BACKUP/state" | tail -n 1)
if [ "$step" != "done" ] && [ "$FORCE" = 0 ]; then
	die "php85fix.sh did not finish (last step: ${step:-none}); --force puts back whatever it changed"
fi
if [ "$FULL" = 1 ] && [ ! -f "$BACKUP/database.sql.gz" ]; then
	die "there is no database dump in $BACKUP"
fi

say "Checking the forum scripts ..."
if [ -f "$BACKUP/files-after.cksum" ]; then
	changed=$(xargs cksum < "$BACKUP/files.txt" 2>/dev/null | diff "$BACKUP/files-after.cksum" - | sed -n 's/^[<>] [0-9]* [0-9]* //p' | sort -u)
	if [ -n "$changed" ]; then
		say "These scripts were changed after php85fix.sh ran:"
		say "$changed" | sed 's/^/  /'
		[ "$FORCE" = 1 ] || die "restoring would lose those changes; --force restores them anyway. Nothing was changed."
	fi
fi

DBNAME=$("$PHP" "$KIT/vb_php8_db_fix.php" "$FORUM" --client-config "$CNF") && [ -n "$DBNAME" ] ||
	die "cannot read the database settings from includes/config.php"
saved_db=$(sed -n 's/^database=//p' "$BACKUP/state")
if [ "$saved_db" != "$DBNAME" ] && [ "$FORCE" = 0 ]; then
	die "the backup is of the database '$saved_db', but includes/config.php uses '$DBNAME' (--force restores anyway)"
fi
mysql --defaults-extra-file="$CNF" -e 'SELECT 1' "$DBNAME" >/dev/null ||
	die "cannot log in to the database '$DBNAME'"
if [ "$FULL" = 1 ]; then
	# room to dump the current database (written unpacked, then compressed) and to unpack
	# the old dump: about twice the size of the table data
	dbkb=$(mysql --defaults-extra-file="$CNF" -N -e 'SELECT CEIL(SUM(data_length) / 1024) FROM information_schema.tables WHERE table_schema = DATABASE()' "$DBNAME" 2>/dev/null)
	freekb=$(df -Pk "$KIT" 2>/dev/null | awk 'NR == 2 { print $4 }')
	case "$dbkb:$freekb" in
		*[!0-9:]*|:*|*:) say "(could not check the free disk space)" ;;
		*) [ "$freekb" -ge $((dbkb * 2)) ] ||
			die "not enough disk space in $KIT to dump the current database and unpack the old dump: about $((dbkb * 2 / 1024)) MB are needed, $((freekb / 1024)) MB are free. Nothing was changed." ;;
	esac
fi

LOG=$BACKUP/php85undo.log
if [ "$FULL" = 0 ] && [ -s "$BACKUP/database-undo.dat" ]; then
	say "Checking the database ..."
	# shellcheck disable=SC2086
	"$PHP" "$KIT/vb_php8_db_fix.php" "$FORUM" --restore "$BACKUP/database-undo.dat" --dry-run $forceflag > "$LOG" 2>&1
	case $? in
		0) say "  $(tail -n 1 "$LOG")" ;;
		3)
			sed -n 's/^  //p' "$LOG" | head -n 20 | sed 's/^/  /'
			die "these database values were changed after php85fix.sh ran. --force puts back the old values anyway; --full-db loads the whole database dump. Nothing was changed."
			;;
		*) die "the database check failed (see $LOG). Nothing was changed." ;;
	esac
fi

# ---- restore ----------------------------------------------------------------------------------

# Dump the database to $1.gz. The tables are locked while they are dumped (most vBulletin
# tables are MyISAM); without the LOCK TABLES privilege, the dump is made without locks.
# It is written unpacked and compressed afterwards, so that the tables are locked for a
# shorter time. Nothing is left behind if it fails.
dump_db() {
	rm -f "$1" "$1.gz" "$1.err"
	# shellcheck disable=SC2086
	mysqldump --defaults-extra-file="$CNF" $DUMPOPTS "$DBNAME" > "$1" 2>"$1.err"
	status=$?
	if [ "$status" != 0 ] && grep -q 'LOCK TABLES' "$1.err"; then
		say "  (no LOCK TABLES privilege: dumping without locking the tables)"
		cat "$1.err" >> "$LOG"
		# shellcheck disable=SC2086
		mysqldump --defaults-extra-file="$CNF" $DUMPOPTS --single-transaction "$DBNAME" > "$1" 2>"$1.err"
		status=$?
	fi
	cat "$1.err" >> "$LOG"
	rm -f "$1.err"
	if [ "$status" = 0 ] && tail -n 1 "$1" | grep -q 'Dump completed' && gzip "$1" 2>>"$LOG"; then
		return 0
	fi
	rm -f "$1" "$1.gz"
	return 1
}

# the restore takes a few seconds (longer with --full-db): do not stop half way
trap '' HUP INT TERM

if [ "$FULL" = 1 ]; then
	DUMPOPTS="--quick --default-character-set=utf8mb4"
	for o in --no-tablespaces --set-gtid-purged=OFF --column-statistics=0; do
		mysqldump --help 2>/dev/null | grep -q -- "${o%%=*}" && DUMPOPTS="$DUMPOPTS $o"
	done
	# a new file each time: if loading fails and this is run again, the first one is kept
	before=$BACKUP/database-before-undo-$(date +%Y%m%d-%H%M%S).sql
	say "Dumping the current database to $before.gz ..."
	dump_db "$before" ||
		die "dumping the current database failed (see $LOG). Nothing was changed."
	say "Loading the database dump that php85fix.sh made ..."
	# unpacked first, so that a damaged dump is noticed before anything is loaded
	if ! gzip -dc "$BACKUP/database.sql.gz" > "$BACKUP/database-restore.sql" 2>>"$LOG"; then
		rm -f "$BACKUP/database-restore.sql"
		die "cannot unpack $BACKUP/database.sql.gz (see $LOG). Nothing was changed."
	fi
	if ! mysql --defaults-extra-file="$CNF" "$DBNAME" < "$BACKUP/database-restore.sql" 2>>"$LOG"; then
		rm -f "$BACKUP/database-restore.sql"
		die "loading the dump failed (see $LOG). The database before this attempt is in $before.gz; the forum files were not changed."
	fi
	rm -f "$BACKUP/database-restore.sql"
elif [ -s "$BACKUP/database-undo.dat" ]; then
	say "Putting back the database values ..."
	# shellcheck disable=SC2086
	"$PHP" "$KIT/vb_php8_db_fix.php" "$FORUM" --restore "$BACKUP/database-undo.dat" $forceflag >>"$LOG" 2>&1 ||
		die "putting back the database values failed (see $LOG); the forum files were not changed"
	say "  $(tail -n 1 "$LOG")"
fi

say "Restoring the forum scripts ..."
tar -xzf "$BACKUP/files.tar.gz" 2>>"$LOG" ||
	die "restoring the forum scripts failed (see $LOG); run php85undo.sh --force again"
xargs cksum < "$BACKUP/files.txt" 2>>"$LOG" | cmp -s - "$BACKUP/files-before.cksum" ||
	die "the restored scripts do not match the backup (see $LOG); run php85undo.sh --force again"

done_dir=$BACKUP.undone-$(date +%Y%m%d-%H%M%S)
say "step=undone" >> "$BACKUP/state"
mv "$BACKUP" "$done_dir" || die "cannot rename $BACKUP"

trap 'exit 1' HUP INT TERM

say ""
say "Done: $(wc -l < "$done_dir/files.txt") scripts and the database are as they were before php85fix.sh."
say "The original scripts need PHP 5 (5.6 or older); they do not run on PHP 8."
if "$PHP" -r '$config = array(); include "includes/config.php"; exit(!empty($config["Datastore"]["class"]) && $config["Datastore"]["class"] != "vB_Datastore" ? 0 : 1);' >/dev/null 2>&1; then
	say "A datastore cache is configured in includes/config.php: clear it, so that the old"
	say "plugin code is used."
fi
say "The backup was moved to $done_dir; delete it once it is no longer needed."
