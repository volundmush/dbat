#15000
Tsuna Greet~
0 g 100
~
if (%actor.level% <= 2)
 wait 1
 say Hello there %actor.name%, you are one of my newest students. If you need any help getting started speak with Klvant, he will direct you on where to start.
 if !%actor.varexists(tsunaeq)%
  wait 2 sec
  say Now %actor.name% here is the official symbol of my students.
  wait 1 sec
  mload obj 15000
  give symbol %actor.name%
  set tsunaeq
  remote tsunaeq %actor.id%
 end
elseif (%actor.level% < 5)
 wait 1
 say %actor.name% you are still pretty new, make sure to do your best. Practice your hydro manipulation and don't neglect your basic fighting skills.
 if %actor.varexists(tsunaeq)%
  rdelete tsunaeq %actor.id%
 end
elseif (%actor.level% < 10)
 wait 1
 say %actor.name% keep training your hydro manipulation techniques. You are doing well so far.
elseif (%actor.level% < 20)
 wait 1
 say %actor.name% you are doing really well, you are one of my most dedicated students.
elseif (%actor.level% < 40)
 wait 1
 say %actor.name% you do me proud with your dedication. You are an exceptional student.
elseif (%actor.level% < 75)
 wait 1
 say %actor.name% you are one of my best students, make sure to be an example to the rest.
elseif (%actor.level% < 100)
 wait 1
 say %actor.name% you are nearly a master yourself, keep it up.
elseif (%actor.level% == 100)
 wait 1
 say %actor.name% you are a Hydro Master. You should be out putting your skills to use.
 smile
else
 wait 1
 say %actor.name% I don't know what to say about you.
end
~
#15001
Klvant Guard~
0 q 100
~
if (%direction% == inside && %actor.class% != Tsuna)
 return 0
 wait 1
 say You may not enter, you are not one of Lady Tsuna's students.
elseif (%actor.carry%)
 return 0
 wait 1
 say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == inside && %actor.class% == Tsuna)
 wait 1
 say Welcome in friend.
 %send% %actor% @bKlvant@C says, 'Welcome in friend.'
 return 1
end
~
#15002
Klvant Greet~
0 g 100
~
if (%actor.level% <= 2 && %actor.class% == Tsuna)
 if (!%actor.varexists(tsunafood)%)
  wait 1
  say Ah you must be a new student. If you need it, just ask for some @Gfood@C to help you keep your strength up for training.@n
  wait 2 s
  say Lady Tsuna's training can be quite rigorous.
  wait 2 s
  say Since you are new I guess I should give you some advice.
  wait 1 s
  say You should start out by training with some of your fellow students in the school.
  wait 2 s
  say When you feel ready for some more advanced training partners you can try the temple.
  wait 2 s
  say There are some more advanced Hydromancers there.
  wait 1 s
  say You can find it at the southwest edge of the city, on Sylph Street.
 else
  wait 1
  say Hello, friend.
 end
elseif (%actor.class% == Tsuna)
 wait 1
 say Hello, friend.
elseif (%actor.race% != Kanassan)
 set target_char %self.room.people%
 eval found 0
 while %target_char%
  set tmp_char %target_char.next_in_room%
  if (%target_char.vnum% == 15004)
   eval found 1
  end
  set target_char %tmp_char%
 done
 if (%found% == 0)
  say What are you doing around here? We don't like strange races wandering around our sacred city!
  wait 1
  %echo% %self.name% calls for a guard!
  %load% m 15004
 end
else
say If you aren't a student of this school you shouldn't be snooping around here.
end
~
#15003
Aquis Guard Trigger~
0 ab 100
~
if (%actor.is_pc%)
if (!%self.varexists(fighting)%)
 set target_char %self.room.people%
 while (%target_char% && !%self.varexists(fighting)%))
  set tmp_tar %target_char.next_in_room%
  if (%target_char.race% != Kanassan)
   slam %target_char.name%
   set fighting
   remote fighting %self.id%
  else
   set target_char %tmp_tar%
  end
 done
