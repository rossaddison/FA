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

$page_security = 'SA_GLANALYTIC';
/** @var string $path_to_root */
$path_to_root="../..";

include(dirname(__DIR__, 2) . "/includes/db_pager.inc");
include_once(dirname(__DIR__, 2) . "/includes/session.inc");

include_once(dirname(__DIR__, 2) . "/includes/date_functions.inc");
include_once(dirname(__DIR__, 2) . "/includes/ui.inc");
$js = "";
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(800, 500);
if ((bool) user_use_date_picker())
	$js .= get_js_date_picker();

page(_($help_context = "Journal Inquiry"), false, false, "", $js);

//-----------------------------------------------------------------------------------
// Ajax updates
//
if (get_post('Search'))
{
	ajax()->activate('journal_tbl');
}
//--------------------------------------------------------------------------------------
if (!isset($_POST['filterType']))
	$_POST['filterType'] = -1;

start_form();

start_table(TABLESTYLE_NOBORDER);
start_row();

ref_cells(_("Reference:"), 'Ref', '',null, _('Enter reference fragment or leave empty'));

journal_types_list_cells(_("Type:"), "filterType");
date_cells(_("From:"), 'FromDate', '', null, -user_transaction_days());
date_cells(_("To:"), 'ToDate');

end_row();
start_row();
ref_cells(_("Memo:"), 'Memo', '',null, _('Enter memo fragment or leave empty'));
users_list_cells(_("User:"), 'userid', null, false);
if ((bool) get_company_pref('use_dimension') && isset($_POST['dimension'])) // display dimension only, when started in dimension mode
	dimensions_list_cells(_('Dimension:'), 'dimension', null, true, null, true);
check_cells( _("Show closed:"), 'AlsoClosed', null);
submit_cells('Search', _("Search"), '', '', 'default');
end_row();
end_table();

/** @psalm-pure */
function journal_pos(array $row): string
{
	return (bool)$row['gl_seq'] ? (string) $row['gl_seq'] : '-';
}

/** @return string */
function systype_name(string|int|float|bool|array $dummy, string|int|float|bool|null $type)
{
	global $systypes_array;
	/** @var array<int, string> $systypes_array */
	
	return $systypes_array[(int) $type];
}

function person_link(array $row): string|int|float|bool|null
{
    return payment_person_name(is_scalar($row["person_type_id"]) ? $row["person_type_id"] : null, is_scalar($row["person_id"]) ? $row["person_id"] : null);
}

function view_link(array $row): ?string
{
	return get_trans_view_str(is_scalar($row["trans_type"]) ? $row["trans_type"] : null, (int) $row["trans_no"]);
}

function gl_link(array $row): string|int|float|bool|null
{
	return get_gl_view_str(is_scalar($row["trans_type"]) ? $row["trans_type"] : null, (int) $row["trans_no"]);
}

function edit_link(array $row): string
{

	$ok = true;
	if ($row['trans_type'] == ST_SALESINVOICE)
	{
		$myrow = row_or_empty(get_customer_trans(is_scalar($row["trans_no"]) ? $row["trans_no"] : null, is_scalar($row["trans_type"]) ? $row["trans_type"] : null));
		if ($myrow['alloc'] != $myrow['Total'] || get_voided_entry(ST_SALESINVOICE, (int) $row["trans_no"]) !== false)
			$ok = false;
	}

	return $ok ? trans_editor_link( is_scalar($row["trans_type"]) ? $row["trans_type"] : null, (int) $row["trans_no"]) : '--';
}

/** @psalm-pure */
function invoice_supp_reference(array $row): string|int|float|bool|null
{
	return is_scalar($row['supp_reference']) ? $row['supp_reference'] : null;
}

$filter_type_post = get_post('filterType', -1);
$from_date_post = get_post('FromDate');
$to_date_post = get_post('ToDate');
$userid_post = get_post('userid');
$sql = get_sql_for_journal_inquiry(is_string($filter_type_post) || is_int($filter_type_post) ? $filter_type_post : null,
	is_string($from_date_post) ? $from_date_post : null,
	is_string($to_date_post) ? $to_date_post : null, get_post('Ref'), get_post('Memo'), check_value('AlsoClosed'),
	is_string($userid_post) ? $userid_post : null);

$cols = array(
	_("#") => array('fun'=>'journal_pos', 'align'=>'center'), 
	_("Date") =>array('name'=>'tran_date','type'=>'date','ord'=>'desc'),
	_("Type") => array('fun'=>'systype_name'), 
	_("Trans #") => array('fun'=>'view_link'), 
	_("Counterparty") => array('fun' => 'person_link'),
	_("Supplier's Reference") => 'skip',
	_("Reference"), 
	_("Amount") => array('type'=>'amount'),
	_("Memo"),
	_("User"),
	_("View") => array('insert'=>true, 'fun'=>'gl_link'),
	array('insert'=>true, 'fun'=>'edit_link')
);

if (!check_value('AlsoClosed')) {
	$cols[_("#")] = 'skip';
}

if($_POST['filterType'] == ST_SUPPINVOICE) //add the payment column if shown supplier invoices only
{
	$cols[_("Supplier's Reference")] = array('fun'=>'invoice_supp_reference', 'align'=>'center');
}

$table = new_db_pager('journal_tbl', $sql, $cols);

$table->width = "80%";

display_db_pager($table);

end_form();
end_page();

