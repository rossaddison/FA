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
/* Definition of the Supplier Transactions class to hold all the information for an accounts payable invoice or credit note
*/

include_once(dirname(__DIR__, 2) . "/taxes/tax_calc.inc");

use FrontAccounting\Purchasing\GrnItem;
use FrontAccounting\Purchasing\GlCodes;

/**
 * @psalm-suppress PropertyNotSetInConstructor supplier_name/tax_description/
 * Comments/tran_date/due_date/supp_reference/reference/ov_gst/currency/
 * dimension/dimension2 are always populated by __construct()'s call to
 * read(), via read_supp_invoice()/read_supplier_details_to_trans() - both
 * external functions Psalm can't trace property assignment through.
 */
final class supp_trans
{

	/** @var array<int, GrnItem> */
	var $grn_items; /*array of objects of class GrnItem using the id as the pointer */

        /** @var array<int, GlCodes> */
	var $gl_codes; /*array of objects of class GlCodes using a counter as the pointer */

	/**
	 * @var string|int|float|bool|null
	 */
	var $supplier_id;
	
        /** @var string */
	
        var $supplier_name;
	/** @var array{description?: mixed, days_before_due?: mixed, day_in_following_month?: mixed}|string|int|float|bool|null */
	var $terms;
	
        /** @var float|int|string|null */
	var $ex_rate;
	
	/** @var string */
	var $tax_description;
	
        /** @var string|int|float|bool|null */
	var $tax_group_id;
	
        /** @var string|int|float|bool|null */
	var $tax_included;
	
	/** @var int|string */
	var $trans_type;	// invoice or credit
	
        /** @var string|int|float|bool|null|array */
	var $trans_no;

	/** @var string */
	var $Comments;
	
        /** @var string */
	var $tran_date;
	
        /** @var string */
	var $due_date;
	
        /** @var array<array-key, mixed> */
	var $src_docs = array();		// source invoice for this credit note (if any)

	/** @var string */
	var $supp_reference;
	
        /** @var string */
	var $reference;

	/**
	 * @var float|int|string|null
	 */
	var $ov_amount;

	/**
	 * @var float|int|string|null
	 */
	var $ov_discount;
	
        /** @var float|int|string */
	var $ov_gst;
	
        /** @var int */
	var $gl_codes_counter=0;

	/**
	 * @var int|string|null
	 */
	var $credit = 0;
	
        /** @var string */
	var $currency;

	/**
	 * @var array<array-key, int|float|false>
	 */
	var $tax_overrides = array();		// array of taxes manually inserted during sales invoice entry

	/** @var int|string */
	var $dimension;
	
        /** @var int|string */
	var $dimension2;

	/**
	 * @param int $trans_type
	 * @param string|int|array<array-key, mixed>|null $trans_no
	 */
	function __construct($trans_type, $trans_no=0)
	{
		$this->trans_type = $trans_type;
		/*Constructor function initialises a new Supplier Transaction object */
		$this->read($trans_type, $trans_no);
	}

	/**
	 * @param int $trans_type
	 * @param string|int|array<array-key, mixed>|null $trans_no
	 * @return void
	 */
	function read($trans_type, $trans_no)
	{
		$this->trans_type = $trans_type;
		$this->trans_no = $trans_no;
 		/** @var array<int, GrnItem> */
 		$this->grn_items = array();
 		/** @var array<int, GlCodes> */
 		$this->gl_codes = array();
		if ((bool)$trans_no) {
			read_supp_invoice(is_array($trans_no) ? null : $trans_no, $trans_type, $this);
			if ($trans_type == ST_SUPPCREDIT)
			{
                $this->src_docs = find_src_invoices($this);
			}
			read_supplier_details_to_trans($this, $this->supplier_id);
		}
	}

	/**
	 * @param string|int|float|bool|null $grn_item_id
	 * @param string|int|float|bool|null $po_detail_item
	 * @param string|int|float|bool|null $item_code
	 * @param string|int|float|bool|null $item_description
	 * @param float|int|string|bool|null $qty_recd
	 * @param float|int|string|bool|null $prev_quantity_inv
	 * @param int|float|bool|null $this_quantity_inv
	 * @param float|int|string|bool|null $order_price
	 * @param string|int|float|bool|null $chg_price
	 * @param int|bool|null $std_cost_unit
	 * @param string|int|bool|null $gl_code
	 * @return 1
	 */
	function add_grn_to_trans($grn_item_id, $po_detail_item, $item_code, $item_description,
		$qty_recd, $prev_quantity_inv, $this_quantity_inv, $order_price, $chg_price,
		$std_cost_unit=null, $gl_code='')
	{
		$this->grn_items[(int) $grn_item_id] = new GrnItem($grn_item_id, $po_detail_item,
			$item_code, $item_description, $qty_recd, $prev_quantity_inv, $this_quantity_inv,
			$order_price, $chg_price, $std_cost_unit, $gl_code, (int) $this->tax_included);

		$this->src_docs = find_src_invoices($this);

		unset($this->tax_overrides); // cancel tax overrides after cart change
		return 1;
	}

