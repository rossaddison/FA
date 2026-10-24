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
//
// FrontAccounting extension modules integration.
// This file is included in session.inc even before session is started,
// and includes hooks.php connector files from all installed extensions.
// To make hooks active install_hooks() have to be called after interface
// language is set.
//
// To find how various hooks are processed look into respective hook_* functions below.
//
/**
 * Do NOT mark this class final. Psalm's ClassMustBeFinal suggests it because
 * nothing in the scanned tree extends hooks - but modules/import_transactions/
 * hooks.php's hooks_import_transactions does (overriding install_options() and
 * install_access()), and modules/ is deliberately out of Psalm's scope (see
 * psalm.xml's ignoreFiles). Marking a base class final when a real module
 * extends it has already broken this codebase once before (Cart, see
 * doc/LIVE_CRASHES_FROM_TYPE_NARROWING.md) - same bug class, don't repeat it.
 * Left as an accepted, visible ClassMustBeFinal finding rather than
 * suppressed, per this project's standing no-@psalm-suppress policy.
 */
class hooks {
	/** @var string|null */
	var $module_name; // extension module name.
	/** @var string|null */
	var $path = null;
	// 
	// Helper for updating databases with extension scheme
	//
	// $comp can be company number, -1 for all, 
	// $updates - table of filename => array(table, field, property)
	// $check_only - don't update database, check table/field/property existence only
	//
	/**
	 * @param array<string, array{0?: string, 1?: string, 2?: array<array-key, mixed>|null}> $updates
	 * @return bool
	 */
	function update_databases(int $comp, array $updates, bool $check_only=false)
	{
		global $db_connections, $path_to_root;

		if ($comp == -1)
			$conn = $db_connections;
		else
			$conn = array( $comp => $db_connections[$comp]);
		$result = true;

		foreach($conn as $comp => $con) {
			set_global_connection($comp);
			foreach($updates as $file => $update) {
				$table = $update[0] ?? null;
				$field = $update[1] ?? null;
				/** @var array<string, string|int|float|bool|null>|null $properties */
				$properties = $update[2] ?? null;

				$ok = check_table($con['tbpref'], $table, $field, $properties) == 0;

				if (!$check_only && !$ok) {
					$ok = (bool) db_import((string)$path_to_root.'/modules/'.(string)$this->module_name.'/sql/'.$file,
						$con);
				}
				if (!$ok)
					$result = false;
				if (!$result)
					break;
			}
			db_close();
			if (!$result)
				break;
		}
		set_global_connection(0); // return to siteadmin account

		return $result;
	}
	//
	//	Install additional tabs provided by extension
	//
	/**
	 * @psalm-mutation-free
	 * @return void
	 */
	function install_tabs(front_accounting $app): void
	{
//		$app->add_application(new example_class); // add menu tab defined by example_class
	}
	//
	//	Install additonal menu options provided by extension
	//
	/**
	 * @psalm-mutation-free
	 * @return void
	 */
	function install_options(application $app): void
	{
//		global $path_to_root;
//
//		switch($app->id) {
//			case 'orders':
//				$app->add_rapp_function( 0, _("&Example option"), 
//					$path_to_root.'/modules/example/example.php?', 'SA_OPEN');
//		}
	}
	
	/**
	 * @psalm-mutation-free
	 * @return void
	 */
	function install_access()
	{
//		$security_areas['SA_EXAMPLE'] = array(SS_EXAMPLE|100, _("Example security area."));
//
//		$security_sections = array(SS_EXAMPLE => _("Example module implementation"));
//
//		return array($security_areas, $security_sections);
	}

	//
	//	Invoked for all modules before page header is displayed
	//
	/**
	 * @param array<array-key, mixed> $fun_args
	 * @psalm-mutation-free
	 * @return void
	 */
	function pre_header(array $fun_args): void
	{
	}
	//
	//	Invoked for all modules before page footer is displayed
	//
	/**
	 * @param array<array-key, mixed> $fun_args
	 * @psalm-mutation-free
	 * @return void
	 */
	function pre_footer(array $fun_args): void
	{
	}

	//
	// Price in words. $doc_type is set to document type and can be used to suppress 
	// price in words printing for selected document types.
	// Used instead of built in simple english price_in_words() function.
	//
	//	Returns: amount in words as string.

	/**
	 * @psalm-mutation-free
	 * @return void
	 */
	function price_in_words(string|int|float|null $amount, mixed $doc_type): void
	{
	}

