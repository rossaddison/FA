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
/** @return float|int|null|string */
function write_supp_payment(?int $trans_no, string|int|float|bool|array|null $supplier_id, string|int|float|bool|array|null $bank_account,
	string|int|float|bool|array|null $date_, string|int|float|bool|array|null $ref, string|int|float|bool|null $supp_amount, int|float|bool|null $supp_discount, string|array|null $memo_, int|float|bool|null $bank_charge=0, int|float|bool|null $bank_amount=0, string|int|float|bool|array|null $dimension=0, string|int|float|bool|array|null $dimension2=0)
{

	begin_transaction();
	$args = func_get_args(); while (count($args) < 12) $args[] = 0;
	$args = (object)array_combine(array('trans_no', 'supplier_id', 'bank_account', 'date_',
		 'ref', 'supp_amount', 'supp_discount', 'memo_', 'bank_charge', 'bank_amount', 'dimension', 'dimension2'), $args);
	$args->trans_no = 0;
	hook_db_prewrite( $args, ST_SUPPAYMENT);

	if ($trans_no != 0) {
	  delete_comments(ST_SUPPAYMENT, $trans_no);
	  void_bank_trans(ST_SUPPAYMENT, $trans_no, true);
	  void_gl_trans(ST_SUPPAYMENT, $trans_no, true);
	  void_supp_allocations(ST_SUPPAYMENT, $trans_no, $date_);
	}

	$bank = row_or_empty(get_bank_account($bank_account));

	if (!(bool)$bank_amount) 	// auto 
	{
		$rate = get_exchange_rate_from_to(get_supplier_currency($supplier_id),
			$bank['bank_curr_code'], $date_ );
		$bank_amount = $supp_amount/$rate;
	}

	if ($bank['bank_curr_code'] == get_company_currency()) // [0002506]
		$rate = $bank_amount/$supp_amount;
	else
		$rate = 0;

	/* Create a supp_trans entry for the supplier payment */
	$payment_id = write_supp_trans(ST_SUPPAYMENT, 0, $supplier_id, $date_, $date_,
		$ref, "", -$supp_amount, 0, -$supp_discount, "", $rate);

	// Now debit creditors account with payment + discount

	$total = 0.0;
    $supplier_accounts = row_or_empty(get_supplier_accounts($supplier_id));
	$total += (float)add_gl_trans_supplier(ST_SUPPAYMENT, $payment_id, $date_, $supplier_accounts["payable_account"], $dimension, $dimension2,
		$supp_amount + $supp_discount, $supplier_id, "", $rate);

	// Now credit discount received account with discounts
	if ($supp_discount != 0)
	{
		$total += add_gl_trans_supplier(ST_SUPPAYMENT, $payment_id, $date_, $supplier_accounts["payment_discount_account"], $dimension, $dimension2,
			-$supp_discount, $supplier_id, "", $rate);
	}

	if ($bank_charge != 0)
	{
		$charge_act = get_company_pref('bank_charge_act');
		$total += add_gl_trans(ST_SUPPAYMENT, $payment_id, $date_, $charge_act, $dimension, $dimension2, '',
			$bank_charge, $bank['bank_curr_code']);
	}

	$total += add_gl_trans(ST_SUPPAYMENT, $payment_id, $date_, $bank['account_code'], $dimension, $dimension2, '',
		-((float)$bank_amount + (float)$bank_charge), $bank['bank_curr_code'], PT_SUPPLIER, $supplier_id);

	/*Post a balance post if $total != 0 due to variance in AP and bank posted values*/
	if ($total != 0)
	{
		$variance_act = get_company_pref('exchange_diff_act');
		add_gl_trans(ST_SUPPAYMENT, $payment_id, $date_, $variance_act, $dimension, $dimension2, '',
			-$total, null, PT_SUPPLIER,  $supplier_id);
	}

   /*now enter the bank_trans entry */
	add_bank_trans(ST_SUPPAYMENT, $payment_id, $bank_account, $ref,
		$date_, -((float)$bank_amount + (float)$bank_charge), PT_SUPPLIER, $supplier_id);

	add_comments(ST_SUPPAYMENT, $payment_id, $date_, $memo_);

	refs()->save(ST_SUPPAYMENT, $payment_id, $ref);

	$args->trans_no = $payment_id;
	hook_db_postwrite($args, ST_SUPPAYMENT);

	commit_transaction();

	return $payment_id;
}

//------------------------------------------------------------------------------------------------

function void_supp_payment(string|int|float|bool|null $type, string|int|float|bool|null $type_no): void
{
	begin_transaction();
	hook_db_prevoid($type, $type_no);

	void_bank_trans($type, $type_no, true);
	void_gl_trans($type, $type_no, true);
	void_supp_allocations($type, $type_no);
	void_supp_trans($type, $type_no);

	commit_transaction();
}


