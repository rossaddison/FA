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
$page_security = 'SA_CHGPASSWD';
/** @var string $path_to_root */
$path_to_root="..";
include_once($path_to_root . "/includes/session.inc");

page(_($help_context = "Change password"));

include_once($path_to_root . "/includes/date_functions.inc");
include_once($path_to_root . "/includes/ui.inc");

include_once($path_to_root . "/admin/db/users_db.inc");

function can_process(): bool
{

	$Auth_Result = hook_authenticate(session_obj('wa_current_user')->username, $_POST['cur_password']);

	if (!isset($Auth_Result))	// if not used external login: standard method
		$Auth_Result = get_user_auth(session_obj('wa_current_user')->username, md5($_POST['cur_password']));

	if (!(bool)$Auth_Result)
   	{
  		display_error( _("Invalid password entered."));
		set_focus('cur_password');
   		return false;
   	}
	
   	if (strlen($_POST['password']) < 4)
   	{
  		display_error( _("The password entered must be at least 4 characters long."));
		set_focus('password');
   		return false;
   	}

   	if (strstr($_POST['password'], session_obj('wa_current_user')->username) != false)
   	{
   		display_error( _("The password cannot contain the user login."));
		set_focus('password');
   		return false;
   	}

   	if ($_POST['password'] != $_POST['passwordConfirm'])
   	{
   		display_error( _("The passwords entered are not the same."));
		set_focus('password');
   		return false;
   	}

	return true;
}

if (isset($_POST['UPDATE_ITEM']) && check_csrf_token())
{

	if (can_process())
	{
		if (sysprefs()->allow_demo_mode) {
		    display_warning(_("Password cannot be changed in demo mode."));
		} else {
			update_user_password(session_obj('wa_current_user')->user, 
				session_obj('wa_current_user')->username,
				md5($_POST['password']));
		    display_notification(_("Your password has been updated."));
		}
		ajax()->activate('_page_body');
	}
}

start_form();

start_table(TABLESTYLE);

$myrow = row_or_empty(get_user(session_obj('wa_current_user')->user));

label_row(_("User login:"), $myrow['user_id']);

$_POST['cur_password'] = "";
$_POST['password'] = "";
$_POST['passwordConfirm'] = "";

password_row(_("Current Password:"), 'cur_password', post_scalar('cur_password'));
password_row(_("New Password:"), 'password', post_scalar('password'));
password_row(_("Repeat New Password:"), 'passwordConfirm', post_scalar('passwordConfirm'));

table_section_title(_("Enter your new password in the fields."));

end_table(1);

submit_center( 'UPDATE_ITEM', _('Change password'), true, '',  'default');
end_form();
end_page();
