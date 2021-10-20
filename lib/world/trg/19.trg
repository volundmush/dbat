#1900
Elevator Trigger~
2 c 100
push~
if (%arg% == button)
wait 1
%send% %actor% You push the button. After the elevator door opens you step on it and the doors close.
%echoaround% %actor% %actor.name% pushes the panel button and steps on an elevator, the doors closing behind %actor.himher%.
 if (%self.vnum% == 1972)
  %teleport% %actor% 1977
 elseif (%self.vnum% == 1977)
  %teleport% %actor% 1972
 end
wait 1 s
%send% %actor% The elevator dings and you step out before the doors close behind you.
%echoaround% %actor% The elevator dings and %actor.name% steps out, the doors closing behind %actor.himher%.
else
return 0
end
~
#1901
Receptionist~
0 g 100
~
if (!%actor.varexists(CCS)%)
wait 1 s
say Greetings and welcome to Capsule Corporation HQ.
wait 4 s
say If you have an appointment with Bulma or Dr. Briefs you may attend to it now.
wait 4 s
say If not then you need to @Gschedule@C an appointment with me.
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
#1902
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
#1903
Receptionist Leave~
0 q 100
~
if (!%actor.varexists(CCS)% && %direction% == north)
return 0
wait 1
say You can not go further until you have @Gschedule@Cd an appointment.
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
#1904
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
#1905
Dr. Briefs Receive~
0 j 100
~
if (%object.vnum% == 1902)
%purge% chrono
wait 1
say AH so you have brought one. Thanks, now I can finish this prototype.
wait 4 s
say Well here, this is a TX-15 microprocessor, the most advanced yet made. I am the only other person to have seen it. We are going to begin mass production in a month or so anyway.
wait 4 s
%load% obj 1903
give TX-15 %actor.name%
else
wait 1
say Well I don't really want this, but alright...
end
~
#1906
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
#1907
Bulma Receive~
0 j 100
~
if (%object.vnum% == 1903)
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
say It will let you @Gdetect@C the location of dragonballs. If you are not interested in those you could always sell it, I am sure someone would want it.
wait 5 s
%load% obj 12
give radar %actor.name%
else
wait 1
say I don't want this.
drop %object.name%
end
~
#1908
Mrs. Briefs Greet~
0 g 100
~
wait 1
say Ah hello there...
wait 2 s
say Would you like some cake, or tea?
wait 4 s
say Just say @Gplease@C and you can have some.
wait 1 s
smile
~
#1909
Mrs. Briefs Speech~
0 d 100
please~
if (%speech% == please && !%actor.varexists(cake)%)
wait 1
say Here you go, enjoy.
wait 1 s
%load% obj 1904
%load% obj 1905
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
#1910
Ticker Seller Greet~
0 g 100
~
if (!%self.varexists(tournament)%)
wait 1
say If you would like to @Gregister@C to fight in the tournament, you may do so for free.
wait 4 s
say If you would like to watch the tournament when it gets underway, you may @Gbuy @Ca @Gticket@C here now for 20 zenni, and enter.
elseif (%actor.varexists(registered)% && %self.tournament% != %actor.id%)
rdelete registered %actor.id%
wait 2
say If you would like to watch the tournament already underway you may @Gbuy @Ca @Gticket@C.
wait 4 s
say It is too late to register for the tournament at this time.
else
wait 1
say If you would like to watch the tournament already underway you may @Gbuy @Ca @Gticket@C.
wait 4 s
say It is too late to register for the tournament at this time.
end
~
#1911
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
 if (!%actor.varexists(registered)% && !%actor.has_item(1906)%)
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
 elseif (%actor.has_item(1906)%)
  return 0
  wait 1
  say Ah, you may enter.
  wait 1 s
  %force% %actor% south
 end
end
~
#1912
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
#1913
Take Ticket~
2 g 100
~
if (%actor.has_item(1906)%)
wait 1
%send% %actor% Your ticket is processed and you no longer have it.
%force% %actor% drop ticket
%purge% ticket
end
~
#1914
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
#1915
Boomer Fight Trigger~
0 k 20
~
eval tactics %random.6%
wait 1
switch %tactics%
 case 1
  %at% 1980 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1981 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1982 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1983 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1984 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1985 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1986 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1987 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1988 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1989 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1990 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1991 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1992 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1993 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1994 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1995 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1996 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1997 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1998 %echo% Boomer falls to his knees for a moment before recovering!
  %at% 1999 %echo% Boomer falls to his knees for a moment before recovering!
 break
 case 2
  %at% 1980 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1981 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1982 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1983 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1984 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1985 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1986 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1987 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1988 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1989 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1990 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1991 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1992 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1993 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1994 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1995 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1996 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1997 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1998 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
  %at% 1999 %echo% Boomer laughs before trying to smash %actor.name% with his boot!
 break
 case 3
  %at% 1980 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1981 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1982 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1983 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1984 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1985 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1986 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1987 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1988 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1989 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1990 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1991 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1992 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1993 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1994 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1995 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1996 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1997 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1998 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
  %at% 1999 %echo% %actor.name% recovers from an attack by Boomer, flipping to safety.
 break
 case 4
  %at% 1980 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1981 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1982 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1983 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1984 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1985 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1986 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1987 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1988 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1989 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1990 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1991 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1992 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1993 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1994 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1995 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1996 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1997 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1998 %echo% %actor.name% and Boomer lock hands together as they struggle!
  %at% 1999 %echo% %actor.name% and Boomer lock hands together as they struggle!
 break
 case 5
  %at% 1980 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1981 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1982 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1983 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1984 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1985 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1986 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1987 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1988 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1989 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1990 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1991 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1992 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1993 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1994 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1995 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1996 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1997 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1998 %echo% Boomer tries to grab %actor.name% but fails!
  %at% 1999 %echo% Boomer tries to grab %actor.name% but fails!
 break
 case 6
  %at% 1980 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1981 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1982 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1983 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1984 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1985 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1986 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1987 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1988 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1989 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1990 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1991 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1992 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1993 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1994 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1995 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1996 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1997 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1998 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
  %at% 1999 %echo% Boomer falls backwards, crunching some tiles of the ring floor in the process.
 break
