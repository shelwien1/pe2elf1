# vBulletin 4.2.6 (encode.su) on PHP 8.5

vBulletin 4 was written for PHP 5 and does not run on PHP 8: the forum dies on the
first page with fatal errors. This directory ports the encode.su installation to
PHP 8.5. Everything here also runs on PHP 5.6 (PHP 5.6 syntax only), so it can be
applied before PHP is upgraded.

| File | Purpose |
|---|---|
| `vbulletin-php85.patch` | ports the forum scripts: 186 PHP files, about 890 changed lines |
| `vb_php8_db_fix.php` | ports the PHP code stored in the database: plugins, the plugin cache, compiled templates, two setting checks |
| `php8_barewords.php` | tokenizer-based quoting of undefined constants, used by `vb_php8_db_fix.php` |
| `php_constants.txt` | names of PHP's own constants (5.6 and 8.5), used by `vb_php8_db_fix.php` |
| `php85fix.sh`, `php85undo.sh` | apply the patch and the database fix to a forum in place, with a backup, and undo them (see below) |
| `CHANGES.md` | commit log of the patch, one commit per topic |
| `cmp7trace/` | diagnostic PHP extension used for testing (see below); not needed to run the forum |

`../setup.sh` applies the patch and the database fix to the local mirror. The
patch is made against the files in the 2021-03-23 archive.

## Applying it to the live forum

`php85fix.sh` backs up and patches a forum in place, and `php85undo.sh` puts it
back. Copy these files to a directory outside the web root, because the backup
holds a dump of the whole database. Next to the forum directory works, `tmp5/`
in this example:

* `php85fix.sh`, `php85undo.sh`
* `vbulletin-php85.patch`, `vb_php8_db_fix.php`, `php8_barewords.php`,
  `php_constants.txt`

Then run it from the forum's root directory, the one with `global.php`:

```sh
cd encode.su
sh ../tmp5/php85fix.sh
```

It needs `patch`, `tar`, `gzip`, `mysql`, `mysqldump` and a PHP 5.6 or later
command line binary with `mysqli`. To use another binary than `php`, run
`PHP=/path/to/php sh ../tmp5/php85fix.sh`. It works in four steps:

1. **Checks.** The patch must apply to the scripts, and the database login in
   `includes/config.php` must work. Files of the patch that do not exist on the
   site, such as a deleted `forumrunner/`, are skipped. A script that was edited
   since 2021 in a line the patch changes stops it, and the log names the file.
   If a check fails, nothing is changed.
2. **Backup** to `../tmp5/php85-backup/`: the scripts that the patch changes
   (`files.tar.gz`) and a dump of the whole database (`database.sql.gz`).
3. **Scripts.** The patch is applied.
4. **Database.** The PHP code stored in the database is fixed: plugins, the
   plugin cache, compiled templates and two setting checks. The old value of
   every field that changes is recorded in `database-undo.dat`.

If step 3 or 4 fails, the scripts and the database values are put back. The
backup is then kept as `php85-backup.failed-<date>`, with the log
`php85fix.log`. If the script was stopped half way (by a power cut, say),
`php85undo.sh --force` puts back whatever it changed.

The tables are locked while they are dumped, for seconds to minutes depending on
the size of the database. Run it when the forum is quiet, or turn the forum off
for the switch (Admin CP > Settings > Options > Turn Your vBulletin On and
Off). The dump is written unpacked and then compressed. Before it starts, the
script checks that there is free space for twice the size of the table data:
about 680 MB for the October 2025 database, whose compressed dump is 150 MB.

Then switch the site to PHP 8.5. The patched scripts also run on PHP 5.6, so
this can wait.

* The forum needs the `mysqli`, `mbstring`, `gd`, `xml` and `curl` extensions.
* Keep `zend.assertions = -1`, the production default. With assertions
  enabled, PHP 8 throws where a failed `assert()` in vB's code only warned in
  PHP 5.
* A datastore cache (memcache, APC, file cache) in `includes/config.php` has to
  be cleared, so that the fixed plugin code is used. The script says so when one
  is configured.

### Undoing it

Switch the site back to PHP 5.6 first, because the original scripts do not run
on PHP 8. Then run, from the forum root:

