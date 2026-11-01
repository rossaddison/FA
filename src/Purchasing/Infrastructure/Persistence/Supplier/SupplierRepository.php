<?php

declare(strict_types=1);

namespace FrontAccounting\Purchasing\Infrastructure\Persistence\Supplier;

use Cycle\ORM\Select\Repository;

// Not generically parameterized as Repository<Supplier> here: Cycle's own
// RepositoryInterface declares "@template TEntity of object" without
// @template-covariant, so Psalm treats it as invariant -- a class-string for
// Repository<Supplier> doesn't satisfy the Entity attribute's own
// class-string<RepositoryInterface<object>> constructor parameter type,
// even though Supplier is-a object and this repository is read-only safe.
// Using the exact upper-bound type parameter instead avoids that mismatch.
/**
 * @extends Repository<object>
 */
final class SupplierRepository extends Repository
{
}
