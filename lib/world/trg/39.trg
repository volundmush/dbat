#3900
Falcon Ship Controller~
1 b 100
swipe~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Earth (load location) 
*----
if (!%falconlocation%)
  set falconlocation Earth
  global falconlocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Earth
set firstplanetport 408
set firstplanetorbit 50
set secondplanetname Namek
set secondplanetport 11626
set secondplanetorbit 54
set thirdplanetname SpaceHub
set thirdplanetport 16300
set thirdplanetorbit 33385
set room1 3900
set room2 3901
set room3 3902
set room4 3903
set room5 3904
set shipname Falcon
set route1s1 35780
set route1s2 35751
set route1s3 35687
set route1s4 40475
set route2s1 42899
set route2s2 42923
set route2s3 42956
set route2s4 42975
set route3s1 37686
set route3s2 33286
set route3s3 33336
set route3s4 33365
set route4s1 35385
set route4s2 37385
set route4s3 39014
set route4s4 40182
*----
*---- Now, travel from Earth to Namek, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 3 || %time.hour% == 6 || %time.hour% == 9 || %time.hour% == 12 || %time.hour% == 15 || %time.hour% == 18 || %time.hour% == 21)
  if (%self.room.vnum% == %firstplanetport%)
    set falconlocation %firstplanetname%
    global falconlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set falconlocation Departing from %firstplanetname%
    global falconlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set falconlocation In transit to %secondplanetname%
    global falconlocation
    *---- Start of Travels
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set falconlocation Landing on %secondplanetname%
    global falconlocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the planet.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set falconlocation %secondplanetname%
    global falconlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, travel from Namek to Space Hub, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 5 || %time.hour% == 8 || %time.hour% == 11 || %time.hour% == 14 || %time.hour% == 17 || %time.hour% == 20 || %time.hour% == 23)
  if (%self.room.vnum% == %secondplanetport%)
    set falconlocation %secondplanetname%
    global falconlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %thirdplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %thirdplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    ogoto %secondplanetorbit%
    set falconlocation Departing from %secondplanetname%
    global falconlocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %thirdplanetorbit%
    set falconlocation Landing on %thirdplanetname%
    global falconlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the planet.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %thirdplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    ogoto %thirdplanetport%
    wait 1
    set falconlocation %thirdplanetname%
    global falconlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, travel from the Space Hub to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 4 || %time.hour% == 7 || %time.hour% == 10 || %time.hour% == 13 || %time.hour% == 16 || %time.hour% == 19 || %time.hour% == 22 || %time.hour% == 1)
  if (%self.room.vnum% == %thirdplanetport%)
    set falconlocation %thirdplanetname%
    global falconlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %thirdplanetorbit%
    set falconlocation Departing from %fourthplanetname%
    global falconlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route4s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set falconlocation In transit to %firstplanetname%
    global falconlocation
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route4s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route4s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting 
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route4s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %firstplanetorbit%
    set falconlocation Landing on %firstplanetname%
    global falconlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the planet.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set falconlocation %firstplanetname%
    global falconlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3901
Falcon Travel Controls~
1 ab 100
~
*
~
#3902
Falcon Board Script~
1 ac 100
boardfalcon~
*@Y---- @cThis command is to be attached to all public ships@n
*@Y---- @cand will use variables to determine what ship@n
*@Y---- @cis being boarded and what to do about it@n\
*@Y----@n
*@Y----@g First, figure out which ship this is and set variables@n
*@Y----@n
if (%self.vnum% == 3900)
  set shipname Falcon
  set shipinside 3900
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*@Y---------------------@n
*@Y----@g Now that we have th4s ship and vars, GET IN MY BELLY!
*@Y---------------------@n
if (%failure% == Pass)
  %echoaround% %actor% @R%actor.name% holds a ticket to a scanner on the %shipname%@R, then boards.@n
  %send% %actor% @RYou hold your ticket to a scanner on the %shipname%@R, then board.@n
  %teleport% %actor% %shipinside%
  wait 1
  %force% %actor% look
  %echoaround% %actor% @R%actor.name% boards the %shipname%@R.@n
  %purge% %actor.inventory(3999)%
else
  %echoaround% %actor% @R%actor.name% tries to board the %shipname% but is rejected with an error beep.@n
  %send% %actor% @RYou cannot board the %shipname%@R %failure%!@n
end  
~
#3903
Depart script~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== Falcon
if (%self.vnum% == 3900)
  if (%findobj.408(3900)%)
    set departroom 408
  elseif (%findobj.11626(3900)%)
    set departroom 11626
  elseif (%findobj.16300(3900)%)
set departroom 16300
  else
    set departroom fail
  end
  set shipname Falcon
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3904
Ship Depart Script~
2 g 100
depart~
*@Y----@c This script is the departure script for ships.@n
*@Y----@c It should be attached to all public ships.
*@Y----@n
*@Y----@g First, find this ship and set vars...
*@Y----@n
if (%self.vnum% == 3900)
  set shipname Falcon
  set shipvnum 3900
  *@Y---@g Dest Earth@n
  set dest1 408
  *@Y---@g Dest Vegeta@n
  set dest2 2323
  *@Y---@g Dest Namek@n
  set dest3 11626
