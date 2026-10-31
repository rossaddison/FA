<?php

declare(strict_types=1);

namespace FrontAccounting\Taxes\Infrastructure\Persistence\TaxType;

use FrontAccounting\Infrastructure\Persistence\Trait\RequireId;
use Cycle\Annotated\Annotation\Column;
use Cycle\Annotated\Annotation\Entity;
use Cycle\Annotated\Annotation\Relation\BelongsTo;

#[Entity(repository: \FrontAccounting\Taxes\Infrastructure\Persistence\TaxType\TaxTypeRepository::class, table: 'tax_types')]
// Do NOT mark this class final: Cycle ORM generates a lazy-loading proxy
// subclass of every entity at runtime (visible as "<Entity> Cycle ORM
// Proxy" if you var_dump a hydrated instance) -- a final class can't be
// subclassed, which breaks Cycle's own proxy mechanism, not just some
// hypothetical external extender. Left as an accepted, visible
// ClassMustBeFinal finding rather than suppressed.
/** @psalm-external-mutation-free */
class TaxType
{
    use RequireId;

    #[Column(type: 'primary')]
    private ?int $id = null;

    /** @psalm-mutation-free */
    public function __construct(
        #[Column(type: 'double', nullable: false)]
        private float $rate = 0.0,

        #[Column(type: 'string(15)', nullable: false)]
        private string $sales_gl_code = '',

        #[Column(type: 'string(15)', nullable: false)]
        private string $purchasing_gl_code = '',

        #[Column(type: 'string(60)', nullable: false)]
        private string $name = '',

        #[Column(type: 'bool', typecast: 'bool', nullable: false, default: false)]
        private bool $inactive = false,

    ) {}

    /** @psalm-mutation-free */
    public function reqId(): int
    {
        return $this->requireId($this->id, 'TaxType');
    }

    /** @psalm-mutation-free */
    public function hasIdentity(): bool
    {
        return $this->id !== null;
    }

    /** @psalm-external-mutation-free */
    public function setId(int $id): void
    {
        $this->id = $id;
    }



    public function getRate(): float
    {
        return $this->rate;
    }

    /** @psalm-external-mutation-free */
    public function setRate(float $rate): void
    {
        $this->rate = $rate;
    }

    public function getSalesGlCode(): string
    {
        return $this->sales_gl_code;
    }

    /** @psalm-external-mutation-free */
    public function setSalesGlCode(string $sales_gl_code): void
    {
        $this->sales_gl_code = $sales_gl_code;
    }

    public function getPurchasingGlCode(): string
    {
        return $this->purchasing_gl_code;
    }

    /** @psalm-external-mutation-free */
    public function setPurchasingGlCode(string $purchasing_gl_code): void
    {
        $this->purchasing_gl_code = $purchasing_gl_code;
    }

    public function getName(): string
    {
        return $this->name;
    }

    /** @psalm-external-mutation-free */
    public function setName(string $name): void
    {
        $this->name = $name;
    }

    public function getInactive(): bool
    {
        return $this->inactive;
    }

    /** @psalm-external-mutation-free */
    public function setInactive(bool $inactive): void
    {
        $this->inactive = $inactive;
    }
}
