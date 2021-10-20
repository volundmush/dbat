#19500
Elevator Trigger~
2 c 100
push~
if (%arg% == button)
wait 1
%send% %actor% You push the button. After the elevator door opens you step on it and the doors close.
%echoaround% %actor% %actor.name% pushes the panel button and steps on an elevator, the doors closing behind %actor.himher%.
 if (%self.vnum% == 19564)
  %teleport% %actor% 19565
 elseif (%self.vnum% == 19565)
  %teleport% %actor% 19564
 end
wait 1 s
%send% %actor% The elevator dings and you step out before the doors close behind you.
%echoaround% %actor% The elevator dings and %actor.name% steps out, the doors closing behind %actor.himher%.
else
return 0
end
~
#19501
Receptionist~
0 g 100
~
if (!%actor.varexists(CCS)%)
wait 1 s
say Greetings and welcome to Capsule Corporation HQ.
wait 4 s
say If you have an appointment with Bulma or Dr. Briefs you may attend to it now.
wait 4 s
say If not then you need to @Gschedule@C an appointment with me.@n
elseif (%direction% == north)
rdelete CCS %actor.id%
else
wait 1
say %actor.name%, you still have your appointment.
wait 3 s
say I suggest you hurry along with it.
wait 1 s
smile
end
~
#19502
Receptionist Schedule~
0 c 100
schedule~
if (!%actor.varexists(CCS)%)
wait 1
say Alright, %actor.name%, it seems we can fit you in right now.
wait 3 s
say You may go on through.
set CCS
remote CCS %actor.id%
else
wait 1
say You already have an appointment, %actor.name%. I can see your name written right here.
end
~
#19503
Receptionist Leave~
0 q 100
~
if (!%actor.varexists(CCS)% && %direction% == north)
return 0
wait 1
say You can not go further until you have @Gschedule@Cd an appointment.@n
elseif (%direction% == north)
return 0
wait 1
say You may enter now, %actor.name%.
wait 1 s
%force% %actor% north
else
return 1
end
~
#19504
Dr. Briefs Greet~
0 g 100
~
wait 1
say Hmm...no..this can't be right.
wait 3 s
say I don't see how that would be better...
wait 4 s
%echoaround% %actor% %self.name% looks up from his work at %actor.name%
%send% %actor% %self.name% looks up from his work at you.
wait 2 s
say Ah, are you the young person who is supposed to bring me the chrono prism?
wait 5 s
say I can't finish my latest invention without it and I put in an order a week ago for the prism.
wait 6 s
say They are hard to come by I know, but if one can't be found I will have to buy one from Techirus here in west city
wait 6 s
say If you manage to get one for me, I will give you a TX-15 microprocessor. It isn't on the market yet, and not even Bulma has gotten to see it.
~
#19505
Dr. Briefs Receive~
0 j 100
~
if (%object.vnum% == 19502)
%purge% chrono
wait 1
say AH so you have brought one. Thanks, now I can finish this prototype.
wait 4 s
say Well here, this is a TX-15 microprocessor, the most advanced yet made. I am the only other person to have seen it. We are going to begin mass production in a month or so anyway.
wait 4 s
%load% obj 19503
give TX-15 %actor.name%
else
wait 1
say Well I don't really want this, but alright...
end
~
#19506
Bulma Greet~
0 g 100
~
wait 1
sigh
wait 2 s
%echo% %self.name% tinkers with a half complete dragon radar.
wait 3 s
%echoaround% %actor% %self.name% looks up at %actor.name%
%send% %actor% %self.name% looks up at you.
wait 1 s
say Who are you? A friend of daddy's?
wait 3 s
say Can you believe he won't let me see the new TX-15 microprocessor?
wait 4 s
say After all the projects I have been involved on...
wait 3 s
say You know what, I would give anything to see that processor.
wait 5 s
say I know it is just a microprocessor, but the fact that he wants to keep it secret from me...
wait 1 s
%echo% %self.name% scrunches her face in frustration.
~
#19507
Bulma Receive~
0 j 100
~
if (%object.vnum% == 19503)
%purge% microprocessor
wait 1
say Wow, how did you get a hold of this?
wait 3 s
say Doesn't really matter, now I get to see what the big deal was all about.
wait 5 s
say Here before I forget.
wait 2 s
%echo% %self.name% quickly finishes the dragon radar she was working on.
wait 4 s
say This was going to be a spare for me, but in thanks take it.
wait 4 s
say It will let you @Gdetect@C the location of dragonballs. If you are not interested in those you could always sell it, I am sure someone would want it.@n
wait 5 s
%load% obj 12
give radar %actor.name%
else
wait 1
say I don't want this.
drop %object.name%
end
~
#19508
Mrs. Briefs Greet~
0 g 100
~
wait 1
say Ah hello there...
wait 2 s
say Would you like some cake, or tea?
wait 4 s
say Just say @Gplease@C, or nod your head, and you can have some.@n
wait 1 s
smile
~
#19509
Mrs. Briefs Speech~
0 d 100
please~
if (%speech% == please && !%actor.varexists(cake)%)
wait 1
say Here you go, enjoy.
wait 1 s
%load% obj 19504
%load% obj 19505
set cake
remote cake %actor.id%
give cake %actor.name%
give tea %actor.name%
elseif (%speech% == please && %actor.varexists(cake)%)
wait 1
say Sorry but I am all out of cake and tea. You already got some.
else
return 0
end
~
#19510
Mrs Briefs Mute~
0 c 100
nod~
if (%cmd% == nod && !%actor.varexists(cake)%)
%send% %actor% You nod your head, indicating you want cake and tea.
%echoaround% %actor% %actor.name% Nods %actor.hisher% head, indicating %actor.heshe% wants cake and tea.
wait 1
say Here you go, enjoy.
wait 1 s
%load% obj 19504
%load% obj 19505
set cake
remote cake %actor.id%
give cake %actor.name%
give tea %actor.name%
elseif (%cmd% == nod && %actor.varexists(cake)%)
wait 1
say Sorry but I am all out of cake and tea. You already got some.
end
~
#19511
Ticket Seller Greet~
0 g 100
~
if (!%self.varexists(tournament)%)
wait 1
say If you would like to @Gregister@C to fight in the tournament, you may do so for free.@n
wait 4 s
say If you would like to watch the tournament when it gets underway, you may @Gbuy@C a @Gticket@C here now for 20 zenni, and enter.@n
elseif (%actor.varexists(registered)% && %self.tournament% != %actor.id%)
rdelete registered %actor.id%
wait 2
say If you would like to watch the tournament already underway you may @Gbuy @Ca @Gticket@C.@n
wait 4 s
say It is too late to register for the tournament at this time.
else
wait 1
say If you would like to watch the tournament already underway you may @Gbuy @Ca @Gticket@C.@n
wait 4 s
say It is too late to register for the tournament at this time.
end
~
#19512
Ticket Seller Leave~
0 q 100
~
if (%direction% == north && %actor.varexists(registered)%)
 wait 1
 %send% %actor% You are no longer registered since you have left.
 rdelete registered %actor.id%
 if (%self.varexists(tournament)%)
  if (%actor.id% == %self.tournament%)
   rdelete tournament %self.id%
  end
 end
