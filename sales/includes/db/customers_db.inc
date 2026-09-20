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
require_once($path_to_root. "/admin/db/attachments_db.inc");

function add_customer(string|array|null $CustName, string|array|null $cust_ref, string|array|null $address, string|array|null $tax_id, string|array|null $curr_code,
	string|array|null $dimension_id, string|array|null $dimension2_id, string|array|null $credit_status, string|array|null $payment_terms, int|float|null $discount, int|float|null $pymt_discount, 
	int|float|bool|null $credit_limit, string|array|null $sales_type, string|array|null $notes): void
{
	$sql = "INSERT INTO ".TB_PREF."debtors_master (name, debtor_ref, address, tax_id,
		dimension_id, dimension2_id, curr_code, credit_status, payment_terms, discount, 
		pymt_discount,credit_limit, sales_type, notes) VALUES ("
		.db_escape($CustName) .", " .db_escape($cust_ref) .", "
		.db_escape($address) . ", " . db_escape($tax_id) . ","
		.db_escape($dimension_id) . ", " 
		.db_escape($dimension2_id) . ", ".db_escape($curr_code) . ", 
		" . db_escape($credit_status) . ", ".db_escape($payment_terms) . ", " . $discount . ", 
		" . $pymt_discount . ", " . $credit_limit 
		 .", ".db_escape($sales_type).", ".db_escape($notes) . ")";
	db_query($sql,"The customer could not be added");
}

function update_customer(string|array|null $customer_id, string|array|null $CustName, string|array|null $cust_ref, string|array|null $address, string|array|null $tax_id, string|array|null $curr_code,
	string|array|null $dimension_id, string|array|null $dimension2_id, string|array|null $credit_status, string|array|null $payment_terms, int|float|null $discount, int|float|null $pymt_discount,
	int|float|bool|null $credit_limit, string|array|null $sales_type, string|array|null $notes): void
{
	$sql = "UPDATE ".TB_PREF."debtors_master SET name=" . db_escape($CustName) . ", 
		debtor_ref=" . db_escape($cust_ref) . ",
		address=".db_escape($address) . ", 
		tax_id=".db_escape($tax_id) . ", 
		curr_code=".db_escape($curr_code) . ", 
		dimension_id=".db_escape($dimension_id) . ", 
		dimension2_id=".db_escape($dimension2_id) . ", 
		credit_status=".db_escape($credit_status) . ", 
		payment_terms=".db_escape($payment_terms) . ", 
		discount=" . $discount . ", 
		pymt_discount=" . $pymt_discount . ", 
		credit_limit=" . $credit_limit . ", 
		sales_type = ".db_escape($sales_type) . ", 
		notes=".db_escape($notes) ."
		WHERE debtor_no = ".db_escape($customer_id);

	db_query($sql,"The customer could not be updated");
}

function delete_customer(string|int|float|bool|array|null $customer_id): void
{
	begin_transaction();
	delete_entity_contacts('customer', $customer_id);

	$sql = "DELETE FROM ".TB_PREF."debtors_master WHERE debtor_no=".db_escape($customer_id);
	db_query($sql,"cannot delete customer");
	delete_attachments(ST_CUSTOMER, $customer_id);
	commit_transaction();
}

/*
	This function probably should be renamed to get_customer_summary
*/
/**
 * @return array<array-key, null|string>|false
 */
function get_customer_details(string|int|float|bool|array|null $customer_id, string|array|null $to=null, string|bool|array|null $all=true)
{

	if ($to == null)
		$todate = date("Y-m-d");
	else
		$todate = date2sql($to);
	$past1 = get_company_pref('past_due_days');
	$past2 = 2 * $past1;

	$sign = "IF(trans.type IN(".implode(',',  array(ST_CUSTCREDIT,ST_CUSTPAYMENT,ST_BANKDEPOSIT))."), -1, 1)";
	$value = "$sign*(IF(trans.prep_amount, trans.prep_amount,
		ABS(trans.ov_amount + trans.ov_gst + trans.ov_freight + trans.ov_freight_tax + trans.ov_discount)) ".($all ? '' : "- trans.alloc").")";

	$due = "IF (trans.type=".ST_SALESINVOICE.", trans.due_date, trans.tran_date)";
	$sql = "SELECT debtor.name, debtor.curr_code, terms.terms,
		debtor.credit_limit, credit_status.dissallow_invoices, credit_status.reason_description,

		Sum(IFNULL($value,0)) AS Balance,
		Sum(IF ((TO_DAYS('$todate') - TO_DAYS($due)) >= 0,$value,0)) AS Due,
		Sum(IF ((TO_DAYS('$todate') - TO_DAYS($due)) >= $past1,$value,0)) AS Overdue1,
		Sum(IF ((TO_DAYS('$todate') - TO_DAYS($due)) >= $past2,$value,0)) AS Overdue2

		FROM ".TB_PREF."debtors_master debtor
			LEFT JOIN ".TB_PREF."debtor_trans trans ON trans.tran_date <= '$todate' AND debtor.debtor_no = trans.debtor_no AND trans.type <> ". ST_CUSTDELIVERY.",
			".TB_PREF."payment_terms terms,
			".TB_PREF."credit_status credit_status

		WHERE
			 debtor.payment_terms = terms.terms_indicator
			 AND debtor.credit_status = credit_status.id";

	if ($customer_id)
		$sql .= " AND debtor.debtor_no = ".db_escape($customer_id);

	if (!$all)
		$sql .= " AND ABS(IF(trans.prep_amount, trans.prep_amount, ABS(trans.ov_amount) + trans.ov_gst + trans.ov_freight + trans.ov_freight_tax + trans.ov_discount) - trans.alloc) > ".FLOAT_COMP_DELTA;

	$sql .= " GROUP BY
				debtor.name,
				terms.terms,
				terms.days_before_due,
				terms.day_in_following_month,
				debtor.credit_limit,
				credit_status.dissallow_invoices,
				credit_status.reason_description";

    $result = db_query($sql,"The customer details could not be retrieved");

    $customer_record = db_fetch($result);

    return $customer_record;

}


