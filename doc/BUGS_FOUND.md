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

- **`admin/inst_module.php`.** `$js` was only assigned inside
  `if (sysprefs()->use_popup_windows) { $js = get_js_open_window(900, 500); }`,
  then passed unconditionally to `page()`'s 5th parameter, `string $js=""`.
  With popup windows off (a normal, legitimate preference setting, not an
  edge case), `$js` is undefined at that point - `global`/plain variable
  reads of an undefined variable become `null`, and passing `null` to a
  native `string` parameter is a `TypeError` under `strict_types=1`. Every
  visit to this page (Install/Activate extensions) with that preference off
  would crash immediately on load. Fixed by initializing `$js = "";`
  unconditionally before the conditional block, matching the parameter's
  own default.

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

- **`includes/db/connect_db_mysqli.inc` — `db_escape()`.** Missing a
  `(string)` cast before passing `$value` to `html_entity_decode()`'s native
  `string $string` parameter. `db_escape()` is called constantly with plain
  integers (e.g. `db_escape($trans_type)`, `db_escape($id)`) throughout the
  codebase, so this broke essentially every page that executes a query built
  from a non-string value — e.g. `sales_order_entry.php?NewOrder=Yes`, which
  calls `db_escape(ST_SALESORDER)` deep in `create_cart()`.

- **`purchasing/includes/ui/po_ui.inc` (×2), `includes/db/inventory_db.inc`,
  `purchasing/includes/po_class.inc`, `purchasing/includes/supp_trans_class.inc`
  — `round()` with `user_price_dec()`.** Same bug class as
  `current_user.inc`'s `round2()` above but in five more call sites:
  `user_price_dec(): int|string` passed straight to `round()`'s native
  `int $precision` parameter. Other call sites in `tax_calc.inc`,
  `sales_order_ui.inc` and `cart_class.inc` already cast with
  `(int) user_price_dec()`; these five didn't. Crashed
  `purchasing/po_entry_items.php?NewOrder=Yes` immediately on load. Cast all
  five to match the established `(int) user_price_dec()` idiom.

- **`sales/sales_order_entry.php` — include order; `sales/includes/sales_ui.inc`
  — `processing_end()`.** Not a strict_types issue — a pre-existing PHP
  session/class-loading bug, unmasked by live testing. `sales_order_entry.php`
  included `includes/session.inc` (which calls `session_start()`) *before*
  `sales/includes/cart_class.inc`; every other cart-using page in `sales/`
  (`customer_invoice.php`, `customer_delivery.php`, `credit_note_entry.php`,
  `create_recurrent_invoices.php`, `view/view_sales_order.php`) and the
  equivalent purchasing/GL pages (`po_receive_items.php`, `gl_journal.php`,
  `allocation_cart.inc` consumers) include their cart class first. Once a
  `Cart` had been serialized into `$_SESSION['Items']` by a prior request,
  `session_start()` on the *next* request unserialized it before `class Cart`
  was defined, producing a permanent `__PHP_Incomplete_Class` stand-in for
  that object (confirmed with a standalone repro — PHP does not retroactively
  fix up an already-unserialized instance once the real class loads later in
  the same request). Reading properties off it is silently fine; writing one
  is a fatal "tried to modify a property on an incomplete object." The only
  place in the whole codebase that *writes* to a cart property instead of
  reading one is `processing_end()`'s `unset(session_obj('Items')->line_items)`
  — and that call was already redundant, since the very next line,
  `unset($_SESSION['Items'])`, discards the whole object anyway. Fixed both:
  reordered the two includes in `sales_order_entry.php` to match the rest of
  the codebase, and removed the redundant property-level `unset()`. Verified
  live by hitting `sales_order_entry.php?NewOrder=Yes` twice in the same
  session (second request unserializes the cart the first request created).

- **`reporting/includes/reporting.inc` — `print_link()`.** Two `urlencode()`
  calls fed non-string values straight into its native `string $string`
  parameter: `urlencode($val)` over caller-supplied `$pars` (often an int,
  e.g. a customer or transaction id), and `urlencode($rep)` where `$rep` is
  typed `?int` — the latter would crash on *every* non-null call. `reporting/`
  is parked out of Psalm scope for now, but this is a live crash (hit via
  `sales/inquiry/sales_orders_view.php?OutstandingOnly=1`, which builds a
  report link with an int param), so fixed it independent of the Psalm sweep.
  Cast both to `(string)`.

- **`includes/session.inc` — `set_page_security()`.** `isset($trans[$value])`
  with `$value` able to be `null` (its own parameter default, and the normal
  case for pages with no matching `$_GET` key and no session-derived trans
  type yet) — PHP 8.5 deprecates using `null` as an array offset. Hit on
  `sales/inquiry/sales_orders_view.php?type=32`. Guarded with
  `$value !== null &&` before the `isset()` check, per the deprecation
  notice's own suggestion.

