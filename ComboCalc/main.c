#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
char* infiltrator_string = "infiltrator";
char* basic_attack_string = "aa";
char* weapon_ability_string = "wa";
char* q_ability_string = "q";
char* w_ability_string = "w";
char* e_ability_string = "e";
char* r_ability_string = "r";

enum DamageType {
	PHYSICAL,
	MAGICAL,
	TRUE_DAMAGE
};

struct AbilityData {
	float physical_mult;
	float magical_mult;
	float percent_max_health;
	float percent_missing_health;
	unsigned int physical_flat_damage;
	unsigned int magical_flat_damage;
	unsigned int physical_dot;
	unsigned int magical_dot;
	enum DamageType type;
	enum DamageType dot_type;
	float cast_time;
	float cooldown;
	float uninteractable_cc_time;
	float interactable_cc_time;
};

struct DamageLog {
	int physical;
	int magical;
	int physical_dot;
	int magical_dot;
	int true_damage;
	int crits;
};

struct AutoAttackData {
	float physical_mult;
	float magical_mult;
};

struct Stats {
	int physical_power;
	int magical_power;
	int physical_resistance;
	int magical_resistance;
	int auto_attack_crit_damage;
	int auto_attack_crit_chance;
	int ability_crit_damage;
	int ability_crit_chance;
	float attack_speed;
};

struct Results {
	int basic_attack_total;
	int weapon_ability_total;
	int q_total;
	int w_total;
	int e_total;
	int r_total;
	int total;
	int hp_to_execute;
};

enum Action {
	NONE,
	BASIC_ATTACK,
	WEAPON_ABILITY,
	Q_ABILITY,
	W_ABILITY,
	E_ABILITY,
	R_ABILITY
};

char toLowerCase(const char c) {
	if (!isalpha(c)) {
		return c;
	}
	return c >= 'a' ? c : c - 'A' + 'a'; 
}

bool insensitiveStringCompare(const char* s1, const char* s2) {
	while (*s1 && *s2) {
		if (*s1 != *s2 && toLowerCase(*s1) != toLowerCase(*s2)) {
			return false;
		}
		s1++;
		s2++;
	}
	return true;
}

bool getCrit(float chance) {
	return rand() % 101 < chance;
}

bool isValidActionCharacter(char c) {
	return c >= 'a' && c <= 'z';
}

int calculateDamageDoneAndMitigated(int damage, enum DamageType type, struct Stats* defending, struct DamageLog* attacking_log, struct DamageLog* defending_log, struct DamageLog* ability_log) {
	defending->physical_power++; //Because the compiler is a crybaby
	defending->physical_power--; //Because the compiler is a crybaby
	if (type == TRUE_DAMAGE) {
		attacking_log->true_damage = damage;
		ability_log->true_damage += damage;
		defending_log->true_damage = 0;
		return damage;
	}
	int damage_done = damage; //TODO: Figure out damage formula;
	int damage_mitigated = damage - damage_done;

	if (type == PHYSICAL) {
		attacking_log->physical += damage_done;
		ability_log->physical += damage_done;
		defending_log->physical += damage_mitigated;
		return damage_done;
	} else if (type == MAGICAL) {
		attacking_log->magical += damage_done;
		ability_log->magical += damage_done;
		defending_log->magical += damage_mitigated;
		return damage_done;
	}
	return 0;
}

void calculateDotDamageDoneAndMitigated(int damage, enum DamageType type, struct Stats* defending, struct DamageLog* attacking_log, struct DamageLog* defending_log, struct DamageLog* ability_log) {
	defending->physical_power++; //Because the compiler is a crybaby
	defending->physical_power--; //Because the compiler is a crybaby
	if (type == TRUE_DAMAGE) {
		attacking_log->true_damage += damage;
		ability_log->true_damage += damage;
		defending_log->true_damage += 0;
		return;
	}

	int damage_done = damage; //TODO: Figure out damage formula;
	int damage_mitigated = damage - damage_done;

	if (type == PHYSICAL) {
		attacking_log->physical_dot += damage_done;
		ability_log->physical_dot += damage_done;
		defending_log->physical_dot += damage_mitigated;
		return;
	} else if (type == MAGICAL) {
		attacking_log->magical_dot += damage_done;
		ability_log->magical_dot += damage_done;
		defending_log->magical_dot += damage_mitigated;
		return;
	}
}