	//
	// Exchange rate currency $curr as on date $date.
	// Keep in mind FA has internally implemented 3 exrate providers
	// If any of them supports your currency, you can simply use function below
	// with apprioprate provider set, otherwise implement your own.
	// Returns: $curr value in home currency units as a real number.

	/**
	 * @psalm-pure
	 * @return null
	 */
	function retrieve_exrate(?string $curr, ?string $date): null
	{
//	 	$provider = 'ECB'; // 'ECB', 'YAHOO' or 'GOOGLE'
//		return get_extern_rate($curr, $provider, $date);
		return null;
	}

	// External authentication
	// If used should return true after successfull athentication, false otherwise.
	/**
	 * @psalm-pure
	 * @return null
	 */
	function authenticate(?string $login, ?string $password): null
	{
		return null;
	}
	// Generic function called at the end of Tax Report (report 709)
	// Can be used e.g. for special database updates on every report printing
	// or to print special tax report footer 
	//
	// Returns: nothing
	/**
	 * @psalm-mutation-free
	 * @return void
	 */
	function tax_report_done(): void
	{
	}
	// Following database transaction hooks akcepts array of parameters:
	// 'cart' => transaction data
	// 'trans_type' => transaction type

	/**
	 * @psalm-pure
	 * @return true
	 */
	function db_prewrite(mixed &$cart, string|int|float|bool|null $trans_type): true
	{
		return true;
	}

	/**
	 * @psalm-pure
	 * @return true
	 */
	function db_postwrite(mixed &$cart, string|int|float|bool|null $trans_type): true
	{
		return true;
	}

	/**
	 * @psalm-pure
	 * @return true
	 */
	function db_prevoid(string|int|float|bool|null $trans_type, string|int|float|bool|null $trans_no): true
	{
		return true;
	}
	/*
		This method is called after module install.
	*/
	/**
	 * @psalm-pure
	 * @return true
	 */
	function install_extension(bool $check_only=true): true
	{
		return true;
	}
	/*
		This method is called after module uninstall.
	*/
	/**
	 * @psalm-pure
	 * @return true
	 */
	function uninstall_extension(bool $check_only=true): true
	{
		return true;
	}
	/*
		This method is called on extension activation for company.
	*/
	/**
	 * @psalm-pure
	 * @return true
	 */
	function activate_extension(string|int|float|bool|array|null $company, bool $check_only=true): true
	{
		return true;
	}
	/*
		This method is called when extension is deactivated for company.
	*/
	/**
	 * @psalm-pure
	 * @return true
	 */
	function deactivate_extension(string|int|float|bool|array|null $company, bool $check_only=true): true
	{
		return true;
	}

	/*
	 * Returns the quantity allowed to be dispatched for a particular item 
	 * and a status (which can be used to style the row).
	 * This quantity would be the default value on the delivery note.
	 * The usual use case for this is when a item is in stock,
	 * but has been reserved by someone else.
	 * This allows extensions to implements its own priority algorithm.
	 * This function is by detail_id and not item in case the item is present
	 * more than one in  the cart.
	 */
	/* Default behavior check if there is enough quantity on hand and change the css
 	 * class if needed */
	/**
	 * @param string|int|float|bool|array|null $location
	 * @param string|int|float|bool|array|null $date
	 * @param string|int|float|bool|null $qoh
	 * @return list{mixed, 'stockmankobg'}|true
	 */
	static function  default_get_dispatchable_quantity(line_details $line_item, $location, $date, $qoh) {

		if ((bool)sysprefs()->allow_negative_stock() || ($line_item->qty_dispatched <= $qoh)) {
			return true;
		}
		return array($qoh, 'stockmankobg');
	}

}

