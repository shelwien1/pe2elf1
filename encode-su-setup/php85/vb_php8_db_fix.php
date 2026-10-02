<?php
// Make the PHP code stored in a vBulletin 4 database PHP 8 compatible:
//   - plugins (plugin.phpcode) and the plugin code cached per hook in the
//     datastore ('pluginlist', 'pluginlistadmin')
//   - compiled templates (template.template)
// PHP 8 throws Error for undefined constants, which PHP 7 silently turned into
// strings: $vbulletin->options[post_thanks_delete_own], THIS_SCRIPT === member, and
// template code like $post[onlinestatusphrase] that the vB4 template compiler produced
// from {vb:raw post['onlinestatusphrase']}. Such barewords are quoted, which is what PHP 7
// did at run time; nothing else is changed.
//
// It talks to the database directly (credentials from includes/config.php) and does not
// boot vBulletin, so it works while the forum itself still fails on PHP 8. It runs on
// PHP 5.6 and later.
//
// usage: php vb_php8_db_fix.php /path/to/forum            (dry run: report only)
//        php vb_php8_db_fix.php /path/to/forum --apply [--undo-file FILE]
//                write the changes; with --undo-file, also record every value that is
//                changed (the old and the new value), so --restore can put it back
//        php vb_php8_db_fix.php /path/to/forum --restore FILE [--force] [--dry-run]
//                put back the values recorded in FILE. A value that was changed again
//                since (by an administrator, say) is only overwritten with --force;
//                otherwise nothing is written and the exit status is 3. --dry-run only
//                checks.
//        php vb_php8_db_fix.php /path/to/forum --client-config FILE
//                write the database login of the forum as a MySQL option file (mode 600)
//                for mysql/mysqldump --defaults-extra-file=FILE; prints the database name
// Exit status: 0 = success, 1 = error, 2 = usage, 3 = --restore found changed values.

if (PHP_SAPI != 'cli')
{
	exit("CLI only\n");
}

function fail($message, $status = 1)
{
	fwrite(STDERR, $message . "\n");
	exit($status);
}

function option_value($name)
{
	global $argv;
	$i = array_search($name, $argv);
	if ($i === false)
	{
		return null;
	}
	if (!isset($argv[$i + 1]) OR $argv[$i + 1] === '' OR substr($argv[$i + 1], 0, 2) == '--')
	{
		fail("$name needs a file name", 2);
	}
	return $argv[$i + 1];
}

if ($argc < 2 OR substr($argv[1], 0, 2) == '--')
{
	fail("usage: php {$argv[0]} /path/to/forum [--apply [--undo-file FILE] | --restore FILE [--force] [--dry-run] | --client-config FILE]", 2);
}
$forumdir = rtrim($argv[1], '/');
$apply = in_array('--apply', $argv);
$force = in_array('--force', $argv);
$dryrun = in_array('--dry-run', $argv);
$undofile = option_value('--undo-file');
$restorefile = option_value('--restore');
$clientconfig = option_value('--client-config');
if ($undofile !== null AND !$apply)
{
	fail('--undo-file needs --apply', 2);
}
if (!is_file($forumdir . '/includes/config.php'))
{
	fail("$forumdir/includes/config.php not found");
}

// ---- database connection, as configured for the forum ----------------------------------
$config = array();
require($forumdir . '/includes/config.php');
$prefix = $config['Database']['tableprefix'];
$port = !empty($config['MasterServer']['port']) ? intval($config['MasterServer']['port']) : 3306;

if ($clientconfig !== null)
{
	// MySQL option file values: \\ for a backslash, and quotes around values with # or spaces
	$lines = array('[client]');
	foreach (array('host' => $config['MasterServer']['servername'], 'port' => $port,
		'user' => $config['MasterServer']['username'], 'password' => $config['MasterServer']['password']) AS $key => $value)
	{
		$value = str_replace(array('\\', "\n", "\r", "\t"), array('\\\\', '\\n', '\\r', '\\t'), strval($value));
		if (strpos($value, '"') === false)
		{
			$value = '"' . $value . '"';
		}
		else if (strpos($value, "'") === false)
		{
			$value = "'" . $value . "'";
		}
		else
		{
			fail("The database $key contains both kinds of quotes; it cannot be written to a MySQL option file.");
		}
		$lines[] = "$key=$value";
	}
	umask(077);
	if (file_put_contents($clientconfig, implode("\n", $lines) . "\n") === false)
	{
		fail("Cannot write $clientconfig");
	}
	chmod($clientconfig, 0600);
	echo $config['Database']['dbname'], "\n";
	exit(0);
}

