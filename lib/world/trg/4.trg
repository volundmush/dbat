#400
Fred Trigger~
0 g 100
~
switch (%random.3%)
case 1
 wait 1 sec
 say Hey man....have a seat....
 wait 4 sec
 %echo% Fred holds up a bong to his lips and brings his lighter to the bowl.
 wait 2 sec
 %echo% Fred lights the bowl and starts to inhale.
 wait 2 sec
 %echo% The bong gurgles slowly.
 wait 2 sec
 %echo% The bong gurgles slowly.
 wait 4 sec
 %echo% Fred completes his hit and raises his head.
 wait 4 sec
 %echo% Fred exhales a steady wave of smoke from his mouth.
 wait 3 sec
 say Whoa man...I'm so baked.
 break
 case 2
 wait 2 sec
 say Hey..wanna hit?
 wait 3 sec
 %echo% Fred holds up a bong invitingly.
 wait 4 sec
 say No? Oh well more for me....
 break
 case 3
 wait 2 sec
 say Hey man...I am almost out of weed, so you are out of luck.
 wait 3 sec
 %echo% Fred starts to take a hit from a small bowl.
 wait 2 sec
 %echo% The weed inside the bowl glows as Fred takes his hit.
 wait 3 sec
 %echo% Fred stops taking his hit and holds it for a little bit.
 wait 2 sec
 %echo% Fred exhales into the room.
 wait 3 sec
 %echo% Fred smiles broadly as he sinks further into the couch cushions.
 break
 default
 wait 1 sec
 smile
 break
done
~
#401
Fred Load~
0 n 100
~
sit couch
~
#402
Fred Random~
0 ab 10
~
if %self.has_item(407)%
switch %random.5%
 case 1
  wait 1 sec
  %echo% Fred sips from his beer can.
  break
 case 2
  wait 1 sec
  %echo% Fred guzzles his beer can.
  break
 case 3
  wait 1 sec
  %echo% Fred sips from his beer can.
  break
 case 4
  wait 1 sec
  %echo% Fred sips from his beer can.
  break
 default
  wait 1 sec
  %echo% Fred crushes the empty beer can.
  junk beer
  break
 done
else
 if %findobj.423(407)% >= 1
  wait 1 sec
  stand
  wait 2 sec
  %echo% Fred seems to think about what to do for a second.
  wait 3 sec
  n
  wait 2 sec
  open frige
  wait 1 sec
  get beer frige
  wait 3 sec
  s
  wait 1 sec
  sit couch
  sit recliner
  wait 2 sec
  open beer
  wait 1 sec
  %echo% Fred sips from the beer can.
  wait 3 sec
  %echo% Fred smiles contentedly.
 else
  wait 1 sec
  say Well I guess I better go get some more brew...
  wait 2 sec
  stand
  wait 3 sec
  say Don't touch my stash while I am gone ok?
  wait 3 sec
  %echo% Fred opens the door and walks out, closing the door behind him.
  mgoto 49
  mload obj 407
  mload obj 407
  mload obj 407
  mload obj 407
  mload obj 407
  mload obj 407
  wait 30 sec
  mgoto 422
  %echo% The door opens and Fred steps through.
  wait 3 sec
  say Damn that took forever, better put these in the frige to cool.
  wait 3 sec
  n
  wait 1 sec
  open frige
  wait 1
  put all.beer frige
  wait 2 sec
  s
  wait 1
  sit couch
  sit recliner
 end
