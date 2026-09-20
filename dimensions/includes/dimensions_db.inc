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
function add_dimension(string|int|float|bool|null $reference, string|int|float|bool|null $name, string|int|float|bool|null $type_, string|int|float|bool|null $date_, string|int|float|bool|null $due_date, string|int|float|bool|null $memo_): int|string
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

function update_dimension(string|int|float|bool|null $id, string|int|float|bool|null $name, string|int|float|bool|null $type_, string|int|float|bool|null $date_, string|int|float|bool|null $due_date, string|int|float|bool|null $memo_)
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

function delete_dimension(string|int|float|bool|null $id): void
{
	begin_transaction();

	// delete the actual dimension
	$sql="DELETE FROM ".TB_PREF."dimensions WHERE id=".db_escape($id);
	db_query($sql,"The dimension could not be deleted");

	delete_comments(ST_DIMENSION, $id);

	commit_transaction();
}

//--------------------------------------------------------------------------------------

/**
 * @return array<array-key, null|string>|false
 */
function get_dimension(string|int|float|bool|array|null $id, bool $allow_null=false)
{
    $sql = "SELECT * FROM ".TB_PREF."dimensions	WHERE id=".db_escape($id);

	$result = db_query($sql, "The dimension could not be retrieved");

	if (!$allow_null && db_num_rows($result) == 0)
		display_db_error("Could not find dimension $id", $sql);

	return db_fetch($result);
}

//--------------------------------------------------------------------------------------

function get_dimension_string(int|string|null $id, bool $html=false, ?string $space=' '): string
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
		$dim = (string)$row['reference'] . $space . (string)$row['name'];
	}

	return $dim;
}

//--------------------------------------------------------------------------------------

/**
 * @return bool|mysqli_result
 */
function get_dimensions()
{
	$sql = "SELECT * FROM ".TB_PREF."dimensions ORDER BY date_";

	return db_query($sql, "The dimensions could not be retrieved");
}

//--------------------------------------------------------------------------------------

function dimension_has_deposits(string|int|float|bool|null $id): bool
{
	return dimension_has_payments($id);
}

//--------------------------------------------------------------------------------------

function dimension_has_payments(string|int|float|bool|null $id): bool
{
	$sql = "SELECT COUNT(*) FROM ".TB_PREF."gl_trans WHERE dimension_id = ".db_escape($id)
	 . " OR dimension2_id = ".db_escape($id);
	$res = db_query($sql, "Transactions could not be calculated");
	$row = db_fetch_row($res);
	return ($row[0] > 0);
}

function dimension_is_closed(string|array|null $id): bool
{
	$result = get_dimension($id);
	return ($result['closed'] == '1');
}

//--------------------------------------------------------------------------------------

function close_dimension(string|int|float|bool|null $id): void
{
	$sql = "UPDATE ".TB_PREF."dimensions SET closed='1' WHERE id = ".db_escape($id);
	db_query($sql, "could not close dimension");
}

//--------------------------------------------------------------------------------------

function reopen_dimension(string|int|float|bool|null $id): void
{
	$sql = "UPDATE ".TB_PREF."dimensions SET closed='0' WHERE id = ".db_escape($id);
	db_query($sql, "could not reopen dimension");
}

//--------------------------------------------------------------------------------------

/**
 * @return bool|mysqli_result
 */
function get_dimension_balance_all(string|int|float|bool|null $id, string|int|float|bool|null $from, string|int|float|bool|null $to) 
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

/**
 * @return null|string
 */
function get_dimension_balance(string|int|float|bool|null $id, string|array|null $from, string|array|null $to) 
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

function get_sql_for_search_dimensions(string|int|float|bool|null $dim, string|array|null $from, string|array|null $to, string|int|float|bool|null $order='', string|int|array|null $type = -1, int|bool|array|null $open=false, int|bool|array|null $overdue = false): string
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
