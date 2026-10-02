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

namespace FrontAccounting\Session;

/**
 * The last-picked supplier/stock-item/customer/currency, remembered across pages
 * via $_SESSION. Backs the global set_global_*()/get_global_*() wrappers in
 * includes/ui/ui_globals.inc.
 */
final class GlobalSelections
{
	public static function setSupplier(string|int|float|bool|array|null $supplierId): void
	{
		$_SESSION['wa_global_supplier_id'] = $supplierId;
	}

	public static function getSupplier(bool $returnAll = true): string|int|float|bool|array|null
	{
		if (!isset($_SESSION['wa_global_supplier_id']) ||
			($returnAll == false && $_SESSION['wa_global_supplier_id'] == ALL_TEXT))
			return "";
		/** @var string|int|float|bool|array<array-key, mixed> $value */
		$value = $_SESSION['wa_global_supplier_id'];
		return $value;
	}

	public static function setStockItem(string|int|float|bool|array|null $stockId): void
	{
		$_SESSION['wa_global_stock_id'] = $stockId;
	}

	public static function getStockItem(string|int|float|bool|null $returnAll = true): string|int|float|bool|array|null
	{
		if (!isset($_SESSION['wa_global_stock_id']) ||
			($returnAll == false && $_SESSION['wa_global_stock_id'] == ALL_TEXT))
			return "";
		/** @var string|int|float|bool|array<array-key, mixed> $value */
		$value = $_SESSION['wa_global_stock_id'];
		return $value;
	}

	public static function setCustomer(string|int|float|bool|array|null $customerId): void
	{
		$_SESSION['wa_global_customer_id'] = $customerId;
	}

	public static function getCustomer(bool $returnAll = true): string|int|float|bool|array|null
	{
		if (!isset($_SESSION['wa_global_customer_id']) ||
			($returnAll == false && $_SESSION['wa_global_customer_id'] == ALL_TEXT))
			return "";
		/** @var string|int|float|bool|array<array-key, mixed> $value */
		$value = $_SESSION['wa_global_customer_id'];
		return $value;
	}

	public static function setCurrencyCode(string|array|null $currCode): void
	{
		$_SESSION['wa_global_curr_code'] = $currCode;
	}

	public static function getCurrencyCode(): string|array|null
	{
		if (!isset($_SESSION['wa_global_curr_code']))
			return "";
		/** @var string|array<array-key, mixed> $value */
		$value = $_SESSION['wa_global_curr_code'];
		return $value;
	}
}
