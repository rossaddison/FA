<?php
declare(strict_types=1);

namespace FrontAccounting\Purchasing;

/**
 * One manually-entered GL coding line on a supplier transaction (an
 * accounts-payable invoice/credit posted straight to a GL account rather
 * than against a GRN item).
 */
final class GlCodes
{
    /** @var int */
    public $Counter;

    /** @var string */
    public $gl_code;

    /** @var string */
    public $gl_act_name;

    /** @var int|string */
    public $gl_dim;

    /** @var int|string */
    public $gl_dim2;

    /** @var float|int|string */
    public $amount;

    /** @var string */
    public $memo_;

    /**
     * @param int $Counter
     * @param string|int|float|bool|null $gl_code
     * @param string|int|float|bool|null $gl_act_name
     * @param int|string|float|bool|null $gl_dim
     * @param int|string|float|bool|null $gl_dim2
     * @param float|int|string|bool|null $amount
     * @param string|int|float|bool|null $memo_
     * @psalm-mutation-free
     */
    public function __construct($Counter, $gl_code, $gl_act_name, $gl_dim, $gl_dim2, $amount, $memo_)
    {
        $this->Counter = $Counter;
        $this->gl_code = (string) $gl_code;
        $this->gl_act_name = (string) $gl_act_name;
        $this->gl_dim = (is_float($gl_dim) || is_bool($gl_dim)) ? (int) $gl_dim : ($gl_dim ?? 0);
        $this->gl_dim2 = (is_float($gl_dim2) || is_bool($gl_dim2)) ? (int) $gl_dim2 : ($gl_dim2 ?? 0);
        $this->amount = is_bool($amount) ? (int) $amount : ($amount ?? 0);
        $this->memo_ = (string) $memo_;
    }
}
