#62900
Altar's Candles Flickering~
2 b 5
~
%echo% The room darkens as the candles burn out... 
wait 2s
%echo% The markings on the wall glow brightly, and light returns to the room as the candles burn again.
~
#62901
Woodrat Eating Corpse~
0 h 100
~
wait 1 s
emote sinks her teeth into the corpse and rips off the decaying flesh. The @yW@wo@Do@wd@yr@Da@wt@n@w chews savagely.@n
wait 1 s
emote notices you and then snarls aggressively.
~
#62903
Hoshijin Runic Language~
2 c 100
Decipher~
if (%actor.race% == Hoshijin)
  wait 1 s
  %send% %actor% The runes grow brilliantly as you read them...
  %echoaround% %actor% The runes seem to react to %actor.name% and they begin to glow in %actor.hisher% presence.
  wait 2 s
  %send% %actor% @WThe @Csecrets@W are in the stars...@n
  wait 2 s
  %send% %actor% @WRemember this...@n
  wait 2 s
  %send% %actor% @WThe @Canswers@W are in the stars...@n
  wait 2 s
  %send% %actor% @WThe stars' secrets and answers can be found just above your head...@n
  wait 2 s
  %echoaround% %actor% %actor.name% seems to have a good understanding of the runes.
  %send% %actor% You easily understand the runes.
else
  wait 1 s
  %send% %actor% You are unable to decipher the runes
  %echoaround% %actor% %actor.name% appears to be baffled by the runes.
end
~
#62904
Scared Holy Order Entrance~
0 g 100
~
wait 1 s
say Oh it's just you, I thought you were something scarier!
~
#62905
Flickering Lights~
2 bg 100
~
%send% %actor% A @cstrong@w wind blows through the pyramid...
~
#62906
Scared Guard Entrance~
0 g 100
~
if %actor.race% == Demon
 wait 1 s
 say AH! A Demon!
 wait 1 s
 %send% %actor% The guard charges straight to YOU!
 %echoaround% %actor% The guard seems to have gone berserk.
 wait 1 s
 punch %actor.name%
else
 say Oh it's just you. I thought you were some evil monster or something.
end
~
#62907
Commander's Recruit~
0 g 100
~
if %actor.race% == Demon
  wait 1 s
  emote notices @c%actor.name%@w, raises his sword and then utters an ancient dialect.@n
  wait 1 s
  %send% %actor% @WYou feel your very soul and body being torn apart as you are expelled from the room!@n
  %teleport% %actor% 62901
  %echoaround% %actor% @WThe @CC@commander@W raises his sword and a @Ybright light@W fills the room!@n
elseif (%actor.varexists(good_cavern)%)
  wait 1 s
  say Please keep helping us. The Void must not be allowed to come
else
  wait 2 s
  emote smiles warmly at %actor.name%. &1Greetings there. We are currently undermanned in our conflict with the Demons here. I and many of my Holy Order brothers would appreciate it if you could help us out a bit.&2
  wait 4 s
  %force% Kopernukis emote nods in agreement with the Commander. &1Yes the sooner you and the Demons leave this place, the sooner I can get back to fully training Astromancers...&2
  wait 5 s
  emote ignores the @YH@yigh @BA@Yst@Wr@wo@bm@Yan@Wc@we@Yr@w's comment and continues speaking with @c%actor.name%@n. &1Will you @Gassist@C us in stopping the Demons?&2@n
end
~
#62908
Demonic Owl Passage~
2 c 100
touch~
if (%cmd% == touch)
  if (%arg% == owl)
    if (%actor.race% == Demon)
      wait 1 s
      %send% %actor% @WTouching the owl causes it look upon you like prey!@n
      wait 1 s
      %send% %actor% @WThe ground beneath you quickly disappears and you fall down before it reappears again!@n
      wait 1 s
      %echoaround% %actor% @C%actor.name%@W literally falls through the ground!@n
      %teleport% %actor% 62999
      %force% %actor% look
    elseif (%actor.hoshi_cavern% == Owl)
      wait 1 s
      %send% %actor% @WYou place your hand on the statue...@n
      %echoaround% %actor% @C%actor.name%@W places %actor.hisher% hand on the statue...@n
      wait 1 s
      %echo% @WThe statue rumbles loudly!@n
      wait 1 s
      %load% mob 62922
      %echo% @WThe statue's defender appears!@n
    else 
      wait 1 s
      %send% %actor% @WThe owl's eyes seem to concentrate on you as you touch it.@n
    end
  else
    wait 1 s
    %send% %actor% @WOkay, you typed @Gtouch@W, but you didn't type touch owl. Do you want to touch the owl, or are you just trying to molest someone here?@n
  end
else
  wait 1 s
  %send% %actor% @WLook, ya mook. You can't add stuff to the end of @Gtouch@W. You typed @G%cmd%@W, which is probably something dumb like touchee. Type touch exactly for your first word.@n
end
~
#62909
Levay's Greet Message~
0 g 100
~
wait 1 s
%echo% @RL@Ce@Dv@Ca@Dy@w, a Greater @RD@De@Wm@Do@Rn@w, picks his sharp teeth with with an equally sharper toothpick that has been carved out of bone.@n
wait 2 s
%echo% @RL@Ce@Dv@Ca@Dy@w, a Greater @RD@De@Wm@Do@Rn@w, notices you and then throws the bone aside.@n
wait 1 s
say Hey maggot! When you're done standing there looking like an idiot, I have something I want you to do for me. Just tell me that you'll @Gaccept@C my task and I will tell you more on what I want@n
~
#62910
Levay's Quest Message 1~
0 d 100
accept~
set evil_cavern_q1
* @CHere we are creating the variable that will be used in this quest@w
if (%actor.varexists(evil_cavern_q1)%)
  eval evil_cavern_q1 %actor.evil_cavern_q1%
  * @CIf the actor has the variable already, set it to whatever@w
  * @Cthe actor's variable is set to (started, finished, etc)@w
  * @CBelow, we'll use this variable to determine response from the mob@w
else
  eval evil_cavern_q1 Not_set
  * @CIf the var doesn't exist, set evil_cavern_q1 to "not_set"@w
end
* @CTake note:  Right off the bat, I download the variable from the@w
* @Cplayer to the variable we created on the first script line.@w
if (%evil_cavern_q1% == Not_set)
  * @CIf the player hasn't accepted the quest yet...@w
  if (%actor.align% <= -300)
    * @CIf the player's alignment is evil (less than -300)@w
    say Excellent!  I am glad that you aren't as stupid as you look.
    wait 2s
    say The Holy Order swine are trying to stop us from releasing the first Horseman of the Void, Pestilence.  I need you to go to Jayne and get her half of the fragmented key.
    wait 3s
    say When you're done, return with the broken key and give it to me.
    eval evil_cavern_q1 Started
    remote evil_cavern_q1 %actor.id%
  else
    * @CIf the player's alignment isn't evil...@w
    wait 1s
    say Your Ki tastes to sweet for my liking!  Return to me once you have acquired a more sour Ki!
    wait 2s
    emote snaps his fingers and *%actor.name% is forced out of the room!
    wait 1
    if (%actor.room.vnum% == %self.room.vnum%)
      %teleport% %actor% 62901
      * @CNote that I checked to make sure the char was still in the room@w
      * @Cbefore I teleported them away.@w
      %force% %actor% look
    end
  end
