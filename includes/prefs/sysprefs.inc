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
include_once(dirname(__DIR__, 2) . "/admin/db/company_db.inc");

/*
	System and per company prefferences.
	Object is created only with site wide preffernces.
	After user login refresh method have to be called to re-initialize company specific settings.
*/
final class sys_prefs extends stdClass
{
	public string $error_logfile = '';
	public string $app_title = 'FrontAccounting';
	public int $no_check_edit_conflicts = 0;
	public int $use_icon_for_editkey = 0;
	public int $auto_create_branch = 1;
	public int $use_popup_windows = 1;
	public int $use_audit_trail = 0;
	public int $use_oldstyle_convert = 0;
	public int $show_users_online = 0;
	public int $old_style_help = 0;
	public ?string $help_base_url = 'http://frontaccounting.com/fawiki/index.php?n=Help.';
	public string $comp_path = '';
	public int $date_system = 0;
	public int $allow_gl_reopen = 0;
	public int $dflt_date_fmt = 0;
	public int $dflt_date_sep = 0;
	public bool $check_qty_charged_vs_del_qty = true;
	public bool $check_price_charged_vs_order_price = true;
	public float $config_allocation_settled_allowance = 0.005;
	public int $use_costed_values = 1;
	public int $show_menu_category_icons = 1;
	public bool $allow_demo_mode = false;
	public bool $allow_password_reset = false;
	public int $max_image_size = 500;
	public bool $text_company_selection = false;
	public int $hide_inaccessible_menu_items = 0;
	public int $login_delay = 30;
	public int $login_max_attempts = 10;
	/** @var array<int, string> */
	public array $xr_providers = ["ECB", "EXCHANGE-RATES.ORG", "GOOGLE", "YAHOO", "BLOOMBERG"];
	public int $dflt_xr_provider = 0;
	public bool $xr_provider_authoritative = false;
	public bool $sort_sales_items = false;
	public bool $clear_trial_balance_opening = false;
	public int $max_rows_in_search = 10;
	public bool $use_popup_search = true;
	public int $use_yii_layout = 0;
	public int $security_headers = 0;
	public string $content_security_policy = '';
	public int|bool $csp_report_only = 1;
	public int $debug = 1;
	public int $show_sql = 0;
	public int $go_debug = 0;
	public int $pdf_debug = 0;
	public int|bool $sql_trail = 0;
	public int $select_trail = 0;
	public string $build_version = '';
	public string $power_by = 'FrontAccounting';
	public string $power_url = 'http://frontaccounting.com';
	/** @var array<int, string> */
	public array $dateformats = ["MMDDYYYY", "DDMMYYYY", "YYYYMMDD", "MmmDDYYYY", "DDMmmYYYY", "YYYYMmmDD"];
	/** @var array<int, string> */
	public array $dateseps = ["/", ".", "-", " "];
	/** @var array<int, string> */
	public array $thoseps = [",", ".", " "];
	/** @var array<int, string> */
	public array $decseps = [".", ","];
	/** @var array<int, string> */
	public array $pagesizes = ["Letter", "A4"];
	public int $pic_height = 50;
	public string $UTF8_fontfile = '';
	public string $time_zone = '';
	/* Optional backup path override; %s stands for the company number. Not set in config.default.php. */
	public ?string $backup_path = null;
	/* Optional application version override; falls back to $src_version. Not set in config.default.php. */
	public ?string $version = null;
	/**
	 * Optional package-repository auth override, only set by admins who add it to config.php.
	 * @var array{scheme?: string, login?: string, pass?: string, host?: string, path?: string, branch?: string}|null
	 */
	public ?array $repo_auth = null;

	/** @var array<string, string|int|float|bool|null> */
	public array $prefs = array();

	public ?bool $db_ok = null; // check whether database has been upgraded after src update.

	function __construct()
	{
		global $path_to_root;

		// retrieve set system wide preferences
		include $path_to_root.'/config.default.php';
		if (file_exists($path_to_root.'/config.php'))
			include $path_to_root.'/config.php';

		foreach(get_defined_vars() as $name => $value)
			$this->$name = $value;

		if (!$this->time_zone)
			$this->time_zone = (string) ini_get('date.timezone');

		if (!$this->time_zone)
			$this->time_zone = 'Europe/Berlin';

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
	/** @var Cart|purch_order|supp_trans|allocation|items_cart|language|current_user $object */
	$object = $_SESSION[$key];
	return $object;
}
