#include <pokeagb/pokeagb.h>
#include "battle_data/pkmn_bank.h"
#include "battle_data/pkmn_bank_stats.h"
#include "battle_slide_in_data/battle_obj_sliding.h"
#include "battle_data/battle_state.h"
#include "battle_text/battle_textbox_gfx.h"

extern void pick_encounter_message(enum BattleFlag);
extern void setup(void);
extern void battle_slidein(void);
extern void player_throwball_and_moveout_scene(struct Object*);
extern void player_hpbar_slidin_slow(u8 t_id);
extern void vblank_cb_no_merge(void);
extern void vblank_cb_merge_move_select(void);
extern void battle_loop(void);
extern void init_selection_cursor(u8 mode, u8 bank);
extern void load_icons_moves(u8 bank);
extern void set_active_movement(u8 task_id);
extern void update_cursor_move_select(u8 task_id);
extern void show_move_data(void);
extern void option_selection(void);
extern u8 get_ability(struct Pokemon* p);
extern void dprintf(const char * str, ...);
extern void status_graphical_update(u8 bank, enum Effect status);
extern void residual_cbs_init(void);
void init_battle_elements()
{
    // allocate battle specific resources
    for(u8 i = 0; i < 4; i++) {
        p_bank[i] = malloc_and_clear(sizeof(struct pkmn_bank));
        for(u8 j = 0; j < 4; j++) {
            p_bank[i]->objid_hpbox[j] = 0x3F;
        }
            
    }
    //p_bank = (struct pkmn_bank(*)[4])malloc_and_clear(sizeof(struct pkmn_bank) * 4);
    bs_env_windows = (struct bs_elements_positions*)malloc_and_clear(sizeof(struct bs_elements_positions));
    battle_master = (struct battle_main*)malloc_and_clear(sizeof(struct battle_main));
    for(u8 i = 0; i < 10; ++i) {
        battle_master->switch_main.type_objid[i] = 0x3F;
    }
    residual_cbs_init();    
    setup();
    help_system_disable__sp198();
    super.multi_purpose_state_tracker = 0;
    set_callback1((SuperCallback)battle_slidein);
    
    
    battle_type_flags = BATTLE_FLAG_WILD;
    u16 t = MOVE_NUZZLE;
    pokemon_setattr(&party_player[0], REQUEST_MOVE3, &t);
    t = MOVE_PLASMA_FISTS;
    pokemon_setattr(&party_player[0], REQUEST_MOVE4, &t);
    t = 4;
    pokemon_setattr(&party_player[0], REQUEST_PP3, &t);
    t = 10;
    pokemon_setattr(&party_player[0], REQUEST_PP4, &t);
    //t = ITEM_POTION;
    //pokemon_setattr(&party_player[0], REQUEST_HELD_ITEM, &t);
    t = MOVE_DISABLE;
    pokemon_setattr(&party_opponent[0], REQUEST_MOVE1, &t);
    pokemon_setattr(&party_opponent[0], REQUEST_MOVE2, &t);
    pokemon_setattr(&party_opponent[0], REQUEST_MOVE3, &t);
}

extern void ailment_decode(u8 bank, u8 ailment);

