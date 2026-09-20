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
$page_security = 'SA_WORKORDERENTRY';
/** @var string $path_to_root */
$path_to_root = "..";

include_once($path_to_root . "/includes/session.inc");

include_once($path_to_root . "/includes/date_functions.inc");
include_once($path_to_root . "/includes/data_checks.inc");

include_once($path_to_root . "/manufacturing/includes/manufacturing_db.inc");
include_once($path_to_root . "/manufacturing/includes/manufacturing_ui.inc");

$js = "";
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(900, 500);
if (user_use_date_picker())
	$js .= get_js_date_picker();
page(_($help_context = "Work Order Entry"), false, false, "", $js);


check_db_has_manufacturable_items(_("There are no manufacturable items defined in the system."));

check_db_has_locations(("There are no inventory locations defined in the system."));

//---------------------------------------------------------------------------------------

if (isset($_GET['trans_no']))
{
	$selected_id = $_GET['trans_no'];
}
elseif(isset($_POST['selected_id']))
{
	$selected_id = $_POST['selected_id'];
}

//---------------------------------------------------------------------------------------

if (isset($_GET['AddedID']))
{
	$id = $_GET['AddedID'];
	$stype = ST_WORKORDER;

	display_notification_centered(_("The work order been added."));

    display_note(get_trans_view_str($stype, $id, _("View this Work Order")), 0, 1);

	if ($_GET['type'] != WO_ADVANCED)
	{
		include_once($path_to_root . "/reporting/includes/reporting.inc");

		submenu_print(_("&Print This Work Order"), ST_WORKORDER, $id, 'prtopt');
		submenu_print(_("&Email This Work Order"), ST_WORKORDER, $id, null, 1);
    	display_note(get_gl_view_str($stype, $id, _("View the GL Journal Entries for this Work Order")), 1);
    	$ar = array('PARAM_0' => $_GET['date'], 'PARAM_1' => $_GET['date'], 'PARAM_2' => $stype, 'PARAM_3' => '',
    		'PARAM_4' => (user_def_print_orientation() == 1 ? 1 : 0)); 
    	display_note(print_link(_("Print the GL Journal Entries for this Work Order"), 702, $ar), 1);
		hyperlink_params("$path_to_root/admin/attachments.php", _("Add an Attachment"), "filterType=$stype&trans_no=$id");
	}
	
	safe_exit();
}

//---------------------------------------------------------------------------------------

if (isset($_GET['UpdatedID']))
{
	$id = $_GET['UpdatedID'];

	display_notification_centered(_("The work order been updated."));
	safe_exit();
}

//---------------------------------------------------------------------------------------

if (isset($_GET['DeletedID']))
{
	$id = $_GET['DeletedID'];

	display_notification_centered(_("Work order has been deleted."));
	safe_exit();
}

//---------------------------------------------------------------------------------------

if (isset($_GET['ClosedID']))
{
	$id = $_GET['ClosedID'];

	display_notification_centered(_("This work order has been closed. There can be no more issues against it.") . " #$id");
	safe_exit();
}

//---------------------------------------------------------------------------------------

function safe_exit(): void
{
	global $path_to_root;

	hyperlink_no_params("", _("Enter a new work order"));
	hyperlink_no_params("search_work_orders.php", _("Select an existing work order"));
	
	display_footer_exit();
}

//-------------------------------------------------------------------------------------
if (!isset($_POST['date_']))
{
	$_POST['date_'] = new_doc_date();
	if (!(bool)is_date_in_fiscalyear(post_scalar('date_')))
		$_POST['date_'] = end_fiscalyear();
}

