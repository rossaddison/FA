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
$page_security = 'SA_SUPPLIERALLOC';
/** @var string $path_to_root */
$path_to_root = "../..";
include($path_to_root . "/includes/db_pager.inc");
include_once($path_to_root . "/includes/session.inc");

include_once($path_to_root . "/includes/date_functions.inc");

include_once($path_to_root . "/includes/ui.inc");
include_once($path_to_root . "/includes/banking.inc");

include_once($path_to_root . "/sales/includes/sales_db.inc");
$js = "";
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(900, 500);
page(_($help_context = "Supplier Allocations"), false, false, "", $js);

//--------------------------------------------------------------------------------

start_form();

	/* show all outstanding receipts and credits to be allocated */
    if (!isset($_POST['supplier_id']))
    	$_POST['supplier_id'] = get_global_supplier();

    echo "<center>" . _("Select a Supplier: ") . "&nbsp;&nbsp;";
	echo supplier_list('supplier_id', $_POST['supplier_id'], true, true);
    echo "<br>";
    check(_("Show Settled Items:"), 'ShowSettled', null, true);
	echo "</center><br><br>";
	set_global_supplier($_POST['supplier_id']);

	if (isset($_POST['supplier_id']) && ($_POST['supplier_id'] == ALL_TEXT)) 
	{
		unset($_POST['supplier_id']);
	}

	$settled = false;
	if (check_value('ShowSettled'))
		$settled = true;
	$supplier_id = null;
	if (isset($_POST['supplier_id']))
		$supplier_id = $_POST['supplier_id'];

//--------------------------------------------------------------------------------
/** @return string */
function systype_name(string|int|float|bool|array|null $dummy, string|int|float|bool|null $type)
{
	global $systypes_array;

	return $systypes_array[$type];
}

function trans_view(array $trans)
{
	return get_trans_view_str($trans["type"], $trans["trans_no"]);
}

function alloc_link(array|false|null $row): string
{
	return pager_link(_("Allocate"),
		"/purchasing/allocations/supplier_allocate.php?trans_no="
 			.(string)$row["trans_no"] . "&trans_type=" . (string)$row["type"]. "&supplier_id=" . (string)$row["supplier_id"], ICON_ALLOC);
}

function amount_left(array|false|null $row): string
{
 	return price_format($row['type'] == ST_JOURNAL ?  (float)abs($row["Total"])-(float)$row["alloc"] : (float)(-$row["Total"])-(float)$row["alloc"]);
}

function amount_total(array|false|null $row): string
{
	return price_format(-$row["Total"]);
}

function check_settled(array|false|null $row): bool
{
	return $row['settled'] == 1;
}


$sql = get_allocatable_from_supp_sql($supplier_id, $settled);

$cols = array(
	_("Transaction Type") => array('fun'=>'systype_name'),
	_("#") => array('fun'=>'trans_view', 'align'=>'right'),
	_("Reference"), 
	_("Date") => array('name'=>'tran_date', 'type'=>'date', 'ord'=>'asc'),
	_("Supplier") => array('ord'=>''),
	_("Currency") => array('align'=>'center'),
	_("Total") => array('align'=>'right', 'fun'=>'amount_total'), 
	_("Left to Allocate") => array('align'=>'right','insert'=>true, 'fun'=>'amount_left'), 
	array('insert'=>true, 'fun'=>'alloc_link')
	);

if (isset($_POST['customer_id'])) {
	$cols[_("Supplier")] = 'skip';
	$cols[_("Currency")] = 'skip';
}

$table =& new_db_pager('alloc_tbl', $sql, $cols);
$table->set_marker('check_settled', _("Marked items are settled."), 'settledbg', 'settledfg');

$table->width = "80%";

display_db_pager($table);

end_form();
end_page();
