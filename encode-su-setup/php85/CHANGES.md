# Changes to the vBulletin 4.2.6 scripts for PHP 8

Commit log of `vbulletin-php85.patch`, oldest first. The commits were made in a local git
repository of the forum files, on top of the 2021-03-23 archive.

## PHP 8: define SAPI_NAME before it can be needed; log uncaught throwables

Undefined constants are an Error in PHP 8, so vB_Database::halt() and the
error/exception handlers died when they ran before init.php defined
SAPI_NAME, hiding the original error. The exception handler now also
writes the uncaught throwable (with trace) to the PHP error log.

## PHP 8: replace removed curly-brace string offsets ($str{0} -> $str[0])

## PHP 8: fix compile errors (nested ternaries, continue outside a loop)

The two log_admin_action() nested ternaries were evaluated left-associatively
by PHP 5 (logging the wrong id); they are now parenthesized as intended.

## PHP 8: remove uses of functions/features removed in PHP 7/8

- implode() with reversed arguments (39 calls)
- create_function() -> closures (the two DM lambdas were also called with
  3 arguments, which never worked)
- each() loops in Snoopy -> foreach
- $php_errormsg/track_errors -> error_get_last(); eval() ParseError is caught
  in check_template_errors() so template validation still reports it
- static calls to instance methods (methods made static where they do not
  use $this, vB_Shutdown called through its instance)
- mktime()/gmmktime() without arguments, print_standard_redirect() without
  its required argument (ArgumentCountError)
- PCRE2 (PHP 7.3+) rejects [\w- ] in a character class
- $$a[$i] is evaluated left to right since PHP 7 (attachment.php)
- gmp_random() -> gmp_random_bits()

## PHP 8: undefined constants are an Error

PHP 7 evaluated an undefined constant to its own name (with a warning).
- 56 bareword array keys quoted ($a[key] -> $a['key'], also in "{$a[key]}"),
  which is exactly what PHP 7 did
- bareword strings in other places: callback names, case labels,
  function_exists(), clean_gpc() keys, MASTER_LANGUAGE
- typos that only 'worked' through this conversion: return flase,
  UINT -> TYPE_UINT, strpos(varname, ...) -> $varname, !physicaldel ->
  !$physicaldel, $this->rate.rateid -> $this->rate['rateid'], a case for the
  non-existent VURL_URL_URL constant

## PHP 8: template compiler kept dropping the quotes of array keys

{vb:raw post['onlinestatusphrase']} (e.g. inside {vb:rawphrase ...}) compiled to
$post[onlinestatusphrase]: the curly lexer strips the quotes from string
tokens and simple_var() glued the stripped value back in. PHP 7 read the
bareword as a string; PHP 8 throws Error. Keep the original quoted text.
Templates compiled earlier are fixed in the database by vb_php8_db_fix.php.

## PHP 8: built-in XMLParser class clashes with vB's legacy XMLparser alias

Class names are case-insensitive; declaring XMLparser is a fatal error on
PHP 8 (search.php, tags.php). The alias is unused; declare it only when
the name is free.

## PHP 8: runtime fixes found by testing; no compile-time deprecations left

Runtime (found with page comparisons against PHP 5.6, write-path and Admin CP tests):
- vbulletin_error_handler(): PHP 8 turned many notices into warnings (undefined
  variables/array keys/properties, offsets on null/false, ...). vBulletin runs
  with notices off, so treat them as notices again; they were flooding the logs
  and were printed into pages that enable display_errors (acp/attachment.php).
  Deprecations are only shown/logged with SHOW_DS_ERRORS, as vB intended.
  E_STRICT (a deprecated constant in PHP 8.4) is referenced by value.
- @mb_*() calls relied on getting false for an unknown/empty charset; PHP 8
  throws ValueError (user creation from the CLI died in vbstrlen()).
  vb_mb_encoding_ok() checks first; behaviour is unchanged.
- vb_number_format(): number_format() throws TypeError for "0.29," (load
  averages on the Admin CP home); use the leading number like PHP 5 did.
- acp/index.php navprefs: $string[] = ... is an Error since PHP 7.1.
- vB_XML_Parser handlers took the parser by reference; PHP 8 passes the
  XMLParser object by value (thousands of warnings per page).
- WOLPATH defined twice (a warning since PHP 8); vB_Collection destructured
  an int (warning in PHP 8.5) -- same values as before.

