/*
 * Copyright (C) 2016 Adawarp Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "psvr.h"

static psvr_context *ctx;

static int command_set_power(uint32_t on)
{
    printf("Set Power %d\n", on);
    return psvr_send_command_sync(ctx, 0x17, (uint8_t *) &on, 4);
}

static int command_set_distance(uint32_t distance)
{
    printf("Set disance %d\n", distance);
    return psvr_send_command_sync(ctx, 0x21, (uint8_t *) &distance, 3);
}

static int recenter_vr()
{
    uint32_t payload[2];
    payload[0] = 0x1b00aa04;
    payload[1] = 0x00000000;
    printf("Recenter VR\n");
    return psvr_send_command_sync(ctx, 0x11, (uint8_t *) &payload, 8);
}

static int command_enable_vr_mode()
{
    uint32_t payload[2];
    payload[0] = 0xFFFFFF00;
    payload[1] = 0x00000000;
    printf("Enable VR Mode\n");
    return psvr_send_command_sync(ctx, 0x11, (uint8_t *) &payload, 8);
}

static void print_sensor_data(struct psvr_sensor_frame *frame)
{
    int i;
    printf("Button: Plus=%d, Minus=%d, Mute=%d\n", frame->button.plus, frame->button.minus, frame->button.mute);
    printf("Volume: %d\n", frame->volume);
    printf("Status: %x\n", frame->status.as_byte);
    for (i = 0; i < 2; i++) {
        printf("Frame#%d: gyro=%8d, %8d, %8d accel=%8d, %8d, %8d\n", i,
            frame->data[i].gyro.yaw, frame->data[i].gyro.pitch, frame->data[i].gyro.roll,
            frame->data[i].accel.x, frame->data[i].accel.y, frame->data[i].accel.z);
    }
}

int main(int argc, char *argv[])
{
    int r, i;
    struct psvr_sensor_frame frame;

    if ((r = psvr_open(&ctx)) < 0) {
        printf("Cannot open PSVR\n");
        return -1;
    }

    //command_set_power(1);

    for (i = 0; i < 1000; i++) command_enable_vr_mode();
    usleep(10 * 10000);

    /*
    for (i = 0; i < 1000; i++) {
        r = psvr_read_sensor_sync(ctx, (uint8_t *) &frame, sizeof (struct psvr_sensor_frame));
        print_sensor_data(&frame);
        usleep(10 * 1000);
    }
    */

    //if(strcmp(argv[2], "poweron")==0) command_set_power(1);
    //if(strcmp(argv[2], "poweroff")==0) command_set_power(0);
    //if(strcmp(argv[2], "dist-")==0)
    //command_set_distance(50);
    //if(strcmp(argv[2], "dist+")==0) command_set_distance(0);
    //if(strcmp(argv[2], "recenter")==0) recenter_vr();
    //if(strcmp(argv[2], "vrmode")==0) command_enable_vr_mode();

    psvr_close(ctx);

    return 0;
}
#ifdef __cplusplus
}
#endif
