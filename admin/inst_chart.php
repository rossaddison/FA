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
$page_security = 'SA_CREATEMODULES';
/** @var string $path_to_root */
$path_to_root="..";
include_once(dirname(__DIR__) . "/includes/session.inc");
include_once(dirname(__DIR__) . "/includes/packages.inc");

$js = "";
if (sysprefs()->use_popup_windows) {
	$js = get_js_open_window(900, 500);
}
page(_($help_context = "Install Charts of Accounts"), false, false, '', $js);

include_once(dirname(__DIR__) . "/includes/date_functions.inc");
include_once(dirname(__DIR__) . "/admin/db/company_db.inc");
include_once(dirname(__DIR__) . "/admin/db/maintenance_db.inc");
include_once(dirname(__DIR__) . "/includes/ui.inc");

//---------------------------------------------------------------------------------------------

if ((bool)($id = find_submit('Delete', false)))
{
	$extensions = get_company_extensions();
	$id = (int) $id;
	$package = $extensions[$id]['package'] ?? null;
	$package = is_scalar($package) ? $package : null;
	if (($extensions[$id]['type']=='chart') && $package !== null && $package !== '' && uninstall_package($package)) {
		unset($extensions[$id]);
		if (update_extensions($extensions)) {
			display_notification(_("Selected chart has been successfully deleted"));
			meta_forward($_SERVER['PHP_SELF'] ?? '');
		}
	}
}

if ((bool)($id = find_submit('Update', false)))
	install_extension($id);

//---------------------------------------------------------------------------------------------

/** @psalm-pure */
function sortByOption(array $a, array $b): int {
    return strcmp((string) $a['name'], (string) $b['name']);
}

start_form(true);

	div_start('ext_tbl');

	$mods = get_charts_list();

	if (!$mods)
		display_note(_("No optional chart of accounts is currently available."));
	else
	{
        uasort($mods, 'sortByOption');

		$th = array(_("Chart"),  _("Installed"), _("Available"), _("Encoding"), "", "");
		$k = 0;

		start_table(TABLESTYLE);
		table_header($th);
		foreach($mods as $pkg_name => $ext)
		{
			// $available/$installed/$id/$encoding below are genuinely mixed by design:
			// get_charts_list()'s own @return docblock types this as PkgListItem, same
			// reasoning as admin/inst_lang.php's identical get_languages_list() loop --
			// each field is guarded with is_scalar()/cast immediately below before any use
			$ext_name = (string) ($ext['name'] ?? '');
			$available = $ext['available'] ?? null;
			$installed = $ext['version'] ?? null;
			$id = $ext['local_id'] ?? null;
			$encoding = $ext['encoding'] ?? null;

			alt_table_row_color($k);

			label_cell((bool)$available ? get_package_view_str($pkg_name, $ext_name) : $ext_name);

			label_cell($id === null ? _("None") :
				(((bool)$available && (bool)$installed) ? (string) $installed : _("Unknown")));
			label_cell((bool)$available ? (string) $available : _("None"));
			label_cell((bool)$encoding ? (string) $encoding : _("Unknown"));

			if ((bool)$available && check_pkg_upgrade(is_scalar($installed) ? $installed : null, is_scalar($available) ? $available : null)) // outdated or not installed theme in repo
				button_cell('Update'.$pkg_name, (bool)$installed ? _("Update") : _("Install"),
					_('Upload and install latest extension package'), ICON_DOWN);
			else
				label_cell('');

			if ($id !== null) {
				delete_button_cell('Delete'.(string)$id, _('Delete'));
				submit_js_confirm('Delete'.(string)$id,
					sprintf(_("You are about to remove package \'%s\'.\nDo you want to continue ?"),
						$ext_name));
			} else
				label_cell('');

			end_row();
		}
	end_table(1);
	}
	div_end();

//---------------------------------------------------------------------------------------------
end_form();

end_page();
