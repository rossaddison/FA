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
$page_security = 'SA_SUPPLIERINVOICE';
/** @var string $path_to_root */
$path_to_root = "..";

include_once(dirname(__DIR__) . "/purchasing/includes/purchasing_db.inc");

include_once(dirname(__DIR__) . "/includes/session.inc");

include_once(dirname(__DIR__) . "/includes/banking.inc");
include_once(dirname(__DIR__) . "/includes/data_checks.inc");

include_once(dirname(__DIR__) . "/purchasing/includes/purchasing_ui.inc");
$js = "";
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(900, 500);
if ((bool) user_use_date_picker())
	$js .= get_js_date_picker();
//----------------------------------------------------------------------------------------

if (isset($_GET['New']))
{
	if (isset( $_SESSION['supp_trans']))
	{
		unset (session_obj('supp_trans')->grn_items);
		unset (session_obj('supp_trans')->gl_codes);
		unset ($_SESSION['supp_trans']);
	}
	$help_context = "Enter Supplier Invoice";
	$_SESSION['page_title'] = _("Enter Supplier Invoice");

	$_SESSION['supp_trans'] = new supp_trans(ST_SUPPINVOICE);
} else if(isset($_GET['ModifyInvoice'])) {
	$help_context = 'Modifying Purchase Invoice';
	$modify_invoice = $_GET['ModifyInvoice'] ?? null;
	$_SESSION['page_title'] = sprintf( _("Modifying Purchase Invoice # %d"), is_numeric($modify_invoice) ? (int) $modify_invoice : 0);
	$_SESSION['supp_trans'] = new supp_trans(ST_SUPPINVOICE, $_GET['ModifyInvoice']);
}

$page_title = $_SESSION['page_title'] ?? _("Supplier Invoice Entry");
page(is_string($page_title) ? $page_title : _("Supplier Invoice Entry"), false, false, "", $js);

if (isset($_GET['ModifyInvoice']))
	check_is_editable(ST_SUPPINVOICE, get_scalar('ModifyInvoice'));

check_db_has_suppliers(_("There are no suppliers defined in the system."));

//---------------------------------------------------------------------------------------------------------------

if (isset($_GET['AddedID']))
{
	$added_id = $_GET['AddedID'] ?? null;
	$invoice_no = is_scalar($added_id) ? $added_id : '';
	$trans_type = ST_SUPPINVOICE;


    echo "<center>";
    display_notification_centered(_("Supplier invoice has been processed."));
    display_note(get_trans_view_str($trans_type, $invoice_no, _("View this Invoice")));

	display_note(get_gl_view_str($trans_type, $invoice_no, _("View the GL Journal Entries for this Invoice")), 1);

	hyperlink_params("$path_to_root/purchasing/supplier_payment.php", _("Entry supplier &payment for this invoice"),
		"PInvoice=". $invoice_no."&trans_type=".$trans_type);

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter Another Invoice"), "New=1");

	hyperlink_params("$path_to_root/admin/attachments.php", _("Add an Attachment"), "filterType=$trans_type&trans_no=$invoice_no");
	
	display_footer_exit();
}

//--------------------------------------------------------------------------------------------------

if (isset($_GET['New']))
{
	if (isset( $_SESSION['supp_trans']))
	{
		unset (session_obj('supp_trans')->grn_items);
		unset (session_obj('supp_trans')->gl_codes);
		unset ($_SESSION['supp_trans']);
	}

	$_SESSION['supp_trans'] = new supp_trans(ST_SUPPINVOICE);
} else if(isset($_GET['ModifyInvoice'])) {
	$_SESSION['supp_trans'] = new supp_trans(ST_SUPPINVOICE, $_GET['ModifyInvoice']);
}

if (!isset($_SESSION['supp_trans']))
{
	// reached with neither New nor ModifyInvoice, and nothing already in the session from an
	// earlier request on this page (the normal form post-back relies on that instead) --
	// there is nothing to enter or edit
	display_error(_("This page can only be opened to enter a new supplier invoice or to modify an existing one."));
	display_footer_exit();
}