require_once(__DIR__ . '/php8_barewords.php');
if (!function_exists('mysqli_init'))
{
	fail('The PHP running this script has no mysqli extension.');
}
// errors are checked here; PHP 8.1+ would throw exceptions by default
mysqli_report(MYSQLI_REPORT_OFF);
$db = mysqli_init();
if (!@mysqli_real_connect($db, $config['MasterServer']['servername'], $config['MasterServer']['username'],
	$config['MasterServer']['password'], $config['Database']['dbname'], $port))
{
	fail('Cannot connect to the database: ' . mysqli_connect_error());
}
mysqli_set_charset($db, !empty($config['Mysqli']['charset']) ? $config['Mysqli']['charset'] : 'utf8');

function q($sql)
{
	global $db;
	$res = mysqli_query($db, $sql);
	if ($res === false)
	{
		fail("SQL error: " . mysqli_error($db) . "\n$sql");
	}
	return $res;
}

function esc($value)
{
	global $db;
	return "'" . mysqli_real_escape_string($db, $value) . "'";
}

// ---- --restore: put back the values recorded with --undo-file ---------------------------
if ($restorefile !== null)
{
	// per field: the value before the fix (old value of its first change) and the value the
	// fix left (new value of its last change; a template can be changed twice)
	$fields = array();
	foreach (file($restorefile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) ?: array() AS $line)
	{
		$entry = @unserialize(base64_decode($line));
		if (!is_array($entry) OR count($entry) != 6)
		{
			fail("$restorefile is damaged");
		}
		list($table, $keycol, $keyval, $col, $old, $new) = $entry;
		$id = "$table|$keycol|$keyval|$col";
		if (!isset($fields[$id]))
		{
			$fields[$id] = array('table' => $table, 'keycol' => $keycol, 'keyval' => $keyval, 'col' => $col, 'before' => $old);
		}
		$fields[$id]['after'] = $new;
	}
	// check everything before writing anything
	$todo = $conflicts = array();
	$done = 0;
	foreach ($fields AS $id => $f)
	{
		$row = mysqli_fetch_assoc(q("SELECT `$f[col]` AS value FROM `$f[table]` WHERE `$f[keycol]` = " . esc($f['keyval'])));
		if (!$row)
		{
			echo "  $f[table] $f[keycol]=$f[keyval]: the row no longer exists, skipped\n";
		}
		else if ($row['value'] === $f['before'])
		{
			$done++;
		}
		else if ($row['value'] === $f['after'])
		{
			$todo[] = $f;
		}
		else
		{
			echo "  $f[table] $f[keycol]=$f[keyval]: $f[col] was changed after the fix\n";
			$conflicts[] = $f;
		}
	}
	if ($conflicts AND !$force)
	{
		echo count($conflicts) . " of " . count($fields) . " value(s) were changed after the fix; nothing was written (--force puts back the old values anyway).\n";
		exit(3);
	}
	if ($dryrun)
	{
		echo count($todo) + count($conflicts) . " value(s) to restore" . ($done ? ", $done already as before" : '') . " (dry run, nothing written).\n";
		exit(0);
	}
	q('START TRANSACTION');
	foreach (array_merge($todo, $conflicts) AS $f)
	{
		q("UPDATE `$f[table]` SET `$f[col]` = " . esc($f['before']) . " WHERE `$f[keycol]` = " . esc($f['keyval']));
	}
	q('COMMIT');
	echo count($todo) + count($conflicts) . " value(s) restored" . ($done ? ", $done already as before" : '') . ".\n";
	exit(0);
}

// ---- --undo-file: every change is recorded before it is written --------------------------
$undo = null;
if ($undofile !== null)
{
	$undo = @fopen($undofile, 'a');
	if (!$undo)
	{
		fail("Cannot write $undofile");
	}
}