elseif (%evil_cavern_q1% == Started)
  * @CIf the quest is already started...@w
  wait 1s
  say Haven't I already told you what I wanted? 
  wait 1s
  emote shrugs.
  wait 2s
  say Well I guess all idiots look alike. Go get Jayne's part of the key from her.
elseif (%evil_cavern_q1% == Finished)
  wait 1s
  say You've already performed my task.  What are you, some kind of idiot?
else
  say Hey, you've broken the quest somehow.  Please inform an imm to take a look at this quest.
end
~
#62911
Jayne's Smooch Request~
0 d 100
Fragmented Broken Key~
if (%actor.varexists(evil_cavern_q1)%)
  * @CHere we are checking to see if the actor has the quest vnum from trig @R62910@w
  if (%actor.evil_cavern_q1% == Started)
    * @CHere we are checking to see if the player has the quest. This is important once the player has the completed the quest.@w
    wait 1s
    say Ah yes, you want my part of the key.
    wait 1s
    emote giggles mischievously.
    wait 1s
    say I haven't been to Hell in quite some time so I haven't had any chance to indulge my desires.
    wait 2s
    emote puts a finger on her lips, &1How about you give me one little smooch&2 She winks suggestively at %actor.name%. &1If it's a good kiss then I'll give you what you want&2
  end
  * @CThis ends the part where the mob asks the player to smooch her.@w
else
  wait 1s
  * @CThe following will only apply because the variable did not exist on the player.@w
  emote looks away from %actor.name%, &1Sorry, I have no idea what you're talking about&2
  * @CThis ends the "else" part. Else does not get an end so the else we see below is for the first if@w
end
~
#62912
Jayne's Smooch Trigger~
0 c 100
smooch~
if (%cmd% == smooch)
  if (%arg% == Jayne)
    if (%actor.varexists(evil_cavern_q1)%)
      if (%actor.evil_cavern_q1% == Started)
        if (%actor.int% >= 20)
          * @CYIf the actor's INT stat is greater than 20@w
          wait 2 s
          %echo% @WYou grab @RJ@Ca@Dy@Cn@De@w, a @RD@De@Wm@Do@Rn@w of @ML@Wu@Ms@Wt@w @Wand give her a big wet kiss!@n
          wait 1 s
          emote smiles at@c %actor.name%@n. &1Not one of the best ones I've had but I guess it was satisfactory...&2
          wait 2 s
          say Now then just like we agreed
          wait 3 s
          emote reaches into her cleavage and pulls a small, golden item out.
          %load% obj 62912
          wait 2 s
          emote hands you the item, &1Here&2
          give key %actor.name%
        else
          * @YThis is the player does not meet our INT stat reqs but has the started var@w
          wait 2 s
          %echo% @WYou grab @RJ@Ca@Dy@Cn@De@w@W, a @RD@De@Wm@Do@Rn@w of @ML@Wu@Ms@Wt@w @wand give her a sloppy kiss!
          wait 2 s
          emote looks like she's going to vomit. &1That was a horrible kiss!&2 The sexy Demon shoos@c %actor.name%@n. &1Come back when you have the intelligence and experience to give me a better kiss!&2
          %send% %actor% @WMaybe you should try again when you have higher @RIntelligence@w@n.
          * @YThis ends the part where the character has the started variable but not the INT.@w@n
        end
        * @YEnds the check for if the variable exists.@w@n
        * @YNow we're on the part where the correct command was used but not on the right/full argument@w@n
      end
      * @YIs the end for checking if the actor has the started var@n
    else 
      wait 2 s 
      emote puts her hands up to stop %actor.name%.
      wait 2 s
      say Hey! What the fuck are you trying to do!
    end
    * @YIs the end for whether the player has the variable at all@n
  else
    wait 1 s
    %send% %actor% @WGood. You typed @Gsmooch %cmd%@W but you didn't type @Gsmooch Jayne@W.@n
    * @CHaving some issues with this particular one. The one trigger works perfectly fine but it's not working in the same manner as @Rtrig 62908@n
  end
  * @YIs the end for the @G%arg%@Y if.@w@n   
else
  wait 1 s
  %send% %actor% @WYes, but WHO are you trying to smooch?@n
end
* @YIs the end for the entire trigger.@w@n
~
#62913
Levay Accepting Jayne's Key~
0 j 100
~
if (%actor.varexists(evil_cavern_q1)%)
  if (%actor.evil_cavern_q1% == Started)
    if (%object.vnum% != 62912)
      say What the Hell am I suppose to do with that, maggot?!
      return 0
    else
      wait 1 s
      emote holds the key up, looking at it with a satisfied look.
      wait 1 s
      say Well you've proven that you can listen to instructions.
      wait 3 s
      smote The key starts to glow a golden-red color as   heats it with his hand. The Demon then takes out another portion of the broken key fuses them together.
      wait 2 s
      %load% obj 62913
      say This should allow you open the door that Alistar was guarding.
      give key %actor.name%
    end
    * @CThis now ends IF that deals with the obj.vnum@w@n
  end
  * @CThis now ends IF that deals with the Started@w@n
end
* @CThis now ends IF that deals with the Varexist@w@n
set evil_cavern_q1 Nearly_Done
remote evil_cavern_q1 %actor.id%
~
#62914
Alistar's Key Response~
0 d 100
key gate~
set evil_cavern_q1 Not_Started
if (%actor.varexists(evil_cavern_q1)%)
  set evil_cavern_q1 %actor.evil_cavern_q1%
end
if (%evil_cavern_q1% == Nearly_Done)
  wait 1 s
  say I see that you got your own key to this gate
  wait 2 s
  emote shrugs, &1If you asked me nicely I would have opened it for you myself&2
  wait 2 s
  say in any case, we're close to breaking the seal. All you have to do now is go and kill one of the Holy Order's leaders
  wait 2 s
  emote pulls out a crumbled piece of paper and starts reading it. &1From what it says here, the swine that should have The Seal to releasing the first Horseman should be a general on the second floor.&2
  wait 2
  emote nods his head to the gate. &1Well get to it then! Get the seal and then bring it give it to Levay&2
elseif (%actor.evil_cavern_q1% == Started)
  wait 1 s
  say If you want to gain access to the gate behind me, you should go speak with Levay. I have strict orders to guard this barricade
elseif (%actor.evil_cavern_q1% == Done)
  wait 2 s
  say if you lost or your key or something and you want another one, just give me ten thousand zenni and I'll give you my own one
else
  wait 2s
  say Hey, sugartits.  What your name is guuuurl?
  say %actor.evil_cavern_q1% <--- This is your variable
