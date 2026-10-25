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
 * SessionInterface backed directly by the live $_SESSION superglobal -
 * reads and writes the same array every raw $_SESSION[...] access in the
 * rest of the codebase sees, so this can be adopted key by key without
 * any migration step.
 */
final class NativeSession implements SessionInterface
{
    /** @psalm-impure */
    #[\Override]
    public function has(string $key): bool
    {
        return isset($_SESSION[$key]);
    }

    /** @psalm-impure */
    #[\Override]
    public function get(string $key): mixed
    {
        return $_SESSION[$key] ?? null;
    }

    /** @psalm-impure */
    #[\Override]
    public function getTyped(string $key, string $type): object
    {
        // get()'s whole point is to return an untyped value for this method to narrow
        $value = $this->get($key);
        if (!$value instanceof $type) {
            throw new \RuntimeException(sprintf(
                "Session key '%s' expected to be an instance of %s, got %s",
                $key,
                $type,
                is_object($value) ? get_class($value) : gettype($value)
            ));
        }
        return $value;
    }

    #[\Override]
    public function set(string $key, mixed $value): void
    {
        // set()'s whole point is accepting an arbitrary value to store
        $_SESSION[$key] = $value;
    }

    #[\Override]
    public function remove(string $key): void
    {
        unset($_SESSION[$key]);
    }
}
