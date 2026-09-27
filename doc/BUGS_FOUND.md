# Genuine bugs found during the Psalm cleanup

Running log of real defects — not Psalm noise — turned up while working
through the categories described in [PSALM_MIGRATION.md](PSALM_MIGRATION.md).
Most severe first within each section.

## strict_types fallout: live crashes found via real login/page testing

Unlike the rest of this document, these weren't found by Psalm — the
`declare(strict_types=1)` rollout (see PSALM_MIGRATION.md) surfaced them the
moment a real browser session hit them: login itself was completely broken,
then each fix revealed the next one in the chain. Same underlying cause
throughout — a value that was already loosely one type (a numeric string, a
`float` from `ceil()`/`round()`-family functions, a `0`/`1` sentinel used as
a stand-in for a boolean) reaching a native, strictly-typed parameter.

- **`includes/main.inc` — `random_id()`.** `$n = ceil($strength/8);` then
  passed straight to `openssl_random_pseudo_bytes()`'s `int $length`
  parameter — `ceil()` always returns `float`. This runs on effectively
  every request (session/CSRF id generation), so this alone made the whole
  app unusable. Cast to `(int)`.

- **`includes/page/footer.inc` and `themes/default/renderer.php` —
  `page_footer()` / `menu_footer()`.** Both call `yiiLayoutEnabled()`
  unconditionally, but that function only becomes defined once
  `includes/page/header.inc` has run (it does the `include_once` for
  `includes/yii/layout.inc`). The normal `page()` flow guarantees that
  ordering, but the *error handler's own* attempt to render a graceful
  error page (triggered by the `random_id()` crash above, which happens
  before `page()` ever runs) calls `end_page()` directly and hit this gap —
  masking the real error behind a second "Call to undefined function"
  crash. Guarded both call sites with `function_exists()` first, matching
  the identical guard `includes/errors.inc` already uses for this exact
  scenario.

- **`includes/db/connect_db_mysqli.inc` — `set_global_connection()`.**
  `$connection["port"]` (a string from `config_db.php`) passed straight into
  `mysqli_connect()`'s `?int $port` parameter. Cast to `(int)`.

- **`includes/ui/ui_lists.inc` — `combo_input()`.** `$search_box` and
  `$search_button` are legitimately `false` (a "no search box configured"
  sentinel) most of the time, but were passed straight to `get_post()`,
  whose `$name` parameter only accepts `string|array`. Hit on the majority
  of list/combo-heavy pages (customers, suppliers, bank accounts, GL account
  types, stock inquiries, ...). Guarded both call sites with `is_string()`.

- **`includes/dashboard.inc`.** Ten separate `round($myrow['total'])` /
  `round($myrow['costs'])` / `round($myrow['sales'])` / `round($row['Balance'])`
  -style calls across the topten/chart widgets — numeric values read back
  from `db_fetch()` come back as strings, and `round()`'s first parameter is
  `int|float`. Cast each to `(float)`.

- **`includes/current_user.inc` — `round2()`.** `$decimals` is typed
  `string|int|float|bool|null` (matching the broad `user_*_dec()` family
  that feeds it) but was passed straight to `round()`'s `int $precision`
  parameter. `round2()` is the core formatting primitive behind
  `number_format2()`, so this affected essentially every currency/quantity
  display. Cast to `(int)`.

- **`includes/date_functions.inc` — `add_days()` / `add_months()` /
  `add_years()`.** All three build a Unix timestamp via `mktime()` from
  `explode_date_to_dmy()`'s day/month/year, then do further arithmetic on
  the pieces (`$day + $days`, `($months-1)%12+1`, `$year + $years`) before
  passing them back into `mktime()` — `mktime()`'s month/day/year parameters
  are all `?int`. Fixed at the source: `explode_date_to_dmy()` now returns
  `(int)`-cast day/month/year, and the follow-on arithmetic at each call
  site is explicitly cast too.

