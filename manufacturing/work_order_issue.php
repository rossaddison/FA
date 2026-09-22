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
$page_security = 'SA_MANUFISSUE';
/** @var string $path_to_root */
$path_to_root = "..";

include_once(dirname(__DIR__) . "/includes/ui/items_cart.inc");

include_once(dirname(__DIR__) . "/includes/session.inc");

include_once(dirname(__DIR__) . "/includes/date_functions.inc");
include_once(dirname(__DIR__) . "/includes/data_checks.inc");

include_once(dirname(__DIR__) . "/manufacturing/includes/manufacturing_db.inc");
include_once(dirname(__DIR__) . "/manufacturing/includes/manufacturing_ui.inc");
include_once(dirname(__DIR__) . "/manufacturing/includes/work_order_issue_ui.inc");
$js = "";
if (sysprefs()->use_popup_windows)
	$js .= get_js_open_window(800, 500);
if (user_use_date_picker())
	$js .= get_js_date_picker();

page(_($help_context = "Issue Items to Work Order"), false, false, "", $js);

//-----------------------------------------------------------------------------------------------

if (isset($_GET['AddedID'])) 
{
	$id = $_GET['AddedID'];
   	display_notification(_("The work order issue has been entered."));

    display_note(get_trans_view_str(ST_WORKORDER, $id, _("View this Work Order")));

   	display_note(get_gl_view_str(ST_WORKORDER, $id, _("View the GL Journal Entries for this Work Order")), 1);

   	hyperlink_no_params("search_work_orders.php", _("Select another &Work Order to Process"));

	display_footer_exit();
}
//--------------------------------------------------------------------------------------------------

function line_start_focus(): void {

  ajax()->activate('items_table');
  set_focus('_stock_id_edit');
}

//--------------------------------------------------------------------------------------------------

function handle_new_order(): void
{
	if (isset($_SESSION['issue_items']))
	{
		session_obj('issue_items')->clear_items();
		unset ($_SESSION['issue_items']);
	}

     $_SESSION['issue_items'] = new items_cart(ST_MANUISSUE);
     session_obj('issue_items')->order_id = $_GET['trans_no'];
}

//-----------------------------------------------------------------------------------------------
function can_process(): bool
{
	if (!is_date(post_scalar('date_')))
	{
		display_error(_("The entered date for the issue is invalid."));
		set_focus('date_');
		return false;
	} 
	elseif (!(bool)is_date_in_fiscalyear(post_scalar('date_')))
	{
		display_error(_("The entered date is out of fiscal year or is closed for further data entry."));
		set_focus('date_');
		return false;
	}
	if (!check_reference($_POST['ref'], ST_MANUISSUE))
	{
		set_focus('ref');
		return false;
	}

	$failed_item = session_obj('issue_items')->check_qoh($_POST['Location'], $_POST['date_'], !(bool)$_POST['IssueType']);
	if ($failed_item)
	{
   		display_error(_("The issue cannot be processed because it would cause negative inventory balance for marked items as of document date or later."));
		return false;
	}

	return true;
}

if (isset($_POST['Process']) && can_process())
{

	// if failed, returns a stockID
	$failed_data = add_work_order_issue(session_obj('issue_items')->order_id,
		post_scalar('ref'), $_POST['IssueType'], session_obj('issue_items')->line_items,
		post_scalar('Location'), post_scalar('WorkCentre'), post_scalar('date_'), post_scalar('memo_'));

	if ($failed_data != null) 
	{
		display_error(_("The process cannot be completed because there is an insufficient total quantity for a component.") . "<br>"
		. _("Component is :"). $failed_data[0] . "<br>"
		. _("From location :"). $failed_data[1] . "<br>");
	} 
	else 
	{
		meta_forward($_SERVER['PHP_SELF'], "AddedID=".session_obj('issue_items')->order_id);
	}

} /*end of process credit note */

//-----------------------------------------------------------------------------------------------

function check_item_data(): bool
{
	if (input_num('qty') == 0 || !check_num('qty', 0))
	{
		display_error(_("The quantity entered is negative or invalid."));
		set_focus('qty');
		return false;
	}

	if (!check_num('std_cost', 0))
	{
		display_error(_("The entered standard cost is negative or invalid."));
		set_focus('std_cost');
		return false;
	}

   	return true;
}

//-----------------------------------------------------------------------------------------------

function handle_update_item(): void
{
    if($_POST['UpdateItem'] != "" && check_item_data())
    {
		$id = $_POST['LineNo'];
    	session_obj('issue_items')->update_cart_item($id, input_num('qty'), input_num('std_cost'));
    }
	line_start_focus();
}

//-----------------------------------------------------------------------------------------------

function handle_delete_item(string|int|null $id): void
{
	session_obj('issue_items')->remove_from_cart($id);
	line_start_focus();
}

//-----------------------------------------------------------------------------------------------

function handle_new_item(): void
{
	if (!check_item_data())
		return;

	add_to_issue($_SESSION['issue_items'], $_POST['stock_id'], input_num('qty'),
		 input_num('std_cost'));
	line_start_focus();
}

//-----------------------------------------------------------------------------------------------
$id = find_submit('Delete');
if ($id != -1)
	handle_delete_item($id);

if (isset($_POST['AddItem']))
	handle_new_item();

if (isset($_POST['UpdateItem']))
	handle_update_item();

if (isset($_POST['CancelItemChanges'])) {
	line_start_focus();
}

//-----------------------------------------------------------------------------------------------

if (isset($_GET['trans_no']))
{
	handle_new_order();
}

//-----------------------------------------------------------------------------------------------

display_wo_details(session_obj('issue_items')->order_id);
echo "<br>";

start_form();

start_table(TABLESTYLE, "width='90%'", 10);
echo "<tr><td>";
display_issue_items(_("Items to Issue"), $_SESSION['issue_items']);
issue_options_controls();
echo "</td></tr>";

end_table();

submit_center('Process', _("Process Issue"), true, '', 'default');

end_form();

//------------------------------------------------------------------------------------------------

end_page();

