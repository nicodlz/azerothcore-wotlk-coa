-- Thermal Anvil's native spell 9931265 summons forge 2201000 and anvil 2202000.
-- The original CoA cache records both objects on 2026-09-04,
-- matching the 2026-07-03 CoA beta cache: hertigservices/ascension-data,
-- dataset e54620b179904343b6aedc3953a75caa21f14a5c, gameobjectcache.tsv.gz.
UPDATE `item_template` SET `ScriptName` = '' WHERE `entry` = 1777028 AND `ScriptName` = 'item_ethereal_set_cache';

INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `size`, `data0`, `data1`, `data5`, `data19`)
VALUES
(2201000, 8, 137975, 'Thermal Anvil', 1, 3, 20, 1, 6542),
(2202000, 8, 1287, 'Thermal Anvil', 0.2, 1, 20, 0, 6542)
ON DUPLICATE KEY UPDATE `type` = VALUES(`type`), `displayId` = VALUES(`displayId`),
`name` = VALUES(`name`), `size` = VALUES(`size`), `data0` = VALUES(`data0`),
`data1` = VALUES(`data1`), `data5` = VALUES(`data5`), `data19` = VALUES(`data19`);
