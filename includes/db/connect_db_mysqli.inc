<?php
declare(strict_types=1);
/**********************************************************************
    Copyright (C) FrontAccounting, LLC.
	Released under the terms of the GNU General Public License, GPL, 
	as published by the Free Software Foundation, either version 3 
	of the License, or (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
    See the License here <http://www.gnu.org/licenses/gpl-3.0.html>.
***********************************************************************/
define('DB_DUPLICATE_ERROR', 1062);
define('SQL_MODE', 'STRICT_ALL_TABLES'); // prevents SQL injection with silent field content truncation

$db_last_inserted_id = 0;

function set_global_connection(string|int|float|bool|null $company=-1): mysqli
{
	global $db, $path_to_root, $db_connections, $SysPrefs;

	include (dirname(__DIR__, 2) . "/config_db.php");

	if ($company == -1) 
		$company = user_company();

	cancel_transaction(); // cancel all aborted transactions (if any)

	session_obj('wa_current_user')->cur_con = $company;

	$connection = $db_connections[(int) $company];

	$db = mysqli_connect($connection["host"], $connection["dbuser"], $connection["dbpassword"], "",
		(bool)($connection["port"] ?? null) ? (int) $connection["port"] : 3306); // default port in mysql is 3306
	if (!$db instanceof mysqli) {
		throw new RuntimeException('set_global_connection(): could not connect to the database for company ' . (string) $company);
	}

	mysqli_select_db($db, $connection["dbname"]);
	///// From mysqli release 5.6.6 the sql_mode is no longer empty as it was prior to
	///// this release. Just for safety we make it empty for all 5.6 release and higher.
	///// This non empty sql_mode values can interphere with FA, so all is set empty during
	///// our sessions.
	///// We are, however, investigating the existing code to be compatible in the future.
	db_query("SET sql_mode = '".SQL_MODE."'");
	/////
	sysprefs()->refresh();
	/** @var mysqli $db */
	return $db;
}

//DB wrapper functions to change only once for whole application

