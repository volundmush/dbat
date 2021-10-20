#9200
Lord Ruhtra and Nilrem play~
2 b 100
~
wait 30 s
%echo% @nA dashing, handsome male Konatsu appears on stage, closely followed by an Eldery male with a long grey beard.
wait 10 s
%echo% @nThe dashing male turns to the elderly one and enquires, "@YIs this the spot, Nilrem?@n"
wait 10 s
%echo% @nNilrem scans the surroundings for a moment and begins stroking his beard before replying, "@YYes, Lord Ruhtra, I believe so.@n"
wait 10 s
%echo% @nNilrem raises his arm and points to the left of the stage where a rock mound is, with a glinting sword stuck within it.
wait 10 s
%echo% @nLord Ruhtra exclaims, "@YWell, so you were right Nilrem.@n"
wait 10 s
%echo% @nRuhtra strides towards the rock and beams a big smile belying his confidence.
wait 10 s
%echo% @nNilrem joins Ruhtra near the sword in the rock.
wait 10 s
%echo% @nPerplexingly, Nilrem poses the question, "@YAre you sure you want to proceed with this my lord? Many have tried in the past and all have failed.@n"
wait 10 s
%echo% @n"@YNilrem, I am destined to remove this sword from this stone, I will do this.@n" Ruhtra proclaims.
wait 10 s
%echo% @nNilrem frowns slightly and hopes that he is right about Ruhtra.
wait 10 s
%echo% @nThe dashing Lord Ruhtra grasps the hilt of the sword and begins pulling, groaning as he does.
wait 30 s
%echo% @nReleasing his grasp of the sword's hilt, frustratingly, Ruhtra shouts "@YIt won't budge!@n"
wait 10 s
%echo% T@nhe wizened old man Nilrem, casts a steely glaze upon Ruhtra and says, "@YTry again, maybe you loosened it.@n"
wait 10 s
%echo% @nOnce again, Lord Ruhtra grasps the hilt of the sword but this time placing his right foot upon the rock to gain more leverage.
wait 10 s
%echo% @nHe continues to try and pull the sword out, shouting, "@YCome on!@n" in frustration as he does.
wait 30 s
%echo% @nFinally the sword slides out of the rock and Lord Ruhtra holds it aloft proudly.
wait 10 s
%echo% @n"@YI Lord Ruhtra, claim Excalibur as my own, so that I may use this fine blade to forge a new kingdom in my vision!@n" Ruhtra proclaims, still holding the sword aloft.
wait 10 s
%echo% @nNilrem approaches Lord Ruhtra and insists that they leave at once.
wait 10 s
%echo% @nLord Ruhtra sheathes Excalibur and both the men wonder off into the Forest.
wait 120 s
~
#9210
Shamrock Spins the wheel of fortune~
0 m 100
1000~
if !%actor.varexists(shamrockprize)%
  emote checks to make sure %actor.name% isn't already in line to win a prize.
  if !%self.varexists(alreadyspinning)%
    set alreadyspinning
    remote alreadyspinning %self.id%
    wait 1 s
    say Alrighty then %actor.name%, I'll just spin this 'ere wheel and we will see who the lucky one is today.
    wait 2 s
    emote approaches the Wheel of fortune
    wait 2 s
    emote grips the handle and spins it firmly.
    wait 3 s
    emote grins with self-assurance as the wheel spins around and around.
    wait 2 s
    %echo% The wheel of fortune begins to slow and is about to stop on...
    eval result %random.8%
    switch %result%
      case 1
        %echo% The wheel comes to a stop on "You're outta luck!"
        wait 2 s
        say Better luck next time, %actor.name%.
        rdelete alreadyspinning %self.id%
      break
      case 2
        %echo% The wheel comes to a stop on "You've found the pot of gold!"
        wait 2 s
        say Well, I guess everyone's luck has to run out sometime, even mine.
        wait 2 s
        say You've earned a prize. Type claim and I'll give you one. At least, so long as I'm not already spinning the Wheel!
        set shamrockprize
        remote shamrockprize %actor.id%
        rdelete alreadyspinning %self.id%
      break
      case 3
        %echo% The wheel comes to a stop on "There's no gold at the end of this rainbow!"
        wait 2 s
        say Easy money %actor.name%, easy money.
        wait 2 s
        emote grins.
        rdelete alreadyspinning %self.id%
      break
      case 4
        %echo% The wheel comes to a stop on "You lose!"
        wait 2 s
        say Sorry %actor.name%, care to try again?
        rdelete alreadyspinning %self.id%
      break
      case 5
        %echo% The wheel comes to a stop on "You lose!"
        wait 2 s
        say Sorry %actor.name%, care to try again?
        rdelete alreadyspinning %self.id%
      break
      case 6
        %echo% The wheel comes to a stop on "Lucked out"!
        wait 2 s
        say Better luck next time, %actor.name%.
        rdelete alreadyspinning %self.id%
      break
      case 7
       %echo% The wheel comes to a stop on "Unlucky Charm"!
       wait 2 s
       say Woo hoo, Shamrock's luck never runs out!
       rdelete alreadyspinning %self.id%
     break
     default
       %echo% The wheel comes to a stop on "Broken Mirror"!
       wait 2 s
       say You're one unlucky person, %actor.name%. For sure, for sure..
       rdelete alreadyspinning %self.id%
     break
   done
  else
    %send% %actor.name% The wheel is already spinning, please wait.
  end