Compile time (now none left in the active code), keeping PHP 5.6 syntax:
- non-canonical casts (integer)/(boolean)/(double)
- "case x;" / "default;"
- optional parameters before required ones: the never-usable default removed
- implicitly nullable "Type $x = null": the type is dropped ("?Type" would
  need PHP 7.1)
- "${expr}" string interpolation

## PHP 8: keep PHP 7 results where a number meets '' or a non-numeric string

PHP 8 compares 0 with '' or 'abc' as strings, so '' == 0 became false. Sites
found by page diffs against PHP 5.6 and by tracing comparisons at run time now
use vb_loose_equals()/vb_loose_in_array() (PHP 7 rules) or intval():
form helpers (pre-selected options/radios), vB_Session::set() change tracking,
friendly URL unicode mode '' (= ignore), profile options checkboxes,
print_cells_row(), the private message/subscription folder jump, the search
prefix selector and the 'Use Default Style' default for new forums.
report.php: array & int is a TypeError now (no valid post).
adminfunctions.php also guards CP constants that can be defined twice.

## PHP 8: warnings outside vB's error handler, errors found by crawling and cron runs

- Superglobal reads before init.php installs vbulletin_error_handler: vB sets
  error_reporting(E_ALL & ~E_NOTICE), but undefined array keys are E_WARNING in
  PHP 8 and were logged on every request (member.php, newreply.php, search.php,
  attachment.php, css.php, api.php and the API whitelists, global.php ...).
  isset()/empty() guards keep the PHP 5 values.
- Constants that can be defined twice in one request (NOPMPOPUP, NOSHUTDOWNFUNC,
  VB_ERROR_PERMISSION, VB_ERROR_LITE, AS_PROFILE): redefinition is a warning now.
- "continue" inside switch (inlinemod.php, forumrunner search) -> break, which is
  what it always did.
- Forum Runner cron scripts: define(MCWD, ...) with a bareword name is an Error.
- Thread search (search.php?searchthreadid=): writes to an undefined $vbulletin
  had no effect in PHP 5 and are an Error in PHP 8; commented out.

## PHP 8: writes through undefined variables (found testing mail and by PHPStan)

PHP 5 silently created an object for $undefined->prop = ... or =& $undefined->prop;
PHP 8 throws an Error.
- sendmessage.php: $url =& $vBulletin->url (typo) killed the Contact Us form;
  now $vbulletin->url, so the 'Referring Page' line of the email is filled in
  (it was always empty).
- class_profileblock_blog.php: bloginfo was assigned to the handler before the
  first one existed; same effect as before, without the Error.
- vbcms comments.php: getConfigEditorView() wrote to an undefined $view; the
  object PHP 5 created implicitly is now created explicitly.

## PHP 8 review: signatures, return types, entity-decoding defaults, mysqli exceptions

A second pass over the patched scripts with token-based scanners (method and
callback signatures, PHP 4 constructors, static calls, uniform variable syntax,
foreach pointer use, flags of the html functions):
- vBCMS error controller: getResponse() must accept the parameter of
  vB_Controller::getResponse(); an incompatible signature is a fatal error.
- vB_Collection and vB_dB_Result implement Iterator/ArrayAccess without return
  types: #[\ReturnTypeWillChange] (a comment before PHP 8; PHP 8.1 deprecation).
- Forum Runner called iconv() with iconv_substr()'s arguments: an
  ArgumentCountError in PHP 8. It always failed in PHP 5, which then used the
  next fallback; it is now the iconv_substr() call that was meant.
- PHP 8.1 changed the default flags of html_entity_decode(),
  htmlspecialchars_decode(), htmlentities() and get_html_translation_table()
  from ENT_COMPAT to ENT_QUOTES | ENT_SUBSTITUTE | ENT_HTML401. The 21 calls
  without flags pass ENT_COMPAT.
- PHP 8.1 made mysqli throw mysqli_sql_exception by default, which bypassed
  vB's connection retries, its database error page and halt():
  mysqli_report(MYSQLI_REPORT_OFF), the PHP 5 default.

## PHP 8 review: arrays and objects PHP 5 created or tolerated implicitly

Found by a scope-aware scan for locals that are only built with $x[] = ...,
were last assigned '' or a scalar, or are written as objects without being one;
it now also covers top-level script code and functions that run plugin hooks.
- vb_base64_encode() fallback: $return = '' and then $return[] = ...; PHP 5
  turned '' into an array, PHP 8 throws.
