/**
 *@author Unknown, Jurica Hlevnjak - drop table bugs fixed, reorganized code structure, system catalog tables drop disabled, drop index added, drop view added, drop sequence added, drop trigger added, drop_function added, drop user added, drop group added, AK_drop_test updated
 * 
 *@file drop.c Provides DROP functions
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 17 */

#include "drop.h"

#define DROP_TABLE      0
#define DROP_INDEX      1
#define DROP_VIEW       2
#define DROP_SEQUENCE   3
#define DROP_TRIGGER    4
#define DROP_FUNCTION   5
#define DROP_USER       6
#define DROP_GROUP      7
#define DROP_CONSTRAINT 8

#define NUM_SYS_TABLES  20

// list of system catalog tables
char *system_catalog[NUM_SYS_TABLES] = {
    "AK_relation",
    "AK_attribute",
    "AK_index",
    "AK_view",
    "AK_sequence",
    "AK_function",
    "AK_function_arguments",
    "AK_trigger",
    "AK_trigger_conditions",
    "AK_db",
    "AK_db_obj",
    "AK_user",
    "AK_group",
    "AK_user_group",
    "AK_user_right",
    "AK_group_right",
    "AK_constraints_between",
    "AK_constraints_not_null",
    "AK_constraints_unique",
    "AK_reference"
};

/**
 * @author Unknown, Jurica Hlevnjak
 * @brief Function for DROP table, index, view, sequence, trigger, function, user, group and constraint.
 * @param type drop type
 * @param drop_arguments arguments of DROP command
 */
