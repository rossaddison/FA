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
$page_security = 'SA_GLANALYTIC';
/** @var string $path_to_root */
$path_to_root="../..";

include_once(dirname(__DIR__, 2) . "/includes/session.inc");

include_once(dirname(__DIR__, 2) . "/includes/ui.inc");
include_once(dirname(__DIR__, 2) . "/includes/date_functions.inc");
include_once(dirname(__DIR__, 2) . "/admin/db/fiscalyears_db.inc");
include_once(dirname(__DIR__, 2) . "/includes/data_checks.inc");

include_once(dirname(__DIR__, 2) . "/gl/includes/gl_db.inc");

$js = "";
if ((bool) user_use_date_picker())
	$js = get_js_date_picker();

page(_($help_context = "Trial Balance"), false, false, "", $js);

$k = 0;
$pdeb = $pcre = $cdeb = $ccre = $tdeb = $tcre = $pbal = $cbal = $tbal = 0.0;

//----------------------------------------------------------------------------------------------------
// Ajax updates
//
if (get_post('Show'))
{
	ajax()->activate('balance_tbl');
}


function gl_inquiry_controls(): void
{
	$dim = get_company_pref('use_dimension');
    start_form();

    start_table(TABLESTYLE_NOBORDER);

	$date = today();
	if (!isset($_POST['TransToDate']))
		$_POST['TransToDate'] = end_month($date);
	if (!isset($_POST['TransFromDate']))
		$_POST['TransFromDate'] = add_days(end_month($date), -user_transaction_days());
	start_row();	
    date_cells(_("From:"), 'TransFromDate');
	date_cells(_("To:"), 'TransToDate');
	if ($dim >= 1)
		dimensions_list_cells(_("Dimension")." 1:", 'Dimension', null, true, " ", false, 1);
	if ($dim > 1)
		dimensions_list_cells(_("Dimension")." 2:", 'Dimension2', null, true, " ", false, 2);
	check_cells(_("No zero values"), 'NoZero', null);
	check_cells(_("Only balances"), 'Balance', null);
	check_cells(_("Group totals only"), 'GroupTotalOnly', null);
	submit_cells('Show',_("Show"),'','', 'default');
	end_row();
    end_table();
    end_form();
}

//----------------------------------------------------------------------------------------------------