end
~
#62915
Levay Throne Sit (Load-Type)~
0 n 100
~
sit throne
~
#62916
Keru's Key Load~
0 f 100
~
if (%actor.varexists(evil_cavern_q1)%)
  if (%actor.evil_cavern_q1% == Nearly_Done)
    emote screams loudly just before he dies. &1Forgive me! I have failed to protect the Seal!&2
    %load% obj 62926
  end
end
~
#62917
Levay First Seal Response (Recieve)~
0 j 100
~
if (%actor.varexists(evil_cavern_q1)%)
  if (%actor.evil_cavern_q1% == Started)
    if (%object.vnum% != 62926)
      say What the Hell am I suppose to do with that, maggot?
    else
      wait 2 s
      Say I'm not sure where you got that thing but I will hold onto it for me because I don't trust a worthless maggot like yourself with such an important item.
    end
  elseif (%actor.evil_cavern_q1% == Nearly_Done)
    * The above elseif will fire if the player doesn't have the Started version of the cavern trigger.
    if (%object.vnum% != 62926)
      say And here I thought that you weren't an idiot. What am I suppose to do with that thing?
    else
      wait 2 s
      say Well done, the seal is now in our possession!
      wait 2 s
      emote reaches to the side of the Skull Throne and pulls out a Hoshijin skull. The Demon starts reading what has been written on the skull: &1Scouts say that the location where the Seal is to be broken is bewteen a jug and crab...&2
      wait 4 s
      say Wait! No!
      wait 4 s
      emote sharpens his eyes on the allegedly horrible writing. &1I'm not sure if this is suppose to be fish or crab...&2 
      wait 2 s
      echo The large Demon shrugs. &1Whatever then, a maggot like yourself has nothing better to do anyway. Go to this place and then Break the Seal&2
      wait 3 s
      Say If our reports are correct the Horseman should be released and we will be one step closer to starting the Void.
    end
  elseif (%actor.evil_cavern_q1% == Done)
    * The above elseif will fire if the player doesn't have the Started AND the N-Done version of the cavern trigger.
    if (%object.vnum% != 62926)
      say We have nothing else to discuss anymore. I don't want anything from you.
    else
      wait 3 s
      say Keep that Seal safe. You may need it to free the other Horsemen
    end
  end
else
  *  The above else is set to be on the same line as the var.exists. This is section is intended to be given to players who do not have have the variable and any of its forms.
  say you managed to get this on your own with your own information?
  wait 2 s
  say Well I'll just hold on to it for you since you got this key under questionable circumstances
end
~
#62918
Keru's 1st Trans (Hit-Per Type)~
0 k 100
~
if (%self.vnum% == 62915)
  @R* @Y Above should always check for the mob that we want it to be transformed into; not untransformed mob's vnum@n
  Say I'm going to go all out!
  wait 2 s
  %echo% @C%self.name%@W's face tenses, it becoming clear momentarily that they are deep in thought. After a brief lapse in focus, their attention seems to return to their surroundings.@n
  %echo% @WThough it's not apparent why they were so distracted, until he finally achieves @CMystic @GFirst@C!@n
  mtransform -62916
  detach 62918 %self.id%
end
~
#62919
Mystic Keru Attack (Load)~
0 n 100
~
%load% mob 62916
%purge% BaseKeru
%echoaround% @CKeru@W's face tenses, it becoming clear momentarily that they are deep in thought. After a brief lapse in focus, their attention seems to return to their surroundings.
%echoaround% Though it's not apparent why they were so distracted, until he finally achieves @CMystic @GFirst@W!@n
barrage %actor.name%
say There's no holding back now!
~
#62920
Break Seal (Command-Type)~
2 c 100
break~
if (%actor.varexists(evil_cavern_q1)%)
  if (%cmd% == break)
    if (%arg% == seal)
      if (%object.vnum% != 62926)
        if (%actor.evil_cavern_q1% == Nearly_Done)
          wait 1 s
          %send% %actor% @WAs you break the seal, the entire area quakes violently!@n
          %send% %actor% @WThe broken seal glows brightly!@n
          %echoaround% %actor% @WAs the seal is broken, the entire entire begins to quake violently!@n
          wait 3 s
          %zoneecho% %self.vnum% @YThe whole cavern undergoes a massive quake as The First Seal is broken!@n
          set actor.evil_cavern_q1 Done
          eval evil_cavern_q1 Done
          remote evil_cavern_q1 %actor.id%
          wait 2 s
          %send% %actor% @WThe seal magically begins repairing itself and as it does you are sucked into the space where the seal was broken.@n
          %echoaround% %actor% @C%actor.name%@W is sucked into the broken seal!@n
          %teleport% %actor% 62991
          wait 1 s
          %force% %actor% look
        elseif (%actor.evil_cavern_q1% == Done)
          wait 1 s
          %send% %actor% @WAs you break the seal, the entire area quakes violently!@n
          %send% %actor% @WThe broken seal glows brightly!@n
          %echoaround% %actor% @WAs the seal is broken, the entire area begins to quake violently!@n
          wait 1 s
          %zoneecho% %self.vnum% @YLoud applauds and cheers can be heard coming from everywhere as a sense of despair begins to set in...@n
          wait 3 s
          %send% %actor% @WOnce more, you are sucked into the seal!@n
          %teleport% %actor% 62991
        end
      end
    end
  end
elseif (%actor.varexists(good_cavern)%)
  if (%actor.good_cavern% == Finished)
    if (%cmd% == break)
      if (%arg% == seal)
        if (%object.vnum% != 62926)
          wait 1 s
          %send% %actor% @WAs you break the seal, the entire area quakes violently!@n
          %send% %actor% @WThe broken seal glows brightly!@n
          %echoaround% %actor% @WAs the seal is broken, the entire entire begins to quake violently!@n
          wait 1 s
          %zoneecho% %self.vnum% @YA sense of despair fills the area as The First Seal is broken...@n
          wait 3 s
          %send% %actor% @WOnce more, you are sucked into the seal!@n
          %teleport% %actor% 62991
        end
      end
    end
  end
~
#62921
Commander's Quest Message 1~
0 d 100
Assist~
set good_cavern
* @CHere we are creating the variable that will be used in this quest@n
if (%actor.varexists(good_cavern)%)
  eval good_cavern %good_cavern%
  * @CIf the actor has the variable already, set it to whatever@n
  * @Cthe actor's variable is set to (started, finished, etc)@n
  * @CBelow, we'll use this variable to determine response from the mob@n
else
  eval good_cavern Not_set
  * @CIf the var doesn't exist, set evil_cavern_q1 to "not_set"@n
