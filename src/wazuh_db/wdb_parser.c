/*
 * Wazuh Database Daemon
 * Copyright (C) 2018 Wazuh Inc.
 * January 16, 2018.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "wdb.h"

int wdb_parse(char * input, char * output) {
    char * actor;
    char * id;
    char * query;
    char * sql;
    char * next;
    int agent_id;
    wdb_t * wdb;
    cJSON * data;
    char * out;
    int result = 0;

    // Clean string

    while (*input == ' ' || *input == '\n') {
        input++;
    }

    if (!*input) {
        mdebug1("Empty input query.");
        return -1;
    }

    if (next = strchr(input, ' '), !next) {
        mdebug1("Invalid DB query syntax.");
        mdebug2("DB query: %s", input);
        snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", input);
        return -1;
    }

    actor = input;
    *next++ = '\0';

    if (strcmp(actor, "agent") == 0) {
        id = next;

        if (next = strchr(id, ' '), !next) {
            mdebug1("Invalid DB query syntax.");
            mdebug2("DB query error near: %s", id);
            snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", id);
            return -1;
        }

        *next++ = '\0';
        query = next;

        if (agent_id = strtol(id, &next, 10), *next) {
            mdebug1("Invalid agent ID '%s'", id);
            snprintf(output, OS_MAXSTR + 1, "err Invalid agent ID '%.32s'", id);
            return -1;
        }

        if (wdb = wdb_open_agent2(agent_id), !wdb) {
            merror("Couldn't open DB for agent '%d'", agent_id);
            snprintf(output, OS_MAXSTR + 1, "err Couldn't open DB for agent %d", agent_id);
            return -1;
        }

        mdebug2("Executing query: %s", query);

        if (next = strchr(query, ' '), next) {
            *next++ = '\0';
        }

        if (strcmp(query, "syscheck") == 0) {
            if (!next) {
                mdebug1("Invalid Syscheck query syntax.");
                mdebug2("Syscheck query error near: %s", query);
                snprintf(output, OS_MAXSTR + 1, "err Invalid Syscheck query syntax, near '%.32s'", query);
                result = -1;
            } else {
                result = wdb_parse_syscheck(wdb, next, output);
            }
        } else if (strcmp(query, "osinfo") == 0) {
            if (!next) {
                mdebug1("Invalid DB query syntax.");
                mdebug2("DB query error near: %s", query);
                snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", query);
                result = -1;
            } else {
                if (wdb_parse_osinfo(wdb, next, output) == 0){
                    mdebug2("Stored OS information in DB for agent '%d'", agent_id);
                }
            }
        } else if (strcmp(query, "hardware") == 0) {
            if (!next) {
                mdebug1("Invalid DB query syntax.");
                mdebug2("DB query error near: %s", query);
                snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", query);
                result = -1;
            } else {
                if (wdb_parse_hardware(wdb, next, output) == 0){
                    mdebug2("Stored HW information in DB for agent '%d'", agent_id);
                }
            }
        } else if (strcmp(query, "port") == 0) {
            if (!next) {
                mdebug1("Invalid DB query syntax.");
                mdebug2("DB query error near: %s", query);
                snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", query);
                result = -1;
            } else {
                if (wdb_parse_ports(wdb, next, output) == 0){
                    mdebug2("Stored Port information in DB for agent '%d'", agent_id);
                }
            }
        } else if (strcmp(query, "program") == 0) {
            if (!next) {
                mdebug1("Invalid DB query syntax.");
                mdebug2("DB query error near: %s", query);
                snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", query);
                result = -1;
            } else {
                if (wdb_parse_programs(wdb, next, output) == 0){
                    mdebug2("Updated 'programs' table in DB for agent '%d'", agent_id);
                }
            }
        } else if (strcmp(query, "process") == 0) {
            if (!next) {
                mdebug1("Invalid DB query syntax.");
                mdebug2("DB query error near: %s", query);
                snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", query);
                result = -1;
            } else {
                if (wdb_parse_processes(wdb, next, output) == 0){
                    mdebug2("Stored process information in DB for agent '%d'", agent_id);
                }
            }
        } else if (strcmp(query, "sql") == 0) {
            if (!next) {
                mdebug1("Invalid DB query syntax.");
                mdebug2("DB query error near: %s", query);
                snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", query);
                result = -1;
            } else {
                sql = next;

                if (data = wdb_exec(wdb->db, sql), data) {
                    out = cJSON_PrintUnformatted(data);
                    snprintf(output, OS_MAXSTR + 1, "ok %s", out);
                    free(out);
                    cJSON_Delete(data);
                } else {
                    mdebug1("Cannot execute SQL query.");
                    mdebug2("SQL query: %s", sql);
                    snprintf(output, OS_MAXSTR + 1, "err Cannot execute SQL query");
                    result = -1;
                }
            }
        } else if (strcmp(query, "begin") == 0) {
            if (wdb_begin2(wdb) < 0) {
                mdebug1("Cannot begin transaction.");
                snprintf(output, OS_MAXSTR + 1, "err Cannot begin transaction");
                result = -1;
            } else {
                snprintf(output, OS_MAXSTR + 1, "ok");
            }
        } else if (strcmp(query, "commit") == 0) {
            if (wdb_commit2(wdb) < 0) {
                mdebug1("Cannot end transaction.");
                snprintf(output, OS_MAXSTR + 1, "err Cannot end transaction");
                result = -1;
            } else {
                snprintf(output, OS_MAXSTR + 1, "ok");
            }
        } else if (strcmp(query, "close") == 0) {
            wdb_leave(wdb);
            w_mutex_lock(&pool_mutex);

            if (wdb_close(wdb) < 0) {
                mdebug1("Cannot close database.");
                snprintf(output, OS_MAXSTR + 1, "err Cannot close database");
                result = -1;
            } else {
                snprintf(output, OS_MAXSTR + 1, "ok");
                result = 0;
            }

            w_mutex_unlock(&pool_mutex);
            return result;
        } else {
            mdebug1("Invalid DB query syntax.");
            mdebug2("DB query error near: %s", query);
            snprintf(output, OS_MAXSTR + 1, "err Invalid DB query syntax, near '%.32s'", query);
            result = -1;
        }
        wdb_leave(wdb);
        return result;
    } else {
        mdebug1("Invalid DB query actor: %s", actor);
        snprintf(output, OS_MAXSTR + 1, "err Invalid DB query actor: '%.32s'", actor);
        return -1;
    }
}

int wdb_parse_syscheck(wdb_t * wdb, char * input, char * output) {
    char * curr;
    char * next;
    int ftype;
    char * checksum;
    int result;
    char buffer[OS_MAXSTR + 1];

    if (next = strchr(input, ' '), !next) {
        mdebug1("Invalid Syscheck query syntax.");
        mdebug2("Syscheck query: %s", input);
        snprintf(output, OS_MAXSTR + 1, "err Invalid Syscheck query syntax, near '%.32s'", input);
        return -1;
    }

    curr = input;
    *next++ = '\0';

    if (strcmp(curr, "load") == 0) {
        if (result = wdb_syscheck_load(wdb, next, buffer, sizeof(buffer)), result < 0) {
            mdebug1("Cannot load Syscheck.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot load Syscheck");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok %s", buffer);
        }

        return result;
    } else if (strcmp(curr, "save") == 0) {
        curr = next;

        if (next = strchr(curr, ' '), !next) {
            mdebug1("Invalid Syscheck query syntax.");
            mdebug2("Syscheck query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Syscheck query syntax, near '%.32s'", curr);
            return -1;
        }

        *next++ = '\0';

        if (strcmp(curr, "file") == 0) {
            ftype = WDB_FILE_TYPE_FILE;
        } else if (strcmp(curr, "registry") == 0) {
            ftype = WDB_FILE_TYPE_REGISTRY;
        } else {
            mdebug1("Invalid Syscheck query syntax.");
            mdebug2("Syscheck query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Syscheck query syntax, near '%.32s'", curr);
            return -1;
        }

        checksum = next;

        if (next = strchr(checksum, ' '), !next) {
            mdebug1("Invalid Syscheck query syntax.");
            mdebug2("Syscheck query: %s", checksum);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Syscheck query syntax, near '%.32s'", checksum);
            return -1;
        }

        *next++ = '\0';

        if (result = wdb_syscheck_save(wdb, ftype, checksum, next), result < 0) {
            mdebug1("Cannot save Syscheck.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot save Syscheck");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;
    } else {
        mdebug1("Invalid Syscheck query syntax.");
        mdebug2("DB query error near: %s", curr);
        snprintf(output, OS_MAXSTR + 1, "err Invalid Syscheck query syntax, near '%.32s'", curr);
        return -1;
    }
}

int wdb_parse_osinfo(wdb_t * wdb, char * input, char * output) {
    char * curr;
    char * next;
    char * scan_id;
    char * scan_time;
    char * hostname;
    char * architecture;
    char * os_name;
    char * os_version;
    char * os_codename;
    char * os_major;
    char * os_minor;
    char * os_build;
    char * os_platform;
    char * sysname;
    char * release;
    char * version;
    int result;

    if (next = strchr(input, ' '), !next) {
        mdebug1("Invalid OS info query syntax.");
        mdebug2("OS info query: %s", input);
        snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", input);
        return -1;
    }

    curr = input;
    *next++ = '\0';

    if (strcmp(curr, "save") == 0) {
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_id = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_id, "NULL"))
            scan_id = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_time = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_time, "NULL"))
            scan_time = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", scan_time);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", scan_time);
            return -1;
        }

        hostname = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(hostname, "NULL"))
            hostname = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", hostname);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", hostname);
            return -1;
        }

        architecture = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(architecture, "NULL"))
            architecture = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", architecture);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", architecture);
            return -1;
        }

        os_name = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(os_name, "NULL"))
            os_name = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", os_name);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", os_name);
            return -1;
        }

        os_version = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(os_version, "NULL"))
            os_version = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", os_version);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", os_version);
            return -1;
        }

        os_codename = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(os_codename, "NULL"))
            os_codename = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", os_codename);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", os_codename);
            return -1;
        }

        os_major = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(os_major, "NULL"))
            os_major = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", os_major);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", os_major);
            return -1;
        }

        os_minor = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(os_minor, "NULL"))
            os_minor = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", os_minor);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", os_minor);
            return -1;
        }

        os_build = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(os_build, "NULL"))
            os_build = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", os_build);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", os_build);
            return -1;
        }

        os_platform = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(os_platform, "NULL"))
            os_platform = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", os_platform);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", os_platform);
            return -1;
        }

        sysname = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(sysname, "NULL"))
            sysname = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid OS info query syntax.");
            mdebug2("OS info query: %s", sysname);
            snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", sysname);
            return -1;
        }

        release = curr;
        *next++ = '\0';

        if (!strcmp(release, "NULL"))
            release = NULL;

        if (!strcmp(next, "NULL"))
            version = NULL;
        else
            version = next;

        if (result = wdb_osinfo_save(wdb, scan_id, scan_time, hostname, architecture, os_name, os_version, os_codename, os_major, os_minor, os_build, os_platform, sysname, release, version), result < 0) {
            mdebug1("Cannot save OS information.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot save OS information.");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;
    } else {
        mdebug1("Invalid OS info query syntax.");
        mdebug2("DB query error near: %s", curr);
        snprintf(output, OS_MAXSTR + 1, "err Invalid OS info query syntax, near '%.32s'", curr);
        return -1;
    }
}

int wdb_parse_hardware(wdb_t * wdb, char * input, char * output) {
    char * curr;
    char * next;
    char * scan_id;
    char * scan_time;
    char * serial;
    char * cpu_name;
    int cpu_cores;
    char * cpu_mhz;
    long ram_total;
    long ram_free;
    int result;

    if (next = strchr(input, ' '), !next) {
        mdebug1("Invalid HW info query syntax.");
        mdebug2("HW info query: %s", input);
        snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", input);
        return -1;
    }

    curr = input;
    *next++ = '\0';

    if (strcmp(curr, "save") == 0) {
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid HW info query syntax.");
            mdebug2("HW info query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_id = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_id, "NULL"))
            scan_id = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid HW info query syntax.");
            mdebug2("HW info query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_time = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_time, "NULL"))
            scan_time = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid HW info query syntax.");
            mdebug2("HW info query: %s", scan_time);
            snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", scan_time);
            return -1;
        }

        serial = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(serial, "NULL"))
            serial = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid HW info query syntax.");
            mdebug2("HW info query: %s", serial);
            snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", serial);
            return -1;
        }

        cpu_name = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(cpu_name, "NULL"))
            cpu_name = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid HW info query syntax.");
            mdebug2("HW info query: %s", cpu_name);
            snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", cpu_name);
            return -1;
        }

        cpu_cores = strtol(curr,NULL,10);
        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid HW info query syntax.");
            mdebug2("HW info query: %d", cpu_cores);
            snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", curr);
            return -1;
        }

        cpu_mhz = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(cpu_mhz, "NULL"))
            cpu_mhz = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid HW info query syntax.");
            mdebug2("HW info query: %s", cpu_mhz);
            snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", curr);
            return -1;
        }

        ram_total = strtol(curr,NULL,10);
        *next++ = '\0';
        ram_free = strtol(next,NULL,10);

        if (result = wdb_hardware_save(wdb, scan_id, scan_time, serial, cpu_name, cpu_cores, cpu_mhz, ram_total, ram_free), result < 0) {
            mdebug1("Cannot save HW information.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot save HW information.");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;
    } else {
        mdebug1("Invalid HW info query syntax.");
        mdebug2("DB query error near: %s", curr);
        snprintf(output, OS_MAXSTR + 1, "err Invalid HW info query syntax, near '%.32s'", curr);
        return -1;
    }
}

int wdb_parse_ports(wdb_t * wdb, char * input, char * output) {
    char * curr;
    char * next;
    char * scan_id;
    char * scan_time;
    char * protocol;
    char * local_ip;
    int local_port;
    char * remote_ip;
    int remote_port;
    int tx_queue;
    int rx_queue;
    int inode;
    char * state;
    int pid;
    char * process;
    int result;

    if (next = strchr(input, ' '), !next) {
        mdebug1("Invalid Port query syntax.");
        mdebug2("Port query: %s", input);
        snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", input);
        return -1;
    }

    curr = input;
    *next++ = '\0';

    if (strcmp(curr, "save") == 0) {
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_id = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_id, "NULL"))
            scan_id = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_time = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_time, "NULL"))
            scan_time = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %s", scan_time);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", scan_time);
            return -1;
        }

        protocol = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(protocol, "NULL"))
            protocol = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %s", protocol);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", protocol);
            return -1;
        }

        local_ip = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(local_ip, "NULL"))
            local_ip = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %s", local_ip);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", local_ip);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            local_port = -1;
        else
            local_port = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %d", local_port);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", curr);
            return -1;
        }

        remote_ip = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(remote_ip, "NULL"))
            remote_ip = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %s", remote_ip);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", remote_ip);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            remote_port = -1;
        else
            remote_port = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %d", remote_port);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            tx_queue = -1;
        else
            tx_queue = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %d", tx_queue);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            rx_queue = -1;
        else
            rx_queue = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %d", rx_queue);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            inode = -1;
        else
            inode = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %d", inode);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", curr);
            return -1;
        }

        state = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(state, "NULL"))
            state = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Port query syntax.");
            mdebug2("Port query: %s", state);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", state);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            pid = -1;
        else
            pid = strtol(curr,NULL,10);

        *next++ = '\0';
        if (!strncmp(next, "NULL", 4))
            process = NULL;
        else
            process = next;

        if (result = wdb_port_save(wdb, scan_id, scan_time, protocol, local_ip, local_port, remote_ip, remote_port, tx_queue, rx_queue, inode, state, pid, process), result < 0) {
            mdebug1("Cannot save Port information.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot save Port information.");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;
    } else if (strcmp(curr, "del") == 0) {

        curr = next;

        if (!strcmp(next, "NULL"))
            scan_id = NULL;
        else
            scan_id = next;

        if (result = wdb_port_delete(wdb, scan_id), result < 0) {
            mdebug1("Cannot delete old Port information.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot delete old Port information.");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;

    } else {
        mdebug1("Invalid Port query syntax.");
        mdebug2("DB query error near: %s", curr);
        snprintf(output, OS_MAXSTR + 1, "err Invalid Port query syntax, near '%.32s'", curr);
        return -1;
    }
}


int wdb_parse_programs(wdb_t * wdb, char * input, char * output) {
    char * curr;
    char * next;
    char * scan_id;
    char * scan_time;
    char * format;
    char * name;
    char * vendor;
    char * version;
    char * architecture;
    char * description;
    int result;

    if (next = strchr(input, ' '), !next) {
        mdebug1("Invalid Program info query syntax.");
        mdebug2("Program info query: %s", input);
        snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", input);
        return -1;
    }

    curr = input;
    *next++ = '\0';

    if (strcmp(curr, "save") == 0) {
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Program info query syntax.");
            mdebug2("Program info query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_id = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_id, "NULL"))
            scan_id = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Program info query syntax.");
            mdebug2("Program info query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_time = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_time, "NULL"))
            scan_time = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Program info query syntax.");
            mdebug2("Program info query: %s", scan_time);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", scan_time);
            return -1;
        }

        format = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(format, "NULL"))
            format = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Program info query syntax.");
            mdebug2("Program info query: %s", format);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", format);
            return -1;
        }

        name = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(name, "NULL"))
            name = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Program info query syntax.");
            mdebug2("Program info query: %s", name);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", name);
            return -1;
        }

        vendor = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(vendor, "NULL"))
            vendor = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Program info query syntax.");
            mdebug2("Program info query: %s", vendor);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", vendor);
            return -1;
        }

        version = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(version, "NULL"))
            version = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Program info query syntax.");
            mdebug2("Program info query: %s", version);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", version);
            return -1;
        }

        architecture = curr;
        *next++ = '\0';

        if (!strcmp(architecture, "NULL"))
            architecture = NULL;

        if (!strcmp(next, "NULL"))
            description = NULL;
        else
            description = next;

        if (result = wdb_program_save(wdb, scan_id, scan_time, format, name, vendor, version, architecture, description), result < 0) {
            mdebug1("Cannot save Program information.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot save Program information.");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;

    } else if (strcmp(curr, "del") == 0) {

        curr = next;

        if (!strcmp(next, "NULL"))
            scan_id = NULL;
        else
            scan_id = next;

        if (result = wdb_program_delete(wdb, scan_id), result < 0) {
            mdebug1("Cannot delete old Program information.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot delete old Program information.");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;

    } else {
        mdebug1("Invalid Program info query syntax.");
        mdebug2("DB query error near: %s", curr);
        snprintf(output, OS_MAXSTR + 1, "err Invalid Program info query syntax, near '%.32s'", curr);
        return -1;
    }
}

int wdb_parse_processes(wdb_t * wdb, char * input, char * output) {
    char * curr;
    char * next;
    char * scan_id;
    char * scan_time;
    int pid, ppid, utime, stime, priority, nice, size, vm_size, resident, share, start_time, pgrp, session, nlwp, tgid, tty, processor;
    char * name;
    char * state;
    char * cmd;
    char * argvs;
    char * euser;
    char * ruser;
    char * suser;
    char * egroup;
    char * rgroup;
    char * sgroup;
    char * fgroup;
    int result;

    if (next = strchr(input, ' '), !next) {
        mdebug1("Invalid Process query syntax.");
        mdebug2("Process query: %s", input);
        snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", input);
        return -1;
    }

    curr = input;
    *next++ = '\0';

    if (strcmp(curr, "save") == 0) {
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_id = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_id, "NULL"))
            scan_id = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", curr);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        scan_time = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(scan_time, "NULL"))
            scan_time = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", scan_time);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", scan_time);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            pid = -1;
        else
            pid = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", pid);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        name = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(name, "NULL"))
            name = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", name);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", name);
            return -1;
        }

        state = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(state, "NULL"))
            state = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", state);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", state);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            ppid = -1;
        else
            ppid = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", ppid);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            utime = -1;
        else
            utime = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", utime);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            stime = -1;
        else
            stime = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", stime);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        cmd = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(cmd, "NULL"))
            cmd = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", cmd);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", cmd);
            return -1;
        }

        argvs = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(argvs, "NULL"))
            argvs = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", argvs);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", argvs);
            return -1;
        }

        euser = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(euser, "NULL"))
            euser = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", euser);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", euser);
            return -1;
        }

        ruser = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(ruser, "NULL"))
            ruser = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", ruser);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", ruser);
            return -1;
        }

        suser = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(suser, "NULL"))
            suser = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", suser);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", suser);
            return -1;
        }

        egroup = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(egroup, "NULL"))
            egroup = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", egroup);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", egroup);
            return -1;
        }

        rgroup = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(rgroup, "NULL"))
            rgroup = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", rgroup);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", rgroup);
            return -1;
        }

        sgroup = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(sgroup, "NULL"))
            sgroup = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", sgroup);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", sgroup);
            return -1;
        }

        fgroup = curr;
        *next++ = '\0';
        curr = next;

        if (!strcmp(fgroup, "NULL"))
            fgroup = NULL;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %s", fgroup);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", fgroup);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            priority = -1;
        else
            priority = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", priority);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            nice = -1;
        else
            nice = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", nice);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            size = -1;
        else
            size = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", size);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            vm_size = -1;
        else
            vm_size = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", vm_size);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            resident = -1;
        else
            resident = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", resident);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            share = -1;
        else
            share = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", share);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            start_time = -1;
        else
            start_time = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", start_time);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            pgrp = -1;
        else
            pgrp = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", pgrp);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            session = -1;
        else
            session = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", session);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            nlwp = -1;
        else
            nlwp = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", nlwp);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            tgid = -1;
        else
            tgid = strtol(curr,NULL,10);

        *next++ = '\0';
        curr = next;

        if (next = strchr(curr, '|'), !next) {
            mdebug1("Invalid Process query syntax.");
            mdebug2("Process query: %d", tgid);
            snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
            return -1;
        }

        if (!strncmp(curr, "NULL", 4))
            tty = -1;
        else
            tty = strtol(curr,NULL,10);

        *next++ = '\0';
        if (!strncmp(next, "NULL", 4))
            processor = -1;
        else
            processor = strtol(next,NULL,10);

        if (result = wdb_process_save(wdb, scan_id, scan_time, pid, name, state, ppid, utime, stime, cmd, argvs, euser, ruser, suser, egroup, rgroup, sgroup, fgroup, priority, nice, size, vm_size, resident, share, start_time, pgrp, session, nlwp, tgid, tty, processor), result < 0) {
            mdebug1("Cannot save Process information.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot save Process information.");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;
    } else if (strcmp(curr, "del") == 0) {

        curr = next;

        if (!strcmp(next, "NULL"))
            scan_id = NULL;
        else
            scan_id = next;

        if (result = wdb_process_delete(wdb, scan_id), result < 0) {
            mdebug1("Cannot delete old Process information.");
            snprintf(output, OS_MAXSTR + 1, "err Cannot delete old Process information.");
        } else {
            snprintf(output, OS_MAXSTR + 1, "ok");
        }

        return result;

    } else {
        mdebug1("Invalid Process query syntax.");
        mdebug2("DB query error near: %s", curr);
        snprintf(output, OS_MAXSTR + 1, "err Invalid Process query syntax, near '%.32s'", curr);
        return -1;
    }
}
