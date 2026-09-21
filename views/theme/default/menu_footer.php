<?php

declare(strict_types=1);

use Yiisoft\View\WebView;

/**
 * The default theme's page footer: the bottom status line and the footer block. It is what
 * themes/default/renderer.php echoes from menu_footer(), as a template; the markup is the same.
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
 * @var bool $demo demo mode adds an empty row
 */
?>
</td></tr></table>
<?php if (!$noMenu) : ?>
<table class='<?= $isIndex ? 'bottomBar' : 'bottomBar2' ?>'>
<tr><?php if ($loggedIn) : ?><td class='bottomBarCell'><?= $now ?></td>
<td id='hotkeyshelp'><?= $pageHelp ?></td><?php endif ?></tr></table>
<?php endif ?>
</td></tr> </table>
<?php if (!$noMenu) : ?>
<table align='center' id='footer'>
<tr>
<td align='center' class='footer'><a target='_blank' href='<?= $powerUrl ?>' tabindex='-1'><font color='#ffffff'><?= $appTitle ?> <?= $version ?> - <?= $themeLine ?></font></a></td>
</tr>
<tr>
<td align='center' class='footer'><a target='_blank' href='<?= $powerUrl ?>' tabindex='-1'><font color='#ffff00'><?= $powerBy ?></font></a></td>
</tr>
<?php if ($demo) : ?><tr>
</tr>
<?php endif ?></table><br><br>
<?php endif ?>
