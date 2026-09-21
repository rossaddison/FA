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
	Prototype: FA's style sheets and script files as yiisoft/assets bundles.

	add_css_file(), add_js_file() and add_js_ufile() keep filling the globals they always have. When the Yii page
	shell is on, those lists are turned into asset bundles at page() time, and the asset manager hands the view the
	<link> and <script> tags, with a version stamp taken from each file's modification time.
*/

use Yiisoft\Aliases\Aliases;
use Yiisoft\Assets\AssetBundle;
use Yiisoft\Assets\AssetLoader;
use Yiisoft\Assets\AssetManager;
use Yiisoft\View\WebView;

/**
 * A bundle of files that already sit in the web root, so there is nothing to publish.
 *
 * @param string $basePath directory of the files, relative to the running script or absolute
 * @param string $baseUrl the same directory as a URL
 * @param list<string> $css files inside $basePath
 * @param list<string> $js files inside $basePath
 */
function yiiAssetBundle(string $basePath, string $baseUrl, array $css, array $js): AssetBundle
{
	$bundle = new AssetBundle();
	$bundle->basePath = (string)realpath($basePath);
	$bundle->baseUrl = $baseUrl;
	$bundle->css = $css;
	$bundle->js = $js;
	return $bundle;
}

/**
 * The asset manager for this page, with every style sheet and script that FA has collected registered.
 *
 * @param string $extraCss an extra style sheet for this page
 */
function yiiAssetManager(string $extraCss): AssetManager
{
	// FA's own scripts are compressed and cached per user; scriptFiles() makes sure the cache is current.
	$scripts = scriptFiles();
	$cacheDir = user_js_cache();

	// the bundles' base URL is '.', so a leading './' would be doubled
	$css = array_map(fn(string $url): string => preg_replace('#^\./#', '', $url) ?? $url, cssUrls($extraCss));
	// the layout is HTML5 (standards mode): a small style sheet keeps the pages the height they had before
	// (after the calls above: Psalm forgets a global's type across a call)
	global $path_to_root;
	$css[] = preg_replace('#^\./#', '', $path_to_root . '/views/assets/html5.css') ?? '';
	// the alert rules of Bootstrap, for the system messages (includes/yii/alert.inc)
	$css[] = preg_replace('#^\./#', '', $path_to_root . '/css/fa-alerts.css') ?? '';
	$userJs = array_map(fn(string $url): string => preg_replace('#^\./#', '', $url) ?? $url, $scripts['user']);

	$bundles = array(
		'fa.core-scripts' => yiiAssetBundle($cacheDir, $cacheDir, array(), array_map('basename', $scripts['static'])),
		'fa.styles' => yiiAssetBundle('.', '.', $css, array()),
		'fa.extension-scripts' => yiiAssetBundle('.', '.', array(), $userJs),
	);

	$aliases = new Aliases();
	$manager = new AssetManager($aliases, new AssetLoader($aliases, true), array(), $bundles);
	foreach (array_keys($bundles) as $name) {
		$manager->register($name, WebView::POSITION_HEAD, WebView::POSITION_HEAD);
	}
	return $manager;
}

/**
 * Puts the page's style sheets and scripts on the view.
 */
function yiiRegisterAssets(WebView $view, string $extraCss): void
{
	$manager = yiiAssetManager($extraCss);
	$view->addCssFiles($manager->getCssFiles());
	$view->addJsFiles($manager->getJsFiles());
}
