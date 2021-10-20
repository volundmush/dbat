#12833
Namek Dojo F1 Instigate~
2 c 100
Instigate~
if %arg% == hard
 wait 1s
 eval Inz %random.1000%
 if (%Inz% >= 800)
  wait 1s
  %send% %actor% @nAs you call for opponants, a bunch of the new students approach at once!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, a bunch of the newer students gather around them at once, and attack!
  wait 3s
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
 elseif (%Inz% >= 500)
  wait 1s
  %send% %actor% @nAs you call for opponants, a handfull of the new students approach at once!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, a handfull of the newer students gather around them at once, and attack!
  wait 3s
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
 elseif (%Inz% >= 1)
  wait 1s
  %send% %actor% @nAs you call for opponants, several of the new students approach at once!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, several of the newer students gather around them at once, and attack!
  wait 3s
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
 end
elseif %arg% == Medium
 wait 1s
 eval Inz %random.1000%
 if (%Inz% >= 800)
  wait 1s
  %send% %actor% @nAs you call for opponants, a group of the new students approach at once!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, a group of the newer students gather around them at once, and attack!
  wait 3s
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
 elseif (%Inz% >= 500)
  wait 1s
  %send% %actor% @nAs you call for opponants, some of the new students approach at once!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, some of the newer students gather around them at once, and attack!
  wait 3s
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
 elseif (%Inz% >= 1)
  wait 1s
  %send% %actor% @nAs you call for opponants, a quartet of the new students approach at once!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, a quartet of the newer students gather around them at once, and attack!
  wait 3s
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
 end
elseif %arg% == easy
 wait 1s
 eval Inz %random.1000%
 if (%Inz% >= 800)
  wait 1s
  %send% %actor% @nAs you call for opponants, a trio of the new students approach at once!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, a trio of the newer students approach them at once, and attack!
  wait 3s
  %load% mob 12834
  %load% mob 12834
  %load% mob 12834
 elseif (%Inz% >= 500)
  wait 1s
  %send% %actor% @nAs you call for opponants, a pair of the new students approach at once!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, a pair of the newer students approach them at once, and attack!
  wait 3s
  %load% mob 12834
  %load% mob 12834
 elseif (%Inz% >= 1)
  wait 1s
  %send% %actor% @nAs you call for opponants, one measly student approaches!
  %echoaround% %actor% @nAs @R%actor.name%@n calls out for opponants, one measly student approaches them, and attacks!
  wait 3s
  %load% mob 12834
 end
end
~
#12834
@nDojo Mob Respawn~
0 f 100
~
%load% mob 12834
%purge% corpse
return 0
~
$~
