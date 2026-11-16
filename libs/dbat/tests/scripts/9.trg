#900
dispatch 0 d 0~
0 d 0
hello there~
%echo% heard the phrase: %speech%
~
#901
dispatch 0 d 1~
0 d 1
test words~
%echo% heard a word: %speech%
~
#902
dispatch 0 d 1~
0 d 1
*~
%echo% heard anything: %speech%
~
#903
dispatch 0 e 0~
0 e 0
test~
%echo% act seen: %arg%
~
#904
dispatch 0 e 1~
0 e 1
test~
%echo% act word: %arg% %actor.name% %victim.name% %object.name%
~
#905
dispatch 0 m 50~
0 m 50
~
%echo% bribed %amount%
~
#906
dispatch 0 m 1~
0 m 1
~
%echo% bribed %amount%
~
#907
dispatch 0 j 100~
0 j 100
~
%echo% received %object.name%
return 0
~
#908
dispatch 0 f 100~
0 f 100
~
%echo% died to %actor.name%
~
#909
dispatch 0 k 100~
0 k 100
~
%echo% fighting %actor.name%
~
#910
dispatch 0 l 100~
0 l 100
~
%echo% hit percent
~
#911
dispatch 0 n 100~
0 n 100
~
%echo% loaded
~
#912
dispatch 0 p 100~
0 p 100
~
%echo% cast at by %actor.name% spell %spellname%
~
#913
dispatch 0 q 100~
0 q 100
~
%echo% %actor.name% leaves %direction%
return 0
~
#914
dispatch 0 r 100~
0 r 100
~
%echo% door %cmd% %direction%
~
#915
dispatch 0 i 100~
0 i 100
~
%echo% entered %self.room.vnum%
~
#916
dispatch 0 o 100~
0 o 100
~
%echo% remembered
~
#917
dispatch 0 b 100~
0 b 100
~
%echo% random tick
~
#918
dispatch 0 bt 0~
0 bt 0
~
%echo% the time
~
#919
dispatch 0 h 100~
0 h 100
~
%echo% greet all %direction%
~
#920
dispatch 0 g 0~
0 g 0
~
%echo% greet with no chance
~
#921
dispatch 0 c 100~
0 c 100
te~
%echo% abbreviated command %cmd%
~
#922
dispatch 0 c 100~
0 c 100
other~
%echo% not this command
~
#923
dispatch 1 b 100~
1 b 100
~
%echo% obj random
~
#924
dispatch 1 c 1~
1 c 1
test~
%echo% obj command equip
~
#925
dispatch 1 c 2~
1 c 2
test~
%echo% obj command inventory
return 0
~
#926
dispatch 1 c 4~
1 c 4
test~
%echo% obj command room
~
#927
dispatch 1 f 100~
1 f 100
~
%echo% obj timer
~
#928
dispatch 1 g 100~
1 g 100
~
%echo% obj got by %actor.name%
return 0
~
#929
dispatch 1 h 100~
1 h 100
~
%echo% obj dropped
return 0
~
#930
dispatch 1 i 100~
1 i 100
~
%echo% obj given to %victim.name%
~
#931
dispatch 1 j 100~
1 j 100
~
%echo% obj worn
return 0
~
#932
dispatch 1 l 100~
1 l 100
~
%echo% obj removed
~
#933
dispatch 1 n 100~
1 n 100
~
%echo% obj loaded
~
#934
dispatch 1 p 100~
1 p 100
~
%echo% obj cast %spellname%
~
#935
dispatch 1 q 100~
1 q 100
~
%echo% obj sees leave %direction%
~
#936
dispatch 1 s 100~
1 s 100
~
%echo% obj consumed %command%
return 0
~
#937
dispatch 1 t 0~
1 t 0
~
%echo% obj time
~
#938
dispatch 2 b 100~
2 b 100
~
%echo% room random
~
#939
dispatch 2 c 100~
2 c 100
test~
%echo% room command %cmd% [%arg%]
~
#940
dispatch 2 d 0~
2 d 0
test~
%echo% room speech %speech%
~
#941
dispatch 2 f 100~
2 f 100
~
%echo% room reset
~
#942
dispatch 2 g 100~
2 g 100
~
%echo% room enter %direction%
return 0
~
#943
dispatch 2 h 100~
2 h 100
~
%echo% room drop %object.name%
return 0
~
#944
dispatch 2 p 100~
2 p 100
~
%echo% room cast %spellname%
~
#945
dispatch 2 q 100~
2 q 100
~
%echo% room leave %direction%
~
#946
dispatch 2 r 100~
2 r 100
~
%echo% room door %cmd%
~
#947
dispatch 2 t 0~
2 t 0
~
%echo% room time
~
#950
speech phrases~
0 d 1
"hello there" aaab "unclosed~
%echo% 950 heard [%speech%] from [%actor.name%]
~
#951
speech whole text~
0 d 0
aaab~
%echo% 951 heard [%speech%]
~
#952
speech list ended by an empty phrase~
0 d 1
"" never~
%echo% 952 heard [%speech%]
~
#953
act words~
0 e 1
smiles says~
%echo% 953 saw [%arg%] actor [%actor.name%] victim [%victim.name%]
~
#954
speech and act corners~
0 g 100
~
attach 950 %self.id%
%force% %actor% say hello there friend
%force% %actor% say nothing to see
%force% %actor% say unclose
%force% %actor% say unclosed
%force% %actor% say aaab!
detach 950 %self.id%
attach 951 %self.id%
%force% %actor% say aaaab
%force% %actor% say xaaab.
%force% %actor% say AAAB
%force% %actor% say aaabc
detach 951 %self.id%
attach 952 %self.id%
%force% %actor% say never
detach 952 %self.id%
attach 953 %self.id%
%force% %actor% emote smiles broadly
%force% %actor% emote   smiles, says nothing
%force% %actor% say anything
~
$~