end
*@Y---@n
*@Y---@g Now, find where the ship is and set vars...
*@Y---@n
set departureroom Fail
if (%%findobj.%shipvnum%(%dest1%)%%)
  set departureroom %dest1%
end
if (%%findobj.%shipvnum%(%dest2%)%%)
  set departureroom %dest2%
end
if (%%findobj.%shipvnum%(%dest3%)%%)
  set departureroom %dest3%
end
%echo% %departureroom% <---- The room.
~
#3905
Simurgh Ship Course Script~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Aether (load location) 
*----
if (!%simurghlocation%)
  set simurghlocation Aether
  global simurghlocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Aether
set firstplanetport 12002
set firstplanetorbit 55
set secondplanetname Konack
set secondplanetport 8194
set secondplanetorbit 52
set thirdplanetname SpaceHub
set thirdplanetport 16300
set thirdplanetorbit 33385
set room1 3905
set room2 3997
set room3 3998
set room4 3999
set room5 3996
set shipname Simurgh
set route1s1 35781
set route1s2 35752
set route1s3 35681
set route1s4 40471
set route2s1 32341
set route2s2 35141
set route2s3 37652
set route2s4 37671
set route3s1 29055
set route3s2 32144
set route3s3 33351
set route3s4 33370
set route4s1 33365
set route4s2 33339
set route4s3 33304
set route4s4 40344
*----
*---- Now, travel from first planet to secod planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 3 || %time.hour% == 6 || %time.hour% == 9 || %time.hour% == 12 || %time.hour% == 15 || %time.hour% == 18 || %time.hour% == 21)
  if (%self.room.vnum% == %firstplanetport%)
    set simurghlocation %firstplanetname%
    global simurghlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set simurghlocation Departing from %firstplanetname%
    global simurghlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set simurghlocation In transit to %secondplanetname%
    global simurghlocation
    *---- Start of Travels
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set simurghlocation Landing on %secondplanetname%
    global simurghlocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the planet.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set simurghlocation %secondplanetname%
    global simurghlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, travel from second planet to third planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 5 || %time.hour% == 8 || %time.hour% == 11 || %time.hour% == 14 || %time.hour% == 17 || %time.hour% == 20 || %time.hour% == 23)
  if (%self.room.vnum% == %secondplanetport%)
    set simurghlocation %secondplanetname%
    global simurghlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %thirdplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %thirdplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    ogoto %secondplanetorbit%
    set simurghlocation Departing from %secondplanetname%
    global simurghlocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2% %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %thirdplanetorbit%
    set simurghlocation Landing on %thirdplanetname%
    global simurghlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking in the hangar bay of the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of the %thirdplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    ogoto %thirdplanetport%
    wait 1
    set simurghlocation Namek
    global simurghlocation
    %echo% %self.shortdesc% docks in the hangar bay of the %thirdplanetname%.
  end
end
*----
*---- Now, travel from third planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 4 || %time.hour% == 7 || %time.hour% == 10 || %time.hour% == 13 || %time.hour% == 16 || %time.hour% == 19 || %time.hour% == 22 || %time.hour% == 1)
  if (%self.room.vnum% == %thirdplanetport%)
    set simurghlocation %thirdplanetname%
    global simurghlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %thirdplanetorbit%
    set simurghlocation Departing from %thirdplanetname%
    global simurghlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route4s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set simurghlocation In transit to %firstplanetname%
    global simurghlocation
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route4s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route4s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route4s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %firstplanetorbit%
    set simurghlocation Landing on %firstplanetname%
    global simurghlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the planet.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set simurghlocation %firstplanetname%
    global simurghlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3906
Zypher Ship Course Script~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Frigid(load location) 
*----
if (!%zypherlocation%)
  set zypherlocation Frigid
  global zypherlocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Frigid
set firstplanetport 4250
set firstplanetorbit 51
set secondplanetname SpaceHub
set secondplanetport 16300
set secondplanetorbit 33385
set thirdplanetname Space Mall
set thirdplanetport 17200
set thirdplanetorbit 41490
set room1 3906
set room2 3907
set room3 3908
set room4 3909
set room5 3910
set shipname Zypher
set route1s1 32961
set route1s2 33333
set route1s3 33570
set route1s4 33370
set route2s1 32343
set route2s2 35143
set route2s3 37653
set route2s4 37673
set route3s1 42893
set route3s2 42923
set route3s3 42953
set route3s4 42973
*----
*---- Now, travel from first planet to secod planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 3 || %time.hour% == 6 || %time.hour% == 9 || %time.hour% == 12 || %time.hour% == 15 || %time.hour% == 18 || %time.hour% == 21)
  if (%self.room.vnum% == %firstplanetport%)
    set zypherlocation %firstplanetname%
    global zypherlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set zypherlocation Departing from %firstplanetname%
    global zypherlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set zypherlocation In transit to %secondplanetname%
    global zypherlocation
    *---- Start of Travels
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set zypherlocation Landing on %secondplanetname%
    global zypherlocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking on the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set zypherlocation %secondplanetname%
    global zypherlocation
    %echo% %self.shortdesc% docks in the hangar bay of the Space Hub.
  end
