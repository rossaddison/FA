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
//---------------------------------------------------------------------------
//
//	Entry/Modify Sales Invoice against single delivery
//	Entry/Modify Batch Sales Invoice against batch of deliveries
//
$page_security = 'SA_SALESINVOICE';
/** @var string $path_to_root */
$path_to_root = "..";
include_once(dirname(__DIR__) . "/sales/includes/cart_class.inc");
include_once(dirname(__DIR__) . "/includes/session.inc");
include_once(dirname(__DIR__) . "/includes/data_checks.inc");
include_once(dirname(__DIR__) . "/sales/includes/sales_db.inc");
include_once(dirname(__DIR__) . "/sales/includes/sales_ui.inc");
include_once(dirname(__DIR__) . "/reporting/includes/reporting.inc");
include_once(dirname(__DIR__) . "/taxes/tax_calc.inc");
include_once(dirname(__DIR__) . "/admin/db/shipping_db.inc");

$js = "";
if (sysprefs()->use_popup_windows) {
	$js .= get_js_open_window(900, 500);
}
if ((bool) user_use_date_picker()) {
	$js .= get_js_date_picker();
}

if (isset($_GET['ModifyInvoice'])) {
	$_SESSION['page_title'] = sprintf(_("Modifying Sales Invoice # %d.") ,(string) get_scalar('ModifyInvoice'));
	$help_context = "Modifying Sales Invoice";
} elseif (isset($_GET['DeliveryNumber'])) {
	$_SESSION['page_title'] = _($help_context = "Issue an Invoice for Delivery Note");
} elseif (isset($_GET['BatchInvoice'])) {
	$_SESSION['page_title'] = _($help_context = "Issue Batch Invoice for Delivery Notes");
} elseif (isset($_GET['AllocationNumber']) || isset($_GET['InvoicePrepayments'])) {
	$_SESSION['page_title'] = _($help_context = "Prepayment or Final Invoice Entry");
}
/** @var string $page_title */
$page_title = $_SESSION['page_title'] ?? '';
page($page_title, false, false, "", $js);

//-----------------------------------------------------------------------------

check_edit_conflicts(get_post('cart_id'));

if (isset($_GET['AddedID'])) {

	$invoice_no = (string) get_scalar('AddedID');
	$trans_type = ST_SALESINVOICE;

	display_notification(_("Selected deliveries has been processed"), true);

	display_note(get_customer_trans_view_str($trans_type, $invoice_no, _("&View This Invoice")), 0, 1);

	display_note(print_document_link($invoice_no."-".$trans_type, _("&Print This Invoice"), true, ST_SALESINVOICE));
	display_note(print_document_link($invoice_no."-".$trans_type, _("&Email This Invoice"), true, ST_SALESINVOICE, false, "printlink", "", 1),1);

	display_note(get_gl_view_str($trans_type, $invoice_no, _("View the GL &Journal Entries for this Invoice")),1);

	hyperlink_params("$path_to_root/sales/inquiry/sales_deliveries_view.php", _("Select Another &Delivery For Invoicing"), "OutstandingOnly=1");

	$allocatable = get_allocatable_from_cust_transactions(null, $invoice_no, $trans_type);
	if (!($allocatable instanceof mysqli_result && db_num_rows($allocatable)))
		hyperlink_params("$path_to_root/sales/customer_payments.php", _("Entry &customer payment for this invoice"),
		"SInvoice=".$invoice_no);

	hyperlink_params("$path_to_root/admin/attachments.php", _("Add an Attachment"), "filterType=$trans_type&trans_no=$invoice_no");

	display_footer_exit();

} elseif (isset($_GET['UpdatedID']))  {

	$invoice_no = (string) get_scalar('UpdatedID');
	$trans_type = ST_SALESINVOICE;

	display_notification_centered(sprintf(_('Sales Invoice # %d has been updated.'),$invoice_no));

	display_note(get_trans_view_str(ST_SALESINVOICE, $invoice_no, _("&View This Invoice")));
	echo '<br>';
	display_note(print_document_link($invoice_no."-".$trans_type, _("&Print This Invoice"), true, ST_SALESINVOICE));
	display_note(print_document_link($invoice_no."-".$trans_type, _("&Email This Invoice"), true, ST_SALESINVOICE, false, "printlink", "", 1),1);

	/** @var string $root */
	$root = $path_to_root;
	hyperlink_no_params($root . "/sales/inquiry/customer_inquiry.php", _("Select Another &Invoice to Modify"));

	display_footer_exit();

} elseif (isset($_GET['RemoveDN'])) {

	$remove_dn = get_scalar('RemoveDN');
	for($line_no = 0; $line_no < count(session_obj('Items')->line_items); $line_no++) {
		$line = &session_obj('Items')->line_items[$line_no];
		if ($line->src_no == $remove_dn) {
			$line->quantity = $line->qty_done;
			$line->qty_dispatched=0;
		}
	}
	unset($line);

    // Remove also src_doc delivery note
    $sources = &session_obj('Items')->src_docs;
    if (is_array($sources) && $remove_dn !== null)
    	unset($sources[(string) $remove_dn]);
}

