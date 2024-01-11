#include "dbat/transformation_class.h"
#include <string>
#include <dbat/utils.h>



enum tfflag {
	damage_bonus,
	damage_reduction,
	dodge_chance,
	oozaru,
	silent_transform,
	speed_increase,
	accuracy_increase
};

class State {
public:
	float value;
	tfflag tfflag;
};


class Transformation {
public:
	const char* name;
	int64_t flat_bonus[3] = { 0, 0, 0 };
	float multiplier[3] = { 1, 1, 1 };
	float drain = 0;
	int64_t requirement = 0;
	int flag_index = -1;
	int ps_cost = 0;
	const char* transform_to;
	const char* transform_from;
	const char* transform_to_room;
	const char* transform_from_room;
	std::vector<State> state_list;

	Transformation() {
	}

	Transformation(const char* name, int64_t flat_bonus[3], float multiplier[3], float drain, int64_t requirement, int flag_index, int ps_cost, const char* transform_to,
		const char* transform_from, const char* transform_to_room, const char* transform_from_room) {
		this->name = name;
		this->flat_bonus[0] = flat_bonus[0];
		this->flat_bonus[1] = flat_bonus[1];
		this->flat_bonus[2] = flat_bonus[2];
		this->multiplier[0] = multiplier[0];
		this->multiplier[1] = multiplier[1];
		this->multiplier[2] = multiplier[2];
		this->drain = drain;
		this->requirement = requirement;
		this->flag_index = flag_index;
		this->ps_cost = ps_cost;
		this->transform_to = transform_to;
		this->transform_from = transform_from;
		this->transform_to_room = transform_to_room;
		this->transform_from_room = transform_from_room;
	}


	Transformation(const char* name, int64_t flat_bonus[3], float multiplier[3], float drain, int64_t requirement, int flag_index, int ps_cost, const char* transform_to,
		const char* transform_from, const char* transform_to_room, const char* transform_from_room, std::vector<State> state_list) {
		this->name = name;
		this->flat_bonus[0] = flat_bonus[0];
		this->flat_bonus[1] = flat_bonus[1];
		this->flat_bonus[2] = flat_bonus[2];
		this->multiplier[0] = multiplier[0];
		this->multiplier[1] = multiplier[1];
		this->multiplier[2] = multiplier[2];
		this->drain = drain;
		this->requirement = requirement;
		this->flag_index = flag_index;
		this->ps_cost = ps_cost;
		this->transform_to = transform_to;
		this->transform_from = transform_from;
		this->transform_to_room = transform_to_room;
		this->transform_from_room = transform_from_room;
		this->state_list = state_list;
	}

	bool hasForm(struct char_data* ch) {
		if (IS_NPC(ch)) return false;

		if (flag_index != -1)
			return TF_FLAGGED(ch, flag_index);
		else
			return true;
	}

	bool canTransform(struct char_data* ch) {
		if (IS_NPC(ch) || (ch->getBasePL()) < requirement) return false;


		if (flag_index != -1)
			return TF_FLAGGED(ch, flag_index);
		else
			return true;
	}

	void echoTransform(struct char_data* ch) {
		act(transform_to, true, ch, nullptr, nullptr, TO_CHAR);
		act(transform_to_room, true, ch, nullptr, nullptr, TO_ROOM);
	}

	void echoUnTransform(struct char_data* ch) {
		act(transform_from, true, ch, nullptr, nullptr, TO_CHAR);
		act(transform_from_room, true, ch, nullptr, nullptr, TO_ROOM);
	}

	float getTransformMultiplier(struct char_data* ch, int choice) {
		return multiplier[choice];
	}

	float getTransformFlatBonus(struct char_data* ch, int choice) {
		return flat_bonus[choice];
	}

	bool hasState(tfflag state) {
		int size = state_list.size();

		if (size == 0) return false;

		for (int i = 0; i < size; i++) {
			if (state_list[i].tfflag == state) return true;
		}
		return false;
	}

	float getState(tfflag state) {
		int size = state_list.size();

		for (int i = 0; i < size; i++) {
			if (state_list[i].tfflag == state) return state_list[i].value;
		}

		return 0;
	}

