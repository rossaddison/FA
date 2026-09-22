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
// Author: Joe Hunt, 17/11/2015. Upgraded to release 2.4. 10/11/2015.
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
 function wa_get_apps($title, $applications, $sel_app)
 {
  foreach ($applications as $app) {
   foreach ($app->modules as $module) {
    $apps = [];
    foreach ($module->lappfunctions as $appfunction) {
     $apps[] = $appfunction;
    }
    foreach ($module->rappfunctions as $appfunction) {
     $apps[] = $appfunction;
    }
    $application = [];
    foreach ($apps as $application) {
     $url = explode('?', $application->link);
     $app_lnk = $url[0];
     $pos = strrpos($app_lnk, "/");
     if ($pos > 0) {
      $app_lnk = substr($app_lnk, $pos + 1);
      $lnk = $_SERVER['REQUEST_URI'];
      $url = explode('?', $lnk);
      $asset = false;
      if (isset($url[1])) {
       $asset = strstr($url[1], "FixedAsset");
      }
      $lnk = $url[0];
      $pos = strrpos($lnk, "/");
      $lnk = substr($lnk, $pos + 1);
      if ($app_lnk == $lnk) {
       $acc = access_string($app->name);
       $app_id = ($asset != false ? "assets" : $app->id);
       return [
        $acc[0],
        $module->name,
        $application->label,
        $app_id,
       ];
      }
     }
    }
   }
  }
  return ["", "", "", $sel_app];
 }

 function wa_header()
 {
  page(_($help_context = "Main Menu"), false, true);
 }

 function wa_footer()
 {
  end_page(false, true);
 }

 function shortcut($url, $label)
 {
  echo H::openTag('li'); //0
   echo menu_link($url, $label);
  echo H::closeTag('li'); //0
 }

 /**
  * One icon + label link of the header list.
  *
  * @param array<string, string> $attributes
  */
 private function header_item(
  array $attributes,
  string $image,
  string $alt,
  string $label
 ): void {
  global $path_to_root;

  $src = "$path_to_root/themes/" . user_theme() . "/images/$image";
  echo H::openTag('li'); //0
   echo H::openTag('a', $attributes); //1
    echo H::img($src)->class('fa-icon14-pb3')->alt($alt)->render();
    echo '&nbsp;&nbsp;' . $label;
   echo H::closeTag('a'); //1
  echo H::closeTag('li'); //0
 }

 /**
  * The links of the header: preferences, password, help, logout.
  */
 private function header_links(): void
 {
  global $path_to_root, $SysPrefs;

  $username = $_SESSION["wa_current_user"]->username;
  $this->header_item(
   ['href' => "$path_to_root/admin/display_prefs.php?"],
   'preferences.gif',
   _('Preferences'),
   _("Preferences")
  );
  $this->header_item(
   [
    'href' => "$path_to_root/admin/change_current_user_password.php"
     . "?selected_id=" . $username,
   ],
   'lock.gif',
   _('Change Password'),
   _("Change password")
  );
  if ($SysPrefs->help_base_url != null) {
   $this->header_item(
    [
     'target' => '_blank',
     'data-fa-action' => 'open-window',
     // help_url() answers an HTML-encoded URL; H encodes it again
     'href' => html_entity_decode(help_url()),
    ],
    'help.gif',
    _('Help'),
    _("Help")
   );
  }
  $this->header_item(
   ['href' => "$path_to_root/access/logout.php?"],
   'on_off.png',
   _('Logout'),
   _("Logout")
  );
 }

 /**
  * The list item of one module in the drop-down of an application, with the
  * links of the module below it.
  */
 private function menu_module($module, bool $alignRight): void
 {
  global $path_to_root;

  $user = $_SESSION["wa_current_user"];
  echo H::openTag('li', ['class' => 'has-sub']); //0
   echo H::openTag('a', ['href' => '#']); //1
    echo H::openTag('span'); //2
     echo $module->name;
    echo H::closeTag('span'); //2
   echo H::closeTag('a'); //1
  $functions = [];
  foreach ($module->lappfunctions as $appfunction) {
   $functions[] = $appfunction;
  }
  foreach ($module->rappfunctions as $appfunction) {
   $functions[] = $appfunction;
  }
  if (count($functions)) {
   echo H::openTag('ul', $alignRight ? ['class' => 'align_right'] : []); //1
   foreach ($functions as $application) {
    $lnk = access_string($application->label);
    if ($user->can_access_page($application->access)) {
     if ($application->label != "") {
      $href = "$path_to_root/$application->link";
      echo H::openTag('li'); //2
       echo H::openTag('a', ['href' => $href]); //3
        echo H::openTag('span'); //4
         echo $lnk[0];
        echo H::closeTag('span'); //4
       echo H::closeTag('a'); //3
      echo H::closeTag('li'); //2
     }
    } elseif (!$user->hide_inaccessible_menu_items()) {
     echo H::openTag('li'); //2
      echo H::openTag('a', ['href' => '#']); //3
       echo H::openTag('span'); //4
        echo H::openTag('font', ['color' => 'gray']); //5
         echo $lnk[0];
        echo H::closeTag('font'); //5
       echo H::closeTag('span'); //4
      echo H::closeTag('a'); //3
     echo H::closeTag('li'); //2
    }
   }
   echo H::closeTag('ul'); //1
  }
  echo H::closeTag('li'); //0
 }

 /**
  * The tab of one application, with its drop-down of modules.
  *
  */
 private function tab($app, string $active, int $i, bool $mobile): void
 {
  global $path_to_root;

  $acc = access_string($app->name);
  $n = count($app->modules);
  $class = trim(($active == $app->id ? "active" : "") . ($n ? " has-sub" : ""));
  $dashboard = "";
  $attributes = [];
  if ($mobile) {
   $attributes['href'] = '#';
   $dashboard = "$path_to_root/index.php?application=$app->id";
  } else {
   $attributes['href'] = "$path_to_root/index.php?application=$app->id";
   // access_string() answers " accesskey='X'"
   if (preg_match("/accesskey='(.)'/", (string)$acc[1], $key) === 1) {
    $attributes['accesskey'] = $key[1];
   }
  }
  echo H::openTag('li', $class !== '' ? ['class' => $class] : []); //0
   echo H::openTag('a', $attributes); //1
    echo H::openTag('span'); //2
     echo $acc[0];
    echo H::closeTag('span'); //2
   echo H::closeTag('a'); //1
  if ($n) {
   echo H::openTag('ul'); //1
   if ($dashboard != "") {
    echo H::openTag('li'); //2
     echo H::openTag('a', ['href' => $dashboard]); //3
      echo H::openTag('span'); //4
       echo H::openTag('font', ['color' => 'red']); //5
        echo _("Dashboard");
       echo H::closeTag('font'); //5
      echo H::closeTag('span'); //4
     echo H::closeTag('a'); //3
    echo H::closeTag('li'); //2
   }
   foreach ($app->modules as $module) {
    if ($_SESSION["wa_current_user"]->check_module_access($module)) {
     $this->menu_module($module, $i > 5);
    }
   }
   echo H::closeTag('ul'); //1
  }
  echo H::closeTag('li'); //0
 }

 function menu_header($title, $no_menu, $is_index)
 {
  global $path_to_root, $SysPrefs, $version;

  $sel_app = $_SESSION['sel_app'];
  echo H::openTag('div', ['class' => 'fa-main']); //0
  if (!$no_menu) {
   $applications = $_SESSION['App']->applications;
   $indicator = "$path_to_root/themes/" . user_theme()
    . "/images/ajax-loader.gif";
   $ajaxMark = [
    'id' => 'ajaxmark',
    'align' => 'center',
    'class' => 'fa-hidden',
   ];
    echo H::openTag('div', ['id' => 'header']); //1
     echo H::openTag('ul'); //2
   $this->header_links();
     echo H::closeTag('ul'); //2
   add_access_extensions();
     echo H::openTag('h1'); //2
      echo "$SysPrefs->power_by $version";
      echo H::openTag('span', ['class' => 'fa-pl300']); //3
       echo H::img($indicator)->addAttributes($ajaxMark)->render();
      echo H::closeTag('span'); //3
     echo H::closeTag('h1'); //2
    echo H::closeTag('div'); //1 header

    echo H::openTag('div', ['id' => 'cssmenu']); //1
     echo H::openTag('ul'); //2
   $i = 0;
   $account = $this->wa_get_apps($title, $applications, $sel_app);
   $u_agent = $_SERVER['HTTP_USER_AGENT'] ?? '';
   $mobile = preg_match('/android/i', $u_agent)
    && preg_match('/mobile/i', $u_agent);
   foreach ($applications as $app) {
    if ($_SESSION["wa_current_user"]->check_application_access($app)) {
     $this->tab($app, (string)$account[3], $i, (bool)$mobile);
     // an application without modules is not counted
     if (count($app->modules)) {
      $i++;
     }
    } else {
     $i++;
    }
   }
     echo H::closeTag('ul'); //2
    echo H::closeTag('div'); //1 menu
  }
  echo H::openTag('div', ['class' => 'fa-body']); //1
  if ($no_menu) {
   echo H::br();
  } elseif ($title && !$no_menu && !$is_index) {
    echo H::openTag('div', ['class' => 'fa-content']); //2
     echo H::openTag('center'); //3
      echo H::openTag('table', ['id' => 'title']); //4
       echo H::openTag('tr'); //5
        echo H::openTag('td', ['width' => '100%', 'class' => 'titletext']); //6
         echo $title;
        echo H::closeTag('td'); //6
        echo H::openTag('td', ['align' => 'right']); //6
   if (user_hints()) {
          echo H::tag('span', '', ['id' => 'hints']);
   }
        echo H::closeTag('td'); //6
       echo H::closeTag('tr'); //5
      echo H::closeTag('table'); //4
     echo H::closeTag('center'); //3
  }
 }

 function menu_footer($no_menu, $is_index)
 {
  global $path_to_root, $SysPrefs, $version, $db_connections;
  include_once(dirname(__DIR__, 2) . "/includes/date_functions.inc");

  $date = ['class' => 'date'];

  if (!$no_menu && !$is_index) {
   echo H::closeTag('div'); //2 fa-content
  }
  echo H::closeTag('div'); //1 fa-body
  if (!$no_menu) {
   $mobileMenu = "if (/Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile"
    . "|Opera Mini/i.test(navigator.userAgent))\n"
    . "{document.getElementById('cssmenu').style.position = 'fixed';}";
   echo H::script($mobileMenu)
    ->addAttributes(['type' => 'text/javascript'])
    ->nonce((bool)sysprefs()->security_headers ? csp_nonce() : null);
   echo H::openTag('div', ['class' => 'fa-footer']); //1
   if (isset($_SESSION['wa_current_user'])) {
    $user = $_SESSION["wa_current_user"];
     echo H::openTag('span', ['class' => 'power']); //2
      echo H::openTag('a', [
       'target' => '_blank',
       'href' => $SysPrefs->power_url,
      ]); //3
       echo "$SysPrefs->power_by $version";
      echo H::closeTag('a'); //3
     echo H::closeTag('span'); //2
    $items = [
     Today() . "&nbsp;" . Now(),
     $db_connections[$user->company]["name"],
     $_SERVER['SERVER_NAME'],
     $user->name,
     _("Theme:") . " " . user_theme(),
     show_users_online(),
    ];
    foreach ($items as $item) {
     echo H::openTag('span', $date); //2
      echo $item;
     echo H::closeTag('span'); //2
    }
   }
   echo H::closeTag('div'); //1 footer
  }
  echo H::closeTag('div'); //0 fa-main
 }

 function display_applications(&$waapp)
 {
  global $path_to_root;

  $sel = $waapp->get_selected_application();
  meta_forward("$path_to_root/admin/dashboard.php", "sel_app=$sel->id");
  end_page();
  exit;
 }
}
