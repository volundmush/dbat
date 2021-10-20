#19600
fly nullifier~
2 g 100
~
***********************************************************
* Check to see if flying/holding correct object, etc...
***********************************************************
eval objheld %actor.eq(17)%
if (%objheld.vnum% != 19600)
  wait 1
  if (%actor.flying%)
    %force% %actor% fly
    wait 1
  end
end
~
#19601
Keep track of kills/status between sides~
0 f 100
~
***********************************************************
* "hoshifaction" and "hoshisedquest" player variables
***********************************************************
if (!%actor.varexists(hoshipeacekills)% || !%actor.varexists(hoshichaoskills)%)
  set hoshichaoskills 0
  set hoshipeacekills 0
  remote hoshichaoskills %actor.id%
  remote hoshipeacekills %actor.id%
end
if (%actor.varexists(hoshisedquest)%)
  set hoshichaoskills %actor.hoshichaoskills%
  set hoshipeacekills %actor.hoshipeacekills%
  if (%self.vnum% == 19605 || %self.vnum% == 19606 || %self.vnum% == 19603 || %self.vnum% == 19610)
    eval hoshichaoskills %hoshichaoskills% + 1
  else
    eval hoshipeacekills %hoshipeacekills% + 1
  end
  remote hoshichaoskills %actor.id%
  remote hoshipeacekills %actor.id%
end
~
#19602
Sisuri Quest Greeting and Reaction~
0 g 100
~
@n*********************************************************
* Sisuri quest reaction/initiation
*********************************************************
if (%actor.varexists(hoshichaoskills)% && %actor.varexists(hoshipeacekills)%)
  set num1 %actor.hoshichaoskills%
  set num2 %actor.hoshipeacekills%
  eval hoshifaction %num1% - %num2%
end
wait 1s
*********************************************************
* If player hasn't accepted quest yet...
*********************************************************
if (!%actor.varexists(hoshisedquest)%)
  wait 1s
  emote sighs, tapping his temple with a rolled up piece of paper.  @C'Damnit!  There has to be a way to win this.'@n
  wait 5s
  emote looks up, startled.  @C'Hey... you're not one of us.'@n  He stops tapping his temple with the piece of paper.  @C'You don't seem to be one of them, either.'@n
  wait 5s
  say Hm.  I think you could help us.  Would you @Raccept@C a mission to help us against those who would seek to plunge the Hoshi-Jin into a civil war?
  *********************************************************
  * If the player has accepted before...
  *********************************************************
  * And chose the other side...
  *********************************************************
elseif (%actor.hoshisedquest% == Addonmera || %hoshifaction% < 0 || %actor.hoshisedquest% == Addonmeradone)
  wait 2s
  emote frowns, his face set in an expression of disapproval.  @C'You take up arms against us.  You aren't welcome here.'@n
  wait 5s
  emote stands up and advances on *%actor.name%.  @C'Turn the tide of battle in our favor, and perhaps I will reconsider your worth to us.  Tell Sura you wish to @Rdedicate@C yourself to our cause if that's what you desire.'@n
  wait 5s
  if (%actor.vnum% == %self.vnum%)
    %force% %actor% w
  end
  *********************************************************
  * Or chose this side...
  *********************************************************
else
  wait 2s
  emote looks up, smiling.  @C'Ahhh.  Hello, %actor.name%.'@n
  wait 4s
  set limit 100
  if (%hoshifaction% <= %limit%)
    say You're doing quite well.  I'm not ready to give you clearance to see the General yet, but I do appreciate your efforts.
  else
    say Wow.  You've really helped turn the tide of battle for us.  If you want to see the General, go right on ahead.
  end
end
wait 1
~
#19603
Addonmera Quest Greeting and Reaction~
0 g 100
~
@n*********************************************************
* Addonmera quest reaction/initiation
*********************************************************
if (%actor.varexists(hoshichaoskills)% && %actor.varexists(hoshipeacekills)%)
  set num1 %actor.hoshichaoskills%
  set num2 %actor.hoshipeacekills%
  eval hoshifaction %num2% - %num1%
