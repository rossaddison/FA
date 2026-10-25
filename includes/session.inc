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
define('VARLIB_PATH', $path_to_root.'/tmp');
define('VARLOG_PATH', $path_to_root.'/tmp');
define('SECURE_ONLY', false); // if you really need also http (unsecure) access allowed, you can set this to NULL

require_once dirname(__DIR__) . '/vendor/autoload.php'; // composer packages (e.g. yiisoft/html) used unconditionally, not just by the optional Yii layout

/**
 * Project-wide accessor for the typed session facade (src/Session/SessionInterface.php) -
 * a thin, incrementally-adoptable wrapper over the live $_SESSION superglobal,
 * not a replacement for it. See that interface's own docblock for the
 * reasoning. Mirrors session_obj()'s existing "one global accessor function"
 * shape (includes/prefs/sysprefs.inc) rather than introducing a DI container.
 *
 * @psalm-external-mutation-free
 */
function app_session(): \FrontAccounting\Session\SessionInterface
{
	/** @var \FrontAccounting\Session\SessionInterface|null $instance */
	static $instance = null;
	if ($instance === null) {
		$instance = new \FrontAccounting\Session\NativeSession();
	}
	return $instance;
}

final class SessionManager
{
   	/**
   	 * @return void
   	 */
   	function sessionStart(?string $name, int $limit = 0, ?string $path = '/', ?string $domain = null, ?bool $secure = null)
	{
		// Set the cookie name
		session_name($name);

		// Set SSL level
		$httpsVar = $_SERVER['HTTPS'] ?? null;
		$https = isset($secure) ? $secure : ((bool) $httpsVar && $httpsVar !== 'off');

		// Set session cookie options
		session_set_cookie_params($limit, $path, $domain, $https, true);

		session_start();

		// Make sure the session hasn't expired, and destroy it if it has
		if ($this->validateSession())
		{
			// Check to see if the session is new or a hijacking attempt
			if(!$this->preventHijacking())
			{
				// Reset session data and regenerate id
				$_SESSION = array();
				$_SESSION['IPaddress'] = $_SERVER['REMOTE_ADDR'] ?? '';
				$_SESSION['userAgent'] = $_SERVER['HTTP_USER_AGENT'] ?? '';
				$this->regenerateSession();

			// Give a 5% chance of the session id changing on any request
			}
			elseif (rand(1, 100) <= 5)
			{
				$this->regenerateSession();
			}
		}
		else
		{
			$_SESSION = array();
			session_destroy();
			session_start();
		}
	}

	/** @return bool */
	function preventHijacking()
	{
		if (!isset($_SESSION['IPaddress']) || !isset($_SESSION['userAgent']))
			return false;

		if ($_SESSION['IPaddress'] != ($_SERVER['REMOTE_ADDR'] ?? ''))
			return false;

		if ( $_SESSION['userAgent'] != ($_SERVER['HTTP_USER_AGENT'] ?? ''))
			return false;

		return true;
	}

	/** @return void */
	function regenerateSession()
	{
		// If this session is obsolete it means there already is a new id
		if (isset($_SESSION['OBSOLETE']) && ($_SESSION['OBSOLETE'] == true))
			return;

		// Set current session to expire in 10 seconds
		$_SESSION['OBSOLETE'] = true;
		$_SESSION['EXPIRES'] = time() + 10;

		// Create new session without destroying the old one
		session_regenerate_id();
		// Grab current session ID and close both sessions to allow other scripts to use them
		$newSession = session_id();
		session_write_close();
		// Set session ID to the new one, and start it back up again

		session_id($newSession !== false ? $newSession : null);
		session_start();
		
		// Now we unset the obsolete and expiration values for the session we want to keep
		unset($_SESSION['OBSOLETE']);
		unset($_SESSION['EXPIRES']);
	}

	/** @return bool */
	function validateSession()
	{
		if (isset($_SESSION['OBSOLETE']) && !isset($_SESSION['EXPIRES']) )
			return false;

		if (isset($_SESSION['EXPIRES']) && $_SESSION['EXPIRES'] < time())
			return false;

		return true;
	}
}

