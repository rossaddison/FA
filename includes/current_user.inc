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

include_once(dirname(__DIR__) . "/includes/prefs/userprefs.inc");
if (!defined('TB_PREF')) {
	define('TB_PREF', '&TB_PREF&');
}
//--------------------------------------------------------------------------

final class current_user
{
	/** @var int|string */
	var $user = 0;
	/** @var string */
	var $loginname = '';
	/** @var string */
	var $username = '';
	/** @var string */
	var $name = '';
	/** @var string */
	var $email = '';
	/** @var string|int|float|bool|null */
	var $company; // user's company
	/** @var string|int|float|bool|null */
	var $pos;
	/** @var string|int|float|bool|null */
	var $access;
	/** @var int|string */
	var $timeout;

	/**
	 * @var int|null
	 */
	var $last_act;
	/** @var bool|array<array-key, mixed> */
	var $role_set = false;

	/**
	 * @var bool|null
	 */
	var $old_db;

	/**
	 * @var bool
	 */
	var $logged;

	/**
	 * @var int
	 */
	var $ui_mode = 0;

	/**
	 * @var int
	 */
	var $login_attempt=0;
	
	/**
	 * @var user_prefs
	 */
	var $prefs;
	/** @var string|int|float|bool|null */
	var $cur_con; // current db connection (can be different from $company for superuser)

	function __construct()
	{
		// checked via $GLOBALS rather than isset() so this doesn't depend on psalm.xml's
		// non-nullable stub for $def_coy, which only describes its value once config_db.php
		// (written during installation) has defined it, not whether it exists yet
		// pre-installation -- same pattern as includes/session.inc's equivalent check
		global $def_coy;

		$this->loginname = $this->username = $this->name = "";
		$this->company = array_key_exists('def_coy', $GLOBALS) ? $def_coy : 0;
		$this->logged = false;

		$this->prefs = new user_prefs();
	}

	/** @return bool */
	function logged_in()
	{
		return $this->logged;
	}

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function set_company(int $company)
	{
		$this->company = $company;
	}

	function login(mixed $company, string $loginname, string $password): bool
	{
		global $security_areas, $security_groups, $security_headings, $path_to_root, $dflt_lang;

		$this->set_company((int) $company);
	    $this->logged = false;

		set_global_connection(is_scalar($company) ? $company : null);
		session_obj('language')->set_language(session_obj('language')->code);
		db_set_encoding(session_obj('language')->encoding);

		// Use external authentication source if any.
		// Keep in mind you need to have user data set for $loginname
		// in FA users table anyway to successfully log in.
		// $Auth_Result is genuinely mixed by design: hook_authenticate() is an
		// extension point whose real return type is up to each hook implementation
		$Auth_Result = hook_authenticate($loginname, $password);

		if (!isset($Auth_Result))	// if not used: standard method
			$Auth_Result = get_user_auth($loginname, md5($password));

		if (sysprefs()->login_delay > 0)
			write_login_filelog($loginname, $Auth_Result);

		if ((bool)$Auth_Result)
		{
			$myrow = row_or_empty(get_user_by_login($loginname));
			$myrow_language = $myrow['language'] ?? null;
			if ($myrow_language != $dflt_lang)
			{	 // refresh language and user data
 				session_obj('language')->set_language(is_scalar($myrow_language) ? $myrow_language : null);
				db_set_encoding(session_obj('language')->encoding);
				$myrow = row_or_empty(get_user_by_login($loginname));
			}

			$this->old_db = isset($myrow["full_access"]);
			if (! (bool)(@$myrow["inactive"])) {
				if ($this->old_db) {
					// Transition code:
					// db was not yet upgraded after source update to v.2.2
					// give enough access for admin user to continue upgrade
					// $security_groups is deliberately left untyped (no psalm.xml global
					// stub): it's a pre-2.2 config.php legacy array that only exists on a
					// database never upgraded past that version, used nowhere else of
					// value, not worth the global-stub ripple risk already documented in
					// doc/PSALM_UPSTREAM_LIMITATIONS.md for exactly this kind of change
					if (!isset($security_groups) || !isset($security_headings)) {
						echo "<center><br><br><font size='5' color='red'><b>";
						echo _('Before software upgrade you have to include old $security_groups and $security_headings arrays from old config.php file to the new one.');
						echo '<br>'."<a href=$path_to_root/index.php>"._("Back")."</a>";
						echo "</b></font><br><br></center>";
						exit;
					}
	        	    $this->access = $myrow["full_access"];
	        	    $access_key = is_scalar($this->access) ? $this->access : null;
	        	    if (in_array(20, $security_groups[$access_key] ?? array())) {
						// temporary access for admin users
						$this->role_set = array();
						$this->role_set[] = $security_areas['SA_SOFTWAREUPGRADE'][0];
					} else {
						echo "<center><br><br><font size='5' color='red'><b>";
						echo _('System is available for site admin only until full database upgrade');
						echo "</b></font><br><br></center>";
						exit;
					}
	        	} else {
					$this->role_set = array();
	        	    $this->access = $myrow["role_id"];
					// store area codes available for current user role
					$role = row_or_empty(get_security_role(is_scalar($this->access) ? $this->access : null));
					if (!$role)
						return false;
					/** @var list<string> $role_areas */
					$role_areas = $role['areas'] ?? array();
					/** @var list<string> $role_sections */
					$role_sections = $role['sections'] ?? array();
					foreach( $role_areas as $code )
						// filter only area codes for enabled security sections
						if (in_array(((int) $code)&~0xff, $role_sections))
							$this->role_set[] = $code;
	        	}
        	    $this->name = is_scalar($myrow["real_name"]) ? $myrow["real_name"] : '';
        	    $this->pos = $myrow["pos"];
        	    $this->loginname = $loginname;
        	    $this->username = $this->loginname;
        	    $this->prefs = new user_prefs($myrow);
        	    $user_id = $myrow["id"] ?? null;
        	    $this->user = is_scalar($user_id) ? $user_id : 0;
                $myrow_email = $myrow["email"] ?? null;
                $this->email = is_scalar($myrow_email) ? $myrow_email : '';
		    	update_user_visitdate($this->username);
		    	$this->logged = true;
		    	$this->login_attempt=0;
				$this->last_act = time();
				$this->timeout = session_timeout();
				flush_dir(user_js_cache()); // refresh cache on login
			}
		}
		return $this->logged;
	}

