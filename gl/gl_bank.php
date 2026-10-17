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
$path_to_root = "..";
include_once(dirname(__DIR__) . "/includes/ui/items_cart.inc");
include_once(dirname(__DIR__) . "/includes/session.inc");
$page_security = isset($_GET['NewPayment']) || 
	@(session_obj('pay_items')->trans_type==ST_BANKPAYMENT)
 ? 'SA_PAYMENT' : 'SA_DEPOSIT';

include_once(dirname(__DIR__) . "/includes/date_functions.inc");
include_once(dirname(__DIR__) . "/includes/data_checks.inc");

include_once(dirname(__DIR__) . "/gl/includes/ui/gl_bank_ui.inc");
include_once(dirname(__DIR__) . "/gl/includes/gl_db.inc");
include_once(dirname(__DIR__) . "/gl/includes/gl_ui.inc");
include_once(dirname(__DIR__) . "/admin/db/attachments_db.inc");

$js = '';
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(800, 500);
if ((bool) user_use_date_picker())
	$js .= get_js_date_picker();

if (isset($_GET['NewPayment'])) {
	$_SESSION['page_title'] = _($help_context = "Bank Account Payment Entry");
	create_cart_bank(ST_BANKPAYMENT, 0);
} else if(isset($_GET['NewDeposit'])) {
	$_SESSION['page_title'] = _($help_context = "Bank Account Deposit Entry");
	create_cart_bank(ST_BANKDEPOSIT, 0);
} else if(isset($_GET['ModifyPayment'])) {
	$trans_no_scalar = get_scalar('trans_no');
	$trans_no_for_cart = is_int($trans_no_scalar) ? $trans_no_scalar : ($trans_no_scalar === null ? null : (string) $trans_no_scalar);
	$_SESSION['page_title'] = _($help_context = "Modify Bank Account Entry")." #".(string) $trans_no_for_cart;
	create_cart_bank(ST_BANKPAYMENT, $trans_no_for_cart);
} else if(isset($_GET['ModifyDeposit'])) {
	$trans_no_scalar = get_scalar('trans_no');
	$trans_no_for_cart = is_int($trans_no_scalar) ? $trans_no_scalar : ($trans_no_scalar === null ? null : (string) $trans_no_scalar);
	$_SESSION['page_title'] = _($help_context = "Modify Bank Deposit Entry")." #".(string) $trans_no_for_cart;
	create_cart_bank(ST_BANKDEPOSIT, $trans_no_for_cart);
}
page((string) ($_SESSION['page_title'] ?? _("Bank Account Entry")), false, false, '', $js);

//-----------------------------------------------------------------------------------------------
check_db_has_bank_accounts(_("There are no bank accounts defined in the system."));

if (isset($_GET['ModifyDeposit']) || isset($_GET['ModifyPayment']))
	check_is_editable(session_obj('pay_items')->trans_type, session_obj('pay_items')->order_id);

if (!isset($_SESSION['pay_items']))
{
	// reached without any of the New*/Modify* requests above, and nothing already in the
	// session from an earlier request on this page (the normal form post-back relies on that
	// instead) -- there is nothing to enter or edit
	display_error(_("This page can only be opened to enter a new bank payment or deposit, or to modify an existing one."));
	end_page(); exit;
}
/** @var items_cart $pay_items_cart */
$pay_items_cart = $_SESSION['pay_items'];

//----------------------------------------------------------------------------------------
if (list_updated('PersonDetailID')) {
	$person_detail_id = get_post('PersonDetailID');
	$br = row_or_empty(get_branch(is_array($person_detail_id) ? null : $person_detail_id));
	$_POST['person_id'] = $br['debtor_no'];
	ajax()->activate('person_id');
}

//--------------------------------------------------------------------------------------------------
function line_start_focus_bank(): void {

    unset($_POST['amount']);
    unset($_POST['dimension_id']);
    unset($_POST['dimension2_id']);
    unset($_POST['LineMemo']);
  	ajax()->activate('items_table');
  	ajax()->activate('footer');
  	set_focus('_code_id_edit');
}

//-----------------------------------------------------------------------------------------------

if (isset($_GET['AddedID']))
{
	$trans_no = is_array($_GET['AddedID']) ? '' : $_GET['AddedID'];
	$trans_type = ST_BANKPAYMENT;

   	display_notification_centered(sprintf(_("Payment %d has been entered"), $trans_no));

	display_note(get_gl_view_str($trans_type, $trans_no, _("&View the GL Postings for this Payment")));

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter Another &Payment"), "NewPayment=yes");

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter A &Deposit"), "NewDeposit=yes");

	hyperlink_params("$path_to_root/admin/attachments.php", _("Add an Attachment"), "filterType=$trans_type&trans_no=$trans_no");

	display_footer_exit();
}

