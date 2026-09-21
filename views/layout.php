<?php

declare(strict_types=1);

use Yiisoft\Html\Html;
use Yiisoft\View\WebView;

/**
 * The document skeleton of a FrontAccounting page, rendered by yiisoft/view when $use_yii_layout is on.
 *
 * FA streams its output, so the page body is not available when the head is sent. The layout is rendered with a marker
 * where the body goes; includes/yii/layout.inc sends what comes before the marker at page() and what comes after it at
 * end_page(). The markup matches what page_header() and page_footer() echo (HTML 4.01 Transitional, as today), so the
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
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html dir="<?= Html::encode($dir) ?>" >
<head profile="http://www.w3.org/2005/10/profile"><title><?= $title ?></title><meta http-equiv='X-UA-Compatible' content='IE=10'>
<meta http-equiv='Content-type' content='text/html; charset=<?= Html::encode($encoding) ?>'><link href='<?= Html::encode($favicon) ?>' rel='icon' type='image/x-icon'>
<?php $this->head() ?>
</head>
<?= $onload === '' ? '<body>' : '<body onload="' . $onload . '">' ?>
<?php $this->beginBody() ?>
<?= $content ?>
<?php $this->endBody() ?>
</body></html>
<?php $this->endPage() ?>