function output_html(string|int|float|bool|null $text): string
{
	global $before_box, $messages;
	// Fatal errors are not send to error_handler,
	// so we must check the output
	if ((bool)$text && preg_match('/\bFatal error(<.*?>)?:(.*)/i', (string) $text, $m)) {
		ajax()->aCommands = array();  // Don't update page via ajax on errors
		$text = preg_replace('/\bFatal error(<.*?>)?:(.*)/i','', (string) $text);
		$messages[] = array(E_ERROR, $m[2], null, null, '');
	}
	ajax()->run();
	return  in_ajax() ? fmt_errors() : ($before_box.fmt_errors().(string) $text);
}
//----------------------------------------------------------------------------------------

function kill_login(): void
{
	session_unset();
	session_destroy();
}
//----------------------------------------------------------------------------------------

function login_fail(): void
{
	global $path_to_root;

	header("HTTP/1.1 401 Authorization Required");
	echo "<center><br><br><font size='5' color='red'><b>" . _("Incorrect Password") . "<b></font><br><br>";
	echo "<b>" . _("The user and password combination is not valid for the system.") . "<b><br><br>";
	echo _("If you are not an authorized user, please contact your system administrator to obtain an account to enable you to use the system.");
	echo "<br><a href='$path_to_root/index.php'>" . _("Try again") . "</a>";
	echo "</center>";
	kill_login();
	die();
}

function password_reset_fail(): void
{
	global $path_to_root;
	
  	echo "<center><br><br><font size='5' color='red'><b>" . _("Incorrect Email") . "<b></font><br><br>";
  	echo "<b>" . _("The email address does not exist in the system, or is used by more than one user.") . "<b><br><br>";

  	echo _("Plase try again or contact your system administrator to obtain new password.");
  	echo "<br><a href='$path_to_root/index.php?reset=1'>" . _("Try again") . "</a>";
  	echo "</center>";

	kill_login();
	die();
}

function password_reset_success(): void
{
	global $path_to_root;

  	echo "<center><br><br><font size='5' color='green'><b>" . _("New password sent") . "<b></font><br><br>";
  	echo "<b>" . _("A new password has been sent to your mailbox.") . "<b><br><br>";

  	echo "<br><a href='$path_to_root/index.php'>" . _("Login here") . "</a>";
  	echo "</center>";
	
	kill_login();
	die();
}

/** @return bool */
function check_faillog()
{
	/** @var array<array-key, array<array-key, int|string>> $login_faillog */
	global $login_faillog;

	$user = session_obj('wa_current_user')->user;
	$ip = $_SERVER['REMOTE_ADDR'] ?? '';

	session_obj('wa_current_user')->login_attempt++;
	$attempts = @$login_faillog[$user][$ip];
	$last = @$login_faillog[$user]['last'];
	if ((bool)sysprefs()->login_delay && is_numeric($attempts) && $attempts >= sysprefs()->login_max_attempts
		&& is_numeric($last) && time() < (int) $last + sysprefs()->login_delay)
		return true;

	return false;
}

/*
	Ensure file is re-read on next request if php caching is active
*/
function cache_invalidate(string $filename): void
{
	if (function_exists('opcache_invalidate'))	// OpCode extension
		opcache_invalidate($filename);
}

