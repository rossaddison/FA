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
	Delegated handlers for the links and controls that used to carry inline event attributes (onclick="...") or
	javascript: URLs, which a Content-Security-Policy without 'unsafe-inline' blocks.

	The markup names an action instead of carrying code:

		<a href="#" data-fa-action="open-window">                       runs on click
		<a href="#" data-fa-action="select-combo" data-fa-args='["name","value"]'>
		<select data-fa-change="chart-update" data-fa-args='["id"]'>     runs on change

	includes/ui/ui_controls.inc fa_action_attrs() writes these attributes. Only the actions listed here can be
	asked for; the arguments are data (JSON), never code.
*/

var fa_actions = {
	// links and buttons: run(element, args) - 'follow' lets the browser's default action go on afterwards, 'defer' runs
	// the action after the click has been dispatched
	'open-window':      { run: function (el) { openWindow(el.href, el.target); } },
	'print':            { run: function () { window.print(); } },
	'back':             { run: function (el, a) { goBack(a[0]); } },
	'select-combo':     { run: function (el, a) { selectComboItem(window.opener.document, a[0], a[1]); } },
	'set-combo':        { run: function (el, a) { setComboItem(window.opener.document, a[0], a[1], a[2]); } },
	'lookup-window':    { run: function (el, a) { lookupWindow(a[0], a[1]); } },
	'call-editor':      { run: function (el, a) { callEditor(a[0]); } },
	'allocate-all':     { run: function (el) { allocate_all(el.name.substr(5)); } },
	'allocate-none':    { run: function (el) { allocate_none(el.name.substr(5)); } },
	'window-close':     { run: function (el, a) { WindowClose(a[0], a[1]); } },
	// defer: the date picker hides itself on a click outside it, and redraws itself (replacing the link that was
	// clicked) when the month changes. A javascript: URL ran after the click had been dispatched, so these run after it too.
	'date-picker':      { defer: true, run: function (el, a) { date_picker(document.getElementsByName(a[0])[0]); } },
	'date-picker-form': { defer: true, run: function (el, a) { date_picker(document.forms[0][a[0]]); } },
	'cc-month':         { defer: true, run: function (el, a) { changeCCMonth(a[0]); } },
	'cc-year':          { defer: true, run: function (el, a) { changeCCYear(a[0]); } },
	'cc-date':          { defer: true, run: function (el, a) { setCCDate(a[0], a[1], a[2]); } },
	'cc-hide':          { defer: true, run: function () { hideCC(); } },
	// inputs: the browser's own action (toggling, submitting) goes on
	'submit-update':    { follow: true, run: function (el, a) { JsHttpRequest.request('_' + a[0] + '_update', el.form); } },
	'retry':            { follow: true, run: function () { retry(); } },
	'set-fullmode':     { follow: true, run: function () { set_fullmode(); } },
	// change events
	'chart-update':     { run: function (el, a) { chart_update(el, a[0]); } }
};

function fa_run_action(el, attribute, e) {
	var name = el.getAttribute(attribute);
	var action = fa_actions[name];
	if (!action) {
		return;
	}
	var args = [];
	try {
		args = JSON.parse(el.getAttribute('data-fa-args') || '[]');
	} catch (err) {
		args = [];
	}
	if (action.defer) {
		setTimeout(function () { action.run(el, args); }, 0);
	} else {
		action.run(el, args);
	}
	if (!action.follow) {
		e.preventDefault();
	}
}

document.addEventListener('click', function (e) {
	var el = e.target && e.target.closest ? e.target.closest('[data-fa-action]') : null;
	if (el) {
		fa_run_action(el, 'data-fa-action', e);
	}
});

document.addEventListener('change', function (e) {
	var el = e.target && e.target.closest ? e.target.closest('[data-fa-change]') : null;
	if (el) {
		fa_run_action(el, 'data-fa-change', e);
	}
});
