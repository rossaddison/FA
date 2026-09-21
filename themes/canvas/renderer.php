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
			if (preg_match("/accesskey='(.)'/", (string)$pars[1], $key) === 1)
				$attributes['accesskey'] = $key[1];
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
		private function header_item(array $attributes, string $image, string $alt, string $label): void
		{
			global $path_to_root;

			echo H::openTag('li'); //0
			 echo H::openTag('a', $attributes); //1
			  echo H::img("$path_to_root/themes/".user_theme()."/images/$image")->class('fa-icon14-pb3')->alt($alt)->render();
			  echo '&nbsp;&nbsp;'.$label;
			 echo H::closeTag('a'); //1
			echo H::closeTag('li'); //0
		}

		function menu_header($title, $no_menu, $is_index)
		{
			global $path_to_root, $SysPrefs, $version;

			$sel_app = $_SESSION['sel_app'];
			echo H::openTag('div', ['class' => 'fa-main']); //0
			if (!$no_menu)
			{
				$applications = $_SESSION['App']->applications;
				$local_path_to_root = $path_to_root;
				$indicator = "$path_to_root/themes/".user_theme(). "/images/ajax-loader.gif";
				 echo H::openTag('div', ['id' => 'header']); //1
				  echo H::openTag('ul'); //2
				   $this->header_item(['class' => 'shortcut', 'href' => "$path_to_root/admin/display_prefs.php?"], 'preferences.gif', _('Preferences'), _("Preferences"));
				   $this->header_item(['class' => 'shortcut', 'href' => "$path_to_root/admin/change_current_user_password.php?selected_id=" . $_SESSION["wa_current_user"]->username], 'lock.gif', _('Change Password'), _("Change password"));
				if ($SysPrefs->help_base_url != null)
					// help_url() answers an HTML-encoded URL; H encodes it again
					$this->header_item(['target' => '_blank', 'data-fa-action' => 'open-window', 'href' => html_entity_decode(help_url())], 'help.gif', _('Help'), _("Help"));
				   $this->header_item(['class' => 'shortcut', 'href' => "$path_to_root/access/logout.php?"], 'login.gif', _('Logout'), _("Logout"));
				  echo H::closeTag('ul'); //2
				  echo H::openTag('h1'); //2
				   echo "$SysPrefs->power_by $version";
				   echo H::openTag('span', ['class' => 'fa-pl300']); //3
				    echo H::img($indicator)->addAttributes(['id' => 'ajaxmark', 'align' => 'center', 'class' => 'fa-hidden'])->render();
				   echo H::closeTag('span'); //3
				  echo H::closeTag('h1'); //2
				 echo H::closeTag('div'); //1 header
				 echo H::openTag('div', ['class' => 'fa-menu']); //1
				  echo H::openTag('ul'); //2
				foreach($applications as $app)
				{
					if ($_SESSION["wa_current_user"]->check_application_access($app))
					{
						$acc = access_string($app->name);
						$attributes = [
							'class' => $sel_app == $app->id ? 'selected' : 'menu_tab',
							'href' => "$path_to_root/index.php?application=" . $app->id,
						];
						if (preg_match("/accesskey='(.)'/", (string)$acc[1], $key) === 1)
							$attributes['accesskey'] = $key[1];
						echo H::openTag('li', $sel_app == $app->id ? ['class' => 'active'] : []); //3
						 echo H::openTag('a', $attributes); //4
						  echo H::openTag('b'); //5
						   echo $acc[0];
						  echo H::closeTag('b'); //5
						 echo H::closeTag('a'); //4
						echo H::closeTag('li'); //3
					}
				}
				  echo H::closeTag('ul'); //2
				 echo H::closeTag('div'); //1 menu
				 echo H::tag('div', '', ['class' => 'clear']);
			}
			echo H::openTag('div', ['class' => 'fa-body']); //1
			if (!$no_menu)
			{
				 echo H::openTag('div', ['id' => 'fa-submenu']); //2
				  echo H::openTag('ul'); //3
				switch ($sel_app) // Shortcuts
				{
					case "orders":
						$this->shortcut($local_path_to_root."/sales/sales_order_entry.php?NewOrder=Yes",_("Sales Order"));
						$this->shortcut($local_path_to_root."/sales/sales_order_entry.php?NewInvoice=0",_("Direct Invoice"));
						$this->shortcut($local_path_to_root."/sales/customer_payments.php?", _("Payments"));
						$this->shortcut($local_path_to_root."/sales/inquiry/sales_orders_view.php?", _("Sales Order Inquiry"));
						$this->shortcut($local_path_to_root."/sales/inquiry/customer_inquiry.php?", _("Transactions"));
						$this->shortcut($local_path_to_root."/sales/manage/customers.php?", _("Customers"));
						$this->shortcut($local_path_to_root."/sales/manage/customer_branches.php?", _("Branch"));
						$this->shortcut($local_path_to_root."/reporting/reports_main.php?Class=0", _("Reports and Analysis"));
						break;
					case "AP":
						$this->shortcut($local_path_to_root."/purchasing/po_entry_items.php?NewOrder=0", _("Purchase Order"));
						$this->shortcut($local_path_to_root."/purchasing/inquiry/po_search.php?", _("Receive"));
						$this->shortcut($local_path_to_root."/purchasing/supplier_invoice.php?New=1", _("Supplier Invoice"));
						$this->shortcut($local_path_to_root."/purchasing/supplier_payment.php?", _("Payments"));
						$this->shortcut($local_path_to_root."/purchasing/inquiry/supplier_inquiry.php?", _("Transactions"));
						$this->shortcut($local_path_to_root."/purchasing/manage/suppliers.php?", _("Suppliers"));
						$this->shortcut($local_path_to_root."/reporting/reports_main.php?Class=1", _("Reports and Analysis"));
						break;
					case "stock":
						$this->shortcut($local_path_to_root."/inventory/adjustments.php?NewAdjustment=1", _("Inventory Adjustments"));
						$this->shortcut($local_path_to_root."/inventory/inquiry/stock_movements.php?", _("Inventory Movements"));
						$this->shortcut($local_path_to_root."/inventory/manage/items.php?", _("Items"));
						$this->shortcut($local_path_to_root."/inventory/prices.php?", _("Sales Pricing"));
						$this->shortcut($local_path_to_root."/reporting/reports_main.php?Class=2", _("Reports and Analysis"));
						break;
					case "manuf":
						$this->shortcut($local_path_to_root."/manufacturing/work_order_entry.php?", _("Work Order Entry"));
						$this->shortcut($local_path_to_root."/manufacturing/search_work_orders.php?outstanding_only=1", _("Ourstanding Work Orders"));
						$this->shortcut($local_path_to_root."/manufacturing/search_work_orders.php?", _("Work Order Inquiry"));
						$this->shortcut($local_path_to_root."/manufacturing/manage/bom_edit.php?", _("Bills Of Material"));
						$this->shortcut($local_path_to_root."/reporting/reports_main.php?Class=3", _("Reports and Analysis"));
						break;
					case "assets":
						$this->shortcut($local_path_to_root."/purchasing/po_entry_items.php?NewInvoice=Yes&FixedAsset=1", _("Fixed Assets Purchase"));
						$this->shortcut($local_path_to_root."/fixed_assets/inquiry/stock_inquiry.php?", _("Fixed Assets Inquiry"));
						$this->shortcut($local_path_to_root."/inventory/manage/items.php?FixedAsset=1", _("Fixed Assets"));
						$this->shortcut($local_path_to_root."/fixed_assets/process_depreciation.php?", _("Depreciations"));
						$this->shortcut($local_path_to_root."/reporting/reports_main.php?Class=7", _("Reports and Analysis"));
						break;
					case "proj":
						$this->shortcut($local_path_to_root."/dimensions/dimension_entry.php?", _("Dimension Entry"));
						$this->shortcut($local_path_to_root."/dimensions/inquiry/search_dimensions.php?", _("Dimension Inquiry"));
						$this->shortcut($local_path_to_root."/reporting/reports_main.php?Class=4", _("Reports and Analysis"));
						break;
					case "GL":
						$this->shortcut($local_path_to_root."/gl/gl_bank.php?NewPayment=Yes",_("Payments"));
						$this->shortcut($local_path_to_root."/gl/gl_bank.php?NewDeposit=Yes",_("Deposits"));
						$this->shortcut($local_path_to_root."/gl/gl_journal.php?NewJournal=Yes",_("Journal Entry"));
						$this->shortcut($local_path_to_root."/gl/inquiry/bank_inquiry.php?",_("Bank Account Inquiry"));
						//$this->shortcut($local_path_to_root."/gl/inquiry/gl_account_inquiry.php?",_("GL Account Inquiry"));
						$this->shortcut($local_path_to_root."/gl/inquiry/gl_trial_balance.php?",_("Trial Balance"));
						$this->shortcut($local_path_to_root."/gl/manage/exchange_rates.php?",_("Exchange Rates"));
						$this->shortcut($local_path_to_root."/gl/manage/gl_accounts.php?",_("GL Accounts"));
						$this->shortcut($local_path_to_root."/reporting/reports_main.php?Class=6",_("Reports and Analysis"));
						break;
					case "system":
						$this->shortcut($local_path_to_root."/admin/company_preferences.php?",_("Company Setup"));
						$this->shortcut($local_path_to_root."/admin/gl_setup.php?",_("General GL"));
						$this->shortcut($local_path_to_root."/taxes/tax_types.php?",_("Taxes"));
						$this->shortcut($local_path_to_root."/taxes/tax_groups.php?",_("Tax Groups"));
						$this->shortcut($local_path_to_root."/admin/forms_setup.php?",_("Forms Setup"));
						$this->shortcut($local_path_to_root."/admin/backups.php?",_("Backup and Restore"));
						break;
				}
				  $this->shortcut($local_path_to_root."/admin/dashboard.php?sel_app=$sel_app", _("Dashboard"));
				  echo H::closeTag('ul'); //3
				 echo H::closeTag('div'); //2 fa-submenu
				 echo H::tag('div', '', ['class' => 'clear']);
				 echo H::openTag('div', ['class' => 'fa-content']); //2
			}
			if ($no_menu)
				echo H::br();
			elseif ($title && !$no_menu && !$is_index)
			{
				echo H::openTag('center'); //3
				 echo H::openTag('table', ['id' => 'title']); //4
				  echo H::openTag('tr'); //5
				   echo H::openTag('td', ['width' => '100%', 'class' => 'titletext']); //6
				    echo $title;
				   echo H::closeTag('td'); //6
				   echo H::openTag('td', ['align' => 'right']); //6
				    if (user_hints())
				     echo H::tag('span', '', ['id' => 'hints']);
				   echo H::closeTag('td'); //6
				  echo H::closeTag('tr'); //5
				 echo H::closeTag('table'); //4
				echo H::closeTag('center'); //3
			}
		}

		function menu_footer($no_menu, $is_index)
		{
			global $path_to_root, $SysPrefs, $version, $db_connections;
			include_once($path_to_root . "/includes/date_functions.inc");

			$date = ['class' => 'date'];

			if (!$no_menu)
				echo H::closeTag('div'); //2 fa-content
			echo H::closeTag('div'); //1 fa-body
			if (!$no_menu)
			{
				echo H::openTag('div', ['class' => 'fa-footer']); //1
				if (isset($_SESSION['wa_current_user']))
				{
					 echo H::openTag('span', ['class' => 'power']); //2
					  echo H::openTag('a', ['target' => '_blank', 'href' => $SysPrefs->power_url]); //3
					   echo "$SysPrefs->power_by $version";
					  echo H::closeTag('a'); //3
					 echo H::closeTag('span'); //2
					 echo H::openTag('span', $date); //2
					  echo Today() . "&nbsp;" . Now();
					 echo H::closeTag('span'); //2
					 echo H::openTag('span', $date); //2
					  echo $db_connections[$_SESSION["wa_current_user"]->company]["name"];
					 echo H::closeTag('span'); //2
					 echo H::openTag('span', $date); //2
					  echo $_SERVER['SERVER_NAME'];
					 echo H::closeTag('span'); //2
					 echo H::openTag('span', $date); //2
					  echo $_SESSION["wa_current_user"]->name;
					 echo H::closeTag('span'); //2
					 echo H::openTag('span', $date); //2
					  echo _("Theme:") . " " . user_theme();
					 echo H::closeTag('span'); //2
					 echo H::openTag('span', $date); //2
					  echo show_users_online();
					 echo H::closeTag('span'); //2
				}
				echo H::closeTag('div'); //1 footer
			}
			echo H::closeTag('div'); //0 fa-main
		}

		/**
		 * The links of one column of a module: an icon and the link (or the greyed label), one per line.
		 */
		private function display_functions(array $functions, string $img): void
		{
			foreach ($functions as $appfunction)
			{
				if ($appfunction->label == "")
				{
					echo H::openTag('div', ['class' => 'empty']); //0
					 echo '&nbsp;';
					 echo H::br();
					echo H::closeTag('div'); //0
				}
				elseif ($_SESSION["wa_current_user"]->can_access_page($appfunction->access))
				{
					echo H::openTag('div'); //0
					 echo $img.menu_link($appfunction->link, $appfunction->label);
					echo H::closeTag('div'); //0
				}
				elseif (!$_SESSION["wa_current_user"]->hide_inaccessible_menu_items())
				{
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
			if (!$_SESSION["wa_current_user"]->check_application_access($sel_app))
				return;
			if ($sel_app->id == "system")
				$imgs2 = array("page_edit.png", "page_edit.png", "page_edit.png", "page_edit.png", "folder.gif");
			else
				$imgs2 = array("folder.gif", "report.png", "page_edit.png", "money.png", "folder.gif");
			$menuGroup = ['class' => 'menu_group'];
			$menuItems = ['width' => '50%', 'class' => 'menu_group_items'];
			foreach ($sel_app->modules as $module)
			{
				if (!$_SESSION["wa_current_user"]->check_module_access($module))
					continue;
				$img = H::img("$path_to_root/themes/".user_theme()."/images/".$imgs2[$i])->class('fa-icon14-pb3')->render().'&nbsp;&nbsp;';
				echo H::openTag('table', ['width' => '95%', 'align' => 'center']); //0
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
				      $this->display_functions($module->lappfunctions, $img);
				     echo H::closeTag('td'); //5
				if (sizeof($module->rappfunctions) > 0)
				{
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
