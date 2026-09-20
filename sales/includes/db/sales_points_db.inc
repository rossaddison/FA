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
function add_sales_point(string|int|float|bool|null $name, string|int|float|bool|null $location, string|int|float|bool|null $account, int|array|null $cash, int|array|null $credit): void
{
	$sql = "INSERT INTO ".TB_PREF."sales_pos (pos_name, pos_location, pos_account, cash_sale, credit_sale) VALUES (".db_escape($name)
			. ",".db_escape($location).",".db_escape($account)
			. ",$cash,$credit)";
	db_query($sql, "could not add point of sale");
}

function update_sales_point(string|int|float|bool|null $id, string|int|float|bool|null $name, string|int|float|bool|null $location, string|int|float|bool|null $account, int|array|null $cash, int|array|null $credit): void
{

	$sql = "UPDATE ".TB_PREF."sales_pos SET pos_name=".db_escape($name)
				.",pos_location=".db_escape($location)
				.",pos_account=".db_escape($account)
				.",cash_sale =$cash"
				.",credit_sale =$credit"
		 		." WHERE id = ".db_escape($id);
	
	db_query($sql, "could not update sales type");			
}

/**
 * @return bool|mysqli_result
 */
function get_all_sales_points(int|bool|array|null $all=false)
{
	$sql = "SELECT pos.*, loc.location_name, acc.bank_account_name FROM "
		.TB_PREF."sales_pos as pos
		LEFT JOIN ".TB_PREF."locations as loc on pos.pos_location=loc.loc_code
		LEFT JOIN ".TB_PREF."bank_accounts as acc on pos.pos_account=acc.id";
	if (!(bool)$all) $sql .= " WHERE !pos.inactive";
	
	return db_select($sql, "could not get all POS definitions");
} 

/**
 * @return array<array-key, null|string>|false
 */
function get_sales_point(string|int|float|bool|null $id)
{
	$sql = "SELECT pos.*, loc.location_name, acc.bank_account_name FROM "
		.TB_PREF."sales_pos as pos
		LEFT JOIN ".TB_PREF."locations as loc on pos.pos_location=loc.loc_code
		LEFT JOIN ".TB_PREF."bank_accounts as acc on pos.pos_account=acc.id
		WHERE pos.id=".db_escape($id);
	
	$result = db_select($sql, "could not get POS definition");
	
	return db_fetch($result);
}

/**
 * @return false|null|string
 */
function get_sales_point_name(string|int|float|bool|null $id)
{
	$sql = "SELECT pos_name FROM ".TB_PREF."sales_pos WHERE id=".db_escape($id);
	
	$result = db_select($sql, "could not get POS name");
	
	$row = db_fetch_row($result);
	return is_array($row) ? $row[0] : false;
}

function delete_sales_point(string|int|float|bool|null $id): void
{
	$sql="DELETE FROM ".TB_PREF."sales_pos WHERE id=".db_escape($id);
	db_query($sql,"The point of sale record could not be deleted");
}

