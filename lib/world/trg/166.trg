#16600
Ooze Portal between Yetti and Mining~
1 c 100
enter~
wait 1
if %arg% == ooze
  %echoaround% %actor% %actor.name% hops into a @mp@Mu@mddl@Me@D of @MOo@mz@Me@Z!@n
  %send% %actor% You hop into a @mp@Mu@mddl@Me@D of @MOo@mz@Me!@n
  wait 1s
  if %actor.room.vnum% == 16620
    set destination 16665
  elseif %actor.room.vnum% == 16665
    set destination 16620
  end
  %teleport% %actor% 16697
  %force% %actor% look
  wait 2s
  %send% %actor% @MYou become lost inside the @mOoze@M.@n
  wait 2s
  %send% %actor% @MThe @mooze @Mthreatens to crush your body!@n
  wait 2s
  %teleport% %actor% %destination%
  wait 1
  %echoaround% %actor% @MWith a sickening, plopping sound, @D%actor.name%@M is thrust up out of the @mooze@M.@n
  %force% %actor% look
  wait 1
  %send% %actor% @MSomehow, you manage to find your way to the other side!@n
else
  %send% %actor% @COOC:@c Try "enter ooze"@n
end
~
#16601
Look at ooze desc~
1 c 100
look~
if %arg% == ooze
  wait 1
  %send% %actor% @MThe @mooze@M writhes and pulsates, almost as if alive.  Its depths are indiscernable, but it looks as if it is deep enough to @Yenter@M.@n
else
  %force% %actor% look %arg%
end
~
#16602
Izzera Speech Trig (Entrance to Mining)~
0 g 100
~
wait 1s
%echo% @GI@gzz@Ge@gr@Ga@m sighs and twitters her wings in frustration.@n
wait 5s
%echo% @GI@gzz@Ge@gr@Ga@m slams her fist on her desk, rattling some pens.  @C'I'll never collect enough @cOre@C at this rate.'@n
wait 4s
%echo% @GI@gzz@Ge@gr@Ga@m looks up.  @C'Another slave to work the mine?  Where are your guards?'@n
wait 7s
%echo% @C'Oh, you're not a slave, then.'  @GI@gzz@Ge@gr@Ga@m sighs.  @C'Well, we're behind on our @cOre@C collection, so if you @cmine@C some @cOre@C, I'll let you keep some of the profit.'@n
wait 7s
%echo% @C'If you don't have a pickaxe, give me one-hundred zenni and I'll give you one.' @GI@gzz@Ge@gr@Ga@m sighs and looks back down at her papers.@n
~
#16603
Izzera bribe for pickaxe~
0 m 100
~
wait 1s
if %amount% >= 100
  if %amount% > 100
    eval %therefund% %amount% - 100
    %send% %actor% @GI@gzz@Ge@gr@Ga @mcounts the money, then raises an eyebrow at you.  @C'This is too much.  No refunds, though.'@n
  end
  %load% obj 2008
  give pickaxe %actor.name%
end
if %amount% < 100
  wait 1s
  %send% %actor% @GI@gzz@Ge@gr@Ga @mlaughs at you.  @C'That isn't enough, and I keeping this.  Hahaha!'@n
end
~
#16604
Adolescent Yetti Aggressiveness~
0 g 100
~
wait 2s
if %actor.room.vnum% == %self.room.vnum%
  %echoaround% %actor% @cAn adolescent @WY@we@Dt@wt@Wi@c looks up, noticing %actor.name%.  After a brief second, it smiles malevolently, and growls out, @C'You! I bash!'@n
  %send% %actor% @cAn adolescent @WY@we@Dt@wt@Wi@c looks up, noticing you.  After a brief second, it smiles malevolently, and growls out, @C'You! I bash!'@n
end
wait 4s
if %actor.room.vnum% == %self.room.vnum%
  %echoaround% %actor% @cAn adolescent @WY@we@Dt@wt@Wi@c, rears up, preparing to charge at %actor.name%.@n
  %send% %actor% @cAn adolescent @WY@we@Dt@wt@Wi@c, rears up, preparing to charge at you!@n
  wait 1s
  punch %actor.name%