void calculateBasicAttackDamage(
	struct AutoAttackData* auto_attack_data,
	struct Stats* attacking,
	struct Stats* defending,
	float mult,
	bool force_crit,
	struct DamageLog* attacking_log,
	struct DamageLog* defending_log,
	struct DamageLog* basic_attack_log
) {
	bool did_crit = force_crit || getCrit(attacking->auto_attack_crit_chance);
	if (did_crit) {
		attacking_log->crits++;
		basic_attack_log->crits++;
	}

	int damage = (did_crit ? attacking->auto_attack_crit_damage : 1) * (attacking->physical_power * auto_attack_data->physical_mult + attacking->magical_power * auto_attack_data->magical_mult) * mult ;
	calculateDamageDoneAndMitigated(damage, PHYSICAL, defending, attacking_log, defending_log, basic_attack_log);
}

void calculateAbilityDamage(struct AbilityData* ability, struct Stats* attacking, struct Stats* defending, float mult, bool force_crit, struct DamageLog* attacking_log, struct DamageLog* defending_log, struct DamageLog* ability_log) {
	defending->physical_power++; //Because the compiler is a crybaby
	defending->physical_power--; //Because the compiler is a crybaby
	bool did_crit = force_crit || getCrit(attacking->auto_attack_crit_chance);
	if (did_crit) {
		attacking_log->crits++;
		ability_log->crits++;
	}

	int damage = (did_crit ? attacking->ability_crit_damage : 1) * mult * ((
		ability->physical_mult * attacking->physical_power
	) + (
		ability->magical_mult * attacking->magical_power
	));
	int dot = ability->physical_dot * attacking->physical_power + ability->magical_dot * attacking->magical_power;

	calculateDamageDoneAndMitigated(damage, ability->type, defending, attacking_log, defending_log, ability_log);
	calculateDotDamageDoneAndMitigated(dot, ability->dot_type, defending, attacking_log, defending_log, ability_log);
}

float calculateStacksMult(float base, unsigned int stacks, float value) {
	stacks = stacks > 5 ? 5 : stacks;
	return base + (float)stacks * value;
}

char* getNextAction(char* execution_order, enum Action* action) {
	size_t i;
	char result[3];
	for (i = 0; i < 3; i++) {
		if (!isValidActionCharacter(execution_order[i])) {
			break;
		}
		result[i] = execution_order[i];
	}
	result[i < 3 ? i : 2] = 0;

	if (execution_order != 0) {
		i++;
	}

	if (strcmp(q_ability_string, result) == 0) {
		*action = Q_ABILITY;
	} else if (strcmp(w_ability_string, result) == 0) {
		*action = W_ABILITY;
	} else if (strcmp(e_ability_string, result) == 0) {
		*action = E_ABILITY;
	} else if (strcmp(r_ability_string, result) == 0) {
		*action = R_ABILITY;
	} else if (strcmp(basic_attack_string, result) == 0) {
		*action = BASIC_ATTACK;
	} else if (strcmp(weapon_ability_string, result) == 0) {
		*action = WEAPON_ABILITY;
	} else {
		*action = NONE;
	}
	return execution_order + i;
}