/*
	Simple brute force attack detection is performed before connection to company database is open. Therefore access counters have to be stored in file.
	Login attempts counter is created for every new user IP, which partialy prevent DOS attacks.
*/
function write_login_filelog(string|int|float|bool|null $login, mixed $result): void
{
	/** @var array<array-key, array<array-key, int|string>> $login_faillog */
	global $login_faillog, $path_to_root;

	$user = session_obj('wa_current_user')->user;

	$ip = $_SERVER['REMOTE_ADDR'] ?? '';

	if (!isset($login_faillog[$user][$ip]) || $result) // init or reset on successfull login
		$login_faillog[$user] = array($ip => 0, 'last' => '');

 	if (!$result)
	{
		$attempts = $login_faillog[$user][$ip];
		if (is_numeric($attempts) && $attempts < sysprefs()->login_max_attempts) {

 			$login_faillog[$user][$ip] = (int) $attempts + 1;
 		} else {
 			$login_faillog[$user][$ip] = 0; // comment out to restart counter only after successfull login.
	 		error_log(sprintf(_("Brute force attack on account '%s' detected. Access for non-logged users temporarily blocked."	), (string) $login));
	 	}
 		$login_faillog[$user]['last'] = time();
	}

	$msg = "<?php\n";
	$msg .= "/*\n";
	$msg .= "Login attempts info.\n";
	$msg .= "*/\n";
	$msg .= "\$login_faillog = " .var_export($login_faillog, true). ";\n";

	$filename = VARLIB_PATH."/faillog.php";

	if ((!file_exists($filename) && is_writable(VARLIB_PATH)) || is_writable($filename))
	{
		file_put_contents($filename, $msg);
		cache_invalidate($filename);
	}
}

//----------------------------------------------------------------------------------------

function check_page_security(string|int|float|bool|null $page_security): void
{

	$msg = '';

	if (!session_obj('wa_current_user')->check_user_access())
	{
		// notification after upgrade from pre-2.2 version
		$msg = session_obj('wa_current_user')->old_db ?
			 _("Security settings have not been defined for your user account.")
				. "<br>" . _("Please contact your system administrator.")
			: _("Please remove \$security_groups and \$security_headings arrays from config.php file!");
	} elseif (!sysprefs()->db_ok && !session_obj('wa_current_user')->can_access('SA_SOFTWAREUPGRADE'))
	{
		$msg = _('Access to application has been blocked until database upgrade is completed by system administrator.');
	}

	if ($msg){
		display_error($msg);
		end_page((bool) ($_REQUEST['popup'] ?? null));
		kill_login();
		exit;
	}

	if (!session_obj('wa_current_user')->can_access_page($page_security))
	{

		echo "<center><br><br><br><b>";
		echo _("The security settings on your account do not permit you to access this function");
		echo "</b>";
		echo "<br><br><br><br></center>";
		end_page((bool) ($_REQUEST['popup'] ?? null));
		exit;
	}
	if (!sysprefs()->db_ok 
		&& !in_array($page_security, array('SA_SOFTWAREUPGRADE', 'SA_OPEN', 'SA_BACKUP')))
	{
		display_error(_('System is blocked after source upgrade until database is updated on System/Software Upgrade page'));
		end_page();
		exit;
	}

}
/*
	Helper function for setting page security level depeding on 
	GET start variable and/or some value stored in session variable.
	Before the call $page_security should be set to default page_security value.
*/
/**
 * @param array<array-key, string> $trans
 * @param array<array-key, string> $gtrans
 */
function set_page_security(string|int|float|bool|null $value=null, array $trans = array(), array $gtrans = array()): void
{
	global $page_security;

	// first check is this is not start page call
	foreach($gtrans as $key => $area)
		if (isset($_GET[$key])) {
			$page_security = $area;
			return;
		}

	// then check session value
	if ((is_string($value) || is_int($value)) && isset($trans[$value])) {
		$page_security = $trans[$value];
		return;
	}
}

/*
	htmlspecialchars does not support certain encodings.
	ISO-8859-2 fortunately has the same special characters positions as 
	ISO-8859-1, so fix is easy. If any other unsupported encoding is used,
	add workaround here.
*/
/** @psalm-taint-escape html */
function html_specials_encode(string|int|float|bool|null $str): string
{
	return htmlspecialchars((string) $str, ENT_QUOTES, session_obj('language')->encoding=='iso-8859-2' ?
		 'ISO-8859-1' : session_obj('language')->encoding);
}