function update($table, $keycol, $keyval, $values, $oldvalues)
{
	global $undo;
	$set = array();
	foreach ($values AS $col => $value)
	{
		if ($undo AND (fwrite($undo, base64_encode(serialize(array($table, $keycol, $keyval, $col, $oldvalues[$col], $value))) . "\n") === false OR !fflush($undo)))
		{
			fail('Cannot write the undo file');
		}
		$set[] = "`$col` = " . esc($value);
	}
	q("UPDATE `$table` SET " . implode(', ', $set) . " WHERE `$keycol` = " . esc($keyval));
}

// ---- constants that are defined somewhere (PHP itself, forum files and plugins) ----------
$known = array();
if (is_file(__DIR__ . '/php_constants.txt'))
{
	foreach (file(__DIR__ . '/php_constants.txt', FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) AS $name)
	{
		if ($name[0] != '#')
		{
			$known[trim($name)] = true;
		}
	}
}
$it = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($forumdir, FilesystemIterator::SKIP_DOTS));
foreach ($it AS $file)
{
	if (substr($file->getFilename(), -4) == '.php')
	{
		foreach (php8_defined_constant_names(file_get_contents($file->getPathname())) AS $name)
		{
			$known[$name] = true;
		}
	}
}
$plugins = array();
$res = q("SELECT pluginid, product, hookname, title, active, phpcode FROM {$prefix}plugin ORDER BY pluginid");
while ($row = mysqli_fetch_assoc($res))
{
	$plugins[] = $row;
	foreach (php8_defined_constant_names($row['phpcode']) AS $name)
	{
		$known[$name] = true;
	}
}

function lint_ok($code)
{
	// tokenizing cannot change validity, but make sure: compile without running
	$tmp = tempnam(sys_get_temp_dir(), 'vbphp8');
	file_put_contents($tmp, $code);
	exec(escapeshellarg(PHP_BINARY) . ' -n -l ' . escapeshellarg($tmp) . ' 2>&1', $out, $ret);
	unlink($tmp);
	return $ret === 0;
}

function fix_code($code, &$quoted, $wrap = 'php')
{
	global $known;
	// $wrap: 'php' = plain PHP statements, 'dq' = body of a double-quoted string
	// (old-style templates that vB evaluates as $final_rendered = "...")
	$head = ($wrap == 'dq') ? "<?php \$final_rendered = \"" : "<?php\n";
	$tail = ($wrap == 'dq') ? "\";\n" : "\n";
	$fixed = php8_quote_undefined_constants($head . $code . $tail, $known, $quoted);
	if (!$quoted)
	{
		return $code;
	}
	if (substr($fixed, 0, strlen($head)) !== $head OR substr($fixed, -strlen($tail)) !== $tail OR !lint_ok($fixed))
	{
		echo "    !! could not fix safely, left unchanged\n";
		$quoted = array();
		return $code;
	}
	return substr($fixed, strlen($head), -strlen($tail));
}

$changes = 0;
if ($apply)
{
	q('START TRANSACTION');
}

// ---- plugins ---------------------------------------------------------------------------
echo "Plugins:\n";
foreach ($plugins AS $p)
{
	$new = fix_code($p['phpcode'], $quoted);
	if ($quoted)
	{
		$changes++;
		echo "  #$p[pluginid] $p[product]/$p[hookname] \"$p[title]\"" . ($p['active'] ? '' : ' (inactive)') . ': ' . implode(', ', array_unique($quoted)) . "\n";
		if ($apply)
		{
			update("{$prefix}plugin", 'pluginid', $p['pluginid'], array('phpcode' => $new), array('phpcode' => $p['phpcode']));
		}
	}
}

// ---- plugin code cached in the datastore -------------------------------------------------
echo "Datastore:\n";
$res = q("SELECT title, data FROM {$prefix}datastore WHERE title IN ('pluginlist', 'pluginlistadmin')");
while ($row = mysqli_fetch_assoc($res))
{
	$list = @unserialize($row['data']);
	if (!is_array($list))
	{
		continue;
	}
	$changed = false;
	foreach ($list AS $hook => $code)
	{
		$new = fix_code($code, $quoted);
		if ($quoted)
		{
			$changed = true;
			$list[$hook] = $new;
			echo "  $row[title] / $hook: " . implode(', ', array_unique($quoted)) . "\n";
		}
	}
	if ($changed)
	{
		$changes++;
		if ($apply)
		{
			update("{$prefix}datastore", 'title', $row['title'], array('data' => serialize($list)), array('data' => $row['data']));
		}
	}
}

