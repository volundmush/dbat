#630
Wall trigger~
2 c 2
push~
if (%arg% == wall)
if (%self.south(bits)% != DOOR)
wait 1 sec
%echoaround% %actor% %actor.name% pushes against the wall and the wall slowly grinds open revealing a hidden area!
wdoor 630 2 flags ae
wdoor 653 0 flags ae
%send% %actor% You push on the wall and it slowly grinds open to reveal a hidden area!
else
%send% %actor% The wall is already pushed open...
end
else
wait 1 s
%send% %actor% Push what?
end
~
#631
Wall Trigger 2~
2 c 2
push~
if (%arg% == wall)
if (%self.north(bits)% != DOOR)
wait 1 sec
%echoaround% %actor% %actor.name% pushes against the wall and the wall slowly grinds open revealing a hidden area!
wdoor 653 0 flags ae
wdoor 630 2 flags ae
%send% %actor% You push on the wall and it slowly grinds open to reveal a hidden area!
else
%send% %actor% The wall is already pushed open...
end
else
wait 1 s
%send% %actor% Push what?
end
~
#657
Smithy echo~
2 ab 10
~
%echo% @RA loud hiss resounds throughout the area as the bellows generate a massive gust of air, raising the temperature as heat exudes from the forge.@n
~
#684
Black Belt Random~
0 b 15
~
wait 1
mecho A loud slam is heard as a fighter slams their foot on the floor where an opponent had been.
wait 4 sec
mecho A yell is heard as a blow lands.
wait 3 sec
say HYA!
~
#685
Seiji Greet~
0 g 100
~
wait 1
say Why are you here? You are not one of my students.
wait 3 sec
say I am not taking any more students right now.
wait 2 sec
mecho Seiji begins to meditate.
wait 1 sec
mecho The torches on either side of Seiji lift slowly from the ground and begin to slowly circle him.
~
#686
Krillin Quest White Belt~
0 f 100
~
if (%actor.varexists(onkq1)% || %actor.varexists(onTienQ1)%)
    %load% obj 686
elseif (%actor.varexists(onTenQ1)%)
    %load% obj 687
