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
	The markup follows _html_php_conventions.php: everything through Yiisoft\Html\Html (H), one space of indent per
	nesting level, the same //N on every open and close tag. Text that FA holds as HTML (titles, tab labels, the
	links menu_link() builds) is echoed as it is; encoding it here would encode it twice.
*/
declare(strict_types=1);

use Yiisoft\Html\Html as H;

// the markup below needs yiisoft/html, which composer installs
require_once dirname(__DIR__, 2) . '/vendor/autoload.php';

	class renderer
	{
		function get_icon($category)
		{
			global  $path_to_root, $SysPrefs;

			if ($SysPrefs->show_menu_category_icons)
				$img = $category == '' ? 'right.gif' : $category.'.png';
			else
				$img = 'right.gif';
			return H::img("$path_to_root/themes/".user_theme()."/images/$img")
				->class('fa-vam')
				->addAttributes(['border' => '0'])
				->render().'&nbsp;&nbsp;';
		}

		function wa_header()
		{
			page(_($help_context = "Main Menu"), false, true);
		}

		function wa_footer()
		{
			end_page(false, true);
		}

		/**
		 * One icon + label link of the status bar, with the gap FA puts after each.
		 *
		 * @param array<string, string> $attributes
		 */
		private function status_link(array $attributes, string $image, string $alt, string $label): string
		{
			global $path_to_root;

			return H::openTag('a', $attributes)
				.H::img("$path_to_root/themes/".user_theme()."/images/$image")->class('fa-icon14')->alt($alt)->render()
				.'&nbsp;&nbsp;'.$label
				.H::closeTag('a')
				.'&nbsp;&nbsp;&nbsp;';
		}

		function menu_header($title, $no_menu, $is_index)
		{
			if (yiiLayoutEnabled())
			{	// prototype: the header comes from views/theme/default/menuHeader.php
				echo yiiMenuHeader((string)$title, (bool)$no_menu, (bool)$is_index);
				return;
			}
			global $path_to_root, $SysPrefs, $db_connections;

			// ─── CSS shortcut variables ──────────────────────────────────────────────
			$calloutMain    = ['class' => 'callout_main', 'border' => '0', 'cellpadding' => '0', 'cellspacing' => '0'];
			$mainPage       = ['class' => 'main_page', 'border' => '0', 'cellpadding' => '0', 'cellspacing' => '0'];
			$fullWidth      = ['width' => '100%', 'border' => '0', 'cellpadding' => '0', 'cellspacing' => '0'];
			$tabsTable      = ['cellpadding' => '0', 'cellspacing' => '0', 'width' => '100%'];
			$logoutBarRight = ['class' => 'logoutBarRight'];
			$indicator      = "$path_to_root/themes/".user_theme(). "/images/ajax-loader.gif";
			$ajaxMark       = ['id' => 'ajaxmark', 'align' => 'center', 'class' => 'fa-hidden', 'alt' => 'ajaxmark'];

			echo H::openTag('table', $calloutMain); //0
			 echo H::openTag('tr'); //1
			  echo H::openTag('td', ['colspan' => '2', 'rowspan' => '2']); //2
			   echo H::openTag('table', $mainPage); //3
			    echo H::openTag('tr'); //4
			     echo H::openTag('td'); //5
			      echo H::openTag('table', $fullWidth); //6
			       echo H::openTag('tr'); //7
			        echo H::openTag('td', ['class' => 'quick_menu']); // tabs //8
			if (!$no_menu)
			{
				$applications = $_SESSION['App']->applications;
				$local_path_to_root = $path_to_root;
				$sel_app = $_SESSION['sel_app'];
				echo H::openTag('table', $tabsTable); //9
				 echo H::openTag('tr'); //10
				  echo H::openTag('td'); //11
				   echo H::openTag('div', ['class' => 'tabs']); //12
				foreach($applications as $app)
				{
					if ($_SESSION["wa_current_user"]->check_application_access($app))
					{
						$acc = access_string($app->name);
						$attributes = [
							'class' => $sel_app == $app->id ? 'selected' : 'menu_tab',
							'href' => "$local_path_to_root/index.php?application=".$app->id,
						];
						// access_string() answers " accesskey='X'"
						if (preg_match("/accesskey='(.)'/", (string)$acc[1], $key) === 1)
							$attributes['accesskey'] = $key[1];
						echo H::openTag('a', $attributes); //13
						 echo $acc[0];
						echo H::closeTag('a'); //13
					}
				}
				   echo H::closeTag('div'); //12
				  echo H::closeTag('td'); //11
				 echo H::closeTag('tr'); //10
				echo H::closeTag('table'); //9
				// top status bar
				echo H::openTag('table', ['class' => 'logoutBar']); //9
				 echo H::openTag('tr'); //10
				  echo H::openTag('td', ['class' => 'headingtext3']); //11
				   echo $db_connections[user_company()]["name"] . " | " . $_SERVER['SERVER_NAME'] . " | " . $_SESSION["wa_current_user"]->name;
				  echo H::closeTag('td'); //11
				  echo H::openTag('td', $logoutBarRight); //11
				   echo H::img($indicator)->addAttributes($ajaxMark)->render();
				  echo H::closeTag('td'); //11
				  echo H::openTag('td', $logoutBarRight); //11
				   echo $this->status_link(['href' => "$path_to_root/admin/dashboard.php?sel_app=$sel_app"], 'report.png', _('Dashboard'), _("Dashboard"));
				   echo $this->status_link(['class' => 'shortcut', 'href' => "$path_to_root/admin/display_prefs.php?"], 'preferences.gif', _('Preferences'), _("Preferences"));
				   echo $this->status_link(['class' => 'shortcut', 'href' => "$path_to_root/admin/change_current_user_password.php?selected_id=" . $_SESSION["wa_current_user"]->username], 'lock.gif', _('Change Password'), _("Change password"));
				if ($SysPrefs->help_base_url != null)
				{
					// help_url() answers an HTML-encoded URL; H encodes it again
					echo $this->status_link(['target' => '_blank', 'data-fa-action' => 'open-window', 'href' => html_entity_decode(help_url())], 'help.gif', _('Help'), _("Help"));
				}
				   echo $this->status_link(['class' => 'shortcut', 'href' => "$local_path_to_root/access/logout.php?"], 'login.gif', _('Logout'), _("Logout"));
				  echo H::closeTag('td'); //11
				 echo H::closeTag('tr'); //10
				 echo H::openTag('tr'); //10
				  echo H::openTag('td', ['colspan' => '3']); //11
				  echo H::closeTag('td'); //11
				 echo H::closeTag('tr'); //10
				echo H::closeTag('table'); //9
			}
			        echo H::closeTag('td'); //8
			       echo H::closeTag('tr'); //7
			      echo H::closeTag('table'); //6

			if ($no_menu)
			{	// ajax indicator for installer and popups
				echo H::openTag('center'); //6
				 echo H::openTag('table', ['class' => 'tablestyle_noborder']); //7
				  echo H::openTag('tr'); //8
				   echo H::openTag('td'); //9
				    echo H::img($indicator)->addAttributes($ajaxMark)->render();
				   echo H::closeTag('td'); //9
				  echo H::closeTag('tr'); //8
				 echo H::closeTag('table'); //7
				echo H::closeTag('center'); //6
			} elseif ($title && !$is_index)
			{
				echo H::openTag('center'); //6
				 echo H::openTag('table', ['id' => 'title']); //7
				  echo H::openTag('tr'); //8
				   echo H::openTag('td', ['width' => '100%', 'class' => 'titletext']); //9
				    echo $title;
				   echo H::closeTag('td'); //9
				   echo H::openTag('td', ['align' => 'right']); //9
				    if (user_hints())
				     echo H::tag('span', '', ['id' => 'hints']);
				   echo H::closeTag('td'); //9
				  echo H::closeTag('tr'); //8
				 echo H::closeTag('table'); //7
				echo H::closeTag('center'); //6
			}
		}

		function menu_footer($no_menu, $is_index)
		{
			if (yiiLayoutEnabled())
			{	// prototype: the footer comes from views/theme/default/menuFooter.php
				echo yiiMenuFooter((bool)$no_menu, (bool)$is_index);
				return;
			}
			global $version, $path_to_root, $Pagehelp, $Ajax, $SysPrefs;

			include_once($path_to_root . "/includes/date_functions.inc");

			// ─── CSS shortcut variables ──────────────────────────────────────────────
			$footerCell = ['class' => 'footer', 'align' => 'center'];
			$powerLink  = ['target' => '_blank', 'href' => $SysPrefs->power_url, 'tabindex' => '-1'];

			// the tables menu_header() left open, from the innermost
			echo H::closeTag('td'); //5
			echo H::closeTag('tr'); //4
			echo H::closeTag('table'); //3
			if ($no_menu == false) // bottom status line
			{
				echo H::openTag('table', ['class' => $is_index ? 'bottomBar' : 'bottomBar2']); //3
				 echo H::openTag('tr'); //4
				if (isset($_SESSION['wa_current_user'])) {
					$phelp = implode('; ', $Pagehelp);
					  echo H::openTag('td', ['class' => 'bottomBarCell']); //5
					   echo Today() . " | " . Now();
					  echo H::closeTag('td'); //5
					$Ajax->addUpdate(true, 'hotkeyshelp', $phelp);
					  echo H::openTag('td', ['id' => 'hotkeyshelp']); //5
					   echo $phelp;
					  echo H::closeTag('td'); //5
				}
				 echo H::closeTag('tr'); //4
				echo H::closeTag('table'); //3
			}
			 echo H::closeTag('td'); //2
			echo H::closeTag('tr'); //1
			echo H::closeTag('table'); //0
			if ($no_menu == false)
			{
				echo H::openTag('table', ['align' => 'center', 'id' => 'footer']); //0
				 echo H::openTag('tr'); //1
				  echo H::openTag('td', $footerCell); //2
				   echo H::openTag('a', $powerLink); //3
				    echo H::openTag('font', ['color' => '#ffffff']); //4
				     echo $SysPrefs->app_title." $version - " . _("Theme:") . " " . user_theme() . " - ".show_users_online();
				    echo H::closeTag('font'); //4
				   echo H::closeTag('a'); //3
				  echo H::closeTag('td'); //2
				 echo H::closeTag('tr'); //1
				 echo H::openTag('tr'); //1
				  echo H::openTag('td', $footerCell); //2
				   echo H::openTag('a', $powerLink); //3
				    echo H::openTag('font', ['color' => '#ffff00']); //4
				     echo $SysPrefs->power_by;
				    echo H::closeTag('font'); //4
				   echo H::closeTag('a'); //3
				  echo H::closeTag('td'); //2
				 echo H::closeTag('tr'); //1
				if ($SysPrefs->allow_demo_mode)
				{
					echo H::openTag('tr'); //1
					echo H::closeTag('tr'); //1
				}
				echo H::closeTag('table'); //0
				echo H::br();
				echo H::br();
			}
		}

		/**
		 * The links of one column of a module: an icon and the link (or the greyed label), one per line.
		 */
		private function display_functions(array $functions): void
		{
			foreach ($functions as $appfunction)
			{
				$img = $this->get_icon($appfunction->category);
				if ($appfunction->label == "")
				{
					echo '&nbsp;';
					echo H::br();
				}
				elseif ($_SESSION["wa_current_user"]->can_access_page($appfunction->access))
				{
					echo $img.menu_link($appfunction->link, $appfunction->label);
					echo H::br();
				}
				elseif (!$_SESSION["wa_current_user"]->hide_inaccessible_menu_items())
				{
					echo $img;
					echo H::openTag('span', ['class' => 'inactive']); //0
					 echo access_string($appfunction->label, true);
					echo H::closeTag('span'); //0
					echo H::br();
				}
			}
		}

		function display_applications(&$waapp)
		{
			global $path_to_root;

			$selected_app = $waapp->get_selected_application();
			if (!$_SESSION["wa_current_user"]->check_application_access($selected_app))
				return;

			if (method_exists($selected_app, 'render_index'))
			{
				$selected_app->render_index();
				return;
			}

			$menuGroup = ['class' => 'menu_group'];
			$menuItems = ['class' => 'menu_group_items'];

			echo H::openTag('table', ['width' => '100%', 'cellpadding' => '0', 'cellspacing' => '0']); //0
			foreach ($selected_app->modules as $module)
			{
				if (!$_SESSION["wa_current_user"]->check_module_access($module))
					continue;
				 echo H::openTag('tr'); //1
				  echo H::openTag('td', ['valign' => 'top', 'class' => 'menu_group']); //2
				   echo H::openTag('table', ['border' => '0', 'width' => '100%']); //3
				    echo H::openTag('tr'); //4
				     echo H::openTag('td', $menuGroup); //5
				      echo $module->name;
				     echo H::closeTag('td'); //5
				    echo H::closeTag('tr'); //4
				    echo H::openTag('tr'); //4
				     echo H::openTag('td', $menuItems); //5
				      $this->display_functions($module->lappfunctions);
				     echo H::closeTag('td'); //5
				if (sizeof($module->rappfunctions) > 0)
				{
					 echo H::openTag('td', ['width' => '50%', 'class' => 'menu_group_items']); //5
					  $this->display_functions($module->rappfunctions);
					 echo H::closeTag('td'); //5
				}
				    echo H::closeTag('tr'); //4
				   echo H::closeTag('table'); //3
				  echo H::closeTag('td'); //2
				 echo H::closeTag('tr'); //1
			}
			echo H::closeTag('table'); //0
		}
	}