/*
	Installs hooks provided by extension modules
*/
function install_hooks(string|int|float|bool|null $lang_code=null): void
{
	global $path_to_root, $hooks, $installed_extensions;

	$hooks = array();
	// include current language related $hooks object if locale file exists
	if (!(bool)$lang_code)
		$lang_code = session_obj('language')->code;
	$lang_code = clean_file_name($lang_code);
	$lang_code = is_array($lang_code) ? '' : (string) $lang_code;

	if (file_exists($path_to_root . "/lang/" . $lang_code . "/locale.inc"))
	{
		include_once($path_to_root . "/lang/" . $lang_code . "/locale.inc");
		$code = $lang_code;
		$hook_class = 'hooks_'.$code;
		/**
		 * @var class-string<hooks> $hook_class
		 * UnsafeInstantiation left as an accepted, visible finding (no @psalm-suppress,
		 * per this project's standing policy): extension/locale hook classes are plain
		 * data holders with no constructor of their own, trusted by design, but hooks
		 * can't be marked final (see its own class docblock), so Psalm can't rule out
		 * an unsafe subclass constructor through this dynamic class-name lookup.
		 */
		$hooks[$code] = new $hook_class;
		$hooks[$code]->path = "/lang/" . $lang_code . "/locale.inc";
		unset($code, $hook_class);
	}
	// install hooks provided by active extensions
	if (!empty($installed_extensions)) {
		foreach($installed_extensions as $ext) {
			$package = (string) ($ext['package'] ?? '');
			$hook_class = 'hooks_'.$package;
			if ((bool)($ext['active'] ?? false) && class_exists($hook_class)) {
				/**
				 * @var class-string<hooks> $hook_class
				 * UnsafeInstantiation left as an accepted, visible finding -- same
				 * reasoning as install_hooks()'s other instantiation above.
				 */
				$hooks[$package] = new $hook_class;
				$ext_path = $ext['path'] ?? null;
				$hooks[$package]->path = is_string($ext_path) ? $ext_path : null;
			}
		}
	}
}
/*
	Non active hooks are not included in $hooks array, so we can use special function to
	activate.
*/
function activate_hooks(string|int|float|bool|null $ext, string|int|float|bool|array|null $comp, bool $on=true): mixed
{
	global $hooks;

	$ext = (string) $ext;
	if (isset($hooks[$ext])) {
		$hook = $hooks[$ext];
	} else {
		$hookclass = 'hooks_'.$ext;
		if (class_exists($hookclass)) {
			/**
			 * @var class-string<hooks> $hookclass
			 * UnsafeInstantiation left as an accepted, visible finding -- same
			 * reasoning as install_hooks()'s instantiation above.
			 */
			$hook = new $hookclass;
		} else
			return true; // extension does not have hooks file
	}
	if ($on)
		return $hook->activate_extension($comp, false);
	else
		return $hook->deactivate_extension($comp, false);
}
/*
	Calls hook $method defined in extension $ext (if any)
*/
/**
 * @template T
 * @param T $data
 * @param-out T $data
 */
function hook_invoke(string|int|float|bool|null $ext, string|int|float|bool|null $method, mixed &$data, ?array $opts=null): mixed
{

	global $hooks;

	$ext = (string) $ext;
	$method = (string) $method;
	$ret = null;
	if (isset($hooks[$ext]) && method_exists($hooks[$ext], $method)) {
		set_ext_domain($hooks[$ext]->path);
		$ret = $hooks[$ext]->$method($data, $opts);
		set_ext_domain();
	}
	return $ret;
}

/*
	Calls hook $methods defined in all extensions (if any)
*/
/**
 * @template T
 * @param T $data
 * @param-out T $data
 */
function hook_invoke_all(string|int|float|bool|null $method, mixed &$data, array|int|string|float|bool|null $opts=null): array
{

	global $hooks;

	$method = (string) $method;
	$return = array();
	if (isset($hooks)) {
		foreach($hooks as $ext => $hook)
			if (method_exists($hook, $method)) {
				set_ext_domain($hook->path);
				$result = $hook->$method($data, $opts);
				if (isset($result) && is_array($result)) {
					$return = array_merge_recursive($return, $result);
				} else if (isset($result)) {
					$return[] = $result;
				}
				set_ext_domain();
			}
	}
	return $return;
}
/*
	Returns first non-null result returned from hook.
*/
/**
 * @template T
 * @param T $data
 * @param-out T $data
 */
function hook_invoke_first(string $method, mixed &$data, ?array $opts=null): mixed
{

	global $hooks;

	$result = null;
	foreach($hooks ?? array() as $ext => $hook) {
		if (method_exists($hook, $method)) {
			set_ext_domain($hook->path);
			$result = $hook->$method($data, $opts);
			set_ext_domain();
			if (isset($result))
				break;
		}
	}
	return $result;
}
/*
	Returns last non-null result returned from modules method. Helps implement hooks overriding by
	extensions installed later.
*/
/**
 * @template T
 * @param T $data
 * @param-out T $data
 */