- Admin CP statistics (stats.php, apistats.php): sizeof($results) of the unset
  variable when the date range has no data gave 0 in PHP 5 ("no matches"), a
  TypeError in PHP 8.
- vBCMS recent content widget: count() of the never-set $articles.
- Facebook profile import: implode() of the unset $occupation (PHP 5: NULL).
- vB_Model::writeCache(): a property write on an array, ignored by PHP 5.
- Search result icons: count() of the unset $post_statusicon. Not reachable at
  the moment (the code reads replydata['lastread'], which is never set, so
  'new' is always added), but initialised for when it is.

## PHP 8: sprintf() with phrase formats from the database

vB formats phrases with @call_user_func_array('sprintf', ...). For a bad format
PHP 5 returned false (too few arguments; construct_phrase() then fills in
"[ARG:n UNDEFINED]" or returns the phrase as is) or skipped the bad conversion
specification: a lone '%' in a phrase, '100%' at the end. PHP 8 throws an
ArgumentCountError or ValueError, which @ does not suppress, so an edited or
translated phrase with an extra {n} placeholder or a stray % killed the page.
PHP 8 also reads '%.f' (precision without digits) as 0 digits, PHP 5 as 6, and
understands '*' widths and %h/%H, which PHP 5 printed as nothing.

vb_sprintf_array() (class_core.php) rewrites the specifications PHP 5 did not
understand into '%.0s', which also uses up an argument and prints nothing,
drops a precision without digits, and returns false for the errors. Checked
against PHP 5.6's sprintf() with 400,000 random format strings and argument
lists: identical results (except that PHP 7.1+ reads '1e3' as 1000 in %d).
Used by construct_phrase_from_array() (all phrases, also {vb:rawphrase} in
templates), vB_Phrase and the custom BB code replacement.

## PHP 8: TypeErrors from data: empty timezone offset, calendar recurrence, signature sizes

PHP 8 throws a TypeError when '' or another non-numeric string meets
arithmetic, and compares a number with such a string as strings ('' == 0 is
false), so guards like "== 0" stop catching it. Found by reviewing every
division and by browsing as real members:
- user.timezoneoffset is '' for 53 old accounts. fetch_time_data() computed
  hourdiff with it on every page, so these members got a fatal error on every
  page once logged in; the digest mails (vbdate() with a user's data) had the
  same problem. '' counts as 0 again, as in PHP 5. The time zone select of the
  profile and calendar forms pre-selects GMT for them again (vb_loose_equals()).
- Calendar recurrence: a recuroption with a missing number (only from a
  crafted request) passed the "== 0" checks and reached "% ''".
- Signatures: the [size] option check allows '-', '+', '--1' or '10-'. A
  signature with [size=-] gave an HTTP 500 when saved (int + '-'), and
  '10-' > 7 compared as strings, so the size limit did not apply. The options
  are read as PHP 5 did (the leading number); checked against PHP 5.6 with
  6,632 signatures.
- vBCMS: the rating average divided by the vote count, which is 0 when the
  "votes needed to show the rating" setting is 0 (PHP 5: false, shown as 0).

## PHP 8: more warnings that PHP 5 did not raise are handled as notices

vbulletin_error_handler() already treats the notices PHP 8 promoted to
warnings as notices (hidden, as vB runs with notices off). Added:
"A non-numeric value encountered" (a number with trailing text such as '10px'
in arithmetic: silent in PHP 5, a notice in PHP 7), "String offset cast
occurred" (a notice before PHP 8) and PHP 8.3's warnings about non-numeric
values in array_sum()/array_product().

## PHP 8 review: vB_DM allow-lists, fast path for plain phrase formats

- vB_DM::allowUpdateWithoutCondition()/allowDeleteWithoutCondition() call
  in_array() on properties that default to false and are never overridden.
  PHP 5 returned NULL (vB then threw its own vB_Exception_DM for an update or
  delete without a condition); PHP 8 throws a TypeError instead.
- vb_sprintf_array(): formats with only %s, %d and %1$s style conversions (and
  %%), which PHP 5 and PHP 8 handle alike, go straight to sprintf(). The
  results are unchanged (same 400,000-case comparison with PHP 5.6).
