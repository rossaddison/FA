# PHP 8.5 / Psalm Migration

Branch `chore/php-8.5-minimum` is undergoing a file-by-file Psalm static-analysis
cleanup as part of raising the minimum PHP version to 8.5. This doc covers the
methodology and the categories of noise vs. real signal; see
[BUGS_FOUND.md](BUGS_FOUND.md) for the running list of genuine defects the
cleanup has turned up.

## Snapshot

Full project scan, 2026-09-27: **10,158** Psalm errors (errorLevel=1).

## `declare(strict_types=1)` rollout

Every first-party `.php`/`.inc` file (the same set covered by `psalm.xml`'s
`<projectFiles>`, 433 files) got `declare(strict_types=1);` inserted as the
first statement, by explicit request — done as a deliberate blanket,
fix-forward change rather than an incremental one. This codebase relies on
PHP's weak-typing coercion throughout — the pervasive
`string|int|float|bool|null` parameter unions exist specifically because
callers pass mismatched-but-coercible scalars today (e.g. `date('m')`, which
returns a zero-padded numeric *string*, passed straight into `mktime()`'s
`int|null` month parameter in `rep302.php`'s `getPeriods()`, discovered and
fixed as part of this rollout). Enabling strict_types per-file makes that
exact class of call throw a fatal `TypeError` at runtime instead of silently
coercing.

Verified before pushing: `php -l` clean and no BOM introduced across all 433
files, plus a live smoke test of 10 representative authenticated pages
(dashboard, GL journal, sales/purchase invoice entry, reports menu,
inventory/GL inquiries) with no fatal errors. That only exercises page
*loads*, not deeper form-submission or report-generation paths — those are
where the real risk lives, and where breakage is expected to keep surfacing.
The fix pattern established by the `rep302.php` case: cast the actual value
explicitly at the point it flows into the strictly-typed parameter, never
revert the `declare(strict_types=1)` line itself.

Progress is tracked by the overall project total, not per-file counts — see
"Known noise" below for why per-file counts are unstable and misleading here.

| Batch | Psalm issue types | Notes |
|---|---|---|
| 1. Mixed-family | MixedArgument, MixedAssignment, MixedOperand, MixedArrayAccess, MixedArrayOffset, MixedArrayAssignment, MixedPropertyFetch/Assignment, MixedMethodCall, MixedArgumentTypeCoercion, MixedPropertyTypeCoercion, etc. | Cast at use-site or narrow the loosely-typed helper's signature |
| 2. Null/false-safety | PossiblyNullOperand, InvalidOperand, PossiblyNullArgument, PossiblyNullArrayOffset/Access, PossiblyNullPropertyAssignmentValue, PossiblyNullReference, PossiblyFalseOperand/Argument, etc. | Casts, `?:` fallbacks, guard checks |
| 3. Argument mismatches | PossiblyInvalidArgument, ArgumentTypeCoercion, InvalidArgument, InvalidScalarArgument, TooManyArguments, TooFewArguments | Same cast idiom; Too Many/Few worth checking individually for real bugs, but mostly stem from the function-name collisions below |
| 4. Casting noise (accepted) | PossiblyInvalidCast, RiskyCast, RedundantCast(GivenDocblockType) | Established accepted leftover; candidate for formal suppression |
| 5. Undefined vars/offsets/methods/properties | UndefinedVariable, UndefinedGlobalVariable, UndefinedFunction, UndefinedMethod, UndefinedPropertyFetch, UndefinedConstant, InvalidMethodCall, DuplicateArrayKey, PossiblyUndefinedArrayOffset/Variable, InvalidArrayOffset | Highest scrutiny — best source of genuine bugs; see BUGS_FOUND.md |
| 6. RiskyTruthyFalsyComparison | (single type) | Established accepted idiomatic-PHP pattern |
| 7. Missing type declarations | MissingParamType, MissingReturnType, MissingPropertyType, MissingOverrideAttribute, MissingPureAnnotation | Add native types (covariance-check first) or docblocks |
| 8. Property/constructor | PropertyNotSetInConstructor, PropertyTypeCoercion, Invalid/PossiblyInvalidPropertyAssignmentValue | Widen or fix property docblocks |
| 9. Security taint findings | TaintedFile, TaintedHtml, TaintedTextWithQuotes, TaintedSSRF, TaintedInclude, TaintedSql, TaintedCookie, TaintedHeader, TaintedCallable | Real vulnerability hunting; several confirmed and fixed (see BUGS_FOUND.md) |
| 10. Reference-usage (Psalm limitation) | UnsupportedReferenceUsage, UnsupportedPropertyReferenceUsage, ReferenceConstraintViolation | Mostly accepted as a Psalm limitation |
| 11. Dead-code signals | RedundantCondition(GivenDocblockType), DocblockTypeContradiction, TypeDoesNotContainType/Null, RedundantFunctionCall, NoValue, LoopInvalidation | Often genuine dead code |
| 12. Return-type mismatches | InvalidReturnStatement, LessSpecificReturnStatement/Type, NullableReturnStatement, ImplementedReturnTypeMismatch | Narrow/correct declared return types |
| 13. Everything else | Class/architecture, purity annotations, misc rare types | Case-by-case, low volume |

