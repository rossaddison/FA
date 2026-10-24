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
// checked via $GLOBALS rather than isset($path_to_root) so this doesn't depend on
// psalm.xml's non-nullable stub for the global, which only describes its value once a
// calling page script has set it, not whether it's been set at all. Safe here (no `global`
// statement for $path_to_root precedes this -- top-level script scope only)
if (!array_key_exists('path_to_root', $GLOBALS) || isset($_GET['path_to_root']) || isset($_POST['path_to_root']))
	die("Restricted access");
	include_once(__DIR__ . '/applications/application.php');
	include_once(__DIR__ . '/applications/customers.php');
	include_once(__DIR__ . '/applications/suppliers.php');
	include_once(__DIR__ . '/applications/inventory.php');
	include_once(__DIR__ . '/applications/fixed_assets.php');
	include_once(__DIR__ . '/applications/manufacturing.php');
	include_once(__DIR__ . '/applications/dimensions.php');
	include_once(__DIR__ . '/applications/generalledger.php');
	include_once(__DIR__ . '/applications/setup.php');
	include_once(__DIR__ . '/installed_extensions.php');

	final class front_accounting
	{
		/** @var mixed */
		var $user = null;
		/** @var mixed */
		var $settings = null;
		/** @var array<string, application> */
		var $applications = array();
		/** @var mixed */
		var $selected_application = null;
		/** @var mixed */
		var $renderer = null;

		/**
		 * @var menu|null
		 */
		var $menu = null;

		/** @psalm-external-mutation-free */
		function add_application(application $app): void
		{
			if ((bool)$app->enabled) // skip inactive modules
				$this->applications[(string) $app->id] = $app;
		}
		/** @psalm-mutation-free */
		function get_application(string $id): ?application
		{
			 if (isset($this->applications[$id]))
				return $this->applications[$id];
			 return null;
		}
		/** @psalm-mutation-free */
		function get_selected_application(): ?application
		{
			$selected_id = is_scalar($this->selected_application) ? (string) $this->selected_application : null;
			if ($selected_id !== null && isset($this->applications[$selected_id]))
				 return $this->applications[$selected_id];
			foreach ($this->applications as $application)
				return $application;
			return null;
		}
		function display(): void
		{
			global $path_to_root;

			// TaintedInclude on this line: confirmed safe, see doc/BUGS_FOUND.md
			// (Psalm doesn't honor @psalm-suppress for this finding here - tried both
			// statement- and method-level placement; see doc/PSALM_UPSTREAM_LIMITATIONS.md)
			include_once($path_to_root . "/themes/".user_theme()."/renderer.php");

			$this->init();
			$rend = new renderer();
			$rend->wa_header();

			$rend->display_applications($this);

			$rend->wa_footer();
			$this->renderer =& $rend;
		}
		function init(): void
		{
			global $SysPrefs;

			$this->menu = new menu(_("Main  Menu"));
			$this->menu->add_item(_("Main  Menu"), "index.php");
			$this->menu->add_item(_("Logout"), "/account/access/logout.php");
			$this->applications = array();
			$this->add_application(new customers_app());
			$this->add_application(new suppliers_app());
			$this->add_application(new inventory_app());
			if ((bool)get_company_pref('use_manufacturing'))
				$this->add_application(new manufacturing_app());
			if ((bool)get_company_pref('use_fixed_assets'))
			    $this->add_application(new assets_app());
			$this->add_application(new dimensions_app());
			$this->add_application(new general_ledger_app());

			hook_invoke_all('install_tabs', $this);

			$this->add_application(new setup_app());
		}
	}
