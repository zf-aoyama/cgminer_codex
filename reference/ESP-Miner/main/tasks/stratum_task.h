#ifndef STRATUM_TASK_H_
#define STRATUM_TASK_H_

typedef struct
{
    uint32_t stratum_difficulty;
} SystemTaskModule;

void stratum_task(void *pvParameters);
void stratum_close_connection(GlobalState * GLOBAL_STATE);

#endif