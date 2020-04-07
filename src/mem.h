#ifndef MEM_H_
#define MEM_H_

#include "str_buf.h"


bool arc_obj_diag(BUFFER *output);

struct char_data;
struct obj_data;
struct area_data;
struct room_index_data;
struct exit_data;
struct reset_data;
struct mob_index_data;
struct obj_index_data;
struct prog_code;
struct prog_list;
struct shop_data;
struct affect_data;
struct help_data;
struct descriptor_data;
struct boss_achieve_entry;
struct boss_achieve_record;

struct char_data *alloc_CHAR_DATA(void);
void dealloc_CHAR_DATA(struct char_data *ptr);
struct obj_data *alloc_OBJ_DATA(void);
void dealloc_OBJ_DATA(struct obj_data *ptr);
struct area_data *alloc_AREA_DATA(void);
void dealloc_AREA_DATA(struct area_data *ptr);
struct room_index_data *alloc_ROOM_INDEX_DATA(void);
void dealloc_ROOM_INDEX_DATA(struct room_index_data *ptr);
struct exit_data *alloc_EXIT_DATA(void);
void dealloc_EXIT_DATA(struct exit_data *ptr);
struct reset_data *alloc_RESET_DATA(void);
void dealloc_RESET_DATA(struct reset_data *ptr);
struct mob_index_data *alloc_MOB_INDEX_DATA(void);
void dealloc_MOB_INDEX_DATA(struct mob_index_data *ptr);
struct obj_index_data *alloc_OBJ_INDEX_DATA(void);
void dealloc_OBJ_INDEX_DATA(struct obj_index_data *ptr);
struct prog_code *alloc_PROG_CODE(void);
void dealloc_PROG_CODE(struct prog_code *ptr);
struct prog_list *alloc_MPROG_LIST(void);
void dealloc_MPROG_LIST(struct prog_list *ptr);
struct prog_list *alloc_OPROG_LIST(void);
void dealloc_OPROG_LIST(struct prog_list *ptr);
struct prog_list *alloc_APROG_LIST(void);
void dealloc_APROG_LIST(struct prog_list *ptr);
struct prog_list *alloc_RPROG_LIST(void);
void dealloc_RPROG_LIST(struct prog_list *ptr);
struct shop_data *alloc_SHOP_DATA(void);
void dealloc_SHOP_DATA(struct shop_data *ptr);
struct affect_data *alloc_AFFECT_DATA(void);
void dealloc_AFFECT_DATA(struct affect_data *ptr);
struct help_data *alloc_HELP_DATA(void);
void dealloc_HELP_DATA(struct help_data *ptr);
struct descriptor_data *alloc_DESCRIPTOR_DATA(void);
void dealloc_DESCRIPTOR_DATA(struct descriptor_data *ptr);
struct boss_achieve_entry *alloc_BOSSACHV(void);
void dealloc_BOSSACHV(struct boss_achieve_entry *ptr);
struct boss_achieve_record *alloc_BOSSREC(void);
void dealloc_BOSSREC(struct boss_achieve_record *ptr);

bool is_valid_CHAR_DATA(struct char_data *p);
bool is_valid_OBJ_DATA(struct obj_data *p);
bool is_valid_AREA_DATA(struct area_data *p);
bool is_valid_ROOM_INDEX_DATA(struct room_index_data *p);
bool is_valid_EXIT_DATA(struct exit_data *p);
bool is_valid_RESET_DATA(struct reset_data *p);
bool is_valid_MOB_INDEX_DATA(struct mob_index_data *p);
bool is_valid_OBJ_INDEX_DATA(struct obj_index_data *p);
bool is_valid_PROG_CODE(struct prog_code *p);
bool is_valid_PROG_LIST(struct prog_list *p);
bool is_valid_SHOP_DATA(struct shop_data *p);
bool is_valid_AFFECT_DATA(struct affect_data *p);
bool is_valid_HELP_DATA(struct help_data *p);
bool is_valid_DESCRIPTOR_DATA(struct descriptor_data *p);
bool is_valid_BOSSACHV(struct boss_achieve_entry *p);
bool is_valid_BOSSREC(struct boss_achieve_record *p);

#ifdef MEM_C_

#ifdef ARC_CUTEST
#define AO_STATIC
#define AO_TEST_EXTERN extern
#else
#define AO_STATIC static
#define AO_TEST_EXTERN
#endif

struct arc_obj_type
{
    unsigned long long ao_count;
    const char * const name;
    const size_t aoh_offset;
    const size_t wrapped_offset;
    const size_t aot_offset;
};

#define AO_MAGIC_INIT_0 0xC6
#define AO_MAGIC_INIT_1 0xA5
#define AO_MAGIC_DEINIT_0 0x39
#define AO_MAGIC_DEINIT_1 0x5A

struct arc_obj
{
    unsigned char magic_id[2];
    struct arc_obj *ao_next;
    struct arc_obj *ao_prev;
    struct arc_obj_type *ao_type;
};

AO_TEST_EXTERN AO_STATIC struct arc_obj all_aoh;
AO_TEST_EXTERN AO_STATIC struct arc_obj all_aot;

AO_STATIC void arc_obj_init(struct arc_obj_type *ao_type, struct arc_obj *aoh, struct arc_obj *aot);
AO_STATIC void arc_obj_deinit(struct arc_obj_type *ao_type, struct arc_obj *aoh, struct arc_obj *aot);

#endif // MEM_C_


#endif // MEM_H_