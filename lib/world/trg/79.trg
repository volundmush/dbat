#7900
Zannah receive~
0 j 100
~
if (%actor.align% > 0)
 return 0
 wait 1
 say I don't want anything from a goodie two-shoes like you.
elseif (%actor.varexists(zantrain)%)
 return 0
 wait 1
 say You got another hide? Well, you can already train with me so you keep it.
 wait 2 s
elseif (%object.vnum% == 3605)
 wait 1
 %purge% %object%
 say Ahh great, you managed to kill a Fearramorphus. This hide will do nicely in my collection.
 wait 1 s
 %echo% %self.name% takes the hide and slips it into her rucksack.
 wait 2 s
 say You may train with me whenever you like now.
 set zantrain
 remote zantrain %actor.id%
else
 return 0
 wait 1
 say I don't want that.
end
~
#7901
Zannah train~
0 c 100
pr~
Trigger Intended Assignment: Mobiles
Trigger Type: Command , Numeric Arg: 100, Arg list: pr
Commands:
if (%actor.align% > 0)
 return 1
 wait 1
 say I won't train a goodie two-shoes like you.
elseif (!%actor.varexists(zantrain)%)
 return 1
 wait 1
 say You can't train with me until you bring me the hide of Fearramorphus.
else
 return 0
end
~
#7902
Zannah greet~
0 g 100
~
if (%actor.align% > 0)
 wait 1 s
 emote wrinkles her nose.
 wait 2 s
 say What do you want... stranger.
elseif (%actor.varexists(zantrain)%)
 wait 1 s
 say Hello %actor.name%, ready for some more training?
else
 wait 1 s
 say Why hello there stranger.
 wait 3 s
 say You've happened upon the greatest assassin of all time.
 wait 2 s
 say And I happen to be in a good mood, no less.
 wait 2 s
 say Tell you what. Bring me the hide of a legendary creature known as Fearramorphus and I might teach you a trick that'll put the fear into your enemies.
 wait 4 s
 say Well gone on then. Don't stand around here gawking.
 wait 2 s
 say I might change my mind and just kill you instead..
 wait 2 s
 emote waves you away.
end
~
$~
