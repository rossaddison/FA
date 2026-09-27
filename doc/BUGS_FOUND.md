# Genuine bugs found during the Psalm cleanup

Running log of real defects — not Psalm noise — turned up while working
through the categories described in [PSALM_MIGRATION.md](PSALM_MIGRATION.md).
Most severe first within each section.

## Live crashes

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

## Missing feature (implemented, not just guarded)

- **`includes/ui/attachment.inc` — no attachment size limit.** `db_insert()`
  and `db_update()` computed a local `$max_image_size` (falling back to
  5000) but never actually compared any uploaded file's size against it —
  the variable was set and then never read again. Attachment uploads had no
  size limit at all. Implemented real enforcement using
  `sysprefs()->max_image_size * 1024`, matching the pattern already used for
  image uploads in `inventory/manage/items.php` and
  `admin/company_preferences.php`.