function display_trial_balance(?string $type, ?string $typename): void
{
	global $path_to_root, $k, $pdeb, $pcre, $cdeb, $ccre, $tdeb, $tcre, $pbal, $cbal, $tbal;

	$printtitle = 0; //Flag for printing type name

	$k = 0;

	//Get Accounts directly under this group/type
	$accounts = get_gl_accounts(null, null, $type);

	$trans_from_date = (string) post_scalar('TransFromDate');
	$trans_to_date = (string) post_scalar('TransToDate');
	$dimension = post_scalar('Dimension');
	$dimension2 = post_scalar('Dimension2');

	$begin = get_fiscalyear_begin_for_date($trans_from_date);
	if (date1_greater_date2($begin, $trans_from_date))
		$begin = $trans_from_date;
	$begin = add_days($begin, -1);

	$Apdeb=$pdeb;
	$Apcre=$pcre;
	$Acdeb=$cdeb;
	$Accre=$ccre;
	$Atdeb=$tdeb;
	$Atcre=$tcre;
	$Apbal=$pbal;
	$Acbal=$cbal;
	$Atbal=$tbal;

	while ($account = db_fetch($accounts))
	{
		//Print Type Title if it has atleast one non-zero account
		if (!$printtitle)
		{
			if (!check_value('GroupTotalOnly'))
			{
				start_row("class='inquirybg fa-bold'");
				label_cell(_("Group")." - ".(string)$type ." - ".(string)$typename, "colspan=8");
				end_row();
			}
			$printtitle = 1;
		}

		// FA doesn't really clear the closed year, therefore the brought forward balance includes all the transactions from the past, even though the balance is null.
		// If we want to remove the balanced part for the past years, this option removes the common part from from the prev and tot figures.
		$account_code = $account["account_code"] ?? '';
		if (@sysprefs()->clear_trial_balance_opening)
		{
			$open = row_or_empty(get_balance($account_code, $dimension, $dimension2, $begin,  $begin, false, true));
			$offset = min((float)($open['debit'] ?? 0), (float)($open['credit'] ?? 0));
		} else
			$offset = 0.0;

		$prev = row_or_empty(get_balance($account_code, $dimension, $dimension2, $begin, $trans_from_date, false, false));
		$curr = row_or_empty(get_balance($account_code, $dimension, $dimension2, $trans_from_date, $trans_to_date, true, true));
		$tot = row_or_empty(get_balance($account_code, $dimension, $dimension2, $begin, $trans_to_date, false, true));
		if (check_value("NoZero") && !(bool)($prev['balance'] ?? null) && !(bool)($curr['balance'] ?? null) && !(bool)($tot['balance'] ?? null))
			continue;
		if (!check_value('GroupTotalOnly'))
		{
			alt_table_row_color($k);

			$url = "<a href='$path_to_root/gl/inquiry/gl_account_inquiry.php?TransFromDate=" . $trans_from_date . "&TransToDate=" . $trans_to_date . "&account=" . $account_code . "&Dimension=" . (string)$dimension . "&Dimension2=" . (string)$dimension2 . "'>" . $account_code . "</a>";

			label_cell($url);
			label_cell($account["account_name"]);
		}
		if (check_value('Balance'))
		{
			if (!check_value('GroupTotalOnly'))
			{
				display_debit_or_credit_cells($prev['balance'] ?? 0);
				display_debit_or_credit_cells($curr['balance'] ?? 0);
				display_debit_or_credit_cells($tot['balance'] ?? 0);
			}
		}
		else
		{
			if (!check_value('GroupTotalOnly'))
			{
				amount_cell((float)($prev['debit'] ?? 0)-$offset);
				amount_cell((float)($prev['credit'] ?? 0)-$offset);
				amount_cell((float)($curr['debit'] ?? 0));
				amount_cell((float)($curr['credit'] ?? 0));
				amount_cell((float)($tot['debit'] ?? 0)-$offset);
				amount_cell((float)($tot['credit'] ?? 0)-$offset);
			}
			$pdeb += (float)($prev['debit'] ?? 0);
			$pcre += (float)($prev['credit'] ?? 0);
			$cdeb += (float)($curr['debit'] ?? 0);
			$ccre += (float)($curr['credit'] ?? 0);
			$tdeb += (float)($tot['debit'] ?? 0);
			$tcre += (float)($tot['credit'] ?? 0);
		}
		$pbal += (float)($prev['balance'] ?? 0);
		$cbal += (float)($curr['balance'] ?? 0);
		$tbal += (float)($tot['balance'] ?? 0);
		end_row();
	}

	//Get Account groups/types under this group/type
	$result = get_account_types(false, false, $type);
	while ($accounttype=db_fetch($result))
	{
		//Print Type Title if has sub types and not previously printed
		if (!$printtitle)
		{
			start_row("class='inquirybg fa-bold'");
			label_cell(_("Group")." - ".(string)$type ." - ".(string)$typename, "colspan=8");
			end_row();
			$printtitle = 1;

		}
		display_trial_balance(is_scalar($accounttype["id"] ?? null) ? $accounttype["id"] : null, ($accounttype["name"] ?? '').' ('.(string)$typename.')');
	}

	start_row("class='inquirybg fa-bold'");
	if (!check_value('GroupTotalOnly'))
		label_cell(_("Total") ." - ".(string)$typename, "colspan=2");
	else
		label_cell(" - ".(string)$typename, "colspan=2");


	/** @var float $pdeb */
	$pdeb = $pdeb;
	/** @var float $pcre */
	$pcre = $pcre;
	/** @var float $cdeb */
	$cdeb = $cdeb;
	/** @var float $ccre */
	$ccre = $ccre;
	/** @var float $tdeb */
	$tdeb = $tdeb;
	/** @var float $tcre */
	$tcre = $tcre;
	/** @var float $pbal */
	$pbal = $pbal;
	/** @var float $cbal */
	$cbal = $cbal;
	/** @var float $tbal */
	$tbal = $tbal;
	if (!check_value('Balance'))
	{
		amount_cell($pdeb-$Apdeb );
		amount_cell($pcre-$Apcre);
		amount_cell($cdeb-$Acdeb );
		amount_cell($ccre-$Accre );
		amount_cell($tdeb-$Atdeb );
		amount_cell($tcre-$Atcre);
	}
	else
	{
		display_debit_or_credit_cells($pbal-$Apbal);
		display_debit_or_credit_cells($cbal-$Acbal );
		display_debit_or_credit_cells($tbal-$Atbal);
	}
	end_row();
}

