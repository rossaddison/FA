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
$page_security = 'SA_DIMENSIONREP';
// ----------------------------------------------------------------
// $ Revision:	2.0 $
// Creator:	Joe Hunt
// date_:	2005-05-19
// Title:	Dimension Summary
// ----------------------------------------------------------------
/** @var string $path_to_root */
$path_to_root="..";

include_once(dirname(__DIR__) . "/includes/session.inc");
include_once(dirname(__DIR__) . "/includes/date_functions.inc");
include_once(dirname(__DIR__) . "/includes/data_checks.inc");
include_once(dirname(__DIR__) . "/gl/includes/gl_db.inc");

//----------------------------------------------------------------------------------------------------

print_dimension_summary();

/**
 * @return bool|mysqli_result
 */
function getTransactions(string|int|array|null $from, string|array|null $to)
{
	$sql = "SELECT *
		FROM
			".TB_PREF."dimensions
		WHERE id >= ".db_escape($from)."
		AND id <= ".db_escape($to)."
		ORDER BY
			reference";

    return db_query($sql,"No transactions were returned");
}

function getYTD(?string $dim): float|null|string
{
	$date = begin_fiscalyear();
	$date = date2sql($date);
	
	$sql = "SELECT SUM(amount) AS Balance
		FROM
			".TB_PREF."gl_trans
		WHERE (dimension_id = ".db_escape($dim)." OR dimension2_id = ".db_escape($dim).")
		AND tran_date >= '$date'";

    $TransResult = db_select($sql,"No transactions were returned");
	if (db_num_rows($TransResult) == 1)
	{
		$DemandRow = row_or_empty(db_fetch_row($TransResult));
		$balance = $DemandRow[0];
	}
	else
		$balance = 0.0;

    return $balance;
}

//----------------------------------------------------------------------------------------------------

function print_dimension_summary(): void
{
    global $path_to_root;

    $fromdim = post_scalar('PARAM_0');
    $todim = post_scalar('PARAM_1');
    $showbal = post_scalar('PARAM_2');
    $comments = post_scalar('PARAM_3');
	$orientation = post_scalar('PARAM_4');
	$destination = post_scalar('PARAM_5');
	if ((bool)$destination)
		include_once(dirname(__DIR__) . "/reporting/includes/excel_report.inc");
	else
		include_once(dirname(__DIR__) . "/reporting/includes/pdf_report.inc");

	$orientation = ((bool)$orientation ? 'L' : 'P');
	$cols = array(0, 50, 210, 250, 320, 395, 465,	515);

	$headers = array(_('Reference'), _('Name'), _('Type'), _('Date'), _('Due Date'), _('Closed'), _('YTD'));

	$aligns = array('left',	'left', 'left',	'left', 'left', 'left', 'right');

    $params =   array( 	0 => $comments,
    				    1 => array('text' => _('Dimension'), 'from' => get_dimension_string((string) $fromdim), 'to' => get_dimension_string((string) $todim)));

    $rep = new FrontReport(_('Dimension Summary'), "DimensionSummary", user_pagesize(), 9, $orientation);
    if ($orientation == 'L')
    	recalculate_cols($cols);

    $rep->Font();
    $rep->Info($params, $cols, $headers, $aligns);
    $rep->NewPage();

	$res = getTransactions((string) $fromdim, (string) $todim);
	if (!($res instanceof mysqli_result))
		return;
	while ($trans=db_fetch($res))
	{
		$rep->TextCol(0, 1, $trans['reference']);
		$rep->TextCol(1, 2, $trans['name']);
		$rep->TextCol(2, 3, $trans['type_']);
		$rep->DateCol(3, 4, $trans['date_'], true);
		$rep->DateCol(4, 5, $trans['due_date'], true);
		if ((bool)$trans['closed'])
			$str = _('Yes');
		else
			$str = _('No');
		$rep->TextCol(5, 6, $str);
		if ((bool)$showbal)
		{
			$balance = getYTD((string) $trans['id']);
			$rep->AmountCol(6, 7, $balance, 0);
		}	
		$rep->NewLine(1, 2);
	}
	$rep->Line($rep->row);
    $rep->End();
}

