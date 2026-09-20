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
function add_branch(string|int|array|null $customer_id, string|array|null $br_name, string|array|null $br_ref, string|array|null $br_address, string|array|null $salesman, string|array|null $area, 
	string|array|null $tax_group_id, string|array|null $sales_account, string|int|float|bool|array|null $sales_discount_account, string|int|float|bool|array|null $receivables_account, 
	string|int|float|bool|array|null $payment_discount_account, string|array|null $default_location, string|array|null $br_post_address, string|int|array|null $group_no,
	string|array|null $default_ship_via, string|array|null $notes, string|array|null $bank_account): void
{
	$sql = "INSERT INTO ".TB_PREF."cust_branch (debtor_no, br_name, branch_ref, br_address,
		salesman, area, tax_group_id, sales_account, receivables_account, payment_discount_account, 
		sales_discount_account, default_location,
		br_post_address, group_no, default_ship_via, notes, bank_account)
		VALUES (".db_escape($customer_id). ",".db_escape($br_name) . ", "
			.db_escape($br_ref) . ", "
			.db_escape($br_address) . ", ".db_escape($salesman) . ", "
			.db_escape($area) . ","
			.db_escape($tax_group_id) . ", "
			.db_escape($sales_account) . ", "
			.db_escape($receivables_account) . ", "
			.db_escape($payment_discount_account) . ", "
			.db_escape($sales_discount_account) . ", "
			.db_escape($default_location) . ", "
			.db_escape($br_post_address) . ","
			.db_escape($group_no) . ", "
			.db_escape($default_ship_via). ", "
			.db_escape($notes). ", "
			.db_escape($bank_account, true).")";
	db_query($sql,"The branch record could not be added");
}

function update_branch(string|array|null $customer_id, string|int|float|bool|null $branch_code, string|array|null $br_name, string|array|null $br_ref, string|array|null $br_address,
	string|array|null $salesman, string|array|null $area, string|array|null $tax_group_id, string|array|null $sales_account, string|array|null $sales_discount_account, string|array|null $receivables_account, 
	string|array|null $payment_discount_account, string|array|null $default_location, string|array|null $br_post_address, string|array|null $group_no,
	string|array|null $default_ship_via, string|array|null $notes, string|array|null $bank_account): void
{
	$sql = "UPDATE ".TB_PREF."cust_branch SET br_name = " . db_escape($br_name) . ",
		branch_ref = " . db_escape($br_ref) . ",
		br_address = ".db_escape($br_address). ",
		salesman= ".db_escape($salesman) . ",
		area=".db_escape($area) . ",
		tax_group_id=".db_escape($tax_group_id). ",
		sales_account=".db_escape($sales_account) . ",
		sales_discount_account=".db_escape($sales_discount_account) . ",
		receivables_account=".db_escape($receivables_account) . ",
		payment_discount_account=".db_escape($payment_discount_account) . ",
		default_location=".db_escape($default_location) . ",
		br_post_address =".db_escape($br_post_address) . ",
		group_no=".db_escape($group_no) . ", 
		default_ship_via=".db_escape($default_ship_via) . ",
		notes=".db_escape($notes) . ",
		bank_account=".db_escape($bank_account, true)."
		WHERE branch_code =".db_escape($branch_code) . "
		AND debtor_no=".db_escape($customer_id);
	db_query($sql,"The branch record could not be updated");
}

function delete_branch(string|array|null $customer_id, string|int|float|bool|null $branch_code): void
{
	delete_entity_contacts('cust_branch', $branch_code);

	$sql="DELETE FROM ".TB_PREF."cust_branch WHERE branch_code=".db_escape($branch_code)." AND debtor_no=".db_escape($customer_id);
	db_query($sql,"could not delete branch");
}

function branch_in_foreign_table(string|array|null $customer_id, string|int|float|bool|null $branch_code, ?string $table): bool
{
	$sql= "SELECT COUNT(*) FROM ".TB_PREF."$table WHERE branch_code=".db_escape($branch_code)
		." AND debtor_no = ".db_escape($customer_id);
	$result = db_select($sql,"could not query $table");
	$myrow = row_or_empty(db_fetch_row($result));
	return ($myrow[0] > 0);
}

