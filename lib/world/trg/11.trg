#1122
Android Eighteen~
0 g 100
~
wait 1 s
say Ugh, I'm going to kill him if he makes me wait on around on this stupid Island any longer.
if (%actor.race% == android)
 wait 3 s
 say Hmm you seem to be like me. Were you built by that old fool?
 wait 2 s
 say Well I have nothing better to do right now. I can show you a thing or two if you like...
else
 wait 3 s
 %echo% %self.name%@W shouts, Krillin! You better hurry up!
end
~
#1123
Krillin Newb Quest Greet~
0 g 100
~
if (%actor.class% == Roshi)
    if (%actor.level% <= 2)
        wait 1
        emote sees %actor.name% leave the house and waves him over, &1Hey! Are you a new student of Master Roshi?&2
        wait 2 s
        emote crosses his arms and lets out a 'Hmm', &1How about I give you some friendly advice?&2
        wait 2 s
        emote points north, &1Head north from here, you'll see a sign. Just @Glook@C at the @Gsign@C and it will show you how to get to what I think is a good training area for you, Rosewater Park.@n&2
        wait 2 s
        emote goes back to his training, &1Remember, follow the signs to the park.&2
    
    elseif (%actor.level% >= 5 && %actor.level% <= 9 && !%actor.varexists(kq1complete)% && !%actor.varexists(onkq1) && !%actor.varexists(kqcomplete)%)
        wait 1
        emote waves over %actor.name%, &1Well, seems like you're getting stronger. And it's all thanks to my advice of starting in the park!&2 %self.name% stands with his hands on his waist, a smug look on his face.&2
        wait 2 s
        emote coughs and says, &1Well anyway, since you seem stronger now, maybe I'll tell you another location you could train at... And if you bring me back an item from there, I'll even give you a little bit of money.&2
        wait 2 s
        emote points north, &1If you go north to the sign again, go west instead until you see another path into the city. Go north, then west, then north again until you come to the intersection of @GEvergreen@C and @GEagle@C street.@n&2
        wait 2 s
        say From there, head east a bit and you will come to the @GHeaven's Gate Dojo@C. There, you can find some people who you can spar with. If you fight one of the White Belts there, bring me back their belt, and I'll give you something in return.@n
        wait 3 s
        emote rubs the back of his head and looks a bit worried, &1You can either beat the crap out of them, or you could try asking them for it, they might be willing to part with it for a little cash...&2
        set onKQ1
        remote onKQ1 %actor.id%
        
    elseif (%actor.level% >= 10 && %actor.level% <= 14 && !%actor.varexists(kq2complete)% && !%actor.varexists(onkq2)% && !%actor.varexists(kqcomplete)%)
        wait 1
        emote waves over %actor.name%, &1Wow... It seems like you're getting stronger faster than Goku did.&2 A sad look crosses %self.name% for a brief second.
        wait 2 s
        emote blinks, &1Ah well, I have a suggestion for a new training location for you. The @GNorthern Plains@C. Though it's kind of a long way off.@n&2 %self.name% lets out a 'hmm'. 
        wait 2 s
        say If you practiced on @Gfocus@Cing your mind a bit you could quite easily @Gfly@C there. I've heard some people @Gfly@C into @Gspace@C to get to locations on a planet easier. I think they're crazy.@n
        wait 1 s
        chuckle
        wait 2 s
        say If you would like the challenge of walking there, though, just head north until you come across the @GNexus Field@C, from there continue north to @GCherry Blossom Mountain@C and just follow the paths there, just watch out for the bears and snakes.@n
        wait 3 s
        say Just cross the mountain and you'll be in the @GSandy Desert@C, but be careful there as well, those Scorpions can be nasty to travelers there. North of the Desert though is your target location.@n
        wait 2 s
        %echo% %self.name%'s mouth starts to water, '@CIn these plains there are Bobcats and nice...juicy...@GBuffalos@C. And if you bring back some Buffalo Meat for me, I'll make it worth it!@n'
        set onKQ2
        remote onKQ2 %actor.id%
        
    elseif (%actor.level% >= 15 && %actor.level% <= 19 && !%actor.varexists(kq3complete)% && !%actor.varexists(onkq3)% && !%actor.varexists(kqcomplete)%)
        wait 1
        emote waves over %actor.name% &1Once again...stronger. Wow. You're starting to make me look bad.&2 %self.name% lets off a nervous chuckle.
        wait 2 s
        say I have an idea of where you can train, and where you can make a little bit of cash on the side.
        wait 2 s
        emote points north, &1You probably saw them if you walked there, but on @GCherry Blossom Mountain@C, there are some Tyrannosaurus Rex's. I would like one of their skulls, so I can scare Bulma next time she's over.@n&2
        wait 3 s
        say Just head north of Nexus to get there, and climb the mountain until you see them. They're not hard to miss.
        set onKQ3
        remote onKQ3 %actor.id%
        
    elseif (%actor.level% >= 20 && !%actor.varexists(kq4complete)% && !%actor.varexists(onkq4)% && !%actor.varexists(kqcomplete)%)
        wait 1
        emote waves over %actor.name%, &1You need to stop training. You're going to make me depressed.&2
        wait 2 s
        emote sighs, &1I have one final suggestion for you, then you're on your own. But this time, I won't give you exact directions, as you'll need to learn how to get around on your own. Or ask other people for help.&2
        wait 3 s
        say In @GWest City@C there is a mine. In the mine, are some worms. I've heard these worms can sometimes eat up valuable things from the mines. I'd like you to get me a diamond that one might have eaten up. If you do this, I'll make it really worth your while.@n
        wait 2 s
        emote smiles, &1The only hint you get is names. West City, Silver Mine, and Silver Worms. Oh, and if you want to make a bit of cash, the Foreman might be seeking some help.&2
        set onKQ4
        remote onKQ4 %actor.id%     
        
    else
        if (!%actor.varexists(onkq1)% && !%actor.varexists(onkq2)% && !%actor.varexists(onkq3)% && !%actor.varexists(onkq4)%)
            wait 1 
            say Oh hey there, %actor.name%. Hope your training is going well.
        end
    end 
