#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using uint32 = std::uint32_t;
using PreparedQueryResult = bool;
constexpr uint32 TEAM_ALLIANCE = 0;
constexpr uint32 TEAM_HORDE = 1;
constexpr uint32 CHAR_SEL_MAIL_SERVER_CHARACTER = 1;

struct ObjectGuid
{
    std::uint64_t Value;
    uint32 GetCounter() const { return static_cast<uint32>(Value); }
    bool operator==(ObjectGuid const&) const = default;
};

struct CharacterDatabasePreparedStatement
{
    uint32 Values[2]{};
    void SetData(uint32 index, uint32 value) { Values[index] = value; }
};

struct QueryCallback
{
    std::function<void(PreparedQueryResult)> Callback;
    QueryCallback WithPreparedCallback(std::function<void(PreparedQueryResult)> callback)
    {
        Callback = std::move(callback);
        return *this;
    }
};

struct Database
{
    CharacterDatabasePreparedStatement Statement;
    std::vector<std::pair<uint32, uint32>> Queries;
    CharacterDatabasePreparedStatement* GetPreparedStatement(uint32) { return &Statement; }
    QueryCallback AsyncQuery(CharacterDatabasePreparedStatement* statement)
    {
        Queries.emplace_back(statement->Values[0], statement->Values[1]);
        return {};
    }
} CharacterDatabase;

struct QueryProcessor
{
    std::vector<QueryCallback> Callbacks;
    void AddCallback(QueryCallback callback) { Callbacks.push_back(std::move(callback)); }
    void Complete(bool previouslyDelivered)
    {
        auto callbacks = std::move(Callbacks);
        Callbacks.clear();
        for (auto& callback : callbacks)
            callback.Callback(previouslyDelivered);
    }
};

struct Player;
struct WorldSession
{
    Player* Current = nullptr;
    QueryProcessor Processor;
    Player* GetPlayer() const { return Current; }
    QueryProcessor& GetQueryProcessor() { return Processor; }
};

struct Player
{
    ObjectGuid Guid;
    uint32 Team;
    WorldSession* Session;
    ObjectGuid GetGUID() const { return Guid; }
    uint32 GetTeamId() const { return Team; }
    WorldSession* GetSession() const { return Session; }
};

struct ServerMailItems
{
    uint32 Item;
};

struct ServerMailCondition { };
struct ServerMail
{
    uint32 id;
    uint32 senderEntry;
    uint32 moneyA;
    uint32 moneyH;
    std::vector<ServerMailItems> itemsA;
    std::vector<ServerMailItems> itemsH;
    std::vector<ServerMailCondition> conditions;
    std::string subject;
    std::string body;
};

struct Delivery
{
    ObjectGuid Recipient;
    uint32 Id;
    uint32 Money;
    uint32 Item;
};

struct ServerMailManager
{
    std::map<uint32, ServerMail> Store;
    std::vector<Delivery> Deliveries;
    auto const& GetAllServerMailStore() const { return Store; }
    void SendServerMail(Player* player, uint32 id, uint32, uint32 money,
        std::vector<ServerMailItems> const& items, std::vector<ServerMailCondition> const&,
        std::string const&, std::string const&)
    {
        if (!player)
            throw std::runtime_error("Delayed server mail received a missing player");
        Deliveries.push_back({player->GetGUID(), id, money, items.at(0).Item});
    }
} MailManager;

ServerMailManager* sServerMailMgr = &MailManager;

struct PlayerScript
{
    virtual ~PlayerScript() = default;
    virtual void OnPlayerLogin(Player*) { }
};

struct ServerMailReward : PlayerScript
{
    // ACTUAL_LOGIN
};

void Require(bool result, char const* message)
{
    if (!result)
        throw std::runtime_error(message);
}

void Check(uint32 team, uint32 replacement, bool delivered)
{
    MailManager.Deliveries.clear();
    CharacterDatabase.Queries.clear();
    WorldSession session;
    Player original{{42}, team, &session};
    Player other{{replacement == 3 ? (std::uint64_t{1} << 32) + 42 : 43}, TEAM_HORDE, &session};
    session.Current = &original;
    ServerMailReward script;
    script.OnPlayerLogin(&original);
    Require(session.Processor.Callbacks.size() == 1, "Login must enqueue one asynchronous query");
    Require(CharacterDatabase.Queries.at(0) == std::make_pair(42u, 7u),
        "Query must use the original GUID and mail id");
    if (replacement == 1)
        session.Current = nullptr;
    else if (replacement >= 2)
        session.Current = &other;
    session.Processor.Complete(delivered);
    if (replacement != 0 || delivered)
        Require(MailManager.Deliveries.empty(), "Stale or previously delivered mail must not be sent");
    else
    {
        Require(MailManager.Deliveries.size() == 1, "Original player must receive one mail");
        auto const& mail = MailManager.Deliveries.front();
        Require(mail.Recipient == original.Guid && mail.Id == 7, "Mail must retain its original recipient");
        Require(mail.Money == (team == TEAM_ALLIANCE ? 100u : 200u), "Mail must retain faction money");
        Require(mail.Item == (team == TEAM_ALLIANCE ? 11u : 22u), "Mail must retain faction items");
    }
}

int main()
{
    MailManager.Store.emplace(7, ServerMail{7, 123, 100, 200, {{11}}, {{22}}, {}, "Subject", "Body"});
    uint32 failures = 0;
    auto check = [&failures](char const* name, uint32 team, uint32 replacement, bool delivered)
    {
        try
        {
            Check(team, replacement, delivered);
            std::cout << name << ": passed\n";
        }
        catch (std::exception const& error)
        {
            ++failures;
            std::cerr << name << ": failed: " << error.what() << '\n';
        }
    };
    check("logout", TEAM_ALLIANCE, 1, false);
    check("character switch", TEAM_ALLIANCE, 2, false);
    check("full GUID identity", TEAM_ALLIANCE, 3, false);
    check("Alliance original player", TEAM_ALLIANCE, 0, false);
    check("Horde original player", TEAM_HORDE, 0, false);
    check("previously delivered", TEAM_ALLIANCE, 0, true);
    CharacterDatabase.Queries.clear();
    Player noSession{{42}, TEAM_ALLIANCE, nullptr};
    ServerMailReward script;
    script.OnPlayerLogin(&noSession);
    Require(CharacterDatabase.Queries.empty(), "Missing session must not enqueue mail");
    std::cout << "missing session: passed\n";
    return failures ? 1 : 0;
}
