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
$page_security = 'SA_CREATELANGUAGE';
/** @var string $path_to_root */
$path_to_root="..";
include_once(dirname(__DIR__) . "/includes/session.inc");

/** @var array<int, array<string, string|bool>> $installed_languages */
$installed_languages = $installed_languages;

include_once(dirname(__DIR__) . "/includes/packages.inc");
include_once(dirname(__DIR__) . "/admin/db/maintenance_db.inc");
include_once(dirname(__DIR__) . "/includes/ui.inc");

$js = "";
if (sysprefs()->use_popup_windows)
	$js = get_js_open_window(900, 500);

page(_($help_context = "Install/Update Languages"), false, false, "", $js);

simple_page_mode(true);

//---------------------------------------------------------------------------------------------
//
// Display all packages - both already installed and available from repository
//
function display_languages(): void
{
	global $installed_languages, $dflt_lang, $GetText;
	
	$th = array(_("Language"), _("Name"), _("Encoding"), _("Right To Left"),
		_("Installed"), _("Available"), _("Default"), "", "");
	$currlang = session_obj('language')->code;

	div_start('lang_tbl');
	start_form();
	//
	// select/display system locales support for sites using native gettext
	//
	if (function_exists('gettext'))
	{
		if (check_value('DisplayAll'))
			 array_insert($th, 7, _("Supported"));
		start_table();
		check_row(_('Display also languages not supported by server locales'), 'DisplayAll', null, true);
		end_table();
	}

	start_table(TABLESTYLE);
	table_header($th);

	$k = 0;

	// get list of all (available and installed) langauges
	// $available/$installed/$id/$support below are genuinely mixed by design:
	// get_languages_list()'s own @return docblock types this as PkgListItem, a
	// @psalm-type alias defined in includes/packages.inc, but that alias isn't
	// resolvable from this file without a class to @psalm-import-type from --
	// tried re-declaring the same shape inline here instead (confirmed via
	// rescan: zero effect, reverted), each field is guarded with is_scalar()/
	// cast immediately below before any use
	$langs = get_languages_list();
	foreach ($langs as $pkg_name => $lng)
	{
		$lang = (string) ($lng['code'] ?? '');
		if ($lang === 'C') // skip default locale (aka no translation)
			continue;

		$lang_name = (string) ($lng['name'] ?? '');
		$charset = (string) ($lng['encoding'] ?? '');
		$rtl = ($lng['rtl'] ?? null) == 'yes' || ($lng['rtl'] ?? null) === true;
		$available = $lng['available'] ?? null;
		$installed = $lng['version'] ?? null;
		$id = $lng['local_id'] ?? null;

		if ($lang == $currlang)
			start_row("class='stockmankobg'");
		else
			alt_table_row_color($k);

		$support = $GetText->check_support($lang, $charset);

		if (function_exists('gettext') && !$support && !get_post('DisplayAll') && $lang != 'C') continue;

		label_cell($lang);
		label_cell((bool)$available ? get_package_view_str($lang, $lang_name) : $lang_name);
		label_cell($charset);
		label_cell($rtl ? _("Yes") : _("No"));

		label_cell($id === null ? _("None") :
			(((bool)$available && (bool)$installed) ? (string) $installed : _("Unknown")));

		label_cell((bool)$available ? (string) $available : _("None"));

		label_cell($id === null ? '' :
			radio(null, 'CurDflt', is_scalar($id) ? $id : null, $dflt_lang == $lang, true),
			"align='center'");

		if (function_exists('gettext') && check_value('DisplayAll'))
			label_cell($support ? _("Yes") :_("No"));

		if (!(bool)$available && ($lang != 'C'))	// manually installed language
			button_cell('Edit'.(string)$id, _("Edit"), _('Edit non standard language configuration'),
				ICON_EDIT);
		elseif (check_pkg_upgrade(is_scalar($installed) ? $installed : null, is_scalar($available) ? $available : null)) // outdated or not installed language in repo
			button_cell('Update'.(string)$pkg_name, (bool)$installed ? _("Update") : _("Install"),
				_('Upload and install latest language package'), ICON_DOWN);
		else
			label_cell('');

		if (($id !== null) && ($lang != $currlang) && ($lang != 'C')) {
			delete_button_cell('Delete'.(string)$id, _('Delete'));
			submit_js_confirm('Delete'.(string)$id,
				sprintf(_("You are about to remove language \'%s\'.\nDo you want to continue ?"),
					$lang_name));
		} else
			label_cell('');
		end_row();
	}
	end_table();
	display_note(_("The marked language is the current language which cannot be deleted."), 0, 0, "class='currentfg'");
	br();
	submit_center_first('Refresh', _("Update default"), '', null);

	submit_center_last('Add', _("Add new language manually"), '', false);

	end_form();
	div_end();
}
//---------------------------------------------------------------------------------------------
// Non standard (manually entered) languages support.
//
#[\NoDiscard]
function check_data(): bool
{
	global $installed_languages;

	if (get_post('code') == '' || get_post('name') == '' || get_post('encoding') == '') {
		display_error(_("Language name, code nor encoding cannot be empty"));
		return false;
	}
	/** @var array-key|null $id */
	$id = array_search_key(post_scalar('code'), $installed_languages, 'code');
	if ($id !== null && ($installed_languages[(int)$id]['package'] ?? null) != null) {
		display_error(_('Standard package for this language is already installed. If you want to install this language manually, uninstall standard language package first.'));
		return false;
	}
	return true;
}

