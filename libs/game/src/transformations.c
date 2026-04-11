#include "dbat/game/transformations.h"
#include "dbat/db/consts/races.h"
#include "dbat/db/consts/playerflags.h"
#include "dbat/game/utils.h"

struct transform_bonus base_form = {
    .bonus=0, 
    .mult=1.0,
    .drain=0.0,
    .flag=0,
    .rpp_cost=0,
    .requires_pl=0,
    .key="base",
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
    .key="first",
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
    .key="second",
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
    .key="third",
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
    .key="fourth",
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

/* =============================================================================
 * Human Transformations
 * ============================================================================= */

static struct transform_bonus human_super_human_1 = {
    .bonus=1000000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=1800000,
    .key="first",
    .name="@YSuper @CHuman @WFirst@n",
    .msg_transform_self="@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!@n",
    .msg_transform_others="@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!@n"
};

static struct transform_bonus human_super_human_2 = {
    .bonus=12000000,
    .mult=3.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=35000000,
    .key="second",
    .name="@YSuper @CHuman @WSecond@n",
    .msg_transform_self="@WSuddenly a bright white aura bursts into existance around your body, you feel the intensity of your hidden potential boil until it can't be contained any longer! Waves of ki shoot out from your aura streaking outwards in many directions. A roar that shakes everything in the surrounding area sounds right as your energy reaches its potential and you achieve @CSuper @cHuman @GSecond@W!@n",
    .msg_transform_others="@C$n@W is suddenly covered with a bright white aura as $e grits $s teeth, apparently struggling with the power boiling to the surface! Waves of ki shoot out from $s aura, streaking in several directions as a mighty roar shakes everything in the surrounding area. As $s aura calms $e smiles, having achieved @CSuper @cHuman @GSecond@W!@n"
};

static struct transform_bonus human_super_human_3 = {
    .bonus=50000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=190000000,
    .key="third",
    .name="@YSuper @CHuman @WThird@n",
    .msg_transform_self="@WYou clench both of your fists as the bright white aura around your body is absorbed back into your flesh. As it is absorbed, your muscles triple in size and electricity crackles across your flesh. You grin as you feel the power of @CSuper @cHuman @GThird@W!@n",
    .msg_transform_others="@C$n@W clenches both of $s fists as the bright white aura around $s body is absorbed back into $s flesh. As it is absorbed, $s muscles triple in size and bright electricity crackles across $s flesh. $e smiles as $e achieves the power of @CSuper @cHuman @GThird@W!@n"
};

static struct transform_bonus human_super_human_4 = {
    .bonus=270000000,
    .mult=4.5,
    .drain=0.2,
    .flag=PLR_TRANS4,
    .rpp_cost=0,
    .requires_pl=1200000000,
    .key="fourth",
    .name="@YSuper @CHuman @WFourth@n",
    .msg_transform_self="@WYou grit your teeth and clench your fists as a sudden surge of power begins to tear through your body! Your muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over you, flashes of multicolored light flaring up in rising stars around your new form, a corona of glory! You feel your ultimate potential realized as you ascend to @CSuper @cHuman @GFourth@W!@n",
    .msg_transform_others="@C$n@W grits $s teeth and clenches $s fists as a sudden surge of power begins to tear through $s body! $n@W's muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over $m, flashes of multicolored light flare up in rising stars around $s new form, a corona of glory! $n@W smiles as his ultimate potential is realized as $e ascends to @CSuper @cHuman @GFourth@W!@n"
};

static struct transform_bonus human_transformations[] = {
    human_super_human_1,
    human_super_human_2,
    human_super_human_3,
    human_super_human_4
};

static struct transforms_available human_transforms_available = {
    .number=4,
    .bonuses=human_transformations
};

/* =============================================================================
 * Icer Transformations
 * ============================================================================= */

static struct transform_bonus icer_transform_1 = {
    .bonus=400000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=500000,
    .key="first",
    .name="@YTransform @WFirst@n",
    .msg_transform_self="@WYou yell with pain as your body begins to grow and power surges within! Your legs expand outward to triple their previous length. Soon after your arms, chest, and head follow. Your horns grow longer and curve upwards while lastly your tail expands. You are left confidently standing, having completed your @GFirst @cTransformation@W.@n",
    .msg_transform_others="@C$n@W yells with pain as $s body begins to grow and power surges outward! $s legs expand outward to triple their previous length. Soon after $s arms, chest, and head follow. $s horns grow longer and curve upwards while lastly $s tail expands. $e is left confidently standing, having completed $s @GFirst @cTransformation@W.@n"
};

static struct transform_bonus icer_transform_2 = {
    .bonus=7000000,
    .mult=3.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=17500000,
    .key="second",
    .name="@YTransform @WSecond@n",
    .msg_transform_self="@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n",
    .msg_transform_others="@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n"
};

static struct transform_bonus icer_transform_3 = {
    .bonus=45000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=150000000,
    .key="third",
    .name="@YTransform @WThird@n",
    .msg_transform_self="@WA blinding light surrounds your body while your rising power begins to rip up the ground beneath you! Your skin and torso shell begin to crack as your new body struggles to free its self. Huge chunks of debris lift free of the ground as your power begins to rise to unbelievable heights. Suddenly your old skin and torso shell burst off from your body, leaving a sleek form glowing where they had been. Everything comes crashing down as your power evens out, leaving you with your @GThird @cTransformation @Wcompleted!@n",
    .msg_transform_others="@WA blinding light surrounds @C$n@W's body while $s rising power begins to rip up the ground beneath $m! $s skin and torso shell begin to crack as $s new body struggles to free its self. Huge chunks of debris lift free of the ground as $s power begins to rise to unbelievable heights. Suddenly $s old skin and torso shell burst off from $s body, leaving a sleek form glowing where they had been. Everything comes crashing down as @C$n@W's power evens out, leaving $m with $s @GThird @cTransformation @Wcompleted!@n"
};

static struct transform_bonus icer_transform_4 = {
    .bonus=200000000,
    .mult=5.0,
    .drain=0.2,
    .flag=PLR_TRANS4,
    .rpp_cost=0,
    .requires_pl=850000000,
    .key="fourth",
    .name="@YTransform @WFourth@n",
    .msg_transform_self="@WA feeling of complete power courses through your viens as your body begins to change radically! You triple in height while a hard shell forms over your entire torso. Hard bones grow out from your head forming four ridges that jut outward. A hard covering grows up over your mouth and nose completing the transformation! A dark crimson aura flames around your body as you realize your @GFourth @cTransformation@W!@n",
    .msg_transform_others="@C$n@W's body begins to change radically! $e triples in height while a hard shell forms over $s entire torso. Hard bones grow out from $s head forming four ridges that jut outward. A hard covering grows up over $s mouth and nose completing the transformation! A dark crimson aura flames around @C$n@W's body as $e realizes $s @GFourth @cTransformation@W!@n"
};

static struct transform_bonus icer_transformations[] = {
    icer_transform_1,
    icer_transform_2,
    icer_transform_3,
    icer_transform_4
};

static struct transforms_available icer_transforms_available = {
    .number=4,
    .bonuses=icer_transformations
};

/* =============================================================================
 * Namekian Transformations
 * ============================================================================= */

static struct transform_bonus namekian_super_namek_1 = {
    .bonus=200000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=360000,
    .key="first",
    .name="@YSuper @CNamek @WFirst@n",
    .msg_transform_self="@WYou crouch down and clench your fists as your muscles begin to bulge! Sweat pours down your body as the ground beneath your feet cracks and warps under the pressure of your rising ki! With a sudden burst that sends debris flying you realize a new plateau in your power, having achieved @CSuper @gNamek @GFirst@W!@n",
    .msg_transform_others="@C$n @Wcrouches down and clenches $s fists as $s muscles begin to bulge! Sweat pours down $s body as the ground beneath $s feet cracks and warps under the pressure of  $s rising ki! With a sudden burst that sends debris flying $e seems to realize a new plateau in $s power, having achieved @CSuper @gNamek @GFirst@W!@n"
};

static struct transform_bonus namekian_super_namek_2 = {
    .bonus=4000000,
    .mult=3.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=9500000,
    .key="second",
    .name="@YSuper @CNamek @WSecond@n",
    .msg_transform_self="@WYou gasp in shock as a power within your body that you had not been aware of begins to surge to the surface! Your muscles grow larger as energy crackles between your antennae intensely! A shockwave of energy explodes outward as you achieve a new plateau in power, @CSuper @gNamek @GSecond@W!@n",
    .msg_transform_others="@C$n @Wgasps in shock as a power within $s body begins to surge out! $s muscles grow larger as energy crackles between $s antennae intensely! A shockwave of energy explodes outward as $e achieves a new plateau in power, @CSuper @gNamek @GSecond@W!@n"
};

static struct transform_bonus namekian_super_namek_3 = {
    .bonus=65000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=220000000,
    .key="third",
    .name="@YSuper @CNamek @WThird@n",
    .msg_transform_self="@WA fierce clear aura bursts up around your body as you struggle to control a growing power within! Energy leaks off of your aura at an astounding rate filling the air around you with small orbs of ki. As your power begins to level off the ambient ki hovering around you is absorbed inward in a sudden shock that leaves your skin glowing! You have achieved a rare power, @CSuper @gNamek @GThird@W!@n",
    .msg_transform_others="@WA fierce clear aura bursts up around @C$n@W's body as $e struggles to control $s own power! Energy leaks off of $s aura at an astounding rate filling the air around $m with small orbs of ki. As $s power begins to level off the ambient ki hovering around $m is absorbed inward in a sudden shock that leaves $s skin glowing! $e has achieved a rare power, @CSuper @gNamek @GThird@W!@n"
};

static struct transform_bonus namekian_super_namek_4 = {
    .bonus=230000000,
    .mult=4.5,
    .drain=0.2,
    .flag=PLR_TRANS4,
    .rpp_cost=0,
    .requires_pl=900000000,
    .key="fourth",
    .name="@YSuper @CNamek @WFourth@n",
    .msg_transform_self="@WAn inner calm fills your mind as your power surges higher than ever before. Complete clarity puts everything once questioned into perspective. While this inner calm is filling your mind, an outer storm of energy erupts around your body! The storm of energy boils and crackles while growing larger. You have achieved @CSuper @gNamek @GFourth@W, a mystery of the ages.@n",
    .msg_transform_others="@C$n@W smiles calmly as a look of complete understand fills $s eyes. While $e remains perfectly calm and detached a massivly powerful storm of energy erupts from his body. This storm of energy shimmers with the colors of the rainbow and boils and crackles with awesome power! $s smile disappears as he realizes a mysterious power of the ages, @CSuper @gNamek @GFourth@W!@n"
};

static struct transform_bonus namekian_transformations[] = {
    namekian_super_namek_1,
    namekian_super_namek_2,
    namekian_super_namek_3,
    namekian_super_namek_4
};

static struct transforms_available namekian_transforms_available = {
    .number=4,
    .bonuses=namekian_transformations
};

/* =============================================================================
 * Konatsu Transformations
 * ============================================================================= */

static struct transform_bonus konatsu_shadow_1 = {
    .bonus=1000000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=1800000,
    .key="first",
    .name="@YShadow @WFirst@n",
    .msg_transform_self="@WA dark shadowy aura with flecks of white energy begins to burn around your body! Strength and agility can be felt rising up within as your form becomes blurred and ethereal looking. You smile as you realize your @GFirst @DShadow @BForm@W!@n",
    .msg_transform_others="@WA dark shadowy aura with flecks of white energy begins to burn around @C$n@W's body! $s form becomes blurred and ethereal-looking as $s muscles become strong and lithe. $e smiles as $e achieves $s @GFirst @DShadow @BForm@W!@n"
};

static struct transform_bonus konatsu_shadow_2 = {
    .bonus=56000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=225000000,
    .key="second",
    .name="@YShadow @WSecond@n",
    .msg_transform_self="@WThe shadowy aura surrounding your body burns larger than ever as dark bolts of purple electricity crackles across your skin. Your eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into your aura in an instant as you achieve your @GSecond @DShadow @BForm@W!@n",
    .msg_transform_others="@WThe shadowy aura surrounding @C$n@W's body burns larger than ever as dark bolts of purple electricity crackles across $s skin. $s eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into $s aura in an instant as $e achieves $s @GSecond @DShadow @BForm@W!@n"
};

static struct transform_bonus konatsu_shadow_3 = {
    .bonus=290000000,
    .mult=5.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=1400000000,
    .key="third",
    .name="@YShadow @WThird@n",
    .msg_transform_self="@WThe shadowy aura around you explodes outward as your power begins to rise!  You're overcome with a sudden realization, that the shadows are an extension of yourself, that light isn't needed for your shadows to bloom.  With this newfound wisdom comes ability and power!  The color in your aura drains as the shadows slide inward and cling to your body like a second, solid black skin!  Shockwaves roll off of you in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath you, slowly spreading the shadows to the whole area, projecting onto any surface nearby!  Purple and black electricity crackle in your solid white aura, and you grin as you realize your @GThird @DShadow @BForm@W!@n",
    .msg_transform_others="@WThe shadowy aura around $n explodes outward as $s power begins to rise!  Realization dawns on $s face, followed shortly by confidence! The color in $s aura drains as the shadows slide inward to cling to $s body like a second, solid black skin! Shockwaves roll off of $n in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath them, slowly spreading the shadows to the whole area, projecting onto any surface nearby! Purple and black electricity crackle in $s solid white aura, and he grins as $e realizes $s @GThird @DShadow @BForm@W!@n"
};

static struct transform_bonus konatsu_transformations[] = {
    konatsu_shadow_1,
    konatsu_shadow_2,
    konatsu_shadow_3
};

static struct transforms_available konatsu_transforms_available = {
    .number=3,
    .bonuses=konatsu_transformations
};

/* =============================================================================
 * Mutant Transformations
 * ============================================================================= */

static struct transform_bonus mutant_mutation_1 = {
    .bonus=100000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=180000,
    .key="first",
    .name="@YMutate @WFirst@n",
    .msg_transform_self="@WYour flesh grows tougher as power surges up from within. Your fingernails grow longer, sharper, and more claw-like. Lastly your muscles double in size as you achieve your @GFirst @mMutation@W!@n",
    .msg_transform_others="@C$n@W flesh grows tougher as power surges up around $m. $s fingernails grow longer, sharper, and more claw-like. Lastly $s muscles double in size as $e achieves $s @GFirst @mMutation@W!@n"
};

static struct transform_bonus mutant_mutation_2 = {
    .bonus=8500000,
    .mult=3.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=27500000,
    .key="second",
    .name="@YMutate @WSecond@n",
    .msg_transform_self="@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n",
    .msg_transform_others="@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n"
};

static struct transform_bonus mutant_mutation_3 = {
    .bonus=80000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=700000000,
    .key="third",
    .name="@YMutate @WThird@n",
    .msg_transform_self="@WA dark cyan aura bursts up around your body as the ground begins to crack beneath you! You scream out in pain as your power begins to explode! Two large spikes grow out from your shoulder blades as you reach your @GThird @mMutation!@n",
    .msg_transform_others="@WA dark cyan aura bursts up around @C$n@W's body as the ground begins to crack beneath $m and $e screams out in pain as $s power begins to explode! Two large spikes grow out from $s shoulder blades as $e reaches $s @GThird @mMutation!@n"
};

static struct transform_bonus mutant_transformations[] = {
    mutant_mutation_1,
    mutant_mutation_2,
    mutant_mutation_3
};

static struct transforms_available mutant_transforms_available = {
    .number=3,
    .bonuses=mutant_transformations
};

/* =============================================================================
 * Halfbreed Transformations (different from Saiyan)
 * ============================================================================= */

static struct transform_bonus halfbreed_super_saiyan_1 = {
    .bonus=900000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=1400000,
    .key="first",
    .name="@YSuper @CSaiyan @WFirst@n",
    .msg_transform_self="@WSomething inside your mind snaps as your rage spills over! Lightning begins to strike the ground all around you as you feel torrents of power rushing through every fiber of your being. Your hair suddenly turns golden as your eyes change to the color of emeralds. In a final rush of power a golden aura rushes up around your body! You have become a @CSuper @YSaiyan@W!@n",
    .msg_transform_others="@C$n@W screams in rage as lightning begins to crash all around! $s hair turns golden and $s eyes change to an emerald color as a bright golden aura bursts up around $s body! As $s energy stabilizes $e wears a fierce look upon $s face, having transformed into a @CSuper @YSaiyan@W!@n"
};

static struct transform_bonus halfbreed_super_saiyan_2 = {
    .bonus=16500000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=55000000,
    .key="second",
    .name="@YSuper @CSaiyan @WSecond@n",
    .msg_transform_self="@WBlinding rage burns through your mind as a sudden eruption of energy surges forth! A golden aura bursts up around your body, glowing as bright as the sun. Rushing winds rocket out from your body in every direction as bolts of electricity begin to crackle in your aura. As your aura dims you are left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n",
    .msg_transform_others="@C$n@W stands up straight with $s head back as $e releases an ear piercing scream! A blindingly bright golden aura bursts up around $s body, glowing as bright as the sun. As rushing winds begin to rocket out from $m in every direction, bolts of electricity flash and crackle in $s aura. As $s aura begins to dim $e is left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n"
};

static struct transform_bonus halfbreed_super_saiyan_3 = {
    .bonus=240000000,
    .mult=5.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=1050000000,
    .key="third",
    .name="@YSuper @CSaiyan @WThird@n",
    .msg_transform_self="@WElectricity begins to crackle around your body as your aura grows explosively! You yell as your powerlevel begins to skyrocket while your hair grows to multiple times the length it was previously. Your muscles become incredibly dense instead of growing in size, preserving your speed. Finally your irises appear just as your transformation becomes complete, having achieved @CSuper @YSaiyan @GThird@W!@n",
    .msg_transform_others="@WElectricity begins to crackle around @C$n@W, as $s aura grows explosively! $e yells as the energy around $m skyrockets and $s hair grows to multiple times its previous length. $e smiles as $s irises appear and $s muscles tighten up. $s transformation complete, $e now stands confidently, having achieved @CSuper @YSaiyan @GThird@W!@n"
};

static struct transform_bonus halfbreed_transformations[] = {
    halfbreed_super_saiyan_1,
    halfbreed_super_saiyan_2,
    halfbreed_super_saiyan_3
};

static struct transforms_available halfbreed_transforms_available = {
    .number=3,
    .bonuses=halfbreed_transformations
};

/* =============================================================================
 * Bio-Android Transformations
 * ============================================================================= */

static struct transform_bonus bio_mature = {
    .bonus=1000000,
    .mult=2.0,
    .drain=0.0,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=1800000,
    .key="mature",
    .name="@YMature@n",
    .msg_transform_self="@gYou bend over as @rpain@g wracks your body! Your limbs begin to grow out, becoming more defined and muscular. As your limbs finish growing outward you feel a painful sensation coming from your back as a long tail with a spike grows out of your back! As the pain subsides you stand up straight and a current of power shatters part of the ground beneath you. You have @rmatured@g beyond your @Gl@ga@Dr@gv@Ga@ge stage!@n",
    .msg_transform_others="@W$n @gbends over as a @rpainful@g look covers $s face! $s limbs begin to grow out, becoming more defined and muscular. As $s limbs finish growing outward $e screams as a long tail with a spike grows rips out of $s back! As $e calms $e stands up straight and a current of power shatters part of the ground beneath $m. $e has @rmatured@g beyond $s @Gl@ga@Dr@gv@Ga@ge stage!@n"
};

static struct transform_bonus bio_semi_perfect = {
    .bonus=8000000,
    .mult=3.0,
    .drain=0.0,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=25000000,
    .key="semi_perfect",
    .name="@YSemi@D-@GPerfect@n",
    .msg_transform_self="@WYour exoskeleton begins to glow spectacularly while the shape of your body begins to change. Your tail shrinks slightly. Your hands, feet, and facial features become more refined. While your body colors change slightly. The crests on your head change, standing up straighter on either side of your head as well. As you finish transforming a wave of power floods your being. You have achieved your @gSemi@D-@GPerfect @BForm@W!@n",
    .msg_transform_others="@C$n@W's exoskeleton begins to glow spectacularly while the shape of $s body begins to change. $s tail shrinks slightly. $s hands, feet, and facial features become more refined. While $s body colors change slightly. The crests on $s head change, standing up straighter on either side of $s head as well. As $e finishes transforming a wave of power rushes out from $m. $e has achieved $s @gSemi@D-@GPerfect @BForm@W!@n"
};

static struct transform_bonus bio_perfect = {
    .bonus=70000000,
    .mult=3.5,
    .drain=0.0,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=220000000,
    .key="perfect",
    .name="@YPerfect@n",
    .msg_transform_self="@WYour whole body is engulfed in blinding light as your exoskeleton begins to change shape! Your hands, feet, and facial features become more refined and humanoid. While your colors change, becoming more subdued and neutral. A bright golden aura bursts up around your body as you achieve your @GPerfect @BForm@W!@n",
    .msg_transform_others="@C$n@W whole body is engulfed in blinding light as $s exoskeleton begins to change shape! $s hands, feet, and facial features become more refined and humanoid. While $s colors change, becoming more subdued and neutral. A bright golden aura bursts up around $s body as $e achieves $s @GPerfect @BForm@W!@n"
};

static struct transform_bonus bio_super_perfect = {
    .bonus=400000000,
    .mult=4.0,
    .drain=0.0,
    .flag=PLR_TRANS4,
    .rpp_cost=0,
    .requires_pl=300000000,
    .key="super-perfect",
    .name="@YSuper @GPerfect@n",
    .msg_transform_self="@WA rush of power explodes from your perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout your aura intensely while your muscles grow slightly larger but incredibly dense. You smile as you realize that you have taken your perfect form beyond imagination. You are now @CSuper @GPerfect@W!@n",
    .msg_transform_others="@WA rush of power explodes from @C$n@W's perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout $s aura intensely while $s muscles grow slightly larger but incredibly dense. $e smiles as $e has taken $s perfect form beyond imagination. $e is now @CSuper @GPerfect@W!@n"
};

static struct transform_bonus bio_transformations[] = {
    bio_mature,
    bio_semi_perfect,
    bio_perfect,
    bio_super_perfect
};

static struct transforms_available bio_transforms_available = {
    .number=4,
    .bonuses=bio_transformations
};

/* =============================================================================
 * Android Transformations
 * ============================================================================= */

static struct transform_bonus android_upgrade_1 = {
    .bonus=5000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=1000000,
    .key="1.0",
    .name="@Y1.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_upgrade_2 = {
    .bonus=20000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=8000000,
    .key="2.0",
    .name="@Y2.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_upgrade_3 = {
    .bonus=125000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=50000000,
    .key="3.0",
    .name="@Y3.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_upgrade_4 = {
    .bonus=1000000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS4,
    .rpp_cost=0,
    .requires_pl=300000000,
    .key="4.0",
    .name="@Y4.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_upgrade_5 = {
    .bonus=2500000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS5,
    .rpp_cost=0,
    .requires_pl=800000000,
    .key="5.0",
    .name="@Y5.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_upgrade_6 = {
    .bonus=5000000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS6,
    .rpp_cost=0,
    .requires_pl=1200000000,
    .key="6.0",
    .name="@Y6.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_transformations[] = {
    android_upgrade_1,
    android_upgrade_2,
    android_upgrade_3,
    android_upgrade_4,
    android_upgrade_5,
    android_upgrade_6
};

static struct transforms_available android_transforms_available = {
    .number=6,
    .bonuses=android_transformations
};

/* =========================================================*/
// Android Sense Transformations
/* =========================================================*/

static struct transform_bonus android_sense_upgrade_1 = {
    .bonus=12500000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=1000000,
    .key="1.0",
    .name="@Y1.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_sense_upgrade_2 = {
    .bonus=50000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=8000000,
    .key="2.0",
    .name="@Y2.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_sense_upgrade_3 = {
    .bonus=312000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=50000000,
    .key="3.0",
    .name="@Y3.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_sense_upgrade_4 = {
    .bonus=2500000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS4,
    .rpp_cost=0,
    .requires_pl=300000000,
    .key="4.0",
    .name="@Y4.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_sense_upgrade_5 = {
    .bonus=5000000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS5,
    .rpp_cost=0,
    .requires_pl=800000000,
    .key="5.0",
    .name="@Y5.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_sense_upgrade_6 = {
    .bonus=10000000000,
    .mult=1.0,
    .drain=0.0,
    .flag=PLR_TRANS6,
    .rpp_cost=0,
    .requires_pl=1200000000,
    .key="6.0",
    .name="@Y6.0@n",
    .msg_transform_self="@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n",
    .msg_transform_others="@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n"
};

static struct transform_bonus android_sense_transformations[] = {
    android_sense_upgrade_1,
    android_sense_upgrade_2,
    android_sense_upgrade_3,
    android_sense_upgrade_4,
    android_sense_upgrade_5,
    android_sense_upgrade_6
};

static struct transforms_available android_sense_transforms_available = {
    .number=6,
    .bonuses=android_sense_transformations
};

/* =============================================================================
 * Majin Transformations
 * ============================================================================= */

static struct transform_bonus majin_morph_1 = {
    .bonus=1250000,
    .mult=2.0,
    .drain=0.0,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=2200000,
    .key="affinity",
    .name="@YMorph @WAffinity@n",
    .msg_transform_self="@WA dark pink aura bursts up around your body as images of good and evil fill your mind! You feel the power within your body growing intensely, reflecting your personal alignment as your body changes!@n",
    .msg_transform_others="@WA dark pink aura bursts up around @C$n@W's body as images of good and evil fill $s mind! $e feels the power within $s body growing intensely, reflecting $s personal alignment as $s body changes!@n"
};

static struct transform_bonus majin_morph_2 = {
    .bonus=15000000,
    .mult=3.0,
    .drain=0.0,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=45000000,
    .key="super",
    .name="@YMorph @WSuper@n",
    .msg_transform_self="@WAn intense pink aura surrounds your body as it begins to change, taking on the characteristics of those you have ingested! Explosions of pink energy burst into existence all around you as your power soars to sights unseen!@n",
    .msg_transform_others="@WAn intense pink aura surrounds @C$n@W's body as it begins to change, taking on the characteristics of those $e has ingested! Explosions of pink energy burst into existence all around $m as $s power soars to sights unseen!@n"
};

static struct transform_bonus majin_morph_3 = {
    .bonus=340000000,
    .mult=4.5,
    .drain=0.0,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=1550000000,
    .key="true",
    .name="@YMorph @WTrue@n",
    .msg_transform_self="@WRipples of intense pink energy rush upwards around your body as it begins to morph into its truest form! The ground beneath your feet forms into a crater from the very pressure of your rising ki! Earthquakes shudder throughout the area as your finish morphing!@n",
    .msg_transform_others="@WRipples of intense pink energy rush upwards around @C$n@W's body as it begins to morph into its truest form! The ground beneath $s feet forms into a crater from the very pressure of $s rising ki! Earthquakes shudder throughout the area as $e finishes morphing!@n"
};

static struct transform_bonus majin_transformations[] = {
    majin_morph_1,
    majin_morph_2,
    majin_morph_3
};

static struct transforms_available majin_transforms_available = {
    .number=3,
    .bonuses=majin_transformations
};

/* =============================================================================
 * Kai Transformations
 * ============================================================================= */

static struct transform_bonus kai_mystic_1 = {
    .bonus=1100000,
    .mult=3.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=3000000,
    .key="first",
    .name="@YMystic @WFirst@n",
    .msg_transform_self="@WThoughts begin to flow through your mind of events throughout your life. The progression leads up to more recent events and finally to this very moment. All of it's significance overwhelms you momentarily and your motivation and drive increase. As your attention is drawn back to your surroundings, you feel as though your thinking, senses, and reflexes have sharpened dramatically.  At the core of your being, a greater depth of power can be felt.@n",
    .msg_transform_others="@W$n@W's face tenses, it becoming clear momentarily that they are deep in thought. After a brief lapse in focus, their attention seems to return to their surroundings. Though it's not apparent why they were so distracted, something definitely seems different about $m.@n"
};

static struct transform_bonus kai_mystic_2 = {
    .bonus=115000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=650000000,
    .key="second",
    .name="@YMystic @WSecond@n",
    .msg_transform_self="@WYou feel a sudden rush of emotion, escalating almost to a loss of control as your thoughts race. Your heart begins to beat fast as memories mix with the raw emotion. A faint blue glow begins to surround you. As your emotions level off, you feel a deeper understanding of the universe as you know it. You visibly calm back down to an almost steely eyed resolve as you assess your surroundings. The blue aura wicks around you for a few moments and then dissipates. Thought it's full impact is not yet clear to you, you are left feeling as though both your power and inner strength have turned into nearly bottomless wells.@n",
    .msg_transform_others="@W$n@W's appears to be hit by some sudden pangs of agony, their face contorted in pain.  After a moment a faint blue aura appears around them, glowing brighter as time passes. You can feel something in the pit of your stomach, letting you know that something very significant is changing around you. Before long $n@W's aura fades, leaving a very determined looking person in your presence.@n"
};

static struct transform_bonus kai_mystic_3 = {
    .bonus=270000000,
    .mult=5.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=1300000000,
    .key="third",
    .name="@YMystic @WThird@n",
    .msg_transform_self="@WYour minds' eye becomes overwhelmed by secrets unimaginable. The threads of the very universe become visible in your heightened state of awareness. Reaching out, a single thread vibrates, producing a @Rred @Wcolor -- yours. Your fingertips brush against it and your senses become clouded by a vast expanse of white color and noise. As your vision and hearing return, you understand the threads tying every living being together. Your awareness has expanded beyond comprehension!@n",
    .msg_transform_others="@C$n@W's eyes grow wide, mouth agape. $s body begins to shiver uncontrollably! $s arms reaches out cautiously before falling back down to $s side. $s face relaxes visibly, features returning to a normal state. $s irises remain larger than before, a slight smile softening $s gaze.@n"
};

static struct transform_bonus kai_transformations[] = {
    kai_mystic_1,
    kai_mystic_2,
    kai_mystic_3
};

static struct transforms_available kai_transforms_available = {
    .number=3,
    .bonuses=kai_transformations
};

/* =============================================================================
 * Truffle Transformations
 * ============================================================================= */

static struct transform_bonus truffle_ascend_1 = {
    .bonus=1300000,
    .mult=3.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=3600000,
    .key="first",
    .name="@YAscend @WFirst@n",
    .msg_transform_self="@WYour mind accelerates working through the mysteries of the universe while at the same time your body begins to change! Innate nano-technology within your body begins to activate, forming flexible metal plating across parts of your skin!@n",
    .msg_transform_others="@C$n@W begins to write complicated calculations in the air as though $e were possessed while at the same time $s body begins to change! Innate nano-technology within $s body begins to activate, forming flexible metal plating across parts of $s skin!@n"
};

static struct transform_bonus truffle_ascend_2 = {
    .bonus=80000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=300000000,
    .key="second",
    .name="@YAscend @WSecond@n",
    .msg_transform_self="@WComplete understanding of every physical thing floods your mind as the nano-technology within you continues to change your body! Your eyes change; becoming glassy, hard, and glowing. Your muscles merge with a nano-fiber strengthening them at the molecular level! Finally your very bones become plated in nano-metals that have yet to be invented naturally!@n",
    .msg_transform_others="@C$n@.s nano-technology continues to change $s body! $s eyes change; becoming glassy, hard, and glowing. $s muscles merge with a nano-fiber strengthening them at the molecular level! Finally $s very bones become plated in nano-metals that have yet to be invented naturally!@n"
};

static struct transform_bonus truffle_ascend_3 = {
    .bonus=300000000,
    .mult=5.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=1450000000,
    .key="third",
    .name="@YAscend @WThird@n",
    .msg_transform_self="@WYou have reached the final stage of enlightenment and the nano-technology thriving inside you begin to initiate the changes! Your neural pathways become refined, your reflexes honed, your auditory and ocular senses sharpening far beyond normal levels! Your gravitational awareness improves, increasing sensitivity and accuracy in your equilibrum!@n",
    .msg_transform_others="@C$n begins to mumble quietly, slowly at first and gradually picking up speed. A glint is seen from $s eyes and $s arms reach outwards briefly as $e appears to catch his balance. $s arms drop back to $s sides as balance is regained, a vicious smile on $s face.@n"
};

static struct transform_bonus truffle_transformations[] = {
    truffle_ascend_1,
    truffle_ascend_2,
    truffle_ascend_3
};

static struct transforms_available truffle_transforms_available = {
    .number=3,
    .bonuses=truffle_transformations
};

/* =============================================================================
 * Legendary Saiyan Transformations (PLR_LSSJ flag)
 * ============================================================================= */

static struct transform_bonus legendary_super_saiyan_1 = {
    .bonus=800000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=500000,
    .key="first",
    .name="@YSuper @CSaiyan @WFirst@n",
    .msg_transform_self="@WSomething inside your mind snaps as your rage spills over! Lightning begins to strike the ground all around you as you feel torrents of power rushing through every fiber of your being. Your hair suddenly turns golden as your eyes change to the color of emeralds. In a final rush of power a golden aura rushes up around your body! You have become a @CSuper @YSaiyan@W!@n",
    .msg_transform_others="@C$n@W screams in rage as lightning begins to crash all around! $s hair turns golden and $s eyes change to an emerald color as a bright golden aura bursts up around $s body! As $s energy stabilizes $e wears a fierce look upon $s face, having transformed into a @CSuper @YSaiyan@W!@n"
};

static struct transform_bonus legendary_super_saiyan_2 = {
    .bonus=185000000,
    .mult=6.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=250000000,
    .key="second",
    .name="@YLegendary @CSuper Saiyan@n",
    .msg_transform_self="@WYou roar and then stand at your full height. You flex every muscle in your body as you feel your strength grow! Your eyes begin to glow @wwhite@W with energy, your hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around your body! You release your @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n",
    .msg_transform_others="@C$n @Wroars and then stands at $s full height. Then $s muscles start to buldge and grow as $e flexes them! Suddenly $s eyes begin to glow @wwhite@W with energy, $s hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around $s body! @C$n@W releases $s @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n"
};

static struct transform_bonus legendary_transformations[] = {
    legendary_super_saiyan_1,
    legendary_super_saiyan_2
};

static struct transforms_available legendary_transforms_available = {
    .number=2,
    .bonuses=legendary_transformations
};

/* =============================================================================
 * Oozaru Transformation
 * ============================================================================= */

struct transform_bonus oozaru = {
    .bonus=10000,
    .mult=2.0,
    .drain=0.0,
    .flag=PLR_OOZARU,
    .rpp_cost=0,
    .requires_pl=0,
    .key="oozaru",
    .name="@ROozaru@n",
    .msg_transform_self="@WYou howl as the power of the full moon washes over you! Your body grows massive amounts of hair, your face pushes out into a muzzle, and your tail grows back if it was missing. You are now the @RGreat @ROozaru@W!@n",
    .msg_transform_others="@C$n@W howls as a wave of power washes over $m! $s body grows massive amounts of hair, $s face pushes out into a muzzle, and $s tail grows back if it was missing. $e is now the @RGreat @ROozaru@W!@n"
};

// Hoshijin Crap
struct transform_bonus hoshijin_birth = {
    .bonus=0,
    .mult=2.0,
    .drain=0.0,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=0,
    .key="birth",
    .name="@YBirth@n",
    .msg_transform_self="@WYou are born into the world as a Hoshijin!@n",
    .msg_transform_others="@C$n@W is born into the world as a Hoshijin!@n"
};

struct transform_bonus hoshijin_life = {
    .bonus=0,
    .mult=3.0,
    .drain=0.0,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=0,
    .key="life",
    .name="@YLife@n",
    .msg_transform_self="@WYou are born into the world as a Hoshijin!@n",
    .msg_transform_others="@C$n@W is born into the world as a Hoshijin!@n"
};

struct transforms_available* get_transforms_available(struct char_data* ch) {
    switch(ch->race) {
        case RACE_SAIYAN:
        if(PLR_FLAGGED(ch, PLR_LSSJ)) {
            return &legendary_transforms_available;
        }
        else {
            return &saiyan_transforms_available;
        }
        case RACE_KONATSU:
            return &konatsu_transforms_available;
        case RACE_HUMAN:
            return &human_transforms_available;
        case RACE_BIO:
            return &bio_transforms_available;
        case RACE_MUTANT:
            return &mutant_transforms_available;
        case RACE_KAI:
            return &kai_transforms_available;
        case RACE_ANDROID:
        if(PLR_FLAGGED(ch, PLR_SENSEM)) {
            return &android_sense_transforms_available;
        }
        else {
            return &android_transforms_available;
        }
        case RACE_MAJIN:
            return &majin_transforms_available;
        case RACE_HALFBREED:
            return &halfbreed_transforms_available;
        case RACE_NAMEK:
            return &namekian_transforms_available;
        default:
            return NULL;
    }
}

int race_can_transform(int race_id) {
    switch (race_id) {
        case RACE_HUMAN:
        case RACE_KAI:
        case RACE_TRUFFLE:
        case RACE_KONATSU:
        case RACE_MUTANT:
        case RACE_ICER:
        case RACE_HALFBREED:
        case RACE_NAMEK:
        case RACE_SAIYAN:
        case RACE_BIO:
        case RACE_MAJIN:
        case RACE_ANDROID:
            return true;
        default:
            return false;
    }
}

int race_can_revert(int race_id) {
    if(!race_can_transform(race_id)) return 0;
    switch(race_id) {
        case RACE_MAJIN:
        case RACE_ANDROID:
        case RACE_BIO:
        case RACE_TRUFFLE:
            return 0;
        default:    
            return 1;
    }
}

int trans_flag_to_tier(int flag) {
    switch (flag) {
        case PLR_TRANS1:
            return 1;
        case PLR_TRANS2:
            return 2;
        case PLR_TRANS3:
            return 3;
        case PLR_TRANS4:
            return 4;
        case PLR_TRANS5:
            return 5;
        case PLR_TRANS6:
            return 6;
        default:
            return 0;
    }
}

int race_has_noisy_transformations(int race_id) {
    return race_id != RACE_ANDROID;
}

void display_transforms(struct char_data* ch) {
    if(!race_can_transform(ch->race)) {
        send_to_char(ch, "Your race has no transformations.\r\n");
        return;
    }
    struct transforms_available* available = get_transforms_available(ch);
    if(!available) {
        send_to_char(ch, "Error retrieving transformations.\r\n");
        return;
    }

    send_to_char(ch, "Available transformations:\r\n");
    send_to_char(ch, "@b------------------------------------------------@n\r\n");
    int tclass = GET_TRANSCLASS(ch);

    double modifier = 1.0;
    // tclass 1 lowers requirements by about 30%, tclass 2 has no affect, and tclass 3 increases by 30%
    switch(tclass) {
        case 1:
            modifier = 1.15;
            break;
        case 2:
            modifier = 1.0;
            break;
        case 3:
            modifier = 0.85;
            break;
    };
    int64_t pl = getBasePL(ch);

    for(int i = 0; i < available->number; i++) {
        struct transform_bonus* bonus = &available->bonuses[i];
        int64_t req_pl = (int64_t)(bonus->requires_pl * modifier);
        const char* req_pl_str = (pl >= req_pl) ? add_commas(req_pl) : "??????????";
        send_to_char(ch, "%s  @R-@G %s BPL Req\r\n", bonus->name, req_pl_str);

    }
    send_to_char(ch, "@b------------------------------------------------@n\r\n");
}

static int trans_flags[] = {PLR_TRANS1, PLR_TRANS2, PLR_TRANS3, PLR_TRANS4, PLR_TRANS5, PLR_TRANS6};

int get_current_trans_tier(struct char_data *ch) {
    int trans_tier = 0;
    for(int i = 0; i < 6; i++) {
        trans_tier++;
        if(PLR_FLAGGED(ch, trans_flags[i])) {
            return trans_tier;
        }
    }
    return 0;
};

struct transform_bonus get_current_transform(char_data *ch) {
    if(PLR_FLAGGED(ch, PLR_OOZARU)) return oozaru;
    if(IS_HOSHIJIN(ch)) {
        transform_bonus hoshi_form;
        double bon_mult = 0;
        switch(GET_PHASE(ch)) {
            case 0: // death phase
                return base_form;
            case 1: // birth phase
                hoshi_form = hoshijin_birth;
                bon_mult = 4;
                break;
            case 2: // life phase
                hoshi_form = hoshijin_life;
                bon_mult = 8;
                break;
            default:
                return base_form;
        }
        if(ETHER_STREAM(ch))
            bon_mult += .5;
        hoshi_form.bonus = (getBasePL(ch) * .1) * bon_mult;
        return hoshi_form;
    }