//--------------------------------------------------------------------------------------------------
function clear_fields(): void
{
	
	unset($_POST['gl_code']);
	unset($_POST['dimension_id']);
	unset($_POST['dimension2_id']);
	unset($_POST['amount']);
	unset($_POST['memo_']);
	unset($_POST['AddGLCodeToTrans']);
	ajax()->activate('gl_items');
	set_focus('gl_code');
}

function reset_tax_input(): void
{

	unset($_POST['mantax']);
	ajax()->activate('inv_tot');
}

//------------------------------------------------------------------------------------------------
//	GL postings are often entered in the same form to two accounts
//  so fileds are cleared only on user demand.
//
if (isset($_POST['ClearFields']))
{
	clear_fields();
}

if (isset($_POST['AddGLCodeToTrans'])){

	ajax()->activate('gl_items');
	$input_error = false;
	$gl_act_name = null;
	$gl_code_post = post_scalar('gl_code');

	$result = get_gl_account_info(is_string($gl_code_post) ? $gl_code_post : null);
	if (db_num_rows($result) == 0)
	{
		display_error(_("The account code entered is not a valid code, this line cannot be added to the transaction."));
		set_focus('gl_code');
		$input_error = true;
	}
	else
	{
		$myrow = row_or_empty(db_fetch_row($result));
		$gl_act_name = $myrow[1] ?? null;
		if (!check_num('amount'))
		{
			display_error(_("The amount entered is not numeric. This line cannot be added to the transaction."));
			set_focus('amount');
			$input_error = true;
		}
	}

	if (!is_tax_gl_unique(get_post('gl_code'))) {
   		display_error(_("Cannot post to GL account used by more than one tax type."));
		set_focus('gl_code');
   		$input_error = true;
	}

	if ($input_error == false)
	{
		$dimension_id = $_POST['dimension_id'] ?? null;
		$dimension2_id = $_POST['dimension2_id'] ?? null;
		$memo = $_POST['memo_'] ?? null;
		session_obj('supp_trans')->add_gl_codes_to_trans($gl_code_post, $gl_act_name,
			is_scalar($dimension_id) ? $dimension_id : null, is_scalar($dimension2_id) ? $dimension2_id : null,
			input_num('amount'), is_scalar($memo) ? $memo : null);
		reset_tax_input();
		set_focus('gl_code');
	}
}

//------------------------------------------------------------------------------------------------