function handle_submit_inst_lang(string|int|float|bool|array|null $id): bool
{
	global $path_to_root, $installed_languages, $dflt_lang, $Mode;
	$id = is_scalar($id) ? (int) $id : 0;

	$code_raw = clean_file_name(post_scalar('code'));
	$code = is_string($code_raw) ? $code_raw : '';

	if ((bool)post_scalar('dflt')) {
			$dflt_lang = $code;
	}

	$installed_languages[$id]['code'] = $code;
	$installed_languages[$id]['name'] = (string) post_scalar('name');
	$installed_languages[$id]['path'] = 'lang/' . $code;
	$installed_languages[$id]['encoding'] = (string) post_scalar('encoding');
	$installed_languages[$id]['rtl'] = (bool)post_scalar('rtl');
	$installed_languages[$id]['package'] = '';
	$installed_languages[$id]['version'] = '';
	if (!write_lang())
		return false;
	$directory = (string) $path_to_root . "/lang/" . $code;
	if (!file_exists($directory))
	{
		mkdir($directory);
		mkdir($directory . "/LC_MESSAGES");
	}
	if (is_uploaded_file($_FILES['uploadfile']['tmp_name']))
	{
		$file1 = $_FILES['uploadfile']['tmp_name'];
		$fcode = preg_replace('/[^a-zA-Z_]/', '', $code);
		$file2 = $directory . "/LC_MESSAGES/$fcode.po";
		if (file_exists($file2))
			unlink($file2);
		move_uploaded_file($file1, $file2);
	}
	if (is_uploaded_file($_FILES['uploadfile2']['tmp_name']))
	{
		$file1 = $_FILES['uploadfile2']['tmp_name'];
		$fcode = preg_replace('/[^a-zA-Z_]/', '', $code);
		$file2 = $directory . "/LC_MESSAGES/$fcode.mo";
		if (file_exists($file2))
			unlink($file2);
		move_uploaded_file($file1, $file2);
	}
	return true;
}

//---------------------------------------------------------------------------------------------

