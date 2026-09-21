<?php

declare(strict_types=1);

use Yiisoft\Html\Html as H;
use Yiisoft\View\WebView;

/**
 * The default theme's page footer. It is what themes/default/renderer.php echoed from menu_footer(), and follows
 * _html_php_conventions.php: everything through Yiisoft\Html\Html, one space of indent per nesting level, the same //N
 * on every open and close tag.
 *
 * It starts by closing the wrapping tables that menuHeader.php left open (//5, //4, //3, then //2, //1, //0).
 *
 * @var WebView $this
 * @var bool $noMenu
 * @var bool $isIndex
 * @var bool $loggedIn
 * @var string $now the date and time shown in the status line
 * @var string $pageHelp the hot keys of the page
 * @var string $powerUrl
 * @var string $appTitle
 * @var string $version
 * @var string $themeLine "Theme: <name> - <users online>"
 * @var string $powerBy
 * @var bool $demo demo mode adds an empty row to the footer table
 */

// ─── CSS shortcut variables ──────────────────────────────────────────────────
$footerCell = ['class' => 'footer', 'align' => 'center'];

     echo H::closeTag('td'); //5
    echo H::closeTag('tr'); //4
   echo H::closeTag('table'); //3
   if (!$noMenu) {
   // bottom status line
   echo H::openTag('table', ['class' => $isIndex ? 'bottomBar' : 'bottomBar2']); //3
    echo H::openTag('tr'); //4
     if ($loggedIn) {
     echo H::openTag('td', ['class' => 'bottomBarCell']); //5
      echo $now;
     echo H::closeTag('td'); //5
     echo H::openTag('td', ['id' => 'hotkeyshelp']); //5
      echo $pageHelp;
     echo H::closeTag('td'); //5
     }
    echo H::closeTag('tr'); //4
   echo H::closeTag('table'); //3
   }
  echo H::closeTag('td'); //2
 echo H::closeTag('tr'); //1
echo H::closeTag('table'); //0
if (!$noMenu) {
echo H::openTag('table', ['align' => 'center', 'id' => 'footer']); //0
 echo H::openTag('tr'); //1
  echo H::openTag('td', $footerCell); //2
   echo H::openTag('a', ['target' => '_blank', 'href' => $powerUrl, 'tabindex' => '-1']); //3
    echo H::openTag('font', ['color' => '#ffffff']); //4
     echo $appTitle . ' ' . $version . ' - ' . $themeLine;
    echo H::closeTag('font'); //4
   echo H::closeTag('a'); //3
  echo H::closeTag('td'); //2
 echo H::closeTag('tr'); //1
 echo H::openTag('tr'); //1
  echo H::openTag('td', $footerCell); //2
   echo H::openTag('a', ['target' => '_blank', 'href' => $powerUrl, 'tabindex' => '-1']); //3
    echo H::openTag('font', ['color' => '#ffff00']); //4
     echo $powerBy;
    echo H::closeTag('font'); //4
   echo H::closeTag('a'); //3
  echo H::closeTag('td'); //2
 echo H::closeTag('tr'); //1
 if ($demo) {
 echo H::openTag('tr'); //1
 echo H::closeTag('tr'); //1
 }
echo H::closeTag('table'); //0
echo H::br();
echo H::br();
}
