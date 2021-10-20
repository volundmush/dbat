#63200
Edward Guard~
0 q 100
~
if (%direction% == north)
return 0
wait 1 s
emote blocks your way. "@MYO BITCH! If you wish to gain further entry, you will have to fight me in mortal combat..@n"
end
~
#63201
Sulong's Comment~
0 g 100
~
wait 2 s
emote looks at %actor.name%.
wait 1 s
emote raises an eyebrow towards you. "@mYou are no soldier of mine, head back down stairs and find your captain. Battle can come at any moment and we must be ready.@n"
wait 25 s
say @mFine! Let me teach you discipline.@n
wait 1 s
%load% obj 63213
wield long
attack %actor.name%
~
#63202
Knight's Random Speech~
0 g 100
~
eval num %random.3%
if (%num% == 1)
  wait 1 s
  say Go to the Armory and earn your weapon.
elseif (%num% == 2)
  wait 1 s
  say We must be ready, battle looms over us always.
elseif (%num% == 3)
end
~
#63203
Honor Random Speech~
0 g 100
~
eval num %random.4%
if (%num% == 1)
  wait 1 s
  Say With or without the Jarl we will defend this Outpost.
elseif (%num% == 2)
  wait 1 s
  say I find you lacking in the ways of experience for battle.
elseif (%num% == 3)
  wait 1 s
  say Who will lead us without the Jarl?!
end
~
#63204
Scout Nod~
0 g 100
~
wait 2 s
emote looks at %actor.name%.
wait 1 s
emote nods towards you, a serious expression pressed sternly onto his face.
~
#63205
Random Look Battlement~
0 g 100
~
eval num %random.2%
if (%num% == 1)
  wait 1 s
  emote looks at %actor.name%.
end
~
#63206
Hiem's Decloration~
0 g 100
~
wait 2 s
emote looks at %actor.name%.
wait 1 s
emote gestures to the statue. "@YBehold, the Warrior of Warriors. Revel in their glory.@n"
~
#63207
Iaijutsu wield~
1 c 3
Chant~
eval skillholder %actor.skill(hayasa)%
nop %actor.skillset(hayasa 100)%
%force% %actor% hayasa
nop %actor.skillset(hayasa %skillholder%)%
eval skillholder %actor.skill(flex)%
nop %actor.skillset(flex 100)%
%force% %actor% focus flex
nop %actor.skillset(flex %skillholder%)%
~
#63208
Wings~
1 j 100
~
%echo% %actor.name% touches the feather and with a blinding flash of light six large white wings appear on their back.
~
#63209
Crown Control~
1 l 3
~
%force% %actor% wear crown
wait 1
%force% %actor% emote tries to remove the crown from %actor.hisher% head, but is unable to remove it somehow. It may be stuck.
~
#63210
Ronnoc Greet~
0 g 100
~
if (%actor.varexists(dsmith)% && !%self.varexists(smithing)% && !%self.varexists(smithed)%)
 wait 1 s
 say Im sorry %actor.name%. The piece I crafted wasnt perfect enough. I owe you a refund.
 wait 3 s
 nop %actor.gold(25000)%
 wait 3 s
 say My deepest apologies.
 rdelete dsmith %actor.id%
elseif (%actor.varexists(dsmith)% && %self.varexists(smithing)%)
 if (%actor.id% == %self.smithing%)
  wait 1 s
  say Have patience, %actor.name% I am still perfecting your piece. I will let you know when I am finished.
  wait 3 s
  say Feel free to go about your business warrior, until I notify you.
else
  wait 1 s
  say I am working on someone else's piece right now, %actor.name%.
  wait 2 s
  say Come back later and I will talk to you about your piece.
 end
elseif (%actor.varexists(dsmith)% && %self.varexists(smithed)%)
 if (%actor.id% == %self.smithed%)
  wait 1 s
  say Aha, %actor.name%! Your piece is finished.
  wait 2 s
  %echo% %self.name% pulls a glossy black helm out of a soot covered chest.
  wait 1 s
  %load% obj 63235
  give cowl %actor.name%
  rdelete dsmith %actor.id%
  rdelete smithed %self.id%
  wait 2 s
  say Thanks for your gold, warrior.
 else
  wait 1 s
  say Im sorry %actor.name%. The piece I crafted wasnt perfect enough. I owe you a refund.
  wait 3 s
  nop %actor.gold(25000)%
  wait 3 s
  say My deepest apologies.
  rdelete dsmith %actor.id%
 end
