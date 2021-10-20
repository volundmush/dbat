#8100
Greet potential House Indoril member~
0 g 100
~
if !%actor.varexists(konhousejoined)%
  wait 1 s
  say Would you like to join House Indoril?
  wait 2 s
  say To join you must answer a question correctly. If you possess the knowledge to answer, you'll be accepted.
  wait 4 s
  say It is important to note before I ask, if you answer correctly you will be inducted into House Indoril and unable to leave or join another House. Understood?
  wait 3 s
  emote ponders.
  wait 2 s
  say To join, this is the question you must answer. What is the name of the councillor who occupies House Indoril's Kal Tyor seat?
end
~
#8101
Induct into house indoril~
0 d 100
Marias~
if !%actor.varexists(konhousejoined)%
  wait 2 s
  say Congratulations, you are now a House Indoril member.
  set konhousejoined
  remote konhousejoined %actor.id%
  set houseindoril initiate
  remote houseindoril %actor.id%
wait 2 s
say You can obtain missions from Councillor Marias, by saying mission.
else
  wait 1 s
  say You've already joined a House.
end
~
#8102
Councillor Marias quests~
0 d 100
mission~
if %speech% == mission
  if (%actor.varexists(konhousejoined)% && %actor.varexists(houseindoril)%)
    if (%actor.level% > 13 && %actor.houseindoril% == oathman && !%actor.varexists(doingoathmanq)%)
      wait 2 s
      say Greetings Oathman %actor.name%, I was just about to send for you.
      wait 2 s
      say Another job has come up. House Indoril needs you to hunt down and kill a crocodile.
      wait 3 s
      say It is known as Old Toothe and is more vicious and large than any of it's kind.
      wait 3 s
      say Old Toothe was last seen in the Lake Gander region.
      set doingoathmanq
      remote doingoathmanq %actor.id%
    elseif (%actor.houseindoril% == retainer && !%actor.varexists(doingretainerq)%)
      wait 2 s
      say Ahh, hello retainer %actor.name%. I have a job for you.
      wait 3 s
      say We are finalising the details of an armor shipment from Cirali's Smithy.
      wait 3 s
      %load% obj 8201
      give unsigned %actor.name%
      say You need to take this to Cirali and have him sign it. Then bring it back to me.
      set doingretainerq
      remote doingretainerq %actor.id%
    elseif (%actor.level% > 3 && %actor.houseindoril% == initiate && !%actor.varexists(doinginitiateq)%)
      wait 2 s
      say Hmm, I don't really have any work for initiates however...
      wait 3 s
      emote ponders for a moment.
      wait 3 s
      say I'll tell you what, if you can defeat an Ordinator Trainee and bring me back a Trainee badge to prove it, I'll promote you.
      set doinginitiateq
      remote doinginitiateq %actor.id%
    else
      wait 1 s
      say Sorry, I have no missions for you currently.
    end
  end
end
~
#8103
Councillor Marias receive~
0 j 100
~
if %actor.varexists(houseindoril)%
  if (%actor.houseindoril% == oathman && %object.vnum% == 8203)
    wait 2 s
    say You've done House Indoril and Konack a great service.
    wait 2 s
    say With Old Toothe gone, that region should be safer to travel now.
    wait 2 s
    say It is with honour that I promote you to Lawman, %actor.name%.
    rdelete finishedoathmanq %actor.id%
    set houseindoril lawman
    remote houseindoril %actor.id%
    wait 2 s
    say As for you're reward..
    wait 2 s
    emote hands you a sack of zenni.
    nop %actor.gold(1000)%
    %actor.exp(12000)%
    wait 2 s
    say Now you are a high enough rank to get missions from Highborne Sappherion. Good luck.
    %purge% %object%
  elseif (%actor.houseindoril% == retainer && %object.vnum% == 8202)
    wait 2 s
    emote examines the contract thoroughly.
    wait 3 s
    say Good, this should see the Ordinators continue to have the best equipment available.
    wait 2 s
    say I'm promoting you to Oathman, %actor.name%. And also here's a little reward for your work.
    set houseindoril oathman
    remote houseindoril %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    rdelete finishedretainerq %actor.id%
    nop %actor.gold(750)%
    %actor.exp(1500)%
    %purge% %object%
  elseif (%actor.houseindoril% == initiate && %object.vnum% == 8200)
    wait 2 s
    say Ok, well done. I'll promote you.
    wait 2 s
    rdelete finishedinitiateq %actor.id%
    set houseindoril retainer
    remote houseindoril %actor.id%
    say Congratulations, you are now a Retainer. Now for your reward..
    wait 2 s
    nop %actor.gold(500)%
    %actor.exp(1000)%
    emote hands you a small sack of zenni.
    %purge% %object%
  else
    say I don't want that.
    drop %object%
  end
end
~
#8104
Ordinator trainee drop badge~
0 f 100
~
if (%actor.varexists(houseindoril)% && %actor.varexists(doinginitiateq)%)
  %load% obj 8200
  rdelete doinginitiateq %actor.id%
  set finishedinitiateq
  remote finishedinitiateq %actor.id%
end
~
#8105
Cirali contract receive~
0 j 100
~
if (%actor.varexists(doingretainerq)% && %object.vnum% == 8201)
  wait 1 s
  say A contract from Councillor Marias it seems?
  wait 2 s
  say One moment, I'll just sign this for you so you can be on your way.
  wait 2 s
  emote grabs a quilled pen and signs the piece of paper.
  wait 2 s
  %purge% %object%
  %load% obj 8202
  rdelete doingretainerq %actor.id%
  set finishedretainerq
  remote finishedretainerq %actor.id%
give contract %actor.name%
  wait 2 s
  say There you go, take that back to Marias immediately.
else
  say I don't have a use for that.
  drop %object%