end
*----
*---- Now, travel from second planet to third planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 5 || %time.hour% == 8 || %time.hour% == 11 || %time.hour% == 14 || %time.hour% == 17 || %time.hour% == 20 || %time.hour% == 23)
  if (%self.room.vnum% == %secondplanetport%)
    set zypherlocation %secondplanetname%
    global zypherlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %thirdplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %thirdplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    ogoto %secondplanetorbit%
    set zypherlocation Departing from %secondplanetname%
    global zypherlocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed,  
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %thirdplanetorbit%
    set zypherlocation Landing on %thirdplanetname%
    global zypherlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the complex.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %thirdplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %thirdplanetname%.
    ogoto %thirdplanetport%
    wait 1
    set zypherlocation Namek
    global zypherlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, travel from third planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 4 || %time.hour% == 7 || %time.hour% == 10 || %time.hour% == 13 || %time.hour% == 16 || %time.hour% == 19 || %time.hour% == 23 || %time.hour% == 1)
 if (%self.room.vnum% == %thirdplanetport%)
    set zypherlocation %thirdplanetname%
    global zypherlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %thirdplanetorbit%
    set zypherlocation Departing from %thirdplanetname%
    global zypherlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route3s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set zypherlocation In transit to %firstplanetname%
    global zypherlocation
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route3s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route3s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route3s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %firstplanetorbit%
    set zypherlocation Landing on %firstplanetname%
    global zypherlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the planet.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set zypherlocation %firstplanetname%
    global zypherlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3907
Zypher Depart Script~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== Zypher
if (%self.vnum% == 3906)
  if (%findobj.4250(3906)%)
    set departroom 4250
  elseif (%findobj.16300(3906)%)
    set departroom 16300
  elseif (%findobj.17200(3906)%)
    set departroom 17200
  else
    set departroom fail
  end
  set shipname Zypher
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3908
Zypher Board Script~
1 c 100
boardzypher~
*@Y---- @cThis command is to be attached to all public ships@n
*@Y---- @cand will use variables to determine what ship@n
*@Y---- @cis being boarded and what to do about it@n\
*@Y----@n
*@Y----@g First, figure out which ship this is and set variables@n
*@Y----@n
if (%self.vnum% == 3906)
  set shipname Zypher
  set shipinside 3906
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*@Y---------------------@n
*@Y----@g Now that we have th4s ship and vars, GET IN MY BELLY!
*@Y---------------------@n
if (%failure% == Pass)
  %echoaround% %actor% @R%actor.name% holds a ticket to a scanner on the %shipname%@R, then boards.@n
  %send% %actor% @RYou hold your ticket to a scanner on the %shipname%@R, then board.@n
  %teleport% %actor% %shipinside%
  wait 1
  %force% %actor% look
  %echoaround% %actor% @R%actor.name% boards the %shipname%@R.@n
  %purge% %actor.inventory(3999)%
else
  %echoaround% %actor% @R%actor.name% tries to board the %shipname% but is rejected with an error beep.@n
  %send% %actor% @RYou cannot board the %shipname%@R %failure%!@n
end  
~
#3911
Valkyrie Script Scheduler~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Vegeta (load location) 
*----
if (!%valkyrielocation%)
  set valkyrielocation Vegeta
  global valkyrielocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Vegeta
set firstplanetport 2323
set firstplanetorbit 53
set secondplanetname Earth
set secondplanetport 408
set secondplanetorbit 50
set thirdplanetname SpaceHub
set thirdplanetport 16300
set thirdplanetorbit 33385
set room1 3911
set room2 3912
set room3 3913
set room4 3914
set room5 3915
set shipname Valkyrie
set route1s1 42974
set route1s2 42954
set route1s3 42924
set route1s4 42894
set route2s1 40182
set route2s2 39014
set route2s3 37385
set route2s4 35385
set route3s1 33369
set route3s2 33350
set route3s3 33311
set route3s4 32355
*----
*---- Now, travel from first planet to second planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 3 || %time.hour% == 6 || %time.hour% == 9 || %time.hour% == 12 || %time.hour% == 15 || %time.hour% == 18 || %time.hour% == 21)
  if (%self.room.vnum% == %firstplanetport%)
    set valkyrielocation %firstplanetname%
    global valkyrielocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set valkyrielocation Departing from %firstplanetname%
    global valkyrielocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set valkyrielocation In transit to %secondplanetname%
    global valkyrielocation
    *---- Start of Travels
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set valkyrielocation Landing on %secondplanetname%
    global valkyrielocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the planet.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set valkyrielocation %secondplanetname%
    global valkyrielocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, travel from second planet to third planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 5 || %time.hour% == 8 || %time.hour% == 11 || %time.hour% == 14 || %time.hour% == 17 || %time.hour% == 20 || %time.hour% == 23)
  if (%self.room.vnum% == %secondplanetport%)
    set valkyrielocation %secondplanetname%
    global valkyrielocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %thirdplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %thirdplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %thirdplanetname%.
    ogoto %secondplanetorbit%
    set valkyrielocation Departing from %secondplanetname%
    global valkyrielocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 20s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %thirdplanetorbit%
    set valkyrielocation Docking in %thirdplanetname%
    global valkyrielocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking in the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of %thirdplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %thirdplanetname%.
    ogoto %thirdplanetport%
    wait 1
    set valkyrielocation %thirdplanetname%
    global valkyrielocation
    %echo% %self.shortdesc% docks in the hangar bay of the Space Hub.
  end