end
~
#403
Fred stand trigger~
0 k 100
~
stand
~
#405
Stove Trigger~
1 c 4
cook~
if (%self.count(411)% > 1)
wait 1
%send% %actor% Don't try to cook more than one thing at a time...
elseif (%self.count(409)% > 1)
wait 1
%send% %actor% Don't try to cook more than one thing at a time...
elseif (%self.has_in(409)% &&  %self.has_in(411)%)
wait 1
%send% %actor% Don't try to cook more than one thing at a time...
elseif %self.has_in(409)%
wait 1
%send% %actor% You start the oven.
%echoaround% %actor% %actor.name% starts the oven.
wait 3 sec
%echo% Heat can be felt coming from the stove. 
wait 4 sec
%echo% A sizzling sound can be heard from the stove.
wait 2 sec
%echo% The stove timer goes off and so does the stove.
%load% obj 405
%load% obj 413
%force% %actor% get cooked
%purge% self
elseif %self.has_in(411)%
wait 1
%send% %actor% You place an egg in a pot and start the stove.
%echoaround% %actor% %actor.name% starts the oven.
wait 3 sec
%echo% Heat can be felt coming from the stove.
wait 4 sec
%echo% A gurgling sound can be heard from the stove.
wait 2 sec
%echo% The stove timer goes off and so does the stove.
%load% obj 405
%load% obj 414
%force% %actor% get cooked
%purge% self
else
wait 1
%send% %actor% Nothing worth cooking is in the stove.
end
~
#425
Old Man Random~
0 b 5
~
wait 1
mecho The old man grumbles as he looks around.
wait 3 sec
say Darn young people...
~
#426
Nexus City Police Officer Greet~
0 g 100
~
wait 2 sec
nod %actor.name%
~
#427
Nexus Police Officer Random~
0 b 5
~
wait 1
mecho The police officer speaks into his radio casually.
wait 3 sec
mecho The police officer's radio responds back.
~
#428
Street Thug Random~
0 b 5
~
wait 1
mecho The street thug looks around for some easy prey.
wait 4 sec
say Now all I need is a sucker....
~
#429
Street Thug Greet~
0 g 50
~
wait 1 sec
grin %actor.name%
wait 2 sec
say Hey there... You lost?
wait 4 sec
if (%actor.room.vnum% == %self.room.vnum%)
echoaround %actor% The street thug approaches %actor.name% suspiciously, but notices a large crowd and backs off.
%send% %actor% The street thug begins to approach you suspiciously, but notices a large crowd and backs off.
end
~
#430
Business man random~
0 b 10
~
wait 1
mecho The businessman looks at his watch.
wait 1 sec
say Hmph...
~
#450
Newspaper Lady Interaction~
0 c 100
talk~
if %cmd% ==  talk
if %arg% == %self.alias%
  %send% %actor% %self.name% says, 'What would you like to talk about?'
  %send% %actor% ------------------------------------
  %send% %actor% @ROption @Y1) @WFood@n
  %send% %actor% @ROption @Y2) @WDrink@n
  %send% %actor% @ROption @Y3) @WRumour@n
  %send% %actor% @D @n
  %send% %actor% @RSyntax@n: @Ytalk %self.alias% <option number>@n
  %send% %actor% ------------------------------------
elseif %arg% /= %self.alias% && %arg% /= 1
  %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
  %send% %actor% %self.name% says, 'Sakuras Bakery in the Akibura Shopping District is really good!'
elseif %arg% /= %self.alias% && %arg% /= 2
  %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
  %send% %actor% %self.name% says, 'Ahh Spa is the place I go to unwind and they have quite a few refreshing beverages.'
elseif %arg% /= %self.alias% && %arg% /= 3
  %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
  %send% %actor% %self.name% says, 'Sometimes I find myself reading the newspaper in Rosewater park. I often see people fighting the animals there. Bizarre.'
end
end
~
#451
Utility Worker Interact~
0 c 100
talk~
if %cmd% ==  talk
if %arg% == %self.alias%
  %send% %actor% %self.name% says, 'What would you like to talk about?'
  %send% %actor% ------------------------------------
  %send% %actor% @ROption @Y1) @WAbout Me@n
  %send% %actor% @ROption @Y2) @WRumour@n
  %send% %actor% @D @n
  %send% %actor% @RSyntax@n: @Ytalk %self.alias% <option number>@n
  %send% %actor% ------------------------------------
elseif %arg% /= %self.alias% && %arg% /= 1
  %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
  %send% %actor% %self.name% says, 'Can't you see I am working here?'
elseif %arg% /= %self.alias% && %arg% /= 2
  %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
  %send% %actor% %self.name% says, 'The city leaves access to the sewer open to the public. Dunno why, it is dangerous and people go missing all the time.'