void AK_drop(int type, AK_drop_arguments *drop_arguments) {

    char *name;
    char *sys_table, *sys_tbl2, *sys_tbl3, *tbl;

    switch (type) {
        case DROP_TABLE:

            sys_table = "AK_relation";
            name = (char*) drop_arguments->value;

            int status = 0;
            int x = 0;

            if (AK_if_exist(name, sys_table) == 0) {
                printf("Table %s does not exist!\n", name);
            } else {
                for (x = 0; x < NUM_SYS_TABLES; x++) {
                    if (strcmp(name, system_catalog[x]) == 0) {
                        status = 1;
                        printf("Table %s is a System Catalog Table and can't be DROPPED!\n", name);
                        break;
                    }
                }
            }

            if ((!AK_if_exist(name, sys_table) == 0) && (status != 1)) {
                AK_drop_help_function(name, sys_table);
                printf("TABLE %s DROPPED!\n", name);
            }

            break;

        case DROP_INDEX:

            sys_table = "AK_index";
            name = (char*) drop_arguments->value;

            if (!AK_if_exist(name, sys_table) == 0) {
                AK_drop_help_function(name, sys_table);
                printf("INDEX %s DROPPED!\n", name);
            } else {
                printf("Index %s does not exist!\n", name);
            }

            break;

        case DROP_VIEW:

            sys_table = "AK_view";
            name = (char*) drop_arguments->value;

            if ((AK_table_empty(sys_table)) || (AK_if_exist(name, sys_table) == 0)) {
                printf("View %s does not exist!\n", name);
            } else {
                AK_view_remove_by_name(name);
                printf("VIEW %s DROPPED!\n", name);
            }

            break;

        case DROP_SEQUENCE:

            sys_table = "AK_sequence";
            name = (char*) drop_arguments->value;

            if ((AK_table_empty(sys_table)) || (AK_if_exist(name, sys_table) == 0)) {
                printf("Sequence %s does not exist!\n", name);
            } else {
                AK_sequence_remove(name);
                printf("SEQUENCE %s DROPPED!\n", name);
            }

            break;

        case DROP_TRIGGER:

            sys_table = "AK_trigger";
            sys_tbl2 = "AK_relation";

            name = (char*) drop_arguments->value;
            tbl = (char*) drop_arguments->next->value;

            if ((AK_table_empty(sys_table)) || (AK_if_exist(name, sys_table) == 0)) {
                printf("Trigger %s does not exist!\n", name);
            } else if ((AK_table_empty(sys_tbl2)) || (AK_if_exist(tbl, sys_tbl2) == 0)) {
                printf("Table %s does not exist!\n", tbl);
            } else {
                AK_trigger_remove_by_name(name, tbl);
                printf("TRIGGER %s DROPPED!\n", name);
            }

            break;

        case DROP_FUNCTION:

            sys_table = "AK_function";
            name = (char*) drop_arguments->value;

            AK_list *args = (AK_list *) malloc(sizeof (AK_list));
            Ak_Init_L(args);

            if (drop_arguments != NULL) {
                while (drop_arguments != NULL) {
                    // printf("Argument: %s\n", drop_arguments->value);
                    drop_arguments = drop_arguments->next;
                    if (drop_arguments != NULL) {
                        Ak_InsertAtEnd_L(TYPE_VARCHAR, drop_arguments->value, sizeof (drop_arguments), args);
                    }
                }
            }

            if ((AK_table_empty(sys_table)) || (AK_if_exist(name, sys_table) == 0)) {
                printf("Function %s does not exist!\n", name);
            } else {
                if (AK_function_remove_by_name(name, args) == EXIT_SUCCESS) {
                    printf("FUNCTION %s DROPPED!\n", name);
                } else {
                    printf("Function does not exist!\n"); // vraćen je EXIT_ERROR jer funkcija s danim argumentima nije pronađena
                }
            }

            break;

        case DROP_USER:

            sys_table = "AK_user";
            name = (char*) drop_arguments->value;

            int status1 = 0;

            if (strcmp(drop_arguments->next->value, "CASCADE") == 0) {
                status1 = 1;
            }

            if ((AK_table_empty(sys_table)) || (AK_if_exist(name, sys_table) == 0)) {
                printf("User %s does not exist!\n", name);
            } else {
                // if CASCADE
                if (status1 == 1) {
                    // revokes all user privileges
                    AK_revoke_all_privileges_user(name);
                    // remove user from group(s)
                    AK_remove_user_from_all_groups(name);

                    AK_user_remove_by_name(name);
                    printf("USER %s DROPPED!\n", name);

                } else { // if not CASCADE
                    // check if user has any privilege or belong to group
                    if (AK_check_user_privilege(name) == EXIT_SUCCESS) {
                        printf("User %s can not be dropped because it has related objects!\n", name);
                    } else {
                        AK_user_remove_by_name(name);
                        printf("USER %s DROPPED!\n", name);
                    }
                }
            }

            break;

        case DROP_GROUP:

            sys_table = "AK_group";
            name = (char*) drop_arguments->value;

            int status2 = 0;

            if (strcmp(drop_arguments->next->value, "CASCADE") == 0) {
                status2 = 1;
            }

            if ((AK_table_empty(sys_table)) || (AK_if_exist(name, sys_table) == 0)) {
                printf("Group %s does not exist!\n", name);
            } else {
                // if CASCADE
                if (status2 == 1) {
                    // revoke group privileges
                    AK_revoke_all_privileges_group(name);
                    // remove group users
                    AK_remove_all_users_from_group(name);

                    AK_group_remove_by_name(name);
                    printf("GROUP %s DROPPED!\n", name);
                } else { // if not CASCADE
                    // check if group has any privilege or have users
                    if (AK_check_group_privilege(name) == EXIT_SUCCESS) {
                        printf("Group %s can not be dropped because it has related objects!\n", name);
                    } else {
                        AK_group_remove_by_name(name);
                        printf("GROUP %s DROPPED!\n", name);
                    }
                }
            }

            break;

        case DROP_CONSTRAINT:

            // TODO - prije je potrebno popraviti greske u pojedinom constraintu

            sys_table = "AK_constraints_between";
            sys_tbl2 = "AK_constraints_not_null";
            sys_tbl3 = "AK_constraints_unique";

            name = (char*) drop_arguments->value;

            // TODO - preuzeti argumente

            int exist_between = 1, exist_null = 1, exist_unique = 1;

            if ((AK_table_empty(sys_table)) || (AK_if_exist(name, sys_table) == 0)) {
                exist_between = 0;
            }

            if ((AK_table_empty(sys_tbl2)) || (AK_if_exist(name, sys_tbl2) == 0)) {
                exist_null = 0;
            }

            if ((AK_table_empty(sys_tbl3)) || (AK_if_exist(name, sys_tbl3) == 0)) {
                exist_unique = 0;
            }


            if ((exist_between == 0) && (exist_null == 0) && (exist_unique == 0)) {
                printf("Constraint %s does not exist!\n", name);
            } else {
                if (exist_between == 1) {
                    // funkcija za brisanje between constrainta
                }
                if (exist_null == 1) {
                    // funkcija za brisanje not null constrainta
                }
                if (exist_unique == 1) {
                    // funkcija za brisanje unique constrainta
                }

                printf("CONSTRAINT %s DROPPED!\n", name);

            }

            break;

        default:
            break;
    }
}