```sh
sh ../tmp5/php85undo.sh
```

It restores the scripts from `files.tar.gz` and puts back the old values of the
database fields that `php85fix.sh` changed. Posts, users and everything else
written since are kept. If a patched script or one of those database values
was changed after `php85fix.sh` ran, it lists them and changes nothing. With
`--force`, it overwrites them. Afterwards the backup is renamed to
`php85-backup.undone-<date>`, so `php85fix.sh` can run again.

`sh ../tmp5/php85undo.sh --full-db` loads the complete dump instead. The
database is then exactly as it was before `php85fix.sh`, and everything written
since is lost. The current database is dumped to
`database-before-undo-<date>.sql.gz` first. That dump and the unpacked old one
need about twice the size of the table data again, which is checked first.

### By hand

The scripts use the patch and the database tool, which can also be run
directly. `includes/config.php` supplies the database login. Without `--apply`,
the tool only lists what it would change:

```sh
cd /path/to/forum
patch -p1 --binary --dry-run < vbulletin-php85.patch
patch -p1 --binary < vbulletin-php85.patch
php vb_php8_db_fix.php /path/to/forum
php vb_php8_db_fix.php /path/to/forum --apply
```

Running the tool again is safe: the second run finds nothing to change. With
`--apply --undo-file FILE`, it records the old values, and `--restore FILE` puts
them back.

Later changes stay PHP 8 compatible on their own:

* **Templates** saved in the Admin CP compile correctly, because the template
  compiler is fixed.
* **Plugins and products** installed later have to be PHP 8 clean. Write
  `$vbulletin->options['x']`, not `$vbulletin->options[x]`, or run the database tool
  again after installing them.

## What was changed in the scripts

**Removed syntax and functions**
* `$str{0}` string offsets became `$str[0]`.
* Unparenthesised nested ternaries got parentheses.
* `continue` inside a `switch` became `break`, which is what it always did. For
  the one `continue` outside any loop (vBCMS), see the behaviour changes below.
* `implode()` calls with reversed arguments (39) were put in the right order.
* `create_function()` became closures.
* `each()` became `foreach`.
* `$php_errormsg`/`track_errors` became `error_get_last()`.
* `gmp_random()` became `gmp_random_bits()`.
* `mktime()` without arguments was fixed.
* The PCRE2 character class `[\w- ]` was fixed.
* `$$a[$i]` (attachment.php) was rewritten.
* Static calls to instance methods were fixed.
* A vBCMS controller method whose signature did not match its parent (a fatal
  error in PHP 8) was fixed, and the Iterator/ArrayAccess methods of two
  classes got `#[\ReturnTypeWillChange]` (a comment before PHP 8).

**Undefined constants are an Error.** Bareword array keys such as
`$vbulletin->options[foo]` and `define(MCWD, ...)` were quoted. The template
compiler dropped the quotes of `{vb:raw var['key']}` keys and now keeps them.

**Type errors**
* `number_format()` on strings like `"0.29,"`, `array & int`, `$string[] = ...`.
* `mb_*()` with an unknown or empty charset, which is now a ValueError and was
  `false` before.
* PHP 8.5's built-in `XMLParser` class clashes with vB's `XMLparser` alias.
* The XML parser handlers took the parser by reference.
* Writes through undefined variables. PHP 5 silently created an object; PHP 8
  throws an Error. Affected: the thread search (`$vbulletin`), the Contact Us form
  (`$vBulletin`), the blog profile block and the vBCMS comment editor.
* Arrays that were never initialised: `count()`/`sizeof()`/`implode()` of an
  unset variable (the Admin CP statistics for a date range without data, among
  others) and `''` used as an array.
* `sprintf()` with phrase formats from the database. PHP 8 throws for too few
  arguments or a stray `%` in a phrase, where PHP 5 returned false or skipped
  the bad part. `vb_sprintf_array()` (in `class_core.php`) gives the PHP 5
  results, including PHP 5's conversion of numbers (`'1e3'` is 1 for `%d`);
  `construct_phrase()`, `vB_Phrase` and custom BB codes use it.