done 
~
#1916
Ticket buy~
0 c 100
buy~
if (%arg% == ticket)
 if (%actor.gold% >= 20)
  wait 1
  say Alright here you go.
  wait 1 s
  %load% obj 1906
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
#1917
Announcer Greet~
0 g 100
~
if (%direction% == north)
wait 1 s
%echo% The announcer holds up the microphone to his mouth.
wait 3 s
say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1980 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1981 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1982 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1983 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1984 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1985 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1986 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1987 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1988 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1989 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1990 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1991 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1992 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1993 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1994 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1995 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1996 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1997 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1998 say Alright, we have our final competitor who has been seeded to the semi-finals!
  %at% 1999 say Alright, we have our final competitor who has been seeded to the semi-finals!
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
%send% %actor% You need to finish your match. If you don't you must @Gforfeit@n to leave.
  %at% 1980 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1981 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1982 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1983 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1984 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1985 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1986 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1987 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1988 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1989 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1990 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1991 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1992 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1993 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1994 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1995 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1996 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1997 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1998 say %actor.name% is trying to run away! I wonder if he will forfeit!
  %at% 1999 say %actor.name% is trying to run away! I wonder if he will forfeit!
elseif (%self.varexists(match2)% && %direction% == south)
wait 1
say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1980 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1981 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1982 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1983 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1984 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1985 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1986 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1987 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1988 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1989 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1990 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1991 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1992 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1993 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1994 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1995 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1996 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1997 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1998 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
  %at% 1999 say Congratulation to %actor.name% for %actor.hisher% victory over Boomer!
rdelete matchwon %self.id%
wait 4 s
say It seems Valeria has won her match against Goom!
  %at% 1980 say It seems Valeria has won her match against Goom!
  %at% 1981 say It seems Valeria has won her match against Goom!
  %at% 1982 say It seems Valeria has won her match against Goom!
  %at% 1983 say It seems Valeria has won her match against Goom!
  %at% 1984 say It seems Valeria has won her match against Goom!
  %at% 1985 say It seems Valeria has won her match against Goom!
  %at% 1986 say It seems Valeria has won her match against Goom!
  %at% 1987 say It seems Valeria has won her match against Goom!
  %at% 1988 say It seems Valeria has won her match against Goom!
  %at% 1989 say It seems Valeria has won her match against Goom!
  %at% 1990 say It seems Valeria has won her match against Goom!
  %at% 1991 say It seems Valeria has won her match against Goom!
  %at% 1992 say It seems Valeria has won her match against Goom!
  %at% 1993 say It seems Valeria has won her match against Goom!
  %at% 1994 say It seems Valeria has won her match against Goom!
  %at% 1995 say It seems Valeria has won her match against Goom!
  %at% 1996 say It seems Valeria has won her match against Goom!
  %at% 1997 say It seems Valeria has won her match against Goom!
  %at% 1998 say It seems Valeria has won her match against Goom!
  %at% 1999 say It seems Valeria has won her match against Goom!
wait 2 s
say Alright the next match is Valeria vs %actor.name%
  %at% 1980 say Alright the next match is Valeria vs %actor.name%
  %at% 1981 say Alright the next match is Valeria vs %actor.name%
  %at% 1982 say Alright the next match is Valeria vs %actor.name%
  %at% 1983 say Alright the next match is Valeria vs %actor.name%
  %at% 1984 say Alright the next match is Valeria vs %actor.name%
  %at% 1985 say Alright the next match is Valeria vs %actor.name%
  %at% 1986 say Alright the next match is Valeria vs %actor.name%
  %at% 1987 say Alright the next match is Valeria vs %actor.name%
  %at% 1988 say Alright the next match is Valeria vs %actor.name%
  %at% 1989 say Alright the next match is Valeria vs %actor.name%
  %at% 1990 say Alright the next match is Valeria vs %actor.name%
  %at% 1991 say Alright the next match is Valeria vs %actor.name%
  %at% 1992 say Alright the next match is Valeria vs %actor.name%
  %at% 1993 say Alright the next match is Valeria vs %actor.name%
  %at% 1994 say Alright the next match is Valeria vs %actor.name%
  %at% 1995 say Alright the next match is Valeria vs %actor.name%
  %at% 1996 say Alright the next match is Valeria vs %actor.name%
  %at% 1997 say Alright the next match is Valeria vs %actor.name%
  %at% 1998 say Alright the next match is Valeria vs %actor.name%
  %at% 1999 say Alright the next match is Valeria vs %actor.name%