	/** @return bool */
	function reset_password(int|string $company, string|int|float|bool|null $email) {

		$this->set_company((int) $company);
		$this->logged = false;

		set_global_connection();

		$user = get_user_by_email($email);

		if ($user != false) {

			$password = generate_password();
			$hash = md5($password);

			update_user_password($user['id'], $user['user_id'], $hash);

			$sender = get_company_pref('email');
			if (!(bool) $sender)
				$header = "";
			else
				$header = "From: $sender";

			mail((string) $email, _("New password for")." ".sysprefs()->app_title, $password, $header);

			return true;
		}
   		return false;
    }

	/** @return bool */
	function check_user_access()
	{
		global $security_groups;
		if ($this->old_db) {
			// notification after upgrade from pre-2.2 version
			$access_key = is_scalar($this->access) ? $this->access : null;
			return isset($security_groups) && is_array($security_groups[$access_key] ?? null);
		} else
			return !isset($security_groups) && is_array($this->role_set);
	}

	/** @return bool */
	function can_access(string|int|float|bool|null $sec_area)
	{
		global $security_groups, $security_areas;
		if (isset($security_groups)) {
			// $security_groups is the same deliberately-untyped pre-2.2 legacy array
			// documented in login() above
			$access_key = is_scalar($this->access) ? $this->access : null;
			return is_admin_company() &&
				in_array(20, $security_groups[$access_key] ?? array());
		}

		if ($sec_area === 'SA_OPEN')
			return true;
		if ($sec_area === 'SA_DENIED' || $sec_area === '')
			return false;

		$area_key = is_scalar($sec_area) ? (string) $sec_area : '';
		$code = $security_areas[$area_key][0] ?? null;

		// only first registered company has site admin privileges
		return (bool) $code && is_array($this->role_set) && in_array($code, $this->role_set)
			&& ($this->company == 0 || (($code&~0xff) != SS_SADMIN));
	}

	/** @return bool */
	function can_access_page(string|int|float|bool|null $page_level)
	{
		return $this->can_access($page_level);
	}

	/** @return bool */
	function check_application_access(application $waapp)
	{
		if (!$this->hide_inaccessible_menu_items())
		{
			return true;
		}

		foreach ($waapp->modules as $module)
		{
			if ($this->check_module_access($module))
			{
				return true;
			}
		}

		return false;

	}

