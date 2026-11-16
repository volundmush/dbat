#400
if chains~
0 g 100
~
set x 2
if %x% == 1
  %echo% one
elseif %x% == 2
  %echo% two
  if %x% > 1
    %echo% nested true
  else
    %echo% nested false
  end
elseif %x% == 2
  %echo% never
else
  %echo% the else
end
if 0
  %echo% no
end
if %x%
else
  %echo% never either
end
if %nosuch%
  %echo% empty is false
elseif 0
  %echo% zero is false
elseif   abc
  %echo% text is true
end
if (%x% == 2) && (%x% != 3)
  %echo% both
end
~
#401
while loops~
0 g 100
~
set i 0
while %i% < 5
  eval i %i% + 1
  if %i% == 3
    %echo% three, breaking
    break
  end
  %echo% loop %i%
done
set j 0
while %j% < 2
  eval j %j% + 1
  set k 0
  while %k% < 2
    eval k %k% + 1
    %echo% inner %j% %k%
  done
done
while 0
  %echo% never
done
%echo% after loops %i% %j% %k%
~
#402
switches~
0 g 100
~
set v b
switch %v%
  case a
    %echo% a
  case b
    %echo% b falls
  case c
    %echo% c
    break
  default
    %echo% default
done
switch zzz
  case a
    %echo% a
  default
    %echo% default reached
done
switch %v%
  default
    %echo% default first
  case b
    %echo% b after default
done
switch %v%
  case b
    %echo% b falls into default
  default
    %echo% default after b
done
set n 0
while %n% < 3
  eval n %n% + 1
  switch %n%
    case 1
      %echo% switch in while 1
      break
    case 2
      %echo% switch in while 2
      if %n% == 2
        %echo% if in case
      end
      break
    default
      %echo% switch in while default
  done
done
switch nothing
  case other
    %echo% no match
done
~
#403
waits in blocks~
0 g 100
~
set i 0
while %i% < 2
  eval i %i% + 1
  if %i% == 1
    wait 1
    %echo% after wait in if in while %i%
  else
    switch %i%
      case 2
        wait 2 s
        %echo% after wait in switch %i%
        break
    done
  end
done
wait 1 t
%echo% after wait 1 t
wait until 12:30
%echo% after wait until 12:30
wait until 1430
%echo% after wait until 1430
wait until 5
%echo% after wait until 5
wait
wait 5 x
wait -3
wait 99999999999999999999 t
%echo% done waiting
~
#404
loop limits~
0 g 100
~
set i 0
while %i% < 100
  eval i %i% + 1
done
%echo% counted %i%
set j 0
while 1
  eval j %j% + 1
done
%echo% after endless %j%
~
#405
halt and friends~
0 g 100
~
* a comment
   * an indented comment

