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
/*
	Security response headers, off unless $security_headers is set in config.php.

	$content_security_policy holds the policy; with $csp_report_only (the default) it is sent as
	Content-Security-Policy-Report-Only, so a browser reports what the policy would block without blocking it.
	frame-ancestors is ignored in report-only mode, so X-Frame-Options covers framing.
*/

/**
 * The headers to send, in the order they are sent.
 *
 * @return list<string>
 */
function security_header_lines(): array
{
	if (!(bool)sysprefs()->security_headers) {
		return array();
	}
	$lines = array(
		'X-Content-Type-Options: nosniff',
		'Referrer-Policy: same-origin',
		'X-Frame-Options: SAMEORIGIN',
	);
	$policy = trim((string)sysprefs()->content_security_policy);
	if ($policy !== '') {
		$lines[] = ((bool)sysprefs()->csp_report_only ? 'Content-Security-Policy-Report-Only: ' : 'Content-Security-Policy: ') . $policy;
	}
	return $lines;
}

function send_security_headers(): void
{
	if (headers_sent()) {
		return;
	}
	foreach (security_header_lines() as $line) {
		header($line);
	}
}