else
    wait 1 s
    say Hey there! If you're here for Master Roshi, he's out right now.
end
~
#1124
Roshi Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say Hello there %actor.name%, you are one of my newest students. If you need any help getting started speak with Krillin outside, he will direct you on where you can go.
 if !%actor.varexists(roshieq)%
  wait 2 sec
  say Now %actor.name% here is the official symbol of my students.
wait 1 sec
  mload obj 1100
  give symbol %actor.name%
  set roshieq
  remote roshieq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still pretty new, make sure to do your best. Reflect the pride of the Turtle Hermit style with your dedication.
 if %actor.varexists(roshieq)%
  rdelete roshieq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you are coming along great, keep at it.
elseif (%actor.level% < 20)
wait 1
say %actor.name% you are doing really well, you are one of my most dedicated students.
elseif (%actor.level% < 40)
wait 1
say %actor.name% you do me proud with your dedication. You are an exceptional student.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are one of my best students, make sure to be an example to the rest.
elseif (%actor.level% < 100)
wait 1
say %actor.name% you are nearly a master yourself, keep it up.
elseif (%actor.level% == 100)
wait 1
say %actor.name% what are you doing here still? Come to visit? I know you don't need any training, I have little to teach someone of your skill.
smile
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#1125
Launch Newb Food Greet~
0 g 100
~
if (!%actor.varexists(launchfood)%)
  wait 1
  say Oh hi there, sweetie. Since you're new here, just let me know if you need some @Gfood@C or drink, and I'll be happy to make you something!@n
else
  wait 1
  say I hope you enjoyed all the cake and tea I made for you!
end
~
#1126
Roshi Guild Guard~
0 q 100
~
if (%direction% == inside && %actor.class% != Roshi)
 return 0
 wait 1
 say You may not enter, you are not one of Master Roshi's students.
* elseif (%actor.carry%)
* return 1
* wait 1
* Say you may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == inside && %actor.class% == Roshi)
wait 1
 say Welcome in friend.
%send% %actor% @CKrillin @wsays, '@CWelcome in friend.@w'@n
return 1
end
~
#1127
Senseis sell their symbols~
0 m 1
~
if (%self.vnum% == 1125)
 eval symb 1100
 set symbname symbol