nop anything at all
nop
version
%echo% before halt
halt
%echo% never
~
#406
command returns 0~
0 c 100
poke~
%echo% poked by %actor.name% with '%arg%' as %cmd%
return 0
%echo% after return
~
#407
set eval unset~
0 g 100
~
set
set a
set b hello world
%echo% b=%b% a=%a%
unset b
%echo% b after unset=%b%
unset
unset nosuch
eval
eval c
eval c 1 + 2 * 3
%echo% c=%c%
eval d (1 + 2) * 3
%echo% d=%d%
eval e "quoted (paren" == "quoted (paren"
%echo% e=%e%
eval f 10 / 0
eval g abc /= b
eval h !0
eval i !1
eval j 5 - 7
eval k 3 >= 3
eval l 2 <= 1
eval m abc == ABC
eval n abc != abd
eval o 1 && 0
eval p 0 || 1
eval q 9223372036854775807 + 1
eval r -5 * -5
eval s 7 / 2
eval t ((((((((1))))))))
eval u 3 < 10
eval w 10 > 3
eval x abc < abd
eval y  spaced   +   2
eval z (1 + (2 * (3 - 1)))
%echo% %f% %g% %h% %i% %j% %k% %l% %m% %n% %o% %p% %q% %r% %s% %t% %u% %w% %x% %y% %z%
eval big 999999999999 * 999999999999
eval neg -9223372036854775807 - 10
%echo% %big% %neg%
~
#408
globals remotes context~
0 g 100
~
set gv globalvalue
global gv
%echo% gv=%self.gv% %gv%
context 5
set cv contextual
global cv
context 0
%echo% cv in 0=%cv%
context 5
%echo% cv in 5=%cv%
context
context x
set rv remoteval
remote rv %actor.id%
%echo% actor rv=%actor.rv%
%echo% exists=%actor.varexists(rv)% nope=%actor.varexists(nope)%
rdelete rv %actor.id%
%echo% after rdelete=%actor.varexists(rv)%
remote nosuch %actor.id%
remote rv 12345
remote rv
rdelete rv
rdelete rv 12345
remote
global
global nosuch
set keep kept
remote keep %actor.id%
set roomvar inroom
remote roomvar %self.room.id%
%echo% room var=%self.room.roomvar%
~
#409
extract letter makeuid~
0 g 100
~
set words alpha beta gamma
extract w 2 %words%
%echo% w=%w%
extract w 9 %words%
extract w 0 %words%
extract w
extract
dg_letter l 3 hello
%echo% l=%l%
dg_letter l 99 hello
dg_letter l 0 hello
dg_letter
makeuid u %actor.id%
%echo% u=%u.name%
makeuid r %self.room.vnum%
%echo% r=%r%
makeuid
makeuid u2
makeuid bad notanumber
~
#410
substitution~
0 g 100
~
%echo% 100%% sure %%%%
%echo% chain %actor.room.vnum% %self.room.people.name%
%echo% sub %actor.gold(%actor.level%)%
%echo% tmpvr after chain=%tmpvr%
%echo% nofield %actor.nosuchfield%
%echo% unknown var [%nosuch%] and [%nosuch.name%]
%echo% text %actor.name.strlen% %actor.name.car% %actor.name.cdr% %actor.name.charat(2)% %actor.name.contains(ct)% %actor.name.trim% %actor.name.mudcommand%
set sp   padded words
%echo% [%sp.trim%] [%sp.car%] [%sp.cdr%] [%sp.strlen%] [%sp.charat(99)%] [%sp.contains(dd)%]
set cmdword nort
%echo% mud [%cmdword.mudcommand%]
%echo% %ctime% %time.hour% %time.day% %time.month% %time.year% %time.other%
%echo% %people.300% %people.0% %people.x% %findmob.300(300)% %findobj.300(1)% %findmob.99999(1)% %findmob% %findobj.300%
%echo% %random.10% %random.0% %random.char% %random.dir% %random.x%
%echo% %send% %echo% %door% %force% %load% %purge% %teleport% %damage% %zoneecho% %asound% %at% %transform% %recho% %echoaround%
%echo% self=%self% global=%global% %global.gv%
%echo% deep %self.room.people.room.people.room.vnum%
%echo% paren %actor.name(%self.name%)% %actor.vnum(300)% %self.vnum(300)% %self.vnum(301)%
%echo% unclosed %self.name
%echo% long xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx %self.name% %random.100% end
~
#411
recursion~
0 c 100
poke~
%echo% poke depth
attach 411 %self.id%
%force% %actor% poke
%echo% back from poking
~
#412
purges itself~
0 g 100
~
%echo% before purge
%purge% %self%
%echo% after purge, never
~
#413
attaches and detaches~
0 g 100
~
attach 405 %self.id%
%echo% attached
detach 405 %self.id%
attach 405 %self.id%
detach all %self.id%
attach 99999 %self.id%
attach 405 12345
attach 405
attach
detach 405
detach
detach 99999 %self.id%
attach 405 %actor.id%
attach 405 %self.room.id%
detach 405 %self.room.id%
%echo% after detaching
~
#414
detaches itself~
0 g 100
~
%echo% before detach
detach all %self.id%
%echo% after detaching myself
~
#415
dg_cast dg_affect~
0 g 100
~
dg_cast 'heal' %actor%
dg_cast 'nosuchspell' %actor%
dg_cast
dg_cast 'heal'
dg_affect %actor% str 2 10
dg_affect %actor% nosuch 2 10
dg_affect %actor% str x 10
dg_affect %actor% str 2
dg_affect
dg_affect %actor% blind on 5
dg_affect %actor% blind off 5
dg_affect nobody str 2 10
~
#416
stray end~
0 g 100
~
end
%echo% after stray end
~
#417
else outside~
0 g 100
~
else
%echo% after stray else
end
~
#418
done outside~
0 g 100
~
done
%echo% after stray done
~
#419
if never closed~
0 g 100
~
if 1
  %echo% in an unclosed if
~
#420
while never closed~
0 g 100
~
set i 0
while %i% < 2
  eval i %i% + 1
  %echo% unclosed while %i%
~
#421
switch never closed~
0 g 100
~
switch 1
  case 1
    %echo% unclosed switch
~
#422
text after else~
0 g 100
~
if 0
  %echo% no
