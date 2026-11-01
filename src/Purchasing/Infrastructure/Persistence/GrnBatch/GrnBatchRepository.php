<?php

declare(strict_types=1);

namespace FrontAccounting\Purchasing\Infrastructure\Persistence\GrnBatch;

use Cycle\ORM\Select\Repository;

// See SupplierRepository.php's own comment: not parameterized as
// Repository<GrnBatch> for the same Psalm invariant-generics reason.
/**
 * @extends Repository<object>
 */
final class GrnBatchRepository extends Repository
{
}