void calculateComboDamage(
	struct Stats* attacking,
	struct Stats* defending,
	struct DamageLog* attacking_log,
	struct DamageLog* defending_log,
	struct AutoAttackData* auto_attack_data,
	struct AbilityData* weapon_ability_data,
	struct AbilityData* q_data,
	struct AbilityData* w_data,
	struct AbilityData* e_data,
	struct AbilityData* r_data,
	struct DamageLog* auto_attack_log,
	struct DamageLog* weapon_ability_log,
	struct DamageLog* q_log,
	struct DamageLog* w_log,
	struct DamageLog* e_log,
	struct DamageLog* r_log,
	char* execution_order
) {
	unsigned int action_index = 0;
	unsigned int ability_stacks = 0;
	unsigned int basic_attack_stacks = 0;
	enum Action action;


	printf("\nCOMBO:\n");
	while (action != NONE) {
		execution_order = getNextAction(execution_order, &action);
		switch (action) {
			case Q_ABILITY: {
				calculateAbilityDamage(
					q_data,
					attacking,
					defending,
					1,
					false,
					attacking_log,
					defending_log,
					q_log
				);
				ability_stacks++;
				printf("\t%u) Q\n", action_index + 1);
				break;
			}
			case W_ABILITY: {
				calculateAbilityDamage(
					w_data,
					attacking,
					defending,
					1,
					false,
					attacking_log,
					defending_log,
					w_log
				);
				ability_stacks++;
				printf("\t%u) W\n", action_index + 1);
				break;
			}
			case E_ABILITY: {
				calculateAbilityDamage(
					e_data,
					attacking,
					defending,
					1,
					false,
					attacking_log,
					defending_log,
					e_log
				);
				ability_stacks++;
				printf("\t%u) E\n", action_index + 1);
				break;
			}
			case R_ABILITY: {
				calculateAbilityDamage(
					r_data,
					attacking,
					defending,
					1,
					false,
					attacking_log,
					defending_log,
					r_log
				);
				ability_stacks++;
				printf("\t%u) R\n", action_index + 1);
				break;
			}
			case BASIC_ATTACK: {
				calculateBasicAttackDamage(
					auto_attack_data,
					attacking,
					defending,
					1,
					false,
					attacking_log,
					defending_log,
					auto_attack_log
				);
				basic_attack_stacks++;
				printf("\t%u) BASIC ATTACK\n", action_index + 1);
				break;
			}
			case WEAPON_ABILITY: {
				calculateAbilityDamage(
					weapon_ability_data,
					attacking,
					defending,
					1,
					false,
					attacking_log,
					defending_log,
					weapon_ability_log
				);
				ability_stacks++;
				printf("\t%u) WEAPON\n", action_index + 1);
				break;
			}
			default: break;
		}
		action_index++;
	}
}

void printInfo(struct DamageLog* attacking, struct DamageLog* defending) {
	printf("\nDAMAGE SUMMARY\n");
	printf("\n\tBASIC ATTACK:\n\t\t%d\n", results->basic_attack_total);
	printf("\n\tWEAPON ABILITY:\n\t\t%d\n", results->weapon_ability_total);
	printf("\n\tQ:\n\t\t%d\n", results->q_total);
	printf("\n\tW:\n\t\t%d\n", results->w_total);
	printf("\n\tE:\n\t\t%d\n", results->e_total);
	printf("\n\tR:\n\t\t%d\n", results->r_total);
	printf("\n\tTOTAL DAMAGE:\n\t\t%d\n", results->total);
	printf("\n\tEXECUTES PLAYERS UNDER %d HP!\n", results->hp_to_execute);
}

// void printExtraInfo(
// 	struct AutoAttackData* auto_attack_data,
// 	struct AbilityData* weapon_ability_data,
// 	struct AbilityData* q_data,
// 	struct AbilityData* w_data,
// 	struct AbilityData* e_data,
// 	struct AbilityData* r_data,
// 	struct DamageLog* auto_attack_log,
// 	struct DamageLog* weapon_ability_log,
// 	struct DamageLog* q_log,
// 	struct DamageLog* w_log,
// 	struct DamageLog* e_log,
// 	struct DamageLog* r_log
// ) {
// 	
// }