wait 1 s
%send% %actor% You should go back out to the ring.
elseif (%self.varexists(match3)% && %direction% == south)
wait 1
say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1980 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1981 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1982 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1983 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1984 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1985 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1986 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1987 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1988 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1989 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1990 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1991 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1992 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1993 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1994 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1995 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1996 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1997 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1998 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
  %at% 1999 say Congratulation to %actor.name% for %actor.hisher% victory over Valeria!
rdelete matchwon %self.id%
wait 4 s
say It seems Rohxa has won his match against Bulvo!
  %at% 1980 say It seems Rohxa has won his match against Bulvo!
  %at% 1981 say It seems Rohxa has won his match against Bulvo!
  %at% 1982 say It seems Rohxa has won his match against Bulvo!
  %at% 1983 say It seems Rohxa has won his match against Bulvo!
  %at% 1984 say It seems Rohxa has won his match against Bulvo!
  %at% 1985 say It seems Rohxa has won his match against Bulvo!
  %at% 1986 say It seems Rohxa has won his match against Bulvo!
  %at% 1987 say It seems Rohxa has won his match against Bulvo!
  %at% 1988 say It seems Rohxa has won his match against Bulvo!
  %at% 1989 say It seems Rohxa has won his match against Bulvo!
  %at% 1990 say It seems Rohxa has won his match against Bulvo!
  %at% 1991 say It seems Rohxa has won his match against Bulvo!
  %at% 1992 say It seems Rohxa has won his match against Bulvo!
  %at% 1993 say It seems Rohxa has won his match against Bulvo!
  %at% 1994 say It seems Rohxa has won his match against Bulvo!
  %at% 1995 say It seems Rohxa has won his match against Bulvo!
  %at% 1996 say It seems Rohxa has won his match against Bulvo!
  %at% 1997 say It seems Rohxa has won his match against Bulvo!
  %at% 1998 say It seems Rohxa has won his match against Bulvo!
  %at% 1999 say It seems Rohxa has won his match against Bulvo!
wait 2 s
say Alright the next match is Rohxa vs %actor.name%
  %at% 1980 say Alright the next match is Rohxa vs %actor.name%
  %at% 1981 say Alright the next match is Rohxa vs %actor.name%
  %at% 1982 say Alright the next match is Rohxa vs %actor.name%
  %at% 1983 say Alright the next match is Rohxa vs %actor.name%
  %at% 1984 say Alright the next match is Rohxa vs %actor.name%
  %at% 1985 say Alright the next match is Rohxa vs %actor.name%
  %at% 1986 say Alright the next match is Rohxa vs %actor.name%
  %at% 1987 say Alright the next match is Rohxa vs %actor.name%
  %at% 1988 say Alright the next match is Rohxa vs %actor.name%
  %at% 1989 say Alright the next match is Rohxa vs %actor.name%
  %at% 1990 say Alright the next match is Rohxa vs %actor.name%
  %at% 1991 say Alright the next match is Rohxa vs %actor.name%
  %at% 1992 say Alright the next match is Rohxa vs %actor.name%
  %at% 1993 say Alright the next match is Rohxa vs %actor.name%
  %at% 1994 say Alright the next match is Rohxa vs %actor.name%
  %at% 1995 say Alright the next match is Rohxa vs %actor.name%
  %at% 1996 say Alright the next match is Rohxa vs %actor.name%
  %at% 1997 say Alright the next match is Rohxa vs %actor.name%
  %at% 1998 say Alright the next match is Rohxa vs %actor.name%
  %at% 1999 say Alright the next match is Rohxa vs %actor.name%
wait 1 s
%send% %actor% You should go back out to the ring.
elseif (%self.varexists(match4)% && %direction% == south)
wait 1
say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1980 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1981 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1982 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1983 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1984 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1985 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1986 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1987 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1988 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1989 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1990 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1991 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1992 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1993 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1994 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1995 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1996 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1997 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1998 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
  %at% 1999 say Congratulation to %actor.name% for %actor.hisher% victory over Rohxa!
rdelete matchwon %self.id%
wait 4 s
say It seems Vox easily won his match against Goom!
  %at% 1980 say It seems Vox easily won his match against Dorroha
  %at% 1981 say It seems Vox easily won his match against Dorroha
  %at% 1982 say It seems Vox easily won his match against Dorroha
  %at% 1983 say It seems Vox easily won his match against Dorroha
  %at% 1984 say It seems Vox easily won his match against Dorroha
  %at% 1985 say It seems Vox easily won his match against Dorroha
  %at% 1986 say It seems Vox easily won his match against Dorroha
  %at% 1987 say It seems Vox easily won his match against Dorroha
  %at% 1988 say It seems Vox easily won his match against Dorroha
  %at% 1989 say It seems Vox easily won his match against Dorroha
  %at% 1990 say It seems Vox easily won his match against Dorroha
  %at% 1991 say It seems Vox easily won his match against Dorroha
  %at% 1992 say It seems Vox easily won his match against Dorroha
  %at% 1993 say It seems Vox easily won his match against Dorroha
  %at% 1994 say It seems Vox easily won his match against Dorroha
  %at% 1995 say It seems Vox easily won his match against Dorroha
  %at% 1996 say It seems Vox easily won his match against Dorroha
  %at% 1997 say It seems Vox easily won his match against Dorroha
  %at% 1998 say It seems Vox easily won his match against Dorroha
  %at% 1999 say It seems Vox easily won his match against Dorroha
