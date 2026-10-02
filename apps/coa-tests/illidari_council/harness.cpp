#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

using uint8 = std::uint8_t;
using uint32 = std::uint32_t;
using int32 = std::int32_t;
using namespace std::chrono_literals;

// NATIVE_SAYS
// NATIVE_SPELLS
// NATIVE_ACTIONS
// NATIVE_DATA

enum EncounterState { NOT_STARTED, IN_PROGRESS, DONE, TO_BE_DECIDED };
constexpr int ENCOUNTER_CREDIT_KILL_CREATURE = 0;
constexpr int EVADE_REASON_OTHER = 0;
bool successfulRoll = false;
bool roll_chance_i(int) { return successfulRoll; }

struct Unit;
struct Creature;
struct Instance;
struct FixtureAI
{
    Creature* me = nullptr;
    Instance* instance = nullptr;
    std::vector<int> texts;
    virtual ~FixtureAI() = default;
    virtual void DoAction(int32) { }
    virtual void JustDied(Unit*) { }
    virtual void JustEngagedWith(Unit*) { }
    void Talk(int text) { texts.push_back(text); }
};

struct Unit
{
    static void Kill(Creature*, Creature*);
};

struct Resettable
{
    int scheduled = 0;
    void Reset() { }
    void CancelAll() { scheduled = 0; }
    void DespawnAll() { }
    void KillAllEvents(bool) { }
};

struct Map
{
    void UpdateEncounterState(int, int, Creature*) { }
};

struct Creature : Unit
{
    bool active = false;
    bool alive = true;
    bool engaged = false;
    FixtureAI* ai = nullptr;
    Creature* owner = nullptr;
    Map map;
    Resettable m_Events;
    bool isActiveObject() const { return active; }
    void setActive(bool value) { active = value; }
    bool IsAlive() const { return alive; }
    bool IsEngaged() const { return engaged; }
    void SetCombatPulseDelay(int) { }
    void ResetLootMode() { }
    FixtureAI* AI() { return ai; }
    FixtureAI* GetAI() { return ai; }
    int GetGUID() const { return 1; }
    int GetEntry() const { return 23426; }
    void SetOwnerGUID(int);
    void CallForHelp(int) { }
    Map* GetMap() { return &map; }
    void SetInCombatWithZone()
    {
        if (engaged)
            return;
        engaged = true;
        ai->JustEngagedWith(nullptr);
    }
    void KillSelf()
    {
        alive = false;
        ai->JustDied(nullptr);
    }
};

void Unit::Kill(Creature*, Creature* target) { target->KillSelf(); }

struct Instance
{
    EncounterState state = TO_BE_DECIDED;
    Creature* controller = nullptr;
    std::array<Creature*, 4> members = {};
    EncounterState GetBossState(int) const { return state; }
    void SetBossState(int, EncounterState value) { state = value; }
    void SaveToDB() { }
    bool CheckRequiredBosses(int) { return true; }
    Creature* GetCreature(int id)
    {
        if (id == DATA_ILLIDARI_COUNCIL)
            return controller;
        return members.at(id - DATA_GATHIOS_THE_SHATTERER);
    }
};

void Creature::SetOwnerGUID(int) { owner = ai->instance->controller; }

struct Controller : FixtureAI
{
    Resettable events, scheduler, summons;
    std::vector<int> _healthCheckEvents;
    int _bossId = DATA_ILLIDARI_COUNCIL;
    int balanceCasts = 0;
    int callForHelpRange = 0;
    void DoZoneInCombat() { }
    void ScheduleTasks() { }
    void EnterEvadeMode() { _Reset(); }
    void ClearUniqueTimedEventsDone() { }
    void DoCastSelf(int spell, bool)
    {
        if (spell == SPELL_EMPYREAL_BALANCE)
            ++balanceCasts;
    }
    template<class Duration, class Callback, class Repeat>
    void ScheduleTimedEvent(Duration, Callback, Repeat) { ++scheduler.scheduled; }
    // NATIVE_RESET
    // NATIVE_DONE
    // NATIVE_CONTROLLER_ENGAGE
    void JustEngagedWith(Unit*) override { _JustEngagedWith(); }
    // NATIVE_START
    void JustDied(Unit*) override { _JustDied(); }
};

struct Member : FixtureAI
{
    // NATIVE_DEATH
    // NATIVE_ENGAGE
};

int main()
{
    for (bool roll : { false, true })
    {
        successfulRoll = roll;
        for (int pulled = 0; pulled < 4; ++pulled)
        {
            Instance instance;
            Creature controllerCreature;
            Controller controller;
            controller.me = &controllerCreature;
            controller.instance = &instance;
            controllerCreature.ai = &controller;
            instance.controller = &controllerCreature;
            std::array<Creature, 4> creatures;
            std::array<Member, 4> members;
            for (int i = 0; i < 4; ++i)
            {
                members[i].me = &creatures[i];
                members[i].instance = &instance;
                creatures[i].ai = &members[i];
                instance.members[i] = &creatures[i];
            }
            controller._Reset();
            assert(instance.state == NOT_STARTED);
            creatures[pulled].SetInCombatWithZone();
            if (creatures[pulled].owner)
                creatures[pulled].owner->SetInCombatWithZone();
            assert(instance.state == IN_PROGRESS);
            assert(controller.balanceCasts == 1);
            assert(controller.scheduler.scheduled == 1);
            int yells = 0;
            for (int i = 0; i < 4; ++i)
            {
                assert(creatures[i].engaged);
                yells += members[i].texts.size();
            }
            assert(yells == 1);
            assert(members[roll ? 0 : 3].texts == std::vector<int>{SAY_COUNCIL_AGGRO});
            controller.DoAction(ACTION_START_ENCOUNTER);
            assert(controller.balanceCasts == 1);
            for (auto& creature : creatures)
                creature.engaged = false;
            controllerCreature.engaged = false;
            controller._Reset();
            assert(instance.state == NOT_STARTED);
            assert(controller.scheduler.scheduled == 0);
            creatures[pulled].SetInCombatWithZone();
            if (creatures[pulled].owner)
                creatures[pulled].owner->SetInCombatWithZone();
            assert(instance.state == IN_PROGRESS);
            assert(controller.balanceCasts == 2);
            assert(controller.scheduler.scheduled == 1);
            controller.DoAction(ACTION_END_ENCOUNTER);
            assert(instance.state == DONE);
            assert(!controllerCreature.alive);
            assert(controller.scheduler.scheduled == 0);
            for (int i = 0; i < 4; ++i)
            {
                assert(!creatures[i].alive);
                assert(members[i].texts.back() == 5);
            }
            controller.DoAction(ACTION_START_ENCOUNTER);
            assert(instance.state == DONE);
            assert(controller.balanceCasts == 2);
        }
    }
}