function html_cleanup(array &$parms): void
{
	// $value is genuinely mixed: this sanitizes the real $_GET/$_POST/$_REQUEST/$_SERVER
	// superglobals in place, whose values are arbitrary by nature
	foreach($parms as $name => &$value) {
		if (is_array($value))
			html_cleanup($value);
		else
			$value = html_specials_encode(is_scalar($value) ? $value : null);
	}
	unset($value);
	reset($parms); // needed for direct key() usage later throughout the sources
}

//============================================================================
//
//
function login_timeout(): void
{
	// skip timeout on logout page
	if (session_obj('wa_current_user')->logged) {
		$tout = (int) session_obj('wa_current_user')->timeout;
		if ($tout && (time() > (int) session_obj('wa_current_user')->last_act + $tout))
		{
			session_obj('wa_current_user')->logged = false;
		}
		session_obj('wa_current_user')->last_act = time();
	}
}
//============================================================================
// checked via $GLOBALS rather than isset($path_to_root) so this doesn't depend on
// psalm.xml's non-nullable stub for the global, which only describes its value once a
// calling page script has set it, not whether it's been set at all. Safe here (no `global`
// statement for $path_to_root precedes this -- this file is only ever included at true
// top-level script scope, never from inside a function/method)
if (!array_key_exists('path_to_root', $GLOBALS))
{
	$path_to_root = ".";
}

// Prevent register_globals vulnerability
if (isset($_GET['path_to_root']) || isset($_POST['path_to_root']))
	die("Restricted access");

include_once(dirname(__DIR__) . "/includes/errors.inc");
// colect all error msgs
set_error_handler('error_handler' /*, errtypes */);
set_exception_handler('exception_handler');

include_once(dirname(__DIR__) . "/includes/current_user.inc");
include_once(dirname(__DIR__) . "/frontaccounting.php");
include_once(dirname(__DIR__) . "/admin/db/security_db.inc");
include_once(dirname(__DIR__) . "/includes/lang/language.inc");
include_once(dirname(__DIR__) . "/config_db.php");
include_once(dirname(__DIR__) . "/includes/ajax.inc");
include_once(dirname(__DIR__) . "/includes/ui/ui_msgs.inc");
include_once(dirname(__DIR__) . "/includes/prefs/sysprefs.inc");

include_once(dirname(__DIR__) . "/includes/hooks.inc");
//
// include all extensions hook files.
// UnresolvableInclude here is just Psalm being unable to statically resolve a
// dynamic path, not a tainted-input finding -- $ext_path comes from the
// installed_extensions config, not user input
//
foreach ($installed_extensions as $ext)
{
	$ext_path = (string) ($ext['path'] ?? '');
	if (file_exists($path_to_root.'/'.$ext_path.'/hooks.php'))
		include_once($path_to_root.'/'.$ext_path.'/hooks.php');
}

ini_set('session.gc_maxlifetime', 36000); // moved from below.

$Session_manager = new SessionManager();
$Session_manager->sessionStart('FA'.md5(dirname(__FILE__)), 0, '/', null, SECURE_ONLY);

$_SESSION['SysPrefs'] = new sys_prefs();

/** @var sys_prefs $SysPrefs */
$SysPrefs = &$_SESSION['SysPrefs'];

include_once(__DIR__ . "/security_headers.inc");
send_security_headers();

//----------------------------------------------------------------------------------------
// set to reasonable values if not set in config file (pre-2.3.12 installations)

if ((!isset(sysprefs()->login_delay)) || (sysprefs()->login_delay < 0))
    sysprefs()->login_delay = 10;

if ((!isset(sysprefs()->login_max_attempts)) || (sysprefs()->login_max_attempts < 0))
    sysprefs()->login_max_attempts = 3; 

if (sysprefs()->go_debug > 0)
	$cur_error_level = -1;
else
	$cur_error_level = E_USER_WARNING|E_USER_ERROR|E_USER_NOTICE;

error_reporting($cur_error_level);
ini_set("display_errors", "On");

if (sysprefs()->error_logfile != '') {
	ini_set("error_log", sysprefs()->error_logfile);
	ini_set("ignore_repeated_errors", "On");
	ini_set("log_errors", "On");
}