elseif (%direction% == south)
 if (!%actor.varexists(registered)% && !%actor.has_item(19506)%)
  return 0
  wait 1
  say You are neither registered for the tournamanet or have a ticket. You may not pass.
 elseif (%actor.varexists(registered)% && !%self.varexists(tournament)%)
  return 0
  if (!%self.varexists(tournament)%)
   set tournament %actor.id%
   remote tournament %self.id%
  end
  wait 1
  say Greetings, %actor.name%, you are registered for the tournament. You may enter now.
  wait 1 s
  %force% %actor% south
 elseif (%actor.varexists(registered)% && %self.varexists(tournament)%)
  if (%self.tournament% != %actor.id%)
   return 0
   wait 1
   say You are neither registered for the tournamanet or have a ticket. You may not pass.
   rdelete registered %actor.id%
  else
   return 0
   wait 1
   say Greetings, %actor.name%, you are registered for the tournament. You may enter now.
   wait 1 s
   %force% %actor% south
  end
 elseif (%actor.has_item(19506)%)
  return 0
  wait 1
  wait 1 s
  %force% %actor% south
 end
end
~
#19513
Register for Tourney~
0 c 100
register~
if (%self.varexists(tournament)%)
 if (%actor.varexists(registered)% && %self.tournament% != %actor.id%)
  rdelete registered %actor.id%
 end