function db_query(string $sql, ?string $err_msg=null): mysqli_result|bool
{
	global $db, $sql_queries, $db_connections, $db_last_inserted_id;

	// set current db prefix
	$comp = isset(session_obj('wa_current_user')->cur_con) ? (int) session_obj('wa_current_user')->cur_con : 0;
	$cur_prefix = @$db_connections[$comp]['tbpref'];
	$sql = str_replace(TB_PREF, $cur_prefix, $sql);

	if (sysprefs()->show_sql)
	{
		ajax()->activate('footer_debug');
		$sql_queries .= "<pre>$sql</pre>\n<hr>";
	}

	db_profile(); // mysql profiling

	$retry = MAX_DEADLOCK_RETRY;
	do {
		$result = mysqli_query($db, $sql);
		if (mysqli_errno($db) == 1213)	{ // deadlock detected
			sleep(1); $retry--;
		} else
			$retry = 0;
	} while ($retry);

	db_profile($sql);

	if((bool) sysprefs()->sql_trail) {
		$db_last_inserted_id = mysqli_insert_id($db);	// preserve in case trail insert is done
		if (sysprefs()->select_trail || (strstr($sql, 'SELECT') === false)) {
			mysqli_query($db, "INSERT INTO ".$cur_prefix."sql_trail
				(`sql`, `result`, `msg`)
				VALUES(".db_escape($sql).",".($result ? 1 : 0).",
				".db_escape($err_msg).")");
		}
	}

	if ($err_msg != null || sysprefs()->go_debug) {
		$exit = $err_msg != null;
		if (function_exists('xdebug_call_file'))
			$err_msg = '<br>At file '.xdebug_call_file().':'.xdebug_call_line().':<br>'.(string) $err_msg;
		check_db_error($err_msg, $sql, $exit);
	}
	return $result;
}

/**
 * db_query() for a statement that returns rows. db_query() answers with a bool for INSERT/UPDATE/DELETE, so a query
 * whose result is fetched from goes through this function, which returns the result set. A failed query used to reach
 * db_fetch() as false and raise the same TypeError here.
 */
function db_select(string $sql, ?string $err_msg=null): mysqli_result
{
	$result = db_query($sql, $err_msg);
	if (!$result instanceof mysqli_result) {
		throw new TypeError('db_select(): the query did not return a result set: ' . $sql);
	}
	return $result;
}

/**
 * A row, or an empty array when the lookup found none: db_fetch() and the get_*() lookups built on it return false when
 * there is no row, and reading $row['x'] of false is a warning.
 *
 * @template T of array<array-key, mixed>
 * @param T|false|null $row
 * @return T|array{}
 * @psalm-pure
 */
function row_or_empty(array|false|null $row): array
{
	return $row === false || $row === null ? array() : $row;
}

/**
 * @return list<string|null>|false a row with numeric keys; mysqli returns every column as a string or null
 * @psalm-pure
 */
function db_fetch_row(mysqli_result $result): array|false
{
	/** @var list<string|null>|false|null $ret every column is a string or null: FA does not set MYSQLI_OPT_INT_AND_FLOAT_NATIVE */
	$ret = mysqli_fetch_row($result);
	return ($ret === null ? false : $ret);
}

/**
 * @return array<string, string|null>|false a row keyed by column name
 * @psalm-pure
 */
function db_fetch_assoc(mysqli_result $result): array|false
{
	/** @var array<string, string|null>|false|null $ret every column is a string or null: FA does not set MYSQLI_OPT_INT_AND_FLOAT_NATIVE */
	$ret = mysqli_fetch_assoc($result);
	return ($ret === null ? false : $ret);
}

/**
 * @return array<array-key, string|null>|false a row keyed by column name and by position
 * @psalm-pure
 */
function db_fetch(mysqli_result $result): array|false
{
	/** @var array<array-key, string|null>|false|null $ret every column is a string or null: FA does not set MYSQLI_OPT_INT_AND_FLOAT_NATIVE */
	$ret = mysqli_fetch_array($result);
	return ($ret === null ? false : $ret);
}

/** @psalm-pure */
function db_seek(mysqli_result &$result, int $record): bool
{
	return mysqli_data_seek($result, $record);
}

function db_free_result(bool|mysqli_result $result): void
{
	if ($result instanceof mysqli_result)
		mysqli_free_result($result);
}

/** @psalm-pure */
function db_num_rows(mysqli_result $result): int|string
{
	return mysqli_num_rows($result);
}

/** @psalm-pure */
function db_num_fields(mysqli_result $result): int
{
	return mysqli_num_fields($result);
}

/** @psalm-taint-escape sql */
function db_escape(string|int|float|bool|null $value = "", ?bool $nullify = false): string
{
	global $db;

	if (is_bool($value))
		$value = $value ? '1' : '0';

	$value = @html_entity_decode((string) $value, ENT_QUOTES, session_obj('language')->encoding=='iso-8859-2' ? 'ISO-8859-1' : session_obj('language')->encoding);
	$value = html_specials_encode($value);

  	//reset default if second parameter is skipped
	$nullify = ($nullify === null) ? (false) : ($nullify);

  	//check for empty strings ($value is unconditionally a non-null string by this point,
	//thanks to the (string) cast + html_entity_decode()/html_specials_encode() calls above,
	//so the is_string()/is_numeric() branching this used to have here was all dead code)
	if ($value === "") {
		$value = ($nullify) ? ("NULL") : ("''");
	} else {
		// mysqli_real_escape_string() has no declared return type in the vendored Psalm
		// stub (vendor/vimeo/psalm/stubs/CoreGenericFunctions.phpstub), so Psalm sees its
		// result as mixed; it always genuinely returns a string per PHP's own docs
		$value = "'" . (string) mysqli_real_escape_string($db, $value) . "'";
	}
	return $value;
}

function db_error_no(): int
{
	global $db;
    return mysqli_errno($db);
}

/** @psalm-pure */
function db_error_msg(mysqli $conn): string
{
	return mysqli_error($conn);
}

function db_insert_id(): int|string
{
	global $db_last_inserted_id, $db;

	return (bool) sysprefs()->sql_trail ? $db_last_inserted_id : mysqli_insert_id($db);
}

function db_num_affected_rows(): int|string
{
	global $db;
	return mysqli_affected_rows($db);
}

/** @psalm-pure */
function db_field_name(mysqli_result $result, string|int|float|bool|null $n): string
{
    $fieldinfo = mysqli_fetch_field_direct($result, (int) $n);
    return $fieldinfo === false ? '' : $fieldinfo->name;
}

function db_set_collation(mysqli $db, string|int|float|bool|null $fa_collation): bool|mysqli_result
{
	return mysqli_query($db, "ALTER DATABASE COLLATE ".get_mysql_collation($fa_collation));
}

/*
	Create database for FA company. If database already exists,
	just set collation to be sure nothing weird will happen later.
*/
/** @param array<string, mixed> $connection */
function db_create_db(array $connection): int|mysqli|false
{
	global $db;

	$host = $connection["host"] === null ? null : (string) $connection["host"];
	$dbuser = $connection["dbuser"] === null ? null : (string) $connection["dbuser"];
	$dbpassword = $connection["dbpassword"] === null ? null : (string) $connection["dbpassword"];
	$dbname = (string) $connection["dbname"];
	/** @var string|int|float|bool|null $collation */
	$collation = is_array($connection["collation"] ?? null) ? null : ($connection["collation"] ?? null);

	$db = mysqli_connect($host, $dbuser, $dbpassword, "",
		!empty($connection["port"]) ? (int) $connection["port"] : 3306); // default port in mysql is 3306
	if (!$db instanceof mysqli) {
		return false;
	}

	if (!mysqli_select_db($db, $dbname))
	{
		$sql = "CREATE DATABASE IF NOT EXISTS `" . str_replace('`', '``', $dbname) . "`"
			. " DEFAULT COLLATE '" . get_mysql_collation($collation) . "'";

		if (!mysqli_query($db, $sql) || !mysqli_select_db($db, $dbname))
			return 0;
	} else {
		if (!db_set_collation($db, $collation))
		{
			return 0;
		}
	}
	return $db;
}

/** @param array<string, string> $connection */
function db_drop_db(array $connection): bool|mysqli_result
{
	global $db;

	if ($connection["tbpref"] == "")
	{
		$sql = "DROP DATABASE IF EXISTS `" . str_replace('`', '``', $connection["dbname"]) . "`";
		return mysqli_query($db, $sql);
	}
	else
	{
    	$res = db_select("show table status");
    	$all_tables = array();
    	while($row = db_fetch($res))
    		$all_tables[] = $row;
        // get table structures
		foreach ($all_tables as $table)
		{
			if (strpos((string) $table['Name'], $connection["tbpref"]) === 0)
				db_query("DROP TABLE `".(string)$table['Name'] . "`");
		}
		//deleting the tables, how??
		return true;
	}
}

function db_close(?mysqli $dbase = null): bool
{
	global $db;

	if ($dbase === null)
		$dbase = $db;
	return mysqli_close($dbase);
}

/** @psalm-pure */
function db_extension_exists(): bool
{
	return function_exists('mysqli_connect');
}

function db_escape_function(string|int|float|bool|null $string): string
{
	global $db;

	// same untyped-vendor-stub situation as db_escape() above
	return (string) mysqli_real_escape_string($db, (string) $string);
}

/*
	Set mysql client encoding.
	Default is is encoding used by default language.
*/
function db_set_encoding(?string $ui_encoding=null): void
{
	global $db, $dflt_lang, $installed_languages;

	if (!isset($ui_encoding))
	{
		$lang = array_search_value($dflt_lang, $installed_languages, 'code');
		$ui_encoding = strtoupper((string) ($lang !== null ? ($lang['encoding'] ?? '') : ''));
	}

	if ((bool)($mysql_enc = get_mysql_encoding_name($ui_encoding)))
		mysqli_set_charset($db, $mysql_enc);
}

/** @psalm-pure */
function db_get_charset(mysqli $db): string
{
	return mysqli_character_set_name($db);
}

function db_set_charset(mysqli $db, string $charset): bool
{
	return mysqli_set_charset($db, $charset);
}
