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
    Legacy code has been psalm level 1 converted courtesy of:
      GPT 5.6 sol extra high

***********************************************************************/
include_once(dirname(__DIR__) . "/includes/db/connect_db.inc");
include_once(dirname(__DIR__) . "/includes/types.inc");
include_once(dirname(__DIR__) . "/admin/db/company_db.inc");
include_once(dirname(__DIR__) . "/taxes/db/tax_groups_db.inc");
include_once(dirname(__DIR__) . "/taxes/db/tax_types_db.inc");
include_once(dirname(__DIR__) . "/taxes/db/item_tax_types_db.inc");
include_once(dirname(__DIR__) . "/includes/current_user.inc");
include_once(dirname(__DIR__) . "/includes/prefs/userprefs.inc");

/**
 * @psalm-type TaxItem = array{
 *     tax_type_id: array-key,
 *     tax_type_name?: scalar|null,
 *     sales_gl_code?: scalar|null,
 *     purchasing_gl_code?: scalar|null,
 *     rate: scalar|null,
 *     included_in_price?: scalar|null,
 *     Value?: scalar|null,
 *     Net?: scalar|null,
 *     ...<array-key, scalar|null>
 * }
 * @psalm-type TaxTotalsItem = array{
 *     tax_type_id: array-key,
 *     tax_type_name?: scalar|null,
 *     sales_gl_code?: scalar|null,
 *     purchasing_gl_code?: scalar|null,
 *     rate: scalar|null,
 *     included_in_price?: scalar|null,
 *     Value: float,
 *     Net: float,
 *     ...<array-key, scalar|null>
 * }
 */

/**
 * @psalm-pure
 */
function tax_calc_scalar_or_null(int|float|string|bool|null $value): int|float|string|bool|null
{
    return $value;
}

/**
 * @return array-key
 * @psalm-pure
 */
function tax_calc_array_key_or_empty(int|float|string|bool|null $value): int|string
{
    if (is_int($value) || is_string($value)) {
        return $value;
    }

    if (is_float($value) || is_bool($value)) {
        return (int)$value;
    }

    return '';
}

/**
 * @param array<array-key, scalar|null> $tax_item
 * @return TaxItem
 * @psalm-pure
 */
function tax_calc_normalize_tax_item(array $tax_item): array
{
    /** @var array<array-key, scalar|null> $normalized */
    $normalized = array();
    foreach ($tax_item as $key => $value) {
        $normalized[$key] = tax_calc_scalar_or_null($value);
    }

    $normalized['tax_type_id'] = tax_calc_array_key_or_empty($tax_item['tax_type_id'] ?? null);
    $normalized['rate'] = tax_calc_scalar_or_null($tax_item['rate'] ?? null);

    /** @var TaxItem $normalized */
    return $normalized;
}

/**
 * @param array<array-key, array<array-key, scalar|null>> $tax_items
 * @return array<array-key, TaxItem>
 * @psalm-pure
 */
function tax_calc_normalize_tax_items(array $tax_items): array
{
    /** @var array<array-key, TaxItem> $normalized */
    $normalized = array();
    foreach ($tax_items as $key => $tax_item) {
        $normalized[$key] = tax_calc_normalize_tax_item($tax_item);
    }

    return $normalized;
}

/**
 * @param TaxItem $tax_item
 * @return TaxTotalsItem
 * @psalm-pure
 */
function tax_calc_to_totals_item(array $tax_item): array
{
    $tax_totals_item = $tax_item;
    $tax_totals_item['Value'] = isset($tax_item['Value']) ? (float)$tax_item['Value'] : 0.0;
    $tax_totals_item['Net'] = isset($tax_item['Net']) ? (float)$tax_item['Net'] : 0.0;

    /** @var TaxTotalsItem $tax_totals_item */
    return $tax_totals_item;
}

//---------------------------------------------------------------------------------

// returns the price of a given item minus any included taxes
// for item $stock_id with line price $price,
// with applicable tax rates $tax_group_array or group id $tax_group

/**
 * @param array<array-key, TaxItem>|null $tax_group_array
 */
function get_tax_free_price_for_item(
    int|string $stock_id,
    ?float $price,
    string|int|float|bool|null $tax_group,
    int|bool $tax_included,
    ?array $tax_group_array = null
): ?float {
    if ($price === null) {
        return null;
    }

    // if price is zero, then can't be taxed !
    if ($price === 0.0)
        return 0.0;

    if ($tax_included === 0 || $tax_included === false) return $price;

    // if array already read, then make a copy and use that
    if ((bool)$tax_group_array) {
        $ret_tax_array = tax_calc_normalize_tax_items($tax_group_array);
    } else {
        /** @var array<array-key, array<array-key, scalar|null>> $tax_group_items */
        $tax_group_items = get_tax_group_items_as_array($tax_group);
        $ret_tax_array = tax_calc_normalize_tax_items($tax_group_items);
    }

    $tax_array = get_taxes_for_item($stock_id, $ret_tax_array);

    // if no exemptions or taxgroup is empty, then no included/excluded taxes
    if ($tax_array == null)
        return $price;

    // to avoid rounding errors we have to just subtract taxes from tax_included price.
    $tax_multiplier = 0.0;

    foreach ($tax_array as $taxitem)
    {
        $tax_multiplier += (float)$taxitem['rate'];
    }

    $tax = 0.0;
    $userPrefs = new user_prefs();
    $price_dec = (int)$userPrefs->price_dec();

    foreach ($tax_array as $taxitem)
    {
        $taxItemRate = (float)$taxitem['rate'];
        $tax += round($price * $taxItemRate / (100.0 + $tax_multiplier),
            $price_dec);
    }
    return $price - $tax;
}

