<?php
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

function set_global_connection(mixed $company=-1): mysqli|false
{
	global $db, $path_to_root, $db_connections, $SysPrefs;

	include ($path_to_root . "/config_db.php");

	if ($company == -1) 
		$company = user_company();

	cancel_transaction(); // cancel all aborted transactions (if any)

	$_SESSION["wa_current_user"]->cur_con = $company;

	$connection = $db_connections[$company];

	$db = mysqli_connect($connection["host"], $connection["dbuser"], $connection["dbpassword"], "", 
		!empty($connection["port"]) ? $connection["port"] : 3306); // default port in mysql is 3306
		
	mysqli_select_db($db, $connection["dbname"]);
	///// From mysqli release 5.6.6 the sql_mode is no longer empty as it was prior to
	///// this release. Just for safety we make it empty for all 5.6 release and higher.
	///// This non empty sql_mode values can interphere with FA, so all is set empty during
	///// our sessions.
	///// We are, however, investigating the existing code to be compatible in the future.
	db_query("SET sql_mode = '".SQL_MODE."'");
	/////
	$SysPrefs->refresh();
	return $db;
}

//DB wrapper functions to change only once for whole application

function db_query(mixed $sql, mixed $err_msg=null): mysqli_result|bool
{
	global $db, $SysPrefs, $sql_queries, $Ajax,	$db_connections, $db_last_inserted_id;

	// set current db prefix
	$comp = isset($_SESSION["wa_current_user"]->cur_con) ? $_SESSION["wa_current_user"]->cur_con : 0;
	$cur_prefix = (string)@$db_connections[$comp]['tbpref'];
	$sql = str_replace(TB_PREF, $cur_prefix, $sql);

	if ($SysPrefs->show_sql)
	{
		$Ajax->activate('footer_debug');
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

	if($SysPrefs->sql_trail) {
		$db_last_inserted_id = mysqli_insert_id($db);	// preserve in case trail insert is done
		if ($SysPrefs->select_trail || (strstr($sql, 'SELECT') === false)) {
			mysqli_query($db, "INSERT INTO ".$cur_prefix."sql_trail
				(`sql`, `result`, `msg`)
				VALUES(".db_escape($sql).",".($result ? 1 : 0).",
				".db_escape($err_msg).")");
		}
	}

	if ($err_msg != null || $SysPrefs->go_debug) {
		$exit = $err_msg != null;
		if (function_exists('xdebug_call_file'))
			$err_msg = '<br>At file '.xdebug_call_file().':'.xdebug_call_line().':<br>'.$err_msg;
		check_db_error($err_msg, $sql, $exit);
	}
	return $result;
}

/**
 * @return list<string|null>|false a row with numeric keys; mysqli returns every column as a string or null
 */
function db_fetch_row(mixed $result): array|false
{
	$ret = mysqli_fetch_row($result);
	return ($ret === null ? false : $ret);
}

/**
 * @return array<string, string|null>|false a row keyed by column name
 */
function db_fetch_assoc(mixed $result): array|false
{
	$ret = mysqli_fetch_assoc($result);
	return ($ret === null ? false : $ret);
}

/**
 * @return array<array-key, string|null>|false a row keyed by column name and by position
 */
function db_fetch(mixed $result): array|false
{
	$ret = mysqli_fetch_array($result);
	return ($ret === null ? false : $ret);
}

function db_seek(&$result,mixed $record): bool
{
	return mysqli_data_seek($result, $record);
}

function db_free_result(bool|mysqli_result $result): void
{
	if ($result)
		mysqli_free_result($result);
}

function db_num_rows(mixed $result): int|string
{
	return mysqli_num_rows($result);
}

function db_num_fields(mixed $result): int
{
	return mysqli_num_fields($result);
}

function db_escape(mixed $value = "", mixed $nullify = false): string
{
	global $db;

	$value = @html_entity_decode($value, ENT_QUOTES, $_SESSION['language']->encoding=='iso-8859-2' ? 'ISO-8859-1' : $_SESSION['language']->encoding);
	$value = html_specials_encode($value);

  	//reset default if second parameter is skipped
	$nullify = ($nullify === null) ? (false) : ($nullify);

  	//check for null/unset/empty strings
	if ((!isset($value)) || (is_null($value)) || ($value === "")) {
		$value = ($nullify) ? ("NULL") : ("''");
	} else {
		if (is_string($value)) {
			$value = "'" . mysqli_real_escape_string($db, $value) . "'";
      		//value is a string and should be quoted; 
		} else if (!is_numeric($value)) {
			//value is not a string nor numeric
			display_error("ERROR: incorrect data type send to sql query");
			echo '<br><br>';
			exit();
		}
	}
	return $value;
}

function db_error_no(): int
{
	global $db;
    return mysqli_errno($db);
}

function db_error_msg(mixed $conn): string
{
	return mysqli_error($conn);
}

function db_insert_id(): int|string
{
	global $db_last_inserted_id, $SysPrefs, $db;

	return $SysPrefs->sql_trail ? $db_last_inserted_id : mysqli_insert_id($db);
}

function db_num_affected_rows(): int|string
{
	global $db;
	return mysqli_affected_rows($db);
}

function db_field_name(mixed $result, string|int|float|bool|null $n): string
{
    $fieldinfo = mysqli_fetch_field_direct($result, $n);
    return $fieldinfo->name;
}

function db_set_collation(mixed $db, mixed $fa_collation): bool|mysqli_result
{
	return mysqli_query($db, "ALTER DATABASE COLLATE ".get_mysql_collation($fa_collation));
}

/*
	Create database for FA company. If database already exists,
	just set collation to be sure nothing weird will happen later.
*/
function db_create_db(mixed $connection): int|mysqli|false
{
	global $db;

	$db = mysqli_connect($connection["host"], $connection["dbuser"], $connection["dbpassword"], "",
		!empty($connection["port"]) ? $connection["port"] : 3306); // default port in mysql is 3306

	if (!mysqli_select_db($db, $connection["dbname"]))
	{
		$sql = "CREATE DATABASE IF NOT EXISTS `" . (string)$connection["dbname"] . "`"
			. " DEFAULT COLLATE '" . get_mysql_collation($connection["collation"]) . "'";

		if (!mysqli_query($db, $sql) || !mysqli_select_db($db, $connection["dbname"]))
			return 0;
	} else {
		if (!db_set_collation($db, $connection["collation"]))
		{
			return 0;
		}
	}
	return $db;
}

function db_drop_db(mixed $connection): bool|mysqli_result
{

	if ($connection["tbpref"] == "")
	{
		global $db;
		$sql = "DROP DATABASE IF EXISTS " . (string)$connection["dbname"] . "";
		return mysqli_query($db, $sql);
	}
	else
	{
    	$res = db_query("show table status");
    	$all_tables = array();
    	while($row = db_fetch($res))
    		$all_tables[] = $row;
        // get table structures
		foreach ($all_tables as $table)
		{
			if (strpos($table['Name'], $connection["tbpref"]) === 0)
				db_query("DROP TABLE `".$table['Name'] . "`");
		}
		//deleting the tables, how??
		return true;
	}
}

function db_close(mixed $dbase = null): bool
{
	global $db;
	
	if (!$dbase)
		$dbase = $db;
	return mysqli_close($dbase);
}

function db_extension_exists(): bool
{
	return function_exists('mysqli_connect');
}

function db_escape_function(mixed $string)
{
	global $db;
	
	return (mysqli_real_escape_string($db, $string));
}

/*
	Set mysql client encoding.
	Default is is encoding used by default language.
*/
function db_set_encoding(mixed $ui_encoding=null): void
{
	global $db, $dflt_lang, $installed_languages;

	if (!isset($ui_encoding))
	{
		$lang = array_search_value($dflt_lang, $installed_languages, 'code');
		$ui_encoding = strtoupper($lang['encoding']);
	}

	if ($mysql_enc = get_mysql_encoding_name($ui_encoding))
		mysqli_set_charset($db, $mysql_enc);
}

function db_get_charset(mixed $db): string
{
	return mysqli_character_set_name($db);
}

function db_set_charset(mixed $db, ?string $charset): bool
{
	global $db;

	return mysqli_set_charset($db, $charset);
}