wait 1
say The tournament has already started, sorry.
elseif (%actor.varexists(champion)%)
wait 1
say Alright....wait.. You are already a champion.
wait 2 s
say Sorry but the tournament should have been notified before hand.
wait 4 s
say VIP participants must register through the chairman. Sorry.
elseif (%actor.maxhitp% < 400000)
wait 1
say You do not qualify. Sorry but you have no hope of winning at your powerlevel.
elseif (!%actor.varexists(registered)%)
wait 1
say Alright, %actor.name%, you have been seeded to fight in the semi-finals and finals. How lucky...
set registered
remote registered %actor.id%
eval num %actor.id%
set tournament %num%
remote tournament %self.id%
else
wait 1
say You are already registered, %actor.name%. What are you still doing out here?
set tournament %actor.id%
remote tournament %self.id%
end
~
#19514
Take Ticket~
2 g 100
~
if (%actor.has_item(19506)%)
wait 1
%send% %actor% Your ticket is processed and you no longer have it.
%force% %actor% drop ticket
%purge% ticket
end
~
#19515
Prep Room Entry~
2 q 100
~
if (%direction% == south)
if (%actor.varexists(registered)%)
return 0
wait 1
%send% %actor% You are shown into the preparation room for tournament fighters.
wait 1
%force% %actor% south
else
return 0
wait 1
%send% %actor% You are not premitted into that area as you are not registered to fight.
end
end
~
#19516
Boomer Fight Trigger~
0 k 100
~
eval tactics %random.6%
wait 1
switch %tactics%
 case 1
  %zoneecho% 3800 %echo% Boomer falls to his knees for a moment before recovering!
 case 2
  %zoneecho% 3800 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
 break
 case 3
  %zoneecho% 3800 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
 break
 case 4
  %zoneecho% 3800 %echo% %actor.name% and Boomer lock hands together as they struggle!
 break
 case 5
  %zoneecho% 3800 %echo% Boomer tries to grab %actor.name% but fails!
 break
 case 6
  %zoneecho% 3800 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
 break
done 
~
#19517
Ticket Buy~
0 c 100
buy~
if (%arg% == ticket)
 if (%actor.gold% >= 20)
  wait 1
  say Alright here you go.
  wait 1 s
  %load% obj 19506
  give ticket %actor.name%
  set %actor.gold(-20)%
 else
  wait 1
  say The ticket is 20 zenni. You do not have enough.
 end
else
return 0
end
~
#19518
Announcer Greet~
0 g 100
~
if (%direction% == north)
 wait 1 s
 %echo% The announcer holds up the microphone to his mouth.
 wait 3 s
 %zoneecho% 3800 The announcer yells over the sound system, '@CAlright, we have our final competitor who has been seeded to the semi-finals!@n'
 wait 3 s
 %echoaround% %actor% The announcer lowers the microphone and looks at %actor.name%
 %send% %actor% The announcer lowers the microphone and looks at you.
 wait 3 s
 say Alright, one match is over in the semi finals already. You are up next.
 wait 4 s
 say Head south to meet your first opponent, Boomer.
elseif (!%self.varexists(matchwon)% && %direction% == south)
 return 0
 wait 1
 %send% %actor% You need to finish your match. If you don't you must forfeit to leave.
 %zoneecho% 3800 The announcer yells over the sound system, '@C%actor.name% is trying to run away! I wonder if he will forfeit!@n'
elseif (%self.varexists(match2)% && %direction% == south)
 wait 1
 %zoneecho% 3800 The announcer yells over the sound system, '@CCongratulation to %actor.name% for %actor.hisher% victory over Boomer!@n'
 rdelete matchwon %self.id%
 wait 4 s
 %zoneecho% 3800 The announcer yells over the sound system, '@CIt seems Valeria has won her match against Goom!@n'
 wait 2 s
 %zoneecho% 3800 The announcer yells over the sound system, '@CAlright the next match is Valeria vs %actor.name%!@n'
 wait 1 s
 %send% %actor% You should go back out to the ring.