//-----------------------------------------------------------------------------

if ( (isset($_GET['DeliveryNumber']) && ((float) get_scalar('DeliveryNumber') > 0) )
	|| isset($_GET['BatchInvoice'])) {

	processing_start();

	if (isset($_GET['BatchInvoice'])) {
		/** @var array<array-key, mixed> $src */
		$src = $_SESSION['DeliveryBatch'] ?? array();
		unset($_SESSION['DeliveryBatch']);
	} else {
		$src = array(get_scalar('DeliveryNumber'));
	}

	/*read in all the selected deliveries into the Items cart  */
	$dn = new Cart(ST_CUSTDELIVERY, $src, true);

	if ($dn->count_items() == 0) {
		/** @var string $root */
		$root = $path_to_root;
		hyperlink_params($root . "/sales/inquiry/sales_deliveries_view.php",
			_("Select a different delivery to invoice"), "OutstandingOnly=1");
		die ("<br><b>" . _("There are no delivered items with a quantity left to invoice. There is nothing left to invoice.") . "</b>");
	}

	$_SESSION['Items'] = $dn;
	copy_from_cart();

} elseif (isset($_GET['ModifyInvoice']) && (float) get_scalar('ModifyInvoice') > 0) {

	check_is_editable(ST_SALESINVOICE, get_scalar('ModifyInvoice'));

	processing_start();
	$modify_id = get_scalar('ModifyInvoice');
	$_SESSION['Items'] = new Cart(ST_SALESINVOICE, is_int($modify_id) ? $modify_id : (string) $modify_id);

	if (session_obj('Items')->count_items() == 0) {
		echo"<center><br><b>" . _("All quantities on this invoice has been credited. There is nothing to modify on this invoice") . "</b></center>";
		display_footer_exit();
	}
	copy_from_cart();
} elseif (isset($_GET['AllocationNumber']) || isset($_GET['InvoicePrepayments'])) {

	check_deferred_income_act(_("You have to set Deferred Income Account in GL Setup to entry prepayment invoices."));

	if (isset($_GET['AllocationNumber']))
	{
		$payments = array(get_cust_allocation(get_scalar('AllocationNumber')));

		if (!$payments[0] || ($payments[0]['trans_type_to'] != ST_SALESORDER))
		{
			display_error(_("Please select correct Sales Order Prepayment to be invoiced and try again."));
			display_footer_exit();
		}
		$order_no = $payments[0]['trans_no_to'];
	}
	else {
		$order_no = get_scalar('InvoicePrepayments');
	}
	processing_start();

	$order_no_key = is_int($order_no) ? $order_no : (string) $order_no;
	$_SESSION['Items'] = new Cart(ST_SALESORDER, $order_no_key, ST_SALESINVOICE);
	session_obj('Items')->order_no = $order_no_key;
	session_obj('Items')->src_docs = array($order_no_key);
	session_obj('Items')->trans_no = 0;
	session_obj('Items')->trans_type = ST_SALESINVOICE;

	session_obj('Items')->update_payments();

	copy_from_cart();
}
elseif (!processing_active()) {
	/* This page can only be called with a delivery for invoicing or invoice no for edit */
	display_error(_("This page can only be opened after delivery selection. Please select delivery to invoicing first."));

	hyperlink_no_params("$path_to_root/sales/inquiry/sales_deliveries_view.php", _("Select Delivery to Invoice"));

	end_page();
	exit;
} elseif (!isset($_POST['process_invoice']) && (!session_obj('Items')->is_prepaid() && !check_quantities())) {
	display_error(_("Selected quantity cannot be less than quantity credited nor more than quantity not invoiced yet."));
}