//
//	Full price (incl. VAT) for item $stock_id with line price $price,
//	with tax rates $tax_group_array or applicable group $tax_group
//
/**
 * @param array<array-key, TaxItem>|null $tax_group_array
 */
function get_full_price_for_item(
    int|string $stock_id,
    ?float $price,
    string|int|float|bool|null $tax_group,
    int|bool $tax_included,
    ?array $tax_group_array = null
): float {
    // if price is zero, then can't be taxed !
    if ($price === null || $price == 0)
        return 0.0;

    if ($tax_included == 1) return $price;

    // if array already read, then make a copy and use that
    if ((bool)$tax_group_array) {
        $ret_tax_array = tax_calc_normalize_tax_items($tax_group_array);
    } else {
        /** @var array<array-key, array<array-key, scalar|null>> $tax_group_items */
        $tax_group_items = get_tax_group_items_as_array($tax_group);
        $ret_tax_array = tax_calc_normalize_tax_items($tax_group_items);
    }

    //print_r($ret_tax_array);

    $tax_array = get_taxes_for_item($stock_id, $ret_tax_array);
    // if no exemptions or taxgroup is empty, then no included/excluded taxes
    if ($tax_array == null) return $price;

    $tax_multiplier = 0.0;

    // loop for all items
    foreach ($tax_array as $taxitem) 
    {
        $tax_multiplier += (float)$taxitem["rate"];
    }
    return round($price * (1.0 + ($tax_multiplier / 100.0)), (int)user_price_dec());
}

//---------------------------------------------------------------------------------
// return an array of (tax_type_id, tax_type_name, sales_gl_code, purchasing_gl_code, rate)

/**
 * @param array<array-key, TaxItem> $tax_group_items_array
 * @return array<array-key, TaxItem>|null
 */
function get_taxes_for_item(
    int|string $stock_id,
    array $tax_group_items_array
): ?array {
    $item_tax_type_row = get_item_tax_type_for_item($stock_id);
    $item_tax_type = is_array($item_tax_type_row) ? $item_tax_type_row : array();

    // if the item is exempt from all taxes then return 0
    if ((bool)($item_tax_type["exempt"] ?? false))
            return null;

    // get the exemptions for this item tax type
    $itemTaxTypeId = $item_tax_type["id"] ?? null;
    $item_tax_type_exemptions_db = get_item_tax_type_exemptions($itemTaxTypeId);

    // read them all into an array to minimize db querying
    /** @var list<scalar|null> $item_tax_type_exemptions */
    $item_tax_type_exemptions = array();
    while ($item_tax_type_exemp = db_fetch($item_tax_type_exemptions_db))
    {
            $item_tax_type_exemptions[] = $item_tax_type_exemp["tax_type_id"] ?? null;
    }

    /** @var array<array-key, TaxItem> $ret_tax_array */
    $ret_tax_array = array();

    // if any of the taxes of the tax group are in the exemptions, then skip
    foreach ($tax_group_items_array as $tax_group_item)
    {

        $skip = false;

        // if it's in the exemptions, skip
        foreach ($item_tax_type_exemptions as $exemption)
        {
                if (($tax_group_item['tax_type_id'] == $exemption))
                {
                    $skip = true;
                    break;
                }
        }

        if (!$skip)
        {
                $index = $tax_group_item['tax_type_id'];
                $ret_tax_array[$index] = $tax_group_item;
        }
    }
    return $ret_tax_array;
}

//-----------------------------------------------------------------------------------
// return an array of (tax_type_id, tax_type_name, sales_gl_code, purchasing_gl_code, rate, included_in_price, Value) 

/**
 * @param list<int|string> $items
 * @param list<int|float|string|null> $prices
 * @param array<array-key, TaxItem>|null $tax_items_array
 * @return array<array-key, TaxTotalsItem>
 */