## Known noise (investigated, not bugs)

**Global function name collisions.** Around three dozen global function names
are declared more than once across `reporting/` and `gl/` (`getTransactions` up
to 19 times, `display_type` 8 times, `getPeriods`, `get_open_balance`,
`get_bank_balance_to`, `get_bank_transactions`, `get_supp_inv_reference`,
`getTaxTransactions`, `fetch_items`, `trans_qty`, `get_domestic_price`,
`print_gl_rows`, `add_to_order`, and others). Psalm resolves a call against
whichever declaration it picks, which is not necessarily the one actually
executed at that call site, producing false `TooManyArguments`/
`TooFewArguments`/`UndefinedProperty`-style noise. **Policy: never touch these
functions' own signatures — only cast call-site arguments.** Editing one
sibling's body can shift which declaration Psalm treats as canonical
elsewhere, causing count fluctuations in *other* sibling files; this is
expected and tracked via the overall project total only.

**Global-variable `@var` narrowing can leak across files via a collision
function.** `/** @var int|string $selected_id */ $selected_id = $selected_id;`
(the established pattern for narrowing the shared `simple_page_mode()`
global ahead of local use) is file-local by itself, but if that narrowed
variable is then passed as an argument to a *collision* function (one of the
~37 names above, e.g. `can_delete($selected_id)`, declared separately in 8+
`manage/*.php` CRUD pages), Psalm's whole-project analysis can attribute the
narrowed type to *other* files' calls to that same collision function,
producing new `MixedArgument`/`PossiblyInvalidArgument` regressions in files
you never touched (confirmed: adding this assertion to
`sales/manage/credit_status.php` shifted error counts in `admin/tags.php`,
`fixed_assets/fixed_asset_classes.php`, three `gl/manage/*.php` files,
`inventory/manage/locations.php`, and `manufacturing/manage/work_centres.php`
— all unrelated files whose only connection is also calling their own
same-named `can_delete()`). **Fix pattern: never re-type the global itself
when a collision function is anywhere downstream in the same file — guard
inline at each call site instead**, e.g.
`can_delete(is_array($selected_id) ? '' : (string) $selected_id)`, which
computes a narrowed *expression* for that one call without altering
`$selected_id`'s inferred type for the rest of the file (or project).
Because full-project scans are also subject to the cache noise below, verify
a fix like this with `--no-cache` and by diffing the unrelated collision
files' own counts before/after, not just the overall total.

**Dynamic `$_SESSION[$key]` access against a shaped array type.** `psalm.xml`
declares `$_SESSION` as a shaped array mapping specific literal keys to
specific class types (`Items?: Cart, supp_trans?: supp_trans, ...`). When the
key is a runtime variable rather than a literal, Psalm must union every value
type in the shape, producing false `UndefinedPropertyFetch` for properties
that only exist on some of those types. Fix pattern: narrow with a local
typed variable, e.g. `/** @var Cart $cart */ $cart = $_SESSION[$cartname] ?? null;`
then use `$cart->prop`.

