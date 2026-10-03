# Request data flow: input escaping and table-prefix substitution

Two non-obvious, load-bearing mechanisms that control how data moves
through every single request in this codebase. Both were surfaced
incidentally while investigating unrelated bugs ([a transaction-view filter
bug](TRANSACTION_VIEW_FILTER_BUG.md) and a [Psalm `TaintedHtml`
investigation](TAINTEDHTML_VENDOR_CLASS_LIMITATION.md)), not by deliberately
documenting the architecture — this page exists so the next question like
"how does this codebase prevent XSS?" or "how does the table prefix system
actually work?" has a direct answer instead of requiring another bug
investigation to rediscover.

## Input escaping: every request value is HTML-escaped twice before display

### Layer 1: `html_cleanup()` escapes the request superglobals themselves

`includes/session.inc` (included at or near the top of essentially every
page) calls a function that recursively walks an array by reference and
runs every scalar through `html_specials_encode()` (a thin
`htmlspecialchars()` wrapper):

```php
function html_cleanup(array &$parms): void
{
    foreach($parms as $name => $value) {
        if (is_array($value))
            html_cleanup($parms[$name]);
        else
            $parms[$name] = html_specials_encode($value);
    }
}
...
html_cleanup($_GET);
html_cleanup($_POST);
html_cleanup($_REQUEST);
html_cleanup($_SERVER);
```

So `$_GET`, `$_POST`, `$_REQUEST` and `$_SERVER` are HTML-escaped **in
place, in full, before a single line of page-specific code runs.** This
includes the classic `PHP_SELF`-reflected-XSS vector
(`$_SERVER['PHP_SELF']` injected via `PATH_INFO`) — confirmed directly with
a `"><script>` payload injected via `PATH_INFO`, reflected back fully
escaped, by printing `$_SERVER['PHP_SELF']` immediately before and
immediately after `session.inc`'s `include` with nothing else running in
between.

**Known gap: `$_COOKIE` is not covered.** The one place in this codebase
that reads a cookie into a value which later reaches `$_POST` directly
(`reporting/includes/reports_classes.inc:172`,
`$_POST['PARAM_'.$cnt] = $_COOKIE['select'][$id][$cnt];`, restoring a saved
report parameter) does so *after* `html_cleanup($_POST)` has already run, so
that specific value bypasses this layer entirely. `reporting/` is already
out of Psalm's scope in this migration (`psalm.xml`'s `ignoreFiles`, "parked
while focusing elsewhere" — see [PSALM_MIGRATION.md](PSALM_MIGRATION.md)),
so this hasn't been investigated further; worth a look for whoever picks
`reporting/` back up.

### Layer 2: `db_escape()` re-escapes on the way into the database

`db_escape()` (`includes/db/connect_db_mysqli.inc`) calls
`html_specials_encode()` *before* its SQL-escaping step:

```php
function db_escape(string|int|float|bool|null $value = "", ?bool $nullify = false): string
{
    global $db;
    $value = @html_entity_decode((string) $value, ENT_QUOTES, ...);
    $value = html_specials_encode($value);   // <- HTML-escapes here, on the way IN
    ...
    $value = "'" . mysqli_real_escape_string($db, $value) . "'"; // then SQL-escapes
```

