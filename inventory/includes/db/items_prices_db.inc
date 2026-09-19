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
function add_item_price(mixed $stock_id, string|array|null $sales_type_id, mixed $curr_abrev, mixed $price): void
{
	$sql = "INSERT INTO ".TB_PREF."prices (stock_id, sales_type_id, curr_abrev, price) 
		VALUES (".db_escape($stock_id).", ".db_escape($sales_type_id)
		.", ".db_escape($curr_abrev).", ".db_escape($price).")";

	db_query($sql,"an item price could not be added");
}

function update_item_price(mixed $price_id, string|array|null $sales_type_id, mixed $curr_abrev, mixed $price): void
{
	$sql = "UPDATE ".TB_PREF."prices SET sales_type_id=".db_escape($sales_type_id).", 
		curr_abrev=".db_escape($curr_abrev).", 
		price=".db_escape($price)." WHERE id=".db_escape($price_id);

	db_query($sql,"an item price could not be updated");
}

function delete_item_price(mixed $price_id): void
{
	$sql="DELETE FROM ".TB_PREF."prices WHERE id= ".db_escape($price_id);
	db_query($sql,"an item price could not be deleted");
}

/**
 * @return bool|mysqli_result
 */
function get_prices(mixed $stock_id)
{
	$sql = "SELECT pricelist.sales_type, price.* 
		FROM ".TB_PREF."prices price, "
			.TB_PREF."sales_types pricelist
		WHERE price.sales_type_id = pricelist.id
		AND stock_id=".db_escape($stock_id)
		." ORDER BY curr_abrev, sales_type_id";

	return db_query($sql,"item prices could not be retreived");
}

/**
 * @return array<array-key, null|string>|false
 */
function get_stock_price(mixed $price_id)
{
	$sql = "SELECT * FROM ".TB_PREF."prices WHERE id=".db_escape($price_id);

	$result = db_query($sql,"price could not be retreived");

	return db_fetch($result);
}

/**
 * @return array<array-key, null|string>|false
 */
function get_stock_price_type_currency(mixed $stock_id, string|array|null $type, mixed $currency)
{
	$sql = "SELECT * FROM ".TB_PREF."prices WHERE stock_id=".db_escape($stock_id)."
		AND sales_type_id=".db_escape($type)."
		AND curr_abrev=".db_escape($currency);

	$result = db_query($sql,"price could not be retreived");

	return db_fetch($result);
}
