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
include_once 'class.data_set.inc';

/**
 * Data as submitted for insert/update (prefix/trans_type/pattern/etc. validation).
 * @psalm-type ReflineData = array{trans_type?: string|int|float|bool|null, prefix?: string|int|float|bool|null, pattern?: string|int|float|bool|null, description?: string|int|float|bool|null, default?: string|int|float|bool|null, id?: string|int|float|bool|null, inactive?: string|int|float|bool|null}
 */

/**
 * A row as read back from the reflines table; mysqli returns every column as a string or null.
 * @psalm-type ReflineRow = array{trans_type?: string|null, prefix?: string|null, pattern?: string|null, description?: string|null, default?: string|null, id?: string|null, inactive?: string|null}
 */

/**
*	Reference lines.
*
**/

final class reflines_db extends data_set {
	function __construct()
	{
		$this->set_validator('prefix:ui:_check_prefix', _("This prefix conflicts with another one already defined. Prefix have to be unambigous."));
		$this->set_validator('prefix:ui:_check_template', _("Invalid template format."));
		$this->set_validator('trans_type:ui:required', _("Transaction type cannot be empty."));
		$this->set_validator('pattern:ui:required', _("Next reference cannot be empty."));
		parent::__construct('reflines', 
			array('trans_type', 'prefix', 'description', 'default', 'pattern', 'id', 'inactive'), 
			'id');
	}

	/*
		Prefix cannot be ambigous.
	*/
	/**
	 * @param array<array-key, mixed> $data
	 * @param array<array-key, mixed> $dummy_opt
	 * @param string|int|array<array-key, mixed>|null $key
	 */
	function _check_prefix($data, $dummy_opt, $key): bool
	{
		/** @var ReflineData */
		$data = $data;
		$prefix = (string) ($data['prefix'] ?? '');
		$pattern = (string) ($data['pattern'] ?? '');
		$cond = "`id`<>".db_escape(is_array($key) ? null : $key)." AND `trans_type`=".db_escape($data['trans_type'] ?? null);
		if ($prefix === '')
			$cond .= " AND `prefix`=''";
		else
			$cond .= "AND ((LOCATE('".$prefix."', CONCAT(`prefix`,`pattern`))=1 OR (`prefix`<>'' AND LOCATE(`prefix`, '".$prefix.$pattern."')=1)))";

		// get_all() can return false on a failed query; db_num_rows() requires a real mysqli_result.
		$rows = $this->get_all($cond);
		return !($rows instanceof mysqli_result) || db_num_rows($rows) == 0;
	}

	/**
	 * @param array<array-key, mixed> $data
	 * @param array<array-key, mixed> $dummy_opt
	 * @param string|int|array<array-key, mixed>|null $key
	 */
	function _check_template($data, $dummy_opt, $key): bool
	{
		global $refline_options, $refline_placeholders;

		/** @var ReflineData */
		$data = $data;
		$prefix = (string) ($data['prefix'] ?? '');
		$pattern = (string) ($data['pattern'] ?? '');
		$trans_type = (int) ($data['trans_type'] ?? 0);

		if (strpbrk($prefix, '{}') !== false)
			return $this->error(_("You cannot use placeholders in refline prefix."));

		if (substr_count($pattern, '{') != substr_count($pattern, '}'))
			return $this->error(_("Curly brackets does not balance."));

		if ((bool)preg_match_all('/\{([^\}]*)\}/', $pattern, $match)) // placeholders defind in template
		{
			$numph = 0;
			foreach($match[1] as $ph) {
				if (is_numeric($ph))
					$numph++;
				elseif (!isset($refline_placeholders[$ph]) || !@in_array($refline_placeholders[$ph], $refline_options[$trans_type] ?? array())) {
					$allowed = array();
					foreach($refline_placeholders as $id => $dt)
						 if (in_array($dt, $refline_options[$trans_type] ?? array()))
						 	$allowed[] = $id;

					return $this->error(sprintf(_("Invalid placeholder '%s'. Placeholders allowed for this transaction type are: %s."),
						$ph, implode(',', $allowed)));
				}
			}

			if ($numph !== 1)
				return $this->error(_("Missing numeric placeholder. If you want to use template based references, you have to define numeric placeholder too."));
		}
		return true;
	}

	/**
	 * @param string|int|float|bool|array<array-key, mixed>|null $prefix
	 * @param string|int|float|bool|array<array-key, mixed>|null $trans_type
	 * @return int|string
	 */
	function is_used($prefix, $trans_type)
	{
		$prefix = is_array($prefix) ? null : $prefix;
		$trans_type = is_array($trans_type) ? null : $trans_type;

		$sql = "SELECT *
			FROM (SELECT r.* FROM ".TB_PREF."refs r 
				LEFT JOIN ".TB_PREF."voided as v
					ON r.id=v.id AND r.type=v.type
				WHERE r.type=".db_escape($trans_type)." AND ISNULL(v.id)
				) ref
			LEFT JOIN ".TB_PREF."reflines line ON ref.type=line.trans_type AND substr(ref.reference,1, LENGTH(line.prefix))= line.prefix AND line.prefix<>''
			WHERE ".($prefix == '' ? "ISNULL(prefix)" : "prefix=".db_escape($prefix));

		$res = db_select($sql, "cannot check reference line");

		return db_num_rows($res);
	}