end
wait 1s
*********************************************************
* If player hasn't accepted quest yet...
*********************************************************
if (!%actor.varexists(hoshisedquest)%)
  wait 1s
  emote laughs as she reads a report.  @C'It won't be much longer now.  Those fools can't adapt to the new ways.@n
  wait 5s
  emote grins and looks up with a sneer.  @C'Yet another warrior to talk about peace?'@n  She considers something for a moment.  @C'No.  You're not one of them.  You look like someone with a little more sense.'@n
  wait 5s
  say Hm.  I would reward you if you murdered some of those annoying Peace warriors.  Enough to turn the tide of battle even more in our favor.  What do you say?  Do you @Raccept@C?'@n
  *********************************************************
  * If the player has accepted before...
  *********************************************************
  * And chose the other side...
  *********************************************************
elseif (%actor.hoshisedquest% == Sisuri || %hoshifaction% < 1 || %actor.hoshisedquest% == Sisuridone)
  wait 2s
  emote frowns, her face set in an expression of anger.  @C'You take up arms against us.  You aren't welcome here.'@n
  wait 5s
  emote stands up and advances on *%actor.name%.  @C'Turn the tide of battle in our favor, and perhaps I will reconsider your worth to us.  Tell Mustuc you wish to @Rdedicate@C yourself to our cause if that's what you desire.'@n
  wait 5s
  if (%actor.vnum% == %self.vnum%)
    %force% %actor% w
  end
  *********************************************************
  * Or chose this side...
  *********************************************************
else
  wait 2s
  emote looks up, smiling.  @C'Ahhh.  Hello, %actor.name%.'@n
  wait 4s
  set limit 100
  if (%hoshifaction% <= %limit%)
    say I like the way you've taken to slaughtering those annoying Peace warriors.  Keep it up, and I'll have a nice reward for you!
  else
    say Excellent!  You've done an amazing job.  Those foolish Peace warriors will soon be destroyed beneath my heel!  Proceed east to the General for your well earned reward. 
  end
end
wait 1
~
#19604
Mustoc "dedicate" script (change sides)~
0 d 100
dedicate~
@n*********************************************************
*  Mustoc "dedicate" agree to the cause
*********************************************************
if (!%actor.varexists(hoshisedquest)%)
  wait 1s
  if (%self.vnum% == 19603)
    set theboss Addonmera
  elseif (%self.vnum% == 19604)
    set theboss Sisuri
  end
  say I think maybe you should talk to %theboss%, %actor.name%
elseif (%actor.hoshisedquest% == Addonmera)
  wait 1s  
  say You seem to already be dedicated to our cause, %actor.name%.  Go kill some Warriors of Peace!
elseif (%actor.hoshisedquest% == Sisuri)
  wait 1s  
  say Hmm... You want to defect to our side, eh?  Alright.  Stop killing our warriors and go after the Warriors of Peace.  See Addonmera when you've proven yourself.
  set hoshisedquest Addonmera
  remote hoshisedquest %actor.id% 
  wait 5s
  emote stares at *%actor.name% threateningly.  @C'I'll be watching you, %actor.name%'@n
elseif (%actor.hoshisedquest% == Sisuridone)
  wait 1s
  say Really?  You want to Join us?  HAH!  You've done irreperable harm to our cause!
  wait 4s
  emote growls angrily.  @C'You can never be trusted!  In fact...'@n  He draws back a fist, preparing to strike!
  punch %actor.name%
elseif (%actor.hoshisedquest% == Addonmeradone)
  wait 1s
  emote grins, slapping you on the back amiably, if a little roughly.  @C'%actor.name%, your dedication is legend around here!  With the help you've given us, we should be able to take out those annoying Warriors of peace in no time!'@n
end
~
#19605
Sura "dedicate" script (change sides)~
0 d 100
dedicate~
@n*********************************************************
*  Sura "dedicate" agree to the cause
*********************************************************
if (!%actor.varexists(hoshisedquest)%)
  wait 1s
  if (%self.vnum% == 19603)
    set theboss Addonmera
  elseif (%self.vnum% == 19604)
    set theboss Sisuri
  end
  say I think maybe you should talk to %theboss%, %actor.name%
