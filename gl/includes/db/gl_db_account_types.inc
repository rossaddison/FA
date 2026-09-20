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
function add_account_type(string|int|float|bool|null $id, string|int|float|bool|null $name, string|int|float|bool|null $class_id, string|int|float|bool|null $parent)
{
	$sql = "INSERT INTO ".TB_PREF."chart_types (id, name, class_id, parent)
		VALUES (".db_escape($id).", ".db_escape($name).", ".db_escape($class_id).", ".db_escape($parent).")";

	return db_query($sql);
}

/**
 * @return bool|mysqli_result
 */
function update_account_type(string|int|float|bool|null $id, string|int|float|bool|null $name, string|int|float|bool|null $class_id, string|int|float|bool|null $parent, string|int|float|bool|null $old_id)
{
	begin_transaction();
	if ($old_id !== $id)
	{
		$sql = "SELECT id 
			FROM ".TB_PREF."chart_types 
			WHERE parent = ".db_escape($old_id);

		$result = db_query($sql, "could not get account type");

		while ($myrow = db_fetch($result))
		{
    		$sql = "UPDATE ".TB_PREF."chart_types SET parent=".db_escape($id)
				." WHERE id = '".(string)$myrow['id']."'";
			db_query($sql, "could not update account type");
		}
		$sql = "SELECT account_code 
			FROM ".TB_PREF."chart_master 
			WHERE account_type = ".db_escape($old_id);

		$result = db_query($sql, "could not get account");

		while ($myrow = db_fetch($result))
		{
    		$sql = "UPDATE ".TB_PREF."chart_master 
    			SET account_type=".db_escape($id)
				." WHERE account_code = '".(string)$myrow['account_code']."'";
			db_query($sql, "could not update account");
		}
	}
    $sql = "UPDATE ".TB_PREF."chart_types 
    		SET id=".db_escape($id) .", name=".db_escape($name).",
			class_id=".db_escape($class_id).", parent=".db_escape($parent)
		." WHERE id = ".db_escape($old_id);

	$ret = db_query($sql, "could not update account type");
	commit_transaction();
	return $ret;
}

/**
 * @return bool|mysqli_result
 */
function get_account_types(int|bool|array|null $all=false, string|int|float|bool|null $class_id=false, string|int|bool|null $parent=false)
{
	$sql = "SELECT * FROM ".TB_PREF."chart_types";

	$where = array();
	
	if (!(bool)$all)
		$where[] = "!inactive";

	if ($class_id != false)  
		$where[] = "class_id=".db_escape($class_id);

	if ($parent == -1) 
		$where[] ="(parent = '' OR parent = '-1')";
	elseif ($parent !== false) 
		$where[] = "parent=".db_escape($parent);

	if (count($where))
		$sql .= ' WHERE '.implode(' AND ', $where);
	$sql .= " ORDER BY class_id, id, parent";

	return db_query($sql, "could not get account types");
}

/**
 * @return array<array-key, null|string>|false
 */
function get_account_type(string|int|float|bool|array|null $id)
{
	$sql = "SELECT * FROM ".TB_PREF."chart_types WHERE id = ".db_escape($id);

	$result = db_query($sql, "could not get account type");

	return db_fetch($result);
}

/**
 * @return false|null|string
 */
function get_account_type_name(string|array|null $id)
{
	$sql = "SELECT name FROM ".TB_PREF."chart_types WHERE id = ".db_escape($id);

	$result = db_query($sql, "could not get account type");

	$row = db_fetch_row($result);
	return is_array($row) ? $row[0] : false;
}

function delete_account_type(string|int|float|bool|null $id): void
{
	$sql = "DELETE FROM ".TB_PREF."chart_types WHERE id = ".db_escape($id);

	db_query($sql, "could not delete account type");
}

/**
 * @return bool|mysqli_result
 */
function add_account_class(string|int|float|bool|null $id, string|int|float|bool|null $name, string|int|float|bool|null $ctype)
{
	$sql = "INSERT INTO ".TB_PREF."chart_class (cid, class_name, ctype)
		VALUES (".db_escape($id).", ".db_escape($name).", ".db_escape($ctype).")";

	return db_query($sql);
}

/**
 * @return bool|mysqli_result
 */
function update_account_class(string|int|float|bool|null $id, string|int|float|bool|null $name, string|int|float|bool|null $ctype)
{
    $sql = "UPDATE ".TB_PREF."chart_class SET class_name=".db_escape($name).",
		ctype=".db_escape($ctype)." WHERE cid = ".db_escape($id);

	return db_query($sql);
}

/**
 * @return bool|mysqli_result
 */
function get_account_classes(int|bool|array|null $all=false, ?int $balance=-1)
{
	$sql = "SELECT * FROM ".TB_PREF."chart_class";

	if (!(bool)$all) 
		$sql .= " WHERE !inactive";
	if ($balance == 0) 
		$sql .= " AND ctype>".CL_EQUITY." OR ctype=0";
	elseif ($balance == 1) 
		$sql .= " AND ctype>0 AND ctype<".CL_INCOME; 
	$sql .= " ORDER BY ctype, cid";

	return db_query($sql, "could not get account classes");
}

/**
 * @return array<array-key, null|string>|false
 */
function get_account_class(string|int|float|bool|null $id)
{
	$sql = "SELECT * FROM ".TB_PREF."chart_class WHERE cid = ".db_escape($id);

	$result = db_query($sql, "could not get account type");

	return db_fetch($result);
}

/**
 * @return null|string
 */
function get_account_class_name(?string $id)
{
	$sql = "SELECT class_name FROM ".TB_PREF."chart_class WHERE cid =".db_escape($id);

	$result = db_query($sql, "could not get account type");

	$row = row_or_empty(db_fetch_row($result));
	return $row[0];
}

function delete_account_class(string|int|float|bool|null $id): void
{
	$sql = "DELETE FROM ".TB_PREF."chart_class WHERE cid = ".db_escape($id);

	db_query($sql, "could not delete account type");
}