function hook_invoke_last(string $method, mixed &$data, mixed $opts=null): mixed
{

	global $hooks;

	$result = null;
	$reverse = array_reverse($hooks ?? array());
	foreach($reverse as $ext => $hook) {
		if (method_exists($hook, $method)) {
			set_ext_domain($hook->path);
			$result = $hook->$method($data, $opts);
			set_ext_domain();
			if (isset($result))
				break;
		}
	}
	return $result;
}
//------------------------------------------------------------------------------------------
//	Database transaction hooks.
//	$type - type of transaction (simplifies cart processing)
//	$cart - transaction cart
//	$args is optional array of parameters
//
// For FA 2.3 prewrite, postwrite and prevoid hooks are implemented for following transaction types:
//
// ST_BANKPAYMENT, ST_BANKDEPOSIT, ST_BANKTRANSFER,
// ST_SALESORDER, ST_SALESQUOTE, ST_SALESINVOICE, ST_CUSTCREDIT, ST_CUSTPAYMENT, ST_CUSTDELIVERY,
// ST_LOCTRANSFER, ST_INVADJUST, 
// ST_PURCHORDER, ST_SUPPINVOICE, ST_SUPPCREDIT, ST_SUPPAYMENT, ST_SUPPRECEIVE,
// ST_WORKORDER, ST_MANUISSUE, ST_MANURECEIVE, 

/*
	Invoked after transaction has been read from database to cart.
	Not implemented yet.
*/
//function hook_db_postread(&$cart, $type)
//{
//	hook_invoke_all('db_postread', $cart, $type);
//}

/*
	Invoked before transaction is written to database.
*/
/**
 * @template T
 * @param T $cart
 * @param-out T $cart
 * @return array<array-key, mixed>
 */
function hook_db_prewrite(&$cart, string|int|float|bool|null $type): array
{
	return hook_invoke_all('db_prewrite', $cart, $type);
}

/*
	Invoked after transaction has been written to database.
*/
/**
 * @template T
 * @param T $cart
 * @param-out T $cart
 * @return array<array-key, mixed>
 */
function hook_db_postwrite(&$cart, string|int|float|bool|null $type): array
{
	return hook_invoke_all('db_postwrite', $cart, $type);
}
/*
	Invoked before transaction is voided
*/
/**
 * @return array<array-key, mixed>
 */
function hook_db_prevoid(string|int|float|bool|null $type, string|int|float|bool|null $type_no): array
{
	return hook_invoke_all('db_prevoid', $type, $type_no);
}

//-------------------------------------------------------------------------------------------
//
//	Various hooks
//
//	Alternative exchange rates feeds.
//
function hook_retrieve_exrate(string|int|float|bool|null $currency, string|int|float|bool|null $date): mixed
{
	return hook_invoke_last('retrieve_exrate', $currency, $date);
}
//
// Generic function called at the end of Tax Report (report 709)
//
/**
 * @return array<array-key, mixed>
 */
function hook_tax_report_done(): array
{
	$dummy = null;
	return hook_invoke_all('tax_report_done', $dummy);
}
//
//	Amount in words displayed on various documents (especially sales invoice)
//
function hook_price_in_words(string|int|float|bool|null $amount, mixed $document): mixed
{
	return hook_invoke_last('price_in_words', $amount, $document);
}
//
//	Session handling hook. This is special case of hook class which have to be run before session is started.
//	If fa_session_manager class is defined in any installed extension, this class provides session handling
//	for application, otherwise standard php session handling is used.
//
function hook_session_start(string|array|null $company): mixed
{
	global $SessionManager;
	if (class_exists('fa_session_manager')) {
		$SessionManager = new fa_session_manager($company);
		return $SessionManager->installed;
	}
	return false;
}
//
//	Third party authentication modules.
//	Returns true after successfull authentication, false otherwise, null if no login hook is defined.
//
function hook_authenticate(string|int|float|bool|null $login, string|int|float|bool|array|null $password): mixed
{
	return hook_invoke_last('authenticate', $login, $password);
}

	/*
	 * Returns the quantity allowed to be dispatched for a particular item 
	 * and a "reason" (css classes).
	 * This quantity would be the default value on the delivery note.
	 * The usual use case for this is when a item is in stock,
	 * but has been reserved by someone else.
	 * This allows extensions to implements its own priority algorithm.
	 * This function is by detail_id and not item in case the item is present
	 * more than one in  the cart.
	 * If 'skip' is returned, the line will be skipped and not displayed
	 */
function hook_get_dispatchable_quantity(line_details $line_item, string|int|float|bool|array|null $location, string|int|float|bool|array|null $date, string|int|float|bool|null $qoh): mixed {
		$result =  hook_invoke_first('get_dispatchable_quantity', $line_item, array($location, $date, $qoh));
		return $result !== null ? $result : hooks::default_get_dispatchable_quantity($line_item, $location, $date, $qoh);
}