* String offsets past the end and negative offsets. PHP 8 throws a ValueError
  for a `strpos()` offset past the end of the string, where PHP 5 returned
  false, and reads negative string offsets from the end:
  * A posted message with an attribute without a value (`<a href=>`) was a
    fatal error. Any member could send it; the editor never produces it.
  * Old-style template code ending in such an attribute or in `<if condition="`
    (Admin CP template input).
  * `vb_unserialize()` with a negative string length in crafted data recursed
    until memory ran out.
* Searching for an empty string finds it everywhere in PHP 8 and nowhere in
  PHP 5: the `[url]` nofollow host check and the Admin CP error log viewer
  (only with settings that are off on this forum).
* Values from the database or from users. `''` or `'-'` in arithmetic and
  division by zero are errors in PHP 8:
  * `user.timezoneoffset` is `''` for 53 old accounts. These members got a fatal
    error on every page once logged in.
  * A signature with `[size=-]` could not be saved (HTTP 500), and `[size=10-]`
    got past the size limit.
  * Calendar recurrence options with a missing number (crafted requests only).
  * The vBCMS rating average divided by 0 when the setting for the number of
    votes is 0.

**Changed defaults**
* `html_entity_decode()`, `htmlspecialchars_decode()`, `htmlentities()` and
  `get_html_translation_table()` default to `ENT_QUOTES | ENT_SUBSTITUTE` since
  PHP 8.1. The 21 calls without flags pass `ENT_COMPAT`, the PHP 5 default.
* mysqli throws exceptions by default since PHP 8.1, which bypassed vB's own
  database error handling (retries, the "Database error" page). It is switched
  back to the PHP 5 default.

**Comparisons.** PHP 8 compares a number with a non-numeric string as strings, so
`'' == 0` is false now. Places where that changed what the forum does now use
`vb_loose_equals()`/`vb_loose_in_array()` (PHP 7 rules, in `class_core.php`) or
`intval()`. They were found by page diffs, by tracing and by the review (see
below):
* pre-selected options and radio buttons in the Admin CP forms
* session change tracking
* the friendly-URL unicode setting
* user option checkboxes
* folder menus
* the search prefix selector
* the default style of a new forum
* the time zone select for accounts with an empty time zone
* the calendar's checks of recurrence options

**Error handling** (`vbulletin_error_handler`)
* vBulletin runs with notices off. PHP 8 made many notices into warnings
  (undefined variables, array keys and properties, offsets on null). Those are
  treated as notices again.
* Deprecations are only shown or logged with `SHOW_DS_ERRORS`, as vB intends.
* Uncaught exceptions and errors are logged.
* Warnings that PHP 5 did not raise at all, such as "A non-numeric value
  encountered" for `'10px' + 1` and PHP 8.3's warnings from `array_sum()`, are
  treated as notices too.
* Code that runs before vB installs its handler got `isset()` guards: superglobal
  reads at the top of entry scripts, `global.php`, the API files.
* Constants that can be defined twice in one request are guarded, for the same
  reason.

**Compile-time deprecations.** None are left. The fixes:
* `(integer)`, `(boolean)` and `(double)` casts
* `case x;`
* optional parameters before required ones; the never-usable default was removed
* implicitly nullable `Type $x = null`; the type was dropped, because `?Type`
  needs PHP 7.1
* `"${expr}"` interpolation

`CHANGES.md` has the commit log of the port, one commit per topic.

### Behaviour changes on purpose

Everything else keeps the PHP 5 behaviour. A few lines were already broken under
PHP 5, and the port makes them do what they were written to do:

* **Inline moderation:** `!physicaldel` was a bareword, so always false. It became
  `!$physicaldel` in `inlinemod.php` and in Forum Runner's moderation code.
* **`vb/profilecustomize.php`:** `strpos(varname, 'background')` became
  `strpos($varname, ...)`. Background style variables are grouped as intended.
* **`packages/vbforum/taggablecontent/picture.php`:** the input type `UINT` became
  `TYPE_UINT`.
* **`class_dm_stylevar.php`:** `return flase;`, a truthy string, became
  `return false;`.
* **Two Admin CP `log_admin_action()` calls:** they used unparenthesised nested
  ternaries, which PHP 5 grouped left to right. They now log the intended text.
