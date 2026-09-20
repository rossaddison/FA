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
/* Definition of the purch_order class to hold all the information for a purchase order and delivery
*/

class purch_order 
{
	/** @var string|int|float|bool|null|array */
	var $trans_no; 
	/** @var int|string */
	var $trans_type; // order/grn/invoice (direct)
	/** @var array<int, po_line_details> */
	var $line_items;
	/** @var string */
	var $curr_code;
	/** @var string */
	var $supp_ref;
	/** @var string */
	var $delivery_address;
	/** @var string */
	var $Comments;
	/** @var string */
	var $Location;
	/** @var string|int|float|bool|null */
	var $supplier_id;
	/** @var string */
	var $supplier_name;
	/** @var string */
	var $orig_order_date;
	/** @var string */
	var $due_date;

	/**
	 * @var int
	 */
	var $order_no; /*Only used for modification of existing orders otherwise only established when order committed */
	/** @var int */
	var $lines_on_order = 0;
	/** @var float|int|string|null */
	var $credit;
	/** @var string|int|float|bool|null */
	var $tax_group_id;
	/** @var array<array-key, mixed>|null */
	var $tax_group_array = null; // saves db queries
	/** @var string|int|float|bool|null */
	var $tax_included; // type of prices
	/** @var string|int|float|bool|null */
	var $terms;
	/** @var float|int|string|null */
	var $ex_rate;
	/** @var string|int|float|bool|null */
	var $cash_account;
	/** @var int|string */
	var $dimension;
	/** @var int|string */
	var $dimension2;

	/** @var string */
	var $reference;

	/**
	 * @var array
	 */
	var $tax_overrides = array();		// array of taxes manually inserted during sales invoice entry (direct invoice)
	
	/**
	 * @var int
	 */
	var $prep_amount = 0; // prepayment required
	/** @var float|int|string|null */
	var $alloc; // sum of payments allocated

	/**
	 * @var array
	 */
	var $prepayments = array();

	/**
	 * @var false
	 */
	var $fixed_asset = false;

	/** @psalm-mutation-free */
	function __construct()
	{
		/*Constructor function initialises a new purchase order object */
		$this->line_items = array();
		$this->lines_on_order = $this->order_no = $this->supplier_id = 0;
	}
	
	/** @return void */
	function set_supplier($supplier_id, $supplier_name, $curr_code, $tax_group_id, $tax_included)
	{
		$this->supplier_id = $supplier_id;
		$this->supplier_name = $supplier_name;
		$this->curr_code = $curr_code;
		$this->tax_group_id = $tax_group_id;
		$this->tax_included = $tax_included;
		$this->tax_group_array = get_tax_group_items_as_array($tax_group_id);
	}
	
	/**
	 * @param int|null $line_no
	 * @param string|int|float|bool|null $qty
	 * @param string|array<array-key, mixed>|null $item_descr
	 * @param string|int|float|bool|null $price
	 * @param string|null $uom
	 * @param string|int|null $qty_inv
	 * @param string|int|null $qty_recd
	 * @return 0|1
	 */
	function add_to_order($line_no, $stock_id, $qty, $item_descr, $price, $uom, $req_del_date, $qty_inv, $qty_recd)
	{
		if (isset($qty) && $qty != 0)
		{
			$this->line_items[$line_no] = new po_line_details($line_no, $stock_id, $item_descr, $qty, $price, $uom, 
				$req_del_date, $qty_inv, $qty_recd);
			$this->lines_on_order++;
			return 1;
		}
		return 0;
	}

	/**
	 * @param int|float|bool|null $qty
	 * @param int|float|bool|null $price
	 * @return void
	 */
	function update_order_item($line_no, $qty, $price, $req_del_date, $description="")
	{
		if ($description != "")
			$this->line_items[$line_no]->item_description = $description;
		$this->line_items[$line_no]->quantity = $qty;
		$this->line_items[$line_no]->price = $price;
		$this->line_items[$line_no]->req_del_date = $req_del_date;
		$this->line_items[$line_no]->item_description = $description;
	}

	/**
	 * @param string|int|null $line_no
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function remove_from_order($line_no)
	{
		array_splice($this->line_items, $line_no, 1);
	}
	
	/**
	 * @psalm-mutation-free
	 * @return bool
	 */
	function order_has_items() 
	{
		return count($this->line_items) != 0;
	}
	
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function clear_items() 
	{
    	unset($this->line_items);
		$this->line_items = array();
		
		$this->lines_on_order = 0;  
		$this->order_no = 0;
	}

	
	/**
	 * @psalm-mutation-free
	 * @return 0|1
	 */
	function any_already_received()
	{
		/* Checks if there have been deliveries or invoiced entered against any of the line items */
		if (count($this->line_items) > 0)
		{
		   	foreach ($this->line_items as $ordered_items) 
		   	{
				if ($ordered_items->qty_received != 0 || $ordered_items->qty_inv != 0)
				{
					return 1;
				}
		   	}
		}
		return 0;
	}

	/**
	 * @param string|int|null $line_no
	 * @psalm-mutation-free
	 * @return 0|1
	 */
	function some_already_received($line_no)
	{
		/* Checks if there have been deliveries or amounts invoiced against a specific line item */
		if (count($this->line_items) > 0)
		{
		   	if ($this->line_items[$line_no]->qty_received != 0 || 
		   		$this->line_items[$line_no]->qty_inv != 0)
		 	{
				return 1;
		   	}
		}
		return 0;
	}
	
