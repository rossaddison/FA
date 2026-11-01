<?php

declare(strict_types=1);

namespace FrontAccounting\Purchasing\Infrastructure\Persistence\GrnBatch;

use FrontAccounting\Purchasing\Infrastructure\Persistence\Supplier\Supplier;
use FrontAccounting\Infrastructure\Persistence\Trait\RequireId;
use Cycle\Annotated\Annotation\Column;
use Cycle\Annotated\Annotation\Entity;
use Cycle\Annotated\Annotation\Relation\BelongsTo;
use \DateTimeImmutable;

#[Entity(repository: \FrontAccounting\Purchasing\Infrastructure\Persistence\GrnBatch\GrnBatchRepository::class, table: 'grn_batch')]
// Do NOT mark this class final: Cycle ORM generates a lazy-loading proxy
// subclass of every entity at runtime (visible as "<Entity> Cycle ORM
// Proxy" if you var_dump a hydrated instance) -- a final class can't be
// subclassed, which breaks Cycle's own proxy mechanism, not just some
// hypothetical external extender. Left as an accepted, visible
// ClassMustBeFinal finding rather than suppressed.
/** @psalm-external-mutation-free */
class GrnBatch
{
    use RequireId;

    #[BelongsTo(target:Supplier::class, innerKey: 'supplier_id', outerKey: 'supplier_id', nullable: false, fkAction:'NO ACTION')]
    private ?Supplier $supplier = null;

    #[Column(type: 'primary', name: 'id')]
    private ?int $id = null;

    /** @psalm-mutation-free */
    public function __construct(
        #[Column(type: 'integer(11)', nullable: false)]
        private int $supplier_id = 0,

        #[Column(type: 'integer(11)', nullable: true)]
        private ?int $purch_order_no = null,

        #[Column(type: 'string(60)', nullable: false)]
        private string $reference = '',

        #[Column(type: 'date', nullable: false)]
        private \DateTimeImmutable|string|null $delivery_date = null,

        #[Column(type: 'string(5)', nullable: true)]
        private ?string $loc_code = '',

        #[Column(type: 'double', nullable: true)]
        private ?float $rate = null,

    ) {}

    /** @psalm-mutation-free */
    public function reqId(): int
    {
        return $this->requireId($this->id, 'GrnBatch');
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


    public function getSupplier(): ?Supplier
    {
        return $this->supplier;
    }

    /** @psalm-external-mutation-free */
    public function setSupplier(?Supplier $supplier): void
    {
        $this->supplier = $supplier;
    }


    /** @psalm-mutation-free */
    public function reqSupplierId(): int
    {
        return $this->requireId($this->supplier_id, 'GrnBatch supplier_id');
    }

    public function getSupplierId(): ?int
    {
        return $this->supplier_id;
    }

    /** @psalm-external-mutation-free */
    public function setSupplierId(int $supplier_id): void
    {
        $this->supplier_id = $supplier_id;
    }

    public function getPurchOrderNo(): ?int
    {
        return $this->purch_order_no;
    }

    /** @psalm-external-mutation-free */
    public function setPurchOrderNo(int $purch_order_no): void
    {
        $this->purch_order_no = $purch_order_no;
    }

    public function getReference(): string
    {
        return $this->reference;
    }

    /** @psalm-external-mutation-free */
    public function setReference(string $reference): void
    {
        $this->reference = $reference;
    }

    public function getDeliveryDate(): \DateTimeImmutable|string|null
    {
        return $this->delivery_date;
    }

    /** @psalm-external-mutation-free */
    public function setDeliveryDate(\DateTimeImmutable|string|null $delivery_date): void
    {
        $this->delivery_date = $delivery_date;
    }

    public function getLocCode(): ?string
    {
        return $this->loc_code;
    }

    /** @psalm-external-mutation-free */
    public function setLocCode(string $loc_code): void
    {
        $this->loc_code = $loc_code;
    }

    public function getRate(): ?float
    {
        return $this->rate;
    }

    /** @psalm-external-mutation-free */
    public function setRate(float $rate): void
    {
        $this->rate = $rate;
    }
}
