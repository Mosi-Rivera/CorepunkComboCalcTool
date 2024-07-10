#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define BASE_W_DAMAGE 320
#define AP_W_MULT 1 
#define W_PROCS 3 
#define BASE_E_DAMAGE 180
#define DOT_E_DAMAGE_SCALING 0
#define BASE_R_DAMAGE 180
#define AD_R_MULT 1.8
#define WEAPON_ABILITY_AD_SCALING 1
#define WEAPON_ABILITY_AP_SCALING 0
#define STACKING_MULT_MULTIPLIER 1
char* basic_attack_string = "aa";
char* weapon_ability_string = "wa";
char* q_ability_string = "q";
char* w_ability_string = "w";
char* e_ability_string = "e";
char* r_ability_string = "r";

struct Stats {
	int ad;
	int armor;
	int ap;
	int mr;
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

bool isValidActionCharacter(char c) {
	return c >= 'a' && c <= 'z';
}

int calculateBasicAttackDamage(int ad, int armor, float mult, bool crit) {
	armor++;
	return (crit ? 2 : 1) * ad * mult;
}

int calculateQDamage(int ad, int armor, float mult) {
	return calculateBasicAttackDamage(ad, armor, 1.35 * mult, true);
}

int calculateWeaponAbilityDamage(int ap, int ad, int armor, int mr, float mult) {
	armor++;
	mr++;
	return WEAPON_ABILITY_AD_SCALING * ad + (WEAPON_ABILITY_AP_SCALING * ap * mult);
}

int calculateWDamage(int ap, int ad, int armor, int mr, float mult) {
	ad++;
	armor++;
	mr++;
	return BASE_W_DAMAGE + AP_W_MULT * ap * mult;
}

int calculateEDamage(int ap, int ad, int armor, int mr, float mult) {
	ad++;
	armor++;
	mr++;
	return BASE_E_DAMAGE + DOT_E_DAMAGE_SCALING * ap * mult;
}

int calculateRDamage(int ap, int ad, int armor, int mr, float mult) {
	ap++;
	armor++;
	mr++;
	return BASE_R_DAMAGE + AD_R_MULT * ad * mult;
}

float calculateMult(float base, unsigned int stacks) {
	stacks = stacks > 5 ? 5 : stacks;
	return base + ((float)stacks * STACKING_MULT_MULTIPLIER) / 100;
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

void calculateComboDamage(struct Stats* stats, struct Results* results, char* execution_order) {
	int mage_mult_stacks = 0;
	unsigned int action_index = 0;
	enum Action action;
	float base_mult = 1.08 + 1.05;

	printf("\nCOMBO:\n");
	while (action != NONE) {
		execution_order = getNextAction(execution_order, &action);
		switch (action) {
			case Q_ABILITY: {
				results->q_total += calculateQDamage(stats->ad, stats->armor, calculateMult(base_mult, mage_mult_stacks));
				printf("\t%u) Q\n", action_index + 1);
				break;
			}
			case W_ABILITY: {
				results->w_total += calculateWDamage(stats->ap, stats->ad, stats->armor, stats->mr, calculateMult(base_mult, mage_mult_stacks));
				mage_mult_stacks++;
				printf("\t%u) W\n", action_index + 1);
				break;
			}
			case E_ABILITY: {
				results->e_total += calculateEDamage(stats->ap, stats->ad, stats->armor, stats->mr, calculateMult(base_mult, mage_mult_stacks));
				mage_mult_stacks++;
				printf("\t%u) E\n", action_index + 1);
				break;
			}
			case R_ABILITY: {
				results->r_total+= calculateRDamage(stats->ap, stats->ad, stats->armor, stats->mr, calculateMult(base_mult, mage_mult_stacks));
				mage_mult_stacks++;
				printf("\t%u) R\n", action_index + 1);
				break;
			}
			case BASIC_ATTACK: {
				results->basic_attack_total += calculateBasicAttackDamage(stats->ad, stats->armor, 1, false);
				printf("\t%u) BASIC ATTACK\n", action_index + 1);
				break;
			}
			case WEAPON_ABILITY: {
				results->weapon_ability_total += calculateWeaponAbilityDamage(stats->ap, stats->ad, stats->armor, stats->mr, calculateMult(base_mult, mage_mult_stacks));
				mage_mult_stacks++;
				printf("\t%u) WEAPON\n", action_index + 1);
				break;
			}
			default: break;
		}
		action_index++;
	}

	results->total = results->basic_attack_total + results->weapon_ability_total + results->q_total + results->w_total + results->e_total + results->r_total;
	results->hp_to_execute = (100 * results->total) / 80;
}

void printInfo(struct Results* results) {
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

void printInvalidArgumentsError(void) {
	perror("Invalid argument, requires either one, two or four arguments\nExamples:\n\t[ability power: number]\n\t[ability power: number] [attack power: number]\n\t[ability power: number] [attack power: number] [magic resist: number] [armor: number]\n");
}

int main(int argc, char** argv) {
	struct Stats stats = {0};
	struct Results results = {0};
	if (argc == 3) {
		stats.ap = atoi(argv[1]);
		stats.ad = 0;
		// stats.armor = 0;
		// stats.mr = 0;
	} else if (argc == 4) {
		stats.ap = atoi(argv[1]);
		stats.ad = atoi(argv[2]);
		// stats.armor = 0;
		// stats.mr = 0;
	} /* else if (argc == 5) {
		stats.ap = atoi(argv[1]);
		stats.ad = atoi(argv[2]);
		stats.armor = atoi(argv[3]);
		stats.mr = atoi(argv[4]);

	} */ else {
		printInvalidArgumentsError();
		return 1;
	}
	calculateComboDamage(&stats, &results, argv[argc - 1]);
	printInfo(&results);
	return 0;
}