// ---- compiled templates -------------------------------------------------------------------
echo "Templates:\n";
$res = q("SELECT templateid, styleid, title, template FROM {$prefix}template WHERE templatetype = 'template' ORDER BY styleid, title");
while ($t = mysqli_fetch_assoc($res))
{
	$wrap = (strpos($t['template'], '$final_rendered') !== false) ? 'php' : 'dq';
	$new = fix_code($t['template'], $quoted, $wrap);
	if ($quoted)
	{
		$changes++;
		echo "  #$t[templateid] style $t[styleid] $t[title]: " . implode(', ', array_unique($quoted)) . "\n";
		if ($apply)
		{
			update("{$prefix}template", 'templateid', $t['templateid'], array('template' => $new), array('template' => $t['template']));
		}
	}
}

// ---- constants that are only defined on some pages ------------------------------------------
// The activity stream templates test <vb:if condition="AS_PROFILE === true">, but AS_PROFILE is
// only defined on member profile pages. PHP 7 compared the string 'AS_PROFILE' (false) elsewhere.
// Both the template source and the compiled code get the equivalent explicit check.
echo "Conditionally defined constants in templates:\n";
$conditional = array(
	'AS_PROFILE === true' => "(defined('AS_PROFILE') AND AS_PROFILE === true)",
);
foreach ($conditional AS $search => $replace)
{
	$res = q("SELECT templateid, styleid, title, template, template_un FROM {$prefix}template
		WHERE templatetype = 'template' AND (template LIKE '%" . mysqli_real_escape_string($db, $search) . "%'
			OR template_un LIKE '%" . mysqli_real_escape_string($db, $search) . "%')");
	while ($t = mysqli_fetch_assoc($res))
	{
		$values = array();
		foreach (array('template', 'template_un') AS $field)
		{
			// skip occurrences that are already guarded
			$new = str_replace($replace, "\0GUARDED\0", $t[$field]);
			$new = str_replace("\0GUARDED\0", $replace, str_replace($search, $replace, $new));
			if ($new !== $t[$field])
			{
				$values[$field] = $new;
			}
		}
		if ($values)
		{
			$changes++;
			echo "  #$t[templateid] style $t[styleid] $t[title]: $search\n";
			if ($apply)
			{
				update("{$prefix}template", 'templateid', $t['templateid'], $values, $t);
			}
		}
	}
}

// ---- setting validation code ------------------------------------------------------------------
// PHP 8 compares a number with a non-numeric string as strings: '' >= 0 is false, so an empty
// value fails "return ($raw_data >= 0);" and the Admin CP refuses to save the options group.
// PHP 7 converted '' to 0; intval() gives the same result for every input.
echo "Setting validation code:\n";
$validation = array(
	'return ($raw_data >= 0);' => 'return (intval($raw_data) >= 0);',
);
foreach ($validation AS $search => $replace)
{
	$res = q("SELECT varname, validationcode FROM {$prefix}setting WHERE validationcode = " . esc($search));
	while ($s = mysqli_fetch_assoc($res))
	{
		$changes++;
		echo "  $s[varname]: $search\n";
		if ($apply)
		{
			update("{$prefix}setting", 'varname', $s['varname'], array('validationcode' => $replace), array('validationcode' => $s['validationcode']));
		}
	}
}

if ($apply)
{
	q('COMMIT');
}
echo "\n$changes item(s) " . ($apply ? 'updated.' : 'need changes (dry run, nothing written; use --apply).') . "\n";
if ($apply AND !empty($config['Datastore']['class']) AND $config['Datastore']['class'] != 'vB_Datastore')
{
	echo "Note: a datastore cache ({$config['Datastore']['class']}) is configured; clear it so the fixed plugin code is used.\n";
}
