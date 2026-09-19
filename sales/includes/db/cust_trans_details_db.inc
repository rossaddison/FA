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
//----------------------------------------------------------------------------------------

/**
 * @return bool|mysqli_result
 */
function get_customer_trans_details(string|int|float|bool|null $debtor_trans_type, string|int|float|bool|array|null $debtor_trans_no)
{
if (!is_array($debtor_trans_no))
	$debtor_trans_no = array( 0=>$debtor_trans_no );

	$sql = "SELECT line.*,
		line.unit_price+line.unit_tax AS FullUnitPrice,
        line.unit_price AS UnitPriceExTax,  
		line.description AS StockDescription,
		item.long_description AS StockLongDescription,
		item.units, item.mb_flag
		FROM "
			.TB_PREF."debtor_trans_details line,"
			.TB_PREF."stock_master item
		WHERE (";

	$tr=array();
	foreach ($debtor_trans_no as $trans_no)
		$tr[] = 'debtor_trans_no='.db_escape($trans_no);

	$sql .= implode(' OR ', $tr);


	$sql.=	") AND debtor_trans_type=".db_escape($debtor_trans_type)."
		AND item.stock_id=line.stock_id
		ORDER BY id";
	return db_query($sql, "The debtor transaction detail could not be queried");
}

//----------------------------------------------------------------------------------------

function void_customer_trans_details(string|int|float|bool|null $type, string|int|float|bool|null $type_no): void
{
	$sql = "UPDATE ".TB_PREF."debtor_trans_details SET quantity=0, unit_price=0,
		unit_tax=0, discount_percent=0, standard_cost=0, src_id=0
		WHERE debtor_trans_no=".db_escape($type_no)."
		AND debtor_trans_type=".db_escape($type);

	db_query($sql, "The debtor transaction details could not be voided");

	// clear the stock move items
	void_stock_move($type, $type_no);
}
//----------------------------------------------------------------------------------------

function write_customer_trans_detail_item(?int $debtor_trans_type, string|int|float|bool|null $debtor_trans_no, string|int|float|bool|null $stock_id, string|int|float|bool|null $description,
	string|int|float|bool|null $quantity, string|int|float|bool|null $unit_price, string|int|float|bool|null $unit_tax, string|int|float|bool|null $discount_percent, string|int|float|bool|null $std_cost, string|int|float|bool|null $src_id, string|int|float|bool|null $line_id=0): void
{
	if ($line_id!=0)
		$sql = "UPDATE ".TB_PREF."debtor_trans_details SET
			stock_id=".db_escape($stock_id).",
			description=".db_escape($description).",
			quantity=$quantity,
			unit_price=$unit_price,
			unit_tax=$unit_tax,
			discount_percent=$discount_percent,
			standard_cost=$std_cost,
			src_id=".db_escape($src_id)." WHERE
			id=".db_escape($line_id);
	else
			$sql = "INSERT INTO ".TB_PREF."debtor_trans_details (debtor_trans_no,
				debtor_trans_type, stock_id, description, quantity, unit_price,
				unit_tax, discount_percent, standard_cost, src_id)
			VALUES (".db_escape($debtor_trans_no).", ".db_escape($debtor_trans_type).", ".db_escape($stock_id).
			", ".db_escape($description).",
				$quantity, $unit_price, $unit_tax, 
				$discount_percent, $std_cost,".db_escape($src_id).")";

	db_query($sql, "The debtor transaction detail could not be written");
}

