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
//
// Sets local POST value and adds Value to ajax posting if needed
//
/*function set_post($name, $value, $ajax_trigger=true) {
    global $Ajax;

    $_POST[$name] = $value;
    if ($ajax_trigger) $Ajax->activate($name);
}
*/
//------------------------------------------------------------------------------
//    Seek for _POST variable with $prefix.
//    If var is found returns variable name with prefix stripped,
//    and null or -1 otherwise.
//
function find_submit(string|int|float|bool|null $prefix, string|int|float|bool|null $numeric=true): int|string|null
{

    foreach($_POST as $postkey=>$postval )
    {
		if (strpos($postkey, $prefix) === 0)
		{
			$id = substr($postkey, strlen($prefix));
			return (bool)$numeric ? (int)$id : $id;
		}
    }
    return (bool)$numeric ? -1 : null;
}
/*
	Helper function.
	Returns true if input $name with $submit_on_change option set is subject to update.
*/
function input_changed(?string $name): bool
{
	return isset($_POST['_'.$name.'_changed']);
}

//------------------------------------------------------------------------------
//
// Helper function for simple db table editor pages
//
function simple_page_mode(bool $numeric_id = true): void
{
	global $Mode, $selected_id;

	$default = $numeric_id ? -1 : '';
	$selected_id = get_post('selected_id', $default);
	foreach (array('ADD_ITEM', 'UPDATE_ITEM', 'RESET', 'CLONE') as $m) {
		if (isset($_POST[$m])) {
			ajax()->activate('_page_body');
			if ($m == 'RESET'  || $m == 'CLONE') 
				$selected_id = $default;
			unset($_POST['_focus']);
			$Mode = $m; return;
		}
	}
	foreach (array('Edit', 'Delete') as $m) {
		foreach ($_POST as $p => $pvar) {
			if (strpos($p, $m) === 0) {
//				$selected_id = strtr(substr($p, strlen($m)), array('%2E'=>'.'));
				unset($_POST['_focus']); // focus on first form entry
				$selected_id = quoted_printable_decode(substr($p, strlen($m)));
				ajax()->activate('_page_body');
				$Mode = $m;
				return;
			}
		}
	}
	$Mode = '';
}

//------------------------------------------------------------------------------
//
//	Read numeric value from user formatted input
//
/**
 * A posted value that a function taking a scalar can use: the value itself, or null when the field was not posted
 * or was posted as an array. Reading $_POST['name'] directly raises "Undefined array key" for a missing field and
 * hands an array to a scalar parameter.
 *
 * @return string|int|float|bool|null
 */
function post_scalar(string $name): string|int|float|bool|null
{
	$value = $_POST[$name] ?? null;
	return is_array($value) ? null : $value;
}

/**
 * The query string counterpart of post_scalar().
 *
 * @return string|int|float|bool|null
 */
function get_scalar(string $name): string|int|float|bool|null
{
	$value = $_GET[$name] ?? null;
	return is_array($value) ? null : $value;
}

/**
 * @template T
 * @param T $dflt
 * @return T|int|float|false
 */
function input_num(?string $postname=null, int|float|string|null $dflt=0)
{
	if ($postname === null || !isset($_POST[$postname]) || $_POST[$postname] == "")
	  	return $dflt;

    return user_numeric($_POST[$postname]);
}

//---------------------------------------------------------------------------------
//
//	Thanks to hidden fields buffering hidden() helper can be used in arbitrary places and 
//  proper html structure is still preserved. Buffered hidden fields are output on the nearest 
//  table or form closing tag (see output_hidden()).
//
$hidden_fields = array();

function hidden(string|int|float|bool|null $name, string|int|float|bool|null $value=null, bool $echo=true): ?string
{
  	global $hidden_fields;
	
	if ($value === null) 
		$value = get_post($name);
	
  	$ret = "<input type=\"hidden\" name=\"$name\" value=\"$value\">";
	ajax()->addUpdate($name, $name, $value);
	if ($echo) {
		$hidden_fields[] = $ret;
		return null;
	}
	return $ret;
}
/*
	Universal submit form button.
	$atype - type of submit:
	 Normal submit:
		false - normal button; optional icon
		null  - button visible only in fallback mode; optional icon
	 Ajax submit:
		true	  - standard button; optional icon

		'default' - default form submit on Ctrl-Enter press; dflt ICON_OK icon
		'selector' - ditto with closing current popup editor window
		'cancel'  - cancel form entry on Escape press; dflt ICON_CANCEL
		'process' - displays progress bar during call; optional icon
		'nonajax' - ditto, non-ajax submit

	$atype can contain also multiply type selectors separated by space, 
	however make sense only combination of 'process' and one of defualt/selector/cancel
*/
/**
 * @return non-falsy-string|null
 */
