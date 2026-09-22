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
$page_security = 'SA_SOFTWAREUPGRADE';
/** @var string $path_to_root */
$path_to_root="..";

include(dirname(__DIR__) . "/includes/session.inc");

page(_($help_context = "System Diagnostics"));

include(dirname(__DIR__) . "/includes/ui.inc");
include(dirname(__DIR__) . "/includes/system_tests.inc");
//-------------------------------------------------------------------------------------------------

display_system_tests();

end_page();