**Cross-file global initialization.** Variables assigned at the top level of
one included file (`version.php`'s `$version`, `config_db.php`'s
`$db_connections`, dynamically-included `sql/alter*.php` upgrade scripts'
`$install`) aren't visible to Psalm when a *different* file does
`global $x;` and reads them, even though the include order guarantees they're
set by the time execution reaches that point. Where the read site already
guards with `isset()`/`unset()` first, this is accepted as unfixable noise
(the config files in question are often per-install/gitignored and outside
Psalm's project scope by design). Where it's a plain `global $x;` followed by
unconditional use of a project-wide config array (`$systypes_array`,
`$tmonths`, `$wo_types_array`, etc.), the fix is a local
`/** @var array<int, string> $systypes_array */` assertion immediately after
the `global` statement — this alone has been the single highest-yield "lever"
fix in the whole cleanup, collapsing large cascades of `MixedArrayAccess`
noise down to a simple offset-cast issue in ~15+ files. The same pattern
applies to a global that's mutated by reference across many function calls
over a request's lifetime (`includes/db/sql_functions.inc`'s
`$transaction_level`, incremented/decremented by `begin_transaction()`/
`commit_transaction()` for nested-transaction reference counting;
`includes/ui/ui_controls.inc`'s `$ajax_divs`, pushed/popped across
`div_start()`/`div_end()`; `includes/db/manufacturing_db.inc`'s
`$qoh_stock`, lazy-loaded once and memoized across recursive calls to
`stock_demand_manufacture()`) — Psalm narrows it to a literal `0`/`-1`/`null`
at points where the real runtime value is a general `int`/array; same
`@var` fix.

**`psalm.xml`'s `<globals>` type declarations can themselves be wrong.**
`installed_languages` was declared `array<int, array<string, string>>`, but
the real `rtl` key is a genuine `bool` everywhere it's set
(`install/isession.inc`, `admin/inst_lang.php`'s `(bool)$_POST['rtl']`,
`includes/packages.inc`). This made every `$lang['rtl'] === true` check
across the codebase look like a permanent-false `DocblockTypeContradiction` —
the application code was correct; the global's declared type was too narrow.
Widened to `array<int, array<string, string|bool>>`.

**Overly-broad legacy `@return` docblocks that were never actually reachable.**
`write_customer_trans()` (`sales/includes/db/cust_trans_db.inc`) was declared
`@return array<array-key, mixed>|null|scalar`, but its only return statement
is `return $trans_no;`, where `$trans_no` is either the incoming
`string|int|float|bool|null` parameter unchanged or the result of
`get_next_trans_no()` (`float|int`) — never an array. The bogus `array<...>`
half of the union then propagated through every caller that returns its
result directly (`write_sales_invoice()`, `write_credit_note()`,
`write_sales_delivery()`, and in turn `Cart::write()`), each independently
declared with the same over-broad `array<array-key, mixed>|null|scalar`.
Narrowing just the one root function's docblock and its three direct callers
resolved a disproportionately large number of downstream findings (a single
run: total Psalm errors dropped by 63, `InvalidReturnStatement`/
`InvalidReturnType` by only 7 of those) — the extra type noise was quietly
inflating `Mixed*`-family findings everywhere invoice/credit-note/delivery
numbers get used afterward (reports, GL views, etc.).

**Template-method base classes with narrower placeholder return types than
their overrides.** `simple_crud` (`includes/ui/simple_crud_class.inc`) and
`simple_crud_view` (`includes/ui/class.crud_view.inc`) declare stub methods
like `db_insert()`, `db_update()`, `db_read()`, `db_delete()`,
`insert_check()`, `list_view()` meant to be overridden by every concrete
subclass — the base implementations just call
`display_notification(__FUNCTION__.' is not defined...')` and are never
meant to run. Their placeholder return types (`void`, `true`, `array<never,
never>`) were narrower than what the real overrides in `attachments`/
`contacts`/`fa_reflines` actually return (`bool|mysqli_result`, `bool`,
`array<array-key, mixed>|false`, etc.), which is an LSP violation Psalm
correctly flags as `ImplementedReturnTypeMismatch`. Fixed by widening the
base declarations to a union covering every current override (e.g.
`void|bool|mysqli_result`) rather than narrowing the overrides — the base
stub's "return nothing meaningful" behavior is a true subtype of that
union. Same category as `archive`'s `create_tar()`/`create_pkg()` stubs
(see BUGS_FOUND.md) but for return types instead of missing methods
entirely.

**Dynamically-loaded theme classes.** `frontaccounting.php` / `includes/page/
header.inc` / `includes/page/footer.inc` load a `renderer` class via
`include_once($path_to_root . "/themes/".user_theme()."/renderer.php")`.
`themes/` is in `psalm.xml`'s `<ignoreFiles>`, so Psalm genuinely cannot see
the class even though it exists identically in every shipped theme. Left
unfixed — fixing it would mean restructuring the dynamic theme-loading system
or adding a synthetic Psalm stub, out of proportion to the value.

