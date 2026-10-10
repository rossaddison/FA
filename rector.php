<?php

declare(strict_types=1);

use Rector\Config\RectorConfig;
use Rector\Set\ValueObject\LevelSetList;
use Rector\Set\ValueObject\SetList;

// Dry-run survey only, per the standing decision not to apply Rector changes
// yet (risk of conflicting with the ongoing Psalm cleanup, see
// doc/PSALM_MIGRATION.md). Always invoke with --dry-run.
return RectorConfig::configure()
    ->withPaths([
        __DIR__ . '/admin',
        __DIR__ . '/dimensions',
        __DIR__ . '/fixed_assets',
        __DIR__ . '/gl',
        __DIR__ . '/includes',
        __DIR__ . '/install',
        __DIR__ . '/inventory',
        __DIR__ . '/manufacturing',
        __DIR__ . '/purchasing',
        __DIR__ . '/sales',
        __DIR__ . '/src',
        __DIR__ . '/taxes',
    ])
    ->withSkip([
        __DIR__ . '/reporting', // same "revisit later" scope as psalm.xml
    ])
    ->withPhpSets()
    ->withSets([
        LevelSetList::UP_TO_PHP_84,
        SetList::PHP_85,
    ]);