wait 2 s
say Alright the next match is Vox vs %actor.name%
  %at% 1980 say Alright the next match is Vox vs %actor.name%
  %at% 1981 say Alright the next match is Vox vs %actor.name%
  %at% 1982 say Alright the next match is Vox vs %actor.name%
  %at% 1983 say Alright the next match is Vox vs %actor.name%
  %at% 1984 say Alright the next match is Vox vs %actor.name%
  %at% 1985 say Alright the next match is Vox vs %actor.name%
  %at% 1986 say Alright the next match is Vox vs %actor.name%
  %at% 1987 say Alright the next match is Vox vs %actor.name%
  %at% 1988 say Alright the next match is Vox vs %actor.name%
  %at% 1989 say Alright the next match is Vox vs %actor.name%
  %at% 1990 say Alright the next match is Vox vs %actor.name%
  %at% 1991 say Alright the next match is Vox vs %actor.name%
  %at% 1992 say Alright the next match is Vox vs %actor.name%
  %at% 1993 say Alright the next match is Vox vs %actor.name%
  %at% 1994 say Alright the next match is Vox vs %actor.name%
  %at% 1995 say Alright the next match is Vox vs %actor.name%
  %at% 1996 say Alright the next match is Vox vs %actor.name%
  %at% 1997 say Alright the next match is Vox vs %actor.name%
  %at% 1998 say Alright the next match is Vox vs %actor.name%
  %at% 1999 say Alright the next match is Vox vs %actor.name%
wait 1 s
%send% %actor% You should go back out to the ring.
elseif (%self.varexists(victory)% && %direction% == south)
wait 1
say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1980 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1981 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1982 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1983 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1984 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1985 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1986 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1987 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1988 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1989 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1990 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1991 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1992 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1993 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1994 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1995 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1996 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1997 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1998 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  %at% 1999 say Congratulation to %actor.name% for %actor.hisher% victory over Vox and the entire tournament!
  rdelete matchwon %self.id%
  wait 3 s
  say For winning the whole tournament, here is what you have won!
  set %actor.gold(1000)%
  wait 3 s
  say Here is the one thousand zenni grand prize, the championship belt, and nano-diode!
  wait 1 s
  %send% %actor% %self.name% hands you a stack of zenni.
  wait 1 s
  %load% obj 1907
  %load% obj 1908
  give belt %actor.name%
  give nano %actor.name%
  wait 2 s
  %send% %actor.name% You are escorted out of the preparation room.
  %teleport% %actor% 1978
  rdelete registered %actor.id%
  set champion
  remote champion %actor.id%
  mgoto 1961
  %purge% seller
  %load% mob 1905
  mgoto 2002
  %load% mob 1906
  %purge% self
end
~
#1918
Match 1 Leave~
0 q 100
~
if (%actor.varexists(registered)% && %direction% == north)
return 0
wait 1
say To leave you must @Gforfeit@C.
else
if ((!%self.varexists(match2)% && !%self.varexists(match3)% && !%self.varexists(match4)%)&& %direction% == south)
return 0
wait 1
%echo% The announcer raises the microphone towards his mouth.
wait 2 s
%at% 1980 %echo% The speaker system crackles to life.
%at% 1980 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1981 %echo% The speaker system crackles to life.
%at% 1981 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1982 %echo% The speaker system crackles to life.
%at% 1982 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1983 %echo% The speaker system crackles to life.
%at% 1983 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1984 %echo% The speaker system crackles to life.
%at% 1984 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1985 %echo% The speaker system crackles to life.
%at% 1985 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1986 %echo% The speaker system crackles to life.
%at% 1986 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1987 %echo% The speaker system crackles to life.
%at% 1987 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1988 %echo% The speaker system crackles to life.
%at% 1988 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1989 %echo% The speaker system crackles to life.
%at% 1989 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1990 %echo% The speaker system crackles to life.
%at% 1990 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1991 %echo% The speaker system crackles to life.
%at% 1991 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1992 %echo% The speaker system crackles to life.
%at% 1992 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1993 %echo% The speaker system crackles to life.
%at% 1993 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1994 %echo% The speaker system crackles to life.
%at% 1994 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1995 %echo% The speaker system crackles to life.
%at% 1995 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1996 %echo% The speaker system crackles to life.
%at% 1996 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1997 %echo% The speaker system crackles to life.
%at% 1997 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1998 %echo% The speaker system crackles to life.
%at% 1998 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
%at% 1999 %echo% The speaker system crackles to life.
%at% 1999 say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious Boomer!
say Alright! Ladies and Gentlemen! %actor.name% is stepping into the ring to challenge the vicious wrestler Boomer!
wait 3 s
%teleport% %actor% 2004
%send% %actor% You step out into the bright light of the tournament ring.
%at% 1980 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1981 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1982 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1983 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1984 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1985 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1986 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1987 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1988 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1989 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1990 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1991 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1992 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1993 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1994 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1995 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1996 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1997 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1998 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
%at% 1999 %echo% The warrior called %actor.name% can be seen stepping out onto the ring floor.
wait 3 s
%send% %actor% You can see the giant wrestler Boomer standing at the opposite side of the ring.
%at% 1980 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1981 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1982 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1983 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1984 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1985 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1986 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1987 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1988 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1989 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1990 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1991 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1992 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1993 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1994 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1995 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1996 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1997 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1998 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 1999 %echo% The giant wrestler Boomer can be seen opposite of %actor.name% flexing his muscles!
%at% 2010 %load% mob 1907
wait 4 s
%send% %actor% The announcer yells over the sound system 'Let the fighting commence!'
%at% 1980 say Let the fighting commence!
%at% 1981 say Let the fighting commence!
%at% 1982 say Let the fighting commence!
%at% 1983 say Let the fighting commence!
%at% 1984 say Let the fighting commence!
%at% 1985 say Let the fighting commence!
%at% 1986 say Let the fighting commence!
%at% 1987 say Let the fighting commence!
%at% 1988 say Let the fighting commence!
%at% 1989 say Let the fighting commence!
%at% 1990 say Let the fighting commence!
%at% 1991 say Let the fighting commence!
%at% 1992 say Let the fighting commence!
%at% 1993 say Let the fighting commence!
%at% 1994 say Let the fighting commence!
%at% 1995 say Let the fighting commence!
%at% 1996 say Let the fighting commence!
%at% 1997 say Let the fighting commence!
%at% 1998 say Let the fighting commence!
%at% 1999 say Let the fighting commence!
set match1
remote match1 %self.id%
elseif (%self.varexists(match2)% && %direction% == south)
return 0
wait 1
say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1980 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1981 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1982 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1983 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1984 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1985 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1986 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1987 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1988 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1989 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1990 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1991 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1992 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1993 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1994 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1995 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1996 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1997 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1998 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %at% 1999 say %actor.name% prepares to face %actor.hisher% second opponent, the beautiful Valeria!
  %teleport% %actor% 2004
  %at% 2010 %load% mob 1908
