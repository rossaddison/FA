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
//	Controler part of database table pager with column sort.
//	To display actual html object call display_db_pager($name) inside
//  any form.
//
//	View definition you will find in the following file:
include_once(dirname(__DIR__) . "/includes/ui/db_pager_view.inc");

final class db_pager {
	/** @var string|array<string, mixed> */
	var $sql = '';
	/** @var string */
	var $name;
	/** @var array<int, array<string, mixed>> */
	var $columns = array();		// column definitions (head, type, order)

	/**
	 * @var null|string
	 */
	var $marker;		// marker check function

	/**
	 * @var null|string
	 */
	var $marker_txt;	

	/**
	 * @var null|string
	 */
	var $marker_class;

	/**
	 * @var null|string
	 */
	var $notice_class;

	/**
	 * @var string
	 */
	var $width;			// table width (default '95%')
	/** @var string|null */
	var $header_fun;	// additional row between title and body
	/** @var string|null */
	var $header_class;
    /** @var string|null */
    var $row_fun;       // Function for row preprocessing
	/** @var string|null */
	var $footer_fun;
	/** @var string|null */
	var $footer_class;

	/**
	 * @var array<array-key, array<array-key, mixed>>
	 */
	var $data = array();

	/** @var int */
	var $curr_page = 0;

	/**
	 * @var float|int
	 */
	var $max_page = 0;

	/**
	 * @var float|int|null
	 */
	var $last_page;
	/** @var int|null */
	var $prev_page;
	/** @var int|null */
	var $next_page;

	/**
	 * @var int|null
	 */
	var $first_page;
	    
	/** @var int */
	var $page_len;
	/** @var int|string */
	var $rec_count = 0;

	/**
	 * @var string
	 */
	var $select = '';

	/**
	 * @var string
	 */
	var $where = '';

	/**
	 * @var string
	 */
	var $from = '';

	/**
	 * @var string
	 */
	var $group = '';

	/**
	 * @var string
	 */
	var $order = '';
	/** @var array<int, string> */
	var	$extra_where = array();
	
	/**
	 * @var bool
	 */
	var $ready = false; // this var is false after change in sql before first
						// and before first query.
	/**
	 * @var array|bool
	 */
	var $inactive_ctrl = false;
	/** @var string|null */
	var $main_tbl;		// table and key field name for inactive ctrl and edit/delete links
	/** @var string|null */
	var $key;	// key field name
	
