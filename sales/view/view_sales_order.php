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
$page_security = 'SA_SALESTRANSVIEW';
$path_to_root = "../..";
include_once($path_to_root . "/sales/includes/cart_class.inc");

include_once($path_to_root . "/includes/session.inc");
include_once($path_to_root . "/includes/date_functions.inc");

include_once($path_to_root . "/sales/includes/sales_ui.inc");
include_once($path_to_root . "/sales/includes/sales_db.inc");

$js = "";
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(900, 600);

if ($_GET['trans_type'] == ST_SALESQUOTE)
{
	page(_($help_context = "View Sales Quotation"), true, false, "", $js);
	display_heading(sprintf(_("Sales Quotation #%d"),$_GET['trans_no']));
}	
else
{
	page(_($help_context = "View Sales Order"), true, false, "", $js);
	display_heading(sprintf(_("Sales Order #%d"),$_GET['trans_no']));
}

if (isset($_SESSION['View']))
{
	unset ($_SESSION['View']);
}

$_SESSION['View'] = new Cart($_GET['trans_type'], $_GET['trans_no']);

start_table(TABLESTYLE2, "width='95%'", 5);

if ($_GET['trans_type'] != ST_SALESQUOTE)
{
	echo "<tr valign=top><td>";
	display_heading2(_("Order Information"));
	echo "</td><td>";
	display_heading2(_("Deliveries"));
	echo "</td><td>";
	display_heading2(_("Invoices/Credits"));
	echo "</td></tr>";
}	

echo "<tr valign=top><td>";

start_table(TABLESTYLE, "width='95%'");
label_row(_("Customer Name"), session_obj('View')->customer_name, "class='tableheader2'",
	"colspan=3");
start_row();
label_cells(_("Customer Order Ref."), session_obj('View')->cust_ref, "class='tableheader2'");
label_cells(_("Deliver To Branch"), session_obj('View')->deliver_to, "class='tableheader2'");
end_row();
start_row();
label_cells(_("Ordered On"), session_obj('View')->document_date, "class='tableheader2'");
if ($_GET['trans_type'] == ST_SALESQUOTE)
	label_cells(_("Valid until"), session_obj('View')->due_date, "class='tableheader2'");
elseif (session_obj('View')->reference == "auto")
	label_cells(_("Due Date"), session_obj('View')->due_date, "class='tableheader2'");
else
	label_cells(_("Requested Delivery"), session_obj('View')->due_date, "class='tableheader2'");
end_row();
start_row();
label_cells(_("Order Currency"), session_obj('View')->customer_currency, "class='tableheader2'");
label_cells(_("Deliver From Location"), session_obj('View')->location_name, "class='tableheader2'");
end_row();


if (session_obj('View')->payment_terms['days_before_due']<0)
{
start_row();
label_cells(_("Payment Terms"), session_obj('View')->payment_terms['terms'], "class='tableheader2'");
label_cells(_("Required Pre-Payment"), price_format(session_obj('View')->prep_amount), "class='tableheader2'");
end_row();
start_row();
label_cells(_("Non-Invoiced Prepayments"), price_format(session_obj('View')->alloc), "class='tableheader2'");
label_cells(_("All Payments Allocated"), price_format(session_obj('View')->sum_paid), "class='tableheader2'");
end_row();
} else
	label_row(_("Payment Terms"), session_obj('View')->payment_terms['terms'], "class='tableheader2'", "colspan=3");

label_row(_("Delivery Address"), nl2br(session_obj('View')->delivery_address),
	"class='tableheader2'", "colspan=3");
label_row(_("Reference"), session_obj('View')->reference, "class='tableheader2'", "colspan=3");
label_row(_("Telephone"), session_obj('View')->phone, "class='tableheader2'", "colspan=3");
label_row(_("E-mail"), "<a href='mailto:" . session_obj('View')->email . "'>" . session_obj('View')->email . "</a>",
	"class='tableheader2'", "colspan=3");
label_row(_("Comments"), !empty(session_obj('View')->Comments) ? nl2br(session_obj('View')->Comments) : "", "class='tableheader2'", "colspan=3");
end_table();