wait 2 s
%send% %actor% You step out onto the ring and see Valeria standing at the opposite end, one hand on hip.
  %at% 1980 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1981 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1982 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1983 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1984 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1985 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1986 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1987 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1988 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1989 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1990 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1991 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1992 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1993 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1994 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1995 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1996 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1997 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1998 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
  %at% 1999 %echo% %actor.name% steps out onto the ring opposite of Valeria, who is standing with one hand on hip.
wait 1 s
%send% %actor% The announcer yells over the sound system 'Let the fighting commence!'
%at% 1980 say Let the fighting commence!
%at% 1981 say Let the fighting commence!
%at% 1982 say Let the fighting commence!
%at% 1983 say Let the fighting commence!
%at% 1984 say Let the fighting commence!
%at% 1985 say Let the fighting commence!
%at% 1986 say Let the fighting commence!
%at% 1987 say Let the fighting commence!
%at% 1988 say Let the fighting commence!
%at% 1989 say Let the fighting commence!
%at% 1990 say Let the fighting commence!
%at% 1991 say Let the fighting commence!
%at% 1992 say Let the fighting commence!
%at% 1993 say Let the fighting commence!
%at% 1994 say Let the fighting commence!
%at% 1995 say Let the fighting commence!
%at% 1996 say Let the fighting commence!
%at% 1997 say Let the fighting commence!
%at% 1998 say Let the fighting commence!
%at% 1999 say Let the fighting commence!
elseif (%self.varexists(match3)% && %direction% == south)
return 0
wait 1
say %actor.name% prepares to face the mysterious Roxha!
  %at% 1980 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1981 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1982 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1983 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1984 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1985 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1986 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1987 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1988 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1989 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1990 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1991 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1992 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1993 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1994 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1995 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1996 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1997 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1998 say %actor.name% prepares to face the mysterious Roxha!
  %at% 1999 say %actor.name% prepares to face the mysterious Roxha!
wait 2 s
  %teleport% %actor% 2004
  %at% 2010 %load% mob 1909
  %send% %actor% You step out onto the ring and see Roxha at the other end ignoring you.
  %at% 1980 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1981 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1982 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1983 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1984 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1985 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1986 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1987 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1988 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1989 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1990 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1991 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1992 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1993 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1994 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1995 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1996 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1997 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1998 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
  %at% 1999 %echo% %actor.name% steps out onto the ring floor with Roxha opposite %actor.himher%