end
~
#460
Space Mall Pilot Greet~
0 g 100
~
wait 2s
say Come one, come all!  Take a trip to the space mall, only 100 zenni!  Hand me your money and I'll take you directly there... but I don't offer return flights!
~
#483
Talk: [200] Karl~
0 c 100
Talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% A %self.name% barely looks at you come inside the store and says '@CKarl's Fishing Emporium. How can I help?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWhy would I want to fish?@n
    %send% %actor% @ROption @Y2) @WWhat's so special about the fish here?@n
    %send% %actor% @ROption @Y3) @WAnything else?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%.
    %send% %actor% %self.name% shrugs, '@CHey man, if you want to eat that GMO'ed nonsense that McStuff sells then go ahead but food you make yourself makes for a better eating experience@w'@n
    * @YThe below fires if @Goption 2@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%.
    %send% %actor% A %self.name% says, '@CThe fish in my lake don't come into contact with toxic things that big corporations like Celestial Corp dumps into bodys of water@w'@n
    * @YThe below fires if @Goption 3@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with  %self.name%
    %send% %actor% %self.name% says, '@CYeah, you break it...@w' He points to the fishing poles on the wall, '@CYou buy it. Understand?@w'@n
  end
end
~
#484
Talk: [685] Master Seiji~
0 g 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% meditates peacefully, seemingly unconcerned about you, '@CYou won't convince me to take you as my student@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWhy are you not taking anymore students?@n
    %send% %actor% @ROption @Y2) @WHow did you get that scar?@n
    %send% %actor% @ROption @Y3) @WHas that experience had any affect on you?@n
    %send% %actor% @ROption @Y4) @WHow can I become as strong as you?@n
    %send% %actor% @ROption @Y5) @WWhere can I go to challenge myself?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% shuts his eyes on continues meditating, '@CI believe in quality over quantity and if I take on any more students then they will not be taught adequately@w'@n
    * @YThe below fires if @Goption 2@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% hangs his head and loudly sighs, '@CI fought in the King Piccolo War and was almost blinded by the Demon King himself@w'@n
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% sits idly on his cushion and ponders the question for a moment. '@CI can not see well on my right eye.@w' He laughs at his own joke softly, '@CThough that could be because of my age and not the scar@w'@n
    * @YThe below fires if @Goption 4 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 4
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% looks at you intently, '@CStrong? What does it mean to be strong?@w'@n %self.name% asks you. '@CStrength and growth only comes from constantly challenging yourself.@w'@n
    * @YThe below fires if @Goption 5 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 5
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% The torches around %self.name% begin slowly doing as his concentration waivers momentarily as his thoughts over take him.@n
    wait 1 s
    %send% %actor% %self.name% says, '@CTo challenge yourself, you should go to Planet Namek...@w' The old Human begins, '@CThere in Senzu Village, to the east, you will find warriors both weaker and stronger than myself@w'@n
    wait 2 s
    %send% %actor% %self.name% looks at you intently, '@CGo and experience this vast universe and let the universe experience@w' %self.name% slowly nods at his own wise words, '@CConflict, as you will see on Namek, exists everywhere@w'@n
    wait 2 s
    %send% %actor% %self.name% places a hand on his chest. '@CYou can even be at conflict with yourself.@w' The master stops speaking and continues meditating.@n
  end
end
~
#485
Talk: [211] Garrick~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% growls at you, '@CWhat do you want?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WHow did you get that scar?@n
    %send% %actor% @ROption @Y2) @WWhere did you get that chain?@n
    %send% %actor% @ROption @Y3) @WRobots in the mountains?@n
    %send% %actor% @ROption @Y4) @WWhat do numbers have to do with anything?@n
    %send% %actor% @ROption @Y5) @WIs there anything else I should know?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% smiles proudly and runs his hand across his throat as he says, '@CI got this scar when I got into a fight with the champion himself, Mr Satan.@w'@n
    * @YThe below fires if @Goption 2 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% twirls his chain, '@CI pulled it together from some robots I fought up in the mountains'@n
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says, '@CYeah. There are robots as strong as me somewhere in Cherry Blossom Mountain@w'@n %self.name% slams his chain into the ground, '@CBut I can't go back in because I'm not good with numbers?@w'@n
    * @YThe below fires if @Goption 4 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 4
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% laughs loudly, '@CLet me just say that you'll understand what I mean if you find it@w'@n
    * @YThe below fires if @Goption 5 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 5
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% shakes his head, '@CNah, I got nothing for you@w'@n
  end
