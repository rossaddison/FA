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

/**
 * @return bool|mysqli_result
 */
function add_user(string|int|float|bool|null $user_id, string|int|float|bool|null $real_name, ?string $password, string|int|float|bool|null $phone, string|int|float|bool|null $email, string|int|float|bool|null $role_id, 
	string|int|float|bool|null $language, string|int|float|bool|null $profile, int|null $rep_popup, mixed $pos)
{
	$sql = "INSERT INTO ".TB_PREF."users (user_id, real_name, password"
		.", phone, email, role_id, language, pos, print_profile, rep_popup)
		VALUES (".db_escape($user_id).", 
		".db_escape($real_name).", ".db_escape($password) .",".db_escape($phone)
		.",".db_escape($email).", ".db_escape($role_id).", ".db_escape($language)
		.", ".db_escape($pos).",".db_escape($profile).",".db_escape($rep_popup)
		." )";

	return db_query($sql, "could not add user for $user_id");
}

//-----------------------------------------------------------------------------------------------

/**
 * @return bool|mysqli_result
 */
function update_user_password(string|int|float|bool|null $id, string|int|float|bool|null $user_id, string|int|float|bool|null $password)
{
	$sql = "UPDATE ".TB_PREF."users SET password=".db_escape($password) . ",
		user_id = ".db_escape($user_id). " WHERE id=".db_escape($id);

	return db_query($sql, "could not update user password for $user_id");
}

//-----------------------------------------------------------------------------------------------

/**
 * @return bool|mysqli_result
 */
function update_user(string|int|float|bool|null $id, string|int|float|bool|null $user_id, string|int|float|bool|null $real_name, string|int|float|bool|null $phone, string|int|float|bool|null $email, string|int|float|bool|null $role_id, 
	string|int|float|bool|null $language, string|int|float|bool|null $profile, string|int|float|bool|null $rep_popup, mixed $pos)
{
	$sql = "UPDATE ".TB_PREF."users SET real_name=".db_escape($real_name).
	", phone=".db_escape($phone).",
		email=".db_escape($email).",
		role_id=".db_escape($role_id).",
		language=".db_escape($language).",
		print_profile=".db_escape($profile).",
		rep_popup=".db_escape($rep_popup).",
		pos=".db_escape($pos).",
		user_id = " . db_escape($user_id)
		. " WHERE id=" . db_escape($id);
	return db_query($sql, "could not update user for $user_id");
}

//-----------------------------------------------------------------------------------------------

/**
 * @return bool|mysqli_result
 */
function update_user_prefs(string|int|float|bool|null $id, mixed $prefs)
{
	$sql = "UPDATE ".TB_PREF."users SET ";
	foreach($prefs as $name => $value) {
		$prefs[$name] = $name.'='. db_escape($value);
	}
	$sql .= implode(',', $prefs) . " WHERE id=".db_escape($id);

	return db_query($sql, "could not update user display prefs for $id");
}

//-----------------------------------------------------------------------------------------------


/**
 * @return mysqli_result
 */
function get_users(int|bool|array|null $all=false)
{
	$sql = "SELECT u.*, r.role FROM ".TB_PREF."users u, ".TB_PREF."security_roles r
		WHERE u.role_id=r.id";
	if (!(bool)$all) $sql .= " AND !u.inactive";
	
	return db_select($sql, "could not get users");
}

//-----------------------------------------------------------------------------------------------

/**
 * @return array<array-key, null|string>|false
 */
function get_user(string|int|float|bool|null $id)
{
	$sql = "SELECT * FROM ".TB_PREF."users WHERE id=".db_escape($id);

	$result = db_select($sql, "could not get user $id");

	return db_fetch($result);
}

//-----------------------------------------------------------------------------------------------

/**
 * @return array<array-key, null|string>|false
 */
function get_user_by_login(string|int|float|bool|null $user_id)
{
	$sql = "SELECT * FROM ".TB_PREF."users WHERE user_id=".db_escape($user_id);

	$result = db_select($sql, "could not get user $user_id");

	return db_fetch($result);
}

//-----------------------------------------------------------------------------------------------

/**
 * @return array<array-key, null|string>|false
 */
function get_user_by_email(string|int|float|bool|null $email)
{
	$sql = "SELECT * FROM ".TB_PREF."users WHERE email=".db_escape($email);

	$result = db_select($sql, "could not get user for email $email");

	if (db_num_rows($result) != 1)
		return false;

	return db_fetch($result);
}

//-----------------------------------------------------------------------------------------------

function delete_user(string|int|float|bool|null $id): void
{
	$sql="DELETE FROM ".TB_PREF."users WHERE id=".db_escape($id);

	db_query($sql, "could not delete user $id");
}

//-----------------------------------------------------------------------------------------------

function get_user_auth(string|int|float|bool|null $user_id, string|int|float|bool|null $password): bool
{

	$sql = "SELECT * FROM ".TB_PREF."users WHERE user_id = ".db_escape($user_id)." AND"
		." password=".db_escape($password);

	return db_num_rows(db_select($sql, "could not get validate user login for $user_id")) != 0;
}

//-----------------------------------------------------------------------------------------------

function update_user_visitdate(string|int|float|bool|null $user_id): void
{
	$sql = "UPDATE ".TB_PREF."users SET last_visit_date='". date("Y-m-d H:i:s") ."'
		WHERE user_id=".db_escape($user_id);

	db_query($sql, "could not update last visit date for user $user_id");
}

//-----------------------------------------------------------------------------------------------
/**
 * @return null|string
 */
function check_user_activity(string|int|float|bool|null $id) 
{
	$sql = "SELECT COUNT(*) FROM ".TB_PREF."audit_trail WHERE audit_trail.user="
		. db_escape($id);
	$result = db_select($sql,"Cant check user activity");
	$ret = row_or_empty(db_fetch($result));

	return $ret[0];
}

//-----------------------------------------------------------------------------------------------
function show_users_online(): string
{
	global $db, $GetText;

	if (!isset(sysprefs()->show_users_online) || sysprefs()->show_users_online == 0 || !defined('TB_PREF') || 
		!isset($GetText) || !isset($db))
		return "";
	$result = db_select("SHOW TABLES LIKE '".TB_PREF."useronline'");	
	if (db_num_rows($result) == 1)
	{
		$timeoutseconds = 120;

		$timestamp=time();
		$timeout=$timestamp-$timeoutseconds;
		/*
		This will find out if user is from behind proxy server. 
		In that case, the script would count them all as 1 user.
		This function tryes to get real IP address.
		*/
		if (isset($_SERVER['HTTP_CLIENT_IP'])) {
			$ip = $_SERVER['HTTP_CLIENT_IP'];
		}
		elseif (isset($_SERVER['HTTP_X_FORWARDED_FOR'])) {
			$ip = $_SERVER['HTTP_X_FORWARDED_FOR'];
		}
		elseif (isset($_SERVER['HTTP_X_FORWARDED'])) {
			$ip = $_SERVER['HTTP_X_FORWARDED'];
		}
		elseif (isset($_SERVER['HTTP_FORWARDED_FOR'])) {
			$ip = $_SERVER['HTTP_FORWARDED_FOR'];
		}
		elseif (isset($_SERVER['HTTP_FORWARDED'])) {
			$ip = $_SERVER['HTTP_FORWARDED'];
		}
		else {
			$ip = $_SERVER['REMOTE_ADDR'];
		}

		// Add user to database
		db_query("INSERT INTO ".TB_PREF."useronline (timestamp, ip, file) VALUES ('". $timestamp ."',". db_escape($ip) .",". db_escape($_SERVER['PHP_SELF']) .")");
		//Remove users that were not online within $timeoutseconds.
		db_query("DELETE FROM ".TB_PREF."useronline WHERE timestamp<". $timeout);

		// Select online users
		$result = db_select("SELECT DISTINCT ip FROM ".TB_PREF."useronline");
		$users = db_num_rows($result);
	}
	else
		$users = 1;
	return "$users ".($users == 1 ? _("user online") : _("users online"));

}
