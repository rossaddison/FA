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
include_once(dirname(__DIR__, 2) . "/includes/packages.inc");
//
//	Utility class contains basic database upgrade routines.
//
class fa_patch {
	/** @var string */
	var $previous = '';		// previous database version
	/** @var string */
	var $version = '';	 		// version after upgrade
	/** @var string */
	var $description = '';		// short patch description

	/** @var string */
	var $sql = '';				// basic sql file

	/** @var int|string */
	var $cur_company = 0;
	/** @var false|null|string */
	var $backup;				// pre-upgrade backup filename
	/** @var string|false */
	var $save_log = false;		// ini_set('error_log', ...) previous value, restored after upgrade

	/** @var array<int, string|bool|null> */
	var $errors = array();

	/**
	 * @var int
	 */
	var	$max_upgrade_time = 300;
	/** @var array<int, array<string, string>> */
	var $companies = array();

	function __construct()
	{
		global $db_connections;

		include dirname(__DIR__, 2) . "/config_db.php";

		$this->companies = $db_connections;
	}

	/*
		Collect/log messages generated during upgrade process.
	*/
	/**
	 * @param string|bool|null $msg
	 * @param string|null $type
	 * @return false
	 */
	function log_error($msg, $type='Error')
	{
		if ($type == 'Error')
			$this->errors[] = $msg;
		error_log(sprintf('[%s] %s', $type ?? 'Error', (string) ($msg ?? '')));
		return false;
	}

	/*
		Selectively extends access to selected security areas/sections.
	*/
	/**
	 * @param array<array-key, array<array-key, string>> $sec_updates every real caller (the
	 *   sql/alter*.php subclasses, outside Psalm's project scope) passes a hardcoded array of
	 *   security-area constant-name strings
	 * @return true
	 */
	function update_security_roles(array $sec_updates)
	{
		global $security_areas, $security_sections;

		$roles = db_select("SELECT * FROM ".TB_PREF."security_roles");

		while($role = db_fetch($roles))
		{
			$role['areas'] = array_map('intval', explode(';', $role['areas'] ?? ''));
			$role['sections'] = array_map('intval', explode(';', $role['sections'] ?? ''));
			foreach($sec_updates as $has => $add)
			{
				if (in_array($security_areas[(string) $has][0], $role['areas']))
				{
					$sections = array();
					foreach($add as $area)
					{
						$role['areas'][] = $security_areas[$area][0];
						$role['sections'][] = $security_areas[$area][0]&~0xff;
					}
					sort($role['areas']);
					update_security_role($role['id'], $role['role'], $role['description'],
						$role['sections'], $role['areas']);
				}
			}
		}
		return true;
	}

	/*
		Check and disable incompatible extensions.
	*/
	/** @return void */
	function update_extensions()
	{
		global $version;

		$mods = get_company_extensions();
		$exts = get_company_extensions($this->cur_company);

		$fixed = false;
		foreach($mods as $key => $ins) {
			// get_company_extensions() deliberately types each entry's values as mixed --
			// extension metadata is author-supplied, already guarded with is_scalar() below
			$ins_version = $ins['version'] ?? null;
			foreach($exts as $ext)
				if ($ext['name'] == $ins['name'] && (!check_src_ext_version(is_scalar($ins_version) ? $ins_version : null))) {
					$mods[$key]['active'] = false;
					$this->log_error(sprintf(_("Uncompatible extension '%s' disabled for company %d."), (string) ($ins['name'] ?? ''), $this->cur_company), 'Notice');
					$fixed = true;
					continue 2;
				}
		}
		if ($fixed)
			write_extensions($mods, $this->cur_company);
	}

	/*
		Pre-install maintenance: login to company, open upgrade log, make a backup
	*/
	/** @return bool */
	function pre_install(string|int $company)
	{

		$this->cur_company = $company;
		$this->errors = array();
		$this->backup = null;

		$this->save_log = ini_set('error_log', VARLOG_PATH.'/upgrade.'.$this->cur_company.'.log');
		$this->log_error(sprintf(_("Upgrade started for company %s."), $this->cur_company), 'Info');

		if (!set_global_connection($this->cur_company))
			return $this->log_error(_("Cannot connect to company database."));

		$cur_ver = get_company_pref('version_id');
		if ($cur_ver != $this->previous)
			return $this->log_error(sprintf(_("Cannot upgrade company %s: database version is incompatible ('%s' instead of '%s')."),
				$this->cur_company, (string) ($cur_ver ?? ''), $this->previous));

		$this->update_extensions();	// disable uncompatible extensions

		if (!$this->prepare())	// fetch params, perform additional checks (if any)
		  return false;

		if (!$this->sql)
			return true;	// skip security backup if database content is not changed

		$this->backup = db_backup($this->companies[(int) $this->cur_company], 'no', 'Security backup before upgrade',
			sysprefs()->backup_dir($this->cur_company));

		if (!(bool)$this->backup)
		  return $this->log_error(_("Security backup failed."));

		$this->log_error(sprintf(_("Security backup in file %s done."), $this->backup), 'Info');
		return true;
	}

