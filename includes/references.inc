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
include_once(dirname(__DIR__) . "/includes/db/class.reflines_db.inc");
include_once(dirname(__DIR__) . "/admin/db/fiscalyears_db.inc");
include_once(dirname(__DIR__) . "/includes/types.inc");
//---------------------------------------------------------------------------------------------
// 2.4 - further changes toward removing refs table introduced:
//	. all transactions now have references stored in trans table.
//  . all reference related moved to class (is_new_reference yet preserved)
//	. template based reflines implemented
//
// FIXME:
//		- implement refline field in all transaction tables (obsoletes not always accurate find_refline_id)
//		- remove save() and restore_last() - for now preserved for reflines without placeholder
//		- see fixmes below
//		- remove refs table and create view instead (need e.g. CREATE VIEW support in db_import/db_export)

$refline_options = array(
	ST_JOURNAL => array('date', 'user'),
	ST_COSTUPDATE => array('date', 'user'),

	ST_BANKPAYMENT => array('date', 'user'),
	ST_BANKDEPOSIT => array('date', 'user'),
	ST_BANKTRANSFER => array('date', 'user'),
	ST_SUPPAYMENT => array('date', 'user'),
	ST_CUSTPAYMENT => array('date', 'user'),

	ST_SALESORDER => array('date', 'customer', 'branch', 'user', 'pos'),
	ST_SALESQUOTE => array('date', 'customer', 'branch', 'user', 'pos'),
	ST_SALESINVOICE => array('date', 'customer', 'branch', 'user', 'pos'),
	ST_CUSTCREDIT => array('date', 'customer', 'branch', 'user', 'pos'),
	ST_CUSTDELIVERY => array('date', 'customer', 'branch', 'user', 'pos'),

	ST_LOCTRANSFER => array('date', 'location', 'user'),
	ST_INVADJUST => array('date', 'location', 'user'),

	ST_PURCHORDER => array('date', 'location', 'supplier', 'user'),
	ST_SUPPINVOICE => array('date', 'location', 'supplier', 'user'),
	ST_SUPPCREDIT => array('date', 'location', 'supplier', 'user'),
	ST_SUPPRECEIVE => array('date', 'location', 'supplier', 'user'),

	ST_WORKORDER => array('date', 'location', 'user'),
	ST_MANUISSUE => array('date', 'location', 'user'),
	ST_MANURECEIVE => array('date', 'user'),
	ST_DIMENSION => array('date','user'),
);

$refline_placeholders = array(
	'MM' => 'date',
	'YY' => 'date',
	'YYYY' => 'date',
	'FF' => 'date', // fiscal year
	'FFFF' => 'date',
	'UU' => 'user',
 	'P' => 'pos',
//	 FIXME:  for placeholders below all the code should work, but as the ref length is variable,
//	   length specification in placeholder format should be implemented.
//	'C' => 'customer',
//	'B' => 'branch',
//	'S' => 'supplier',
//	'L' => 'location'
);

final class references
{
	/**
	 * @var reflines_db
	 */
	var $reflines;

	function __construct()
	{
		$this->reflines = new reflines_db();
	}

	/**
	 * @param array<array-key, mixed> $refline
	 * @psalm-pure
	 * @return bool
	 */
	function _legacy_line($refline)
	{
		return strpbrk((string) $refline['pattern'], '{}') == false;
	}