if ($_GET['trans_type'] != ST_SALESQUOTE)
{
	echo "</td><td valign='top'>";

	start_table(TABLESTYLE);
	display_heading2(_("Delivery Notes"));


	$th = array(_("#"), _("Ref"), _("Date"), _("Total"));
	table_header($th);

	$dn_numbers = array();
	$delivery_total = 0;

	if ($result = get_sales_child_documents(ST_SALESORDER, $_GET['trans_no'])) {

		$k = 0;
		while ($del_row = db_fetch($result))
		{

			alt_table_row_color($k);
			$dn_numbers[] = $del_row["trans_no"];
			$this_total = (float)$del_row["ov_freight"]+ (float)$del_row["ov_amount"] + (float)$del_row["ov_freight_tax"]  + (float)$del_row["ov_gst"] ;
			$delivery_total += (float)$this_total;

			label_cell(get_customer_trans_view_str($del_row["type"], $del_row["trans_no"]));
			label_cell($del_row["reference"]);
			label_cell(sql2date($del_row["tran_date"]));
			amount_cell($this_total);
			end_row();
		}
	}

	label_row(null, price_format($delivery_total), " ", "colspan=4 align=right");

	end_table();
	echo "</td><td valign='top'>";

	start_table(TABLESTYLE);
	display_heading2(_("Sales Invoices"));

	$th = array(_("#"), _("Ref"), _("Date"), _("Total"));
	table_header($th);
	
	$inv_numbers = array();
	$invoices_total = 0;

	if (session_obj('View')->prepaid)
		$result = get_sales_order_invoices($_GET['trans_no']);
	else
		$result = get_sales_child_documents(ST_CUSTDELIVERY, $dn_numbers);

	if ($result) {
		$k = 0;

		while ($inv_row = db_fetch($result))
		{
			alt_table_row_color($k);

			$this_total = session_obj('View')->prepaid ? $inv_row["prep_amount"] : 
				(float)$inv_row["ov_freight"] + (float)$inv_row["ov_freight_tax"]  + (float)$inv_row["ov_gst"] + (float)$inv_row["ov_amount"];
			$invoices_total += (float)$this_total;

			$inv_numbers[] = $inv_row["trans_no"];
			label_cell(get_customer_trans_view_str($inv_row["type"], $inv_row["trans_no"]));
			label_cell($inv_row["reference"]);
			label_cell(sql2date($inv_row["tran_date"]));
			amount_cell($this_total);
			end_row();
		}
	}
	label_row(null, price_format($invoices_total), " ", "colspan=4 align=right");

	end_table();

	display_heading2(_("Credit Notes"));

	start_table(TABLESTYLE);
	$th = array(_("#"), _("Ref"), _("Date"), _("Total"));
	table_header($th);

	$credits_total = 0;

	if ($result = get_sales_child_documents(ST_SALESINVOICE, $inv_numbers)) {
		$k = 0;

		while ($credits_row = db_fetch($result))
		{

			alt_table_row_color($k);

			$this_total = (float)$credits_row["ov_freight"] + (float)$credits_row["ov_freight_tax"]  + (float)$credits_row["ov_gst"] + (float)$credits_row["ov_amount"];
			$credits_total += (float)$this_total;

			label_cell(get_customer_trans_view_str($credits_row["type"], $credits_row["trans_no"]));
			label_cell($credits_row["reference"]);
			label_cell(sql2date($credits_row["tran_date"]));
			amount_cell(-$this_total);
			end_row();

		}

	}
	label_row(null, "<font color=red>" . price_format(-$credits_total) . "</font>",
		" ", "colspan=4 align=right");

	end_table();

	echo "</td></tr>";

	end_table();
}
echo "<center>";
if (session_obj('View')->so_type == 1)
	display_note(_("This Sales Order is used as a Template."), 0, 0, "class='currentfg'");
display_heading2(_("Line Details"));

start_table(TABLESTYLE, "width='95%'");
$th = array(_("Item Code"), _("Item Description"), _("Quantity"), _("Unit"),
	_("Price"), _("Discount"), _("Total"), _("Quantity Delivered"));
table_header($th);

$k = 0;  //row colour counter

foreach (session_obj('View')->line_items as $stock_item) {

	$line_total = round2((float)$stock_item->quantity * (float)$stock_item->price * (1.0 - (float)$stock_item->discount_percent),
	   user_price_dec());

	alt_table_row_color($k);

	label_cell($stock_item->stock_id);
	label_cell($stock_item->item_description);
	$dec = get_qty_dec($stock_item->stock_id);
	qty_cell($stock_item->quantity, false, $dec);
	label_cell($stock_item->units);
	amount_cell($stock_item->price);
	amount_cell((float)$stock_item->discount_percent * 100.0);
	amount_cell($line_total);

	qty_cell($stock_item->qty_done, false, $dec);
	end_row();
}

if (session_obj('View')->freight_cost != 0.0)
	label_row(_("Shipping"), price_format(session_obj('View')->freight_cost),
		"align=right colspan=6", "nowrap align=right", 1);

$sub_tot = (float)session_obj('View')->get_items_total() + (float)session_obj('View')->freight_cost;

$display_sub_tot = price_format($sub_tot);

label_row(_("Sub Total"), $display_sub_tot, "align=right colspan=6",
	"nowrap align=right", 1);

$taxes = session_obj('View')->get_taxes();

$tax_total = display_edit_tax_items($taxes, 6, session_obj('View')->tax_included,2);

$display_total = price_format((float)$sub_tot + (float)$tax_total);

start_row();
label_cells(_("Amount Total"), $display_total, "colspan=6 align='right'","align='right'");
label_cell('', "colspan=2");
end_row();
end_table();

display_allocations_to(PT_CUSTOMER, session_obj('View')->customer_id, $_GET['trans_type'], $_GET['trans_no'], (float)$sub_tot + (float)$tax_total);

end_page(true, false, false, get_scalar('trans_type'), $_GET['trans_no']);