function submit(string|int|float|bool|null $name, string|int|float|bool|null $value, string|int|float|bool|null $echo=true, string|int|float|bool|null $title=false, string|int|float|bool|null $atype=false, string|int|float|bool|null $icon=false)
{
	global $path_to_root;

	$aspect='';
	$hide = '';
	if ($atype === null) {
		$aspect = fallback_mode() ? " aspect='fallback'" : '';
		$hide = fallback_mode() ? '' : ' fa-none';

	} elseif (!is_bool($atype)) { // necessary: switch uses '=='

		$aspect = " aspect='$atype' ";
		$types = explode(' ', $atype);

		foreach ($types as $type) {
			switch($type) {
				case 'selector':
					$aspect = " aspect='selector' rel = '$value'"; 
					$value = _("Select");
					if ($icon===false) $icon=ICON_SUBMIT; break;

				case 'default':
					if ($icon===false) $icon=ICON_SUBMIT; break;

				case 'cancel':
					if ($icon===false) $icon=ICON_ESCAPE; break;

				case 'nonajax':
				case 'download':
					$atype = false;
			}
		}
	}
	$submit_str = "<button class=\""
	    .((bool)$atype ? 'ajaxsubmit' : 'inputsubmit') . $hide
		."\" type=\"submit\""
		.$aspect
	    ." name=\"$name\"  id=\"$name\" value=\"$value\""
	    .((bool)$title ? " title='$title'" : '')
	    .">"
		.((bool)$icon ? "<img src='$path_to_root/themes/".user_theme()."/images/$icon' height='12' alt=''>" : '')
		."<span>$value</span>"
		."</button>\n";
	if ((bool)$echo)
		echo $submit_str;
	else
		return $submit_str;
}

function submit_center(?string $name, ?string $value, bool $echo=true, string|bool|null $title=false, string|bool|null $async=false, bool $icon=false): void
{
	if ($echo) echo "<center>";
	submit($name, $value, $echo, $title, $async, $icon);
	if ($echo) echo "</center>";
}

function submit_center_first(?string $name, string|int|float|bool|null $value, string|bool|null $title=false, string|bool|null $async=false, string|bool|null $icon=false): void
{
	echo "<center>";
	submit($name, $value, true, $title, $async, $icon);
	echo "&nbsp;";
}

function submit_center_last(?string $name, ?string $value, string|bool|null $title=false, string|bool|null $async=false, string|bool|null $icon=false): void
{
	echo "&nbsp;";
	submit($name, $value, true, $title, $async, $icon);
	echo "</center>";
}
/*
	For following controls:
	'both' - use both Ctrl-Enter and Escape hotkeys 
	'upgrade' - use Ctrl-Enter with progress ajax indicator and Escape hotkeys. Nonajax request for OK option is performed.
	'cancel' - apply to 'RESET' button
*/
function submit_add_or_update(string|int|float|bool|null $add=true, string|int|float|bool|null $title=false, string|int|float|bool|null $async=false, string|int|float|bool|null $clone=false): void
{
	$cancel = $async;

	if ($async === 'both') {
		$async = 'default'; $cancel = 'cancel';
	}
	elseif ($async === 'upgrade') {
		$async = 'default nonajax process'; $cancel = 'cancel';
	}
	elseif ($async === 'default')
		$cancel = true;
	elseif ($async === 'cancel')
		$async = true;
	
	if ((bool)$add)
		submit('ADD_ITEM', _("Add new"), true, $title, $async);
	else {
		submit('UPDATE_ITEM', _("Update"), true, _('Submit changes'), $async);
		if ((bool)$clone) submit('CLONE', _("Clone"), true, 
			_('Edit new record with current data'), $async);
		submit('RESET', _("Cancel"), true, _('Cancel edition'), $cancel);
	}
}

function submit_add_or_update_center(bool $add=true, string|bool|null $title=false, string|bool|null $async=false, bool $clone=false): void
{
	echo "<center>";
	submit_add_or_update($add, $title, $async, $clone);
	echo "</center>";
}

function submit_add_or_update_row(string|int|float|bool|null $add=true, string|int|float|bool|null $right=true, string|int|float|bool|null $extra="", string|int|float|bool|null $title=false, string|int|float|bool|null $async=false, string|int|float|bool|null $clone = false): void
{
	echo "<tr>";
	if ((bool)$right)
		echo "<td>&nbsp;</td>\n";
	echo "<td $extra>";
	submit_add_or_update($add, $title, $async, $clone);
	echo "</td></tr>\n";
}

function submit_cells(?string $name, ?string $value, ?string $extra="", string|bool|null $title=false, string|bool|null $async=false): void
{
	echo "<td $extra>";
	submit($name, $value, true, $title, $async);
	echo "</td>\n";
}

function submit_row(?string $name, ?string $value, bool $right=true, ?string $extra="", string|bool|null $title=false, string|bool|null $async=false): void
{
	echo "<tr>";
	if ($right)
		echo "<td>&nbsp;</td>\n";
	submit_cells($name, $value, $extra, $title, $async);
	echo "</tr>\n";
}

