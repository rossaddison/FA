<?php

declare(strict_types=1);

namespace FrontAccounting\Currency\Infrastructure\Persistence\Currencie;

use Cycle\Annotated\Annotation\Column;
use Cycle\Annotated\Annotation\Entity;
use Cycle\Annotated\Annotation\Relation\BelongsTo;
use Cycle\Annotated\Annotation\Table\PrimaryKey;

#[PrimaryKey(columns: ['curr_abrev'])]
#[Entity(repository: \FrontAccounting\Currency\Infrastructure\Persistence\Currencie\CurrencieRepository::class, table: 'currencies')]
// Do NOT mark this class final: Cycle ORM generates a lazy-loading proxy
// subclass of every entity at runtime (visible as "<Entity> Cycle ORM
// Proxy" if you var_dump a hydrated instance) -- a final class can't be
// subclassed, which breaks Cycle's own proxy mechanism, not just some
// hypothetical external extender. Left as an accepted, visible
// ClassMustBeFinal finding rather than suppressed.
/** @psalm-external-mutation-free */
class Currencie
{
    /** @psalm-mutation-free */
    public function __construct(
        #[Column(type: 'string(60)', nullable: false)]
        private string $currency = '',

        #[Column(type: 'string(3)', nullable: false)]
        private string $curr_abrev = '',

        #[Column(type: 'string(10)', nullable: false)]
        private string $curr_symbol = '',

        #[Column(type: 'string(100)', nullable: false)]
        private string $country = '',

        #[Column(type: 'string(15)', nullable: false)]
        private string $hundreds_name = '',

        #[Column(type: 'bool', typecast: 'bool', nullable: false, default: true)]
        private bool $auto_update = true,

        #[Column(type: 'bool', typecast: 'bool', nullable: false, default: false)]
        private bool $inactive = false,

    ) {}



    public function getCurrency(): string
    {
        return $this->currency;
    }

    /** @psalm-external-mutation-free */
    public function setCurrency(string $currency): void
    {
        $this->currency = $currency;
    }

    public function getCurrAbrev(): string
    {
        return $this->curr_abrev;
    }

    /** @psalm-external-mutation-free */
    public function setCurrAbrev(string $curr_abrev): void
    {
        $this->curr_abrev = $curr_abrev;
    }

    public function getCurrSymbol(): string
    {
        return $this->curr_symbol;
    }

    /** @psalm-external-mutation-free */
    public function setCurrSymbol(string $curr_symbol): void
    {
        $this->curr_symbol = $curr_symbol;
    }

    public function getCountry(): string
    {
        return $this->country;
    }

    /** @psalm-external-mutation-free */
    public function setCountry(string $country): void
    {
        $this->country = $country;
    }

    public function getHundredsName(): string
    {
        return $this->hundreds_name;
    }

    /** @psalm-external-mutation-free */
    public function setHundredsName(string $hundreds_name): void
    {
        $this->hundreds_name = $hundreds_name;
    }

    public function getAutoUpdate(): bool
    {
        return $this->auto_update;
    }

    /** @psalm-external-mutation-free */
    public function setAutoUpdate(bool $auto_update): void
    {
        $this->auto_update = $auto_update;
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
