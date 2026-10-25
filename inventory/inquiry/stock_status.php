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
$page_security = 'SA_ITEMSSTATVIEW';
/** @var string $path_to_root */
$path_to_root = "../..";
include_once(dirname(__DIR__, 2) . "/includes/session.inc");

$js = "";
if (sysprefs()->use_popup_windows && sysprefs()->use_popup_search)
	$js .= get_js_open_window(900, 500);

if (isset($_GET['stock_id']))
	$_POST['stock_id'] = $_GET['stock_id'];

page(_($help_context = "Inventory Item Status"), isset($_GET['stock_id']), false, "", $js);

include_once(dirname(__DIR__, 2) . "/includes/date_functions.inc");
include_once(dirname(__DIR__, 2) . "/includes/ui.inc");
include_once(dirname(__DIR__, 2) . "/includes/data_checks.inc");

include_once(dirname(__DIR__, 2) . "/inventory/includes/inventory_db.inc");
include_once(dirname(__DIR__, 2) . "/includes/db/manufacturing_db.inc");

if (list_updated('stock_id')) 
	ajax()->activate('status_tbl');
//----------------------------------------------------------------------------------------------------

check_db_has_stock_items(_("There are no items defined in the system."));

start_form();

if (!isset($_POST['stock_id']))
	$_POST['stock_id'] = get_global_stock_item();

if (!$page_nested)
{
	echo "<center> " . _("Item:"). " ";
	echo stock_costable_items_list('stock_id', $_POST['stock_id'], false, true);
}
echo "<br>";

echo "<hr></center>";

set_global_stock_item($_POST['stock_id']);

$mb_flag = get_mb_flag(post_scalar('stock_id'));
$kitset_or_service = false;

div_start('status_tbl');
// get_mb_flag() returns -1 for a stock_id with no matching row (includes/db/inventory_db.inc) --
// is_service() is declared ?string under declare(strict_types=1), so passing that -1 through
// directly is a real TypeError crash, not just a Psalm type mismatch (confirmed via a standalone
// repro); -1 isn't a service, so treat it as the non-service case rather than pass it as a string.
if (is_service(is_string($mb_flag) ? $mb_flag : null))
{
	display_note(_("This is a service and cannot have a stock holding, only the total quantity on outstanding sales orders is shown."), 0, 1);
	$kitset_or_service = true;
}

$loc_details = get_loc_details(post_scalar('stock_id'));

start_table(TABLESTYLE);

if ($kitset_or_service == true)
{
	$th = array(_("Location"), _("Demand"));
}
else
{
	$th = array(_("Location"), _("Quantity On Hand"), _("Re-Order Level"),
		_("Demand"), _("Available"), _("On Order"));
}
table_header($th);
// post_scalar() is declared string|int|float|bool|null, but get_qty_dec() is ?string under
// declare(strict_types=1) -- guard the same way as get_mb_flag()'s result above
$stock_id_scalar = post_scalar('stock_id');
$dec = get_qty_dec(is_string($stock_id_scalar) ? $stock_id_scalar : null);
$j = 1;
$k = 0; //row colour counter

while ($myrow = db_fetch($loc_details))
{

	alt_table_row_color($k);

	$demand_qty = (float) get_demand_qty(post_scalar('stock_id'), $myrow["loc_code"]);
	$demand_qty += get_demand_asm_qty(post_scalar('stock_id'), $myrow["loc_code"]);

	$qoh = (float) get_qoh_on_date(post_scalar('stock_id'), $myrow["loc_code"]);

	if ($kitset_or_service == false)
	{
		$qoo = (float) get_on_porder_qty(post_scalar('stock_id'), $myrow["loc_code"]);
		$qoo += (float) get_on_worder_qty(post_scalar('stock_id'), $myrow["loc_code"]);
		label_cell($myrow["location_name"]);
		qty_cell($qoh, false, $dec);
        qty_cell($myrow["reorder_level"], false, $dec);
        qty_cell($demand_qty, false, $dec);
        qty_cell($qoh - $demand_qty, false, $dec);
        qty_cell($qoo, false, $dec);
        end_row();

	}
	else
	{
	/* It must be a service or kitset part */
		label_cell($myrow["location_name"]);
		qty_cell($demand_qty, false, $dec);
		end_row();

	}
	$j++;
	If ($j == 12)
	{
		$j = 1;
		table_header($th);
	}
}

end_table();
div_end();
end_form();
end_page();

