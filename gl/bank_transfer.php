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
$page_security = 'SA_BANKTRANSFER';
/** @var string $path_to_root */
$path_to_root = "..";

include_once(dirname(__DIR__) . "/includes/session.inc");

include_once(dirname(__DIR__) . "/includes/date_functions.inc");
include_once(dirname(__DIR__) . "/includes/data_checks.inc");

include_once(dirname(__DIR__) . "/gl/includes/gl_db.inc");
include_once(dirname(__DIR__) . "/gl/includes/gl_ui.inc");

$js = "";
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(800, 500);
if ((bool) user_use_date_picker())
	$js .= get_js_date_picker();

if (isset($_GET['ModifyTransfer'])) {
	$_SESSION['page_title'] = _($help_context = "Modify Bank Account Transfer");
} else {
	$_SESSION['page_title'] = _($help_context = "Bank Account Transfer Entry");
}

page($_SESSION['page_title'], false, false, "", $js);

check_db_has_bank_accounts(_("There are no bank accounts defined in the system."));

//----------------------------------------------------------------------------------------

if (isset($_GET['AddedID']))
{
	$trans_no = get_scalar('AddedID');
	$trans_type = ST_BANKTRANSFER;

   	display_notification_centered( _("Transfer has been entered"));

	display_note(get_gl_view_str($trans_type, $trans_no, _("&View the GL Journal Entries for this Transfer")));

   	hyperlink_no_params($_SERVER['PHP_SELF'] ?? '', _("Enter &Another Transfer"));

	display_footer_exit();
}

if (isset($_POST['_DatePaid_changed'])) {
	ajax()->activate('_ex_rate');
}

//----------------------------------------------------------------------------------------

function gl_payment_controls(string|int|float|bool|null $trans_no): void
{
	
	if (!in_ajax()) {
		if ((bool)$trans_no) {
			$result = get_bank_trans(ST_BANKTRANSFER, $trans_no);

			if (db_num_rows($result) != 2)
				display_db_error("Bank transfer does not contain two records");

			$trans1 = row_or_empty(db_fetch($result));
			$trans2 = row_or_empty(db_fetch($result));

			if ((float)($trans1["amount"] ?? 0) < 0) {
				$from_trans = $trans1; // from trans is the negative one
				$to_trans = $trans2;
			} else {
			$from_trans = $trans2;
				$to_trans = $trans1;
			}
			$_POST['DatePaid'] = sql2date((string) $to_trans['trans_date']);
			$_POST['ref'] = $to_trans['ref'];
			$_POST['memo_'] = get_comments_string($to_trans['type'], $trans_no);
			$_POST['FromBankAccount'] = $from_trans['bank_act'];
			$_POST['ToBankAccount'] = $to_trans['bank_act'];
			$_POST['target_amount'] = price_format($to_trans['amount']);
			$_POST['amount'] = price_format(-(float)($from_trans['amount'] ?? 0));
			$_POST['dimension_id'] = $to_trans['dimension_id'];
			$_POST['dimension2_id'] = $to_trans['dimension2_id'];
		} else {
			$_POST['ref'] = refs()->get_next(ST_BANKTRANSFER, null, get_post('DatePaid'));
			$_POST['memo_'] = '';
			$_POST['FromBankAccount'] = 0;
			$_POST['ToBankAccount'] = 0;
			$_POST['amount'] = 0;
			$_POST['dimension_id'] = 0;
			$_POST['dimension2_id'] = 0;
		}
	}

	start_form();

	start_outer_table(TABLESTYLE2);

	table_section(1);

	bank_accounts_list_row(_("From Account:"), 'FromBankAccount', null, true);

	bank_balance_row(post_scalar('FromBankAccount'));

    bank_accounts_list_row(_("To Account:"), 'ToBankAccount', null, true);

	if (!isset($_POST['DatePaid'])) { // init page
		$_POST['DatePaid'] = new_doc_date();
		if (!(bool)is_date_in_fiscalyear(post_scalar('DatePaid')))
			$_POST['DatePaid'] = end_fiscalyear();
	}
    date_row(_("Transfer Date:"), 'DatePaid', '', true, 0, 0, 0, null, true);

    ref_row(_("Reference:"), 'ref', '', refs()->get_next(ST_BANKTRANSFER, null, get_post('DatePaid')), false, ST_BANKTRANSFER,
    	array('date' => get_post('DatePaid')));
	$dim = get_company_pref('use_dimension');
	if ($dim > 0)
		dimensions_list_row(_("Dimension").":", 'dimension_id', 
			null, true, ' ', false, 1, false);
	else
		hidden('dimension_id', 0);

	table_section(2);

	$from_currency = get_bank_account_currency(post_scalar('FromBankAccount'));
	$to_currency = get_bank_account_currency(post_scalar('ToBankAccount'));
	if ($from_currency != "" && $to_currency != "" && $from_currency != $to_currency) 
	{
		amount_row(_("Amount:"), 'amount', null, null, $from_currency);
		amount_row(_("Bank Charge:"), 'charge', null, null, $from_currency);

		amount_row(_("Incoming Amount:"), 'target_amount', null, '', $to_currency, 2);
	} 
	else 
	{
		amount_row(_("Amount:"), 'amount');
		amount_row(_("Bank Charge:"), 'charge');
	}
	if ($dim > 1)
		dimensions_list_row(_("Dimension")." 2:", 'dimension2_id', 
			null, true, ' ', false, 2, false);
	else
		hidden('dimension2_id', 0);

    textarea_row(_("Memo:"), 'memo_', null, 40,4);

	end_outer_table(1); // outer table

	if ((bool)$trans_no) {
		hidden('_trans_no', $trans_no);
		submit_center('submit', _("Modify Transfer"), true, '', 'default');
	} else {
		submit_center('submit', _("Enter Transfer"), true, '', 'default');
	}

	end_form();
}

