#1300
Yamcha Greet~
0 g 100
~
if (%actor.align% < -50)
 wait 1 s
 say Hello...What do you want?
elseif (%actor.varexists(ytrain)%)
 wait 1 s
 say Hello %actor.name%, ready for some more training?
else
 wait 1 s
 say Hello.
 wait 3 s
 say If you would like to train with me I know a few skills I might teach ya.
 wait 2 s
 say You have to do me a favor first though.
 wait 2 s
 say Mrs. Brief has the best cake around. If you bring me some I will let you train with me.
 wait 4 s
 say Where is Mrs. Brief? Are you kidding? You do know about Capsule Corp. right?
 wait 5 s
 say She is the wife of Dr. Brief, so you will find her at Capsule Corp. Headquarters.
 wait 5 s
 say I would go myself but I am in the doghouse with a certain someone living there...
end
~
#1301
Puar Random~
0 b 25
~
wait 2 sec
say You're the greatest Yamcha!
wait 5 sec
say Oh Yamcha, don't you ever get tired?
wait 2 sec
clap yamcha
~
#1302
Yamcha Receive~
0 j 100
~
if (%actor.align% < -50)
 return 0
 wait 1
 say I don't want anything from scum like you.
elseif (%actor.varexists(ytrain)%)
 wait 1
 %purge% %object%
 say You already can train with me, but thanks for the extra cake.
 wait 2 s
 %echo% %self.name% gobbles down the cake.
elseif (%object.vnum% == 19504)
 wait 1
 %purge% %object%
 say Oh great you got some!
 wait 1 s
 %echo% %self.name% gobbles down the cake.
 wait 2 s
  if (%actor.sex% == MALE || %actor.sex% == NEUTRAL)
   set dude dude
  elseif (%actor.sex% == FEMALE)
   set dude chick
  end
 say Alright you are one ok %dude%. You can train with me.
 set ytrain
 remote ytrain %actor.id%
else
 return 0
 wait 1
 say I don't want that.
end
~
#1303
Yamcha Command~
0 c 100
pr~
if (%actor.align% < -50)
 return 1
 wait 1
 say I won't train scum like you.
elseif (!%actor.varexists(ytrain)%)
 return 1
 wait 1
 say You can't train with me until you bring me the cake like I asked.
else
 return 0
end
~
$~