//----------------------------------------------------------------------------------------------------

gl_inquiry_controls();

if (isset($_POST['TransFromDate']))
{
	$row = row_or_empty(get_current_fiscalyear());
	if (date1_greater_date2((string) post_scalar('TransFromDate'), sql2date($row['end'])))
	{
		display_error(_("The from date cannot be bigger than the fiscal year end."));
		set_focus('TransFromDate');
		return;
	}
}
div_start('balance_tbl');
if (!isset($_POST['Dimension']))
	$_POST['Dimension'] = 0;
if (!isset($_POST['Dimension2']))
	$_POST['Dimension2'] = 0;
start_table(TABLESTYLE);
$tableheader =  "<tr>
	<td rowspan=2 class='tableheader'>" . _("Account") . "</td>
	<td rowspan=2 class='tableheader'>" . _("Account Name") . "</td>
	<td colspan=2 class='tableheader'>" . _("Brought Forward") . "</td>
	<td colspan=2 class='tableheader'>" . _("This Period") . "</td>
	<td colspan=2 class='tableheader'>" . _("Balance") . "</td>
	</tr><tr>
	<td class='tableheader'>" . _("Debit") . "</td>
	<td class='tableheader'>" . _("Credit") . "</td>
	<td class='tableheader'>" . _("Debit") . "</td>
	<td class='tableheader'>" . _("Credit") . "</td>
	<td class='tableheader'>" . _("Debit") . "</td>
	<td class='tableheader'>" . _("Credit") . "</td>
	</tr>";

echo $tableheader;

//display_trial_balance();

$classresult = get_account_classes(false);
while ($class = db_fetch($classresult))
{
	start_row("class='inquirybg fa-bold'");
	label_cell(_("Class")." - ".(string)$class['cid'] ." - ".(string)$class['class_name'], "colspan=8");
	end_row();

	//Get Account groups/types under this group/type with no parents
	$typeresult = get_account_types(false, $class['cid'], -1);
	while ($accounttype=db_fetch($typeresult))
	{
		display_trial_balance(is_scalar($accounttype["id"] ?? null) ? (string)$accounttype["id"] : null, is_scalar($accounttype["name"] ?? null) ? (string)$accounttype["name"] : null);
	}
}

$trans_to_date_label = (string) post_scalar('TransToDate');
/** @var float $pdeb */
$pdeb = $pdeb;
/** @var float $pcre */
$pcre = $pcre;
/** @var float $cdeb */
$cdeb = $cdeb;
/** @var float $ccre */
$ccre = $ccre;
/** @var float $tdeb */
$tdeb = $tdeb;
/** @var float $tcre */
$tcre = $tcre;
/** @var float $pbal */
$pbal = $pbal;
/** @var float $cbal */
$cbal = $cbal;
/** @var float $tbal */
$tbal = $tbal;
if (!check_value('Balance'))
{
	start_row("class='inquirybg fa-bold'");
	label_cell(_("Total") ." - ".$trans_to_date_label, "colspan=2");
	amount_cell($pdeb);
	amount_cell($pcre);
	amount_cell($cdeb);
	amount_cell($ccre);
	amount_cell($tdeb);
	amount_cell($tcre);
	end_row();
}
start_row("class='inquirybg fa-bold'");
label_cell(_("Ending Balance") ." - ".$trans_to_date_label, "colspan=2");
display_debit_or_credit_cells($pbal);
display_debit_or_credit_cells($cbal);
display_debit_or_credit_cells($tbal);
end_row();

end_table(1);
if (($pbal = round2($pbal, user_price_dec())) != 0 && $_POST['Dimension'] == 0 && $_POST['Dimension2'] == 0)
	display_warning(_("The Opening Balance is not in balance, probably due to a non closed Previous Fiscalyear."));
div_end();

//----------------------------------------------------------------------------------------------------

end_page();

