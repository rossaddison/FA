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
include_once(dirname(__DIR__) . "/includes/db/inventory_db.inc");


/** @psalm-pure */
function is_manufactured(string|int|float|bool|array|null $mb_flag): bool
{
	return ($mb_flag == 'M');
}

/** @psalm-pure */
function is_purchased(string|int|float|bool|null $mb_flag): bool
{
	return ($mb_flag == 'B');
}

/** @psalm-pure */
function is_service(?string $mb_flag): bool
{
	return ($mb_flag == 'D');
}

/** @psalm-pure */
function is_fixed_asset(string|int|float|bool|array|null $mb_flag): bool
{
	return ($mb_flag == 'F');
}

/** @psalm-pure */
function has_stock_holding(string|int|float|bool|null $mb_flag): bool
{
	return is_purchased($mb_flag) || is_manufactured($mb_flag);
}