void update_pbank(u8 bank, struct update_flags* flags)
{
    // base stats
    u16 species = pokemon_getattr(p_bank[bank]->this_pkmn, REQUEST_SPECIES, NULL);
    p_bank[bank]->b_data.species = species;
    p_bank[bank]->b_data.gender = pokemon_get_gender(p_bank[bank]->this_pkmn);
    p_bank[bank]->b_data.current_hp = pokemon_getattr(p_bank[bank]->this_pkmn, REQUEST_CURRENT_HP, NULL);;
    p_bank[bank]->b_data.total_hp = pokemon_getattr(p_bank[bank]->this_pkmn, REQUEST_TOTAL_HP, NULL);;
    p_bank[bank]->b_data.ability = get_ability(p_bank[bank]->this_pkmn);
    p_bank[bank]->b_data.item = pokemon_getattr(p_bank[bank]->this_pkmn, REQUEST_HELD_ITEM, NULL);
    p_bank[bank]->b_data.level = pokemon_getattr(p_bank[bank]->this_pkmn, REQUEST_LEVEL, NULL);
    p_bank[bank]->b_data.poke_ball = 0;
    p_bank[bank]->b_data.type[0] = pokemon_base_stats[species].type[0];
    p_bank[bank]->b_data.type[1] = pokemon_base_stats[species].type[1];
    p_bank[bank]->b_data.type[1] = (p_bank[bank]->b_data.type[1]) ? p_bank[bank]->b_data.type[1] : MTYPE_EGG;
    p_bank[bank]->b_data.type[2] = MTYPE_EGG;

    ailment_decode(bank, pokemon_getattr(p_bank[bank]->this_pkmn, REQUEST_STATUS_AILMENT, NULL));

    if (!flags->pass_stats) {
        p_bank[bank]->b_data.attack = 0;
        p_bank[bank]->b_data.defense = 0;
        p_bank[bank]->b_data.speed = 0;
        p_bank[bank]->b_data.sp_atk = 0;
        p_bank[bank]->b_data.sp_def = 0;
        p_bank[bank]->b_data.accuracy = 0;
        p_bank[bank]->b_data.evasion = 0;
        p_bank[bank]->b_data.crit_mod = 0;
    }
    
    // user actions should always be cleared
    p_bank[bank]->b_data.is_running = 0;
    p_bank[bank]->b_data.using_item = 0;
    p_bank[bank]->b_data.is_switching = 0;
    p_bank[bank]->b_data.skip_move_select = 0;
    p_bank[bank]->b_data.first_turn = 1;
    
    if (!flags->pass_atk_history) {
        p_bank[bank]->b_data.my_target = 0xFF;
        p_bank[bank]->b_data.last_move = 0;
        p_bank[bank]->b_data.current_move = 0;
        p_bank[bank]->b_data.last_damage = 0;
        p_bank[bank]->b_data.last_attacked_by = 0xFF;
        p_bank[bank]->b_data.last_used_item = 0;
        p_bank[bank]->b_data.ate_berry = 0;
    }

    if (!flags->pass_status) {
        p_bank[bank]->b_data.status = 0;
        p_bank[bank]->b_data.confusion_turns = 0;
        p_bank[bank]->b_data.status_turns = 0;
        p_bank[bank]->b_data.substitute_health = 0;
        p_bank[bank]->b_data.v_status = 0;
        p_bank[bank]->b_data.is_taunted = 0;
        p_bank[bank]->b_data.is_charmed = 0;
        p_bank[bank]->b_data.is_grounded = 0;
    } else {
        status_graphical_update(bank, p_bank[bank]->b_data.status);
    }
    
    if (!flags->pass_disables) {
        p_bank[bank]->b_data.disabled_moves[0] = 0;
        p_bank[bank]->b_data.disabled_moves[1] = 0;
        p_bank[bank]->b_data.disabled_moves[2] = 0;
        p_bank[bank]->b_data.disabled_moves[3] = 0;
		p_bank[bank]->b_data.disable_used_on_slot = 0xFF; // valid slot means used
    }
    
    p_bank[bank]->b_data.illusion = 0;
    p_bank[bank]->b_data.fainted = 0;
}

extern u8 ailment_encode(u8 bank);

void sync_battler_struct(u8 bank)
{
    u16 c_hp = p_bank[bank]->b_data.current_hp;
    u8 ailment = ailment_encode(bank);
    pokemon_setattr(p_bank[bank]->this_pkmn, REQUEST_CURRENT_HP, &c_hp);

    /* TODO: make ailment array conform with external ailments */
    pokemon_setattr(p_bank[bank]->this_pkmn, REQUEST_STATUS_AILMENT, &ailment);
}

/* Called right after sliding in effects have finished executing. */
void init_battle()
{
    switch (super.multi_purpose_state_tracker) {
        case 0:
            gpu_pal_apply((void*)stdpal_get(0), 16*rboxes[0x18 & 0x3F].pal_id, 32);
            pick_encounter_message(battle_type_flags);
            battle_show_message((u8*)string_buffer, 0x18);
            super.multi_purpose_state_tracker++;
            break;
        case 1:
            if (!dialogid_was_acknowledged(0x18 & 0x3F)) {
                objects[bs_env_windows->player_trainer_objid].anim_number++;
                objects[bs_env_windows->player_trainer_objid].callback = player_throwball_and_moveout_scene;
                objects[bs_env_windows->player_trainer_objid].priv[2] = PLAYER_SINGLES_BANK;
                bs_anim_status = 1;
                super.multi_purpose_state_tracker++;
            }
            break;
        case 2:
            if (bs_anim_status)
                return;
            // create and slide into place HPbox
            task_add(player_hpbar_slidin_slow, 1);
            bs_anim_status = 1;
            super.multi_purpose_state_tracker++;
            break;
        case 3:
        {
            // wait for sliding animations to finish
            if (bs_anim_status)
                return;  

            // up and down movement of the active moving Pokemon
            u8 t_id = task_add(set_active_movement, 1);
            tasks[t_id].priv[0] = PLAYER_SINGLES_BANK;
            
            // build p_bank data once animation is finished
            struct update_flags* flags = (struct update_flags*)malloc_and_clear(sizeof(struct update_flags));
            flags->pass_status = true;
            flags->pass_stats = false;
            flags->pass_atk_history = false;
            flags->pass_disables = false;
            update_pbank(PLAYER_SINGLES_BANK, flags);
            update_pbank(OPPONENT_SINGLES_BANK, flags);
            free(flags);
            super.multi_purpose_state_tracker++;
        }
        case 4:
        {
            // initialization finished, set c1 option selection
            super.multi_purpose_state_tracker = 0;
            set_callback1(option_selection);
            break;
        }
        default:
            break;
    };
}