**Missing `@psalm-taint-escape` annotations on real sanitizers.** Psalm's
taint analysis only knows a function neutralizes a given taint type
(`sql`, `html`, `file`, ...) if it's annotated `@psalm-taint-escape <type>`.
Several of this codebase's actual sanitizers were missing it —
`db_escape()` (genuinely calls `mysqli_real_escape_string()`),
`clean_file_name()` (strips to `[a-zA-Z0-9.\-_]` only), `html_specials_encode()`
(both copies — genuinely calls `htmlspecialchars()`), and `date2sql()`
(always returns a hardcoded safe literal or
`sprintf("%04d-%02d-%02d", (int)..., (int)..., (int)...)`, never raw input).
Without the annotation, Psalm treats every value that ever passed through
these functions as still tainted, which is most of `TaintedHtml`'s and a good
chunk of `TaintedSql`'s volume. All four are now annotated; see
[BUGS_FOUND.md](BUGS_FOUND.md) for the real SQL-injection bugs this exercise
actually turned up along the way.

**`TaintedFile`/`TaintedSSRF` via a database round-trip.** A large share of
these two categories (`file_get_contents(company_path()."/attachments/".
$row['unique_name'])`-style patterns in `admin/attachments.php`,
`includes/ui/attachment.inc`, `includes/ui/ui_view.inc`) trace back through a
`db_select()`/`db_fetch()` call to a `$_GET`/`$_POST` value used only as a
`db_escape()`-guarded numeric lookup key — the actual file path comes from a
column that's always server-generated (`uniqid()` at upload time, never
attacker-supplied). Psalm's taint model treats all data read back from the
database as still tainted by design (a stored-injection chain is a real risk
class in general), so this is expected, not something to blanket-annotate
away — doing so would risk masking a genuine future stored-injection bug.
Spot-checked each occurrence's SQL for a missing `db_escape()` instead of
trying to silence the category.

## Established idioms

- Raw `$_POST['PARAM_N']` / `$_GET[...]` access → `post_scalar('PARAM_N')` /
  `get_scalar('PARAM_N')` (from `includes/ui/ui_input.inc`), except where
  genuine array support is needed (multi-select fields), which keep raw
  access with `@var` assertions and manual `is_array()`/`is_bool()`/
  `is_float()` narrowing.
- Never cast `$_GET`/`$_POST`/broadly-unioned variables directly to
  `(int)`/`(float)` when the type could include `array` (yields unpredictable
  0/1 — Psalm's `RiskyCast`); guard with
  `is_array($x) ? <safe-default> : (int)/(float) $x`.
- `instanceof mysqli_result` in place of `$result === false` before
  `db_fetch()`/`db_num_rows()`, since `db_query()` returns `mysqli_result|bool`
  and `=== false` narrowing alone still leaves `mysqli_result|true`.
- `row_or_empty(array|false|null $row): array` (in
  `includes/db/connect_db_mysqli.inc`) wraps lookups that can return `false`
  (`get_default_bank_account()`, `get_branch()`, `get_bank_account_by_name()`,
  etc.) so callers can treat the result as an array unconditionally.
- `for ($i = $from; $i <= $to; $i++)` where `$from`/`$to` come from
  `min()`/`max()` on `explode()` results and are still typed `string` triggers
  PHP's alphanumeric string-increment semantics instead of numeric increment.
  Wrap immediately with `(int)` after the `min()`/`max()` call.
- `array{0:float,1:float,...}` docblock shape assertions plus a literal loop
  bound (`$i < 5` instead of `$i < count($arr)`) resolve Psalm's
  `InvalidArrayOffset` false positives on fixed-size literal arrays.

## Process notes

- Single-file/folder-scoped `psalm <path>` runs are unreliable in this
  codebase (the collision class-name resolution above depends on which files
  are in scope) — always use a filtered full-project scan
  (`vendor/bin/psalm --output-format=json`, then filter by `file_name`).
  Exit code 2 means "errors were found" (normal), not an infrastructure
  failure.
- Even full-project scans have measurable cache noise: consecutive *cached*
  runs with no source changes between them have been observed to drift by
  ~10-20 in the overall total (confirmed 8410 vs 8423 vs 8430 across three
  cached runs around the same edit). A `--no-cache` run is the only reliable
  number for before/after comparison when the delta is small or when ruling
  out the collision-leakage class above; don't chase small total-count
  deltas using cached runs alone.
- Work proceeds one file (or one well-scoped bug) at a time; no batching or
  architectural shortcuts.
