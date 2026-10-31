<?php

declare(strict_types=1);

namespace FrontAccounting\Infrastructure\Persistence\Trait;

/** @psalm-immutable */
trait RequireId
{
    /** @psalm-pure */
    protected function requireId(?int $id, string $context): int
    {
        if ($id === null) {
            throw new \LogicException($context . ' not persisted');
        }

        return $id;
    }
}