function submit_return(?string $name, string|int|float|bool|null $value, string|bool|null $title=false): void
{
	if ((bool)(@$_REQUEST['popup'])) {
		submit($name, $value, true, $title, 'selector');
	}
}

function submit_js_confirm(?string $name, ?string $msg, bool $set = true): void {
	if (in_ajax()) {
		ajax()->addState(true, 'confirm', $set ? (string)$msg : null, $name);
	} else
		add_js_source("_validate.$name=".($set ? "function(){ return confirm('"
				. strtr((string)$msg, array("\n"=>'\\n')) . "');};"
				: 'null;'));
}
//-----------------------------------------------------------------------------------

function set_icon(string|int|float|bool|null $icon, string|int|float|bool|null $title=false): string
{
	global $path_to_root;
	if (basename($icon) === $icon) // standard icons does not contain path separator
		$icon = "$path_to_root/themes/".user_theme()."/images/$icon";
	return "<img src='$icon' class='fa-icon12'".((bool)$title ? " title='$title'" : "")." >\n";	
}

function button(string|int|float|bool|null $name, string|int|float|bool|null $value, string|int|float|bool|null $title=false, string|int|float|bool|null $icon=false,  string|int|float|bool|null $aspect=''): string
{
	// php silently changes dots,spaces,'[' and characters 128-159
	// to underscore in POST names, to maintain compatibility with register_globals
	$rel = '';
	if ($aspect == 'selector') {
		$rel = " rel='$value'";
		$value = _("Select");
	}
	if (user_graphic_links() && (bool)$icon)
	{
		if ($value == _("Delete")) // Helper during implementation
			$icon = ICON_DELETE;
		return "<button type='submit' class='editbutton' name='"
			.html_specials_encode(strtr($name, array('.'=>'=2E', '='=>'=3D',// ' '=>'=20','['=>'=5B'
			)))
			."' value='1'" . ((bool)$title ? " title='$title'":" title='$value'")
			. ((bool)$aspect ? " aspect='$aspect'" : '')
			. $rel
			." >".set_icon($icon)."</button>\n";
	}
	else
		return "<input type='submit' class='editbutton' name='"
			.html_specials_encode(strtr($name, array('.'=>'=2E', '='=>'=3D',// ' '=>'=20','['=>'=5B'
			)))
			."' value='$value'"
			.((bool)$title ? " title='$title'":'')
			. ((bool)$aspect ? " aspect='$aspect'" : '')
			. $rel
			." >\n";
}

function button_cell(string|int|float|bool|null $name, string|int|float|bool|null $value, string|int|float|bool|null $title=false, string|int|float|bool|null $icon=false, ?string $aspect=''): void
{
	echo "<td align='center'>";
	echo button($name, $value, $title, $icon, $aspect);
	echo "</td>";
}

function delete_button_cell(string|int|float|bool|null $name, string|int|float|bool|null $value, string|bool|null $title=false): void
{
	button_cell($name, $value, $title, ICON_DELETE);
}

function edit_button_cell(string|int|float|bool|null $name, string|int|float|bool|null $value, string|bool|null $title=false): void
{
	button_cell($name, $value, $title, ICON_EDIT);
}

function select_button_cell(string|int|float|bool|null $name, string|int|float|bool|null $value, string|int|float|bool|null $title=false): void
{
	button_cell($name, $value, $title, ICON_ADD, 'selector');
}
//-----------------------------------------------------------------------------------

function check_value(string|array $name): int|array
{
	if (is_array($name)) {
		$ret = array();
		foreach($name as $key)
			$ret[$key] = check_value($key);
		return $ret;
	} else
		return (!(bool)($_POST[$name] ?? null) ? 0 : 1);
}

function checkbox(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value=null, string|int|float|bool|null $submit_on_change=false, string|int|float|bool|null $title=false): string
{

	$str = '';	

	if ((bool)$label)
		$str .= $label . "  ";
	if ($submit_on_change !== false) {
		if ($submit_on_change === true)
			$submit_on_change = 'update';
	}
	if ($value === null)
		$value = get_post($name,0);

	$str .= "<input"
	    .($value == 1 ? ' checked':'')
	    ." type='checkbox' name='$name' value='1'"
	    .($submit_on_change === 'update' ? fa_action_attrs('submit-update', array($name)) : ((bool)$submit_on_change ? " onclick='$submit_on_change'" : ''))
	    .((bool)$title ? " title='$title'" : '')
	    ." >\n";

	ajax()->addUpdate($name, $name, $value);
	return $str;
}

function check(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value=null, string|int|float|bool|null $submit_on_change=false, string|int|float|bool|null $title=false): void
{
	echo checkbox($label, $name, $value, $submit_on_change, $title);
}

function check_cells(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value=null, bool $submit_on_change=false, string|bool|null $title=false,
	?string $params=''): void
{
	if ($label != null)
		echo "<td>$label</td>\n";
	echo "<td $params>";
	echo check(null, $name, $value, $submit_on_change, $title);
	echo "</td>";
}