wait 1 s
%send% %actor% The announcer yells over the sound system 'Let the fighting commence!'
%at% 1980 say Let the fighting commence!
%at% 1981 say Let the fighting commence!
%at% 1982 say Let the fighting commence!
%at% 1983 say Let the fighting commence!
%at% 1984 say Let the fighting commence!
%at% 1985 say Let the fighting commence!
%at% 1986 say Let the fighting commence!
%at% 1987 say Let the fighting commence!
%at% 1988 say Let the fighting commence!
%at% 1989 say Let the fighting commence!
%at% 1990 say Let the fighting commence!
%at% 1991 say Let the fighting commence!
%at% 1992 say Let the fighting commence!
%at% 1993 say Let the fighting commence!
%at% 1994 say Let the fighting commence!
%at% 1995 say Let the fighting commence!
%at% 1996 say Let the fighting commence!
%at% 1997 say Let the fighting commence!
%at% 1998 say Let the fighting commence!
%at% 1999 say Let the fighting commence!
elseif (%self.varexists(match4)% && %direction% == south)
return 0
wait 1
say %actor.name% prepares to face the fearsome Vox!
  %at% 1980 say %actor.name% prepares to face the fearsome Vox!
  %at% 1981 say %actor.name% prepares to face the fearsome Vox!
  %at% 1982 say %actor.name% prepares to face the fearsome Vox!
  %at% 1983 say %actor.name% prepares to face the fearsome Vox!
  %at% 1984 say %actor.name% prepares to face the fearsome Vox!
  %at% 1985 say %actor.name% prepares to face the fearsome Vox!
  %at% 1986 say %actor.name% prepares to face the fearsome Vox!
  %at% 1987 say %actor.name% prepares to face the fearsome Vox!
  %at% 1988 say %actor.name% prepares to face the fearsome Vox!
  %at% 1989 say %actor.name% prepares to face the fearsome Vox!
  %at% 1990 say %actor.name% prepares to face the fearsome Vox!
  %at% 1991 say %actor.name% prepares to face the fearsome Vox!
  %at% 1992 say %actor.name% prepares to face the fearsome Vox!
  %at% 1993 say %actor.name% prepares to face the fearsome Vox!
  %at% 1994 say %actor.name% prepares to face the fearsome Vox!
  %at% 1995 say %actor.name% prepares to face the fearsome Vox!
  %at% 1996 say %actor.name% prepares to face the fearsome Vox!
  %at% 1997 say %actor.name% prepares to face the fearsome Vox!
  %at% 1998 say %actor.name% prepares to face the fearsome Vox!
  %at% 1999 say %actor.name% prepares to face the fearsome Vox!
  wait 2 s
  %teleport% %actor% 2004
  %at% 2010 %load% mob 1910
  %send% %actor% You see Vox standing opposite of you with his arms crossed.
  %at% 1980 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1981 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1982 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1983 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1984 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1985 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1986 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1987 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1988 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1989 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1990 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1991 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1992 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1993 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1994 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1995 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1996 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1997 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1998 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
  %at% 1999 %echo% %actor.name% steps out onto the ring and notices the great Vox already out there!
wait 1 s
%send% %actor% The announcer yells over the sound system 'Let the fighting commence!'
%at% 1980 say Let the fighting commence!
%at% 1981 say Let the fighting commence!
%at% 1982 say Let the fighting commence!
%at% 1983 say Let the fighting commence!
%at% 1984 say Let the fighting commence!
%at% 1985 say Let the fighting commence!
%at% 1986 say Let the fighting commence!
%at% 1987 say Let the fighting commence!
%at% 1988 say Let the fighting commence!
%at% 1989 say Let the fighting commence!
%at% 1990 say Let the fighting commence!
%at% 1991 say Let the fighting commence!
%at% 1992 say Let the fighting commence!
%at% 1993 say Let the fighting commence!
%at% 1994 say Let the fighting commence!
%at% 1995 say Let the fighting commence!
%at% 1996 say Let the fighting commence!
%at% 1997 say Let the fighting commence!
%at% 1998 say Let the fighting commence!
%at% 1999 say Let the fighting commence!
else
return 0
wait 1
say You can't go out there, you are not permitted!
end
end
~
#1919
Boomer Death~
0 f 100
~
mgoto 2002
set ann %self.room.people%
  %at% 1980 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1981 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1982 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1983 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1984 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1985 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1986 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1987 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1988 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1989 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1990 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1991 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1992 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1993 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1994 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1995 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1996 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1997 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1998 %echo% %actor.name% stands triumphant over the defeated Boomer!
  %at% 1999 %echo% %actor.name% stands triumphant over the defeated Boomer!
while %ann%
  set tmp_ann %ann.next_in_room%
  if (%ann.vnum% == 1906)
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
  %teleport% %actor% 2004
  %force% %actor% north
end
~
#1920
Valeria Death~
0 f 100
~
mgoto 2002
set ann %self.room.people%
  %at% 1980 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1981 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1982 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1983 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1984 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1985 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1986 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1987 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1988 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1989 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1990 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1991 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1992 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1993 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1994 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1995 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1996 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1997 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1998 %echo% %actor.name% stands triumphant over the defeated Valeria!
  %at% 1999 %echo% %actor.name% stands triumphant over the defeated Valeria!
while %ann%
  set tmp_ann %ann.next_in_room%
  if (%ann.vnum% == 1906)
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
  %teleport% %actor% 2004
  %force% %actor% north
 end
~
#1921
Rohxa Death~
0 f 100
~
mgoto 2002
set ann %self.room.people%
  %at% 1980 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1981 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1982 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1983 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1984 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1985 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1986 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1987 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1988 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1989 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1990 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1991 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1992 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1993 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1994 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1995 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1996 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1997 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1998 %echo% %actor.name% stands triumphant over the defeated Rohxa!
  %at% 1999 %echo% %actor.name% stands triumphant over the defeated Rohxa!
while %ann%
  set tmp_ann %ann.next_in_room%
  if (%ann.vnum% == 1906)
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
  %teleport% %actor% 2004
  %force% %actor% north
 end
~
#1922
Vox Death~
0 f 100
~
mgoto 2002
set ann %self.room.people%
  %at% 1980 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1981 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1982 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1983 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1984 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1985 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1986 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1987 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1988 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1989 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1990 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1991 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1992 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1993 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1994 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1995 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1996 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1997 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1998 %echo% %actor.name% stands triumphant over the defeated Vox!
  %at% 1999 %echo% %actor.name% stands triumphant over the defeated Vox!
while %ann%
  set tmp_ann %ann.next_in_room%
  if (%ann.vnum% == 1906)
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
  %teleport% %actor% 2004
  %force% %actor% north
 end