else
  %send% %actor.name% You've won a prize. You need to claim before you can spin again.
end
~
#9211
Shamrock awards a prize~
0 c 100
claim~
if %cmd% == claim
  if !%self.varexists(alreadyspinning)%
    wait 1 s
    say Alright, let's see if you have won something, %actor.name%.
    if !%actor.varexists(shamrockprize)%
      wait 2 s
      say Hold on a second, you haven't even won a prize, %actor.name%!
    else
      wait 2 s
      say Ahh, we have a winner. Let me just get your prize.
      eval prize %random.8%
      switch %prize%
        case 1
          %load% obj 9206
          give hat %actor.name%
          wait 2 s
          say Enjoy!
          rdelete shamrockprize %actor.id%
        break
        case 2
          %load% obj 9207
          give vest %actor.name%
          wait 2 s
          say Enjoy!
          rdelete shamrockprize %actor.id%
        break
        case 3
          %load% obj 9208
          give shirt %actor.name%
          wait 2 s
          say Enjoy!
          rdelete shamrockprize %actor.id%
        break
        case 4
          %load% obj 9209
          give pants %actor.name%
          wait 2 s
          say Enjoy!
          rdelete shamrockprize %actor.id%
        break
        case 5
          %load% obj 9210
          give shoes %actor.name%
          wait 2 s
          say Enjoy!
          rdelete shamrockprize %actor.id%
        break
        case 6
          %load% obj 9211
          give pendant %actor.name%
          wait 2 s
          say Enjoy!
          rdelete shamrockprize %actor.id%
        break
        case 7
          %load% obj 9212
          give cane %actor.name%
          wait 2 s
          say Enjoy!
          rdelete shamrockprize %actor.id%
        break
        default
          %load% obj 9213
          give pot %actor.name%
          wait 2 s
          say Enjoy!
          rdelete shamrockprize %actor.id%
        break
      done
    end
  else
    %send% %actor.name% The wheel of fortune is currently spinning.
  end
end
~
#9212
Shamrock Greet~
0 g 100
~
if %actor.is_pc%
wait 1 s
say Step right up, come play my game of chance. Take a spin on the Wheel of Fortune!
wait 2 s
say Will you be the first to find the pot of gold at the end of rainbow?
wait 2 s
say Just give me 1000 zenni and I'll spin the wheel for you! But remember, no refunds if you give me zenni while the wheel is spinning or you need to claim a prize. A man's got to make a living you know!
wait 5 s
say So, do you feel lucky?
end
~
$~