//----------------------------------------------------------------------------------------

function check_valid_entries_bt(string|int|float|bool|null $trans_no): bool
{
	global $systypes_array;

	$date_paid_scalar = post_scalar('DatePaid');
	$date_paid_str = $date_paid_scalar === null ? null : (string) $date_paid_scalar;
	if (!is_date($date_paid_str))
	{
		display_error(_("The entered date is invalid."));
		set_focus('DatePaid');
		return false;
	}
	if (!(bool)is_date_in_fiscalyear($date_paid_str))
	{
		display_error(_("The entered date is out of fiscal year or is closed for further data entry."));
		set_focus('DatePaid');
		return false;
	}

	if (!check_num('amount', 0)) 
	{
		display_error(_("The entered amount is invalid or less than zero."));
		set_focus('amount');
		return false;
	}
	if (input_num('amount') == 0) {
		display_error(_("The total bank amount cannot be 0."));
		set_focus('amount');
		return false;
	}

	$from_account_scalar = post_scalar('FromBankAccount');
	$from_account_str = $from_account_scalar === null ? null : (string) $from_account_scalar;

	$limit = get_bank_account_limit($from_account_str, $date_paid_str);

	$amnt_tr = (float)input_num('charge') + (float)input_num('amount');

	$to_account_post = $_POST['ToBankAccount'] ?? null;
	$to_account_str = is_array($to_account_post) ? '' : (string) $to_account_post;
	$to_account_post = is_array($to_account_post) ? $to_account_post : $to_account_str;

	$problemTransaction = null;
	if ((bool)$trans_no) {
		$problemTransaction = check_bank_transfer( $trans_no, $from_account_str, $to_account_post, $date_paid_str,
			$amnt_tr, input_num('target_amount', $amnt_tr));

	if ($problemTransaction != null	) {
		if (!array_key_exists('trans_no', $problemTransaction)) {
			display_error(sprintf(
				_("This bank transfer change would result in exceeding authorized overdraft limit (%s) of the account '%s'"),
				price_format(-(float)($problemTransaction['amount'] ?? 0)), $problemTransaction['bank_account_name'] ?? ''
			));
		} else {
			display_error(sprintf(
				_("This bank transfer change would result in exceeding authorized overdraft limit on '%s' for transaction: %s #%s on %s."),
				$problemTransaction['bank_account_name'] ?? '', $systypes_array[(int) ($problemTransaction['type'] ?? 0)],
				$problemTransaction['trans_no'] ?? '', sql2date($problemTransaction['trans_date'] ?? null)
			));
		}
		set_focus('amount');
		return false;
		}
	} else {
		if (null != ($problemTransaction = check_bank_account_history(-$amnt_tr, $from_account_str, $date_paid_str))) {
			if (!array_key_exists('trans_no', $problemTransaction)) {
				display_error(sprintf(
					_("This bank transfer would result in exceeding authorized overdraft limit of the account (%s)"),
					price_format(-(float)($problemTransaction['amount'] ?? 0))
				));
			} else {
				display_error(sprintf(
					_("This bank transfer would result in exceeding authorized overdraft limit for transaction: %s #%s on %s."),
					$systypes_array[(int) ($problemTransaction['type'] ?? 0)], $problemTransaction['trans_no'] ?? '', sql2date($problemTransaction['trans_date'] ?? null)
				));
			}
			set_focus('amount');
			return false;
		}
	}

	if (isset($_POST['charge']) && !check_num('charge', 0)) 
	{
		display_error(_("The entered amount is invalid or less than zero."));
		set_focus('charge');
		return false;
	}
	if (isset($_POST['charge']) && input_num('charge') > 0 && get_bank_charge_account($from_account_str) == '') {
		display_error(_("The Bank Charge Account has not been set in System and General GL Setup."));
		set_focus('charge');
		return false;
	}

	if (!check_reference(post_scalar('ref'), ST_BANKTRANSFER, $trans_no)) {
		set_focus('ref');
		return false;
	}

	if ($from_account_str == $to_account_str)
	{
		display_error(_("The source and destination bank accouts cannot be the same."));
		set_focus('ToBankAccount');
		return false;
	}

	if (isset($_POST['target_amount']) && !check_num('target_amount', 0)) 
	{
		display_error(_("The entered amount is invalid or less than zero."));
		set_focus('target_amount');
		return false;
	}
	if (isset($_POST['target_amount']) && input_num('target_amount') == 0) {
		display_error(_("The incomming bank amount cannot be 0."));
		set_focus('target_amount');
		return false;
	}

	if (!db_has_currency_rates(get_bank_account_currency($from_account_str), $date_paid_str))
		return false;

	if (!db_has_currency_rates(get_bank_account_currency($to_account_str), $date_paid_str))
		return false;

    return true;
}

