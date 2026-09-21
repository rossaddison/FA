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
	FA's system messages (errors, warnings, notices) as yiisoft/bootstrap5 alerts.

	fmt_errors() calls yiiAlert() when the Yii page shell is on. Only the alert rules of Bootstrap are loaded, from
	css/fa-alerts.css: Bootstrap's reboot would restyle FA's table layout. The alert is not dismissable, so no
	Bootstrap JavaScript is needed either.
*/

use Yiisoft\Bootstrap5\Alert;
use Yiisoft\Bootstrap5\AlertVariant;

/**
 * One message box.
 *
 * @param string $type 'err_msg', 'warn_msg' or 'note_msg', the classes the classic markup uses
 * @param string $html the messages, as the HTML FA collected them
 */
function yiiAlert(string $type, string $html): string
{
	$variant = match ($type) {
		'err_msg' => AlertVariant::DANGER,
		'warn_msg' => AlertVariant::WARNING,
		default => AlertVariant::SUCCESS,
	};
	return Alert::widget()
		->variant($variant)
		->addClass('fa-alert')
		->dismissable(false)
		->body($html, false)
		->render();
}