* **`attachment.php`:** `$$arrayname[$$index]` became `${$arrayname}[$index]`. It
  sets the who's-online thread/forum for attachment views, which never worked.
* **Contact Us emails:** the "Referring Page" line was always empty because of a
  `$vBulletin` typo. It now shows the page.
* **Forum Runner:** `iconv()` was called with `iconv_substr()`'s arguments and
  always failed. It is now `iconv_substr()`. It only runs when `mb_substr()` is
  not available.
* **vBCMS search results:** a `continue` outside a loop became `return false`.
  Unpublished articles are skipped. Before, PHP 5 hit a fatal error when the line
  ran, and since PHP 7 the file does not compile at all.
* **Function parameters:**
  * a default that could never be used, because it came before a required
    parameter, was removed
  * implicitly nullable typed parameters (`Type $x = null`) lost the type

Admin CP plugin pages show highlighted PHP code in PHP 8.3's new
`highlight_string()` markup (`<pre>`). The code itself is the same.

### Not changed

* `acp0/` and `mcp0/` are old copies of the Admin and Moderator CP that nothing
  links to. They were not ported and fail on PHP 8. Consider deleting them on
  the server.
* The mobile API (`api.php`, disabled in the settings) was only made quiet.
  `xmlrpc_*` functions are gone in PHP 8.
* The `mysql_*` database driver was removed in PHP 7 and is not used:
  `config.php` selects `mysqli`.
* Disabled products, which stay disabled:
  * The plugin of the `stg_table` product (a table BB code) uses `''` as an
    array and fails on PHP 8 if the product is enabled again. vBulletin 4.2 has
    its own `[TABLE]` code, which is the one in use. Uninstall the product.
  * Templates of disabled products (style ids -10 and -20) were compiled by an
    older vBulletin and call `htmlspecialchars()` without flags. Rebuild the
    styles after enabling such a product.
* Settings that only an administrator can set to unusable values: a blog
  "per page" option of 0 divides by zero (PHP 5 printed warnings and showed
  an empty list; PHP 8 stops).
