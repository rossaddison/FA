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
$page_security = 'SA_CRMCATEGORY';
/** @var string $path_to_root */
$path_to_root = '..';
include(dirname(__DIR__) . "/includes/session.inc");
include_once(dirname(__DIR__) . "/includes/db/crm_contacts_db.inc");

page(_($help_context = "Contact Categories"));

include(dirname(__DIR__) . "/includes/ui.inc");

simple_page_mode(true);

if ($Mode=='ADD_ITEM' || $Mode=='UPDATE_ITEM') 
{

	$input_error = 0;

	$description_check = post_scalar('description');
	if (strlen(is_string($description_check) ? $description_check : '') == 0)
	{
		$input_error = 1;
		display_error(_("Category description cannot be empty."));
		set_focus('description');
	}

	if ($input_error != 1)
	{
		$type_post = get_post('type');
		$subtype_post = get_post('subtype');
		$name_post = get_post('name');
		$description_post = get_post('description');
		$type_str = is_string($type_post) ? $type_post : null;
		$subtype_str = is_string($subtype_post) ? $subtype_post : null;
		$name_str = is_string($name_post) ? $name_post : null;
		$description_str = is_string($description_post) ? $description_post : null;
    	if ($selected_id != -1)
    	{
    		update_crm_category(is_array($selected_id) ? '' : (string) $selected_id, $type_str, $subtype_str,
    			$name_str, $description_str);
			$note = _('Selected contact category has been updated');
    	}
    	else
    	{
    		add_crm_category($type_str, $subtype_str, $name_str,
    			$description_str);
			$note = _('New contact category has been added');
    	}

		display_notification($note);
		$Mode = 'RESET';
	}
}

if ($Mode == 'Delete')
{
	$cancel_delete = 0;

	if ((bool)is_crm_category_used(is_array($selected_id) ? '' : (string) $selected_id))
	{
		$cancel_delete = 1;
		display_error(_("Cannot delete this category because there are contacts related to it."));
	}
	if ($cancel_delete == 0)
	{
		delete_crm_category(is_array($selected_id) ? '' : (string) $selected_id);

		display_notification(_('Category has been deleted'));
	}
	$Mode = 'RESET';
}

if ($Mode == 'RESET')
{
	$selected_id = -1;
	$sav = get_post('show_inactive');
	unset($_POST);
	$_POST['show_inactive'] = $sav;
}

//-------------------------------------------------------------------------------------------------

$result = get_crm_categories(check_value('show_inactive'));

start_form();
start_table(TABLESTYLE, "width='70%'");

$th = array(_("Category Type"), _("Category Subtype"), _("Short Name"), _("Description"),  "", "&nbsp;");
inactive_control_column($th);

table_header($th);
$k = 0; 

while ($myrow = db_fetch($result)) 
{
	
	alt_table_row_color($k);
		
	label_cell($myrow["type"]);
	label_cell($myrow["action"]);
	label_cell($myrow["name"]);
	label_cell($myrow["description"]);
	
	inactive_control_cell($myrow["id"], $myrow["inactive"], 'crm_categories', 'id');

 	edit_button_cell("Edit".(string)$myrow["id"], _("Edit"));
 	if ((bool)$myrow["system"])
		label_cell('');
	else
		delete_button_cell("Delete".(string)$myrow["id"], _("Delete"));
	end_row();
}
	
inactive_control_row($th);
end_table(1);

//-------------------------------------------------------------------------------------------------
start_table(TABLESTYLE2);

$edit_row = array();
if ($selected_id != -1)
{
 	if ($Mode == 'Edit') {
		//editing an existing area
		$edit_row = row_or_empty(get_crm_category(is_array($selected_id) ? '' : (string) $selected_id));

		$_POST['name']  = $edit_row["name"];
		$_POST['type']  = $edit_row["type"];
		$_POST['subtype']  = $edit_row["action"];
		$_POST['description']  = $edit_row["description"];
	}
	hidden("selected_id", is_array($selected_id) ? '' : (string) $selected_id);
}

if ($Mode == 'Edit' && ($edit_row['system'] ?? false)) {
	label_row(_("Contact Category Type:"), post_scalar('type'));
	label_row(_("Contact Category Subtype:"), post_scalar('subtype'));
} else {
//	crm_category_type_list_row(_("Contact Category Type:"), 'type', null, _('Other'));
	text_row_ex(_("Contact Category Type:"), 'type', 30); 
	text_row_ex(_("Contact Category Subtype:"), 'subtype', 30); 
}

text_row_ex(_("Category Short Name:"), 'name', 30); 
textarea_row(_("Category Description:"), 'description', null, 60, 4);

end_table(1);

submit_add_or_update_center($selected_id == -1, '', 'both');

end_form();

end_page();