void printInvalidArgumentsError(void) {
	perror("Invalid argument, requires either one, two or four arguments\nExamples:\n\t[ability power: number]\n\t[ability power: number] [attack power: number]\n\t[ability power: number] [attack power: number] [magic resist: number] [armor: number]\n");
}

void setInfiltratorData(
	struct AutoAttackData* auto_attack_data,
	struct AbilityData* weapon_ability_data,
	struct AbilityData* q_data,
	struct AbilityData* w_data,
	struct AbilityData* e_data,
	struct AbilityData* r_data
) {
	*auto_attack_data = (struct AutoAttackData){
		1,
		0
	};
	*weapon_ability_data = (struct AbilityData){
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		PHYSICAL,
		PHYSICAL,
		0,
		26,
		5,
		0
	};
	*q_data = (struct AbilityData){
		1.45,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		PHYSICAL,
		PHYSICAL,
		0,
		14,
		5,
		0
	};
	*w_data = (struct AbilityData){
		0,
		1.1,
		0,
		0,
		0,
		125,
		0,
		0,
		MAGICAL,
		MAGICAL,
		0,
		15,
		2,
		0
	};
	*e_data = (struct AbilityData){
		0,
		0,
		0,
		0,
		0,
		85,
		0,
		0,
		MAGICAL,
		MAGICAL,
		0,
		10,
		0,
		2
	};
	*r_data = (struct AbilityData){
		1.3,
		0,
		0,
		0,
		250,
		0,
		0,
		0,
		PHYSICAL,
		PHYSICAL,
		0,
		10,
		0,
		2
	};
}

int main(int argc, char** argv) {
	struct Stats attacking = {0};
	struct Stats defending = {0};
	struct DamageLog attacking_log = {0};
	struct DamageLog defending_log = {0};
	struct AutoAttackData auto_attack_data = {0};
	struct AbilityData weapon_ability_data = {0};
	struct AbilityData q_data = {0};
	struct AbilityData w_data = {0};
	struct AbilityData e_data = {0};
	struct AbilityData r_data = {0};
	struct DamageLog auto_attack_log = {0};
	struct DamageLog weapon_ability_log = {0};
	struct DamageLog q_log = {0};
	struct DamageLog w_log = {0};
	struct DamageLog e_log = {0};
	struct DamageLog r_log = {0};

	struct Stats stats = {0};
	struct Results results = {0};
	if (argc == 5) {
		if (insensitiveStringCompare(argv[1], infiltrator_string)) {
			setInfiltratorData(
				&auto_attack_data,
				&weapon_ability_data,
				&q_data,
				&w_data,
				&e_data,
				&r_data
			);
		} else {
			perror("Invalid character.");
		}
		attacking.physical_power = atoi(argv[1]);
		attacking.magical_power = atoi(argv[1]);
	} else if (argc == 4) {
		printInvalidArgumentsError();
		return 1;
	}  else {
		printInvalidArgumentsError();
		return 1;
	}
	srand(time(NULL));
	calculateComboDamage(
		&attacking,
		&defending,
		&attacking_log,
		&defending_log,
		&auto_attack_data,
		&weapon_ability_data,
		&q_data,
		&w_data,
		&e_data,
		&r_data,
		&auto_attack_log,
		&weapon_ability_log,
		&q_log,
		&w_log,
		&e_log,
		&r_log,
		argv[argc - 1]
	);
	printInfo(
		attacking_log,
		defending_log,
	);
	// if (SHOW_EXTRA_INFO) {
	// 	printExtraInfo(
	// 		auto_attack_data,
	// 		weapon_ability_data,
	// 		q_data,
	// 		w_data,
	// 		e_data,
	// 		r_data,
	// 		weapon_ability_log,
	// 		weapon_ability_log,
	// 		q_log,
	// 		w_log,
	// 		e_log,
	// 		r_log
	// 	);
	// }
	return 0;
}
