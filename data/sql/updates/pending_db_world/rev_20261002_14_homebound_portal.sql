INSERT INTO `gameobject_template`
(`entry`, `type`, `displayId`, `name`, `size`, `Data0`, `Data1`, `Data2`, `Data3`, `ScriptName`) VALUES
(80775, 22, 1026702, 'Homebound Portal', 1.5, 93187, 0, 0, 1, 'go_ascension_homebound_portal')
ON DUPLICATE KEY UPDATE `type` = VALUES(`type`), `displayId` = VALUES(`displayId`),
`name` = VALUES(`name`), `size` = VALUES(`size`), `Data0` = VALUES(`Data0`),
`Data1` = VALUES(`Data1`), `Data2` = VALUES(`Data2`), `Data3` = VALUES(`Data3`),
`ScriptName` = VALUES(`ScriptName`);
