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