	/** @return bool */
	function check_module_access(module $module)
	{

		if (!$this->hide_inaccessible_menu_items())
		{
			return true;
		}

		if (sizeof($module->lappfunctions) > 0)
		{
			foreach ($module->lappfunctions as $appfunction)
			{
				if ($appfunction->label != "" && $this->can_access_page($appfunction->access))
				{
					return true;
				}
			}
		}

		if (sizeof($module->rappfunctions) > 0)
		{
			foreach ($module->rappfunctions as $appfunction)
			{
				if ($appfunction->label != "" && $this->can_access_page($appfunction->access))
				{
					return true;
				}
			}
		}

		return false;

	}

	/** @return bool */
	function hide_inaccessible_menu_items()
	{

		if (!isset(sysprefs()->hide_inaccessible_menu_items) || sysprefs()->hide_inaccessible_menu_items == 0)
		{
			return false;
		}

		else
		{
			return true;
		}
	}

	/** @return false|mysqli */
	function set_db_connection(string|int|float|bool|null $id = -1)
	{
		return set_global_connection($id);
	}

	/**
	 * @param int|array<array-key, mixed>|null $prefs
	 * @return void
	 */
	function update_prefs($prefs)
	{

		if (!sysprefs()->allow_demo_mode) {
			update_user_prefs($this->user, is_array($prefs) ? $prefs : array());
		}

		$this->prefs = new user_prefs(get_user($this->user));
	}
}

//--------------------------------------------------------------------------

/** @psalm-pure */
function round2(string|int|float|bool|null $number, string|int|float|bool|null $decimals=0): float
{
	$number = (float)$number;
	$delta = ($number < 0 ? -.0000000001 : .0000000001);
	return round($number+$delta, (int) $decimals);
}

/*
	Returns number formatted according to user setup and using $decimals digits after dot 
	(defualt is 0). When $decimals is set to 'max' maximum available precision is used 
	(decimals depend on value) and trailing zeros are trimmed.
*/
function number_format2(string|int|float|bool|null $number, int|string $decimals=0): string
{
	$tsep = sysprefs()->thoseps[user_tho_sep()];
	$dsep = sysprefs()->decseps[user_dec_sep()];

	$number = (float)$number;
	if($decimals==='max')
		$dec = 15.0 - floor(log10(abs($number)));
	else {
		$delta = ($number < 0 ? -.0000000001 : .0000000001);
		$number += $delta;
		$dec = $decimals;
	}

	$num = number_format($number, intval($dec), $dsep, $tsep);

	return $decimals==='max' ? rtrim($num, '0') : $num;

}

/* price/float comparision helper to be used in any suspicious place for zero values? 
usage:
if (!floatcmp($value1, $value2)) 
	compare value is 0
*/

define('FLOAT_COMP_DELTA', 0.004);

/** @psalm-pure */
function floatcmp(string|int|float|bool|null $a, string|int|float|bool|null $b): int
{
    $a = (float)$a;
    $b = (float)$b;
    return $a - $b > FLOAT_COMP_DELTA ? 1 : ($b - $a > FLOAT_COMP_DELTA ? -1 : 0);
}

//
//	Current ui mode.
//
function fallback_mode(): bool {
    return session_obj('wa_current_user')->ui_mode==0;
}

function price_format(string|int|float|bool|null $number): string {
    return number_format2($number, user_price_dec());
}

function price_decimal_format(string|int|float|bool|null $number, int|string|null &$dec): string
{
	$dec = user_price_dec();
	$str = strval($number);
	$pos = strpos($str, '.');
	if ($pos !== false)
	{
		$len = strlen(substr($str, $pos + 1));
		if ($len > $dec && $len < (int)ini_get('precision')-3)
			$dec = $len;
	}
	return number_format2($number, $dec);
}

// 2008-06-15. Added extra parameter $stock_id and reference for $dec
//--------------------------------------------------------------------
function qty_format(string|int|float|bool|null $number, string|int|float|bool|null $stock_id, int|string|null &$dec): string {
	$dec = get_qty_dec(is_string($stock_id) ? $stock_id : null);
    return number_format2($number, $dec);
}

// and get_qty_dec
function get_qty_dec(?string $stock_id=null): int|string
{
	global $path_to_root;
	include_once(dirname(__DIR__) . "/inventory/includes/db/items_units_db.inc");
	$dec = null;
	if ($stock_id != null)
		$dec = get_unit_dec($stock_id);
	if ($stock_id == null || $dec == -1 || $dec == null)
		$dec = user_qty_dec();
	return $dec;
}
//-------------------------------------------------------------------
//
//	Maximum precision format. Strips trailing unsignificant digits.
//
function maxprec_format(string|int|float|bool|null $number): string {
    return number_format2($number, 'max');
}

