# Known upstream Psalm bugs/limitations affecting this project

A running list of confirmed, currently-open gaps in Psalm itself (not in
this codebase) that were each independently rediscovered during this
migration before being traced to an existing upstream issue. Kept as its
own page so the next person who hits one of these symptoms can find the
root cause in one search instead of re-deriving it from scratch — each of
these took a real isolated-test investigation to pin down the first time.

Project version: Psalm `7.0.0-beta21` (see `composer.json`). Re-check this
page's entries after any Psalm upgrade — an item may have shipped a fix.

## `@psalm-taint-escape` on a vendor class method is silently ignored for external calls

**[vimeo/psalm#11752](https://github.com/vimeo/psalm/issues/11752)** —
"Taint analysis: autoloaded classes / methods are not safe for
`@psalm-taint-escape` except in the class itself." Open.

**Symptom:** a stub file (`psalm.xml`'s `<stubs>`) adding
`@psalm-taint-escape` to a third-party class's method parses and registers
fine, but has zero effect on taint findings for any call made from outside
that class — confirmed by giving the stub a deliberately wrong return type
and watching Psalm not notice, proving the stub's info never merges onto
the real, autoloaded class. Psalm only trusts the tag when the call happens
from *inside* the same class, since it can't be certain which physical file
actually provided an autoloaded class (composer resolves ambiguous/
duplicate class names alphabetically, and `spl_autoload_register`'s
"prepend" option can reorder autoloaders at runtime).

**Where this hit us:** `Yiisoft\Html\Html::encode()`/`encodeAttribute()`
(genuinely `htmlspecialchars()`-based, already used correctly) — every
external call site still showed `TaintedHtml`/`TaintedTextWithQuotes`.

**Workaround:** route the call through a small first-party wrapper
*function* instead of calling the vendor method directly — a plain global
function isn't a class method, so it isn't subject to this restriction at
all. See `html_encode()`/`html_encode_attribute()`
(`includes/ui/ui_lists.inc`) and the full writeup in
[TAINTEDHTML_VENDOR_CLASS_LIMITATION.md](TAINTEDHTML_VENDOR_CLASS_LIMITATION.md).
This is now the established pattern for *any* future vendor sanitizer this
project wants Psalm to trust.

## Pipe operator (`|>`) infers as `mixed`

**[vimeo/psalm#11865](https://github.com/vimeo/psalm/issues/11865)** —
"PHP 8.5: support pipe operator." Open since 2026-05-29, no PR yet as of
last check.

**Symptom:** `$array |> array_keys(...)` parses without a syntax error, but
Psalm can't infer a type through the pipe — the result is treated as
`mixed`, producing a `MixedReturnStatement`/`MixedAssignment` wherever it's
used. Confirmed with the exact minimal repro posted in the issue
(`$array |> array_keys(...)` inside a `@psalm-pure` function with a
declared `list<string>` return type).

**Where this would hit us:** hasn't been adopted anywhere in this codebase
yet, specifically *because* of this gap — see
[PHP85_FEATURE_CANDIDATES.md](PHP85_FEATURE_CANDIDATES.md)'s "Tooling
caveat" section. Adopting the pipe operator anywhere Psalm still analyzes
(everywhere except `src/`) would reintroduce `mixed` into a codebase this
migration has spent thousands of fixes eliminating.

**Workaround:** none needed yet — simply not adopting the feature outside
`src/`, where PHPStan (which infers through pipe chains correctly,
confirmed empirically) is the tool of record instead. No PR exists upstream
yet, so there's no fix to watch for beyond re-testing after each Psalm
release.

## `array_first()`/`array_last()` infer as `mixed` — fix in progress upstream

Not yet a single tracked issue, but two competing PRs exist:
[vimeo/psalm#11883](https://github.com/vimeo/psalm/pull/11883) (template-
signature approach, June 2026) was superseded by
[vimeo/psalm#12067](https://github.com/vimeo/psalm/pull/12067) (return-type-
provider approach, per maintainer feedback on #11883) — `[6.x] Infer types
for array_first, array_last, array_any and array_all, and sharpen
array_pop/array_shift`. As of last check, #12067 is still a draft with no
reviews.

**Symptom:** same shape as the pipe operator above — `array_first($a)`/
`array_last($a)` return `mixed` instead of the array's actual value type,
confirmed with an isolated test.

**Where this would hit us:** same as the pipe operator — catalogued as a
candidate in
[PHP85_FEATURE_CANDIDATES.md](PHP85_FEATURE_CANDIDATES.md) but not adopted
outside `src/` for the same `mixed`-reintroduction reason.

**What to watch for:** once `#12067` (or whatever supersedes it) merges and
ships in a Psalm release, re-run the isolated test from
`PHP85_FEATURE_CANDIDATES.md` before adopting the feature project-wide —
don't assume the shipped behavior matches the draft PR's description
exactly.

## For contrast: an upstream issue that *was* already fixed

**[vimeo/psalm#11381](https://github.com/vimeo/psalm/issues/11381)** —
"Add support for `#[\NoDiscard]`" — closed/merged. Confirmed empirically
that Psalm `7.0.0-beta21` already handles this PHP 8.5 attribute correctly,
with no inference gap. This is why `#[\NoDiscard]` was the one feature
adopted codebase-wide immediately (60 functions — see
[PHP85_FEATURE_CANDIDATES.md](PHP85_FEATURE_CANDIDATES.md)) while the other
three stayed catalog-only. Listed here as the control case: not every
PHP 8.5/Psalm interaction surveyed during this migration turned out to be a
live bug.
