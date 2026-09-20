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
function add_credit_status(string|int|float|bool|null $description, string|int|float|bool|null $disallow_invoicing): void
{
	$sql = "INSERT INTO ".TB_PREF."credit_status (reason_description, dissallow_invoices) 
		VALUES (".db_escape($description).",".db_escape($disallow_invoicing).")";

	db_query($sql, "could not add credit status");
}

function update_credit_status(string|int|float|bool|null $status_id, string|int|float|bool|null $description, string|int|float|bool|null $disallow_invoicing): void
{
	$sql = "UPDATE ".TB_PREF."credit_status SET reason_description=".db_escape($description).",
		dissallow_invoices=".db_escape($disallow_invoicing)." WHERE id=".db_escape($status_id);

	db_query($sql, "could not update credit status");
}

/**
 * @return mysqli_result
 */
function get_all_credit_status(int|bool|array|null $all=false)
{
	$sql = "SELECT * FROM ".TB_PREF."credit_status";
	if (!(bool)$all) $sql .= " WHERE !inactive";

	return db_select($sql, "could not get all credit status");
}

/**
 * @return array<array-key, null|string>|false
 */
function get_credit_status(string|int|float|bool|null $status_id)
{
	$sql = "SELECT * FROM ".TB_PREF."credit_status WHERE id=".db_escape($status_id);

	$result = db_select($sql, "could not get credit status");

	return db_fetch($result);
}

function delete_credit_status(string|int|float|bool|null $status_id): void
{
	$sql="DELETE FROM ".TB_PREF."credit_status WHERE id=".db_escape($status_id);

	db_query($sql, "could not delete credit status");
}

