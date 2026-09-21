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
	from globals and the session, and hand it to views/theme/default/menuHeader.php and menuFooter.php.
	The templates follow _html_php_conventions.php: everything through Yiisoft\Html\Html, one space of indent per level.
*/

/**
 * One of the links in the status bar, in the form the template loops over.
 *
 * @param array<string, string> $attributes the attributes of the <a>
 * @return array{attributes: array<string, string>, icon: string, iconClass: string, alt: string, label: string}
 * @psalm-pure
 */
function yiiMenuLink(array $attributes, string $icon, string $iconClass, string $alt, string $label): array
{
	return array('attributes' => $attributes, 'icon' => $icon, 'iconClass' => $iconClass, 'alt' => $alt, 'label' => $label);
}

/**
 * The header markup: the tab bar, the status bar and the page title. It leaves the page's wrapping table open; the
 * footer closes it.
 */
function yiiMenuHeader(string $title, bool $noMenu, bool $isIndex): string
{
	global $path_to_root, $db_connections;

	$images = $path_to_root . '/themes/' . (string)user_theme() . '/images';
	$data = array(
		'noMenu' => $noMenu,
		'showTitle' => (bool)$title && !$isIndex,
		'title' => $title,
		'indicator' => $images . '/ajax-loader.gif',
		'images' => $images,
		'hints' => (bool)user_hints(),
		'tabs' => array(),
		'status' => '',
		'links' => array(),
	);
	if (!$noMenu) {
		/** @var front_accounting $app */
		$app = $_SESSION['App'];
		$user = session_obj('wa_current_user');
		$selected = (string)$_SESSION['sel_app'];
		$icon = 'fa-icon14'; // css/fa-inline.css
		$tabs = array();
		foreach ($app->applications as $application) {
			if ($user->check_application_access($application)) {
				$access = access_string($application->name);
				$attributes = array(
					'class' => $selected == $application->id ? 'selected' : 'menu_tab',
					'href' => $path_to_root . '/index.php?application=' . (string)$application->id,
				);
				// access_string() answers " accesskey='X'"
				if (preg_match("/accesskey='(.)'/", (string)$access[1], $key) === 1) {
					$attributes['accesskey'] = $key[1];
				}
				$tabs[] = array('attributes' => $attributes, 'label' => (string)$access[0]);
			}
		}
		$links = array(
			yiiMenuLink(array('href' => $path_to_root . '/admin/dashboard.php?sel_app=' . $selected), 'report.png', $icon, _('Dashboard'), _('Dashboard')),
			yiiMenuLink(array('class' => 'shortcut', 'href' => $path_to_root . '/admin/display_prefs.php?'), 'preferences.gif', $icon, _('Preferences'), _('Preferences')),
			yiiMenuLink(array('class' => 'shortcut', 'href' => $path_to_root . '/admin/change_current_user_password.php?selected_id=' . $user->username), 'lock.gif', $icon, _('Change Password'), _('Change password')),
		);
		if (sysprefs()->help_base_url != null) {
			// help_url() answers an HTML-encoded URL; the template encodes it again
			$links[] = yiiMenuLink(array(
				'target' => '_blank',
				'data-fa-action' => 'open-window',
				'href' => html_entity_decode(help_url()),
			), 'help.gif', $icon, _('Help'), _('Help'));
		}
		$links[] = yiiMenuLink(array('class' => 'shortcut', 'href' => $path_to_root . '/access/logout.php?'), 'login.gif', $icon, _('Logout'), _('Logout'));
		$data['tabs'] = $tabs;
		$data['links'] = $links;
		$data['status'] = $db_connections[(int)user_company()]['name'] . ' | ' . ($_SERVER['SERVER_NAME'] ?? '') . ' | ' . $user->name;
	}
	return yiiView()->render('theme/default/menuHeader', $data);
}

/**
 * The footer markup: it closes the wrapping table the header left open, then the bottom status line and the footer block.
 */
function yiiMenuFooter(bool $noMenu, bool $isIndex): string
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
	return yiiView()->render('theme/default/menuFooter', array(
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
