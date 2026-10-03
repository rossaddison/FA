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

## What's left

95 `TaintedHtml`/`TaintedTextWithQuotes` findings remain, in other
`includes/ui/*.inc` files (`ui_input.inc`, `ui_controls.inc`, `ui_msgs.inc`,
`ui_view.inc`) and a handful of page-level files
(`gl/view/gl_trans_view.php`, `purchasing/includes/ui/invoice_ui.inc`,
`includes/ui/class.reflines_crud.inc`). These are separate functions with
their own escaping gaps (not `combo_input()`/`array_selector()` callers,
which are now all clean) — not yet investigated, tracked here for whoever
picks this up next.
