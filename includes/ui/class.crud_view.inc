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

include_once $path_to_root.'/includes/db/class.data_set.inc';

/*
	TODO: for php5:
	. use __construct in base class to avoid need for implicit call to parent constructor.
	. use private instead _* function name convention
*/
//
//	User input-output conversions.
//
class user_view {

	/** @var array<array-key, mixed>|false|null */
	var	$data; 		// data in php format
	/** @var array<array-key, mixed> */
	var $fields = array();	// input fields format descriptions
	/** @var array<string, string> */
	var $errors = array();
	/** @var int */
	var $dec = 0;
	/** @var string */
	var $name;

	/** @psalm-mutation-free */
	function __construct(string $name)
	{
		$this->name = $name;
	}

	/**
	 * @param string $msg
	 * @param string|int|null $context
	 * @psalm-external-mutation-free
	 * @return false
	 */
	function error($msg, $context=null)
	{
		// save error message
		if (!isset($context))
			$context = count($this->errors);
		$this->errors[(string) $context] = $msg;

		return false;
	}
	/*
		Input/output formatters - convert values between php/user domains.
	*/
	/**
	 * @param string|int|float|bool|null $value
	 * @param string|null $fmt
	 * @return string|int|float|bool|null
	 */
	function _format_input($value, $fmt)
	{
		switch($fmt) {
			case 'stock':
				$this->dec = (int) get_qty_dec(is_string($value) ? $value : null);
				return $value;
			case 'price':
			case 'qty':
			case 'number':
				    return user_numeric($value);
			case 'percent':
					return (float)user_numeric($value)/100.0;
			case 'check':
					return isset($value) ? 1 : 0;
			case 'text':
			case 'date':
			default:
				return $value;
		}
	}
	//
	// Returns formatted value
	//
	/**
	 * @param string|int|float|bool|null $value
	 * @param string|null $fmt
	 * @return string|int|float|bool|null
	 */
	function _format_output($value, $fmt)
	{
		switch($fmt) {
			case 'price':
				return price_format($value);
			case 'qty':
				return number_format2($value, $this->dec);
			case 'number':
				return number_format2($value);
			case 'percent':
				return percent_format((float)$value*100.0);
			case 'check':
				return !empty($value);
			case 'stock':
				$this->dec = (int) get_qty_dec(is_string($value) ? $value : null); // retrieve dec for use in following qty fields
			case 'text':
			case 'date':
			default:
				return $value;
		}
	}
	/**
	*	Returns html element for given field
	*	@mode - true for edit, false for read-only
	* @param string|int|float|bool|null $value
	* @param string|null $fmt
	* @param bool|string|null $mode
	* @return void
	**/
	function _format_cells($value, $fmt, $mode)
	{
		$value = $this->_format_output($value, $fmt);

		// available formatters with parameters
		$formatters = array(
			'email_cell' => array('label', 'params'=>'', 'id'=>null),
			'amount_cell' => array('label'=>null, 'bold'=>false, 'params'=>'', 'id'=>null),
			'text_cells' => array('label', 'name', 'value'=>null, 'size'=>"", 'max'=>"", 'title'=>false,
				'labparams'=>"", 'post_label'=>"", 'inparams'=>"")
		);
		// format functions used in various modes
		$formats = array(
			// field format => (view [,edit])
			'' => array('label_cell', 'text_cells'), // default
			'price' => array('label_cell', 'amount_cell'),
			'qty',
			'number',
			'percent',
			'stock',
			'text',
			'date',
		);
	}

	/**
	*
	*	PHP->user format values convertion.
	*
	* @param array<array-key, mixed>|false|null $data
	* @param array<array-key, mixed>|null $output
	* @return void
	**/
	function set_output($data=null, &$output=null)
	{
		if (isset($data))
			$this->data = $data;

		if (!isset($output)) {
			$output = &$_POST;
			$prefix = $this->name;
		} else
			$prefix = '';

		foreach($this->fields as $name => $fmt) {

			if (is_int($name)) {
				$name = $fmt;
				$fmt = array();
			} elseif (!is_array($fmt))
				$fmt = array('fmt' => $fmt);

			$post = $prefix.(isset($fmt['post']) ? $fmt['post'] : $name);

			$fld = isset($fmt['fld']) ? $fmt['fld'] : $name;

			$value = (is_array($this->data) && isset($this->data[$fld])) ? $this->data[$fld] : @$fmt['dflt'];

			if (isset($value)) {
				$fmt_val = @$fmt['fmt'];
				$output[$post] = $this->_format_output(is_scalar($value) ? $value : null, is_string($fmt_val) ? $fmt_val : null);
			}
		}
	}