~
#1923
Valeria Fight Trigger~
0 k 20
~
eval tactics %random.6%
wait 1
switch %tactics%
 case 1
  %at% 1980 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1981 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1982 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1983 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1984 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1985 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1986 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1987 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1988 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1989 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1990 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1991 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1992 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1993 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1994 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1995 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1996 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1997 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1998 %echo% %actor.name% manages to avoid an attack from Valeria!
  %at% 1999 %echo% %actor.name% manages to avoid an attack from Valeria!
 break
 case 2
  %at% 1980 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1981 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1982 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1983 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1984 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1985 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1986 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1987 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1988 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1989 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1990 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1991 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1992 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1993 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1994 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1995 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1996 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1997 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1998 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
  %at% 1999 %echo% %actor.name% manages to shock Valeria with %actor.hisher% Fiercness!
 break
 case 3
  %at% 1980 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1981 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1982 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1983 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1984 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1985 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1986 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1987 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1988 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1989 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1990 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1991 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1992 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1993 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1994 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1995 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1996 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1997 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1998 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
  %at% 1999 %echo% Valeria spits some blood onto the ring after an attack by %actor.name%
 break
 case 4
  %at% 1980 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1981 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1982 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1983 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1984 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1985 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1986 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1987 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1988 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1989 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1990 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1991 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1992 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1993 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1994 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1995 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1996 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1997 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1998 %echo% Valeria leaps out of the way of %actor.name%'s attack!
  %at% 1999 %echo% Valeria leaps out of the way of %actor.name%'s attack!
 break
 case 5
  %at% 1980 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1981 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1982 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1983 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1984 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1985 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1986 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1987 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1988 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1989 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1990 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1991 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1992 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1993 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1994 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1995 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1996 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1997 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1998 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
  %at% 1999 %echo% Valeria stops to taunt %actor.name% by waving a finger at %actor.himher%.
 break
 case 6
  %at% 1980 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1981 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1982 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1983 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1984 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1985 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1986 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1987 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1988 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1989 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1990 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1991 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1992 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1993 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1994 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1995 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1996 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1997 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1998 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
  %at% 1999 %echo% Valeria leaps high into the air, landing behind %actor.name% after doing a flip!
 break
done
~
#1924
Roxha Fight Trigger~
0 k 20
~
eval tactics %random.6%
wait 1
switch %tactics%
 case 1
  %at% 1980 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1981 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1982 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1983 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1984 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1985 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1986 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1987 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1988 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1989 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1990 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1991 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1992 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1993 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1994 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1995 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1996 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1997 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1998 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
  %at% 1999 %echo% Roxha easily dodges an attack by %actor.name%, while cackling.
 break
 case 2
  %at% 1980 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1981 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1982 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1983 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1984 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1985 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1986 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1987 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1988 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1989 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1990 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1991 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1992 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1993 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1994 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1995 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1996 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1997 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1998 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
  %at% 1999 %echo% %actor.name% barely manages to avoid a devestating attack by Roxha!
 break
 case 3
  %at% 1980 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1981 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1982 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1983 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1984 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1985 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1986 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1987 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1988 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1989 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1990 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1991 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1992 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1993 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1994 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1995 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1996 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1997 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1998 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
  %at% 1999 %echo% %actor.name% looks confused as Roxha begins to move his hands in a strange way!
 break
 case 4
  %at% 1980 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1981 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1982 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1983 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1984 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1985 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1986 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1987 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1988 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1989 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1990 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1991 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1992 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1993 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1994 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1995 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1996 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1997 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1998 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
  %at% 1999 %echo% Roxha throws a small ball of energy at %actor.name% but the ball disappears after a few feet!
 break
 case 5
  %at% 1980 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1981 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1982 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1983 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1984 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1985 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1986 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1987 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1988 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1989 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1990 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1991 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1992 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1993 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1994 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1995 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1996 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1997 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1998 %echo% Roxha leaps backwards and almost falls out of the ring!
  %at% 1999 %echo% Roxha leaps backwards and almost falls out of the ring!
 break
 case 6
  %at% 1980 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1981 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1982 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1983 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1984 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1985 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1986 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1987 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1988 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1989 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1990 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1991 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1992 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1993 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1994 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1995 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1996 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1997 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1998 %echo% %actor.name% flips out of the way of Roxha's latest attack!
  %at% 1999 %echo% %actor.name% flips out of the way of Roxha's latest attack!
 break
done
~
#1925
Vox Fight Trigger~
0 k 20
~
eval tactics %random.6%
wait 1
switch %tactics%
 case 1
  %at% 1980 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1981 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1982 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1983 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1984 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1985 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1986 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1987 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1988 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1989 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1990 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1991 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1992 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1993 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1994 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1995 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1996 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1997 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1998 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
  %at% 1999 %echo% Vox leaps into the air and comes down hard, shattering several stone tiles on the ring!
 break
 case 2
  %at% 1980 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1981 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1982 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1983 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1984 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1985 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1986 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1987 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1988 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1989 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1990 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1991 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1992 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1993 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1994 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1995 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1996 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1997 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1998 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
  %at% 1999 %echo% Vox lifts a stone tile from the ring floor and throws it at %actor.name%, but misses!
 break
 case 3
  %at% 1980 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1981 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1982 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1983 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1984 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1985 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1986 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1987 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1988 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1989 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1990 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1991 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1992 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1993 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1994 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1995 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1996 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1997 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1998 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
  %at% 1999 %echo% Vox points at %actor.name% twice rapidly before giving a thumbs down sign.
 break
 case 4
  %at% 1980 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1981 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1982 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1983 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1984 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1985 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1986 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1987 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1988 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1989 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1990 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1991 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1992 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1993 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1994 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1995 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1996 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1997 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1998 %echo% %actor.name% manages to avoid a series of attack from Vox!
  %at% 1999 %echo% %actor.name% manages to avoid a series of attack from Vox!
 break
 case 5
  %at% 1980 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1981 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1982 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1983 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1984 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1985 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1986 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1987 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1988 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1989 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1990 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1991 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1992 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1993 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1994 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1995 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1996 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1997 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1998 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
  %at% 1999 %echo% %actor.name% leaps up as Vox slams a fist into the ring floor creating a small crater!
 break
 case 6
  %at% 1980 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1981 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1982 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1983 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1984 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1985 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1986 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1987 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1988 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1989 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1990 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1991 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1992 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1993 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1994 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1995 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1996 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1997 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1998 %echo% Vox manages to block a series of attacks from %actor.name%!
  %at% 1999 %echo% Vox manages to block a series of attacks from %actor.name%!
 break