end
* @CTake note:  Right off the bat, I download the variable from the@n
* @Cplayer to the variable we created on the first script line.@n
if (%good_cavern% == Not_set)
  * @CIf the player hasn't accepted the quest yet...@n
  if (%actor.race% == Demon)
    wait 1 s
    %send% %actor% @WYou are unable to speak for some odd reason!@n
  else
    wait 2 s
    say Many thanks to you, sir. Now let me start explaining...
    wait 2 s
    say We believe that the the Demons here are trying to free the First Horseman of the Void, Pestilence.
    wait 2 s
    say It is important that we prevent this because if the Demons are successful then it means that will be able to bring about the Void!
    wait 4 s
    %force% Kopernukis emote raises an eyebrow. &1A bit overdramatic aren't you?&2 The Hoshijin shakes his head. &1The Void will only come about if all four of them are freed&2
    wait 5 s
    smote &1Four? Is that what you th---&2 The Commander shakes his head. &1It's not important. If we stop them here with the first Horseman then they won't be able to free the others&2
    wait 5 s
    say Go and speak with the lovely Amby, you can't miss her because she's covered in her Holy Order armor.
    eval good_cavern Started
    remote good_cavern %actor.id%
  end
  * @CThis ends the IF Demon@n
elseif (%good_cavern% == Started)
  wait 2 s
  Say I am really sorry but it seems that I told you earlier to go and speak with Amby
  wait 2 s
  emote scratches his head. &1I need to retire soon...&2
elseif (%good_cavern% == Going)
  wait 2 s
  Say Oh my bad, it's you. According to what I've heard, you're suppose to be trying to expel that whorish Demon and the other Demon who is blocking access to the lower floor
  wait 3 s
  emote scratches his head. &1I need to retire soon. I can't even remember people's faces anymore&2
elseif (%good_cavern% == Keru)
  wait 2 s
  Say Go to Keru and get the seal from him
elseif (%good_cavern% == Quest_Reward)
  wait 2 s
  say what do you want to be rewarded with?
elseif (%good_cavern% == Finished)
  wait 2 s
  Say You have already helped us. The best way to help us now would be to go and prevent the next Horseman from being freed. 
else
  say @RHey, you've broken this quest somehow. Please inform an IMM to take a look at this quest.@n
end
~
#62922
Ambriella's Telepathy Q-MSG #1~
0 g 100
~
set good_cavern
if (%actor.varexists(good_cavern)%)
  if (%actor.good_cavern% == Started)
    if (%actor.room.vnum% == %self.room.vnum%)
      wait 1 s
      %send% %actor% Ambriella starts speaking with you telepathically...
      wait 1 s
      %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CGreetings I've been expecting you. As you already know the Demons here are plotting something big...@w'@n
      wait 2 s
      %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CIf we can purge their leaders back to Hell then we can put their plans back@w'@n
      wait 3 s
      %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CI've learnt that their major leaders are Demons called Jayne, Levay and Alistar...@w'@n
      wait 3 s
      emote frowns.
      wait 1 s
      %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CWe can't find Levay anyway but we know that Jayne is located somewhere to the northeast of this cavern and Alistar is guarding the door...@w'@n
      wait 3 s
      %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CI will recite an ancient incantation to allow you to be able to temporarily purge Jayne, Alistar and Levay... If you find him@w'@n
      wait 2 s
      %echoaround% %actor% @CA@Ymb@wrie@Ylla@n places a hand on @c%actor.name%@n's forehead and a rune burns onto @c%actor.hisher%@n skull.
      %send% %actor% @CA@Ymb@wrie@Ylla@n places a hand on your head and forehead a rune burns into your skull.
      eval good_cavern Going
      remote good_cavern %actor.id%
      wait 2 s
      %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CThere now go. It is important that you return with at least two Demon horns, you will get said horns after you @Gexpel@C two of the three Demons I named@w'@n
      wait 3 s
      emote nods at %actor.name%. &1Now go&2
    end
  elseif (%actor.good_cavern% == Going)
    wait 1 s
    %send% %actor% @cAmbriella@W talks to you telepathically,@w '@COh please tell me that you managed to send the Demons back to Hell!@w'@n
  end
elseif (%actor.good_cavern% == Keru)
  wait 1 s
  say You should go speak with Keru. He should be on the lower floor. Be careful though, the Demons down there have been affected by the Horseman
end
~
#62923
Necklace Dabura-Only~
1 j 100
~
if (%actor.class% == Dabura)
  wait 1 s
  %send% %actor% @WYour head is suddenly filled with the sounds of a thousand souls wailing in pain!@n
  %echoaround% %actor% @c%actor%@W quickly shuts their ears as though something loud and unwelcomed filled %char.hisher% ears.@n
else
  wait 1 s
  return 0
end
~
#62924
Alistar's Expel Trigger~
0 c 100
Expel~
if (%actor.varexists(good_cavern)%)
  if (%actor.good_cavern% == Going)
    if (%cmd% == expel)
      wait 1 s
      %send% %actor% @WYour mind suddenly pains as an ocean of knowledge fills your mind!@n
      wait 1 s
      %send% %actor% @WYou begin reciting an incantation whose words you don't even understand!@n
      %echoaround% %actor% @W%Actor% starts speaking an ancient dialect...@n
      wait 2 s
      %echo% The words seem to start having an effect...
      wait 1 s
      %echo% @RA@Cl@Di@Cs@Dt@Ca@Dr@W screams loudly as he bursts into @Gg@gr@Ge@ge@Gn@W flames and is forcefully sent back to Hell!@n 
      %load% obj 62928
      %load% obj 62913
      nop drop Horn
      nop drop Key
      set actor.good_cavern
      eval good_cavern Alistar
      remote good_cavern %actor.id%
      %purge% Alistar
    end
  end
end
~
#62925
Jayne's Expel Trigger~
0 c 100
expel~
if (%actor.varexists(good_cavern)%)
  if (%actor.good_cavern% == Alistar)
    if (%cmd% == expel)
      wait 1 s
      %send% %actor% @WYour mind suddenly pains as an ocean of knowledge fills your mind!@n
      wait 1 s
      %send% %actor% @WYou begin reciting an incantation whose words you don't even understand!@n
      %echoaround% %actor% @W%Actor% starts speaking an ancient dialect...@n
      wait 2 s
      %echo% The words seem to start having an effect...
      wait 1 s
      %echo% @RJ@Ca@Dy@Cn@De@W screams loudly as she bursts into @Gg@gr@Ge@ge@Gn@W flames and is forcefully sent back to Hell!@n 
      %send% %actor% @cAmbriella@W talks to you telepathically, @w'@CGood job! Now you should go speak with Keru. He is on the second. Be careful though, the Demons there have been affected by the Horseman@w'@n
      %load% obj 62927
      nop drop Horn
      set actor.good_cavern
      eval good_cavern Keru
      remote good_cavern %actor.id%
      %purge% Jayne
    end
  elseif (%actor.good_cavern% == Going)
    %send% %actor% @WYou should go expel Alistar first...@n
  end