elseif (%self.varexists(match3)% && %direction% == south)
 wait 1
 %zoneecho% 3800 The announcer yells over the sound system, '@CCongratulation to %actor.name% for %actor.hisher% victory over Valeria!@n'
 rdelete matchwon %self.id%
 wait 4 s
 %zoneecho% 3800 The announcer yells over the sound system, '@CIt seems Rohxa has won his match against Bulvo!@n'
 wait 2 s
 %zoneecho% 3800 The announcer yells over the sound system, '@CAlright the next match is Rohxa vs %actor.name%!@n'
 wait 1 s
 %send% %actor% You should go back out to the ring.
elseif (%self.varexists(match4)% && %direction% == south)
 wait 1
 %zonecho% 3800 The announcer yells over the sound system, '@CCongratulation to %actor.name% for %actor.hisher% victory over Rohxa!@n'
 rdelete matchwon %self.id%
 wait 4 s
 %zoneecho% 3800 The announcer yells over the sound system, '@CIt seems Vox easily won his match against Dorroha!@n'
 wait 2 s
 %zoneecho% 3800 The announcer yells over the sound system, '@CAlright the next match is Vox vs %actor.name%!@n'
 wait 1 s
 %send% %actor% You should go back out to the ring.
elseif (%self.varexists(victory)% && %direction% == south)
 wait 1
 %zoneecho% 3800 The announcer yells over the sound system, '@CCongratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!@n'
 rdelete matchwon %self.id%
 wait 3 s
 say For winning the whole tournament, here is what you have won!
 set %actor.gold(1000)%
 wait 3 s
 say Here is the one thousand zenni grand prize, the championship belt, and nano-diode!
 wait 1 s
 %send% %actor% %self.name% hands you a stack of zenni.
 wait 1 s
 %load% obj 19507
 %load% obj 19508
 give belt %actor.name%
 give nano %actor.name%
 wait 2 s
 %teleport% %actor% 3801
 rdelete registered %actor.id%
 set champion
 remote champion %actor.id%
 mgoto 3800
 %purge% seller
 %load% mob 19505
 mgoto 3825
 %load% mob 19506
 %purge% self
end
~
#19519
Match 1 Leave~
0 q 100
~
if (%actor.varexists(registered)% && %direction% == north)
return 0
wait 1
say To leave you must @Gforfeit@C.@n
else
if ((!%self.varexists(match2)% && !%self.varexists(match3)% && !%self.varexists(match4)%)&& %direction% == south)
return 0
wait 1
  %echo% The announcer raises the microphone towards his mouth.
wait 2 s
  %zoneecho% 3800 The speaker system crackles to life.
  %zoneecho% 3800 The announcer yells over the sound system, '@CAlright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!@n'
wait 3 s
  %teleport% %actor% 3826
  %force% %actor% look
  %send% %actor% You step out into the bright light of the tournament ring.
  %zoneecho% 3800 The warrior called %actor.name% can be seen stepping out onto the ring floor.
wait 3 s
  %send% %actor% You can see the giant wrestler Boomer standing at the opposite side of the ring.
  %zoneecho% 3800 The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
  %at% 3830 %load% mob 19507
wait 4 s
%zoneecho% 3800 The announcer yells over the sound system '@CLet the fighting commence!@n'
set match1
remote match1 %self.id%
elseif (%self.varexists(match2)% && %direction% == south)
return 0
wait 1
  %at% 3800 The announcer yells over the sound system '@C%actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!@n'
  %teleport% %actor% 3826
  %force% %actor% look
  %at% 3830 %load% mob 19508
wait 2 s
  %send% %actor% You step out onto the ring and see Valeria standing at the opposite end, one hand on hip.
  %zoneecho% 3800 The announcer yells over the sound system '@C%actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.@n'
wait 1 s
  %zoneecho% 3800 The announcer yells over the sound system '@CLet the fighting commence!@n'
