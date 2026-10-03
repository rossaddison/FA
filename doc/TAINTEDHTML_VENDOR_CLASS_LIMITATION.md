# Why `Html::encode()`/`Html::encodeAttribute()` are no longer called directly

`includes/ui/ui_lists.inc`'s `combo_input()` and `array_selector()` — the two
shared functions behind essentially every dropdown/combo widget in the
application (`customer_list()`, `supplier_list()`, and dozens of others all
funnel through one or the other) — used to call Yiisoft's
`Html::encode()`/`Html::encodeAttribute()` directly to escape database-sourced
option text. Both genuinely call `htmlspecialchars()` (confirmed by reading
the vendor source in `vendor/yiisoft/html/src/Html.php`) and were already
being used correctly — `encode()` for plain option text, `encodeAttribute()`
for quoted attribute values — but Psalm's taint analysis still flagged every
value that passed through them as unescaped (`TaintedHtml`/
`TaintedTextWithQuotes`), 229 findings project-wide, concentrated in
`ui_lists.inc` and the many files that call into it.

## The Psalm limitation

Annotating a vendor class's method with `@psalm-taint-escape` via a stub file
(`psalm.xml`'s `<stubs>`, the mechanism this project already uses for a
handful of first-party sanitizers — see [PSALM_MIGRATION.md](PSALM_MIGRATION.md)'s
"Missing `@psalm-taint-escape` annotations on real sanitizers" section) does
**not** work for an externally-called vendor method, confirmed empirically
with an isolated test before touching this codebase: a stub correctly parses
and registers, and even a deliberately-wrong return type in it goes
completely unnoticed by Psalm, proving the stub's information is never
merged onto the real, autoloaded class at all.

This is a confirmed, currently open upstream bug:
[vimeo/psalm#11752](https://github.com/vimeo/psalm/issues/11752) — "Taint
analysis: autoloaded classes / methods are not safe for
`@psalm-taint-escape` except in the class itself." Psalm can't be certain
which physical file actually provided an autoloaded class (composer
resolves ambiguous/duplicate class names alphabetically, and
`spl_autoload_register`'s "prepend" option can reorder autoloaders at
runtime), so it only trusts a `@psalm-taint-escape` tag on a class method
when the call happens from *inside that same class*. Every real call site
in this codebase is external (`Html::encode(...)` from a plain global
function), so the tag could never take effect no matter how the stub was
written.

## The fix

Two small first-party wrapper functions, `html_encode()` and
`html_encode_attribute()` (`includes/ui/ui_lists.inc`, right before
`combo_input()`), each a one-line passthrough to the corresponding
`Html::` method with an identical signature. Being plain global functions —
not vendor class methods — `@psalm-taint-escape` on them isn't subject to
the autoload-ambiguity restriction at all, matching this project's already-
proven pattern for `db_escape()`/`date2sql()`/`clean_file_name()`/
`html_specials_encode()`.

The two taint types are assigned by actually reading each method's escaping
flags, not copied blindly:
- `html_encode()` → `@psalm-taint-escape html` only. `Html::encode()` uses
  `ENT_NOQUOTES`, so it does *not* escape quote characters — safe for plain
  HTML text content, never for a quoted attribute value.
- `html_encode_attribute()` → both `@psalm-taint-escape html` and
  `@psalm-taint-escape has_quotes`. `Html::encodeAttribute()` uses
  `ENT_QUOTES` plus escapes tab/space/`=`/backtick, safe for text and for
  both quoted and unquoted attribute values.

All direct `Html::encode(...)`/`Html::encodeAttribute(...)` call sites in
`combo_input()`/`array_selector()` were replaced with the wrappers. Fixing
these two functions resolved almost the entire 229-finding category in one
pass (down to 95 remaining, all in other files/functions that have their
own, separate unescaped-output issues — see below) — confirming that the
vast majority of this codebase's `TaintedHtml` volume traced back to just
these two shared functions once the vendor-class limitation itself was
worked around.

## A real bug this surfaced along the way

With the two `Html::` calls no longer the bottleneck, Psalm's taint tracing
reached one line further and surfaced a second, genuinely separate issue in
`combo_input()`'s popup-search-icon block: `$theme = user_theme();` (a
stored user preference, set via `post_scalar('theme')` in
`admin/display_prefs.php`) was concatenated raw into
`<img src="...themes/$theme/images/...">` with no escaping at all. This one
fix alone (wrapping it in `html_encode_attribute()`) dropped the
project-wide Psalm total by 134 — far more than the `Html::` wrapper change
itself — since this code path is shared by every combo widget across the
whole application. Not confirmed exploitable (themes are normally chosen
from a fixed, installed-themes dropdown), but fixed as real defense in
depth regardless, the same judgment call as `delete_attachments()`'s
`basename()` hardening elsewhere in this migration.

