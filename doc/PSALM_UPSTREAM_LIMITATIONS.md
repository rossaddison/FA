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

## A `psalm.xml` `<globals>`-declared variable resists local narrowing under its own name

No upstream issue number found/filed for this one yet - noted here from a
direct isolated test, not a citation.

**Symptom:** `admin/attachments.php` reads the `selected_id` global
(declared in `psalm.xml` as `string|int|float|bool|array<array-key,
mixed>|null`) and passes it to several functions requiring
`string|int|float|bool|null` (no array). The usual narrowing idiom used
successfully elsewhere this migration -
`$selected_id = is_scalar($selected_id) ? $selected_id : null;` (either
inline at each call site or as a single reassignment near the top of the
script) - left every one of those call sites still flagged
`MixedArgument ... cannot be mixed|null`, confirmed across three separate
full-project `--no-cache` rescans (inline ternary, top-of-file
reassignment via `is_array()` negation, top-of-file reassignment via
`is_scalar()`). Assigning the exact same narrowing expression to a
brand-new local variable name instead (`$att_id = is_scalar($selected_id)
? $selected_id : null;`, then using `$att_id` at the call sites) resolved
every one of them in the next rescan, with no other code change. The
narrowing logic itself was never the problem - only reusing the
`<globals>`-declared name for the narrowed value was.

**Where this hit us:** `admin/attachments.php`'s `get_attachment()`/
`update_attachment()`/`delete_attachment()`/`hidden()` call sites keyed
on `selected_id`. Likely affects any `psalm.xml`-declared global reused
as its own narrowed name, not just this one - `gl/manage/bank_accounts.php`
earlier this session worked around the same global (there as the source
for a differently-named local, `$bank_id`) without ever hitting this,
which in hindsight is consistent with this theory rather than a
counterexample.

Confirmed again independently in `admin/inst_lang.php`, while resolving
an unrelated `handle_submit()` cross-file name collision there: narrowing
and reassigning `$selected_id` to its own name
(`$selected_id = is_scalar($selected_id) ? (int) $selected_id : -1;`,
placed right after `simple_page_mode(true)`) left `handle_submit_inst_lang()`'s
and `display_language_edit()`'s call sites still flagged `MixedArgument`/
`PossiblyInvalidArgument`, confirmed via a full `--no-cache` rescan.
Switching to a differently-named local (`$lang_id = is_scalar($selected_id)
? $selected_id : null;`, used at all three call sites instead of
`$selected_id`) resolved all three findings in the very next rescan, no
other change. Same root cause, same fix, independently rediscovered -
reinforces this is a general property of the global's *name*, not
something specific to `admin/attachments.php`.

**Workaround:** narrow a `psalm.xml`-declared global into a differently-
named local variable before passing it anywhere Psalm needs the narrower
type; don't reassign the global's own name and expect the narrowing to
stick.

## `@psalm-suppress TaintedInclude` doesn't suppress on at least one confirmed-safe sink

No upstream issue number found/filed for this one yet - noted here from a
direct isolated test, not a citation.

**Symptom:** `frontaccounting.php`'s `display()` has a `TaintedInclude`
finding on `include_once($path_to_root . "/themes/".user_theme()."/renderer.php")`
that a `@psalm-suppress TaintedInclude ...` annotation does not remove from
the project's Psalm report, confirmed via two full `--no-cache` rescans -
one with the annotation directly above the `include_once` statement, one
with it moved to the enclosing `display()` method's own docblock. Both
left the exact same `TaintedInclude` entry in the report, at the same
line. This is not a project-config issue: the identical
`@psalm-suppress TaintedFile confirmed safe, ...` pattern on
`admin/db/maintenance_db.inc`'s `create_comp_dirs()` (four occurrences, see
BUGS_FOUND.md's "Path traversal / arbitrary file write" section) reliably
suppresses those findings in the same rescans - so plain `@psalm-suppress`
handling isn't broken project-wide, just seemingly not honored for this
specific `TaintedInclude` sink in this case.

**Where this hit us:** the dynamically-loaded theme renderer include - see
BUGS_FOUND.md's "Path traversal / arbitrary file write" section for why
it's confirmed safe despite the finding.

**Workaround:** none found. Left as a plain explanatory comment instead of
a suppression; the finding stays in the project's Psalm count, which is
why this file's post-cleanup count (see its commit message) is one higher
than it would otherwise be.

## `$_SERVER`'s inferred array shape is non-deterministic between scans

No upstream issue number found/filed for this one yet - noted here from a
direct isolated test, not a citation.

**Symptom:** two back-to-back `--no-cache` full-project rescans of the
exact same, unchanged committed code produce a different set of
`PossiblyUndefinedArrayOffset` findings on `$_SERVER[...]` reads (`HTTPS`,
`SERVER_NAME`, `PHP_SELF`, `HTTP_USER_AGENT`, and others), scattered across
dozens of unrelated files. Confirmed this isn't caused by code changes:
diffing two reports taken moments apart, with nothing in between, still
shows ~50 of these findings appearing/disappearing. Inspecting the
reported array shape directly shows why - it includes a *literal* key
like `REQUEST_TIME?: int<1792358928, max>`, an actual Unix timestamp
baked into the type. Psalm is inferring `$_SERVER`'s shape from the real
`$_SERVER` superglobal of the CLI process running Psalm itself, not from a
fixed stub - so the set of keys considered "known present" shifts with
whatever the OS/webserver/CLI environment happened to populate at the
moment that specific `psalm.bat` invocation started.

**Where this hit us:** first noticed while verifying
`inventory/manage/items.php`'s Psalm cleanup commit - a `check_value()`
signature change was wrongly suspected of rippling into unrelated files
like `access/login.php` and `admin/inst_chart.php` purely because of a
batch of these findings appearing in the diff; re-running the scan without
that change present reproduced the same findings, proving they were never
related.

**Workaround:** when diffing two Psalm reports to check for regressions,
exclude any finding whose message contains `$_SERVER` before comparing -
otherwise this noise will look like a real regression (or mask one) at
random. There is no way to make the array shape stable short of stubbing
`$_SERVER` explicitly in `psalm.xml`, which hasn't been attempted since
the existing per-file `post_scalar()`/`get_post()` idiom already avoids
reading `$_SERVER` unguarded in application code - only a handful of
low-level files (`access/login.php`, session bootstrap code) touch it
directly, and they predate this migration's Psalm pass.

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
