#!/usr/bin/env bash
# End-to-end check of the local encode.su mirror over plain HTTP:
# index -> subforum -> thread -> single post, legacy URLs, attachments, CSS.
set -u
BASE=${BASE:-http://encode.su}
fail=0
body=$(mktemp)
trap 'rm -f "$body"' EXIT

# check <name> <url> <expected status> [<regex the body must match>] [<regex it must not match>]
check() {
	local name=$1 url=$2 want=$3 must=${4:-} mustnot=${5:-} code why=""
	code=$(curl -sS --noproxy '*' --max-time 60 -o "$body" -w '%{http_code}' "$url" 2>/dev/null) || code=000
	if [ "$code" != "$want" ]; then
		why="status $code, expected $want"
	elif [ -n "$must" ] && ! grep -q -E "$must" "$body"; then
		why="body lacks /$must/"
	elif [ -n "$mustnot" ] && grep -q -E "$mustnot" "$body"; then
		why="body contains /$mustnot/"
	fi
	if [ -n "$why" ]; then
		printf 'FAIL  %-22s %s (%s)\n' "$name" "$url" "$why"
		fail=1
	else
		printf 'ok    %-22s %s\n' "$name" "$url"
	fi
}

check "index"              "$BASE/"                                                200 'href="http://encode\.su/forums/2-Data-Compression' 'https://encode\.su'
check "subforum"           "$BASE/forums/2-Data-Compression"                       200 'id="thread_title_[0-9]+"'
check "subforum page 2"    "$BASE/forums/2-Data-Compression/page2"                 200 'id="thread_title_[0-9]+"'
check "thread"             "$BASE/threads/3422-GDC-Competition-Discussions"        200 'id="post_message_[0-9]+"'
check "thread page 2"      "$BASE/threads/3422-GDC-Competition-Discussions/page2"  200 'id="post_message_[0-9]+"'
check "post in thread"     "$BASE/threads/3422-GDC-Competition-Discussions?p=65504" 200 'id="post_message_65504"'
check "showpost redirect"  "$BASE/showpost.php?p=65504"                            301
check "legacy thread URL"  "$BASE/showthread.php?t=3422"                           301
check "legacy forum URL"   "$BASE/forumdisplay.php?f=2"                            301
check "attachment"         "$BASE/attachment.php?attachmentid=7643"                200
check "post_thanks.js"     "$BASE/clientscript/post_thanks.js"                     200 'function post_thanks_give'
check "stylesheet"         "$BASE/clientscript/vbulletin_css/style00002l/main-rollup.css" 200 'http://encode\.su/images/' 'https://encode\.su'
# PHP must be executed, never served as source (that would reveal the database password)
check "PHP runs (no source)" "$BASE/includes/config.php"                         200 '' '\$config\['

if [ "$fail" = 0 ]; then
	echo "All checks passed."
fi
exit $fail