	/**
	*
	*	User->php format values convertion.
	*	$input - data in user format
	*	$all - return also null values for non-existing input field.
	* @param array<array-key, mixed>|null $input
	* @param bool $all
	* @return array<array-key, mixed>
	**/
	function get_input($input=null, $all=false)
	{
		if (!isset($input))
			$input = $_POST;

		if ($this->name)	// strip view name prefix
		    foreach($input as $postkey=>$postval )
    		{
				if (strpos((string) $postkey, $this->name) === 0)
				{
					$id = substr($postkey, strlen($this->name));
					$input[$id] = $postval;
				}
				unset($input[$postkey]);
    		}

    	$data = array();
		foreach ($this->fields as $name => $fmt) {
			if (is_int($name)) {	// direct string passed: this is name of field
				$name = $fmt;
				$fmt = array(); // default format
			} elseif (!is_array($fmt))
				$fmt = array('fmt' => $fmt);
			$post = isset($fmt['post']) ? $fmt['post'] : $name; // input name (default to field name)
			$fld = isset($fmt['fld']) ? $fmt['fld'] : $name;	// input value (default to field name)


//			if ($all || array_key_exists($post, $input))
//			{
				if (@$fmt['fmt'] == 'check')
					$value = (bool)(@$input[$post]) ? 1 : 0;
				else {
					$in_val = @$input[$post];
					$fmt_val = @$fmt['fmt'];
					$value = $this->_format_input(is_scalar($in_val) ? $in_val : null, is_string($fmt_val) ? $fmt_val : null);
				}

//				if (is_array($data))
				if ($all || isset($value))
					$data[$fld] = $value;
//				else
//					$data->$fld = $value;
//			}
		}

		return $data;
	}

	//--------------------------------------------------------
	//
	//	Return data formatted according to field descriptions.
	//
	/**
	 * @param array<array-key, mixed>|null $input
	 * @param bool|string|null $mode
	 * @return array<array-key, mixed>
	 */
	function get_fields_views($input=null, $mode=null)
	{

		if (!isset($input))
			$input = $_POST;

		$view = array();
		foreach ($this->fields as $name => $fmt) {
			if (is_int($name)) {
				$name = $fmt;
				$fmt = array();
			}
			$post = isset($fmt['post']) ? $fmt['post'] : $name;
			$fld = isset($fmt['fld']) ? $fmt['fld'] : $name;

			$cell_val = @$input[$post];
			$fmt_val = @$fmt['fmt'];
			$this->_format_cells(is_scalar($cell_val) ? $cell_val : null, is_string($fmt_val) ? $fmt_val : null, $mode);

			$view[$fld] = null;
		}
		return $view;
	}

}

//
//	Template for simple table editors
//

class simple_crud_view extends user_view {
	/** @var string */
	var $name;
		// object status:
	/** @var string */
	var $Mode = 'RESET';

	/**
	 * @var null|string
	 */
	var $selected_id;
	/** @var null|string */
	var $prev_id;

	/**
	 * @var string
	 */
	var $_none = ''; // selector value when no item is selected
	/** @var array<string, string> */
	var $pre_handlers = array(); // control buttons and related methods called before view display
	/** @var array<string, string> */
	var $views = array();
	/** @var array<array-key, mixed>|false|null */
	var $data = array();
	/** @var array<array-key, mixed> */
	var $fields;
	/** @var array<string, array<int, string|bool|null>> */
	var $tool_buttons = array();

	/**
	 * @var array{delete: bool|string, update: bool|string, insert: bool|string, clone: bool|string}
	 */
	var $options = array(
		'delete' => true,		// true or message for successfull action.
		'update' => true,
		'insert' => true,
		'clone' => true,
	);
	/** @var int */
	var $dec = 0;
	/** @var record_set */
	var $data_set;

	/**
	 * @var bool
	 */
	var $display_both = false; //when set to true both list and editor are displayed all the time (eventually set in sub classes)
	//
	//
	/**
	 * @param array{delete?: bool|string, update?: bool|string, insert?: bool|string, clone?: bool|string} $options
	 * @psalm-mutation-free
	 */
	function __construct(string $name, record_set $data_set, $options=array())
	{
		parent::__construct($name);

		$this->options = array_merge($this->options, $options);

		$this->views[''] = 'list_view';			// default view

		if ((bool)$this->options['update'])
			$this->_add_action('Edit', '_edit', _('Edit'), _('Edit document line'), ICON_EDIT, '',
				'editor_view');

		if ((bool)$this->options['delete'])
			$this->_add_action('Delete', '_delete', _('Delete'), _('Remove line from document'), ICON_DELETE, '',
				'list_view');

		if ((bool)$this->options['update'])
			$this->_add_action('UPDATE', '_update', _('Update'), _('Submit changes'), ICON_UPDATE, 'default',
				'editor_view');

		$this->_add_action('RESET', '_cancel', _('Cancel'), _('Cancel changes'), ICON_ESCAPE, 'cancel',
			'list_view');

		if ((bool)$this->options['insert'])
			$this->_add_action('ADD', '_add', _('Add'), _('Add new'), ICON_ADD, 'default',
				'editor_view');

		if ((bool)$this->options['insert'])
			$this->_add_action('NEW', '_add', _('New'), _('Add new'), ICON_ADD, 'default',
				'editor_view');

		if ((bool)$this->options['clone'])
			$this->_add_action('CLONE', '_cloning', _('Clone'), _('Clone'), ICON_ADD, '',
				'editor_view');

		$this->data_set = $data_set;
		$this->fields = $data_set->fields;

//		$this->_prev_status();
	}