done
~
#1926
Clean Tournament~
0 b 80
~
if (%self.varexists(tournament)%)
 if (%people.2002% >= 2)
 eval num1 1
 elseif (%people.2002% <= 1)
 eval num1 0
 end
 eval num2 %people.2003%
 eval num3 %people.2004%
 eval num4 %people.2005%
 eval num5 %people.2006%
 eval num6 %people.2007%
 eval num7 %people.2008%
 eval num8 %people.2009%
 eval num9 %people.2010%
 eval numx %people.2011%
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
  %echo% The tournament has an opening again because one participant is missing.
  %at% 1978 %echo% The tournament has an opening again because one participant is missing.
  %at% 1979 %echo% The tournament has an opening again because one participant is missing.
  %at% 1980 %echo% The tournament has an opening again because one participant is missing.
  %at% 1981 %echo% The tournament has an opening again because one participant is missing.
  %at% 1982 %echo% The tournament has an opening again because one participant is missing.
  %at% 1983 %echo% The tournament has an opening again because one participant is missing.
  %at% 1984 %echo% The tournament has an opening again because one participant is missing.
  %at% 1985 %echo% The tournament has an opening again because one participant is missing.
  %at% 1986 %echo% The tournament has an opening again because one participant is missing.
  %at% 1987 %echo% The tournament has an opening again because one participant is missing.
  %at% 1988 %echo% The tournament has an opening again because one participant is missing.
  %at% 1989 %echo% The tournament has an opening again because one participant is missing.
  %at% 1990 %echo% The tournament has an opening again because one participant is missing.
  %at% 1991 %echo% The tournament has an opening again because one participant is missing.
  %at% 1992 %echo% The tournament has an opening again because one participant is missing.
  %at% 1993 %echo% The tournament has an opening again because one participant is missing.
  %at% 1994 %echo% The tournament has an opening again because one participant is missing.
  %at% 1995 %echo% The tournament has an opening again because one participant is missing.
  %at% 1996 %echo% The tournament has an opening again because one participant is missing.
  %at% 1997 %echo% The tournament has an opening again because one participant is missing.
  %at% 1998 %echo% The tournament has an opening again because one participant is missing.
  %at% 1999 %echo% The tournament has an opening again because one participant is missing.
  %at% 2000 %echo% The tournament has an opening again because one participant is missing.
  %at% 2001 %echo% The tournament has an opening again because one participant is missing.
  %at% 2003 %purge%
  %at% 2004 %purge%
  %at% 2005 %purge%
  %at% 2006 %purge%
  %at% 2007 %purge%
  %at% 2008 %purge%
  %at% 2009 %purge%
  %at% 2010 %purge%
  %at% 2011 %purge%
   mgoto 2002
   %purge% announcer
   %load% mob 1906
   mgoto 1961
   %load% mob 1905
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
#1927
Forfeit Trigger~
2 c 100
forfeit~
if (%actor.varexists(registered)%)
 rdelete registered %actor.id%
end
wait 1
%send% %actor% You forfeit and leave the ring.
%teleport% %actor% 1978
  %at% 1980 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1981 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1982 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1983 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1984 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1985 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1986 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1987 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1988 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1989 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1990 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1991 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1992 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1993 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1994 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1995 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1996 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1997 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1998 %echo% %actor.name% forfeits and leaves the ring in shame!
  %at% 1999 %echo% %actor.name% forfeits and leaves the ring in shame!
%at% 2002 %purge% announcer
%at% 2002 %load% mob 1906
%at% 1961 %purge% seller
%at% 1961 %load% mob 1905
%at% 2003 %purge%
%at% 2004 %purge%
%at% 2005 %purge%
%at% 2006 %purge%
%at% 2007 %purge%
%at% 2008 %purge%
%at% 2009 %purge%
%at% 2010 %purge%
%at% 2011 %purge%
~
#1930
Testing 1 2 3~
1 c 100
drop~
if %arg% /= hydration capsule
  wait 1 s
  %send% %actor% You throw the capsule to the ground and a puff of smoke envelops the area.
  %echoaround% %actor% throws a capsule to the ground a puff of smoke envelops the area! 
  set d 0
  while %d% < 11
    %load% obj 69
    %echo% A bottled water rolls out of the cooler.
    eval d %d% + 1
  done
  %echo% Several bottles of water now stand where the capsule landed!
else
  %send% %actor% Try dropping the hydration capsule.
end
~
$~