if (isset($_GET['UpdatedID']))
{
	$trans_no = is_array($_GET['UpdatedID']) ? '' : $_GET['UpdatedID'];
	$trans_type = ST_BANKPAYMENT;

   	display_notification_centered(sprintf(_("Payment %d has been modified"), $trans_no));

	display_note(get_gl_view_str($trans_type, $trans_no, _("&View the GL Postings for this Payment")));

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter Another &Payment"), "NewPayment=yes");

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter A &Deposit"), "NewDeposit=yes");

	display_footer_exit();
}

if (isset($_GET['AddedDep']))
{
	$trans_no = is_array($_GET['AddedDep']) ? '' : $_GET['AddedDep'];
	$trans_type = ST_BANKDEPOSIT;

   	display_notification_centered(sprintf(_("Deposit %d has been entered"), $trans_no));

	display_note(get_gl_view_str($trans_type, $trans_no, _("View the GL Postings for this Deposit")));

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter Another Deposit"), "NewDeposit=yes");

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter A Payment"), "NewPayment=yes");

	display_footer_exit();
}
if (isset($_GET['UpdatedDep']))
{
	$trans_no = is_array($_GET['UpdatedDep']) ? '' : $_GET['UpdatedDep'];
	$trans_type = ST_BANKDEPOSIT;

   	display_notification_centered(sprintf(_("Deposit %d has been modified"), $trans_no));

	display_note(get_gl_view_str($trans_type, $trans_no, _("&View the GL Postings for this Deposit")));

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter Another &Deposit"), "NewDeposit=yes");

	hyperlink_params($_SERVER['PHP_SELF'] ?? '', _("Enter A &Payment"), "NewPayment=yes");

	display_footer_exit();
}

//--------------------------------------------------------------------------------------------------

function create_cart_bank(string|int|null $type, string|int|null $trans_no): void
{

	if (isset($_SESSION['pay_items']))
	{
		unset ($_SESSION['pay_items']);
	}

	$cart = new items_cart($type);
    $cart->order_id = $trans_no;

	if ((bool)$trans_no) {

		$bank_trans = row_or_empty(db_fetch(get_bank_trans($type, $trans_no)));
		$_POST['bank_account'] = $bank_trans["bank_act"];
		$_POST['PayType'] = $bank_trans["person_type_id"];
		$cart->reference = $bank_trans["ref"] ?? '';

		if ($bank_trans["person_type_id"] == PT_CUSTOMER)
		{
			$trans = row_or_empty(get_customer_trans($trans_no, $type));	
			$_POST['person_id'] = $trans["debtor_no"];
			$_POST['PersonDetailID'] = $trans["branch_code"];
		}
		elseif ($bank_trans["person_type_id"] == PT_SUPPLIER)
		{
			$trans = row_or_empty(get_supp_trans($trans_no, $type));
			$_POST['person_id'] = $trans["supplier_id"];
		}
		elseif ($bank_trans["person_type_id"] == PT_MISC)
			$_POST['person_id'] = $bank_trans["person_id"];
		elseif ($bank_trans["person_type_id"] == PT_QUICKENTRY)
			$_POST['person_id'] = $bank_trans["person_id"];
		else 
			$_POST['person_id'] = $bank_trans["person_id"];

		$cart->memo_ = get_comments_string($type, $trans_no);
		$cart->tran_date = sql2date($bank_trans['trans_date']);

		$cart->original_amount = $bank_trans['amount'] ?? 0;
		$result = get_gl_trans($type, $trans_no);
		$ex_rate = 1.0;
		while ($row = db_fetch($result)) {
			if ((bool)is_bank_account($row['account'])) {
				// date exchange rate is currenly not stored in bank transaction,
				// so we have to restore it from original gl amounts
				$ex_rate = (float)$bank_trans['amount']/(float)$row['amount'];
			} else {
				$cart->add_gl_item( $row['account'], $row['dimension_id'],
					$row['dimension2_id'], $row['amount'], $row['memo_']);
			}
		}

		// apply exchange rate
		foreach($cart->gl_items as $line_no => $line)
			$cart->gl_items[$line_no]->amount = (float) $cart->gl_items[$line_no]->amount * $ex_rate;

	} else {
		$cart->reference = refs()->get_next($cart->trans_type, null, $cart->tran_date);
		$cart->tran_date = new_doc_date();
		if (!(bool)is_date_in_fiscalyear($cart->tran_date))
			$cart->tran_date = end_fiscalyear();
	}

	$_POST['memo_'] = $cart->memo_;
	$_POST['ref'] = $cart->reference;
	$_POST['date_'] = $cart->tran_date;

	$_SESSION['pay_items'] = &$cart;
}
//-----------------------------------------------------------------------------------------------