function exrate_format(string|int|float|bool|null $number): string {
    return number_format2($number, user_exrate_dec());
}

function percent_format(string|int|float|bool|null $number): string {
    return number_format2($number, user_percent_dec());
}

function user_numeric(string|int|float|bool|null $input): int|float|false {

    $num = trim((string) $input);
    $sep = sysprefs()->thoseps[user_tho_sep()];
    if ($sep!='')
    	$num = str_replace( $sep, '', $num);

    $sep = sysprefs()->decseps[user_dec_sep()];
    if ($sep!='.')
    	$num = str_replace( $sep, '.', $num);

    if (!is_numeric($num))
	  	return false;
    $num = (float)$num;
    $int_num = (int) $num;
    if ($num == $int_num)
	  	return $int_num;
    else
	  	return $num;
}

/** @return null|scalar */
function user_company()
{
	global $def_coy;
	
	return isset($_SESSION["wa_current_user"]) ? session_obj('wa_current_user')->company : $def_coy;
}

/** @return null|scalar */
function user_pos()
{
	return session_obj('wa_current_user')->pos;
}

/** @return string */
function user_language()
{
	return session_obj('wa_current_user')->prefs->language();
}

function user_qty_dec(): int|string
{
	if (!isset($_SESSION["wa_current_user"])) {
		return 2;
	}
	$dec = session_obj('wa_current_user')->prefs->qty_dec();
	return is_int($dec) || is_string($dec) ? $dec : (int)($dec ?? 2);
}

function user_price_dec(): int|string
{

	if (!isset($_SESSION["wa_current_user"])) {
		return 2;
	}
	$dec = session_obj('wa_current_user')->prefs->price_dec();
	return is_int($dec) || is_string($dec) ? $dec : (int)($dec ?? 2);
}

function user_exrate_dec(): int|string
{
	if (!isset($_SESSION["wa_current_user"])) {
		return 4;
	}
	$dec = session_obj('wa_current_user')->prefs->exrate_dec();
	return is_int($dec) || is_string($dec) ? $dec : (int)($dec ?? 4);
}

function user_percent_dec(): int|string
{
	if (!isset($_SESSION["wa_current_user"])) {
		return 1;
	}
	$dec = session_obj('wa_current_user')->prefs->percent_dec();
	return is_int($dec) || is_string($dec) ? $dec : (int)($dec ?? 1);
}

/** @return null|scalar */
function user_show_gl_info()
{
	return session_obj('wa_current_user')->prefs->show_gl_info();
}

/** @return null|scalar */
function user_show_codes()
{
	return session_obj('wa_current_user')->prefs->show_codes();
}

/** @return null|scalar */
function user_date_format()
{

	return isset($_SESSION["wa_current_user"]) ? session_obj('wa_current_user')->prefs->date_format() : sysprefs()->dflt_date_fmt;
}

function user_date_display(): void
{
 	$fmt ='m/d/Y';
 	if (isset($_SESSION["wa_current_user"])) {
  		$fmt = session_obj('wa_current_user')->prefs->date_display();
 	} else {
  		// was (string) user_date_sep() -- that casts the *preference index* (0/1/2/3)
  		// straight to a string and uses it as the separator itself, instead of looking
  		// up the actual separator character from sysprefs()->dateseps, the way every
  		// other caller of user_date_sep() in this codebase does (and the way the real
  		// user_prefs::date_display() this function falls back from already does)
  		$sep = sysprefs()->dateseps[(int) user_date_sep()] ?? '/';
  		$user_date_fmt = user_date_format();
  		switch ($user_date_fmt) {
   			case 0:
    			$fmt = "m".$sep."d".$sep."Y"; break;
   			case 1:
    			$fmt = "d".$sep."m".$sep."Y"; break;
   			case 2:
    			$fmt = "Y".$sep."m".$sep."d"; break;
   			case 3:
    			$fmt = "M".$sep."j".$sep."Y"; break;
   			case 4:
    			$fmt = "j".$sep."M".$sep."Y"; break;
   			default:
    			$fmt = "Y".$sep."M".$sep."j";
  		}
  	}
}