if (isset($_POST['Update'])) {
	ajax()->activate('Items');
}
if (isset($_POST['_InvoiceDate_changed'])) {
	$_POST['due_date'] = get_invoice_duedate(session_obj('Items')->payment, post_scalar('InvoiceDate'));
	ajax()->activate('due_date');
}

//-----------------------------------------------------------------------------
function check_quantities(): int
{
	$ok =1;
	foreach (session_obj('Items')->line_items as $line_no=>$itm) {
		if (isset($_POST['Line'.$line_no])) {
			if((bool)session_obj('Items')->trans_no) {
				$min = (float) $itm->qty_done;
				$max = (float) $itm->quantity;
			} else {
				$min = 0;
				// Fixing floating point problem in PHP.
				$max = round2((float)$itm->quantity - (float)$itm->qty_done, get_qty_dec($itm->stock_id));
			}
			if (check_num('Line'.$line_no, $min, $max)) {
				session_obj('Items')->line_items[$line_no]->qty_dispatched =
				    (float) input_num('Line'.$line_no);
			}
			else {
				$ok = 0;
			}
				
		}

		if (isset($_POST['Line'.$line_no.'Desc'])) {
			$line_desc = post_scalar('Line'.$line_no.'Desc');
			if (strlen((string) $line_desc) > 0) {
				session_obj('Items')->line_items[$line_no]->item_description = (string) $line_desc;
			}
		}
	}
 return $ok;
}

/**
 * @param array<array-key, int|string>|null $delivery_notes
 */
function set_delivery_shipping_sum(?array $delivery_notes): void
{

    $shipping = 0.0;

    foreach($delivery_notes ?? array() as $delivery_num)
    {
        $myrow = row_or_empty(get_customer_trans((string) $delivery_num, ST_CUSTDELIVERY));

        $shipping += (float) $myrow['ov_freight'];
    }
    $_POST['ChargeFreightCost'] = price_format($shipping);
}


function copy_to_cart(): void
{
	$cart = session_obj('Items');
	$cart->due_date = $cart->document_date = (string) post_scalar('InvoiceDate');
	$cart->Comments = (string) post_scalar('Comments');
	$cart->due_date = (string) post_scalar('due_date');
	$pos = $cart->pos;
	if ($pos && ((bool)$pos['cash_sale'] || (bool)$pos['credit_sale']) && isset($_POST['payment'])) {
		$cart->payment = post_scalar('payment');
		$cart->payment_terms = (array) get_payment_terms(post_scalar('payment'));
	}
	if (session_obj('Items')->trans_no == 0) {
		$cart->reference = (string) post_scalar('ref');
        }
	if (!((bool) $cart->is_prepaid()))
	{
		$cart->ship_via = post_scalar('ship_via');
		$cart->freight_cost = (float) input_num('ChargeFreightCost');
	}

	$cart->update_payments();

	$dim_id = post_scalar('dimension_id');
	$cart->dimension_id = is_int($dim_id) ? $dim_id : (string) $dim_id;
	$dim2_id = post_scalar('dimension2_id');
	$cart->dimension2_id = is_int($dim2_id) ? $dim2_id : (string) $dim2_id;
}
//-----------------------------------------------------------------------------

