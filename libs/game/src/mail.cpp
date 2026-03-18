#include "dbat/game/mail.hpp"
#include "dbat/game/Database.hpp"
#include "dbat/game/Character.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/TimeInfo.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/game/interpreter.hpp"

#include "dbat/game/const/AdminLevel.hpp"

int count_unreceived_mail(const std::string& playerId)
{
    auto txn = dbat::db::txn.get();
    if (!txn)
        return 0;

    auto result = txn->exec(
        "SELECT COUNT(*) FROM dbat.mail WHERE recipient_id = $1 AND received_at IS NULL",
        pqxx::params{playerId}
    );

    if (result.empty())
        return 0;

    return result[0][0].as<int>();
}

int count_unread_mail(const std::string& playerId)
{
    auto txn = dbat::db::txn.get();
    if (!txn)
        return 0;

    auto result = txn->exec(
        "SELECT COUNT(*) FROM dbat.mail WHERE recipient_id = $1 AND received_at IS NOT NULL AND is_read = false",
        pqxx::params{playerId}
    );

    if (result.empty())
        return 0;

    return result[0][0].as<int>();
}


void postmaster_send_mail(Character *ch, Character *mailman,
                         int cmd, char *arg)
{
    ch->send_to("@YThe mail system is currently disabled for composition.@n\r\n"
                "@YPlease try again in a future update.@n\r\n");
}

void postmaster_check_mail(Character *ch, Character *mailman,
                           int cmd, char *arg)
{
    if (!ch->player || ch->player->id.empty())
    {
        act("$n tells you, 'I can't seem to find your mail records.'", false, mailman, nullptr, ch, TO_VICT);
        return;
    }

    auto txn = dbat::db::txn.get();
    if (!txn)
    {
        act("$n tells you, 'The mail system is having technical difficulties.'", false, mailman, nullptr, ch, TO_VICT);
        return;
    }

    auto unreadResult = txn->exec(
        "SELECT COUNT(*) FROM dbat.mail WHERE recipient_id = $1 AND received_at IS NULL",
        pqxx::params{ch->player->id}
    );

    int unreadCount = 0;
    if (!unreadResult.empty())
    {
        unreadCount = unreadResult[0][0].as<int>();
    }

    txn->exec(
        "UPDATE dbat.mail SET received_at = NOW() WHERE recipient_id = $1 AND received_at IS NULL",
        pqxx::params{ch->player->id}
    );

    if (unreadCount > 0)
    {
        act("$n tells you, '@GYou have mail waiting!@n'", false, mailman, nullptr, ch, TO_VICT);
    }
    else
    {
        act("$n tells you, 'Sorry, you don't have any mail waiting.'", false, mailman, nullptr, ch, TO_VICT);
    }
}

void postmaster_receive_mail(Character *ch, Character *mailman,
                             int cmd, char *arg)
{
    if (!ch->player || ch->player->id.empty())
    {
        act("$n tells you, 'I can't seem to find your mail records.'", false, mailman, nullptr, ch, TO_VICT);
        return;
    }

    auto txn = dbat::db::txn.get();
    if (!txn)
    {
        act("$n tells you, 'The mail system is having technical difficulties.'", false, mailman, nullptr, ch, TO_VICT);
        return;
    }

    auto mailResult = txn->exec(
        "SELECT id, sender_id, subject, body, ic_timestamp, created_at, is_read "
        "FROM dbat.mail "
        "WHERE recipient_id = $1 AND received_at IS NOT NULL "
        "ORDER BY created_at ASC",
        pqxx::params{ch->player->id}
    );

    if (mailResult.empty())
    {
        act("$n tells you, 'Sorry, you don't have any mail waiting.'", false, mailman, nullptr, ch, TO_VICT);
        return;
    }

    ch->send_to("@D@Y* * * * @CGalactic Mail System @Y* * * *@n\r\n");
    ch->send_to("@c-------------------------------------------------------@n\r\n");

    int mailNum = 0;
    for (const auto &row : mailResult)
    {
        mailNum++;
        std::string senderId = row["sender_id"].as<std::string>();
        std::string subject = row["subject"].as<std::string>();
        int64_t icTimestamp = row["ic_timestamp"].as<int64_t>();
        bool isRead = row["is_read"].as<bool>();

        auto senderResult = txn->exec(
            "SELECT name FROM public.pcs WHERE id = $1",
            pqxx::params{senderId}
        );

        std::string senderName = "Unknown";
        if (!senderResult.empty())
        {
            senderName = senderResult[0]["name"].as<std::string>();
        }

        std::string readStatus = isRead ? "@g[Read]" : "@r[Unread]";
        time_info_data icTime(icTimestamp);
        std::string timestamp = fmt::format("@cIC@D: @wYear {} Day {} Hour {}@n",
                                            icTime.year, icTime.day + 1, icTime.hours);

        ch->send_to("@D[@Y{:3}@D] @cFrom@D:@w {:15} @cSubject@D:@w {:25} {}\r\n",
                     mailNum, senderName.substr(0, 15), subject.substr(0, 25), readStatus);
        ch->send_to("@D       {}\r\n", timestamp);
    }

    ch->send_to("@c-------------------------------------------------------@n\r\n");
    ch->send_to("@YUse 'read <number>' to read a message.@n\r\n");
}

