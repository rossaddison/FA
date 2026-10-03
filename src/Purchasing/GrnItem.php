<?php
declare(strict_types=1);

namespace FrontAccounting\Purchasing;

/**
 * Cached line-item data for one GRN (goods-received-note) detail being
 * invoiced/credited on a supplier transaction. Holds everything needed to
 * write the entry without re-querying purch_order_details.
 */
final class GrnItem
{
    /** @var string|int|float|bool|null */
    public $id;

    /** @var string|int|float|bool|null */
    public $po_detail_item;

    /** @var string */
    public $item_code;

    /** @var string */
    public $item_description;

    /** @var float|int|string */
    public $qty_recd;

    /** @var float|int|string */
    public $prev_quantity_inv;

    /** @var float|int|string */
    public $this_quantity_inv;

    /** @var float|int|string */
    public $order_price;

    /** @var float|int|string */
    public $chg_price;

    /** @var float|int|string */
    public $std_cost_unit;

    /** @var string|int|float|bool|null */
    public $gl_code;

    /** @var int|bool */
    public $tax_included;

    /**
     * @param string|int|float|bool|null $id
     * @param string|int|float|bool|null $po_detail_item
     * @param string|int|float|bool|null $item_code
     * @param string|int|float|bool|null $item_description
     * @param float|int|string|bool|null $qty_recd
     * @param float|int|string|bool|null $prev_quantity_inv
     * @param int|float|bool|null $this_quantity_inv
     * @param float|int|string|bool|null $order_price
     * @param string|int|float|bool|null $chg_price
     * @param int|bool|null $std_cost_unit
     * @param string|int|bool|null $gl_code
     * @param int|bool|null $tax_included
     * @psalm-mutation-free
     */
    public function __construct(
        $id,
        $po_detail_item,
        $item_code,
        $item_description,
        $qty_recd,
        $prev_quantity_inv,
        $this_quantity_inv,
        $order_price,
        $chg_price,
        $std_cost_unit,
        $gl_code,
        $tax_included
    ) {
        $this->id = $id;
        $this->po_detail_item = $po_detail_item;
        $this->item_code = (string) $item_code;
        $this->item_description = (string) $item_description;
        $this->qty_recd = is_bool($qty_recd) ? (int) $qty_recd : ($qty_recd ?? 0);
        $this->prev_quantity_inv = is_bool($prev_quantity_inv) ? (int) $prev_quantity_inv : ($prev_quantity_inv ?? 0);
        $this->this_quantity_inv = (float) ($this_quantity_inv ?? 0);
        $this->order_price = is_bool($order_price) ? (int) $order_price : ($order_price ?? 0);
        $this->chg_price = is_bool($chg_price) ? (int) $chg_price : ($chg_price ?? 0);
        $this->std_cost_unit = (float) ($std_cost_unit ?? 0);
        $this->gl_code = $gl_code;
        $this->tax_included = (int) ($tax_included ?? 0);
    }

    /**
     * @param string|int|float|bool|null $tax_group_id
     * @param array<array-key, array{tax_type_id: array-key, tax_type_name?: scalar|null,
     *     sales_gl_code?: scalar|null, purchasing_gl_code?: scalar|null, rate: scalar|null,
     *     included_in_price?: scalar|null, Value?: scalar|null, Net?: scalar|null,
     *     ...<array-key, scalar|null>}>|null $tax_group
     */
    public function full_charge_price($tax_group_id, $tax_group = null): float
    {
        return \get_full_price_for_item(
            $this->item_code,
            (float) $this->chg_price,
            $tax_group_id,
            $this->tax_included,
            $tax_group
        );
    }

    /**
     * @param string|int|float|bool|null $tax_group_id
     * @param array<array-key, array{tax_type_id: array-key, tax_type_name?: scalar|null,
     *     sales_gl_code?: scalar|null, purchasing_gl_code?: scalar|null, rate: scalar|null,
     *     included_in_price?: scalar|null, Value?: scalar|null, Net?: scalar|null,
     *     ...<array-key, scalar|null>}>|null $tax_group
     */
    public function taxfree_charge_price($tax_group_id, $tax_group = null): ?float
    {
        return \get_tax_free_price_for_item(
            $this->item_code,
            (float) $this->chg_price,
            $tax_group_id,
            $this->tax_included,
            $tax_group
        );
    }
}