	//
	//	Returns taxes for PO/GRN.
	//	$receival=true in purchase receive context.
	//
	/**
	 * @param int|float|bool|null $shipping_cost
	 * @param bool $receival
	 */
	function get_taxes($shipping_cost=null, $receival=false)
	{
		$items = array();
		$prices = array();
		if($shipping_cost==null)
			$shipping_cost = 0;//$this->freight_cost;

		foreach ($this->line_items as $ln_itm) {
			$items[] = $ln_itm->stock_id;
			$prices[] = round((float)$ln_itm->price * ((float)($receival ? $ln_itm->receive_qty : $ln_itm->quantity)),  user_price_dec());
		}
		$taxes = get_tax_for_items($items, $prices, $shipping_cost,
		  $this->tax_group_id, $this->tax_included,  $this->tax_group_array);

    	// Adjustment for swiss franken, we always have 5 rappen = 1/20 franken
	    if ($this->curr_code == 'CHF') {
			$val = $taxes['1']['Value'];
			$val1 = (floatval((intval(round(($val*20),0)))/20));
			$taxes['1']['Value'] = $val1;
		}
		foreach($this->tax_overrides as $id => $value) // add values entered manually
		{
			$taxes[$id]['Override'] = $value;
		}
		return $taxes;
	}

	/*
		Returns order value including all taxes
	*/
	/** @return 0|float */
	function get_trans_total() {
		
		$total = 0;
		$dec = user_price_dec();

		foreach ($this->line_items as $ln_itm) {
			$items[] = $ln_itm->stock_id;
			$value = round((float)$ln_itm->quantity * (float)$ln_itm->price, $dec);
			$prices[] =$value;
			$total += (float)$value;
		}

		if (!(bool)$this->tax_included ) {
			$taxes = get_tax_for_items($items, $prices, 0, $this->tax_group_id,
			$this->tax_included,  $this->tax_group_array);

			foreach($taxes as $tax)
				$total += (float)round($tax['Value'], $dec);
		}
		return $total;
	}

} /* end of class defintion */

class po_line_details 
{

	/** @var int */
	var $line_no;
	/** @var string|int|float|bool|null */
	var $po_detail_rec;
	/** @var string|int|float|bool|null */
	var $grn_item_id;
	/** @var string */
	var $stock_id;
	/** @var string */
	var $item_description;
	/** @var float|int|string */
	var $price;
	/** @var string */
	var $units;
	/** @var string */
	var $req_del_date;
	/** @var string|int|float|bool|null */
	var $tax_type;
	/** @var string */
	var $tax_type_name;

	/** @var float|int|string */
	var $quantity;		// current/entry quantity of PO line
	/** @var float|int|string */
	var $qty_inv;	// quantity already invoiced against this line

	/**
	 * @var int
	 */
	var $receive_qty;	// current/entry GRN quantity
	/** @var float|int|string */
	var $qty_received;	// quantity already received against this line

	/**
	 * @var int
	 */
	var $standard_cost;
	/** @var string|int|float|bool|null */
	var $descr_editable;

	/**
	 * @param int|null $grn_item_id
	 * @param int|null $line_no
	 * @param string|array<array-key, mixed>|null $item_descr
	 * @param string|null $uom
	 * @param string|int|null $qty_inv
	 * @param string|int|null $qty_recd
	 */
	function __construct($line_no, $stock_item, $item_descr, $qty, $prc, $uom, $req_del_date, 
		$qty_inv, $qty_recd, $grn_item_id=0)
	{

		/* Constructor function to add a new LineDetail object with passed params */
		$this->line_no = $line_no;
		$this->stock_id = $stock_item;
		$item_row = row_or_empty(get_item($stock_item));
		if (!$item_row) 
			return;

		$this->descr_editable = $item_row["editable"];
		//if ($item_descr == null || !$this->descr_editable) this was overriding the prchase description!!
		if ($item_descr == null)
			$this->item_description = $item_row["description"];
		else
			$this->item_description = $item_descr;
		$this->quantity = $qty;
		$this->req_del_date = $req_del_date;
		$this->price = $prc;
//		$this->units = $uom;
		$this->tax_type = $item_row["tax_type_id"];
		$this->tax_type_name = $item_row["tax_type_name"];
		$this->units = $item_row["units"];
		$this->qty_received = $qty_recd;
		$this->qty_inv = $qty_inv;
		$this->receive_qty = 0;	/*initialise these last two only */
		$this->standard_cost =0;
		$this->grn_item_id = $grn_item_id;
	}
	
	// Deprecated, use with care. If you need line value as it is calculated in invoice, use taxfree_charge_value below.
 	function taxfree_charge_price($po)
	{
		return get_tax_free_price_for_item($this->stock_id, $this->price, 
		  $po->tax_group_id, $po->tax_included, $po->tax_group_array);
	}

	//
	// GRN line tax free value.
	//
	/**
	 * @param purch_order $po
	 */
	function taxfree_charge_value($po)
	{
		return get_tax_free_price_for_item($this->stock_id, (float)$this->receive_qty*(float)$this->price,
		  $po->tax_group_id, $po->tax_included, $po->tax_group_array);
	}
}

