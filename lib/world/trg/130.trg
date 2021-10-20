#13000
Krane Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say %actor.name%, you are one of my newest students. I know you will need help, you are weak right now.
wait 1 s
say %actor.name% go speak with Tien, he will tell you what to do.
 if !%actor.varexists(kraneeq)%
  wait 2 sec
  say Now %actor.name% here is the official symbol of my students, wear it with pride.
wait 1 sec
  mload obj 13000
  give symbol %actor.name%
  set kraneeq
  remote kraneeq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still pretty new, make sure to do your best if you want to prove your worth.
 if %actor.varexists(roshieq)%
  rdelete roshieq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you are coming along alright, don't slack.
elseif (%actor.level% < 20)
wait 1
say %actor.name% you are doing well, don't get lazy.
elseif (%actor.level% < 40)
wait 1
say %actor.name% you do me proud with your dedication.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are one of my best students, don't let it go to your head.
elseif (%actor.level% < 100)
wait 1
say %actor.name% you may one day replace me as master.
elseif (%actor.level% == 100)
wait 1
say %actor.name% what are you doing here still? You..are...beyond my ability to teach, go....
smile
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#13001
Krane Guild Guard~
0 q 100
~
if (%direction% == inside && %actor.class% != Krane)
 return 0
 wait 1
 say You may not enter, Krane doesn't teach cowards like you.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == inside && %actor.class% == Krane)
wait 1
 %send% %actor% Tienshinhan says, 'Greetings comrad. You may enter.'