	/**
	 * @param string|int|float|bool|null $type
	 * @param string $template
	 * @param string|array<array-key, mixed>|null $context
	 * @return string
	 */
	function _parse_next($type, $template, $context=null)
	{
		global $refline_options, $refline_placeholders;

		// date based placeholders are always allowed, so default for non-array context is date
		if (!isset($context))
			$context = new_doc_date();

		if (is_string($context))
			$context = array('date' => $context);

		$context['user'] = session_obj('wa_current_user')->user;
		$context['pos'] = session_obj('wa_current_user')->pos;
		$out = '';

		while(($start = strpos($template, '{')) !==false) {

			$out .= substr($template, 0, $start);
			$stop = strpos($template, '}');
			if ($stop === false) {
				display_warning(_("Invalid refline template."));
				$out .= $template; // debug
				break;
			}
			$ph = substr($template, $start+1, $stop-$start-1);
			$template = substr($template, $stop+1);

			if (isset($refline_placeholders[$ph])) {
				if (!isset($context[$refline_placeholders[$ph]]))
				{
					display_warning(sprintf(_("Missing refline context data: '%s'"), $refline_placeholders[$ph]));
					$out .= $ph; // debug
				} else {
					$ctx_date = $context['date'] ?? null;
					$ctx_date = is_scalar($ctx_date) ? $ctx_date : null;
					switch ($ph)
					{
						case 'MM':
						case 'YY':
						case 'YYYY':
							list($day, $month, $year) = explode_date_to_dmy($ctx_date);
							$out .= $ph == 'MM' ? sprintf('%02d', $month) : ($ph == 'YY' ? sprintf('%02d', $year%100): sprintf('%04d', $year));
							break;
						case 'FF':
						case 'FFFF':
							list($day, $month, $year) = explode_date_to_dmy(get_fiscalyear_begin_for_date($ctx_date));
							$out .= $ph == 'FF' ? sprintf('%02d', $year%100): sprintf('%04d', $year);
							break;
						case 'C':
							$ctx_val = $context['customer'];
							$out .= sprintf('%d', is_scalar($ctx_val) ? (int) $ctx_val : 0);
							break;

						case 'B':
							$ctx_val = $context['branch'];
							$out .= sprintf('%d', is_scalar($ctx_val) ? (int) $ctx_val : 0);
							break;

						case 'S':
							$ctx_val = $context['supplier'];
							$out .= sprintf('%d', is_scalar($ctx_val) ? (int) $ctx_val : 0);
							break;

						case 'L':
							$ctx_val = $context['location'];
							$out .= sprintf('%s', is_scalar($ctx_val) ? (string) $ctx_val : '');
							break;

						case 'P':
							$ctx_val = $context['pos'];
							$out .= sprintf('%s', is_scalar($ctx_val) ? (string) $ctx_val : '');
							break;

						case 'UU':
							$ctx_val = $context['user'];
							$out .= sprintf('%02d', (int) $ctx_val);
						break;
					}
				}
			} elseif (is_numeric($ph)) {
				$out .= '{'.$ph.'}'; // index placeholder
			}
		}

		$out .= $template;	// add postfix
		if (!preg_match('/^([^\{]*)?\{([^\}]*)\}(.*)/', $out, $match)) {	// parse index
			display_error(_("Missing numeric placeholder in refline definition."));
			return $out;
		}

		$prefix = $match[1];
		$postfix = $match[3];

		$db_info = get_systype_db_info($type);
		if ($db_info === null) {
			display_error(_("Unknown transaction type."));
			return $out;
		}
		$trans_table = $db_info[0];
		$type_fld = $db_info[1];
		$tno_fld = $db_info[2];
		$ref_fld = $db_info[3];
		$type = db_escape($type);

		// retrieve last ref number in the refline from original transaction table
		$sql = "SELECT MAX(CAST(SUBSTR($ref_fld, ".(strlen($prefix)+1).",LENGTH($ref_fld)-".(strlen($postfix)+strlen($prefix)).") AS UNSIGNED))"
				." FROM `$trans_table` tbl
					LEFT JOIN ".TB_PREF."voided v ON tbl.`$tno_fld`=v.id AND v.type=$type"
				." WHERE ISNULL(v.id)"
				.($type_fld ? " AND tbl.`$type_fld`=$type" : '')
				." AND `$ref_fld` REGEXP ".db_escape('^'.preg_quote($prefix).'[0-9]*'.preg_quote($postfix).'$');
		$result = db_select($sql, 'cannot retrieve last reference');
		$result = row_or_empty(db_fetch_row($result));
		$last_num = $result[0] ?? null;
		$next_num = is_numeric($last_num) ? (int)$last_num + 1 : 1;

			// fill with zeros to the length of original index placeholder
		return $prefix.sprintf('%0'.strlen($match[2]).'d', $next_num).$postfix;
	}

