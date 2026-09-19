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
//--------------------------------------------------------------------------------------------------

/**
 * @return bool|mysqli_result
 */
function get_comments(string|int|float|bool|array|null $type, string|int|float|bool|null $type_no)
{
	$sql = "SELECT * FROM ".TB_PREF."comments WHERE type="
		.db_escape($type)." AND id=".db_escape($type_no);

	return db_query($sql, "could not query comments transaction table");
}

//--------------------------------------------------------------------------------------------------

function add_comments(string|int|float|bool|null $type, string|int|float|bool|null $type_no, string|int|float|bool|array|null $date_, string|int|float|bool|array|null $memo_): void
{
	if ($memo_ != null && $memo_ != "")
	{
    	$date = date2sql($date_);
    	$sql = "INSERT INTO ".TB_PREF."comments (type, id, date_, memo_)
    		VALUES (".db_escape($type).", ".db_escape($type_no)
			.", '$date', ".db_escape($memo_).")";

    	db_query($sql, "could not add comments transaction entry");
	}
}

//--------------------------------------------------------------------------------------------------

function update_comments(string|int|float|bool|null $type, string|int|float|bool|null $id, string|int|float|bool|null $date_, string|int|float|bool|null $memo_): void
{
	if ($date_ == null)
	{
    	delete_comments($type, $id);
    	add_comments($type, $id, Today(), $memo_);
	}
	else
	{
		$date = date2sql($date_);
    	$sql = "UPDATE ".TB_PREF."comments SET memo_=".db_escape($memo_)
			." WHERE type=".db_escape($type)." AND id=".db_escape($id)
			." AND date_='$date'";
    	db_query($sql, "could not update comments");
	}
}

//--------------------------------------------------------------------------------------------------

function delete_comments(string|int|float|bool|null $type, string|int|float|bool|array|null $type_no): void
{
	$sql = "DELETE FROM ".TB_PREF."comments WHERE type=".db_escape($type)
		." AND id=".db_escape($type_no);

	db_query($sql, "could not delete from comments transaction table");
}

//--------------------------------------------------------------------------------------------------

