# Stale journal entries never filtered from transaction view/search

Found while chasing a Psalm `DocblockTypeContradiction` on
`admin/db/transactions_db.inc` during the [Psalm cleanup](PSALM_MIGRATION.md)
of `purchasing/includes/db/suppliers_db.inc`'s neighbourhood. Not fixed yet —
documented first per standing practice for anything beyond a type
annotation. See [BUGS_FOUND.md](BUGS_FOUND.md) for the project's running
list of fixed defects; this one gets its own page because confirming it
required first understanding how `TB_PREF` actually works, which is reusable
background for any future table-name-comparison bug in this codebase.

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

**Fix (not yet applied):** change `TB_PREF.'gl_trans'` to `TB_PREF.'journal'`
on that one line.

## Why the fix holds regardless of per-company table prefixes

FrontAccounting supports multiple companies against possibly-different
`tbpref` values (`config_db.php`'s `$db_connections[$company]['tbpref']`).
Asked directly whether this fix stays correct if a second company with a
different prefix is added — yes, and for a stronger reason than "both sides
happen to use the same constant": **`TB_PREF` is never the real prefix at
the point this comparison runs.**

`includes/current_user.inc` defines it once, as a literal placeholder
token:

```php
define('TB_PREF', '&TB_PREF&');
```

Every `TB_PREF.'tablename'` expression in the codebase — including both
sides of the comparison in this bug — builds a plain PHP string containing
the literal text `&TB_PREF&`, e.g. `"&TB_PREF&journal"`. The *real*
substitution happens later, once, inside `db_query()`
(`includes/db/connect_db_mysqli.inc:50-57`):

```php
$comp = isset(session_obj('wa_current_user')->cur_con) ? session_obj('wa_current_user')->cur_con : 0;
$cur_prefix = @$db_connections[$comp]['tbpref'];
$sql = str_replace(TB_PREF, $cur_prefix, $sql);
```

— i.e. the active company's real prefix is looked up fresh and substituted
into the *finished SQL string*, immediately before every single query
executes. The PHP-level string comparison in
`get_sql_for_view_transactions()` happens entirely before that
substitution, comparing two strings both still carrying the same
unsubstituted `&TB_PREF&` token. Which company is active, or what its
`tbpref` is, never enters into it — the fix is correct for a single-company
install, for `fa_spike` here, and for any future multi-company setup with
arbitrary/differing prefixes, by construction.

This also explains an unrelated piece of Psalm noise: Psalm's
`allConstantsGlobal` resolves `TB_PREF` to its literal define()'d value, so
every `TB_PREF.'x'` concatenation Psalm sees really does type as the literal
string `'&TB_PREF&x'` — which is why a hand-written docblock like
`get_systype_db_info()`'s enumerates `'&TB_PREF&bank_accounts'|'&TB_PREF&bank_trans'|...`
rather than anything resembling a real table name. Not a bug, just a
faithful reflection of how this placeholder mechanism actually works.

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
- Not yet done: applying the fix, and a live before/after test (create a
  journal entry, edit it, confirm the stale row appears in
  `view_print_transaction.php` pre-fix and is excluded post-fix).
