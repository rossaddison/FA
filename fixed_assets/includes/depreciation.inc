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

/** @psalm-pure */
function next_depreciation_date(string|int|float|bool|null $depreciation_date): int|false {

  $start = strtotime((string) $depreciation_date);
  if ($start === false) {
    return false;
  }

  $y = (int) date('Y', $start);
  $m = (int) date('n', $start) + 1;

  if ($m > 12) {
    $m = 1;
    $y++;
  }

  return strtotime("$y-$m-1");
}

/** @psalm-pure */
function depreciation_months(?string $depreciation_start): float|int {

  // assume that depreciation start is the same fiscal year
  //$start = strtotime($depreciation_start);
  //$end = strtotime($year['end']);

  $start = next_depreciation_date($depreciation_start);
  if ($start === false) {
    return 0;
  }

  return 12.0 - (float)date('n', $start) + 1.0;

  //$d1 = date('j', $start); // day of the month
  //$d2 = date('t', $start); // number of days in month

  //if ($d2 > $d1)
    //$months++;
}

/** @psalm-pure */
function months_between_dates(string|int|float|bool|null $start, string|int|float|bool|null $end): int|float {
  $start_ts = strtotime((string) $start);
  $end_ts = strtotime((string) $end);
  if ($start_ts === false || $end_ts === false) {
    return 0;
  }

  $y1 = (int) date('Y', $start_ts);
  $m1 = (int) date('n', $start_ts);

  $y2 = (int) date('Y', $end_ts);
  $m2 = (int) date('n', $end_ts);

  return 12.0 * ((float)($y2 - $y1)) + (float) $m2 - (float) $m1;
}

/**
 * @param array<array-key, mixed> $item
 * @return list<array{date: string, value: float}>
 */
function compute_gl_rows_for_depreciation(array $item, string|array|null $no_months, string|int|float|bool|null $period): array {
  $rows = array();

  $no_months_int = is_array($no_months) ? 0 : (int) $no_months;
  $depreciation_method = (string) ($item['depreciation_method'] ?? '');
  $depreciation_date = is_array($item['depreciation_date'] ?? null) ? null : (string) ($item['depreciation_date'] ?? '');
  $depreciation_start = is_array($item['depreciation_start'] ?? null) ? null : (string) ($item['depreciation_start'] ?? '');

  $year = row_or_empty(get_current_fiscalyear());
  $year_end = $year['end'] ?? '';
  $y_ts = strtotime($year_end);
  $y = $y_ts === false ? date('Y') : date('Y', $y_ts);

  $value = 0.0;
  $remaining_months = 0.0;

  switch ($depreciation_method) {
	case 'D':
		$line_value = (float)$item['purchase_cost']*(float)$item['depreciation_rate']/100.0/12.0;
		$value = (float)$item['material_cost'] * (float)$item['depreciation_rate'] * (float)$item['depreciation_factor']/100.0/12.0;
		if ($value < $line_value)
			$value = $line_value;
		break;

    case 'S':	// purchase_cost stores start cost of item
		$done_months = months_between_dates($depreciation_start, $depreciation_date);
		$remaining_months = 12.0 * 100.0/(float)$item['depreciation_rate'] - (float) $done_months;
		$value = (float)$item['purchase_cost']*(float)$item['depreciation_rate']/100.0/12.0;
		break;

	case 'N':
		$N = (float) $item['depreciation_rate'];
		$done_years = (float)months_between_dates($depreciation_start, $depreciation_date)/12.0;
		$value = (float)$item['purchase_cost']* ($N-$done_years)/($N*($N+1.0)/2.0)/12.0;
		break;

    case 'O':
		$value = (float) $item['material_cost'];
		break;
  }

  $next = next_depreciation_date($depreciation_date);
  $m = $next === false ? 1 : (int) date('n', $next);

  $total = 0.0;
  $cnt = 0;
  for ($i=$m; $i < $m + $no_months_int; $i++) {

	if ($depreciation_method == 'S') {
		if ($cnt >= $remaining_months)
			$value = 0.0;
	}

    $i_ts = strtotime("$y-$i-1");
    $date = sql2date($i_ts === false ? '' : date("$y-$i-t", $i_ts));

    $total += $value;

    if (FA_YEARLY == $period) {
      // yearly
      if ($i == (float)$m + (float)$no_months_int - 1.0)
        $rows[] = array('date' => $date, 'value' => $total);
      /*else
        $rows[] = array('date' => $date, 'value' => 0);
        */
    }
    else {
      // monthly
      $rows[] = array('date' => $date, 'value' => $value);
    }

    $cnt++;

	if ($depreciation_method == 'O') {
		// depreciate only in the first month
		$value = 0.0;
	}
 }

  return $rows;
}