end
~
#8106
Old toothe drops tooth~
0 f 100
~
if (%actor.varexists(houseindoril)% && %actor.varexists(doingoathmanq)%)
  %load% obj 8203
  rdelete doingoathmanq %actor.id%
  set finishedoathmanq
  remote finishedoathmanq %actor.id%
end
~
#8107
Highborne Sappherion missions~
0 d 100
mission~
if %speech% == mission
  if (%actor.varexists(konhousejoined)% && %actor.varexists(houseindoril)%)
    if (%actor.level% > 45 && %actor.houseindoril% == grandordinator && !%actor.varexists(doinggrandordinatorq)%)
      wait 2 s
      say Greetings Grand Ordinator, it is good that you have come.
      wait 2 s
      say It has taken a while to get this pushed through the proper channels but my execution writ on High Priest Semaj has come through. 
      wait 3 s
      say You have proven yourself greatly to House Indoril and I offer this task to you.
      wait 3 s
      say Take down High Priest Semaj so this feud will end!
      set doinggrandordinatorq
      remote doinggrandordinatorq %actor.id%
    elseif (%actor.level% > 28 && %actor.houseindoril% == ordinator && !%actor.varexists(doingordinatorq)%)
      wait 2 s
      say Greetings Ordinator, I have an urgent mission for you.
      wait 3 s
      say It seems that the High Priest won't back down so easily.
      wait 3 s
      say Sources tell me he has hired an assassin to try and kill Councillor Marias as a means to get to me.
      wait 3 s
      say I have a contact that can provide what time this assassin will be arriving at the starport.
      wait 3 s
      say The bartender at the inn in City Dres is the contact you need to meet. Go now before it is too late.
      set doingordinatorq
      remote doingordinatorq %actor.id%
    elseif (%actor.houseindoril% == lawman && !%actor.varexists(doinglawmanq)%)
      wait 2 s
      say Greetings Lawman %actor.name%. I find myself in a bit of a pickle.. I could use your assitance.
      wait 3 s
      say High Priest Semaj seems determined to have me removed from office before my term...
      wait 3 s
      say He seems to have it in him that the Highborne should not serve more than 2 terms. So I need you to acquire some leverage for me that will force him to back down.
      wait 3 s
      say There is a clerk in the basement of the Oroist Temple, he has secured a document for me. You need to go get it off him, though he will expect 500 zenni for it.
      wait 4 s
      say Don't worry though, you're reward will cover the costs.
      set doinglawmanq
      remote doinglawmanq %actor.id%
    else
      wait 1 s
      say Sorry, I have no missions for you currently.
    end
  end
end
~
#8108
Highborne Sappherion Receive~
0 j 100
~
if %actor.varexists(houseindoril)%
  if (%actor.houseindoril% == grandordinator && %object.vnum% == 8206)
    wait 2 s
    say You've done House Indoril and Konack a great service.
    wait 3 s
    say With High Priest Semaj no longer able to meddle in my affairs, I am free to continue as normal.
    wait 4 s
    say To show my appreciation, I promote you to the esteemed rank of Councillor, %actor.name%.
    rdelete finishedgrandordinatorq %actor.id%
    set houseindoril councillor
    remote houseindoril %actor.id%
    wait 2 s
    say As for you're reward..
    wait 2 s
    emote hands you a sack of zenni.
    nop %actor.gold(3000)%
    %actor.exp(500000)%
    wait 2 s
    say Some final gifts for becoming a councillor.
    %load% obj 8207
    %load% obj 8208
    give entry %actor.name%
    give helm %actor.name%
    %purge% %object%
  elseif (%actor.houseindoril% == ordinator && %object.vnum% == 8205)
    wait 2 s
    say It is good you were able to get to him before it was too late.
    wait 3 s
    say And this writ should provide me with enough evidence to get an execution writ on High Priest Semaj.
    wait 2 s
    say I'm promoting you to Grand Ordinator, %actor.name%. And also here's a little reward for your work.
    set houseindoril grandordinator
    remote houseindoril %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    rdelete finishedordinatorq %actor.id%
    nop %actor.gold(2500)%
    %actor.exp(100000)%
    %purge% %object%
  elseif (%actor.houseindoril% == lawman && %object.vnum% == 8204)
    wait 2 s
    say Hmmm, most excellent, this should enable me a swift political victory over Semaj.
    wait 2 s
    rdelete finishedlawmanq %actor.id%
    set houseindoril ordinator
    remote houseindoril %actor.id%
    say Congratulations, you are now an Ordinator. Now for your reward..
    wait 2 s
    nop %actor.gold(2000)%
    %actor.exp(3000)%
    emote hands you a small sack of zenni.
    %purge% %object%
  else
    say I don't want that.
    drop %object%
  end
end
~
#8109
Oroist Clerk for Indoril mission greet~
0 g 100
~
wait 1 s
if %actor.varexists(doinglawmanq)%
  say Pay me the 500 zenni and the information is yours.
end
~
#8110
Oroist Clerk for Indoril mission prt 2~
0 m 100
500~
if (%actor.varexists(houseindoril)% && %actor.varexists(doinglawmanq)%)
  wait 2 s
  say Ok, the zenni is good. Now I'll hold up my end of the bargain.
  %load% obj 8204
  wait 2 s
  emote grabs a document from his desk.
  wait 1 s
  give leverage %actor.name%
  rdelete doinglawmanq %actor.id%
  set finishedlawmanq
  remote finishedlawmanq %actor.id%
  wait 1 s
  say There you go, I hope never to have to deal with this again..
end
~
#8111
Dres Barkeep informs quest~
0 g 100
~
if (%actor.varexists(houseindoril)% && %actor.varexists(doingordinatorq)%)
  if %findmob.8190(8124) < 1
    wait 3 s
    say Hmm, you are the Ordinator Highborne Sappherion said I was to meet here..
    wait 2 s
    say There is no time to waste, the Assassin should be passing through the clearing near the spaceport at any moment now!
    %at% 8190 %load% mob 8124
  else
    wait 2 s
    say Hurry, the Assassin is nearly at Tiranoc, Ordinator!
  end
