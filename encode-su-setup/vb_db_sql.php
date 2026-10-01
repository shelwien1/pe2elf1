<?php
// Print the SQL that creates the database and MySQL account that a vBulletin
// includes/config.php expects, so the config file itself can stay untouched.
// The password is read from config.php and never shown on the terminal.
//
// Usage: php5.6 vb_db_sql.php /var/www/encode.su/includes/config.php | mysql -uroot
//        php5.6 vb_db_sql.php /var/www/encode.su/includes/config.php --dbname

if (PHP_SAPI != 'cli')
{
	exit("CLI only\n");
}
if ($argc < 2)
{
	exit("Usage: php {$argv[0]} <path/to/includes/config.php> [--dbname]\n");
}

$config = array();
include($argv[1]);

$dbname = $config['Database']['dbname'];
if ($argc > 2 AND $argv[2] == '--dbname')
{
	echo $dbname . "\n";
	exit;
}

$user = $config['MasterServer']['username'];
$pass = $config['MasterServer']['password'];

function sql_str($s)
{
	return "'" . addslashes($s) . "'";
}
function sql_ident($s)
{
	return '`' . str_replace('`', '``', $s) . '`';
}

// The forum tables are utf8mb3 and vBulletin connects with charset 'utf8'.
echo "CREATE DATABASE IF NOT EXISTS " . sql_ident($dbname) . " DEFAULT CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci;\n";

// config.php connects over TCP to a host name mapped to 127.0.0.1 / ::1.
// PHP 5.6 (mysqlnd) only supports mysql_native_password, not MySQL 8's caching_sha2_password.
foreach (array('localhost', '127.0.0.1', '::1') AS $host)
{
	$account = sql_str($user) . '@' . sql_str($host);
	echo "CREATE USER IF NOT EXISTS $account IDENTIFIED WITH mysql_native_password BY " . sql_str($pass) . ";\n";
	echo "ALTER USER $account IDENTIFIED WITH mysql_native_password BY " . sql_str($pass) . ";\n";
	echo "GRANT ALL PRIVILEGES ON " . sql_ident($dbname) . ".* TO $account;\n";
}
echo "FLUSH PRIVILEGES;\n";