	/**
	 * @param string|int|float|bool|null $gl_code
	 * @param string|int|float|bool|null $gl_act_name
	 * @param int|string|float|bool|null $gl_dim
	 * @param int|string|float|bool|null $gl_dim2
	 * @param string|int|float|bool|null $amount
	 * @param string|int|float|bool|null $memo_
	 * @psalm-external-mutation-free
	 * @return 1
	 */
	function add_gl_codes_to_trans($gl_code, $gl_act_name, $gl_dim, $gl_dim2, $amount, $memo_)
	{
		$this->gl_codes[$this->gl_codes_counter] = new GlCodes($this->gl_codes_counter,
			$gl_code, $gl_act_name, $gl_dim, $gl_dim2, $amount, $memo_);
		$this->gl_codes_counter++;
		unset($this->tax_overrides); // cancel tax overrides after cart change
		return 1;
	}

	/**
	 * @param string|int|null $grn_item_id
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function remove_grn_from_trans($grn_item_id)
	{
	    unset($this->tax_overrides); // cancel tax overrides after cart change
	    if ($grn_item_id !== null)
	    	unset($this->grn_items[(int) $grn_item_id]);
	}
	/**
	 * @param string|int|null $gl_code_counter
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function remove_gl_codes_from_trans($gl_code_counter)
	{
		unset($this->tax_overrides); // cancel tax overrides after cart change
		if ($gl_code_counter !== null)
			unset($this->gl_codes[(int) $gl_code_counter]);
	}

	/**
	 * @psalm-mutation-free
	 * @return bool
	 */
	function is_valid_trans_to_post()
	{
		return (count($this->grn_items) > 0 || count($this->gl_codes) > 0 || 
			($this->ov_amount != 0) || ($this->ov_discount > 0));
	}

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function clear_items()
	{
		unset($this->grn_items);
		unset($this->gl_codes);
		$this->ov_amount = $this->ov_discount = $this->supplier_id = 0;

		/** @var array<int, GrnItem> */
		$this->grn_items = array();
		/** @var array<int, GlCodes> */
		$this->gl_codes = array();
	}

    /**
     * @param string|int|float|bool|null $tax_group_id
     * @param int|null $shipping_cost
     * @param bool $gl_codes
     */
    function get_taxes($tax_group_id=null, $shipping_cost=0, $gl_codes=true): array
    {
    	$items = array();
    	$prices = array();

        if ($tax_group_id == null)
        	$tax_group_id = $this->tax_group_id;

		// preload the taxgroup
		$tax_group = get_tax_group_items_as_array($tax_group_id);

        foreach ($this->grn_items as $ln_itm)
        {
        	$items[] = $ln_itm->item_code;
        	$prices[] =round( ((float)$ln_itm->this_quantity_inv * (float)$ln_itm->chg_price),
				(int) user_price_dec());
        }

        if ($tax_group_id == null)
        	$tax_group_id = $this->tax_group_id;
        $taxes = get_tax_for_items($items, $prices, $shipping_cost, $tax_group_id,
        	$this->tax_included);

		// isset() here is not redundant at runtime despite the always-array
		// docblock type: add_grn_to_trans()/add_gl_codes_to_trans()/
		// remove_*_from_trans() all unset($this->tax_overrides) deliberately
		// to cancel overrides after a cart change - Psalm doesn't model
		// unset() narrowing a typed property back to "not set".
		/** @psalm-suppress RedundantConditionGivenDocblockType */
		if (isset($this->tax_overrides))
			foreach($this->tax_overrides as $id => $value) // add values entered manually
			{
				$taxes[$id]['Override'] = (float) $value;
			}

		// Taxes included in gl_codes table have exact value, but count to overrides as well.
		// Therefore when we want to now taxes only for items (gl_codes==false),
		// we have to subtract gl_taxes from override values.
		foreach ($this->gl_codes as $gl_code)
		{
			$index = is_tax_account($gl_code->gl_code);
			if ($index !== false && $index !== null)
			{
				if ($gl_codes)
					$taxes[$index]['Value'] += (float) $gl_code->amount;
				else {
					/** @psalm-suppress RedundantConditionGivenDocblockType */
					if (isset($this->tax_overrides)) {
						$tax_entry = $taxes[$index];
						$prev_override = isset($tax_entry['Override']) ? (float) $tax_entry['Override'] : 0.0;
						$tax_entry['Override'] = $prev_override - (float) $gl_code->amount;
						$taxes[$index] = $tax_entry;
					}
				}
			}
		}

        return $taxes;
    }
	//
	//	Returns total invoice amount without taxes.
	//
    /**
     * @param string|int|float|bool|null $tax_group_id
     * @return float
     */
    function get_total_taxfree($tax_group_id=null)
    {
    	$total = 0.0;

		// preload the taxgroup !
		if ($tax_group_id != null)
			$tax_group = get_tax_group_items_as_array($tax_group_id);
		else
			$tax_group = null;

		foreach ($this->grn_items as $ln_itm)
    		$total += round(((float)$ln_itm->this_quantity_inv * (float)$ln_itm->taxfree_charge_price($tax_group_id, $tax_group)),
			 (int) user_price_dec());

   		foreach ($this->gl_codes as $gl_line)
   		{
   			if (!(bool)is_tax_account($gl_line->gl_code))
   				$total += (float)$gl_line->amount;
   		}
   		return $total;
    }
	//
	//	Returns transaction total 
	//
	/** @return float */
	function get_items_total()
	{
		$total = 0.0;

		foreach ($this->grn_items as $ln_itm)
			$total += round((float)$ln_itm->this_quantity_inv * (float)$ln_itm->chg_price, (int) user_price_dec());

		foreach ($this->gl_codes as $gl_line)
		{
   			if (!(bool)is_tax_account($gl_line->gl_code) || (bool)$this->tax_included)
				$total += (float)$gl_line->amount;
		}
		return $total;
	}
} /* end of class defintion */