end
*----
*---- Now, travel from third planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 4 || %time.hour% == 7 || %time.hour% == 10 || %time.hour% == 13 || %time.hour% == 16 || %time.hour% == 19 || %time.hour% == 22 || %time.hour% == 1)
  if (%self.room.vnum% == %thirdplanetport%)
    set valkyrielocation %thirdplanetname%
    global valkyrielocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %thirdplanetorbit%
    set valkyrielocation Departing from %thirdplanetname%
    global valkyrielocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route3s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set valkyrielocation In transit to %firstplanetname%
    global valkyrielocation
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route3s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route3s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route3s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 30s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %firstplanetorbit%
    set valkyrielocation Landing on %firstplanetname%
    global valkyrielocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the planet.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set valkyrielocation %firstplanetname%
    global valkyrielocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3912
Valkyrie Board Trigger~
1 c 100
boardvalkyrie~
*@Y---- @cThis command is to be attached to all public ships@n
*@Y---- @cand will use variables to determine what ship@n
*@Y---- @cis being boarded and what to do about it@n\
*@Y----@n
*@Y----@g First, figure out which ship this is and set variables@n
*@Y----@n
if (%self.vnum% == 3911)
  set shipname Valkyrie
  set shipinside 3911
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*@Y---------------------@n
*@Y----@g Now that we have th4s ship and vars, GET IN MY BELLY!
*@Y---------------------@n
if (%failure% == Pass)
  %echoaround% %actor% @R%actor.name% holds a ticket to a scanner on the %shipname%@R, then boards.@n
  %send% %actor% @RYou hold your ticket to a scanner on the %shipname%@R, then board.@n
  %teleport% %actor% %shipinside%
  wait 1
  %force% %actor% look
  %echoaround% %actor% @R%actor.name% boards the %shipname%@R.@n
  %purge% %actor.inventory(3999)%
else
  %echoaround% %actor% @R%actor.name% tries to board the %shipname% but is rejected with an error beep.@n
  %send% %actor% @RYou cannot board the %shipname%@R %failure%!@n
end  
~
#3913
Valkyrie Depart Script~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== Valkyrie
if (%self.vnum% == 3911)
  if (%findobj.2323(3911)%)
    set departroom 2323
  elseif (%findobj.408(3911)%)
    set departroom 408
  elseif (%findobj.16300(3911)%)
    set departroom 16300
  else
    set departroom fail
  end
  set shipname Valkyrie
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3916
Pheonix Control Script~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Earth(load location) 
*----
*---- Phoenix
if (!%pheonixlocation%)
  set pheonixlocation Earth
  global pheonixlocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Earth
set firstplanetport 408
set firstplanetorbit 50
set secondplanetname Space Hub
set secondplanetport 16300
set secondplanetorbit 33385
set room1 3916
set room2 3917
set room3 3918
set room4 3919
set room5 3920
set shipname Phoenix
set route1s1 32961
set route1s2 33333
set route1s3 33570
set route1s4 33370
set route2s1 32343
set route2s2 35143
set route2s3 37653
set route2s4 37673
*----
*---- Now, travel from first planet to secod planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 4 || %time.hour% == 8 || %time.hour% == 12 || %time.hour% == 16 || %time.hour% == 20)
  if (%self.room.vnum% == %firstplanetport%)
    set pheonixlocation %firstplanetname%
    global pheonixlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set pheonixlocation Departing from %firstplanetname%
    global pheonixlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set pheonixlocation In transit to %secondplanetname%
    global pheonixlocation
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set pheonixlocation Landing on %secondplanetname%
    global pheonixlocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking on the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set pheonixlocation %secondplanetname%
    global pheonixlocation
    %echo% %self.shortdesc% docks in the hangar bay of the Space Hub.
  end
end
*----
*---- Now, travel from second planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 6 || %time.hour% == 10 || %time.hour% == 14 || %time.hour% == 18 || %time.hour% == 22)
  if (%self.room.vnum% == %secondplanetport%)
    set pheonixlocation %secondplanetname%
    global pheonixlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %secondplanetorbit%
    set pheonixlocation Departing from %secondplanetname%
    global pheonixlocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed,  
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %firstplanetorbit%
    set pheonixlocation Landing on %firstplanetname%
    global pheonixlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the complex.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set pheonixlocation Earth
    global pheonixlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3917
Pheonix Departure Script~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== Pheonix
if (%self.vnum% == 3916)
  if (%findobj.408(3916)%)
    set departroom 408
  elseif (%findobj.16300(3916)%)
    set departroom 16300
  else
    set departroom fail
  end
  set shipname Pheonix
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3918
Pheonix Board~
1 c 100
boardphoenix~
*---- This command is to be attached to all public ships
*---- and will use variables to determine what ship
*---- is being boarded and what to do about it\
*----
*---- First, figure out which ship this is and set variables
*----
if (%self.vnum% == 3916)
  set shipname Phoenix
  set shipinside 3916
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*---------------------
*---- Now that we have this ship and vars, GET IN MY BELLY!
*---------------------
if (%failure% == Pass)
    %echoaround% %actor% %actor.name% holds a ticket to a scanner on the %shipname%, then boards.
    %send% %actor% You hold your ticket to a scanner on the %shipname%, then board.
    %teleport% %actor% %shipinside%
    wait 1
    %force% %actor% look
    %echoaround% %actor% %actor.name% boards the %shipname%.