function check_trans(): int
{
	global $systypes_array;

	$input_error = 0;

	if (session_obj('pay_items')->count_gl_items() < 1) {
		display_error(_("You must enter at least one payment line."));
		set_focus('code_id');
		$input_error = 1;
	}

	if (session_obj('pay_items')->gl_items_total() == 0.0) {
		display_error(_("The total bank amount cannot be 0."));
		set_focus('code_id');
		$input_error = 1;
	}

	$bank_account_scalar = post_scalar('bank_account');
	$bank_account_str = $bank_account_scalar === null ? null : (string) $bank_account_scalar;
	$date_scalar = post_scalar('date_');
	$date_str = $date_scalar === null ? null : (string) $date_scalar;

	$limit = get_bank_account_limit($bank_account_str, $date_str);

	$amnt_chg = -session_obj('pay_items')->gl_items_total()-(float)session_obj('pay_items')->original_amount;

	if ($limit !== null && floatcmp($limit, -$amnt_chg) < 0)
	{
		display_error(sprintf(_("The total bank amount exceeds allowed limit (%s)."), price_format((float)$limit-(float)session_obj('pay_items')->original_amount)));
		set_focus('code_id');
		$input_error = 1;
	}
	if ($trans = check_bank_account_history($amnt_chg, $bank_account_str, $date_str)) {

		if (isset($trans['trans_no'])) {
			display_error(sprintf(_("The bank transaction would result in exceed of authorized overdraft limit for transaction: %s #%s on %s."),
				$systypes_array[(int) $trans['type']], $trans['trans_no'], sql2date($trans['trans_date'] ?? null)));
			set_focus('amount');
			$input_error = 1;
		}
	}
	if (!check_reference($_POST['ref'], session_obj('pay_items')->trans_type, session_obj('pay_items')->order_id))
	{
		set_focus('ref');
		$input_error = 1;
	}
	if (!is_date($date_str))
	{
		display_error(_("The entered date for the payment is invalid."));
		set_focus('date_');
		$input_error = 1;
	}
	elseif (!(bool)is_date_in_fiscalyear($date_str))
	{
		display_error(_("The entered date is out of fiscal year or is closed for further data entry."));
		set_focus('date_');
		$input_error = 1;
	}

	if (get_post('PayType')==PT_CUSTOMER && (!get_post('person_id') || !get_post('PersonDetailID'))) {
		display_error(_("You have to select customer and customer branch."));
		set_focus('person_id');
		$input_error = 1;
	} elseif (get_post('PayType')==PT_SUPPLIER && (!get_post('person_id'))) {
		display_error(_("You have to select supplier."));
		set_focus('person_id');
		$input_error = 1;
	}
	if (!db_has_currency_rates(get_bank_account_currency($bank_account_str), $date_str, true))
		$input_error = 1;

	if (isset($_POST['settled_amount']) && in_array(get_post('PayType'), array(PT_SUPPLIER, PT_CUSTOMER)) && (input_num('settled_amount') <= 0)) {
		display_error(_("Settled amount have to be positive number."));
		set_focus('person_id');
		$input_error = 1;
	}
	return $input_error;
}