	/**
	 * @param string $name
	 * @param string $handler
	 * @param string|null $but_value
	 * @param string|bool|null $but_title
	 * @param string|bool|null $but_icon
	 * @param string|null $aspect
	 * @param string|null $view
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function _add_action($name, $handler, $but_value=null, $but_title=false, $but_icon=false, $aspect='', $view=null)
	{
		$this->pre_handlers[$name] = $handler;

		if ($but_value)
			$this->tool_buttons[$name] = array($but_value, $but_title, $but_icon, $aspect);

		if ($view)
			$this->_add_mode($name, $view);
	}

	/**
	 * @param string $name
	 * @param string $view
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function _add_mode($name, $view)
	{
		$this->views[$name] = $view;
	}

	/** @return void */
	function _prev_status()
	{
		// Restore previous mode/key (obsolete for views stored in session)

		$mod = get_post($this->name.'Mode', $this->Mode);
		$new_mode = $mod;
		if ((bool)$mod) {
			if (is_array($mod)) {
				$val = key($mod);
				$this->selected_id = @quoted_printable_decode((string) $val);
				$new_mode = $mod[$val];
			} else {
				$this->selected_id = $this->_none;
			}
		}
		$this->Mode = (string) $new_mode;
	}

	/** @return ''|'RESET'|key-of<array<array-key, mixed>> */
	function _check_mode()
	{

		$mod = '';//$this->Mode;
		// Detect action (mode change)
		foreach (array_keys($this->pre_handlers) as $m) { // check button controls

			if (isset($_POST[$this->name.$m])) {
				unset($_POST['_focus']); // focus on first form entry
				ajax()->activate($this->name.'_div');
				ajax()->activate($this->name.'_controls');
				$post_val = $_POST[$this->name.$m];
				$val = is_array($post_val) ? key($post_val) : null;
				$this->prev_id = $this->selected_id;
				$this->selected_id = $val!==null ? @quoted_printable_decode((string) $val) : $this->_none;
				$mod = $m; break;
			}
		}
		if (!(bool)$mod && ($_SERVER['REQUEST_METHOD'] ?? '') == 'GET') // initialize on every GET
			$mod = 'RESET';

		return $mod;
	}

	/** @return void */
	function display_error()
	{
		$this->errors = array_merge($this->data_set->errors, $this->errors);
		$firsterr = reset($this->errors);
		$field = key($this->errors);

		if (!is_numeric($field))
			set_focus($this->name.(string)$field);
		display_error($firsterr);

		$this->errors = array(); // clean up to prevent false errors on object reuse
	}
	//
	//	Set record for edition
	//
	/**
	 * @param string|null $mode
	 * @return void
	 */
	function _edit($mode)
	{
		if ($this->selected_id != $this->prev_id || $mode != $this->Mode) { // get record for edition
			$this->data = $this->data_set->get($this->selected_id !== $this->_none ? $this->selected_id : null);
			$this->set_output();
		}
//		if ($this->Mode != $mode) {
//		}

//		else
//			$this->display_error();

		$this->Mode = (string) $mode;
	}
	//
	//	Update record after edition
	//
	/**
	 * @param string|null $mode
	 * @return void
	 */
	function _update($mode)
	{
		if (!(bool)$this->options['update'])
			return;

		$this->data = $this->get_input();
		$data = $this->data;
		if ($this->data_set->update_check($this->selected_id, $data) && $this->data_set->update($this->selected_id, $data)) {
				$this->selected_id = $this->_none;
				$this->Mode = 'RESET';
				$this->_cancel();
				if (is_string($this->options['update']))
					display_notification($this->options['update']);
				return;
		}
		else
			$this->display_error();

		$this->Mode = (string) $mode;
	}
	//
	//	Add new record
	//
	/**
	 * @param string|null $mode
	 * @return void
	 */
	function _add($mode)
	{
		if (!(bool)$this->options['insert'])
			return;

		if ($mode == 'ADD') {
			$this->data = $this->get_input();
			$data = $this->data;
			if ($this->data_set->insert_check($data) && ($this->data_set->insert($data) !== false)) {
				$this->_cancel();
				if (is_string($this->options['insert']))
					display_notification($this->options['insert']);
				$this->Mode = 'RESET';
				return;
			}
			else
				$this->display_error();
		} else {// mode==NEW
				$this->data = $this->data_set->get();
		}
		$this->Mode = 'ADD';
	}
	//
	//	Delete selected  record
	//
	/** @return void */
	function _delete()
	{
		if (!(bool)$this->options['delete'])
			return;

		if ($this->data_set->delete_check($this->selected_id) && $this->data_set->delete($this->selected_id))
		{
			if (is_string($this->options['delete']))
				display_notification($this->options['delete']);
		} else
			$this->display_error();
		$this->_cancel();
	}
	//
	//	Return to listing view
	//
	/** @return void */
	function _cancel()
	{
		$this->selected_id = $this->_none;
		if ($this->display_both)
		{
			$this->data = $this->data_set->get();
			$this->set_output();
		}
		$this->cancel();
		if (!$this->display_both)
			ajax()->activate($this->name.'_div');
		$this->Mode = 'RESET';
	}
	//
	// Clone record for new edition
	//
	/** @return void */
	function _cloning()
	{
		if (!(bool)$this->options['clone'])
			return;
		$this->Mode = 'ADD';
		$this->_edit('Edit');
		$this->selected_id = $this->_none;
	}
	/*
		Generate form controls
	*/

