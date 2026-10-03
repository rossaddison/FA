# Stale journal entries never filtered from transaction view/search

Found while chasing a Psalm `DocblockTypeContradiction` on
`admin/db/transactions_db.inc` during the [Psalm cleanup](PSALM_MIGRATION.md)
of `purchasing/includes/db/suppliers_db.inc`'s neighbourhood. Documented
first, fix applied and verified afterward (commit `ce06569f`) — see
[BUGS_FOUND.md](BUGS_FOUND.md) for the one-paragraph summary in the
project's running list of fixed defects; this one gets its own page because
confirming it required first understanding how `TB_PREF` actually works,
which is reusable background for any future table-name-comparison bug in
this codebase.

## The bug

`get_sql_for_view_transactions()` (`admin/db/transactions_db.inc:16`), used
by `admin/void_transaction.php` and `admin/view_print_transaction.php` to
list/search transactions of a user-chosen type, has this exclusion step:

```php
// the ugly hack below is necessary to exclude old gl_trans records lasting after edition,
// otherwise old data transaction can be retrieved instead of current one.
if ($table_name==TB_PREF.'gl_trans')
    $sql .= " AND t.`amount` <> 0";
```

`$table_name` always comes from `get_systype_db_info()`, whose `switch`
statement is exhaustive and **never returns `TB_PREF.'gl_trans'`** — for
`ST_JOURNAL`/`ST_COSTUPDATE` (the only cases this comment could plausibly be
about) it returns `TB_PREF.'journal'`. `gl_trans` and `journal` are two
different, real tables (confirmed against the live `fa_spike` database:
both exist, with different schemas — `gl_trans` keys on `type`/`type_no`,
`journal` keys on `type`/`trans_no`). So the condition can never be true,
for any transaction type, ever — Psalm's `DocblockTypeContradiction` on the
hand-written `@return` docblock a few lines below is what surfaced this.

The comment's intent is correct, just pointed at the wrong table.
`void_journal_trans()` (`gl/includes/db/gl_journal.inc:183-197`) does
`UPDATE journal SET amount=0 WHERE type=... AND trans_no=...` when a journal
entry is voided or replaced by an edit — leaving exactly the kind of stale,
zero-amount row in `journal` the comment describes. The exclusion should
check `TB_PREF.'journal'`, not `TB_PREF.'gl_trans'`.

**User-visible effect:** editing a previously-saved GL journal entry leaves
the old (zeroed) version visibly listed alongside the new one in
`void_transaction.php`'s and `view_print_transaction.php`'s transaction
search, for any user searching/filtering by the Journal Entry transaction
type. Confirmed the mechanism is live-reachable; this specific dev database
just hasn't had a journal entry edited yet (2 rows total, neither zeroed),
so reproducing it visibly needs a deliberate edit-then-search first.

**Fix (applied, commit `ce06569f`):** changed `TB_PREF.'gl_trans'` to
`TB_PREF.'journal'` on that one line.

## Why the fix holds regardless of per-company table prefixes

FrontAccounting supports multiple companies against possibly-different
`tbpref` values (`config_db.php`'s `$db_connections[$company]['tbpref']`).
Asked directly whether this fix stays correct if a second company with a
different prefix is added — yes, and for a stronger reason than "both sides
happen to use the same constant": `TB_PREF` is never the real prefix at the
point this comparison runs — it's a literal placeholder token, substituted
into the finished SQL string only once, immediately before each query
executes. Both sides of this bug's comparison happen entirely before that
substitution. Full mechanism (with the actual `define()`/`db_query()` code)
now lives in [DATA_FLOW_ARCHITECTURE.md](DATA_FLOW_ARCHITECTURE.md#tb_pref-a-placeholder-token-substituted-at-query-time--not-a-real-per-company-constant),
since it's a codebase-wide fact this bug happened to be the one that
surfaced it, not something specific to this file.

## Verification done so far

- Confirmed `get_systype_db_info()`'s `switch` is exhaustive and traced
  every case — none returns `TB_PREF.'gl_trans'`.
- Confirmed both callers of `get_sql_for_view_transactions()`
  (`void_transaction.php`, `view_print_transaction.php`) pass `$table_name`
  only via `get_systype_db_info()` — no other code path reaches this
  comparison.
- Queried the live `fa_spike` database directly: `gl_trans` and `journal`
  both exist, unprefixed (this install's `tbpref` is `''`), with distinct
  column sets; `0_gl_trans` does not exist.
- Traced `void_journal_trans()`'s `UPDATE journal SET amount=0` as the
  source of the stale rows the original comment is about.
- A full-project `--no-cache` Psalm scan after the fix: both findings at
  this line are gone, no regressions (7908 total, down from 7910).
- A live end-to-end test against the real `fa_spike` database: inserted two
  `journal` rows directly (`trans_no` 9999001, `amount` 0, simulating a
  voided/stale entry; `trans_no` 9999002, `amount` 555, the current one),
  logged in as Administrator, and searched
  `admin/view_print_transaction.php` filtered by the Journal Entry type
  across that range. Only 9999002 appeared in the results table — 9999001
  was correctly excluded. Test rows deleted afterward.