%purge% %actor.inventory(3999)%
else
  %echoaround% %actor% %actor.name% tries to board the %shipname% but is rejected with an error beep.
  %send% %actor% You cannot board the %shipname% %failure%!
end  
~
#3921
merganser control script~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Vegeta(load location) 
*----
*---- merganser
if (!%merganserlocation%)
  set merganserlocation Vegeta
  global merganserlocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Vegeta
set firstplanetport 2323
set firstplanetorbit 53
set secondplanetname Space Hub
set secondplanetport 16300
set secondplanetorbit 33385
set room1 3921
set room2 3922
set room3 3923
set room4 3924
set room5 3925
set shipname merganser
set route1s1 32355
set route1s2 33311
set route1s3 33350
set route1s4 43369
set route2s1 43369
set route2s2 33350
set route2s3 33311
set route2s4 32355
*----
*---- Now, travel from first planet to secod planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 4 || %time.hour% == 8 || %time.hour% == 12 || %time.hour% == 16 || %time.hour% == 20)
  if (%self.room.vnum% == %firstplanetport%)
    set merganserlocation %firstplanetname%
    global merganserlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set merganserlocation Departing from %firstplanetname%
    global merganserlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set merganserlocation In transit to %secondplanetname%
    global merganserlocation
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set merganserlocation Landing on %secondplanetname%
    global merganserlocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking on the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set merganserlocation %secondplanetname%
    global merganserlocation
    %echo% %self.shortdesc% docks in the hangar bay of the Space Hub.
  end
end
*----
*---- Now, travel from second planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 6 || %time.hour% == 10 || %time.hour% == 14 || %time.hour% == 18 || %time.hour% == 22)
  if (%self.room.vnum% == %secondplanetport%)
    set merganserlocation %secondplanetname%
    global merganserlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %secondplanetorbit%
    set merganserlocation Departing from %secondplanetname%
    global merganserlocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed,  
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %firstplanetorbit%
    set merganserlocation Landing on %firstplanetname%
    global merganserlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the complex.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set merganserlocation Vegeta
    global merganserlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3922
merganser depart~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== merganser
if (%self.vnum% == 3921)
  if (%findobj.2323(3921)%)
    set departroom 2323
  elseif (%findobj.16300(3921)%)
    set departroom 16300
  else
    set departroom fail
  end
  set shipname merganser
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3923
merganser board~
1 c 100
boardmerganser~
*---- This command is to be attached to all public ships
*---- and will use variables to determine what ship
*---- is being boarded and what to do about it\
*----
*---- First, figure out which ship this is and set variables
*----
if (%self.vnum% == 3921)
  set shipname Merganser
  set shipinside 3921
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*---------------------
*---- Now that we have this ship and vars, GET IN MY BELLY!
*---------------------
if (%failure% == Pass)
  %echoaround% %actor% %actor.name% holds a ticket to a scanner on the %shipname%, then boards.
  %send% %actor% You hold your ticket to a scanner on the %shipname%, then board.
  %teleport% %actor% %shipinside%
  wait 1
  %force% %actor% look
  %echoaround% %actor% %actor.name% boards the %shipname%.
  %purge% %actor.inventory(3999)%
else
  %echoaround% %actor% %actor.name% tries to board the %shipname% but is rejected with an error beep.
  %send% %actor% You cannot board the %shipname% %failure%!
end  
~
#3926
Wraith Control Script~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Namek(load location) 
*----
*---- wraith
if (!%wraithlocation%)
  set wraithlocation Namek
  global wraithlocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Namek
set firstplanetport 11626
set firstplanetorbit 54
set secondplanetname Space Hub
set secondplanetport 16300
set secondplanetorbit 33385
set room1 3926
set room2 3927
set room3 3928
set room4 3929
set room5 3930
set shipname wraith
set route1s1 37686
set route1s2 33286
set route1s3 33336
set route1s4 33365
set route2s1 33365
set route2s2 33336
set route2s3 33286
set route2s4 37686
*----
*---- Now, travel from first planet to second planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 4 || %time.hour% == 8 || %time.hour% == 12 || %time.hour% == 16 || %time.hour% == 20)
  if (%self.room.vnum% == %firstplanetport%)
    set wraithlocation %firstplanetname%
    global wraithlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set wraithlocation Departing from %firstplanetname%
    global wraithlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set wraithlocation In transit to %secondplanetname%
    global wraithlocation
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area.
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set wraithlocation Landing on %secondplanetname%
    global wraithlocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking on the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set wraithlocation %secondplanetname%
    global wraithlocation
    %echo% %self.shortdesc% docks in the hangar bay of the Space Hub.
  end
end
*----
*---- Now, travel from second planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 6 || %time.hour% == 10 || %time.hour% == 14 || %time.hour% == 18 || %time.hour% == 22)
  if (%self.room.vnum% == %secondplanetport%)
    set wraithlocation %secondplanetname%
    global wraithlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %secondplanetorbit%
    set wraithlocation Departing from %secondplanetname%
    global wraithlocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed,  
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %firstplanetorbit%
    set wraithlocation Landing on %firstplanetname%
    global wraithlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the complex.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set wraithlocation Namek
    global wraithlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3927