/**
 * @return array<array-key, null|string>|false
 */
function get_branch(string|array|null $branch_id)
{
	$sql = "SELECT branch.*, salesman.salesman_name 
		FROM "
			.TB_PREF."cust_branch branch,"
			.TB_PREF."salesman salesman
		WHERE branch.salesman=salesman.salesman_code 
			AND branch_code=".db_escape($branch_id);

	$result = db_select($sql, "Cannot retreive a customer branch");

	return db_fetch($result);
}

/**
 * @return array<array-key, null|string>|false
 */
function get_cust_branch(string|array|null $customer_id, string|array|null $branch_code)
{
	$sql = "SELECT * FROM ".TB_PREF."cust_branch
		WHERE branch_code=".db_escape($branch_code)."
		AND debtor_no=".db_escape($customer_id);
	$result = db_select($sql,"check failed");
	return db_fetch($result);
}

/**
 * @return array<array-key, null|string>|false
 */
function get_branch_accounts(string|int|float|bool|array|null $branch_id)
{
	$sql = "SELECT receivables_account,sales_account, sales_discount_account, payment_discount_account 
		FROM ".TB_PREF."cust_branch WHERE branch_code=".db_escape($branch_id);
	
	$result = db_select($sql, "Cannot retreive a customer branch");
	
	return db_fetch($result);
}

/**
 * @return null|string
 */
function get_branch_name(string|int|float|bool|null $branch_id)
{
	$sql = "SELECT br_name FROM ".TB_PREF."cust_branch 
		WHERE branch_code = ".db_escape($branch_id);

	$result = db_select($sql,"could not retreive name for branch" . $branch_id);

	$myrow = row_or_empty(db_fetch_row($result));
	return $myrow[0];
}

/**
 * @return mysqli_result
 */
function get_cust_branches_from_group(?string $group_no)
{
	$sql = "SELECT branch_code, debtor_no FROM ".TB_PREF."cust_branch 
		WHERE group_no = ".db_escape($group_no);

	return db_select($sql,"could not retreive branches for group " . $group_no);
}

/**
 * @return array<array-key, null|string>|false
 */
function get_default_info_for_branch(string|array|null $customer_id)
{
	$sql = "SELECT name, address, debtor_ref
		FROM ".TB_PREF."debtors_master WHERE debtor_no = ".db_escape($customer_id);
	$result = db_select($sql,"check failed");
	return db_fetch($result);
}

