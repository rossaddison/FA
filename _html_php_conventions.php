<?php

/**
 * HTML-to-PHP Conversion Reference
 * ==================================
 * This file is the canonical goto reference for writing HTML inside the
 * new namespaced (src/) code, as that code starts rendering markup of
 * its own rather than only holding server-side logic like
 * src/Session/GlobalSelections.php. It does not apply to the legacy
 * procedural codebase (includes/, sales/, gl/, etc.), which pre-dates
 * this convention and keeps its own established echo/string style.
 *
 * Adapted from rossaddison/invoice's _html_php_conventions.php for this
 * project: Yii3's TranslatorInterface/UrlGeneratorInterface DI isn't part
 * of FrontAccounting's architecture, so these patterns use FA's existing
 * gettext wrapper (_()) instead of $translator->translate(), and drop the
 * invoice-specific SettingRepository examples in favour of generic
 * placeholders. The core HTML-building and formatting rules are the same.
 *
 * RULES:
 *  - declare(strict_types=1) always appears at the top.
 *  - No raw HTML. Everything is produced via Yiisoft\Html\Html (aliased H).
 *  - Indentation is ONE SPACE per nesting level. No tabs.
 *  - Every H::closeTag() call carries a numeric comment matching its openTag.
 *  - Reusable CSS-class arrays are declared as $variables at file top
 *    (see "CSS shortcut variables" section below) rather than repeated inline.
 *  - Option tags inside a <select> use `new Option()` fluent chain, NOT H::tag.
 *  - Checkbox inputs use H::hiddenInput + H::checkbox OR H::tag('input', '', [...])
 *    — both patterns exist; prefer H::tag for consistency with void elements.
 *  - User-supplied / persisted values placed inside attribute arrays must be
 *    wrapped with H::encode() to prevent XSS.
 *  - If two or more H:: calls appear on the same line, the second (and any
 *    further) H:: call must move to the next line indented one extra space:
 *      // WRONG — two H:: on one line:
 *      echo H::tag('td', H::encode($val), $tdEnd);
 *      // CORRECT — last H:: on its own line, one extra space of indent:
 *      echo H::tag('td',
 *       H::encode($val),
 *       $tdEnd);
 *  - Lines are kept to 85 characters or fewer (see phpcs.xml.dist).
 */

declare(strict_types=1);

// ─── Imports ────────────────────────────────────────────────────────────────
use Yiisoft\Html\Html as H;
use Yiisoft\Html\Tag\Input;
use Yiisoft\Html\Tag\Option;
use Yiisoft\Html\Tag\Input\Checkbox;
use Yiisoft\Html\Tag\Textarea;

// ─── PHPDoc / injected variables ─────────────────────────────────────────────
/**
 * @var array $values   e.g. the current field values, keyed by field name
 * @var array $someList e.g. locations, currencies, tax groups …
 * @var string $name
 * @var non-empty-string $id
 * @var string $label
 */

// ─── CSS shortcut variables (declare once, reuse everywhere) ─────────────────
$row            = ['class' => 'row'];
$colMd6         = ['class' => 'col-xs-12 col-md-6'];
$colMd8Offset2  = ['class' => 'col-xs-12 col-md-8 col-md-offset-2'];
$panel          = ['class' => 'panel panel-default'];
$panelHead      = ['class' => 'panel-heading'];
$panelBody      = ['class' => 'panel-body'];
$formGroup      = ['class' => 'form-group'];
$formControl    = ['class' => 'form-control form-control-lg'];
$checkbox       = ['class' => 'checkbox'];
$inputGroup     = ['class' => 'input-group'];
$inputGroupText = ['class' => 'input-group-text'];
$inputSm        = ['class' => 'input-sm form-control form-control-lg'];
$helpBlock      = ['class' => 'help-block'];

// ════════════════════════════════════════════════════════════════════════════
// PATTERN 1 — Panel / grid scaffold
// ════════════════════════════════════════════════════════════════════════════
echo H::openTag('div', $row); //0
 echo H::openTag('div', $colMd8Offset2); //1
  echo H::openTag('div', $panel); //2
   echo H::openTag('div', $panelHead); //3
    echo _('Section heading');
   echo H::closeTag('div'); //3
   echo H::openTag('div', $panelBody); //3
    echo H::openTag('div', $row); //4
     // ── columns go here ──
    echo H::closeTag('div'); //4
   echo H::closeTag('div'); //3
  echo H::closeTag('div'); //2
 echo H::closeTag('div'); //1
echo H::closeTag('div'); //0