end
~
#8112
Assassin evidence quest~
0 f 100
~
if (%actor.varexists(konhousejoined)% && %actor.varexists(doingordinatorq)%
  %load% obj 8205
  rdelete doingordinatorq %actor.id%
  set finishedordinatorq
  remote finishedordinatorq %actor.id%
end
~
#8113
Kill Semaj quest~
0 f 100
~
if (%actor.varexists(konhousejoined)% && %actor.varexists(doinggrandordinatorq)%
  %load% obj 8206
  rdelete doinggrandordinatorq %actor.id%
  set finishedgrandordinatorq
  remote finishedgrandordinatorq %actor.id%
end
~
#8114
Greet potential house oroist member~
0 g 100
~
if !%actor.varexists(konhousejoined)%
  wait 1 s
  say Would you like to join House Oroist?
  wait 2 s
  say To join you must answer a question correctly. If you possess the knowledge to answer, you'll be accepted.
  wait 4 s
  say It is important to note before I ask, if you answer correctly you will be inducted into House Oroist and unable to leave or join another House. Understood?
  wait 3 s
  emote ponders.
  wait 2 s
  say To join, this is the question you must answer. Who is the leader of House Oroist?
end
~
#8115
Induct into House Oroist~
0 d 100
Semaj~
if !%actor.varexists(konhousejoined)%
  wait 2 s
  say Congratulations, you are now a House Oroist member.
  set konhousejoined
  remote konhousejoined %actor.id%
  set houseoroist initiate
  remote houseoroist %actor.id%
wait 2 s
say You can obtain missions from me by saying mission.
else
  wait 1 s
  say You've already joined a House.
end
~
#8116
Councillor Orlu quests~
0 d 100
mission~
if %speech% == mission
  if (%actor.varexists(konhousejoined)% && %actor.varexists(houseoroist)%)
    if (%actor.houseoroist% == oathman && !%actor.varexists(doingoathmanq)%)
      wait 2 s
      say Greetings Oathman %actor.name%.
      wait 2 s
      say The final person I need you to get a pledge from is Lord Llaro Stern of House Dres.
      wait 3 s
      say He can be found at his mansion in City Dres.
      set doingoathmanq
      remote doingoathmanq %actor.id%
    elseif (%actor.houseoroist% == retainer && !%actor.varexists(doingretainerq)%)
      wait 2 s
      say The next person we need to get a pledge from is Mistress Shela Taesal of House Taesal.
      wait 3 s
      say She can usually be found at the Taesal Manor in town.
      set doingretainerq
      remote doingretainerq %actor.id%
    elseif (%actor.houseoroist% == initiate && !%actor.varexists(doinginitiateq)%)
      wait 2 s
      say House Oroist cannot continue to perform it's holy tasks if we do not get pledges from various nobles.
      wait 3 s
      say Your work for me will consist of getting pledges from specific nobles, so easy really.
      wait 3 s
      say First you need to get Councillor Marias of House Indoril to pledge a donation. She can usually be found in at the Indoril Compound.
      set doinginitiateq
      remote doinginitiateq %actor.id%
    else
      wait 1 s
      say Sorry, I have no missions for you currently.
    end
  end
end
~
#8117
Councillor Orlu receive~
0 j 100
~
if %actor.varexists(houseoroist)%
  if (%actor.houseoroist% == oathman && %object.vnum% == 8209)
    wait 2 s
    say That's the final pledge secured, excellent work.
    wait 2 s
    say It is with honour that I promote you to Lawman, %actor.name%.
    rdelete finishedoathmanq %actor.id%
    set houseoroist lawman
    remote houseoroist %actor.id%
    wait 2 s
say As for your reward...
    wait 2 s
    emote hands you a sack of zenni.
    nop %actor.gold(1000)%
    %actor.exp(2000)%
    wait 2 s
    say Now you are a high enough rank to get missions from High Priest Semaj. Good luck.
    %purge% %object%
  elseif (%actor.houseoroist% == retainer && %object.vnum% == 8209)
    wait 2 s
    say Excellent, this is the pledge from Shela Taesal and quite a large one too!
    wait 3 s
    say I'm promoting you to Oathman, %actor.name%. And also here's a little reward for your work.
    set houseoroist oathman
    remote houseoroist %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    rdelete finishedretainerq %actor.id%
    nop %actor.gold(750)%
    %actor.exp(1500)%
    %purge% %object%
  elseif (%actor.houseoroist% == initiate && %object.vnum% == 8209)
    wait 2 s
    say Good job getting this pledge from Councillor Marias.
    wait 3 s
    say It's always good to see the ruling House providing us with donations. 
    rdelete finishedinitiateq %actor.id%
    set houseoroist retainer
    remote houseoroist %actor.id%
    say Congratulations, you are now a Retainer. Now for your reward..
    wait 2 s
    nop %actor.gold(500)%
    %actor.exp(1000)%
    emote hands you a small sack of zenni.
    %purge% %object%
  else
    say I don't want that.
    drop %object%
  end
end
~
#8118
Councillor Marias pledge~
0 d 100
pledge~
if (%actor.varexists(houseoroist)% && %actor.varexists(doinginitiateq)%)
  wait 2 s
  say Alright, I'll donate to House Oroist.
  wait 2 s
  %load% obj 8209
  give pledge %actor.name%
  rdelete doinginitiateq %actor.id%
  set finishedinitiateq
  remote finishedinitiateq %actor.id%
end
~
#8119
Shela Taesal pledge~
0 d 100
pledge~
if (%actor.varexists(houseoroist)% && %actor.varexists(doingretainerq)%)
  wait 2 s
  say Alright, I'll donate to House Oroist.
  wait 2 s
  %load% obj 8209
  give Donation %actor.name%
  rdelete doingretainerq %actor.id%
  set finishedretainerq
  remote finishedretainerq %actor.id%
end
~
#8120
Llaro Stern pledge~
0 d 100
pledge~
if (%actor.varexists(houseoroist)% && %actor.varexists(doingoathmanq)%)
  wait 2 s
  say Alright, I'll donate to House Oroist.
  wait 2 s
  %load% obj 8209
  give Donation %actor.name%
  rdelete doingoathmanq %actor.id%
  set finishedoathmanq
  remote finishedoathmanq %actor.id%
end
~
#8121
High Priest Semaj Missions~
0 d 100
mission~
if %speech% == mission
  if (%actor.varexists(konhousejoined)% && %actor.varexists(houseoroist)%)
    if (%actor.level% > 48 && %actor.houseoroist% == priest && !%actor.varexists(doingpriestq)%)
      wait 2 s
      say Long ago House Oroist was put in charge of keeping an eye on House Sereg-Vanma.
      wait 3 s
      say The evidence provided by our spy indicates Lord Nikolash is close to following in his ancestor's footsteps.
      wait 3 s
      say Konack may endure a war the likes of which hasn't been seen for centuries if you don't put a stop to him.
      wait 3 s
      say Quick! Hurry over to the Sereg-Vanma manor now and stop him before it is too late.
      set doingpriestq
      remote doingpriestq %actor.id%
    elseif (%actor.houseoroist% == monk && !%actor.varexists(doingmonkq)%)
      wait 2 s
      say %actor.name%, tensions are rising and I fear a war might be coming.
      wait 3 s
      say For years I've been trying to get someone inside House Sereg-Vanma's higher ranks.
      wait 3 s
      say My spy on the inside has a report I need to get ahold of. You need to head to the Old Kerberos ruins and meet him.
      wait 3 s
      say The code word is araklon.
      set doingmonkq
      remote doingmonkq %actor.id%
    elseif (%actor.level% > 27 && %actor.houseoroist% == lawman && !%actor.varexists(doinglawmanq)%)
      wait 2 s
      say Ahh, there you are %actor.name%. I have something off the records that I need you to take care of for me.
      wait 3 s
      emote ponders for a moment.
      wait 3 s
      say My daughter used to be a monk. I was grooming her to become High Priestess and my successor.
      wait 3 s
      say Unfortunately she fled the temple and has since turned to the dark arts.
      wait 3 s
      say She currently is hiding out in the sewer somewhere beneath the temple, striking out at monks and priests by themselves.
      wait 3 s
      say She's my daughter but it has to stop.. please put an end to Asijiah.
      set doinglawmanq
      remote doinglawmanq %actor.id%
    else
      wait 1 s
      say Sorry, I have no missions for you currently.
    end
  end
end
~
#8122
High Priest Semaj receive~
0 j 100
~
if %actor.varexists(houseoroist)%
  if (%actor.houseoroist% == priest && %object.vnum% == 8213)
    wait 2 s
    say You've done House Oroist and Konack a great service.
    wait 3 s
    say Hopefully with the death of Lord Nikolash, House Sereg-Vanma think twice before attempting to return to the old days.
    wait 4 s
    say To show my appreciation, I promote you to the esteemed rank of Councillor, %actor.name%.
    rdelete finishedpriestq %actor.id%
    set houseoroist councillor
    remote houseoroist %actor.id%
    wait 2 s
    say As for you're reward..
    wait 2 s
    emote hands you a sack of zenni.
    nop %actor.gold(3000)%
    %actor.exp(500000)%
    wait 2 s
    say Some final gifts for becoming a councillor.
    %load% obj 8207
    %load% obj 8301
    give entry %actor.name%
    give orb %actor.name%
    %purge% %object%
  elseif (%actor.houseoroist% == monk && %object.vnum% == 8212)
    wait 2 s
    say Hmm, these documents are most disturbing.
    wait 3 s
    say It does seem that Lord Nikolash is planning to bring back the old days of his house.
    wait 2 s
    say For your good work, I'm promoting you to Priest, %actor.name%. And also here's a little reward for your work.
    set houseoroist priest
    remote houseoroist %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    rdelete finishedmonkq %actor.id%
    nop %actor.gold(2500)%
    %actor.exp(4000)%
    %purge% %object%
  elseif (%actor.houseoroist% == lawman && %object.vnum% == 8211)
    wait 2 s
    say It saddens me it had to happen like this, never the less I shall reward you.
    wait 2 s
    rdelete finishedlawmanq %actor.id%
    set houseoroist monk
    remote houseoroist %actor.id%
    say Congratulations, you are now a Monk. Now for your reward..
    wait 2 s
    nop %actor.gold(2000)%
    %actor.exp(100000)%
    emote hands you a small sack of zenni.
    %purge% %object%
  else
    say I don't want that.
    drop %object%
  end
end
~
#8123
Asijiah death load necklace~
0 f 100
~
if (%actor.varexists(houseoroist)% && %actor.varexists(doinglawmanq)%)
  %load% obj 8211
  rdelete doinglawmanq %actor.id%
  set finishedlawmanq
  remote finishedlawmanq %actor.id%
end
~
#8124
Oroist spy gives documents~
0 d 100
araklon~
if (%actor.varexists(houseoroist)% && %actor.varexists(doingmonkq)%)
  wait 2 s
  say You're the monk Semaj was sending? Alright then.. here is the info.
  %load% obj 8212
  give documents %actor.name%
  rdelete doingmonkq %actor.id%
  set finishedmonkq
  remote finishedmonkq %actor.id%
  wait 2 s
  say Get it these to him fast!
end
~
#8125
Lord Nikolash drops signet ring~
0 f 100
~
if (%actor.varexists(houseoroist)% && %actor.varexists(doingpriestq)%)
%load% obj 8213
rdelete doingpriestq %actor.id%
set finishedpriestq
remote finishedpriestq %actor.id%
end
~
#8126
Mishie Shuro missions~
0 d 100
mission~
if %speech% == mission
  if (%actor.varexists(konhousejoined)% && %actor.varexists(housedres)%)
    if (%actor.housedres% == oathman && !%actor.varexists(doingoathmanq)%)
      wait 2 s
      say The Mazori family have fallen on hard times and they blame the Colvians.
      wait 3 s
      say They've been pushing very hard for an investigation into Matsu Colvian.
      wait 3 s
      say I've promised them that someone will investigate and that someone is you.
      wait 3 s
      say Head over to the Mazori farm in Dres District and speak to Krathius Mazori.
      set doingoathmanq
      remote doingoathmanq %actor.id%
    elseif (%actor.level% > 7 && %actor.housedres% == retainer && !%actor.varexists(doingretainerq)%)
      wait 2 s
      say Retainers are expected to perform a task for one of the trades guilds that comprise House Dres.
      wait 3 s
      say Head to the Jormun Swamp and hunt and kill a coypu. Then bring it's pelt back to me.
      set doingretainerq
      remote doingretainerq %actor.id%
    elseif (%actor.housedres% == initiate && !%actor.varexists(doinginitiateq)%)
      wait 2 s
      say The head of the Dye and Skinning Guild feels is in Tiranoc to negotiate a deal with myself.
      wait 3 s
      say Other matters see to it that I will not make the meeting, so %actor.name%, your task is to go meet him.
      wait 3 s
      say He should be outside Lord Aranni's Construction company office. Just say negotiate and he'll know what to do.
      set doinginitiateq
      remote doinginitiateq %actor.id%
    else
      wait 1 s
      say Sorry, I have no missions for you currently.
    end
  end
end
~
#8127
Greet Potential House Dres member~
0 g 100
~
if !%actor.varexists(konhousejoined)%
  wait 1 s
  say Would you like to join House Dres?
  wait 2 s
  say To join you must answer a question correctly. If you possess the knowledge to answer, you'll be accepted.
  wait 4 s
  say It is important to note before I ask, if you answer correctly you will be inducted into House Dres and unable to leave or join another House. Understood?
  wait 3 s
  emote ponders.
  wait 2 s
  say To join, this is the question you must answer. Who is the our most important historical figure in House Dres?
end
~
#8128
Induct House Dres Member~
0 d 100
Taloc~
if !%actor.varexists(konhousejoined)%
  wait 2 s
  say Congratulations, you are now a House Dres member.
  set konhousejoined
  remote konhousejoined %actor.id%
  set housedres initiate
  remote housedres %actor.id%
  wait 2 s
  say You can obtain missions from me, by saying mission.
else
  wait 1 s
  say You've already joined a House.
end
~
#8129
Mishie Shuro recieve~
0 j 100
~
if %actor.varexists(housedres)%
  if (%actor.housedres% == oathman && %object.vnum% == 8216)
    wait 2 s
    say Well, this sigil is indeed that of the Colvian family.
    wait 3 s
    say And rather odd that it should be found at the bottom of the Mazori's well.
    wait 3 s
    say However it is hardly concrete evidence for the claims.
    wait 3 s
    say Regardless, here's your reward, %actor.name%
    rdelete finishedoathmanq %actor.id%
    set housedres lawman
    remote housedres %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    nop %actor.gold(1000)%
    %actor.exp(2000)%
    wait 2 s
    say You should go see Lord Llaro Stern if you want more missions.
    %purge% %object%
  elseif (%actor.housedres% == retainer && %object.vnum% == 8215)
    wait 3 s
    say Well, this pelt isn't in the best of shape.. but you completed the task.
    wait 2 s
    say I'm promoting you to Oathman, %actor.name%. And also here's a little reward for your work.
    set housedres oathman
    remote housedres %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    rdelete finishedretainerq %actor.id%
    nop %actor.gold(750)%
    %actor.exp(3499)%
    %purge% %object%
  elseif (%actor.housedres% == initiate && %object.vnum% == 8202)
    wait 2 s
    say Hmm, he signed the contract. Excellent, with the Dye & Skinning Guild back on board, House Dres will strengthen in the coming months.
    wait 2 s
    say I hearby promote you to the rank of Retainer and I will give you a little reward for a job well done..
    rdelete finishedinitiateq %actor.id%
    set housedres retainer
    remote housedres %actor.id%
    wait 2 s
    nop %actor.gold(500)%
    %actor.exp(1000)%
    emote hands you a small sack of zenni.
    %purge% %object%
  else
    say I don't want that.
    drop %object%
  end
end
~
#8130
Trades Guild mission~
0 d 100
negotiate~
if (%actor.varexists(housedres)% && %actor.varexists(doinginitiateq)%)
  wait 2 s
  say Hmm, Mishie Shuro must be busy if she sent an initiate to meet me.
  wait 3 s
  say Oh well, this meeting would have been a mere formality anyway.
  wait 3 s
  say I've decided I will sign the contract for this particular deal.
  wait 1 s
  %load% obj 8202
  give contract %actor.name%
  rdelete doinginitiateq %actor.id%
  set finishedinitiateq
  remote finishedinitiateq %actor.id%
  wait 2 s
  say Take that to Mishie Shuro straight away!
end
~
#8131
Coypu pelt mission~
0 f 100
~
if (%actor.varexists(housedres)% && %actor.varexists(doingretainerq)%)
  %load% obj 8215
  rdelete doingretainerq %actor.id%
  set finishedretainerq
  remote finishedretainerq %actor.id%
end
~
#8132
Krathius Mazori greet~
0 g 100
~
if (%actor.varexists(housedres)% && %actor.varexists(doingoathmanq)%)
  wait 2 s
  say A representative from House Dres, Finally!!
  wait 3 s
  say This needs to be done as fast as possible, we've suffered long enough.
  wait 3 s
  say Our well dried up strangely and this was the beginning of our problems.
  wait 3 s
  say You should probably head to the bottom of the well and inspect it.
end
~
#8133
Inspect well mission~
2 c 100
inspect~
if (%actor.varexists(housedres)% && %actor.varexists(doingoathmanq)%)
  if "%arg%" != "well"
    %send% %actor% Try inspecting the well.
  else
    wait 2 s
    %send% %actor% You inspect the well and see that a Colvian Sigil us laying on the ground.
    wait 2 s
    %send% %actor% You should pick it up and take it back to Mishie Shuro
    %load% obj 8216
    rdelete doingoathmanq %actor.id%
    set finishedoathmanq
    remote finishedoathmanq %actor.id%
  end
end
~
#8134
Llaro Stern Dres missions~
0 d 100
mission~
if %speech% == mission
  if (%actor.varexists(konhousejoined)% && %actor.varexists(housedres)%)
    if (%actor.level% > 40 && %actor.housedres% == kinsman && !%actor.varexists(doingkinsmanq)%)
      wait 2 s
      say Everything the Mazori Family accussed Matsu Colvian seems to be true.
      wait 3 s
      say And somehow he seems linked to the were-konatsu curse.
      wait 3 s
      say It can't be pure coincidence that Matsu Colvian stood to gain alot from both events.
      wait 3 s
      say Head over to his farm and bring him to justice!
      set doingkinsmanq
      remote doingkinsmanq %actor.id%
    elseif (%actor.level% > 38 && %actor.housedres% == yeoman && !%actor.varexists(doingyeomanq)%)
      wait 2 s
      say Saint Alucia was once a lovely little village but it has been inflicted with a terrible curse..
      wait 3 s
      say It's inhabitants were turned into dreaded were-konatsus. Unfortunately we don't know where they come from, so far all we have been able to do is make sure they don't spread.
      wait 4 s
      say I want you to go to Saint Alucia and purge as many were-konatsus as you can. If you happen upon anything other than their fur, bring that back to me.. it may just hold a clue to this curse.
      set doingyeomanq
      remote doingyeomanq %actor.id%
    elseif (%actor.housedres% == lawman && !%actor.varexists(doinglawmanq)%)
      wait 2 s
      say Mishie Shuro has informed me of the progress of the investigation into Matsu Colvian.
      wait 3 s
      say The Colvian family are quite powerful, so we have to move carefully.
      wait 3 s
      say The next step is to go to Matsu Colvian and request a sigil.
      wait 2 s
      say If he hands one over, bring it back to me to inspect.
      set doinglawmanq
      remote doinglawmanq %actor.id%
    else
      wait 1 s
      say Sorry, I have no missions for you currently.
    end
  end
end
~
#8135
Llaro Stern Dres receive~
0 j 100
~
if %actor.varexists(housedres)%
  if (%actor.housedres% == kinsman && %object.vnum% == 8218)
    wait 2 s
    emote skims through Matsu Colvian's journal.
    wait 3 s
    say It seems we were right about him...
    wait 3 s
    say You have done the Dres District a great deed here and you have my thanks.
    wait 3 s
    say Here's your reward, %actor.name%
    rdelete finishedkinsmanq %actor.id%
    set housedres councillor
    remote housedres %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    nop %actor.gold(3000)%
    %actor.exp(360000)%
    wait 2 s
    say And for becoming a councillor in House Dres, I further present you with these gifts.
    %load% obj 8207
    %load% obj 8217
    give entry %actor.name%
    give scythe %actor.name%
    %purge% %object%
  elseif (%actor.housedres% == yeoman && %object.vnum% == 8216)
    wait 3 s
    say Well, well, well. The Colvian Family sigil just seems to keep popping up in the oddest of places doesn't it?
    wait 3 s
    say I'm not sure what to make of this... I'm going to require some thinking time to evaluate the situation now.
    wait 3 s
    say However, for now, I'm promoting you to Kinsman, %actor.name%. And also here's a little reward for your work.
    set housedres kinsman
    remote housedres %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
rdelete finishedyeomanq %actor.id%
    nop %actor.gold(2500)%
    %actor.exp(340000)%
    %purge% %object%
  elseif (%actor.housedres% == lawman && %object.vnum% == 8219)
    wait 2 s
    say Well, it's easy to see that this is the same sigil as the one previously found.
    wait 2 s
    say Still, the evidence is hardly concrete by any measure of the imagination.
    wait 3 s
    say For performing the task, I promote you to Yeoman.
    rdelete finishedlawmanq %actor.id%
    set housedres yeoman
    remote housedres %actor.id%
    wait 2 s
    nop %actor.gold(2000)%
    %actor.exp(3000)%
    emote hands you a small sack of zenni.
    %purge% %object%
  else
    say I don't want that.
    drop %object%
  end
end
~
#8136
Matsu Colvian request~
0 d 100
request~
if (%actor.varexists(housedres)% && %actor.varexists(doinglawmanq)%
  wait 2 s
  say Lord Llaro Stern wants a copy of my family's Sigil?
  wait 3 s
  say Well, that's a strange request but I'll honor it.
  wait 2 s
  %load% obj 8219
  give sigil %actor.name%
  rdelete doinglawmanq %actor.id%
  set finishedlawmanq
  remote finishedlawmanq %actor.id%
end
~
#8137
Were-konatsu drop sigil quest~
0 f 100
~
if (%actor.varexists(housedres)% && %actor.varexists(doingyeomanq)%
  %load% obj 8216
  rdelete doingyeomanq %actor.id%
  set finishedyeomanq
  remote finishedyeomanq %actor.id%
end
~
#8138
Matsu Colvian dies mission~
0 f 100
~
if (%actor.varexists(housedres)% && %actor.varexists(doingkinsmanq)%
  %load% obj 8218
  rdelete doingkinsmanq %actor.id%
  set finishedkinsmanq
  remote finishedkinsmanq %actor.id%
end
~
#8139
Greet potential house taesal member~
0 g 100
~
if !%actor.varexists(konhousejoined)%
  wait 1 s
  say Would you like to join House Taesal?
  wait 2 s
  say To join you must answer a question correctly. If you possess the knowledge to answer, you'll be accepted.
  wait 4 s
  say It is important to note before I ask, if you answer correctly you will be inducted into House Taesal and unable to leave or join another House. Understood?
  wait 3 s
  emote ponders.
  wait 2 s
  say To join, this is the question you must answer. Who is the our most important historical figure in House Taesal?
end
~
#8140
Induct House Taesal Member~
0 d 100
Meridius~
if !%actor.varexists(konhousejoined)%
  wait 2 s
  say Congratulations, you are now a House Taesal member.
  set konhousejoined
  remote konhousejoined %actor.id%
  set housetaesal initiate
  remote housetaesal %actor.id%
  wait 2 s
  say You can obtain missions from me, by saying mission.
else
  wait 1 s
  say You've already joined a House.
end
~
#8141
Lord Aranni Taesal Missions~
0 d 100
mission~
if %speech% == mission
  if (%actor.varexists(konhousejoined)% && %actor.varexists(housetaesal)%)
    if (%actor.housetaesal% == oathman && !%actor.varexists(doingoathmanq)%)
      wait 2 s
      say Ahh, greetings Oathman %actor.name%. Looks like I have one task left for you.
      wait 3 s
      say Complete this and you'll be moving on up to bigger and better things in House Taesal.
      wait 3 s
      say And that usually means more gold for you.
      wait 2 s
      wink %actor.name%
      wait 3 s
      say Anyway, there's a Smith is City Dres that's willing to throw his support to House Taesal.
      wait 3 s
      say So, get over there and give him this contract to sign and head on back quick smart.
      %load% obj 8220
      give contract %actor.name%
      set doingoathmanq
      remote doingoathmanq %actor.id%
    elseif (%actor.housetaesal% == retainer && !%actor.varexists(doingretainerq)%)
      wait 2 s
      say Back for more errand work I see %actor.name%. Good thing too, I've got another contract that needs signing.
      wait 3 s
      say Gladroon runs a general store in town, same as before, give him the contract to sign then return with it.
      %load% obj 8220
      give contract %actor.name%
      set doingretainerq
      remote doingretainerq %actor.id%
    elseif (%actor.housetaesal% == initiate && !%actor.varexists(doinginitiateq)%)
      wait 2 s
      say Hello initiate %actor.name%. Your first task is to go see Olquar at his shop in town.
      wait 3 s
      say You need to give him this contract for him to sign, then return here with it.
      wait 3 s
      say Not glamorous work no, but it helps pay the bills.
      %load% obj 8220
      give contract %actor.name%
      set doinginitiateq
      remote doinginitiateq %actor.id%
    else
      wait 1 s
      say Sorry, I have no missions for you currently.
    end
  end
end
~
#8142
Lord Aranni receives~
0 j 100
~
if %actor.varexists(housetaesal)%
  if (%actor.housetaesal% == oathman && %object.vnum% == 8221)
    wait 2 s
    say Hah, got one right under their noses, House Dres won't have seen that coming.
    wait 3 s
    say Traditionally House Dres merchants and traders are fiercely loyal to their House.
    wait 3 s
    say It warms the heart that, you've pulled through with this, bravo.
    wait 3 s
    say But let's move on to the rewards shall we? I promote you to Lawman in addition to your payment.
    rdelete finishedoathmanq %actor.id%
    set housetaesal lawman
    remote housetaesal %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    nop %actor.gold(1000)%
    %actor.exp(2000)%
    wait 2 s
    say You should go see Mistress Shela Taesal if you want more missions.
    %purge% %object%
  elseif (%actor.housetaesal% == retainer && %object.vnum% == 8221)
    wait 3 s
    say Excellent, another trader pledges his allegiance to House Taesal.
    wait 2 s
    say I'm promoting you to Oathman, %actor.name%. And of course, your payment.
    set housetaesal oathman
    remote housetaesal %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    rdelete finishedretainerq %actor.id%
    nop %actor.gold(750)%
    %actor.exp(1500)%
    %purge% %object%
  elseif (%actor.housetaesal% == initiate && %object.vnum% == 8221)
    wait 2 s
    say Good good, more money in the bank so to speak with this account secured.
    wait 2 s
    say I hearby promote you to the rank of Retainer and as customary, payment for your efforts.
    rdelete finishedinitiateq %actor.id%
    set housetaesal retainer
    remote housetaesal %actor.id%
    wait 2 s
    nop %actor.gold(500)%
    %actor.exp(1000)%
    emote hands you a small sack of zenni.
    %purge% %object%
  else
    say I don't want that.
    drop %object%
  end
end
~
#8143
Olquar receives~
0 j 100
~
if (%actor.varexists(doinginitiateq)% && %object.vnum% == 8220)
  wait 2 s
  say Ah yes, must be that time again, House Taesal always wanting merchants under their thumb.
  wait 3 s
  emote signs the contract.
  wait 2 s
  say Here you go, all done.
  %purge% %object%
  %load% obj 8221
  give contract %actor.name%
  rdelete doinginitiateq %actor.id%
  set finishedinitiateq
  remote finishedinitiateq %actor.id%
end
~
#8144
Gladroon receives~
0 j 100
~
if (%actor.varexists(doingretainerq)% && %object.vnum% == 8220)
  wait 2 s
  say That time? I could of sworn it was still some months away, oh well.
  wait 3 s
  emote signs the contract.
  wait 2 s
  say Here you go, all done.
  %purge% %object%
  %load% obj 8221
  give contract %actor.name%
  rdelete doingretainerq %actor.id%
  set finishedretainerq
  remote finishedretainerq %actor.id%
end
~
#8145
Dres Smith Receives~
0 j 100
~
if (%actor.varexists(doingoathmanq)% && %object.vnum% == 8220)
  wait 2 s
  say I feel bad about doing this to my house... but with Taesal, I stand to make more money.
  wait 3 s
  emote signs the contract.
  wait 2 s
  say Here you go, all done.
  %purge% %object%
  %load% obj 8221
  give contract %actor.name%
  rdelete doingoathmanq %actor.id%
  set finishedoathmanq
  remote finishedoathmanq %actor.id%
end
~
#8146
Shela Taesal missions~
0 d 100
mission~
if %speech% == mission
  if (%actor.varexists(konhousejoined)% && %actor.varexists(housetaesal)%)
    if (%actor.housetaesal% == lord && !%actor.varexists(doinglordq)%)
      wait 2 s
      say You've been quite the useful asset haven't you, %actor.name%?.
      wait 3 s
      say I have but one last task for you.
      wait 3 s
      say Councillor Marias seeks to pass a motion in the Kal Tyor council that would impact House Taesal negatively.
      wait 3 s
      say She needs to have a serious accident.
      set doinglordq
      remote doinglordq %actor.id%
    elseif (%actor.housetaesal% == merchant && !%actor.varexists(doingmerchantq)%)
      wait 2 s
      say I haven't got anything interesting for you other than some errand work.
      wait 3 s
      say You would of done alot of this with Lord Aranni so I assume you know the score.
      wait 3 s
      say Take this contract to the Bartender at the Turlon Fair.
      %load% obj 8220
      give contract %actor.name%
      set doingmerchantq
      remote doingmerchantq %actor.id%
    elseif (%actor.level% > 30 && %actor.housetaesal% == lawman && !%actor.varexists(doinglawmanq)%)
      wait 2 s
      say Greetings %actor.name%. You'll be dealing with me from now on.
      wait 3 s
      say I need something taken care of. Krathius Mazori has been causing trouble for Matsu Colvian.
      wait 3 s
      say If his wife, Ellyrion Mazori were to have an 'unfortunate' accident, the situation might resolve itself.
      set doinglawmanq
      remote doinglawmanq %actor.id%
    else
      wait 1 s
      say Sorry, I have no missions for you currently.
    end
  end
end
~
#8147
Shela Taesal receive~
0 j 100
~
if %actor.varexists(housetaesal)%
  if (%actor.housetaesal% == lord && %object.vnum% == 8223)
    wait 2 s
    say Mmm, I hear that Councillor Marias also suffered a most tragic accident.
    wait 3 s
    say Such a shame, she had much planned for Konack in the future.
    wait 3 s
    say Congratulations, you are now a councillor with House Taesal.
    rdelete finishedlordq %actor.id%
    set housetaesal councillor
    remote housetaesal %actor.id%
    %load% obj 8207
    give entry %actor.name%
    wait 2 s
    emote hands you a sack of zenni.
    nop %actor.gold(10000)%
    %actor.exp(500000)%
    %purge% %object%
  elseif (%actor.housetaesal% == merchant && %object.vnum% == 8221)
    wait 3 s
    say That one contract secures the loyalty of the entire Turlon Fair,  most pleasing.
    wait 2 s
    say I'm promoting you to Lord, %actor.name%. And of course, your payment.
    set housetaesal lord
    remote housetaesal %actor.id%
    wait 2 s
    emote hands you a sack of zenni.
    rdelete finishedmerchantq %actor.id%
    nop %actor.gold(2500)%
    %actor.exp(4000)%
    %purge% %object%
  elseif (%actor.housetaesal% == lawman && %object.vnum% == 8222)
    wait 2 s
    say I heard about the terrible accident that befell Ellyrion Mazori.
    wait 3 s
    say With Krathius grieving, it should give Matsu Colvian plenty of time to clean up whatever mess he got himself into.
    wait 3 s
    say And it seems you will profit from this. I promote you to the Merchant rank of House Taesal.
    rdelete finishedlawmanq %actor.id%
    set housetaesal merchant
    remote housetaesal %actor.id%
    wait 2 s
    nop %actor.gold(2000)%
    %actor.exp(158000)%
    emote hands you a small sack of zenni.
    %purge% %object%
  else
    say I don't want that.
    drop %object%
  end
end
~
#8148
Ellyrion assassinated mission~
0 f 100
~
if (%actor.varexists(housetaesal)% && %actor.varexists(doinglawmanq)%
  %load% obj 8222
  rdelete doinglawmanq %actor.id%
  set finishedlawmanq
  remote finishedlawmanq %actor.id%
end
~
#8149
Turlon Fair contract~
0 j 100
~
if (%actor.varexists(doingmerchantq)% && %object.vnum% == 8220)
  wait 2 s
  say Bout time Mistress Shela Taesal sent this contract my way to sign.
  wait 3 s
  emote signs the contract.
  wait 2 s
  say Alright, get it back to her straight away, she doesn't like being kept waiting.
  %purge% %object%
  %load% obj 8221
  give contract %actor.name%
  rdelete doingmerchantq %actor.id%
  set finishedmerchantq
  remote finishedmerchantq %actor.id%
end
~
#8150
Councillor marias accident mission~
0 f 100
~
if (%actor.varexists(housetaesal)% && %actor.varexists(doinglordq)%
  %load% obj 8223
  rdelete doinglordq %actor.id%
  set finishedlordq
  remote finishedlordq %actor.id%
end
~
$~
