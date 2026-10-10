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
$page_security = 'SA_OPEN';
/** @var string $path_to_root */
$path_to_root="..";

if (file_exists($path_to_root.'/config_db.php'))
	header("Location: $path_to_root/index.php");

include(dirname(__DIR__) . "/install/isession.inc");

page(_("FrontAccouting ERP Installation Wizard"), true, false, "", '', false, 'stylesheet.css');

include(dirname(__DIR__) . "/includes/ui.inc");
include(dirname(__DIR__) . "/includes/system_tests.inc");
include(dirname(__DIR__) . "/admin/db/maintenance_db.inc");
include(dirname(__DIR__) . "/includes/packages.inc");
if (file_exists($path_to_root . "/installed_extensions.php"))
	include(dirname(__DIR__) . "/installed_extensions.php");
//-------------------------------------------------------------------------------------------------

function subpage_title(?string $txt): void 
{
	global $path_to_root;
	
	echo '<center><img src="'.$path_to_root.'/themes/default/images/logo_frontaccounting.png" width="250" height="50" alt="Logo" >
		</center>';

	$page_raw = @$_POST['Page'];
	$page = is_scalar($page_raw) && (bool)$page_raw ? $page_raw : 1;

	display_heading(
		$page == 6 ? $txt :
			_("FrontAccouting ERP Installation Wizard").'<br>'
			. sprintf(_('Step %d: %s'), (int) $page, (string) $txt));
	br();
}

function display_coas(): void
{
	start_table(TABLESTYLE);
	$th = array(_("Chart of accounts"), _("Encoding"), _("Description"), _("Install"));
	table_header($th);

	$k = 0;
	/**
	 * @var array<array-key, array{
	 *     package?: string|int|float|bool|null,
	 *     name?: string|int|float|bool|null,
	 *     version?: string|int|float|bool|null,
	 *     available?: string|int|float|bool|null,
	 *     Descr?: string|int|float|bool|array<array-key, string>|null,
	 *     encoding?: string|int|float|bool|null,
	 *     local_id?: array-key,
	 *     ...<array-key, mixed>
	 * }> $charts
	 */
	$charts = get_charts_list();

	foreach($charts as $pkg_name => $coa)
	{
		$installed = $coa['version'] ?? null;
		$descr = $coa['Descr'] ?? null;
		$coa_name = $coa['name'] ?? null;
		$coa_encoding = $coa['encoding'] ?? null;

		alt_table_row_color($k);
		label_cell(is_scalar($coa_name) ? $coa_name : null);
		label_cell(is_scalar($coa_encoding) ? $coa_encoding : null);
		label_cell(is_array($descr) ? implode('<br>', $descr) :  $descr);
		label_cell((bool)$installed ?
			_("Installed") : checkbox(null, 'coas['.(string)($coa['package'] ?? '').']'), "align='center'");

		end_row();
	}
	end_table(1);
}

function display_langs(): void
{
	start_table(TABLESTYLE);
	$th = array(_("Language"), _("Encoding"), _("Description"), _("Install"));
	table_header($th);

	$k = 0;
	/**
	 * @var array<array-key, array{
	 *     package?: string|int|float|bool|null,
	 *     name?: string|int|float|bool|null,
	 *     version?: string|int|float|bool|null,
	 *     available?: string|int|float|bool|null,
	 *     Descr?: string|int|float|bool|array<array-key, string>|null,
	 *     encoding?: string|int|float|bool|null,
	 *     local_id?: array-key,
	 *     ...<array-key, mixed>
	 * }> $langs
	 */
	$langs = get_languages_list();

	foreach($langs as $pkg_name => $lang)
	{
		$available = $lang['available'] ?? null;
		$installed = $lang['version'] ?? null;
		$descr = $lang['Descr'] ?? null;
		$lang_name = $lang['name'] ?? null;
		$lang_encoding = $lang['encoding'] ?? null;
		if (!(bool)$available) continue;

		alt_table_row_color($k);
		label_cell(is_scalar($lang_name) ? $lang_name : null);
		label_cell(is_scalar($lang_encoding) ? $lang_encoding : null);
		label_cell(is_array($descr) ? implode('<br>', $descr) :  $descr);
		label_cell((bool)$installed ?
			_("Installed") : checkbox(null, 'langs['.(string)($lang['package'] ?? '').']'), "align='center'");
		end_row();
	}
	end_table(1);
}

