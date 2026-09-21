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
    The markup follows _html_php_conventions.php: everything through
    Yiisoft\Html\Html (H), one space of indent per nesting level, the same //N
    on every open and close tag. Text that FA holds as HTML (titles, tab
    labels, the links menu_link() builds) is echoed as it is; encoding it here
    would encode it twice. Lines are at most 85 characters.
*/
declare(strict_types=1);

use Yiisoft\Html\Html as H;

// the markup below needs yiisoft/html, which composer installs
require_once dirname(__DIR__, 2) . '/vendor/autoload.php';

class renderer
{
 function get_icon($category)
 {
  global $path_to_root, $SysPrefs;

  if ($SysPrefs->show_menu_category_icons) {
   $img = $category == '' ? 'right.gif' : $category . '.png';
  } else {
   $img = 'right.gif';
  }
  return H::img("$path_to_root/themes/" . user_theme() . "/images/$img")
   ->class('fa-vam')
   ->addAttributes(['border' => '0'])
   ->render() . '&nbsp;&nbsp;';
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
  * One icon + label link of the status bar, with the gap FA puts after it.
  *
  * @param array<string, string> $attributes
  */
 private function status_link(
  array $attributes,
  string $image,
  string $alt,
  string $label
 ): string {
  global $path_to_root;

  $src = "$path_to_root/themes/" . user_theme() . "/images/$image";
  return H::openTag('a', $attributes)
   . H::img($src)->class('fa-icon14')->alt($alt)->render()
   . '&nbsp;&nbsp;' . $label
   . H::closeTag('a')
   . '&nbsp;&nbsp;&nbsp;';
 }

 /**
  * The status bar: company | server | user on the left, the links on the right.
  */
 private function status_bar(string $indicator, string $sel_app): void
 {
  global $path_to_root, $SysPrefs, $db_connections;

  $user = $_SESSION["wa_current_user"];
  $logoutBarRight = ['class' => 'logoutBarRight'];
  $ajaxMark = [
   'id' => 'ajaxmark',
   'align' => 'center',
   'class' => 'fa-hidden',
   'alt' => 'ajaxmark',
  ];
  $dashboard = [
   'href' => "$path_to_root/admin/dashboard.php?sel_app=$sel_app",
  ];
  $prefs = [
   'class' => 'shortcut',
   'href' => "$path_to_root/admin/display_prefs.php?",
  ];
  $password = [
   'class' => 'shortcut',
   'href' => "$path_to_root/admin/change_current_user_password.php"
    . "?selected_id=" . $user->username,
  ];
  $logout = [
   'class' => 'shortcut',
   'href' => "$path_to_root/access/logout.php?",
  ];

  echo H::openTag('table', ['class' => 'logoutBar']); //0
   echo H::openTag('tr'); //1
    echo H::openTag('td', ['class' => 'headingtext3']); //2
     echo $db_connections[user_company()]["name"] . " | "
      . $_SERVER['SERVER_NAME'] . " | " . $user->name;
    echo H::closeTag('td'); //2
    echo H::openTag('td', $logoutBarRight); //2
     echo H::img($indicator)->addAttributes($ajaxMark)->render();
    echo H::closeTag('td'); //2
    echo H::openTag('td', $logoutBarRight); //2
     echo $this->status_link(
      $dashboard,
      'report.png',
      _('Dashboard'),
      _("Dashboard")
     );
     echo $this->status_link(
      $prefs,
      'preferences.gif',
      _('Preferences'),
      _("Preferences")
     );
     echo $this->status_link(
      $password,
      'lock.gif',
      _('Change Password'),
      _("Change password")
     );
  if ($SysPrefs->help_base_url != null) {
   // help_url() answers an HTML-encoded URL; H encodes it again
   $help = [
    'target' => '_blank',
    'data-fa-action' => 'open-window',
    'href' => html_entity_decode(help_url()),
   ];
   echo $this->status_link($help, 'help.gif', _('Help'), _("Help"));
  }
     echo $this->status_link(
      $logout,
      'login.gif',
      _('Logout'),
      _("Logout")
     );
    echo H::closeTag('td'); //2
   echo H::closeTag('tr'); //1
   echo H::openTag('tr'); //1
    echo H::openTag('td', ['colspan' => '3']); //2
    echo H::closeTag('td'); //2
   echo H::closeTag('tr'); //1
  echo H::closeTag('table'); //0
 }

 /**
  * The tabs, one per application the user may open.
  */
 private function tabs(string $sel_app): void
 {
  global $path_to_root;

  echo H::openTag('table', [
   'cellpadding' => '0',
   'cellspacing' => '0',
   'width' => '100%',
  ]); //0
   echo H::openTag('tr'); //1
    echo H::openTag('td'); //2
     echo H::openTag('div', ['class' => 'tabs']); //3
  foreach ($_SESSION['App']->applications as $app) {
   if (!$_SESSION["wa_current_user"]->check_application_access($app)) {
    continue;
   }
   $acc = access_string($app->name);
   $attributes = [
    'class' => $sel_app == $app->id ? 'selected' : 'menu_tab',
    'href' => "$path_to_root/index.php?application=" . $app->id,
   ];
   // access_string() answers " accesskey='X'"
   if (preg_match("/accesskey='(.)'/", (string)$acc[1], $key) === 1) {
    $attributes['accesskey'] = $key[1];
   }
   echo H::openTag('a', $attributes); //4
    echo $acc[0];
   echo H::closeTag('a'); //4
  }
     echo H::closeTag('div'); //3
    echo H::closeTag('td'); //2
   echo H::closeTag('tr'); //1
  echo H::closeTag('table'); //0
 }

 function menu_header($title, $no_menu, $is_index)
 {
  if (yiiLayoutEnabled()) {
   // prototype: the header comes from views/theme/default/menuHeader.php
   echo yiiMenuHeader((string)$title, (bool)$no_menu, (bool)$is_index);
   return;
  }
  global $path_to_root;

  // --- CSS shortcut variables ---------------------------------------
  $calloutMain = [
   'class' => 'callout_main',
   'border' => '0',
   'cellpadding' => '0',
   'cellspacing' => '0',
  ];
  $mainPage = [
   'class' => 'main_page',
   'border' => '0',
   'cellpadding' => '0',
   'cellspacing' => '0',
  ];
  $fullWidth = [
   'width' => '100%',
   'border' => '0',
   'cellpadding' => '0',
   'cellspacing' => '0',
  ];
  $indicator = "$path_to_root/themes/" . user_theme()
   . "/images/ajax-loader.gif";
  $ajaxMark = [
   'id' => 'ajaxmark',
   'align' => 'center',
   'class' => 'fa-hidden',
   'alt' => 'ajaxmark',
  ];
  $titleText = ['width' => '100%', 'class' => 'titletext'];

  echo H::openTag('table', $calloutMain); //0
   echo H::openTag('tr'); //1
    echo H::openTag('td', ['colspan' => '2', 'rowspan' => '2']); //2
     echo H::openTag('table', $mainPage); //3
      echo H::openTag('tr'); //4
       echo H::openTag('td'); //5
        echo H::openTag('table', $fullWidth); //6
         echo H::openTag('tr'); //7
          echo H::openTag('td', ['class' => 'quick_menu']); // tabs //8
  if (!$no_menu) {
   $this->tabs((string)$_SESSION['sel_app']);
   $this->status_bar($indicator, (string)$_SESSION['sel_app']);
  }
          echo H::closeTag('td'); //8
         echo H::closeTag('tr'); //7
        echo H::closeTag('table'); //6

  if ($no_menu) {
   // ajax indicator for installer and popups
   echo H::openTag('center'); //6
    echo H::openTag('table', ['class' => 'tablestyle_noborder']); //7
     echo H::openTag('tr'); //8
      echo H::openTag('td'); //9
       echo H::img($indicator)->addAttributes($ajaxMark)->render();
      echo H::closeTag('td'); //9
     echo H::closeTag('tr'); //8
    echo H::closeTag('table'); //7
   echo H::closeTag('center'); //6
  } elseif ($title && !$is_index) {
   echo H::openTag('center'); //6
    echo H::openTag('table', ['id' => 'title']); //7
     echo H::openTag('tr'); //8
      echo H::openTag('td', $titleText); //9
       echo $title;
      echo H::closeTag('td'); //9
      echo H::openTag('td', ['align' => 'right']); //9
   if (user_hints()) {
        echo H::tag('span', '', ['id' => 'hints']);
   }
      echo H::closeTag('td'); //9
     echo H::closeTag('tr'); //8
    echo H::closeTag('table'); //7
   echo H::closeTag('center'); //6
  }
 }

 function menu_footer($no_menu, $is_index)
 {
  if (yiiLayoutEnabled()) {
   // prototype: the footer comes from views/theme/default/menuFooter.php
   echo yiiMenuFooter((bool)$no_menu, (bool)$is_index);
   return;
  }
  global $version, $path_to_root, $Pagehelp, $Ajax, $SysPrefs;

  include_once($path_to_root . "/includes/date_functions.inc");

  // --- CSS shortcut variables ---------------------------------------
  $footerCell = ['class' => 'footer', 'align' => 'center'];
  $bottomBar = $is_index ? 'bottomBar' : 'bottomBar2';
  $powerLink = [
   'target' => '_blank',
   'href' => $SysPrefs->power_url,
   'tabindex' => '-1',
  ];

  // the tables menu_header() left open, from the innermost
  echo H::closeTag('td'); //5
  echo H::closeTag('tr'); //4
  echo H::closeTag('table'); //3
  if ($no_menu == false) { // bottom status line
   echo H::openTag('table', ['class' => $bottomBar]); //3
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
  if ($no_menu == false) {
   echo H::openTag('table', ['align' => 'center', 'id' => 'footer']); //0
    echo H::openTag('tr'); //1
     echo H::openTag('td', $footerCell); //2
      echo H::openTag('a', $powerLink); //3
       echo H::openTag('font', ['color' => '#ffffff']); //4
        echo $SysPrefs->app_title . " $version - " . _("Theme:") . " "
         . user_theme() . " - " . show_users_online();
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
   if ($SysPrefs->allow_demo_mode) {
    echo H::openTag('tr'); //1
    echo H::closeTag('tr'); //1
   }
   echo H::closeTag('table'); //0
   echo H::br();
   echo H::br();
  }
 }

 /**
  * The links of one column of a module: an icon and the link (or the greyed
  * label), one per line.
  */
 private function display_functions(array $functions): void
 {
  $user = $_SESSION["wa_current_user"];
  foreach ($functions as $appfunction) {
   $img = $this->get_icon($appfunction->category);
   if ($appfunction->label == "") {
    echo '&nbsp;';
    echo H::br();
   } elseif ($user->can_access_page($appfunction->access)) {
    echo $img . menu_link($appfunction->link, $appfunction->label);
    echo H::br();
   } elseif (!$user->hide_inaccessible_menu_items()) {
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
  $selected_app = $waapp->get_selected_application();
  if (!$_SESSION["wa_current_user"]->check_application_access($selected_app)) {
   return;
  }

  if (method_exists($selected_app, 'render_index')) {
   $selected_app->render_index();
   return;
  }

  $menuGroup = ['class' => 'menu_group'];
  $menuItems = ['class' => 'menu_group_items'];
  $menuItems50 = ['width' => '50%', 'class' => 'menu_group_items'];
  $menuCell = ['valign' => 'top', 'class' => 'menu_group'];

  echo H::openTag('table', [
   'width' => '100%',
   'cellpadding' => '0',
   'cellspacing' => '0',
  ]); //0
  foreach ($selected_app->modules as $module) {
   if (!$_SESSION["wa_current_user"]->check_module_access($module)) {
    continue;
   }
    echo H::openTag('tr'); //1
     echo H::openTag('td', $menuCell); //2
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
   if (sizeof($module->rappfunctions) > 0) {
     echo H::openTag('td', $menuItems50); //5
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
