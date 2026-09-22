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
require_once(dirname(__DIR__) . "/includes/JsHttpRequest.php");

class Ajax extends JsHttpRequest {

	/** @var array<int, array<string, mixed>> */
	var $aCommands = array();
	/** @var array<string, bool> */
	var $triggers = array();
	
    function __construct() 
    {
	  parent::__construct(@session_obj('language')->encoding);
    }
	//
	//	This function is used in ctrl routines to activate
	//	update of ajaxified html element selected by given name/id.
	//
	/** @return void */
	function activate($trigname) {
	  if (in_ajax()) {
		$this->triggers[$trigname] = true;
	  }
	}
	//
	//	Javascript clientside redirection.
	//	This is the last command added to reponse (if any).
	//
	/**
	 * @param string|null $url
	 * @return void
	 */
	function redirect($url) {
	    if(in_ajax()) {
			$this->_addCommand(true, array('n'=>'rd'), absolute_url($url));
		  	$this->run();
	    }
	}
	//
	// Popup window (target=_blank)
	//
	/**
	 * @param string|null $url
	 * @return void
	 */
	function popup($url) {
		  $this->_addCommand(true, array('n'=>'pu'), absolute_url($url));
	}
	//
	// Adds an executable Javascript code.
	//
	/**
	 * Sets page state on the client without sending code: 'editors' (the list of editors), 'processing' (the message
	 * shown when leaving a modified page, or null), 'modified' (0 or 1) or 'confirm' (a confirmation message for the
	 * validator called $name, or null). js/utils.js set_page_state() applies it.
	 *
	 * @param bool|string $trigger
	 * @param string $key
	 * @param mixed $value
	 * @param string|null $name
	 * @return static
	 */
	function addState($trigger, $key, $value, $name = null)
	{
		$this->_addCommand($trigger, array('n'=>'st', 't'=>$key, 'p'=>$name), $value);
		return $this;
	}
	//
	// Focus an element now (by name or id).
	//
	/**
	 * @param bool|string $trigger
	 * @param string $sTarget
	 * @return static
	 */
	function addFocusElement($trigger, $sTarget)
	{
		$this->_addCommand($trigger, array('n'=>'fe', 't'=>$sTarget), null);
		return $this;
	}
	//
	// Adds an executable Javascript code. Kept for extensions; it only works with a Content-Security-Policy that allows
	// eval, so FA itself uses addState() and addFocusElement().
	//
	/**
	 * @param string|bool|null $trigger
	 * @param string|null $sJS
	 * @return static
	 */
	function addScript($trigger, $sJS)
	{
		$this->_addCommand($trigger, array('n'=>'js'),$sJS);
		return $this;
	}
	//
	// Assign target attribute with data.
	//
	/**
	 * @param string|null $sAttribute
	 * @return static
	 */
	function addAssign($trigger, $sTarget,$sAttribute,$sData)
	{
		$this->_addCommand($trigger, array('n'=>'as','t'=>$sTarget,'p'=>$sAttribute),$sData);
		return $this;
	}
	//
	// Updates input element or label with data.
	//
	/** @return static */
	function addUpdate($trigger, $sTarget, $sData)
	{
		$this->_addCommand($trigger, array('n'=>'up','t'=>$sTarget),$sData);
		return $this;
	}
	//
	// Set disable state of element.
	//
	/**
	 * @param bool $trigger
	 * @param string|null $sTarget
	 * @param bool $sData
	 * @return static
	 */
	function addDisable($trigger, $sTarget, $sData=true)
	{
		$this->_addCommand($trigger, array('n'=>'di','t'=>$sTarget),$sData);
		return $this;
	}
	//
	// Set state of element to enabled.
	//
	/** @return static */
	function addEnable($trigger, $sTarget, $sData=true)
	{
		$this->_addCommand($trigger, array('n'=>'di','t'=>$sTarget), !$sData);
		return $this;
	}
	//
	// Set current focus.
	//
	/**
	 * @param bool $trigger
	 * @param string|null $sTarget
	 * @return static
	 */
	function addFocus($trigger, $sTarget)
	{
		$this->_addCommand($trigger, array('n'=>'fc'),$sTarget);
		return $this;
	}
	//
	// Internal procedure adding command to response.
	//
	/**
	 * @param array<array-key, mixed>|null $aAttributes
	 * @return void
	 */
	function _addCommand($trigger, $aAttributes, $mData)
	{
	  	if ($this->isActive() && ($trigger !== false)) {

			$aAttributes['why'] = $trigger;
			$aAttributes['data'] = $mData;
			$this->aCommands[] = $aAttributes;
	  	}
	}

	/** @return void */
	function run() 
	{
	    if (!$this->isActive()) return;

		// remove not active commands
		foreach ($this->aCommands as $idx => $com) {
// If we should reload whole page content ignore all commands but the update.
// This is page repost equivalent, although header and footer are not reloaded.
		  	if ($com['why'] !== true && !isset($this->triggers[$com['why']])) {
				unset($this->aCommands[$idx]);
			}
			elseif($com['n'] == 'up' && $com['t'] == '_page_body') {
			  	$cmds = array($com);
			  	foreach( $this->aCommands as $k=> $cmd) {
				  	if ($cmd['n'] == 'fc' || $cmd['n'] == 'js' || $cmd['n'] == 'st' || $cmd['n'] == 'fe') {	// save focus and page state
						$cmds[] = $cmd; //break;
			  		}
				}
				$this->aCommands = $cmds;
				break;
			}
		}
	    $GLOBALS['_RESULT'] = $this->aCommands;
	}
}

/** @return bool */
function in_ajax() {
    return ajax()->isActive();
}

// Returns absolute path of relative $url. To be used in ajax calls
// for proper redirection from any referer page.
//
/** @return null|scalar */
function absolute_url(string|int|float|bool|null $url) 
{
   return strpos($url, '..')===0 ? dirname($_SERVER['PHP_SELF']).'/'.$url : $url;
}

/**
 * The $Ajax object created by session.inc. Psalm forgets the type of a global after any function call, so code
 * reaches it through this accessor instead of `global $Ajax`.
 *
 * @return Ajax
 */
function ajax()
{
	global $Ajax;

	return $Ajax;
}
