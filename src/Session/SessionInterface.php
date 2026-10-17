<?php

declare(strict_types=1);
/**********************************************************************
    Copyright (C) FrontAccounting, LLC.
    Released under the terms of the GNU General Public License, GPL,
    as published by the Free Software Foundation, either version 3
    of the License, or (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the License here <http://www.gnu.org/licenses/gpl-3.0.html>.
***********************************************************************/

namespace FrontAccounting\Session;

/**
 * A thin, typed facade over $_SESSION.
 *
 * Deliberately NOT a session *storage* abstraction - FrontAccounting's
 * native PHP session (bootstrapped by includes/session.inc's
 * SessionManager) stays the source of truth, including the live objects
 * already stored in it (carts, current_user, ...). This interface exists
 * so individual, newly-touched call sites can read/write specific keys
 * with real types instead of the project's usual
 * string|int|float|bool|array|null|mixed spread, one key at a time,
 * without migrating session.inc's own bootstrap or any of the ~370
 * existing raw $_SESSION[...] accesses project-wide.
 *
 * Every method reads or writes the mutable $_SESSION superglobal, which
 * is impure by definition even for the read-only methods (the value can
 * change between two reads within the same request, e.g. after a set()
 * elsewhere) - no method here is a pure function of its own arguments.
 *
 * @psalm-mutable
 */
interface SessionInterface
{
    /** @psalm-impure */
    public function has(string $key): bool;

    /** @psalm-impure */
    public function get(string $key): mixed;

    /**
     * Read a key and assert it is an instance of $type, matching the
     * project's established pattern of storing live objects (carts,
     * current_user, ...) directly in the session. Throws rather than
     * silently returning a wrongly-typed value, since a session key
     * holding the wrong class is itself a real bug worth surfacing.
     *
     * @template T of object
     * @param class-string<T> $type
     * @return T
     * @throws \RuntimeException if the key is missing or not an instance of $type
     * @psalm-impure
     */
    public function getTyped(string $key, string $type): object;

    /** @psalm-impure */
    public function set(string $key, mixed $value): void;

    /** @psalm-impure */
    public function remove(string $key): void;
}