elseif (%actor.hoshisedquest% == Sisuri)
  wait 1s  
  say You seem to already be dedicated to our cause, %actor.name%.  Go subdue some of those Warriors of Chaos!
elseif (%actor.hoshisedquest% == Addonmera)
  wait 1s  
  say Hmm... You want to defect to our side, eh?  Alright.  Stop killing our warriors and go after the Warriors of Chaos.  See Sisuri when you've proven yourself.
  set hoshisedquest Sisuri
  remote hoshisedquest %actor.id% 
  wait 5s
  emote stares at *%actor.name% threateningly.  @C'I'll be watching you, %actor.name%'@n
elseif (%actor.hoshisedquest% == Addonmeradone)
  wait 1s
  say Really?  You want to Join us?  HAH!  You've done irreperable harm to our cause!
  wait 4s
  emote growls angrily.  @C'You can never be trusted!  In fact...'@n  She draws back a fist, preparing to strike!
  punch %actor.name%
elseif (%actor.hoshisedquest% == Sisuridone)
  wait 1s
  emote smiles and bows.  @C'Great %actor.name%, you have already proven your dedication.  With your help, I think we can finally win this senseless struggle.'@n
end
~
#19606
Addonmera Quest Accept~
0 d 100
accept~
@n*******************************************************
* Addonmera Accept Quest Trigger
*******************************************************
if (!%actor.varexists(hoshisedquest)%)
  wait 1s
  say very well, then.  Go, kill the Warriors of Peace!  Weaken them so we can crush them once and for all!
  set hoshisedquest Addonmera
  set hoshichaoskills 0
  set hoshipeacekills 0
  remote hoshisedquest %actor.id%
  remote hoshipeacekills %actor.id%
  remote hoshichaoskills %actor.id%
elseif (%actor.hoshisedquest% == Sisuri || %actor.hoshisedquest% == Sisuridone)
  wait 1s  
  say Pfft!  It seems you've already joined those pathetic Warriors of Peace!
  wait 5s
  emote rubs her hands together, her face twisted in a malicious snarl.  @C'If you're thinking about defecting, however, tell Mustuc you want to @Rdedicate@C yourself to our cause instead.'@n
elseif (%actor.hoshisedquest% == Addonmera || %actor.hoshisedquest% == Adonmeradone)
  wait 1s  
  say Idiot, you're already on our side.  What's taking you so long?  Go kill some Warriors of Peace!
end
~
#19607
Sisuri Quest Accept~
0 d 100
accept~
@n*******************************************************
* Sisuri Accept Quest Trigger
*******************************************************
if (!%actor.varexists(hoshisedquest)%)
  wait 1s
  say Excellent news!  We need all the help we can get to beat back the chaos that threatens to overrun the Hoshi-Jin.  Subdue as many Warriors of Chaos as you can, mighty %actor.name%.
  set hoshisedquest Sisuri
  set hoshichaoskills 0
  set hoshipeacekills 0
  remote hoshisedquest %actor.id%
  remote hoshipeacekills %actor.id%
  remote hoshichaoskills %actor.id%
elseif (%actor.hoshisedquest% == Addonmera || %actor.hoshisedquest% == Addonmeradone)
  wait 1s  
  say Pfft!  It seems you've already joined those pathetic Warriors of Chaos!
  wait 5s
  emote manages to control his anger.  He continues more calmly, @C'If you're thinking about defecting, however, tell Sura you want to @Rdedicate@C yourself to our cause instead.'@n
elseif (%actor.hoshisedquest% == Sisuri || %actor.hoshisedquest% == Sisuridone)
  wait 1s  
  say %actor.name%, you're already pledged to support our cause.  Please, make haste and help thin the ranks of the Warriors of Chaos.
end
~
#19608
No fly zone~
2 c 100
fly~
wait 1
eval objheld %actor.eq(17)%
if (%objheld.vnum% != 19600)
  %send% %actor% @RYour body feels strange as you attempt to fly here.  You struggle to remain in the air, but eventually fail.  As you land, a sudden weakness overcomes you and your knees give out.@n
  %echoaround% %actor% @R%actor.name% flails for a moment in the air before crashing into the ground.@n
  %force% %actor% sit
else
  %force% %actor% fly