if (isset($_POST['Process']) && !check_trans())
{
	begin_transaction();

	$pay_items = $pay_items_cart;
	$new = $pay_items->order_id == 0;

	$bank_account_scalar = post_scalar('bank_account');
	$bank_account_str = $bank_account_scalar === null ? null : (string) $bank_account_scalar;
	$date_scalar = post_scalar('date_');
	$date_str = $date_scalar === null ? null : (string) $date_scalar;
	$person_type_scalar = post_scalar('PayType');
	$person_type_str = $person_type_scalar === null ? null : (string) $person_type_scalar;
	$person_id_scalar = post_scalar('person_id');
	$person_id_str = $person_id_scalar === null ? null : (string) $person_id_scalar;
	$ref_scalar = post_scalar('ref');
	$ref_str = $ref_scalar === null ? null : (string) $ref_scalar;
	$memo_scalar = post_scalar('memo_');
	$memo_str = $memo_scalar === null ? null : (string) $memo_scalar;

	add_new_exchange_rate(get_bank_account_currency($bank_account_str), get_post('date_'), input_num('_ex_rate'));

	/** @var array{0?: string|int|float|bool|null, 1?: string|int|float|bool|null} $trans */
	$trans = row_or_empty(write_bank_transaction(
		$pay_items->trans_type, $pay_items->order_id, $bank_account_str,
		$pay_items, $date_str,
		$person_type_str, $person_id_str, (string) post_scalar('PersonDetailID'),
		$ref_str, $memo_str, true, input_num('settled_amount', null)));

	$trans_type = $trans[0] ?? null;
   	$trans_no = $trans[1] ?? null;
	new_doc_date($_POST['date_']);

	session_obj('pay_items')->clear_items();
	unset($_SESSION['pay_items']);

	commit_transaction();

	if ($new)
		meta_forward($_SERVER['PHP_SELF'] ?? '', $trans_type==ST_BANKPAYMENT ?
			"AddedID=$trans_no" : "AddedDep=$trans_no");
	else
		meta_forward($_SERVER['PHP_SELF'] ?? '', $trans_type==ST_BANKPAYMENT ?
			"UpdatedID=$trans_no" : "UpdatedDep=$trans_no");

}

//-----------------------------------------------------------------------------------------------

function check_item_data_bank(): bool
{
	if (!check_num('amount', 0))
	{
		display_error( _("The amount entered is not a valid number or is less than zero."));
		set_focus('amount');
		return false;
	}
	if (isset($_POST['_ex_rate']) && input_num('_ex_rate') <= 0)
	{
		display_error( _("The exchange rate cannot be zero or a negative number."));
		set_focus('_ex_rate');
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------------

function handle_update_item_bank(): void
{
	$amount = ((float)(session_obj('pay_items')->trans_type==ST_BANKPAYMENT ? 1:-1)) * (float)input_num('amount');
    if($_POST['UpdateItem'] != "" && check_item_data_bank())
    {
    	session_obj('pay_items')->update_gl_item((string) post_scalar('Index'), post_scalar('code_id'),
    	    post_scalar('dimension_id'), post_scalar('dimension2_id'), $amount , post_scalar('LineMemo'));
    }
	line_start_focus_bank();
}

//-----------------------------------------------------------------------------------------------

function handle_delete_item_bank(string|int|null $id): void
{
	session_obj('pay_items')->remove_gl_item($id);
	line_start_focus_bank();
}

//-----------------------------------------------------------------------------------------------

function handle_new_item_bank(): void
{
	if (!check_item_data_bank())
		return;
	$amount = ((float)(session_obj('pay_items')->trans_type==ST_BANKPAYMENT ? 1:-1)) * (float)input_num('amount');

	session_obj('pay_items')->add_gl_item(post_scalar('code_id'), post_scalar('dimension_id'),
		post_scalar('dimension2_id'), $amount, post_scalar('LineMemo'));
	line_start_focus_bank();
}
//-----------------------------------------------------------------------------------------------
$id = find_submit('Delete');
if ($id != -1)
	handle_delete_item_bank($id);

if (isset($_POST['AddItem']))
	handle_new_item_bank();

if (isset($_POST['UpdateItem']))
	handle_update_item_bank();

if (isset($_POST['CancelItemChanges']) || isset($_POST['Index']))
	line_start_focus_bank();

if (isset($_POST['go']))
{
	display_quick_entries($pay_items_cart, post_scalar('person_id'), input_num('totamount'),
		session_obj('pay_items')->trans_type==ST_BANKPAYMENT ? QE_PAYMENT : QE_DEPOSIT);
	$_POST['totamount'] = price_format(0); ajax()->activate('totamount');
	line_start_focus_bank();
}
//-----------------------------------------------------------------------------------------------

start_form();

/** @var items_cart $pay_items_cart */
$pay_items_cart = $pay_items_cart;
display_bank_header($pay_items_cart);

start_table(TABLESTYLE2, "width='90%'", 10);
start_row();
echo "<td>";
display_gl_bank_items(session_obj('pay_items')->trans_type==ST_BANKPAYMENT ?
	_("Payment Items"):_("Deposit Items"), $pay_items);
gl_options_controls_bank($pay_items);
echo "</td>";
end_row();
end_table(1);

submit_center_first('Update', _("Update"), '', null);
submit_center_last('Process', session_obj('pay_items')->trans_type==ST_BANKPAYMENT ?
	_("Process Payment"):_("Process Deposit"), '', 'default');

end_form();

//------------------------------------------------------------------------------------------------

end_page();