end
~
#486
Talk: [439] Male Beach Visitor~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% A %self.name% waves at you, '@CHow's it going?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWhat are you doing?@n
    %send% %actor% @ROption @Y2) @WWhen and where is this party?@n
    %send% %actor% @ROption @Y3) @WAnything else I can do before the party starts?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with a %self.name%
    %send% %actor% A %self.name% says, '@CGetting ready for the biggest party of the year!@w'@n
    * @YThe below fires if @Goption 2@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with a %self.name%
    %send% %actor% A %self.name% says, '@CWe're still planning it but the karaoke machine has been set up but the party will be happening at Hercule Beach@w'@n
    * @YThe below fires if @Goption 3@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with a %self.name%
    %send% %actor% A %self.name% says, '@CThere's a tiny island on South Ocean. A few kilometres North East of you can submerge into the ocean and find some stuff that's been claimed by the sea@w'@n
  end
end
~
#487
Talk: [433] Female Skateboader~
0 c 100
talk~
if (%actor.age% >= 30)
  * @YThe following fires if the character is @Golder than 30@n
  if %cmd% ==  talk
    if %arg% == %self.alias%
      %send% %actor% %self.name% looks at her map and then at you, '@CHey, could you please help me?@w'@n
      %send% %actor% 
      %send% %actor% ------------------------------------
      %send% %actor% @ROption @Y1) @WWhy aren't you in school?@n
      %send% %actor% @ROption @Y2) @WHow long have you been skating?@n
      %send% %actor% @ROption @Y3) @WDo you know anything interesting?@n
      %send% %actor%  
      %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
      %send% %actor% ------------------------------------
      * @YThe below fires if @Goption 1@Y is selected@n
    elseif %arg% /= %self.alias% && %arg% /= 1
      %echoaround% %actor% %actor.name% strikes up a conversation with a %self.name%
      %send% %actor% A %self.name% says, '@CWhy aren't you in a grave?@w' and then laughs at her own retort.@n
      * @YThe below fires if @Goption 2 @Yis selected@n
    elseif %arg% /= %self.alias% && %arg% /= 2
      %echoaround% %actor% %actor.name% strikes up a conversation with a %self.name%
      %send% %actor% A %self.name% shouts '@CI've been skating for as long as your generation has been destroying the world and my future!@w'@n
      * @YThe below fires if @Goption 3 @Yis selected@n
    elseif %arg% /= %self.alias% && %arg% /= 3
      %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
      %send% %actor% %self.name% says, '@CThe only thing your generation cares about is money. You can steal a car and sell it, and nobody will say anything@w'@n
    end
  end
  * @YThe following fires if the character is @Gyounger than 30@n
  if %cmd% ==  talk
    if %arg% == %self.alias%
      %send% %actor% %self.name% looks at her map and then at you, '@CHey, could you please help me?@w'@n
      %send% %actor% 
      %send% %actor% ------------------------------------
      %send% %actor% @ROption @Y1) @WWhy aren't you in school?@n
      %send% %actor% @ROption @Y2) @WHow are you supposed to learn anything@n
      %send% %actor% @ROption @Y3) @WHave you learnt anything interesting?@n
      %send% %actor%  
      %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
      %send% %actor% ------------------------------------
      * @YThe below fires if @Goption 1@Y is selected@n
    elseif %arg% /= %self.alias% && %arg% /= 1
      %echoaround% %actor% %actor.name% strikes up a conversation with a %self.name%
      %send% %actor% A %self.name% says, '@CSchool is nothing more than a form of propaganda for the government to brainwash its own citizens. Stay out of school.@w'.@n
      * @YThe below fires if @Goption 2 @Yis selected@n
    elseif %arg% /= %self.alias% && %arg% /= 2
      %echoaround% %actor% %actor.name% strikes up a conversation with a %self.name%
      %send% %actor% A %self.name% shrugs, '@CI don't need an old person to teach me anything. I am enrolled in the School of Life!'@n
      * @YThe below fires if @Goption 3 @Yis selected@n
    elseif %arg% /= %self.alias% && %arg% /= 3
      %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
      %send% %actor% %self.name% nods her head and says, '@CThere's this hot guy at Sandy Desert who lives alone and will teach you a few moves he came up with@w'
    end
  end