elseif (%self.vnum% == 8230)
 eval symb 8230
 set symbname symbol
elseif (%self.vnum% == 1600)
  eval symb 1602
  set symbname symbol
elseif (%self.vnum% == 1700)
  eval symb 1700
set symbname logo
elseif (%self.vnum% == 13000)
  eval symb 13000
  set symbname symbol
elseif (%self.vnum% == 11610)
  eval symb 11612
  set symbname symbol
elseif (%self.vnum% == 12000)
  eval symb 12000
  set symbname symbol
elseif (%self.vnum% == 6413)
  eval symb 6400
  set symbname mark
elseif (%self.vnum% == 2200)
  eval symb 2200
set symbname badge
elseif (%self.vnum% == 4227)
  eval symb 4265
  set symbname badge
elseif (%self.vnum% == 4226)
  eval symb 4264
  set symbname brand
elseif (%self.vnum% == 3413)
  eval symb 3421
  set symbname glyph
elseif (%self.vnum% == 15000)
  eval symb 15000
  set symbname symbol
end
if (%amount% >= 100)
 if (%amount% > 100)
  eval num %amount% - 100
  wait 1
  drop %num% zenni
  wait 1 s
  say You overpaid me.
 end
 wait 2 s
 say You lost the %symbname% I gave you eh?
 wait 2 s
 %load% obj %symb%
 give %symbname% %actor.name%
 say Try not to lose this one.
else
 return 0
 wait 1
 say No no, the price is 100 zenni.
end
~
#1128
Krillin Noob Quest Complete~
0 j 100
~
if (%actor.varexists(onkq1)% && %object.vnum% == 686)
    %echoaround% %actor% %actor.name% gives Krillin a White Belt.
    %purge% %object%
    wait 1
    emote takes the belt and says, &1Wow, you got it. I didn't think yo-I mean I totally had faith in your abilities!&2
    wait 2 s
    emote lets off an awkward chuckle.
    wait 2 s
    say Oh, right! Your reward.
    wait 1 s
    emote hands %actor.name% a bag filled with some zenni, &1Oh, and if you come back when you're stronger, I might have some something else for you to do.&2
    nop %actor.gold(500)%
    rdelete onkq1 %actor.id%
    set kq1complete
    remote kq1complete %actor.id%
elseif (%actor.varexists(onkq2)% && %object.vnum% == 1410)
    %echoaround% %actor% %actor.name% gives Krillin some Buffalo Meat.
    %purge% %object%
    wait 1
    emote takes the meat, mouth watering. &1Oh wow, you got me the meat! I've gotta get Launch to cook this.&2
    wait 2 s
    emote runs into the house and returns a few seconds later with some stuff in hand, &1Here, take these. They're some tickets for the spaceship in Nexus City. I heard they'll take you to a space station where you can get to all sorts of planets.&2
    wait 1 s
    say And take some money as well.
    nop %actor.gold(500)%
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    give all.ticket %actor.name%
    wait 1
    say Oh yeah, the spaceport is on @GShale Boulevard@C. And be sure to stop by later! I'll have something else for you to do.@n
    rdelete onkq2 %actor.id%
    set kq2complete
    remote kq2complete %actor.id%
elseif (%actor.varexists(onkq3)% && %object.vnum% == 1203)
    %echoaround% %actor% %actor.name% gives Krillin a T-Rex Skull.
    %purge% %object%
    wait 1
    emote takes the skull, &1Great! I'll surely scare Bulma with this. Though... It's a bit big. Maybe you should have killed a smaller one. Oh well...&2
    wait 2 s
    emote sighs and hands %actor.name% a bag of Zenni. &1Come by when you're stronger and I'll give you something else to do.&2
    nop %actor.gold(2000)%
    rdelete onkq3 %actor.id%
    set kq3complete
    remote kq3complete %actor.id%
