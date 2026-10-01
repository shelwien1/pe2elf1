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
// boot vBulletin, so it works while the forum itself still fails on PHP 8.
//
// usage: php vb_php8_db_fix.php /path/to/forum            (dry run: report only)
//        php vb_php8_db_fix.php /path/to/forum --apply    (write the changes)

if (PHP_SAPI != 'cli')
{
	exit("CLI only\n");
}
if ($argc < 2)
{
	exit("usage: php {$argv[0]} /path/to/forum [--apply]\n");
}
$forumdir = rtrim($argv[1], '/');
$apply = in_array('--apply', $argv);
require_once(__DIR__ . '/php8_barewords.php');

// ---- database connection, as configured for the forum ----------------------------------
$config = array();
require($forumdir . '/includes/config.php');
$prefix = $config['Database']['tableprefix'];
$db = mysqli_init();
if (!@mysqli_real_connect($db, $config['MasterServer']['servername'], $config['MasterServer']['username'],
	$config['MasterServer']['password'], $config['Database']['dbname'], $config['MasterServer']['port'] ? $config['MasterServer']['port'] : 3306))
{
	exit('Cannot connect to the database: ' . mysqli_connect_error() . "\n");
}
mysqli_set_charset($db, !empty($config['Mysqli']['charset']) ? $config['Mysqli']['charset'] : 'utf8');
if (function_exists('mysqli_report'))
{
	mysqli_report(MYSQLI_REPORT_OFF);
}

function q($sql)
{
	global $db;
	$res = mysqli_query($db, $sql);
	if ($res === false)
	{
		exit("SQL error: " . mysqli_error($db) . "\n$sql\n");
	}
	return $res;
}

// ---- constants that are defined somewhere (forum files and plugins) -------------------
$known = array();
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
			q("UPDATE {$prefix}plugin SET phpcode = '" . mysqli_real_escape_string($db, $new) . "' WHERE pluginid = " . intval($p['pluginid']));
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
			q("UPDATE {$prefix}datastore SET data = '" . mysqli_real_escape_string($db, serialize($list)) . "' WHERE title = '" . mysqli_real_escape_string($db, $row['title']) . "'");
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
			q("UPDATE {$prefix}template SET template = '" . mysqli_real_escape_string($db, $new) . "' WHERE templateid = " . intval($t['templateid']));
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
		$fields = array();
		foreach (array('template', 'template_un') AS $field)
		{
			// skip occurrences that are already guarded
			$new = str_replace($replace, "\0GUARDED\0", $t[$field]);
			$new = str_replace("\0GUARDED\0", $replace, str_replace($search, $replace, $new));
			if ($new !== $t[$field])
			{
				$fields[] = "$field = '" . mysqli_real_escape_string($db, $new) . "'";
			}
		}
		if ($fields)
		{
			$changes++;
			echo "  #$t[templateid] style $t[styleid] $t[title]: $search\n";
			if ($apply)
			{
				q("UPDATE {$prefix}template SET " . implode(', ', $fields) . " WHERE templateid = " . intval($t['templateid']));
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
	$res = q("SELECT varname FROM {$prefix}setting WHERE validationcode = '" . mysqli_real_escape_string($db, $search) . "'");
	while ($s = mysqli_fetch_assoc($res))
	{
		$changes++;
		echo "  $s[varname]: $search\n";
		if ($apply)
		{
			q("UPDATE {$prefix}setting SET validationcode = '" . mysqli_real_escape_string($db, $replace) . "' WHERE varname = '" . mysqli_real_escape_string($db, $s['varname']) . "'");
		}
	}
}

echo "\n$changes item(s) " . ($apply ? 'updated.' : 'need changes (dry run, nothing written; use --apply).') . "\n";
if ($apply AND !empty($config['Datastore']['class']) AND $config['Datastore']['class'] != 'vB_Datastore')
{
	echo "Note: a datastore cache ({$config['Datastore']['class']}) is configured; clear it so the fixed plugin code is used.\n";
}