end
~
#689
Basic Instinct Trigger~
2 g 100
~
wait 30 sec
%echo% A BEDROOM - NIGHT
wait 6 sec
%echo% It is dark; we don't see clearly. It is a bedroom; a man and woman make love on a brass bed. There are mirrors on the walls and ceiling. On a side table, atop a small mirror, lines of cocaine. A tape deck plays the Stones "Sympathy for the Devil." She is atop him... she straddles his chest... her breasts in his face. He cups her breasts. She leans down, kisses him...
wait 8 sec
%echo% The man is in his late 40's, slim, good-looking. We don't see the woman's face. She has long blonde hair. She leans close over his face, her tongue in his mouth... she kisses him... she moves her hands up, holds both of his arms above his head.
wait 8 sec
%echo% She moves higher atop him... she reaches to the side of the bed... a white silk scarf is in her hand... her hips above his face now, moving... slightly, oh-so slightly... his face strains towards her. The scarf in her hand... she ties his hands with it... gently... to the brass bed... his eyes are closed... tighter... lowering hips into his face... lower... over his chest... his navel.
wait 8 sec
%echo% The SONG plays. He is inside her... his head arches back... his throat white. She arches her back... her hips grind... her breasts are high... Her back arches back... back... her head tilts back... she extends her arms...
wait 8 sec
%echo% Her right arm comes down suddenly... the steel flashes... a thin, steel-handled icepick, plunging into his breast. He bucks, writhes, bucks, convulses... It flashes again, and again, covered in blood.
wait 8 sec
%echo% A POLICE INTERROGATION ROOM - DAY
wait 6 sec
%echo% It is large, fluorescent-lighted, antiseptic. A beautiful girl with long, blonde hair, Catherine, walks in with Nick, a police officer. In the room are policemen, hard faced guys bent on squeezing her until the truth is out. There is a police stenographer, a plain young woman in her 20's. As soon as Catherine comes in, the cops introduce themselves. She declines an unspoken offer of coffee.
wait 8 sec
%echo% LT. WALKER: Why have you waived your right to an attorney, Ms. Tramell?
wait 2 sec
%echo% CATHERINE: Why did you think I wouldn't want one? I have nothing to hide.
wait 2 sec
%echo% The two of them keep their eyes on each other.
wait 2 sec
%echo% She sits down. They sit around her. Nick sits directly across from her. She lights up a cigarette. They watch her. She is poised, cool, in complete command of herself.
wait 5 sec
%echo% LT. WALKER: There is no smoking in this building, Ms. Tramell.
wait 2 sec
%echo% CATHERINE: What are you going to do? Charge me with smoking?
wait 2 sec
%echo% Ever so casually, she blows her smoke across at Nick. Her legs spread enough for the men to catch a glipse of her lack of underwear.
wait 5 sec
%echo% LT. WALKER: Would you tell us the nature of your relationship with Mr. Boz?
wait 2 sec
%echo% CATHERINE: I had sex with him for about a year and a half. I liked having sex with him. He wasn't afraid of experimenting. I like men like that. I like men who give me pleasure. He gave me a lot of pleasure.
wait 2 sec
%echo% A beat, as they watch her. She is so matter-of-fact. She has control of the room; she looks from one man to the other as she speaks.
wait 4 sec
%echo% NICK: You like playing games, don't you?
wait 2 sec
%echo% CATHERINE: I've got a degree in psych. It goes with the turf. Games are fun.
wait 2 sec
%echo% She smiles. They are holding each other's eyes.
wait 3 sec
%echo% CATHERINE: Would you like me to take a lie detector test?
wait 2 sec
%echo% INT. THE POLICE BUILDING - NIGHT
wait 6 sec
%echo% Catherine is in a glass-enclosed cubicle with a polygraph. The examiner shuts the machine down, gathers rolls of papers, and comes out of the cubicle to report to the gathered men.
wait 8 sec
%echo% THE EXAMINER: No blips, no blood pressure variations, no pulse variance. Either she's telling the truth or I've never met anyone like her.
wait 3 sec
%echo% NICK: How does you beat this machine?
wait 2 sec
%echo% THE EXAMINER: You don't. You'd have to be able to mask the truth from your own central nervous system, your circulatory system, your adrenal glands. In my opinion, this woman is telling the truth.
wait 3 sec
%echo% The Examiner walks away. They stand there. Catherine stands at the door of the cubicle behind them.
wait 3 sec
%echo% CATHERINE: Can I go now?
wait 2 sec
%echo% LT. WALKER: (after a beat) Yes. Thanks for coming in, Ms. Tramell. I'm sorry to inconvenience you.
wait 3 sec
%echo% She says nothing, has a thin smile.
wait 2 sec
%echo% CATHERINE: Can I ask one of you for a ride? They look at her a beat.
wait 3 sec
%echo% NICK: Sure.
wait 2 sec
%echo% CATHERINE: (smiles) Thanks.
wait 2 sec
%echo% And he and Catherine walk away. Gus and Walker watch them.
wait 3 sec
%echo% IN HIS CAR - NIGHT
wait 6 sec
%echo% It is an old, mint-condition silver Porsche. It is pouring rain; the wind is blowing a San Francisco winter storm. Nothing is said a long beat as he drives. She yawns. Stretches. He looks at her.
wait 5 sec
%echo% CATHERINE: I'm tired.
wait 3 sec
%echo% NICK: It's got to be tiring to beat that machine.
wait 3 sec
%echo% She looks at him and looks away. A beat.
wait 3 sec
%echo% CATHERINE: If I were guilty, and if I wanted to beat that machine, it wouldn't be tiring. It wouldn't be tiring at all.
wait 3 sec
%echo% NICK: Why not?
wait 2 sec
%echo% CATHERINE: Because I'm a professional liar. I spend most of my waking hours dwelling on my lies. (pause) For my writing.
wait 3 sec
%echo% He looks at her.
wait 2 sec
%echo% CATHERINE: (in a whisper) I love you.
wait 2 sec
%echo% They kiss.
wait 2 sec
%echo% A BEDROOM - NIGHT
wait 6 sec
%echo% Nick and Catherine make love on a bed. A tape deck plays the Stones "Sympathy for the Devil." The camera backs away from them slowly as they kiss feverishly, and we see them from the right side of the bedroom. And then the camera lowers slowly as their passion mounts. It keeps going lower. There is something under the bed.
wait 8 sec
%echo% The camera moves closer towards it as the music plays louder. We see it now in closeup as the bed rustles above... It is a thin, steel-handled icepick.
wait 5 sec
%echo% As a woman's hand reaches out towards it, the song plays louder and louder.
wait 4 sec
~
#690
Ticket Seller Greet~
0 g 100
~
wait 1
say In order to go any further you must 'buy ticket' which will cost 15 zenni.
wait 3 sec
say Here at Nexus Theater one ticket will allow you free access to all the movies being shown for one visit.
~
#691
Ticket Prevent~
0 q 100
~
if (%direction% == west || %direction% == south || %direction% == southwest)
if (%actor.has_item(696)%)
  %force% %actor.name% give movie seller
  %purge% movie
