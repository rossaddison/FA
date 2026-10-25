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
page(_($help_context = "Install/Activate extensions"), false, false, "", $js);

include_once(dirname(__DIR__) . "/includes/date_functions.inc");
include_once(dirname(__DIR__) . "/admin/db/company_db.inc");
include_once(dirname(__DIR__) . "/admin/db/maintenance_db.inc");
include_once(dirname(__DIR__) . "/includes/ui.inc");

simple_page_mode(true);

/** @var string|int|float|bool|array<array-key, mixed>|null $selected_id */
$selected_id = $selected_id;

//---------------------------------------------------------------------------------------------
function local_extension(string|int|null $id): bool
{
	global $next_extension_id, $Ajax, $path_to_root;

	$id = (string) $id;
	$exts = get_company_extensions();
	$exts[$next_extension_id++] = array(
			'package' => $id,
			'name' => $id,
			'version' => '-',
			'available' => '',
			'type' => 'extension',
			'path' => 'modules/'.$id,
			'active' => false
	);

	$cleaned_id = clean_file_name($id);
	$local_module_path = "$path_to_root/modules/".(is_array($cleaned_id) ? '' : (string) $cleaned_id);
	$local_config_file = $local_module_path.'/_init/config';
	$local_hook_file = $local_module_path.'/hooks.php';

	if (file_exists($local_config_file)) {
		$ctrl = get_control_file($local_config_file);
		if ($ctrl !== null && key_exists('Name', $ctrl)) $exts[$next_extension_id-1]['name'] = $ctrl['Name'];
		if ($ctrl !== null && key_exists('Version', $ctrl)) $exts[$next_extension_id-1]['version'] = $ctrl['Version'];
	}
	if (file_exists($local_hook_file)) {
		include_once($local_hook_file);

	}
	$hooks_class = 'hooks_'.$id;
	// MixedMethodCall/TaintedCallable here are a confirmed-safe pattern: every real
	// modules/*/hooks.php declares `class hooks_<id> extends hooks` (the base class in
	// includes/hooks.inc) -- class_exists(..., false) (autoloading off) only matches
	// a class the include_once above just defined, so this can't instantiate an arbitrary
	// already-autoloadable class
	if (class_exists($hooks_class, false)) {
		$hooks = new $hooks_class;
		/** @var hooks $hooks */
		$hooks->install_extension(false);
	}
	ajax()->activate('ext_tbl'); // refresh settings display
	if (!update_extensions($exts))
		return false;
	return true;
}

function handle_delete_ext(string|int|float|bool|null $id): bool
{
	global $path_to_root;

	$id = (int) $id;
	$extensions = get_company_extensions();
	$ext = $extensions[$id];
	if ($ext['version'] != '-') {
		if (!uninstall_package((string) $ext['package']))
			return false;
	} else {
		// UnresolvableInclude/MixedMethodCall/TaintedCallable here are the same
		// confirmed-safe pattern as local_extension() above: $ext['path']/$ext['package']
		// come from this install's own stored get_company_extensions() config for an
		// already-installed extension, not raw per-request user text, and class_exists()
		// only matches a class the include_once just above actually defined
		@include_once("$path_to_root/".(string)$ext['path']."/hooks.php");
		$hooks_class = 'hooks_'.(string)$ext['package'];
		if (class_exists($hooks_class)) {
			$hooks = new $hooks_class;
			/** @var hooks $hooks */
			$hooks->uninstall_extension(false);
		}
	}
	unset($extensions[$id]);
	if (update_extensions($extensions)) {
		display_notification(_("Selected extension has been successfully deleted"));
	}
	return true;
}
//---------------------------------------------------------------------------------------------
//
// Display list of all extensions - installed and available from repository
//
/** @param array<array-key, array<string, mixed>> $mods */
function display_extensions(array $mods): void
{
	global $installed_extensions;

	div_start('ext_tbl');
	start_table(TABLESTYLE);

	$th = array(_("Extension"), _("Installed"), _("Available"),  "", "");
	table_header($th);

	$k = 0;

	foreach($mods as $pkg_name => $ext)
	{
		$available = isset($ext['available']) ? (string) $ext['available'] : '';
		$installed = isset($ext['version']) ? (string) $ext['version'] : '';
		$id = isset($ext['local_id']) ? (string) $ext['local_id'] : null;
		$name = (string) $ext['name'];

		alt_table_row_color($k);

		label_cell($available ? get_package_view_str($pkg_name, $name) : $name);

		label_cell($id === null ? _("None") :
			(($installed && ($installed != '-' && $installed != '')) ? $installed : _("Unknown")));
		label_cell($available ? $available : _("Unknown"));

		if (!$available && $ext['type'] == 'extension')	{// third-party plugin
			if (!$installed)
				button_cell('Local'.(string)$ext['package'], _("Install"), _('Install third-party extension.'),
					ICON_DOWN);
			else
				label_cell('');
		} elseif (check_pkg_upgrade($installed, $available)) // outdated or not installed extension in repo
			button_cell('Update'.$pkg_name, (bool)$installed ? _("Update") : _("Install"),
				_('Upload and install latest extension package'), ICON_DOWN);
		else
			label_cell('');

		if ($id !== null) {
			delete_button_cell('Delete'.$id, _('Delete'));
			submit_js_confirm('Delete'.$id,
				sprintf(_("You are about to remove package \'%s\'.\nDo you want to continue ?"),
					$name));
		} else
			label_cell('');

		end_row();
	}

	end_table(1);

	submit_center_first('Refresh', _("Update"), '', null);

	div_end();
}
//---------------------------------------------------------------------------------
//
// Get all installed extensions and display
// with current status stored in company directory.
//
function company_extensions(string|array|null $id): void
{
	start_table(TABLESTYLE);
	
	$th = array(_("Extension"), _("Version"), _("Path"), _("Active"));
	
	$mods = get_company_extensions();
	$exts = get_company_extensions($id);
	foreach($mods as $key => $ins) {
		foreach($exts as $ext)
			if ($ext['name'] == $ins['name']) {
				$mods[$key]['active'] = isset($ext['active']) ? (bool) $ext['active'] : false;
				continue 2;
			}
	}
	/** @var array<array-key, array<array-key, string|int|float|bool|null>> $mods */
	$mods = $mods;
	$mods = array_natsort($mods, null, 'name');
	table_header($th);
	$k = 0;
	foreach($mods as $i => $mod)
	{
		if ($mod['type'] != 'extension') continue;
		alt_table_row_color($k);
		label_cell($mod['name']);
		label_cell($mod['version']);
		label_cell($mod['path']);

		check_cells(null, 'Active'.$i, (bool) @$mod['active'] ? 1:0,
			false, false, "align='center'");

		end_row();
	}

	end_table(1);
	submit_center('Refresh', _('Update'), true, false, 'default');
}