/**
 * @author unknown, Jurica Hlevnjak - fix bugs and reorganize code in this function
 * @brief Help function for drop command. Delete memory blocks and addresses of table 
 * and removes table or index from system table.
 * @param tblName name of table or index
 * @param sys_table name of system catalog table
 */
void AK_drop_help_function(char *tblName, char *sys_table) {

    table_addresses *addresses;

    //addresses = AK_get_segment_addresses(tblName, SEGMENT_TYPE_TABLE);
    addresses = (table_addresses*) AK_get_table_addresses(tblName);

    AK_mem_block *mem_block;

    int from = 0, to = 0, i = 0, j = 0, c;

    for (j = 0; j < MAX_EXTENTS_IN_SEGMENT; j++) {
        if (addresses->address_from != 0) {
            from = addresses->address_from[j];
            to = addresses->address_to[j];

            if (from == 0 || to == 0) break;
            for (i = from; i <= to; i++) {
                mem_block = (AK_mem_block *) AK_get_block(i);
                mem_block->block->type = BLOCK_TYPE_FREE;

                for (c = 0; c < DATA_BLOCK_SIZE; c++) {
                    mem_block->block->tuple_dict[c].type = FREE_INT;
                    mem_block->block->data[c] = FREE_CHAR;
                }
            }
        } else break;
    }

    int data_adr = 0;
    int data_size = 0;
    //int data_type = 0;
    int address_sys;
    char name_sys[MAX_ATT_NAME];

    AK_mem_block *mem_block2 = (AK_mem_block *) AK_get_block(0);

    for (i = 0; i < DATA_BLOCK_SIZE; i++) {
        memset(name_sys, 0, MAX_ATT_NAME);

        if (mem_block2->block->tuple_dict[i].address == FREE_INT) {
            break;
        }

        data_adr = mem_block2->block->tuple_dict[i].address;
        data_size = mem_block2->block->tuple_dict[i].size;
        //data_type = mem_block2->block->tuple_dict[i].type;
        memcpy(name_sys, mem_block2->block->data + data_adr, data_size);

        i++;
        data_adr = mem_block2->block->tuple_dict[i].address;
        data_size = mem_block2->block->tuple_dict[i].size;
        //data_type = mem_block2->block->tuple_dict[i].type;
        memcpy(&address_sys, mem_block2->block->data + data_adr, data_size);

        if (strcmp(name_sys, sys_table) == 0) {
            break;
        }
    }

    mem_block2 = (AK_mem_block *) AK_get_block(address_sys);
    table_addresses *addresses2;

    //addresses2 = (table_addresses *) malloc(sizeof (table_addresses));
    addresses2 = (table_addresses*) AK_get_table_addresses(tblName);

    //memset(addresses2->address_from, 0, MAX_EXTENTS_IN_SEGMENT);
    //memset(addresses2->address_to, 0, MAX_EXTENTS_IN_SEGMENT);

    for (i = 0; i < MAX_EXTENTS_IN_SEGMENT; i++) {
        addresses2->address_from[i] = 0;
        addresses2->address_to[i] = 0;
    }

    char name[MAX_VARCHAR_LENGTH];

    for (i = 0; i < DATA_BLOCK_SIZE; i++) {
        if (mem_block2->block->tuple_dict[i].type == FREE_INT)
            break;
        i++;
        memcpy(name, &(mem_block2->block->data[mem_block2->block->tuple_dict[i].address]), mem_block2->block->tuple_dict[i].size);
        name[ mem_block2->block->tuple_dict[i].size] = '\0';
        if (strcmp(name, tblName) == 0) {
            i++;
            mem_block2->block->data[mem_block2->block->tuple_dict[i].address] = 0;
            i++;
            mem_block2->block->data[mem_block2->block->tuple_dict[i].address] = 0;
        }
    }

    AK_list_elem row_root = (AK_list_elem) malloc(sizeof (AK_list_elem));
    Ak_Init_L(row_root);
    Ak_DeleteAll_L(row_root);

    Ak_Insert_New_Element_For_Update(TYPE_VARCHAR, tblName, sys_table, "name", row_root, 1);

    Ak_delete_row(row_root);
}

/**
 * @author Jurica Hlevnjak
 * @brief Help function for check if element(view, function, sequence, user ...) exist in system catalog table
 * @param tblName name of table, index view, function, trigger, sequence, user, group or constraint
 * @param sys_table name of system catalog table
 * @return if element exist in system catalog returns 1, if not returns 0
 */