end
~
#488
Talk: [488] Busy Businessman~
0 c 100
~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% seems bothered by your prescence, '@CWhat do you want?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWhy are you in a rush@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% A %self.name% walks off as he says, '@CI'm off to a meeting. I don't have time to talk with you@w'@n
  end
end @R
* @YNote: Find out how to check for zenni on hand so that this NPC has a different dialogue for "rich" players@n
~
#489
Talk: [434] Garbage Collector~
0 c 100
talk~
* @YThe below is only avaliable to @GHumans@n
if (%actor.race% == Human)
  if %cmd% ==  talk
    if %arg% == %self.alias%
      %send% %actor% 
      %send% %actor% ------------------------------------
      %send% %actor% @ROption @Y1) @WHello@n
      %send% %actor% @ROption @Y2) @WUh... "Fellow Human"?@n
      %send% %actor% @ROption @Y3) @WAre there others that think like you?@n
      %send% %actor% @ROption @Y4) @WThe Friends of Humanity?@n
      %send% %actor%  
      %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
      %send% %actor% ------------------------------------
      * @YThe below fires if @Goption 1@Y is selected@n
    elseif %arg% /= %self.alias% && %arg% /= 1
      %echoaround% %actor% A %self.name% warmly welcomes %actor.name%'s greeting with a smile.
      %send% %actor% A %self.name% stops working and smiles at you, '@CGreetings, my fellow Human@w'@n
      * @YThe below fires if @Goption 2@Y is selected@n
    elseif %arg% /= %self.alias% && %arg% /= 2
      %echoaround% %actor% A %self.name% pats %actor.name% on the back and says something to them.
      %send% %actor% A %self.name% pats you on the back and says, '@CYes, we Humans need to stick together because if we don't then these aliens will take over our planet@w'@n
      * @YThe below fires if @Goption 3@Y is selected@n
    elseif %arg% /= %self.alias% && %arg% /= 3
      %echoaround% %actor% A %self.name% whispers something to %actor.name%.@n
      %send% %actor% A %self.name% whispers to you, '@CYes, there are many like us. We call ourselves the Friends of Humanity@w'@n
      * @YThe below fires if @Goption 4@Y is selected@n
    elseif %arg% /= %self.alias% && %arg% /= 4
      %echoaround% %actor% A %self.name% nods at %actor.name% and says something.@n
      %send% %actor% A %self.name% nods at you and then says, '@CWe're a secret organisation that wants to protect Humanity.@w' He looks around suspiciously, '@CWe're everywhere so if you publically do things that we agree with...@w'
      wait 1 s
      %send% %actor% @w'@C... Then you might get an invite to join us@w' A %self.name% winks at you and then gets back to working.
    end
  end
  * @YThe below is for @Gnon-Humans@Y.@n
else
  if %cmd% ==  talk
    if %arg% == %self.alias%
      %send% %actor% ------------------------------------
      %send% %actor% @ROption @Y1) @WHello@n
      %send% %actor%  
      %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
      %send% %actor% ------------------------------------
      * @YThe below fires if @Goption 1@Y is selected@n
      %echoaround% %actor% %actor.name% greets a %self.name% but a %self.name% shakes his head at them. '@CNo time to talk. I have work to do@w'@n
      %send% %actor% A %self.name% shakes his head at you and says, '@CNo time to talk. I have work to do@w'@n
    end
  end
end
~
#490
Talk: [437] Lively Young Man~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% does not seem to hear you over the music playing in his headphones.@n 
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWhat are you listening to?@n
    %send% %actor% @ROption @Y2) @WCan you hear me?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% attempts to strike up a conversation with a %self.name% but fails as the %self.name% sings along to his music. '@CBE AS FREE AS YOU CAN BE...@w'@n
    %send% %actor% A %self.name% does not hear over the music blasting in his headphone as he keeps singing, '@CBE AS FREE AS YOU CAN BE...@w' The young man moves his head side to side, '@CA CHILDLIKE LOVE OF FANTASY...@w'@n
    * @YThe below fires if @Goption 2@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% asks the %self.name% something but he does not seem to hear %actor.name% as he continues singing, '@CTHE WORLD IS A MYSTERY FOR YOU AND ME@w'@n
    %send% %actor% A %self.name% does not appear to hear you as he continues loudly singing, '@CTHE WORLD IS A MYSTERY FOR YOU AND ME!@w'@n
  end