function get_sql_for_customer_branches(string|array|null $customer_id): string
{
	$sql = "SELECT "
		."b.branch_code, "
		."b.branch_ref, "
		."b.br_name, "
		."p.name as contact_name, "
		."s.salesman_name, "
		."a.description, "
		."p.phone, "
		."p.fax, "
		."p.email, "
		."t.name AS tax_group_name, "
		."b.inactive
		FROM ".TB_PREF."cust_branch b "
		. "LEFT JOIN ".TB_PREF."crm_contacts c
			ON c.entity_id=b.branch_code AND c.type='cust_branch' AND c.action='general'
			LEFT JOIN ".TB_PREF."crm_persons p on c.person_id=p.id,"
			.TB_PREF."areas a, "
			.TB_PREF."salesman s, "
			.TB_PREF."tax_groups t
		WHERE b.tax_group_id=t.id
		AND b.area=a.area_code
		AND b.salesman=s.salesman_code
		AND b.debtor_no = ".db_escape($customer_id);

	if (!get_post('show_inactive')) $sql .= " AND !b.inactive";
	$sql .= " GROUP BY b.branch_code ORDER BY branch_ref";

	return $sql;
}
/*
	Get contacts of given type for customer branch.
	$branch_code - branch id
	$action - type of contact
	$customer_id - if passed: get also customer contacts for given action
	$default - return only single record selected with defaults order defined in $defs array,
	 otherways get all $action contacts
*/
function get_branch_contacts(string|int|float|bool|null $branch_code, ?string $action=null, string|int|float|bool|null $customer_id=null, bool $default = true)
{
	$defs = array('cust_branch.'.$action, 
				'customer.'.$action,
				'cust_branch.general',
				'customer.general');

	$sql = "SELECT p.*, r.action, r.type, CONCAT(r.type,'.',r.action) as ext_type 
		FROM "
			.TB_PREF."crm_persons p,"
			.TB_PREF."crm_contacts r
		WHERE r.person_id=p.id AND ((r.type='cust_branch' 
			AND r.entity_id=".db_escape($branch_code).')';
	if((bool)$customer_id) {
		$sql .= " OR (r.type='customer' AND r.entity_id=".db_escape($customer_id).")";
	}
	$sql .= ')';
	
	if ((bool)$action)
		$sql .= ' AND (r.action='.db_escape($action).($default ? " OR r.action='general'" : '').')';
	
	$res = db_select($sql, "can't retrieve branch contacts");

	$results = array();
	while($contact = db_fetch($res))
		$results[] = $contact;

	if ($results && $default) {
		// select first available contact in $defs order
		foreach($defs as $type) {
			if ($n = array_search_value($type, $results, 'ext_type'))
				return $n;
		}
		return null;
	}
	return $results;
}

function _get_branch_contacts(string|int|float|bool|null $branch_code, string|int|float|bool|null $action=null, string|int|float|bool|null $customer_id=null, mixed $default = false)
{
	$sql = "SELECT p.*, r.action, r.type, CONCAT(r.type,'.',r.action) as ext_type 
		FROM ".TB_PREF."crm_persons p,"
			.TB_PREF."crm_contacts r
		WHERE r.person_id=p.id AND r.type='cust_branch' 
			AND r.entity_id=".db_escape($branch_code);

	if ((bool)$action)
		$sql .= ' AND (r.action='.db_escape($action).($default ? " OR r.action='general'":'').')';

	if((bool)$customer_id) {
		$sql = "($sql) UNION (SELECT p.*, r.action, r.type, CONCAT(r.type,'.',r.action) as ext_type
		FROM ".TB_PREF."crm_persons p,"
			.TB_PREF."crm_contacts r 
		WHERE r.person_id=p.id AND r.type='customer' 
			AND r.entity_id=".db_escape($customer_id);
		if ((bool)$action) 
			$sql .= ' AND (r.action='.db_escape($action).($default ? " OR r.action='general'":'').')';
		$sql .= ')';
	}

	$res = db_select($sql, "can't retrieve branch contacts");
	$results = array();
	$type = '';
	while($contact = db_fetch($res)) {
		if ((bool)$type && $type != $contact['type']) break; // skip cust when branch contacts found
		$results[] = $contact;
		$type = $contact['type'];
	}
	if ($results && $default) {
		// here we have general and action contacts selected
		if ($n = array_search_value($action, $results, 'action')) {
			return $n;
		}
		// only general contact found
		return $results[0];
	}
	return $results;
}

/**
 * @return array<array-key, null|string>|false
 */
function get_default_branch(string|int|float|bool|null $customer_id, string|int|float|bool|null $ar_account=null)
{
	$sql = "SELECT * 
		FROM ".TB_PREF."cust_branch WHERE debtor_no = ".db_escape($customer_id);
	if((bool)$ar_account)
		$sql .= " AND receivables_account=".db_escape($ar_account);
	$result = db_select($sql,"cannot retrieve default branch");
	return db_fetch($result);
}

/**
 * @return mysqli_result
 */
function  get_branches_search(string|int|float|bool|null $customer, string|array|null $branch)
{

	if (isset(sysprefs()->max_rows_in_search))
		$limit = sysprefs()->max_rows_in_search;
	else
		$limit = 10;

	$sql = "SELECT 
		    b.branch_code,
	    	b.branch_ref,
	    	b.br_name,
    		p.name as contact_name,
		    p.phone
		FROM ".TB_PREF."cust_branch b
		LEFT JOIN ".TB_PREF."crm_contacts c
			ON c.entity_id=b.branch_code AND c.type='cust_branch' AND c.action='general'
		LEFT JOIN ".TB_PREF."crm_persons p ON c.person_id=p.id
		WHERE b.debtor_no = ".db_escape($customer)."
			AND b.br_name LIKE " . db_escape("%" . $branch. "%") . "
		ORDER BY b.br_name LIMIT 0,".(int)($limit);

	return db_select($sql, "Failed in retreiving branches list.");
}