function check_row(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value=null, bool $submit_on_change=false, string|bool|null $title=false): void
{
	echo "<tr><td class='label'>$label</td>";
	echo check_cells(NULL, $name, $value, $submit_on_change, $title);
	echo "</tr>\n";
}

//-----------------------------------------------------------------------------------
function radio(?string $label, ?string $name, string|int|float|bool|null $value, ?bool $selected=null, bool $submit_on_change=false): string
{
	if (!isset($selected))
		$selected = get_post($name) === (string)$value;

	return "<input type='radio' name=$name value='$value' ".($selected ? "checked":'')
	    .($submit_on_change ? fa_action_attrs('submit-update', array($name)) : '')
		.">".((bool)$label ? $label : '');
}

//-----------------------------------------------------------------------------------
function labelheader_cell(string|int|float|bool|null $label, ?string $params=""): void
{
	echo "<td class='tableheader' $params>$label</td>\n";
}

function label_cell(string|int|float|bool|null $label, ?string $params="", string|int|null $id=null): string|int|float|bool|null
{

	if(isset($id))
	{
	    $params .= " id='$id'";
	    ajax()->addUpdate($id, $id, $label);
	}
	echo "<td $params>$label</td>\n";

	return $label;
}

function email_cell(string|int|float|bool|null $label, string|int|float|bool|null $params="", string|int|float|bool|null $id=null): void
{
	label_cell("<a href='mailto:$label'>$label</a>", $params, $id);
}

function amount_decimal_cell(string|int|float|bool|null $label, ?string $params="", string|int|float|bool|null $id=null): void
{
	$dec = 0;
	label_cell(price_decimal_format($label, $dec), "nowrap align=right ".$params, $id);
}

function amount_cell(string|int|float|bool|null $label, ?bool $bold=false, string $params="", string|int|null $id=null): void
{
	if ($bold)
		label_cell("<b>".price_format($label)."</b>", "nowrap align=right ".$params, $id);
	else
		label_cell(price_format($label), "nowrap align=right ".$params, $id);
}

//JAM  Allow entered unit prices to be fractional
function unit_amount_cell(string|int|float|bool|null $label, string|int|float|bool|null $bold=false, string|int|float|bool|null $params="", string|int|float|bool|null $id=null): void
{
	if ((bool)$bold)
		label_cell("<b>".unit_price_format($label)."</b>", "nowrap align=right ".$params, $id);
	else
		label_cell(unit_price_format($label), "nowrap align=right ".$params, $id);
}


function percent_cell(string|int|float|bool|null $label, bool $bold=false, string|int|float|bool|null $id=null): void
{
	if ($bold)
		label_cell("<b>".percent_format($label)."</b>", "nowrap align=right", $id);
	else
		label_cell(percent_format($label), "nowrap align=right", $id);
}
// 2008-06-15. Changed
function qty_cell(string|int|float|bool|null $label, string|bool|null $bold=false, int|string|null $dec=null, string|int|null $id=null): void
{
	if (!isset($dec))
		$dec = get_qty_dec();
	if ((bool)$bold)
		label_cell("<b>".number_format2($label, $dec)."</b>", "nowrap align=right", $id);
	else
		label_cell(number_format2($label, $dec), "nowrap align=right", $id);
}

function label_cells(string|int|float|bool|null $label, string|int|float|bool|null $value, string|int|float|bool|null $params="", ?string $params2="", ?string $id=null): void
{
	if ($label != null)
		echo "<td $params>$label</td>\n";
	label_cell($value, $params2, $id);
}

function label_row(string|int|float|bool|null $label, string|int|float|bool|null $value, ?string $params="", ?string $params2="", string|int|null $leftfill=0, ?string $id=null): void
{
	echo "<tr>";
	if ($params == "")
	{
		echo "<td class='label'>$label</td>";
		$label = null;
	}	
	label_cells($label, $value, $params, $params2, $id);
	if ($leftfill!=0)
	  	echo "<td colspan=$leftfill></td>";
	echo "</tr>\n";
}

function text_input(string|int|float|bool|null $name, string|int|float|bool|array|null $value=null, string|int|float|bool|null $size='', string|int|float|bool|null $max='', string|int|float|bool|null $title='', string|int|float|bool|null $params=''): string
{
	if ($value === null)
		$value = get_post($name);

	return "<input $params type=\"text\" name=\"$name\" size=\"$size\" maxlength=\"$max\" value=\"$value\""
	    .((bool)$title ? " title='$title'" : '')
	    .">";
}

//-----------------------------------------------------------------------------------

function text_cells(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value=null, string|int|float|bool|null $size="", string|int|float|bool|null $max="", string|int|float|bool|null $title=false, 
	string|int|float|bool|null $labparams="", string|int|float|bool|null $post_label="", string $inparams=""): void
{

	default_focus($name);
	if ($label != null)
		label_cell($label, $labparams);
	echo "<td>";

	echo text_input($name, $value, $size, $max, $title, $inparams);

	if ($post_label != "")
		echo " " . $post_label;

	echo "</td>\n";
	ajax()->addUpdate($name, $name, $value);
}

