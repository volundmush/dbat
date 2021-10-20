#14800
Examine Hyper Dial~
1 c 100
examine~
if %arg% == dial
  if %actor.race% == Truffle
    wait 1 sec
    %send% %actor% You examine the Hyper Dial device.
    %echoaround% %actor% %actor.name% examines the Hyper Dial device.
    wait 2 sec
    %send% %actor% @YDestinations@n
    %send% %actor% @GSS Neo Nirvana@n: push ariescancerbootesdracolyraorion
    %send% %actor% @GEarth@n: push cancerarieslyrabootesdracoorion
    %send% %actor% @GVegeta@n: push lyrabootesariesdracocancerorion
    %send% %actor% @GNamek@n: push bootesdracocancerarieslyraorion
    %send% %actor% @GKonack@n: push dracolyracancerbootesariesorion
    %send% %actor% @GAether@n: push ariesbooteslyradracocancerorion
    %send% %actor% @GFrigid@n: push booteslyradracocancerariesorion
    wait 15 sec
    %send% %actor% Please note that only the Hyper Gate on Neo Nirvana has free access to the Network. All other gates only possess the ability to dial Neo Nirvana.
  else
    wait 1 sec
    %send% %actor% You examine the Hyper Dial device but cannot understand the symbols upon it's surface.
    %echoaround% %actor% %actor.name% examines the Hyper Dial device with a puzzled look.
  end
else
  %send% %actor% Try examining the dial instead!
end
~
#14801
Dial the Gate~
1 c 100
push~
if %arg% == ariescancerbootesdracolyraorion
  if %actor.race% == Truffle
    if %self.room.vnum% == 14834
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You are already on Neo Nirvana. This won't work.
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
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive on Neo Nirvana!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 14834
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else
        wait 1 sec
        %send% %actor% The device is already in use.
      end
    end
  else
    %send% %actor% The device does not respond to you!
  end
elseif %arg% == cancerarieslyrabootesdracoorion
  if %actor.race% == Truffle
    if %self.room.vnum% != 14834
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You cannot dial that location from here.
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
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive on Earth!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 1420
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else 
        %send% %actor% The device is already in use.
      end
    end
  else
    %send% %actor% The device does not respond to you!
  end
elseif %arg% == lyrabootesariesdracocancerorion
  if %actor.race% == Truffle
    if %self.room.vnum% != 14834
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You cannot dial that location from here.
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
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive on Vegeta!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 2879
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else 
        %send% %actor% The device is already in use.
      end
    end
  else
    %send% %actor% The device does not respond to you!
  end
elseif %arg% == bootesdracocancerarieslyraorion
  if %actor.race% == Truffle
    if %self.room.vnum% != 14834
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You cannot dial that location from here.
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
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive on Namek!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 10983
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else 
        %send% %actor% The device is already in use.
      end
    end
  else
    %send% %actor% The device does not respond to you!
  end
elseif %arg% == dracolyracancerbootesariesorion
  if %actor.race% == Truffle
    if %self.room.vnum% != 14834
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You cannot dial that location from here.
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
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive on Konack!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 8968
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else 
        %send% %actor% The device is already in use.
      end
    end
  else
    %send% %actor% The device does not respond to you!
  end
elseif %arg% == ariesbooteslyradracocancerorion
  if %actor.race% == Truffle
    if %self.room.vnum% != 14834
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You cannot dial that location from here.
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
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive on Aether!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 12340
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else 
        %send% %actor% The device is already in use.
      end
    end
  else
    %send% %actor% The device does not respond to you!
  end
elseif %arg% == booteslyradracocancerariesorion
  if %actor.race% == Truffle
    if %self.room.vnum% != 14834
      if !%actor.varexists(dialinghypergate)%
        %send% %actor% You cannot dial that location from here.
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
        %send% %actor% You walk through the Event Horizon and after a brief moment inside the wormhole, arrive on Frigid!
        %echoaround% %actor% %actor.name% walks through the Event Horizon and disappears!
        %teleport% %actor% 4924
        %force% %actor.name% look
        rdelete dialinghypergate %actor.id%
      else 
        %send% %actor% The device is already in use.
      end
    end
  else
    %send% %actor% The device does not respond to you!
  end
else
  %send% %actor% Try entering a correct sequence!
end
~
#14802
Block the dialer~
1 q 100
~
if %actor.varexists(dialinghypergate)%
  %send% %actor% You can't leave while the gate is dialing!
  return 0
end
~
#14803
Valeria Greet~
0 g 100
~
if %actor.is_pc%
wait 2 s
say Hi there! Welcome to Neo Nirvana.
wait 2 s
say If you need a keycard, see Alexej in the Storage Room.
end
~
#14804
Alpha and Omega Fight trigger~
0 k 15
~
final %actor.name%
~
#14805
Bebi Fight Trigger~
0 k 25
~
eval tactics %random.3%
switch %tactics%
  case 1
    wait 1 sec
    emote chuckles maniacally and says, 'Revenge Deathball!'
    wait 2 sec
    deathball %actor.name%
  break
  case 2
    wait 1 sec
    emote summons an enthralled scientist to come to his aid!
    %load% mob 14814
  break
  default
    %echo% Nano-bots repair damage Bebi has sustained in battle.
    nop %self.hitp(50000000)%
  break
done
~
#14806
Receptionist speaks~
0 g 100
~
if %actor.is_pc%
wait 2 s
say You'll need a C class keycard to enter the labs. Boris over at the Fortran Complex might have one.
end
~
$~