elseif (%self.varexists(match3)% && %direction% == south)
return 0
wait 1
say %actor.name% prepares to face the mysterious Roxha!
  %zoneecho% 3800 The announcer yells over the sound system '@C%actor.name% prepares to face the mysterious Roxha!@n'
wait 2 s
  %teleport% %actor% 3826
  %force% %actor% look
  %at% 3830 %load% mob 19509
  %send% %actor% You step out onto the ring and see Roxha at the other end ignoring you.
  %zoneecho% 3800 The announcer yells over the sound system '@C%actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%@n'
wait 1 s
  %zoneecho% 3800 The announcer yells over the sound system '@CLet the fighting commence!@n'
elseif (%self.varexists(match4)% && %direction% == south)
return 0
wait 1
%zoneecho% 3800 The announcer yells over the sound system '@C%actor.name% prepares to face the fearsome Vox!@n'
  wait 2 s
  %teleport% %actor% 3826
  %force% %actor% look
  %at% 3830 %load% mob 19510
  %send% %actor% You see Vox standing opposite of you with his arms crossed.
  %zoneecho% 3800 The announcer yells over the sound system '@C%actor.name% steps out onto the ring and notices the great Vox already out there!@n'
wait 1 s
%zoneecho% 3800 The announcer yells over the sound system '@CLet the fighting commence!@n'
else
return 0
wait 1
say You can't go out there, you are not permitted!
end
end
~
#19520
Boomer Death~
0 f 100
~
mgoto 3825
set ann %self.room.people%
  %zoneecho% 3800 %echo% %actor.name% stands triumphant over the defeated Boomer!
while %ann%
  set tmp_ann %ann.next_in_room%
  if (%ann.vnum% == 19506)
   set announcer %ann%
  end
   set ann %tmp_ann%
 done
 if (%announcer% && %announcer.varexists(match1)%)
  rdelete match1 %announcer.id%
  set match2
  remote match2 %announcer.id%
  set matchwon
  remote matchwon %announcer.id%
  %teleport% %actor% 3826
  %force% %actor% north
end
~
#19521
Valeria Death~
0 f 100
~
mgoto 3825
set ann %self.room.people%
  %zoneecho% 3800 %actor.name% stands triumphant over the defeated Valeria!
while %ann%
  set tmp_ann %ann.next_in_room%
  if (%ann.vnum% == 19506)
   set announcer %ann%
  end
   set ann %tmp_ann%
 done
 if (%announcer% && %announcer.varexists(match2)%)
  rdelete match2 %announcer.id%
  set match3
  remote match3 %announcer.id%
  set matchwon
  remote matchwon %announcer.id%
  %teleport% %actor% 3826
  %force% %actor% north
 end
~
#19522
Rohxa Death~
0 f 100
~
mgoto 3825
set ann %self.room.people%
  %zoneecho% 3800 %actor.name% stands triumphant over the defeated Rohxa!
while %ann%
  set tmp_ann %ann.next_in_room%
  if (%ann.vnum% == 19506)
   set announcer %ann%
  end
   set ann %tmp_ann%
 done
 if (%announcer% && %announcer.varexists(match3)%)
  rdelete match3 %announcer.id%
  set match4
  remote match4 %announcer.id%
  set matchwon
  remote matchwon %announcer.id%
  %teleport% %actor% 3826
  %force% %actor% north
 end
~
#19523
Vox Death~
0 f 100
~
mgoto 3825
set ann %self.room.people%
  %zoneecho% 3800 %actor.name% stands triumphant over the defeated Vox!
while %ann%
  set tmp_ann %ann.next_in_room%
  if (%ann.vnum% == 19506)
   set announcer %ann%
  end
   set ann %tmp_ann%
 done
 if (%announcer% && %announcer.varexists(match4)%)
  rdelete match4 %announcer.id%
  set victory %actor.id%
  remote victory %announcer.id%
  set matchwon
  remote matchwon %announcer.id%
  %teleport% %actor% 3826
  %force% %actor% north
 end
