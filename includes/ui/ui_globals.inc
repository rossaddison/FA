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
//include_once($path_to_root . "/includes/reserved.inc");

function set_global_supplier(string|int|float|bool|array|null $supplier_id): void
{
	\FrontAccounting\Session\GlobalSelections::setSupplier($supplier_id);
}

function get_global_supplier(bool $return_all=true): string|int|float|bool|array|null
{
	return \FrontAccounting\Session\GlobalSelections::getSupplier($return_all);
}

function set_global_stock_item(string|int|float|bool|array|null $stock_id): void
{
	\FrontAccounting\Session\GlobalSelections::setStockItem($stock_id);
}

function get_global_stock_item(string|int|float|bool|null $return_all=true): string|int|float|bool|array|null
{
	return \FrontAccounting\Session\GlobalSelections::getStockItem($return_all);
}

function set_global_customer(string|int|float|bool|array|null $customer_id): void
{
	\FrontAccounting\Session\GlobalSelections::setCustomer($customer_id);
}

function get_global_customer(bool $return_all=true): string|int|float|bool|array|null
{
	return \FrontAccounting\Session\GlobalSelections::getCustomer($return_all);
}

function set_global_curr_code(string|array|null $curr_code): void
{
	\FrontAccounting\Session\GlobalSelections::setCurrencyCode($curr_code);
}

function get_global_curr_code(): string|array|null
{
	return \FrontAccounting\Session\GlobalSelections::getCurrencyCode();
}

/** @psalm-pure */
function count_array(mixed $array): int
{
	return (is_array($array)) ? count($array) : (($array === NULL) ? 0 : 1);
}

//--------------------------------------------------------------------------------------

