#2400
Gastron Speech~
0 cd 100
worth~
if !%actor.varexists(weaponring)%
  wait 2 s
  say So you want to enter our secret club, eh? Fine.
  set weaponring start
  remote weaponring %actor.id%
  wait 2 s
  say Write down these instructions or something, because there's no way I am repeating myself. Got it?
  wait 2 s
  say Five rings of mastery you must bring me. Frozen, Swift, Fury, Storm and Tempered.
else
  wait 2 s
  say Damnit!  You idiot!  @RPay attention!@C
  wait 2s
  say Five rings of mastery you must bring me. Frozen, Swift, Fury, Storm and Tempered.  
end
~
#2401
Master Rings Receive~
0 j 100
~
if (!%actor.varexists(weaponring)%)
 return 0
 say Wait. Don't just start handing me things when I don't even know your @Gworth@C.
else
 if %actor.weaponring% != complete
  if (%object.vnum% == 2450)
   %purge% %object%
   set weaponring frozen
   remote weaponring %actor.id%
   wait 2 s
   say Ahh yes, the Ring of Frozen Mastery, forged by the Icers. Now seek out those who are renowned for their swift strikes.
  elseif (%object.vnum% == 2451)
   %purge% %object%
   set weaponring swift
   remote weaponring %actor.id%
   wait 2 s
   say Swift and precise, that is the way of the Konatsus. The next ring is a symbol of weapon mastery utilising fury.
  elseif (%object.vnum% == 2452)
   %purge% %object%
   set weaponring fury
   remote weaponring %actor.id%
   wait 2 s
   say Saiyans certainly favor brute strength and fury over skill, don't they? The next ring you require is that of the Storm.
  elseif (%object.vnum% == 2453)
   %purge% %object%
   set weaponring storm
   remote weaponring %actor.id%
   wait 2 s
   say Violent tempests, storms, torrential water. I had not thought to see one of these rings ever again after Kanassa was wiped out.
  elseif (%object.vnum% == 2454)
   %purge% %object%
   set weaponring complete
   remote weaponring %actor.id%
   wait 2 s
   say Ahh, the final ring. Tempered, resilient, this ring mirrors the qualities of humanity. You have.. proven yourself and thus I will let you enter our secret club.
   wait 2 s
   say Just head inside the tunnel.
  else
   return 0
   say I don't want that!
  end
 else
  return 0
  say I don't want that!
 end
end
~
#2402
Gastron Guard~
0 q 100
~
if !%actor.varexists(weaponring)%
  if %direction% == inside
    return 0
    %send% %actor% Gastron blocks you from entering there!
    say Where do you think you're going? Can't let you pass.
    say Though if you say you want to prove your worth, I might tell you how.
  end
else
  if %direction% == inside
    if %actor.weaponring% == complete
      if %actor.drag%
        return 0
        wait 1
        say You may not drag anyone inside.
      else
        return 1
        %send% %actor% Gastron says, @C'@WHead on in.@C'@n
      end
    else
      return 0
      wait 1
      say You haven't yet proved your @Yworth@C.
    end
  else
    return 1
  end
end
~
#2403
Turugolo defeated~
0 f 100
~
if !%actor.varexists(chaozardefeated)%
 if !%actor.varexists(turugolodefeated)%
  set turugolodefeated
  remote turugolodefeated %actor.id%
  wait 2 s
  %send% %actor% @RYou have defeated Turugolo!@n
 end
end
~
#2404
Bultsu Defeated~
0 f 100
~
if !%actor.varexists(chaozardefeated)%
 if !%actor.varexists(bultsudefeated)%
  set bultsudefeated
  remote bultsudefeated %actor.id%
  wait 2 s
  %send% %actor% @RYou have defeated Bultsu!@n
 end
end
~
#2405
Bibiria Defeated~
0 f 100
~
if !%actor.varexists(chaozardefeated)%
 if !%actor.varexists(bibiriadefeated)%
  set bibiriadefeated
  remote bibiriadefeated %actor.id%
  wait 2 s
  %send% %actor% @RYou have defeated Bibiria!@n
 end
end
~
#2406
Seiten Defeated~
0 f 100
~
if !%actor.varexists(chaozardefeated)%
 if !%actor.varexists(seitendefeated)%
  set seitendefeated
  remote seitendefeated %actor.id%
  wait 2 s
  %send% %actor% @RYou have defeated Saiten!@n
 end
end
~
#2407
Raishe Defeated~
0 f 100
~
if !%actor.varexists(chaozardefeated)%
 if !%actor.varexists(raishedefeated)%
  set raishedefeated
  remote raishedefeated %actor.id%
  wait 2 s
  %send% %actor% @RYou have defeated Raishe!@n
 end
end
~
#2408
Chaozar Defeated~
0 f 100
~
if !%actor.varexists(chaozardefeated)%
  set chaozardefeated
  remote chaozardefeated %actor.id%
  %send% %actor% @RYou have defeated Chaozar!@n
  if (%actor.varexists(turugolodefeated)% && %actor.varexists(bultsudefeated)% && %actor.varexists(bibiriadefeated)% && %actor.varexists(seitendefeated)% && %actor.varexists(raishedefeated)%)
   rdelete turugolodefeated %actor.id%
   rdelete bultsudefeated %actor.id%
   rdelete bibiriadefeated %actor.id%
   rdelete seitendefeated %actor.id%
   rdelete raishedefeated %actor.id%
  end
end
~
#2409
VIP Guard block~
0 q 100
~
if (%direction% == north && %actor.varexists(chaozardefeated)%)
 %send% %actor% The VIP Guard lets you pass.
 return 1