return 1
end
~
#13002
Tien Newbie Quest~
0 g 100
~
if (%actor.class% == Krane)
    if (%actor.level% <= 2)
        wait 1
        emote sees %actor.name% exit the dojo, and raises a hand in greeting, &1Hey, you must be Krane's new student.&2
        wait 2 s
        emote glances at the door to the dojo, &1Krane can be pretty harsh sometimes, but it's definitely worth it. I'll give you some help.&2
        wait 2 s
        emote gestures to the west, &1Follow the trail to the main road, then head south to the sign. Follow its instructions and go to the Satan City Zoo. Once you're in there, you can use that place to train your body.&2
        wait 1 s
        say Krane wants all the newcomers to go there, so just train to your hearts content, then come back to me.
        wait 2 s
        emote steps back in front of the doors, folding his arms over his chest, &1Remember, follow the signs, and train well.&2 He nods in your direction.
    
    elseif (%actor.level% >= 5 && %actor.level% <= 9 && !%actor.varexists(TienQ1Complete)% && !%actor.varexists(onTienQ1) && !%actor.varexists(TienQComplete)%)
        wait 1
        emote waves at %actor.name% as they approach, a smile on his face. &1Hey! Looks like you got stronger. Told you Krane's advice pays off.&2
        wait 2 s
        emote chuckles, then sighs contently, &1He's at least a little pleased with your newfound strength, so he's recommended a new training zone. It is called @GHeaven's Gate Dojo@n.
        wait 2 s
        emote points north, &1If you head north, instead of south into Satan City, you'll find the ocean. Keep going north until you reach the other beach. 
        wait 2 s
        say From there, head east, then north to the entrance of Rosewater park. Move to the west a bit and you will come to the @GHeaven's Gate Dojo@C.There, you can find some people who you can spar with. If you fight one of the White Belts there, bring me back their belt, and I'll give you something in return.@n
        wait 3 s
        emote looks at you curiously, tilting his head to the side, &1Look, if you keep training hard, Krane'll start appreciating you more. Go on, then. Get training, and remember, bring me back a belt.&2 He turns and continues practicing his techniques.
        set onTienQ1
        remote onTienQ1 %actor.id%
        
    elseif (%actor.level% >= 10 && %actor.level% <= 14 && !%actor.varexists(TienQ2Complete)% && !%actor.varexists(onTienQ2)% && !%actor.varexists(TienQComplete)%)
        wait 1
        emote waves over %actor.name%, &1Hey, you've gotten a lot stronger since we last met! Keep working at it, and you'll be able to train with me, eventually.&2
        wait 2 s
        emote nods, &1I've got a new training area for you, even if its a way out there. It is called the @GNorthern Plains@n@C. If you have trained @Gfocus@Cing your mind with Krane, you should be able to @Gfly@C to @Gspace@C to get there.@n&2
        wait 2 s
        emote strokes his chin in thought, &1Though, it could be said that you can @Gland@C at most places when you do that. I don't know, I don't do it often.@n&2
        wait 1 s
        chuckle
        wait 2 s
        say If you would like the challenge of walking there, though, just head north until you come across the @GNexus Field@C, from there continue north to @GCherry Blossom Mountain@C and just follow the paths there, just watch out for the bears and snakes.@n
        wait 3 s
        say Just cross the mountain and you'll be in the @GSandy Desert@C, but be careful there as well, those Scorpions can be nasty to travelers there. North of the Desert though is your target location.@n
        wait 2 s
        emote smiles slightly, &1Once there, you can fight bobcats and buffalo. The buffalo can get pretty strong, so be careful when fighting them.  If you can bring me back a tasty buffalo steak, it'll be worth your time.&2 %self.name% nods, as if confirming a thought to himself.
        set onTienQ2
        remote onTienQ2 %actor.id%
        
    elseif (%actor.level% >= 15 && %actor.level% <= 19 && !%actor.varexists(TienQ3Complete)% && !%actor.varexists(onTienQ3)% && !%actor.varexists(TienQComplete)%)
        wait 1
        emote waves over %actor.name% &1Once again...stronger. You're doing fantastic, and Krane is starting to think better of you.&2
        wait 1
        chuckle
        wait 2 s
        say I have an idea of where you can train, and where you can make a little bit of cash on the side.
        wait 2 s
        emote points north, &1You probably saw them if you walked there, but on @GCherry Blossom Mountain@C, there are some Tyrannosaurus Rex's. We need a skull from one of them. Krane's old one got busted when some idiot trainee stumbled into it and it shattered.  Bring me one that's in good condition, and I'll mention your work to our master.@n&2
        wait 3 s
        say Just head north of Nexus to get there, and climb the mountain until you see them. They're not hard to miss.
        set onTienQ3
        remote onTienQ3 %actor.id%
        
    elseif (%actor.level% >= 20 && !%actor.varexists(TienQ4Complete)% && !%actor.varexists(onTienQ4)% && !%actor.varexists(TienQComplete)%)
        wait 1
        emote waves over %actor.name%, &1Your strength is rising, I can feel it.  Keep working.&2
        wait 2 s
        emote scratches the back of his head, &1This is my last suggestion for you, and I can't really give you directions. Sooner or later, you'll have to figure things out for your own, or have someone tell you. Krane doesn't like it when he has to hold his trainee's hands.&2
        wait 3 s
        say In northern @GWest City@C there is a mine. The inside of which is infested with worms and other odd creatures. If you fight some worms for me, eventually you'll get one that has a diamond. Every student has to do this, to help pay back the school.@n
        wait 2 s
        emote leans in, as if sharing a secret, &1It's what keeps this place running, but don't tell Krane I said that!&2
        wait 2 s
        emote returns to his casual position once more, looking at the doors of the dojo once more, &1I can't tell you anything other than that; West City, Silver Mine, Silver Worm, and Diamond. If you need some spending money, ask the Foreman for a job mining.&2
        set onTienQ4
        remote onTienQ4 %actor.id%     
        
    else
        if (!%actor.varexists(onTienQ1)% && !%actor.varexists(onTienQ2)% && !%actor.varexists(onTienQ3)% && !%actor.varexists(onTienQ4)%)
            wait 1
            say Oh hey there, %actor.name%. Hope your training is going well.
        end
    end 
else
    wait 1 
    say Oh hi there. If you're here for Krane, he's not available.
end
~
#13003
Chaotzu Newb Food Greet~
0 g 100
~
if (!%actor.varexists(kranefood)%)
    wait 1
    say Hey there! Since you're new here, I have some spare @Gfood@C I can give you. Just ask me.@n