end
~
#16605
Kazz Warning about Yetti~
0 g 100
~
wait 1s
emote digs at the wall with his bare hands, blood dripping from his talony fingers.
wait 4s
emote punches the wall desparately.  @C'Please, please don't let them find me.'@n
wait 5s
emote jumps, startled, noticing he's not alone now.
wait 3s
say Don't go down that way!
wait 2s
emote points toward the east.
wait 5s
say Yettis.  Giant, abominations.  Mutant Arlians.  THEY WILL KILL YOU!
wait 4s
emote begins crying and gets back on his knees, once again clawing at the wall.
~
#16610
Mining Trigger~
2 c 100
mine~
* ----- Arlia Mining Trigger (mine) by Sedalis
* - See trigger 2006 for basis of this Trigger
************************************************************
* ----- Check to see if you can mine...
************************************************************
if (%actor.fighting%)
  return 1
  wait 1
  %send% %actor% You are fighting!
else
  if (%actor.eq(17)%)
    eval obj %actor.eq(17)%
  end
  if (%obj.vnum% != 2008)
    wait 1
%send% %actor% You do not even have a pickaxe equiped!
  elseif (%actor.move% < (%actor.maxmove% / 25))
    wait 1
    %send% %actor% You are too tired to mine right now.
  elseif (%actor.pos% != Standing)
    wait 1
    %send% %actor% You must be standing!
  else
    ********************************************************
    * ----- Let's start mining!
    ********************************************************
    * ----- Variables and modifiers
    ********************************************************
    eval stamdrain2 %actor.maxmove% / 25
    eval stamdrain -%stamdrain2%
    set ore 0
    set levelmod 0
    set num 0
    set racemod 0
    set roommod 0
    set levelrestrict 0
    set outcomes 0
    set oremessage @CThis is broken!  Report to an Imm!  (Preferably Sedalis)@n
    set oreothermessage @CSedalis Loses.@n
    ********************FUTURE ADDITION*********************
    set questmod 0
    ********************************************************
    eval num %random.80%
    eval levelmod %actor.level%
    eval levelmod %levelmod% / 10
    if %actor.race% == Arlian
      eval racemod %racemod% + 6
    elseif %actor.race% == Human
      eval racemod %racemod% + 4
    elseif (%actor.race% == Android || %actor.race% == Demon || %actor.race% == Mutant)
      eval racemod %racemod% + 3
    elseif (%actor.race% == Truffle || %actor.race% == Saiyan || %actor.race% == Majin || %actor.race% == Kanassan)
      eval racemod %racemod% - 5
    end
    eval outcomes %outcomes% + %racemod%
    if (%self.vnum% == 16628 || %self.vnum% == 16627)
      eval roommod %roommod% - 40
      eval levelrestrict %levelrestrict% + 0
      set gwokmob 16618
    elseif (%self.vnum% == 16637 || %self.vnum% == 16638 || %self.vnum% == 16639 || %self.vnum% == 16641)
      eval roommod %roommod% - 30
      eval levelrestrict %levelrestrict% + 15
      set gwokmob 16619
    elseif (%self.vnum% == 16649 || %self.vnum% == 16650 || %self.vnum% == 16647 || %self.vnum% == 16648)
      eval roommod %roommod% - 15
      eval levelrestrict %levelrestrict% + 25
      set gwokmob 16620
    elseif (%self.vnum% == 16662 || %self.vnum% == 16661 || %self.vnum% == 16664 || %self.vnum% == 16663 || %self.vnum% == 16660)
      eval roommod %roommod% - 0
      eval levelrestrict %levelrestrict% + 40
      set gwokmob 16621
    end
    eval outcomes %num% + %levelmod% + %questmod% + %racemod% + %roommod%
    if %outcomes% <= 0
      set outcomes 1
    end
    ********************************************************
    * ----- Calculate success
    ******************************************************** 
    if %actor.level% < %levelrestrict%
      %send% %actor% You are not ready to mine here yet!
    else
      if %outcomes% < 2
        set ore 0
        set mobchance %random.10%
        if %mobchance% <= 4
          set oremessage @DYou knock loose some dust... and an @DA@wrl@Dia@wn W@Da@wll Cr@Dee@wp@De@wr@D falls out of the dirt and prepares to attack you!@n
          set oreothermessage @D%actor.name% knocks loose some dust... and an @DA@wrl@Dia@wn W@Da@wll Cr@Dee@wp@De@wr@D falls out and prepares to attack %actor.hisher%!@n
        else
          set oremessage @DYou knock loose some dust.@n
          set oreothermessage @D%actor.name% knocks loose some dust.@n
        end          
      elseif %outcomes% < 35
        set ore 16608
        set oremessage @DYou knock loose a @wCh@yu@wnk@y o@wf R@yo@wck.@n
        set oreothermessage @D%actor.name% knocks loose a @wCh@yu@wnk@y o@wf R@yo@wck.@n
      elseif %outcomes% < 70
        set ore 16604        
        set oremessage @DYou knock loose some @yD@Du@yll @DO@yr@De.@n
        set oreothermessage @D%actor.name% knocks loose some @yD@Du@yll @DO@yr@De.@n
      elseif %outcomes% < 85
        set ore 16605
        set oremessage @DYou knock loose some @WSh@wi@Wn@Wi@wng @WO@wr@We@D!@n
        set oreothermessage @D%actor.name% knocks loose some @WSh@wi@Wn@Wi@wng @WO@wr@We@D!@n
      elseif %outcomes% < 95
        set ore 16606
        set oremessage @DWow!  You knock loose some @WSp@Ya@Wrkl@Yi@Wng @YO@Wr@Ye@D!@n
        set oreothermessage @D%actor.name% knocks loose some @WSp@Ya@Wrkl@Yi@Wng @YO@Wr@Ye@D!@n
      elseif %outcomes% < 200
        set ore 16607
        set oremessage @DNo way!  What incredible luck!  You've knocked loose some @CTr@ca@Cnsp@ca@Cr@ce@Cnt @cO@Cr@ce@D!!@n
        set oreothermessage @D%actor.name% knocks loose some @CTr@ca@Cnsp@ca@Cr@ce@Cnt @cO@Cr@ce@D!!@n
      end
      ******************************************************
      * ----- Now let's display some stuff and make it happen...
      ******************************************************
      set %actor.move(%stamdrain%)%
      wait 1s
      if %actor.room.vnum% == %self.vnum%
        %send% %actor.name% @DYou heft your pickaxe and strike a blow at the ore deposit.@n
        %echoaround% %actor.name% @D%actor.name% hefts %actor.hisher% pickaxe and strikes a blow at the ore deposit.@n
      end
      wait 3s
      if %actor.room.vnum% == %self.vnum%
        %send% %actor.name% @DYou heft your pickaxe and strike another blow at the ore deposit.@n
        %echoaround% %actor.name% @D%actor.name% hefts %actor.hisher% pickaxe and strikes another blow at the ore deposit.@n
      end
      wait 3s
      if %actor.room.vnum% == %self.vnum%
        %send% %actor.name% @DYou heft your pickaxe and strike another blow at the ore deposit.@n
        %echoaround% %actor.name% @D%actor.name% hefts %actor.hisher% pickaxe and strikes another blow at the ore deposit.@n
      end
      wait 3s
      if %actor.room.vnum% == %self.vnum%
        %send% %actor.name% %oremessage%
        %echoaround% %actor.name% %oreothermessage%
      end
      if %ore% != 0
        %load% obj %ore%
        %force% %actor% get ore
      elseif %mobchance% <= 4
        %load% mob %gwokmob%
        wait 2s
        %force% gwok punch %actor.name%
      end
    end
  end