function text_cells_ex(?string $label, string|int|float|bool|null $name, string|int|float|bool|null $size, string|int|float|bool|null $max=null, string|int|float|bool|null $init=null, string|int|float|bool|null $title=null,
	string|int|float|bool|null $labparams=null, string|int|float|bool|null $post_label=null, string|bool|null $submit_on_change=false): void
{

	default_focus($name);
	if (!isset($_POST[$name]) || $_POST[$name] == "")
	{
		if ((bool)$init)
			$_POST[$name] = $init;
		else
			$_POST[$name] = "";
	}
	if ($label != null)
		label_cell($label, $labparams);

	if (!isset($max))
		$max = $size;

	echo "<td>";
	$class = (bool)$submit_on_change ? 'class="searchbox"' : '';
	echo "<input $class type=\"text\" name=\"$name\" size=\"$size\" maxlength=\"$max\" value=\"" . $_POST[$name]. "\""
	 .((bool)$title ? " title='$title'": '')." >";

	if ((bool)$post_label)
		echo " " . $post_label;

	echo "</td>\n";
	ajax()->addUpdate($name, $name, $_POST[$name]);
}

function text_row(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value, string|int|float|bool|null $size, string|int|float|bool|null $max, string|int|float|bool|null $title=null, ?string $params="", ?string $post_label=""): void
{
	echo "<tr><td class='label'>$label</td>";
	text_cells(null, $name, $value, $size, $max, $title, $params, $post_label);

	echo "</tr>\n";
}

//-----------------------------------------------------------------------------------

function text_row_ex(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $size, string|int|float|bool|null $max=null, string|int|float|bool|null $title=null, string|int|float|bool|null $value=null, string|int|float|bool|null $params=null, string|int|float|bool|null $post_label=null): void
{
	echo "<tr><td class='label'>$label</td>";
	text_cells_ex(null, $name, $size, $max, $value, $title, $params, $post_label);

	echo "</tr>\n";
}

//-----------------------------------------------------------------------------------
function email_row(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value, string|int|float|bool|null $size, string|int|float|bool|null $max, string|int|float|bool|null $title=null, ?string $params="", ?string $post_label=""): void
{
	if (get_post($name)) 
		$label = "<a href='Mailto:".$_POST[$name]."'>$label</a>";
	text_row($label, $name, $value, $size, $max, $title, $params, $post_label);
}

function email_row_ex(?string $label, ?string $name, ?int $size, ?int $max=null, string|int|float|bool|null $title=null, string|int|float|bool|null $value=null, string|int|float|bool|null $params=null, string|int|float|bool|null $post_label=null): void
{
	if (get_post($name)) 
		$label = "<a href='Mailto:".$_POST[$name]."'>$label</a>";
	text_row_ex($label, $name, $size, $max, $title, $value, $params, $post_label);
}

function link_row(?string $label, ?string $name, string|int|float|bool|null $value, ?int $size, ?int $max, string|int|float|bool|null $title=null, ?string $params="", ?string $post_label=""): void
{
	$val = get_post($name);
	if ($val) {
		if (strpos($val,'http://')===false)
			$val = 'http://'.$val;
		$label = "<a href='$val' target='_blank'>$label</a>";
	}
	text_row($label, $name, $value, $size, $max, $title, $params, $post_label);
}

function link_row_ex(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $size, string|int|float|bool|null $max=null, string|int|float|bool|null $title=null, string|int|float|bool|null $value=null, string|int|float|bool|null $params=null, string|int|float|bool|null $post_label=null): void
{
	$val = get_post($name);
	if ($val) {
		if (strpos($val,'http://')===false)
			$val = 'http://'.$val;
		$label = "<a href='$val' target='_blank'>$label</a>";
	}
	text_row_ex($label, $name, $size, $max, $title, $value, $params, $post_label);
}