	/** @param string|int|array<array-key, mixed>|null $key */
	#[\Override]
	function delete_check($key): bool
	{
		$rec = row_or_empty($this->get($key));
		/** @var ReflineRow */
		$rec = $rec;
		if ((bool) ($rec['default'] ?? false))
			return $this->error(_("Reference line which is default for any transaction type cannot be deleted."));

		if ($this->is_used($rec['prefix'] ?? null, $rec['trans_type'] ?? null))
			return $this->error(_("Reference line cannot be deleted because it is already in use."));

		return true;
	}

	/**
	 * @param string|int|array<array-key, mixed>|null $id
	 * @param string|int|float|bool|array<array-key, mixed>|null $type
	 * @return bool|mysqli_result
	 */
	function _set_as_default($id, $type)
	{
		$sql  = "UPDATE ".TB_PREF."reflines SET `default`=(`id`=".db_escape(is_array($id) ? null : $id).")
			 WHERE `trans_type`=".db_escape(is_array($type) ? null : $type);
		return db_query($sql, "cannot update default refline");
	}

	/** @param array<array-key, mixed>|null $data */
	#[\Override]
	function insert($data)
	{
		if (!parent::insert($data))
			return false;

		/** @var ReflineData */
		$data = $data ?? array();
		if ((bool) ($data['default'] ?? false))
			return (bool) $this->_set_as_default(db_insert_id(), $data['trans_type'] ?? null);
		return true;
	}

	/**
	 * @param string|int|array<array-key, mixed>|null $key
	 * @param array<array-key, mixed>|null $data
	 */
	#[\Override]
	function update($key, $data)
	{
		if (!parent::update($key, $data))
			return false;

		/** @var ReflineData */
		$data = $data ?? array();
		if ((bool) ($data['default'] ?? false))
			return (bool) $this->_set_as_default($key, $data['trans_type'] ?? null);
		return true;
	}

	/**
	 * @param string|int|float|bool|array<array-key, mixed>|null $type
	 * @return array<array-key, null|string>|false
	 */
	function get_default($type)
	{
		$type = is_array($type) ? null : $type;
		$sql = "SELECT * FROM ".TB_PREF."reflines WHERE trans_type=".db_escape($type)." AND `default`";
		$result = db_select($sql, "cannot retreive default refline for trnasaction type $type");
		return db_fetch($result);
	}

	/**
	 * @param string|int|float|bool|array<array-key, mixed>|null $type
	 * @return int|string|null
	 */
	function count($type, bool $all=false)
	{
		$type = is_array($type) ? null : $type;
		$sql = "SELECT count(*) FROM ".TB_PREF."reflines WHERE trans_type=".db_escape($type);

		if (!$all)
			$sql .= " AND !inactive";
		$result = db_select($sql, "cannot retreive refline count for transaction type $type");
		$rec = row_or_empty(db_fetch($result));
		return $rec ? $rec[0] : 0;
	}

	/*
		Recognize refline by reference prefix
	*/
	/**
	 * @param string|int|float|bool|array<array-key, mixed>|null $reference
	 * @param string|int|float|bool|array<array-key, mixed>|null $type
	 * @return string|null
	 */
	function find_refline_id($reference, $type, bool $fallback=true)
	{
		$reference = is_array($reference) ? null : $reference;
		$type = is_array($type) ? null : $type;
		$sql = "SELECT * FROM ".TB_PREF."reflines WHERE trans_type=".db_escape($type)
			." AND CHAR_LENGTH(`prefix`) AND LEFT(".db_escape($reference).", CHAR_LENGTH(`prefix`)) = `prefix`";
		if ($fallback)	// if not found return refline with empty prefix
			$sql .= " UNION SELECT * FROM ".TB_PREF."reflines WHERE trans_type=".db_escape($type)." AND `prefix`=''";
		$ret = db_select($sql, "cannot check reference line id");
		$line = row_or_empty(db_fetch($ret));

		if (!$fallback && (db_num_rows($ret) != 1))	// more than one record means ambigous reference line
			return null;

		// mysqli returns every column as a string or null, never int
		$id = $line['id'] ?? null;
		return is_string($id) ? $id : null;
	}

	/**
	 * @param string|int|float|bool|array<array-key, mixed>|null $type
	 * @param string|int|array<array-key, mixed>|null $line
	 * @return bool|mysqli_result
	 */
	function save_next($type, string $reference, $line=null)
	{
		$type = is_array($type) ? null : $type;
		$line = is_array($line) ? null : $line;
	    $sql = "UPDATE ".TB_PREF."reflines SET pattern=SUBSTRING(" . db_escape(trim($reference)) .", LENGTH(`prefix`)+1)"
		. " WHERE trans_type = ".db_escape($type)." AND ";

		if (isset($line))
			$sql .= "`id`=".db_escape($line);
		else
			$sql .= "`default`";

		return db_query($sql, "The next transaction ref for $type could not be updated");
	}
}