end
~
#16611
Mining Trigger (Turn In Ore)~
0 c 100
trade~
* ----- Mining Trigger (Turn in Ore) by Sedalis
**************************************************
* ----- Qualify Position...
**************************************************
if %actor.pos% != Standing
  wait 1
  %send% %actor You should probably stand up for this.
else
  ************************************************%
  * ----- Set Racial Bonus Constants and Other Vars
  ************************************************
  set oretotal 0
  set racebonus 2
  if %actor.race% == Arlian
    set racebonus 4
  elseif %actor.race% == Saiyan
    set racebonus 1
  elseif %actor.race% == Halfbreed
    set racebonus 1
  elseif %actor.race% == Android
    set racebonus 1
  end
  ************************************************
  * ----- Figure up total
  ************************************************
  %force% %actor% give all.ore izzera
  while %self.inventory(16608)%
    eval oretotal %oretotal% + 1 + %racebonus%
    eval thisore %self.inventory(16608)%
    %purge% %thisore.name%
  done
  while %self.inventory(16604)%
    eval oretotal %oretotal% + 5 + %racebonus%
    eval thisore %self.inventory(16604)%
    %purge% %thisore.name%
  done
  while %self.inventory(16605)%
    eval oretotal %oretotal% + 15 + %racebonus%
    eval thisore %self.inventory(16605)%
    %purge% %thisore.name%
  done
  while %self.inventory(16606)%
    eval oretotal %oretotal% + 25 + %racebonus%
    eval thisore %self.inventory(16606)%
    %purge% %thisore.name%
  done
  while %self.inventory(16607)%
    eval oretotal %oretotal% + 40 + %racebonus%
    eval thisore %self.inventory(16607)%
    %purge% %thisore.name%
  done
  ************************************************
  * ----- Give out zenni and say stuff...
  ************************************************
  wait 1s
  say Let's see here.  Your cut is...
  set %actor.gold(%oretotal%)%
  %send% %actor% You receive @Y%oretotal%@n zenni.