function instlang_list_row(?string $label, ?string $name, string|int|float|bool|null $value=null): void {

	global $inst_langs;
	/** @var array<string, array{name: string, code: string, encoding: string, rtl?: bool}> $inst_langs */

	$langs = array();
	foreach ($inst_langs as $n => $lang)
			$langs[$n] = $lang['name'];

	echo "<td>$label</td>\n" . "<td>\n" 
		.array_selector($name, $value, $langs, 
			array(
				'select_submit' => true,
				'async' => true
			)) . "</td>\n";
}

/** @return int|mysqli|false */
function install_connect_db() {

	global $db;

	/** @var array<string, mixed> $conn */
	$conn = $_SESSION['inst_set'];

	$db = db_create_db($conn);
	$result = $db;
	if (!(bool)$result) {
		display_error(_("Cannot connect to database. User or password is invalid or you have no permittions to create database."));
	} else {
		if (strncmp((string) db_get_version(), "5.6", 3) >= 0)
			db_query("SET sql_mode = ''");
	}
	return $result;
}

function do_install(): bool {

	global $path_to_root, $db_connections, $def_coy, $installed_extensions, $tb_pref_counter,
		$dflt_lang, $installed_languages;

	/** @var string $coa */
	$coa = $_SESSION['inst_set']['coa'];
	/** @var array<string, mixed> $inst_set */
	$inst_set = $_SESSION['inst_set'];
	if ((bool)install_connect_db() && (bool)db_import($path_to_root.'/sql/'.$coa, $inst_set)) {
		$con = $inst_set;
		$table_prefix = (string) $con['tbpref'];

		$def_coy = 0;
		$tb_pref_counter = 0;
		$db_connections = array (0=> array (
		 'name' => (string) $con['name'],
		 'host' => (string) $con['host'],
		 'port' => (string) $con['port'],
		 'dbname' => (string) $con['dbname'],
		 'collation' => (string) $con['collation'],
		 'tbpref' => $table_prefix,
		 'dbuser' => (string) $con['dbuser'],
		 'dbpassword' => (string) $con['dbpassword'],
		));

		/** @var current_user $wa_current_user */
		$wa_current_user = $_SESSION['wa_current_user'];
		$wa_current_user->cur_con = 0;

		update_company_prefs(array('coy_name'=>$con['name']));
		$admin = row_or_empty(get_user_by_login('admin'));
		$admin_id = $admin['id'];
		update_user_prefs(is_scalar($admin_id) ? $admin_id : null, array(
			'language' => $con['lang'],
			'password' => md5((string) $con['pass']),
			'user_id' => $con['admin']));

		if (!copy($path_to_root. "/config.default.php", $path_to_root. "/config.php")) {
			display_error(_("Cannot save system configuration file 'config.php'."));
			return false;
		}

		$err = write_config_db($table_prefix != "");

		if ($err == -1) {
			display_error(_("Cannot open 'config_db.php' configuration file."));
			return false;
		} else if ($err == -2) {
			display_error(_("Cannot write to the 'config_db.php' configuration file."));
			return false;
		} else if ($err == -3) {
			display_error(_("Configuration file 'config_db.php' is not writable. Change its permissions so it is, then re-run installation step."));
			return false;
		}
		// update default language
		if (file_exists($path_to_root . "/lang/installed_languages.inc"))
			include_once(dirname(__DIR__) . "/lang/installed_languages.inc");
		$dflt_lang = $_POST['lang'];
		write_lang();
		return true;
	}
	return false;
}

if (!isset($_SESSION['inst_set'])) { // default settings
	/** @var array<string, string> $default_inst_set */
	$default_inst_set = array(
		'host'=>'localhost',
		'port' => '', // 3306
		'dbuser' => 'root',
		'dbpassword' => '',
		'username' => 'admin',
		'tbpref' => '0_',
		'admin' => 'admin',
		'inst_lang' => 'C',
		'collation' => 'xx',
	);
	$_SESSION['inst_set'] = $default_inst_set;
}

if (!@$_POST['Tests'])
	$_POST['Page'] = 1; // set to start page

