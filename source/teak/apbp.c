// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom

#include <teak/apbp.h>

void apbpSendData(uint16_t id, uint16_t data)
{
    switch (id)
    {
        case 0:
            while (REG_APBP_STAT & APBP_STAT_REP0_UNREAD);
            REG_APBP_REP0 = data;
            break;
        case 1:
            while (REG_APBP_STAT & APBP_STAT_REP1_UNREAD);
            REG_APBP_REP1 = data;
            break;
        case 2:
            while (REG_APBP_STAT & APBP_STAT_REP2_UNREAD);
            REG_APBP_REP2 = data;
            break;
    }
}

uint16_t apbpReceiveData(uint16_t id)
{
    switch (id)
    {
        case 0:
            while ((REG_APBP_STAT & APBP_STAT_CMD0_NEW) == 0);
            return REG_APBP_CMD0;
        case 1:
            while ((REG_APBP_STAT & APBP_STAT_CMD1_NEW) == 0);
            return REG_APBP_CMD1;
        case 2:
            while ((REG_APBP_STAT & APBP_STAT_CMD2_NEW) == 0);
            return REG_APBP_CMD2;
    }

    return 0;
}