/** @return scalar */
function user_date_sep()
{

	return (isset(session_obj('wa_current_user')->prefs->date_sep) ? session_obj('wa_current_user')->prefs->date_sep() : sysprefs()->dflt_date_sep) ?? '/' ;
}

function user_tho_sep(): int
{
	if (!isset($_SESSION["wa_current_user"])) {
		return 0;
	}
	return (int) session_obj('wa_current_user')->prefs->tho_sep();
}

function user_dec_sep(): int
{
	if (!isset($_SESSION["wa_current_user"])) {
		return 0;
	}
	return (int) session_obj('wa_current_user')->prefs->dec_sep();
}

function user_theme() : string
{
	return isset($_SESSION["wa_current_user"]) ? session_obj('wa_current_user')->prefs->get_theme() : 'default';
}

function user_pagesize(): int|string
{
	$pagesize = session_obj('wa_current_user')->prefs->get_pagesize();
	return is_int($pagesize) || is_string($pagesize) ? $pagesize : (int)($pagesize ?? 0);
}

/** @return null|scalar */
function user_hints()
{
	return session_obj('wa_current_user')->prefs->show_hints();
}

/** @return null|scalar */
function user_print_profile()
{
	return session_obj('wa_current_user')->prefs->print_profile();
}

/** @return null|scalar */
function user_rep_popup()
{
	return session_obj('wa_current_user')->prefs->rep_popup();
}

/** @return null|scalar */
function user_query_size()
{
	return session_obj('wa_current_user')->prefs->query_size();
}

/** @return null|scalar */
function user_graphic_links()
{
	return session_obj('wa_current_user')->prefs->graphic_links();
}

/** @return null|scalar */
function sticky_doc_date()
{
	return session_obj('wa_current_user')->prefs->sticky_date();
}

/** @return string */
function user_startup_tab()
{
	return session_obj('wa_current_user')->prefs->start_up_tab();
}

/** @return null|scalar */
function user_transaction_days()
{
    return session_obj('wa_current_user')->prefs->transaction_days();
}

/** @return null|scalar */
function user_save_report_selections()
{
    return session_obj('wa_current_user')->prefs->save_report_selections();
}

/** @return null|scalar */
function user_use_date_picker()
{
    return session_obj('wa_current_user')->prefs->use_date_picker();
}

/** @return null|scalar */
function user_def_print_destination()
{
    return session_obj('wa_current_user')->prefs->def_print_destination();
}

/** @return null|scalar */
function user_def_print_orientation()
{
    return session_obj('wa_current_user')->prefs->def_print_orientation();
}

/** @return bool */
function user_check_access(string|int|float|bool|null $sec_area)
{
	return session_obj('wa_current_user')->can_access($sec_area);
}

function set_user_prefs(int|array|null $prefs): void
{
	session_obj('wa_current_user')->update_prefs($prefs);
}

function add_user_js_data(): void {
	global $path_to_root;

	$ts = sysprefs()->thoseps[user_tho_sep()];
	$ds = sysprefs()->decseps[user_dec_sep()];

    $js = "\n"
	  . "var user = {\n"
	  . "theme: '". $path_to_root . '/themes/'. user_theme().'/'."',\n"
	  . "loadtxt: '"._('Requesting data...')."',\n"
	  . "date: '".Today()."',\n"	// server date
	  . "datesys: ".sysprefs()->date_system.",\n"
	  . "datefmt: ". (string) user_date_format().",\n"
	  . "datesep: '".(sysprefs()->dateseps[(int) user_date_sep()] ?? '/')."',\n"
	  . "ts: '$ts',\n"
	  . "ds: '$ds',\n"
	  . "pdec : " . user_price_dec() . "}\n";

  add_js_source($js);
}

function user_js_cache(string|int|float|bool|null $id=null): string
{
	global $path_to_root;

	if (!(bool)$id)
		$id = @session_obj('wa_current_user')->user;

	// DocblockTypeContradiction below is a Psalm false positive: current_user::$user
	// is declared int|string (session_obj()'s own type for this key), but genuinely
	// defaults to int 0 (falsy) before a real login happens -- this check is real and
	// necessary, not dead code
	if (!$id)
		$id = 0; // before login
	return $path_to_root.'/company/'. (string) user_company().'/js_cache/'. (string) $id;
}

//--------------------------------------------------------------------------

/** @return int|string */
function session_timeout()
{
	$tout = @get_company_pref('login_tout'); // mask warning for db ver. 2.2
	if ((bool) $tout && (is_int($tout) || is_string($tout)))
		return $tout;
	$gc_maxlifetime = ini_get('session.gc_maxlifetime');
	return $gc_maxlifetime === false ? 0 : $gc_maxlifetime;
}