elseif (%actor.varexists(onkq4)% && %object.vnum% == 2013)
    %echoaround% %actor% %actor.name% gives Krillin a Diamond.
    %purge% %object%
    wait 1
    emote takes the Diamond from %actor.name% and looks it over, &1Ooo, it looks so nice. I'm sure I can get married with this!&2
    wait 3 s
    emote puts the diamond away and grabs an orange gi that was laying nearby, &1Take this as a reward. It's a true sign that you are Roshi's student. I was actually supposed to give this to you sooner.&2 %self.name% lets off a nervous chuckle while rubbing the back of his head with one of his hands.
    %load% obj 1108
    give gi %actor.name%
    rdelete onKQ4 %actor.id%
    rdelete kq1complete %actor.id%
    rdelete kq2complete %actor.id%
    rdelete kq3complete %actor.id%
    set kqcomplete
    remote kqcomplete %actor.id%
else
    return 0
    wait 1
    %echoaround% %actor% %actor.name% attempts to give Krillin something.
    say I don't want this.
end
~
#1129
Launch Food Give~
0 d 100
food~
if (!%actor.varexists(launchfood1)% && !%actor.varexists(launchfood)%)
    wait 1 sec
    say Alright, just give me a second or two.
    wait 3 sec
    emote runs around gathering up some stuff.
    wait 3 sec
    say Here you go sweetie.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1905
    give all.cake %actor.name%
    give all.tea %actor.name%
    set launchfood1
    remote launchfood1 %actor.id%
elseif (!%actor.varexists(launchfood2)% && !%actor.varexists(launchfood)%)
    wait 1 sec
    say You want some more? I'm glad you like my cooking! I actually have some leftovers that you can have from when Goku visited not too long ago.
    wait 3 sec
    emote gathers up the remaining food and drinks.
    wait 3 sec
    emote hands the remaining food to %actor.name%, &1It totally surprised everyone that there was even a crumb left.&2
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1905
    give all.cake %actor.name%
    give all.tea %actor.name%
    set launchfood2
    remote launchfood2 %actor.id%
elseif (!%actor.varexists(launchfood3)% && !%actor.varexists(launchfood)%)
    wait 1 sec
    say Alright, more food for you! You seem much stronger than last time, too! I'm glad your training seems to be working out for you.
    wait 3 sec
    say Just baked and brewed these fresh!
    wait 1 sec
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1905
    give all.cake %actor.name%
    give all.tea %actor.name%
    rdelete launchfood1 %actor.id%
    rdelete launchfood2 %actor.id%
    set launchfood
    remote launchfood %actor.id%
else
    wait 1 sec
    say I'm sorry, but I can't give you any more cake. Roshi yelled at me when he learned that I gave you so much.
end
~
#1130
Krillin Quest Checkers~
0 g 100
~
if (%actor.varexists(onkq1)%)
    wait 1
    say You're back? Did you forget where to go?
    wait 2 s
    emote points north, &1Go north to the sign, then go west, then north again, then once again, go west. Then go north until you get to the intersection of @GEvergreen@C and @GEagle@C street, go a bit east from there until you get to @GHeaven's Gate Dojo@C.@n&2
elseif (%actor.varexists(onkq2)%)
    wait 1
    say You're back? Did you forget where to go?
    wait 2 s
    emote points north, &1Remember, go north of @GNexus@C to @GCherry Blossom Mountain@C and follow the path through it, into the @GDesert@C while continuing north and then you are there. And don't forget the Buffalo Meat!@n&2
elseif (%actor.varexists(onkq3)%)
    wait 1
    say You're back? Did you forget where to go?
    wait 2 s
    emote points north, &1Just head north and climb @GCherry Blossom Mountain@C.@n&2
elseif (%actor.varexists(onkq4)%)
    wait 1
    say You're back? Did you forget where to go?
    wait 2 s
    say Remember, the Silver Mine is in West City.