Wraith Depart~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== wraith
if (%self.vnum% == 3930)
  if (%findobj.11626(3930)%)
    set departroom 11626
  elseif (%findobj.16300(3930)%)
    set departroom 16300
  else
    set departroom fail
  end
  set shipname wraith
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3928
Wraith board~
1 c 100
boardwraith~
*---- This command is to be attached to all public ships
*---- and will use variables to determine what ship
*---- is being boarded and what to do about it\
*----
*---- First, figure out which ship this is and set variables
*----
if (%self.vnum% == 3930)
  set shipname wraith
  set shipinside 3930
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*---------------------
*---- Now that we have this ship and vars, GET IN MY BELLY!
*---------------------
if (%failure% == Pass)
    %echoaround% %actor% %actor.name% holds a ticket to a scanner on the %shipname%, then boards.
    %send% %actor% You hold your ticket to a scanner on the %shipname%, then board.
    %teleport% %actor% %shipinside%
    wait 1
    %force% %actor% look
    %echoaround% %actor% %actor.name% boards the %shipname%.
%purge% %actor.inventory(3999)%
else
  %echoaround% %actor% %actor.name% tries to board the %shipname% but is rejected with an error beep.
  %send% %actor% You cannot board the %shipname% %failure%!
end  
~
#3931
Ghost Control Script~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Konack(load location) 
*----
*---- ghost
if (!%ghostlocation%)
  set ghostlocation Konack
  global ghostlocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Konack
set firstplanetport 8194
set firstplanetorbit 52
set secondplanetname Space Hub
set secondplanetport 16300
set secondplanetorbit 33385
set room1 3931
set room2 3932
set room3 3933
set room4 3934
set room5 3935
set shipname ghost
set route1s1 29055
set route1s2 32144
set route1s3 33351
set route1s4 33370
set route2s1 33370
set route2s2 33351
set route2s3 32144
set route2s4 29055
*----
*---- Now, travel from first planet to second planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 4 || %time.hour% == 8 || %time.hour% == 12 || %time.hour% == 16 || %time.hour% == 20)
  if (%self.room.vnum% == %firstplanetport%)
    set ghostlocation %firstplanetname%
    global ghostlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set ghostlocation Departing from %firstplanetname%
    global ghostlocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set ghostlocation In transit to %secondplanetname%
    global ghostlocation
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area.
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set ghostlocation Landing on %secondplanetname%
    global ghostlocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking on the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set ghostlocation %secondplanetname%
    global ghostlocation
    %echo% %self.shortdesc% docks in the hangar bay of the Space Hub.
  end
end
*----
*---- Now, travel from second planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 6 || %time.hour% == 10 || %time.hour% == 14 || %time.hour% == 18 || %time.hour% == 22)
  if (%self.room.vnum% == %secondplanetport%)
    set ghostlocation %secondplanetname%
    global ghostlocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %secondplanetorbit%
    set ghostlocation Departing from %secondplanetname%
    global ghostlocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed,  
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %firstplanetorbit%
    set ghostlocation Landing on %firstplanetname%
    global ghostlocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the complex.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set ghostlocation Konack
    global ghostlocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3932
Ghost Board~
1 c 100
boardghost~
*---- This command is to be attached to all public ships
*---- and will use variables to determine what ship
*---- is being boarded and what to do about it\
*----
*---- First, figure out which ship this is and set variables
*----
if (%self.vnum% == 3935)
  set shipname ghost
  set shipinside 3935
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*---------------------
*---- Now that we have this ship and vars, GET IN MY BELLY!
*---------------------
if (%failure% == Pass)
    %echoaround% %actor% %actor.name% holds a ticket to a scanner on the %shipname%, then boards.
    %send% %actor% You hold your ticket to a scanner on the %shipname%, then board.
    %teleport% %actor% %shipinside%
    wait 1
    %force% %actor% look
    %echoaround% %actor% %actor.name% boards the %shipname%.
%purge% %actor.inventory(3999)%
else
  %echoaround% %actor% %actor.name% tries to board the %shipname% but is rejected with an error beep.
  %send% %actor% You cannot board the %shipname% %failure%!
end  
~
#3933
Ghost Depart~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== ghost
if (%self.vnum% == 3935)
  if (%findobj.8194(3935)%)
    set departroom 8194
  elseif (%findobj.16300(3935)%)
    set departroom 16300
  else
    set departroom fail
  end
  set shipname ghost
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3936
Wisp Control Script~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Aether(load location) 
*----
*---- wisp
if (!%wisplocation%)
  set wisplocation Aether
  global wisplocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Aether
set firstplanetport 12002
set firstplanetorbit 55
set secondplanetname Space Hub
set secondplanetport 16300
set secondplanetorbit 33385
set room1 3936
set room2 3937
set room3 3938
set room4 3939
set room5 3940
set shipname Wisp
set route1s1 40344
set route1s2 33304
set route1s3 33339
set route1s4 33365
set route2s1 33365
set route2s2 33339
set route2s3 33304
set route2s4 40344
*----
*---- Now, travel from first planet to second planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 4 || %time.hour% == 8 || %time.hour% == 12 || %time.hour% == 16 || %time.hour% == 20)
  if (%self.room.vnum% == %firstplanetport%)
    set wisplocation %firstplanetname%
    global wisplocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set wisplocation Departing from %firstplanetname%
    global wisplocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set wisplocation In transit to %secondplanetname%
    global wisplocation
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set wisplocation Landing on %secondplanetname%
    global wisplocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking on the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set wisplocation %secondplanetname%
    global wisplocation
    %echo% %self.shortdesc% docks in the hangar bay of the Space Hub.
  end