//-----------------------------------------------------------------------------------
//
//	Since FA 2.2  $init parameter is superseded by $check. 
//  When $check!=null current date is displayed in red when set to other 
//	than current date.
//	
function date_cells(?string $label, string|int|float|bool|null $name, ?string $title = null, string|bool|null $check=null, string|int|float|bool|null $inc_days=0, 
	?int $inc_months=0, ?int $inc_years=0, ?string $params=null, bool $submit_on_change=false): void
{
	global $path_to_root;

	if (!isset($_POST[$name]) || $_POST[$name] == "")
	{
		if ($inc_years == 1001)
			$_POST[$name] = null;
		else
		{
			$dd = Today();
			if ($inc_days != 0)
				$dd = add_days($dd, $inc_days);
			if ($inc_months != 0)
				$dd = add_months($dd, $inc_months);
			if ($inc_years != 0)
				$dd = add_years($dd, $inc_years);
			$_POST[$name] = $dd;
		}
	}
	if (user_use_date_picker())
	{
		$calc_image = (file_exists("$path_to_root/themes/".user_theme()."/images/cal.gif")) ? 
			"$path_to_root/themes/".user_theme()."/images/cal.gif" : "$path_to_root/themes/default/images/cal.gif";
		$post_label = "<a tabindex='-1' href='#'".fa_action_attrs('date-picker', array($name)).">"
		. "	<img src='$calc_image' class='fa-icon16' alt='"._('Click Here to Pick up the date')."'></a>\n";
	}	
	else
		$post_label = "";

	if ($label != null)
		label_cell($label, $params);

	echo "<td>";
	
	$class = $submit_on_change ? 'date active' : 'date';

	$aspect = (bool)$check ? 'aspect="cdate"' : '';
	if ((bool)$check && (get_post($name) != Today()))
		$class .= ' fa-red';

	default_focus($name);
	$size = (user_date_format()>3)?11:10; 
	echo "<input type=\"text\" name=\"$name\" class=\"$class\" $aspect size=\"$size\" maxlength=\"12\" value=\"" 
	 . $_POST[$name]. "\""
	 .((bool)$title ? " title='$title'": '')." > $post_label";
	echo "</td>\n";
	ajax()->addUpdate($name, $name, $_POST[$name]);
}

function date_row(string|int|float|bool|null $label, string|int|float|bool|null $name, ?string $title=null, string|bool|null $check=null, string|int|float|bool|null $inc_days=0, ?int $inc_months=0, 
	?int $inc_years=0, ?string $params=null, bool $submit_on_change=false): void
{
	echo "<tr><td class='label'>$label</td>";
	date_cells(null, $name, $title, $check, $inc_days, $inc_months, 
		$inc_years, $params, $submit_on_change);
	echo "</tr>\n";
}

//-----------------------------------------------------------------------------------
function password_row(?string $label, ?string $name, string|int|float|bool|null $value): void
{
	echo "<tr><td class='label'>$label</td>";
	label_cell("<input type='password' name='$name' size=32 maxlength=32 value='$value' >");
	echo "</tr>\n";
}	

//-----------------------------------------------------------------------------------
function file_cells(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $id=""): void
{
	if ($id != "")
		$id = "id='$id'";
	label_cells($label, "<input type='file' name='$name' $id >");
}		
function file_row(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $id = ""): void
{
	echo "<tr><td class='label'>$label</td>";
	file_cells(null, $name, $id);
	echo "</tr>\n";
}	

/*-----------------------------------------------------------------------------------

 Reference number input.

 Optional  $context array contains transaction data used in number parsing:
 	'data' - data used for month/year codes
	'location' - location code
 	'customer' - debtor_no
 	'supplier' - supplier id
 	'branch' - branch_code
*/
function ref_cells(?string $label, ?string $name, ?string $title=null, string|int|float|bool|null $init=null, ?string $params=null, string|bool|null $submit_on_change=false, string|int|float|bool|null $type=null, string|array|null $context=null): void
{

	if (isset($type)) {
		if (!(bool)($_POST[$name.'_list'] ?? null)) // restore refline id
			$_POST[$name.'_list'] = refs()->reflines->find_refline_id(!(bool)($_POST[$name] ?? null) ? $init : $_POST[$name], $type);

		if (!(bool)($_POST[$name] ?? null)) // initialization
		{
			if (isset($init))
			{
				$_POST[$name] = $init;
			} else {
				$_POST[$name] = refs()->get_next($type, $_POST[$name.'_list'], $context);
			}
			ajax()->addUpdate(true, $name, $_POST[$name]);
		}

		if (check_ui_refresh($name)) { // call context changed
			$_POST[$name] = refs()->normalize($_POST[$name], $type, $context, $_POST[$name.'_list']);
			ajax()->addUpdate(true, $name, $_POST[$name]);
		}

		if (refs()->reflines->count($type)>1) {
			if (list_updated($name.'_list')) {
				$_POST[$name] = refs()->get_next($type, $_POST[$name.'_list'], $context);
				ajax()->addUpdate(true, $name, $_POST[$name]);
			}
			$list = refline_list($name.'_list', $type);
		} else {
			$list = '';
		}

		if (isset($label))
			label_cell($label, $params);

		label_cell($list."<input name='".$name."' "
			.(check_edit_access($name) ? '' : 'disabled ')
			."value='".@$_POST[$name]."' size=16 maxlength=35>");
	}
	else // just wildcard ref field (e.g. for global inquires)
	{
		text_cells_ex($label, $name, 16, 35, $init, $title, $params, null, $submit_on_change);
	}
}

//-----------------------------------------------------------------------------------