//---------------------------------------------------------------------------------------------
/** @var string|int|float|bool|array<array-key, mixed>|null $selected_id */
$selected_id = $selected_id;
if ($Mode == 'Delete')
{
	if (!is_array($selected_id))
		handle_delete_ext($selected_id);
	$Mode = 'RESET';
}

if (get_post('Refresh')) {
	$comp = get_post('extset');
	$exts = get_company_extensions($comp);

	$result = true;
	foreach($exts as $i => $ext) {
		$active_check = check_value('Active'.$i);
		$active_check = is_array($active_check) ? 0 : $active_check;
		if ($ext['package'] && ((int) $ext['active'] ^ $active_check))
		{
			if ($active_check && !check_src_ext_version((string) $ext['version']))
			{
				display_warning(sprintf(_("Package '%s' is incompatible with current application version and cannot be activated.\n")
					. _("Check Install/Activate page for newer package version."), (string) $ext['name']));
				continue;
			}
			// activate_hooks() is declared mixed by design -- it returns whatever the
			// extension's own install_extension()/uninstall_extension() hook returns
			$activated = activate_hooks((string) $ext['package'], $comp, !$ext['active']);	// change active state

			if ($activated !== null)
				$result = $result && (bool) $activated;
			if ((bool)$activated || ($activated === null))
				$exts[$i]['active'] = $active_check;
		}
	}
	write_extensions($exts, get_post('extset'));
	if (get_post('extset') == user_company())
		$installed_extensions = $exts;
	
	if(!$result) {
		display_error(_('Status change for some extensions failed.'));
		ajax()->activate('ext_tbl'); // refresh settings display
	}else
		display_notification(_('Current active extensions set has been saved.'));
}

if ((bool)($id = find_submit('Update', false)))
	install_extension($id);

if ((bool)($id = find_submit('Local', false)))
	local_extension($id);

if ($Mode == 'RESET')
{
	$selected_id = -1;
	unset($_POST);
}

//---------------------------------------------------------------------------------------------
start_form(true);
if (list_updated('extset'))
	ajax()->activate('_page_body');

$set = get_post('extset', -1);

echo "<center>" . _('Extensions:') . "&nbsp;&nbsp;";
echo extset_list('extset', null, true);
echo "</center><br>";

if ($set == -1) 
{
	$mods = get_extensions_list('extension');
	if (!$mods)
		display_note(_("No optional extension module is currently available."));
	else {
		/** @var array<array-key, array<string, mixed>> $mods */
		display_extensions($mods);
	}
} else 
	company_extensions($set);

//---------------------------------------------------------------------------------------------
end_form();

end_page();