end
~
#62926
Ambriella's Request for Seal Q-MSG 2~
0 j 100
~
if (%actor.varexists(good_cavern)%)
  if (%actor.good_cavern% == Going)
    if (%actor.room.vnum% == %self.room.vnum%)
      if (%object.vnum% != 62928)
        if (%object.vnum% != 62927)
          wait 1 s
          %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CThis is great! From the looks of these horns, they must have come from that demonic whore and the larger one calld Alistar...@w'@n
          wait 3 s
          %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CNow this is good. The Demons should struggle with sending and recieving orders now that those two are back where they belong@w'@n
          wait 3 s
          %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CWe'll use this moment to our advantage. Now you need to go to Keru and ask him for the first...@w'@n
          wait 2 s
          emote puts a finger to her mouth, indicating for silence.
          wait 2 s
          emote looks around the passageway and then eventually shrugs as she realizes everything is safe. The over-armored Kaio then turns to %actor.name%.
          wait 3 s
          %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CGo to him and ask for the first seal. Once you have the first seal, come back and bring it to me.@w'@n
        end
      end
    end
  end
end
~
#62927
Ambriella's Go get Jayne's Horn Q-MSG~
0 j 100
~
if (%object.vnum% != 62928)
  %send% %actor% @cAmbriella@W talks to you telepathically,@w '@CThis is great! Now you just need to bring me Jayne's horn!@w'@n
else
  return 0
  say this isn't what I asked for
end
~
#62928
Keru's "Kill Demons"~
0 g 100
~
if (%actor.varexists(good_cavern)%)
  if (%actor.good_cavern% == Keru)
    if (%actor.room.vnum% == %self.room.vnum%)
      wait 2 s
      %echoaround% %actor% @CK@Ye@wr@Yu@n shouts '@CI will van---@n' at @c%actor.name%@n and then abruptly stops when he realizes that %char.heshe% is not a Demon. 
      %send% %actor% @CK@Ye@wr@Yu@n shouts '@CDemon! I will van---@n' at you and then abruptly stops when he realizes that you are not a Demon. 
      wait 2 s
      say Oh my apologies, I thought that you were a Demon
      wait 3 s
      %send% %actor% @CK@Ye@wr@Yu@n looks at you. '@CI suppose that you must be the one that Ambriella sent...@n' The Kai shuts his eyes and seems to be in deep thought.
      %echoaround% %actor% @CK@Ye@wr@Yu@n looks at @c%actor.name%@n. '@CI suppose that you must be the one that Ambriella sent...@n' The Kai shuts his eyes and seems to be in deep thought.
      wait 4 s
      %send% %actor% @cKeru@W talks to you telepathically, @w'@CWell I can't just give out the First Seal...@w'@n
      wait 3 s
      %send% %actor% @cKeru@W talks to you telepathically, @w'@CI need you to help us out here with the Demons...@w'@n
      wait 4 s
      %send% %actor% @cKeru@W talks to you telepathically, @w'@CBecause of the war happening on Aether, we haven't been the biggest priority for most Kais and the Heavenly Kaios...@w'@n
      wait 5 s
      %send% %actor% @cKeru@W talks to you telepathically, @w'@CSo if you can help us out here by killing ten or so Demons then I'll gladly give you the Seal@w'@n
      set demon_kills
      eval demon_kills 0
      remote demon_kills %actor.id%
      wait 5 s
      %send% %actor% @cKeru@W talks to you telepathically, @w'@CGo and kill the Demons I tasked you with killing@w'@n
      set good_cavern
      eval good_cavern Task
      remote good_cavern %actor.id%
    end
  elseif (%actor.good_cavern% == Task)
    wait 1 s
    %send% %actor% @cKeru@W talks to you telepathically, @w'@CAh! Welcome back %actor.name%, if you've killed the Demons I asked you to kill just inform me that you have @Gcomplete@Cd my task@w'@n
  end
elseif (%actor.race% == Demon)
  wait 1 s
  Say Die you filthy Demon!
  barrage %actor%
end
~
#62929
Demon Kills Count~
0 f 100
~
set demon_kills
if (%actor.varexists(demon_kills)%)
  if (%actor.demon_kills% == 0)
    eval demon_kills 1
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y1@W Demon in total.@n
  elseif (%actor.demon_kills% == 1)
    eval demon_kills 2
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y2@W Demons in total.@n
  elseif (%actor.demon_kills% == 2)
    eval demon_kills 3
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y3@W Demons in total.@n
  elseif (%actor.demon_kills% == 3)
    eval demon_kills 4
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y4@W Demons in total.@n
  elseif (%actor.demon_kills% == 4)
    eval demon_kills 5
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y5@W Demons in total.@n
  elseif (%actor.demon_kills% == 5)
    eval demon_kills 6
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y6@W Demons in total.@n
  elseif (%actor.demon_kills% == 6)
    eval demon_kills 7
    remote demon_kills %actor.id%
    %send% %actor% @WYou have killed @Y7@W Demons in total.@n
  elseif (%actor.demon_kills% == 7)
    eval demon_kills 8
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y8@W Demons in total.@n
  elseif (%actor.demon_kills% == 8)
    eval demon_kills 9
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y9@W Demons in total.@n
  elseif (%actor.demon_kills% == 9)
    eval demon_kills 10
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y10@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 10)
    eval demon_kills 11
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y11@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 11)
    eval demon_kills 12
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y12@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 12)
    eval demon_kills 13
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y13@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 13)
    eval demon_kills 14
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y14@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 14)
    eval demon_kills 15
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y15@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 15)
    eval demon_kills 16
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y16@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 16)
    eval demon_kills 17
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y17@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 17)
    eval demon_kills 18
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y18@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 18)
    eval demon_kills 19
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y19@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 19)
    eval demon_kills 20
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y20@W Demons in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == 20)
    eval demon_kills Special
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed numerous Demons...@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%actor.demon_kills% == Special)
    %send% %actor% @WYou have killed numerous Demons...@n
    %send% %actor% @CYou have killed enough Demons.@n
  end
