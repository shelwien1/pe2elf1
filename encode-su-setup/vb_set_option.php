<?php
// Change a vBulletin 4 setting from the command line and rebuild the 'options'
// datastore exactly like the Admin CP does (build_options()).
//
// Usage: php vb_set_option.php /var/www/encode.su bburl http://encode.su
//        php vb_set_option.php /var/www/encode.su bburl          (show only)

if (PHP_SAPI != 'cli')
{
	exit("CLI only\n");
}
if ($argc < 3)
{
	exit("Usage: php {$argv[0]} <forum_dir> <varname> [<new value>]\n");
}

// init.php unsets globals named like superglobal keys ($_SERVER['argv'] etc.),
// so keep the arguments in constants
define('CLI_FORUMDIR', rtrim($argv[1], '/'));
define('CLI_VARNAME', $argv[2]);
define('CLI_SET', $argc > 3);
define('CLI_VALUE', CLI_SET ? $argv[3] : '');

error_reporting(E_ALL & ~E_NOTICE & ~E_DEPRECATED & ~2048); // 2048: E_STRICT (PHP 5; the constant is deprecated in PHP 8.4)
chdir(CLI_FORUMDIR);

define('THIS_SCRIPT', 'cli_set_option');
define('VB_AREA', 'Maintenance');
define('CWD', CLI_FORUMDIR);
define('SKIP_SESSIONCREATE', 1);
define('NOCOOKIES', 1);

// init.php expects a web request environment
$_SERVER['REMOTE_ADDR'] = '127.0.0.1';
$_SERVER['HTTP_HOST'] = 'localhost';
$_SERVER['SERVER_NAME'] = 'localhost';
$_SERVER['REQUEST_METHOD'] = 'GET';
$_SERVER['REQUEST_URI'] = '/' . THIS_SCRIPT . '.php';
$_SERVER['SCRIPT_NAME'] = '/' . THIS_SCRIPT . '.php';
$_SERVER['PHP_SELF'] = '/' . THIS_SCRIPT . '.php';

require_once(CWD . '/includes/init.php');
require_once(DIR . '/includes/adminfunctions.php');

$setting = $vbulletin->db->query_first("
	SELECT varname, value FROM " . TABLE_PREFIX . "setting
	WHERE varname = '" . $vbulletin->db->escape_string(CLI_VARNAME) . "'
");
if (!$setting)
{
	exit("No such setting: " . CLI_VARNAME . "\n");
}
echo CLI_VARNAME . " (setting table): $setting[value]\n";
echo CLI_VARNAME . " (datastore):     " . $vbulletin->options[CLI_VARNAME] . "\n";

if (CLI_SET)
{
	$vbulletin->db->query_write("
		UPDATE " . TABLE_PREFIX . "setting
		SET value = '" . $vbulletin->db->escape_string(CLI_VALUE) . "'
		WHERE varname = '" . $vbulletin->db->escape_string(CLI_VARNAME) . "'
	");
	$options = build_options();
	echo CLI_VARNAME . " is now:          " . $options[CLI_VARNAME] . "\n";
}
