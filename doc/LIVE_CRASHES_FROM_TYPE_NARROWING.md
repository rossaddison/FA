# Live crashes found via real browser testing, from earlier narrowing passes

Three real, user-facing crashes were found by clicking around a logged-in
session and screenshotting the errors — not by Psalm, and not by this
session's own HTTP "live tests" either, which (until caught and corrected)
were accidentally hitting FrontAccounting's login page instead of the real
target pages. All three traced back to a parameter or class being narrowed
too aggressively in an earlier Psalm cleanup pass, each for the same root
reason: the narrowing only considered what Psalm could see, not everything
that calls the code at runtime.

## 1. `display_profit_and_loss()` rejected a plain `int`

**Symptom:** `gl/inquiry/profit_loss.php` — `Unhandled exception:
display_profit_and_loss(): Argument #1 ($compare) must be of type
array|string|null, int given`.

An earlier pass narrowed `$compare` from `mixed` to `string|array|null`,
excluding `int`. Its one call site, `display_profit_and_loss(get_post('Compare'))`,
can genuinely receive a plain `int` at runtime via this app's AJAX
request-decoding path, not just the `string`/`array` shapes `get_post()`'s
own declared return type suggests. Fixed by widening to
`string|int|array|null` and adding a defensive guard that treats `array`/
`null` as the default comparison mode (`0`), since those branches were
never actually safe for the function's own body anyway (`$compare` is used
as an array offset and concatenated into a string).

## 2. `Cart` was marked `final`, but a real subclass exists outside Psalm's scope

**Symptom:** `modules/import_transactions/...` — `Class import_sales_cart
cannot extend final class Cart`, a hard fatal on every page of that module.

An earlier pass marked `Cart` `final` because nothing in Psalm's *scanned*
tree extends it. But `modules/import_transactions/includes/
import_sales_cart_class.inc`'s `import_sales_cart` genuinely does, and
`modules/` is deliberately excluded from `psalm.xml`'s project files
(parked, out of scope — see `doc/PSALM_MIGRATION.md`). Fixed by removing
`final` and adding `@psalm-suppress ClassMustBeFinal` with a comment
explaining why, so a future Psalm-driven pass doesn't silently "fix" it
back. Audited every other `final class` in the scanned tree against
`modules/` and `reporting/` (the other ignored directories) for the same
bug class — no other instances found.

## 3. A row-color counter was accidentally typed as `float`

**Symptom:** `includes/ui/ui_view.inc` → `includes/ui/ui_controls.inc` —
`alt_table_row_color(): Argument #1 ($k) must be of type string|int|null,
float given`.

`display_allocations()` initialized two unrelated variables in one chained
assignment, `$k = $total_allocated = 0.0;` — which made the row-color
counter `$k` a `float` by accident (`$total_allocated` genuinely needs to
be `float`; it accumulates a running money total). `alt_table_row_color()`
takes its counter *by reference* and only accepts `int|string|null`, so
this was a guaranteed fatal on the very first row. Fixed by splitting the
assignment: `$k = 0; $total_allocated = 0.0;`.

## Not a code bug: duplicate bank transaction demo data

A fourth screenshot (`gl/view/gl_payment_view.php?trans_no=58`,
"duplicate payment bank transaction found") turned out not to be a
regression. The dev database genuinely has three separate `bank_trans`
rows with `type=1` (`ST_BANKPAYMENT`) and `trans_no=58` (different `id`s,
refs, amounts and dates) — pre-existing demo/seed data, confirmed via a
direct query, not something created by this branch's testing. The page
already handles this gracefully via `display_db_error()` rather than
crashing.

## The lesson

A Psalm-driven narrowing is only as good as what Psalm can see. Two of the
three crashes here came from code paths Psalm has no visibility into at
all — a dynamically-AJAX-decoded request value, and a class hierarchy that
spans into a directory `psalm.xml` deliberately ignores. The third was a
plain copy-paste/chaining mistake that Psalm *did* flag correctly, but
which never got caught before now because the verification step that
should have caught it — hitting the real page, logged in — wasn't actually
doing that. All three are a reminder that a clean Psalm scan is necessary
but not sufficient; narrowing a parameter's type is a claim about every
real caller, not just the ones in the scanned tree, and confirming that
claim requires exercising the code for real, not just syntax-checking it.
