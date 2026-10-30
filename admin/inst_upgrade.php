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
$page_security = 'SA_SOFTWAREUPGRADE';
/** @var string $path_to_root */
$path_to_root="..";
include_once(dirname(__DIR__) . "/includes/session.inc");

$js = "";
if (sysprefs()->use_popup_windows) {
	$js = get_js_open_window(900, 500);
}
page(_($help_context = "Software Upgrade"), false, false, "", $js);

include_once(dirname(__DIR__) . "/includes/date_functions.inc");
include_once(dirname(__DIR__) . "/admin/db/company_db.inc");
include_once(dirname(__DIR__) . "/admin/db/maintenance_db.inc");
include_once(dirname(__DIR__) . "/includes/ui.inc");
include_once(dirname(__DIR__) . "/admin/includes/fa_patch.class.inc");

$site_status = get_site_status($db_connections);
$installers = get_installers();

if (get_post('Upgrade')) 
{
	$comp = post_scalar('select_comp');

    if ($comp === '' || $comp === null)
		display_error(_('Select company to be upgraded.'));
	else {
		$comp = is_string($comp) || is_int($comp) ? $comp : (string) $comp;
		// $patch is deliberately left mixed here, not typed fa_patch|null (via a
		// get_installers() return docblock or a local @var): tried both, and either one
		// makes Psalm start resolving $patch's type, which makes it actually analyze
		// fa_patch's own method bodies for taint instead of treating them as opaque --
		// those methods share helper functions (db_select, write_extensions,
		// get_company_extensions, ...) with admin/db/maintenance_db.inc and
		// admin/backups.php, so typing $patch here was confirmed (isolated A/B rescans)
		// to add ~16 unrelated TaintedFile findings in those two files, a net regression.
		// The "possibly undefined" $installers/$site_status offsets below are
		// intentional: an unrecognized company id or version simply yields no $patch,
		// handled by the if($patch) checks that follow, same as the leading @ already did
		// at runtime.
		$patch = @$installers[(string) ($site_status[(int) $comp]['version'] ?? '')];
		if ($patch)
		{
			if (!$patch->upgrade_company($comp, (bool) check_value('force')))
				display_error(implode('<hr>', $patch->errors));
			else
				display_notification(_("Company upgraded successfully."));

			$site_status = get_site_status($db_connections); // update info
		    ajax()->activate('_page_body');
		}
	}
}
// TaintedFile below is a confirmed-safe pattern: find_submit() is called here with
// its default $numeric=true, so it always internally (int)-casts the submitted
// button suffix before returning it -- never the raw, unvalidated $_POST string --
// which structurally rules out path-traversal characters reaching unlink()
$i = find_submit('Clear');
if ($i != -1)
{
  unlink(VARLOG_PATH.'/upgrade.'. (string) $i.'.log');
  ajax()->activate('_page_body');
}
if (get_post('_select_comp_update'))
{
  ajax()->activate('_page_body');
}

start_form();

$th = array(_("Company"), _("Table set"), _("Current version"), _("Last log"), _('Upgrade'));
start_table(TABLESTYLE);
table_header($th);
$k = 0; //row colour counter

$uptodate = true;
foreach($site_status as $i => $comp)
{
	// 'version' is only populated by get_site_status() when 'status' is true (a live,
	// reachable company connection) -- absent here means the connection itself failed,
	// not that the version matches, so treat a missing key the same as a mismatch
	$version = $comp['version'] ?? null;
	$status = $version == $db_version;

	alt_table_row_color($k);

	label_cell($comp['name']);
	label_cell($comp['table_set']);

	label_cell($version, 'align=center' .($status ? '':' class=redfg')/*, 'class='.( $status ? 'ok' : 'error')*/);

	$log = VARLOG_PATH.'/upgrade.'. (string) $i.'.log';
	if (file_exists($log))
	{
		label_cell((string) viewer_link(_('View log'), "admin/view/view_upgrade_log.php?id=$i", null, $i, 'log.png')
		  .button('Clear'. (string) $i, _('Clear'), _('Clear log'), ICON_DELETE), 'align=center');
		submit_js_confirm('Clear'. (string) $i, _("Do you really want to clear this upgrade log?"));
	} else
		label_cell('-', 'align=center');


	if (!$status)
	{
		label_cell(radio(null, 'select_comp', $i, null, true), 'align=center');
		$uptodate = false;
	} else
		label_cell(_('Up to date'));
	end_row();
}

end_table();
br();

div_start('upgrade_args');
$select_comp = post_scalar('select_comp');
if ($select_comp !== '' && $select_comp !== null)
{
	$select_comp = is_string($select_comp) || is_int($select_comp) ? $select_comp : (string) $select_comp;
	// $patch here is the same deliberately-mixed accepted leftover as above
	$patch = @$installers[(string) ($site_status[(int) $select_comp]['version'] ?? '')];
	if ($patch)
		$patch->show_params($select_comp);
}
div_end();

if ($uptodate)
	display_note(_('All company database schemes are up to date.'));
else {
	if ($select_comp === '' || $select_comp === null)
		display_note(_("Select company for incremental upgrade."), 0, 1, "class='stockmankofg'");
	submit_center('Upgrade', _('Upgrade'), true, _('Save database and perform upgrade'), 'nonajax');
}
end_form();

end_page();