else
   wait 1
   say Hope your training is going well, %actor.name%!
end
time
~
#13004
George Trigger~
0 g 100
~
wait 2 sec
say May I take your order?
wait 3 sec
say Geez I can't wait till my shift ends...
wait 4 sec
say Gotta get home and back to my MUD. There I rule all...
~
#13005
Tien Newb Quest Complete~
0 j 100
~
if (%actor.varexists(onTienQ1)% && %object.vnum% == 686)
    %echoaround% %actor% %actor.name% gives Tien a White Belt.
    %purge% %object%
    wait 1
    emote takes the belt and says, &1Good. You brought the belt, now we have another belt to replace our old one...&2
    wait 2 s
    emote hands it off to someone inside.
    wait 2 s
    say Oh, right! Your reward.
    wait 1 s
    emote hands %actor.name% a bag filled with some zenni, &1Oh, and if you come back when you're stronger, I might have some something else for you to do.&2
    nop %actor.gold(500)%
    rdelete onTienQ1 %actor.id%
    set TienQ1Complete
    remote TienQ1Complete %actor.id%
    
elseif (%actor.varexists(onTienQ2)% && %object.vnum% == 1410)
    %echoaround% %actor% %actor.name% gives Tien some Buffalo Meat.
    %purge% %object%
    wait 1
    emote takes the meat, smiling, &1Good, you brought back the meat. Now we'll have some extra dinner for tonight...&2 he trails off as he opens the door to the dojo.
    wait 2 s
    emote runs inside for a few seconds, then comes back out with some items, &1Here, these are for you.  The tickets are for Nexus City's Spaceport. I've heard that they can take you to a space station that leads to other planets. Sounds amazing to me.&2
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
    rdelete onTienQ2 %actor.id%
    set TienQ2Complete
    remote TienQ2Complete %actor.id%
    
elseif (%actor.varexists(onTienQ3)% && %object.vnum% == 1203)
    %echoaround% %actor% %actor.name% gives Tien a T-Rex Skull.
    %purge% %object%
    wait 1
    emote takes the skull, &1Great! Krane'll definitely like this one. I'll take it to him in just a second.&2
    wait 2 s
    emote tosses %actor.name% a bag full of Zenni, &1Your training is going well, come back to me when you're stronger again, and I'll have something else for you to do.&2
    nop %actor.gold(2000)%
    rdelete onTienQ3 %actor.id%
    set TienQ3Complete
    remote TienQ3Complete %actor.id%
    
elseif (%actor.varexists(onTienQ4)% && %object.vnum% == 2013)
    %echoaround% %actor% %actor.name% gives Tien a Diamond.
    %purge% %object%
    wait 1
    emote takes the Diamond from %actor.name% and looks it over, &1Great! This is a nice big one, and I'm sure I could get a decent amount of Zenni off of it...&2 He trails off, placing the diamond away.
    wait 3 s
    emote grabs a green shirt with the Krane symbol on the front and hands it over to %actor.name%. &1Here, take this. It'll truly show that you are one of the Krane Dojo, a proud group of warriors.  Krane wanted to make sure you were trained well before you went wearing this around, but be proud! You did well with your first set of training with Krane.  Now, go on, train, and don't make our sensei angry.&2 He chuckles, returning to the inside of the dojo.
    %load% obj 13061
    give gi %actor.name%
    rdelete onTienQ4 %actor.id%
    rdelete TienQ1Complete %actor.id%
    rdelete TienQ2Complete %actor.id%
    rdelete TienQ3Complete %actor.id%
    set TienQComplete
    remote TienQComplete %actor.id%
    
else
    return 0
    wait 1
    %echoaround% %actor% %actor.name% attempts to give Tien something.
    say I don't want this.
end
~
#13006
Chatozu Food Give~
0 d 100
food~
if (!%actor.varexists(kranefood1)% && !%actor.varexists(kranefood)%)
    wait 1 sec
    say Okay! I will get something for you.
    wait 2 sec
    emote floats around the kitchen, gathering up some things.
    wait 2 sec
    say Here you go! I hope you enjoy it.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1614
    give all.cake %actor.name%
    give all.cup %actor.name%
    set kranefood1
    remote kranefood1 %actor.id%
