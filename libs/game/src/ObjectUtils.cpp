#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/const/WearSlot.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/game/class.hpp"

#include "dbat/game/const/AdminLevel.hpp"

constexpr int LOC_INVENTORY = 0;
void auto_equip(Character *ch, Object *obj, int location)
{
    int j;

    /* Lots of checks... */
    if (location > 0)
    { /* Was wearing it. */
        switch (j = (location - 1))
        {
        case WEAR_UNUSED0:
            j = WEAR_WIELD2;
            break;
        case WEAR_FINGER_R:
        case WEAR_FINGER_L:
            if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
                location = LOC_INVENTORY;
            break;
        case WEAR_NECK_1:
        case WEAR_NECK_2:
            if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
                location = LOC_INVENTORY;
            break;
        case WEAR_BODY:
            if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
                location = LOC_INVENTORY;
            break;
        case WEAR_HEAD:
            if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
                location = LOC_INVENTORY;
            break;
        case WEAR_LEGS:
            if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
                location = LOC_INVENTORY;
            break;
        case WEAR_FEET:
            if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
                location = LOC_INVENTORY;
            break;
        case WEAR_HANDS:
            if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
                location = LOC_INVENTORY;
            break;
        case WEAR_ARMS:
            if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
                location = LOC_INVENTORY;
            break;
        case WEAR_UNUSED1:
            if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
                location = LOC_INVENTORY;
            j = WEAR_WIELD2;
            break;
        case WEAR_ABOUT:
            if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
                location = LOC_INVENTORY;
            break;
        case WEAR_WAIST:
            if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
                location = LOC_INVENTORY;
            break;
        case WEAR_WRIST_R:
        case WEAR_WRIST_L:
            if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
                location = LOC_INVENTORY;
            break;
        case WEAR_WIELD1:
            if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
                location = LOC_INVENTORY;
            break;
        case WEAR_WIELD2:
            break;
        case WEAR_EYE:
            if (!CAN_WEAR(obj, ITEM_WEAR_EYE))
                location = LOC_INVENTORY;
            break;
        case WEAR_BACKPACK:
            if (!CAN_WEAR(obj, ITEM_WEAR_PACK))
                location = LOC_INVENTORY;
            break;
        case WEAR_SH:
            if (!CAN_WEAR(obj, ITEM_WEAR_SH))
                location = LOC_INVENTORY;
            break;
        case WEAR_EAR_R:
        case WEAR_EAR_L:
            if (!CAN_WEAR(obj, ITEM_WEAR_EAR))
                location = LOC_INVENTORY;
            break;
        default:
            location = LOC_INVENTORY;
        }

        if (location > 0)
        { /* Wearable. */
            if (!GET_EQ(ch, j))
            {
                /*
                 * Check the characters's alignment to prevent them from being
                 * zapped through the auto-equipping.
                 */
                if (invalid_align(ch, obj) || invalid_class(ch, obj))
                    location = LOC_INVENTORY;
                else
                    equip_char(ch, obj, j);
            }
            else
            { /* Oops, saved a player with double equipment? */
                mudlog(BRF, ADMLVL_IMMORT, true, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch),
                       location);
                location = LOC_INVENTORY;
            }
        }
    }

    if (location <= 0) /* Inventory */
        ch->addToInventory(obj);
}