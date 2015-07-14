/*
 * Alarm clock
 * Functionality:
 * - Setting an alarm with certain parameters
 * - Resetting an alarm
 * - Ring an alarm, resetting it
 * - Search for an alarm and change its parameters
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>

#include "atlas_api.h"
#include "atlas_alloc.h"

#define AC_TABLE_SIZE 32
#define AC_TABLE_MASK (AC_TABLE_SIZE-1)
#define AC_TABLE_ENTRY(hour, min) (AlarmClockTab + (((hour)+(min)) & AC_TABLE_MASK))
#define AC_LOCK(hour, min) (AlarmClockLockTab + (((hour)+(min)) & AC_TABLE_MASK))

typedef struct AlarmClockInfo
{
    uint8_t hour;
    uint8_t min;
    uint8_t mode;
    uint8_t repeat_factor;
    char    name[128];
    struct AlarmClockInfo *next;
}AlarmClockInfo;

AlarmClockInfo **AlarmClockTab;
pthread_mutex_t AlarmClockLockTab[AC_TABLE_SIZE];

int set_alarms = 0;
int update_alarms = 0;
int cancelled_alarms = 0;
int failed_cancel_alarms = 0;

uint32_t alarm_clock_rgn_id;

AlarmClockInfo *CreateNewInfo(uint8_t hour, uint8_t min, uint8_t mode,
                              uint8_t repeat_factor, char *name)
{
    AlarmClockInfo *ninfo =
        (AlarmClockInfo*) nvm_alloc(sizeof(AlarmClockInfo), alarm_clock_rgn_id);
    ninfo->hour = hour;
    ninfo->min = min;
    ninfo->mode = mode;
    ninfo->repeat_factor = repeat_factor;
    strcpy(ninfo->name, name);
    ninfo->next = NULL;
    return ninfo;
}

inline pthread_mutex_t *GetLock(uint8_t hour, uint8_t min)
{
    return AC_LOCK(hour, min);
}

inline AlarmClockInfo *GetHeader(uint8_t hour, uint8_t min)
{
    return *AC_TABLE_ENTRY(hour, min);
}

int add_or_update_alarm(uint8_t hour, uint8_t min, uint8_t mode,
                        uint8_t repeat_factor, char *name)
{
//    fprintf(stderr, "adding alarm %d %d\n", hour, min);
    
    pthread_mutex_t *bucket_mtx = GetLock(hour, min);
    pthread_mutex_lock(bucket_mtx);
    AlarmClockInfo *header = GetHeader(hour, min);
    AlarmClockInfo *temp = header;
    // Check whether an existing entry should be updated
    while (temp)
    {
        if (temp->hour == hour && temp->min == min)
        {
            temp->mode = mode;
            temp->repeat_factor = repeat_factor;
            strcpy(temp->name, name);
            pthread_mutex_unlock(bucket_mtx);
            return 0;
        }
        temp = temp->next;
    }
    // A new entry is inserted
    AlarmClockInfo *naci = CreateNewInfo(hour,min,mode,repeat_factor,name);
    naci->next = header;
    *AC_TABLE_ENTRY(hour,min) = naci;
    pthread_mutex_unlock(bucket_mtx);
    return 1;
}

// Return 1 if found and removed, otherwise return 0
int cancel_alarm(uint8_t hour, uint8_t min, int play)
{
//    fprintf(stderr, "canceling alarm %d %d\n", hour, min);
    
    pthread_mutex_t *bucket_mtx = GetLock(hour, min);
    pthread_mutex_lock(bucket_mtx);
    AlarmClockInfo *cand = GetHeader(hour, min);
    AlarmClockInfo *prev = NULL;
    while (cand)
    {
        if (cand->hour == hour && cand->min == min)
        {
            if (play) ; // sound an alarm
            if (!prev) *AC_TABLE_ENTRY(hour, min) = cand->next;
            else prev->next = cand->next;
            nvm_free(cand);
            pthread_mutex_unlock(bucket_mtx);
            return 1;
        }
        prev = cand;
        cand = cand->next;
    }
    pthread_mutex_unlock(bucket_mtx);
    return 0;
}

void print_alarms()
{
    int i;
    fprintf(stderr, "---------------\n");
    for (i=0; i < AC_TABLE_SIZE; ++i)
    {
        AlarmClockInfo *aci = AlarmClockTab[i];
        while (aci)
        {
            fprintf(stderr, "%d %d %d %d %s\n",
                    aci->hour, aci->min, aci->mode, aci->repeat_factor,
                    aci->name);
            aci = aci->next;
        }
    }
}

void *play_alarms()
{
//    pthread_cond_signal(&cond);
    
    int8_t hour = 23;
    int8_t min;
    int status;
    while (hour >= 0)
    {
        min = 59;
        while (min >= 0)
        {
            status = cancel_alarm(hour, min, 1);
            if (status) ++cancelled_alarms;
            else ++failed_cancel_alarms;
            min -= 1;
        }
        -- hour;
    }
    return NULL;
}

void initialize()
{
    void *rgn_root = NVM_GetRegionRoot(alarm_clock_rgn_id);
    if (rgn_root) AlarmClockTab = (AlarmClockInfo**)rgn_root;
    else AlarmClockTab = (AlarmClockInfo**)nvm_alloc(
        AC_TABLE_SIZE * sizeof(AlarmClockInfo*), alarm_clock_rgn_id);
}

int main(int argc, char *argv[])
{
    pthread_t th;

    NVM_Initialize();
    alarm_clock_rgn_id = NVM_FindOrCreateRegion("alarm_clock", O_RDWR, NULL);

    initialize();
    
    NVM_SetRegionRoot(alarm_clock_rgn_id, AlarmClockTab);
    
    pthread_create(&th, 0, (void *(*)(void*))play_alarms, 0);

//    pthread_mutex_lock(&cond_mtx);
//    pthread_cond_wait(&cond, &cond_mtx);
//    pthread_mutex_unlock(&cond_mtx);
    
    uint8_t hour = 0;
    uint8_t min;
    int ret;
    while (hour < 24)
    {
        min = 0;
        while (min < 60)
        {
            ret = add_or_update_alarm(hour, min, 0, 0, "");
            if (ret) ++ set_alarms;
            else ++ update_alarms;
            min += 1;
        }
        hour += 1;
    }
    pthread_join(th, NULL);

//    print_alarms();
    NVM_CloseRegion(alarm_clock_rgn_id);
    NVM_Finalize();
    
    fprintf(stderr, "Set = %d Updated = %d Cancelled = %d Failed canceling = %d\n",
            set_alarms, update_alarms, cancelled_alarms, failed_cancel_alarms);
    return 0;
}