// ════════════════════════════════════════════════════════════════════════════
// PATTERN 2 — Yes / No select  (most common control)
// ════════════════════════════════════════════════════════════════════════════
echo H::openTag('div', $colMd6); //0
 echo H::openTag('div', $formGroup); //1
  echo H::openTag('label', ['for' => 'some_flag']); //2
   echo _('Some flag'); //3
  echo H::closeTag('label'); //2
  echo H::openTag('select', [
   'name'  => 'some_flag',
   'id'    => 'some_flag',
   'class' => 'form-control form-control-lg',
   'data-minimum-results-for-search' => 'Infinity'   // omit when search is useful
  ]); //2
   echo  new Option()
         ->value('0')
         ->content(_('No')); //3
   echo  new Option()
         ->value('1')
         ->selected($values['some_flag'] == '1')
         ->content(_('Yes')); //3
  echo H::closeTag('select'); //2
 echo H::closeTag('div'); //1
echo H::closeTag('div'); //0

// ════════════════════════════════════════════════════════════════════════════
// PATTERN 3 — Select with dynamic option list (foreach)
// ════════════════════════════════════════════════════════════════════════════
echo H::openTag('div', $colMd6); //0
 echo H::openTag('div', $formGroup); //1
  echo H::openTag('label', ['for' => 'some_code']); //2
   echo _('Some code');
  echo H::closeTag('label'); //2
  echo H::openTag('select', [
   'name'  => 'some_code',
   'id'    => 'some_code',
   'class' => 'input-sm form-control'
  ]); //2
   echo  new Option()
    ->value('0')
    ->content(_('None')); //3
   /**
   * @var string $key
   * @var string $val
   */
   foreach ($someList as $key => $val) {
    echo  new Option()
     ->value($key)
     ->selected($values['some_code'] == $key)
      ->content(H::encode($val));
   } //3
  echo H::closeTag('select'); //2
 echo H::closeTag('div'); //1
echo H::closeTag('div'); //0

// ════════════════════════════════════════════════════════════════════════════
// PATTERN 4 — Text input
// ════════════════════════════════════════════════════════════════════════════
echo H::openTag('div', $colMd6); //0
 echo H::openTag('div', $formGroup); //1
  echo H::openTag('label', ['for' => 'some_text']); //2
   echo _('Some text');
  echo H::closeTag('label'); //2
  echo new Input()
       ->type('text')
       ->class('form-control form-control-lg')
       ->id('some_text')
       ->name('some_text')
       ->value(H::encode($values['some_text']));
 echo H::closeTag('div'); //1
echo H::closeTag('div'); //0

// ════════════════════════════════════════════════════════════════════════════
// PATTERN 5 — Number input
// ════════════════════════════════════════════════════════════════════════════
echo H::openTag('div', $colMd6); //0
 echo H::openTag('div', $formGroup); //1
  echo H::openTag('label', ['for' => 'some_number']); //2
   echo _('Some number'); //3
  echo H::closeTag('label'); //2
   echo new Input()
        ->type('number')
        ->class('form-control form-control-lg')
        ->id('some_number')
        ->name('some_number')
        ->addAttributes([
            'min' => '1',
        ])
        ->required(true)
        ->value(H::encode($values['some_number'])); //2
 echo H::closeTag('div'); //1
echo H::closeTag('div'); //0

// ════════════════════════════════════════════════════════════════════════════
// PATTERN 6 — Textarea
// ════════════════════════════════════════════════════════════════════════════
echo H::openTag('div', $colMd6); //0
 echo H::openTag('div', $formGroup); //1
  echo H::openTag('label', ['for' => 'some_notes']); //2
   echo _('Some notes');
  echo H::closeTag('label'); //2
  echo new TextArea()
       ->name('some_notes')
       ->id('some_notes')
       ->rows(4)
       ->value(H::encode($values['some_notes'])); //2
 echo H::closeTag('div'); //1
echo H::closeTag('div'); //0

// ════════════════════════════════════════════════════════════════════════════
// PATTERN 7 — Checkbox
// ════════════════════════════════════════════════════════════════════════════
echo H::openTag('div', $colMd6); //0
 echo H::openTag('div', $formGroup); //1
  echo H::openTag('div', $checkbox); //2
   echo new Checkbox()
         ->name($name)
         ->id($id)
         ->label($label)
         ->checked($values['some_flag'] === '1'); //3
   echo H::closeTag('div'); //2
 echo H::closeTag('div'); //1
echo H::closeTag('div'); //0

// ==================================================================
// PATTERN 8 - Bold
// ==================================================================
echo H::openTag('b'); //0
 echo 'This is a statement';
echo H::closeTag('b'); //0
