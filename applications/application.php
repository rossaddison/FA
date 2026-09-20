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

define('MENU_ENTRY', 'menu_entry');
define('MENU_TRANSACTION', 'menu_transaction');
define('MENU_INQUIRY', 'menu_inquiry');
define('MENU_REPORT', 'menu_report');
define('MENU_MAINTENANCE', 'menu_maintenance');
define('MENU_UPDATE', 'menu_update');
define('MENU_SETTINGS', 'menu_settings');
define('MENU_SYSTEM', 'menu_system');

    class menu_item
	{
		/**
		 * @var null|string
		 */
		var $label;

		/**
		 * @var null|string
		 */
		var $link;
		
		/**
		 * @param string|null $label
		 * @param string|null $link
		 * @psalm-mutation-free
		 */
		function __construct($label, $link) 
		{
			$this->label = $label;
			$this->link = $link;
		}
	}

	class menu 
	{
		/**
		 * @var null|string
		 */
		var $title;
		/** @var array<int, app_function> */
		var $items;
		
		/**
		 * @param string|null $title
		 * @psalm-mutation-free
		 */
		function __construct($title) 
		{
			$this->title = $title;
			$this->items = array();
		}
		
		/**
		 * @param string|null $label
		 * @param string|null $link
		 * @psalm-external-mutation-free
		 * @return menu_item
		 */
		function add_item($label, $link) 
		{
			$item = new menu_item($label,$link);
			array_push($this->items,$item);
			return $item;
		}
		
	}

	class app_function 
	{
		/** @var string */
		var $label;
		/** @var string */
		var $link;
		/** @var string */
		var $access;
        /** @var string */
        var $category;
		
		/**
		 * @psalm-mutation-free
		 * @return app_function
		 */
		function __construct($label,$link,$access='SA_OPEN',$category='')
		{
			$this->label = $label;
			$this->link = $link;
			$this->access = $access;
            $this->category = $category;
		}
	}

	class module 
	{
		/**
		 * @var null|string
		 */
		var $name;
		/** @var string|null */
		var $icon;
		/** @var array<int, app_function> */
		var $lappfunctions;
		/** @var array<int, app_function> */
		var $rappfunctions;
		
		/**
		 * @param string|null $name
		 * @psalm-mutation-free
		 * @return module
		 */
		function __construct($name,$icon = null) 
		{
			$this->name = $name;
			$this->icon = $icon;
			$this->lappfunctions = array();
			$this->rappfunctions = array();
		}
		
		/**
		 * @psalm-external-mutation-free
		 * @return app_function
		 */
		function add_lapp_function($label,$link="",$access='SA_OPEN',$category='')
		{
			$appfunction = new app_function($label,$link,$access,$category);
			$this->lappfunctions[] = $appfunction;
			return $appfunction;
		}

		/**
		 * @psalm-external-mutation-free
		 * @return app_function
		 */
		function add_rapp_function($label,$link="",$access='SA_OPEN',$category='')
		{
			$appfunction = new app_function($label,$link,$access,$category);
			$this->rappfunctions[] = $appfunction;
			return $appfunction;
		}
		
		
	}

	class application 
	{
		/**
		 * @var null|string
		 */
		var $id;

		/**
		 * @var null|string
		 */
		var $name;
		/** @var string|null */
		var $help_context;
		/** @var array<int, module> */
		var $modules;
		/** @var bool|int */
		var $enabled;
		
		/**
		 * @param string|null $id
		 * @param string|null $name
		 * @psalm-mutation-free
		 */
		function __construct($id, $name, $enabled=true) 
		{
			$this->id = $id;
			$this->name = $name;
			$this->enabled = $enabled;
			$this->modules = array();
		}
		
		/**
		 * @param string|null $name
		 * @psalm-external-mutation-free
		 * @return module
		 */
		function add_module($name, $icon = null) 
		{
			$module = new module($name,$icon);
			$this->modules[] = $module;
			return $module;
		}
		
		/**
		 * @param int|null $level
		 * @param string|null $label
		 * @param string|null $link
		 * @param string|null $access
		 * @param string|null $category
		 * @psalm-mutation-free
		 * @return void
		 */
		function add_lapp_function($level, $label,$link="",$access='SA_OPEN',$category='')
		{
			$this->modules[$level]->lappfunctions[] = new app_function($label, $link, $access, $category);
		}
		
		/**
		 * @param int|null $level
		 * @param string|null $label
		 * @param string|null $link
		 * @param string|null $access
		 * @param string|null $category
		 * @psalm-mutation-free
		 * @return void
		 */
		function add_rapp_function($level, $label,$link="",$access='SA_OPEN',$category='')
		{
			$this->modules[$level]->rappfunctions[] = new app_function($label, $link, $access, $category);
		}
		
		/** @return void */
		function add_extensions()
		{
			hook_invoke_all('install_options', $this);
		}
		//
		// Helper returning link to report class added by extension module.
		//
		/**
		 * @psalm-pure
		 * @return string
		 */
		function report_class_url($class)
		{
			return "reporting/reports_main.php?Class=".$class;
		}
	}


