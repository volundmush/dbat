
import dbat
import typing
from pydantic import BaseModel, Field, PrivateAttr

class BodyPartHandler:
    """
    A BodyPartHandler is responsible for handling a specific body part. It defines the properties of that body part, such as its name, description, and what it can do. It also defines how that body part interacts with hediffs, such as injuries or diseases.
    """
    
    def key(self) -> str:
        """
        Return the key that identifies this body part handler. This is used for looking up the handler for a given body part.
        """
        raise NotImplementedError()

    def slot(self) -> str:
        """
        Return the slot that this body part occupies. This is used for determining what can be attached to this body part, such as armor or clothing.
        """
        raise NotImplementedError()
    
    def name(self) -> str:
        """
        Return the display name of this body part. This is used for displaying the body part to the user.
        """
        raise NotImplementedError()
    
    def provides_manipulation(self) -> set[str]:
        """
        Return a set of strings representing the "provides" of this body part. This is used for determining what this body part can do, such as what kind of attacks it can make, what kind of armor it can wear, etc.
        For example, a humanoid head might provide "can_see", "can_hear", "can_speak", while a humanoid arm might provide "can_grasp", "can_strike".
        """
        return set()
    
    def provides_mobility(self) -> set[str]:
        """
        Return a set of strings representing the "provides" of this body part related to mobility. This is used for determining how this body part contributes to the character's movement capabilities, such as walking, running, jumping, etc.
        For example, a humanoid leg might provide "can_walk", "can_run", "can_jump", while a humanoid arm would not provide any mobility-related provides.
        """
        return set()
    
    def provides_senses(self) -> set[str]:
        """
        Return a set of strings representing the "provides" of this body part related to senses. This is used for determining how this body part contributes to the character's sensory capabilities, such as seeing, hearing, smelling, etc.
        For example, a humanoid head might provide "can_see", "can_hear", "can_smell", while a humanoid arm would not provide any senses-related provides.
        """
        return set()
    
    def provides_attacks(self) -> set[str]:
        """
        Return a set of strings representing the "provides" of this body part related to attacks. This is used for determining what kind of attacks this body part can make, such as striking, kicking, biting, etc.
        For example, a humanoid arm might provide "can_strike", while a humanoid leg might provide "can_kick", and a humanoid head might provide "can_bite".
        """
        return set()

    def provides_communication(self) -> set[str]:
        """
        Return a set of strings representing the "provides" of this body part related to communication. This is used for determining how this body part contributes to the character's ability to communicate, such as speaking, writing, gesturing, etc.
        For example, a humanoid head might provide "can_speak", while a humanoid arm might provide "can_gesture".
        """
        return set()

class Soul(BodyPartHandler):
    """
    Not really sure this should be a body part to be honest.
    Still chewing on it.
    """
    def key(self) -> str:
        return "soul"
    
    def slot(self) -> str:
        return "soul"
    
    def name(self) -> str:
        return "Soul"
    
    def provides_senses(self) -> set[str]:
        return {"sense_spiritual"}

class Head(BodyPartHandler):
    def slot(self) -> str:
        return "head"
    
    def name(self) -> str:
        return "Head"
    
    def provides_attacks(self):
        return {"can_headbutt", "can_bite"}
    
    def provides_senses(self) -> set[str]:
        return {"can_see", "can_hear", "can_smell"}
    
    def provides_communication(self):
        return {"can_speak", "can_expression"}
    
    def provides_manipulation(self) -> set[str]:
        return {"can_eat"}


class HumanoidHead(Head):
    def key(self) -> str:
        return "humanoid_head"
    

class HumanoidTorso(BodyPartHandler):
    """
    The torso is the central body part, and is the default target of any effects like buffs or diseases.
    """

    def key(self) -> str:
        return "humanoid_torso"
    
    def slot(self) -> str:
        return "torso"
    
    def name(self) -> str:
        return "Torso"


class HumanoidArm(BodyPartHandler):
    def provides_manipulation(self) -> set[str]:
        return {"can_grasp"}
    
    def provides_mobility(self) -> set[str]:
        return {"can_crawl"}
    
    def provides_senses(self) -> set[str]:
        return {"can_touch"}
    
    def provides_attacks(self) -> set[str]:
        return {"can_punch"}
    
    def provides_communication(self):
        return {"can_gesture"}


class HumanoidLeftArm(HumanoidArm):
    def key(self) -> str:
        return "humanoid_left_arm"
    
    def slot(self) -> str:
        return "left_arm"
    
    def name(self) -> str:
        return "Left Arm"


class HumanoidRightArm(HumanoidArm):
    def key(self) -> str:
        return "humanoid_right_arm"
    
    def slot(self) -> str:
        return "right_arm"
    
    def name(self) -> str:
        return "Right Arm"