/**
 * @return array<array-key, null|string>|false
 */
function get_customer(string|int|float|bool|array|null $customer_id)
{
	$sql = "SELECT * FROM ".TB_PREF."debtors_master WHERE debtor_no=".db_escape($customer_id);

	$result = db_query($sql, "could not get customer");

	return db_fetch($result);
}

/**
 * @return false|null|string
 */
function get_customer_name(string|int|float|bool|array|null $customer_id)
{
	$sql = "SELECT name FROM ".TB_PREF."debtors_master WHERE debtor_no=".db_escape($customer_id);

	$result = db_query($sql, "could not get customer");

	$row = db_fetch_row($result);

	return is_array($row) ? $row[0] : false;
}

/**
 * @return array<array-key, null|string>|false
 */
function get_customer_habit(string|array|null $customer_id)
{
	$sql = "SELECT debtor.pymt_discount, credit_status.dissallow_invoices
			FROM ".TB_PREF."debtors_master debtor,"
				.TB_PREF."credit_status credit_status
			WHERE debtor.credit_status = credit_status.id
			AND debtor.debtor_no = ".db_escape($customer_id);

	$result = db_query($sql, "could not query customers");

	return db_fetch($result);
}

function get_customer_contacts(?string $customer_id, ?string $action=null): array
{
	$results = array();
	$res = get_crm_persons('customer', $action, $customer_id);
	while($contact = db_fetch($res))
	{
		if ($contact['lang'] == 'C') // Fix for improper lang in demo sql files.
			$contact['lang'] = '';
		$results[] = $contact;
	}	
	return $results;
}

function get_current_cust_credit(string|int|float|bool|null $customer_id)
{
	$custdet = get_customer_details($customer_id);

	if (!is_array($custdet))
		return 0;
	return (float)$custdet['credit_limit']-(float)$custdet['Balance'];

}

function is_new_customer(string|int|float|bool|null $id): bool
{
	$tables = array('cust_branch', 'debtor_trans', 'recurrent_invoices', 'sales_orders');

	return !key_in_foreign_table($id, $tables, 'debtor_no');
}

/**
 * @return array<array-key, null|string>|false
 */
function get_customer_by_ref(string|int|float|bool|null $reference)
{
	$sql = "SELECT * FROM ".TB_PREF."debtors_master WHERE debtor_ref=".db_escape($reference);

	$result = db_query($sql, "could not get customer");

	return db_fetch($result);
}

//----------------------------------------------------------------------------------

function get_customer_currency(string|int|float|bool|array|null $customer_id=null, string|int|float|bool|null $branch_id=null)
{
    $sql = "SELECT curr_code
    	FROM ".TB_PREF."debtors_master cust
    		LEFT JOIN ".TB_PREF."cust_branch branch ON branch.debtor_no=cust.debtor_no
    	WHERE " .(isset($branch_id) ? "branch_code = ".db_escape($branch_id) : "cust.debtor_no = ".db_escape($customer_id));

	$result = db_query($sql, "Retreive currency of customer $customer_id");

	$myrow=db_fetch_row($result) ?: array();
	return $myrow ? $myrow[0] : get_company_currency();
}

/**
 * @return bool|mysqli_result
 */
function get_customers_search(string|array|null $customer)
{
	global $SysPrefs;

	if (isset($SysPrefs->max_rows_in_search))
		$limit = $SysPrefs->max_rows_in_search;
	else
		$limit = 10;

	$sql = "SELECT debtor_no, name, debtor_ref, address, tax_id FROM ".TB_PREF."debtors_master 
	  WHERE (  name LIKE " . db_escape("%" . $customer. "%") . " OR 
    		 debtor_ref LIKE " . db_escape("%" . $customer. "%") . " OR 
	        address LIKE " . db_escape("%" . $customer. "%") . " OR 
    	     tax_id LIKE " . db_escape("%" . $customer. "%").")
	  ORDER BY name LIMIT 0,".(int)($limit);

	return db_query($sql, "Failed in retreiving customer list.");
}