end
~
#1131
Launch Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(launchfood1)% && !%actor.varexists(launchfood)%)
        wait 1 sec
        say Alright, just give me a second or two.
        wait 3 sec
        emote runs around gathering up some stuff.
        wait 3 sec
        say Here you go sweetie.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1905
        give all.cake %actor.name%
        give all.tea %actor.name%
        set launchfood1
        remote launchfood1 %actor.id%
    elseif (!%actor.varexists(launchfood2)% && !%actor.varexists(launchfood)%)
        wait 1 sec
        say You want some more? I'm glad you like my cooking! I actually have some leftovers that you can have from when Goku visited not too long ago.
        wait 3 sec
        emote gathers up the remaining food and drinks.
        wait 3 sec
        emote hands the remaining food to %actor.name%, &1It totally surprised everyone that there was even a crumb left.&2
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1905
        give all.cake %actor.name%
        give all.tea %actor.name%
        set launchfood2
        remote launchfood2 %actor.id%
    elseif (!%actor.varexists(launchfood3)% && !%actor.varexists(launchfood)%)
        wait 1 sec
        say Alright, more food for you! You seem much stronger than last time, too! I'm glad your training seems to be working out for you.
        wait 3 sec
        say Just baked and brewed these fresh!
        wait 1 sec
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1905
        give all.cake %actor.name%
        give all.tea %actor.name%
        rdelete launchfood1 %actor.id%
        rdelete launchfood2 %actor.id%
        set launchfood
        remote launchfood %actor.id%
    else
        wait 1 sec
        say I'm sorry, but I can't give you any more cake. Roshi yelled at me when he learned that I gave you so much.
    end
else
    %send% %actor% point at what?
end
~
#1132
Standard Basic Scouter Greet~
0 g 100
/n~
if ( %actor.varexists(WTOScouter)% )
  set WTOScouter %actor.WTOScouter%
else
  set WTOScouter 1
end
eval WTOScouterCost %WTOScouter% * 50
if ( %actor.room.vnum% = %self.room.vnum%  )
  wait 1s
  say Hello, there.
  wait 1s
end
if ( %actor.room.vnum% = %self.room.vnum% )
  smile
  wait 2s
  if ( %WTOScouter% >= 2 )
    say Oh... you've already relinquished any claim you had on planets.  We have you on file.  Hm... Well, if you still want one, I could sell you one for %WTOScouterCost% zenni...
    wait 3s
    if (%actor.room.vnum% = %self.room.vnum% )
      say Just tell me "@YI want to buy a scouter@C"@n
    else
      say Jerk just left like they didn't need a scouter...
    end
  else
    say I'm an ambassador from the World Trade Organization.  We're offering free scouters out of the goodness of our hearts.  All you have to do to get one is say @YI have no claim on this planet@C.@n
  end
else
  frown
  wait 2s
  say Jerk left before I could tell them about the World Trade Organization and our free scouters.  Frieza will hear about this...
end
~
#1133
Ask for a WTO Scouter~
0 d 100
"I have no claim on this planet"~
*********************** speech & getting a scouter
if ( %actor.varexists(WTOScouter)% )
  set WTOScouter %actor.WTOScouter%
else
  set WTOScouter 1
end
if ( %WTOScouter% > 1 )
  wait 1s
  say Tsk, tsk.  You've already gotten a free scouter.  From now on, you have to give us something we want.  If you want another, tell me "@YI want to buy a scouter@C"@n
else
  wait 1s
  smile
  wait 1s
  emote smiles as %self.hisher% scouter beeps as if it recorded something.
  say Excellent.  Well, here you are.
  wait 1s
  if ( %actor.room.vnum% = %self.room.vnum% )
    eval WTOScouter %WTOScouter% + 1
    %load% obj 1132
    give gift %actor.name%
    remote WTOScouter %actor.id%
  else
    say Idiot left before getting a scouter...
  end
  wait 60s
