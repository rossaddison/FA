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

class user_prefs
{

	/** @var string */
	var $language;

	/** @var string|int|float|bool|null */
	var $qty_dec;
	/** @var string|int|float|bool|null */
	var $price_dec;
	/** @var string|int|float|bool|null */
	var	$exrate_dec;
	/** @var string|int|float|bool|null */
	var	$percent_dec;

	/** @var string|int|float|bool|null */
	var	$show_gl_info;
	/** @var string|int|float|bool|null */
	var	$show_codes;

	/** @var string|int|float|bool|null */
	var $date_format;
	/** @var string|int|float|bool|null */
	var $date_sep;
	/** @var string|int|float|bool|null */
	var $tho_sep;
	/** @var string|int|float|bool|null */
	var $dec_sep;
	/** @var string */
	var $theme;
	/** @var string|int|float|bool|null */
	var $print_profile;
	/** @var string|int|float|bool|null */
	var $rep_popup;
	/** @var string|int|float|bool|null */
	var $pagesize; // for printing
	/** @var string|int|float|bool|null */
	var $show_hints;
	/** @var string|int|float|bool|null */
	var $query_size; // table pager page length
	/** @var string|int|float|bool|null */
	var $graphic_links; // use graphic links
	/** @var string|int|float|bool|null */
	var $sticky_date;	// save date on subsequent document entry
	/** @var string */
	var $startup_tab;  // default start-up menu tab
	/** @var string|int|float|bool|null */
	var $transaction_days; // transaction days in inquiries
	/** @var string|int|float|bool|null */
	var $save_report_selections; // save report selections days 0...
	/** @var string|int|float|bool|null */
	var $use_date_picker; // use date picker for all date fields
	/** @var string|int|float|bool|null */
	var $def_print_destination; // default print destination. 0 = PDF/Printer, 1 = Excel
	/** @var string|int|float|bool|null */
	var $def_print_orientation; // default print orientation. 0 = Portrait. 1 = Landscape

	/**
	 * @param bool|array<array-key, mixed>|null $user
	 */
	function __construct($user=null)
	{
		if ($user == null) { 
			// set default values, used before login
			global $dflt_lang;

			$this->date_sep = sysprefs()->dflt_date_sep;
			$this->date_format = sysprefs()->dflt_date_fmt;
			$this->tho_sep = 0;
			$this->dec_sep = 0;
			$this->price_dec = 2;
			$this->language = $dflt_lang;
			$this->theme = 'default';
			$this->transaction_days = -30;
			$this->save_report_selections = 0;
			$this->use_date_picker = 1;
			$this->def_print_destination = 0;
			$this->def_print_orientation = 0;

		} else {
			global $path_to_root;
			$this->language = $user["language"];
			$_SESSION['language']->set_language($this->language);

			$this->qty_dec = $user["qty_dec"];
			$this->price_dec = $user["prices_dec"];
			$this->exrate_dec = $user["rates_dec"];
			$this->percent_dec = $user["percent_dec"];

			$this->show_gl_info = $user["show_gl"];
			$this->show_codes = $user["show_codes"];
			$this->date_format = $user["date_format"];
			$this->date_sep = $user["date_sep"];
			$this->tho_sep = $user["tho_sep"];
			$this->dec_sep = $user["dec_sep"];
			$this->theme = $user["theme"];
			$this->pagesize = $user["page_size"];
			$this->show_hints = $user["show_hints"];
			$this->print_profile = $user["print_profile"];
			$this->rep_popup = $user["rep_popup"];
			$this->query_size = $user["query_size"];
			$this->graphic_links = $user["graphic_links"];
			if (isset($user["sticky_doc_date"]))
			{
				$this->sticky_date = $user["sticky_doc_date"];
				$this->startup_tab = $user['startup_tab'];
			}
			else
			{
				$this->sticky_date = 0;
				$this->startup_tab = "orders";
			}
			$this->transaction_days = @$user['transaction_days'];
			$this->save_report_selections = @$user['save_report_selections'];
			$this->use_date_picker = @$user['use_date_picker'];
			$this->def_print_destination = @$user['def_print_destination'];
			$this->def_print_orientation = @$user['def_print_orientation'];

			if (!file_exists("$path_to_root/themes/$this->theme"))
				$this->theme = "default";
		}
	}
	
