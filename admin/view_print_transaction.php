<?php
declare(strict_types=1);
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
$page_security = 'SA_VIEWPRINTTRANSACTION';
/** @var string $path_to_root */
$path_to_root = "..";

include(dirname(__DIR__) . "/includes/db_pager.inc");
include_once(dirname(__DIR__) . "/includes/session.inc");

include_once(dirname(__DIR__) . "/includes/date_functions.inc");
include_once(dirname(__DIR__) . "/includes/ui.inc");
include_once(dirname(__DIR__) . "/includes/data_checks.inc");
include_once(dirname(__DIR__) . "/admin/db/transactions_db.inc");

include_once(dirname(__DIR__) . "/reporting/includes/reporting.inc");
$js = "";
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(800, 500);
page(_($help_context = "View or Print Transactions"), false, false, "", $js);

//----------------------------------------------------------------------------------------
function view_link(array $trans): ?string
{
	if (!isset($trans['type']))
		$trans['type'] = $_POST['filterType'];
	return get_trans_view_str(is_scalar($trans["type"]) ? $trans["type"] : null, is_scalar($trans["trans_no"]) ? $trans["trans_no"] : null);
}

/** @return non-empty-string|null */
function prt_link(array $row)
{
	if (!isset($row['type']))
		$row['type'] = $_POST['filterType'];
	$type = is_scalar($row['type']) ? $row['type'] : null;
	$trans_no = is_scalar($row['trans_no']) ? $row['trans_no'] : null;
  	if ($row['type'] == ST_PURCHORDER || $row['type'] == ST_SALESORDER || $row['type'] == ST_SALESQUOTE ||
  		$row['type'] == ST_WORKORDER)
 		return print_document_link($trans_no, _("Print"), true, $type, ICON_PRINT);
 	else
		return print_document_link(((string) $trans_no)."-".((string) $type), _("Print"), true, $type, ICON_PRINT);
}

function gl_view(array $row): string|int|float|bool|null
{
	if (!isset($row['type']))
		$row['type'] = $_POST['filterType'];
	return get_gl_view_str(is_scalar($row["type"]) ? $row["type"] : null, (int) $row["trans_no"]);
}

function date_view(array $row): string|int|float|bool|null
{
	return is_scalar($row['trans_date']) ? $row['trans_date'] : null;
}

function ref_view(array $row): string|int|float|bool|null
{
	return is_scalar($row['ref']) ? $row['ref'] : null;
}

function viewing_controls(): void
{
	display_note(_("Only documents can be printed."));

    start_table(TABLESTYLE_NOBORDER);
	start_row();

	systypes_list_cells(_("Type:"), 'filterType', null, true, array(ST_CUSTOMER, ST_SUPPLIER));

	if (!isset($_POST['FromTransNo']))
		$_POST['FromTransNo'] = "1";
	if (!isset($_POST['ToTransNo']))
		$_POST['ToTransNo'] = "999999";

    ref_cells(_("from #:"), 'FromTransNo');

    ref_cells(_("to #:"), 'ToTransNo');

    submit_cells('ProcessSearch', _("Search"), '', '', 'default');

	end_row();
    end_table(1);

}

//----------------------------------------------------------------------------------------

function check_valid_entries(): bool
{
	if (!is_numeric($_POST['FromTransNo']) OR $_POST['FromTransNo'] <= 0)
	{
		display_error(_("The starting transaction number is expected to be numeric and greater than zero."));
		return false;
	}

	if (!is_numeric($_POST['ToTransNo']) OR $_POST['ToTransNo'] <= 0)
	{
		display_error(_("The ending transaction number is expected to be numeric and greater than zero."));
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------------------

function handle_search(): void
{
	if (check_valid_entries()==true)
	{
		$trans_ref = null;
		$sql = get_sql_for_view_transactions(post_scalar('filterType'), post_scalar('FromTransNo'), post_scalar('ToTransNo'), $trans_ref);
		if ($sql == "")
			return;

		$print_type = get_post('filterType');
		$print_out = ($print_type == ST_SALESINVOICE || $print_type == ST_CUSTCREDIT || $print_type == ST_CUSTDELIVERY ||
			$print_type == ST_PURCHORDER || $print_type == ST_SALESORDER || $print_type == ST_SALESQUOTE ||
			$print_type == ST_CUSTPAYMENT || $print_type == ST_SUPPAYMENT || $print_type == ST_WORKORDER);

		$cols = array(
			_("#") => array('insert'=>true, 'fun'=>'view_link'), 
			_("Reference") => array('fun'=>'ref_view'), 
			_("Date") => array('type'=>'date', 'fun'=>'date_view'),
			_("Print") => array('insert'=>true, 'fun'=>'prt_link'), 
			_("GL") => array('insert'=>true, 'fun'=>'gl_view')
		);
		if(!$print_out) {
			array_remove($cols, 3);
		}
		if(!(bool) $trans_ref) {
			array_remove($cols, 1);
		}

		$table = new_db_pager('transactions', $sql, $cols);
		$table->width = "40%";
		display_db_pager($table);
	}

}

//----------------------------------------------------------------------------------------

if (isset($_POST['ProcessSearch']))
{
	if (!check_valid_entries())
		unset($_POST['ProcessSearch']);
	ajax()->activate('transactions');
}

//----------------------------------------------------------------------------------------

start_form(false);
	viewing_controls();
	handle_search();
end_form(2);

end_page();

