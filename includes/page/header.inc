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
/*
	If no context is set current page/menu screen is selected.
*/
function help_url(mixed $context=null): string
{

	$country = session_obj('language')->code;
	$clean = 0;

	// $help_context may legitimately never have been set by the current page script; checked
	// against $GLOBALS directly (never bound via `global $help_context;`, which would itself
	// auto-vivify $GLOBALS['help_context'] to null and make this check always true) so this
	// doesn't depend on psalm.xml's stub type for the global, which only describes its type
	// once set, not whether it exists at all
	$has_help_context = array_key_exists('help_context', $GLOBALS) && $GLOBALS['help_context'] !== null;
	if ($context!=null)
	{
		$help_page_url = is_scalar($context) ? (string) $context : '';
	}
	elseif ($has_help_context)
	{
		$help_page_url = is_scalar($GLOBALS['help_context']) ? (string) $GLOBALS['help_context'] : '';
	} else // main menu
	{
		$app_id = $_SESSION['sel_app'] ?? '';
		// help_url() is only reachable from post-login menu rendering, by which point
		// includes/session.inc has already set $_SESSION['App'] -- the null fallback below is
		// purely defensive for the case Psalm can't rule out across that include boundary
		$fa = $_SESSION['App'] ?? null;
		if (!($fa instanceof front_accounting)) $fa = null;
		// fall back to the default tab if the stored app id is stale (e.g. a removed/inactivated extension)
		$application = $fa === null ? null : ($fa->get_application($app_id) ?? $fa->get_application('orders'));
		$help_page_url = $application?->help_context ?? '';
		$clean = 1;
	}

	if (@sysprefs()->old_style_help)
		$help_page_url = _($help_page_url);

	if ($clean)
		$help_page_url = access_string($help_page_url, true);

	return html_specials_encode(
		(string) sysprefs()->help_base_url
		.urlencode(strtr(ucwords($help_page_url), array(' '=>'', '/'=>'', '&'=>'And')))
		.'&ctxhelp=1&lang='.$country);
}

/**
 * The style sheets of the page: the theme's, those added with add_css_file(), and an extra one for this page.
 *
 * @return list<string>
 */
function cssUrls(string|int|float|bool|null $css = ''): array
{
	css_files_ensure_init();

	// after the call: Psalm forgets a global's type across a function call
	global $css_files;

	$urls = array();
	foreach ($css_files as $css_file)
	{
		$urls[] = $css_file;
	}
	if ((bool)$css)
	{
		$urls[] = (string)$css;
	}
	return $urls;
}

function send_css(string|int|float|bool|null $css = ''): void
{
	foreach (cssUrls($css) as $url)
	{
		echo "<link href='$url' rel='stylesheet' type='text/css'> \n";
	}
}

/**
 * The script files of the page: FA's own (compressed and cached per user) and those added with add_js_ufile().
 *
 * @return array{static: list<string>, user: list<string>}
 */
function scriptFiles(): array
{
	global $js_static, $js_path, $js_userlib;

	$static = array();
	foreach($js_static as $jsfile)
	{
		$cached_name = basename($jsfile);
		$fpath = user_js_cache().'/'.$cached_name;
	    // compress also static files
	  	if (!file_exists($fpath) || (bool)sysprefs()->go_debug)
			cache_js_file($fpath, file_get_contents($js_path.$jsfile));

		$static[] = $fpath;
	}
	$user = array();
	foreach($js_userlib as $jsfile)
	{
		$user[] = $jsfile;
	}
	return array('static' => $static, 'user' => $user);
}

/**
 * @return list<string> every script file of the page, FA's own first
 */
function scriptUrls(): array
{
	$files = scriptFiles();
	return array_merge($files['static'], $files['user']);
}
function send_scripts(): void
{
	$js ='';
	foreach(scriptUrls() as $url)
	{
	  	$js .= '<script language="javascript" type="text/javascript" src="'.
			$url . '"></script>';
	}

    echo $js;
}
include_once(dirname(__DIR__, 2) . "/includes/ui/ui_lists.inc");
include_once(dirname(__DIR__, 2) . "/includes/yii/layout.inc");