elseif (%self.varexists(smithing)%)
 wait 1 s
 %echo% %self.name% hammers a piece of metal.
 wait 2 s
 %send% %actor% %self.name% looks up at you.
 %echoaround% %actor% %self.name% looks up at %actor.name%.
 wait 2 s
 say I am working on a piece right now, so I cant take any orders. Please check with me later, warrior.
 wait 2 s
 %echo% %self.name% resumes his work on a @Dthin black helm@n.
elseif (!%self.varexists(smithed)%)
  wait 1 s
 %echo% %self.name% hammers a piece of metal.
 wait 2 s
 %send% %actor% %self.name% stops what he is doing and looks at you.
 %echoaround% %actor% %self.name% stops what he is doing and looks at %actor.name%.
 wait 2 s
 say If youre looking for a Smiths touch, I only work in @DO@rn@Dy@Rx@n Ore.
 wait 3 s
 say It is a rare type of stone only found in the world of the living.
 wait 3 s
 say Find me some, and twenty five thousand zenni? Ill make you something battle-worthy.
 wait 3 s
 say What piece? A fine helm, methinks. To protect your simple mind.
 wait 3 s
 say If that isn't a good enough answer, find another smith.
end
~
#63211
Ronnoc Receive~
0 j 100
~
if (%object.vnum% != 63318)
 wait 1 s
 say That isnt what I asked for, warrior.
 wait 2 s
 say Im only working with @DO@rn@Dy@Rx@n Ore right now.
elseif (!%actor.varexists(paid)%)
 wait 1 s
 say Alright. Now about that twenty five thousand zenni?
 set ore
 remote ore %actor.id%
 wait 1 s
 say twenty five thousand zenni.
else
 wait 1
 say Alright, I will get started on your helm.
 set dsmith
 remote dsmith %actor.id%
 wait 2 s
 say You can go do something else, it will take about two hours.
 wait 4 s
 say I will notify you when the piece is done.
 if (%time.hour% <= 21)
  eval tme %time.hour% + 2
 elseif (%time.hour% == 22)
  eval tme 0
 elseif (%time.hour% == 23)
  eval tme 1
 end
 set smithc %tme%
 remote smithc %self.id%
 set smithing %actor.id%
 remote smithing %self.id%
 set workfor %actor.name%
 remote workfor %self.id%
end
~
#63212
Ronnoc Bribe~
0 m 1
~
if (%amount% < 25000)
 return 0
 wait 1
 say That isn't enough, don't try and cheat me, warrior.
elseif (!%actor.varexists(ore)%)
 wait 1 s
 say Alright now give me the ore.
 set paid
 remote paid %actor.id%
else
 wait 1
 say Alright, I will get started on your helm.
 set dsmith
 remote dsmith %actor.id%
 wait 2 s
 say You can go do something else, it will take about two hours.
 wait 4 s
 say I will notify you when the piece is done.
 if (%time.hour% <= 21)
  eval tme %time.hour% + 2
 elseif (%time.hour% == 22)
  eval tme 0
 elseif (%time.hour% == 23)
  eval tme 1
 end
 set smithc %tme%
 remote smithc %self.id%
 set smithing %actor.id%
 remote smithing %self.id%
 set workfor %actor.name%
 remote workfor %self.id%
end
~
#63213
Ronnoc Timer~
0 ab 100
~
if (%self.varexists(smithing)%)
 if (%self.smithc% == %time.hour%)
  rdelete smithc %self.id%
  tell %self.workfor% I have finished my work, come and see me to receive the product.
  set smithed %self.smithing%
  rdelete smithing %self.id%
  redelete workfor %self.id%
  remote smithed %self.id%
 end
end
~
#63214
Heidrun Hide Drop~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 700)
    %load% obj 63236 
  Else
End
~
#63215
Anansi Greet~
0 g 100
~
 if (!%actor.varexists(onanq)% && !%actor.varexists(anqcomp)%)
      wait 2s
      %echoaround% %self% @n@RA@Dn@Ra@Dn@Rs@Di@n turns to regard %actor.name% with a wicked grin.
      wait 3s
      %echoaround% %self% @n"@GYeah ... you'll do. You'll do just fine.@n" The voice is smooth, and a little oily like that of a salesman.
      wait 2s
      %echoaround% %self% The Namekian turns to regard %actor.name% more fully. @n"@GListen. I need a @Rfavor@G. If you do it for me, I'll cut you some skin in the game, huh?@n"
      wait 3s
      %echoaround% %self% @RA@Dn@Ra@Dn@Rs@Di@n waits for a time in silence, as if to help sell the deal. @n"@GIf you think you're good enough, that is.@n"
      wait 2s
      %echoaround% %self% The Namekian turns away, returning to his own schemes.