end
*----
*---- Now, travel from second planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 6 || %time.hour% == 10 || %time.hour% == 14 || %time.hour% == 18 || %time.hour% == 22)
  if (%self.room.vnum% == %secondplanetport%)
    set wisplocation %secondplanetname%
    global wisplocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %secondplanetorbit%
    set wisplocation Departing from %secondplanetname%
    global wisplocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed,  
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %firstplanetorbit%
    set wisplocation Landing on %firstplanetname%
    global wisplocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the complex.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set wisplocation Aether
    global wisplocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3937
Wisp Depart~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== wisp
if (%self.vnum% == 3940)
  if (%findobj.12002(3940)%)
    set departroom 12002
  elseif (%findobj.16300(3940)%)
    set departroom 16300
  else
    set departroom fail
  end
  set shipname Wisp
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3938
Wisp board~
1 c 100
boardwisp~
*---- This command is to be attached to all public ships
*---- and will use variables to determine what ship
*---- is being boarded and what to do about it\
*----
*---- First, figure out which ship this is and set variables
*----
if (%self.vnum% == 3940)
  set shipname Wisp
  set shipinside 3940
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*---------------------
*---- Now that we have this ship and vars, GET IN MY BELLY!
*---------------------
if (%failure% == Pass)
    %echoaround% %actor% %actor.name% holds a ticket to a scanner on the %shipname%, then boards.
    %send% %actor% You hold your ticket to a scanner on the %shipname%, then board.
    %teleport% %actor% %shipinside%
    wait 1
    %force% %actor% look
    %echoaround% %actor% %actor.name% boards the %shipname%.
%purge% %actor.inventory(3999)%
else
  %echoaround% %actor% %actor.name% tries to board the %shipname% but is rejected with an error beep.
  %send% %actor% You cannot board the %shipname% %failure%!
end
~
#3941
Eagle Control Script~
1 ab 100
~
*------------------------------------
*- Public Ship script by Sedalis
*------------------------------------
*----
*---- This script works in conjunction with other scripts
*---- to control the public ship transport system.
*---- The script checks the time, and if it's time to move,
*---- the ship moves to the new location and prepares for boarding.
*----
*---- For now, ships move every 5 minutes (every mud hour)
*----
*---- For the first run, set location to Frigid(load location) 
*----
*---- eagle
if (!%eaglelocation%)
  set eaglelocation Frigid
  global eaglelocation
end
*----
*---- Let's set up some variables to make this easy to modify
*---- if changes are made in the future that affect where we
*---- need the ships to go.
*----
set firstplanetname Frigid
set firstplanetport 4250
set firstplanetorbit 51
set secondplanetname Space Hub
set secondplanetport 16300
set secondplanetorbit 33385
set room1 3941
set room2 3942
set room3 3943
set room4 3944
set room5 3945
set shipname Eagle
set route1s1 32961
set route1s2 33333
set route1s3 33570
set route1s4 33370
set route2s1 33370
set route2s2 33570
set route2s3 33333
set route2s4 32961
*----
*---- Now, travel from first planet to second planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 0 || %time.hour% == 4 || %time.hour% == 8 || %time.hour% == 12 || %time.hour% == 16 || %time.hour% == 20)
  if (%self.room.vnum% == %firstplanetport%)
    set eaglelocation %firstplanetname%
    global eaglelocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %secondplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %secondplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off, on its way to %secondplanetname%.
    ogoto %firstplanetorbit%
    set eaglelocation Departing from %firstplanetname%
    global eaglelocation
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set eaglelocation In transit to %secondplanetname%
    global eaglelocation
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route1s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %secondplanetorbit%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    set eaglelocation Landing on %secondplanetname%
    global eaglelocation
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins docking on the Space Hub.
    wait 10s
    %echo% %self.shortdesc% docks in the hangar bay of %secondplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it docks in the %secondplanetname%.
    ogoto %secondplanetport%
    wait 1
    set eaglelocation %secondplanetname%
    global eaglelocation
    %echo% %self.shortdesc% docks in the hangar bay of the Space Hub.
  end