return 1
 else
return 0
wait 1
say You can not go further without a ticket.
 end
else
return 1
end
~
#692
Ticket Seller Ticket Sale~
0 c 100
buy~
if (%arg% == ticket && %actor.gold% > 14)
 wait 1
%echoaround% %actor% %actor.name% offers Ticket Seller some zenni.
%send% %actor% The Ticket Seller takes your fifteen zenni.
%actor.gold(-15)%
wait 1 sec
 say Here you go.
 %load% obj 696
 give movie %actor.name%
elseif (%arg% == ticket && %actor.gold% < 15)
wait 1
%echoaround% %actor% %actor.name% tries to buy a ticket.
say You do not have enough money, you need 15 zenni and only have %actor.gold%
else
wait 1
say What are you wanting to buy? Perhaps a 'ticket'?
smile
end
~
#693
Nightmare On Elm Stree 3 Random~
2 g 100
~
wait 30 sec
%echo% A blonde haired teen girl on screen cautiously walks through what appears to be a boiler room.
wait 5 sec
%echo% The blonde teen girl on screen turns her head from side to side fearfully.
wait 3 sec
%echo% An expectant murmur passes through the crowd of movie-patrons.
wait 6 sec
%echo% The blonde haired teenager turns a corner and sees who she apparently has been following.
wait 6 sec
%echo% The blonde teen approaches a little girl and turns her around.
wait 3 sec
%echo% "What are you doing in here?" The teenager asks the little girl.
wait 6 sec
%echo% Suddenly a loud noise is heard from upstairs and the teenager jumps at the noise.
wait 6 sec
%echo% "Freddy's home!" The little girl says as the teenager suddenly wakes up. It was just a dream.
wait 6 sec
%echo% The crowd of movie-patrons breath a sigh of relief collectivly.
wait 13 sec
%echo% Errie music plays as the movie transitions to a mental instituion.
wait 5 sec
%echo% Several supposedly mentally disturbed teenagers are slowly shown in a group session.
wait 6 sec
%echo% "I am not suicidal!" The blonde teenager from earlier says to an ironfisted orderly.
wait 6 sec
%echo% "Kristen, you are not calm, we need to sedate you." An older lady who appears to be a nurse says as the orderly tries to get a hold of her.
wait 13 sec
%echo% "No!" Kristen shouts as she grabs a unattended scalpel and manages to cut the orderly.
wait 5 sec
%echo% Several other orderlies manage to grab Kristen after gang-rushing here.
wait 6 sec
%echo% The nurse move up quickly and injects Kristen with a syringe full of sedative.
wait 4 sec
%echo% Kristen quickly becomes tired and woozy.
wait 6 sec
%echo% Kristen suddenly finds herself in a dream again, and hears a rhyme being spoken by several school girls jumping rope.
wait 10 sec
%echo% "One....two...Freddy's coming for you."
wait 4 sec
%echo% "Three...four...Better lock your door."
wait 6 sec
%echo% "Five...six...Grab a crucifix."
wait 6 sec
%echo% "Seven...Eight...Need to stay up late."
wait 6 sec
%echo% "Nine...Ten...Never sleep again."
wait 8 sec
%echo% Kristen turns to look at an old beat up house that was apparently Freddy's.
wait 6 sec
%echo% Suddenly Kristen finds herself inside the house and turns as the door closes behind here.
wait 5 sec
%echo% Kristen beats her fists on the door. On the outside of the door a waterfall of blood goes down the door.
wait 8 sec
%echo% Kristen turns around as suddenly she sees Freddy Kruegar. A man dressed in a beat up hat, green and red sweater and brown pants. He has horrible disfiguring scars on his face.
wait 12 sec
%echo% Freddy raises his right hand and a glove with four razor sharp claws on it can be seen on the hand.
wait 6 sec
%echo% "Welcome home baby! HAHAHAHA!" Freddy shouts as he tries to slash Kristen with his claws.
wait 6 sec
%echo% Kristen manages to avoid the swipe and runs down a hall but suddenly finds her feet stuck the melting floor.
wait 8 sec
%echo% Kristen struggles against the floor which now has the consistancy of melted plastic.
wait 6 sec
%echo% Kristen cries out in despair as Freddy is seen getting closer.
~
#694
Spiderman Trigger~
2 g 100
~
wait 30 sec
%echo% Spiderman swings upwards and lands on the spire of a clocktower. Doc Ock is handing onto the side of the clocktower with his mechanical arms.
wait 8 sec
%echo% "What have you done with her Ock!?" Spiderman shouts as he looks down at Doctor Octopus.
wait 6 sec
%echo% "Oh she's just fine. Come on..." Doc Ock responds as he beckons spiderman with his index finger.
wait 8 sec
%echo% Spiderman lets go of the spire and falls towards Doc Ock. Doc Ock flings two of his mechanical arms at Spiderman who manages to block them both.
wait 8 sec
%echo% Spiderman lands a fist against Doc Ock's face with repeated punches. Doc Ock manages to fling Spiderman off of him with one of his mechanical arms.
wait 12 sec
%echo% Doc Ock grabs the minute hand of the clock with his mechanical arms and rips it in half, throwing first one chunk at Spiderman and then the other.
wait 10 sec
%echo% Spiderman kicks the first chunk away and then grabs the other with a web. He then slingshots the chunk back at Doc Ock who barely manages to block it.
wait 12 sec
%echo% While Doc Ock is stunned Spiderman shoots two webs that latch onto his chest and flipping down the side of the tower he yanks Doc Ock loose so they both fall.
wait 8 sec
%echo% Spiderman and Doc Ock pull in close as they fall, with Spiderman punching and kicking Doc Ock.
wait 8 sec
%echo% Suddenly both Spiderman and Doc Ock slam into a moving elevated train.
wait 6 sec
%echo% Spiderman springs to his feet as Doc Ock struggles to his.
wait 6 sec
%echo% Doc Ock swings first one of his mechanical arms at Spiderman and then another with Spiderman managing to avoid both of them.
~
#695
Crying Rivers Trigger~
2 g 100
~
wait 30 sec
%echo% A woman is seen on screen crying.
wait 6 sec
%echo% The woman continues crying profusely.
wait 6 sec
%echo% A man reaches out and pulls her close and they both cry.
wait 6 sec
%echo% The two continue crying, while those in the theater audience start crying too.
wait 6 sec
%echo% Sobs resound through the entire area pitifully.
wait 15 sec
%echo% The crying finally comes to an end and the couple look out a large picture window.
wait 6 sec
%echo% Dark clouds start to drop rain on the ground creating gloomy scenery.
wait 6 sec
%echo% The couple start crying again...
~
#696
Starter and Booster Deck Sale~
0 c 100
buy~
if (%arg% == starter)
 if (%actor.gold% < 15000)
  %send% %actor% You need 15,000 zenni to purchase a starter deck.
 else
  eval num 50
  while (%num% > 0)
   if (%num% >= 48)
     eval lod %random.30% + 699
     %load% o %lod%
   elseif (%num% >= 42)
     eval lod %random.90% + 729
     %load% o %lod%
   else
     eval lod %random.130% + 819
     %load% o %lod%
   end
   eval num %num% - 1
  done
  eval casnum %random.5%
  switch (%casnum%)
   case 1
     %load% o 697
    break
   case 2
     %load% o 698
    break
   case 3
     %load% o 682
    break
   case 4
     %load% o 683
    break
   case 5
     %load% o 684
    break
  done
  nop %actor.gold(-15000)%
  put all.card case