class HumanoidLeg(BodyPartHandler):
    def provides_manipulation(self) -> set[str]:
        return {"can_kick"}
    
    def provides_mobility(self) -> set[str]:
        return {"can_walk", "can_run", "can_jump"}

class HumanoidLeftLeg(HumanoidLeg):
    def key(self) -> str:
        return "humanoid_left_leg"
    
    def slot(self) -> str:
        return "left_leg"
    
    def name(self) -> str:
        return "Left Leg"
    
class HumanoidRightLeg(HumanoidLeg):
    def key(self) -> str:
        return "humanoid_right_leg"
    
    def slot(self) -> str:
        return "right_leg"
    
    def name(self) -> str:
        return "Right Leg"


class Tail(BodyPartHandler):
    def key(self) -> str:
        return "generic_tail"
    
    def slot(self) -> str:
        return "tail"
    
    def name(self) -> str:
        return "Tail"
    
    def provides_attacks(self) -> set[str]:
        return {"can_tail_whip", "can_tail_wrap"}
    
    def provides_manipulation(self) -> set[str]:
        return {"can_grasp"}


class SaiyanTail(Tail):
    def key(self) -> str:
        return "saiyan_tail"


class IcerTail(Tail):
    def key(self) -> str:
        return "icer_tail"



class BioAndroidTail(Tail):
    def key(self) -> str:
        return "bioandroid_tail"


ALL_BODY_PARTS = [HumanoidHead, HumanoidTorso, HumanoidLeftArm, HumanoidRightArm, HumanoidLeftLeg, HumanoidRightLeg, Tail, IcerTail, SaiyanTail, BioAndroidTail]


class BodyPlanHandler:
    """
    A BodyPlanHandler is responsible for handling a specific body plan. It defines the properties of that body plan, such as its name, description, and what body parts it has. It also defines how the body parts interact with each other, such as which body parts are connected to which other body parts.
    """
    
    def key(self) -> str:
        """
        Return the key that identifies this body plan handler. This is used for looking up the handler for a given body plan.
        """
        raise NotImplementedError()
    
    def base_body_parts(self, character: "Character") -> list[BodyPartHandler]:
        """
        Return a list of body part handlers. This is used for creating the body parts for a character with this body plan.
        """
        raise NotImplementedError()


class HumanoidBody(BodyPlanHandler):

    def key(self) -> str:
        return "humanoid"

    def base_body_parts(self, character: "Character") -> list[BodyPartHandler]:
        out = list()

        for x in ("torso", "head", "left_arm", "right_arm", "left_leg", "right_leg"):
            if (found := dbat.INDEX.get_bodypart(f"humanoid_{x}")):
                out.append(found)
            else:
                raise ValueError(f"Body part handler not found for humanoid {x}")

        return out


class SaiyanBody(HumanoidBody):
    def key(self) -> str:
        return "saiyan"
    
    def base_body_parts(self, character: "Character") -> list[BodyPartHandler]:
        out = super().base_body_parts()

        if (found := dbat.INDEX.get_bodypart("saiyan_tail")):
            out.append(found)
        else:
            raise ValueError("Body part handler not found for saiyan tail")

        return out


class IcerBody(HumanoidBody):
    def key(self) -> str:
        return "icer"
    
    def base_body_parts(self) -> list[BodyPartHandler]:
        out = super().base_body_parts()

        if (found := dbat.INDEX.get_bodypart("icer_tail")):
            out.append(found)
        else:
            raise ValueError("Body part handler not found for icer tail")

        return out


class BioAndroidBody(HumanoidBody):
    def key(self) -> str:
        return "bioandroid"
    
    def base_body_parts(self) -> list[BodyPartHandler]:
        out = super().base_body_parts()

        if (found := dbat.INDEX.get_bodypart("bioandroid_tail")):
            out.append(found)
        else:
            raise ValueError("Body part handler not found for bioandroid tail")

        return out


ALL_BODY_PLANS = [HumanoidBody, SaiyanBody, IcerBody, BioAndroidBody]


class BodyPart(BaseModel):
    handler_key: str = Field(..., description="The key that identifies this body part. This is used for looking up the body part for a given character.")
    _handler: BodyPartHandler | None = PrivateAttr(default=None)


class BodyPlan(BaseModel):
    parts: dict[str, BodyPart] = Field(default_factory=dict, description="A mapping of body part names to body part data. This is used for storing the data for each body part, such as its name, description, and what it can do.")
    handler_key: str = Field(..., description="The key that identifies this body plan. This is used for looking up the body plan for a given character.")
    _handler: BodyPlanHandler | None = PrivateAttr(default=None)