	//
	//	Get/check transaction reference.
	// 	$ref!=null => check reference is not used (or unique for $trans_no!=0)
	//  $trans!=0 $ref=null => retrieve reference for the $type/$trans_no (if any)
	//
	/**
	 * @param string|int|float|bool|null $type
	 * @param string|int|float|bool|null $trans_no
	 * @param string|int|float|bool|null $ref
	 * @return false|null|string
	 */
	function _get($type, $trans_no=0, $ref=null)
	{
		$db_info = get_systype_db_info($type);
		if ($db_info === null)
			return false;
		$trans_table = $db_info[0];
		$type_fld = $db_info[1];
		$tno_fld = $db_info[2];
		$ref_fld = $db_info[3];

		$type = db_escape($type);

		$sql = "SELECT `$ref_fld`
				FROM `$trans_table` tbl
					LEFT JOIN ".TB_PREF."voided v ON
				tbl.`$tno_fld`=v.id AND v.type=$type"
			." WHERE ISNULL(v.id)"
			.($type_fld ? " AND tbl.`$type_fld`=$type" : '');
		if ((bool)$ref)
		{
			$sql .= " AND tbl.`$ref_fld`=".db_escape(trim((string) $ref));
			if ((bool)$trans_no)
				$sql .= " AND tbl.`$tno_fld` != ".db_escape($trans_no);
		} else {
			$sql .= " AND tbl.`$tno_fld`=".db_escape($trans_no);
		}

		$result = db_query($sql, "could not test for unique reference");
		if (!($result instanceof mysqli_result))
			return false;

		$result = db_fetch_row($result);
		return $result == false ? false : (is_scalar($result[0]) ? $result[0] : null);
	}

	/**
	 * @param string|int|float|bool|null $ref
	 * @param string|int|float|bool|null $type
	 * @param string|int|float|bool|null $trans_no
	 * @return bool
	 */
	function is_new_reference($ref, $type, $trans_no=0)
	{
		return !(bool)$this->_get($type, $trans_no, $ref);
	}

	//
	// Get default reference for new transaction.
	//
	/**
	 * @param string|int|float|bool|null $type
	 * @param string|int|array<array-key, mixed>|null $line
	 * @param string|array<array-key, mixed>|null $context
	 * @return string
	 */
	function get_next($type, $line=null, $context=null)
	{

		if (isset($line))
			$refline = $this->reflines->get($line);
		else {
			$refs = $this->reflines->get_all("trans_type=".db_escape($type)." AND `default`");
			$refline = $refs instanceof mysqli_result ? db_fetch($refs) : false;
		}
		$refline = row_or_empty($refline);
		if (!isset($refline['pattern']))
			return '';

		if ($this->_legacy_line($refline))
			return (string) $refline['pattern'];

		return $this->_parse_next($type, (string)$refline['prefix'].(string)$refline['pattern'], $context);
	}

	/**
 	*	Normalize reference to format allowed by refline (optionally selected by prefix).
 	*	FIXME: currently this is fake function which return either input reference or
 	*	next reference when no line has been recognized.
	* @param string|int|float|bool|null $ref
	* @param string|int|float|bool|null $type
	* @param string|array<array-key, mixed>|null $context
	* @param string|int|array<array-key, mixed>|null $line
	* @return string|int|float|bool|null
	**/
	function normalize($ref, $type, $context, $line=null)
	{
		if (!isset($type)) // inquiries
			return $ref;

		if (!(bool)$line)
			$line = $this->reflines->find_refline_id($ref, $type);

		return $this->is_valid($ref, $type, $context, $line) ? $ref : $this->get_next($type, $line, $context);
	}

