#15600
The Kraken Flies!~
1 ab 20
~
eval move %random.4%
 switch (%move%)
  case 1
   if (%self.room.north%)
    wait 1
    %echo% %self.name% flies north.
    wait 1
    ogoto %self.room.north(vnum)%
   end
  break
  case 2
   if (%self.room.east%)
    wait 1
    %echo% %self.name% flies east.
    wait 1
    ogoto %self.room.east(vnum)%
   end
  break
  case 3
   if (%self.room.south%)
    wait 1
    %echo% %self.name% flies south.
    wait 1
    ogoto %self.room.south(vnum)%
   end
  break
  case 4
   if (%self.room.west%)
    wait 1
    %echo% %self.name% flies west.
    wait 1
    ogoto %self.room.west(vnum)%
   end
  break
 done
~
#15601
Unlocker Minion Kraken~
0 n 100
~
wait 1
%load% obj 15600
unlock airlock
mgoto 53340
close ship
mgoto 49
~
#15602
Dial Blocker~
1 q 100
~
if %actor.varexists(dialinghypergate)%
  %send% %actor% You can't leave while the gate is dialing!
  return 0
end
~
#15603
Hyper Gate Dialer (Yunkai)~
1 c 100
dial~
if %arg% == corvislepuscalumarahydraorion
    if %self.room.vnum% == 15685
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You are already at the Yunkai Pirates base.
      else
       %send% %actor% The device is already in use.
      end
    else
      if !%actor.varexists(dialinghypergate)%
        wait 1 sec
        set dialinghypergate
        remote dialinghypergate %actor.id%
        %send% %actor% You push several buttons on the Hyper Dial device
        %echoaround% %actor% %actor.name% pushes several buttons on the Hyper Dial device. 
        wait 2 sec
        %echo% Bolts of electricity jump about the Hyper Gate as the inner ring begins spinning around.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the first symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the second symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the third symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the fourth symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the fifth symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the final symbol, causing it to glow red.
        wait 2 sec
        %echo% A massive burst of energy erupts from the Hyper Gate as it forms an event horizon.
        wait 2 sec
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive on Kanassa!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 15685
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else
        wait 1 sec
        %send% %actor% The device is already in use.
      end
    end
elseif %arg% == leohydrapiscesgrusvirgoorion
    if %self.room.vnum% == 15622
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You are already aboard the Kraken.
      else
        %send% %actor% The device is already in use.
      end
    else
      if !%actor.varexists(dialinghypergate)%
        wait 1 sec
        set dialinghypergate
        remote dialinghypergate %actor.id%
        %send% %actor% You push several buttons on the Hyper Dial device
        %echoaround% %actor% %actor.name% pushes several buttons on the Hyper Dial device. 
        wait 2 sec
        %echo% Bolts of electricity jump about the Hyper Gate as the inner ring begins spinning around.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the first symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the second symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the third symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the fourth symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the fifth symbol, causing it to glow red, before continuing to spin.
        wait 2 sec
        %echo% The inner ring comes to a stop as it locks in the final symbol, causing it to glow red.
        wait 2 sec
        %echo% A massive burst of energy erupts from the Hyper Gate as it forms an event horizon.
        wait 2 sec
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive aboard the Kraken!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 15622
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else 
        %send% %actor% The device is already in use.
      end
    end
else
  %send% %actor% Try entering a correct sequence!
end
~
#15604
Euron greet~
0 g 100
~
if (!%actor.varexists(yunkaistarted)% && !%actor.varexists(yunkaifinished)%)
  if %actor.level% > 90
    wait 2 s
    say Ho there %actor.race%, I've got some work for you.. if you'll hear me out. Just say yes..
  else
    wait 2 s
    say Ho there %actor.race%, pleasant day to you.
  end
else
  wait 2 s
  say Ho there %actor.race%, pleasant day to you.
end
~
#15605
Euron Gives Quest~
0 d 100
yes~
if (!%actor.varexists(yunkaistarted)% && !%actor.varexists(yunkaifinished)%)
  if %actor.level% > 90
    set yunkaistarted
    remote yunkaistarted %actor.id%
    wait 2 s
    say Look %actor.race%, I'll speak plainly. I used to run with the Yunkai pirates. An infamous lot who only raid World Trade Organization holdings.
    wait 4 s
    say Their leader, Captain Dread banished me however after I tried to convince them to broaden the spectrum so to speak.
    wait 4 s
    say The old fart may have founded the Yunkai pirates after Kanassa was devasted by the World Trade Organization, but let me tell you this, %actor.race%..
    wait 4 s
    say They could be a lot more profitable than what they are presently.
    wait 4 s
    say And I, Euron the Crow, intend to put the Yunkai on their proper path.
    wait 4 s
    say With your help, of course. I need to you to kill Captain Dread and bring me his spear named Dread Spear. Original I know...
    wait 4 s
    emote chuckles.
    wait 2 s
    say I would do it myself, alas the boys know who I am and will surely make certain I never get close to my goal.
    wait 4 s
    say With the Captain dead though and the spear in my possession, few will deny that Euron the Crow is the new captain.
    wait 4 s
    say Captain Dread rarely leaves his flagship, the Kraken, though. You'll have to assault the base out in the ocean.
    wait 4 s
    say Limmy will have a security card that will grant you access to the Kraken.
    wait 4 s
    say It will be no easy feat, but if you bring me the spear, I will pay you 30,000 zenni. A handsome fee I'm sure you'll agree.
    wait 4 s
    say Now begone, I am eager to begin my plundering as Captain Euron!
  else
    wait 2 s
    say You are much too weak to help me, %actor.race%.
  end
else
  emote nods his head.
end
~
#15606
Captain Dread drops spear~
0 f 100
~
if (%actor.varexists(yunkaistarted)% && !%actor.varexists(dreadspear)%)
  set dreadspear
  remote dreadspear %actor.id%
  %load% obj 15607
  %load% obj 15606
end 
~
#15607
Captain Dread fight~
0 k 10
~
wait 2 s
eval speech %random.3
  switch %speech%
    case 1
      say I'm going to gut you!
      break
    case 2
      say I'll loot your corpse when you are dead!
      break
    default
      say Captain Dread is going to put you down like the animal that you are!
      break
done
~
#15608
Euron Receives~
0 j 100
~
if (%actor.varexists(dreadspear)% && %object.vnum% == 15607)
  rdelete dreadspear %actor.id%
  rdelete yunkaistarted %actor.id%
  set yunkaifinished
  remote yunkaifinished %actor.id%
  %purge% %object%
  wait 2 s
  say You've done it! The Dread Spear! With this I can command the Yunkai pirates.
  wait 2 s
  say Euron the Crow rewards those who serve him well. And you my friend..
  wait 2
  say You've earned this 30,000 zenni.
  wait 2 s
  nop %actor.gold(30000)%
  emote hands you a rather large sack of zenni.
  wait 2 s
  say Pleasure doing business with you.
else
  return 0
  wait 2 s
  say I don't want that!
end
~
#15609
Yunkai guard~
0 q 100
~
if (%direction% == north)
   say Piss off mate. This here is Yunkai Pirate territory.
   %send% %actor.name% %self.name% prevents you from going any further.
   %echoaround% %actor.name% As %actor.name% tries to enter the compound %self.name% stops %actor.himher%
return 0
else
return 1
end
~
$~