	/**
	 * @psalm-mutation-free
	 * @return array{date_format: mixed, date_sep: mixed, dec_sep: mixed, def_print_destination: mixed, def_print_orientation: mixed, graphic_links: mixed, language: mixed, page_size: mixed, percent_dec: mixed, prices_dec: mixed, print_profile: mixed, qty_dec: mixed, query_size: mixed, rates_dec: mixed, rep_popup: mixed, save_report_selections: mixed, show_codes: mixed, show_gl: mixed, show_hints: mixed, startup_tab: mixed, sticky_doc_date: mixed, theme: mixed, tho_sep: mixed, use_date_picker: mixed}
	 */
	function get_all()
	{
			return array(
			'language' => $this->language,
			'qty_dec' => $this->qty_dec,
			'prices_dec' => $this->price_dec,
			'rates_dec' => $this->exrate_dec,
			'percent_dec' => $this->percent_dec,
			'show_gl' => $this->show_gl_info,
			'show_codes' => $this->show_codes,
			'date_format' =>$this->date_format,
			'date_sep' => $this->date_sep,
			'tho_sep' => $this->tho_sep,
			'dec_sep' => $this->dec_sep,
			'theme' => $this->theme,
			'page_size' => $this->pagesize,
			'show_hints' => $this->show_hints,
			'print_profile' => $this->print_profile,
			'rep_popup' => $this->rep_popup,
			'query_size' => $this->query_size,
			'graphic_links' => $this->graphic_links,
			'sticky_doc_date' => $this->sticky_date,
			'startup_tab' => $this->startup_tab,
			'save_report_selections' => $this->save_report_selections,
			'use_date_picker' => $this->use_date_picker,
			'def_print_destination' => $this->def_print_destination,
			'def_print_orientation' => $this->def_print_orientation);
	}
	
	/**
	 * @psalm-mutation-free
	 * @return string
	 */
	function language() 
	{
		return $this->language;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function qty_dec() 
	{
		return $this->qty_dec;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function price_dec() 
	{
		return $this->price_dec;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function exrate_dec() 
	{
		return $this->exrate_dec;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function percent_dec() 
	{
		return $this->percent_dec;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function show_gl_info() 
	{
		return $this->show_gl_info;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function show_codes() 
	{
		return $this->show_codes;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function date_format() 
	{
		return $this->date_format;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function date_sep() 
	{
		return $this->date_sep;
	}

	/** @return non-falsy-string */
	function date_display() 
	{
		$sep = sysprefs()->dateseps[$this->date_sep];
		if ($this->date_format == 0)
			return "m".$sep."d".$sep."Y";
		elseif ($this->date_format == 1)
			return "d".$sep."m".$sep."Y";
		elseif ($this->date_format == 2)
			return "Y".$sep."m".$sep."d";
		elseif ($this->date_format == 3)
			return "M".$sep."j".$sep."Y";
		elseif ($this->date_format == 4)
			return "j".$sep."M".$sep."Y";
		else
			return "Y".$sep."M".$sep."j";
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function tho_sep() 
	{
		return $this->tho_sep;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function dec_sep() 
	{
		return $this->dec_sep;
	}

	/**
	 * @psalm-mutation-free
	 * @return string
	 */
	function get_theme() 
	{
		return $this->theme;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function get_pagesize() 
	{
		return $this->pagesize;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function show_hints() 
	{
		return $this->show_hints;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function print_profile() 
	{
		return $this->print_profile;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function rep_popup() 
	{
		return $this->rep_popup;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function query_size() 
	{
		return $this->query_size;
	}

	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function graphic_links() 
	{
		return $this->graphic_links;
	}
	
	/**
	 * @psalm-mutation-free
	 * @return null|scalar
	 */
	function sticky_date()
	{
		return $this->sticky_date;
	}
	
	/**
	 * @psalm-mutation-free
	 * @return string
	 */
	function start_up_tab()
	{
		return $this->startup_tab;
	}

    /**
     * @psalm-mutation-free
     * @return null|scalar
     */
    function transaction_days() 
    {
        return $this->transaction_days;
    }

    /**
     * @psalm-mutation-free
     * @return null|scalar
     */
    function save_report_selections() 
    {
        return $this->save_report_selections;
    }

    /**
     * @psalm-mutation-free
     * @return null|scalar
     */
    function use_date_picker() 
    {
        return $this->use_date_picker;
    }

    /**
     * @psalm-mutation-free
     * @return null|scalar
     */
    function def_print_destination() 
    {
        return $this->def_print_destination;
    }

    /**
     * @psalm-mutation-free
     * @return null|scalar
     */
    function def_print_orientation() 
    {
        return $this->def_print_orientation;
    }

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function set_dec($price_dec, $qty_dec, $exrate_dec, $percent_dec, $showgl, $showcodes) 
	{
		$this->price_dec = $price_dec;
		$this->qty_dec = $qty_dec;
		$this->exrate_dec = $exrate_dec;
		$this->percent_dec = $percent_dec;
		$this->show_gl_info = $showgl;
		$this->show_codes = $showcodes;
	}

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function set_format($date_format, $date_sep, $tho_sep, $dec_sep, $theme, $pagesize) 
	{
		$this->date_format = $date_format;
		$this->date_sep = $date_sep;
		$this->tho_sep = $tho_sep;
		$this->dec_sep = $dec_sep;
		$this->theme = $theme;
		$this->pagesize = $pagesize;
	}

}

