<?php

declare(strict_types=1);

use Yiisoft\View\WebView;

/**
 * The default theme's page header: the tab bar, the status bar and the page title. It is what
 * themes/default/renderer.php echoes from menu_header(), as a template; the markup is the same.
 *
 * Values are printed as FA prints them: they are already HTML (FA encodes text when it is stored and when it
 * builds links), so encoding them here would encode them twice.
 *
 * @var WebView $this
 * @var bool $noMenu
 * @var bool $isIndex
 * @var string $title
 * @var bool $showTitle whether the page title is shown: FA shows it when it is not empty and this is not the index
 * @var string $root the path to the FA root, as URL
 * @var string $images URL of the theme's images folder
 * @var string $indicator URL of the ajax activity image
 * @var bool $hints whether the "hints" area is shown
 * @var list<array{class: string, href: string, access: string, label: string}> $tabs
 * @var string $selectedApp
 * @var string $status company | server | user
 * @var string $username
 * @var string|null $helpUrl null when help is not configured
 * @var array<string, string> $t the translated labels: dashboard, preferences, changePassword, changePasswordAlt (the icon's text, capitalised differently), help, logout
 */
?>
<table class='callout_main' border='0' cellpadding='0' cellspacing='0'>
<tr>
<td colspan='2' rowspan='2'>
<table class='main_page' border='0' cellpadding='0' cellspacing='0'>
<tr>
<td>
<table width='100%' border='0' cellpadding='0' cellspacing='0'>
<tr>
<td class='quick_menu'>
<?php if (!$noMenu) : ?>
<table cellpadding='0' cellspacing='0' width='100%'><tr><td><div class='tabs'><?php foreach ($tabs as $tab) : ?><a class='<?= $tab['class'] ?>' href='<?= $tab['href'] ?>'<?= $tab['access'] ?>><?= $tab['label'] ?></a><?php endforeach ?></div></td></tr></table><table class='logoutBar'><tr><td class='headingtext3'><?= $status ?></td><td class='logoutBarRight'><img id='ajaxmark' src='<?= $indicator ?>' align='center' style='visibility:hidden;' alt='ajaxmark'></td><td class='logoutBarRight'><a href='<?= $root ?>/admin/dashboard.php?sel_app=<?= $selectedApp ?>'><img src='<?= $images ?>/report.png' style='width:14px;height:14px;border:0;vertical-align:middle;' alt='<?= $t['dashboard'] ?>'>&nbsp;&nbsp;<?= $t['dashboard'] ?></a>&nbsp;&nbsp;&nbsp;
<a class='shortcut' href='<?= $root ?>/admin/display_prefs.php?'><img src='<?= $images ?>/preferences.gif' style='width:14px;height:14px; border:0;vertical-align:middle;' alt='<?= $t['preferences'] ?>'>&nbsp;&nbsp;<?= $t['preferences'] ?></a>&nbsp;&nbsp;&nbsp;
  <a class='shortcut' href='<?= $root ?>/admin/change_current_user_password.php?selected_id=<?= $username ?>'><img src='<?= $images ?>/lock.gif' style='width:14px;height:14px;border:0;vertical-align:middle;' alt='<?= $t['changePasswordAlt'] ?>'>&nbsp;&nbsp;<?= $t['changePassword'] ?></a>&nbsp;&nbsp;&nbsp;
<?php if ($helpUrl !== null) : ?><a target = '_blank' onclick="javascript:openWindow(this.href,this.target); return false;" href='<?= $helpUrl ?>'><img src='<?= $images ?>/help.gif' style='width:14px;height:14px;border:0;vertical-align:middle;'' alt='<?= $t['help'] ?>'>&nbsp;&nbsp;<?= $t['help'] ?></a>&nbsp;&nbsp;&nbsp;<?php endif ?><a class='shortcut' href='<?= $root ?>/access/logout.php?'><img src='<?= $images ?>/login.gif' style='width:14px;height:14px;border:0;vertical-align:middle;' alt='<?= $t['logout'] ?>'>&nbsp;&nbsp;<?= $t['logout'] ?></a>&nbsp;&nbsp;&nbsp;</td></tr><tr><td colspan=3></td></tr></table>
<?php endif ?>
</td></tr></table>
<?php if ($noMenu) : ?>
<center><table class='tablestyle_noborder'><tr><td><img id='ajaxmark' src='<?= $indicator ?>' align='center' style='visibility:hidden;' alt='ajaxmark'></td></tr></table></center>
<?php elseif ($showTitle) : ?>
<center><table id='title'><tr><td width='100%' class='titletext'><?= $title ?></td><td align=right><?= $hints ? "<span id='hints'></span>" : '' ?></td></tr></table></center>
<?php endif ?>