	/*
		Basic install procedure using sql file.
	*/
	/** @return bool */
	function sql_install(string|int $company, bool $force=false)
	{
		global $path_to_root;

		if ($this->sql != '')	// perform basic upgrade operations defined in sql file
		{
			$result = db_import($path_to_root. '/sql/'.$this->sql, $this->companies[(int) $company],
				$force,	true, false, true);

			if ($result !== true)
			{
				if (is_array($result))
				{
					foreach($result as $err)
						$this->log_error($err[1] . ':'. $err[0]);
				} else
				{
					$this->log_error($result);
					unset($this->backup); // prevent restore (database was not touched due to other errors)
				}
				return false;
			}
		}
		return true;
	}

	/*
		Post install procedures: update database version, or restore databse from backup file in case of errors
	*/
	/**
	 * @param bool $result
	 * @return array<array-key, mixed>|bool|string
	 */
	function post_install($result=true)
	{
		global $db_version;

		if ($result !== true)
		{
			if ((bool)$this->backup)
			{
				if (!set_global_connection($this->cur_company)) // reset connection to clear encoding
					return $this->log_error(_("Cannot connect to company database for database restore."));

				set_time_limit($this->max_upgrade_time);
				$result = db_import($this->backup, $this->companies[(int) $this->cur_company], true, false);
				if ((bool)$result)
					$this->log_error(_("Upgrade failed. Original database content restored successfully."), 'Info');
				else
					$this->log_error(sprintf(_("Database restore operation failed. Original database content is in %s file."), $this->backup));
				$this->post_fail($this->cur_company);
			}
		} else {
			update_company_prefs(array('version_id' => $this->version));
		}

		$this->log_error(sprintf(_("Upgrade for company %s finished."), $this->cur_company), 'Info');

		set_global_connection();
		ini_set('error_log', $this->save_log);

		if (db_fixed())
			db_set_encoding();

		return $result;
	}

	/*
		Main routine for single company upgrade.
	*/
	/** @return bool */
	function upgrade_company(string|int $comp, bool $force=false)
	{
		$result = $this->pre_install($comp) && $this->sql_install($comp, $force) && $this->install($comp, $force);

		$this->post_install($result);

		return count($this->errors) == 0;
	}

	/*
		Additional version specific php/sql upgrade procedures.
		This procedure is performed after basic sql upgrade script is run.
	*/
	/**
	 * Base no-op is pure, but this is a hook real upgrade subclasses override
	 * with side-effecting, possibly-failing logic (see e.g. sql/alter2.4.php's
	 * install(), which can return false via update_workorders()) - declared
	 * bool, not true, to match what overrides actually need to return.
	 * @return bool
	 */
	function install(string|int $company, bool $force=false)
	{
		return true;
	}
	/*
		Optional cleanup procedure.
		This procedure is run in case of upgrade failure, before the backup is restored.
	*/
	/**
	 * @psalm-mutation-free
	 * @return void
	 */
	function post_fail(string|int $company)
	{
	}

    /*
		Present upgrade parameters to administrator
		This function presents upgrade choices, after selection company to be upgraded.
    */
	/**
	 * @psalm-mutation-free
	 * @return void
	 */
	function show_params(string|int $comp)
    {
	}

    /*
	    Fetch & check upgrade parameters, check additional upgrade pre-conditions.
		This function is run after successfull switching to target database connection.
    */
	/**
	 * Base no-op is pure, but this is a hook real upgrade subclasses override
	 * with checks that can genuinely fail (see e.g. sql/alter2.2.php's
	 * prepare(), which returns isset($security_groups) || ... and can be
	 * false) - declared bool, not true, to match what overrides actually
	 * need to return.
	 * @return bool
	 */
	function prepare()
    {
		return true;
	}

}

/*
	Return databases status info.
*/
/**
 * @param array<int, array<string, string>> $connections
 * @return array<int, array<string, mixed>>
 */
function get_site_status(array $connections): array
{

		$info = array();

		foreach($connections as $i => $conn)
		{
			$info[$i]['status'] = set_global_connection($i) !== false;

			$info[$i]['name'] = $conn['name'];
			$info[$i]['table_set'] = $conn['host'].'/'.$conn['dbname'].':'.$conn['tbpref'].'*';
			if ($info[$i]['status'])
			{
				$info[$i]['version'] = get_company_pref('version_id');
			}
		}
		set_global_connection();
		sysprefs()->refresh();

		return $info;
}

/*
	Creates table of installer objects sorted by applicable db scheme version.
*/
function get_installers(): array
{
	global $path_to_root;

	$patchdir = $path_to_root."/sql/";
	$upgrades = array();	
	$datadir = @opendir($patchdir);

	if ($datadir)
	{
		while(false !== ($fname = readdir($datadir)))
		{ // check all php files but index.php
			if (!is_dir($patchdir . $fname) && ($fname != 'index.php')
				&& stristr($fname, '.php') != false && $fname[0] != '.')
			{
				// UnresolvableInclude: this is the real discovery mechanism for every
				// sql/alter*.php upgrade script -- $fname comes from readdir() on a fixed,
				// admin-controlled directory, never user input, so genuinely not resolvable
				// statically, not a security concern
				/** @var fa_patch|null $install */
				$install = null;
				include_once($patchdir . $fname);
				if (isset($install)) // add installer if found
					$upgrades[$install->previous] =  $install;
			}
		}
		ksort($upgrades); // sort by file name
	}
	return $upgrades;
}