	/**
	 * @param bool $list_view
	 * @return void
	 */
	function _record_controls($list_view = false)
	{
		$clone = (bool)$this->options['clone'] && $this->selected_id != $this->_none;

		div_start($this->name.'_controls');
		echo "<center>";

		if ($list_view)
			$this->action_button('NEW');
		else {
			if ($this->Mode == 'NEW' || $this->selected_id==$this->_none)
			{
				$this->action_button('ADD');
				$this->action_button('RESET');
			} else {
				$this->action_button('UPDATE', $this->selected_id);
				if ($clone && $this->display_both) 
					$this->action_button('CLONE', $this->selected_id);
				$this->action_button('RESET');
			}
		}

		echo "</center>";
		div_end();
	}

	//===========================================================================
	// Public functions
	//

	//
	//	Submit buttons for form actions
	//
	/**
	 * @param string $action
	 * @param string|null $selected_id
	 * @return void
	 */
	function action_button($action, $selected_id=null)
	{
		list($value, $title, $icon, $aspect) = $this->tool_buttons[$action];
		submit($this->name.$action.(isset($selected_id) ? "[$selected_id]" : ''), $value, true, $title, $aspect, $icon);
	}

	//
	//	Tool button for grid line actions.
	//
	/**
	 * @param string $name
	 * @param string|null $selected_id
	 * @param string|null $params
	 * @return non-falsy-string
	 */
	function tool_button($name, $selected_id=null, $params='')
	{
		$b = $this->tool_buttons[$name];

		return "<td align='center' $params>"
			.button( "{$this->name}$name"
				.($selected_id === null || $selected_id === $this->_none ? '': "[$selected_id]"),
				$b[0], $b[1], $b[2], $b[3])."</td>";
	}
	//
	//	Main function - display current CRUD editor content
	//
	/** @param string|null $Mode
	 *  @return void */
	function show($Mode=null)
	{
		if (!isset($Mode))
			$Mode = $this->_check_mode();

		div_start($this->name.'_div');

		if (array_key_exists($Mode, $this->pre_handlers)) {
			$fun = $this->pre_handlers[$Mode];
			$this->$fun($Mode);
		}

		if (isset($this->views[$this->Mode]))
			$this->{$this->views[$this->Mode]}($Mode);
		else
			$this->{$this->views['']}($Mode); // default view

		// this is needed only when we use temporary crud object together with ajax screen updates
		hidden($this->name.'Mode'.'['.(string)$this->selected_id.']', $this->Mode);
		div_end();
	}
	
	//
	//	Optional things like focus set performed on edition cancel.
	//
	/**
	 * @psalm-mutation-free
	 * @return void
	 */
	function cancel()
	{
	}

	//
	//	Show database content in pager/table
	//	parameter $Mode contains new mode on change, or '' otherwise
	//
	/**
	 * @param string|null $Mode
	 * @return void|bool
	 */
	function list_view($Mode) {
		display_notification(__FUNCTION__. ' is not defined...');
	}

	//
	//	Show record editor screen content
	//	parameter $Mode contains new mode on change, or '' otherwise
	//
	/**
	 * @param string|null $Mode
	 * @return void
	 */
	function editor_view($Mode) {
		display_notification(__FUNCTION__. ' is not defined...');
	}
};

