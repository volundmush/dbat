#16800
Giiges Tunnel Trigger~
0 k 70
~
eval num %random.4%
if (%num% == 3)
 %echo% %self.alias% thrashes around suddenly.
 wait 1 s
 %echo% %self.alias% folds over and slams its maw into the rock below. Within moments the beast has disappeared underground!
 eval numm %random.9%
 eval destination %numm% + 16849
 if (%destination% == %self.room.vnum%)
  eval destination %numm% + 16849
 end
 mgoto %destination%
 wait 1
 %echo% Suddenly %self.alias% emerges through the ground! Its maw aims upward for a second before coming fully upright a hundred feet into the air!
 wait 1 s
 %echo% %self.alias% raises its head and opening wide its maw begins to absorb huge volumes of air as its wounds heal!
 mheal pl 8
end
~
#16801
Giiges Death~
0 f 100
~
%echo% @YA large pile of topaz stones are puked up from %self.alias%@Y's innards as it dies!@n
eval roll %random.7%
while (%roll% > 0)
 eval roll %roll% - 1
 %load% obj 16816
done
eval chance %random.10%
if (%chance% == 10)
 %load% obj 16817
end
drop all
~
#16802
Watchman Greet~
0 g 100
~
if (!%actor.varexists(kilnak)%)
 wait 1 s
 %echo% %self.alias% makes a threatening buzzing noise.
 wait 1 s
 %echo% %self.alias% moves into an aggressive stance.
elseif (%actor.kilnak% == Friend)
 wait 1 s
 %echo% %self.alias% makes a welcoming whistling noise.
 wait 2 s
 say Greetings, tribe-friend.
elseif (%actor.kilnak% == Permission)
 wait 1 s
 %send% %actor% %self.alias% watches you with a cold silent stare.
elseif (%actor.kilnak% == Enemy)
 wait 1
 say Enemy of the tribe! You die now!
 wait 1
 kick %actor.name%
end
~
#16803
Kilnak Watchman Chief Greet~
0 g 100
~
if (!%actor.varexists(kilnak)%)
 wait 1 s
 %echo% %self.alias% appears to tense up.
 wait 2 s
 say Who are you? What you do here?
 wait 3 s
 say You! Speak up! What you do here? You be @Gfriend@C or @Genemy@C?
elseif (%actor.kilnak% == Friend)
 wait 1 s
 %echo% %self.alias% holds his right arm up in greeting.
 wait 3 s
 say Things go well with you, friend?
elseif (%actor.kilnak% == Permission)
 wait 1 s
 %echo% %self.alias% appears to tense up.
 wait 3 s
 say You need to hurry. Get permission from elder. See if you stay or not.
elseif (%actor.kilnak% == Enemy)
 wait 1
 %echo% %self.alias% buzzes with rage!
 wait 1
 slam %actor.name%
end
~
#16804
Kilnak Watchman Chief Permission 1~
0 c 100
friend~
if (!%actor.varexists(kilnak)%)
 wait 1
 say Fine. I will trust you. Seem like trust-friend.
 wait 3 s
 say You have permission enter village. Speak to chief. He decide if you stay or not.
 set kilnak Permission
 remote kilnak %actor.id%
elseif (%actor.kilnak% == Friend)
 wait 1
 say Ha ha ha! You funny. Already tribe-friend, no need ask stay.
 wait 2 s
 say Stay! Be happy!
elseif (%actor.kilnak% == Permission)
 wait 1
 say Yes! You have permission! Talk to chief! Now!
elseif (%actor.kilnak% == Enemy)
 wait 1
 say You have permission die! YES!!!
 punch %actor.name%
end
~
#16805
Giruol Greet~
0 g 100
~
if (!%actor.varexists(kilnak)%)
 wait 1
 %echo% %self.alias% stomps his foot on the ground! A large hole is formed beneath his foot from this stomp.
 wait 2 s
 say You... speak to watch chief. No enter right now.
elseif (%actor.kilnak% == Friend)
 wait 1
 %echo% %self.alias% holds out fist for friendly @Gbump@n.
 set bump %actor.name%
 remote bump %self.id%
elseif (%actor.kilnak% == Permission)
 wait 1
 %echo% %self.alias% shuffles aside slowly while keeping his guard up.
elseif (%actor.kilnak% == Enemy)
 wait 1
 %echo% %self.alias% slams his fists into his exoskeleton a few times and then charges!
 wait 1
 uppercut %actor.name%
end
~
#16806
Kilnak Watchman Chief Permission 2~
0 c 100
enemy~
if (!%actor.varexists(kilnak)%)
 wait 1
 %echo% %self.name% roars with anger!
 set kilnak Enemy
 remote kilnak %actor.id%
 wait 1
 elbow %actor.name%
elseif (%actor.kilnak% == Friend)
 wait 1
 %echo% chuckles at the joke.
elseif (%actor.kilnak% == Permission)
 wait 1
 %echo% %self.name% roars with anger!
 set kilnak Enemy
 remote kilnak %actor.id%
 wait 1
 knee %actor.name%
elseif (%actor.kilnak% == Enemy)
 wait 1
 say I kill you!!!
end
~
#16807
Make Enemy of Kilnak Tribe~
0 k 100
~
if (!%actor.varexists(kilnak)%)
 set kilnak Enemy
 remote kilnak %actor.id%
elseif (%actor.kilnak% != Enemy)
 set kilnak Enemy
 remote kilnak %actor.id%
end
~
#16808
Make Enemy of Kilnak 2~
0 f 100
~
if (!%actor.varexists(kilnak)%)
 set kilnak Enemy
 remote kilnak %actor.id%
elseif (%actor.kilnak% != Enemy)
 set kilnak Enemy
 remote kilnak %actor.id%
end
~
#16809
Heart of Topaz Command~
1 c 3
invoke~
wait 1
%echo% %self.shortdesc% begins to glow softly a pure yellow light.
wait 2 s
%echo% %self.shortdesc% fades out of existence with a bright flash.
wait 1
%send% %actor% You feel like your mind has expanded.
eval %actor.prac(1000)%
%send% %actor% @D<@YPS@D: @G1,000@D>@n
%purge% %self%
~
#16810
Giruol Bump~
0 c 100
bump~
if (!%actor.varexists(kilnak)%)
 wait 1
 %send% %actor% %self.name% ignores your fist.
elseif (%actor.varexists(kilnak)%)
 if (%actor.kilnak% == Enemy)
  wait 1
  %echo% screeches in anger!
  wait 1 s
  headbutt %actor.name%
 elseif (%actor.kilnak% == Permission)
  wait 1
  %send% %actor% %self.name% doesn't trust you yet, and so ignores your friendly request for a fist bump.
 elseif (%actor.kilnak% == Friend)
  wait 1
  emote bumps his fist with *%actor.name% in a friendly manner.
 end
end
~
$~
