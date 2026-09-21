<?php

declare(strict_types=1);

use Yiisoft\Html\Html as H;
use Yiisoft\View\WebView;

/**
 * The document skeleton of a FrontAccounting page, rendered by yiisoft/view when $use_yii_layout is on. It follows
 * _html_php_conventions.php: everything through Yiisoft\Html\Html, one space of indent per nesting level, the same
 * //N on every open and close tag. The doctype is the one piece of markup that library has no helper for.
 *
 * FA streams its output, so the page body is not available when the head is sent. The layout is rendered with a marker
 * where the body goes; includes/yii/layout.inc sends what comes before the marker at page() and what comes after it at
 * end_page(). The markup matches what page_header() and page_footer() echo (HTML 4.01 Transitional, as before), so the
 * themes and the JavaScript see the same document. CSS and script files are registered on the view and rendered by
 * $this->head() and $this->endBody().
 *
 * @var WebView $this
 * @var string $title the page title, as FA passes it (already HTML)
 * @var string $encoding e.g. UTF-8
 * @var string $dir ltr or rtl
 * @var string $favicon URL of the favicon
 * @var string $onload JavaScript for <body onload>, or an empty string
 * @var string $content the marker that stands for the page body
 */

$this->beginPage();
echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">' . "\n";
echo H::openTag('html', ['dir' => $dir]); //0
 echo H::openTag('head', ['profile' => 'http://www.w3.org/2005/10/profile']); //1
  echo H::tag('title', $title)
   ->encode(false); //2
  echo H::meta()
   ->httpEquiv('X-UA-Compatible')
   ->content('IE=10'); //2
  echo H::meta()
   ->httpEquiv('Content-type')
   ->content('text/html; charset=' . $encoding); //2
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