end
~
#1134
WTO Scouter Wear~
1 j 100
~
wait 1
%echo% @G=======================================================@n
%echo% @G=  @WInitializing...@n
%echo% @G=  @n
%echo% @G=  @n
%echo% @G=  @n
%echo% @G=  @n
%echo% @G=======================================================@n
wait 1s
%echo% @G=======================================================@n
%echo% @G=@W  Thank you for using @CW@World @CT@Wrade@C O@Wrganization@W tech.@n
%echo% @G=  @n
%echo% @G=@W  Before transmitting your first broadcast, please@n
%echo% @G=@W  Access the following help files:@n
%echo% @G=  @n
%echo% @G=@R  Help @WScouter@n
%echo% @G=@R  Help @WVoice@n
%echo% @G=@R  Help @WSNet@n
%echo% @G=======================================================@n
wait 1s
~
#1135
WTO Scouter Commercials~
1 b 100
~
set myCarrier %self.worn_by%
set myRand %random.200%
if %myCarrier%
  switch %myRand%
    case 1
      %send% %myCarrier% @G==================@C[@WWTO Scouter Update@C]@G==================@n
      %send% %myCarrier% @G=  Tired of wallowing on planets owned by incompetent@n
      %send% %myCarrier% @G=  idiots?  Can't stand filthy monkeys spraying their@n
      %send% %myCarrier% @G=  pride all over the galaxy?  Be the first to provide@n
      %send% %myCarrier% @G=  information leading to the acquisition of a planet@n
      %send% %myCarrier% @G=  and you could earn a reward!@n
      %send% %myCarrier% @G=@n
      %send% %myCarrier% @G=@R  Disclaimer:@n
      %send% %myCarrier% @G=@Y  Receiving a reward does not guarantee life, liberty,@n
      %send% %myCarrier% @G=@Y  or the pursuit of happiness.@n
      %send% %myCarrier% @G========================================================@n
    break
    case 2
      %send% %myCarrier% @G==================@C[@WWTO Scouter Update@C]@G==================@n
      %send% %myCarrier% @G=  Need a pay raise?  Our glorious, fair, and wonderful@n
      %send% %myCarrier% @G=  leader, Frieza, has an offer for you!  Frieza needs@n
      %send% %myCarrier% @G=  the dragonballs from the indiginous people of planet@n
      %send% %myCarrier% @G=  Namek.  Find, capture, torture, and get the secrets@n
      %send% %myCarrier% @G=  of the dragonballs from a Namekian and earn a reward!@n
      %send% %myCarrier% @G=@n
      %send% %myCarrier% @G=@R  Disclaimer:@n
      %send% %myCarrier% @G=@Y  Receiving a reward does not guarantee life, liberty,@n
      %send% %myCarrier% @G=@Y  or the pursuit of happiness.@n
      %send% %myCarrier% @G========================================================@n
    break
    case 3
      %send% %myCarrier% @G==================@C[@WWTO Scouter Update@C]@G==================@n
      %send% %myCarrier% @G=  The World Trade Organization wants you!  Report to@n
      %send% %myCarrier% @G=  Frigid and ask about the World Trade Organization@n
      %send% %myCarrier% @G=  today!@n
      %send% %myCarrier% @G=@n
      %send% %myCarrier% @G=@R  Disclaimer:@n
      %send% %myCarrier% @G=@Y  Visiting Frigid does not guarantee life, liberty,@n
      %send% %myCarrier% @G=@Y  or the pursuit of happiness.@n
      %send% %myCarrier% @G========================================================@n
    break
    case 4
      %send% %myCarrier% @G==================@C[@WWTO Scouter Update@C]@G==================@n
      %send% %myCarrier% @G=  The World Trade Organization wants you!  Report to@n
      %send% %myCarrier% @G=  Frigid and ask about the World Trade Organization@n
      %send% %myCarrier% @G=  today!@n
      %send% %myCarrier% @G=@n
      %send% %myCarrier% @G=@R  Disclaimer:@n
      %send% %myCarrier% @G=@Y  Visiting Frigid does not guarantee life, liberty,@n
      %send% %myCarrier% @G=@Y  or the pursuit of happiness.@n
      %send% %myCarrier% @G========================================================@n
    break
    case 5
    break
    case 6
    break
    default
    break
  done
end
~
#1136
WTO Scouter:  Pay for it~
0 d 100
"I want to buy a scouter"~
if (%actor.varexists(WTOScouter)%)
  eval WTOScouterCost %actor.WTOScouter% * 50
  wait 1s
  if ( %actor.room.vnum% = %self.room.vnum% )
    if (%actor.gold% >= %WTOScouterCost% )
      %load% obj 1132
      wait 1
      give scouter %actor.name%
      nop %actor.gold(-%WTOScouterCost%)%
      wait 1
      emote accepts zenni in exchange for the scouter.  @C'@WAlright.  Thanks for choosing World Trade Organization Tech!@C'@n
      eval WTOScouter %actor.WTOScouter% + 1
      remote WTOScouter %actor.id%
    else
      wait 1s
      say Don't waste my time if you don't have the zenni...
    end
  else
    say Okay... way to leave before we complete our deal.  What an ass.
  end
end
~
$~