/* Fight menu and move menu selection. Preperation to go into battle loop*/
void option_selection()
{
    switch (super.multi_purpose_state_tracker) {
        case 0:
        {
            // if move is charging up, then option selection is skipped
            if (HAS_VOLATILE(PLAYER_SINGLES_BANK, VOLATILE_CHARGING)) {
                set_callback1(battle_loop);
                return;
            } else if (p_bank[PLAYER_SINGLES_BANK]->b_data.skip_move_select) {
                set_callback1(battle_loop);
                return;
            }
            // initialize fight menu selection
            vblank_handler_set(vblank_cb_no_merge);
            void* map_base = (void *)0x600F800;
            memcpy(map_base, battle_textbox_action_selectMap, sizeof(battle_textbox_action_selectMap));
            init_selection_cursor(1, 0);

            // next state
            bs_anim_status = 1;
            super.multi_purpose_state_tracker++;
            break;
        }
        case 1:
            // wait for input selection from fight menu
            if (bs_anim_status)
                return;
            // interpret selection
            switch (battle_master->selected_option) {
                case OPTION_FIGHT:
                    super.multi_purpose_state_tracker = (battle_master->fight_menu_content_spawned) ? 7 : 2;
                    break;
                case OPTION_POKEMON:
                    super.multi_purpose_state_tracker = 3;
                    break;
                case OPTION_BAG:
                    super.multi_purpose_state_tracker = 4;
                    break;
                case OPTION_RUN:
                    super.multi_purpose_state_tracker = 5;
                    break;
                    
            };
            break;
        case 2:
            /* FIGHT selected from fight menu */
            
            // update tilemap
            vblank_handler_set(vblank_cb_merge_move_select);
            void* map_base = (void *)0x600F800;
            memcpy(map_base, battle_textbox_move_selectMap, sizeof(battle_textbox_action_selectMap));
            
            // init cursor
            init_selection_cursor(0, 0);
            
            // init move types
            load_icons_moves(PLAYER_SINGLES_BANK);
            // set into pause state
            bs_anim_status = 1;
            super.multi_purpose_state_tracker = 6;
            break;
        case 3:
            // POKEMON selection from fight menu
            fade_screen(0xFFFFFFFF, 0,0,16, 0x0000);
            extern void switch_scene_main(void);
            super.multi_purpose_state_tracker = 0;
            set_callback1(switch_scene_main);
            break;
        case 4:
            // BAG selected from fight menu
            break;
        case 5:
        {
            // RUN selected from fight menu
            p_bank[PLAYER_SINGLES_BANK]->b_data.is_running = true;
            super.multi_purpose_state_tracker = 8;
        }
            break;
        case 6:
            break;
        case 7:
            {
            vblank_handler_set(vblank_cb_merge_move_select);
            void* map_base = (void *)0x600F800;
            memcpy(map_base, battle_textbox_move_selectMap, sizeof(battle_textbox_action_selectMap));
            show_move_data();
            tasks[task_add(update_cursor_move_select, 1)].priv[0] = 0;
            bs_anim_status = 1;
            super.multi_purpose_state_tracker = 6;
            break;
            }
        case 8:
        {
            set_callback1(battle_loop);
            void* map_base = (void *)0x600F800;
            memcpy(map_base, battle_textboxMap, sizeof(battle_textboxMap));
            super.multi_purpose_state_tracker = 0;
            break;
        }
    }; 
}













