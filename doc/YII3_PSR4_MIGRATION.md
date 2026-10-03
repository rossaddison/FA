# Yii3 / PSR-4 Namespacing Pilot

FrontAccounting's codebase predates PSR-4 entirely — `composer.json` had no
`autoload` section at all until this effort, and every class/function lives
in the global namespace. This doc tracks the incremental, file-by-file
introduction of a real `FrontAccounting\` PSR-4 root under `src/`, done
alongside the [Psalm cleanup](PSALM_MIGRATION.md) rather than as a separate
rewrite: each pilot picks one small, self-contained piece of an
already-being-cleaned legacy file and extracts it into a properly namespaced,
fully-typed class, leaving everything else in place and working exactly as
before.

The project already depends on several `yiisoft/*` packages via Composer
(there's one precedent for using a Yii3 class directly — `includes/yii/layout.inc`
`use`s `Yiisoft\View\WebView`), so `src/` is reused rather than reinvented as
the landing zone for this code, with the long-term intent that genuinely new
functionality can eventually be built the Yii3 way instead of the legacy
procedural way.

## Why most classes are *not* candidates

The single biggest risk in this codebase isn't Psalm noise, it's
`$_SESSION`. A large number of FrontAccounting's central business objects —
`Cart`, `supp_trans`, `allocation` — are plain objects instantiated once per
request and then written straight into `$_SESSION` for the rest of a
multi-step UI workflow (an invoice/order/credit-note entry form, a
payment-allocation screen). PHP deserializes a session object by looking up
its class **by the exact name stored at serialization time**. If that class
isn't defined and loaded by the time `session_start()` runs, PHP doesn't
error — it silently hands back a `__PHP_Incomplete_Class` object instead,
which behaves like neither the real class nor a clean failure: property
reads return `null`, method calls fatal with "Call to a member function ...
on object of type `__PHP_Incomplete_Class`", and the failure mode is a
runtime crash in the *next* request, not the one that saved the broken
session.

Renaming or re-namespacing one of these classes outright would change the
exact name PHP stores at serialize time, which breaks every session created
under the old name the moment the code ships — a live production incident
for anyone mid-workflow at deploy time, not a theoretical risk. This was
flagged explicitly when `includes/ui/allocation_cart.inc`'s `allocation`
class came up as a candidate earlier in the Psalm pass (it's stored in
`$_SESSION['alloc']`) and was turned down for exactly this reason, and it's
why **`supp_trans` (`purchasing/includes/supp_trans_class.inc`), stored
directly in `$_SESSION['supp_trans']`, is deliberately left as a plain
global class in this pass** — only its two line-item *value* classes,
`grn_item` and `gl_codes`, were extracted (see below). `supp_trans` itself
is a legitimate future candidate, but only behind a compatibility shim —
e.g. a `final class supp_trans extends
\FrontAccounting\Purchasing\SupplierTransaction {}` stub kept in the
original file, always loaded via the existing include chain before
`session_start()` ever runs, so old and new sessions alike keep resolving to
a real, loaded class under the name PHP actually serialized. That's a
bigger, riskier change than a single pass justifies, so it's tracked here as
a deliberate non-goal rather than an oversight.

**Selection criteria for a safe pilot, in order:**

1. **Never stored in `$_SESSION` directly.** Properties of a session-stored
   object are fine (see `grn_item`/`gl_codes` below — they only ever live
   inside `supp_trans::$grn_items`/`$gl_codes` arrays); the root object
   itself is not, unless it gets a same-name compatibility shim.
2. **Few or no external callers reference the class by name.** Checked by
   grepping the whole repo for `new ClassName(`, `ClassName $x` type-hints,
   `@param`/`@return`/`@var ClassName` docblocks, and `instanceof ClassName`
   before touching anything — if the inventory comes back large, the blast
   radius is too wide for one pass.
3. **Self-contained enough to fully type.** A class whose entire job is
   holding a handful of typed properties (a value object) converts cleanly;
   one entangled with global helper-function calls and conditional session
   state does not.

## Completed pilots

### 1. `GlobalSelections` (`includes/ui/ui_globals.inc` → `src/Session/GlobalSelections.php`)

The first pilot and the one that introduced the `autoload` block at all:

```json
"autoload": {
    "psr-4": {
        "FrontAccounting\\": "src/"
    }
}
```

`ui_globals.inc`'s eight `set_global_*()`/`get_global_*()` functions
(last-picked supplier/stock-item/customer/currency, each backed by one
`$_SESSION` key) became thin one-line wrappers around a new static
`FrontAccounting\Session\GlobalSelections` class — chosen specifically
because it's functions wrapping scalar `$_SESSION` reads/writes, never an
object stored in the session itself, so there was no deserialization risk
to design around at all. All 55 existing call sites across 23 files are
untouched.

### 2. `GrnItem` + `GlCodes` (`purchasing/includes/supp_trans_class.inc` → `src/Purchasing/`)

`supp_trans_class.inc` defines three classes: `supp_trans` (the
session-stored root — left alone, see above), and two line-item helper
classes it owns arrays of — `grn_item` (cached GRN/purchase-order-detail
data for one invoiced/credited line) and `gl_codes` (one manually-entered GL
coding line). A repo-wide grep confirmed neither helper class is
instantiated, type-hinted, or referenced anywhere outside this one file
(one stray `@param grn_item` docblock in `purchasing/includes/db/grn_db.inc`
was the only external reference, updated to the new FQCN), so both moved
cleanly:

- `FrontAccounting\Purchasing\GrnItem` replaces `grn_item` — `final`, every
  constructor parameter natively or doc-typed (the original had none,
  cascading into a dozen-plus Psalm `MissingParamType`/`MixedAssignment`
  findings on its own).
- `FrontAccounting\Purchasing\GlCodes` replaces `gl_codes` — same treatment.
- `supp_trans_class.inc` gained `use` imports for both and had its own,
  separate Psalm backlog fixed in the same pass (property-type coercions,
  float casts through the GL-posting arithmetic, and — in a follow-up pass —
  `$tax_overrides` made genuinely nullable (`null` assigned instead of
  `unset()`) so its "cancelled after a cart change" state is honestly
  reflected in the declared type, replacing two `@psalm-suppress
  RedundantConditionGivenDocblockType` annotations that had been covering for
  the mismatch).

Both new classes scan at **0 Psalm errors**; `supp_trans_class.inc` itself
went from roughly 90 errors to 0 (see
[PSALM_MIGRATION.md](PSALM_MIGRATION.md)'s known-noise categories for the
general verification approach).

Verified via a standalone Composer-autoload smoke test (`new GrnItem(...)`,
`new GlCodes(...)` resolved and instantiated with no `dump-autoload` needed,
since the project's PSR-4 mapping is dynamic, not a compiled classmap), a
full-project `--no-cache` Psalm scan, and live HTTP testing of
`purchasing/supplier_invoice.php`, `purchasing/supplier_credit.php`, and
`purchasing/view/view_supp_invoice.php`.

## Process notes

- `composer dump-autoload` is not required after adding a new class under an
  already-mapped PSR-4 root — confirmed via `vendor/composer/autoload_psr4.php`,
  which maps `FrontAccounting\` to `src/` dynamically rather than through a
  compiled classmap.
- Each pilot is verified the same way as any other change in this migration:
  `php -l` on every touched file, a full-project `--no-cache` Psalm scan
  before/after, and a live HTTP smoke test of every page that exercises the
  touched code — see [PSALM_MIGRATION.md](PSALM_MIGRATION.md#process-notes)
  for why `--no-cache` matters and why single-file/folder-scoped scans are
  unreliable here.
- Work proceeds one extraction at a time, same as the wider Psalm cleanup —
  no batch-converting multiple classes in one pass.
