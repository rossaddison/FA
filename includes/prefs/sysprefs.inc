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
include_once($path_to_root . "/admin/db/company_db.inc");

/*
	System and per company prefferences.
	Object is created only with site wide preffernces.
	After user login refresh method have to be called to re-initialize company specific settings.
*/
class sys_prefs extends stdClass
{
	/** @var string|null */
	var $error_logfile;
	/** @var string|null */
	var $app_title;
	/** @var int|string|bool|null */
	var $no_check_edit_conflicts;
	/** @var int|string|bool|null */
	var $use_icon_for_editkey;
	/** @var int|string|bool|null */
	var $auto_create_branch;
	/** @var int|string|bool|null */
	var $use_popup_windows;
	/** @var int|string|bool|null */
	var $use_audit_trail;
	/** @var int|string|bool|null */
	var $use_oldstyle_convert;
	/** @var int|string|bool|null */
	var $show_users_online;
	/** @var int|string|bool|null */
	var $old_style_help;
	/** @var string|null */
	var $help_base_url;
	/** @var string|null */
	var $comp_path;
	/** @var int|string|null */
	var $date_system;
	/** @var int|string|bool|null */
	var $allow_gl_reopen;
	/** @var int|string|null */
	var $dflt_date_fmt;
	/** @var int|string|null */
	var $dflt_date_sep;
	/** @var int|string|bool|null */
	var $check_qty_charged_vs_del_qty;
	/** @var int|string|bool|null */
	var $check_price_charged_vs_order_price;
	/** @var float|int|string|null */
	var $config_allocation_settled_allowance;
	/** @var int|string|bool|null */
	var $use_costed_values;
	/** @var int|string|bool|null */
	var $show_menu_category_icons;
	/** @var int|string|bool|null */
	var $allow_demo_mode;
	/** @var int|string|bool|null */
	var $allow_password_reset;
	/** @var int|string|null */
	var $max_image_size;
	/** @var int|string|bool|null */
	var $text_company_selection;
	/** @var int|string|bool|null */
	var $hide_inaccessible_menu_items;
	/** @var int|string|null */
	var $login_delay;
	/** @var int|string|null */
	var $login_max_attempts;
	/** @var array<int, string>|null */
	var $xr_providers;
	/** @var int|string|null */
	var $dflt_xr_provider;
	/** @var int|string|bool|null */
	var $xr_provider_authoritative;
	/** @var int|string|bool|null */
	var $sort_sales_items;
	/** @var int|string|bool|null */
	var $clear_trial_balance_opening;
	/** @var int|string|null */
	var $max_rows_in_search;
	/** @var int|string|bool|null */
	var $use_popup_search;
	/** @var int|string|null */
	var $debug;
	/** @var int|string|bool|null */
	var $show_sql;
	/** @var int|string|null */
	var $go_debug;
	/** @var int|string|null */
	var $pdf_debug;
	/** @var int|string|bool|null */
	var $sql_trail;
	/** @var int|string|bool|null */
	var $select_trail;
	/** @var string|null */
	var $build_version;
	/** @var string|null */
	var $power_by;
	/** @var string|null */
	var $power_url;
	/** @var array<int, string>|null */
	var $dateformats;
	/** @var array<int, string>|null */
	var $dateseps;
	/** @var array<int, string>|null */
	var $thoseps;
	/** @var array<int, string>|null */
	var $decseps;
	/** @var array<int, string>|null */
	var $pagesizes;
	/** @var int|string|null */
	var $pic_height;
	/** @var string|null */
	var $UTF8_fontfile;
	/** @var string|null */
	var $time_zone;

	/** @var array<string, string|int|float|bool|null>|null */
	var $prefs;

	/**
	 * @var bool|null
	 */
	var $db_ok; // check whether database has been upgraded after src update.

	function __construct()
	{
		global $path_to_root;

		// retrieve set system wide preferences
		include $path_to_root.'/config.default.php';
		if (file_exists($path_to_root.'/config.php'))
			include $path_to_root.'/config.php';

		foreach(get_defined_vars() as $name => $value)
			$this->$name = $value;

		if (!property_exists($this, 'time_zone') or !$this->time_zone)
			$this->time_zone = ini_get('date.timezone');

		if (!(bool)$this->time_zone)
			$this->time_zone = 'Europe/Berlin';
			
		if (!isset($this->use_popup_search))
			$this->use_popup_search = false;

   	    ini_set('date.timezone', $this->time_zone);
	}