	//  db_pager constructor
	//  accepts $sql query either as:
	//  a. string in form 'SELECT field_list FROM table_joins [WHERE conditions [GROUP group_list [ORDER order_list]]]'
	//		- if WHERE keyword is used in table_joins, WHERE conditions is obligatory
	//  b. associative array using select, where, group and order keys ex :
	//  	array('select' => 'SUM(quantity)', 'from' => TB_PREF."stock_moves", 'group' => 'location')
	//
	//	$name is base name for pager controls
	/**
	 * @param string|array<string, mixed> $sql
	 */
	function __construct(string|array $sql, string $name, ?string $table = null, int $page_len=0)
	{
		$this->width = "95%";
		if ($page_len == 0) $page_len = (int) user_query_size();
		$this->name = $name;
		$this->page_len = $page_len;
	 	$this->set_sql($sql);
	}
	//
	//	Parse base sql select query	or use an associative array.
	//
	/**
	 * @param string|array<string, mixed> $sql
	 * @return void
	 */
	function set_sql(string|array $sql)
	{

		if ($sql != $this->sql) {
		    $this->sql = $sql;
		    $this->ready = false;

				if(is_array($sql)) {
					foreach(explode(' ', 'select from where group order') as $section) {
						$this->$section = @$sql[$section];
					}
					$this->select = "SELECT ".$this->select;
				}
				else {
					// parse the query
					$parts = preg_split('/\sFROM\s/si', $sql, 2) ?: array();
					if (count($parts) == 2) {
						$this->select = $parts[0];
						$sql = $parts[1];
					} else {
						if (sysprefs()->go_debug)
							display_error("Invalid sql input for db_pager");
					}

					$parts = preg_split('/\sWHERE(?!.*WHERE.*)\s/si', $sql, 2) ?: array(); // last occurence
					if(count($parts) == 2) {
						$this->from = $parts[0];
						$sql = $parts[1];

						$parts = preg_split('/\sORDER\s*BY\s/si', $sql, 2) ?: array();
						if(count($parts) == 2) {
							$sql = $parts[0];
							$this->order = $parts[1];
						}
						$parts = preg_split('/\sGROUP\s*BY\s/si', $sql, 2) ?: array();
						if(count($parts) == 2) {
							$sql = $parts[0];
							$this->group = $parts[1];
						}
						$this->where = $sql;
					}
			}
		}
	}
	//
	//	Set additional constraint on record set
	//
	/**
	 * @param string|array<array-key, mixed>|null $where
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function set_where(string|array|null $where = null)
	{
	 	if ((bool) $where) {
	    	if (!is_array($where))
			  $where = array($where);

		    if (count($where) == count($this->extra_where) &&
				!count(array_diff($this->extra_where, $where)))
				 return;
		} else {
			$where = array();
		}
		// MixedPropertyTypeCoercion here is accepted: $where's own generic array shape
		// can't be narrowed to extra_where's array<int, string> without misrepresenting
		// this method's documented, general-purpose contract -- see doc/BUGS_FOUND.md's
		// db_pager::set_where() entry for the real bug already found and fixed here
		$this->extra_where = $where;
		$this->ready = false;
	}
	//
	//	Set query result page
	//
	/**
	 * @param string|int|null $page
	 * @return true
	 */
	function change_page($page=null) 
	{
	    $this->set_page($page);
	    $this->query();
	    return true;
	}
	//
	//	Change sort column direction 
	//	in order asc->desc->none->asc
	//
	/**
	 * @param int $col
	 * @return true
	 */
	function sort_table(int $col)
	{

			$max_priority = 0;
			foreach($this->columns as $id => $_col) {
			if(!isset($_col['ord_priority'])) continue;
				$max_priority = max($max_priority, (int) $_col['ord_priority']);
			};


	    // $ord is genuinely mixed: $columns is declared array<int, array<string, mixed>>
	    // by design -- set_columns() below normalizes whatever shape each pager definition
	    // passes into this generic per-column config array
	    $ord = $this->columns[$col]['ord'];
  		$this->columns[$col]['ord_priority']  = $max_priority+1; // set priority , higher than anything else
	    $ord = ($ord == '') ? 'asc' : (($ord == 'asc') ? 'desc' : '');
	    $this->columns[$col]['ord'] = $ord;
	    $this->set_page(1);
	    $this->query();
	    return true;
	}
	//
	// Query database
	//
	/** @return bool */
	function query() 
	{

		ajax()->activate("_{$this->name}_span");
	    $this->data = array();
	    if (!$this->_init()) 
		  return false;

	    if ($this->rec_count == 0) return true;

	    $sql = $this->_sql_gen(false);

	    $result = db_query($sql, 'Error browsing database: '.$sql );

	    if ($result instanceof mysqli_result) {
		// setting field names for subsequent queries
			$c = 0;
		  	// add result field names to column defs for 
		  	// col value retrieve and sort purposes 
		   	$cnt = min(db_num_fields($result), count($this->columns));
			for ($c = $i = 0; $c < $cnt; $c++) {
				if (!(isset($this->columns[$c]['insert']) && $this->columns[$c]['insert'])) {
//					if (!@($this->columns[$c]['type']=='skip'))
			 			$this->columns[$c]['name']= db_field_name($result, $i);
					if (!@($this->columns[$c]['type']=='insert'))
			 			$i++;
			 	}
		  	}
		  	while ($row = db_fetch_assoc($result)) {
				$this->data[] = $row;
		  	}
		} else 
		  	return false;
		return true;
	}
	//
	//	Calculates page numbers for html controls.
	//
	/**
	 * @param string|int|null $to
	 * @return void
	 */
	function set_page($to) 
	{
	    switch($to) {
			case 'next':
			    $page = $this->curr_page+1; break;
			case 'prev':
			    $page = $this->curr_page-1; break;
			case 'last':
			    $page = $this->last_page; break;
			default:
			    if (is_numeric($to)) {
				 	$page = $to; break;
			    }
			case 'first':
			    $page = 1; break;
	    }
	  	if ($page < 1) 
	    	$page = 1;
	  	$max = $this->max_page;
	  	if ($page > $max) 
	    	$page = $max;
	  	$this->curr_page = (int) $page;
	  	$this->next_page = ($page < $max) ? (int) $page+1 : null;
	  	$this->prev_page = ($page > 1) ? (int) $page-1 : null;
	  	$this->last_page = ($page < $max) ? $max : null;
	  	$this->first_page = ($page != 1) ? 1: null;
	}
	//
	//	Set column definitions
	//  $flds: array( fldname1, fldname2=>type,...)
	/**
	 * @param string|array<array-key, mixed> $flds
	 * @return void
	 */
	function set_columns(string|array $flds)
	{
		// $coldef/$c/$h are genuinely mixed: $flds accepts whatever shape each pager
		// definition across the codebase passes (bare name, name=>type, name=>array of
		// options, ...), and this loop's own job is to normalize all of those into the
		// uniform array<string, mixed> $columns entries used everywhere else in this class
		$this->columns = array();
		if (!is_array($flds)) {
			$flds = array($flds);
		}
		foreach ($flds as $colnum=>$coldef) {
			if (is_string($colnum)) {	// 'colname'=>params
			  	$h = $colnum;
			  	$c = $coldef;
			} else {			//  n=>params
				if (is_array($coldef)) {
					$h = '';
					$c = $coldef;
				} else {
					$h = $coldef;
					$c = 'text';
				}
			}
			if (is_string($c)) 			// params is simple column type
			  	$c = array('type'=>$c);

			if (!isset($c['type']))
			  	$c['type'] = 'text';

			switch($c['type']) {
				case 'inactive': 
					$this->inactive_ctrl = true;
				case 'insert':
				default:
					$c['head'] = $h; break;
				case 'skip':		// skip the column (no header)
					unset($c['head']); break;
	  		}
			$this->columns[] = $c;	
		}
	}
	//
	// Generate db query from base sql
	// $count==false - for current page data retrieval 
	// $count==true  - for total records count
	//
	/**
	 * @param bool $count
	 * @return non-empty-string
	 */
	function _sql_gen($count=false) 
	{
		$select = $this->select;
		$from = $this->from;
		$where = $this->where;
		$group = $this->group;
		$order = $this->order;

		if(count($this->extra_where)) {
		    $where .= ($where=='' ? '' : ' AND ')
				.implode(' AND ', $this->extra_where);
		}
		if ($where) $where = " WHERE ($where)";

		if ($count) {
			$inner = "$select FROM $from $where";
			if ($group) $inner .= " GROUP BY $group";
			return "SELECT COUNT(*) FROM ($inner) tmp_count";
		}

		$sql = "$select FROM $from $where";
		if ($group) $sql.= " GROUP BY $group";
	    $ord = array();

		// sort order column by priority instead of table order.
		$columns = array();
	    foreach ($this->columns as $col) {
			if(isset($col['ord_priority'])) {
				$columns[(int) $col['ord_priority']] = $col;
			}
		}
		krsort($columns);

	    foreach ($columns as $col) {
	       	if (isset($col['ord'])) {
	        	if ( $col['ord'] != '' && isset($col['name'])) {
		    	    $ord[] = (string)$col['name'] .' '. (string)$col['ord'];
			    }
			}
	    }

	    if (count($ord)) {
			$ord = array_map('db_escape_function', $ord);
			$sql .= " ORDER BY " . implode(',', $ord);
		} else {
			if($order)
				$sql .= " ORDER BY $order"; // original base query order
		}

	    $page_len = $this->page_len;
	    $offset = ($this->curr_page - 1) * $page_len;

	    $sql .= " LIMIT $offset, $page_len";

		return $sql;
		
	}
	//
	//	Initialization after changing record set
	//
	/** @return bool */
	function _init() 
	{
		
	    if ($this->ready == false ) {
			$sql = $this->_sql_gen(true);
			$result = db_query($sql, 'Error reading record set');
			if (!$result instanceof mysqli_result)
				return false;
			$row = db_fetch_row($result);
			$this->rec_count = is_array($row) && $row[0] !== null ? (int) $row[0] : 0;

	  		$this->max_page = $this->page_len ?
				ceil($this->rec_count/$this->page_len) : 0;
		
			if (sysprefs()->go_debug) { // FIX - need column name parsing, but for now:
				// check if field names are set explicite in col def
				// for all initially ordered columns
			    foreach ($this->columns as $col) {
		    	   	if (isset($col['ord']) && $col['ord'] != '' 
						&&  !isset($col['name'])) {
							display_warning("Result field names must be set
								for all initially ordered db_pager columns.");
			    	}
				}
	    	}
	  		$this->set_page(1);
	  		$this->ready = true;
	    }
    	return true;
	}
	//
	//	Set current page in response to user control.
	//
	/** @return void */
	function select_records() 
	{
		
		$page = find_submit($this->name.'_page_', false);
		$sort = find_submit($this->name.'_sort_', true);
		if ((bool)$page) {
			$this->change_page($page);
			if ($page == 'next' && $this->next_page === null ||
				$page == 'last' && $this->last_page === null)
					set_focus($this->name.'_page_prev');
			if ($page == 'prev' && $this->prev_page === null ||
				$page == 'first' && $this->first_page === null)
					set_focus($this->name.'_page_next');
		} elseif ($sort != -1 && $sort !== null) {
			$this->sort_table((int) $sort);
		} else
			$this->query();
	}
	//
	//	Set check function to mark some rows.
	//	
	/**
	 * @param string|null $func
	 * @param string $notice
	 * @param string $markercl
	 * @param string $msgclass
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function set_marker(?string $func, string $notice='', string $markercl='overduebg', string $msgclass='overduefg' )
	{
		$this->marker = $func;
		$this->marker_txt = $notice;
		$this->marker_class = $markercl;
		$this->notice_class = $msgclass;
	}
	//
	//	Set handler to display additional row between titles and pager body.
	//	Return array of column contents.
	//
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function set_header(?string $func, string $headercl='inquirybg')
	{
		$this->header_fun = $func;
		$this->header_class = $headercl;
	}
	//
	//	Set handler to display additional row between pager body and navibar.
	//	Return array of column contents.
	//
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function set_footer(?string $func, string $footercl='inquirybg')
	{
		$this->footer_fun = $func;
		$this->footer_class = $footercl;
	}
	//
	//	Setter for table editors with inactive cell control.
	//
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function set_inactive_ctrl(?string $table, ?string $key) {
		$this->inactive_ctrl = array('table'=>$table, 'key'=>$key);
	}
	//
	//	Helper for display inactive control cells
	//
	/**
	 * @param array<array-key, mixed> $row
	 * @return ''|null
	 */
	function inactive_control_cell(array &$row)
	{
		if ((bool)$this->inactive_ctrl) {


			$key = $this->key !== null && $this->key !== '' ?
				$this->key : (string) ($this->columns[0]['name'] ?? '');		// TODO - support for complex keys
			/** @var string|int|float|bool|null $id */
			$id = $row[$key];
			$id = (string) $id;
			$table = $this->main_tbl;
			$name = "Inactive". $id;
			$value = $row['inactive'] ? 1:0;

			if (check_value('show_inactive')) {
				if (isset($_POST['LInact'][$id]) && (get_post('_Inactive'.$id.'_update') ||
					get_post('Update')) && (check_value('Inactive'.$id) != $value)) {
					update_record_status($id, !$value, $table, $key);
					$value = !$value;
				}
				echo '<td align="center">'. checkbox(null, $name, $value, true, '')
 				. (string) hidden("LInact[$id]", $value, false) . '</td>';
			}
		} else
			return '';
	}

};
//-----------------------------------------------------------------------------
//	Creates new db_pager $_SESSION object on first page call.
//  Retrieves from $_SESSION var on subsequent $_POST calls
//
//  $name - base name for pager controls and $_SESSION object name
//  $sql  - base sql for data inquiry. Order of fields implies
//		pager columns order.
//	$coldef - array of column definitions. Example definitions
//		Column with title 'User name' and default text format:
//				'User name'
//		Skipped field from sql query. Data for the field is not displayed:
//				'dummy' => 'skip'
//		Column without title, data retrieved form row data with function func():
//	 			array('fun'=>'func')
// 		Inserted column with title 'Some', formated with function rowfun().
//  	formated as date:
//				'Some' => array('type'=>'date, 'insert'=>true, 'fun'=>'rowfun')
// 		Column with name 'Another', formatted as date, 
// sortable with ascending start order (available orders: asc,desc, '').
//				'Another' => array('type'=>'date', 'ord'=>'asc')
//
//	All available column format types you will find in db_pager_view.inc file.
//		If query result has more fields than count($coldef), rest of data is ignored
//  during display, but can be used in format handlers for 'spec' and 'insert' 
//	type columns.

/**
 * @param string|array<string, mixed> $sql
 * @param string|array<array-key, mixed> $coldef
 * @return db_pager
 */
function new_db_pager(string $name, string|array $sql, string|array $coldef, ?string $table = null, ?string $key = null, int $page_len = 0): db_pager  {

    /** @var db_pager|null $existing_pager */
    $existing_pager = $_SESSION[$name] ?? null;
    if (isset($existing_pager) &&
		 (($_SERVER['REQUEST_METHOD'] ?? '') == 'GET' || $existing_pager->sql != $sql)) {
		unset($_SESSION[$name]); // kill pager if sql has changed
	}
	if (!isset($_SESSION[$name])) {
	    $_SESSION[$name] = new db_pager($sql, $name, $table, $page_len);
		$_SESSION[$name]->main_tbl = $table;
		$_SESSION[$name]->key = $key;
		$_SESSION[$name]->set_sql($sql);
		$_SESSION[$name]->set_columns($coldef);
	}

	/** @var db_pager */
	return  $_SESSION[$name];

}
//
//	Force pager initialization.
//
function refresh_pager(?string $name): void
{
	$name = (string) $name;
	if (isset($_SESSION[$name]) && $_SESSION[$name] instanceof db_pager)
		$_SESSION[$name]->ready = false;
}