elseif (%actor.varexists(anqcomp)%)
            wait 2s
            %echoaround% %self% @RA@Dn@Ra@Dn@Rs@Di@n chuckles with a sinister edge as he turns his head to regard %actor.name%.
            wait 3s
            %echoaround% %self% @n"@GHeh .. I already got what I needed from you. Enjoy your cut!@n"
       else
         wait 2s
         %echoaround% %self% The Namekian pauses in his mutterings to lift his head and regard %actor.name% with a shrewd glance.
         wait 3s
         %echoaround% %self% @n"@GHave you brought me that goat hide yet? Hurry up fool! My time is money.@n"
         wait 2s
         %echoaround% %self% The Namekian turns away from %actor.name% as he returns to his own scheming.
  End
~
#63216
Anansi Accept~
0 c 100
favor~
If (%actor.varexists(onanq)%)
  return 0
  wait 3s
   say @GYou've already agreed to help me. Why aren't you off doing just that?@n
    Elseif (%actor.varexists(anqcomp)%)
      return 0
      wait 3s
      say @GYou've already served your purpose. Scram so I can think without you annoying me.@n
Else
  wait 2s
  %echoaround% %self% The Namekian splits his smile even more wickedly and rubs his hands together.
  wait 3s
  say @GAlright. Somewhere out among the lost souls there's a goat. Kill the thing, and bring me its pelt, deal? Now go!@n
  Set onanq
  remote onanq %actor.id%
End
~
#63217
Anansi Receive~
0 j 100
~
If (!%actor.varexists(onanq)% && !%actor.varexists(anqcomp)%)
    return 0
    wait 3s
    say @GYou haven't agreed to help me, numb nuts.@n
Elseif (%actor.varexists(anqcomp)%)
    return 0
    wait 3s
    say @GI've already got the hide. Go away.@n
Else
    If (%object.vnum% != 63236)
        return 0
        wait 3s
        say @GThis isn't what I asked you for, numb nuts!@n
        wait 2s
        %echoaround% %self% The Namekian turns away from %actor.name% with a sucking of his teeth.
        wait 3s
        %echoaround% %self% @RA@Dn@Ra@Dn@Rs@Di@n reaches out greedily to take the goat hide from %actor.name%, almost snatching it from their hands.
        wait 3s
        say @GYes. This is it exactly!@n
        wait 2s
        %purge% %object.name%
        %echoaround% %self% The Namekian gives a positively sinister sounding chuckle.
        wait 3s
        %echoaround% %self% After several moments, he turns to look at %actor.name% with a begrudging look upon his green-skinned face.
        wait 3s
        rdelete onanq %actor.id%
        wait 1s
        say @GTake this, and get out of here!@n
        Eval num %random.1000%
            if (%num% >= 800)
            wait 3s
            %echoaround% %self% A shining @Whalo@n of @Yl@Wi@Yg@Wh@Yt@n shimmers into existence at the feet of %actor.name% after @RA@Dn@Ra@Dn@Rs@Di@n spends a moment in concentration.
            wait 2s
            set anqcomp
            remote anqcomp %actor.id%
            %load% obj 63233
            give halo %actor.name%
            wait 3s
            %echoaround% %self% The Namekian turns back to his own scheming, no longer interested in %actor.name%.
Else
    wait 3s
    %echoaround% %self% The Namekian pulls a dirty looking @yb@nu@yr@Dl@ya@np sack from his belongings and nearly tosses it at %actor.name%
    wait 2s
    set anqcomp
    remote anqcomp %actor.id%
    %load% obj 63226
    give burlap %actor.name%
    wait 3s
    %echoaround% %self% The Namekian turns back to his own scheming, no longer interested in %actor.name%.
    End
  End
End
~
#63299
Test Funds~
2 c 100
procure~
if ("%arg%" != "funds" && "%arg%" != "fund")
   wait 1s
   %send% %actor% @RTry to @YProcure@R some @YFunds@R.@n
else
nop %actor.gold(1000000)%
%send% %actor% @RA Million Bucks.@n
end
~
$~