int AK_if_exist(char *tblName, char *sys_table) {
    int num_rows = AK_get_num_records(sys_table);
    int a;
    int exist = 0;

    for (a = 0; a < num_rows; a++) {
        AK_list_elem el;
        el = AK_get_tuple(a, 1, sys_table);
        if (!strcmp(tblName, el->data)) {
            exist = 1;
        }
    }
    return exist;
}

/**
 * @author unknown, Jurica Hlevnjak - added all tests except drop table test
 * @brief Function for testing all DROP functions
 */
void AK_drop_test() {
    printf("=========================================================\n");
    printf("========================DROP_TEST========================\n");
    AK_drop_arguments *drop_arguments = malloc(10 * sizeof (AK_drop_arguments));

    printf("\n-----DROP TABLE-----\n");
    AK_print_table("AK_relation");
    drop_arguments->value = "department";
    AK_drop(DROP_TABLE, drop_arguments);
    // AK_print_table("department");
    AK_print_table("AK_relation");

    printf("\n-----DROP CATALOG TABLE-----\n");
    drop_arguments->value = "AK_attribute";
    AK_drop(DROP_TABLE, drop_arguments);
    AK_print_table("AK_attribute");
    AK_print_table("AK_relation");

    printf("\n-----DROP VIEW-----\n");
    AK_print_table("AK_view");
    drop_arguments->value = "view300";
    AK_drop(DROP_VIEW, drop_arguments);
    AK_print_table("AK_view");

    printf("\n-----DROP HASH INDEX-----\n");
    drop_arguments->value = "student_hash_index";
    AK_print_table("AK_index");
    // AK_print_table("student_hash_index");
    AK_drop(DROP_INDEX, drop_arguments);
    // AK_print_table("student_hash_index");

    printf("\n-----DROP BITMAP INDEX-----\n");
    drop_arguments->value = "assistantfirstname_bmapIndex";
    AK_print_table("AK_index");
    // AK_print_table("assistantfirstname_bmapIndex");
    AK_drop(DROP_INDEX, drop_arguments);
    // AK_print_table("assistantfirstname_bmapIndex");
    AK_print_table("AK_index");

    printf("\n-----DROP SEQUENCE-----\n");
    drop_arguments->value = "sekvenca5";
    AK_print_table("AK_sequence");
    AK_drop(DROP_SEQUENCE, drop_arguments);
    AK_print_table("AK_sequence");
    free(drop_arguments);

    printf("\n-----DROP TRIGGER-----\n");
    AK_print_table("AK_trigger");
    AK_print_table("AK_trigger_conditions");
    drop_arguments->value = "trigg4";
    drop_arguments->next->value = "AK_reference";
    AK_drop(DROP_TRIGGER, drop_arguments);
    AK_print_table("AK_trigger");
    AK_print_table("AK_trigger_conditions");

    printf("\n-----DROP FUNCTION-----\n");
    AK_print_table("AK_function");
    AK_print_table("AK_function_arguments");
    drop_arguments->value = "test_funkcija";
    drop_arguments->next->value = "date";
    drop_arguments->next->next->value = "number";
    drop_arguments->next->next->next->value = "int";
    drop_arguments->next->next->next->next = NULL;
    AK_drop(DROP_FUNCTION, drop_arguments);
    AK_print_table("AK_function");
    AK_print_table("AK_function_arguments");

    printf("\n-----DROP USER-----\n");
    drop_arguments->value = "kritakac";
    drop_arguments->next->value = "CASCADE";
    AK_print_table("AK_user");
    AK_print_table("AK_user_group");
    AK_print_table("AK_user_right");
    AK_drop(DROP_USER, drop_arguments);
    AK_print_table("AK_user");
    AK_print_table("AK_user_group");
    AK_print_table("AK_user_right");

    // da se izbjegnu problemi prenosenja prethodne vrijednosti za drop group
    drop_arguments->next->value = "asdfghj"; // da ne ostane cascade

    printf("\n-----DROP GROUP-----\n");
    drop_arguments->value = "grupa1";
    //drop_arguments->next->value = "CASCADE";
    AK_print_table("AK_group");
    AK_print_table("AK_user_group");
    AK_print_table("AK_group_right");
    AK_drop(DROP_GROUP, drop_arguments);
    AK_print_table("AK_group");
    AK_print_table("AK_user_group");
    AK_print_table("AK_group_right");

    // printf("\n-----DROP CONSTRAINT-----\n");

    printf("======================END_DROP_TEST======================\n");
}