//-----------------------------------------------------------------------------
//	Inserts $elements into $array at position $index.
//	$elements is list of any objects
//
function array_insert(array &$array, ?int $index, string|array|null $elements): void
{
	if (!is_array($elements)) $elements = array($elements);

	$head  = array_splice($array, 0, $index);
	$array = array_merge($head, $elements, $array);
}

/** @psalm-mutation-free */
function array_remove(array &$array, int $index, ?int $len=1): void
{
	array_splice($array, $index, $len);
}

function array_substitute(array &$array, int $index, ?int $len, ?string $elements): void
{
	array_splice($array, $index, $len);
	array_insert($array, $index, $elements);
}

function array_append(array &$array, ?array $elements): void
{
	// $key/$el are genuinely mixed by design: this generic array-builder helper's one
	// real caller (sales/inquiry/sales_orders_view.php) passes column-definition arrays
	// mixing plain strings and nested 'insert'/'fun' arrays
	foreach($elements ?? array() as $key => $el) {
		if(is_int($key))
			$array[] = $el;
		else
			$array[$key] = $el;
	}
}
//
//	Search $needle in $haystack or in $haystack[][$valuekey]
//	returns $needle found or null.
//
/**
 * @psalm-pure
 * @template T
 * @param array<array-key, T>|null $haystack
 * @return T|null
 */
function array_search_value(string|int|float|bool|null $needle, ?array $haystack, string|int|null $valuekey=null)
{
	if (is_array($haystack)) {
		foreach($haystack as $key => $value) {
			$val = isset($valuekey) ? @$value[$valuekey] : $value;
			if ($needle == $val){
				return $value;
			}
		}
	}
	return null;
}
//
//	Search $needle in $haystack or in $haystack[][$valuekey]
//	returns array of keys of $haystack elements found
//
/**
 * @psalm-pure
 * @return array<int, array-key>
 */
function array_search_keys(string|int|float|bool|null $needle, ?array $haystack, ?string $valuekey=null): array
{
	$keys = array();
	if (is_array($haystack)) {
		foreach($haystack as $key => $value) {
			$val = isset($valuekey) ? @$value[$valuekey] : $value;
			if ($needle == $val){
				$keys[] = $key;
			}
		}
	}	
	return $keys;
}
//
//	Find first (single) $needle in $haystack or in $haystack[][$valuekey]
//	returns $haystack element found or null
//
/**
 * @psalm-pure
 * @return array-key|null
 */
function array_search_key(string|int|float|bool|null $needle, ?array $haystack, ?string $valuekey=null)
{
	$keys = array_search_keys($needle, $haystack, $valuekey);
	return $keys[0] ?? null;
}

// Recalculate report columns if orientation is landscape.
function recalculate_cols(array &$cols): void
{
	$factor = (user_pagesize() == "A4" ? 1.4 : 1.3);
	// $col is genuinely mixed: shared by ~50 report files' own differently-shaped
	// column-width arrays, guarded with is_numeric() immediately below before use
	foreach($cols as $key => $col)
		$cols[$key] = intval((float) (is_numeric($col) ? $col : 0) * $factor);
}

function flush_dir(string $path, string|int|float|bool|null $wipe = false): void
{
	if (!file_exists($path))
		return;
	$dir = @opendir($path);
	if(!$dir)
		return;

	while(false !== ($fname = readdir($dir))) {
		if($fname=='.' || $fname=='..' || $fname=='CVS' || (!(bool)$wipe && $fname=='index.php')) continue;
  		if(is_dir($path.'/'.$fname)) {
		    flush_dir($path.'/'.$fname, $wipe);
		    if ((bool)$wipe) @rmdir($path.'/'.$fname);
		} else
		    @unlink($path.'/'.$fname);
	}
}
/*
	Returns current path to company private folder.
	(Current path can change after chdir).
*/
function company_path(string|int|float|bool|null $comp=null): string
{
	global $path_to_root;

	$comp_path = sysprefs()->comp_path;


	if (!isset($comp))
		$comp = user_company();

	// if path is relative, set current path_to_root
	return ($comp_path[0]=='.' ? $path_to_root.'/'.basename($comp_path) : $comp_path)
			. '/'. (string) $comp;
}

function is_admin_company(): bool
{
	return user_company() == 0;
}