/*
	Uncomment the setting below when using FA on shared hosting
	to avoid unexpeced session timeouts.
	Make sure this directory exists and is writable!
*/
// ini_set('session.save_path', VARLIB_PATH.'/');

// ini_set('session.gc_maxlifetime', 36000); // 10hrs - moved to before session_manager

$company_login_name = @$_POST["company_login_name"];
hook_session_start(is_string($company_login_name) || is_array($company_login_name) ? $company_login_name : null);

// this is to fix the "back-do-you-want-to-refresh" issue - thanx PHPFreaks
header("Cache-control: private");

get_text_init();

// UnresolvableInclude here is just Psalm being unable to statically resolve a
// dynamic path, not a tainted-input finding -- VARLIB_PATH is a fixed,
// admin-controlled constant, never user input
if (sysprefs()->login_delay > 0 && file_exists(VARLIB_PATH."/faillog.php"))
	include_once(VARLIB_PATH."/faillog.php");

// Page Initialisation
// checked via $GLOBALS rather than isset() so this doesn't depend on psalm.xml's non-nullable
// stubs for these globals, which only describe their value once config_db.php (written during
// installation) has defined them, not whether they exist yet pre-installation. Safe here (no
// `global` statement for either precedes this -- top-level script scope only)
if (array_key_exists('dflt_lang', $GLOBALS) && array_key_exists('installed_languages', $GLOBALS))
{
	$session_language = $_SESSION['language'] ?? null;
	if (!isset($_SESSION['wa_current_user']) || !session_obj('wa_current_user')->logged_in()
		|| !isset($session_language) || !method_exists($session_language, 'set_language'))
	{
		$l = array_search_value($dflt_lang, $installed_languages,  'code');
		$l = is_array($l) ? $l : array();
		$_SESSION['language'] = new language($l['name'] ?? null, $l['code'] ?? null, $l['encoding'] ?? null,
	 	(isset($l['rtl']) && $l['rtl'] === true) ? 'rtl' : 'ltr');
	}

	session_obj('language')->set_language(session_obj('language')->code);
}

include_once(dirname(__DIR__) . "/includes/access_levels.inc");
include_once(dirname(__DIR__) . "/version.php");
include_once(dirname(__DIR__) . "/includes/main.inc");
include_once(dirname(__DIR__) . "/includes/app_entries.inc");

// Ajax communication object
$Ajax = new Ajax();

// js/php validation rules container
$Validate = array();
// bindings for editors
$Editors = array();
// page help. Currently help for function keys.
$Pagehelp = array();

$Refs = new references();

// intercept all output to destroy it in case of ajax call
register_shutdown_function('end_flush');
ob_start('output_html',0);

if (!isset($_SESSION["wa_current_user"]))
	$_SESSION["wa_current_user"] = new current_user();

html_cleanup($_GET);
html_cleanup($_POST);
/** @var array<array-key, mixed> $_REQUEST */
html_cleanup($_REQUEST);
/** @var array<array-key, mixed> $_SERVER */
html_cleanup($_SERVER);