## More of the same pattern, fixed the same way

Follow-up pass: `includes/ui/ui_input.inc` had three more
`user_theme()`-into-`<img src>` sites (`submit()`'s optional button icon,
`set_icon()` — the single shared icon-rendering helper used throughout the
UI, and `date_cells()`'s calendar icon), all fixed with the same
`html_encode_attribute()` wrapper. While live-testing this fix, found and
fixed a genuinely unrelated bug surfaced along the way:
`admin/db/shipping_db.inc`'s `add_shipper()`/`update_shipper()` declared
`$contact` as `array` while every real caller passed a plain string, so
*every* add/update crashed outright under `strict_types=1` — see
[BUGS_FOUND.md](BUGS_FOUND.md)'s "Live crashes" section.

## Why most of what's left isn't a `combo_input()`-style fix

The remaining findings are concentrated in a different kind of function —
`label_cell()`, `checkbox()`, `hidden()`, and similar primitives in
`ui_input.inc`/`ui_controls.inc`/`ui_msgs.inc`/`ui_view.inc` — and these
can't be fixed the same way. Confirmed by reading their source:
`label_cell()` is literally `echo "<td $params>$label</td>\n";` — no
escaping of `$label` at all, by design. Callers routinely rely on that to
pass pre-built HTML through, e.g. `email_cell()`:
`label_cell("<a href='mailto:$label'>$label</a>", ...)`. Wrapping
`$label` in `html_encode()` inside `label_cell()` itself would silently
break every caller like this — the `<a>` tag would render as visible text
instead of a link. These functions are intentionally dual-purpose; the
escaping decision has to be made by each *caller*, not the shared
primitive.

That raised an obvious question: with dozens of call sites across the
codebase passing raw database fields straight to `label_cell()` —
`label_cell($myrow["shipper_name"])`, `label_cell($myrow["description"])`,
`label_cell($myrow["address"])`, and many more — is this a live, exploitable
stored-XSS surface? Tested directly rather than guessing: added a shipping
company with `shipper_name`/`contact` set to `<script>bad()</script>`
through the real form (`admin/shipping_companies.php`), then read back both
the raw database row and the rendered list page.

**The payload was already HTML-escaped in the database itself** —
`"Smith &amp; Sons O&#039;Brien"`-style encoding, stored that way, not
decoded back to plain text on display. The cause: `db_escape()`
(`includes/db/connect_db_mysqli.inc`) calls `html_specials_encode()`
*before* its SQL-escaping step:

```php
function db_escape(string|int|float|bool|null $value = "", ?bool $nullify = false): string
{
    global $db;
    $value = @html_entity_decode((string) $value, ENT_QUOTES, ...);
    $value = html_specials_encode($value);   // <- HTML-escapes here, on the way IN
    ...
```

So this codebase escapes HTML **on write**, not on read — the opposite of
the usual "escape at output" convention. `label_cell()` and friends don't
need to escape on the way out because, for any field written through
`db_escape()`, the value is already safe by the time it comes back. This
also confirms the earlier `combo_input()` fix didn't introduce
double-escaping: `Html::encode()`/`encodeAttribute()` are always called
there with `doubleEncode: false` (preserved from the original code, not
something this fix added), and `htmlspecialchars(..., double_encode: false)`
leaves already-valid entities like `&amp;` alone — confirmed with an
isolated test reproducing the exact stored value.

**There's an even more fundamental layer underneath this, found by tracing
the escaping back one step further than `db_escape()`.** The shipping-company
test above actually proved more than write-time DB escaping: the `$_POST`
value was *already* HTML-escaped the moment it arrived, before
`add_shipper()`/`db_escape()` ever touched it. Confirmed directly: added a
temporary debug line printing `$_SERVER['PHP_SELF']` immediately before and
immediately after `admin/shipping_companies.php`'s one `include
(".../includes/session.inc")` call — raw and unescaped before, fully
HTML-entity-encoded after, with no other code running in between (removed
once confirmed; not part of this fix).

The mechanism: `includes/session.inc` (included at or near the top of
essentially every page in the application) calls a function that recursively
walks an array by reference and runs every scalar through
`html_specials_encode()`:

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

So this isn't just "`db_escape()` happens to escape HTML too" — the
application HTML-escapes **every value in every incoming request**
(`$_GET`, `$_POST`, `$_REQUEST`, `$_SERVER`, which covers the classic
`PHP_SELF`-reflected-XSS vector too — tested directly with a
`"><script>` payload injected via `PATH_INFO`, confirmed reflected back
fully escaped) before a single line of page-specific code runs. `db_escape()`
re-escaping on the way into the database is effectively a second,
largely-redundant safety net on top of this first one, for the common case
where the value being written came from user input in the first place.