function display_language_edit(string|int|float|bool|null $selected_id): void
{
	global $installed_languages, $dflt_lang;
	$selected_id = is_scalar($selected_id) ? (int) $selected_id : -1;

	if ($selected_id == -1)
		$n = count($installed_languages);
	else
		$n = $selected_id;

	start_form(true);

	start_table(TABLESTYLE2);

	if ($selected_id != -1)
	{
		$lang = $installed_languages[$n];
		$_POST['code'] = $lang['code'];
		$_POST['name']  = $lang['name'];
		$_POST['encoding']  = $lang['encoding'];
		if (isset($lang['rtl']) && $lang['rtl'] === true)
			$_POST['rtl']  = $lang['rtl'];
		else
			$_POST['rtl'] = false;
		$_POST['dflt'] = $dflt_lang == $lang['code'];
		hidden('selected_id', $selected_id);
	}
	text_row_ex(_("Language Code"), 'code', 20);
	text_row_ex(_("Language Name"), 'name', 20);
	text_row_ex(_("Encoding"), 'encoding', 20);

	yesno_list_row(_("Right To Left"), 'rtl', null, "", "", false);
	yesno_list_row(_("Default Language"), 'dflt', null, "", "", false);

	file_row(_("Language File") . " (PO)", 'uploadfile');
	file_row(_("Language File") . " (MO)", 'uploadfile2');

	end_table(0);
	display_note(_("Select your language files from your local harddisk."), 0, 1);

	submit_add_or_update_center(false, '', 'both');

	end_form();
}

function handle_delete(string|int|float|bool|null $id): void
{
	global  $path_to_root, $installed_languages, $dflt_lang;
	$id = is_scalar($id) ? (int) $id : 0;

	$lang = $installed_languages[$id]['code'];
	if (!empty($installed_languages[$id]['package']))
		if (!uninstall_package($installed_languages[$id]['package']))
			return;
			
	if ($lang == $dflt_lang ) { 
		// on delete set default to current.
		$dflt_lang = session_obj('language')->code;
	}
	
	unset($installed_languages[$id]);
	$installed_languages = array_values($installed_languages);

	if (!write_lang())
		return;

	$dirname = "$path_to_root/lang/$lang";
	if ((bool)$lang && is_dir($dirname)) {	// remove nonstadard language dir
		flush_dir($dirname, true);
		rmdir($dirname);
	}
}

//---------------------------------------------------------------------------------------------

$lang_id = is_scalar($selected_id) ? $selected_id : null;

if ($Mode == 'Delete')
	handle_delete($lang_id);

if ($Mode == 'ADD_ITEM' || $Mode == 'UPDATE_ITEM')
	if (check_data() && handle_submit_inst_lang($lang_id))
		$Mode = 'RESET';

if ((bool)($id = find_submit('Update', false)))
	install_language($id);

if (get_post('_CurDflt_update') || (get_post('Refresh') && get_post('CurDflt', -1) != -1)) {
	$curdflt_raw = get_post('CurDflt', 0);
	$curdflt = is_scalar($curdflt_raw) ? (int) $curdflt_raw : 0;
	// $installed_languages is pinned to array<int, array<string, string|bool>> at the top
	// of this file, but handle_submit_inst_lang() (called earlier in this same request)
	// assigns into it through `global $installed_languages`, which widens Psalm's inferred
	// type for every later read of the same global in this file -- the same global-variable
	// type-narrowing-doesn't-survive-a-later-write pattern already documented in
	// doc/PSALM_MIGRATION.md, not a real bug
	$new_lang = $installed_languages[$curdflt]['code'] ?? null;
	if ($new_lang != $dflt_lang) {
		$dflt_lang = (string) $new_lang;
		write_lang();
		ajax()->activate('lang_tbl');
	}
}
if (get_post('_DisplayAll_update')) {
	ajax()->activate('lang_tbl');
}
	
//---------------------------------------------------------------------------------------------

if (isset($_GET['popup']) || get_post('Add') || $Mode == 'Edit' || $Mode == 'ADD_ITEM' || $Mode == 'UPDATE_ITEM') {
	display_language_edit($lang_id);
} else
	display_languages();

//---------------------------------------------------------------------------------------------
end_page();