// logout.php is the only page we should have always 
// accessable regardless of access level and current login status.
if (!defined('FA_LOGOUT_PHP_FILE')){

	login_timeout();

	// UnresolvableInclude here is just Psalm being unable to statically resolve a
	// dynamic path, not a tainted-input finding -- user_company() resolves to one
	// of this install's own configured company directories, not arbitrary user input
	if (!(bool)session_obj('wa_current_user')->old_db && file_exists($path_to_root . '/company/'.(string) user_company().'/installed_extensions.php'))
		include($path_to_root . '/company/'.(string) user_company().'/installed_extensions.php');

	install_hooks();

	if (!session_obj('wa_current_user')->logged_in())
	{
      if (@sysprefs()->allow_password_reset && !sysprefs()->allow_demo_mode
        && (isset($_GET['reset']) || isset($_POST['email_entry_field']))) {
		  if (!isset($_POST["email_entry_field"])) {
        include(dirname(__DIR__) . "/access/password_reset.php");
        exit();
      }
      else {
        if (isset($_POST["company_login_nickname"]) && !isset($_POST["company_login_name"])) {
          for ($i = 0; $i < count($db_connections); $i++) {
            if ($db_connections[$i]["name"] == $_POST["company_login_nickname"]) {
              $_POST["company_login_name"] = $i;
              unset($_POST["company_login_nickname"]);
              break 1; // cannot pass variables to break from PHP v5.4 onwards
            }
          }
        }
        /** @var string|int|float|bool|null $company_login_name */
        $company_login_name = $_POST["company_login_name"] ?? null;
        $company_login_name = is_int($company_login_name) || is_string($company_login_name) ? $company_login_name : null;
        $_succeed = $company_login_name !== null && isset($db_connections[$company_login_name]) &&
          session_obj('wa_current_user')->reset_password($company_login_name,
          $_POST["email_entry_field"]);
        if ($_succeed)
        {
          password_reset_success();
        }

        password_reset_fail();
      }
    }
		// Show login screen
		if (!isset($_POST["user_name_entry_field"]) or $_POST["user_name_entry_field"] == "")
		{
			// strip ajax marker from uri, to force synchronous page reload
			/** @var string|int|float|bool|null $request_uri */
			$request_uri = $_SERVER['REQUEST_URI'] ?? null;
			$_SESSION['timeout'] = array( 'uri'=>preg_replace('/JsHttpRequest=(?:(\d+)-)?([^&]+)/s',
					'', html_specials_encode($request_uri)),
				'post' => $_POST);
		if (in_ajax()) {
			/** @var string $path_to_root */
			ajax()->popup($path_to_root ."/access/timeout.php");
		}
		else
			include(dirname(__DIR__) . "/access/login.php");
			exit;
		} else {
			if (isset($_POST["company_login_nickname"]) && !isset($_POST["company_login_name"])) {
				for ($i = 0; $i < count($db_connections); $i++) {
					if ($db_connections[$i]["name"] == $_POST["company_login_nickname"]) {
						$_POST["company_login_name"] = $i;
						unset($_POST["company_login_nickname"]);
						break 1; // cannot pass variables to break from PHP v5.4 onwards
					}
				}
			}
			/** @var string|int|float|bool|null $company_login_name */
			$company_login_name = $_POST["company_login_name"] ?? null;
			$company_login_name = is_int($company_login_name) || is_string($company_login_name) ? $company_login_name : null;
			$succeed = $company_login_name !== null && isset($db_connections[$company_login_name]) &&
				session_obj('wa_current_user')->login($company_login_name,
				$_POST["user_name_entry_field"], $_POST["password"]);
			// select full vs fallback ui mode on login
			session_obj('wa_current_user')->ui_mode = (int) ($_POST['ui_mode'] ?? 0);
			/** @var array{uri?: string, post?: mixed}|null $timeout */
			$timeout = $_SESSION['timeout'] ?? null;
			if (!$succeed)
			{
			// Incorrect password
				if (isset($timeout)) {
					include(dirname(__DIR__) . "/access/login.php");
					exit;
				} else
					login_fail();
			}
			elseif(isset($timeout) && !(bool) ($timeout['post'] ?? null))
			{
				// in case of GET request redirect to avoid confirmation dialog
				// after return from menu option
				header("HTTP/1.1 307 Temporary Redirect");
				header("Location: ".($timeout['uri'] ?? ''));
				exit();
			}
			session_obj('language')->set_language(session_obj('language')->code);
		}
	} else
	{
		set_global_connection();

		if (db_fixed())
			db_set_encoding(session_obj('language')->encoding);

		sysprefs()->refresh();
	}
	if (!isset($_SESSION["App"])) {
		$_SESSION["App"] = new front_accounting();
		$_SESSION["App"]->init();
	}
}

// We quote all values later with db_escape() before db update.