    struct transforms_available* available = get_transforms_available(ch);
    if(!available) return base_form;
    int tier = get_current_trans_tier(ch);
    if(!tier) return base_form;
    struct transform_bonus trans = available->bonuses[tier-1];
    return trans;
}

struct transform_bonus select_transformation(struct char_data* ch, const char* key) {
    struct transforms_available* available = get_transforms_available(ch);
    if(!available) {
        return base_form;
    }
    for(int i = 0; i < available->number; i++) {
        struct transform_bonus* bonus = &available->bonuses[i];
        if(!strcasecmp(bonus->key, key)) {
            return *bonus;
        }
    }
    return base_form;
}


int check_can_transform(struct char_data *ch) {
    // No point checking for Saiyan/Halfbreed because it's just as expensive to check for
    // the Oozaru flag.
    if (PLR_FLAGGED(ch, PLR_OOZARU)) {
        send_to_char(ch, "You are the great Oozaru right now and can't transform!\r\n");
        return false;
    }

    if (GET_KAIOKEN(ch) > 0) {
        send_to_char(ch, "You are in kaioken right now and can't transform!\r\n");
        return false;
    }

    return true;
}


int check_trans_unlock(struct char_data *ch, int tier) {
    // First, check for special requirements which are not 'paid'.
    switch (ch->race) {
        case RACE_BIO:
            if(tier > 3-GET_ABSORBS(ch)) {
                send_to_char(ch, "You need to absorb something to transform!\r\n");
                return false;
            }
            break;
        case RACE_MAJIN:
            switch (tier) {
                case 2:
                    if (GET_ABSORBS(ch) > 0) {
                        send_to_char(ch, "You need to ingest someone before you can use that form.\r\n");
                        return false;
                    }
                    if(GET_LEVEL(ch) < 50) {
                        send_to_char(ch, "You must be at least level 50 to reach that form.\r\n");
                        return false;
                    }
            }
            break;
    }
    int rpp_cost = 0;

    // Second, check for RPP requirements.
    struct transforms_available* available = get_transforms_available(ch);
    switch (ch->race) {
        case RACE_ANDROID:
            switch (tier) {
                case 1:
                    break; // free for androids. They pay PS instead.
                case 6:
                    rpp_cost = 5;
                    break;
                default:
                    rpp_cost = 1;
                    break;
            }
            break;
        case RACE_MAJIN:
            switch (tier) {
                case 1:
                    rpp_cost = 1;
                    break;
                case 2:
                case 3:
                    rpp_cost = 2;
                    break;
            }
            break;

        default:
            if (available->number == tier) {
                rpp_cost = 2;
            } else {
                rpp_cost = 1;
            }
    }

    if (rpp_cost) {
        if (GET_TRANSCOST(ch, tier) == FALSE) {
            if (GET_RP(ch) < rpp_cost) {
                send_to_char(ch, "You need %i RPP in order to unlock this transformation.\r\n", rpp_cost);
                return false;
            } else {
                GET_RP(ch) -= rpp_cost;
                GET_TRANSCOST(ch, tier) = TRUE;
                send_to_char(ch, "You pay %i RPP to permanently unlock this transformation!\r\n", rpp_cost);
            }
        }
    }

    // Android upgrades cost PS instead of RPP. But this system has now been standardized so anything can.
    int ps_cost = 0;
    switch (ch->race) {
        case RACE_ANDROID:
            switch (tier) {
                case 1:
                    ps_cost = 50;
                    break;
            }
    }

    if (ps_cost) {
        if (GET_TRANSCOST(ch, tier) == FALSE) {
            if (GET_PRACTICES(ch, GET_CLASS(ch)) < 50) {
                send_to_char(ch,
                                "You need %i practice points in order to obtain a transformation for the first time.\r\n",
                                ps_cost);
                return false;
            } else {
                GET_PRACTICES(ch, GET_CLASS(ch)) -= 50;
                GET_TRANSCOST(ch, tier) = TRUE;
                send_to_char(ch, "You pay %i PS to permanently unlock this transformation!\r\n", ps_cost);
            }
        }
    }

    // if we got down this far, we have unlocked the transformation!
    return true;

}