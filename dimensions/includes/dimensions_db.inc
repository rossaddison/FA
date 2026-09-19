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
function add_dimension(mixed $reference, mixed $name, mixed $type_, mixed $date_, mixed $due_date, mixed $memo_)
{
	global $Refs;

	begin_transaction();

	$date = date2sql($date_);
	$duedate = date2sql($due_date);

	$sql = "INSERT INTO ".TB_PREF."dimensions (reference, name, type_, date_, due_date)
		VALUES (".db_escape($reference).", ".db_escape($name).", ".db_escape($type_)
		.", '$date', '$duedate')";
	db_query($sql, "could not add dimension");
	$id = db_insert_id();

	add_comments(ST_DIMENSION, $id, $date_, $memo_);

	$Refs->save(ST_DIMENSION, $id, $reference);

	commit_transaction();

	return $id;
}

function update_dimension(mixed $id, mixed $name, mixed $type_, mixed $date_, mixed $due_date, mixed $memo_)
{
	begin_transaction();

	$date = date2sql($date_);
	$duedate = date2sql($due_date);

	$sql = "UPDATE ".TB_PREF."dimensions SET name=".db_escape($name).",
		type_ = ".db_escape($type_).",
		date_='$date',
		due_date='$duedate'
		WHERE id = ".db_escape($id);

	db_query($sql, "could not update dimension");

	update_comments(ST_DIMENSION, $id, null, $memo_);

	commit_transaction();

	return $id;
}

function delete_dimension(mixed $id): void
{
	begin_transaction();

	// delete the actual dimension
	$sql="DELETE FROM ".TB_PREF."dimensions WHERE id=".db_escape($id);
	db_query($sql,"The dimension could not be deleted");

	delete_comments(ST_DIMENSION, $id);

	commit_transaction();
}

//--------------------------------------------------------------------------------------

function get_dimension(mixed $id, mixed $allow_null=false)
{
    $sql = "SELECT * FROM ".TB_PREF."dimensions	WHERE id=".db_escape($id);

	$result = db_query($sql, "The dimension could not be retrieved");

	if (!$allow_null && db_num_rows($result) == 0)
		display_db_error("Could not find dimension $id", $sql);

	return db_fetch($result);
}

//--------------------------------------------------------------------------------------

function get_dimension_string(mixed $id, mixed $html=false, mixed $space=' '): string
{
	if ($id <= 0)
	{
		if ($html)
			$dim = "&nbsp;";
		else
			$dim = "";
	}
	else
	{
		$row = get_dimension($id, true);
		$dim = $row['reference'] . $space . $row['name'];
	}

	return $dim;
}

//--------------------------------------------------------------------------------------

function get_dimensions()
{
	$sql = "SELECT * FROM ".TB_PREF."dimensions ORDER BY date_";

	return db_query($sql, "The dimensions could not be retrieved");
}

//--------------------------------------------------------------------------------------

function dimension_has_deposits(mixed $id)
{
	return dimension_has_payments($id);
}

//--------------------------------------------------------------------------------------

function dimension_has_payments(mixed $id): bool
{
	$sql = "SELECT COUNT(*) FROM ".TB_PREF."gl_trans WHERE dimension_id = ".db_escape($id)
	 . " OR dimension2_id = ".db_escape($id);
	$res = db_query($sql, "Transactions could not be calculated");
	$row = db_fetch_row($res);
	return ($row[0] > 0);
}

function dimension_is_closed(mixed $id): bool
{
	$result = get_dimension($id);
	return ($result['closed'] == '1');
}

//--------------------------------------------------------------------------------------

function close_dimension(mixed $id): void
{
	$sql = "UPDATE ".TB_PREF."dimensions SET closed='1' WHERE id = ".db_escape($id);
	db_query($sql, "could not close dimension");
}

//--------------------------------------------------------------------------------------

function reopen_dimension(mixed $id): void
{
	$sql = "UPDATE ".TB_PREF."dimensions SET closed='0' WHERE id = ".db_escape($id);
	db_query($sql, "could not reopen dimension");
}

//--------------------------------------------------------------------------------------

function get_dimension_balance_all(mixed $id, mixed $from, mixed $to) 
{
	$from = date2sql($from);
	$to = date2sql($to);
	$sql = "SELECT account, coa.account_name, sum(amount) AS amt 
		FROM "
		.TB_PREF."gl_trans trans,"
		.TB_PREF."chart_master coa
		WHERE
			trans.account = coa.account_code
		AND	(dimension_id = ".db_escape($id)." OR dimension2_id = ".db_escape($id).")
		AND	tran_date >= '$from' AND tran_date <= '$to' GROUP BY account";
	return db_query($sql, "Transactions could not be calculated");
}

//--------------------------------------------------------------------------------------

function get_dimension_balance(mixed $id, mixed $from, mixed $to) 
{
	$id = db_escape($id);
	$sql = "SELECT SUM(amount)
			FROM ".TB_PREF."gl_trans 
			WHERE
				tran_date >= '" .	date2sql($from) . "' 
			AND	tran_date <= '" . date2sql($to) . "' 
			AND (dimension_id = $id OR dimension2_id = $id)";
	$res = db_query($sql, "Sum of transactions could not be calculated");
	$row = db_fetch_row($res);

	return $row[0];
}

//--------------------------------------------------------------------------------------

function get_sql_for_search_dimensions(mixed $dim, mixed $from, mixed $to, mixed $order='', mixed $type = -1, mixed $open=false, mixed $overdue = false): string
{
	$sql = "SELECT dim.id,
		dim.reference,
		dim.name,
		dim.type_,
		dim.date_,
		dim.due_date,
		dim.closed
		FROM ".TB_PREF."dimensions as dim WHERE id > 0";

	if ($order)
	{
		$sql .= " AND reference LIKE ".db_escape("%". $order . "%");
	} else {

		if ($dim == 1)
			$sql .= " AND type_=1";

		if ($open)
			$sql .= " AND closed=0";

		if ($type > 0)
			$sql .= " AND type_=".db_escape($type);

		if ($overdue)
		{
			$today = date2sql(Today());

			$sql .= " AND due_date < '$today'";
		}

		$sql .= " AND date_ >= '" . date2sql($from) . "'
			AND date_ <= '" . date2sql($to) . "'";
	}
	return $sql;
}
