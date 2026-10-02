/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IntegrationTestFixture.h"

namespace
{
class SpellImmunityApplicationTest : public IntegrationTestFixture
{
};

TEST_F(SpellImmunityApplicationTest, SameSpellRemainsImmuneUntilEveryApplicationEnds)
{
    TestPlayer* target = CreateTestPlayer();
    constexpr uint32 spellId = 38112;

    for (uint32 application = 0; application < 4; ++application)
        target->ApplySpellImmune(spellId, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_ALL, true);

    EXPECT_TRUE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
    for (uint32 removal = 0; removal < 3; ++removal)
    {
        target->ApplySpellImmune(spellId, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_ALL, false);
        EXPECT_TRUE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST)) << removal;
    }

    target->ApplySpellImmune(spellId, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_ALL, false);
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
}

TEST_F(SpellImmunityApplicationTest, RepeatedScriptImmunityEndsWithOneRemoval)
{
    TestPlayer* target = CreateTestPlayer();

    for (uint32 application = 0; application < 4; ++application)
        target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);

    EXPECT_TRUE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
    target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
}

TEST_F(SpellImmunityApplicationTest, ScriptRemovalPreservesOtherSpellAndSchool)
{
    TestPlayer* target = CreateTestPlayer();
    target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);
    target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);
    target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, true);
    target->ApplySpellImmune(990070, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);

    target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_TRUE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
    EXPECT_TRUE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_NATURE));

    target->ApplySpellImmune(990070, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
    EXPECT_TRUE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_NATURE));

    target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, false);
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_NATURE));
}

TEST_F(SpellImmunityApplicationTest, ImmunityCategoriesRemainIndependent)
{
    TestPlayer* target = CreateTestPlayer();
    target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);
    target->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_FROST, true);
    target->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_FROST, true);

    target->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_TRUE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
    target->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
}

TEST_F(SpellImmunityApplicationTest, SingleApplicationAndMissingRemovalDoNotLeaveImmunity)
{
    TestPlayer* target = CreateTestPlayer();
    target->ApplySpellImmune(990070, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));

    target->ApplySpellImmune(990070, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);
    target->ApplySpellImmune(990071, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_TRUE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_NATURE));

    target->ApplySpellImmune(990070, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
    target->ApplySpellImmune(990070, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);
    EXPECT_FALSE(target->IsImmunedToDamage(SPELL_SCHOOL_MASK_FROST));
}
}