end
~
#62930
Keru's "Thanks for killing Demons"~
0 d 100
Complete~
set good_cavern
if (%actor.varexists(good_cavern)%)
  if (%actor.good_cavern% == Task)
    if (%actor.room.vnum% == %self.room.vnum%)
      if (%actor.demon_kills% == 11)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 12)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 13)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 14)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 15)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 16)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        wait 2 s
        %load% obj 62926
        give seal %actor.name%
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 17)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 18)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 19)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == 20)
        wait 1 s
        say Excellent, I am very grateful that you killed the Demons. Now I'll hold up my end of our agreement.
        %load% obj 62926
        give seal %actor.name%
        wait 2 s
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        wait 2 s
        %send% %actor% %self.name% bows respectfully towards you.
        %echoaround% %actor% %self.name% bows respectfully towards %actor.name%.
        eval good_cavern Seal
        remote good_cavern %actor.id%
        rdelete demon_kills %actor.id%
      elseif (%actor.demon_kills% == Special)
        wait 1 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CAmazing! You not only killed more than the ten Demons I asked you to kill but you killed far more than that!@w'@n
        wait 3 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CBecause you went above and beyond in our agreement then it is only fair that I reward your devotion...@n'
        wait 3 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CIn the we've been here on Zenith, we've managed to learn a lot. We've managed to alter the Hoshijins' Topazs@w'@n
        wait 3 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CThe original Topazs would teleport the Hoshijins to several different planets...@w'@n
        wait 3 s
        emote clears his throat and smiles.
        wait 4 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CNow however we've managed to make them work solely on Hell's Demons.@w'@n
        wait 3 s
        emote reaches into his pocket and takes out an @CE@cx@Wo@wd@cu@Cs @YT@yo@Yp@ya@Yz@n.
        wait 4 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CThis Exodus Topaz will expel Hell's Demons and send them back to where they came from: Hell@w'@n
        wait 3 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CAs I personally believe that the Demon scum should be killed not exorcised back Hell...@w'@n
        wait 2 s
        emote shrugs.
        wait 2 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CWell it is what it is. So you can have my Exodus Topaz since I'm not going to ever use it@w'@n
        %load% obj 62930
        give topaz %actor.name%
        rdelete demon_kills %actor.id%
        wait 1 s
        %send% %actor% @cKeru@W talks to you telepathically, @w'@CAnd of course let's not forget the First Seal...@w'@n
        %load% obj 62926
        give seal %actor.name%
        say Now you should go and hand this over to Ambriella, she'll tell you what to do from there.
        eval good_cavern Seal
        remote good_cavern %actor.id%
      end
    end
  end
end
~
#62931
Good Cavern Completion Trigger~
0 j 100
~
if (%actor.varexists(good_cavern)%)
  if (%actor.good_cavern% == Seal)
    if (%object.vnum% == 62926)
      wait 2 s
      %send% %actor% %self.name% smiles warmly at you.
      %echoaround% %actor% %self.name% smiles warmly at %actor%.
      wait 2 s
      say This is brilliant. Now that the Seal isn't on the second floor, where the Demons severly outnumbered us, it is much safer now.
      wait 3 s
      emote bows respectfully.
      wait 1 s
      Say For now, you should hold on to it. You will need the First Seal to access the other Horsemen.
      give seal %actor.name%
      eval good_cavern Finished
      remote good_cavern %actor.id%
      wait 2 s
      say And as a reward, you can have this...
      %load% obj 62931
      give bracer %actor.name%
    else 
      wait 1 s
      Say I have no interest what you have given me
    end
  end
end
~
#62932
Summon Slug Golem~
1 c 100
touch~
if (%actor.varexists(hoshi_cavern)%)
  if (%actor.hoshi_cavern% == Slug)
    if (%cmd% == touch)
      if (%arg% == slug)
        wait 1 s
        %send% %actor% @WYou place your hand on the statue...@n
        %echoaround% %actor% @C%actor.name%@W places %actor.hisher% hand on the statue...@n
        wait 1 s
        %echo% @WThe statue rumbles loudly!
        wait 1 s
        %load% mob 62920
        %echo% @WThe statue's defender appears!@n
      end
    end
  end
else
  if (%cmd% == touch)
    if (%arg% == slug)
      wait 1 s
      %send% %actor% @WNothing happens@n
      %echoaround% %actor% @C%actor.name%@W places %actor.hisher% hand on the statue...@n
    end
  end
end
~
#62933
Hoshi_Cavern Start Quest~
0 g 100
~
set hoshi_cavern
if (%actor.race% == Hoshijin)
  if (%actor.room.vnum% == %self.room.vnum%)
    if (%actor.hoshi_cavern% == Owl)
      wait 1 s
      say You must go and defeat the Champion of Enlightenment
    elseif (%actor.hoshi_cavern% == Eagle)
      wait 1 s
      say You must go and defeat the Champion of Servitude
    elseif (%actor.hoshi_cavern% == Slug)
      say You need to go and defeat the first champion. The champion of Nudity.
    elseif (%actor.hoshi_cavern% == Reward)
      wait 1 s
      Say Congratulations! You have defeated all the Northern Champions. You are an honorary Astromancer.
      wait 2 s
      say As a reward for defeating the Northern Champions I give you this give.
      %load% obj 62934
      give robe %actor.name%
      wait 2 s
      say This is usually given to Astromancers who have obtained the level of Neutron Astromancer but because of the conflict going on here you will not be able reach that level.
      wait 3 s
      emote looks a bit sad. &1So you can have that, make sure to come back and see if the conflict has ended. If it has then you make attempt to become a Celestial Astromancer&2
      wait 2 s
      %send% %actor% @GKepler@n smiles at you. '@CWho knows, you may even become a High Astromancer such as myself someday. Farewell for now.@w'@n
      %echoaround% %actor% @GKepler@n smiles at %actor.name%. '@CWho knows, you may even become a High Astromancer such as myself someday. Farewell for now.@w'@n
      eval hoshi_cavern Astromancer
      remote hoshi_cavern %actor.id%
    elseif (%actor.hoshi_cavern% == Astromancer)
      wait 1 s
      say Unfortunately, the conflict has not yet ended so you can't undergo further Astromancy training.
    else
      wait 1 s
      %send% %actor% @GKepler@n, a @YH@yigh @BA@Yst@Wr@wo@Bm@Yan@Wc@we@Yr@n smiles warmly at you.
      %echoaround% %actor% @GKepler@n, a @YH@yigh @BA@Yst@Wr@wo@Bm@Yan@Wc@we@Yr@n smiles warmly at %actor.name%.
      wait 1 s
      %send% %actor% @cKepler @Wtalks to you telepathically, @w'@CAh greetings, fellow Hoshijin!@w'@n
      wait 2 s
      %send% %actor% @cKepler @Wtalks to you telepathically, @w'@CI see that you've come to undergo Astromancer training@w'@n
      wait 2 s
      %send% %actor% @cKepler @Wtalks to you telepathically, @w'@CThis is unfortunately a bad time to come here because the Kaios and Makaios seem to have decided to turn this location into their own personal battlefield@w'@n
      wait 2 s
      emote looks upset about something.
      wait 2 s
      %send% %actor% @cKepler @Wtalks to you telepathically, @w'@CThey have the whole of Aether to themselves and now they want to bring their conflict here to Zenith@w'@n
      wait 3 s
      emote shakes his head and sighs loudly.
      wait 1 s
      %send% %actor% @cKepler @Wtalks to you telepathically, @w'@CEnough about that though. You can still become an Astromancer however you won't undergo full training like those that came before.@w'@n
      wait 3 s
      %send% %actor% @cKepler @Wtalks to you telepathically, @w'@CThe second floor is over run and I'm given to believe that they destroyed The Virgin's statue.@w'@n
      wait 1 s
      %send% %actor% @cKepler @Wtalks to you telepathically, @w'@CHalf of this level is overrun with mutated creatures.@w'@n
      wait 2 s
      %send% %actor% @cKepler @Wtalks to you telepathically, @w'@CSo all you have to do is go and defeat the champions of Nudity, Servitude and Enlightenment.@w'@n
      wait 2 s
      %send% %actor% Kepler nods at you.
      %echoaround% %actor% Kepler nods at %actor.name%.
      eval hoshi_cavern Slug
      remote hoshi_cavern %actor.id%
    end
  end
