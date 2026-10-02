#!/bin/sh
# php85fix.sh - make the vBulletin 4 forum in the current directory run on PHP 8.5.
#
# Run it from the forum's root directory (the one with global.php), for example
#
#     cd encode.su && sh ../tmp5/php85fix.sh
#
# The directory this script is in (the kit, ../tmp5 here) must also hold
# vbulletin-php85.patch, vb_php8_db_fix.php, php8_barewords.php and php_constants.txt.
# The backup goes to php85-backup/ in the kit, so the kit must not be inside the web root:
# the backup holds a dump of the whole database.
#
# What it does:
#   1. checks that the patch applies and that the database can be reached;
#      nothing is changed if a check fails
#   2. backs up the forum files the patch changes and dumps the whole database
#   3. patches the forum scripts
#   4. fixes the PHP code stored in the database (plugins, compiled templates, the
#      validation code of a setting), recording the old value of every field it changes
# If step 3 or 4 fails, the files and the database values are put back.
# php85undo.sh, run the same way, undoes everything.
#
# The database tables are locked while they are dumped (seconds to minutes, depending on
# the size of the database), and the scripts change within a second: run it when the forum
# is quiet, or turn the forum off (Admin CP > Settings > Options) for the switch.
#
# PHP=/path/to/php selects the PHP command line binary for step 4 (default: php; PHP 5.6
# or later, with mysqli). The patched scripts run on PHP 5.6 to 8.5.

set -u
umask 077

KIT=$(cd "$(dirname "$0")" && pwd -P) || exit 1
FORUM=$(pwd -P)
BACKUP=$KIT/php85-backup
PHP=${PHP:-php}
CNF=$KIT/.php85-mysql.cnf
PATCH=$KIT/php85fix-work.patch
CHECK=$KIT/php85fix-check.log
MISSING=$KIT/php85fix-missing.txt

say() { printf '%s\n' "$*"; }
die() { printf 'php85fix: %s\n' "$*" >&2; exit 1; }

trap 'rm -f "$CNF" "$PATCH" "$MISSING"' EXIT
trap 'exit 1' HUP INT TERM

# ---- checks: nothing is changed until all of them pass --------------------------------------

[ -f global.php ] && [ -f includes/config.php ] && [ -f includes/class_core.php ] ||
	die "run this from the forum's root directory (the one with global.php and includes/)"
for f in vbulletin-php85.patch vb_php8_db_fix.php php8_barewords.php php_constants.txt; do
	[ -f "$KIT/$f" ] || die "$KIT/$f is missing"
done
case "$KIT/" in
	"$FORUM/"*) die "the kit ($KIT) is inside the forum directory; move it out of the web root, the backup holds a database dump" ;;
esac
for tool in patch tar gzip cksum cmp awk mysql mysqldump; do
	command -v "$tool" >/dev/null 2>&1 || die "'$tool' is not installed"
done
"$PHP" -r 'exit(version_compare(PHP_VERSION, "5.6.0", ">=") && function_exists("mysqli_init") && function_exists("token_get_all") ? 0 : 1);' >/dev/null 2>&1 ||
	die "'$PHP' is not a PHP 5.6+ command line binary with the mysqli and tokenizer extensions; set PHP=/path/to/php"
if [ -e "$BACKUP" ]; then
	die "$BACKUP exists, so the fix has been applied already. Run php85undo.sh first, or move that directory away."
fi

say "Checking the forum scripts ..."
# Only the files that exist here: a site may have deleted unused parts (forumrunner/, say).
: > "$CHECK" && : > "$MISSING" || die "cannot write to $KIT"
LC_ALL=C awk -v missing="$MISSING" '
	/^diff --git a\// { f = substr($3, 3); keep = (system("test -f \"" f "\"") == 0); if (!keep) print f >> missing }
	keep { print }
' "$KIT/vbulletin-php85.patch" > "$PATCH" || die "cannot read $KIT/vbulletin-php85.patch"
missing=$(wc -l < "$MISSING")
for f in global.php includes/class_core.php includes/functions.php; do
	grep -q "^$f\$" "$MISSING" && die "$f is in the patch but missing here"
