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
	Print request redirector. This file is fired via print link or 
	print button in reporting module. 
*/
/**
 * Both call sites strip everything outside the pattern's own whitelist before
 * the result ever reaches its sink (a cookie value, a Content-Disposition
 * filename, a file path or an include path below), so none of them can carry
 * a payload -- but Psalm's generic taint tracker has no way to recognize an
 * inline preg_replace()/str_replace() as a real sanitizer for these sink
 * kinds. Named, tagged wrappers are the documented way to tell it (see
 * html_encode()'s docblock in includes/ui/ui_lists.inc for the same pattern
 * applied to HTML output, and doc/PSALM_MIGRATION.md's "Missing taint-escape
 * annotations on real sanitizers" section).
 * @psalm-taint-escape header
 * @psalm-taint-escape has_quotes
 * @psalm-pure
 */
function safe_download_filename(string $s): string
{
	return str_replace(array('"', '\\', '/', "\r", "\n", ';'), '', $s);
}

/**
 * @psalm-taint-escape file
 * @psalm-taint-escape ssrf
 * @psalm-pure
 */
function safe_filename_chars(string $s): string
{
	return (string) preg_replace('/[^0-9_a-z.\-]/i', '', $s);
}

/**
 * @psalm-taint-escape include
 * @psalm-pure
 */
function safe_report_id(string $s): string
{
	return (string) preg_replace('/[^a-z_0-9]/i', '', $s);
}

$path_to_root = "..";
// InvalidGlobal below is a Psalm false positive: this file is always reached
// either as the direct target of a print/report-link request, or via
// reports_classes.inc's BoxReports::getDisplay(), which include()s it from
// inside a method -- a genuinely non-global scope where `global` is
// meaningful. Psalm can't trace a dynamic include() to know which case
// applies at this specific call site.
global $page_security;
$page_security = 'SA_OPEN';	// this level is later overriden in rep file
include_once(dirname(__DIR__) . "/includes/session.inc");

// the report id becomes part of a cookie name, so only a number is accepted
if ((int) user_save_report_selections() > 0 && isset($_POST['REP_ID']) && is_string($_POST['REP_ID']) && ctype_digit($_POST['REP_ID'])) {	// save parameters from Report Center
	for($i=0; $i<12; $i++) { // 2013-01-16 Joe Hunt
		$param = $_POST['PARAM_'.$i] ?? null;
		if ($param !== null && !is_array($param)) {
			$rep = $_POST['REP_ID'];
			// TaintedCookie below is by design, not a bug: this is the save side of
			// the Report Center's "remember my last parameters" feature, and the raw
			// submitted value is exactly what's meant to be restored later. The
			// corresponding read site (reporting/includes/reports_classes.inc,
			// getOptions()) HTML-encodes it before it reaches $_POST/any form output --
			// see that file's history for the XSS gap this closed on the read side.
			setcookie("select[$rep][$i]", (string) $param, time()+60*60*24*((int) user_save_report_selections())); // days from user_save_report_selections()
		}
	}
}

if (isset($_GET['xls']) || isset($_GET['xml']))
{
	// both values come from the query string: keep the download name free of quotes, slashes and line breaks
	$filename = isset($_GET['filename']) && is_string($_GET['filename'])
		? safe_download_filename($_GET['filename']) : 'report';
	$unique_name = isset($_GET['unique']) && is_string($_GET['unique'])
		? safe_filename_chars($_GET['unique']) : '';
	$path =  company_path(). '/pdf_files/';
	header("Content-type: ". (isset($_GET['xls']) ? "application/vnd.ms-excel" : "text/xml"));
	header("Content-Disposition: attachment; filename=\"$filename\"" );
	header("Expires: 0");
	header("Cache-Control: must-revalidate, post-check=0,pre-check=0");
	header("Pragma: public");
	echo file_get_contents($path.$unique_name);
	exit();
}

if (!isset($_POST['REP_ID'])) {	// print link clicked
	$def_pars = array(0, 0, '', '', 0, '', '', 0); //default values
	$rep_id_param = $_GET['REP_ID'] ?? '';
	$rep = $_POST['REP_ID'] = is_scalar($rep_id_param) ? $rep_id_param : '';
	for($i=0; $i<8; $i++) {
		$_POST['PARAM_'.$i] = isset($_GET['PARAM_'.$i])
			? $_GET['PARAM_'.$i] : $def_pars[$i];
	}
}

$rep_id = $_POST['REP_ID'] ?? '';
$rep = safe_report_id(is_scalar($rep_id) ? (string) $rep_id : '');

$rep_file = find_custom_file("/reporting/rep$rep.php");

if ((bool)$rep_file) {
	// UnresolvableInclude here is expected: $rep_file is necessarily dynamic
	// (it names whichever rep*.php report the user picked), so Psalm can't
	// follow it to analyse the included file from this call site -- it
	// still analyses every reporting/rep*.php on its own, as its own entry
	// point elsewhere in this sweep. $rep itself is already restricted to
	// [a-z_0-9] by safe_report_id() above (tagged @psalm-taint-escape
	// include), and find_custom_file() only ever returns a path under
	// company_path()/an installed extension's own directory that passed
	// file_exists() -- no traversal is reachable through either step.
	require($rep_file);
} else
	display_error("Cannot find report file '$rep'");
exit();