end
*----
*---- Now, travel from second planet to first planet, setting location global
*---- along the way so locator script can see where the ship currently
*---- is.
*----
if (%time.hour% == 2 || %time.hour% == 6 || %time.hour% == 10 || %time.hour% == 14 || %time.hour% == 18 || %time.hour% == 22)
  if (%self.room.vnum% == %secondplanetport%)
    set eaglelocation %secondplanetname%
    global eaglelocation
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% is preparing to depart for %firstplanetname%."
    wait 30s
    %echo% An announcement buzzes through speakers:  "Public Transport ship %self.shortdesc% departing for %firstplanetname% in ten seconds."
    wait 10s
    %echo% %self.shortdesc%'s engines blaze into life as it blasts into low orbit.
    %at% %room1% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it takes off on its way to %firstplanetname%.
    ogoto %secondplanetorbit%
    set eaglelocation Departing from %secondplanetname%
    global eaglelocation
    wait 1
    %echo% %shipname% enters orbit from the surface below.
    wait 10s
    %echo% %self.shortdesc%'s engines flare as it bursts into space.
    ogoto %route1s1%
    wait 1
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    *---- Start of Travels
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s2%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 40s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed,  
    ogoto %route2s3%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    ogoto %route2s4%
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 60s
    %echo% %self.shortdesc%'s engines flare as it suddenly bursts to top speed, leaving the area. 
    *---- End of travels
    ogoto %firstplanetorbit%
    set eaglelocation Landing on %firstplanetname%
    global eaglelocation
    %echo% %self.shortdesc% slows down briefly as it enters the area, adjusting course.
    wait 5s
    %echo% %self.shortdesc% breaks orbit and begins descending to the complex.
    wait 10s
    %echo% %self.shortdesc% descends into the atmosphere of planet %firstplanetname%.
    %at% %room1% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room2% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room3% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room4% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    %at% %room5% %echo% The %shipname% lurches as it lands on %firstplanetname%.
    ogoto %firstplanetport%
    wait 1
    set eaglelocation Frigid
    global eaglelocation
    %echo% %self.shortdesc% descends through the atmosphere and lands.
  end
end
*----
*---- Now, we're back where we started... so it'll do it all over again!
*----
~
#3942
eagle depart~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== eagle
if (%self.vnum% == 3945)
  if (%findobj.4250(3945)%)
    set departroom 4250
  elseif (%findobj.16300(3945)%)
    set departroom 16300
  else
    set departroom fail
  end
  set shipname Eagle
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
#3943
Eagle board~
1 c 100
boardeagle~
*---- This command is to be attached to all public ships
*---- and will use variables to determine what ship
*---- is being boarded and what to do about it\
*----
*---- First, figure out which ship this is and set variables
*----
if (%self.vnum% == 3945)
  set shipname Eagle
  set shipinside 3945
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*---------------------
*---- Now that we have this ship and vars, GET IN MY BELLY!
*---------------------
if (%failure% == Pass)
    %echoaround% %actor% %actor.name% holds a ticket to a scanner on the %shipname%, then boards.
    %send% %actor% You hold your ticket to a scanner on the %shipname%, then board.
    %teleport% %actor% %shipinside%
    wait 1
    %force% %actor% look
    %echoaround% %actor% %actor.name% boards the %shipname%.
%purge% %actor.inventory(3999)%
else
  %echoaround% %actor% %actor.name% tries to board the %shipname% but is rejected with an error beep.
  %send% %actor% You cannot board the %shipname% %failure%!
end
~
#3998
Simurgh Board Script~
1 c 100
boardsimurgh~
*@Y---- @cThis command is to be attached to all public ships@n
*@Y---- @cand will use variables to determine what ship@n
*@Y---- @cis being boarded and what to do about it@n\
*@Y----@n
*@Y----@g First, figure out which ship this is and set variables@n
*@Y----@n
if (%self.vnum% == 3905)
  set shipname Simurgh
  set shipinside 3905
  if (%self.room.vnum% >= 20000)
    set failure while the ship is in space!
  else
    set failure Pass
  end
  if (!%actor.inventory(3999)%)
    set failure without a ticket!
  end
end
*@Y---------------------@n
*@Y----@g Now that we have th4s ship and vars, GET IN MY BELLY!
*@Y---------------------@n
if (%failure% == Pass)
    %echoaround% %actor% @R%actor.name% holds a ticket to a scanner on the %shipname%@R, then boards.@n
    %send% %actor% @RYou hold your ticket to a scanner on the %shipname%@R, then board.@n
    %teleport% %actor% %shipinside%
    wait 1
    %force% %actor% look
    %echoaround% %actor% @R%actor.name% boards the %shipname%@R.@n
%purge% %actor.inventory(3999)%
else
  %echoaround% %actor% @R%actor.name% tries to board the %shipname% but is rejected with an error beep.@n
  %send% %actor% @RYou cannot board the %shipname%@R %failure%!@n
end  
  
~
#3999
Simurgh Departure Script~
2 c 100
depart~
*---- This script is the departure script for ships.
*---- It should be attached to all public ships.
*----
*---- First, find this ship and set vars...
*----
*==== Simurgh
if (%self.vnum% == 3905)
  if (%findobj.12002(3905)%)
    set departroom 12002
  elseif (%findobj.8194(3905)%)
    set departroom 8194
  elseif (%findobj.16300(3905)%)
    set departroom 16300
  else
    set departroom fail
  end
  set shipname Simurgh
end
*---
*--- Now, find where the ship is and set vars...
*---
if (%departroom% != fail)
  %echoaround% %actor% %actor.name% departs from the %shipname%.
  %teleport% %actor% %departroom%
  wait 1
  %force% %actor% look
  wait 1
  %send% %actor% You depart the %shipname%.
  %echoaround% %actor% %actor.name% departs from the %shipname% and arrives here.
else
  %send% %actor% You cannot depart the %shipname% while in flight!  Are you crazy!?
  %echoaround% %actor.name% The hatch refuses to open for %actor.name%.
end
~
$~