function ref_row(?string $label, ?string $name, ?string $title=null, string|int|float|bool|null $init=null, string|bool|null $submit_on_change=false, string|int|float|bool|null $type=null, string|array|null $context = null): void
{
	echo "<tr><td class='label'>$label</td>";
	ref_cells(null, $name, $title, $init, null, $submit_on_change, $type, $context);
	echo "</tr>\n";
}

//-----------------------------------------------------------------------------------

function percent_row(?string $label, ?string $name, string|array|null $init=null): void
{

	if (!isset($_POST[$name]) || $_POST[$name]=="")
	{
		$_POST[$name] = $init == null ? '' : $init;
	}

	small_amount_row($label, $name, $_POST[$name], null, "%", user_percent_dec());
}

function amount_cells_ex(string|int|float|bool|null $label, string|int|float|bool|null $name, ?int $size, ?int $max=null, string|int|float|bool|null $init=null, string|int|float|bool|null $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{

	if (!isset($dec))
	  	$dec = user_price_dec();
	if (!isset($_POST[$name]) || $_POST[$name] == "")
	{
		if ($init !== null)
			$_POST[$name] = $init;
		else
			$_POST[$name] = '';
	}
	if ($label != null)
	{
		if ($params == null)
			$params = "class='label'";
		label_cell($label, $params);
	}
	if (!isset($max))
		$max = $size;

	if ($label != null)
		echo "<td>";
	else
		echo "<td align='right'>";

	echo "<input class='amount' type=\"text\" name=\"$name\" size=\"$size\" maxlength=\"$max\" dec=\"$dec\" value=\"" . $_POST[$name]. "\">";

	if ((bool)$post_label) {
		echo "<span id='_{$name}_label'> $post_label</span>";
		ajax()->addUpdate($name, '_'.$name.'_label', $post_label);
	}
	echo "</td>\n";
	ajax()->addUpdate($name, $name, $_POST[$name]);
	ajax()->addAssign($name, $name, 'dec', $dec);
}


//-----------------------------------------------------------------------------------

function amount_cells(string|int|float|bool|null $label, ?string $name, string|int|float|bool|null $init=null, string|int|null $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{
	amount_cells_ex($label, $name, 15, 15, $init, $params, $post_label, $dec);
}

//JAM  Allow entered unit prices to be fractional
function unit_amount_cells(?string $label, ?string $name, string|int|float|bool|null $init=null, ?string $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{
	if (!isset($dec))
	  	$dec = user_price_dec()+2;

	amount_cells_ex($label, $name, 15, 15, $init, $params, $post_label, $dec+2);
}

function amount_row(string|int|float|bool|null $label, ?string $name, string|int|float|bool|null $init=null, ?string $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{
	echo "<tr>";
	amount_cells($label, $name, $init, $params, $post_label, $dec);
	echo "</tr>\n";
}

function small_amount_row(?string $label, ?string $name, string|int|null $init=null, ?string $params=null, ?string $post_label=null, string|int|float|bool|null $dec=null): void
{
	echo "<tr>";
	small_amount_cells($label, $name, $init, $params, $post_label, $dec);
	echo "</tr>\n";
}

//-----------------------------------------------------------------------------------

function qty_cells(string|int|float|bool|null $label, string|int|float|bool|null $name, ?string $init=null, ?string $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{
	if (!isset($dec))
	  	$dec = user_qty_dec();

	amount_cells_ex($label, $name, 15, 15, $init, $params, $post_label, $dec);
}

function qty_row(?string $label, ?string $name, ?string $init=null, ?string $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{
	if (!isset($dec))
	  	$dec = user_qty_dec();

	echo "<tr>";
	amount_cells($label, $name, $init, $params, $post_label, $dec);
	echo "</tr>\n";
}

function small_qty_row(?string $label, ?string $name, string|int|float|bool|null $init=null, string|int|float|bool|null $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{
	if (!isset($dec))
	  	$dec = user_qty_dec();

	echo "<tr>";
	small_amount_cells($label, $name, $init, $params, $post_label, $dec);
	echo "</tr>\n";
}

//-----------------------------------------------------------------------------------

function small_amount_cells(?string $label, string|int|float|bool|null $name, string|int|float|bool|null $init=null, string|int|float|bool|null $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{
	amount_cells_ex($label, $name, 7, 12, $init, $params, $post_label, $dec);
}

//-----------------------------------------------------------------------------------

function small_qty_cells(string|int|float|bool|null $label, ?string $name, ?string $init=null, string|int|float|bool|null $params=null, string|int|float|bool|null $post_label=null, string|int|float|bool|null $dec=null): void
{
  	if (!isset($dec))
	  	$dec = user_qty_dec();
	amount_cells_ex($label, $name, 7, 12, $init, $params, $post_label, $dec);
}

//-----------------------------------------------------------------------------------

function textarea_cells(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value, string|int|float|bool|null $cols, string|int|float|bool|null $rows, ?int $max=255, ?string $title = null, ?string $params=""): void
{

	default_focus($name);
	if ($label != null)
		echo "<td $params>$label</td>\n";
	if ($value == null)
		$value = (!isset($_POST[$name]) ? "" : $_POST[$name]);
	echo "<td><textarea name='$name' cols='$cols' rows='$rows'"
	.((bool)$title ? " title='$title'" : '')
	.((bool)$max ? " maxlength=$max" : '')
	.">$value</textarea></td>\n";
	ajax()->addUpdate($name, $name, $value);
}

function textarea_row(string|int|float|bool|null $label, string|int|float|bool|null $name, string|int|float|bool|null $value, string|int|float|bool|null $cols, string|int|float|bool|null $rows, ?int $max=255, ?string $title=null, ?string $params=""): void
{
	echo "<tr><td class='label'>$label</td>";
	textarea_cells(null, $name, $value, $cols, $rows, $max, $title, $params);
	echo "</tr>\n";
}

//-----------------------------------------------------------------------------------
//
//	When show_inactive page option is set 
//  displays value of inactive field as checkbox cell.
//  Also updates database record after status change.
//
function inactive_control_cell(string|int|float|bool|null $id, string|int|float|bool|null $value, string|int|float|bool|null $table, string|int|float|bool|null $key): void
{

	$name = "Inactive". $id;
	$value = (bool)$value ? 1:0;

	if (check_value('show_inactive')) {
		if (isset($_POST['LInact'][$id]) && (get_post('_Inactive'.$id.'_update') || 
			get_post('Update')) && (check_value('Inactive'.$id) != $value)) {
			update_record_status($id, !$value, $table, $key);
		}
		echo '<td align="center">'. checkbox(null, $name, $value, true, '')
 			. hidden("LInact[$id]", $value, false) . '</td>';	
	}
}
//
//	Displays controls for optional display of inactive records
//
function inactive_control_row(array $th): void {
	echo  "<tr><td colspan=".(count($th)).">"
		."<div class='fa-left'>"
		. checkbox(null, 'show_inactive', null, true). _("Show also Inactive")
		."</div><div class='fa-right'>"
		. submit('Update', _('Update'), false, '', null)
		."</div></td></tr>";
}
//
//	Inserts additional column header when display of inactive records is on.
//
function inactive_control_column(array &$th): void {
	
	if (check_value('show_inactive')) 
		array_insert($th, count($th)-2 , _("Inactive"));
	if (get_post('_show_inactive_update')) {
		ajax()->activate('_page_body');
	}
}

function customer_credit_row(string|int|float|bool|array|null $customer, string|int|float|bool|null $credit, ?string $parms=''): void
{
	global $path_to_root;
	
	label_row( _("Current Credit:"),
		"<a target='_blank' " . ($credit<0 ? 'class="redfg"' : '')
		."href='$path_to_root/sales/inquiry/customer_inquiry.php?customer_id=".$customer."'"
		."".fa_action_attrs('open-window')." >"
		. price_format($credit)
		."</a>", $parms);
}

function supplier_credit_row(string|int|float|bool|null $supplier, string|int|float|bool|null $credit, ?string $parms=''): void
{
	global $path_to_root;
	
	label_row( _("Current Credit:"),
		"<a target='_blank' " . ($credit<0 ? 'class="redfg"' : '')
		."href='$path_to_root/purchasing/inquiry/supplier_inquiry.php?supplier_id=".$supplier."'"
		."".fa_action_attrs('open-window')." >"
		. price_format($credit)
		."</a>", $parms);
}

function bank_balance_row(string|int|float|bool|null $bank_acc, ?string $parms=''): void
{
	global $path_to_root;

	$to = add_days(Today(), 1);
	$bal = get_balance_before_for_bank_account($bank_acc, $to);
	label_row( _("Bank Balance:"),
		"<a target='_blank' " . ($bal<0 ? 'class="redfg"' : '')
		."href='$path_to_root/gl/inquiry/bank_inquiry.php?bank_account=".$bank_acc."'"
		."".fa_action_attrs('open-window')." >&nbsp;"
		. price_format($bal)
		."</a>", $parms);
}

function ahref(?string $label, ?string $href, ?string $target="", ?string $onclick=""): void {
  echo "<a href='$href' target='$target' onclick='$onclick'>$label</a>";
}

/**
 * A cell holding a link that runs an action from js/csp_actions.js (see fa_action_attrs()), for the pop-up lists that
 * used to use ahref_cell() with a javascript: URL and an onclick.
 *
 * @param array<int, mixed> $args
 */
function action_link_cell(?string $label, string $action, array $args = array()): void {
  echo "<td align='center'>&nbsp;&nbsp;<a href='#'" . fa_action_attrs($action, $args) . ">$label</a>&nbsp;&nbsp;</td>";
}

function ahref_cell(?string $label, ?string $href, ?string $target="", ?string $onclick=""): void {
  echo "<td align='center'>&nbsp;&nbsp;";
  ahref($label, $href, $target, $onclick);
  echo "&nbsp;&nbsp;</td>";
}