function page_header(string|int|float|bool|null $title, string|int|float|bool|null $no_menu=false, string|int|float|bool|null $is_index=false, string|int|float|bool|null $onload="", string|int|float|bool|null $js="", string|int|float|bool|null $css=''): void
{
	// titles and screen header
	// captured before `global $db_connections;` below, which would otherwise auto-vivify
	// $GLOBALS['db_connections'] to null and make the pre-install check below always true
	$db_connections_set = array_key_exists('db_connections', $GLOBALS);
	global $path_to_root, $SysPrefs, $db_connections;

	if (in_ajax() || isset($_POST['REP_ID']))
		return; // just for speed up

	$theme = user_theme();

	if (sysprefs()->help_base_url != null && sysprefs()->use_popup_windows && $js == '')
	{
		if (!function_exists('get_js_open_window'))
		{
			include_once(dirname(__DIR__, 2) . '/includes/ui/ui_view.inc');
		}
		add_js_source(get_js_open_window(900, 500));
	}
	if ($js!='')
	  	add_js_source($js);

	if (!isset($no_menu))
	{
		$no_menu = false;
	}
	// $fa is a plain non-front_accounting object here only if $_SESSION['App'] genuinely isn't
	// set yet -- real pages always have it by this point, but the instanceof check below
	// narrows this properly without relying on psalm.xml's $_SESSION stub (which can't rule
	// out $_SESSION['App'] being written with a different type somewhere else in the project)
	$fa = $_SESSION["App"] ?? null;
	if (!($fa instanceof front_accounting)) $fa = null;
	if ($fa !== null && isset($fa->selected_application) && $fa->selected_application != "")
		$sel_app = is_scalar($fa->selected_application) ? (string) $fa->selected_application : '';
	elseif (isset($_SESSION["sel_app"]) && $_SESSION["sel_app"] != "")
		$sel_app = is_scalar($_SESSION["sel_app"]) ? (string) $_SESSION["sel_app"] : '';
	else
		$sel_app = user_startup_tab();
	$_SESSION["sel_app"] = $sel_app;

	// When startup tab for current user was set to already
	// removed/inactivated extension module select Sales tab as default.
	if ($fa !== null)
		$fa->selected_application =
			isset($fa->applications[$sel_app]) ? $sel_app : 'orders';

	$page_header_args = func_get_args();
	if ($db_connections_set)	// skip before installation
		hook_invoke_all('pre_header', $page_header_args);

	$encoding = session_obj('language')->encoding;

	if (!headers_sent()){
		header("Content-type: text/html; charset=$encoding");
	}
	if (yiiLayoutEnabled())
	{
		// prototype: the document shell comes from views/layout.php (yiisoft/view)
		echo yiiLayoutParts((string)$title, (string)$onload, (string)$css, true)[0];
	}
	else
	{
		echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";
		echo "<html dir='" . session_obj('language')->dir . "' >\n";
		echo "<head profile=\"http://www.w3.org/2005/10/profile\"><title>" . html_encode($title) . "</title>";
		// bug: this used to compare against -1 (strpos()'s "not found" is false, not -1), so the
		// compat meta tag below was being echoed for every browser, not just the intended IE11 one
		if (strpos($_SERVER['HTTP_USER_AGENT'] ?? '', 'Trident/7.0; rv:11.0') !== false) // IE 11 doesn't work with input file type in form.
			echo "<meta http-equiv='X-UA-Compatible' content='IE=10'>\n";
		echo "<meta http-equiv='Content-type' content='text/html; charset=$encoding'>";
		echo "<link href='$path_to_root/themes/default/images/favicon.ico' rel='icon' type='image/x-icon'> \n";
	
		send_css($css);
	
		send_scripts();
	
		echo "</head> \n";
		if ($onload == "")
			echo "<body>";
		else
			echo "<body onload=\"$onload\">";
	}

	// TaintedInclude/UnresolvableInclude/UndefinedClass/MixedMethodCall here are the same
	// confirmed-safe, unfixable-in-Psalm finding already investigated for the identical
	// $path_to_root."/themes/".user_theme()."/renderer.php" pattern in frontaccounting.php
	// (see doc/BUGS_FOUND.md's "Path traversal / arbitrary file write" section) -- user_theme()
	// is sanitized through clean_file_name() before it's ever persisted, so this can't actually
	// escape the themes/ directory regardless of what Psalm can see across the session round-trip.
	include_once("$path_to_root/themes/$theme/renderer.php");
	$rend = new renderer();
	$rend->menu_header($title, $no_menu, $is_index);
	error_box();
}