elseif (!%self.fighting%)
 rdelete fighting %self.id%
end
end
~
#15004
Tarla Greet~
0 g 100
~
wait 1 sec
say What can I get you?
wait 2 sec
say Take your time deciding. Better that than wasting my time.
~
#15005
Priest greet~
0 g 100
~
wait 1
say ...and so Aquisa didst point to the sacred cave and say "Lo' and behold. Our refuge."
wait 2 s
say It was then that Aquisa and her people did settle in the sacred cave and establish themselves there in.
~
#15006
Move Shelf~
2 c 100
move~
if (%arg% == shelf)
 wait 1
 %send% %actor% You push the shelf to the side. An opening going down is revealed!
 %echoaround% %actor% %actor.name% pushes the shelf to the side. An opening going down is revealed!
 %door% 15057 5 flags a
 wait 5 sec
 %echo% The shelf moves back into place automatically.
 %door% 15057 5 flags abce
else
 wait 1
 %send% %actor% Move what?
end
~
#15007
High Priest Greet~
0 g 100
~
if (!%actor.varexists(kanpriestsided)% && !%actor.varexists(finsided)% && %actor.race% == kanassan)
wait 2 s
say @WYou there... @n
wait 2 s
%echo% %self.name% shuffles toward %actor.name%.
wait 2 s
say @WI am in need of some help.. of the most urgent kind.@n
wait 2 s
say @WYou'd be doing the city a great service, just know that your life would be placed in great danger..@n
wait 2 s
say @WPlease say you'll lend me some @Yassistance@W.@n
elseif (%actor.varexists(misoridead)% && %actor.varexists(kanpriest4)%
wait 2 s
%send% %actor% You relay all the information to the High Priest.
wait 2 s
say Hrm... I can't believe it. Kisame is still in the game and he is trying to take over the city! %actor.name%, you must put a stop to him at once!
rdelete misoridead %actor.id%
set killkisame
remote killkisame %actor.id%
elseif %actor.varexists(killedkisame)%
rdelete killedkisame %actor.id%
rdelete kanpriest4 %actor.id%
set kanpriest5
remote kanpriest5 %actor.id%
wait 2 s
say You did it, %actor.name%! You've saved the city. From the bottom of my heart I thank you.
wait 2 s
say But words are not enough... may I present you with a reward? Yes, I insist.
%load% obj 15048
give robe %actor.name%
end
~
#15008
High Priest induct and mission 1~
0 d 100
assistance~
if (!%actor.varexists(kanpriestsided)% && !%actor.varexists(finsided)% && %actor.race% == kanassan)
set kanpriestsided
remote kanpriestsided %actor.id%
set kanpriest1
remote kanpriest1 %actor.id%
wait 2 s
say @WThank you kind one. I shall get you started right away.@n
wait 2 s
say @WThere is a courier in town, carrying a package that contains vital information. Tell him Asoku sent you and he'll give you the package.@n
wait 2 s
say @WThen bring the package straight to me.@n
end
~
#15009
Courier gives package~
0 d 100
asoku~
if (%actor.varexists(kanpriest1) && !%actor.varexists(kanpackrec)%)
wait 1 s
say @WOh, the High Priest sent you? Thank Tsuna! I can't wait to be rid of this package.@n
wait 2 s
emote searches in his satchel bag before producing a brown paper package.
wait 2 s
%load% obj 15046
give package %actor.name%
set kanpackrec
remote kanpackrec %actor.id%
wait 1 s
say @WGorram relieved to be rid of that thing. Watch your back!@n
end
~
#15010
Priest accepts package, instructs player next move~
0 j 100
~
if (%actor.varexists(kanpackrec)% && %object.vnum% == 15046)
rdelete kanpackrec %actor.id%
rdelete kanpriest1 %actor.id%
set kanpriest2
remote kanpriest2 %actor.id%
%purge% package 
wait 2 s
emote opens the package and then examines the documents inside for a short while.
wait 2 s
say Well then, this is not what I expected. I guess I have to ask you to do something else..
wait 2 s
say You've heard of the Black Fin organization, haven't you? Sure, everyone has heard SOMETHING about them at least.
wait 2 s
say Suffice to say they are a nasty sort and they are planning something big as far as we can tell..
wait 2 s
say I need you to infiltrate their organization and gain intel on their plans so they can be stopped.
wait 2 s
say Try and do what they ask within reason. We don't know where their hideout is though, you'll have to search the city to find out.
elseif (%actor.varexists(kanevidence)% && %object.vnum% == 15047)
rdelete kanevidence %actor.id%
%purge% evidence
wait 2 s
say You were right to bring me this evidence... The Black Fin are definitely up to something.
rdelete kanpriest2 %actor.id%
set kanpriest3
remote kanpriest3 %actor.id%
wait 2 s
say I'm afraid you are going to have to continue with the infiltration. I'll publicly reprimand Muklu to make it seem like you completed your mission.
end
~
#15011
Recruiter recruits~
0 g 100
~
if %actor.race% == Kanassan
if !%actor.varexists(finsided)%
wait 2 s
say Hey you? Looking for some work?
wait 2 s
%send% %actor% %self.name% whispers to you, 'The Black Fin are recruiting. Good money. Just head over to the Dark Depths bar and talk to Stich. Be sure to say gorlinski to him when you get there.
else
wait 1 s
%echo% %self.name% seems to be ignoring you.
end
%echo% %self.name% glances at you coldly.
end
~
#15012
Stich shows them the shelf~
0 d 100
gorlinski~
if %actor.race% == kanassan
if !%actor.varexists(finsided)%
wait 2 s
say So you're looking for work then? Best follow me..
wait 2 s
%force% %actor% east
%teleport% %self% 15057
%send% %actor% %self.name% leads you to the storage room.
wait 2 s
say Right then.. see that shelf?
wait 1 s
%echo% %self.name% points at the shelf near the center of the room.
wait 2 s
say Move the shelf, speak to Mawl. He'll tell you what to do.
wait 1 s
%echo% %self.name% heads back to the bar.
%teleport% %self% 15056
end
else
wait 1 s
%send% %actor% %self.name% seems to ignore your presence.
end
~
#15013
Mawl's quest giving~
0 g 100
~
if %actor.race% == kanassan
if (!%actor.varexists(finsided)% && !%actor.varexists(kanpriestsided)%)
wait 2 s
%echo% %self.name% looks over you and grins.
wait 2 s
say What have we here? Fresh meat? You want to run with the big boys do you?
wait 2 s
%echo% %self.name% laughs a little before looking very serious.
wait 2 s
say If you want to join us, say join. Don't waste my time.
elseif (%actor.varexists(kanpriest2)% && !%actor.varexists(kanevidence)%)
wait 2 s
say What have we here? New prospect?
wait 2 s
say I got just the job for you. Do it and maybe, just maybe I'll let you join us as a recruit.
wait 2 s
%load% obj 15047
give evidence %actor.name%
set kanevidence
remote kanevidence %actor.id%
say Plant that on a priest called Muklu at the temple. Then head back here.
%send% %actor% @ROOC@n: @YThe High Priest might be interested in this.@n
elseif (%actor.varexists(kanpriest3)% && !%actor.varexists(hotakidead)%)
wait 2 s
say Well well, what do we know? You can get a job done after all...
wait 2 s
say That was toddler work though. But I figure I can trust you enough to give you another assignment.
wait 2 s
say There's a guy named Hotaki. I want him dead. Understand?
%send% %actor% @ROOC@n: @YYou should maybe @Wconvince@Y Hotaki to fake his own death.&n
nop %actor.gold(500)%
%actor.exp(1000)%
elseif (%actor.varexists(hotakidead)% && %actor.varexists(kanpriest3)%)
wait 2 s
say I hear Hotaki went out with a bang!
wait 2 s
say You obviously don't seem to be worried about leaving a mess. But the job's done.
rdelete hotakidead %actor.id%
rdelete kanpriest3 %actor.id%
set kanpriest4
remote kanpriest4 %actor.id%
nop %actor.gold(750)%
%actor.exp(5000)%
wait 2 s
say Next job.. lovely woman by the name of Misori. She used to be one of us, but she.. didn't 'perform' to the expectations demanded of her.
wait 2 s
%echo% %self.name% winks at you suggestively after saying that last bit.
wait 2 s
say She met someone, got out. But no one leaves the Black Fins. Unless it's in a casket. She missed that step. So go see that it is fulfilled.
%send% %actor% @ROOC@n: @YThere might be more to this than meets the eye.@n
elseif (%actor.varexists(finsided)% && %actor.varexists(courierdead)%)
wait 2 s
say Good job there boyo. Might be that you can make something of yourself yet.
rdelete courierdead %actor.id%
set blackfin1
remote blackfin1 %actor.id%
wait 2 s
say That makes you a member of the Black Fins! Well sorta. More like an apprentice if anything.
wait 2 s
set kanevidence
remote kanevidence %actor.id%
say Your next job is rather simple. Plant this on a priest called Muklu at the temple. Then head back here.
%send% %actor% @ROOC@n: @YUse the give command to complete this quest instead of plant.@n
%load% obj 15047
give evidence %actor.name%
elseif (%actor.varexists(blackfin1)% && %actor.varexists(evidenceplanted)%)
rdelete blackfin1 %actor.id%
rdelete evidenceplanted %actor.id%
set blackfin2
remote blackfin2 %actor.id%
wait 2 s
say Hahahaha, well done. That Muklu was a meddling bastard. Now he's out of our way.
wait 2 s
say Keep this up and we'll make a proper Black Fin out of you yet.
wait 2 s
say Your next job is simple enough. Need you to off a fella called Hotaki. See to it.
elseif (%actor.varexists(blackfin2)% && %actor.varexists(hotakidead)%)
wait 2 s
rdelete blackfin2 %actor.id%
rdelete hotakidead %actor.id%
set blackfin3
remote blackfin3 %actor.id%
say Hotaki's dead as desired.
nop %actor.gold(500)%
%actor.exp(1000)%
wait 2 s
say Next job.. lovely woman by the name of Misori. She used to be one of us, but she.. didn't 'perform' to the expectations demanded of her.
wait 2 s
%echo% %self.name% winks at you suggestively after saying that last bit.
wait 2 s
say She met someone, got out. But no one leaves the Black Fins. Unless it's in a casket. She missed that step. So go see that it is fulfilled.
elseif (%actor.varexists(blackfin3)% && %actor.varexists(misoridead)%)
rdelete blackfin3 %actor.id%
rdelete misoridead %actor.id%
set blackfin4
remote blackfin4 %actor.id%
wait 2 s
say Ahhhh, Misori is done for. Brilliant. After a job like that... well, I think the boss will be wanting to talk to you.
nop %actor.gold(750)%
%actor.exp(5000)%
wait 2 s
say Just head on through to the back office. Don't keep him waiting...
end
end
~
#15014
Hotaki nervous~
0 g 100
~
if %actor.varexists(kanpriest3)%
wait 1 s
say Please, please don't kill me!
wait 2 s
say Look, give me 100 zenni and I can disappear. Make it look like I'm dead. No need to kill me then right?
end
~
#15015
Hotaki bribe~
0 m 100
~
if (!%actor.varexists(hotakidead)% && %actor.varexists(kanpriest3)%)
set hotakidead
remote hotakidead %actor.id%
wait 2 s
say Thank you, thank you! You won't regret this, I promise.
wait 2 s
%echo% %self.name% runs off eagerly to fake his own death.
%purge% %self%
end
~
#15016
Misori hint greet~
0 g 100
~
if (%actor.varexists(kanpriest4)% && !%actor.varexists(misoridead)%)
%send% %actor% Try @Yinterrogate@n Misori!
end
~
#15017
Misori Interrogate~
0 c 100
interrogate~
if %arg% == %self.alias%
if (%actor.varexists(kanpriest4)% && !%actor.varexists(misoridead)%)
%send% %actor% You begin interrogating Misori. She seems more than willing to help.
wait 2 s
%send% %actor% After a few minutes, Misori reveals vital info that should be relayed to the High Priest at once!
set misoridead
remote misoridead %actor.id%
%echo% %self.name% sensually walks away from you.
%purge% %self%
else
%echo% %self.name% ignores you.
end
end
~
#15018
Kill Kisame~
0 f 100
~
if %actor.varexists(killkisame)%
rdelete killkisame %actor.id%
set killedkisame
remote killedkisame %actor.id%
%send% %actor% You should return to the High Priest now.
end
~
#15019
Kill courier prt 1~
0 d 100
join~
if %actor.race% == Kanassan
if (!%actor.varexists(finsided)% && !%actor.varexists(kanpriestsided)%)
wait 2 s
set finsided
remote finsided %actor.id%
say This doesn't make you a Black Fin just yet, %actor.name%. You need to do a job to prove your worth to us.
wait 2 s
say There's a courier somewhere in town that is going to make an important delivery. We'd like him.. dead.
set killcourier
remote killcourier %actor.id%
wait 2 s
say Come back when he's dead.
end
else
%echo% %self.name% seems to ignore you.
end
~
#15020
kill courier part 2~
0 f 100
~
if %actor.varexists(killcourier)%
rdelete killcourier %actor.id%
set courierdead
remote courierdead %actor.id%
%echo% %self.name% screams in agony before dieing.
end
~
#15021
Muklu evidence~
0 j 100
~
if (%actor.varexists(kanevidence)% &&  %object.vnum% == 15047)
%purge% evidence
rdelete kanevidence %actor.id%
set evidenceplanted
remote evidenceplanted %actor.id%
wait 1 s
%send% %actor% You plant the evidence on Muklu! You should return to Mawl now.
end
~
#15022
Kill Hotaki~
0 f 100
~
if (%actor.varexists(blackfin2)% && !%actor.varexists(hotakidead)%)
set hotakidead
remote hotakidead %actor.id%
%send% %actor% Head back to Mawl now.
end
~
#15023
Misori dies~
0 f 100
~
if (%actor.varexists(blackfin3)% && !%actor.varexists(misoridead)%)
set misoridead
remote misoridead %actor.id%
%send% %actor% Head back to Mawl now.
end
~
#15024
Kisame final quest~
0 g 100
~
if (%actor.varexists(blackfin4)% && !%actor.varexists(highpriestdead)%)
wait 2 s
say Why hello there? You're the one that Mawl has been telling me about aren't you?
wait 2 s
say Yes... gets the job done no matter what, I believe he said. Which is just what I need.
wait 2 s
say I've got big plans for this city, %actor.name%. Big plans that might just involve you one day.
wait 2 s
say That is, if you complete this job I have for you.
wait 2 s
say Go ice the High Priest and I'll reward you handsomely.
elseif (%actor.varexists(blackfin4)% && %actor.varexists(highpriestdead)%)
rdelete blackfin4 %actor.id%
rdelete highpriestdead %actor.id%
set blackfin5
remote blackfin5 %actor.id%
wait 2 s
say Ahhhhh brilliant. That pedantic fool is out of the way and now the city is mine to do with as I please.
wait 2 s
say But I should give you your reward before I celebrate, yes? Yes I should.
wait 2 s
say I'm honored to give you the title of Black Fin elite. Oh and take this.
%load% obj 15049
give badge %actor.name%
end
~
#15025
Kill the high priest!~
0 f 100
~
if (%actor.varexists(blackfin4) && !%actor.varexists(highpriestdead)%)
%echo% %self.name% says, 'You fool.. what have you done?' before dying.
set highpriestdead
remote highpriestdead %actor.id%
end
~
#15026
Klvant Food Give~
0 d 100
food~
if (!%actor.varexists(tsunafood1)% && !%actor.varexists(tsunafood)% && %actor.class% == Tsuna)
  wait 1
  say Right away.
  wait 2 sec
  emote hands %actor.name% a pouch.
  wait 2 sec
  say Enjoy.
  %load% obj 15001
  %load% obj 15001
  %load% obj 15001
  %load% obj 15001
  %load% obj 1614
  give all.shrimp %actor.name%
  give all.cup %actor.name%
  set tsunafood1
  remote tsunafood1 %actor.id%
elseif (!%actor.varexists(tsunafood2)% && !%actor.varexists(tsunafood)% && %actor.class% == Tsuna)
  wait 1 sec
  say You need some more? Okay.
  wait 2 sec
  emote hands %actor.name% a pouch.
  wait 2 sec
  say Enjoy.
  %load% obj 15001
  %load% obj 15001
  %load% obj 15001
  %load% obj 15001
  %load% obj 1614
  give all.shrimp %actor.name%
  give all.cup %actor.name%
  set tsunafood2
  remote tsunafood2 %actor.id%
elseif (!%actor.varexists(tsunafood3)% && !%actor.varexists(tsunafood)% && %actor.class% == Tsuna)
  wait 1 sec
  say This is the last bit of food I can give you. You'll need to find some on your own.
  wait 2 sec
  emote hands %actor.name% a pouch.
  wait 1 sec
  say Enjoy.
  %load% obj 15001
  %load% obj 15001
  %load% obj 15001
  %load% obj 15001
  %load% obj 1614
  give all.shrimp %actor.name%
  give all.cup %actor.name%
  rdelete tsunafood1 %actor.id%
  rdelete tsunafood2 %actor.id%
  set tsunafood
  remote tsunafood %actor.id%
else
  wait 1 sec
  say I cannot do that. I'm sorry.
end
~
#15027
Klvant Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(tsunafood1)% && !%actor.varexists(tsunafood)% && %actor.class% == Tsuna)
        wait 1
        say Right away.
        wait 2 sec
        emote hands %actor.name% a pouch.
        wait 2 sec
        say Enjoy.
        %load% obj 15001
        %load% obj 15001
        %load% obj 15001
        %load% obj 15001
        %load% obj 1614
        give all.shrimp %actor.name%
        give all.cup %actor.name%
        set tsunafood1
        remote tsunafood1 %actor.id%
    elseif (!%actor.varexists(tsunafood2)% && !%actor.varexists(tsunafood)% && %actor.class% == Tsuna)
        wait 1 sec
        say You need some more? Okay.
        wait 2 sec
        emote hands %actor.name% a pouch.
        wait 2 sec
        say Enjoy.
        %load% obj 15001
        %load% obj 15001
        %load% obj 15001
        %load% obj 15001
        %load% obj 1614
        give all.shrimp %actor.name%
        give all.cup %actor.name%
        set tsunafood2
        remote tsunafood2 %actor.id%
    elseif (!%actor.varexists(tsunafood3)% && !%actor.varexists(tsunafood)% && %actor.class% == Tsuna)
        wait 1 sec
        say This is the last bit of food I can give you. You'll need to find some on your own.
        wait 2 sec
        emote hands %actor.name% a pouch.
        wait 1 sec
        say Enjoy.
        %load% obj 15001
        %load% obj 15001
        %load% obj 15001
        %load% obj 15001
        %load% obj 1614
        give all.shrimp %actor.name%
        give all.cup %actor.name%
        rdelete tsunafood1 %actor.id%
        rdelete tsunafood2 %actor.id%
        set tsunafood
        remote tsunafood %actor.id%
    else
        wait 1 sec
        say I cannot do that. I'm sorry.
    end
else
    %send% %actor% Point at what?
end
~
$~
