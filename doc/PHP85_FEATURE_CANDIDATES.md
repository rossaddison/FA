# PHP 8.5 Feature Adoption Candidates

This started as an inventory, not a to-do list: a catalog of where in the
codebase each new PHP 8.5 language feature would fit, for future reference.
Raising the minimum version to 8.5 (tracked in
[PSALM_MIGRATION.md](PSALM_MIGRATION.md)) only makes these features
*available* — actually adopting one at a given site is a separate decision.
`#[\NoDiscard]` has since moved from catalog to actually adopted, codebase-wide
(see its section below) — it's the one feature with no Psalm/PHPStan
inference gap, so there was no reason to wait. The other three remain
catalog-only for now, each blocked on something external (tooling, in two
cases) rather than on a decision still to be made.

Four features were surveyed: the pipe operator (`|>`), `array_first()`/
`array_last()`, `final` on promoted constructor properties, and the
`#[\NoDiscard]` attribute. All four were confirmed empirically to work on
the local PHP 8.5.10 install before any candidate search began.

## Tooling caveat: pipe operator and array_first()/array_last() under Psalm

Confirmed via an isolated scratch test (not from documentation): Psalm
7.0.0-beta21 — the version this project runs — parses both features with no
syntax error, but infers `mixed` for a pipe chain's result and for
`array_first()`/`array_last()`'s return value. It does not trace through to
the real type the way it does for plain nested function calls.

PHPStan 2.2.16, tested the same way, infers correctly through both (a
deliberately-wrong test case — adding an int to a piped-through string —
was caught, with PHPStan reporting the literal string value inferred through
the chain). This is *why* PHPStan was added to the project, scoped to
`src/` only — see [YII3_PSR4_MIGRATION.md](YII3_PSR4_MIGRATION.md) and
`quality/phpstan-config.neon`.

**Practical consequence: adopting either feature anywhere Psalm still
analyzes (i.e. anywhere outside `src/`) would introduce new `mixed` into a
codebase this migration has spent thousands of fixes eliminating.** Until
Psalm's own inference catches up, these two features are candidates for
`src/`-namespaced code only, not for the legacy procedural files below. The
file:line lists in those two sections are recorded anyway, for when that
changes (or for a `src/`-ported equivalent of the same idiom).

`final` promoted properties and `#[\NoDiscard]` have no such gap — Psalm
handles both cleanly today, with no degradation observed.

## Pipe operator (`|>`)

Chains single-argument callables left-to-right: `$x |> f(...) |> g(...)`
reads as `g(f($x))` but in call order. Best fit: short, linear
transform-and-return idioms already written as nested calls or as a
single-use intermediate variable.

### Highest-value family: `row_or_empty(db_fetch(...))`

The dominant idiom in this codebase for "run a query, get one row, treat no
rows as an empty array" is already a 2-step pipeline:

```php
$row = row_or_empty(db_fetch(db_select($sql)));
// or, more commonly:
$row = row_or_empty(db_fetch($result));
```

102 occurrences across 40 files match this family. `includes/dashboard.inc`
alone has 30 instances of the 2-level `row_or_empty(db_fetch($result))`
form — the single densest file for this pattern. As a pipe chain:

```php
$row = $result |> db_fetch(...) |> row_or_empty(...);
```

This is the strongest candidate precisely because it's so repetitive and
uniform — a mechanical rewrite, not a judgment call per site — but it's
blocked on the Psalm-`mixed` gap above for every occurrence outside `src/`
(all 102 are legacy files today).

### Other named candidates

| Site | Current form | Chain |
|---|---|---|
| `includes/current_user.inc:370` | nested `abs()`/`log10()`/`floor()` | `$n \|> abs(...) \|> log10(...) \|> floor(...)` |
| `reporting/includes/class.mail.inc:72` | `md5(uniqid(time()))`-style nesting | `time() \|> uniqid(...) \|> md5(...)` |
| `includes/data_checks.inc:361-362` | `date2sql(add_months(sql2date($x), $n))` | `$x \|> sql2date(...) \|> add_months(...) \|> date2sql(...)` |
| `includes/ui/ui_lists.inc:1213-1214` | identical duplicate of the above | same chain |
| `includes/page/header.inc:46` | `strtr()` with a fixed 2nd-arg table | marginal — needs a wrapping closure for `strtr`'s 2nd argument, so the chain isn't obviously clearer than the current form |

### Out of scope

Vendor/third-party code (`tcpdf.php`, fpdi, `Workbook.php`,
`JsHttpRequest.php`) was excluded from the survey entirely — this migration
only touches first-party code.

## `array_first()` / `array_last()`

Replace `reset($a)`/`end($a)` (pointer-based, mutate the array's internal
cursor) or `$a[0]`/`$a[count($a)-1]` (index-based) where the array's
internal pointer isn't used for anything else afterward.

### Safe candidates (pointer confirmed unused afterward)

- `sales/includes/db/sales_credit_db.inc:24`
- `sales/includes/cart_class.inc:897` and `:324`
- `sales/includes/db/sales_invoice_db.inc:48` — `end($invoice->prepayments)`;
  confirmed safe despite a later read of the same array, since that later
  read never uses a pointer function (`current()`/`next()`/`key()`) either
- `includes/packages.inc:241`
- `admin/db/maintenance_db.inc:490`
- `sales/includes/sales_db.inc:270`
- `reporting/includes/pdf_report.inc:779` — the `$x[count($x)-1]` idiom

### Explicitly risky — do NOT convert

- `includes/ui/class.crud_view.inc:425-426` — `reset()` + `key()` is reading
  the pointer position deliberately, not just the first value
- `install/isession.inc:48` and `includes/session.inc:342` — comments at
  both sites say the `reset()` call is intentionally priming the array
  pointer for a *later* `current()`/`next()` call elsewhere