* Bugs that behave the same on PHP 5.6:
  * cron tasks `blog_cleanup` and `blog_pending` fail (SQL error; the blog
    product's bitfields are missing)
  * `vb/validate.php` uses `$this` in static methods
  * a regex in `class_bbcode_alt.php`
  * the ICQ regex without a closing delimiter
  * the status icons of search results are always "new" (`lastread` is read,
    `readtime` is set)
  * two vBCMS collection queries use `$this->itemdid` (a typo). They fail on both
    versions: an SQL error on PHP 5, a TypeError on PHP 8.
  * Forum Runner's text parser calls its parent constructor by the old name
    (`parent::StringParser_Node()`), so it fails on both versions.

## How it was tested

* **Reference instance.** The unmodified scripts run on PHP 5.6 (port 8056)
  against the same database as the patched scripts on PHP 8.5. The normalised
  HTML of every page below was compared. The only differences are volatile ones:
  online counts, timers, search ids, timestamps, the PHP version and phpinfo.
  * **Guest:** 75 pages, 91 error and invalid-parameter URLs, and 135 pages found
    by a crawler (one per script/action).
  * **Logged-in member:** 43 pages, the same 91 error URLs, and 151 crawled pages.
  * **Admin CP:** 139 navigation pages, plus 132 options groups and edit forms.
* **Real members.** The tests above use test accounts with clean data. The
  scripts were also run as 612 real accounts, by inserting vB sessions: the 53
  with an empty time zone, staff, banned users, users with unusual options,
  empty or year-less birthdays and read markers, top posters and 300 random
  members. 13 pages each (forum home, user CP, option and profile forms, own
  profile, private messages, subscriptions, new posts, post and thread search
  results, a forum and a thread they read): 8,073 pages, identical apart from
  volatile parts.
  With the time zone fix taken out, every page of the 53 accounts failed.
* **Write paths** (18 tests): new thread, reply and quoted reply, AJAX quick
  reply, edit with edit log, Thanks add/remove, private messages, subscriptions,
  search, tags, a poll with a vote, attachment upload with a GD thumbnail,
  moderation, logout.
* **Admin CP** (30 tests): every options group, forum, usergroup and user forms
  submitted unchanged must leave the database unchanged. Also template save,
  template syntax-error rejection, plugin save, style rebuild, counters, user
  search, statistics, running a cron task, avatar upload, registration with
  COPPA and question verification, signatures, reports. After the review, the
  Admin CP tests were run from the same database snapshot on PHP 5.6 and on
  PHP 8.5, with identical results. On a freshly imported database a few forms
  store `0` for settings that are empty, on both versions.
* **Cron.** All 28 active cron tasks, forced due, on PHP 8.5 and on PHP 5.6. Same
  results.
* **Email.** Mail went through vBulletin's SMTP class to a local SMTP sink:
  * the Admin CP mail test
  * Contact Us
  * a mail queue batch of digests, birthday greetings and contact messages
* **Runtime tracing.** A small PHP extension hooks `==`, `!=`, `<`, `<=`,
  `switch`, `in_array()` and `array_search()`. It logged every comparison whose
  result differs between PHP 7 and PHP 8 rules while all of the above ran. None
  are left. The source is in `cmp7trace/`.
* **PHP 5.6.** The patched scripts on PHP 5.6 give the same pages as the original
  scripts on PHP 5.6.
* **Fuzzing.** The same random input on PHP 5.6 and on PHP 8.5, with identical
  results:
  * `vb_sprintf_array()` against PHP 5.6's `sprintf()`: 700,000 random formats
    and argument lists
  * the signature parser: 6,632 `[size]` combinations
  * the WYSIWYG and old-style template attribute parsers, the template
    conditional parser and `vb_unserialize()`: 40,000 to 400,000 inputs each
* **Static checks.**
  * PHPCompatibility (phpcs 4)
  * PHPStan levels 0 and 1, reviewed for errors that are fatal only in PHP 8
  * token-based scanners for barewords, curly offsets and comparisons
  * a second pass with token-based scanners: method and callback signatures,
    PHP 4 constructors, static calls, uniform variable syntax, `foreach` and the
    array pointer, flags of the html functions, `func_get_args()`, functions
    removed in PHP 7 and 8, arrays and objects created implicitly (also in
    top-level script code and functions that run plugin hooks)
  * every division and modulo (85 sites), and arguments that are a ValueError
    in PHP 8 but only a warning in PHP 5: `strpos()` offsets, empty needles,
    `str_repeat()` counts, `max()` of empty arrays, `mt_rand()` ranges,
    `array_combine()` sizes, `explode()` delimiters
  * the database: string columns that hold numbers were checked for `''`
    (only `user.timezoneoffset`); the PHP code stored in it (plugins, setting
    validation and option code, product install code, 2,437 compiled templates)
    was linted on both versions and run through the same scanners
  * a `php -l` lint of every file on PHP 8.5 with all warnings enabled; the
    changed files were also linted on PHP 5.6
* **Logs.** The PHP 8.5 error log stays empty: no fatal errors, warnings or
  deprecations. The only entry is the expected SMTP timeout, because this
  container cannot send mail.
* **`php85fix.sh` and `php85undo.sh`** were run on a copy of the 2021 files with
  the October 2025 database:
  * After the fix, the 186 scripts are byte-identical to the patched mirror,
    and the plugins and templates match its database. 75 guest pages are
    identical on PHP 5.6 and PHP 8.5.
  * After the undo, every PHP file is identical to the original, and so are the
    plugin, template and setting tables and the plugin cache. After
    `--full-db`, all 259 tables are (`CHECKSUM TABLE`).
  * 14 failure cases are refused or rolled back with nothing changed: the
    wrong directory, the kit inside the web root, undo without a backup, the
    fix run twice, a script or a database value changed after the fix
    (`--force` restores anyway), a script the patch does not fit, scripts
    patched by hand, a database user who may not update templates (the fix
    fails half way and is rolled back), a dump that fails half way, too little
    disk space for the fix or for `--full-db`, a `config.php` that names
    another database, and a `--full-db` load that fails half way (a second run
    then restores everything). A deleted `forumrunner/` and `PHP=php5.6` work.