function can_process(): bool
{
	global $selected_id;

	if (!isset($selected_id))
	{
    	if (!check_reference($_POST['wo_ref'], ST_WORKORDER))
    	{
			set_focus('wo_ref');
    		return false;
    	}
	}

	if (!check_num('quantity', 1))
	{
		display_error( _("The quantity entered is invalid or less than zero."));
		set_focus('quantity');
		return false;
	}

	if (!is_date(post_scalar('date_')))
	{
		display_error( _("The date entered is in an invalid format."));
		set_focus('date_');
		return false;
	}
	elseif (!(bool)is_date_in_fiscalyear(post_scalar('date_')))
	{
		display_error(_("The entered date is out of fiscal year or is closed for further data entry."));
		set_focus('date_');
		return false;
	}
	// only check bom and quantites if quick assembly
	if (!($_POST['type'] == WO_ADVANCED))
	{
        if (!has_bom($_POST['stock_id']))
        {
        	display_error(_("The selected item to manufacture does not have a bom."));
			set_focus('stock_id');
        	return false;
        }

		if ($_POST['Labour'] == "")
			$_POST['Labour'] = price_format(0);
    	if (!check_num('Labour', 0))
    	{
    		display_error( _("The labour cost entered is invalid or less than zero."));
			set_focus('Labour');
    		return false;
    	}
		if ($_POST['Costs'] == "")
			$_POST['Costs'] = price_format(0);
    	if (!check_num('Costs', 0))
    	{
    		display_error( _("The cost entered is invalid or less than zero."));
			set_focus('Costs');
    		return false;
    	}

        if (!sysprefs()->allow_negative_stock())
        {
        	if ($_POST['type'] == WO_ASSEMBLY)
        	{
        		// check bom if assembling
                $result = get_bom($_POST['stock_id']);

            	while ($bom_item = db_fetch($result))
            	{

            		if (has_stock_holding($bom_item["ResourceType"]))
            		{

                		$quantity = (float)$bom_item["quantity"] * (float)input_num('quantity');

                        if (check_negative_stock($bom_item["component"], -$quantity, $bom_item["loc_code"], $_POST['date_']))
                		{
                			display_error(_("The work order cannot be processed because there is an insufficient quantity for component:") .
                				" " . (string)$bom_item["component"] . " - " .  (string)$bom_item["description"] . ".  " . _("Location:") . " " . (string)$bom_item["location_name"]);
							set_focus('quantity');
        					return false;
                		}
            		}
            	}
        	}
        	elseif ($_POST['type'] == WO_UNASSEMBLY)
        	{
        		// if unassembling, check item to unassemble
                if (check_negative_stock(post_scalar('stock_id'), -input_num('quantity'), post_scalar('StockLocation'), $_POST['date_']))
        		{
        			display_error(_("The selected item cannot be unassembled because there is insufficient stock."));
					return false;
        		}
        	}
    	}
     }
     else
     {
    	if (!is_date(post_scalar('RequDate')))
    	{
			set_focus('RequDate');
    		display_error( _("The date entered is in an invalid format."));
    		return false;
		}
    	if (isset($selected_id))
    	{
    		if ($_POST['units_issued'] > input_num('quantity'))
    		{
				set_focus('quantity');
    			display_error(_("The quantity cannot be changed to be less than the quantity already manufactured for this order."));
        		return false;
    		}
    	}
	}

	return true;
}

//-------------------------------------------------------------------------------------

if (isset($_POST['ADD_ITEM']) && can_process())
{
	if (!isset($_POST['cr_acc']))
		$_POST['cr_acc'] = "";
	if (!isset($_POST['cr_lab_acc']))
		$_POST['cr_lab_acc'] = "";
	$id = add_work_order(post_scalar('wo_ref'), post_scalar('StockLocation'), input_num('quantity'),
		post_scalar('stock_id'),  post_scalar('type'), post_scalar('date_'),
		post_scalar('RequDate'), post_scalar('memo_'), input_num('Costs'), post_scalar('cr_acc'), input_num('Labour'), post_scalar('cr_lab_acc'));

	new_doc_date($_POST['date_']);
	meta_forward($_SERVER['PHP_SELF'], "AddedID=$id&type=".(string)$_POST['type']."&date=".(string)$_POST['date_']);
}

//-------------------------------------------------------------------------------------

if (isset($_POST['UPDATE_ITEM']) && can_process())
{

	update_work_order($selected_id, post_scalar('StockLocation'), input_num('quantity'),
		post_scalar('stock_id'),  post_scalar('date_'), post_scalar('RequDate'), post_scalar('memo_'));
	new_doc_date($_POST['date_']);
	meta_forward($_SERVER['PHP_SELF'], "UpdatedID=$selected_id");
}

//--------------------------------------------------------------------------------------

if (isset($_POST['delete']))
{
	//the link to delete a selected record was clicked instead of the submit button

	$cancel_delete = false;

	// can't delete it there are productions or issues
	if (work_order_has_productions($selected_id) ||
		work_order_has_issues($selected_id)	||
		work_order_has_payments($selected_id))
	{
		display_error(_("This work order cannot be deleted because it has already been processed."));
		$cancel_delete = true;
	}

	if ($cancel_delete == false)
	{ //ie not cancelled the delete as a result of above tests

		// delete the actual work order
		delete_work_order($selected_id, post_scalar('stock_id'), post_scalar('quantity'), post_scalar('date_'));
		meta_forward($_SERVER['PHP_SELF'], "DeletedID=$selected_id");
	}
}

//-------------------------------------------------------------------------------------

if (isset($_POST['close']))
{

	// update the closed flag in the work order
	close_work_order($selected_id);
	meta_forward($_SERVER['PHP_SELF'], "ClosedID=$selected_id");
}

//-------------------------------------------------------------------------------------
if (get_post('_type_update')) 
{
  ajax()->activate('_page_body');
}
//-------------------------------------------------------------------------------------

start_form();

start_table(TABLESTYLE2);

$existing_comments = "";