One gap worth naming precisely, for completeness, not as something to fix
here: `html_cleanup()` does **not** cover `$_COOKIE`. The one place this
codebase reads a cookie into a value that later reaches `$_POST` directly
(`reporting/includes/reports_classes.inc:172`,
`$_POST['PARAM_'.$cnt] = $_COOKIE['select'][$id][$cnt];`, restoring a saved
report parameter) does so *after* `html_cleanup($_POST)` has already run,
so that specific value bypasses this layer. `reporting/` is already entirely
out of Psalm's scope in this migration (`psalm.xml`'s `ignoreFiles`,
"parked while focusing elsewhere" — see
[PSALM_MIGRATION.md](PSALM_MIGRATION.md)), so this isn't investigated
further here; noted for whoever eventually picks `reporting/` back up.

**Why this isn't something to blanket-fix or blanket-suppress.** The
`TaintedHtml` findings on `label_cell()`/`checkbox()`/`hidden()` are Psalm
correctly (if conservatively) not trusting this global sanitization layer —
reasonably so, since `html_cleanup()`'s shape (mutate an array by reference,
recursively, in place) isn't something `@psalm-taint-escape` is designed to
express at all (that annotation is for a function that takes a tainted value
and *returns* an untainted one, not one that mutates a superglobal and
leaves every future read of it implicitly safe). Even if Psalm could express
it, blanket-trusting it project-wide would be exactly the kind of
suppression this migration has already decided against for the analogous
`TaintedFile`/`TaintedSSRF`-via-database-round-trip case
([PSALM_MIGRATION.md](PSALM_MIGRATION.md)) — it would stop Psalm from
flagging a *different* future bug where some value reaches HTML output
without ever passing through `$_GET`/`$_POST`/`$_REQUEST`/`$_SERVER` or
`db_escape()` at all (an external API response, a config file value, the
`$_COOKIE` gap just described). A write path that skips `db_escape()` would
also separately show up as a `TaintedSql` finding on the same line, since
`db_escape()` is this codebase's only mechanism for SQL escaping too — so a
real stored-XSS bug here would already be worth fixing as a SQL-injection
bug first, via this migration's existing `TaintedSql`-hunting discipline.

## What's left

87 `TaintedHtml`/`TaintedTextWithQuotes` findings remain (confirmed by a
fresh full-project scan, not assumed), almost entirely in
`label_cell()`/`checkbox()`/`hidden()`-style dual-purpose primitives:
`ui_input.inc` (44), `ui_controls.inc` (20), `ui_msgs.inc` (8),
`ui_view.inc` (4), plus a handful of page-level files
(`purchasing/includes/ui/invoice_ui.inc`, `includes/dashboard.inc`,
`includes/page/header.inc`, `includes/page/footer.inc`,
`includes/db_pager.inc`, `admin/db/maintenance_db.inc` — 1-2 each). Per the
reasoning above, these are expected, not a to-do list to clear — the
productive next step for any of them is the same discipline as this
migration's existing `TaintedSql` hunting: trace one specific call site's
*write* path and confirm `db_escape()` (or equivalent) is actually used,
treating a genuine gap as the SQL-injection-class bug it would be, rather
than trying to patch the shared output primitive.