end
~
#19609
Quest Finish Room Block~
2 q 100
~
if (%direction% == east)
  if (%actor.varexists(hoshisedquest)%
    if (%self.vnum% == 19678 && %actor.hoshisedquest% == Sisuri || %actor.hoshisedquest% == Sisuridone)
      set num1 %actor.hoshichaoskills%
      set num2 %actor.hoshipeacekills%
      eval thebalance %num1% - %num2%
    elseif (%self.vnum% == 19673 && %actor.hoshisedquest% == Addonmera || %actor.hoshisedquest% == Addonmeradone)
      set num1 %actor.hoshipeacekills%
      set num2 %actor.hoshichaoskills%
      eval thebalance %num1% - %num2%
    else
      set thebalance 0
    end
    if (%self.vnum% == 19678)
      set sedmob Sisuri
      if (%actor.hoshisedquest% == Addonmera || %actor.hoshisedquest% == Addonmeradone)
        set thebalance 0
      end
    elseif (%self.vnum% == 19673)
      set sedmob Addonmera
      if (%actor.hoshisedquest% == Sisuri || %actor.hoshisedquest% == Sisuriadone)
        set thebalance 0
      end
    end
    if (%thebalance% < 100)
      %force% %sedmob% emote reaches out a hand, blocking the way east.  @C'I don't think you've earned that priviledge quite yet.@n
      return 0
    end
  else
    %send% %actor% @RYou cannot proceed without at least accepting the quest for the area...@n
    return 0
  end
end
~
#19610
Quest Complete, Give Orb -> Set remote to done~
0 g 100
~
*****************************************************************
* Complete the quest, give the reward (Orb so you can fly here)
*****************************************************************
if (%self.vnum% == 19697)
  set sedmob Mestyssol
elseif (%self.vnum% == 19696)
  set sedmob Tanceri
end
wait 1s
if (%actor.hoshisedquest% != Addonmeradone && %actor.hoshisedquest% != Sisuridone)
  emote looks up from %self.hisher% paperwork.  @C'Ahhhh, %actor.name%!  I've heard about how well you've aided us.'@n
  wait 5s
  emote stands up and takes a few steps closer to you.  @C'You are one of us, now.'@n  %self.heshe% smiles, then laughs.  @C'To reward you, I'm going to give you something special.'@n
  wait 5s
  %load% obj 19600
  give orb %actor.name%
  if (%actor.hoshisedquest% == Addonmera)
    set hoshisedquest Addonmeradone
    remote hoshisedquest %actor.id%
  elseif (%actor.hoshisedquest% == Sisuri)
    set hoshisedquest Sisuridone
    remote hoshisedquest %actor.id%
  end
  wait 2s
  emote returns to %self.hisher% desk.  @C'Hold that orb, and the aura that pervades this place won't interfere with your ability to fly anymore.'@n
  wait 5s
  emote smiles, tapping %self.hisher% fingers on the desk.  @C'Thanks again, %actor.name%.  It won't be long before we win this... for lack of a better word... war.'@n
else
  wait 2s
  emote smiles, tapping %self.hisher% fingers on the desk.  @C'Thanks again, %actor.name%.  It won't be long before we win this... for lack of a better word... war.'@n
end
~
#19611
Jump Across the Chasm to Enter Area~
2 c 100
jump~
wait 1s
%echoaround% %actor.name% %actor.name% readies %actor.himher%self, preparing to jump across the Chasm.  With a running start, %actor.heshe% launches %actor.himher%self into the air!
%send% %actor% You step back and get a running start before attempting to jump over the Chasm.
wait 1s
if (%actor.level% < 20)
  %teleport% %actor% 19698
  wait 2s
  %force% %actor% look
  wait 1
  %send% %actor% The wind rushing past you is incredible!  At least... until you realize you aren't going to make it.  You plummet like a rock and somehow manage to avoid injury as you land in the Chasm.
  %echoaround% %actor.name% %actor.name% falls like a rock from above!  It looks as if %actor.heshe%'s landed without injury, though.
else
  %teleport% %actor% 19601
  %force% %actor% look
  wait 1
  %send% %actor% The wind rushing past you is incredible!  You hang in the air for a moment, then land safely on the other side of the Chasm.
  %echoaround% %actor.name% %actor.name% sails through the air from the south, landing nearby with a thud.
end
~
#19612
Entrance Fail walking North~
2 q 100
~
eval haveorb %actor.eq(17)%
if (%direction% == north && %haveorb.vnum% != 19600)
  %echoaround% %actor.name% %actor.name% walks straight off the edge of the @DChasm@n and immediately begins falling straight down.  So... %actor.heshe% must be insane.
  %teleport% %actor% 19698
  wait 1
  %echoaround% %actor.name% %actor.name% plummets from above and lands here with a thunk.
  %force% %actor% look
  %send% %actor% Uh... You walk straight off the edge and plummet to the @DChasm@n below.
elseif (%direction% == south)
  %force% %actor% s
elseif (%direction% == down)
  %force% %actor% d
else
  %force% %actor% n
end
~
#19613
Entrance Fail Walking South~
2 q 100
~
eval haveorb %actor.eq(17)%
if (%direction% == south && %haveorb.vnum% != 19600)
  %echoaround% %actor.name% %actor.name% walks straight off the edge of the @DChasm@n and immediately begins falling straight down.  So... %actor.heshe% must be insane.
  %teleport% %actor% 19698
  wait 1
  %echoaround% %actor.name% %actor.name% plummets from above and lands here with a thunk.
  %force% %actor% look
  %send% %actor% Uh... You walk straight off the edge and plummet to the @DChasm@n below.
elseif (%direction% == north)
  %force% %actor% n
else
  %force% %actor% s
end
~
#19614
Cepheus Information Greeting~
0 g 100
~
wait 1s
emote sighs, one hand on %self.hisher% hip.  @C'These fools will be the ruin of us all.'@n
wait 5s
emote rubs %self.hisher% temple with his other hand, fighting off a headache.  @C'Hoshi-Jin should not fight Hoshi-Jin.  This is not what master Jinto wants.'@n
~
#19615
Hot Potato Script~
2 c 100
throw~
if (%actor.inventory(19603)%)
  set newtarget %random.char%
  if (%arg% == potato)
    %echoaround% %actor% %actor.name% tosses the potato!
    %send% %actor% You toss the potato!
    wait 2s
    if (%newtarget.name% == %actor.name%)
      %echoaround% %actor% @RH@Wa@Rh@Wa@Rh@Wa@R! @W%actor.name% tries to throw the potato and fails!@n
      %send% %actor% @RYou fail to throw the potato!  WHAT THE HELL!@n
    else
      set hpgame lolpotatonohax %newtarget.name%
      remote hpgame %actor.id%
      %force% %actor% give lolpotatonohax %newtarget.name%
      %echo% @R%newtarget.name% now holds the potato!@n
    end
  else
    %send% %actor% @RTry throwing the potato.@n
  end
else
  %send% %actor% @RYou don't have a potato to throw...@n
end
~
#19616
Hot Potato Cheater~
2 c 100
drop~
%echo% %actor.name% has attempted to cheat!  BYE BYE CHEATER!
wait 1s
%teleport% %actor% 16695
%force% %actor% look
~
#19617
Hot Potato Timer~
2 c 100
hotpotatostart~
%echo% @C%actor.name% has started a game of hot potato!  Timer has started.@n
set potatotimer %random.60%
wait %potatotimer%s
%echo% @RTime's up!@n
set people %self.people%
while (%people%)
  if (%people.inventory(19603)%)
    %echoaround% %people.name% @R%people.name% vanishes in a puff of smoke.  %people.heshe% must have had the potato!@n
    %teleport% %people.name% 16695
    %force% %people.name% look
    %send% %people.name% @RYou lose!  Chill out in the losers lounge with the rest of the losers.@n
    %force% %people.name% junk lolpotatonohax
  end
  set people %people.next_in_room%
done
~
#19618
Hot Potato Cheater~
2 c 100
junk~
%echo% %actor.name% has attempted to cheat!  BYE BYE CHEATER!
wait 1s
%teleport% %actor% 16695
~
#19619
Hot Potato Cheater~
2 c 100
give~
set cheatcheck fail
if (%actor.varexists(hpgame)%)
  set cheatcheck %actor.hpgame%
end
if (%arg% != %cheatcheck%)
  %echo% %actor.name% has attempted to cheat!  BYE BYE CHEATER!
  wait 1s
  %teleport% %actor% 16695
  %force% %actor% look
else
  return 0
end
~
#19620
Sisuri Quest Mute Accept~
0 c 100
accept~
*******************************************************
* Sisuri Accept Quest Trigger
*******************************************************
if (!%actor.varexists(hoshisedquest)%)
  wait 1
  %send% %actor% You nod your head in acceptance.
  %echoaround% %actor% %actor.name% nods %actor.hisher% head in acceptance.
  wait 1s
  say Excellent news!  We need all the help we can get to beat back the chaos that threatens to overrun the Hoshi-Jin.  Subdue as many Warriors of Chaos as you can, mighty %actor.name%.
  set hoshisedquest Sisuri
  set hoshichaoskills 0
  set hoshipeacekills 0
  remote hoshisedquest %actor.id%
  remote hoshipeacekills %actor.id%
  remote hoshichaoskills %actor.id%
elseif (%actor.hoshisedquest% == Addonmera || %actor.hoshisedquest% == Addonmeradone)
  wait 1
  %send% %actor% You nod your head in acceptance.
  %echoaround% %actor% %actor.name% nods %actor.hisher% head in acceptance.
  wait 1s  
  say Pfft!  It seems you've already joined those pathetic Warriors of Chaos!
  wait 5s
  emote manages to control his anger.  He continues more calmly, @C'If you're thinking about defecting, however, tell Sura you want to @Rdedicate@C yourself to our cause instead.'@n
elseif (%actor.hoshisedquest% == Sisuri || %actor.hoshisedquest% == Sisuridone)
  wait 1
  %send% %actor% You nod your head in acceptance.
  %echoaround% %actor% %actor.name% nods %actor.hisher% head in acceptance.
  wait 1s  
  say %actor.name%, you're already pledged to support our cause.  Please, make haste and help thin the ranks of the Warriors of Chaos.
end
~
#19694
Infinity Gauntlet~
1 c 2
sedfix~
%echo% @RBzzzzzzzzzzzzzzzzzzt@n
~
#19695
Weed Brownie~
1 c 3
devour~
set eatit %cmd%
if (%eatit% == devour && %arg% /= brow)
  %send% %actor% You devour some of the brownie... a warm, tingly feeling spreads from your stomach to your toes, fingers, and face.
  %echoaround% %actor% %actor.name% devours a weed brownie!
  wait 10s
  %force% %actor% emote smiles as their eyes begin to gloss over.
  wait 15s
  %force% %actor% emote slumps slightly as their body begins to relax entirely too much.
  wait 15s
  %force% %actor% say Oh my god... I am so hungry...
  wait 15s
  %force% %actor% I have to find food.  I will be back...
  wait 2s
  %force% %actor% e
  %force% %actor% e
  %force% %actor% s
  %force% %actor% s
  %force% %actor% s
  %force% %actor% w
  %force% %actor% n
  %force% %actor% s
  %force% %actor% s
end
return 1
~
#19696
Tower Commands~
2 d 1
tower~
set owner Raist
set guest Anekohji
if (%actor.name% != %owner% && %actor.name% != %guest%)
  wait 1s
  %echo% @RAn automated voice echoes into the room, '@YUnauthorized User.@R'@n
else
  extract commandword 1 %speech%
  extract mycommand 2 %speech%
  extract mycommand2 3 %speech%
  extract mycommand3 4 %speech%
  extract mycommand4 5 %speech%
  extract mycommand5 6 %speech%
  extract mycommand6 7 %speech%
  if (%commandword% /= tower)
    if (%mycommand% /= dim && %mycommand2% /= lights)
      wait 1s
      %echo% @RIn response to the command, the lights in the room dim.@n
    elseif (%mycommand% == raise && %mycommand2% /= lights)
      wait 1s
      %echo% @RIn response to the command, the lights in the room brighten.@n
    elseif (%mycommand% /= music)
      if (%mycommand2% /= on)
        wait 1s
        %echo% @RIn response to the command, music begins playing in the room.@n
      else
        wait 1s
        %echo% @RIn response to the command, a brief note is heard before silence fills the room.@n
      end
    elseif (%mycommand% /= bed)
      wait 1s
      if (%findobj.18995(18988) < 1)
        %echo% @RIn response to the command, a bed appears on a platform in the room.@n
        %load% obj 18988
      else
        %echo% @RIn response to the command, a @WLarge, @RH@Me@Ra@Mr@Rt@W Shaped Bed@R vanishes instantly.@n
        %purge% towerbed
      end
    elseif (%mycommand% /= tell && %mycommand2% /= me && %mycommand3% /= about)
      wait 1s
      if (%mycommand4% /= %owner%)
        %echo% @RAn automated voice echoes into the room, '@Y%owner%.  Male Demon.  Master of Ominous Tower.@R'@n
      elseif (%mycommand4% /= %guest%)
%echo% @RAn automated voice echoes into the room, '@Y%guest%:  half human, half saiyan.  Misunderstood by so many, but seems to trust Raist... the happiest thing any accomplishment of his has ever brought him.@R'@n
      elseif (%mycommand4% /= yourself)
        %echo% @RAn automated voice echoes into the room, '@YMyself.  I am a hell construct built by %owner% and controlled by his subconscious.@R'@n
      elseif (%mycommand4% /= Earth)
        %echo% @RAn automated voice echoes into the room, '@YEarth.  Location of Ominous tower.  Beautiful women.@R'@n
      elseif (%mycommand4% /= Toriko)
        %echo% @RAn automated voice echoes into the room, '@YToriko.  Female halfbreed.  Manipulative, unstable, codependant.  Avoid.@R'@n
      elseif (%mycommand4% /= ship)
        %echo% @RAn automated voice echoes into the room, '@YShip.  Silvenkosappo.  @rERROR!@R'@n
      else
        %echo% @RAn automated voice echoes into the room, '@Y%mycommand4%, unknown.@R'@n  
      end
    else
      wait 1s
      %echo% @RAn automated voice echoes into the room, '@YUnrecognized command.@R'@n
    end
  end
end
~
#19697
Door Sensor Trigger~
2 g 100
~
set insidedir north
set entryroom 18985
set owner Raist
if (%actor.name% != %owner%)
  wait 1s
  if (%actor.room.vnum% == %entryroom% && %direction% != %insidedir%)
    %send% %actor% @RA security device begins tracking your movements.@n
    %echoaround% %actor% @RA security device begins tracking something.@n
    %at% 18986 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18987 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18988 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18989 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18990 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18991 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18992 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18993 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18994 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18995 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18996 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18997 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18998 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
    %at% 18999 %echo%@R An automated voice makes an announcement: '@YA %actor.sex% %actor.race% has been detected at the entrance of the tower.@R'@n
  end
end
~
#19698
Raist Cleaner Mob Script~
0 g 100
~
set cleanchance %random.5%
if (%cleanchance% == 5)
  wait 2s
  emote hums to herself as she dusts objects in this room.
elseif (%cleanchance% == 4)
  wait 2s
  emote drops her featherduster and sighs.  Bending over slowly, a glimpse of her panties can be seen as she picks it back up.
elseif (%cleanchance% == 3)
  wait 2s
  emote looks up at *%actor.name% and smiles.  @M'Oh hello!  Don't mind me, I'm just tidying up a bit, teehee!'@n  With a giggle, she goes back to cleaning.
else
  wait 2s
emote wiggles and dances, her french maid outfit jiggling alarmingly in certain places.  Noticing *%actor.name% enter, she yelps in surprise and quickly stops.  @M'Oh... Uh... I was just... doing some cleaning.  Yeah...'@n  Wish a blush, she goes back to cleaning.
end
~
#19699
Alysara Greet Script~
0 g 100
~
set owner Raist
if (%actor.name% == %owner%)
  wait 2s
  emote smiles and nods respectfully.  @M'Welcome home, master.'@n
else
  wait 2s
  emote crosses her arms and stands in front of the entrance to the ominous tower.  @M'Welcome to %owner%'s abode.  If he didn't invite you here, don't bother trying to get in.'@n
end
~
$~
