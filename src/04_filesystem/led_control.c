/**
 * Copyright 2018 University of Applied Sciences Western Switzerland / Fribourg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project: HEIA-FR / HES-SO MSE - MA-CSEL1 Laboratory
 *
 * Abstract: System programming -  file system
 *
 * Purpose: NanoPi silly status led control system
 *
 * Autĥor:  Daniel Gachet
 * Date:    07.11.2018
 */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED      "/sys/class/gpio/gpio10"
#define LED           "10"

static int open_led()
{
    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // config pin
    f = open(GPIO_LED "/direction", O_WRONLY);
    write(f, "out", 3);
    close(f);

    // open gpio value attribute
    f = open(GPIO_LED "/value", O_RDWR);
    return f;
}

static int open_btn(const char* btn, const char* edge) {
    char path[256];

    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, btn, strlen(btn));
    close(f);

    // config pin direction
    sprintf(path, "/sys/class/gpio/gpio%s/direction", btn);
    f = open(path, O_WRONLY);
    write(f, "in", 2);
    close(f);

    // config pin edge
    sprintf(path, "/sys/class/gpio/gpio%s/edge", btn);
    f = open(path, O_WRONLY);
    write(f, edge, strlen(edge));
    close(f);

    // open gpio value attribute
    sprintf(path, "/sys/class/gpio/gpio%s/value", btn);
    f = open(path, O_RDONLY);
    return f;
}

int main(int argc, char* argv[])
{
    int retval;
    char dummybuf[8];
    long period = 1000;  // ms
    if (argc >= 2) period = atoi(argv[1]);
    period *= 1000000;  // in ns
    int led_state = 0;
    long init_period = period;

    // Open led file descriptor
    int led = open_led();
    // Turn led on
    pwrite(led, "1", sizeof("1"), 0);
    led_state = 1;
    printf("led on\n");

    // Open button file descriptors
    int k1 = open_btn("0", "falling");
    int k2 = open_btn("1", "falling");
    int k3 = open_btn("2", "falling");
    printf("buttons opened\n");

    // Create timer file descriptor
    int timer = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer == -1) {
        perror("timerfd_create");
        exit(EXIT_FAILURE);
    }

    // Configure timer
    struct itimerspec spec = {
        .it_interval = { .tv_sec = 0, .tv_nsec = period/2 },
        .it_value    = { .tv_sec = 0, .tv_nsec = period/2 }
    };
    timerfd_settime(timer, 0, &spec, NULL);
    printf("timer configured with period = %ld ns\n", period);

    // Create epoll file descriptor
    int epfd = epoll_create1(0);
    struct epoll_event ev, events[4];
    ev.events = EPOLLIN;
    ev.data.fd = timer;
    epoll_ctl(epfd, EPOLL_CTL_ADD, timer, &ev);
    /*ev.data.fd = k1;
    epoll_ctl(epfd, EPOLL_CTL_ADD, k1, &ev);
    ev.data.fd = k2;
    epoll_ctl(epfd, EPOLL_CTL_ADD, k2, &ev);
    ev.data.fd = k3;
    epoll_ctl(epfd, EPOLL_CTL_ADD, k3, &ev);*/
    printf("epoll created\n");


    while (1) {
        // Wait on epoll_wait() until input becomes available
        //printf("waiting for input\n");
        retval = epoll_wait(epfd, events, 4, -1);

        if (retval <= 0) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        //printf("input available\n");

        // Check which file descriptor is ready and handle it accordingly
        for (int i = 0; i < retval; i++) {
            if (events[i].data.fd == timer) {
                // Timer expired, toggle led
                read(timer, dummybuf, sizeof(dummybuf));
                pwrite(led, led_state ? "1" : "0", sizeof("1"), 0);
                printf("led %s\n", led_state ? "on" : "off");
                led_state = !led_state;
            }

            /*if (events[i].data.fd == k1) {
                // Button 1 pressed, increase frequency
                period /= 2;
                spec.it_interval.tv_nsec = period/2;
                spec.it_value.tv_nsec = period/2;
                timerfd_settime(timer, 0, &spec, NULL);
                printf("k1 pressed, period = %ld ns\n", period);
            }

            if (events[i].data.fd == k2) {
                // Button 2 pressed, reset frequency
                period = init_period;
                spec.it_interval.tv_nsec = period/2;
                spec.it_value.tv_nsec = period/2;
                timerfd_settime(timer, 0, &spec, NULL);
                printf("k2 pressed, period = %ld ns\n", period);
            }

            if (events[i].data.fd == k3) {
                // Button 3 pressed, decrease frequency
                period *= 2;
                spec.it_interval.tv_nsec = period/2;
                spec.it_value.tv_nsec = period/2;
                timerfd_settime(timer, 0, &spec, NULL);
                printf("k3 pressed, period = %ld ns\n", period);
            }*/
        }
    }

    return 0;
}
