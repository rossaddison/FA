<?php

declare(strict_types=1);

namespace FrontAccounting\Taxes\Infrastructure\Persistence\TaxType;

use Cycle\ORM\Select\Repository;

/**
 * Parameterized as Repository<object>, not Repository<TaxType>: Cycle's own
 * RepositoryInterface declares "@template TEntity of object" without
 * @template-covariant, so Psalm treats it as invariant -- a class-string for
 * Repository<TaxType> doesn't satisfy the Entity attribute's own
 * class-string<RepositoryInterface<object>> constructor parameter type,
 * even though TaxType is-a object and this repository is read-only safe.
 * Using the exact upper-bound type parameter instead avoids that mismatch.
 * @extends Repository<object>
 */
final class TaxTypeRepository extends Repository
{
}