#[\NoDiscard]
function check_data(): bool
{
	global $Refs;

	if (!get_post('supplier_id')) 
	{
		display_error(_("There is no supplier selected."));
		set_focus('supplier_id');
		return false;
	} 

	if (!session_obj('supp_trans')->is_valid_trans_to_post())
	{
		display_error(_("The invoice cannot be processed because the there are no items or values on the invoice.  Invoices are expected to have a charge."));
		return false;
	}

	if (!check_reference(session_obj('supp_trans')->reference, ST_SUPPINVOICE, session_obj('supp_trans')->trans_no))
	{
		set_focus('reference');
		return false;
	}

	if (!is_date( session_obj('supp_trans')->tran_date))
	{
		display_error(_("The invoice as entered cannot be processed because the invoice date is in an incorrect format."));
		set_focus('trans_date');
		return false;
	} 
	elseif (!(bool)is_date_in_fiscalyear(session_obj('supp_trans')->tran_date)) 
	{
		display_error(_("The entered date is out of fiscal year or is closed for further data entry."));
		set_focus('trans_date');
		return false;
	}
	if (!is_date( session_obj('supp_trans')->due_date))
	{
		display_error(_("The invoice as entered cannot be processed because the due date is in an incorrect format."));
		set_focus('due_date');
		return false;
	}

	$supp_reference_post = get_post('supp_reference');
	if (trim(is_scalar($supp_reference_post) ? $supp_reference_post : '') == false)
	{
		display_error(_("You must enter a supplier's invoice reference."));
		set_focus('supp_reference');
		return false;
	}

	$supp_reference_scalar = post_scalar('supp_reference');
	$supp_reference_str = is_string($supp_reference_scalar) ? $supp_reference_scalar : null;
	$supp_trans_no = session_obj('supp_trans')->trans_no;
	if (is_reference_already_there(session_obj('supp_trans')->supplier_id, $supp_reference_str, is_scalar($supp_trans_no) ? $supp_trans_no : 0))
	{ 	/*Transaction reference already entered */
		display_error(_("This invoice number has already been entered. It cannot be entered again.") . " (" . (is_scalar($supp_reference_post) ? $supp_reference_post : '') . ")");
		set_focus('supp_reference');
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------

function handle_commit_invoice(): void
{
	$trans = session_obj('supp_trans');
	copy_to_trans($trans);

	if (!check_data())
		return;
	$invoice_no = add_supp_invoice($trans);

    session_obj('supp_trans')->clear_items();
    unset($_SESSION['supp_trans']);

	meta_forward($_SERVER['PHP_SELF'] ?? '', "AddedID=$invoice_no");
}

//--------------------------------------------------------------------------------------------------

if (isset($_POST['PostInvoice']))
{
	handle_commit_invoice();
}

// Named check_item_data_invoice(), not check_item_data(), to avoid the function-name
// collision with the 6 other same-named check_item_data() declarations across the
// codebase (inventory/transfers.php, inventory/adjustments.php, sales/sales_order_entry.php,
// sales/credit_note_entry.php, manufacturing/work_order_issue.php) -- matches the existing
// disambiguation already used for this exact situation in gl/gl_bank.php
// (check_item_data_bank), gl/gl_journal.php (check_item_data_journal), and
// purchasing/supplier_credit.php (check_item_data_credit)
function check_item_data_invoice(string|int|null $n): bool
{

	if (!check_num('this_quantity_inv'.(string) $n, 0) || input_num('this_quantity_inv'.(string) $n)==0)
	{
		display_error( _("The quantity to invoice must be numeric and greater than zero."));
		set_focus('this_quantity_inv'.(string) $n);
		return false;
	}

	if (!check_num('ChgPrice'.(string) $n))
	{
		display_error( _("The price is not numeric."));
		set_focus('ChgPrice'.(string) $n);
		return false;
	}

	$margin_raw = sysprefs()->over_charge_allowance();
	$margin = is_numeric($margin_raw) ? (float) $margin_raw : 0.0;
	$order_price_post = $_POST['order_price'.(string) $n] ?? null;
	$order_price = is_numeric($order_price_post) ? (float) $order_price_post : 0.0;
	if (sysprefs()->check_price_charged_vs_order_price == True)
	{
		$chg_price = input_num('ChgPrice'.(string) $n);
		$chg_price = (float) ($chg_price === false ? 0 : $chg_price);
		if ($order_price!=$chg_price) {
		     if ($order_price==0 ||
				$chg_price/$order_price >
			    (1.0 + ($margin/ 100.0)))
		    {
			display_error(_("The price being invoiced is more than the purchase order price by more than the allowed over-charge percentage. The system is set up to prohibit this. See the system administrator to modify the set up parameters if necessary.") .
			_("The over-charge percentage allowance is :") . (string) $margin . "%");
			set_focus('ChgPrice'.(string) $n);
			return false;
		    }
		}
	}

	$qty_recd_post = $_POST['qty_recd'.(string) $n] ?? null;
	$prev_quantity_inv_post = $_POST['prev_quantity_inv'.(string) $n] ?? null;
	if (sysprefs()->check_qty_charged_vs_del_qty == true && ($qty_recd_post != $prev_quantity_inv_post)
		&& (bool) $prev_quantity_inv_post)
	{
		$qty_recd = is_numeric($qty_recd_post) ? (float) $qty_recd_post : 0.0;
		$prev_quantity_inv = is_numeric($prev_quantity_inv_post) ? (float) $prev_quantity_inv_post : 0.0;
		if ((float)input_num('this_quantity_inv'.(string) $n) / ($qty_recd - $prev_quantity_inv) >
			(1.0 + ($margin / 100.0)))
		{
			display_error( _("The quantity being invoiced is more than the outstanding quantity by more than the allowed over-charge percentage. The system is set up to prohibit this. See the system administrator to modify the set up parameters if necessary.")
			. _("The over-charge percentage allowance is :") . (string) $margin . "%");
			set_focus('this_quantity_inv'.(string) $n);
			return false;
		}
	}

	return true;
}

function commit_item_data(string|int|null $n): void
{
	if (check_item_data_invoice($n))
	{
		$po_detail_item = $_POST['po_detail_item'.(string) $n] ?? null;
		$item_code = $_POST['item_code'.(string) $n] ?? null;
		$item_description = $_POST['item_description'.(string) $n] ?? null;
		$qty_recd = $_POST['qty_recd'.(string) $n] ?? null;
		$prev_quantity_inv = $_POST['prev_quantity_inv'.(string) $n] ?? null;
		$order_price = $_POST['order_price'.(string) $n] ?? null;
		session_obj('supp_trans')->add_grn_to_trans((string) $n, is_scalar($po_detail_item) ? $po_detail_item : null,
			is_scalar($item_code) ? $item_code : null, is_scalar($item_description) ? $item_description : null,
			is_scalar($qty_recd) ? $qty_recd : null,
			is_scalar($prev_quantity_inv) ? $prev_quantity_inv : null, input_num('this_quantity_inv'.(string) $n),
			is_scalar($order_price) ? $order_price : null, input_num('ChgPrice'.(string) $n));
		reset_tax_input();
	}
}

//-----------------------------------------------------------------------------------------

$id = (int) find_submit('grn_item_id');
if ($id != -1)
{
	commit_item_data($id);
}

if (isset($_POST['InvGRNAll']))
{
   	foreach($_POST as $postkey=>$postval )
    {
		if (strpos($postkey, "qty_recd") === 0)
		{
			$id = (int) substr($postkey, strlen("qty_recd"));
			commit_item_data($id);
		}
    }
}	

//--------------------------------------------------------------------------------------------------
$id3 = (int) find_submit('Delete');
if ($id3 != -1)
{
	session_obj('supp_trans')->remove_grn_from_trans($id3);
	ajax()->activate('grn_items');
	reset_tax_input();
}

$id4 = (int) find_submit('Delete2');
if ($id4 != -1)
{
	session_obj('supp_trans')->remove_gl_codes_from_trans($id4);
	clear_fields();
	reset_tax_input();
	ajax()->activate('gl_items');
}

$id5 = (int) find_submit('Edit');
if ($id5 != -1)
{
    $_POST['gl_code'] = session_obj('supp_trans')->gl_codes[$id5]->gl_code;
    $_POST['dimension_id'] = session_obj('supp_trans')->gl_codes[$id5]->gl_dim;
    $_POST['dimension2_id'] = session_obj('supp_trans')->gl_codes[$id5]->gl_dim2;
    $_POST['amount'] = session_obj('supp_trans')->gl_codes[$id5]->amount;
    $_POST['memo_'] = session_obj('supp_trans')->gl_codes[$id5]->memo_;

       session_obj('supp_trans')->remove_gl_codes_from_trans($id5);
       reset_tax_input();
       ajax()->activate('gl_items');
}

$id2 = -1;
if (session_obj('wa_current_user')->can_access('SA_GRNDELETE'))
{
	$id2 = (int) find_submit('void_item_id');
	if ($id2 != -1) 
	{
		remove_not_invoice_item($id2);
		display_notification(sprintf(_('All yet non-invoiced items on delivery line # %d has been removed.'), $id2));

	}
}

if (isset($_POST['go']))
{
	ajax()->activate('gl_items');
	$trans_for_quick_entry = session_obj('supp_trans');
	display_quick_entries($trans_for_quick_entry, post_scalar('qid'), input_num('totamount'), QE_SUPPINV);
	$_POST['totamount'] = price_format(0); ajax()->activate('totamount');
	reset_tax_input();
}

start_form();

$trans = session_obj('supp_trans');
invoice_header($trans);

if ($_POST['supplier_id']=='')
		display_error(_("There is no supplier selected."));
else {
	display_grn_items($trans, 1);

	display_gl_items($trans, 1);

	div_start('inv_tot');
	invoice_totals($trans);
	div_end();

}

//-----------------------------------------------------------------------------------------

if ($id != -1 || $id2 != -1)
{
	ajax()->activate('grn_items');
	ajax()->activate('inv_tot');
}

if (get_post('AddGLCodeToTrans') || get_post('update'))
	ajax()->activate('inv_tot');

br();
submit_center('PostInvoice', _("Enter Invoice"), true, '', 'default');
br();

end_form();

//--------------------------------------------------------------------------------------------------

end_page();