if (isset($_POST['back']) && (@$_POST['Page']>1)) {
	if ($_POST['Page'] == 5)
		$_POST['Page'] = 2;
	else
		$_POST['Page']--;
}
elseif (isset($_POST['continue'])) {
	$_POST['Page'] = 2;
}
elseif (isset($_POST['db_test'])) {
	if (get_post('host')=='') {
		display_error(_('Host name cannot be empty.'));
		set_focus('host');
	}
	elseif ($_POST['port'] != '' && !is_numeric($_POST['port'])) {
		display_error(_('Database port have to be numeric or empty.'));
		set_focus('port');
	}
	elseif ($_POST['dbuser']=='') {
		display_error(_('Database user name cannot be empty.'));
		set_focus('dbuser');
	}
	elseif ($_POST['dbname']=='') {
		display_error(_('Database name cannot be empty.'));
		set_focus('dbname');
	}
	else {
		/** @var language $lang */
		$lang = $_SESSION['language'];
		/** @var array<string, mixed> $inst_set */
		$inst_set = $_SESSION['inst_set'];
		$_SESSION['inst_set'] = array_merge($inst_set, array(
			'host' => $_POST['host'],
			'port' => $_POST['port'],
			'dbuser' => $_POST['dbuser'],
			'dbpassword' => @html_entity_decode(is_scalar($_POST['dbpassword']) ? (string) $_POST['dbpassword'] : '', ENT_QUOTES, $lang->encoding=='iso-8859-2' ? 'ISO-8859-1' : $lang->encoding),
			'dbname' => $_POST['dbname'],
			'tbpref' => $_POST['tbpref'] ? '0_' : '',
			'sel_langs' => check_value('sel_langs'),
			'sel_coas' => check_value('sel_coas'),
			'collation' => $_POST['collation'],
		));
		if ((bool)install_connect_db()) {
			$_POST['Page'] = check_value('sel_langs') ? 3 :
				(check_value('sel_coas') ? 4 : 5);
		}
	}
	if (!file_exists($path_to_root . "/lang/installed_languages.inc")) {
		$installed_languages = array (
			0 => array ('code' => 'C', 'name' => 'English', 'encoding' => 'iso-8859-1'));
			$dflt_lang = 'C';
			write_lang();
	}
}
elseif(get_post('install_langs')) 
{
	$ret = true;
	$post_langs = @$_POST['langs'];
	if (is_array($post_langs))
		foreach($post_langs as $package => $ok) {
			if (!install_language($package))
				$ret = false;
		}
	/** @var array<string, mixed> $inst_set */
	$inst_set = $_SESSION['inst_set'];
	if ($ret) {
		$_POST['Page'] = (bool)@$inst_set['sel_coas'] ? 4 : 5;
	}
}
elseif(get_post('install_coas'))
{
	$ret = true;
	$next_extension_id = 0;

	$post_coas = @$_POST['coas'];
	if (is_array($post_coas))
		foreach($post_coas as $package => $ok) {
			if (!install_extension($package))
				$ret = false;
		}
	if ($ret) {
		if (file_exists($path_to_root . '/installed_extensions.php'))
			include(dirname(__DIR__) . '/installed_extensions.php');
		$_POST['Page'] = 5;
	}
} elseif (isset($_POST['set_admin'])) {
	// check company settings
	if (get_post('name')=='') {
		display_error(_('Company name cannot be empty.'));
		set_focus('name');
	}
	elseif (get_post('admin')=='') {
		display_error(_('Company admin name cannot be empty.'));
		set_focus('admin');
	}
	elseif (get_post('pass')=='') {
		display_error(_('Company admin password cannot be empty.'));
		set_focus('pass');
	}
	elseif (get_post('pass')!=get_post('repass')) {
		display_error(_('Company admin passwords differ.'));
		unset($_POST['pass'],$_POST['repass']);
		set_focus('pass');
	}
	else {

		/** @var array<string, mixed> $inst_set */
		$inst_set = $_SESSION['inst_set'];
		$_SESSION['inst_set'] = array_merge($inst_set, array(
			'coa' => clean_file_name($_POST['coa']),
			'pass' => $_POST['pass'],
			'name' => $_POST['name'],
			'admin' => $_POST['admin'],
			'lang' => $_POST['lang']
		));
		if (do_install()) {
			$_POST['Page'] = 6;
		}
	}
}

if (list_updated('inst_lang')) {
	/** @var array<string, array{name: string, code: string, encoding: string, rtl?: bool}> $inst_langs */
	$inst_lang_sel = get_post('inst_lang');
	$_SESSION['inst_set']['inst_lang'] = $inst_lang_sel;
	ajax()->setEncoding($inst_langs[is_scalar($inst_lang_sel) ? $inst_lang_sel : '']['encoding']);
	ajax()->activate('welcome');
}