end
~
#491
Talk: [430] Female Tourist~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% looks at her map and then at you, '@CHey, could you please help me?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WHow can I help you?@n
    %send% %actor% @ROption @Y2) @WWhy are you looking for that?@n
    %send% %actor% @ROption @Y3) @WWhat other landmarks have you seen?@n
    %send% %actor% @ROption @Y4) @WWhy did you not stay there long?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says, '@CI am looking for the statue of Iovan but I can't find it on this map@w'@n
    * @YThe below fires if @Goption 2 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says '@CIt's on my bucket list to see statues. After I see Iovan's statue, I am going to Frigid to see Samael's@w'@n
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says, '@CI saw Temujin's statue on Konack but I didn't stay there long@w'@n
    * @YThe below fires if @Goption 4 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 4
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% appears to be angry and says, '@CThey treated me horribly because I was a woman.@w' The woman shrugs, '@CBut if you love swords then they have a lot of them.@w'@n
  end
end
~
#492
Talk: [429] Male Tourist~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% takes a picture of you, '@CSmile for the camera!@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWhere are you from?@n
    %send% %actor% @ROption @Y2) @WHave you seen anything interesting?@n
    %send% %actor% @ROption @Y3) @WWhat can you tell me about Satan City?@n
    %send% %actor% @ROption @Y4) @WWhat is Advent Duel?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% takes another picture of you, '@CI am from Satan City and I'm here on vacation@w'@n
    * @YThe below fires if @Goption 2 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% rubs his hands together and says, '@CYeah! You never see so many aliens in Satan City.@w'@n 
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says, '@CA lot of people think associate the city with Hercule but...@w' He reaches into his bag and takes out a card. '@CLocated just outside the city is the Duel Dome where you can play Advent Duel@w'@n
    * @YThe below fires if @Goption 4 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 4
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says, '@CAdvent Duel is a card game. Any description I give you won't do it justice, so go see the place out for yourself.@w'@n
  end
end
~
#493
Talk: [435] Street Thug~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% says, '@CHey, do you need help?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWho are you?@n
    %send% %actor% @ROption @Y2) @WHow can you help?@n
    %send% %actor% @ROption @Y3) @WAnything else?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% grins slyly at you, '@CHey, I can help you if you're lost@w'@n
    * @YThe below fires if @Goption 2 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% rubs his hands together and says, '@CHow about you go to west of the city and into Shadow Forest.@w'@n 
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% looks around and then whispers to you, '@CParadigm Avenue in West City has a black market store with stuff you won't usually find@w'@n He winks, '@CGo check it out@w'@n
  end
end
~
#494
Talk: [428] Little Girl~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% pops her lollipop out and smiles brightly. @W'@CHiya!@W'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWho are you?@n
    %send% %actor% @ROption @Y2) @WWhat's new?@n
    %send% %actor% @ROption @Y3) @WWhat's your favorite place in town?@n
    %send% %actor%  
    %send% %actor% @RSyntax@W: @Ytalk %self.alias% <option number>@n
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% furrows her brow a little. @W'@CMom says I shouldn't tell that to strangers...@W'@n
    * @YThe below fires if @Goption 2@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% beams a bright smile, @W'@CDinosaurs are my new favorite aminal! Sometimes, you can hear the roars all the way from the mountain!@W'@n Her eyes go wide in awe and she giggles.@n 
    * @YThe below fires if @Goption 3@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% nods vigorously. @W'@CMy favorite is Mister Frost's Ice Cream! They have so many! It's hard to choose! It's in the middle of the mall, and sometimes Mom lets us stop on trips!@W'@n
  end