$dec = 0;
if (isset($selected_id))
{
	$myrow = get_work_order($selected_id, true);

	if ($myrow === false)
	{
		echo _("The order number sent is not valid.");
		safe_exit();
	}

	// if it's a closed work order can't edit it
	if ($myrow["closed"] == 1)
	{
		echo "<center>";
		display_error(_("This work order is closed and cannot be edited."));
		safe_exit();
	}

	$_POST['wo_ref'] = $myrow["wo_ref"];
	$_POST['stock_id'] = $myrow["stock_id"];
	//$_POST['quantity'] = qty_format($myrow["units_reqd"], $_POST['stock_id'], $dec);
	$_POST['quantity'] = $myrow["units_reqd"];
	$_POST['StockLocation'] = $myrow["loc_code"];
	$_POST['released'] = $myrow["released"];
	$_POST['closed'] = $myrow["closed"];
	$_POST['type'] = $myrow["type"];
	$_POST['date_'] = sql2date($myrow["date_"]);
	$_POST['RequDate'] = sql2date($myrow["required_by"]);
	$_POST['released_date'] = sql2date($myrow["released_date"]);
	$_POST['units_issued'] = $myrow["units_issued"];
	$_POST['Costs'] = price_format($myrow["additional_costs"]);

	$_POST['memo_'] = get_comments_string(ST_WORKORDER, $selected_id);

	hidden('wo_ref', post_scalar('wo_ref'));
	hidden('units_issued', post_scalar('units_issued'));
	hidden('released', post_scalar('released'));
	hidden('released_date', post_scalar('released_date'));
	hidden('selected_id',  $selected_id);

	label_row(_("Reference:"), post_scalar('wo_ref'));
	label_row(_("Type:"), $wo_types_array[$_POST['type']]);
	hidden('type', $myrow["type"]);
}
else
{
	$_POST['units_issued'] = $_POST['released'] = 0;

	ref_row(_("Reference:"), 'wo_ref', '', refs()->get_next(ST_WORKORDER, null, get_post('date_')), false, ST_WORKORDER);

	wo_types_list_row(_("Type:"), 'type', null);
}

if (get_post('released'))
{
	hidden('stock_id', post_scalar('stock_id'));
	hidden('StockLocation', post_scalar('StockLocation'));
	hidden('type', post_scalar('type'));

	label_row(_("Item:"), $myrow["StockItemName"]);
	label_row(_("Destination Location:"), $myrow["location_name"]);
}
else
{
	stock_manufactured_items_list_row(_("Item:"), 'stock_id', null, false, true);
	if (list_updated('stock_id'))
		ajax()->activate('quantity');

	locations_list_row(_("Destination Location:"), 'StockLocation', null);
}

if (!isset($_POST['quantity']))
	$_POST['quantity'] = qty_format(1, $_POST['stock_id'], $dec);
else
	$_POST['quantity'] = qty_format(post_scalar('quantity'), $_POST['stock_id'], $dec);
	

if (get_post('type') == WO_ADVANCED)
{
    qty_row(_("Quantity Required:"), 'quantity', null, null, null, $dec);
    if ((bool)$_POST['released'])
    	label_row(_("Quantity Manufactured:"), number_format($_POST['units_issued'], get_qty_dec(post_scalar('stock_id'))));
    date_row(_("Date") . ":", 'date_', '', true);
	date_row(_("Date Required By") . ":", 'RequDate', '', null, sysprefs()->default_wo_required_by());
}
else
{
    qty_row(_("Quantity:"), 'quantity', null, null, null, $dec);
    date_row(_("Date") . ":", 'date_', '', true);
	hidden('RequDate', '');

	if (!isset($_POST['Labour']) || list_updated('stock_id') || list_updated('type'))
	{
		$bank_act = row_or_empty(get_default_bank_account());
		$item = row_or_empty(get_item(get_post('stock_id')));
		$_POST['Labour'] = price_format(get_post('type') == WO_ASSEMBLY ? $item['labour_cost'] : 0);
		$_POST['cr_lab_acc'] = $bank_act['account_code'];
		$_POST['Costs'] = price_format(get_post('type') == WO_ASSEMBLY ? $item['overhead_cost'] : 0);
		$_POST['cr_acc'] = $bank_act['account_code'];
		ajax()->activate('_page_body');
	}

	amount_row($wo_cost_types[WO_LABOUR], 'Labour');
	gl_all_accounts_list_row(_("Credit Labour Account"), 'cr_lab_acc', null);
	amount_row($wo_cost_types[WO_OVERHEAD], 'Costs');
	gl_all_accounts_list_row(_("Credit Overhead Account"), 'cr_acc', null);

}

if (get_post('released'))
	label_row(_("Released On:"),post_scalar('released_date'));

textarea_row(_("Memo:"), 'memo_', null, 40, 5);

end_table(1);

if (isset($selected_id))
{
	echo "<table align=center><tr>";

	submit_cells('UPDATE_ITEM', _("Update"), '', _('Save changes to work order'), 'default');
	if (get_post('released'))
	{
		submit_cells('close', _("Close This Work Order"),'','',true);
	}
	submit_cells('delete', _("Delete This Work Order"),'','',true);

	echo "</tr></table>";
}
else
{
	submit_center('ADD_ITEM', _("Add Workorder"), true, '', 'default');
}

end_form();
end_page();

