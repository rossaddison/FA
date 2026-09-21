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
/*
	Prototype: the default theme's page header and footer (tab bar, status bar, title, bottom line) as view templates.

	themes/default/renderer.php calls these when the Yii page shell is on. They collect what the old echo code read
	from globals and the session, and hand it to views/theme/default/menu_header.php and menu_footer.php.
*/

/**
 * @return string the header markup
 */
function yii_menu_header(string $title, bool $noMenu, bool $isIndex): string
{
	global $path_to_root, $db_connections;

	$images = $path_to_root . '/themes/' . (string)user_theme() . '/images';
	$data = array(
		'noMenu' => $noMenu,
		'isIndex' => $isIndex,
		'title' => $title,
		'showTitle' => (bool)$title && !$isIndex,
		'root' => $path_to_root,
		'images' => $images,
		'indicator' => $images . '/ajax-loader.gif',
		'hints' => (bool)user_hints(),
		'tabs' => array(),
		'selectedApp' => '',
		'status' => '',
		'username' => '',
		'helpUrl' => null,
		't' => array(
			'dashboard' => _('Dashboard'),
			'preferences' => _('Preferences'),
			'changePassword' => _('Change password'),
			'changePasswordAlt' => _('Change Password'),
			'help' => _('Help'),
			'logout' => _('Logout'),
		),
	);
	if (!$noMenu) {
		/** @var front_accounting $app */
		$app = $_SESSION['App'];
		$user = session_obj('wa_current_user');
		$selected = (string)$_SESSION['sel_app'];
		$tabs = array();
		foreach ($app->applications as $application) {
			if ($user->check_application_access($application)) {
				$access = access_string($application->name);
				$tabs[] = array(
					'class' => $selected == $application->id ? 'selected' : 'menu_tab',
					'href' => $path_to_root . '/index.php?application=' . (string)$application->id,
					'access' => (string)$access[1],
					'label' => (string)$access[0],
				);
			}
		}
		$data['tabs'] = $tabs;
		$data['selectedApp'] = $selected;
		$data['status'] = $db_connections[(int)user_company()]['name'] . ' | ' . ($_SERVER['SERVER_NAME'] ?? '') . ' | ' . $user->name;
		$data['username'] = $user->username;
		$data['helpUrl'] = sysprefs()->help_base_url != null ? help_url() : null;
	}
	return yii_view()->render('theme/default/menu_header', $data);
}

/**
 * @return string the footer markup
 */
function yii_menu_footer(bool $noMenu, bool $isIndex): string
{
	global $version, $Pagehelp;

	include_once(dirname(__DIR__, 2) . '/includes/date_functions.inc');

	$loggedIn = isset($_SESSION['wa_current_user']);
	$pageHelp = '';
	$now = '';
	if (!$noMenu && $loggedIn) {
		$pageHelp = implode('; ', $Pagehelp);
		$now = Today() . ' | ' . Now();
		ajax()->addUpdate(true, 'hotkeyshelp', $pageHelp);
	}
	return yii_view()->render('theme/default/menu_footer', array(
		'noMenu' => $noMenu,
		'isIndex' => $isIndex,
		'loggedIn' => $loggedIn,
		'now' => $now,
		'pageHelp' => $pageHelp,
		'powerUrl' => sysprefs()->power_url,
		'appTitle' => sysprefs()->app_title,
		'version' => $version,
		'themeLine' => _('Theme:') . ' ' . (string)user_theme() . ' - ' . show_users_online(),
		'powerBy' => sysprefs()->power_by,
		'demo' => (bool)sysprefs()->allow_demo_mode,
	));
}