end
~
#495
Talk: [425] Old man~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% grumbles about a lot in his life. "@CWhat'cha want, youngin'?@n"
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWho are you?@n
    %send% %actor% @ROption @Y2) @WWhat's an old timer like you up to?@n
    %send% %actor% @ROption @Y3) @WAny advice you can give me?@n
    %send% %actor%  
    %send% %actor% @RSyntax@W: @Ytalk %self.alias% <option number>@n
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% squints at you. "@CYour elder, that's who.@n"
    * @YThe below fires if @Goption 2@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% waves a hand out towards the street in mild irritation. '@CTrying to pass without getting run over by you youngin's running around.@n' 
    * @YThe below fires if @Goption 3@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% pokes you with his cane. '@CMake sure you're eating right. That junkfood and donuts won't do you any good. Nothing's better than a good @Rcooked@C meal. That's how you get to my age!@n'
  end
end
~
#496
Talk: [427] Nexus Police Officer~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% smiles warmly at you, '@CHello, how may I help you@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWho are you?@n
    %send% %actor% @ROption @Y2) @WAnything about the city I should know?@n
    %send% %actor% @ROption @Y3) @WAnything not related to crime?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% tips his hat at you, '@CJust an officer working to keep you and the city safe@w'@n
    * @YThe below fires if @Goption 2 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% smiles at you '@CJust be sure to be safe, we've had reports of some muggings.@w' He glances at his @DG@wl@Doc@Wk @C17@w '@CIf you don't have one of these, travel in groups to stay safe@w'@n 
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% smiles brightly, '@CThere's a movie theatre located to the east of this city. So you can go watch a movie@w' He stops to think for a second, '@COr you can follow the highway on the east and go fishing.@w'@n
    wait 1 s
    %send% %actor% %self.name% stops smiling and looks at you seriously. '@CBut you better have a fishing permit or else...@w'@n
  end
end
~
#497
Talk: [426] Young Woman~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% rolls her eyes at you, '@CWhat?! Lark can you not see that I'm lark... On the phone?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWho are you?@n
    %send% %actor% @ROption @Y2) @WWhat do you do?@n
    %send% %actor% @ROption @Y3) @WDo you know anything helpful?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% speaks into her cellphone whilst looking at you, '@COh my gawd, this creeper just asked who I am!@w'@n
    * @YThe below fires if @Goption 2 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% takes a pictue of you '@CWhat I do is none of your business, creep!@w'@n 
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% points to the north, '@CYeah! Lark... Why don't you go to the mountains and lark go get eaten by a dinosaur or something!@w'@n
  end
end
~
#498
(Space Mall) Pilot [5812]~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% says, '@CWhat would you like to talk about?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWho are you?@n
    %send% %actor% @ROption @Y2) @WWhat do you do?@n
    %send% %actor% @ROption @Y3) @WWhat's so special about the Space Mall?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% shakes his head, '@CThat's too personal, sorry. I can tell you what I do though@w'@n
    * @YThe below fires if @Goption 2 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% points to the small ship behind him and says '@CThis is the fastest ship in the galaxy, I can fly you to the Space Mall quicker than any of these public ships@w'@n 
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% winks at you, '@CGive me 100 zenni and you can go find out yourself@w'@n
  end
end
~
#499
Walker's Talk Trigger~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% says, '@CWhat would you like to talk about?@w'@n
    %send% %actor% 
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1) @WWho are you?@n
    %send% %actor% @ROption @Y2) @WWhat do you recommend I buy from you?@n
    %send% %actor% @ROption @Y3) @WAnything interesting happening around the city?@n
    %send% %actor%  
    %send% %actor% @RSyntax@w: @Ytalk %self.alias% <option number>@w
    %send% %actor% ------------------------------------
    * @YThe below fires if @Goption 1@Y is selected@n
  elseif %arg% /= %self.alias% && %arg% /= 1
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says, '@CHey! I'm Walker! I sell the best supplies on Earth! Take a look!'
    * @YThe below fires if @Goption 2 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 2
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says, '@CI recommend you never leave the city without a flashlight because you never want to in the wild without some light@w' %self.name% points to the bullet-in board, '@CYou can also look at that if you want...@w'@n 
    * @YThe below fires if @Goption 3 @Yis selected@n
  elseif %arg% /= %self.alias% && %arg% /= 3
    %echoaround% %actor% %actor.name% strikes up a conversation with %self.name%
    %send% %actor% %self.name% says, '@CI heard that Rosewater Park's Park Officer was offering a reward for some honest work.@w'@n
  end
end
~
$~
