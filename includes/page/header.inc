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
	If no context is set current page/menu screen is selected.
*/
function help_url(mixed $context=null): string
{

	global $help_context;

	$country = session_obj('language')->code;
	$clean = 0;
	
	if ($context!=null)
	{
		$help_page_url = $context;
	}
	elseif (isset($help_context))
	{
		$help_page_url = $help_context;
	} else // main menu
	{
		$app = $_SESSION['sel_app'];
		$help_page_url = $_SESSION['App']->applications[$app]->help_context;
		$clean = 1;
	}

	if (@sysprefs()->old_style_help)
		$help_page_url = _($help_page_url);

	if ($clean)
		$help_page_url = access_string($help_page_url, true);

	return html_specials_encode(
		sysprefs()->help_base_url
		.urlencode(strtr(ucwords($help_page_url), array(' '=>'', '/'=>'', '&'=>'And')))
		.'&ctxhelp=1&lang='.$country);
}

/**
 * The style sheets of the page: the theme's, those added with add_css_file(), and an extra one for this page.
 *
 * @return list<string>
 */
function css_urls(string|int|float|bool|null $css = ''): array
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
	foreach (css_urls($css) as $url)
	{
		echo "<link href='$url' rel='stylesheet' type='text/css'> \n";
	}
}

/**
 * The script files of the page: FA's own (compressed and cached per user) and those added with add_js_ufile().
 *
 * @return array{static: list<string>, user: list<string>}
 */
function script_files(): array
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
function script_urls(): array
{
	$files = script_files();
	return array_merge($files['static'], $files['user']);
}
function send_scripts(): void
{
	$js ='';
	foreach(script_urls() as $url)
	{
	  	$js .= '<script language="javascript" type="text/javascript" src="'.
			$url . '"></script>';
	}

    echo $js;
}
include_once($path_to_root . "/includes/yii/layout.inc");

function page_header(string|int|float|bool|null $title, string|int|float|bool|null $no_menu=false, string|int|float|bool|null $is_index=false, string|int|float|bool|null $onload="", string|int|float|bool|null $js="", string|int|float|bool|null $css=''): void
{
	// titles and screen header
	global $path_to_root, $SysPrefs, $db_connections;

	if (in_ajax() || isset($_POST['REP_ID']))
		return; // just for speed up

	$theme = user_theme();

	if (sysprefs()->help_base_url != null && sysprefs()->use_popup_windows && $js == '')
	{
		if (!function_exists('get_js_open_window'))
		{
			include_once($path_to_root.'/includes/ui/ui_view.inc');
		}
		add_js_source(get_js_open_window(900, 500));
	}
	if ($js!='')
	  	add_js_source($js);

	if (!isset($no_menu))
	{
		$no_menu = false;
	}
	if (isset($_SESSION["App"]) && is_object($_SESSION["App"]) && isset($_SESSION["App"]->selected_application) &&
		$_SESSION["App"]->selected_application != "")
		$sel_app = $_SESSION["App"]->selected_application;
	elseif (isset($_SESSION["sel_app"]) && $_SESSION["sel_app"] != "")
		$sel_app = $_SESSION["sel_app"];
	else
		$sel_app = user_startup_tab();
	$_SESSION["sel_app"] = $sel_app;

	// When startup tab for current user was set to already 
	// removed/inactivated extension module select Sales tab as default.
	if (isset($_SESSION["App"]) && is_object($_SESSION["App"]))
		$_SESSION["App"]->selected_application = 
			isset($_SESSION["App"]->applications[$sel_app]) ? $sel_app : 'orders';

	$page_header_args = func_get_args();
	if(isset($db_connections))	// skip before installation
		hook_invoke_all('pre_header', $page_header_args);

	$encoding = session_obj('language')->encoding;

	if (!headers_sent()){
		header("Content-type: text/html; charset=$encoding");
	}
	if (yii_layout_enabled())
	{
		// prototype: the document shell comes from views/layout.php (yiisoft/view)
		echo yii_layout_parts((string)$title, (string)$onload, (string)$css, true)[0];
	}
	else
	{
		echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";
		echo "<html dir='" . session_obj('language')->dir . "' >\n";
		echo "<head profile=\"http://www.w3.org/2005/10/profile\"><title>$title</title>";
		if (strpos($_SERVER['HTTP_USER_AGENT'], 'Trident/7.0; rv:11.0') != -1) // IE 11 doesn't work with input file type in form.
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

	include_once($path_to_root . "/themes/$theme/renderer.php");
	$rend = new renderer();
	$rend->menu_header($title, $no_menu, $is_index);
	error_box();
}

