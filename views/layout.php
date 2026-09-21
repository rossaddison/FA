<?php

declare(strict_types=1);

use Yiisoft\Html\Html as H;
use Yiisoft\View\WebView;

/**
 * The document skeleton of a FrontAccounting page, rendered by yiisoft/view when $use_yii_layout is on. It follows
 * _html_php_conventions.php: everything through Yiisoft\Html\Html, one space of indent per nesting level, the same
 * //N on every open and close tag. The doctype is the one piece of markup that library has no helper for. It is HTML5
 * (`<!DOCTYPE html>`, `lang`, `<meta charset>`); the pages FA builds inside it are unchanged.
 *
 * FA streams its output, so the page body is not available when the head is sent. The layout is rendered with a marker
 * where the body goes; includes/yii/layout.inc sends what comes before the marker at page() and what comes after it at
 * end_page(). The body markup matches what page_header() and page_footer() echo. CSS and script files are registered on the view and rendered by
 * $this->head() and $this->endBody().
 *
 * @var WebView $this
 * @var string $title the page title, as FA passes it (already HTML)
 * @var string $encoding e.g. UTF-8
 * @var string $dir ltr or rtl
 * @var string $lang the language tag, e.g. en-GB
 * @var string $favicon URL of the favicon
 * @var string $onload JavaScript for <body onload>, or an empty string
 * @var string $content the marker that stands for the page body
 */

$this->beginPage();
echo '<!DOCTYPE html>' . "\n";
echo H::openTag('html', ['dir' => $dir, 'lang' => $lang]); //0
 echo H::openTag('head'); //1
  echo H::meta()
   ->charset($encoding); //2
  echo H::tag('title', $title)
   ->encode(false); //2
  echo H::link()
   ->rel('icon')
   ->type('image/x-icon')
   ->href($favicon); //2
  $this->head();
 echo H::closeTag('head'); //1
 $bodyAttributes = $onload === '' ? [] : ['onload' => $onload];
 echo H::openTag('body', $bodyAttributes); //1
  $this->beginBody();
  echo $content;
  $this->endBody();
 echo H::closeTag('body'); //1
echo H::closeTag('html'); //0
$this->endPage();
