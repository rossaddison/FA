# PHP 8.5 / Psalm Migration

Branch `chore/php-8.5-minimum` is undergoing a file-by-file Psalm static-analysis
cleanup as part of raising the minimum PHP version to 8.5. This doc covers the
methodology and the categories of noise vs. real signal; see
[BUGS_FOUND.md](BUGS_FOUND.md) for the running list of genuine defects the
cleanup has turned up.

## Snapshot

Full project scan, 2026-09-27: **10,397** Psalm errors (errorLevel=1).

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
noise down to a simple offset-cast issue in ~15+ files.

**Dynamically-loaded theme classes.** `frontaccounting.php` / `includes/page/
header.inc` / `includes/page/footer.inc` load a `renderer` class via
`include_once($path_to_root . "/themes/".user_theme()."/renderer.php")`.
`themes/` is in `psalm.xml`'s `<ignoreFiles>`, so Psalm genuinely cannot see
the class even though it exists identically in every shipped theme. Left
unfixed — fixing it would mean restructuring the dynamic theme-loading system
or adding a synthetic Psalm stub, out of proportion to the value.

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
- Work proceeds one file (or one well-scoped bug) at a time; no batching or
  architectural shortcuts.