- `includes/JsHttpRequest.php:224` — vendor code, manual pointer iteration

### Not recommended (marginal)

The `$row[0]` "first SQL column of a fetched row" idiom appears 40+ times.
It's deliberately about *a specific named column*, not "the first element
of an array" conceptually, so rewriting it as `array_first($row)` would
obscure intent rather than clarify it — left out of the candidate list on
purpose.

## `final` on promoted constructor properties

`public function __construct(public final string $x) {}` — locks a
promoted property against override in a subclass, without needing a
separate property declaration + manual assignment just to add `final`.
Confirmed Psalm-clean with no degradation (unlike the two features above).

Both existing `src/` classes from the [PSR-4 pilot](YII3_PSR4_MIGRATION.md)
qualify directly:

- `src/Purchasing/GrnItem.php` — `id`, `po_detail_item`, `gl_code` (plain
  passthrough constructor params, confirmed read-only everywhere else in
  the class)
- `src/Purchasing/GlCodes.php` — `Counter`
- `src/Session/GlobalSelections.php` has no constructor — not applicable

### Strongest codebase-wide candidate

`includes/db/class.data_set.inc`'s `record_set` base class constructor
(lines 47-53) — all 4 parameters are bare passthrough assignments,
confirmed never reassigned anywhere else in the class, and this base class
is used transitively by both `data_set` and `reflines_db`. The single
highest-leverage non-`src/` candidate found, since it's a base class with
multiple consumers rather than a one-off.

### Other candidates

- `includes/ui/simple_crud_class.inc:38-41`
- `reporting/includes/reports_classes.inc`'s `report_control` class
  (lines 535-540 — `id`/`name` only)
- The recurring `class`/`sub_class`/`entity` trio, identically shaped in
  both `includes/ui/contacts_view.inc:45-47` and
  `includes/ui/attachment.inc:38-40`
- `includes/ui/allocation_cart.inc`'s `payment_allocation` class (`type`/
  `type_no` only, lines 385-386) — explicitly a *different* class from the
  session-stored `allocation` class earlier in the same file; this one has
  no branching logic on a same-named property and is a clean candidate
- `reporting/includes/printer_class.inc:22-29` — marginal/mixed; some
  params are bare passthrough, others aren't

A longer list of constructors was checked and rejected as too complex
(property reassigned post-construction, or derived from more than one
parameter) — not reproduced here since they're non-candidates, not
candidates.

## `#[\NoDiscard]` — adopted codebase-wide

Warns (does not error, under Psalm) if a call's return value is silently
discarded — but on PHP 8.5 itself it's not just a static-analysis hint: the
engine emits a real runtime warning when a marked return value is
discarded. Three families were surveyed, with **every call site
individually verified** (re-verified fresh at adoption time, not just taken
from the original survey, precisely because of that runtime behavior) — the
finding in all three was that there were no currently-silent discard bugs
anywhere in this codebase. Adopting `#[\NoDiscard]` on them is therefore
purely preventive (catching a future mistake), not a fix for an existing
one.

Unlike the pipe operator and `array_first()`/`array_last()` above, this
feature has no Psalm/PHPStan inference gap (confirmed via
vimeo/psalm#11381, already closed/merged), so all three families were
adopted immediately rather than left as a catalog entry:

| Family | Definitions | Call sites | Result | Commit |
|---|---|---|---|---|
| ID-returning `write_*`/`add_*` in `*_db.inc` files | 9 named functions (`write_customer_trans`, `add_sales_order`, `write_sales_invoice`, `add_grn`, `add_grn_batch`, `add_po`, `add_supp_invoice`, `write_supp_trans`, `add_dimension`) | 9 | every call site captures the return value | `7e0d1c57` |
| `check_data()` / `can_commit()` | 13 + 1, across 13 files | 14 | all used in an if/&&/! condition or assignment | `fb810046` |
| `can_process()` / `can_delete()` | 26 + 11, across 32 files | 37 | all used in an if/&&/! condition or assignment | `eafff689` |

60 functions across 55 files in total. `can_process()`/`can_commit()`/
`can_delete()`/`check_data()` are PSALM_MIGRATION.md's documented "global
function name collision" pattern — each entry/manage page declares its own,
sharing the name with every other page's — but every call site sits in the
same file as its own definition, so there's no cross-file resolution
ambiguity for these specific functions the way there is for the genuinely
cross-file-called collision functions (`getTransactions`, `display_type`,
etc., which are NOT part of this rollout). Full-project `--no-cache` Psalm
scans before and after each commit showed no change in the project's error
total (7916 throughout) — the attribute is fully invisible to Psalm's
count, as expected.

### Explicitly rejected as weaker candidates

- `write_extensions()` / `write_lang()` / `write_config_db()` — return
  status codes, not IDs; lower stakes if ignored
- The many `void`-returning `add_*`/`write_*` functions — nothing to
  discard, `#[\NoDiscard]` wouldn't apply

Note: this codebase has no `is_valid*`/`validate_*` naming convention —
`check_data()` and the `can_*()` family are the dominant validation idioms,
which is why they were the ones surveyed.

## Process notes

- Both the pipe-operator/array_first survey and the final-property/NoDiscard
  survey were done by two agents working in parallel against the current
  `chore/php-8.5-minimum` tree; file:line references above reflect that
  snapshot and should be re-verified against current line numbers before
  acting on any of them, same as any other stale-reference risk in a
  fast-moving branch.
- None of the 3 `src/` files existing at survey time contained pipe-operator
  or array_first/array_last candidates of their own (they're new, small,
  already-typed classes with no nested-call chains or array-pointer usage).
- Vendor/third-party code was out of scope for all four features.