end
~
#62934
Slug/Limax Death Trigger~
0 f 100
~
if (%actor.varexists(hoshi_cavern)%)
  if (%actor.hoshi_cavern% == Slug)
    %send% %actor% @WYou have defeated the Champion of Nudity!@n
    %load% obj 62932
    give shinguards %actor.name%
    eval hoshi_cavern Eagle
    remote hoshi_cavern %actor.id%
  end
end
~
#62935
Summon Eagle Golem~
1 c 100
Touch~
if (%actor.varexists(hoshi_cavern)%)
  if (%actor.hoshi_cavern% == Eagle)
    if (%cmd% == touch)
      if (%arg% == eagle)
        wait 1 s
        %send% %actor% @WYou place your hand on the statue...@n
        %echoaround% %actor% @C%actor.name%@W places %actor.hisher% hand on the statue...@n
        wait 1 s
        %echo% @WThe statue rumbles loudly!
        wait 1 s
        %load% mob 62921
        %echo% @WThe statue's defender appears!@n
      end
    end
  end
else
  if (%cmd% == touch)
    if (%arg% == slug)
      wait 1 s
      %send% %actor% @WNothing happens.@n
      %echoaround% %actor% @C%actor.name%@W places %actor.hisher% hand on the statue...@n
    end
  end
end
~
#62936
Aquila/Eagle Death trigger~
0 f 100
~
if (%actor.varexists(hoshi_cavern)%)
  if (%actor.hoshi_cavern% == Eagle)
    %send% %actor% @WYou have defeated the Champion of Servitude!@n
    %load% obj 62933
    eval hoshi_cavern Owl
    remote hoshi_cavern %actor.id%
  end
end
~
#62937
Eagle/Aquila Death trigger~
0 f 100
~
if (%actor.varexists(hoshi_cavern)%)
  if (%actor.hoshi_cavern% == Owl)
    %send% %actor% @WYou have defeated the Champion of Enlightenment!@n
    %load% obj 62935
    eval hoshi_cavern Reward
    remote hoshi_cavern %actor.id%
  end
end
~
#62938
Steed's Horseshoe Load~
0 n 20
~
%load% obj 62936
~
#62939
Quiver's Poisonious Aura~
1 b 1
~
* @YFirst, wait 1 to do damage after room enter.@n
wait 1 s
* @YNow, create a variable that is the same as what@n
* @Ythe character has in his hand slot.@n
eval thisitem %actor.eq(9)%
* @YIf player's hand item is vnum 62901@n
if (%actor.race% == Android)
  * @YDon't do anything@n
elseif (%thisitem.vnum% == 62901)
  * @YDon't do anything@n
  * @YElse, if it isn't vnum 62901@n
else
  * @YMake a variable that is the char's current powerlevel / 100@n
  eval mydamage %actor.hitp% / 100
  * @YDamage the player by the variable (one percent of powerlevel)@n
  %damage% %actor% %mydamage%
  %send% %actor% @WYou feel your body fail you as you succumb to illness@n
  * @YEnd the if/else statement.@n
end
* @YOriginal script(@G3304@G Testport@Y) by @RSedalis@n
~
#62940
Underground Cavern Posion~
2 g 100
~
* @YFirst, wait 1 to do damage after room enter.@n
wait 1 s
* @YNow, create a variable that is the same as what@n
* @Ythe character has in his hand slot.@n
eval thisitem %actor.eq(9)%
* @YIf player's hand item is vnum 62901@n
if (%actor.race% == Android)
  * @YDon't do anything@n
elseif (%thisitem.vnum% == 62901)
  * @YDon't do anything@n
  * @YElse, if it isn't vnum 62901@n
else
  * @YMake a variable that is the char's current powerlevel / 100@n
  eval loss 500 + %500000.random% 
  * @YDamage the player by the variable (one percent of powerlevel)@n
  %damage% %actor% %loss%
  %send% %actor% @WYou feel your body fail you as you succumb to illness.@n
  * @YEnd the if/else statement.@n
end
* @YOriginal script(@G3304@G Testport@Y) by @RSedalis@n
~
#62941
Wormhole Enter trigger~
2 c 100
Enter~
if (%cmd% == Enter)
  if (%arg% == wormhole)
    wait 1 s
    %send% %actor% @CYou step through the @Rwormhole@C.@n
    %echoaround% %actor% @M%actor.name%@C steps through the @Rwormhole@C.@n
    %teleport% %actor% 62988
    %force% %actor% look
  end
end
~
#62942
Corpse-Gauntlet Trigger~
0 n 100
~
%purge% flesh
%load% obj 62901
%load% obj 62900
drop flesh
put gauntlet flesh
%purge% Inviz
~
#62943
Steed's Quiver (Load)~
0 n 10
~
%load% obj 62936
~
#62989
Anti-Forge trigger~
1 c 2
Forge~
if (%cmd% == forge || forg ||)
  if (%arg% == Exodus || Exodu || Exod || Exo || Ex || Topaz || Topa || Top || To || Gemstone || Gemston || Gemsto || Gemst || Gems || Gem || 62930 || Ge)
    wait 1 s
    %send% %actor% @WYou try to tamper with the @YT@yo@Yp@ya@Yz@W but it explodes violently infront of you!@n
    %echoaround% %actor% @C%actor.name%@W tries to tamper with the @YT@yo@Yp@ya@Yz@W but it explodes violently infront of %actor.himher%!@n
    mkill %actor%
  else 
    return 1
    * @YReturn 1 means the MUD will allow the player to forge other items@n
  end
end
~
#62990
Demon Kill Count #2~
0 f 100
~
if (%actor.varexists(demon_kills)%)
  eval demon_kills %actor.demon_kills%
  if (%demon_kills% >= 20)
    eval demon_kills Special
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed numerous Demons...@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%demon_kills% >= 10)
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed@Y %demon_kills% @Wdemon in total.@n
    %send% %actor% @CYou have killed enough Demons.@n
  elseif (%demon_kills% >= 1)
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed@Y %demon_kills% @Wdemons in total.@n
  else
    remote demon_kills %actor.id% 
    %send% %actor% @WYou have killed @Y1@W demon in total.@n
  end
