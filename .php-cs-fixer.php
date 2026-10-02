<?php

declare(strict_types=1);

use PhpCsFixer\Config;
use PhpCsFixer\Finder;

ini_set('memory_limit', '512M');

$root = __DIR__;
$finder = new Finder()
    ->in([
        $root.'/src',
    ]);
// Only the namespaced PSR-4 code under src/ is covered so far (the pilot
// started with src/Session/GlobalSelections.php - see
// doc/PSALM_MIGRATION.md for the wider file-by-file cleanup this sits
// alongside). The legacy procedural codebase (includes/, sales/, gl/,
// etc.) is untouched by this config: it predates PSR-12 and reformatting
// it wholesale is a separate, much larger decision, not something to do
// as a side effect of adding tooling for the new namespaced files.

return new Config()
    ->setCacheFile(__DIR__ . '/.psalm-cache/.php-cs-fixer.cache')

    /**
     * Related logic:
     *
     * https://github.com/PHP-CS-Fixer/PHP-CS-Fixer
     * vendor\friendsofphp\php-cs-fixer\src\RuleSet\Sets
     * https://cs.symfony.com/doc/usage.html
     *
     * e.g. The PSR12 set inherits from the PSR2 set the following line
     * 'single_blank_line_at_eof => true'
     *
     * To run a single check without changes at command line: e.g.
     * php vendor/bin/php-cs-fixer fix . --rules=single_blank_line_at_eof --verbose --dry-run
     *
     */
    ->setRules([
        '@PSR12' => true,
    ])
    ->setFinder($finder);