else %echo% glued
  %echo% in else
end
~
#423
text after end~
0 g 100
~
if 1
  %echo% yes
end @R
%echo% after
~
#424
case outside switch~
0 g 100
~
case 1
%echo% after stray case
~
#425
break outside~
0 g 100
~
break
%echo% after stray break
~
#426
elseif without test~
0 g 100
~
if 0
  %echo% no
elseif
  %echo% bare elseif
end
~
#427
if glued~
0 g 100
~
if(1 == 1)
  %echo% glued if
end
~
#428
elseif glued~
0 g 100
~
if 0
  %echo% no
elseif(1 == 1)
  %echo% glued elseif
else
  %echo% else
end
~
#429
interleaved~
0 g 100
~
set t 0
while %t% < 2
  eval t %t% + 1
  if %t% == 1
    %echo% first
  else
    %echo% second
  done
end
%echo% after
~
#430
default fallen into~
0 g 100
~
switch a
  case a
    %echo% a
  default
    %echo% default
done
~
#431
end prefix~
0 g 100
~
if 1
  %echo% yes
endif
%echo% after endif
~
#432
commands with nothing after them~
0 g 100
~
set %empty%
eval %empty%
wait %empty%
unset %empty%
global %empty%
context %empty%
remote %empty%
rdelete %empty%
extract %empty%
dg_letter %empty%
makeuid %empty%
attach %empty%
detach %empty%
return %empty%
dg_cast %empty%
dg_affect %empty%
wait until %empty%
set onlyname %empty%
remote onlyname %empty%
rdelete onlyname %empty%
remote onlyname }notanid
rdelete onlyname }notanid
attach 405 }notanid
detach 405 }notanid
attach notatrigger %self.id%
attach 405 %actor.id%
extract %empty% 1 a b
extract vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv 1 a b
makeuid %empty% 5
%echo% after the bare commands
~
#433
remotes on rooms and objects~
0 g 100
~
set rv x
remote rv %self.room.id%
%echo% room has [%self.room.rv%]
rdelete rv %self.room.id%
rdelete rv %self.room.id%
remote rv %self.id%
%echo% self has [%self.rv%]
rdelete rv %self.id%
rdelete nosuch %self.id%
rdelete rv %actor.id%
%load% obj 300
makeuid it obj streetlight
remote rv %it.id%
rdelete rv %it.id%
~
#434
makeuid by kind~
0 g 100
~
makeuid m mob dummy
%echo% m=[%m.name%]
makeuid o obj streetlight
%echo% o=[%o%]
makeuid r room
%echo% r=[%r.vnum%]
makeuid x mob nosuchmob
makeuid y nonsense name
%echo% x=[%x%] y=[%y%]
~
#435
detaching from rooms and objects~
0 g 100
~
attach 405 %self.room.id%
detach 405 %self.room.id%
attach 405 %self.room.id%
attach 406 %self.room.id%
detach all %self.room.id%
%load% obj 300
makeuid it obj streetlight
attach 405 %it.id%
detach 405 %it.id%
attach 405 %it.id%
detach all %it.id%
detach all 99999999
~
#436
eval corners~
0 g 100
~
eval a - 5
eval b "esc\"aped" == "esc\"aped"
eval c "unclosed
eval d (1 + 2
eval e 5 /= 5
eval f -9223372036854775807 / -1
eval g 0 - -9223372036854775807
%echo% [%a%] [%b%] [%c%] [%d%] [%e%] [%f%] [%g%]
eval deep ((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((1))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))
%echo% deep=[%deep%]
~
#437
if on the last line~
0 g 100
~
%echo% before
if 1
~
#438
false if on the last line~
0 g 100
~
%echo% before
if 0
~
#439
if with no end anywhere~
0 g 100
~
if 0
  %echo% no
  %echo% still no
~
#440
nested if with no end~
0 g 100
~
if 1
  if 0
    %echo% no
~
#441
switch on the last line~
0 g 100
~
%echo% before
switch 1
~
#442
switch with an open while~
0 g 100
~
switch 1
  while 0
  %echo% in it
~
#443
while on the last line~
0 g 100
~
%echo% before
while 1
~
#444
break with no done~
0 g 100
~
switch 1
  case 1
    break
  %echo% after
~
#445
obj recursion~
1 c 2
poke~
%echo% obj poke depth
attach 445 %self.id%
%force% %actor% poke
~
#446
tokens in messages~
2 g 100
~
%send% %actor% ~%actor.name% |%actor.name% ^%actor.name% &%actor.name% *%actor.name% and ~nobody |nobody ^nobody &nobody *nobody \~escaped
%echoaround% %actor% ~%actor.name% sees |%actor.name% thing
~
#447
tokens from an object~
1 h 100
~
%echo% ~%actor.name% |%actor.name% @%self.name%
%send% %actor% ~%actor.name% |%actor.name% ^%actor.name% &%actor.name% *%actor.name%
~
#448
memory with a command~
0 go 100
~
mremember %actor% say I remember you
%echo% remembered %actor.name%
~
#449
memory with a script~
0 go 100
~
mremember %actor%
%echo% memory script for %actor.name%
~
#450
globals on objects~
1 h 100
~
set gv objvalue
global gv
%echo% [%self.gv%] [%self.nosuch%]
remote gv %self.room.id%
%echo% [%self.room.gv%]
~
#451
globals on rooms~
2 g 100
~
set gv roomvalue
global gv
%echo% [%self.gv%] [%self.nosuch%]
%echo% %random.char% %random.dir%
~
#452
random from an object~
1 h 100
~
%echo% %random.char% %random.dir% %random.5%
~
#453
exits~
2 g 100
~
%echo% north: [%self.north%] [%self.north(vnum)%] [%self.north(key)%] [%self.north(bits)%] [%self.north(room)%] [%self.north(nosuch)%]
%echo% east: [%self.east%] [%self.east(vnum)%] [%self.east(key)%] [%self.east(bits)%] [%self.east(room)%] [%self.east(nosuch)%]
%echo% south: [%self.south%] [%self.south(vnum)%] [%self.south(key)%] [%self.south(bits)%] [%self.south(room)%] [%self.south(nosuch)%]
%echo% west: [%self.west%] [%self.west(vnum)%] [%self.west(key)%] [%self.west(bits)%] [%self.west(room)%] [%self.west(nosuch)%]
%echo% up: [%self.up%] [%self.up(vnum)%] [%self.up(key)%] [%self.up(bits)%] [%self.up(room)%] [%self.up(nosuch)%]
%echo% down: [%self.down%] [%self.down(vnum)%] [%self.down(key)%] [%self.down(bits)%] [%self.down(room)%] [%self.down(nosuch)%]
%echo% northwest: [%self.northwest%] [%self.northwest(vnum)%] [%self.northwest(key)%] [%self.northwest(bits)%] [%self.northwest(room)%] [%self.northwest(nosuch)%]
%echo% northeast: [%self.northeast%] [%self.northeast(vnum)%] [%self.northeast(key)%] [%self.northeast(bits)%] [%self.northeast(room)%] [%self.northeast(nosuch)%]
%echo% southwest: [%self.southwest%] [%self.southwest(vnum)%] [%self.southwest(key)%] [%self.southwest(bits)%] [%self.southwest(room)%] [%self.southwest(nosuch)%]
%echo% southeast: [%self.southeast%] [%self.southeast(vnum)%] [%self.southeast(key)%] [%self.southeast(bits)%] [%self.southeast(room)%] [%self.southeast(nosuch)%]
%echo% inside: [%self.inside%] [%self.inside(vnum)%] [%self.inside(key)%] [%self.inside(bits)%] [%self.inside(room)%] [%self.inside(nosuch)%]
~
#454
long lines~
0 g 100
~
mecho yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
%echo% zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz %self.name% %self.name% %self.name%
~
#455
saved variables~
0 g 100
~
%echo% [%actor.savedvar%] [%actor.incontext%] [%actor.bare%] [%actor.varexists(savedvar)%]
~
#456
fields with state~
0 g 100
~
%load% obj 300 %actor%
%force% %actor% wear all
mfollow %actor%
%echo% eq [%actor.eq(*)%] [%actor.eq(wield)%] [%actor.eq(body)%] follower [%actor.follower%] master [%self.master%]
%echo% killer [%actor.is_killer(on)%] [%actor.is_killer%] [%actor.is_killer(off)%] thief [%actor.is_thief(on)%] [%actor.is_thief%]
%echo% plr [%actor.plr(killer)%] [%actor.pref(brief)%] [%actor.affect(blind)%] inv [%actor.inventory(300)%] [%actor.inventory%]
%echo% has [%actor.has_item(300)%] [%actor.has_item(%actor.inventory%)%]
%echo% mana [%actor.mana(-5)%] [%actor.move(-5)%] [%actor.hitp(-5)%]
mkill %actor%
%echo% fighting [%self.fighting%] [%actor.fighting%]
set thing %actor.inventory%
%echo% thing [%thing.name%] [%thing.worn_by%] [%thing.carried_by%] [%thing.room%] [%thing.contents%] [%thing.affects(blind)%] [%thing.extra(glow)%] [%thing.extra%]
%echo% thing [%thing.setaffects(blind)%] [%thing.affects(blind)%] [%thing.setextra(glow)%] [%thing.extra(glow)%] [%thing.name(renamed)%] [%thing.shortdesc(nick)%]
%echo% thing [%thing.is_inroom%] [%thing.weight(-999)%] [%thing.health(-5)%]
~
#457
object fields with state~
1 h 100
~
%load% obj 300
%echo% [%self.contents%] [%self.worn_by%] [%self.room%] [%self.carried_by%] [%self.next_in_list%] [%self.count(300)%] [%self.has_in(300)%]
%force% %actor% put all %self.name%
%echo% [%self.contents%] [%self.has_in(300)%] [%self.count(300)%]
~
#458
makeuid and remote from 1~
1 h 100
~
makeuid m mob dummy
makeuid o obj streetlight
makeuid r room
%echo% [%m%] [%o%] [%r%]
set rv x
remote rv %self.id%
rdelete rv %self.id%
remote rv %actor.id%
rdelete rv %actor.id%
attach 405 %self.id%
detach 405 %self.id%
attach 405 %self.id%
detach all %self.id%
%echo% [%findmob.300%] [%findobj.99999(1)%] [%findobj.300(300)%] [%findobj.300(%self.id%)%] [%findobj.300(streetlight)%]
~
#459
makeuid and remote from 2~
2 g 100
~
makeuid m mob dummy
makeuid o obj streetlight
makeuid r room
%echo% [%m%] [%o%] [%r%]
set rv x
remote rv %self.id%
rdelete rv %self.id%
remote rv %actor.id%
rdelete rv %actor.id%
attach 405 %self.id%
detach 405 %self.id%
attach 405 %self.id%
detach all %self.id%
%echo% [%findmob.300%] [%findobj.99999(1)%] [%findobj.300(300)%] [%findobj.300(%self.id%)%] [%findobj.300(streetlight)%]
~
#460
global self from an object~
1 h 100
~
%load% mob 300
makeuid m mob dummy
attach 405 %m.id%
%echo% self through a mob's globals: [%m.global(self)%] mine: [%self%]
~
#461
a wait whose time is a variable it can't read~
0 g 100
~
set t abc
wait %t%
%echo% not after an unreadable wait
~
#462
a written wait it can't read~
0 g 100
~
%echo% never runs
wait until x
~
#463
an empty wait from a variable~
0 g 100
~
wait %nothing%
%echo% not after an empty wait
~
#464
control flow corners~
0 g 100
~
if 0
  %echo% not here
else
  %echo% first else
else
  %echo% second else never runs
elseif 1
  %echo% nor this
end
    
set i 0
while 1
  eval i %i% + 1
  if %i% > 3
    break
  end
done
%echo% broke out at %i%
switch b
  default
    %echo% default first
  case b
    %echo% case b after default
    break
done
switch c
  case a
    %echo% not a
  case c
    %echo% c falls
  default
    %echo% through default
  case d
    %echo% and on into d
done
switch x
  case x
    switch y
      case y
        %echo% inner y
        break
    done
    %echo% after the inner switch
done
set n 0
while %n% < 30
  eval n %n% + 1
done
set m 0
while %m% < 30
  eval m %m% + 1
done
%echo% two loops of 30: %n% %m%
switch %n%
  case 30
    set k 0
    while %k% < 3
      eval k %k% + 1
      wait 1
      if %k% == 2
        break
      end
    done
    %echo% waited in a while in a case: %k%
done
~
#465
argument corners~
0 g 100
~
set rv 1
unset %empty%
return %empty%
global %empty%
context %empty%
attach %empty%
attach 405 abc
attach 405 %empty%
attach 99999 %self.id%
attach 405 %actor.id%
detach %empty%
detach 405 abc
detach 99999 %self.room.id%
detach 99999 %self.id%
makeuid %empty%
makeuid u %empty%
makeuid m mob %actor.name%
makeuid o obj nosuch
makeuid r room x
makeuid x mob
makeuid x thing y
%echo% made %m% %r%
remote rv -5
rdelete rv -5
extract aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa 1 a
dg_cast 'heal' nobody
dg_cast 'heal' %actor%
dg_affect %actor% str 2 0
dg_affect %actor% str 2 -1
%send% %actor% |%actor.name% ^%actor.name% &%actor.name% *%actor.name% and \|escaped
~
$~