~
#19524
Valeria Fight Trigger~
0 k 100
~
eval tactics %random.6%
wait 1
switch %tactics%
 case 1
  %zoneecho% 3800 %actor.name% manages to avoid an attack from Valeria!
 break
 case 2
  %zoneecho% 3800 %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
 break
 case 3
  %zoneecho% 3800 Valeria spits some blood onto the ring after an attack by %actor.name%
 break
 case 4
  %zoneecho% 3800 Valeria leaps out of the way of %actor.name%'s attack!
 break
 case 5
  %zoneecho% 3800 Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
 break
 case 6
  %zoneecho% 3800 Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
 break
done
~
#19525
Roxha Fight Trigger~
0 f 100
~
eval tactics %random.6%
wait 1
switch %tactics%
 case 1
  %zoneecho% 3800 Roxha easily dodges an attack by %actor.name%, while cackling.
 break
 case 2
  %zoneecho% 3800 %actor.name% barely manages to avoid a devestating attack by Roxha!
 break
 case 3
  %zoneecho% 3800 %actor.name% looks confused as Roxha begins to move his hands in a strange way!
 break
 case 4
  %zoneecho% 3800 Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
 break
 case 5
  %zoneecho% 3800 Roxha leaps backwards and almost falls out of the ring!
 break
 case 6
  %zoneecho% 3800 %actor.name% flips out of the way of Roxha's latest attack!
 break
done
~
#19526
Vox Fight Trigger~
0 f 100
~
eval tactics %random.6%
wait 1
switch %tactics%
 case 1
  %zoneecho% 3800 Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
 break
 case 2
  %zoneecho% 3800 Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
 break
 case 3
  %zoneecho% 3800 Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
 break
 case 4
  %zoneecho% 3800 %actor.name% manages to avoid a series of attack from Vox!
 break
 case 5
  %zoneecho% 3800 %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
 break
 case 6
  %zoneecho% 3800 Vox manages to block a series of attacks from %actor.name%!
 break
done
~
#19527
Clean Tournament~
0 b 80
~
if (%self.varexists(tournament)%)
 if (%people.3825% >= 2)
 eval num1 1
 elseif (%people.3825% <= 1)
 eval num1 0
 end
 eval num2 %people.3826%
 eval num3 %people.3827%
 eval num4 %people.3828%
 eval num5 %people.3829%
 eval num6 %people.3830%
 eval num7 %people.3831%
 eval num8 %people.3832%
 eval num9 %people.3833%
 eval numx %people.3834%
 eval numf %num2% + %num3% + %num4% + %num5% + %num6% + %num7% + %num8% + %num9% + %numx%
if (%num1% == 1)
 if (%self.varexists(count)%)
  rdelete count %self.id%
 end
elseif (%numf% >= 2)
 if (%self.varexists(count)%)
  rdelete count %self.id%
 end
elseif (%numf% <= 1)
 if (%self.varexists(count)%)
  if (%self.count% >= 10)
  %zoneecho% 3800 The tournament has an opening again because one participant is missing.
  %at% 3826 %purge%
  %at% 3827 %purge%
  %at% 3828 %purge%
  %at% 3829 %purge%
  %at% 3830 %purge%
  %at% 3831 %purge%
  %at% 3832 %purge%
  %at% 3833 %purge%
  %at% 3834 %purge%
   mgoto 3825
   %purge% announcer
   %load% mob 19506
   mgoto 3800
   %load% mob 19505
   %purge% self
  else
   eval plus %self.count% + 1
   set count %plus%
   remote count %self.id%
  end
  else
  set count 1
  remote count %self.id%
  end
end
end
~
#19528
Forfeit Trigger~
2 c 100
forfeit~
if (%actor.varexists(registered)%)
 rdelete registered %actor.id%
end
wait 1
%send% %actor% You forfeit and leave the ring.
%teleport% %actor% 3801
  %zoneecho% 3800 %actor.name% forfeits and leaves the ring in shame!
%at% 3825 %purge% announcer
%at% 3825 %load% mob 19506
%at% 3800 %purge% seller
%at% 3800 %load% mob 19505
%at% 3826 %purge%
%at% 3827 %purge%
%at% 3828 %purge%
%at% 3829 %purge%
%at% 3830 %purge%
%at% 3831 %purge%
%at% 3832 %purge%
%at% 3833 %purge%
%at% 3834 %purge%
~
$~