//----------------------------------------------------------------------------------------

function bank_transfer_handle_submit(): void
{
	$from_account = post_scalar('FromBankAccount');
	$from_account = $from_account === null ? null : (string) $from_account;
	$to_account = post_scalar('ToBankAccount');
	$to_account = $to_account === null ? null : (string) $to_account;
	$date_paid = post_scalar('DatePaid');
	$date_paid = $date_paid === null ? null : (string) $date_paid;
	$ref = post_scalar('ref');
	$ref = $ref === null ? null : (string) $ref;
	$memo = post_scalar('memo_');
	$memo = $memo === null ? null : (string) $memo;
	$dim1 = post_scalar('dimension_id');
	$dim1 = $dim1 === null ? null : (string) $dim1;
	$dim2 = post_scalar('dimension2_id');
	$dim2 = $dim2 === null ? null : (string) $dim2;

	$trans_no = post_scalar('_trans_no');
	if ((bool)$trans_no) {
		$trans_no = (string) $trans_no;
		$trans_no = update_bank_transfer($trans_no, $from_account, $to_account, $date_paid, input_num('amount'),
			$ref, $memo, $dim1, $dim2, input_num('charge'), input_num('target_amount'));
	} else {
		new_doc_date(post_scalar('DatePaid'));
		$trans_no = add_bank_transfer($from_account, $to_account, $date_paid, input_num('amount'), $ref,
			$memo, $dim1, $dim2, input_num('charge'), input_num('target_amount'));
	}

	meta_forward($_SERVER['PHP_SELF'] ?? '', "AddedID=$trans_no");
}

//----------------------------------------------------------------------------------------

$trans_no = post_scalar('_trans_no');
if (!(bool)$trans_no) {
	$trans_no = get_scalar('trans_no');
}

if (isset($_POST['submit'])) {
    if (check_valid_entries_bt($trans_no) == true) {
        bank_transfer_handle_submit();
	}
}

gl_payment_controls($trans_no);

end_page();