elseif (%direction% == north && %actor.varexists(turugolodefeated)% && %actor.varexists(bultsudefeated)% && %actor.varexists(bibiriadefeated)% && %actor.varexists(seitendefeated)% && %actor.varexists(raishedefeated)%)
 %send% %actor% The VIP Guard lets you pass.
 return 1
elseif (%direction% == north)
 return 0
 say You need to defeat all of the weapon bosses before you can proceed.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone north.
end
~
#2410
Alexia Reward~
0 d 100
*~
if (%speech% == sword)
 if !%actor.varexists(chaozardefeated)%
  wait 1 s
  say You need to defeat the Weapon Master Chaozar, before I can reward you.
 elseif %actor.varexists(chaozardefeated)%
  if !%actor.varexists(chaozarreward)%
   set chaozarreward
   remote chaozarreward %actor.id%
   wait 1 s
   say Congratulations on defeating Chaozar. Here is your reward.
   %load% obj 2455
   give blackfyre %actor.name%
  else
   wait 1 s
   say You already claimed your reward!
  end
 end
elseif (%speech% == spear)
 if !%actor.varexists(chaozardefeated)%
  wait 1 s
  say You need to defeat the Weapon Master Chaozar, before I can reward you.
 elseif %actor.varexists(chaozardefeated)%
  if !%actor.varexists(chaozarreward)%
   set chaozarreward
   remote chaozarreward %actor.id%
   wait 1 s
   say Congratulations on defeating Chaozar. Here is your reward.
   %load% obj 2456
   give gungnir %actor.name%
  else
   wait 1 s
   say You already claimed your reward!
  end
 end
elseif (%speech% == club)
 if !%actor.varexists(chaozardefeated)%
  wait 1 s
  say You need to defeat the Weapon Master Chaozar, before I can reward you.
 elseif %actor.varexists(chaozardefeated)%
  if !%actor.varexists(chaozarreward)%
   set chaozarreward
   remote chaozarreward %actor.id%
   wait 1 s
   say Congratulations on defeating Chaozar. Here is your reward.
   %load% obj 2457
   give skullcrusher %actor.name%
  else
   wait 1 s
   say You already claimed your reward!
  end
 end
elseif (%speech% == dagger)
 if !%actor.varexists(chaozardefeated)%
  wait 1 s
  say You need to defeat the Weapon Master Chaozar, before I can reward you.
 elseif %actor.varexists(chaozardefeated)%
  if !%actor.varexists(chaozarreward)%
   set chaozarreward
   remote chaozarreward %actor.id%
   wait 1 s
   say Congratulations on defeating Chaozar. Here is your reward.
   %load% obj 2458
   give mcstabbington %actor.name%
  else
   wait 1 s
   say You already claimed your reward!
  end
 end
elseif (%speech% == gun)
 if !%actor.varexists(chaozardefeated)%
  wait 1 s
  say You need to defeat the Weapon Master Chaozar, before I can reward you.
 elseif %actor.varexists(chaozardefeated)%
  if !%actor.varexists(chaozarreward)%
   set chaozarreward
   remote chaozarreward %actor.id%
   wait 1 s
   say Congratulations on defeating Chaozar. Here is your reward.
   %load% obj 2459
   give deathwind %actor.name%
  else
   wait 1 s
   say You already claimed your reward!
  end
 end
else
 wait 1 s
 say Huh?
end
~
#2411
Alexia Greet~
0 g 100
~
if !%actor.varexists(chaozardefeated)%
 wait 2 s
 say Hi there, welcome to the Amnu-Nation Weapon Club.
 wait 2 s
 say If you defeat our reigning weapon master, I will reward you with a weapon of your choice.
wait 3 s
say I'll also be able to show you a thing or two about weapon handling if you are interested right now.
elseif (%actor.varexists(chaozardefeated)% && !%actor.varexists(chaozarreward)%
 wait 2 s
 say Seems like you've beaten our reigning Weapon Master, Chaozar.
 wait 2 s
 say To claim your reward, say which weapon type you would like; sword, spear, gun, dagger or club.
elseif (%actor.varexists(chaozarreward)%)
 wait 2 s
 say Sorry hun, I got nothing else for you.
end
~
#2412
Alexia Train~
0 c 100
pr~
eval pass 0
if (%actor.skill(sword)% >= 50)
 eval pass %pass% + 1
end
if (%actor.skill(gun)% >= 50)
 eval pass %pass% + 1
end
if (%actor.skill(dagger)% >= 50)
 eval pass %pass% + 1
end
if (%actor.skill(spear)% >= 50)
 eval pass %pass% + 1
end
if (%actor.skill(club)% >= 50)
 eval pass %pass% + 1
end
if (%actor.skill(brawl)% >= 50)
 eval pass %pass% + 1
end
if (%pass% < 1)
 return 1
 wait 1
 say You don't even know anything about any weapon skills. Why are you even here?
elseif (%pass% < 2)
 return 1
 wait 1
 say Sorry, but you need to at least have a good amount of knowledge in two weapon skills.
 %send% %actor% @WOOC@D: @wYou need two weapon skills to at least SLVL 50.@n
else
 return 0
end
~
#2413
Gastron Speech~
0 c 100
worth~
if !%actor.varexists(weaponring)%
  wait 2 s
  say So you want to enter our secret club, eh? Fine.
  set weaponring start
  remote weaponring %actor.id%
  wait 2 s
  say Write down these instructions or something, because there's no way I am repeating myself. Got it?
  wait 2 s
  say Five rings of mastery you must bring me. Frozen, Swift, Fury, Storm and Tempered.
else
  wait 2 s
  say Damnit!  You idiot!  @RPay attention!@C
  wait 2s
  say Five rings of mastery you must bring me. Frozen, Swift, Fury, Storm and Tempered.  
end
~
$~