end
~
#16612
Alpha Yetti Random Actions~
0 g 70
~
set randomaction %random.7%
wait 1s
switch %randomaction%
  case 1
    emote farts loudly, then throws his head back and laughs.
  break
  case 2
    emote growls and bashes his fist into the arm of the throne.
  break
  case 3
    emote raises his hand, looking for a yetti slave to slap.
  break
  case 4
    say One day, me rule Arlia!
  break
  case 5
    say Stupid king...
  break
  case 6
    emote picks his teeth with the femur bone of some unfortunate, deceased warrior.
  break
  case 7
    emote stretches and yawns.  He elbows the throne a few times, attempting to make it more comfortable.
  break
done
~
#16613
Yaz and Yazzart banter~
0 g 100
~
set randombanter %random.10%
switch %randombanter%
  case 1
    wait 2s
    say You know, Yazzart, if we could catch a Yetti down there in the mines and train it, we wouldn't have to take crap from anybody.
    wait 5s
    %force% yazzart say No kiddin', Yaz?  Say, I wonder why we never thought about that before?
    wait 3s
    %force% yazzart emote slaps Yaz on the back of his head.  '@CYou think maybe it's because everyone who finds the Yettis never come back, you idiot?@n'
    wait 4s
    emote rubs the back of his head and glares at Yazzart.  '@CYou know, one of these days, I'm going to strike you back.@n'
    wait 5s
    %force% yazzart say Uh-huh.  Until that day comes, you can just shut your mouth.
  break
  case 10
    wait 1s
    say I heard that there's this enormous Yetti that collects treasures from fallen adventurers, Yazzart.
    wait 4s
    say I bet if we could sneak in there, we could get something valuable.
    wait 5s
    %force% yazzart emote looks up and elbows Yaz in the ribs.  '@CQuiet, idiot.  We aren't alone.@n'
    wait 5s
    emote rubs his ribs and throws an annoyed glance at Yazzart.  '@C... I don't like you, Yazzart.@n'  He mumbles for a minute and then turns his back on Yazzart to contemplate the wall.
  break
  default
    wait 1s
    say ...and then she slammed that guy's face right into one of the mining holes!
    wait 2st
    emote slaps his knee and doubles over with laughter.
    wait 4s
    %force% yazzart emote barely cracks a smile.  '@CIzzera always did have a way with actions.  Quite a woman she is.@n'
    wait 5s
    emote stands up slowly, the last few remnants of a chuckle stretching out thir remaining few seconds of life.  '@CYeah.  I tell ya, Yazzart.  She's as mean as a Yetti, but if you could tame her...@n'  He smiles as he leaves his sentence unfinished.
    wait 6s
    %force% yazzart emote allows his smile to grow a little larger.  '@CI know what you mean, Yar.@n'
  break
