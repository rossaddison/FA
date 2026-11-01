<?php

declare(strict_types=1);

namespace FrontAccounting\Purchasing\Infrastructure\Persistence\Supplier;

use FrontAccounting\Infrastructure\Persistence\Trait\RequireId;
use Cycle\Annotated\Annotation\Column;
use Cycle\Annotated\Annotation\Entity;
use Cycle\Annotated\Annotation\Relation\BelongsTo;

#[Entity(repository: \FrontAccounting\Purchasing\Infrastructure\Persistence\Supplier\SupplierRepository::class, table: 'suppliers')]
// Do NOT mark this class final: Cycle ORM generates a lazy-loading proxy
// subclass of every entity at runtime (visible as "<Entity> Cycle ORM
// Proxy" if you var_dump a hydrated instance) -- a final class can't be
// subclassed, which breaks Cycle's own proxy mechanism, not just some
// hypothetical external extender. Left as an accepted, visible
// ClassMustBeFinal finding rather than suppressed.
/** @psalm-external-mutation-free */
class Supplier
{
    use RequireId;

    #[Column(type: 'primary', name: 'supplier_id')]
    private ?int $id = null;

    /** @psalm-mutation-free */
    public function __construct(
        #[Column(type: 'string(60)', nullable: false)]
        private string $supp_name = '',

        #[Column(type: 'string(30)', nullable: false)]
        private string $supp_ref = '',

        #[Column(type: 'tinyText', nullable: false)]
        private string $address = '',

        #[Column(type: 'tinyText', nullable: false)]
        private string $supp_address = '',

        #[Column(type: 'string(25)', nullable: false)]
        private string $gst_no = '',

        #[Column(type: 'string(60)', nullable: false)]
        private string $contact = '',

        #[Column(type: 'string(40)', nullable: false)]
        private string $supp_account_no = '',

        #[Column(type: 'string(100)', nullable: false)]
        private string $website = '',

        #[Column(type: 'string(60)', nullable: false)]
        private string $bank_account = '',

        #[Column(type: 'string(3)', nullable: true)]
        private ?string $curr_code = '',

        #[Column(type: 'integer(11)', nullable: true)]
        private ?int $payment_terms = null,

        #[Column(type: 'bool', typecast: 'bool', nullable: false, default: false)]
        private bool $tax_included = false,

        #[Column(type: 'integer(11)', nullable: true)]
        private ?int $dimension_id = null,

        #[Column(type: 'integer(11)', nullable: true)]
        private ?int $dimension2_id = null,

        #[Column(type: 'integer(11)', nullable: true)]
        private ?int $tax_group_id = null,

        #[Column(type: 'double', nullable: false)]
        private float $credit_limit = 0.0,

        #[Column(type: 'string(15)', nullable: false)]
        private string $purchase_account = '',

        #[Column(type: 'string(15)', nullable: false)]
        private string $payable_account = '',

        #[Column(type: 'string(15)', nullable: false)]
        private string $payment_discount_account = '',

        #[Column(type: 'tinyText', nullable: false)]
        private string $notes = '',

        #[Column(type: 'bool', typecast: 'bool', nullable: false, default: false)]
        private bool $inactive = false,

    ) {}