function copy_from_cart(): void
{
	$cart = session_obj('Items');
 	$_POST['Comments']= $cart->Comments;
	$_POST['InvoiceDate']= $cart->document_date;
 	$_POST['ref'] = $cart->reference;
	$_POST['cart_id'] = $cart->cart_id;
	$_POST['due_date'] = $cart->due_date;
 	$_POST['payment'] = $cart->payment;
	if (!((bool) session_obj('Items')->is_prepaid()))
	{
		$_POST['ship_via'] = $cart->ship_via;
		$_POST['ChargeFreightCost'] = price_format($cart->freight_cost);
	}
	$_POST['dimension_id'] = $cart->dimension_id;
	$_POST['dimension2_id'] = $cart->dimension2_id;
}

//-----------------------------------------------------------------------------

#[\NoDiscard]
function check_data(): bool
{

	$prepaid = (bool) session_obj('Items')->is_prepaid();

	$invoice_date = post_scalar('InvoiceDate');
	if (!isset($_POST['InvoiceDate']) || !is_date($invoice_date === null ? null : (string) $invoice_date)) {
		display_error(_("The entered invoice date is invalid."));
		set_focus('InvoiceDate');
		return false;
	}

	if (!(bool)is_date_in_fiscalyear(post_scalar('InvoiceDate'))) {
		display_error(_("The entered date is out of fiscal year or is closed for further data entry."));
		set_focus('InvoiceDate');
		return false;
	}


	$due_date_ = post_scalar('due_date');
	if (!$prepaid &&(!isset($_POST['due_date']) || !is_date($due_date_ === null ? null : (string) $due_date_)))	{
		display_error(_("The entered invoice due date is invalid."));
		set_focus('due_date');
		return false;
	}

	if (session_obj('Items')->trans_no == 0) {
		if (!refs()->is_valid($_POST['ref'], ST_SALESINVOICE)) {
			display_error(_("You must enter a reference."));
			set_focus('ref');
			return false;
		}
	}

	if(!$prepaid) 
	{
		if ($_POST['ChargeFreightCost'] == "") {
			$_POST['ChargeFreightCost'] = price_format(0);
		}

		if (!check_num('ChargeFreightCost', 0)) {
			display_error(_("The entered shipping value is not numeric."));
			set_focus('ChargeFreightCost');
			return false;
		}

		if (session_obj('Items')->has_items_dispatch() == 0 && input_num('ChargeFreightCost') == 0) {
			display_error(_("There are no item quantities on this invoice."));
			return false;
		}

		if (!check_quantities()) {
			display_error(_("Selected quantity cannot be less than quantity credited nor more than quantity not invoiced yet."));
			return false;
		}
	} else {
		if ((session_obj('Items')->payment_terms['days_before_due'] == -1) && !count(session_obj('Items')->prepayments)) {
			display_error(_("There is no non-invoiced payments for this order. If you want to issue final invoice, select delayed or cash payment terms."));
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
if (isset($_POST['process_invoice']) && check_data()) {
	$newinvoice=  session_obj('Items')->trans_no == 0;
	copy_to_cart();

	if ($newinvoice) 
		new_doc_date(session_obj('Items')->document_date);

	$invoice_no = session_obj('Items')->write();
	if ($invoice_no == -1)
	{
		display_error(_("The entered reference is already in use."));
		set_focus('ref');
	}
	else
	{
		processing_end();

		if ($newinvoice) {
			meta_forward($_SERVER['PHP_SELF'] ?? '', "AddedID=$invoice_no");
		} else {
			meta_forward($_SERVER['PHP_SELF'] ?? '', "UpdatedID=$invoice_no");
		}
	}
}

if(list_updated('payment')) {
	$order = session_obj('Items');
	copy_to_cart();
	$order->payment = post_scalar('payment');
	$order->payment_terms = (array) get_payment_terms($order->payment);
	$_POST['due_date'] = $order->due_date = get_invoice_duedate($order->payment, $order->document_date);
	$_POST['Comments'] = '';
	ajax()->activate('due_date');
	ajax()->activate('options');
	if ((bool)$order->payment_terms['cash_sale']) {
		$pos = $order->pos;
		if ($pos) {
			$_POST['Location'] = $order->Location = $pos['pos_location'];
			$order->location_name = $pos['location_name'];
		}
	}
}

// find delivery spans for batch invoice display
$dspans = array();
$lastdn = ''; $spanlen=1;

for ($line_no = 0; $line_no < count(session_obj('Items')->line_items); $line_no++) {
	$line = session_obj('Items')->line_items[$line_no];
	if ($line->quantity == $line->qty_done) {
		continue;
	}
	if ($line->src_no == $lastdn) {
		$spanlen++;
	} else {
		if ($lastdn != '') {
			$dspans[] = $spanlen;
			$spanlen = 1;
		}
	}
	$lastdn = $line->src_no;
}
$dspans[] = $spanlen;

//-----------------------------------------------------------------------------

$src_docs = session_obj('Items')->src_docs;
$is_batch_invoice = (is_array($src_docs) ? count($src_docs) : 1) > 1;
$prepaid = session_obj('Items')->is_prepaid();

$is_edition = session_obj('Items')->trans_type == ST_SALESINVOICE && session_obj('Items')->trans_no != 0;
start_form();
hidden('cart_id');

start_table(TABLESTYLE2, "width='80%'", 5);

start_row();
$colspan = 1;
$dim = get_company_pref('use_dimension');
if ($dim > 0) 
	$colspan = 3;
label_cells(_("Customer"), session_obj('Items')->customer_name, "class='tableheader2'");
label_cells(_("Branch"), get_branch_name(session_obj('Items')->Branch), "class='tableheader2'");
$items_pos = session_obj('Items')->pos;
if ($items_pos && ((bool)$items_pos['credit_sale'] || (bool)$items_pos['cash_sale'])) {
	$paymcat = !(bool)$items_pos['cash_sale'] ? PM_CREDIT :
		(!(bool)$items_pos['credit_sale'] ? PM_CASH : PM_ANY);
	label_cells(_("Payment terms:"), sale_payment_list('payment', $paymcat),
		"class='tableheader2'", "colspan=$colspan");
} else
	label_cells(_('Payment:'), session_obj('Items')->payment_terms['terms'], "class='tableheader2'", "colspan=$colspan");

end_row();
start_row();

if (session_obj('Items')->trans_no == 0) {
	ref_cells(_("Reference"), 'ref', '', null, "class='tableheader2'", false, ST_SALESINVOICE,
		array('customer' => session_obj('Items')->customer_id,
			'branch' => session_obj('Items')->Branch,
			'date' => get_post('InvoiceDate')));
} else {
	label_cells(_("Reference"), session_obj('Items')->reference, "class='tableheader2'");
}

label_cells(_("Sales Type"), session_obj('Items')->sales_type_name, "class='tableheader2'");

label_cells(_("Currency"), session_obj('Items')->customer_currency, "class='tableheader2'");
if ($dim > 0) {
	label_cell(_("Dimension").":", "class='tableheader2'");
	$_POST['dimension_id'] = session_obj('Items')->dimension_id;
	dimensions_list_cells(null, 'dimension_id', null, true, ' ', false, 1, false);
}		
else
	hidden('dimension_id', 0);

end_row();
start_row();

if (!isset($_POST['ship_via'])) {
	$_POST['ship_via'] = session_obj('Items')->ship_via;
}
label_cell(_("Shipping Company"), "class='tableheader2'");
if ($prepaid)
{
	$shipper = row_or_empty(get_shipper(session_obj('Items')->ship_via));
	label_cells(null, $shipper['shipper_name']);
} else
	shippers_list_cells(null, 'ship_via', post_scalar('ship_via'));

$invoice_date_ = post_scalar('InvoiceDate');
if (!isset($_POST['InvoiceDate']) || !is_date($invoice_date_ === null ? null : (string) $invoice_date_)) {
	$_POST['InvoiceDate'] = new_doc_date();
	if (!(bool)is_date_in_fiscalyear(post_scalar('InvoiceDate'))) {
		$_POST['InvoiceDate'] = end_fiscalyear();
	}
}

date_cells(_("Date"), 'InvoiceDate', '', session_obj('Items')->trans_no == 0,
	0, 0, 0, "class='tableheader2'", true);

$due_date_2 = post_scalar('due_date');
if (!isset($_POST['due_date']) || !is_date($due_date_2 === null ? null : (string) $due_date_2)) {
	$_POST['due_date'] = get_invoice_duedate(session_obj('Items')->payment, post_scalar('InvoiceDate'));
}

date_cells(_("Due Date"), 'due_date', '', null, 0, 0, 0, "class='tableheader2'");
if ($dim > 1) {
	label_cell(_("Dimension")." 2:", "class='tableheader2'");
	$_POST['dimension2_id'] = session_obj('Items')->dimension2_id;
	dimensions_list_cells(null, 'dimension2_id', null, true, ' ', false, 2, false);
}		
else
	hidden('dimension2_id', 0);
end_row();
end_table();

$row = row_or_empty(get_customer_to_order(session_obj('Items')->customer_id));
if ($row['dissallow_invoices'] == 1)
{
	display_error(_("The selected customer account is currently on hold. Please contact the credit control personnel to discuss."));
	end_form();
	end_page();
	exit();
}	

display_heading($prepaid ? _("Sales Order Items") : _("Invoice Items"));

div_start('Items');

start_table(TABLESTYLE, "width='80%'");
if ($prepaid)
	$th = array(_("Item Code"), _("Item Description"), _("Units"), _("Quantity"),
		_("Price"), _("Tax Type"), _("Discount"), _("Total"));
else
	$th = array(_("Item Code"), _("Item Description"), _("Delivered"), _("Units"), _("Invoiced"),
		_("This Invoice"), _("Price"), _("Tax Type"), _("Discount"), _("Total"));

if ($is_batch_invoice) {
    $th[] = _("DN");
    $th[] = "";
}

if ($is_edition) {
    $th[4] = _("Credited");
}

table_header($th);
$k = 0;
$has_marked = false;
$show_qoh = true;

$dn_line_cnt = 0;

foreach (session_obj('Items')->line_items as $line=>$ln_itm) {
	if (!$prepaid && ($ln_itm->quantity == $ln_itm->qty_done)) {
		continue; // this line was fully invoiced
	}
	alt_table_row_color($k);
	view_stock_status_cell($ln_itm->stock_id);

	if ($prepaid)
		label_cell($ln_itm->item_description);
	else
		text_cells(null, 'Line'.$line.'Desc', $ln_itm->item_description, 30, 50);
	$dec = get_qty_dec($ln_itm->stock_id);
	if (!$prepaid)
		qty_cell($ln_itm->quantity, false, $dec);
	label_cell($ln_itm->units);
	if (!$prepaid)
		qty_cell($ln_itm->qty_done, false, $dec);

	if ($is_batch_invoice || $prepaid) {
		// for batch invoices we can only remove whole deliveries
		echo '<td nowrap align=right>';
		hidden('Line' . $line, $ln_itm->qty_dispatched );
		echo number_format2($ln_itm->qty_dispatched, $dec).'</td>';
	} else {
		small_qty_cells(null, 'Line'.$line, qty_format($ln_itm->qty_dispatched, $ln_itm->stock_id, $dec), null, null, $dec);
	}
	$display_discount_percent = percent_format((float)$ln_itm->discount_percent*100.0) . " %";

	$line_total = ((float)$ln_itm->qty_dispatched * (float)$ln_itm->price * (1.0 - (float)$ln_itm->discount_percent));

	amount_cell($ln_itm->price);
	label_cell($ln_itm->tax_type_name);
	label_cell($display_discount_percent, "nowrap align=right");
	amount_cell($line_total);

	if ($is_batch_invoice) {
		if ($dn_line_cnt == 0) {
			$dn_line_cnt = $dspans[0];
			$dspans = array_slice($dspans, 1);
			label_cell($ln_itm->src_no, "rowspan=$dn_line_cnt class='oddrow'");
			label_cell("<a href='" . ($_SERVER['PHP_SELF'] ?? '') . "?RemoveDN=".
				(string) $ln_itm->src_no."'>" . _("Remove") . "</a>", "rowspan=$dn_line_cnt class='oddrow'");
		}
		$dn_line_cnt--;
	}
	end_row();
}

/*Don't re-calculate freight if some of the order has already been delivered -
depending on the business logic required this condition may not be required.
It seems unfair to charge the customer twice for freight if the order
was not fully delivered the first time ?? */

if (!isset($_POST['ChargeFreightCost']) || $_POST['ChargeFreightCost'] == "") {
	if (session_obj('Items')->any_already_delivered() == 1) {
		$_POST['ChargeFreightCost'] = price_format(0);
	} else {
		$_POST['ChargeFreightCost'] = price_format(session_obj('Items')->freight_cost);
	}

	if (!check_num('ChargeFreightCost')) {
		$_POST['ChargeFreightCost'] = price_format(0);
	}
}

$accumulate_shipping = get_company_pref('accumulate_shipping');
if ($is_batch_invoice && $accumulate_shipping) {
	$items_src_docs = session_obj('Items')->src_docs;
	set_delivery_shipping_sum(is_array($items_src_docs) ? array_keys($items_src_docs) : array());
}

$colspan = $prepaid ? 7:9;
start_row();
label_cell(_("Shipping Cost"), "colspan=$colspan align=right");
if ($prepaid)
	label_cell(post_scalar('ChargeFreightCost'), 'align=right');
else
	small_amount_cells(null, 'ChargeFreightCost', null);
if ($is_batch_invoice) {
label_cell('', 'colspan=2');
}

end_row();
$inv_items_total = session_obj('Items')->get_items_total_dispatch();

$display_sub_total = price_format((float)$inv_items_total + (float)input_num('ChargeFreightCost'));

label_row(_("Sub-total"), $display_sub_total, "colspan=$colspan align=right","align=right", $is_batch_invoice ? 2 : 0);

$taxes = session_obj('Items')->get_taxes((float) input_num('ChargeFreightCost'));
$tax_total = (float) display_edit_tax_items($taxes, $colspan, session_obj('Items')->tax_included, $is_batch_invoice ? 2 : 0);

$display_total = price_format(((float)$inv_items_total + (float)input_num('ChargeFreightCost') + $tax_total));

label_row(_("Invoice Total"), $display_total, "colspan=$colspan align=right","align=right", $is_batch_invoice ? 2 : 0);

end_table(1);
div_end();
div_start('options');
start_table(TABLESTYLE2);
if ($prepaid)
{

	label_row(_("Sales order:"), get_trans_view_str((string) ST_SALESORDER, (string) session_obj('Items')->order_no, get_reference(ST_SALESORDER, session_obj('Items')->order_no)));

	$list = array(); $allocs = 0.0;
	if (count(session_obj('Items')->prepayments))
	{
		foreach(session_obj('Items')->prepayments as $pmt)
		{
			$list[] = get_trans_view_str((string) $pmt['trans_type_from'], (string) $pmt['trans_no_from'], get_reference((string) $pmt['trans_type_from'], (string) $pmt['trans_no_from']));
			$allocs += (float) $pmt['amt'];
		}
	}
	label_row(_("Payments received:"), implode(',', $list));
	label_row(_("Invoiced here:"), price_format(session_obj('Items')->prep_amount), 'class=label');
	label_row(session_obj('Items')->payment_terms['days_before_due'] == -1 ? _("Left to be invoiced:") : _("Invoiced so far:"),
		price_format(session_obj('Items')->get_trans_total()-max((float)session_obj('Items')->prep_amount, $allocs)), 'class=label');
}

textarea_row(_("Memo:"), 'Comments', null, 50, 4);

end_table(1);
div_end();
submit_center_first('Update', _("Update"),
  _('Refresh document page'), true);
submit_center_last('process_invoice', _("Process Invoice"),
  _('Check entered data and save document'), 'default');

end_form();

end_page();