done
~
#16614
hoshi invader random~
0 b 20
~
emote fires energy blasts indiscriminately at men, women, children, buildings... even into the very air and seemingly nothing at all like some sort of berserker.
~
#16680
Restring test~
1 c 100
sedsize~
%echo% %arg% - is what was listed for arguments.
%echo% %self.description% <--- Description
%echo% %self.desc% <--- Desc
%echo% %self.adesc% <---- Adesc
~
#16688
wtf wait until test~
1 b 100
~
*
~
#16689
Time Display~
2 c 100
timenow~
%echo% The current time variable is: %time.hour%
~
#16690
Public Ship Move Test~
1 b 100
~
*@Y------------------------------------@n
*@Y-@R Public Ship script by Sedalis@n
*@Y------------------------------------@n
*@W----@C Important note!@n
*@W----@c Where the ship starts at is determined by the@n
*@W----@c load trigger for the ship!@n
*@Y----@g Differentiate variable stuff by ship@n
*@Y----@g Ship load location should be last on list@n
*@Y----@g Test Ship Transport Schedule:@n
*@Y----@n
if (%self.name% == test ship)
  set stop1 0
  set thisshipname @YTest Ship@n
  set destination1 @REarth@n
  set destination1vnum 408
  set stop2 2
  set destination2 @RVegeta@n
  set destination2vnum 2323
  set stop3 4
  set destination3 @RFrigid@n
  set destination3vnum 4250
  set stop4 6
  set destination4 @RKonack@n
  set destination4vnum 8194
  set stop5 8
  set destination5 @RAether@n
  set destination5vnum 12002
  set stop6 10
  set destination6 @RNamek@n
  set destination6vnum 11626
  set stop7 12
  set destination7 @REarth@n
  set destination7vnum 408
  set stop8 14
  set destination8 @RVegeta@n
  set destination8vnum 2323
  set stop9 16
  set destination9 @RFrigid@n
  set destination9vnum 4250
  set stop10 18
  set destination10 @RKonack@n
  set destination10vnum 8194
  set stop11 20
  set destination11 @RAether@n
  set destination11vnum 12002
  set stop12 22
  set destination12 @RNamek@n
  set destination12vnum 11626
end
*@Y----@n
*@Y----@g Set up next departure details.@n
*@Y----@n
if (%time.hour% < 2)
  set nextdestination %destination2%
  set nextdestinationvnum %destination2vnum%
  set departuretime %stop2%
elseif (%time.hour% < 4)
  set nextdestination %destination3%
  set nextdestinationvnum %destination3vnum%
  set departuretime %stop3%
elseif (%time.hour% < 6)
  set nextdestination %destination4%
  set nextdestinationvnum %destination4vnum%
  set departuretime %stop4%
elseif (%time.hour% < 8)
  set nextdestination %destination5%
  set nextdestinationvnum %destination5vnum%
  set departuretime %stop5%
elseif (%time.hour% < 10)
  set nextdestination %destination6%
  set nextdestinationvnum %destination6vnum%
  set departuretime %stop6%
elseif (%time.hour% < 12)
  set nextdestination %destination7%
  set nextdestinationvnum %destination7vnum%
  set departuretime %stop7%
