<?php

declare(strict_types=1);

namespace FrontAccounting\Currency\Infrastructure\Persistence\Currencie;

use Cycle\ORM\Select\Repository;

// See SupplierRepository.php's own comment: not parameterized as
// Repository<Currencie> for the same Psalm invariant-generics reason.
/**
 * @extends Repository<object>
 */
final class CurrencieRepository extends Repository
{
}