So this codebase escapes HTML **on write**, not on read — the opposite of
the usual "escape at output" convention — and does it a second time on top
of layer 1 above (largely redundant for the common case where the value
being written originated from user input, but it's also the only layer
protecting a value that *didn't* come through `$_GET`/`$_POST`/`$_REQUEST`,
such as one built from a config value or another table's data). This is why
plain output primitives like `label_cell()`/`checkbox()`/`hidden()` don't
escape their arguments themselves: by the time any value reaches them —
whether straight from a request or round-tripped through the database — it
has already been escaped at least once, usually twice.

Confirmed empirically, not just by reading the source: added a shipping
company with `shipper_name`/`contact` set to `<script>bad()</script>`
through the real form (`admin/shipping_companies.php`), then read back both
the raw database row and the rendered list page. The payload was stored as
`"Smith &amp; Sons O&#039;Brien"`-style encoding (using a different example
value) — already escaped in the database itself, not decoded back to plain
text on display.

### Why none of this gets suppressed in Psalm

Psalm's taint analysis doesn't trust either layer — correctly, if
conservatively. `html_cleanup()`'s shape (mutate a superglobal by reference,
recursively) isn't something `@psalm-taint-escape` can express at all (that
annotation is for a function that takes a tainted value and *returns* an
untainted one, not one that mutates a global and leaves every future read
of it implicitly safe). And even where Psalm's taint model *could* be made
to trust a write path (`db_escape()`), doing so project-wide would be the
same kind of blanket suppression this migration has already decided against
for the analogous `TaintedFile`/`TaintedSSRF`-via-database-round-trip case
(see [PSALM_MIGRATION.md](PSALM_MIGRATION.md)) — it would stop Psalm from
flagging a genuinely different future bug where some value reaches HTML
output without ever passing through a request superglobal or `db_escape()`
at all (an external API response, a config file value, the `$_COOKIE` gap
above). A write path that skipped `db_escape()` would also separately show
up as a `TaintedSql` finding on the same line, since `db_escape()` is this
codebase's only mechanism for SQL escaping too — so a real stored-XSS bug
here would already be worth fixing as a SQL-injection bug first, via this
migration's existing `TaintedSql`-hunting discipline.

One confirmed non-issue worth noting for anyone extending the `combo_input()`
pattern: `Html::encode()`/`Html::encodeAttribute()` calls in this codebase
always pass `doubleEncode: false`, and `htmlspecialchars(...,
double_encode: false)` leaves already-valid entities like `&amp;` alone —
so escaping an already-escaped value through this path doesn't double-encode
it. Confirmed with an isolated test reproducing the exact stored-value
format.

## `TB_PREF`: a placeholder token, substituted at query time — not a real per-company constant

FrontAccounting supports multiple companies against possibly-different
table prefixes (`config_db.php`'s `$db_connections[$company]['tbpref']`).
`TB_PREF` is **not** the active company's real prefix — it's defined once,
globally, as a literal placeholder token
(`includes/current_user.inc`):

```php
define('TB_PREF', '&TB_PREF&');
```

Every `TB_PREF.'tablename'` expression anywhere in the codebase builds a
plain PHP string containing the literal text `&TB_PREF&`, e.g.
`"&TB_PREF&suppliers"`. The *real* substitution happens later, exactly
once, inside `db_query()` (`includes/db/connect_db_mysqli.inc`):

```php
$comp = isset(session_obj('wa_current_user')->cur_con) ? session_obj('wa_current_user')->cur_con : 0;
$cur_prefix = @$db_connections[$comp]['tbpref'];
$sql = str_replace(TB_PREF, $cur_prefix, $sql);
```

— the active company's real prefix is looked up fresh and substituted into
the *finished SQL string*, immediately before every single query executes.

**Practical consequence:** any PHP-level string comparison involving
`TB_PREF.'x'` (e.g. `$table_name == TB_PREF.'journal'`) happens entirely
*before* this substitution, comparing values that both still carry the same
unsubstituted `&TB_PREF&` token. Which company is active, or what its
`tbpref` is, never enters into it — such a comparison is correct for a
single-company install and for any future multi-company setup with
arbitrary/differing prefixes, by construction. This was the deciding fact
in confirming the fix for the [transaction-view filter
bug](TRANSACTION_VIEW_FILTER_BUG.md) holds regardless of per-company
prefixes.

**Side effect worth knowing when reading Psalm output:** Psalm's
`allConstantsGlobal` setting resolves `TB_PREF` to its literal `define()`'d
value, so every `TB_PREF.'x'` concatenation Psalm sees really does type as
the literal string `'&TB_PREF&x'`. A hand-written docblock enumerating
table-name literals (e.g. `get_systype_db_info()`'s return type) will show
entries like `'&TB_PREF&bank_accounts'|'&TB_PREF&bank_trans'|...` rather
than anything resembling a real table name — not a bug, just a faithful
reflection of how this placeholder mechanism actually works.