elseif (%time.hour% < 14)
  set nextdestination %destination8%
  set nextdestinationvnum %destination8vnum%
  set departuretime %stop8%
elseif (%time.hour% < 16)
  set nextdestination %destination9%
  set nextdestinationvnum %destination9vnum%
  set departuretime %stop9%
elseif (%time.hour% < 18)
  set nextdestination %destination10%
  set nextdestinationvnum %destination10vnum%
  set departuretime %stop10%
elseif (%time.hour% < 20)
  set nextdestination %destination11%
  set nextdestinationvnum %destination11vnum%
  set departuretime %stop11%
elseif (%time.hour% < 22)
  set nextdestination %destination12%
  set nextdestinationvnum %destination12vnum%
  set departuretime %stop12%
else
  set nextdestination %destination1%
  set nextdestinationvnum %destination1vnum%
  set departuretime %stop1%
end
*@Y----@n
*@Y----@g Set up Low Orbit VNUMs@n
*@Y----@n
*
* @gStill testing other parts.
*
*@Y----@n
*@Y----@g Now that we have our destinations filled out and@n
*@Y----@g taken care of, set up the actual wait and move.@n
*@Y----@n
%echo% @R--@YDetails@n  
%echo%   Departure time: %departuretime%
%echo%   Destination:    %nextdestination%
wait 269s
%echo% %thisshipname%@R will be departing in thirty seconds for %nextdestination%!@n
wait 10s
%echo% %thisshipname%@R will be departing in twenty seconds for %nextdestination%!@n
wait 10s
%echo% %thisshipname%@R will be departing in ten seconds for %nextdestination%!@n
wait 10s
%echo% %thisshipname%@R roars to life and blasts into orbit!@n
ogoto %nextdestinationvnum%
wait 1s
%send% Sedalis %thisshipname%@R is now on %nextdestination%.
%echo% %thisshipname%@R descends through the atmosphere and lands with a roar of thrusters.@n
~
#16691
Good Luck Charm Wear~
1 j 100
~
wait 1
%send% %actor% @RAs you slip the ring on, the agonized howling of thousands of tortured professors fills the room.@n
%echoaround% %actor% @RAs %actor.name% slips the ring on, the agonized howling of thousands of tortured professors fills the room.@n
~
#16692
Set name to object.~
1 j 100
~
set derpname %actor.name%
remote derpname %self.id%
~
#16693
object random test~
1 b 100
~
set gayrand %random.5%
if (%gayrand% == 1)
  %echo% @MGlittering vampires surround you and try to kiss you.@n
elseif (%gayrand% == 2)
  %echo% @mYour intelligence drains and you begin to drool on yourself.@n
elseif (%gayrand% == 3)
  %echo% @RYou suddenly get the urge to be involved in a retardedly terrible and unhealthy relationship.@n
elseif (%gayrand% == 4)
  %echo% @CHomosexual werewolves.  That are in love with children.  This seems acceptable to you.@n
else
  %echo% @YSomeone comes to their senses and throws a book away, never to be seen again.@n
  %purge% %self%
end
~
#16694
new trigger~
2 c 100
ltest~
%echo% Testing...
%load% obj 91
%echo% Finished.
~
#16696
Stay here, you've been bad~
0 q 100
~
if (%direction% \= up)
  return 0
  %echo% @RNope.  You stay right here, %actor.name%.@n
end
~
#16697
No instant, jerk~
2 c 100
inst~
if (%cmd% \= inst)
  %echo% @R%actor.name%, you can't instant out of here.@n
end
~
#16698
Morts stuck in office~
0 q 100
u~
if (%direction% == up || %direction% == west)
if (%actor.name% != Sedalis && %actor.name% != Lacrimosa)
    return 0
    %send% %actor% @RNope.  Stay right there.@n
    %echoaround% %actor% @R%actor.name% tries to leave but can't.@n
  end
end
~
#16699
Trigger Tests~
2 d 1
computer~
extract word1 1 %speech%
extract word2 2 %speech%
%echo% Word one:  %word1% and word two: %word2%
~
$~