elseif (!%actor.varexists(kranefood2)% && !%actor.varexists(kranefood)%)
    wait 1 sec
    say Do you enjoy it? I hope you do because I make it all!
    wait 2 sec
    emote opens the oven and pulls out a pan.
    wait 2 sec
    emote hands the food to %actor.name%, &1Here you go, some fresh ones!%2
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1614
    give all.cake %actor.name%
    give all.cup %actor.name%
    set kranefood2
    remote kranefood2 %actor.id%
elseif (!%actor.varexists(kranefood3)% && !%actor.varexists(kranefood)%)
    wait 1 sec
    say Tien says I shouldn't give you anymore. So don't tell him about this, okay?
    wait 2 sec
    emote hands %actor.name% some food, &1Remember, don't tell him, okay?&2
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1614
    give all.cake %actor.name%
    give all.cup %actor.name%
    rdelete kranefood1 %actor.id%
    rdelete kranefood2 %actor.id%
    set kranefood
    remote kranefood %actor.id%
else
    wait 1 sec
    say I'm sorry, Tien found out and scolded me. I can't give you anymore.
end
~
#13007
Tien Quest checkers~
0 g 100
~
if (%actor.varexists(onTienQ1)%)
    wait 1
    say You're back? Did you forget where to go?
    wait 2 s
    emote points north &1Go north to the sign, then go west, then north again, then once again, go west. Then go north until you get to the intersection of @GEvergreen@C and @GEagle@C street, go a bit east from there until you get to @GHeaven's Gate Dojo@C.@n&2
elseif (%actor.varexists(onTienQ2)%)
    wait 1
    say You're back? Did you forget where to go?
    wait 2 s
    emote points north, &1Remember, go north of @GNexus@C to @GCherry Blossom Mountain@C and follow the path through it, into the @GDesert@C while continuing north and then you are there. And don't forget the Buffalo Meat!@n&2
elseif (%actor.varexists(onTienQ3)%)
    wait 1
    say You're back? Did you forget where to go?
    wait 2 s
    emote points north, &1Just head north and climb @GCherry Blossom Mountain@C.@n&2
elseif (%actor.varexists(onTienQ4)%)
    wait 1
    say You're back? Did you forget where to go?
    wait 2 s
    say Remember, the Silver Mine is in West City.
end
~
#13008
Chaotzu Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(kranefood1)% && !%actor.varexists(kranefood)%)
        wait 1 sec
        say Okay! I will get something for you.
        wait 2 sec
        emote floats around the kitchen, gathering up some things.
        wait 2 sec
        say Here you go! I hope you enjoy it.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1614
        give all.cake %actor.name%
        give all.cup %actor.name%
        set kranefood1
        remote kranefood1 %actor.id%
    elseif (!%actor.varexists(kranefood2)% && !%actor.varexists(kranefood)%)
        wait 1 sec
        say Do you enjoy it? I hope you do because I make it all!
        wait 2 sec
        emote opens the oven and pulls out a pan.
        wait 2 sec
        emote hands the food to %actor.name%, &1Here you go, some fresh ones!%2
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1614
        give all.cake %actor.name%
        give all.cup %actor.name%
        set kranefood2
        remote kranefood2 %actor.id%
    elseif (!%actor.varexists(kranefood3)% && !%actor.varexists(kranefood)%)
        wait 1 sec
        say Tien says I shouldn't give you anymore. So don't tell him about this, okay?
        wait 2 sec
        emote hands %actor.name% some food, &1Remember, don't tell him, okay?&2
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1614
        give all.cake %actor.name%
        give all.cup %actor.name%
        rdelete kranefood1 %actor.id%
        rdelete kranefood2 %actor.id%
        set kranefood
        remote kranefood %actor.id%
    else
        wait 1 sec
        say I'm sorry, Tien found out and scolded me. I can't give you anymore.
    end
else
    %send% %actor% Point at what?
end
~
$~
