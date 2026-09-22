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
  $pars = access_string($label);
  $attributes = ['href' => $url, 'class' => 'menu_option'];
  // access_string() answers " accesskey='X'"
  if (preg_match("/accesskey='(.)'/", (string)$pars[1], $key) === 1) {
   $attributes['accesskey'] = $key[1];
  }
  echo H::openTag('li'); //0
   echo H::openTag('a', $attributes); //1
    echo $pars[0];
   echo H::closeTag('a'); //1
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
   [
    'class' => 'shortcut',
    'href' => "$path_to_root/admin/display_prefs.php?",
   ],
   'preferences.gif',
   _('Preferences'),
   _("Preferences")
  );
  $this->header_item(
   [
    'class' => 'shortcut',
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
   [
    'class' => 'shortcut',
    'href' => "$path_to_root/access/logout.php?",
   ],
   'login.gif',
   _('Logout'),
   _("Logout")
  );
 }

 /**
  * The tab per application the user may open.
  */
 private function tabs(string $sel_app): void
 {
  global $path_to_root;

  foreach ($_SESSION['App']->applications as $app) {
   if (!$_SESSION["wa_current_user"]->check_application_access($app)) {
    continue;
   }
   $acc = access_string($app->name);
   $attributes = [
    'class' => $sel_app == $app->id ? 'selected' : 'menu_tab',
    'href' => "$path_to_root/index.php?application=" . $app->id,
   ];
   if (preg_match("/accesskey='(.)'/", (string)$acc[1], $key) === 1) {
    $attributes['accesskey'] = $key[1];
   }
   echo H::openTag('li', $sel_app == $app->id ? ['class' => 'active'] : []); //0
    echo H::openTag('a', $attributes); //1
     echo H::openTag('b'); //2
      echo $acc[0];
     echo H::closeTag('b'); //2
    echo H::closeTag('a'); //1
   echo H::closeTag('li'); //0
  }
 }

 /**
  * The shortcuts under the tabs of an application: [path, label] pairs.
  *
  * @return list<array{0: string, 1: string}>
  */
 private function shortcuts(string $sel_app): array
 {
  $reports = "/reporting/reports_main.php?Class=";

  switch ($sel_app) {
   case "orders":
    return [
     ["/sales/sales_order_entry.php?NewOrder=Yes", _("Sales Order")],
     ["/sales/sales_order_entry.php?NewInvoice=0", _("Direct Invoice")],
     ["/sales/customer_payments.php?", _("Payments")],
     ["/sales/inquiry/sales_orders_view.php?", _("Sales Order Inquiry")],
     ["/sales/inquiry/customer_inquiry.php?", _("Transactions")],
     ["/sales/manage/customers.php?", _("Customers")],
     ["/sales/manage/customer_branches.php?", _("Branch")],
     [$reports . "0", _("Reports and Analysis")],
    ];
   case "AP":
    return [
     ["/purchasing/po_entry_items.php?NewOrder=0", _("Purchase Order")],
     ["/purchasing/inquiry/po_search.php?", _("Receive")],
     ["/purchasing/supplier_invoice.php?New=1", _("Supplier Invoice")],
     ["/purchasing/supplier_payment.php?", _("Payments")],
     ["/purchasing/inquiry/supplier_inquiry.php?", _("Transactions")],
     ["/purchasing/manage/suppliers.php?", _("Suppliers")],
     [$reports . "1", _("Reports and Analysis")],
    ];
   case "stock":
    return [
     ["/inventory/adjustments.php?NewAdjustment=1",
      _("Inventory Adjustments")],
     ["/inventory/inquiry/stock_movements.php?",
      _("Inventory Movements")],
     ["/inventory/manage/items.php?", _("Items")],
     ["/inventory/prices.php?", _("Sales Pricing")],
     [$reports . "2", _("Reports and Analysis")],
    ];
   case "manuf":
    return [
     ["/manufacturing/work_order_entry.php?", _("Work Order Entry")],
     ["/manufacturing/search_work_orders.php?outstanding_only=1",
      _("Ourstanding Work Orders")],
     ["/manufacturing/search_work_orders.php?",
      _("Work Order Inquiry")],
     ["/manufacturing/manage/bom_edit.php?", _("Bills Of Material")],
     [$reports . "3", _("Reports and Analysis")],
    ];
   case "assets":
    return [
     ["/purchasing/po_entry_items.php?NewInvoice=Yes&FixedAsset=1",
      _("Fixed Assets Purchase")],
     ["/fixed_assets/inquiry/stock_inquiry.php?",
      _("Fixed Assets Inquiry")],
     ["/inventory/manage/items.php?FixedAsset=1", _("Fixed Assets")],
     ["/fixed_assets/process_depreciation.php?", _("Depreciations")],
     [$reports . "7", _("Reports and Analysis")],
    ];
   case "proj":
    return [
     ["/dimensions/dimension_entry.php?", _("Dimension Entry")],
     ["/dimensions/inquiry/search_dimensions.php?",
      _("Dimension Inquiry")],
     [$reports . "4", _("Reports and Analysis")],
    ];
   case "GL":
    return [
     ["/gl/gl_bank.php?NewPayment=Yes", _("Payments")],
     ["/gl/gl_bank.php?NewDeposit=Yes", _("Deposits")],
     ["/gl/gl_journal.php?NewJournal=Yes", _("Journal Entry")],
     ["/gl/inquiry/bank_inquiry.php?", _("Bank Account Inquiry")],
     ["/gl/inquiry/gl_trial_balance.php?", _("Trial Balance")],
     ["/gl/manage/exchange_rates.php?", _("Exchange Rates")],
     ["/gl/manage/gl_accounts.php?", _("GL Accounts")],
     [$reports . "6", _("Reports and Analysis")],
    ];
   case "system":
    return [
     ["/admin/company_preferences.php?", _("Company Setup")],
     ["/admin/gl_setup.php?", _("General GL")],
     ["/taxes/tax_types.php?", _("Taxes")],
     ["/taxes/tax_groups.php?", _("Tax Groups")],
     ["/admin/forms_setup.php?", _("Forms Setup")],
     ["/admin/backups.php?", _("Backup and Restore")],
    ];
  }
  return [];
 }

 function menu_header($title, $no_menu, $is_index)
 {
  global $path_to_root, $SysPrefs, $version;

  $sel_app = $_SESSION['sel_app'];
  echo H::openTag('div', ['class' => 'fa-main']); //0
  if (!$no_menu) {
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
     echo H::openTag('h1'); //2
      echo "$SysPrefs->power_by $version";
      echo H::openTag('span', ['class' => 'fa-pl300']); //3
       echo H::img($indicator)->addAttributes($ajaxMark)->render();
      echo H::closeTag('span'); //3
     echo H::closeTag('h1'); //2
    echo H::closeTag('div'); //1 header
    echo H::openTag('div', ['class' => 'fa-menu']); //1
     echo H::openTag('ul'); //2
   $this->tabs($sel_app);
     echo H::closeTag('ul'); //2
    echo H::closeTag('div'); //1 menu
    echo H::tag('div', '', ['class' => 'clear']);
  }
  echo H::openTag('div', ['class' => 'fa-body']); //1
  if (!$no_menu) {
    echo H::openTag('div', ['id' => 'fa-submenu']); //2
     echo H::openTag('ul'); //3
   foreach ($this->shortcuts($sel_app) as [$path, $label]) {
    $this->shortcut($path_to_root . $path, $label);
   }
   $this->shortcut(
    $path_to_root . "/admin/dashboard.php?sel_app=$sel_app",
    _("Dashboard")
   );
     echo H::closeTag('ul'); //3
    echo H::closeTag('div'); //2 fa-submenu
    echo H::tag('div', '', ['class' => 'clear']);
    echo H::openTag('div', ['class' => 'fa-content']); //2
  }
  if ($no_menu) {
   echo H::br();
  } elseif ($title && !$no_menu && !$is_index) {
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

  if (!$no_menu) {
   echo H::closeTag('div'); //2 fa-content
  }
  echo H::closeTag('div'); //1 fa-body
  if (!$no_menu) {
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

 /**
  * The links of one column of a module: an icon and the link (or the greyed
  * label), one per line.
  */
 private function display_functions(array $functions, string $img): void
 {
  $user = $_SESSION["wa_current_user"];
  foreach ($functions as $appfunction) {
   if ($appfunction->label == "") {
    echo H::openTag('div', ['class' => 'empty']); //0
     echo '&nbsp;';
     echo H::br();
    echo H::closeTag('div'); //0
   } elseif ($user->can_access_page($appfunction->access)) {
    echo H::openTag('div'); //0
     echo $img . menu_link($appfunction->link, $appfunction->label);
    echo H::closeTag('div'); //0
   } elseif (!$user->hide_inaccessible_menu_items()) {
    echo H::openTag('div'); //0
     echo $img;
     echo H::openTag('span', ['class' => 'inactive']); //1
      echo access_string($appfunction->label, true);
     echo H::closeTag('span'); //1
    echo H::closeTag('div'); //0
   }
  }
 }

 function display_applications(&$waapp)
 {
  global $path_to_root;

  $i = 0;
  $sel_app = $waapp->get_selected_application();
  if (!$_SESSION["wa_current_user"]->check_application_access($sel_app)) {
   return;
  }
  if ($sel_app->id == "system") {
   $imgs2 = ["page_edit.png", "page_edit.png", "page_edit.png",
    "page_edit.png", "folder.gif"];
  } else {
   $imgs2 = ["folder.gif", "report.png", "page_edit.png", "money.png",
    "folder.gif"];
  }
  $menuGroup = ['class' => 'menu_group'];
  $menuItems = ['width' => '50%', 'class' => 'menu_group_items'];
  $menuCell = ['valign' => 'top', 'class' => 'menu_group'];
  foreach ($sel_app->modules as $module) {
   if (!$_SESSION["wa_current_user"]->check_module_access($module)) {
    continue;
   }
   $src = "$path_to_root/themes/" . user_theme() . "/images/" . $imgs2[$i];
   $img = H::img($src)->class('fa-icon14-pb3')->render() . '&nbsp;&nbsp;';
   echo H::openTag('table', ['width' => '95%', 'align' => 'center']); //0
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
         $this->display_functions($module->lappfunctions, $img);
        echo H::closeTag('td'); //5
   if (sizeof($module->rappfunctions) > 0) {
     echo H::openTag('td', $menuItems); //5
      $this->display_functions($module->rappfunctions, $img);
     echo H::closeTag('td'); //5
   }
       echo H::closeTag('tr'); //4
      echo H::closeTag('table'); //3
     echo H::closeTag('td'); //2
    echo H::closeTag('tr'); //1
   echo H::closeTag('table'); //0
   $i++;
  }
 }
}
