#19001
Cowgirl Greet~
0 g 100
~
emote says with a southern accent, &1Welcome to Cowboys-R-us! If y'all need any help, just let me know.&2
~
#19002
Hyouga Greet~
0 g 100
~
wait 1
emote looks at %actor.name% as %actor.heshe% enters the store, &1Get what you want and get out. And be quick about it!&2
~
#19003
Mufzin Greet~
0 g 100
~
wait 1
emote glares at %actor.name% as they enter and chuckles to himself, &1Come to get some armor have you? Yes, I suppose you need some. The weaklings usually do.&2
wait 2
laugh
~
#19004
Nedren Greet~
0 g 100
~
wait 1
emote jumps up as %actor.name% enters, &1Ah! Welcome to The Dynasty. My name is Nedren! If you need any help, just let me or my associates know.&2
~
#19005
Kaidran Greet~
0 g 100
~
wait 1
emote glances briefly up at %actor.name% as they step into the store before returning his gaze to the magazine thats infront of him and boredly says, &1Welcome to Ethereal Fashion. My name is Kaidran, and if you have any questions just let me know, blah blah blah...&2
~
#19006
Nomad Greet~
0 g 100
~
wait 1
emote nods at %actor.name%, &1Ah, I see you've found my hut. Everything you see here is for sale.&2
wait 1 s
smile
~
#19007
Clarice Greet~
0 g 100
~
wait 1
emote bounces out of her chair as %actor.name% enters, &1Hi! Welcome to Stylin' Fashion! My name is Clarice, and I will help you pick out the best outfit for you!&2
wait 2 s
emote giggles madly.
~
#19008
Seth Greet~
0 g 100
~
wait 1
say Duude, sup! Welcome to my rad shop, Surf's Up! My name is Seth, and I will be your....Uh, wait... What were we talking about?
~
#19012
Jiro Chef Trigger (Kindle's House)~
0 d 100
sushi~
*******************************************************
* By Sedalis:
* For house ownership change, change the houseowner variable
* to the new char's name
*******************************************************
set houseowner Kindle
set food 19012
set foodname Sushi
if (%actor.name% != %houseowner%)
  wait 2s
  emote stares at *%actor.name% for several seconds.  He raises a cleaver in his right hand threateningly and half lunges before lowing the cleaver.  '@mI only cook for %houseowner%.@n'
else
  wait 3s
  emote nods his head at *%actor.name% respectfully.  '@mI have your favorite prepared just for you.@n'
  wait 2s
  open fridge
  get sushi fridge
  close fridge
  wait 2s
  if (%self.has_item(19012)%)
    emote smiles at *%actor.name% and presents a delicious looking plate of food.  '@mFor you, noble Master.@n'
    give sushi %actor.name%
  else
    emote frowns as he comes up empty handed from the raid on the fridge.  '@mI apologize, Sir.  I haven't had time to make anything for you yet.@n'
    if (%self.has_item(202)% || %self.has_item(203)%)
      wait 2s
      emote checks his stores of raw ingredients.  @m'Ahhhh!  I have a decent bass here I can make sushi out of.  Give me just a moment!@n'
      wait 30s
      if (%self.has_item(202)% || %self.has_item(203)%)
        %purge% bass
        %load% obj %food%
        emote smiles as he puts the finishing touches on the food.  @m"Excellent, excellent.  Here you go, %houseowner%.@n'
        give %foodname% %houseowner%
      else
        emote frowns as he checks the food.  @m'I swear I had all the ingredients.  I must not have.  Sorry, %houseowner%.@n'
      end
    else
      wait 2s
      emote checks his stores of raw ingredients.  '@mWell, you haven't supplied me with any ingredients.  I can't cook air, you know.@n'
    end
  end
end
~
#19013
Kindle's Septic Tank Trigger~
2 b 30
~
%echo% @RA gross, slimy substance squidges down from the ceiling from a pipe.  Is... Is that feces?  WHAT THE HELL!?  GROSS!@n
~
#19014
Kindle's Bimbo Housegirl Script~
0 g 100
~
set speakchance %random.5%
if (%speakchance% == 1)
  wait 1s
  emote drops a wet rag on the ground.  Giggling daintily, she bends over to pick it up.  She picks up the rag and begins wiping down a car.
elseif (%speakchance% == 2)
  wait 3s
emote nods at *%actor.name%.  '@CWelcome to...@n'  She pauses for a minute, looking confused.  Turning to look behind her, she looks at the mansion thoughtfully for a minute.  '@CWelcome to this place.  Yeah!@n'  She says with a giggle as she turns back around.
elseif (%speakchance% == 3)
  wait 2s
  emote leans far over a car, wiping it with a large sponge.  As she stands up and turns around, she smiles at *%actor.name% while her shirt appears mostly see-through.  @C'Hey cutie.  Wanna help me wash this car?'@n
elseif (%speakchance% == 4)
  wait 1s
emote stomps her feet and throws a temper tantrum.  @C'I hate cleaning this stupid car all day every day!'@n  She coughs, then holds her throat.  Her face turns bright red... then she finally coughs one final time and a piece of gum flies out of her mouth.  She breathes in deeply for a few seconds, then bends over and picks the gum up.  Looking at it critically, she shrugs and puts it back in her mouth and chews it.
else
  wait 4s
  emote watches *%actor.name% approach and bounces up and down.  Her feminine assets jiggle obscenely as she waves emphatically.  @C'Hi.  Hi.  Hi!  HELLO!  It's so nice to see you!'@n
end
~
#19015
Standard Anti-IT script~
2 c 100
inst~
if (%cmd% == inst || %cmd% == insta || %cmd% == instan% || %cmd% == instant)
return 1
%send% %actor% You can't sense anything from within here.
end
~
#19039
Door Scan~
2 c 100
scan~
wait 1 s
%echo% The panel lights up with: Processing ID...
wait 1 s
 
if (%actor.has_item(19039)%)
   
   %echo% The panel lights up with: Accepted. Opening door. You have three seconds to enter.
   
   if (%self.vnum% == 19039)
     %at% 19040 %echo% The Secret Door unlocks and slides open.
    else
     %at% 19039 %echo% The Secret Door unlocks and slides open.
   end
   
   %door% 19039 0 flags a
   %door% 19040 2 flags a
   wait 3 s
   %echo% The panel lights up with: Door closing.
    if (%self.vnum% == 19039)
     %at% 19040 %echo% The Secret Door slides closed.
    else
     %at% 19039 %echo% The Secret Door slides closed.
   end
   %door% 19039 0 flags abc
   %door% 19040 2 flags abc
else
   %echo% The panel lights up with: Error. No ID found, or invalid ID.
end
~
#19044
Service Droid Greet~
0 g 100
~
wait 2 s
emote walks over and greets %actor.name%, in a mechanical voice, '@CWelcome to the Mess Hall, please enjoy your meal and time spent here.@n' it returns to cleaning the already clean room.
~
#19045
Lavos Rand~
0 ab 25
~
emote lets out a low rumble as the Mammon Machine leeches energy from it.
~
#19080
Duplicate Key~
1 c 3
duplicate~
if (%self.itemflag(DUPLICATE)%)
 %send% %actor% You can not duplicate a duplicate...
elseif (!%actor.has_item(19079)%)
 %send% %actor% You need a key duplicator.
else
 wait 1
 %send% %actor% The duplicator hums for a moment as it copies itself into a duplicate of the key.
 %purge% %actor.inventory(19079)%
 %load% obj %self.vnum%
 %force% %actor% get key
 odupe
end
~
$~
