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
function display_error(mixed $msg, mixed $center=true): void
{
	 fa_trigger_error($msg, E_USER_ERROR);
}

function display_notification(mixed $msg, int|bool|null $center=true): void
{
	fa_trigger_error($msg, E_USER_NOTICE);
}

function display_warning(mixed $msg, int|bool|null $center=true): void
{
	fa_trigger_error($msg, E_USER_WARNING);
}

function display_notification_centered(?string $msg): void
{
	display_notification($msg, true);
}

function display_caption(mixed $msg): void
{
	echo "<caption>$msg</caption>\n";
}

function display_heading(string|int|float|bool|null $msg): void
{
	echo "<center><span class='headingtext'>$msg</span></center>\n";	
}

function display_heading2(?string $msg): void
{
	echo "<center><span class='headingtext2'>$msg</span></center>\n";	
}

function display_note(string|int|float|bool|null $msg, ?int $br=0, ?int $br2=0, ?string $extra=""): void
{
	for ($i = 0; $i < $br; $i++)
		echo "<br>";
	if ($extra != "")	
		echo "<center><span $extra>$msg</span></center>\n";	
	else	
		echo "<center><span>$msg</span></center>\n";	
	for ($i = 0; $i < $br2; $i++)
		echo "<br>";
}

function stock_item_heading(mixed $stock_id): void
{
	if ($stock_id != "") 
	{
		$result = db_query("SELECT description, units FROM ".TB_PREF."stock_master WHERE stock_id=".db_escape($stock_id));
        $myrow = db_fetch_row($result);
        
    	display_heading("$stock_id - $myrow[0]"); 
    	$units = $myrow[1];
    	display_heading2(_("in units of : ") . $units);        	
	}
}