	/*static Transformation* GET_TRANSFORM_INDEX(int index) {
		if (TF_LIST[index] == nullptr) INITIALIZE(); 
		if (TF_LIST[index] == nullptr) return TF_LIST[1];
		return TF_LIST[index];
	}

	static void INITIALIZE() {
		Transformation base(TF_BASE);
		base.name = "Base";
		base.flat_bonus[0] = 0;
		base.flat_bonus[1] = 0;
		base.flat_bonus[2] = 0;
		base.multiplier[0] = 1;
		base.multiplier[1] = 1;
		base.multiplier[2] = 1;
		base.drain = 0;
		base.requirement = 0;
		base.ps_cost = 0;

		Transformation human_first(TF_HUMAN_FIRST);
		human_first.name = "SuperHuman First";
		human_first.flat_bonus[0] = 1000000;
		human_first.flat_bonus[1] = 1000000;
		human_first.flat_bonus[2] = 1000000;
		human_first.multiplier[0] = 2;
		human_first.multiplier[1] = 2;
		human_first.multiplier[2] = 2;
		human_first.drain = 0.1;
		human_first.requirement = 1800000;
		human_first.ps_cost = 1;
		human_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		human_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		human_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		human_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";

		Transformation lsaiyan_first(TF_LSAIYAN_FIRST);
		lsaiyan_first.name = "SuperHuman First";
		lsaiyan_first.flat_bonus[0] = 1000000;
		lsaiyan_first.flat_bonus[1] = 1000000;
		lsaiyan_first.flat_bonus[2] = 1000000;
		lsaiyan_first.multiplier[0] = 2;
		lsaiyan_first.multiplier[1] = 2;
		lsaiyan_first.multiplier[2] = 2;
		lsaiyan_first.drain = 0.1;
		lsaiyan_first.requirement = 1800000;
		lsaiyan_first.ps_cost = 1;
		lsaiyan_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		lsaiyan_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		lsaiyan_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		lsaiyan_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";


		Transformation saiyan_first(TF_SAIYAN_FIRST);
		saiyan_first.name = "SuperHuman First";
		saiyan_first.flat_bonus[0] = 1000000;
		saiyan_first.flat_bonus[1] = 1000000;
		saiyan_first.flat_bonus[2] = 1000000;
		saiyan_first.multiplier[0] = 2;
		saiyan_first.multiplier[1] = 2;
		saiyan_first.multiplier[2] = 2;
		saiyan_first.drain = 0.1;
		saiyan_first.requirement = 1800000;
		saiyan_first.ps_cost = 1;
		saiyan_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		saiyan_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		saiyan_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		saiyan_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";


		Transformation icer_first(TF_ICER_FIRST);
		icer_first.name = "SuperHuman First";
		icer_first.flat_bonus[0] = 1000000;
		icer_first.flat_bonus[1] = 1000000;
		icer_first.flat_bonus[2] = 1000000;
		icer_first.multiplier[0] = 2;
		icer_first.multiplier[1] = 2;
		icer_first.multiplier[2] = 2;
		icer_first.drain = 0.1;
		icer_first.requirement = 1800000;
		icer_first.ps_cost = 1;
		icer_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		icer_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		icer_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		icer_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";



		Transformation konatsu_first(TF_KONATSU_FIRST);
		konatsu_first.name = "SuperHuman First";
		konatsu_first.flat_bonus[0] = 1000000;
		konatsu_first.flat_bonus[1] = 1000000;
		konatsu_first.flat_bonus[2] = 1000000;
		konatsu_first.multiplier[0] = 2;
		konatsu_first.multiplier[1] = 2;
		konatsu_first.multiplier[2] = 2;
		konatsu_first.drain = 0.1;
		konatsu_first.requirement = 1800000;
		konatsu_first.ps_cost = 1;
		konatsu_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		konatsu_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		konatsu_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		konatsu_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";

		Transformation namek_first(TF_NAMEK_FIRST);
		namek_first.name = "SuperHuman First";
		namek_first.flat_bonus[0] = 1000000;
		namek_first.flat_bonus[1] = 1000000;
		namek_first.flat_bonus[2] = 1000000;
		namek_first.multiplier[0] = 2;
		namek_first.multiplier[1] = 2;
		namek_first.multiplier[2] = 2;
		namek_first.drain = 0.1;
		namek_first.requirement = 1800000;
		namek_first.ps_cost = 1;
		namek_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		namek_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		namek_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		namek_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";

		Transformation mutant_first(TF_MUTANT_FIRST);
		mutant_first.name = "SuperHuman First";
		mutant_first.flat_bonus[0] = 1000000;
		mutant_first.flat_bonus[1] = 1000000;
		mutant_first.flat_bonus[2] = 1000000;
		mutant_first.multiplier[0] = 2;
		mutant_first.multiplier[1] = 2;
		mutant_first.multiplier[2] = 2;
		mutant_first.drain = 0.1;
		mutant_first.requirement = 1800000;
		mutant_first.ps_cost = 1;
		mutant_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		mutant_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		mutant_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		mutant_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";

		Transformation bio_first(TF_BIO_FIRST);
		bio_first.name = "SuperHuman First";
		bio_first.flat_bonus[0] = 1000000;
		bio_first.flat_bonus[1] = 1000000;
		bio_first.flat_bonus[2] = 1000000;
		bio_first.multiplier[0] = 2;
		bio_first.multiplier[1] = 2;
		bio_first.multiplier[2] = 2;
		bio_first.drain = 0.1;
		bio_first.requirement = 1800000;
		bio_first.ps_cost = 1;
		bio_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		bio_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		bio_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		bio_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";

		Transformation android_first(TF_ANDROID_FIRST);
		android_first.name = "SuperHuman First";
		android_first.flat_bonus[0] = 1000000;
		android_first.flat_bonus[1] = 1000000;
		android_first.flat_bonus[2] = 1000000;
		android_first.multiplier[0] = 2;
		android_first.multiplier[1] = 2;
		android_first.multiplier[2] = 2;
		android_first.drain = 0.1;
		android_first.requirement = 1800000;
		android_first.ps_cost = 1;
		android_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		android_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		android_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		android_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";

		Transformation kai_first(TF_KAI_FIRST);
		kai_first.name = "SuperHuman First";
		kai_first.flat_bonus[0] = 1000000;
		kai_first.flat_bonus[1] = 1000000;
		kai_first.flat_bonus[2] = 1000000;
		kai_first.multiplier[0] = 2;
		kai_first.multiplier[1] = 2;
		kai_first.multiplier[2] = 2;
		kai_first.drain = 0.1;
		kai_first.requirement = 1800000;
		kai_first.ps_cost = 1;
		kai_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		kai_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		kai_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		kai_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";

		Transformation tuffle_first(TF_TUFFLE_FIRST);
		tuffle_first.name = "SuperHuman First";
		tuffle_first.flat_bonus[0] = 1000000;
		tuffle_first.flat_bonus[1] = 1000000;
		tuffle_first.flat_bonus[2] = 1000000;
		tuffle_first.multiplier[0] = 2;
		tuffle_first.multiplier[1] = 2;
		tuffle_first.multiplier[2] = 2;
		tuffle_first.drain = 0.1;
		tuffle_first.requirement = 1800000;
		tuffle_first.ps_cost = 1;
		tuffle_first.transform_to = "@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!";
		tuffle_first.transform_from = "@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!";
		tuffle_first.transform_to_room = "@wYou revert from @CSuper @cHuman @GFirst@w.@n";
		tuffle_first.transform_from_room = "@w$n@w reverts from @CSuper @cHuman @GFirst.@n";


		/*
		HUMAN_FIRST();
		HUMAN_SECOND();
		HUMAN_THIRD();
		HUMAN_FOURTH();
		LSAIYAN_FIRST();
		LSAIYAN_SECOND();
		SAIYAN_FIRST();
		SAIYAN_SECOND();
		SAIYAN_THIRD();
		SAIYAN_FOURTH();
		ICER_FIRST();
		ICER_SECOND();
		ICER_THIRD();
		ICER_FOURTH();
		KONATSU_FIRST();
		KONATSU_SECOND();
		KONATSU_THIRD();
		NAMEK_FIRST();
		NAMEK_SECOND();
		NAMEK_THIRD();
		NAMEK_FOURTH();
		MUTANT_FIRST();
		MUTANT_SECOND();
		MUTANT_THIRD();
		BIO_FIRST();
		BIO_SECOND();
		BIO_THIRD();
		BIO_FOURTH();
		ANDROID_FIRST();
		ANDROID_SECOND();
		ANDROID_THIRD();
		ANDROID_FOURTH();
		ANDROID_FIFTH();
		ANDROID_SIXTH();
		KAI_FIRST();
		KAI_SECOND();
		KAI_THIRD();
		TUFFLE_FIRST();
		TUFFLE_SECOND();
		TUFFLE_THIRD();
		OOZARU();
		GOLDEN_OOZARU();
		MYSTIC();
		

	}*/

};
