done

PATCHOPTS="-p1 --binary -f"
patch --version 2>/dev/null | grep -q 'GNU patch' && PATCHOPTS="$PATCHOPTS --no-backup-if-mismatch"
# shellcheck disable=SC2086
if patch $PATCHOPTS -R -s --dry-run < "$PATCH" >/dev/null 2>&1; then
	die "the forum scripts contain the PHP 8.5 patch already"
fi
# shellcheck disable=SC2086
if ! patch $PATCHOPTS -N --dry-run < "$PATCH" >> "$CHECK" 2>&1; then
	grep -i -E 'FAILED|can.t find|Reversed|malformed' "$CHECK" | head -n 20 >&2
	die "the patch does not apply to these files; they differ from the vBulletin 4.2.6 files of 2021 (details: $CHECK). Nothing was changed."
fi
nfiles=$(grep -c '^checking file' "$CHECK")
say "  $nfiles scripts can be patched"
if [ "$missing" -gt 0 ]; then
	say "  $missing file(s) of the patch do not exist here and are skipped (listed in the log)"
fi
nfuzz=$(grep -c -E 'with fuzz|offset' "$CHECK")
if [ "$nfuzz" -gt 0 ]; then
	say "  $nfuzz change(s) apply at a different line (the files were edited since 2021; see the log)"
fi

say "Checking the database ..."
DBNAME=$("$PHP" "$KIT/vb_php8_db_fix.php" "$FORUM" --client-config "$CNF" 2>>"$CHECK") && [ -n "$DBNAME" ] ||
	die "cannot read the database settings from includes/config.php (see $CHECK)"
mysql --defaults-extra-file="$CNF" -e 'SELECT 1' "$DBNAME" >/dev/null 2>>"$CHECK" ||
	die "cannot log in to the database '$DBNAME' (see $CHECK)"
"$PHP" "$KIT/vb_php8_db_fix.php" "$FORUM" > "$CHECK.db" 2>&1 ||
	die "the check of the PHP code in the database failed (see $CHECK.db)"
nitems=$(sed -n 's/^\([0-9][0-9]*\) item(s) .*/\1/p' "$CHECK.db")
say "  ${nitems:-0} item(s) of PHP code in the database '$DBNAME' to fix"
nunsafe=$(grep -c 'could not fix safely' "$CHECK.db")
if [ "$nunsafe" -gt 0 ]; then
	say "  $nunsafe item(s) cannot be fixed automatically and will stay as they are (see the log)"
fi

# Room for the backup: the dump is about 1.2 times the size of the table data, and it is
# written unpacked and then compressed (to about half of that).
dbkb=$(mysql --defaults-extra-file="$CNF" -N -e 'SELECT CEIL(SUM(data_length) / 1024) FROM information_schema.tables WHERE table_schema = DATABASE()' "$DBNAME" 2>/dev/null)
freekb=$(df -Pk "$KIT" 2>/dev/null | awk 'NR == 2 { print $4 }')
case "$dbkb:$freekb" in
	*[!0-9:]*|:*|*:) say "  (could not check the free disk space for the backup)" ;;
	*) [ "$freekb" -ge $((dbkb * 2)) ] ||
		die "not enough disk space in $KIT for the database dump: about $((dbkb * 2 / 1024)) MB are needed, $((freekb / 1024)) MB are free. Nothing was changed." ;;
esac

DUMPOPTS="--quick --default-character-set=utf8mb4"
for o in --no-tablespaces --set-gtid-purged=OFF --column-statistics=0; do
	mysqldump --help 2>/dev/null | grep -q -- "${o%%=*}" && DUMPOPTS="$DUMPOPTS $o"
done

# ---- backup ---------------------------------------------------------------------------------

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

mkdir "$BACKUP" || die "cannot create $BACKUP"
LOG=$BACKUP/php85fix.log
{
	if [ "$missing" -gt 0 ]; then
		say "Files of the patch that do not exist here (skipped):"
		cat "$MISSING"
	fi
	cat "$CHECK" "$CHECK.db"
} > "$LOG"
rm -f "$CHECK" "$CHECK.db"
{
	say "forum=$FORUM"
	say "database=$DBNAME"
	say "date=$(date '+%Y-%m-%d %H:%M:%S')"
	say "php=$("$PHP" -r 'echo PHP_VERSION;')"
} > "$BACKUP/state"

