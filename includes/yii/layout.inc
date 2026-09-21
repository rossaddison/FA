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
	Prototype: the page shell (doctype, head, assets, body tags) rendered by yiisoft/view.

	Off unless $use_yii_layout is set in config.php. page_header() and page_footer() call these functions instead of
	echoing the shell themselves; everything between them (menus, forms, tables) is still echoed by FA.
*/

use Yiisoft\View\WebView;

include_once(__DIR__ . '/assets.inc');
include_once(__DIR__ . '/menu.inc');

/**
 * True when the Yii page shell is switched on and its dependencies are installed.
 */
function yii_layout_enabled(): bool
{
	return (bool)sysprefs()->use_yii_layout && is_file(dirname(__DIR__, 2) . '/vendor/autoload.php');
}

/**
 * The one view of the request. Its state (registered CSS and scripts) is cleared each time a layout is rendered.
 */
function yii_view(): WebView
{
	/** @var WebView|null $view */
	static $view = null;

	if ($view === null) {
		require_once dirname(__DIR__, 2) . '/vendor/autoload.php';
		$view = new WebView(dirname(__DIR__, 2) . '/views');
	}
	return $view;
}

/**
 * Renders the layout and splits it where the page body goes.
 *
 * @param string $title
 * @param string $onload JavaScript for <body onload>
 * @param string $css an extra style sheet for this page
 * @param bool $withAssets false for the closing half, which must not register the head assets a second time
 * @return array{0: string, 1: string} the markup before the body, and the markup after it
 */
function yii_layout_parts(string $title, string $onload, string $css, bool $withAssets): array
{
	global $path_to_root;

	$view = yii_view();
	if ($withAssets) {
		yii_register_assets($view, $css);
	}
	$marker = '<!--fa-page-content-->';
	$html = $view->render('layout', array(
		'title' => $title,
		'encoding' => session_obj('language')->encoding,
		'dir' => session_obj('language')->dir,
		'favicon' => $path_to_root . '/themes/default/images/favicon.ico',
		'onload' => $onload,
		'content' => $marker,
	));
	$at = strpos($html, $marker);
	if ($at === false) {
		return array($html, '');
	}
	return array(substr($html, 0, $at), substr($html, $at + strlen($marker)));
}