%load% o 624
  %load% o 622
  %load% o 623
  drop all
  %force% %actor% get case
%force% %actor% get command
  %force% %actor% get handbook
  %force% %actor% get creatures
 end
elseif (%arg% == booster)
 if (%actor.gold% < 1500)
  %send% %actor% You need 1,500 zenni to purchase a starter deck.
 else
  eval num 5
  while (%num% > 0)
   eval chance %random.10%
   if (%chance% == 10)
    eval lod %random.30% + 699
    %load% o %lod%
   elseif (%chance% >= 7)
     eval lod %random.90% + 729
     %load% o %lod%
   else
    eval lod %random.130% + 819
     %load% o %lod%
   end
   eval num %num% - 1
  done
  nop %actor.gold(-1500)%
  drop all.card
  %force% %actor% get all.card
 end
else
 %send% %actor% Buy what? A starter or booster?
end
~
#697
Advent Duel Counter Energy~
1 c 3
energy~
if (%arg%)
 eval num %arg%
 if (%num% > 0 && %num% + %self.weight% <= 25)
  %force% %actor% emote adds %num% to %actor.hisher% energy pool.
  nop %self.weight(%num%)%
 elseif (%num% + %self.weight% > 25)
  %send% %actor% You can not have more than 25 energy in your energy pool at one time.
 elseif (%num% + %self.weight% < 0)
  %send% %actor% You can not reduce your energy pool below 0.
 else
  %force% %actor% emote subtracts %num% from %actor.hisher% energy pool.
  nop %self.weight(%num%)%
 end