end
~
#62991
Exodus Topaz Expel Trigger~
1 c 2
expel~
if (%cmd% == expel)
  set room_var %actor.room% @R
  * @YThe below sends a message when the Topaz is activated.@n
  wait 1 s
  %send% %actor% @cYou@C raise your arm up into the air with the @CE@cx@Wo@wd@cu@Cs @YT@yo@Yp@ya@Yz@C held tightly. The @YT@yo@Yp@ya@Yz@C explodes violently and a @Yb@Wl@Yi@Wn@Yd@Wi@Yn@Wg@C light fills the entire room!@n
  %echoaround% %actor% @R%actor.name%@C raises %actor.hisher% arm up into the air with their E@cx@Wo@Dd@cu@Cs @YT@yo@Yp@ya@Yz@C held tightly. The @YT@yo@Yp@ya@Yz@C explodes violently and a @Yb@Wl@Yi@Wn@Yd@Wi@Yn@Wg@C light fills the entire room!@n
  set target_char %room_var.people%
  while %target_char%
    set tmp_target %target_char.next_in_room%
    if (%target_char.class% == dabura)
      %send% %target_char% @WThe light tears through your body and soul!@n
      %send% %target_char% @WYou feel your body and soul being pulled apart as you are violently expelt back to the @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@W!@n
      %echoaround% %target_char% @WA burn mark is left where @C%target_char.name%@W was!@n
      %teleport% %target_char% 6040
    else
      %send% %target_char% @WThe bright light burns your eyes!@n
      nop %force% %target_char% closeeyes
    end
    set target_char %tmp_target%
  done
  %purge% 62930
end
~
#62992
@MDynamite Detonate Trigger 2@n~
1 c 2
Detonate~
if (%cmd% == detonate)
  set room_var %actor.room%
  wait 3 s
  %echo% @WThe grendade blasts into smithereens, striking everyone here.@n
  set target_char %room_var.people%
  set tmp_target %target_char.next_in_room%
  while %target_char%
    if (%target.sex% == male)
      %send% %target_char% @WThe explosion hurts you!@n
      %damage% %target_char% 30
      set target_char %tmp_target%
    else
      %send %target% @WYou see a large explosion in the corner of your eye@n
    done
  end
end
%purge% dynamite
~
#62993
@GDynamite Detonate Trigger@n~
1 c 100
~
if (%cmd% == detonate)
  set room_var %actor.room%
  @R* @CSends a message when the bomb goes off.@n
  wait 3 s
  %echo% @WThe grendade blasts into smithereens, striking everyone here.@n
  @R* @CTarget the first char@n
  set target_char %room_var.people%
  @R* @CDo stuff@n
  while %target_char%
    @R* @CSet the next target before this one perhaps dies@n
    set tmp_target %target_char.next_in_room%
    @R* @CThis is where the good/bad things are suppose to happen.@n
    %send% %target_char% @WThe explosion hurts you!@n
    %damage% %target_char% 30
    @R* @CFind next target
    set target_char %tmp_target%
    @R* @CLoop back.@n
  done
end
~
#62994
Good_Cavern @R(TP ONLY)@n~
2 c 100
Control~
if (%arg% == Started)
  eval good_cavern Started
  remote good_cavern %actor.id%
elseif (%arg% == Going)
  eval good_cavern Going
  remote good_cavern %actor.id%
elseif (%arg% == Keru)
  eval good_cavern Keru
  remote good_cavern %actor.id%
elseif (%arg% == Task)
  eval good_cavern Task
  remote good_cavern %actor.id%
elseif (%arg% == Alistar)
  eval good_cavern Alistar
  remote good_cavern %actor.id%
elseif (%arg% == Seal)
  eval good_cavern Seal
  remote good_cavern %actor.id%
elseif (%arg% == Finished)
  eval good_cavern Finished
  remote good_cavern %actor.id%
end
~
#62995
Evil_Cavern @R(TP ONLY)@w~
2 c 100
clear~
if %cmd% == clear
  if %arg% == evil
    rdelete evil_cavern_q1 %actor.id%
  end
end
~
#62996
Get @CNearly_Done@n Trigger~
2 c 100
Nearly_Done~
if (%arg% == Nearly_Done)
  set evil_cavern_q1 Nearly_Done
  remote evil_cavern_q1 %actor.id%
end
~
#62997
Experiment~
0 d 100
~
set evil_cavern_q1
* @cHere we are creating the variable that will be used in this quest@n  
if (%actor.align% < -300)
  * @cHere I am checking to see if the actor's alignment is at 300 or higher(evil)@n
  if (%actor.varexists(evil_cavern_q1)%)
    * @c Because the actor already has this variable on them we show them that the mob is growing impatient@n
    %echo% @YThe Mob wants you to hurry up!@n
  else
    * @cThe actor is evil but the (evil_cavern_q1) is NOT put on them now. So we put it on them with the remote command@n
    %echo% @YThis is where the mob thanks the player for accepting the quest@n
    remote evil_cavern_q1 %actor.id%
    * @c The actor now has the variable. When they return with the same command they will be informed the mob is impatient@n
  end
  * @cThe quest variable is over. In the above we dealt with the player already having the variable or getting the variable.@n
elseif (%actor.align% > -299)
  * @cBecause the player is not evil(but Neutral or Good), and the first IF was false we immediately jump here. Because the player's align is at 299 and below.@n
  %echo% @Y Here the mob will inform the player that their alignment is incorrect so they need to fuck off until they come back with the right alignment@n
end
* @cAnd with that everything is accounted for@n
~
#62998
Only Sexy Baldies~
0 g 100
~
set Razor 0
* @csets the variable I will be using. Not sure what the 0 is for but whatever@n
* @cBecause this is a "Speech" type trigger the actor will only have to say "accept" or anything with it - the activator for this trigger was determined in the "Arg list"@n
if (%actor.bald% == Bald)
  * @cWe are checking to see if the player is Bald.@n
  if  (%actor.sexy% == Sexy)
    * @c We are checking to see if the player is Bald AND Sexy
    if (%actor.varexists(Razor)%)
      * @cWe are checking to see if the actor(who said "accept") has the Razor variable on them
      eval Razor %actor.razor%
      * @c Not sure what's happening now, but I think this where we give the player the Razor variable but they only get th variable if they meet all our criteria(Bald/Sexy/Has variable)@n
      %echo% @YThis message will be displayed because the player has the variable on them so a message of the mob asking why they haven't finished the quest comes here@n
    else
      * @cThe actor meets our REQs(Bald and Sexy) but doesn't have the variable for the quest so we offer them our quest@n
      remote Razor %actor.id%@n
    end
    * @cwe put an end to us checking if the actor is bald, is sexy or has the variable@n
    * @c Probably somewhere after this we have another else ("%elseif%" maybe?) because the actor is not Bald and Sexy or just Bald@n
~
#62999
DemonSpeak~
2 c 100
examine~
if %actor.race% == Demon
  %send% %actor% Your head begin to throb as you examine the markings.
  %echoaround% %actor% %actor.name% walks up and examines the markings.
  wait 1 s
  %send% %actor% The markings begin to move into something coherent...
  wait 1 s
  %send% %actor% Master is hidden below the owl.
  %echoaround% %actor% %actor.name% steps back from the markings.
else
  %send% %actor% You can not make sense of the markings...
  %echoaround% %actor% %actor.name% walks up to the marking and shakes %actor.hisher% head.
end
~
$~