	/*
		Re-initialize company preferrencies.
	*/
	/** @return null */
	function refresh()
	{
		global $db_version;

		$sql = "SELECT name, value FROM ".TB_PREF."sys_prefs";
		$result = @db_query($sql); // supress errors before 2.3 db structure upgrade

		if(!$result)
			return null;

		while($pref = db_fetch_assoc($result)) {
			$this->prefs[$pref['name']] = $pref['value'];
		}

		// update current db status for info in log file
		$this->db_ok = $this->prefs['version_id'] == $db_version;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function allow_negative_stock() 
	{
		return $this->prefs['allow_negative_stock'];
	}

    /* Sherifoz 26.06.03 Proportion by which items can be received over the quantity that is specified in a purchase
    invoice
    The figure entered is interpreted as a percentage ie 10 means 0.1 or 10% not 10 times
    */
	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function over_receive_allowance() 
	{
		return $this->prefs['po_over_receive'];
	}
	
    /* Proportion by which a purchase invoice line is an overcharge for a purchase order item received
    is an overcharge. If the overcharge is more than this percentage then an error is reported and
    purchase invoice line cannot be entered
    The figure entered is interpreted as a percentage ie 20 means 0.2 or 20% not 20 times
    */	
	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function over_charge_allowance() 
	{
		return $this->prefs['po_over_charge'];
	}
	
	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function default_credit_limit() 
	{
		return $this->prefs['default_credit_limit'];
	}				
	
	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function default_wo_required_by() 
	{
		return $this->prefs['default_workorder_required'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function default_quote_valid_days() 
	{
		return $this->prefs['default_quote_valid_days'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function default_delivery_required_by() 
	{
		return $this->prefs['default_delivery_required'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function default_receival_required_by() 
	{
		return $this->prefs['default_receival_required'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function default_dimension_required_by() 
	{
		return $this->prefs['default_dim_required'];
	}	
	
	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function auto_currency_revaluation() 
	{
		return $this->prefs['auto_curr_reval'];
	}	
	
	/**
	 * @psalm-mutation-free
	 * @return float|int|null|string
	 */
	function allocation_settled_allowance()
	{
		return $this->config_allocation_settled_allowance;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function no_zero_lines_amount() 
	{
		return $this->prefs['no_zero_lines_amount'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function show_po_item_codes() 
	{
		return $this->prefs['show_po_item_codes'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function accounts_alpha() 
	{
		return $this->prefs['accounts_alpha'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function loc_notification() 
	{
		return $this->prefs['loc_notification'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function print_invoice_no() 
	{
		return $this->prefs['print_invoice_no'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function allow_negative_prices() 
	{
		return $this->prefs['allow_negative_prices'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function print_item_images_on_quote() 
	{
		return $this->prefs['print_item_images_on_quote'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function alternative_tax_include_on_docs() 
	{
		return $this->prefs['alternative_tax_include_on_docs'];
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function suppress_tax_rates() 
	{
		return $this->prefs['suppress_tax_rates'];
	}

	/** @return string */
	function backup_dir($comp=null)
	{
		if (!isset($comp))
			$comp = user_company();

		if (isset($this->backup_path))
			return sprintf($this->backup_path, $comp);
		else
			return $this->comp_path.'/'.$comp."/backup/";
	}
}


/**
 * The site wide preferences object created by session.inc. Psalm forgets the type of a global after any function
 * call, so code reads the preferences through this accessor instead of `global $SysPrefs`.
 */
function sysprefs(): sys_prefs
{
	global $SysPrefs;

	return $SysPrefs;
}

/**
 * The objects kept in the session (carts, the language, the user). Psalm forgets the type of a $_SESSION key after any function call, so the
 * pages read them through this accessor, which states the type of each key.
 *
 * @param 'Items'|'PO'|'supp_trans'|'alloc'|'transfer_items'|'issue_items'|'adj_items'|'pay_items'|'journal_items'|'View'|'language'|'wa_current_user' $key
 * @return ($key is 'Items'|'View' ? Cart : ($key is 'PO' ? purch_order : ($key is 'supp_trans' ? supp_trans : ($key is 'alloc' ? allocation : ($key is 'language' ? language : ($key is 'wa_current_user' ? current_user : items_cart))))))
 */
function session_obj(string $key)
{
	return $_SESSION[$key];
}