else
 %send% %actor% Syntax: energy (number)
 %send% %actor% You should check your life and energy totals with the @Gcounter@n command.
end
~
#698
Advent Duel Counter Main~
1 c 3
counter~
if (%arg% == check)
 %send% %actor% @D-------------------------------------------------@n
 %send% %actor% @cYour Life       @D: @C%self.cost_per_day%@n
 %send% %actor% @cYour Energy Pool@D: @G%self.weight%@n
 %send% %actor% @D-------------------------------------------------@n
elseif (%arg% == display)
 %force% %actor% emote displays %actor.hisher% counter for all to see.
 %echo% @D----------------------------------------------@n
 %echo% @cLife       @D: @C%self.cost_per_day%@n
 %echo% @cEnergy Pool@D: @G%self.weight%@n
 %echo% @D----------------------------------------------@n
else
 %send% %actor% Syntax: counter (check - display)
end
~
#699
Advent Duel Counter Life~
1 c 3
life~
if (%arg%)
 eval num %arg%
 if (%num% > 0)
  %force% %actor% emote adds %num% to %actor.hisher% life total.
  nop %self.cost_per_day(%num%)
 elseif (%num% + %self.cost_per_day% < 0)
  %send% %actor% You're dead. That reduces it below 0. No point in actually changing the counter.
 else
  %force% %actor% emote subtracts %num% from %actor.hisher% life total.
  nop %self.cost_per_day(%num%)
 end
else
%send% %actor% Syntax: life (number)
 %send% %actor% You should check your life and energy totals with the @Gcounter@n command.
end
~
$~