- **`inventory/manage/items.php`.** `$_POST['fixed_asset']` was set to the
  literal integers `1`/`0` as an ad hoc boolean flag, then passed to
  `stock_categories_list_row()`'s `$fixed_asset` parameter, typed
  `string|bool|null` (no `int`). Changed the sentinels to `true`/`false`,
  which match the parameter's own declared type and every existing
  truthy-check call site.

- **`admin/attachments.php`.** `get_post('filterType')` /
  `get_post('trans_no')` passed straight to `display_rows()`, typed
  `string|null` for both — `get_post()`'s generic return type allows
  `array`. Guarded with `is_array($x) ? null : (string) $x` at the call
  site.

## Live crashes

- **`inventory/includes/inventory_db.inc` — `item_img_name()`.** Parameter
  type included `array`, but the function's first operation, `strtr()`,
  cannot accept an array — passing one would crash immediately with a
  `TypeError`. No real caller passes anything but a `stock_id` scalar (a
  plain database column value). Narrowed the parameter type to drop the
  unreachable `array` case, which also narrowed the return type — that in
  turn cleared "concatenating with possibly array" noise at every call site
  (`rep104.php`, `rep111.php`, `rep303.php`, `inventory/manage/items.php`).

- **`reporting/rep302.php` — `getPeriods()`.** Built five month-boundary
  dates via `mktime(0,0,0, date('m')-N, 1, date('Y'))` — `date('m')` and
  `date('Y')` return strings, but `mktime()`'s month/year parameters are
  typed `int|null`. Worked only through weak-typing coercion; with the
  `declare(strict_types=1)` rollout (see PSALM_MIGRATION.md) this exact
  pattern becomes a fatal `TypeError` instead of a silent coercion. Cast
  both explicitly. Also guarded `getTransactions()`'s two `db_escape()`
  calls, whose `$category`/`$location` parameters are typed to allow
  `array` (`db_escape()` doesn't accept one).

- **`includes/packages.inc` — `get_languages_list()`, `get_extensions_list()`,
  `get_themes_list()`, `get_charts_list()`.** Found via Psalm's
  `NullableReturnStatement`. All four are declared with a native,
  non-nullable `: array` return type and assign their result straight from
  `get_pkg_or_list()`, which is declared `array|null` and genuinely returns
  `null` on several realistic failure paths (the extension repository can't
  be reached, the local cache file can't be downloaded/deleted, or
  `openssl_verify` isn't available on the server). Any of those conditions
  would have thrown a fatal `TypeError` ("Return value must be of type
  array, null returned") instead of the admin's "install/manage
  languages/extensions/themes/charts" page rendering an empty or
  error-flagged list. Fixed by falling back to `?? array()` at the point
  each function reads `get_pkg_or_list()`'s result (also needed before the
  end of each function, since several go on to index into the array before
  returning it).

- **`reporting/includes/reports_classes.inc` — `add_custom_reports()`.**
  Type-hinted `add_custom_reports(array &$reports)`, but its only call site
  (`reporting/reports_main.php`, the main Reports menu page) passes a
  `BoxReports` object, not an array. Passing an object where PHP enforces a
  strict `array` parameter throws a fatal `TypeError` — this crashed the
  Reports menu on every load. Fixed the type hint to `BoxReports &$reports`.

- **`reporting/rep303.php` (Stock Check Sheet) — barcode printing.** The
  "Print Barcode on stock check sheet" company preference (a genuine,
  settable option) triggered calls to `$rep->GetY()` and
  `$rep->write1DBarcode(...)`, neither of which exist anywhere on
  `FrontReport`'s actual class chain — those methods only exist on
  `reporting/includes/tcpdf.php`, a bundled, otherwise-unused alternate PDF
  library. Any install with that preference enabled hit "call to undefined
  method". Fixed by implementing real bar rendering using
  `reporting/includes/barcodes.php`'s `TCPDFBarcode` class (a standalone bar-
  array generator) and `FrontReport::rectangle()` (the same primitive already
  used for picture placement elsewhere in the codebase).

## Data-loss-adjacent: missing audit trail / dead success link

- **`inventory/includes/db/items_trans_db.inc` — `stock_cost_update()`.**
  Found via Psalm's `NoValue` ("all possible types for this argument were
  invalidated") on both this function and its caller. `$update_no` was
  initialized to `-1` and then never reassigned anywhere in the function
  body — the line that was clearly meant to capture the new GL transaction
  id, `add_audit_trail(ST_COSTUPDATE, $update_no, $date_)`, was gated behind
  `if ($update_no != -1)`, which was therefore always false and never ran.
  Net effect: cost updates that actually changed the GL (a real
  `$value_of_change`) never got an audit trail entry, and the caller's
  "View the GL Journal Entries for this Cost Update" link never appeared
  since it also checked `$update_no > 0`. Fixed by capturing
  `write_journal_entries($cart)`'s return value into `$update_no` — which
  also made the function's own `add_audit_trail()` call redundant, since
  `write_journal_entries()` already records the audit trail for the
  transaction it creates, so that redundant call was removed rather than
  reactivated (avoids inserting a duplicate audit_trail row).

- **`manufacturing/work_order_issue.php`.** Found via Psalm's
  `DocblockTypeContradiction` on a dead `if ($failed_data != null)` branch.
  `add_work_order_issue()` (`manufacturing/includes/db/work_order_issues_db.inc`)
  is declared `: void` and never returns anything — but its caller still had
  `$failed_data = add_work_order_issue(...); if ($failed_data != null) {
  display_error(...— insufficient quantity for a component...); }`, preceded
  by a stale `// if failed, returns a stockID` comment. This validation
  path was permanently dead (`$failed_data` is always `null`). The real
  protection already happens earlier via `can_process()`'s `check_qoh()`
  pre-check, so this was cleaned up as dead/misleading code rather than
  reactivated — removed the unreachable branch and the stale comment.

## Cross-site scripting (XSS)

Found by chasing Psalm's `TaintedHtml`/`TaintedTextWithQuotes` findings to
their source.

- **`includes/page/footer.inc` — `page_footer()`.** `get_post('_focus')` (a
  raw, completely unsanitized `$_POST` read) was echoed directly inside a
  single-quoted JavaScript string literal in a `<script>` block —
  `_focus = '" . get_post('_focus') . "';` — with no escaping at all.
  `page_footer()` runs on essentially every page in the application, so any
  request carrying a crafted `_focus` POST value (e.g.
  `x'; alert(document.cookie); var y='`) could break out of the string
  literal and execute arbitrary JavaScript in the victim's session — a
  site-wide reflected XSS. Fixed by replacing the manual quoting with
  `json_encode(get_post('_focus'), JSON_HEX_TAG | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_HEX_AMP)`,
  which produces a safely-escaped JS string literal (and is also hardened
  against breaking out of the surrounding `<script>` tag). Confirmed
  `_focus` is only ever read back as a plain identifier string
  (`js/inserts.js`'s `save_focus()`/`js/utils.js`), so this is a pure
  security fix with no behavior change for legitimate values.

- **`includes/date_functions.inc` — `__date()`.** The shared low-level date
  formatter (used by `sql2date()` and effectively every date-display path in
  the app) cast `$month`/`$day` to `(int)` before formatting but never cast
  `$year` — `sql2date()` builds `$year`/`$month`/`$day` via
  `explode("-"/"/", $date_)` on its input with no validation, so an
  unvalidated `$year` fragment could carry through raw. Reachable with
  attacker-controlled input via `gl/bank_account_reconcile.php:167`'s
  `sql2date(post_scalar('bank_date'))` (no `date2sql()` round-trip first),
  making this a real reflected-XSS/output-corruption vector wherever a date
  built this way is later echoed. Fixed by adding `$year = (int)$year;`
  alongside the existing month/day casts in `__date()` — protects every
  caller project-wide, not just this one call site.

## SQL injection

Found by chasing Psalm's `TaintedSql` findings to their source. `db_escape()`
is the codebase's real SQL-escaping wrapper (it genuinely calls
`mysqli_real_escape_string()`), so every case below is a call site that skipped
it, not a flaw in the escaping function itself.

- **`includes/dashboard.inc` — `gl_week_performance()` / `gl_month_performance()`.**
  `$weeks`/`$months` were read straight from `$_POST['per_g3']` /
  `$_POST['per_g4']` and interpolated into a `LIMIT 0, $weeks` clause with no
  escaping or casting — a direct, unauthenticated-from-the-query's-own-page-
  perspective SQL injection on the dashboard. Fixed by casting both to `(int)`
  at the point of assignment (matching the functions' own declared parameter
  types). `cash_flow()`'s sibling `$_POST['per_g5']` didn't reach SQL directly
  but fed `array_fill()`/loop bounds unguarded; cast defensively too.

- **`gl/includes/db/gl_db_trans.inc` — `get_gl_balance_from_to()` /
  `get_gl_trans_from_to()`.** Both interpolated `$account` into
  `WHERE account='$account'` with no `db_escape()`, while their sibling
  `get_balance()` two functions down correctly escapes the same field —
  a clear one-off oversight. Fixed both to `db_escape($account)`.

- **`inventory/includes/db/items_db.inc`.** `$parent = $_GET['parent'];` used
  raw, unescaped, in `AND i.stock_id <> '$parent'`. Fixed with `db_escape()`.

- **`purchasing/includes/db/suppliers_db.inc` — `get_supplier_details()`.**
  `$supplier_id` (sourced from `get_post('supplier_id')` at its one call site)
  interpolated raw into `AND supp.supplier_id = $supplier_id`. Fixed with
  `db_escape()`.

- **`sales/includes/db/sales_delivery_db.inc` — `adjust_shipping_charge()`.**
  Both `$trans_no` and `$delivery->customer_id` (the latter traceable back to
  `$_POST['customer_id']` via `Cart::set_customer()`) were concatenated raw
  into a `WHERE order_ = $trans_no ... AND debtor_no = $delivery->customer_id`
  clause. Both are meant to be numeric ids; fixed with `(int)` casts.

- **`includes/db/crm_contacts_db.inc` — `update_person_contacts()`, caught and
  fixed in the same pass (see "Process note" below).** `$cat_ids` was meant to
  be escaped via `array_walk($cat_ids, 'db_escape')` before being
  `implode()`'d into `WHERE t.id=... OR t.id=...`. `array_walk()` only mutates
  its array argument when the callback takes its value by reference —
  `db_escape()` doesn't, so this call was silently a no-op and `$cat_ids` went
  into the query completely unescaped. Fixed by replacing it with
  `$cat_ids = array_map('db_escape', $cat_ids);`, which does use the
  callback's return values.

- **Defensive casts for numeric SQL fields typed `int|float|bool|null`,
  concatenated raw** (not directly exploitable — PHP's own type system blocks
  string injection through these parameters — but `bool`/`null` stringify to
  `""`/`"1"`, which produces malformed SQL): `includes/db/manufacturing_db.inc`
  (`add_bom()`, `update_bom()` — `quantity`), `sales/includes/db/sales_groups_db.inc`
  (`add_salesman()`, `update_salesman()` — `provision`/`break_pt`/`provision2`),
  `taxes/db/tax_types_db.inc` (`add_tax_type()`, `update_tax_type()` — `rate`),
  `gl/includes/db/gl_db_bank_accounts.inc` (`update_reconciled_values()` —
  `end_balance`). All fixed with `(float)` casts.

- **`admin/db/users_db.inc` — `update_user_prefs()`.** Builds
  `"$name=".db_escape($value)` from a caller-supplied array without
  validating `$name` (the SQL column name) at all. Every current call site
  only ever passes a hardcoded literal key list, so this isn't exploitable
  today, but the function itself trusts its caller completely. Hardened with
  a `preg_match('/^[a-z_][a-z0-9_]*$/i', $name)` identifier whitelist,
  skipping any key that doesn't match.

**Process note:** `db_escape()`, `clean_file_name()`, `html_specials_encode()`
(both copies — `includes/session.inc` and `install/isession.inc`), and
`date2sql()` were also missing `@psalm-taint-escape` annotations despite
being genuine, correctly-implemented sanitizers — `date2sql()` in particular
always returns either a hardcoded safe literal or
`sprintf("%04d-%02d-%02d", (int)$y, (int)$m, (int)$d)`, so it can never carry
SQL metacharacters through. Annotating these four closed off a large amount of
latent taint-tracking noise (Psalm was treating their output as still
"tainted" purely because it couldn't see inside `mysqli_real_escape_string()`/
`htmlspecialchars()`), and is what let the whole-program taint scan surface
the real bugs above one at a time as each previously-reported path was
resolved.

## Broken access control

- **`includes/dashboard.inc` — `dashboard()`.** The per-app dashboard access
  check was dead code: it guarded on `is_object($sel_app)`, which is always
  false because `$sel_app` is always a string app id (`"orders"`, `"AP"`,
  `"stock"`, ...), and the guarded branch referenced an undefined
  `$selected_app` (should have been `$sel_app`). Net effect: the access check
  documented by `admin/dashboard.php`'s own comment ("the real access level
  is inside the routines") never ran — any authenticated user could view any
  dashboard app's data regardless of role. Fixed by resolving the app object
  via `$_SESSION['App']->get_application((string) $sel_app)` and calling
  `check_application_access()` on it, matching the pattern already used
  correctly in `includes/yii/menu.inc`.

## Data-correctness bugs

- **`includes/packages.inc` — `pkg_prop()`.** The localized-value branch
  checked `isset($pkg[$property.'-'.user_language()])` but then read
  `$pkg[$pname]` — an undefined variable — instead of the key it just
  checked. Localized package/module descriptions were silently never
  retrieved. Fixed to read the actual computed key.

- **`includes/ui/ui_lists.inc` — `fixed_asset_classes_list()`.** The options
  array had `'spec_id' => '-1'` followed later by a duplicate
  `'spec_id' => ''` in the same literal; PHP keeps only the last value, so
  the special-option id was always emptied out instead of `-1`. Removed the
  duplicate.

- **`includes/ui/ui_lists.inc` — `subledger_list_row()`.** Called
  `subledger_list($name, $account, $selected_id)` but `$account` was never a
  parameter of the function — an undefined variable, silently `null` at
  runtime. Added the missing `$account` parameter, matching the sibling
  `subledger_list_cells()`.

- **`reporting/rep108.php`** — an emailed-statement subject line built with
  `sql2date($date)` where `$date` only existed in a different function's
  local scope. Fixed to use `$myrow['tran_date']`, which the query already
  selects for exactly this purpose.

- **`sales/includes/cart_class.inc` / multiple `reporting/rep*.php` (rep107,
  rep110, rep112, rep113, rep210, rep409)** — `for ($i = $from; $i <= $to; $i++)`
  loops where `$from`/`$to` came from `min()`/`max()` on `explode()` results
  and were still typed `string`, triggering PHP's alphanumeric
  string-increment semantics instead of numeric increment (wrong iteration
  count/values). Fixed by casting to `(int)` immediately after the
  `min()`/`max()` call.

- **`reporting/rep701.php` — `display_type()`.** `$prefix` and `$balance`
  were only assigned inside conditional branches but read unconditionally on
  later loop iterations, so a later row could read a value computed for an
  earlier, differently-conditioned row. Fixed with defensive initialization
  before the loop.

## Encoding / output bugs

- **`includes/ui/ui_lists.inc` — mojibake, two distinct causes.**
  (1) `nbsp()` needed `mb_convert_encoding()` to the session's actual
  encoding rather than assuming UTF-8. (2) `Yiisoft\Html\Html::encode()` /
  `encodeAttribute()` default their `$encoding` parameter to `'UTF-8'`,
  corrupting non-UTF-8 bytes (e.g. `nbsp()`'s iso-8859-1 output) into
  replacement-character bytes. Added an `output_encoding()` helper mirroring
  `nbsp()`'s own encoding detection and passed it to all 9 call sites.

- **`includes/ui/ui_lists.inc` — double-escaping.** FrontAccounting's shipped
  seed SQL (`sql/en_US-new.sql`, `sql/en_US-demo.sql`) stores some account
  names pre-escaped (literal `'Shipping &amp; Handling'`).
  `Html::encode()`'s `$doubleEncode` parameter defaults to `true`, re-escaping
  the already-escaped `&amp;` into `&amp;amp;`. Changed to `false` at all 9
  call sites (verified no behavioral change for normal, non-pre-escaped
  input).

## Would-crash-if-triggered (currently unreachable, fixed defensively)

- **`includes/types.inc` — `VC_PARTIAL`.** `includes/ui/items_cart.inc`'s
  `collect_tax_info()` compares `$this->vat_category == VC_PARTIAL`, an
  undefined constant, present since the "partial VAT" feature was first
  added. It's currently unreachable only because the company pref that
  guards it (`partial_vat_percent`) is never seeded in any shipped install —
  if an admin ever set it, this would fatal-crash with "Uninitialized
  constant". Defined the constant.

- **`includes/ui/ui_controls.inc` — `confirm_dialog()`.** Called an undefined
  `find_post()`. Zero call sites currently, so unreachable, but broken since
  the day it was added. Fixed to `get_post()`.

- **`includes/ui/ui_input.inc` — `unit_amount_cell()`.** Called an undefined
  `unit_price_format()` (never existed in this codebase's git history). Zero
  call sites currently. Fixed to the equivalent existing `price_format()`,
  matching the sibling `amount_cell()`.

- **`includes/archive.inc` — `archive` base class.** `pack()`-equivalent
  methods called `$this->create_tar()` / `$this->create_pkg()`, defined only
  on the one concrete subclass chain (`tar_file` / `package`) and never on
  `archive` itself, which is never instantiated directly. Added stub base
  implementations documented as overridden by the subclasses.

- **Missing `false`/`null` guards after DB-lookup misses** — several
  bulk-print report files (`rep107`, `rep109`, `rep110`, `rep111`, `rep113`,
  `rep209`) used the result of `get_customer_trans()` /
  `get_sales_order_header()` / `get_supp_po()` as an array unconditionally,
  even though each function's own docblock documents a `false`/`null` return
  on a lookup miss. Added `if ($myrow === false) continue;` guards.

- **`includes/references.inc` — `references::get_next()`.** Declared
  `@return string`, but both lookup paths (`$this->reflines->get($line)`,
  and `db_fetch()` on a filtered query when no `$line` is given) can
  genuinely return `false`/`null`/an empty array if the requested refline
  doesn't exist or a transaction type has no configured default refline —
  neither path was guarded, so `$refline['pattern']` would silently read as
  `null` and get returned (or fed into `_parse_next()`) instead of a real
  document-reference pattern. Currently only reachable via an incompletely
  configured install (custom transaction type added without a matching
  refline), not shipped seed data. Fixed by wrapping with `row_or_empty()`
  and returning `''` when no pattern is found, satisfying the declared
  non-nullable `string` return.

## Missing feature (implemented, not just guarded)

- **`includes/ui/attachment.inc` — no attachment size limit.** `db_insert()`
  and `db_update()` computed a local `$max_image_size` (falling back to
  5000) but never actually compared any uploaded file's size against it —
  the variable was set and then never read again. Attachment uploads had no
  size limit at all. Implemented real enforcement using
  `sysprefs()->max_image_size * 1024`, matching the pattern already used for
  image uploads in `inventory/manage/items.php` and
  `admin/company_preferences.php`.
