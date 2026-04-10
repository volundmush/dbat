#include "dbat/game/transformations.h"
#include "dbat/db/consts/races.h"
#include "dbat/db/consts/playerflags.h"

struct transform_bonus base_form = {
    .bonus=0, 
    .mult=1.0,
    .drain=0.0,
    .flag=0,
    .rpp_cost=0,
    .requires_pl=0,
    .name="base form",
    .msg_transform_self="",
    .msg_transform_others=""
};

static struct transform_bonus saiyan_super_saiyan_1 = {
    .bonus=800000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=1200000,
    .name="@YSuper @CSaiyan @WFirst@n",
    .msg_transform_self="@WSomething inside your mind snaps as your rage spills over! Lightning begins to strike the ground all around you as you feel torrents of power rushing through every fiber of your being. Your hair suddenly turns golden as your eyes change to the color of emeralds. In a final rush of power a golden aura rushes up around your body! You have become a @CSuper @YSaiyan@W!@n",
    .msg_transform_others="@C$n@W screams in rage as lightning begins to crash all around! $s hair turns golden and $s eyes change to an emerald color as a bright golden aura bursts up around $s body! As $s energy stabilizes $e wears a fierce look upon $s face, having transformed into a @CSuper @YSaiyan@W!@n",
    
};

static struct transform_bonus saiyan_super_saiyan_2 = {
    .bonus=20000000,
    .mult=3.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=55000000,
    .name="@YSuper @CSaiyan @WSecond@n",
    .msg_transform_self="@WBlinding rage burns through your mind as a sudden eruption of energy surges forth! A golden aura bursts up around your body, glowing as bright as the sun. Rushing winds rocket out from your body in every direction as bolts of electricity begin to crackle in your aura. As your aura dims you are left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n",
    .msg_transform_others="@C$n@W stands up straight with $s head back as $e releases an ear piercing scream! A blindingly bright golden aura bursts up around $s body, glowing as bright as the sun. As rushing winds begin to rocket out from $m in every direction, bolts of electricity flash and crackle in $s aura. As $s aura begins to dim $e is left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n"
    
};

static struct transform_bonus saiyan_super_saiyan_3 = {
    .bonus=80000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=200000000,
    .name="@YSuper @CSaiyan @WThird@n",
    .msg_transform_self="@WElectricity begins to crackle around your body as your aura grows explosively! You yell as your powerlevel begins to skyrocket while your hair grows to multiple times the length it was previously. Your muscles become incredibly dense instead of growing in size, preserving your speed. Finally your irises appear just as your transformation becomes complete, having achieved @CSuper @YSaiyan @GThird@W!@n",
    .msg_transform_others="@WElectricity begins to crackle around @C$n@W, as $s aura grows explosively! $e yells as the energy around $m skyrockets and $s hair grows to multiple times its previous length. $e smiles as $s irises appear and $s muscles tighten up. $s transformation complete, $e now stands confidently, having achieved @CSuper @YSaiyan @GThird@W!@n"
};

static struct transform_bonus saiyan_super_saiyan_4 = {
    .bonus=200000000,
    .mult=5.0,
    .drain=0.2,
    .flag=PLR_TRANS4,
    .rpp_cost=0,
    .requires_pl=1625000000,
    .name="@YSuper @CSaiyan @WFourth@n",
    .msg_transform_self="@WHaving absorbed enough blutz waves, your body begins to transform! Red fur grows over certain parts of your skin as your hair grows longer and unkempt. A red outline forms around your eyes while the irises of those very same eyes change to an amber color. Energy crackles about your body violently as you achieve the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n",
    .msg_transform_others="@WHaving absorbed enough blutz waves, @C$n@W's body begins to transform! Red fur grows over certain parts of $s skin as $s hair grows longer and unkempt. A red outline forms around $s eyes while the irises of those very same eyes change to an amber color. Energy crackles about $s body violently as $e achieves the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n"
};

static struct transform_bonus saiyan_transformations[] = {
    saiyan_super_saiyan_1,
    saiyan_super_saiyan_2,
    saiyan_super_saiyan_3,
    saiyan_super_saiyan_4
};

static struct transforms_available saiyan_transforms_available = {
    .number=4,
    .bonuses=saiyan_transformations
};