    /** @psalm-mutation-free */
    public function reqId(): int
    {
        return $this->requireId($this->id, 'Supplier');
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



    public function getSuppName(): string
    {
        return $this->supp_name;
    }

    /** @psalm-external-mutation-free */
    public function setSuppName(string $supp_name): void
    {
        $this->supp_name = $supp_name;
    }

    public function getSuppRef(): string
    {
        return $this->supp_ref;
    }

    /** @psalm-external-mutation-free */
    public function setSuppRef(string $supp_ref): void
    {
        $this->supp_ref = $supp_ref;
    }

    public function getAddress(): string
    {
        return $this->address;
    }

    /** @psalm-external-mutation-free */
    public function setAddress(string $address): void
    {
        $this->address = $address;
    }

    public function getSuppAddress(): string
    {
        return $this->supp_address;
    }

    /** @psalm-external-mutation-free */
    public function setSuppAddress(string $supp_address): void
    {
        $this->supp_address = $supp_address;
    }

    public function getGstNo(): string
    {
        return $this->gst_no;
    }

    /** @psalm-external-mutation-free */
    public function setGstNo(string $gst_no): void
    {
        $this->gst_no = $gst_no;
    }

    public function getContact(): string
    {
        return $this->contact;
    }

    /** @psalm-external-mutation-free */
    public function setContact(string $contact): void
    {
        $this->contact = $contact;
    }

    public function getSuppAccountNo(): string
    {
        return $this->supp_account_no;
    }

    /** @psalm-external-mutation-free */
    public function setSuppAccountNo(string $supp_account_no): void
    {
        $this->supp_account_no = $supp_account_no;
    }

    public function getWebsite(): string
    {
        return $this->website;
    }

    /** @psalm-external-mutation-free */
    public function setWebsite(string $website): void
    {
        $this->website = $website;
    }

    public function getBankAccount(): string
    {
        return $this->bank_account;
    }

    /** @psalm-external-mutation-free */
    public function setBankAccount(string $bank_account): void
    {
        $this->bank_account = $bank_account;
    }

    public function getCurrCode(): ?string
    {
        return $this->curr_code;
    }

    /** @psalm-external-mutation-free */
    public function setCurrCode(string $curr_code): void
    {
        $this->curr_code = $curr_code;
    }

    public function getPaymentTerms(): ?int
    {
        return $this->payment_terms;
    }

    /** @psalm-external-mutation-free */
    public function setPaymentTerms(int $payment_terms): void
    {
        $this->payment_terms = $payment_terms;
    }

    public function getTaxIncluded(): bool
    {
        return $this->tax_included;
    }

    /** @psalm-external-mutation-free */
    public function setTaxIncluded(bool $tax_included): void
    {
        $this->tax_included = $tax_included;
    }

    /** @psalm-mutation-free */
    public function reqDimensionId(): int
    {
        return $this->requireId($this->dimension_id, 'Supplier dimension_id');
    }

    public function getDimensionId(): ?int
    {
        return $this->dimension_id;
    }

    /** @psalm-external-mutation-free */
    public function setDimensionId(int $dimension_id): void
    {
        $this->dimension_id = $dimension_id;
    }

    /** @psalm-mutation-free */
    public function reqDimension2Id(): int
    {
        return $this->requireId($this->dimension2_id, 'Supplier dimension2_id');
    }

    public function getDimension2Id(): ?int
    {
        return $this->dimension2_id;
    }

    /** @psalm-external-mutation-free */
    public function setDimension2Id(int $dimension2_id): void
    {
        $this->dimension2_id = $dimension2_id;
    }

    /** @psalm-mutation-free */
    public function reqTaxGroupId(): int
    {
        return $this->requireId($this->tax_group_id, 'Supplier tax_group_id');
    }

    public function getTaxGroupId(): ?int
    {
        return $this->tax_group_id;
    }

    /** @psalm-external-mutation-free */
    public function setTaxGroupId(int $tax_group_id): void
    {
        $this->tax_group_id = $tax_group_id;
    }

    public function getCreditLimit(): float
    {
        return $this->credit_limit;
    }

    /** @psalm-external-mutation-free */
    public function setCreditLimit(float $credit_limit): void
    {
        $this->credit_limit = $credit_limit;
    }

    public function getPurchaseAccount(): string
    {
        return $this->purchase_account;
    }

    /** @psalm-external-mutation-free */
    public function setPurchaseAccount(string $purchase_account): void
    {
        $this->purchase_account = $purchase_account;
    }

    public function getPayableAccount(): string
    {
        return $this->payable_account;
    }

    /** @psalm-external-mutation-free */
    public function setPayableAccount(string $payable_account): void
    {
        $this->payable_account = $payable_account;
    }

    public function getPaymentDiscountAccount(): string
    {
        return $this->payment_discount_account;
    }

    /** @psalm-external-mutation-free */
    public function setPaymentDiscountAccount(string $payment_discount_account): void
    {
        $this->payment_discount_account = $payment_discount_account;
    }

    public function getNotes(): string
    {
        return $this->notes;
    }

    /** @psalm-external-mutation-free */
    public function setNotes(string $notes): void
    {
        $this->notes = $notes;
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