start_form();
	switch(@$_POST['Page']) {
		default:
		case '1':
			div_start('welcome');
			subpage_title(_('System Diagnostics'));
			start_table();
			/** @var array<string, mixed> $inst_set */
			$inst_set = $_SESSION['inst_set'];
			$cur_inst_lang = @$inst_set['inst_lang'];
			instlang_list_row(_("Select install wizard language:"), 'inst_lang',
				is_scalar($cur_inst_lang) ? $cur_inst_lang : null);
			end_table(1);
			$_POST['Tests'] = display_system_tests(true);
			br();
			if (@$_POST['Tests']) {
				display_notification(_('All application preliminary requirements seems to be correct. Please press Continue button below.'));
				submit_center('continue', _('Continue >>'));
			} else {
				display_error(_('Application cannot be installed. Please fix problems listed below in red, and press Refresh button.'));
				submit_center('refresh', _('Refresh'));
			}
			div_end();
			break;

		case '2':
			if (!isset($_POST['host'])) {
				/** @var array<string, mixed> $inst_set */
				$inst_set = $_SESSION['inst_set'];
				foreach($inst_set as $name => $val)
					$_POST[$name] = $val;
			}
			subpage_title(_('Database Server Settings'));
			start_table(TABLESTYLE);
			text_row_ex(_("Server Host:"), 'host', 30, 60);
			text_row_ex(_("Server Port:"), 'port', 30, 60);
			text_row_ex(_("Database Name:"), 'dbname', 30);
			text_row_ex(_("Database User:"), 'dbuser', 30);
			password_row(_("Database Password:"), 'dbpassword', '');
			collations_list_row(_("Database Collation:"), 'collation');
			yesno_list_row(_("Use '0_' Table Prefix:"), 'tbpref', 1, _('Yes'), _('No'), false);
			check_row(_("Install Additional Language Packs from FA Repository:"), 'sel_langs');
			check_row(_("Install Additional COAs from FA Repository:"), 'sel_coas');
			end_table(1);
			display_note(_("Use database user/password with permissions to create new database, or use proper credentials for previously created empty database."));
			display_note(_("Select collation you want to use. If you are unsure or you will use various languages, select unicode collation."));
			display_note(_("Use table prefix if you share selected database for more than one FA company using the same collation."));
			display_note(_("Do not select additional langs nor COAs if you have no working internet connection right now. You can install them later."));
			display_note(_("Set Only Port value if you cannot use the default port 3306."));
			submit_center_first('back', _('<< Back'));
			submit_center_last('db_test', _('Continue >>'));
			break;

		case '3': // select langauges
			subpage_title(_('User Interface Languages Selection'));
			display_langs();
			submit_center_first('back', _('<< Back'));
			submit_center_last('install_langs', _('Continue >>'));
			break;

		case '4': // select COA
			subpage_title(_('Charts of Accounts Selection'));
			display_coas();
			submit_center_first('back', _('<< Back'));
			submit_center_last('install_coas', _('Continue >>'));
			break;

		case '5':
			if (!isset($_POST['name'])) {
				/** @var array<string, mixed> $inst_set */
				$inst_set = $_SESSION['inst_set'];
				foreach($inst_set as $name => $val)
					$_POST[$name] = $val;
				set_focus('name');
			}
			if (!isset($installed_extensions)) {
				$installed_extensions = array();
				update_extensions($installed_extensions);
			}

			subpage_title(_('Company Settings'));
			start_table(TABLESTYLE);
			text_row_ex(_("Company Name:"), 'name', 30);
			text_row_ex(_("Admin Login:"), 'admin', 30);
			$post_pass = @$_POST['pass'];
			$post_repass = @$_POST['repass'];
			password_row(_("Admin Password:"), 'pass', is_scalar($post_pass) ? $post_pass : null);
			password_row(_("Reenter Password:"), 'repass', is_scalar($post_repass) ? $post_repass : null);
			coa_list_row(_("Select Chart of Accounts:"), 'coa');
			languages_list_row(_("Select Default Language:"), 'lang');
			end_table(1);
			submit_center_first('back', _('<< Back'));
			submit_center_last('set_admin', _('Install'), _('Start installation process'), 'default nonajax');
			break;

		case '6': // final screen
			subpage_title(_('FrontAccounting ERP has been installed successsfully.'));
			display_note(_('Please do not forget to remove install wizard folder.'));
			session_unset();
			session_destroy();
			hyperlink_no_params((string)$path_to_root.'/index.php', _('Click here to start.'));
			break;

	}

	hidden('Tests');
	hidden('Page');
end_form(1);

end_page(false, false, true);