	//
	// Check reference is valid before add/update transaction.
	// FIXME: does not check leading zeros in number
	//
	/**
	 * @param string|int|float|bool|null $reference
	 * @param string|int|float|bool|null $type
	 * @param string|array<array-key, mixed>|null $context
	 * @param string|int|array<array-key, mixed>|null $line
	 * @return 0|1|bool
	 */
 	function is_valid($reference, $type, $context=null, $line=null)
	{
 		if (!isset($line))
 			$line = $this->reflines->find_refline_id($reference, $type, true);

 		if (!isset($line))
 			return false;

		$refline = $this->reflines->get($line);
		$refline = is_array($refline) ? $refline : array();

		if ($this->_legacy_line($refline))	//legacy non-templated line
			return strlen(trim((string) $reference)) > 0;

		$regex = preg_quote((string)$refline['prefix'].(string)$refline['pattern']);
		if (!is_array($context))
			$context = array('date'=>$context);

 		$context['pos'] = session_obj('wa_current_user')->pos;

		$ctx_date = $context['date'] ?? null;
		$ctx_date = is_string($ctx_date) ? $ctx_date : null;
		if (is_date($ctx_date))
		{
			list($year4, $month, $day) = explode("-", (string) date2sql($ctx_date));
			$year2 = substr($year4, 2);
			$f_year = explode("-", (string) date2sql(get_fiscalyear_begin_for_date($ctx_date)));
			$f_year2 = substr($f_year[0], 2);
			$f_year4 = $f_year[0];
		} else
		{
			$month = '\d{2,}';
			$year2 = '\d{2,}';
			$year4 = '\d{4,}';
			$f_year2 = '\d{2,}';
			$f_year4 = '\d{4,}';
		}
		$ctx_cust = $context['customer'] ?? null;
		$ctx_supp = $context['supplier'] ?? null;
		$ctx_branch = $context['branch'] ?? null;
		$ctx_location = $context['location'] ?? null;
		$ctx_pos = $context['pos'] ?? null;
		$cust = is_scalar($ctx_cust) && (bool)$ctx_cust ? (string) $ctx_cust : '\d+';
		$supp = is_scalar($ctx_supp) && (bool)$ctx_supp ? (string) $ctx_supp : '\d+';
		$branch = is_scalar($ctx_branch) && (bool)$ctx_branch ? (string) $ctx_branch : '\d+';
		$location = is_scalar($ctx_location) && (bool)$ctx_location ? (string) $ctx_location : '[a-z0-9]+';
 		$pos = is_scalar($ctx_pos) && (bool)$ctx_pos ? (string) $ctx_pos : '\d+';
		$user = sprintf("%02d", session_obj('wa_current_user')->user);

		$regex = preg_replace(
			array(
				'/\\\{/',	// unquote placeholders
				'/\\\}/',
				'/\{MM\}/',
				'/\{YY\}/',
				'/\{YYYY\}/',
				'/\{FF\}/',
				'/\{FFFF\}/',
				'/\{C\}/',
				'/\{B\}/',
				'/\{S\}/',
				'/\{L\}/',
				'/\{UU\}/',
 				'/\{P\}/',
				'/\{\d+}/',
			),
			array(
				'{',
				'}',
				$month,
				$year2,
				$year4,
				$f_year2,
				$f_year4,
				$cust,
				$branch,
				$supp,
				$location,
				$user,
 				$pos,
				'\d+',
			), $regex) ?? '';

		$regex = '"^'.$regex.'"i';

		return preg_match($regex, (string) $reference, $match) ? 1 : 0;
	}

