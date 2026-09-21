<?php

declare(strict_types=1);

use Yiisoft\Html\Html as H;
use Yiisoft\View\WebView;

/**
 * The default theme's page header: the tab bar, the status bar and the page title. It is what
 * themes/default/renderer.php echoed from menu_header(), and follows _html_php_conventions.php: everything through
 * Yiisoft\Html\Html, one space of indent per nesting level, the same //N on every open and close tag.
 *
 * It ends with the wrapping tables (//0 to //5) still open: menuFooter.php closes them with the same numbers.
 *
 * Text that FA holds as HTML (the title, the company name, the tab labels) is echoed as it is; encoding it here would
 * encode it twice. Attribute values go through H, which encodes them.
 *
 * @var WebView $this
 * @var bool $noMenu
 * @var bool $showTitle whether the page title is shown: FA shows it when it is not empty and this is not the index
 * @var string $title
 * @var string $indicator URL of the ajax activity image
 * @var string $images URL of the theme's images folder
 * @var bool $hints whether the "hints" area is shown
 * @var list<array{attributes: array<string, string>, label: string}> $tabs
 * @var string $status company | server | user
 * @var list<array{attributes: array<string, string>, icon: string, iconStyle: string, alt: string, label: string}> $links
 */

// ─── CSS shortcut variables ──────────────────────────────────────────────────
$calloutMain    = ['class' => 'callout_main', 'border' => '0', 'cellpadding' => '0', 'cellspacing' => '0'];
$mainPage       = ['class' => 'main_page', 'border' => '0', 'cellpadding' => '0', 'cellspacing' => '0'];
$fullWidth      = ['width' => '100%', 'border' => '0', 'cellpadding' => '0', 'cellspacing' => '0'];
$tabsTable      = ['cellpadding' => '0', 'cellspacing' => '0', 'width' => '100%'];
$quickMenu      = ['class' => 'quick_menu'];
$logoutBar      = ['class' => 'logoutBar'];
$logoutBarRight = ['class' => 'logoutBarRight'];
$ajaxMark       = ['id' => 'ajaxmark', 'align' => 'center', 'style' => 'visibility:hidden;'];
$gap            = '&nbsp;&nbsp;&nbsp;';
$iconGap        = '&nbsp;&nbsp;';

echo H::openTag('table', $calloutMain); //0
 echo H::openTag('tr'); //1
  echo H::openTag('td', ['colspan' => '2', 'rowspan' => '2']); //2
   echo H::openTag('table', $mainPage); //3
    echo H::openTag('tr'); //4
     echo H::openTag('td'); //5
      echo H::openTag('table', $fullWidth); //6
       echo H::openTag('tr'); //7
        echo H::openTag('td', $quickMenu); //8
         if (!$noMenu) {
         echo H::openTag('table', $tabsTable); //9
          echo H::openTag('tr'); //10
           echo H::openTag('td'); //11
            echo H::openTag('div', ['class' => 'tabs']); //12
             foreach ($tabs as $tab) {
             echo H::openTag('a', $tab['attributes']); //13
              echo $tab['label'];
             echo H::closeTag('a'); //13
             }
            echo H::closeTag('div'); //12
           echo H::closeTag('td'); //11
          echo H::closeTag('tr'); //10
         echo H::closeTag('table'); //9
         echo H::openTag('table', $logoutBar); //9
          echo H::openTag('tr'); //10
           echo H::openTag('td', ['class' => 'headingtext3']); //11
            echo $status;
           echo H::closeTag('td'); //11
           echo H::openTag('td', $logoutBarRight); //11
            echo H::img($indicator, 'ajaxmark')
             ->addAttributes($ajaxMark);
           echo H::closeTag('td'); //11
           echo H::openTag('td', $logoutBarRight); //11
            foreach ($links as $link) {
            echo H::openTag('a', $link['attributes']); //12
             echo H::img($images . '/' . $link['icon'], $link['alt'])
              ->addAttributes(['style' => $link['iconStyle']]);
             echo $iconGap . $link['label'];
            echo H::closeTag('a'); //12
            echo $gap;
            }
           echo H::closeTag('td'); //11
          echo H::closeTag('tr'); //10
          echo H::openTag('tr'); //10
           echo H::openTag('td', ['colspan' => '3']); //11
           echo H::closeTag('td'); //11
          echo H::closeTag('tr'); //10
         echo H::closeTag('table'); //9
         }
        echo H::closeTag('td'); //8
       echo H::closeTag('tr'); //7
      echo H::closeTag('table'); //6
      if ($noMenu) {
      // the ajax activity image for the installer and for pop-ups
      echo H::openTag('center'); //6
       echo H::openTag('table', ['class' => 'tablestyle_noborder']); //7
        echo H::openTag('tr'); //8
         echo H::openTag('td'); //9
          echo H::img($indicator, 'ajaxmark')
           ->addAttributes($ajaxMark);
         echo H::closeTag('td'); //9
        echo H::closeTag('tr'); //8
       echo H::closeTag('table'); //7
      echo H::closeTag('center'); //6
      } elseif ($showTitle) {
      echo H::openTag('center'); //6
       echo H::openTag('table', ['id' => 'title']); //7
        echo H::openTag('tr'); //8
         echo H::openTag('td', ['width' => '100%', 'class' => 'titletext']); //9
          echo $title;
         echo H::closeTag('td'); //9
         echo H::openTag('td', ['align' => 'right']); //9
          if ($hints) {
          echo H::openTag('span', ['id' => 'hints']); //10
          echo H::closeTag('span'); //10
          }
         echo H::closeTag('td'); //9
        echo H::closeTag('tr'); //8
       echo H::closeTag('table'); //7
      echo H::closeTag('center'); //6
      }
