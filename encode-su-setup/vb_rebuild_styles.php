<?php
// Rebuild all vBulletin 4 styles from the command line, like
// Admin CP > Styles & Templates > Rebuild all styles (acp/template.php?do=rebuild).
// With "store CSS as files" enabled this rewrites clientscript/vbulletin_css/*,
// whose relative url()s are made absolute with the current bburl.
//
// Usage (run as the web server user so it can replace the CSS files later too):
//   runuser -u www-data -- php vb_rebuild_styles.php /var/www/encode.su

if (PHP_SAPI != 'cli')
{
	exit("CLI only\n");
}
if ($argc < 2)
{
	exit("Usage: php {$argv[0]} <forum_dir>\n");
}

// init.php unsets globals named like superglobal keys ($_SERVER['argv'] etc.)
define('CLI_FORUMDIR', rtrim($argv[1], '/'));

error_reporting(E_ALL & ~E_NOTICE & ~E_DEPRECATED & ~2048); // 2048: E_STRICT (PHP 5; the constant is deprecated in PHP 8.4)
chdir(CLI_FORUMDIR);

define('THIS_SCRIPT', 'cli_rebuild_styles');
define('VB_AREA', 'Maintenance');
define('CWD', CLI_FORUMDIR);
define('SKIP_SESSIONCREATE', 1);
define('NOCOOKIES', 1);

$_SERVER['REMOTE_ADDR'] = '127.0.0.1';
$_SERVER['HTTP_HOST'] = 'localhost';
$_SERVER['SERVER_NAME'] = 'localhost';
$_SERVER['REQUEST_METHOD'] = 'GET';
$_SERVER['REQUEST_URI'] = '/' . THIS_SCRIPT . '.php';
$_SERVER['SCRIPT_NAME'] = '/' . THIS_SCRIPT . '.php';
$_SERVER['PHP_SELF'] = '/' . THIS_SCRIPT . '.php';

require_once(CWD . '/includes/init.php');
require_once(DIR . '/includes/adminfunctions.php');
require_once(DIR . '/includes/adminfunctions_template.php');

echo "bburl: " . $vbulletin->options['bburl'] . ", storecssasfile: " . $vbulletin->options['storecssasfile'] . "\n";

// build_all_styles() prints an HTML progress report and flushes it with
// vbflush() (ob_flush), so filter it to plain text in an output callback
function cli_html_to_text($buffer)
{
	$text = strip_tags(str_replace(array('<li>', '<br />', '</p>'), "\n", $buffer));
	$text = preg_replace("/[ \t]+/", ' ', html_entity_decode($text));
	return preg_replace("/\n[ \t]*(?=\n)/", '', $text);
}
ob_start('cli_html_to_text');
$err1 = build_all_styles(0, 0, '', false, 'standard');
$err2 = build_all_styles(0, 0, '', false, 'mobile');
ob_end_flush();
echo "\n";

if ($err1 OR $err2)
{
	echo "Error: $err1 $err2\n";
	exit(1);
}
echo "Done.\n";