- **`sales/inquiry/sales_orders_view.php`.** `$_POST['order_view_mode'] =
  false;` used as an ad hoc "no filter selected" sentinel (same bug class as
  the `fixed_asset` sentinel in `inventory/manage/items.php` above), then fed
  via `get_post('order_view_mode')` into `get_sql_for_sales_orders_view()`'s
  `$filter` parameter, typed `string|array|null` — no `bool`. Hit on
  `sales_orders_view.php?type=30` (the plain "Search All Sales Orders" case,
  where none of the `$_GET['OutstandingOnly']`/`InvoiceTemplates`/etc.
  branches match). Changed the sentinel to `''`, which behaves identically in
  every existing `==` comparison against it elsewhere in the file. Checked
  the sibling `sales_deliveries_view.php`'s equivalent `$_POST['OutstandingOnly']
  = false;` — that one's target parameter (`get_sql_for_sales_deliveries_view()`'s
  `$outstanding`) is typed `string|bool|array|null`, so `bool` is valid there
  and no fix was needed.

- **`sales/includes/cart_class.inc` — `Cart::get_taxes()`.** `$shipping_cost`
  is typed `float|int|string|null` and, when `null`, defaults to
  `$this->freight_cost` (itself `float|int|string|null` — set from form
  input via `set_delivery()`), but was passed straight to
  `get_tax_for_items()`'s native `?float $shipping_cost` parameter. Hit on
  `sales/credit_note_entry.php?NewCredit=Yes`. Cast to `(float)` at the call
  site, matching the `(float)$this->freight_cost` casts already used
  elsewhere in this same class.

- **`includes/ui/allocation_cart.inc`.** Five `abs()` calls on allocation
  amounts (`$alloc_item->amount` ×3, `$cart->amount`,
  `session_obj('alloc')->amount`) — these properties are populated from
  form/DB values via methods documented as accepting `string|null`, but
  `abs()`'s native parameter is `int|float`. Hit on
  `sales/customer_payments.php` (empty-state render of the allocation table)
  and would also hit the allocate/supplier_allocate pages that share this
  include. Cast each to `(float)`.

- **`gl/includes/ui/gl_journal_ui.inc` — `gl_edit_item_controls()`.** Two
  call sites pass a value typed broader than the function's own
  `array|string|null $sub_accounts` parameter: `is_subledger_account()`
  can return the literal int `0` (passed via `$sub_type` for a new line),
  and `$item->person_id` is `string|int|float|bool|null` (passed for an
  existing line being edited). The parameter is only ever read as
  `(bool)$sub_accounts` inside the function, so both are behavior-preserving
  fixes at the call site rather than a signature change: `gl_edit_item_controls`
  collides with a different 3-parameter function of the same name in
  `gl/includes/ui/gl_bank_ui.inc` (the documented name-collision class),
  so per policy the signature itself is off-limits. Hit on
  `gl/gl_journal.php?NewJournal=Yes` for the `$sub_type` case; the
  `$item->person_id` case is the same bug on the "edit an existing journal
  line" path, not yet triggered by testing but fixed alongside it.

- **`admin/print_profiles.php` — `get_reports()`.** `closedir();` called with
  no argument — PHP 8.0 deprecated the implicit "last opened resource"
  fallback that `closedir()`/`fclose()`-family functions used to have. Hit on
  every `print_profiles.php` load. Passed the `$repdir` handle from the
  matching `opendir($path)` call explicitly.

- **`includes/remote_url.inc` — `url_get_contents()`.** Default port fell
  back to the string `'80'` while the `isset($parsedUrl['port'])` branch
  used `parse_url()`'s int port — passed straight to `fsockopen()`'s native
  `int $port` parameter. Hit on `admin/inst_lang.php` (checks a remote URL
  for available language packs). Changed the default to the int `80` and
  cast the parsed-URL branch to `(int)` for consistency.

- **`admin/inst_chart.php` — `sortByOption()`.** The `uasort()` comparator's
  own parameters were typed as plain scalars (`string|int|float|bool|null`),
  but the function body accesses `$a['name']`/`$b['name']` — it's a
  row-comparator, not a scalar one. Hit on every `inst_chart.php` load
  (`uasort($mods, 'sortByOption')` always has rows to sort). Retyped both
  parameters to `array`, matching actual usage.

- **`inventory/includes/inventory_db.inc` — `send_reorder_email()`.** The
  function's own body indexes `$loc`, `$st_num`, and `$st_reorder` as
  arrays (`$loc['location_name']`, `$st_reorder[$i]`, `$st_num[$i]`), and
  its sibling `calculate_reorder_level()` builds `$st_num`/`$st_reorder` as
  arrays via `[] =` — but `send_reorder_email()`'s own declared parameter
  types were plain scalars (`string|int|float|bool|null`) for all three.
  Any real call (this function is only ever called with the arrays
  `calculate_reorder_level()` built) would throw a `TypeError` at the call
  boundary. Retyped to `array` throughout, matching actual usage; also
  added a defensive `is_array($loc) ? $loc : array()` normalization since
  `calculate_reorder_level()` can return `false` when its own location
  lookup misses.

- **`includes/main.inc` — `clean_file_name()`.** Two separate docblocks
  were stacked back to back directly above the function; only the second
  (nearer) one is actually attached to the function for Psalm's purposes,
  and it was missing the `@psalm-pure` annotation the first docblock had —
  silently dropping purity tracking for a function that genuinely has no
  side effects (plain `str_replace`/`preg_replace`). Merged into one
  docblock. Caught via `inventory/includes/inventory_db.inc`'s
  `item_img_name()` (`@psalm-pure`) calling it and getting flagged
  `ImpureFunctionCall`.

- **`includes/dashboard.inc` — `display_title()`.** Its `$id` parameter was
  typed `int|bool|null`, but every real call site passes a `Chart::$id`
  (declared `string` in `reporting/includes/class.graphic.inc`, e.g. `'c1'`,
  `'s1'`, `'g3'`) which the function interpolates into DOM id strings like
  `"select_$id"` — never used as a number. Hit on `admin/dashboard.php?
  sel_app=AP` (and every other dashboard view with a chart). Retyped to
  `string|bool|null`, matching actual usage.

- **`includes/dashboard.inc`.** Three more `abs($row[...])` calls (GL class
  balances and GL analytic widgets) on `db_fetch()` columns, same bug class
  as the ten `round()` casts already documented above — missing `(float)`
  casts before `abs()`'s native `int|float` parameter. Hit on
  `admin/dashboard.php?sel_app=GL`.

## Live crashes

- **`includes/page/header.inc` — `help_url()`, plus three more
  functions with the identical latent bug.** Reported live by the user
  ("ucwords(): Argument #1 ($string) must be of type string, null
  given"). Earlier in this same Psalm cleanup, `isset($global_var)`
  guards on a handful of globals were replaced with
  `array_key_exists('name', $GLOBALS)` specifically to stop depending
  on psalm.xml's non-nullable stub type for those globals (which made
  the `isset()` look redundant to Psalm even though the variable could
  genuinely be unset). But in every one of these functions, a `global
  $var;` statement for that same variable ran *earlier in the same
  function body* — and PHP's `global $x;` auto-vivifies
  `$GLOBALS['x']` to `null` as a side effect of creating the
  reference binding, even when `$x` never existed in global scope
  before. So the `array_key_exists()` check that ran afterward was
  always `true`, regardless of whether the calling page had actually
  set the variable. In `help_url()`, this meant any page that never
  sets `$help_context` fell into the "has context" branch with
  `$help_page_url = null`, crashing `ucwords(null)` under PHP 8.1+'s
  deprecation-turned-error for passing `null` to a non-nullable
  internal function parameter. The identical broken pattern — found
  while fixing this one and checking every other place the same
  `array_key_exists($GLOBALS)` idiom had been used — was also present,
  unfixed and never yet live-tested, in `error_handler()`'s
  `$SysPrefs` check (would have crashed as "Call to a member function
  go_debug() on null" the first time a PHP warning fired before
  `$SysPrefs` was set), `end_flush()`'s `$Ajax` check (same, "call to
  a member function run() on null"), and `page_header()`'s
  `$db_connections` check (would have run `hook_invoke_all()` even
  pre-installation, the exact case that guard exists to skip). Fixed
  by capturing `array_key_exists()` into a plain bool *before* the
  `global` statement runs in each function, reading the global's value
  only through that captured flag afterward; `help_url()` also dropped
  its own `global $help_context;` entirely in favor of reading
  `$GLOBALS['help_context']` directly throughout. This bug class is
  invisible to Psalm itself — it's purely about PHP's `global`-binding
  side effects, not types — so any future `isset($x)` →
  `array_key_exists('x', $GLOBALS)` conversion for a Psalm finding
  needs the same check: does a `global $x;` for that variable run
  earlier in the same function?

- **`manufacturing/work_order_entry.php`.** The `UPDATE_ITEM` handler (every
  edit of an existing work order) called `can_process()` — a function this
  file never defines and never includes a definition of. Psalm's
  whole-project `allFunctionsGlobal` symbol table never flagged this as an
  undefined-function call, because `can_process()` genuinely *is* declared
  globally — independently, with different bodies, in roughly eight
  unrelated entry-point files (`admin/gl_setup.php`,
  `dimensions/dimension_entry.php`, `manufacturing/work_order_issue.php`,
  etc.). None of those files are ever included by this one, so at runtime
  this was a guaranteed "Call to undefined function can_process()" fatal
  error on every single work order save-edit, full stop. This file already
  defines its own validator for exactly this purpose,
  `canProcessWorkOrderEntry()` — used correctly two lines above by the
  `ADD_ITEM` handler, and whose own body already branches on "is this an
  edit" via `isset($selected_id)`. Fixed by calling the right one. Found by
  manually reading the file while typing `$selected_id`'s usages, not by
  Psalm — a reminder that the project-wide function-name-collision
  phenomenon (documented elsewhere in this file and in
  PSALM_MIGRATION.md) can also hide a real undefined-function bug, not
  just cause false-positive type mismatches.

- **`gl/manage/gl_account_classes.php`.** The `ADD_ITEM`/`UPDATE_ITEM`
  handler called `canProcessGlAccountClasses()`, but this file's own
  validator was declared `canPocessGlAccountClasses()` — a typo missing
  the "r", not a collision with some other file's function. Since no
  function of that exact (correctly-spelled) name exists anywhere in the
  project, Psalm's `UndefinedFunction` caught this one directly (unlike
  the `can_process()` case below, where the collision table hid an
  identical symptom). Every Add/Update submission on this page was a
  guaranteed "Call to undefined function" fatal error. Fixed the typo to
  match the call site and every sibling `canProcessXxx()` name elsewhere
  in this codebase.

- **`admin/db/shipping_db.inc` — `add_shipper()` / `update_shipper()`.**
  Both declared their `$contact` parameter as `array`, but both callers
  (`admin/shipping_companies.php`) always passed `$_POST['contact']` — a
  plain string in every normal submission. Under `strict_types=1` this is a
  fatal `TypeError` on *every* add or update, not a weak-typing edge case:
  the "Add Shipping Company" and "Update Shipping Company" actions were
  completely broken. Found while live-testing an unrelated `TaintedHtml`
  fix (confirmed by actually submitting the form, not just reading the
  code). `db_escape()` — which both functions pass `$contact` straight
  into — only accepts scalars, so `array` was never a type this function
  could have correctly handled anyway. Narrowed both to
  `string|int|float|bool|null`, matching every sibling parameter, and
  switched both call sites from raw `$_POST['contact']` to
  `post_scalar('contact')` for consistency with the other fields on the
  same form.

- **`inventory/includes/db/items_locations_db.inc` — `add_item_location()`
  / `update_item_location()`.** The exact same bug as the shipping-company
  one just above, found immediately after by checking for other `array
  $contact` parameters project-wide: both declared `$contact` as `array`,
  both callers (`inventory/manage/locations.php`) always passed
  `$_POST['contact']` raw. Every add/update of an inventory location was a
  fatal `TypeError` under `strict_types=1`. Same fix: narrowed both to
  `string|int|float|bool|null` and switched both call sites to
  `post_scalar('contact')`.

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

- **`admin/change_current_user_password.php` — `can_process()`.** Read
  `$_POST['cur_password']`/`$_POST['password']` raw and passed them straight
  into `md5()`, `strlen()` and `strstr()`, all of which require a `string`
  argument. A submission with an array-valued field (e.g.
  `password[]=x&cur_password[]=y`, trivial to send directly without a
  browser) threw a fatal `TypeError` on the very first `md5()` call under
  `strict_types=1`, instead of the expected "invalid password" error —
  confirmed by sending exactly that payload before and after the fix.
  Switched every read to `post_scalar()` with an explicit `(string)` cast,
  matching the project's established idiom for this parameter class.

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

- **`includes/ui/ui_lists.inc` — `payment_services()`.** Built its options
  list with `(object)array_combine(array_keys($payment_services),
  array_keys($payment_services))` — casting the result to an `object` —
  then passed that object straight into `array_selector()`, whose `$items`
  parameter is strictly type-hinted `array`. This file declares
  `strict_types=1`, so every call was a fatal `TypeError`, not a weak-typing
  coercion. The function's one real call site
  (`reporting/includes/reports_classes.inc`, a report parameter of the
  "payment service" field type) made this a guaranteed crash whenever that
  report field was rendered. The `(object)` cast serves no purpose here —
  `array_combine()` already returns the plain array `array_selector()`
  needs — so the fix is simply to drop it.

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

- **`includes/dashboard.inc` — `dashboard()`.** `$sel_app` — read directly
  from `$_GET['sel_app']`/`$_POST['sel_app']` in `admin/dashboard.php`, with
  no validation beyond rejecting arrays — was echoed raw inside a
  single-quoted JavaScript string literal in an inline `<script>` block
  (`xhr.send('id='+id+'&'+el.id+'='+el.value+'&sel_app=$sel_app');`). Any
  authenticated user could break out of the string literal via a crafted
  `sel_app` value (e.g. `?sel_app='-alert(document.cookie)-'`) and execute
  arbitrary JavaScript on the dashboard page. Same bug class and fix as the
  `includes/page/footer.inc` entry above; used the `json_encode_for_script()`
  helper that migration later added specifically for this pattern (JSON-encodes
  with `JSON_HEX_TAG | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_HEX_AMP`, safe for
  both the JS string literal and the surrounding `<script>` tag) rather than
  interpolating the raw value.

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

- **`includes/ui/ui_input.inc` — nearly every basic form-field primitive
  (`text_input()`, `text_cells_ex()`, `date_cells()`, `amount_cells_ex()`,
  `checkbox()`, `radio()`, `hidden()`, `button()`, `ref_cells()`).** All of
  these interpolate `$name`/`$value`/`$title`/`$_POST[$name]` straight into
  HTML attributes (`name="$name"`, `value="$value"`, `title='$title'`,
  `onclick='$submit_on_change'`) with zero escaping. Since `$value` for a
  text/date/amount field is read straight back from `$_POST[$name]` and
  re-echoed on the very next render (every validation-failure redisplay,
  every multi-step form), this is reflected XSS on essentially every form
  field in the application: submit `"><script>...</script>` into any text
  box, trigger a validation error, and it executes on redisplay.
  `ref_cells()` additionally had a raw, `@`-suppressed `$_POST[$name]` read
  wired straight into a `value='...'` attribute (the `@` only silences the
  PHP notice, not the injection). Fixed by routing every one of these
  through `html_encode()`/`html_encode_attribute()` (the two wrappers this
  migration already added around `Yiisoft\Html\Html::encode()`/
  `encodeAttribute()` specifically so Psalm's taint-escape tracking
  recognizes them — see `PSALM_MIGRATION.md`'s taint-escape section).
  `textarea_cells()` had the same gap for a `<textarea>`'s actual text
  content (not just its attributes) — the single worst instance, since
  textareas are exactly where users type free-form text (memos, notes).

- **`includes/ui/ui_input.inc` — `email_cell()`, `email_row()`,
  `email_row_ex()`, `link_row()`, `link_row_ex()`.** All five build an
  `<a href='...'>` anchor by directly concatenating an email address or a
  user-supplied URL (`$_POST[$name]`, sometimes prefixed with `http://` if
  the user didn't type a scheme) into the `href` attribute, with the same
  value (or the field's label) repeated unescaped as the link text. A
  crafted value like `' onmouseover='alert(1)` or a `javascript:`-adjacent
  payload would break out of the attribute or inject markup the moment the
  field was redisplayed. Fixed the same way — `html_encode_attribute()` on
  the `href` value, `html_encode()` on the link text. `email_row()`/
  `email_row_ex()`/`link_row()`/`link_row_ex()` build their own
  `<tr><td class='label'>` directly now instead of delegating to
  `text_row()`/`text_row_ex()`, because their label is legitimately
  pre-built HTML (the anchor) — see the note below.

  Two related findings were investigated and left as documented, confirmed
  exceptions rather than force-fixed: `label_cell()`'s own `$label` echo,
  and `amount_cells_ex()`'s `$post_label` span. Both are genuine "accept
  caller-supplied HTML, verbatim, by design" primitives — confirmed via
  grep that real call sites across the codebase rely on this (e.g.
  `gl/manage/gl_account_classes.php`, `sales/customer_invoice.php` pass
  `"<b>...</b>"`/badge markup straight into `label_cell()`;
  `inventory/manage/items.php` passes a whole `<button>` element as
  `amount_cells_ex()`'s `$post_label`). Auto-escaping either would visibly
  break rendering across many existing pages without a much larger,
  coordinated rewrite of every caller — out of scope for this pass, and
  called out here rather than silently left as unexplained Psalm noise.
  Every caller of `label_cell()`/`text_row()`/`text_row_ex()` identified in
  this pass that constructs its own HTML internally (the email/link/ref
  functions above) now pre-escapes its own dynamic content before handing
  it to these passthrough primitives, so the real injection points are
  closed even though the primitives themselves stay deliberately
  unescaping.

- **`includes/ui/ui_msgs.inc` — `display_heading()`, `display_heading2()`,
  `display_note()`.** Same genuine "accept caller-supplied HTML, verbatim,
  by design" category as `label_cell()`/`amount_cells_ex()` above, not
  force-fixed for the same reason: confirmed via grep that real call sites
  across the codebase rely on it, pervasively — `display_note()` in
  particular is routinely called with a pre-built `<a href='...'>` anchor
  from `get_trans_view_str()`/`get_gl_view_str()`/`print_link()`
  (`manufacturing/work_order_*.php` and many others), and
  `admin/print_profiles.php`/various `class='...'` style attributes are
  passed through `display_note()`'s own `$extra` parameter the same way.
  Auto-escaping would break every one of these. Left deliberately
  unescaping; callers that build these strings from genuinely
  attacker-influenced data are the real place to fix this, not the shared
  passthrough primitive — none were found reachable from unauthenticated
  or low-privilege input in the sampling done here, but this file's own
  3 functions were not individually audited caller-by-caller the way
  `ui_input.inc`'s were above, so treat this as a documented, deliberate
  design choice rather than an exhaustive clearance.

- **`includes/page/header.inc` — `page_header()`.** `<title>$title</title>`
  interpolated the page title raw with no escaping. `$title` traces back to
  `$_SESSION['page_title']` in several files, at least one of which builds
  it by concatenating a raw `$_GET` value with no escaping
  (`purchasing/po_entry_items.php`:
  `_($help_context = "Modify Purchase Order #") . $_GET['ModifyOrderNumber']`)
  — a crafted `ModifyOrderNumber` could break out of the `<title>` tag and
  inject arbitrary markup, and `page_header()` runs on nearly every page in
  the application. Same bug class as the other entries in this section.
  Fixed with `html_encode()`.

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

- **`purchasing/includes/db/supp_trans_db.inc` — `get_supp_payment_before()`.**
  Interpolated `$supplier_id` directly into
  `supp_trans.supplier_id='$supplier_id'` with no `db_escape()` and no
  surrounding type guard. Traced the call chain
  (`purchasing/supplier_payment.php`'s `get_default_supplier_payment_bank_account()`
  → `get_supp_payment_before()`) back to `$_GET['supplier_id']`/
  `$_POST['supplier_id']`, read raw with no `post_scalar()`/`get_scalar()`
  guard - a real, unauthenticated-from-the-request's-perspective SQL
  injection on the supplier payment page. Not caught by Psalm's `TaintedSql`
  tracking here (a type-only `PossiblyNullOperand`/`InvalidOperand` finding
  on the same line is what led to it instead) - likely lost across the
  `get_default_supplier_payment_bank_account()` parameter boundary, a taint-
  tracking gap rather than a false negative on this function in isolation.
  Fixed by switching to `db_escape($supplier_id)` (removing the function's
  own manual quoting, since `db_escape()` already quotes string values).

- **`inventory/includes/db/items_db.inc`.** `$parent = $_GET['parent'];` used
  raw, unescaped, in `AND i.stock_id <> '$parent'`. Fixed with `db_escape()`.

- **`purchasing/includes/db/suppliers_db.inc` — `get_supplier_details()`.**
  `$supplier_id` (sourced from `get_post('supplier_id')` at its one call site)
  interpolated raw into `AND supp.supplier_id = $supplier_id`. Fixed with
  `db_escape()`.

- **`purchasing/includes/db/grn_db.inc` — `get_grn_items()`.** The scalar
  branch of its `$invoice_no` handling interpolated the value raw into
  `invoice_item.supp_trans_no = $invoice_no` with no `db_escape()` at all
  (the sibling array branch at least passed its elements through
  `implode()`, just without casting them, fixed alongside this as a
  separate `MixedArgumentTypeCoercion` finding). Traced call sites all
  currently pass an internal transaction id (`$supp_trans->trans_no`) or
  a hardcoded `0`, so not confirmed exploitable from request input today,
  but hardened with `db_escape()` on the scalar path and
  `array_map('intval', ...)` on the array path regardless, since this is
  exactly the unescaped-interpolation pattern this section documents
  elsewhere.

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

## Path traversal / arbitrary file write

Found by chasing Psalm's `TaintedFile` findings in `admin/db/maintenance_db.inc`
to their source.

- **`write_extensions()`.** Its `$company` parameter is concatenated directly
  into a filesystem path (`$path_to_root.'/company/'.$company.'/installed_extensions.php'`)
  with no cast or validation, and is meant to always be a plain integer index
  into `$db_connections`. Traced its one unguarded caller,
  `admin/inst_module.php`'s `write_extensions($exts, get_post('extset'))`, back
  to raw, unauthenticated-from-the-request's-perspective POST data with no
  `(int)` cast anywhere in between - an authenticated user with access to the
  extensions-install page could pass a path-traversal payload as `extset` and
  make this function write arbitrary PHP (the `var_export()`'d extensions
  list, built into `$msg` a few lines above) to any writable path on disk, a
  real privilege-escalation/RCE route for a user who only has
  install/activate-extensions access, not full filesystem access. Fixed by
  casting to `(int)` at the point of use inside `write_extensions()` itself
  (the single choke point for all 5 call sites), rather than patching each
  caller individually. `create_comp_dirs()`/`company_path()` were also
  investigated for the identical pattern (Psalm flagged the same `TaintedFile`
  category on their `mkdir()`/`fopen()` calls) but their one real caller,
  `admin/create_coy.php`, already explicitly casts `$selected_id` to `(int)`
  right at the top of the file before it reaches either function (with a
  comment already documenting exactly this concern) - confirmed safe, no
  change needed.

- **`frontaccounting.php` — `front_accounting::display()`'s dynamic theme
  include.** `include_once($path_to_root . "/themes/".user_theme()."/renderer.php")`
  is flagged `TaintedInclude` because `user_theme()` ultimately traces back
  to `post_scalar('theme')` in `admin/display_prefs.php` - the same
  provenance already investigated for the analogous `TaintedHtml` findings
  on `user_theme()`-into-`<img src>` sites elsewhere in this codebase (see
  TAINTEDHTML_VENDOR_CLASS_LIMITATION.md). Checked the write path again
  specifically for path-traversal, not just HTML injection:
  `admin/display_prefs.php` runs the posted value through
  `clean_file_name()` (`@psalm-taint-escape file`-annotated, replaces
  anything outside `[a-zA-Z0-9.\-_]` with `_`) *before* it's ever persisted
  to the user's stored preference, which structurally rules out `/` or `..`
  reaching this include regardless of whether the real UI only offers a
  fixed dropdown. Psalm can't see across the preference's DB/session
  round-trip, the same documented gap this migration already decided not
  to blanket-suppress for `TaintedFile`/`TaintedSSRF` (see
  PSALM_MIGRATION.md's "via a database round-trip" section) - so this is
  spot-checked and confirmed safe here specifically, not silenced as a
  category. `@psalm-suppress TaintedInclude` does not actually suppress
  this specific finding here (tried both statement- and method-level
  placement, confirmed via rescan neither drops it from the report, unlike
  the identical-pattern `@psalm-suppress TaintedFile` annotations in
  `admin/db/maintenance_db.inc` which do work) - see
  PSALM_UPSTREAM_LIMITATIONS.md. Left as a plain comment pointing back to
  this entry instead; the finding itself remains in the Psalm count.

## Server-side request forgery (SSRF) / unsanitized outbound requests

Found by chasing Psalm's `TaintedSSRF`/`TaintedFile` findings in
`gl/includes/db/gl_db_rates.inc` to their source.

- **`get_extern_rate()`.** Its YAHOO/GOOGLE/BLOOMBERG/EXCHANGE-RATES.ORG
  branches splice `$curr_b` (and the company's default currency,
  `$curr_a`) straight into an outbound request URL/path with no
  validation, then hand it to `file_get_contents()`/`curl`. `$curr_b`
  traces back to `gl/manage/exchange_rates.php`'s
  `retrieve_exrate(post_scalar('curr_abrev'), ...)` - `curr_abrev` is
  read raw from `$_POST` with no check against the real currency list
  before reaching this code, even though `0_currencies.curr_abrev` is a
  `char(3)` column (letters only). An authenticated user with
  `SA_EXCHANGERATE` access could inject arbitrary path/query segments
  into the request sent to `download.finance.yahoo.com`,
  `www.google.com`, `www.bloomberg.com` or `exchange-rates.org`. Fixed
  by adding `sanitize_currency_code()`
  (`@psalm-taint-escape ssrf`/`@psalm-taint-escape file`-annotated,
  matching the existing `clean_file_name()` pattern documented above
  under "Path traversal / arbitrary file write"), which strips anything
  but letters before `curr_b`/`curr_a` are used to build any of these
  URLs - the single choke point for every provider branch, rather than
  patching each one individually. (First attempt quoted the tag as
  `@psalm-taint-escape 'ssrf'`; Psalm's own parser takes the tag's
  argument verbatim with no quote-stripping, so the quoted form
  silently failed to match the `ssrf`/`file` taint-kind names until
  the quotes came off - not an upstream bug, just the tag's actual,
  unquoted syntax.)

- **`admin/inst_module.php` — `local_extension()` / `handle_delete_ext()`.**
  Investigated, not fixed — a dynamic class name
  (`'hooks_'.$id`/`'hooks_'.$ext['package']`, `$id` ultimately from
  `find_submit('Local', ...)`, i.e. a submit-button-encoded `$_POST`
  value) is instantiated with `new $hooks_class` and its
  `install_extension()`/`uninstall_extension()` method called — Psalm's
  `TaintedCallable` flags this correctly as attacker-influenced class
  instantiation. Confirmed bounded rather than exploitable as a generic
  object-injection/RCE primitive: `local_extension()`'s gate,
  `class_exists($hooks_class, false)`, explicitly disables autoloading,
  so it can only match a class the preceding
  `include_once($local_hook_file)` just defined from
  `modules/<clean_file_name($id)>/hooks.php` — a path already sanitized
  through `clean_file_name()` before touching the filesystem. An
  attacker would need to already be able to drop a file at that
  sanitized path, at which point they have a stronger primitive
  (arbitrary PHP file write) than this class-instantiation step could
  ever grant on its own. `handle_delete_ext()`'s equivalent gate,
  `class_exists($hooks_class)` (default `true` for autoloading), is
  slightly looser in principle, but `$ext['package']` there comes from
  `get_company_extensions()` (DB/config-stored, not raw request data),
  not from the current request directly. This requires `SA_CREATEMODULES`
  admin-level access either way. Left as-is — tightening the second
  gate to match the first (`class_exists($hooks_class, false)`) would be
  a reasonable future hardening, but wasn't made here since it's a
  behavior change beyond this sweep's scope, not a type fix.

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

- **`purchasing/includes/purchasing_db.inc` — `add_direct_supp_trans()`.**
  Found via Psalm's `TooManyArguments` on the call to
  `supp_trans::add_grn_to_trans()`, which takes 11 parameters
  (9 required + `$std_cost_unit`/`$gl_code` optional) but was being passed
  12 positional arguments — a stray `true` landed in the `$std_cost_unit`
  slot, pushing the real `get_unit_cost($line->stock_id)` call into
  `$gl_code` instead, and leaving a trailing `''` with nothing left to
  bind to (PHP silently drops extra positional arguments rather than
  erroring). Confirmed inert rather than exploitable: neither
  `GrnItem::$std_cost_unit` nor `::$gl_code` on the in-memory object built
  by this specific call is ever read back anywhere in the direct-invoice
  write path (`add_supp_invoice()`'s own `grn_items` loop in
  `purchasing/includes/db/invoice_db.inc` only touches
  `item_code`/`this_quantity_inv`/`chg_price`/`id`/`po_detail_item` on
  each entry) — the persisted `std_cost_unit` DB column for a GRN comes
  from a completely separate code path in `purchasing/includes/db/grn_db.inc`.
  Removed the stray argument so `$std_cost_unit` actually receives the
  unit cost it was clearly meant to (cast from `get_unit_cost()`'s
  `false|null|string` return via `is_numeric($unit_cost) ? (float)
  $unit_cost : null`) and `$gl_code` gets its intended `''` default —
  correct now even though currently still unread for this flow, in case
  a future change starts consuming either field.

- **`includes/db/connect_db_mysqli.inc` — `db_escape()`.** Caught live, not
  by Psalm: a real "Incorrect integer value: '' for column 'tax_included'"
  MySQL error reported while testing `purchasing/po_entry_items.php`'s
  Direct Purchase Invoice Entry flow. `db_escape()`'s very first line does
  `(string) $value` on its whole, widely-typed `string|int|float|bool|null
  $value` parameter before anything else - and in PHP, `(string) false` is
  `""` while `(string) true` is `"1"`. The function's next step treats any
  value that is now an empty string as "null/unset" and emits the SQL
  literal `''` for it (or `NULL` if `$nullify` is set) - so a genuine
  boolean `false` passed in for a tinyint/boolean column (here,
  `supp_trans::tax_included`, ultimately sourced from a `purch_order`
  property a nearby setter assigns directly from a `bool` parameter) was
  indistinguishable from "nothing was passed" and got written as `''`
  instead of `'0'`. `true` happened to survive by the same accident that
  broke `false` - `(string) true` is `"1"`, a non-empty string, so it took
  the normal quoted-string path and produced `'1'`, which MySQL silently
  coerces into the correct integer for a numeric column. Only the `false`
  case was ever broken, which is exactly why this had gone unnoticed:
  every `true` write looked completely fine. On a server running without
  strict SQL mode (this codebase's own `set_global_connection()` explicitly
  sets `STRICT_ALL_TABLES` for every connection, but a differently-configured
  server wouldn't), MySQL would have silently coerced `''` into `0` for an
  int column instead of rejecting it outright - meaning this is not just a
  crash risk here, but a believable silent-data-corruption vector for any
  other `db_escape(false)` call site across the whole codebase that writes
  to a numeric column, on any install not already running strict mode.
  Fixed by converting `bool` to `'1'`/`'0'` explicitly before the string
  cast runs, so `false` round-trips exactly like `true` already did.

- **`includes/ui/attachment.inc` — `attachments::db_update()`.** The entire
  `UPDATE` statement - and its execution - was nested inside the
  "a new file was uploaded and passed validation" branch of the function,
  rather than running unconditionally with the file columns only
  conditionally appended (which is what the surrounding code's own
  `if ($upload_file == 'Yes') $sql .= ...` clearly intended). Net effect:
  editing an existing attachment's description or date *without* also
  re-uploading a file ran no SQL at all - no error, no notification, the
  edit screen just silently discarded the change. Found by contrasting
  this function's structure with its sibling `db_insert()`, where the same
  nesting is actually correct (a brand new attachment genuinely requires a
  file, with its own explicit `else display_error("No Attachment File
  Selected")` for the missing-file case) - `db_update()` had no such
  justification and no fallback branch at all for "no new file" instead of
  silently no-op'ing. Fixed by hoisting the `UPDATE`/`db_query()` out to run
  whenever the optional file (if any) is absent or valid, tracked via a new
  `$run_update` flag that only trips `false` on an actual file-validation
  failure. While restructuring, also fixed a related latent bug the new
  structure made easy to see: `$actual_file_nam`/`$filesize`/`$filetype`
  were only ever assigned inside the nested `if ($unique_name) { ... }`
  check, but read unconditionally whenever `$upload_file == 'Yes'` - if a
  valid file was uploaded but `_get_unique_name()` came back empty (no
  existing attachment row, or one with an empty `unique_name` column), the
  query would reference undefined variables. Initialized all three to
  `null` up front.

- **`admin/inst_module.php` — `display_extensions()`.** The "Installed"
  column's fallback-to-"Unknown" check was
  `$installed && ($installed != '-' || $installed != '')` — for any single
  string value, at least one of `!= '-'` / `!= ''` is always true (a value
  can't equal both at once), so the inner condition was a tautology and the
  intended exclusion of the `'-'` placeholder (used elsewhere in this same
  file as the "not actually installed" sentinel) never took effect. Fixed
  `||` to `&&`.

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

- **`gl/manage/gl_account_types.php`** — when editing an account group
  whose parent is the sentinel `-1` (no parent), the code meant to clear
  `$_POST['parent']` for display (`if ($_POST['parent'] == '-1')
  $_POST['parent'] == "";`) but used `==` instead of `=` - a no-op
  comparison whose result was discarded. The "Subgroup Of" dropdown kept
  showing `-1` as the selected parent instead of "None" whenever editing a
  top-level account group. Fixed to `=`.

- **`inventory/manage/item_units.php`** — the hidden `selected_id` field was
  set from `$myrow["abbr"]` instead of `$selected_id`, but `$myrow` is only
  freshly assigned inside the `if ($Mode == 'Edit')` block a few lines up.
  Reaching this code with `$selected_id != ''` in any mode other than
  `'Edit'` would read the stale `$myrow` left over from the page's earlier
  listing loop (`while ($myrow = db_fetch($result))`) instead - submitting
  the last-listed unit's abbreviation rather than the actually-selected
  one. `$selected_id` already holds the correct value (it *is* the unit's
  abbreviation, used identically by `item_unit_used()`/`get_item_unit()`/
  `delete_item_unit()` elsewhere on the same page) and is what `$myrow`
  equals in the one case this accidentally worked (`Mode == 'Edit'`) -
  fixed by using `$selected_id` directly.

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

- **`sales/includes/db/custalloc_db.inc` — `get_cust_allocation()`.** Called
  `db_fetch(db_select($sql), "Cannot retrieve customer allocation $trans_id")`
  — the error-message argument belongs to `db_select()` (its own second
  parameter), but was attached to `db_fetch()` instead, which only takes one
  argument (PHP silently ignores the extra one). If `db_select()` ever failed
  here, it would die with no custom context instead of the intended message.
  Psalm's `TooManyArguments` on the `db_fetch()` call caught it. Fixed by
  moving the message to `db_select()`'s own argument list, matching every
  other call site in this file (e.g. `delete_cust_allocation()` just above
  it).

- **`sales/includes/db/sales_invoice_db.inc` — `write_sales_invoice()`.**
  Called `refs()->save($type, $id, $reference, $line, $invoice->fixed_asset)`
  — `references::save()` only takes 4 parameters, so the 5th argument was
  silently dropped by PHP on every new invoice (fixed_asset reference data
  was never actually passed to `save()`, which doesn't use it anyway — it
  was dead from the signature's side). Psalm's `TooManyArguments` caught it.
  A near-identical call in `sales/includes/db/sales_order_db.inc` had already
  been fixed the same way in a prior session, with the broken call left
  commented out directly above the fix (dated 29/9/26) as a breadcrumb — this
  file's identical copy was missed at the time. Fixed by dropping the extra
  argument, matching the existing precedent.

- **`sales/customer_invoice.php` — prepayment-allocation validation bypass.**
  The `AllocationNumber` branch builds `$payments = array(get_cust_allocation(...))`
  — always a 1-element array, even when `get_cust_allocation()` itself returns
  `false` (no matching allocation) — then guarded the error path with
  `if (!$payments || ...)`. `!$payments` tests the *outer* array, which is
  always truthy (it always has exactly one element), so the "please select a
  correct prepayment" error could never fire no matter what
  `get_cust_allocation()` returned; a bad/missing `AllocationNumber` fell
  through to read `$payments[0]['trans_no_to']` off a `false` value instead.
  Psalm's `RiskyTruthyFalsyComparison` on the always-truthy array caught it.
  Fixed by checking `$payments[0]` (the actual allocation row) instead of the
  wrapping array.

- **`admin/db/transactions_db.inc` — `get_sql_for_view_transactions()` checked
  the wrong table name.** An exclusion step meant to hide stale, voided
  journal entries from `admin/void_transaction.php`'s and
  `admin/view_print_transaction.php`'s transaction search compared
  `$table_name` against `TB_PREF.'gl_trans'` — a value
  `get_systype_db_info()`'s exhaustive `switch` never actually returns (for
  `ST_JOURNAL`/`ST_COSTUPDATE` it returns `TB_PREF.'journal'`, a different
  real table), so the condition could never be true for any transaction
  type. Psalm's `DocblockTypeContradiction` on a docblock a few lines below
  caught it. `void_journal_trans()` zeroes `journal.amount` on void/edit —
  exactly the stale row the original comment describes. Fixed by comparing
  against `TB_PREF.'journal'` instead; verified live against the real
  database (see [TRANSACTION_VIEW_FILTER_BUG.md](TRANSACTION_VIEW_FILTER_BUG.md)
  for the full trace, including why the fix holds across per-company table
  prefixes).

- **`includes/db_pager.inc` — `db_pager::_sql_gen()`'s record-count query
  ignored `set_where()`'s filter entirely.** The total-row-count query
  (used to compute `max_page` for pagination) was built as
  `"SELECT COUNT(*) FROM ($this->sql) tmp_count"` - wrapping the pager's
  *original, unparsed* input SQL - while the actual per-page data query a
  few lines later was built from `$select`/`$from`/`$where`/`$group`,
  which *do* include any `set_where()`-added filter merged in just above
  both branches. Any pager with an additional UI filter applied would
  compute its page count against the *unfiltered* row count while only
  ever displaying the *filtered* rows - wrong page counts, and genuine
  matching rows past the (wrongly larger) last page silently inaccessible.
  A second, independent bug in the same function compounded this for any
  pager constructed with the array form of `$sql`
  (`array('select' => ..., 'from' => ..., ...)` rather than a plain SQL
  string): `$this->sql` is never reconstructed into a string for that
  form, so interpolating it directly into the count query would silently
  produce `SELECT COUNT(*) FROM (Array) tmp_count` - invalid SQL. Both
  bugs were latent rather than live: `set_where()` itself turned out to
  have zero real call sites anywhere in the codebase (see the next entry),
  so no pager in production actually exercises either path today. Fixed
  by building the count query's inner subquery from the same parsed
  `$select`/`$from`/`$where`/`$group` the data query already uses, instead
  of re-using `$this->sql`.

- **`includes/db_pager.inc` — `db_pager::set_where()`.** Assigned its
  `$where` parameter straight to `$this->extra_where` (declared
  `array<int, string>`) without first checking it had gone through the
  function's own array-wrapping logic - that logic only runs inside
  `if ($where) { ... }`, so calling `set_where()` with no arguments (its
  own documented default) or an empty/falsy value assigned `null`, `''`,
  or `'0'` directly to a property every other method in the class
  (`_sql_gen()`'s `count($this->extra_where)`, for one) assumes is always
  an array. `count(null)` is a fatal `TypeError` on PHP 8+. Zero real call
  sites exist for this method anywhere in the codebase today, so this
  specific crash has never actually fired - fixed defensively (an `else`
  branch defaulting to `array()`) since the method is clearly meant to be
  usable.

- **`gl/includes/db/gl_db_banking.inc` — `write_bank_transaction()`.**
  After the `foreach ($items->gl_items as $gl_item)` loop that posts each
  line item's own GL entry, the function posts one final *balancing* GL
  entry against the bank's own account - but built its dimension
  arguments from `$gl_item->dimension_id`/`$gl_item->dimension2_id`,
  reusing whatever object the `foreach` loop variable happened to be left
  pointing at (PHP doesn't scope `foreach` variables to the loop body) -
  silently tagging every bank payment's and deposit's balancing entry
  with the *last* line item's dimension instead of no dimension at all
  (this bank-balance posting isn't tied to any one line item, the same
  way the sibling `add_gl_balance()` call elsewhere in this file for bank
  transfers takes no dimension arguments whatsoever). This runs on every
  real bank payment/deposit with at least one GL line - not an edge case.
  A cart with zero GL line items (if ever reachable) would instead read
  `$gl_item` completely undefined. Fixed by passing `null, null` for this
  posting's dimensions, matching the no-dimension convention used for the
  equivalent balancing entry elsewhere in this file.

- **`date2sql()` empty-string chain — live crash on Direct Purchase Invoice
  Entry with a non-company currency.** `includes/date_functions.inc`'s
  `date2sql(?string $date_): string` intentionally returns `""` for a
  null/empty input — that's documented, correct behavior — but roughly two
  dozen call sites across `admin/db/`, `dimensions/includes/`,
  `gl/includes/db/`, `includes/db/`, `inventory/includes/db/`,
  `manufacturing/includes/db/`, `purchasing/includes/db/` and
  `sales/includes/db/` embedded the result directly into a SQL
  INSERT/UPDATE value position (or a WHERE-clause date comparison — see
  below) without checking for the empty case first, so any call with a
  genuinely missing date threw a fatal "Incorrect DATE value: ''" under
  MySQL's `STRICT_ALL_TABLES` mode (`set_global_connection()` enables this
  for every connection in this codebase). Hit live, repeatedly, while
  testing `purchasing/po_entry_items.php?NewInvoice=Yes` (Direct Purchase
  Invoice Entry) for a supplier in a non-company currency — each fix
  revealed the next function in the call chain
  (`add_po` → `add_grn` → `add_new_exchange_rate`/`add_exchange_rate` →
  `add_gl_trans_supplier` → `add_gl_trans` → `add_supp_invoice` →
  `add_trans_tax_details`, plus the unrelated-looking sibling functions in
  each of those files). Fixed by adding the standard guard immediately
  after every `date2sql()` call found this way:
  `if ($date == "") $date = "0000-00-00";`.

  **The specific bug that was actually blocking this test case, and the
  reason an earlier assumption had to be corrected:** both a prior
  investigation pass and a delegated search initially dismissed every
  WHERE-clause usage of `date2sql()`'s result as "safe", on the theory that
  MySQL's strict mode only rejects malformed values in an INSERT/UPDATE
  value position, not a comparison. That assumption is wrong.
  `gl/includes/db/gl_db_rates.inc`'s `get_last_exchange_rate()` has
  `WHERE ... AND date_ <= '$date'`, and with `$date === ''` this throws the
  identical 1525 error — confirmed by temporarily wrapping
  `mysqli_query()` in `includes/db/connect_db_mysqli.inc`'s `db_query()` in
  a try/catch to log the exact failing SQL (the error otherwise propagates
  as a `mysqli_sql_exception` before any post-call `mysqli_errno()` check
  can see it, since PHP 8.1+'s default `mysqli_report()` mode throws
  directly on query failure). MySQL still has to cast a string literal to
  `DATE` to evaluate a comparison, and that cast fails under strict mode
  exactly like an INSERT would. Any `date2sql()` result used in a
  WHERE-clause comparison anywhere in this codebase should be treated as
  equally at risk as one used in an INSERT/UPDATE, not exempted.

- **`gl/includes/db/gl_db_trans.inc` — six more unguarded WHERE-clause
  occurrences, found during Psalm cleanup rather than live testing.**
  `get_gl_transactions()`, `get_balance()`, `delete_gl_budget_trans()`,
  `get_only_budget_trans_from_to()`, `get_tax_summary()`, and
  `get_sql_for_journal_inquiry()` all built a `tran_date >= '$x' AND
  tran_date <= '$y'`-shaped WHERE clause directly from `date2sql()`'s
  result, unconditionally, with no `if ($x == "") $x = "0000-00-00";`
  fallback and no surrounding `if ($original_param != "")` guard either
  — unlike several sibling functions in the same file
  (`get_gl_balance_from_to()`, `get_gl_trans_from_to()`,
  `get_budget_trans_from_to()`) which already guard correctly by
  checking the *original* un-converted parameter before appending the
  clause at all. Any of these six reached with a genuinely empty/null
  date (e.g. an inquiry page's optional date-range filter left blank)
  would hit the identical "Incorrect DATE value: ''" crash documented
  above. Fixed with the same `"0000-00-00"` fallback used throughout
  this file's already-correct functions.

- **`admin/print_profiles.php` — the delete-profile handler.** The
  "is this profile still in use?" safety check,
  `check_delete(get_post('name'))`, checked the wrong field.
  `'name'` is the free-text input only rendered when *creating* a new
  profile (`profile_id == ''`); when deleting an *existing* profile
  that input isn't even in the form, so `get_post('name')` always came
  back `''`, and `check_delete()` returns `0` (short-circuits as
  "safe") for an empty name. Net effect: the
  `key_in_foreign_table($name, 'users', 'print_profile')` check never
  actually ran for a real deletion — any print profile could be
  deleted even while still assigned to a user via
  `users.print_profile`, leaving that column dangling. Found while
  typing the selected profile's id through to `delete_printer_profile()`
  for an unrelated Psalm `MixedArgument` fix and noticing `check_delete()`
  was called with a completely different field one line above it.
  Fixed by checking the actual profile being deleted (the page's
  `profile_id` selection) instead of the unrelated name field.

## Encoding / output bugs

- **`includes/page/header.inc` — `page_header()`.** The IE11-compatibility
  check compared `strpos()`'s result against `-1` instead of `false` --
  `strpos($ua, $needle) != -1` is true for every possible result
  (`strpos()` returns `false` or a 0-based position on a match, never
  `-1`), so the `<meta http-equiv='X-UA-Compatible' content='IE=10'>` tag
  was being echoed on every single page load for every browser, not just
  the IE11 user agent the comment next to it says it's meant for. Found
  while routing the same line's `$_SERVER['HTTP_USER_AGENT']` read through
  a `PossiblyUndefinedArrayOffset`/`PossiblyNullArgument` Psalm fix and
  noticing the comparison itself couldn't ever be false. Fixed to
  `!== false`.

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

- **`includes/hooks.inc` — `hook_invoke_all()` lost its `isset($hooks)`
  guard during the Psalm pass on this file.** The original code wrapped its
  `foreach($Hooks as ...)` in `if (isset($Hooks)) { ... }`, because `$hooks`
  is only ever populated by `install_hooks()`, which `includes/session.inc`
  calls inside `if (!defined('FA_LOGOUT_PHP_FILE')){ ... }` — on
  `access/logout.php` (which defines that constant before including
  `session.inc`) `$hooks` is never set. Typing the function's parameters
  during the pass silently dropped the guard, so `foreach($hooks as ...)`
  ran on `null` and crashed every visit to `access/logout.php` with
  "foreach() argument must be of type array|object, null given". Caught by
  live-testing after the pass, not by Psalm (which only knows `$hooks` via
  its non-nullable `psalm.xml` global stub, so it couldn't see the gap).
  Restored the guard. `hook_invoke_first()`/`hook_invoke_last()` have the
  same unguarded `foreach($hooks ...)`/`array_reverse($hooks)` in the
  original code (never hit yet via this call path) — hardened both with
  `$hooks ?? array()` while fixing the regression above.

- **`includes/page/header.inc` — `help_url()`.** `$_SESSION['App']->applications[$app]->help_context`
  assumed `$_SESSION['sel_app']` always indexes a currently-registered
  application, with no fallback — unlike the near-identical lookup a little
  further down in the same file (`page()`, setting `$_SESSION["App"]->selected_application`),
  which already falls back to `'orders'` when the stored id points at a
  removed/inactivated extension's tab. Hit live after a stale `sel_app`
  session value stopped matching any key in `applications`, producing
  "Attempt to read property 'help_context' on null". Applied the same
  `'orders'` fallback used by the other lookup.

- **44 files under `reporting/` — missing `includes/ui/ui_input.inc`
  include.** An earlier pass in this migration converted raw
  `$_POST[...]` reads to the typed `post_scalar()` accessor across nearly
  every file in `reporting/`, matching the pattern already used
  throughout the rest of the codebase. It correctly added the
  `include_once` for `includes/ui/ui_input.inc` (where `post_scalar()`
  lives) to the handful of files that already called some other
  `ui_input.inc` function, but missed every file that previously only
  ever touched `$_POST` directly and so had never needed that include
  before — 44 files in total. Every one of those has been a guaranteed
  "Call to undefined function post_scalar()" fatal error on its
  reporting action ever since, invisible to Psalm (which resolves
  functions project-wide regardless of per-file include order). Hit
  live via Sales → Inquiries → Search All Sales Orders → Print
  (`reporting/rep109.php`). Fixed by adding the same `include_once`
  already present in sibling files (`rep104.php`, `rep307.php`,
  `rep308.php`) to all 44 affected files.

- **`reporting/includes/class.pdf.inc` — `Cpdf::SetLang()` crashed on an
  unmatched language code.** Chained on from the bug above — once
  `rep109.php` could actually run, it hit
  `strtoupper($lang['encoding'])` with `$lang` null, because
  `array_search_value($code, $installed_languages, 'code')` returns
  `null` whenever the requested code (here, the logged-in user's
  language preference) isn't present in `$installed_languages` — this
  install's `lang/installed_languages.inc` only has a single `'C'`
  entry. The function already had dead fallback logic further down
  (falling back to `ISO-8859-1`/`en_GB`) for exactly this case, but
  could never reach it: `strtoupper(null)` throws on PHP 8.1+, where
  passing `null` to a non-nullable native string parameter is a hard
  error instead of the silent empty-string coercion older PHP allowed.
  Same "strict_types fallout" class as the section above. Fixed by
  wrapping the lookup with `row_or_empty()` and resolving
  `$langCode`/`$encoding` with an explicit `ISO-8859-1` fallback before
  either reaches `strtoupper()` or the `GetText` calls.

- **`includes/ui/ui_view.inc` — `display_quick_entries()`.** Decided which
  GL-posting method to call by checking `$type != QE_SUPPINV`, not by
  checking the actual class of `$cart` (declared `items_cart|supp_trans`) -
  calling `$cart->add_gl_item()` (only declared on `items_cart`) whenever
  `$type` wasn't `QE_SUPPINV`, regardless of what `$cart` actually was.
  `supp_trans` is a `final class` and has no `add_gl_item()` method at all,
  so a `supp_trans` cart passed with any other `$type` would fatal with
  "Call to undefined method supp_trans::add_gl_item()". Every one of this
  function's four current call sites happens to keep `$cart`'s class and
  `$type` in sync (the only `supp_trans` caller always passes
  `QE_SUPPINV`), so this has never actually crashed - but nothing enforces
  that pairing. Fixed by branching on `$cart instanceof supp_trans`
  directly instead of `$type`, which cannot pick the wrong method
  regardless of what `$type` is.

- **`includes/ui/simple_crud_class.inc` — `simple_crud::_check_mode()`.**
  Read the editor's Mode hidden field with `get_post($this->name.'Mode', '')`
  and, if the result was truthy, called `key($mod)` on it unconditionally.
  That hidden field is always rendered as an array (`name="{prefix}Mode[{id}]"`,
  via `hidden()` further down in the same class), so `$mod` is normally an
  array - but `get_post()`'s return type genuinely includes plain strings,
  and nothing stopped a request from posting a non-array value for that
  exact field name instead. `key()` has never accepted a non-array,
  non-object argument in any PHP version, so that would be a fatal
  TypeError on every CRUD editor built on this class (contacts,
  attachments) - not merely a `strict_types` coercion edge case. Added an
  `is_array($mod)` guard before the call.

- **`includes/db/connect_db_mysqli.inc` — `db_set_charset()`.** Declared
  `$db` as its own first parameter, then immediately shadowed it with
  `global $db;` inside the function body - silently discarding whatever
  connection the caller explicitly passed in and always operating on the
  global connection instead. Currently harmless only because both of this
  function's two real call sites (`admin/db/maintenance_db.inc`) happen to
  pass the global `$db` right back to it; a future caller passing a
  genuinely different connection (the whole reason this function takes
  `$db` as a parameter rather than reading the global itself, like its
  sibling `db_set_encoding()` does) would have that argument silently
  ignored. Removed the shadowing `global` statement so the parameter is
  actually used.

- **`includes/ui/class.crud_view.inc` — `user_view::get_fields_views()`.**
  Its sibling functions `set_output()` and `get_input()` both normalize a
  `$this->fields` entry before using it as an array: if the entry isn't
  int-keyed (a plain field name) and isn't already an array, they wrap it
  as `array('fmt' => $fmt)`. `get_fields_views()` is missing that same
  `elseif (!is_array($fmt)) ...` branch, so a shorthand format entry (e.g.
  `fa_reflines`'s own `'default' => 'check'` in
  `includes/ui/class.reflines_crud.inc`) reaches `@$fmt['fmt']` with `$fmt`
  still a plain string - accessing a string offset with a non-numeric key
  is a fatal `TypeError` ("Cannot access offset of type string on
  string") on PHP 8, not a suppressible warning the leading `@` would
  catch. Currently unreachable only because nothing in this codebase calls
  `get_fields_views()` yet. Added the missing normalization branch to
  match its siblings.

## Missing feature (implemented, not just guarded)

- **`includes/ui/attachment.inc` — no attachment size limit.** `db_insert()`
  and `db_update()` computed a local `$max_image_size` (falling back to
  5000) but never actually compared any uploaded file's size against it —
  the variable was set and then never read again. Attachment uploads had no
  size limit at all. Implemented real enforcement using
  `sysprefs()->max_image_size * 1024`, matching the pattern already used for
  image uploads in `inventory/manage/items.php` and
  `admin/company_preferences.php`.