	//
	//	Save reference (and prepare next) on write transaction.
	//
	/**
	 * @param string|int|float|bool|null $type
	 * @param string|int|float|bool|null $id
	 * @param string|int|float|bool|null $reference
	 * @param string|int|array<array-key, mixed>|null $line
	 * @return void
	 */
	function save($type, $id, $reference, $line = null)
	{
		if ($reference == 'auto')
			return;

	    $sql = "REPLACE ".TB_PREF."refs SET reference=".db_escape($reference)
			.", type=".db_escape($type).", id=".db_escape($id);
    	db_query($sql, "could not update reference entry");

		if (!isset($line))
		{
			$line = $this->reflines->find_refline_id($reference, $type);
		}

		$refline = $this->reflines->get($line);
		$refline = is_array($refline) ? $refline : array();
		 // legacy code used with simple templates
		if ($this->_legacy_line($refline) && ($reference == $this->get_next($type, $line))) { // if reference was not changed from default
			$next = $this->_increment($reference);	// increment default
			$this->reflines->save_next($type, $next, $line);
		}
	}
	//
	// Restore previous reference (if possible) after voiding transaction.
	//
	/**
	 * @param string|int|float|bool|null $type
	 * @param string|int|float|bool|array|null $id
	 * @return void
	 */
	function restore_last($type, $id)
	{
		// get refline for removed document reference
		$reference = get_reference($type, $id);
		$line = $this->reflines->find_refline_id($reference, $type);
		$refline = $this->reflines->get($line);
		$refline = is_array($refline) ? $refline : array();

		if ($this->_legacy_line($refline)) // legacy code used with simple templates
		{
			$last = $this->_increment($this->get_next($type, $line), true); // find last reference used in this line
			if ($reference == $last)
			{
				// save last reference as next
			    $sql = "UPDATE ".TB_PREF."reflines SET pattern=SUBSTRING(" . db_escape(trim((string) $last)) .", LENGTH(`prefix`)+1)"
					. " WHERE trans_type = ".db_escape($type) . " AND `id`=".db_escape($line);

				db_query($sql, "The next transaction ref for $type could not be updated");
			}
		}
	}

	//-----------------------------------------------------------------------
	//
	//	Increments (or decrements if $back==true) reference template
	//
	/**
	 * @param string|int|float|bool|null $reference
	 * @param bool $back
	 * @return string|int|float|bool|null
	 * @psalm-pure
	 */
	function _increment($reference, $back=false)
	{
		// Legacy code used when no palceholder is in use:
		//  WA036 will increment to WA037 and so on.
       	// If $reference contains at least one group of digits,
        // extract first didgits group and add 1, then put all together.
        // NB. preg_match returns 1 if the regex matches completely
        // also $result[0] holds entire string, 1 the first captured, 2 the 2nd etc.
        //
        if (preg_match('/^(\D*?)(\d+)(.*)/', (string) $reference, $result) == 1)
        {
			list($all, $prefix, $number, $postfix) = $result;
			$dig_count = strlen($number); // How many digits? eg. 0003 = 4
			$fmt = '%0' . $dig_count . 'd'; // Make a format string - leading zeroes
			$num = (int) $number;
			$val = $num + ($back ? ($num<1 ? 0 : -1) : 1);
			$nextval =  sprintf($fmt, $val); // Add one on, and put prefix back on

			return $prefix.$nextval.$postfix;
        }
        else 
            return $reference;
	}

}

//----------------------------------------------------------------------------
//
//	Check if reference was not used so far (for other transaction than $trans_no)
//
/** @return bool */
function is_new_reference(string|int|float|bool|null $ref, string|int|float|bool|null $type, ?int $trans_no=0)
{

	return refs()->is_new_reference($ref, $type, $trans_no);
}

/** @return false|null|string */
function get_reference(string|int|float|bool|null $type, string|int|float|bool|array|null $trans_no)
{

	return refs()->_get($type, is_array($trans_no) ? null : $trans_no);
}

/**
 * The $Refs object created by session.inc. Psalm forgets the type of a global after any function call, so code
 * reaches it through this accessor instead of `global $Refs`.
 *
 * @return references
 */
function refs()
{
	global $Refs;

	return $Refs;
}