void read_mail_message(Character *ch, Character *mailman, int messageNum)
{
    if (!ch->player || ch->player->id.empty())
    {
        act("$n tells you, 'I can't seem to find your mail records.'", false, mailman, nullptr, ch, TO_VICT);
        return;
    }

    auto txn = dbat::db::txn.get();
    if (!txn)
    {
        act("$n tells you, 'The mail system is having technical difficulties.'", false, mailman, nullptr, ch, TO_VICT);
        return;
    }

    auto mailResult = txn->exec(
        "SELECT id, sender_id, subject, body, ic_timestamp, is_read "
        "FROM dbat.mail "
        "WHERE recipient_id = $1 AND received_at IS NOT NULL "
        "ORDER BY created_at ASC "
        "LIMIT 1 OFFSET $2",
        pqxx::params{ch->player->id, messageNum - 1}
    );

    if (mailResult.empty())
    {
        act("$n tells you, 'There is no message with that number.'", false, mailman, nullptr, ch, TO_VICT);
        return;
    }

    const auto &row = mailResult[0];
    std::string senderId = row["sender_id"].as<std::string>();
    std::string subject = row["subject"].as<std::string>();
    std::string body = row["body"].as<std::string>();
    int64_t icTimestamp = row["ic_timestamp"].as<int64_t>();
    int64_t mailId = row["id"].as<int64_t>();

    txn->exec("UPDATE dbat.mail SET is_read = true WHERE id = $1", pqxx::params{mailId});

    auto senderResult = txn->exec(
        "SELECT name FROM public.pcs WHERE id = $1",
        pqxx::params{senderId}
    );

    std::string senderName = "Unknown";
    if (!senderResult.empty())
    {
        senderName = senderResult[0]["name"].as<std::string>();
    }

    time_info_data icTime(icTimestamp);

    ch->send_to("@D@Y* * * * @CGalactic Mail System @Y* * * *@n\r\n");
    ch->send_to("@cFrom@D:@w {}\r\n", senderName);
    ch->send_to("@cSubject@D:@w {}\r\n", subject);
    ch->send_to("@cIC Date@D:@w Year {} Day {} Hour {}\r\n", icTime.year, icTime.day + 1, icTime.hours);
    ch->send_to("@c-------------------------------------------------------@n\r\n");
    ch->send_to("@w{}\r\n", body);
    ch->send_to("@c-------------------------------------------------------@n\r\n");
}


SPECIAL(postmaster)
{
    if (!ch->desc || IS_NPC(ch))
        return false;

    if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive")))
        return false;

    if (CMD_IS("mail"))
    {
        postmaster_send_mail(ch, (Character *)me, cmd, argument);
        return true;
    }
    else if (CMD_IS("check"))
    {
        postmaster_check_mail(ch, (Character *)me, cmd, argument);
        return true;
    }
    else if (CMD_IS("receive"))
    {
        postmaster_receive_mail(ch, (Character *)me, cmd, argument);
        return true;
    }
    return false;
}