fail_unchanged() {
	failed=$BACKUP.failed-$(date +%Y%m%d-%H%M%S)
	mv "$BACKUP" "$failed" 2>/dev/null || failed=$BACKUP
	die "$1 Nothing was changed (log: $failed/php85fix.log)."
}

say "Backing up ..."
sed -n 's#^+++ b/##p' "$PATCH" | tr -d '\r' > "$BACKUP/files.txt"
[ -s "$BACKUP/files.txt" ] || fail_unchanged "cannot read the file names from the patch."
cp "$PATCH" "$BACKUP/applied.patch" || fail_unchanged "cannot write to $BACKUP."
tar -czf "$BACKUP/files.tar.gz" -T "$BACKUP/files.txt" 2>>"$LOG" ||
	fail_unchanged "the backup of the forum files failed."
xargs cksum < "$BACKUP/files.txt" > "$BACKUP/files-before.cksum" 2>>"$LOG" ||
	fail_unchanged "cannot read the forum files."
dump_db "$BACKUP/database.sql" || fail_unchanged "the database dump failed."
say "  $(wc -l < "$BACKUP/files.txt") scripts and the database '$DBNAME' are backed up in $BACKUP"
say "step=backed-up" >> "$BACKUP/state"

# ---- changes ----------------------------------------------------------------------------------

rollback() {
	say "$1 Putting everything back ..." >&2
	ok=1
	if [ -s "$BACKUP/database-undo.dat" ]; then
		"$PHP" "$KIT/vb_php8_db_fix.php" "$FORUM" --restore "$BACKUP/database-undo.dat" --force >>"$LOG" 2>&1 || ok=0
	fi
	tar -xzf "$BACKUP/files.tar.gz" 2>>"$LOG" || ok=0
	if [ "$ok" = 1 ] && xargs cksum < "$BACKUP/files.txt" 2>>"$LOG" | cmp -s - "$BACKUP/files-before.cksum"; then
		failed=$BACKUP.failed-$(date +%Y%m%d-%H%M%S)
		mv "$BACKUP" "$failed"
		die "The files and the database are as they were before (log: $failed/php85fix.log)."
	fi
	say "step=rollback-failed" >> "$BACKUP/state"
	die "Putting things back failed too. Run: sh ${0%php85fix.sh}php85undo.sh --force (log: $LOG)"
}

# the changes take a few seconds: do not stop half way
trap '' HUP INT TERM

say "Patching the forum scripts ..."
# shellcheck disable=SC2086
patch $PATCHOPTS -N < "$PATCH" >>"$LOG" 2>&1 || rollback "Patching the forum scripts failed."
xargs cksum < "$BACKUP/files.txt" > "$BACKUP/files-after.cksum" 2>>"$LOG"
say "step=patched" >> "$BACKUP/state"

say "Fixing the PHP code in the database ..."
"$PHP" "$KIT/vb_php8_db_fix.php" "$FORUM" --apply --undo-file "$BACKUP/database-undo.dat" >>"$LOG" 2>&1 ||
	rollback "Fixing the PHP code in the database failed."
say "step=done" >> "$BACKUP/state"

trap 'exit 1' HUP INT TERM

say ""
say "Done: $nfiles scripts patched, ${nitems:-0} item(s) of PHP code in the database fixed."
say "Backup and log: $BACKUP"
say ""
say "Now switch the site to PHP 8.5. The patched scripts also run on PHP 5.6."
if grep -q '^Note: a datastore cache' "$LOG"; then
	say "A datastore cache is configured in includes/config.php: clear it, so that the fixed"
	say "plugin code is used."
fi
say "To undo everything: sh ${0%php85fix.sh}php85undo.sh (from this directory)."
say "The backup contains a dump of the whole database: keep it private, and delete it once"
say "it is no longer needed."