function get_tax_for_items(
    array $items,
    array $prices,
    ?float $shipping_cost,
    string|int|float|bool|null $tax_group,
    string|int|float|bool|null $tax_included = null,
    ?array $tax_items_array = null,
    ?int $tax_algorithm = null
): array {
    if (!(bool)$tax_algorithm)
        $tax_algorithm = (int)get_company_pref('tax_algorithm');

    // first create and set an array with all the tax types of the tax group
    if ($tax_items_array != null) {
        $tax_group_items = tax_calc_normalize_tax_items($tax_items_array);
    } else {
        /** @var array<array-key, array<array-key, scalar|null>> $tax_group_item_rows */
        $tax_group_item_rows = get_tax_group_items_as_array($tax_group);
        $tax_group_items = tax_calc_normalize_tax_items($tax_group_item_rows);
    }

    $userPrefs = new user_prefs();
    $dec = (int)$userPrefs->price_dec();

    $fully_exempt = true;
    /** @var array<array-key, TaxTotalsItem> $ret_tax_array */
    $ret_tax_array = array();
    foreach ($tax_group_items as $k => $t)
    {
            if ($t['rate'] !== null) $fully_exempt = false;

            $prepared_tax_item = tax_calc_to_totals_item($t);
            $prepared_tax_item['Net'] = 0.0;
            $ret_tax_array[$k] = $prepared_tax_item;
    }

    $ret_tax_array['exempt'] = array('Value' => 0.0, 'Net' => 0.0,
            'rate' => null, 'tax_type_id' => '', 'sales_gl_code' => '');
    // loop for all items
    for ($i = 0; $i < count($items); $i++)
    {
        $price = isset($prices[$i]) ? (float)$prices[$i] : 0.0;
        $item_taxes = get_taxes_for_item($items[$i], $ret_tax_array);
        if ($item_taxes == null || $fully_exempt)
        {
            $ret_tax_array['exempt']['Value'] += round2(0.0, $dec);
            $ret_tax_array['exempt']['Net'] += $price;
        }
        else
        {
            $tax_multiplier = 0.0;
            foreach ($item_taxes as $taxitem)
            {
                $tax_multiplier += (float)$taxitem['rate'];
            }
            foreach ($item_taxes as $item_tax)
            {
                if ($item_tax['rate'] !== null) {
                    $index = (int)$item_tax['tax_type_id'];
                    if (!isset($ret_tax_array[$index])) {
                        $ret_tax_array[$index] = tax_calc_to_totals_item($item_tax);
                    }

                    $itemTaxRate = (float)$item_tax['rate'];
                    if ($tax_included == 1) {
                        $ret_tax_array[$index]['Value'] +=
                            round2($price * $itemTaxRate /
                                (100.0 + $tax_multiplier), $dec);
                        $ret_tax_array[$index]['Net'] +=
                            round2($price * 100.0 / (100.0 + $tax_multiplier), $dec);
                    } else {
                        $ret_tax_array[$index]['Value'] +=
                            round2($price * $itemTaxRate / 100.0, $dec);
                        $ret_tax_array[$index]['Net'] +=
                            $price;
                    }
                }
            }
        }
    }
    // add the shipping taxes, only if non-zero, and only if tax group taxes shipping
    $shipping_net = 0.0;
    if ($shipping_cost !== null && $shipping_cost != 0.0)
    {
        /** @var array<array-key, array<array-key, scalar|null>> $shipping_tax_rows */
        $shipping_tax_rows = get_shipping_tax_as_array($tax_group);
        $item_taxes = tax_calc_normalize_tax_items($shipping_tax_rows);
        if ($item_taxes != null)
        {
            $tax_rate = 0.0;
            if ($tax_included == 1)
            {
                foreach ($item_taxes as $item_tax)
                {
                    $index = (int)$item_tax['tax_type_id'];
                    if (isset($ret_tax_array[$index]) && $item_tax['rate'] !== null) {
                        $tax_rate += (float)$item_tax['rate'];
                    }
                }
                $shipping_net = round2($shipping_cost * 100.0 / (100.0 + $tax_rate), $dec);
            }
            foreach ($item_taxes as $item_tax)
            {
                $index = (int)$item_tax['tax_type_id'];
                $itemTaxRate = $item_tax['rate'];
                if ($itemTaxRate !== null
                    && isset($ret_tax_array[$index])
                    && $ret_tax_array[$index]['rate'] !== null
                ) {
                    $itemTaxRate = (float)$itemTaxRate;
                    if ($tax_included == 1) {
                        $ret_tax_array[$index]['Value'] +=
                            round2($shipping_cost * $itemTaxRate /
                                (100.0 + $tax_rate), $dec);
                        $ret_tax_array[$index]['Net'] += $shipping_net;
                    } else {
		        $ret_tax_array[$index]['Value'] +=
                            round2($shipping_cost * $itemTaxRate / 100.0, $dec);
			$ret_tax_array[$index]['Net'] += $shipping_cost;
                    }
                }
            }
        }
    }

    if ($tax_algorithm == TCA_TOTALS) {
        // update taxes with tax values recalculated from accumulated net totals
        foreach ($ret_tax_array as $index => $item_tax) {
            $ret_tax_array[$index]['Value'] = round2(
                $item_tax['Net'] * (float)($item_tax['rate'] ?? 0.0) / 100.0,
                $dec
            );
        }
    }
    return $ret_tax_array;
}
