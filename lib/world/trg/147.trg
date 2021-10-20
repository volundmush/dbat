#14700
Moisey sits~
0 n 100
~
sit couch
~
#14701
Nevena rests~
0 n 100
~
rest pod
~
#14702
Neo Quest Start~
0 g 100
~
if (%actor.varexists(neoquest1)%)
  set neoquest %actor.neoquest1%
else
  set neoquest unstarted
end
if (%neoquest% == unstarted)
  wait 2s
  emote wipes his head with his forearm and frowns.  He paces back and forth a couple times before yelling and stomping one tiny foot on the floor.  @C'I'm so sick of living on a ship.  I want to live on a planet somewhere and breathe fresh air!'@n
  wait 3s
  emote sighs and turns around, noticing *%actor.name% for the first time.  @C'Oh.  Hello there.  I guess you heard that, huh?'@n
  wait 4s
  emote smiles nervously.  @C'Well, it's not really a secret.  I'm tired of living on this ship.  Do you think you could help me...?'@n
else
  wait 2s
  emote looks up at you and smiles.  @C'Do you have the %actor.neoquest1%?'@n
end
~
#14703
Neo Quest Accept~
0 d 100
yes~
if (%actor.varexists(neoquest1)%)
  set neoquest %actor.neoquest1%
else
  set neoquest unstarted
end
if (%neoquest% != unstarted)
  wait 3s
  emote raises an eyebrow and looks at *%actor.name%.  @C'Yes what?'@n
else
  wait 3s
  emote claps his hands and lets out an explosive breath.  @C'YES!  Finally!  I can't wait until I get off this damn ship!'@n
  wait 3s
  emote taps his chin and ponders for a few seconds.  @C'Okay... let's see.  I need to build me an escape pod.  We truffles are excellent builders, you know.'@n
  wait 3s
  emote smiles and taps on his chest.  @C'Don't let our sizes fool you!'@n  He stares at you for a moment before continuing.
  wait 3s
  emote slaps himself in the face. @C'Oh, right!  Yes.  Well, let's see.  Get me some cybernetic limb circuits.  I can use the wires in those for my escape pod.'@n
  set neoquest1 circuits
  remote neoquest1 %actor.id%
end
~
#14704
Neo Quest Receive~
0 j 100
~
if (%object.vnum% == 43)
  wait 1s
  emote looks at the %object.name%.  @C'Perfect!  Just what I needed.  Come back later and I'll see what else I need.'@n
end
~
#14790
Resize Trigger~
0 j 100
~
**************************************************
* Resize Trigger redone by Sedalis
**************************************************
eval add %object.level% * 50
eval levelcost %object.cost% / 20
eval resizeprice %add% + %levelcost%
if (%object.type% != WORN && %object.type% != LIGHT && %object.type% != WEAPON && %object.type% != CONTAINER && %object.vnum% != 17006)
  return 0
  say This I can't resize.
else
  if (%actor.gold% < %resizeprice%)
    return 0
    say I don't do charity work.  Resizing that item will cost you %resizeprice% zenni.
  elseif (%actor.size% == %object.size%)
    return 0
    say It is already your size.
  else
    nop %actor.gold(-%resizeprice%)%
    wait 1
    say This will only take a moment.
    wait 1 s
    %echo% %self.name% takes %object.shortdesc% and places it inside a strange capsule. Then %self.name% pushes a button on the capsule.
    wait 4 s
    %echo% Green light shines down on %object.shortdesc% inside the capsule chamber.
    nop %object.size(%actor.size%)%
    wait 2 s
    %echo% A ding is heard and the green light disappears.
    wait 2 s
    %echo% %self.name% grabs %object.shortdesc%.
    wait 1 s
    give all %actor.name%
    wait 1 s
    say There you are.  I have resized it for you at a cost of %resizeprice% zenni.
  end
end
~
#14791
Copy of Resize Trigger (Needed fixed badly)~
0 j 100
~
if (%object.type% != WORN && %object.type% != LIGHT && %object.type% != WEAPON && %object.type% != CONTAINER)
  return 0
  say This I can't resize.
else
  eval add %object.level% * 50
  if (%actor.gold% < (%object.cost% / 5) + add)
    return 0
    say You don't have enough money. You need to be able to at least pay 5% of the item's cost plus 2500 zenni.
  elseif (%actor.size% == %object.size%)
    return 0
    say It is already your size.
  else
    eval deduct (%object.cost% / 5) + add
    nop %actor.gold(-%deduct%)%
    wait 1
    say This will only take a moment.
    wait 1 s
    %echo% %self.name% takes %object.shortdesc% and places it inside a strange capsule. Then %self.name% pushes a button on the capsule.
    wait 4 s
    %echo% Green light shines down on %object.shortdesc% inside the capsule chamber.
    nop %object.size(%actor.size%)%
    wait 2 s
    %echo% A ding is heard and the green light disappears.
    wait 2 s
    %echo% %self.name% grabs %object.shortdesc%.
    wait 1 s
    give all %actor.name%
    wait 1 s
    say There you are. It has been resized.
  end
end
~
$~
